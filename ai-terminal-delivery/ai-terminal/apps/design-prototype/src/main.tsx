import { useEffect, useMemo, useState } from "react";
import type { FormEvent } from "react";
import { createRoot } from "react-dom/client";
import {
  Bell,
  Bot,
  Check,
  ChevronDown,
  ChevronRight,
  CircleDot,
  Cloud,
  Code2,
  Command,
  Cpu,
  Folder,
  GitBranch,
  HardDrive,
  Keyboard,
  LayoutPanelTop,
  MemoryStick,
  MonitorCog,
  MoreHorizontal,
  PanelLeft,
  Plus,
  Search,
  Server,
  Settings,
  ShieldCheck,
  SplitSquareHorizontal,
  TerminalSquare,
  WandSparkles,
  X,
  Zap,
} from "lucide-react";
import type { AgentStatus, PaletteCommand, Session, SessionKind, Workspace } from "./types";
import "./styles.css";

const workspaces: Workspace[] = [
  { id: "ai-cicd", name: "AI-CICD", sessionCount: 2, color: "green" },
  { id: "backend", name: "Backend", sessionCount: 1, color: "blue" },
  { id: "frontend", name: "Frontend", sessionCount: 1, color: "amber" },
  { id: "production", name: "Production", sessionCount: 1, color: "slate" },
];

const initialSessions: Session[] = [
  {
    id: "claude-backend",
    title: "Claude — backend",
    kind: "claude",
    status: "running",
    project: "backend",
    cwd: "D:\\codes\\ai-cicd\\backend",
    duration: "03:42",
    detail: "Editing src/service/user.go",
    output: [
      "claude[backend]$ /agent",
      "Claude Code v1.0.35",
      "Model: claude-sonnet-20240620",
      "Workspace: D:\\codes\\ai-cicd\\backend",
      "",
      "→ Plan updated (3 steps)",
      "1. Add user service validation             [x]",
      "2. Implement update logic                  [x]",
      "3. Add unit tests                          [~]",
      "",
      "• Reading src/service/user.go",
      "• Searching for updates...",
      "• Applying edit to src/service/user.go",
      "",
      "@@ -42,7 +42,17 @@ func (s *Service) Update(ctx context.Context,",
      "-  if req.Email == \"\" {",
      "-      return ErrInvalidEmail",
      "+  if req.Email != \"\" {",
      "+      if err := validateEmail(req.Email); err != nil {",
      "+          return nil, fmt.Errorf(\"invalid email: %s\", req.Email)",
      "+      }",
      "+      user.Email = strings.ToLower(req.Email)",
      "+  }",
      "",
      "✓ File updated successfully",
      "• Running gofmt...",
      "• Running go test ./...",
      "",
      "claude[backend]$",
    ],
  },
  {
    id: "codex-frontend",
    title: "Codex — frontend",
    kind: "codex",
    status: "waiting",
    project: "frontend",
    cwd: "D:\\codes\\ai-cicd\\frontend",
    duration: "01:18",
    detail: "Waiting for command approval",
    output: ["codex[frontend]$", "Ready for your approval"],
  },
  {
    id: "powershell",
    title: "PowerShell",
    kind: "shell",
    status: "idle",
    project: "AI-CICD",
    cwd: "D:\\codes\\ai-cicd",
    duration: "00:12",
    output: ["PowerShell 7.5.0", "PS D:\\codes\\ai-cicd> Get-ChildItem", "src  tests  package.json", "PS D:\\codes\\ai-cicd>"],
  },
  {
    id: "ssh-prod",
    title: "SSH prod",
    kind: "ssh",
    status: "idle",
    project: "Production",
    cwd: "/srv/api",
    duration: "08:07",
    output: ["Connected to prod via SSH", "ubuntu@prod:/srv/api$ systemctl status api", "● api.service — active (running)", "ubuntu@prod:/srv/api$"],
  },
];

const paletteCommands: PaletteCommand[] = [
  { id: "new-claude", title: "New Claude Code session", subtitle: "Start Claude in current project", keywords: "claude agent ai new", key: "Enter", action: "new-claude" },
  { id: "new-codex", title: "New Codex session", subtitle: "Start Codex in current project", keywords: "codex agent ai new", action: "new-codex" },
  { id: "split", title: "Split pane right", subtitle: "Split the active terminal pane to the right", keywords: "split pane layout right", action: "split" },
  { id: "workspace", title: "Open workspace: AI-CICD", subtitle: "Restore the saved AI-CICD environment", keywords: "workspace ai cicd open", action: "workspace" },
  { id: "ssh", title: "Connect SSH: prod", subtitle: "Open a new SSH session to production", keywords: "ssh prod remote server", action: "ssh" },
  { id: "settings", title: "Open Settings", subtitle: "Configure AI Terminal", keywords: "settings configure preferences", action: "settings" },
  { id: "dashboard", title: "Open Agent Dashboard", subtitle: "Review running and waiting agents", keywords: "agent dashboard status", action: "dashboard" },
];

