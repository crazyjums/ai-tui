# Validation Notes (Working Log)

## 2026-08-21 — Interactive prototype smoke test

The Vite prototype loaded successfully at `http://localhost:4173/`. The initial frame rendered the expected workspace explorer, four tabs, primary Claude terminal pane, secondary Codex waiting-state pane, status bar, and the non-authoritative approval reminder. The Pane “More actions” control opened the Command Palette successfully, displaying the searchable action list, keyboard hints, and dimmed backdrop.

The prototype correctly labels its permission card as a visibility/focus aid and explicitly states that it never auto-approves commands. This is a functional UX smoke test only; it does **not** validate ConPTY, VT rendering, or Windows native UI because the sandbox host is Linux.

## 2026-08-21 — Prototype interaction checks

The Command Palette action **New Codex session** successfully created a fifth Tab (`Codex — new task`), selected it as the primary pane, set its status to Running, and incremented the status-bar Agent count from one to two. The original Codex waiting pane remained present, demonstrating that session creation does not overwrite an existing Agent session.

The main-window Settings control successfully opened the AI Agents configuration dialog. The dialog exposed the required navigation categories, four discoverable Agent rows, per-workspace behavior, permission dropdown affordances, `Never auto-approve commands`, history options including `Metadata only`, and waiting-notification controls.

## 2026-08-21 — Agent Dashboard check

The status-bar **Agents** control opened the Agent Dashboard. The dashboard grouped the prototype sessions by urgency: the Codex session waiting for command approval appears first, followed by the running Claude backend session and the newly created running Codex session. Each row includes project/cwd, normalized state, summary, duration and a Focus action. The dashboard footer accurately states that generic shells remain unclassified, matching the documented evidence model.
