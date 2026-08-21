#define _UNICODE
#include <windows.h>
#include <string>
#include <thread>
#include <vector>

static constexpr int ID_OUTPUT = 1;
static constexpr int ID_INPUT = 2;
static constexpr int ID_RUN = 3;
static constexpr UINT WM_SHELL_TEXT = WM_APP + 1;
static constexpr UINT WM_SHELL_DONE = WM_APP + 2;

static HWND g_output = nullptr;
static HWND g_input = nullptr;
static HWND g_run = nullptr;
static HANDLE g_shell_input = nullptr;
static HANDLE g_shell_process = nullptr;
static WNDPROC g_input_proc = nullptr;
static HFONT g_ui_font = nullptr;
static HFONT g_mono_font = nullptr;

static constexpr COLORREF kBackground = RGB(8, 13, 23);
static constexpr COLORREF kPanel = RGB(13, 21, 33);
static constexpr COLORREF kInput = RGB(21, 32, 49);
static constexpr COLORREF kText = RGB(224, 231, 241);
static constexpr COLORREF kMuted = RGB(145, 160, 181);
static constexpr COLORREF kBlue = RGB(54, 137, 235);
static constexpr COLORREF kGreen = RGB(79, 211, 115);

static std::wstring from_console_bytes(const char* data, size_t size) {
    if (!size) return {};
    int length = MultiByteToWideChar(CP_OEMCP, 0, data, (int)size, nullptr, 0);
    std::wstring text(length, L'\0');
    MultiByteToWideChar(CP_OEMCP, 0, data, (int)size, text.data(), length);
    return text;
}

static std::string to_console_bytes(const std::wstring& text) {
    int length = WideCharToMultiByte(CP_OEMCP, 0, text.data(), (int)text.size(), nullptr, 0, nullptr, nullptr);
    std::string bytes(length, '\0');
    WideCharToMultiByte(CP_OEMCP, 0, text.data(), (int)text.size(), bytes.data(), length, nullptr, nullptr);
    return bytes;
}

static void append_text(const std::wstring& text) {
    if (!g_output || text.empty()) return;
    int end = GetWindowTextLengthW(g_output);
    SendMessageW(g_output, EM_SETSEL, end, end);
    SendMessageW(g_output, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
    SendMessageW(g_output, EM_SCROLL, SB_PAGEDOWN, 0);
}

static void shell_reader(HWND window, HANDLE output) {
    char buffer[4096];
    DWORD count = 0;
    while (ReadFile(output, buffer, sizeof(buffer), &count, nullptr) && count) {
        PostMessageW(window, WM_SHELL_TEXT, 0, (LPARAM)new std::wstring(from_console_bytes(buffer, count)));
    }
    CloseHandle(output);
    PostMessageW(window, WM_SHELL_DONE, 0, 0);
}

static bool start_shell(HWND window) {
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE output_read = nullptr, output_write = nullptr, input_read = nullptr;
    if (!CreatePipe(&output_read, &output_write, &security, 0)) return false;
    SetHandleInformation(output_read, HANDLE_FLAG_INHERIT, 0);
    if (!CreatePipe(&input_read, &g_shell_input, &security, 0)) return false;
    SetHandleInformation(g_shell_input, HANDLE_FLAG_INHERIT, 0);

    wchar_t root[MAX_PATH]{};
    GetEnvironmentVariableW(L"SystemRoot", root, MAX_PATH);
    std::wstring executable = std::wstring(root) + L"\\System32\\WindowsPowerShell\\v1.0\\powershell.exe";
    std::wstring command = L"\"" + executable + L"\" -NoLogo -NoProfile";
    std::vector<wchar_t> command_line(command.begin(), command.end());
    command_line.push_back(L'\0');

    STARTUPINFOW startup{sizeof(STARTUPINFOW)};
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input_read;
    startup.hStdOutput = output_write;
    startup.hStdError = output_write;
    PROCESS_INFORMATION process{};
    bool ok = CreateProcessW(executable.c_str(), command_line.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);
    CloseHandle(input_read);
    CloseHandle(output_write);
    if (!ok) {
        CloseHandle(output_read);
        CloseHandle(g_shell_input);
        g_shell_input = nullptr;
        return false;
    }
    CloseHandle(process.hThread);
    g_shell_process = process.hProcess;
    std::thread(shell_reader, window, output_read).detach();
    return true;
}

static void execute_command() {
    if (!g_shell_input) return;
    int length = GetWindowTextLengthW(g_input);
    if (!length) return;
    std::wstring command(length, L'\0');
    GetWindowTextW(g_input, command.data(), length + 1);
    command += L"\r\n";
    std::string bytes = to_console_bytes(command);
    DWORD written = 0;
    WriteFile(g_shell_input, bytes.data(), (DWORD)bytes.size(), &written, nullptr);
    SetWindowTextW(g_input, L"");
}

static LRESULT CALLBACK input_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_KEYDOWN && wparam == VK_RETURN) {
        execute_command();
        return 0;
    }
    return CallWindowProcW(g_input_proc, window, message, wparam, lparam);
}