function iconFor(kind: SessionKind, size = 16) {
  const props = { size, strokeWidth: 1.8 };
  if (kind === "claude" || kind === "codex") return <Bot {...props} />;
  if (kind === "ssh") return <Server {...props} />;
  if (kind === "wsl") return <TerminalSquare {...props} />;
  return <TerminalSquare {...props} />;
}

function statusLabel(status: AgentStatus) {
  const labels: Record<AgentStatus, string> = {
    running: "RUNNING",
    waiting: "WAITING FOR YOU",
    completed: "COMPLETED",
    failed: "FAILED",
    idle: "IDLE",
  };
  return labels[status];
}

function App() {
  const [sessions, setSessions] = useState<Session[]>(initialSessions);
  const [activeId, setActiveId] = useState("powershell");
  const [secondaryId, setSecondaryId] = useState("codex-frontend");
  const [activeWorkspace, setActiveWorkspace] = useState("AI-CICD");
  const [explorerOpen, setExplorerOpen] = useState(true);
  const [paletteOpen, setPaletteOpen] = useState(false);
  const [paletteQuery, setPaletteQuery] = useState("");
  const [settingsOpen, setSettingsOpen] = useState(false);
  const [dashboardOpen, setDashboardOpen] = useState(false);
  const [toast, setToast] = useState("Claude is editing src/service/user.go");
  const [noticeOpen, setNoticeOpen] = useState(false);

  const activeSession = sessions.find((session) => session.id === activeId) ?? sessions[0];
  const secondarySession = sessions.find((session) => session.id === secondaryId) ?? sessions[1];
  const filteredCommands = useMemo(() => {
    const query = paletteQuery.replace(/^>\s*/, "").trim().toLowerCase();
    if (!query) return paletteCommands;
    return paletteCommands.filter((command) => `${command.title} ${command.subtitle} ${command.keywords}`.toLowerCase().includes(query));
  }, [paletteQuery]);

  useEffect(() => {
    const onKeyDown = (event: KeyboardEvent) => {
      if (event.key === "Escape") {
        setPaletteOpen(false);
        setSettingsOpen(false);
        setDashboardOpen(false);
        setNoticeOpen(false);
      }
      if (event.ctrlKey && event.shiftKey && event.key.toLowerCase() === "p") {
        event.preventDefault();
        setPaletteOpen(true);
        setPaletteQuery(">");
      }
      if (event.ctrlKey && event.key.toLowerCase() === "k") {
        event.preventDefault();
        setPaletteOpen(true);
        setPaletteQuery("");
      }
      if (event.ctrlKey && event.key === ",") {
        event.preventDefault();
        setSettingsOpen(true);
      }
      if (event.altKey && event.shiftKey && ["ArrowRight", "ArrowDown"].includes(event.key)) {
        event.preventDefault();
        setToast(event.key === "ArrowRight" ? "Pane split to the right" : "Pane split below");
      }
    };
    window.addEventListener("keydown", onKeyDown);
    return () => window.removeEventListener("keydown", onKeyDown);
  }, []);

  function addSession(kind: "claude" | "codex" | "ssh" | "shell") {
    const id = `${kind}-${Date.now()}`;
    const newSession: Session = {
      id,
      title: kind === "claude" ? "Claude — new task" : kind === "codex" ? "Codex — new task" : kind === "ssh" ? "SSH prod" : "PowerShell · Local",
      kind,
      status: kind === "ssh" ? "idle" : "running",
      project: kind === "ssh" ? "Production" : "AI-CICD",
      cwd: kind === "ssh" ? "/srv/api" : "D:\\googledownload\\ai-tui\\ai-terminal-delivery\\ai-terminal\\apps\\design-prototype",
      duration: "00:00",
      detail: kind === "ssh" ? "Connected profile" : kind === "shell" ? "Real local PowerShell session" : "Starting agent session",
      output: [kind === "ssh" ? "ubuntu@prod:/srv/api$" : `${kind}[ai-cicd]$ Starting session...`],
    };
    setSessions((previous) => [...previous, newSession]);
    setActiveId(id);
    setToast(`${newSession.title} created`);
  }

  function handleCommand(command: PaletteCommand) {
    setPaletteOpen(false);
    if (command.action === "new-claude") addSession("claude");
    if (command.action === "new-codex") addSession("codex");
    if (command.action === "ssh") addSession("ssh");
    if (command.action === "split") setToast("Pane split to the right — prototype layout preserved");
    if (command.action === "workspace") {
      setActiveWorkspace("AI-CICD");
      setToast("AI-CICD workspace restored");
    }
    if (command.action === "settings") setSettingsOpen(true);
    if (command.action === "dashboard") setDashboardOpen(true);
  }

  function focusSession(session: Session, target: "primary" | "secondary") {
    if (target === "primary") setActiveId(session.id);
    if (target === "secondary") setSecondaryId(session.id);
    setDashboardOpen(false);
    setToast(`Focused ${session.title}`);
  }

  function markApprovalHandled() {
    setSessions((previous) => previous.map((session) => session.id === "codex-frontend" ? { ...session, status: "running", detail: "Approval acknowledged in original CLI" } : session));
    setToast("Approval status updated — continue in the original Codex terminal");
  }

  const runningCount = sessions.filter((session) => session.status === "running").length;
  const waitingSessions = sessions.filter((session) => session.status === "waiting");

  return (
    <div className="app-shell">
      <header className="window-titlebar">
        <div className="brand"><TerminalSquare size={18} /><span>AI Terminal</span></div>
        <div className="window-controls"><span>—</span><span>□</span><span>×</span></div>
      </header>

      <section className="tabbar">
        <button className="workspace-switcher" onClick={() => setExplorerOpen((value) => !value)}>
          <Folder size={17} /><span>{activeWorkspace}</span><ChevronDown size={15} />
        </button>
        <div className="tabs" role="tablist" aria-label="Terminal sessions">
          {sessions.map((session) => (
            <button className={`tab ${activeId === session.id ? "active" : ""}`} key={session.id} onClick={() => setActiveId(session.id)}>
              {iconFor(session.kind, 15)}<span className="tab-title">{session.title}</span><i className={`status-dot ${session.status}`} />
            </button>
          ))}
        </div>
        <button className="chrome-button" title="New terminal" onClick={() => addSession("shell")}><Plus size={19} /></button>
        <button className="chrome-button" title="Settings" onClick={() => setSettingsOpen(true)}><Settings size={18} /></button>
      </section>

      <main className="workspace-shell">
        {explorerOpen && <aside className="explorer">
          <div className="explorer-heading"><ChevronDown size={15} /> <span>WORKSPACES</span></div>
          <div className="workspace-list">
            {workspaces.map((workspace) => <button key={workspace.id} className={`workspace-item ${workspace.name === activeWorkspace ? "selected" : ""}`} onClick={() => { setActiveWorkspace(workspace.name); setToast(`${workspace.name} selected`); }}>
              <span className={`workspace-symbol ${workspace.color}`}>{workspace.name === "AI-CICD" ? <Code2 size={17} /> : workspace.name === "Production" ? <Cloud size={17} /> : <HardDrive size={17} />}</span>
              <span><strong>{workspace.name}</strong><small>{workspace.sessionCount} session{workspace.sessionCount > 1 ? "s" : ""}</small></span><i className={`status-dot ${workspace.color === "green" ? "running" : workspace.color === "amber" ? "waiting" : "idle"}`} />
            </button>)}
          </div>
          <div className="explorer-actions"><button onClick={() => setToast("New workspace flow opened")}><Plus size={16} /> New workspace</button><button aria-label="Collapse explorer" onClick={() => setExplorerOpen(false)}><ChevronRight size={17} /></button></div>
        </aside>}

        <section className="terminal-workspace">
          <TerminalPane session={activeSession} active onFocus={() => setActiveId(activeSession.id)} onPalette={() => { setPaletteOpen(true); setPaletteQuery(">"); }} />
          <TerminalPane session={secondarySession} onFocus={() => setSecondaryId(secondarySession.id)} onPalette={() => setPaletteOpen(true)} onApproval={markApprovalHandled} />
        </section>
      </main>

      <footer className="statusbar">
        <button onClick={() => setDashboardOpen(true)}><WandSparkles size={15} /><span>Agents</span><i className="status-dot running" /> {runningCount} running</button>
        <button><GitBranch size={15} /><span>main</span><span className="status-number cool">↑2 ↓1</span><span className="status-number">M3</span><span className="status-number positive">+12</span><span className="status-number negative">−4</span></button>
        <button><Folder size={15} />D:\codes\ai-cicd</button>
        <button><TerminalSquare size={15} />PowerShell 7</button>
        <button><Cpu size={15} />CPU&nbsp; 7%</button>
        <button><MemoryStick size={15} />Memory&nbsp; 112 MB</button>
      </footer>

      {toast && <div className="toast" role="status"><CircleDot size={15} /><span>{toast}</span><button onClick={() => setToast("")}><X size={14} /></button></div>}
      {waitingSessions.length > 0 && <button className="attention-chip" onClick={() => setNoticeOpen((value) => !value)}><Bell size={15} />{waitingSessions.length} needs attention</button>}
      {noticeOpen && <div className="notification-popover"><strong>Needs attention</strong>{waitingSessions.map((session) => <button key={session.id} onClick={() => focusSession(session, "secondary")}><span className="status-dot waiting" />{session.title}<small>{session.detail}</small></button>)}</div>}

      {paletteOpen && <CommandPalette query={paletteQuery} commands={filteredCommands} onQuery={setPaletteQuery} onClose={() => setPaletteOpen(false)} onSelect={handleCommand} />}
      {dashboardOpen && <AgentDashboard sessions={sessions} onClose={() => setDashboardOpen(false)} onFocus={focusSession} />}
      {settingsOpen && <SettingsDialog onClose={() => setSettingsOpen(false)} />}
    </div>
  );
}

