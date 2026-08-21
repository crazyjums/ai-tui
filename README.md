# AI Terminal Windows UI

一个适合 AI 大模型的现代版终端命令行工具。

## Native MVP

`AI-Terminal.exe` is the minimal native Windows build. It opens a dark desktop window with one real local PowerShell session, an output area, a command input, and a Run button. It does not use a browser or a simulated terminal.

```powershell
.\AI-Terminal.exe
```

The MVP intentionally leaves tabs, SSH profiles, Agent context and WSL orchestration for later iterations.

This directory is the **production UI boundary**, designed for a C++20 / WinUI 3 Windows App SDK project. It is intentionally separate from the runnable design prototype in `../design-prototype`.

## Responsibility

The Windows UI owns the Fluent window, workspace/tab/pane chrome, DirectWrite/D3D terminal surface, IME, keyboard dispatch, tray, taskbar integration and Toast activation. It never reads ConPTY output directly. All terminal input, lifecycle calls, Agent state, and `FrameDelta` messages travel through the versioned named-pipe protocol in `../../proto/terminal.proto`.

## Windows build prerequisites

Install on a Windows 11 development machine:

1. Visual Studio 2022 or later with **Desktop development with C++**.
2. Windows App SDK / WinUI 3 C++ project tooling.
3. Windows 11 SDK (build 26100 or compatible).
4. Rust stable and the `apps/terminal-service` build prerequisites.
5. `protoc` or a configured protobuf code-generation step for `terminal.proto`.

## Current source status

`src/TerminalFramePresenter.*` is a compilable-style renderer boundary sample that describes dirty-cell rendering inputs. `src/ServiceConnection.*` specifies the native named-pipe client ownership pattern. A full WinUI project file must be generated on Windows because WinUI 3 packaging/SDK targets are not available in this Linux sandbox. The implementation is deliberately not substituted with an Electron shell.

## Acceptance path

The Windows implementation is ready to be connected in Phase 0 as follows:

```text
WinUI window → ServiceConnection → ai-terminal-service.exe
                             ↘ FrameDelta → TerminalFramePresenter (D3D/DirectWrite)
```

The cross-platform, interactive UX prototype is independently buildable now:

```powershell
cd apps/design-prototype
pnpm install
pnpm build
pnpm dev
```
