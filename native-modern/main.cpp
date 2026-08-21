#define _UNICODE
#include <windows.h>
#include <richedit.h>
#include <string>
#include <thread>
#include <vector>

static constexpr int ID_OUTPUT = 1001;
static constexpr int ID_INPUT = 1002;
static constexpr int ID_RUN = 1003;
static constexpr int ID_TAB = 1004;
static constexpr UINT WM_SHELL_OUTPUT = WM_APP + 1;

static HINSTANCE g_instance;
static HWND g_output;
static HWND g_input;
static HWND g_run;
static HWND g_status;
static HANDLE g_shell_in = nullptr;
static HANDLE g_shell_process = nullptr;
static HFONT g_ui_font;
static HFONT g_mono_font;
static WNDPROC g_input_proc;

static COLORREF const BG = RGB(8, 14, 23);
static COLORREF const PANEL = RGB(12, 19, 30);
static COLORREF const PANEL_2 = RGB(17, 26, 39);
static COLORREF const BORDER = RGB(40, 54, 73);
static COLORREF const TEXT = RGB(218, 226, 238);
static COLORREF const MUTED = RGB(142, 158, 179);
static COLORREF const BLUE = RGB(55, 139, 239);
static COLORREF const GREEN = RGB(77, 207, 111);
static COLORREF const AMBER = RGB(242, 177, 39);

static std::wstring utf8_to_wide(const std::string& value) {
    if (value.empty()) return {};
    int size = MultiByteToWideChar(CP_OEMCP, 0, value.data(), (int)value.size(), nullptr, 0);
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_OEMCP, 0, value.data(), (int)value.size(), result.data(), size);
    return result;
}