function TerminalPane({ session, active = false, onFocus, onPalette, onApproval }: { session: Session; active?: boolean; onFocus: () => void; onPalette: () => void; onApproval?: () => void }) {
  if (session.kind === "shell") {
    return <RealTerminalPane session={session} active={active} onFocus={onFocus} onPalette={onPalette} />;
  }
  return <section className={`terminal-pane ${active ? "active" : ""}`} onMouseDown={onFocus}>
    <header className="pane-header">
      <span className="pane-title">{iconFor(session.kind, 16)}<strong>{session.title}</strong></span>
      <span className={`state-pill ${session.status}`}>{statusLabel(session.status)}{session.status === "running" && <span> · {session.duration}</span>}</span>
      <button title="Pin terminal"><Zap size={15} /></button><button title="More actions" onClick={onPalette}><MoreHorizontal size={17} /></button>
    </header>
    <div className="terminal-meta">{session.detail || session.cwd}</div>
    <div className="terminal-content">
      {session.kind === "codex" && session.status === "waiting" ? <ApprovalCard cwd={session.cwd} onContinue={onApproval} /> : <pre>{session.output.map((line, index) => <code key={`${line}-${index}`} className={line.startsWith("+") ? "line-add" : line.startsWith("-") ? "line-remove" : line.startsWith("•") ? "line-note" : line.startsWith("✓") ? "line-success" : line.startsWith("→") ? "line-prompt" : ""}>{line || " "}{"\n"}</code>)}</pre>}
    </div>
  </section>;
}

