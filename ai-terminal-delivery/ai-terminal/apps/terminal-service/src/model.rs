//! Domain objects intentionally shared by ConPTY, session persistence and agent adapters.
//! No UI or operating-system handle is allowed to leak into this module.

use std::time::{Duration, SystemTime};

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum AgentState {
    Idle,
    Starting,
    Running,
    Thinking,
    Executing,
    WaitingPermission,
    WaitingUserInput,
    Completed,
    Failed,
    Interrupted,
}

impl AgentState {
    pub fn needs_attention(&self) -> bool {
        matches!(
            self,
            Self::WaitingPermission | Self::WaitingUserInput | Self::Failed
        )
    }

    pub fn is_terminal(&self) -> bool {
        matches!(self, Self::Completed | Self::Failed | Self::Interrupted)
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum EvidenceSource {
    OfficialProtocol,
    InstalledHook,
    AdapterWrapper,
    ProcessLifecycle,
    OutputPattern,
}

impl EvidenceSource {
    pub fn confidence(&self) -> u8 {
        match self {
            Self::OfficialProtocol => 100,
            Self::InstalledHook => 95,
            Self::AdapterWrapper => 80,
            Self::ProcessLifecycle => 55,
            Self::OutputPattern => 25,
        }
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct AgentEvidence {
    pub source: EvidenceSource,
    pub observed_at: SystemTime,
    pub summary: String,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct AgentSnapshot {
    pub state: AgentState,
    pub title: String,
    pub last_activity: SystemTime,
    pub evidence: Vec<AgentEvidence>,
}

impl AgentSnapshot {
    pub fn generic_running(title: impl Into<String>) -> Self {
        let now = SystemTime::now();
        Self {
            state: AgentState::Running,
            title: title.into(),
            last_activity: now,
            evidence: vec![AgentEvidence {
                source: EvidenceSource::ProcessLifecycle,
                observed_at: now,
                summary: "Child process is alive; no structured adapter state is available".into(),
            }],
        }
    }

    pub fn apply(&mut self, state: AgentState, source: EvidenceSource, summary: impl Into<String>) {
        let now = SystemTime::now();
        // A weak text pattern may decorate a session, but cannot overwrite a stronger state.
        let strongest = self
            .evidence
            .iter()
            .map(|e| e.source.confidence())
            .max()
            .unwrap_or(0);
        if source.confidence() >= strongest {
            self.state = state;
        }
        self.last_activity = now;
        self.evidence.push(AgentEvidence {
            source,
            observed_at: now,
            summary: summary.into(),
        });
        self.evidence.truncate(32);
    }

    pub fn age(&self) -> Duration {
        self.last_activity.elapsed().unwrap_or_default()
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum HistoryPolicy {
    Off,
    MetadataOnly,
    FullEncrypted,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct GridSize {
    pub cols: u16,
    pub rows: u16,
}

impl GridSize {
    pub fn new(cols: u16, rows: u16) -> Result<Self, &'static str> {
        if cols == 0 || rows == 0 {
            return Err("terminal grid must be non-zero");
        }
        if cols > 1000 || rows > 1000 {
            return Err("terminal grid exceeds safe upper bound");
        }
        Ok(Self { cols, rows })
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum LayoutNode {
    Pane {
        pane_id: String,
    },
    Split {
        axis: SplitAxis,
        ratio: f32,
        first: Box<LayoutNode>,
        second: Box<LayoutNode>,
    },
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SplitAxis {
    Horizontal,
    Vertical,
}

impl LayoutNode {
    pub fn validate(&self) -> Result<(), &'static str> {
        match self {
            Self::Pane { pane_id } if pane_id.is_empty() => Err("pane id must be present"),
            Self::Pane { .. } => Ok(()),
            Self::Split {
                ratio,
                first,
                second,
                ..
            } => {
                if !(0.1..=0.9).contains(ratio) {
                    return Err("split ratio must be between 0.1 and 0.9");
                }
                first.validate()?;
                second.validate()
            }
        }
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum SessionLifecycle {
    Creating,
    Running,
    Closing,
    Draining,
    Closed { exit_code: Option<i32> },
    Failed { message: String },
}

#[derive(Clone, Debug)]
pub struct SessionMetadata {
    pub id: String,
    pub profile: String,
    pub cwd: String,
    pub grid: GridSize,
    pub lifecycle: SessionLifecycle,
    pub history: HistoryPolicy,
}

impl SessionMetadata {
    pub fn new(
        id: impl Into<String>,
        profile: impl Into<String>,
        cwd: impl Into<String>,
        grid: GridSize,
    ) -> Self {
        Self {
            id: id.into(),
            profile: profile.into(),
            cwd: cwd.into(),
            grid,
            lifecycle: SessionLifecycle::Creating,
            history: HistoryPolicy::MetadataOnly,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn layout_rejects_unsafely_small_split() {
        let layout = LayoutNode::Split {
            axis: SplitAxis::Vertical,
            ratio: 0.05,
            first: Box::new(LayoutNode::Pane {
                pane_id: "a".into(),
            }),
            second: Box::new(LayoutNode::Pane {
                pane_id: "b".into(),
            }),
        };
        assert!(layout.validate().is_err());
    }

    #[test]
    fn generic_session_remains_running_when_only_a_weak_pattern_arrives() {
        let mut snapshot = AgentSnapshot::generic_running("Generic shell");
        snapshot.apply(
            AgentState::WaitingPermission,
            EvidenceSource::OutputPattern,
            "possible approval text",
        );
        assert_eq!(snapshot.state, AgentState::Running);
        assert_eq!(
            snapshot.evidence.last().unwrap().source,
            EvidenceSource::OutputPattern
        );
    }

    #[test]
    fn safe_grid_limits_are_enforced() {
        assert!(GridSize::new(120, 40).is_ok());
        assert!(GridSize::new(0, 40).is_err());
        assert!(GridSize::new(1201, 40).is_err());
    }
}