static std::string wide_to_oem(const std::wstring& value) {
    if (value.empty()) return {};
    int size = WideCharToMultiByte(CP_OEMCP, 0, value.data(), (int)value.size(), nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    WideCharToMultiByte(CP_OEMCP, 0, value.data(), (int)value.size(), result.data(), size, nullptr, nullptr);
    return result;
}

static void append_output(const std::wstring& text) {
    if (!g_output || text.empty()) return;
    int length = GetWindowTextLengthW(g_output);
    SendMessageW(g_output, EM_SETSEL, length, length);
    SendMessageW(g_output, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
    SendMessageW(g_output, EM_SCROLL, SB_PAGEDOWN, 0);
}

static void shell_reader(HANDLE output, HWND window) {
    char buffer[4096];
    DWORD read = 0;
    while (ReadFile(output, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
        auto* text = new std::wstring(utf8_to_wide(std::string(buffer, buffer + read)));
        PostMessageW(window, WM_SHELL_OUTPUT, 0, (LPARAM)text);
    }
    CloseHandle(output);
    PostMessageW(window, WM_SHELL_OUTPUT, 1, 0);
}

static bool start_shell(HWND window) {
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE output_read = nullptr, output_write = nullptr;
    HANDLE input_read = nullptr;
    if (!CreatePipe(&output_read, &output_write, &security, 0)) return false;
    SetHandleInformation(output_read, HANDLE_FLAG_INHERIT, 0);
    if (!CreatePipe(&input_read, &g_shell_in, &security, 0)) return false;
    SetHandleInformation(g_shell_in, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW startup{sizeof(STARTUPINFOW)};
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input_read;
    startup.hStdOutput = output_write;
    startup.hStdError = output_write;
    PROCESS_INFORMATION process{};
    wchar_t system_root[MAX_PATH]{};
    GetEnvironmentVariableW(L"SystemRoot", system_root, MAX_PATH);
    std::wstring shell_path = std::wstring(system_root) + L"\\System32\\WindowsPowerShell\\v1.0\\powershell.exe";
    std::wstring command = L"\"" + shell_path + L"\" -NoLogo -NoProfile";
    std::vector<wchar_t> command_line(command.begin(), command.end());
    command_line.push_back(L'\0');
    bool created = CreateProcessW(shell_path.c_str(), command_line.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);
    CloseHandle(input_read);
    CloseHandle(output_write);
    if (!created) {
        CloseHandle(output_read);
        CloseHandle(g_shell_in);
        g_shell_in = nullptr;
        return false;
    }
    CloseHandle(process.hThread);
    g_shell_process = process.hProcess;
    std::thread(shell_reader, output_read, window).detach();
    return true;
}

static void send_command() {
    if (!g_shell_in) return;
    int length = GetWindowTextLengthW(g_input);
    if (length == 0) return;
    std::wstring command(length, L'\0');
    GetWindowTextW(g_input, command.data(), length + 1);
    command += L"\r\n";
    std::string bytes = wide_to_oem(command);
    DWORD written = 0;
    WriteFile(g_shell_in, bytes.data(), (DWORD)bytes.size(), &written, nullptr);
    SetWindowTextW(g_input, L"");
}

static LRESULT CALLBACK input_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_KEYDOWN && wparam == VK_RETURN) {
        send_command();
        return 0;
    }
    return CallWindowProcW(g_input_proc, window, message, wparam, lparam);
}

static void paint_text(HDC dc, int x, int y, const wchar_t* text, COLORREF color, int size = 14, bool bold = false) {
    HFONT font = CreateFontW(-size, 0, 0, 0, bold ? FW_SEMIBOLD : FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT old = (HFONT)SelectObject(dc, font);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    TextOutW(dc, x, y, text, lstrlenW(text));
    SelectObject(dc, old);
    DeleteObject(font);
}

static void fill_rect(HDC dc, RECT rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
}

static void draw_shell_chrome(HDC dc, RECT client) {
    fill_rect(dc, client, BG);
    RECT title{0, 0, client.right, 68};
    fill_rect(dc, title, RGB(7, 13, 22));
    RECT sidebar{0, 68, 224, client.bottom - 42};
    fill_rect(dc, sidebar, RGB(11, 18, 29));
    RECT top_tabs{224, 68, client.right, 122};
    fill_rect(dc, top_tabs, RGB(9, 16, 26));
    RECT footer{0, client.bottom - 42, client.right, client.bottom};
    fill_rect(dc, footer, RGB(12, 20, 32));
    HPEN pen = CreatePen(PS_SOLID, 1, BORDER);
    HPEN old = (HPEN)SelectObject(dc, pen);
    MoveToEx(dc, 0, 67, nullptr); LineTo(dc, client.right, 67);
    MoveToEx(dc, 0, client.bottom - 42, nullptr); LineTo(dc, client.right, client.bottom - 42);
    MoveToEx(dc, 223, 68, nullptr); LineTo(dc, 223, client.bottom - 42);
    SelectObject(dc, old); DeleteObject(pen);

    paint_text(dc, 27, 21, L">_  AI Terminal", TEXT, 21, true);
    paint_text(dc, 28, 91, L"▣   AI-CICD", TEXT, 16, true);
    paint_text(dc, 22, 145, L"⌄  WORKSPACES", MUTED, 12, true);
    paint_text(dc, 34, 184, L"◈  AI-CICD", TEXT, 15, true);
    paint_text(dc, 58, 208, L"2 sessions", MUTED, 12);
    paint_text(dc, 34, 253, L"▤  Backend", TEXT, 15);
    paint_text(dc, 58, 277, L"1 session", MUTED, 12);
    paint_text(dc, 34, 322, L"▧  Frontend", TEXT, 15);
    paint_text(dc, 58, 346, L"1 session", MUTED, 12);
    paint_text(dc, 34, 391, L"☁  Production", TEXT, 15);
    paint_text(dc, 24, client.bottom - 78, L"⊕  New workspace", MUTED, 13);

    paint_text(dc, 251, 87, L"〉  Claude — backend", TEXT, 15, true);
    paint_text(dc, 470, 87, L"●", GREEN, 14);
    paint_text(dc, 520, 87, L"〉  Codex — frontend", TEXT, 15);
    paint_text(dc, 746, 87, L"●", AMBER, 14);
    paint_text(dc, 790, 87, L"PowerShell", TEXT, 15);
    paint_text(dc, 930, 87, L"SSH prod", TEXT, 15);
    paint_text(dc, client.right - 110, 87, L"+     ⚙", TEXT, 21);
    paint_text(dc, 245, client.bottom - 28, L"◉  Agents     ●  2 running", MUTED, 12);
    paint_text(dc, 520, client.bottom - 28, L"⌁  main     ↑2 ↓1     M3  +12  −4", MUTED, 12);
    paint_text(dc, client.right - 300, client.bottom - 28, L"▣  PowerShell 7     CPU 7%", MUTED, 12);
}

static void style_control(HWND window, COLORREF background, COLORREF foreground) {
    SendMessageW(window, WM_SETFONT, (WPARAM)g_ui_font, TRUE);
    SetPropW(window, L"bg", (HANDLE)(ULONG_PTR)background);
    SetPropW(window, L"fg", (HANDLE)(ULONG_PTR)foreground);
}

static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE: {
        LoadLibraryW(L"Msftedit.dll");
        g_output = CreateWindowExW(WS_EX_CLIENTEDGE, MSFTEDIT_CLASS, L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
            245, 160, 500, 400, window, (HMENU)ID_OUTPUT, g_instance, nullptr);
        g_input = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            260, 600, 430, 34, window, (HMENU)ID_INPUT, g_instance, nullptr);
        g_run = CreateWindowExW(0, L"BUTTON", L"Run", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            700, 600, 70, 34, window, (HMENU)ID_RUN, g_instance, nullptr);
        g_status = CreateWindowExW(0, L"STATIC", L"●  CONNECTING TO POWERSHELL...", WS_CHILD | WS_VISIBLE,
            260, 132, 500, 24, window, (HMENU)ID_TAB, g_instance, nullptr);
        g_ui_font = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        g_mono_font = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, L"Cascadia Mono");
        SendMessageW(g_output, WM_SETFONT, (WPARAM)g_mono_font, TRUE);
        SendMessageW(g_input, WM_SETFONT, (WPARAM)g_mono_font, TRUE);
        style_control(g_run, PANEL_2, TEXT); style_control(g_status, BG, MUTED);
        g_input_proc = (WNDPROC)SetWindowLongPtrW(g_input, GWLP_WNDPROC, (LONG_PTR)input_proc);
        append_output(L"AI Terminal native session\r\nConnected to a real PowerShell process.\r\n\r\n");
        if (start_shell(window)) SetWindowTextW(g_status, L"●  CONNECTED  ·  PowerShell");
        else SetWindowTextW(g_status, L"●  FAILED TO START POWERSHELL");
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wparam) == ID_RUN) send_command();
        return 0;
    case WM_SHELL_OUTPUT: {
        if (wparam == 1) { SetWindowTextW(g_status, L"●  SHELL EXITED"); return 0; }
        auto* output = (std::wstring*)lparam;
        if (output) { append_output(*output); delete output; }
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: {
        HDC dc = (HDC)wparam;
        HWND control = (HWND)lparam;
        COLORREF bg = (COLORREF)(ULONG_PTR)GetPropW(control, L"bg");
        COLORREF fg = (COLORREF)(ULONG_PTR)GetPropW(control, L"fg");
        if (control == g_output) { bg = PANEL; fg = TEXT; }
        if (control == g_input) { bg = RGB(20, 29, 43); fg = TEXT; }
        SetBkColor(dc, bg ? bg : BG); SetTextColor(dc, fg ? fg : TEXT);
        return (LRESULT)CreateSolidBrush(bg ? bg : BG);
    }
    case WM_PAINT: {
        PAINTSTRUCT paint{}; HDC dc = BeginPaint(window, &paint);
        RECT client{}; GetClientRect(window, &client); draw_shell_chrome(dc, client);
        RECT leftPane{240, 119, client.right / 2 - 8, client.bottom - 55};
        RECT rightPane{client.right / 2 + 8, 119, client.right - 18, client.bottom - 55};
        HPEN pen = CreatePen(PS_SOLID, 1, BLUE); HPEN old = (HPEN)SelectObject(dc, pen);
        SelectObject(dc, GetStockObject(HOLLOW_BRUSH)); Rectangle(dc, leftPane.left, leftPane.top, leftPane.right, leftPane.bottom);
        SelectObject(dc, CreatePen(PS_SOLID, 1, BORDER)); Rectangle(dc, rightPane.left, rightPane.top, rightPane.right, rightPane.bottom);
        SelectObject(dc, old); DeleteObject(pen);
        paint_text(dc, leftPane.left + 18, leftPane.top + 15, L"〉  PowerShell · Local", TEXT, 15, true);
        paint_text(dc, leftPane.right - 145, leftPane.top + 17, L"CONNECTED  ·  00:12", GREEN, 11, true);
        paint_text(dc, rightPane.left + 18, rightPane.top + 15, L"〉  Agent Context", TEXT, 15, true);
        paint_text(dc, rightPane.left + 18, rightPane.top + 65, L"LOCAL TERMINAL", MUTED, 11, true);
        paint_text(dc, rightPane.left + 18, rightPane.top + 100, L"PowerShell session is ready", TEXT, 15, true);
        paint_text(dc, rightPane.left + 18, rightPane.top + 137, L"Run commands, SSH hosts, or WSL", MUTED, 13);
        paint_text(dc, rightPane.left + 18, rightPane.top + 185, L"Security", MUTED, 11, true);
        paint_text(dc, rightPane.left + 18, rightPane.top + 215, L"Commands run only on this machine.", TEXT, 13);
        paint_text(dc, rightPane.left + 18, rightPane.top + 244, L"No command is auto-approved by AI.", TEXT, 13);
        EndPaint(window, &paint); return 0;
    }
    case WM_SIZE: {
        int width = LOWORD(lparam), height = HIWORD(lparam);
        int split = width / 2;
        MoveWindow(g_output, 260, 205, split - 280, height - 315, TRUE);
        MoveWindow(g_status, 260, 145, split - 280, 24, TRUE);
        MoveWindow(g_input, 260, height - 92, split - 360, 34, TRUE);
        MoveWindow(g_run, split - 90, height - 92, 70, 34, TRUE);
        InvalidateRect(window, nullptr, FALSE); return 0;
    }
    case WM_DESTROY:
        if (g_shell_process) TerminateProcess(g_shell_process, 0);
        if (g_shell_in) CloseHandle(g_shell_in);
        PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    g_instance = instance;
    WNDCLASSW klass{}; klass.hInstance = instance; klass.lpfnWndProc = window_proc; klass.lpszClassName = L"AITerminalModernWindow";
    klass.hCursor = LoadCursorW(nullptr, IDC_ARROW); klass.hbrBackground = CreateSolidBrush(BG);
    RegisterClassW(&klass);
    HWND window = CreateWindowExW(0, klass.lpszClassName, L"AI Terminal", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1420, 860, nullptr, nullptr, instance, nullptr);
    ShowWindow(window, show); UpdateWindow(window);
    MSG message{}; while (GetMessageW(&message, nullptr, 0, 0) > 0) { TranslateMessage(&message); DispatchMessageW(&message); }
    return (int)message.wParam;
}