function RealTerminalPane({ session, active, onFocus, onPalette }: { session: Session; active: boolean; onFocus: () => void; onPalette: () => void }) {
  const [command, setCommand] = useState("");
  const [lines, setLines] = useState<string[]>([
    "AI Terminal local shell",
    "Connected to a persistent PowerShell session.",
    "Type a command below and press Enter.",
    "",
  ]);
  const [cwd, setCwd] = useState(session.cwd);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState("");

  async function runCommand(event: FormEvent) {
    event.preventDefault();
    const value = command.trim();
    if (!value || busy) return;
    setCommand("");
    setBusy(true);
    setError("");
    setLines((previous) => [...previous, `PS ${cwd}> ${value}`]);
    try {
      const response = await fetch("/api/terminal/exec", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ command: value }),
      });
      const result = await response.json() as { output?: string; cwd?: string; error?: string };
      if (!response.ok) throw new Error(result.error || `Request failed (${response.status})`);
      const output = result.output;
      if (output) setLines((previous) => [...previous, ...output.split("\n")]);
      if (result.cwd) setCwd(result.cwd);
    } catch (requestError) {
      setError(requestError instanceof Error ? requestError.message : String(requestError));
    } finally {
      setBusy(false);
    }
  }

  return <section className={`terminal-pane ${active ? "active" : ""}`} onMouseDown={onFocus}>
    <header className="pane-header">
      <span className="pane-title">{iconFor("shell", 16)}<strong>PowerShell · Local</strong></span>
      <span className="state-pill running">CONNECTED</span>
      <button title="Pin terminal"><Zap size={15} /></button><button title="More actions" onClick={onPalette}><MoreHorizontal size={17} /></button>
    </header>
    <div className="terminal-meta">{cwd} · real local process</div>
    <div className="terminal-content real-terminal-content">
      <pre>{lines.map((line, index) => <code key={`${index}-${line}`}>{line}{"\n"}</code>)}{error && <code className="line-remove">{error}{"\n"}</code>}</pre>
      <form className="terminal-input-row" onSubmit={runCommand}>
        <span>PS {cwd}&gt;</span>
        <input value={command} onChange={(event) => setCommand(event.target.value)} disabled={busy} autoComplete="off" spellCheck={false} aria-label="PowerShell command" placeholder={busy ? "Running..." : "Enter a PowerShell command"} />
        <button type="submit" disabled={busy || !command.trim()}>{busy ? "…" : "Run"}</button>
      </form>
    </div>
  </section>;
}

