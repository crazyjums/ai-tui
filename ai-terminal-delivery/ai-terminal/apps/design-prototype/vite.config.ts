import { spawn, type ChildProcessWithoutNullStreams } from "node:child_process";
import { randomUUID } from "node:crypto";
import { defineConfig, type Plugin } from "vite";
import react from "@vitejs/plugin-react";

type ShellRequest = { command?: string };

function localTerminalApi(): Plugin {
  let shell: ChildProcessWithoutNullStreams | undefined;
  let pending = Promise.resolve();

  function ensureShell() {
    if (shell && !shell.killed) return shell;
    const executable = process.platform === "win32" ? "powershell.exe" : (process.env.SHELL || "/bin/sh");
    shell = spawn(executable, process.platform === "win32"
      ? ["-NoLogo", "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-Command", "-"]
      : ["-i"], { env: { ...process.env, TERM: "xterm-256color" } });
    shell.on("exit", () => { shell = undefined; });
    return shell;
  }

  function execute(command: string): Promise<{ output: string; cwd: string }> {
    const run = async () => {
      const child = ensureShell();
      const marker = `__AI_TERMINAL_${randomUUID().replaceAll("-", "") }__`;
      const script = process.platform === "win32"
        ? `$ProgressPreference='SilentlyContinue'; & { ${command} } 2>&1; (Get-Location).Path; Write-Output '${marker}'\n`
        : `{ ${command}; } 2>&1; pwd; printf '\\n${marker}\\n'\n`;
      return await new Promise<{ output: string; cwd: string }>((resolve, reject) => {
        let data = "";
        const cleanup = () => {
          child.stdout.off("data", onData);
          child.off("error", onError);
          child.off("exit", onExit);
        };
        const onError = (error: Error) => { cleanup(); reject(error); };
        const onExit = (code: number | null) => {
          cleanup();
          reject(new Error(`local shell exited${code === null ? "" : ` with code ${code}`}`));
        };
        const onData = (chunk: Buffer) => {
          data += chunk.toString();
          if (!data.includes(marker)) return;
          cleanup();
          const before = data.slice(0, data.indexOf(marker)).replace(/\r/g, "").trimEnd();
          const outputLines = before.split("\n");
          const cwd = outputLines.pop()?.trim() || "";
          resolve({ output: outputLines.join("\n").trimEnd(), cwd });
        };
        child.stdout.on("data", onData);
        child.once("error", onError);
        child.once("exit", onExit);
        child.stdin.write(script);
      });
    };
    const result = pending.then(run, run);
    pending = result.then(() => undefined, () => undefined);
    return result;
  }

  return {
    name: "local-terminal-api",
    configureServer(server) {
      server.middlewares.use("/api/terminal/exec", (req, res) => {
        if (req.method !== "POST") { res.statusCode = 405; res.end(); return; }
        let body = "";
        req.on("data", (chunk) => { body += chunk; });
        req.on("end", async () => {
          try {
            const payload = JSON.parse(body) as ShellRequest;
            const command = payload.command?.trim();
            if (!command) { res.statusCode = 400; res.end(JSON.stringify({ error: "command is required" })); return; }
            const result = await execute(command);
            res.setHeader("Content-Type", "application/json");
            res.end(JSON.stringify(result));
          } catch (error) {
            res.statusCode = 500;
            res.setHeader("Content-Type", "application/json");
            res.end(JSON.stringify({ error: error instanceof Error ? error.message : String(error) }));
          }
        });
      });
    },
    closeBundle() {
      shell?.kill();
    },
  };
}

export default defineConfig({
  plugins: [react(), localTerminalApi()],
  server: {
    port: 4173,
    strictPort: true,
  },
});
