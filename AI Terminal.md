# AI Terminal

> A Windows-first, Terminal First workspace for AI Coding agents, shells, WSL and SSH.

AI Terminal is deliberately split into a **production architecture** and a **runnable interaction prototype**. The production path is C++20/WinUI 3 + DirectWrite/D3D for the native Windows shell and renderer, with a separate Rust Terminal Service for ConPTY, VT state, session lifecycle and Agent adapters. The interactive prototype validates the Workspace, Tabs, Panes, Command Palette, Dashboard and Settings experience without pretending to be a terminal emulator.

## Repository map

| Path | Purpose |
| --- | --- |
| `docs/PRODUCT_SPEC.md` | Product positioning, personas, MVP scope and roadmap. |
| `docs/RESEARCH.md` | Current terminal/AI CLI research and citations. |
| `docs/ARCHITECTURE.md` | Technology decision, ConPTY, renderer, IPC, security, data model and tests. |
| `docs/UI_UX_SPEC.md` | Implementable UI specification and high-fidelity mockups. |
| `apps/design-prototype` | Runnable React/Vite UI interaction prototype. It does **not** launch a shell. |
| `apps/terminal-service` | Runnable Rust service scaffold; contains terminal/session/agent domain tests and Windows ConPTY boundary. |
| `apps/windows-ui` | C++20/WinUI 3 + DirectWrite production UI implementation boundary for a Windows machine. |
| `proto/terminal.proto` | Versioned named-pipe IPC schema between UI and service. |

## Quick start: interactive prototype

The prototype is the immediately runnable deliverable. It is intentionally labelled as a **design prototype**: terminal output and Agent states are simulated in the UI, while the production terminal data path lives in the separately documented Windows implementation.

```bash
cd apps/design-prototype
pnpm install
pnpm build
pnpm dev
```

Open the URL printed by Vite. Try these interactions:

| Interaction | How to test |
| --- | --- |
| Command Palette | Press `Ctrl+Shift+P`; fuzzy-search “Claude”, “Split”, “Settings”, or “Dashboard”. |
| Quick Launch | Press `Ctrl+K`; choose a profile or Agent action. |
| Tabs | Click a session tab or create a new one with the `+` button. |
| Pane keyboard handling | Press `Alt+Shift+→` or `Alt+Shift+↓` to see split feedback. |
| Agent Dashboard | Click **Agents** in the status bar and focus an agent. |
| Settings | Click the gear or press `Ctrl+,`; change privacy and notification controls. |
| Waiting state | Click **1 needs attention**, then focus the Codex pane. |

## Terminal service scaffold

The service currently compiles and runs its domain-model diagnostic on any host. On Windows, `src/conpty.rs` contains the isolated ConPTY FFI boundary; the Phase 0 implementation must connect it to synchronous pipes, `STARTUPINFOEX`, VT parsing, Job Objects and the versioned named-pipe transport.

```bash
cd apps/terminal-service
cargo test
cargo run -- doctor
cargo run -- simulate-agent
```

The `simulate-agent` command demonstrates an important security rule: a weak stdout pattern cannot promote a generic live terminal into `WaitingPermission`; a structured Hook/Protocol signal can.

## Production Windows build path

The native UI cannot be compiled in this Linux sandbox because WinUI 3/MSBuild/Windows App SDK targets are Windows-only. On Windows 11, install Visual Studio C++ desktop tooling, Windows App SDK/WinUI 3, a Windows SDK, Rust stable, and protobuf tooling. Then create/open the WinUI 3 C++ project rooted at `apps/windows-ui`, generate bindings from `proto/terminal.proto`, and build `apps/terminal-service` as `ai-terminal-service.exe`.

| Component | Technology | Ownership |
| --- | --- | --- |
| `AI Terminal.exe` | C++20 + WinUI 3 + DirectWrite/D3D | Window chrome, pane layout, terminal frame rendering, IME, Toast, Tray, Taskbar. |
| `ai-terminal-service.exe` | Rust | ConPTY, session lifecycle, VT core, scrollback, workspace state, Agent adapters, policy. |
| IPC | Named Pipe + protobuf | Commands and viewport-only `FrameDelta`; never raw stdout into UI state. |