function ApprovalCard({ cwd, onContinue }: { cwd: string; onContinue?: () => void }) {
  return <div className="approval-card">
    <div className="approval-heading"><ShieldCheck size={27} /><div><strong>Approval required</strong><span>Codex wants to run the following command:</span></div></div>
    <code className="command-box">npm install @playwright/test</code>
    <p>This command will:</p><ul><li>Download and install 3 new packages</li><li>Add 1.2 MB to <code>node_modules/</code></li><li>Update <code>package-lock.json</code></li></ul>
    <label>Working directory</label><code className="command-box muted">{cwd}</code>
    <p className="approval-question">Continue in the original Codex CLI prompt?</p>
    <div className="approval-actions"><button className="primary" onClick={onContinue}>Focus Codex</button><button onClick={onContinue}>Review command</button><button className="danger" onClick={onContinue}>Deny in CLI</button></div>
    <small><ShieldCheck size={13} /> AI Terminal displays this state but never auto-approves commands.</small>
    <div className="terminal-prompt">codex[frontend]$ <i /></div>
  </div>;
}

function CommandPalette({ query, commands, onQuery, onClose, onSelect }: { query: string; commands: PaletteCommand[]; onQuery: (query: string) => void; onClose: () => void; onSelect: (command: PaletteCommand) => void }) {
  return <div className="modal-layer" onMouseDown={onClose}><section className="palette" onMouseDown={(event) => event.stopPropagation()}>
    <div className="palette-search"><Search size={21} /><input autoFocus value={query} onChange={(event) => onQuery(event.target.value)} placeholder="> Search commands" /></div>
    <div className="palette-body"><div className="palette-results">{commands.length ? commands.map((command, index) => <button key={command.id} className={index === 0 ? "highlighted" : ""} onClick={() => onSelect(command)}>{iconFor(command.action === "new-claude" ? "claude" : command.action === "new-codex" ? "codex" : command.action === "ssh" ? "ssh" : "shell", 21)}<span><strong>{command.title}</strong><small>{command.subtitle}</small></span>{command.key && <kbd>{command.key}</kbd>}</button>) : <div className="empty-palette">No command matches “{query}”</div>}</div><aside className="palette-hints"><kbd>Enter</kbd><span>Run</span><kbd>↑ ↓</kbd><span>Navigate</span><kbd>Esc</kbd><span>Close</span></aside></div>
    <footer><span><Command size={14} /> Recent commands</span><span><i className="status-dot running" /> Fuzzy search enabled</span></footer>
  </section></div>;
}

