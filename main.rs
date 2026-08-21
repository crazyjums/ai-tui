mod conpty;
mod model;

use model::{AgentSnapshot, AgentState, EvidenceSource, GridSize, HistoryPolicy, SessionLifecycle, SessionMetadata};
use std::env;
use std::process::ExitCode;

fn main() -> ExitCode {
    let command = env::args().nth(1).unwrap_or_else(|| "doctor".into());
    match command.as_str() {
        "doctor" => doctor(),
        "simulate-agent" => simulate_agent(),
        "help" | "--help" | "-h" => {
            println!("AI Terminal Service\n\nCommands:\n  doctor          Print platform and ConPTY capability\n  simulate-agent  Exercise the normalized agent state model");
            ExitCode::SUCCESS
        }
        other => {
            eprintln!("unknown command: {other}; run `ai-terminal-service help`");
            ExitCode::from(2)
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
    println!("  default grid: {}x{}", session.grid.cols, session.grid.rows);
    println!("  default history policy: {:?}", HistoryPolicy::MetadataOnly);
    println!("  initial lifecycle: {:?}", session.lifecycle);
    println!("  architecture: UI -> Named Pipe -> Terminal Service -> PTY -> CLI");
    println!("  note: this scaffold exposes no fake terminal; the production Windows runner will own ConPTY pipes and a VT core.");
    ExitCode::SUCCESS
}

fn simulate_agent() -> ExitCode {
    let mut snapshot = AgentSnapshot::generic_running("Claude — backend");
    println!("state = {:?}", snapshot.state);

    snapshot.apply(AgentState::WaitingPermission, EvidenceSource::OutputPattern, "possible prompt-shaped output");
    println!("after weak output pattern = {:?} (expected Running)", snapshot.state);

    snapshot.apply(AgentState::WaitingPermission, EvidenceSource::InstalledHook, "Claude hook reported an approval request");
    println!("after structured hook = {:?}", snapshot.state);

    snapshot.apply(AgentState::Completed, EvidenceSource::OfficialProtocol, "Agent completed successfully");
    println!("final state = {:?}; needs_attention = {}", snapshot.state, snapshot.state.needs_attention());

    let mut metadata = SessionMetadata::new("simulation", "Claude Code", "D:\\codes\\ai-cicd", GridSize::new(120, 36).expect("constant grid is valid"));
    metadata.lifecycle = SessionLifecycle::Closed { exit_code: Some(0) };
    println!("session lifecycle = {:?}", metadata.lifecycle);
    ExitCode::SUCCESS
}