static void text(HDC dc, int x, int y, const wchar_t* value, COLORREF color, int size, bool bold = false) {
    HFONT font = CreateFontW(-size, 0, 0, 0, bold ? FW_SEMIBOLD : FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT old = (HFONT)SelectObject(dc, font);
    SetTextColor(dc, color); SetBkMode(dc, TRANSPARENT);
    TextOutW(dc, x, y, value, lstrlenW(value));
    SelectObject(dc, old); DeleteObject(font);
}

static void solid(HDC dc, RECT rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color); FillRect(dc, &rect, brush); DeleteObject(brush);
}

static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE: {
        g_ui_font = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        g_mono_font = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, L"Cascadia Mono");
        g_output = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
            34, 126, 900, 450, window, (HMENU)ID_OUTPUT, GetModuleHandleW(nullptr), nullptr);
        g_input = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            34, 600, 780, 38, window, (HMENU)ID_INPUT, GetModuleHandleW(nullptr), nullptr);
        g_run = CreateWindowExW(0, L"BUTTON", L"Run", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            830, 600, 90, 38, window, (HMENU)ID_RUN, GetModuleHandleW(nullptr), nullptr);
        SendMessageW(g_output, WM_SETFONT, (WPARAM)g_mono_font, TRUE);
        SendMessageW(g_input, WM_SETFONT, (WPARAM)g_mono_font, TRUE);
        SendMessageW(g_run, WM_SETFONT, (WPARAM)g_ui_font, TRUE);
        g_input_proc = (WNDPROC)SetWindowLongPtrW(g_input, GWLP_WNDPROC, (LONG_PTR)input_proc);
        append_text(L"AI Terminal MVP\r\n\r\nReal local PowerShell session. Type a command below and press Enter.\r\n\r\n");
        if (!start_shell(window)) append_text(L"Could not start PowerShell.\r\n");
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wparam) == ID_RUN) execute_command();
        return 0;
    case WM_SHELL_TEXT: {
        auto* value = (std::wstring*)lparam;
        if (value) { append_text(*value); delete value; }
        return 0;
    }
    case WM_SHELL_DONE:
        append_text(L"\r\n[PowerShell exited]\r\n");
        g_shell_input = nullptr;
        return 0;
    case WM_DRAWITEM: {
        auto* item = (DRAWITEMSTRUCT*)lparam;
        if (item->CtlID == ID_RUN) {
            solid(item->hDC, item->rcItem, kBlue);
            SetTextColor(item->hDC, RGB(255, 255, 255)); SetBkMode(item->hDC, TRANSPARENT);
            DrawTextW(item->hDC, L"Run", -1, &item->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            return TRUE;
        }
        break;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: {
        HDC dc = (HDC)wparam; HWND control = (HWND)lparam;
        SetTextColor(dc, kText); SetBkColor(dc, control == g_input ? kInput : kPanel);
        return (LRESULT)CreateSolidBrush(control == g_input ? kInput : kPanel);
    }
    case WM_PAINT: {
        PAINTSTRUCT paint{}; HDC dc = BeginPaint(window, &paint); RECT client{}; GetClientRect(window, &client);
        solid(dc, client, kBackground);
        RECT header{0, 0, client.right, 74}; solid(dc, header, RGB(7, 12, 21));
        text(dc, 28, 19, L">_  AI Terminal MVP", kText, 21, true);
        text(dc, 30, 47, L"A real local shell in a focused native window", kMuted, 12);
        text(dc, 34, 92, L"〉  PowerShell · Local", kText, 16, true);
        text(dc, client.right - 190, 96, L"●  CONNECTED", kGreen, 12, true);
        text(dc, 34, client.bottom - 32, L"PowerShell  ·  local machine  ·  Enter to run", kMuted, 12);
        EndPaint(window, &paint); return 0;
    }
    case WM_SIZE: {
        int width = LOWORD(lparam), height = HIWORD(lparam);
        MoveWindow(g_output, 34, 126, width - 68, height - 235, TRUE);
        MoveWindow(g_input, 34, height - 88, width - 170, 38, TRUE);
        MoveWindow(g_run, width - 124, height - 88, 90, 38, TRUE);
        InvalidateRect(window, nullptr, FALSE); return 0;
    }
    case WM_DESTROY:
        if (g_shell_process) TerminateProcess(g_shell_process, 0);
        if (g_shell_input) CloseHandle(g_shell_input);
        PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    WNDCLASSW klass{}; klass.hInstance = instance; klass.lpfnWndProc = window_proc; klass.lpszClassName = L"AITerminalMvpWindow";
    klass.hCursor = LoadCursorW(nullptr, IDC_ARROW); klass.hbrBackground = CreateSolidBrush(kBackground);
    RegisterClassW(&klass);
    HWND window = CreateWindowExW(0, klass.lpszClassName, L"AI Terminal MVP", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1080, 720, nullptr, nullptr, instance, nullptr);
    ShowWindow(window, show); UpdateWindow(window);
    MSG message{}; while (GetMessageW(&message, nullptr, 0, 0) > 0) { TranslateMessage(&message); DispatchMessageW(&message); }
    return (int)message.wParam;
}