function AgentDashboard({ sessions, onClose, onFocus }: { sessions: Session[]; onClose: () => void; onFocus: (session: Session, target: "primary" | "secondary") => void }) {
  const ordered = [...sessions].sort((left, right) => (left.status === "waiting" ? -2 : left.status === "failed" ? -1 : 0) - (right.status === "waiting" ? -2 : right.status === "failed" ? -1 : 0));
  return <div className="modal-layer" onMouseDown={onClose}><section className="dashboard-dialog" onMouseDown={(event) => event.stopPropagation()}><header><div><span className="eyebrow">AI TERMINAL</span><h2>Agent Dashboard</h2><p>Focus the work that needs your attention.</p></div><button onClick={onClose}><X size={20} /></button></header><div className="agent-list">{ordered.filter((session) => session.kind === "claude" || session.kind === "codex").map((session) => <article key={session.id} className={`agent-row ${session.status}`}><span className="agent-icon">{iconFor(session.kind, 21)}</span><div><strong>{session.title}</strong><small>{session.project} · {session.cwd}</small></div><span className={`dashboard-status ${session.status}`}><i className={`status-dot ${session.status}`} />{statusLabel(session.status)}</span><span className="agent-detail">{session.detail || "No active task"}</span><span className="duration">{session.duration}</span><button onClick={() => onFocus(session, session.status === "waiting" ? "secondary" : "primary")}>Focus</button></article>)}</div><footer><span>State labels are based on adapter evidence. Generic shells remain unclassified.</span><button onClick={onClose}>Done</button></footer></section></div>;
}

function SettingsDialog({ onClose }: { onClose: () => void }) {
  const [history, setHistory] = useState("metadata");
  const [waitingNotifications, setWaitingNotifications] = useState(true);
  const [neverAutoApprove, setNeverAutoApprove] = useState(true);
  const nav = ["General", "Appearance", "Profiles", "Terminal", "Keyboard", "AI Agents", "Workspace", "SSH", "Notifications", "Privacy", "Advanced"];
  return <div className="modal-layer settings-layer"><section className="settings-dialog"><header className="settings-topbar"><div className="brand"><TerminalSquare size={18} /><span>Settings</span></div><button onClick={onClose}><X size={19} /></button></header><div className="settings-body"><nav>{nav.map((item) => <button key={item} className={item === "AI Agents" ? "selected" : ""}>{item === "AI Agents" ? <WandSparkles size={18} /> : item === "Keyboard" ? <Keyboard size={18} /> : item === "SSH" ? <Server size={18} /> : item === "Privacy" ? <ShieldCheck size={18} /> : <MonitorCog size={18} />}{item}</button>)}</nav><main><div className="settings-title"><div><h2>AI Agents</h2><p>Configure agent discovery, notifications and safe defaults.</p></div><button className="link-button">Reset settings</button></div><section className="settings-card"><h3>Available agents</h3><div className="agent-table"><div className="agent-table-head"><span>Agent</span><span>Executable</span><span>Default working directory</span><span>Permissions</span><span>Status</span></div>{["Claude Code", "Codex CLI", "Gemini CLI", "OpenCode"].map((agent, index) => <div className="agent-table-row" key={agent}><span><i className={`agent-badge badge-${index}`}>{agent.split(" ").map((part) => part[0]).join("")}</i><strong>{agent}<small>{index === 0 ? "anthropic/claude-code" : index === 1 ? "openai/codex" : index === 2 ? "google/gemini-cli" : "opencode-ai/opencode"}</small></strong></span><span className="executable"><i className="status-dot running" /> {index === 0 ? "claude" : index === 1 ? "codex" : index === 2 ? "gemini" : "opencode"}<small>Found in PATH</small></span><span>Per workspace</span><button className="select-button">Ask every time <ChevronDown size={15} /></button><span className="switch-row">Enabled <i className="switch on" /></span></div>)}</div></section><section className="settings-card privacy-card"><h3>Privacy & Safety</h3><SettingToggle label="Never auto-approve commands" description="AI Terminal displays requests but does not grant an Agent permission." enabled={neverAutoApprove} onToggle={() => setNeverAutoApprove((value) => !value)} /><div className="setting-row"><div><strong>Save terminal history</strong><small>Store session data with the selected privacy level.</small></div><div className="segmented">{[["off", "None"], ["metadata", "Metadata only"], ["full", "Full encrypted"]].map(([value, label]) => <button key={value} className={history === value ? "active" : ""} onClick={() => setHistory(value)}>{label}</button>)}</div></div><SettingToggle label="Show waiting notifications" description="Notify when an agent needs permission or user input." enabled={waitingNotifications} onToggle={() => setWaitingNotifications((value) => !value)} /></section></main></div></section></div>;
}

function SettingToggle({ label, description, enabled, onToggle }: { label: string; description: string; enabled: boolean; onToggle: () => void }) {
  return <div className="setting-row"><div><strong>{label}</strong><small>{description}</small></div><button className={`switch-button ${enabled ? "on" : ""}`} onClick={onToggle} aria-label={label}><i /></button></div>;
}

createRoot(document.getElementById("root")!).render(<App />);