See `apps/windows-ui/README.md` for the native project boundary and acceptance path.

## Architecture principles

1. **Terminal First.** The primary surface is a real terminal emulator, not a chat tool or editor.
2. **Two planes.** VT/ConPTY is the durable data plane; Agent state, Workspace and Git are optional control-plane metadata.
3. **No auto-approval.** Waiting state is visible and actionable, but the underlying Agent CLI remains authoritative for permission decisions.
4. **Process isolation.** UI, session service and future plugin host have separate fault domains.
5. **Privacy by default.** History defaults to `MetadataOnly`; secrets belong in Windows Credential Manager, never JSON/logs.
6. **Performance by design.** UI receives coalesced viewport deltas, never per-byte/per-line React or XAML updates.

## Configuration

The proposed Windows configuration root is `%APPDATA%\AITerminal\`.

```text
config.json          Global appearance, startup, privacy and notification preferences
profiles.json        Shell, WSL, SSH and Agent profiles; no secret values
keybindings.json     User keyboard overrides
themes\              JSON themes
workspaces\          Portable workspace snapshots
state.db             SQLite metadata and search index
plugins\             Manifests and isolated bundles
logs\                Redacted diagnostics only
```

Passwords, private-key passphrases, OAuth tokens and Agent secrets are referenced through Windows Credential Manager/DPAPI and are never placed in `profiles.json` or `state.db`.

## Keyboard defaults

| Shortcut | Command |
| --- | --- |
| `Ctrl+Shift+T` | New Tab |
| `Ctrl+Shift+W` | Close active Tab/Pane |
| `Ctrl+Tab` / `Ctrl+Shift+Tab` | Next/Previous Tab |
| `Ctrl+Shift+P` | Command Palette |
| `Ctrl+K` | Quick Launch |
| `Alt+Shift+→` | Split Right |
| `Alt+Shift+↓` | Split Down |
| `Alt+Arrow` | Focus directional pane |
| `Alt+Shift+Enter` | Toggle Pane Zoom |
| `Ctrl+F` | Search scrollback |
| `Ctrl+,` | Settings |
| `Ctrl+Shift+E` | Toggle Workspace Explorer |

## Plugin model

The first release supports built-in adapters only. The planned third-party model uses a separate Plugin Host with signed manifests and capability grants. Plugin classes include `AgentAdapter`, `StatusProvider`, `CommandProvider`, `NotificationProvider`, and `TerminalDecorationProvider`. Default capabilities exclude filesystem access, network, secrets, process creation, clipboard and PTY input.

## Troubleshooting

| Symptom | Action |
| --- | --- |
| `pnpm build` fails in the prototype | Run `pnpm install` in `apps/design-prototype`, then rerun build. Node 22+ is recommended. |
| Rust service cannot find Cargo | Install Rust stable (`rustup` on Windows; package manager on Linux/macOS). |
| No ConPTY on a development machine | Use Windows 10 version 1809 or later; run the native service only on Windows. |
| Agent state seems generic | This is safe fallback behavior. Install only a reviewed, explicit adapter/hook; output pattern matching alone must not claim approval state. |
| Workspace does not autostart | Check Workspace trust. Untrusted workspaces must never execute startup commands without explicit user approval. |
| High-output session feels slow | Collect redacted diagnostics, inspect frame queue saturation and scrollback limits; do not route raw VT output through UI control trees. |

## Verification performed in this environment

```text
✓ pnpm build                         # React/TypeScript prototype
✓ cargo test                         # 3 Rust domain tests
✓ cargo run -- doctor                # service diagnostic
✓ cargo run -- simulate-agent        # agent evidence safety flow
```

## License note

This scaffold is intended to be released under `MIT OR Apache-2.0`. Any future reuse of Alacritty, Windows Terminal, WezTerm, xterm.js, Tabby or other dependencies must be pinned and audited, with third-party notices and an SBOM generated before distribution.
