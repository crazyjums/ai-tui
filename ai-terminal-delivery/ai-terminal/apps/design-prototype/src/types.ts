export type AgentStatus =
  | "running"
  | "waiting"
  | "completed"
  | "failed"
  | "idle";

export type SessionKind = "claude" | "codex" | "shell" | "ssh" | "wsl";

export interface Session {
  id: string;
  title: string;
  kind: SessionKind;
  status: AgentStatus;
  project: string;
  cwd: string;
  duration: string;
  output: string[];
  detail?: string;
}

export interface Workspace {
  id: string;
  name: string;
  sessionCount: number;
  color: "green" | "blue" | "amber" | "slate";
}

export interface PaletteCommand {
  id: string;
  title: string;
  subtitle: string;
  keywords: string;
  key?: string;
  action: "new-claude" | "new-codex" | "split" | "workspace" | "ssh" | "settings" | "dashboard";
}

export interface NotificationItem {
  id: string;
  title: string;
  message: string;
  tone: "info" | "warning" | "success";
}
