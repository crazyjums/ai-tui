# AI Terminal Validation Report

**Date:** 2026-08-21  
**Environment:** Ubuntu 24.04 sandbox, Node 22.13, pnpm 11.21, Rust/Cargo 1.75.  
**Scope:** Runnable interaction prototype, service-domain scaffold, documents and visual assets. This report does not claim Windows-native ConPTY/WinUI validation from a Linux host.

## Summary

The deliverable contains a buildable interactive prototype and a testable Rust terminal-service scaffold. The design prototype loaded in a browser and its key interaction paths were exercised manually. The Rust service compiled, passed unit tests and executed a diagnostic/state-simulation command. The native Windows UI/ConPTY implementation is represented by production-boundary source code and an IPC schema, but cannot be compiled or integration-tested in this sandbox because the required Windows App SDK, WinUI 3, DirectWrite/D3D and ConPTY runtime do not exist here.

| Area | Result | Evidence |
| --- | --- | --- |
| Prototype TypeScript compilation | Pass | `pnpm build` completed with Vite production bundle output. |
| Prototype primary layout | Pass | Browser smoke test rendered Workspace Explorer, Tabs, dual panes, state bar and waiting-state card. |
| Command Palette | Pass | Pane action opened Command Palette; `New Codex session` created/selects a new running Tab. |
| Settings UI | Pass | AI Agents screen rendered agent discovery rows, privacy policy and history/notification controls. |
| Agent Dashboard | Pass | Dashboard sorted waiting state before running sessions and provided Focus actions. |
| Rust domain tests | Pass | `cargo test`: 3 passed, 0 failed. |
| Agent evidence safety | Pass | `cargo run -- simulate-agent` kept a generic session Running for weak output pattern, then accepted structured hook evidence. |
| Windows ConPTY | Not run | The host OS is Linux; implementation requires Windows 10 1809+ and a Windows build/test pipeline. |
| WinUI/DirectWrite renderer | Not run | Requires Windows App SDK and Visual Studio C++ tooling. |
| Windows installer | Not produced | Requires a Windows signing/packaging environment and a real native UI binary. |

## Commands executed

```text
cd apps/design-prototype && pnpm install
cd apps/design-prototype && pnpm build
cd apps/terminal-service && cargo test
cd apps/terminal-service && cargo run -- doctor
cd apps/terminal-service && cargo run -- simulate-agent
```

The final `pnpm build` passed. `cargo test` passed all three tests. The Rust compiler emits expected scaffold-stage dead-code warnings because future lifecycle, layout, history-policy and ConPTY paths are deliberately represented before the Windows-specific runner is wired; there were no test failures.

## UI interaction evidence

| Interaction | Observed outcome | Screenshot |
| --- | --- | --- |
| Main Workspace | Claude Running + Codex Waiting dual-pane layout rendered; no claim of real PTY. | `assets/prototype-main.webp` |
| Command Palette | Palette opened with fuzzy-action results and keyboard hints. | `assets/prototype-command-palette.webp` |
| New Agent | Selecting New Codex created `Codex — new task`, selected it, and increased running count. | Captured during smoke test; state recorded in `VALIDATION_NOTES.md`. |
| Settings | AI Agents page rendered with Privacy & Safety options. | `assets/prototype-settings.webp` |
| Agent Dashboard | Waiting Codex row sorted first, running Claude/Codex rows followed. | `assets/prototype-agent-dashboard.webp` |

## Requirements coverage and honest boundaries

The following items are delivered as **research/design/implemented scaffold** rather than falsely represented as fully working Windows terminal behavior: ConPTY process creation, VT parser integration, DirectWrite glyph rasterization, named-pipe protobuf runtime, Windows Credential Manager, real SSH/WSL profiles, real Claude/Codex adapters, Toast, Tray, installer, stress test with 100 MB stdout and Windows native E2E tests.

The validated deliverable deliberately avoids the anti-pattern prohibited in the specification: it does not claim that a React `div` reading `cmd.exe` stdout is a production terminal. The React component is isolated as a functional **UX prototype**; `apps/terminal-service`, `proto/terminal.proto`, and `apps/windows-ui` define the production path in which the real terminal renderer and PTY data plane remain native and separate from the UI state tree.

## Next Windows validation gate

On a Windows 11 development machine, Phase 0 should compile `ai-terminal-service.exe`, generate the IPC bindings, instantiate the WinUI 3 shell, and then verify this exact sequence:

1. Create ConPTY with synchronous input/output pipes at a 120×36 character grid.
2. Launch PowerShell using `STARTUPINFOEX` and the pseudoconsole attribute.
3. Send `Get-ChildItem`, parse VT output and display FrameDelta in the DirectWrite Surface.
4. Resize the Pane; verify `ResizePseudoConsole` and child screen dimensions.
5. Run CMD, WSL, SSH, Python and vim as regression cases.
6. Execute the output-drain close sequence on Windows 10 and Windows 11 24H2 to rule out old-version deadlocks.
7. Run high-output stress tests and record input latency, dropped-frame policy and memory usage.

This gate must pass before marketing the project as an installable Windows terminal or shipping an `.msi`/`.exe` installer.
