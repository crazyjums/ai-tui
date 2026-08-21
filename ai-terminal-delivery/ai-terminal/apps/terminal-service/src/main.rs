mod conpty;
mod model;

use model::{
    AgentSnapshot, AgentState, EvidenceSource, GridSize, HistoryPolicy, SessionLifecycle,
    SessionMetadata,
};
use std::env;
use std::process::Command;
use std::process::ExitCode;

fn main() -> ExitCode {
    let command = env::args().nth(1);
    match command.as_deref() {
        None | Some("terminal") => run_terminal(),
        Some("doctor") => doctor(),
        Some("simulate-agent") => simulate_agent(),
        Some("help") | Some("--help") | Some("-h") => {
            println!("AI Terminal\n\nCommands:\n  terminal        Open a real local PowerShell session (default)\n  doctor          Print platform and ConPTY capability\n  simulate-agent  Exercise the normalized agent state model");
            ExitCode::SUCCESS
        }
        Some(other) => {
            eprintln!("unknown command: {other}; run `ai-terminal-service help`");
            ExitCode::from(2)
        }
    }
}

fn run_terminal() -> ExitCode {
    let (executable, args): (&str, &[&str]) = if cfg!(windows) {
        ("powershell.exe", &["-NoLogo", "-NoProfile"])
    } else {
        ("/bin/sh", &["-i"])
    };

    println!("AI Terminal — native local shell");
    println!("Starting {executable}. Type `exit` to close the terminal.\n");

    match Command::new(executable).args(args).status() {
        Ok(status) => status
            .code()
            .map(|code| ExitCode::from(code.clamp(0, 255) as u8))
            .unwrap_or_else(|| ExitCode::from(1)),
        Err(error) => {
            eprintln!("failed to start {executable}: {error}");
            ExitCode::from(1)
        }
    }
}

fn doctor() -> ExitCode {
    let grid = match GridSize::new(120, 36) {
        Ok(grid) => grid,
        Err(message) => {
            eprintln!("invalid service bootstrap grid: {message}");
            return ExitCode::from(1);
        }
    };
    let session = SessionMetadata::new("doctor-session", "PowerShell", "D:\\codes\\ai-cicd", grid);
    println!("AI Terminal Service diagnostic");
    println!("  platform: {}", env::consts::OS);
    println!("  ConPTY compiled support: {}", conpty::is_supported());
    println!(
        "  default grid: {}x{}",
        session.grid.cols, session.grid.rows
    );
    println!(
        "  default history policy: {:?}",
        HistoryPolicy::MetadataOnly
    );
    println!("  initial lifecycle: {:?}", session.lifecycle);
    println!("  architecture: UI -> Named Pipe -> Terminal Service -> PTY -> CLI");
    println!("  note: this scaffold exposes no fake terminal; the production Windows runner will own ConPTY pipes and a VT core.");
    ExitCode::SUCCESS
}

fn simulate_agent() -> ExitCode {
    let mut snapshot = AgentSnapshot::generic_running("Claude — backend");
    println!("state = {:?}", snapshot.state);

    snapshot.apply(
        AgentState::WaitingPermission,
        EvidenceSource::OutputPattern,
        "possible prompt-shaped output",
    );
    println!(
        "after weak output pattern = {:?} (expected Running)",
        snapshot.state
    );

    snapshot.apply(
        AgentState::WaitingPermission,
        EvidenceSource::InstalledHook,
        "Claude hook reported an approval request",
    );
    println!("after structured hook = {:?}", snapshot.state);

    snapshot.apply(
        AgentState::Completed,
        EvidenceSource::OfficialProtocol,
        "Agent completed successfully",
    );
    println!(
        "final state = {:?}; needs_attention = {}",
        snapshot.state,
        snapshot.state.needs_attention()
    );

    let mut metadata = SessionMetadata::new(
        "simulation",
        "Claude Code",
        "D:\\codes\\ai-cicd",
        GridSize::new(120, 36).expect("constant grid is valid"),
    );
    metadata.lifecycle = SessionLifecycle::Closed { exit_code: Some(0) };
    println!("session lifecycle = {:?}", metadata.lifecycle);
    ExitCode::SUCCESS
}
