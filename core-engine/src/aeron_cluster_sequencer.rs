use std::time::Instant;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum NodeRole {
    Leader,
    Follower,
    Candidate,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ClusterNode {
    pub node_id: u32,
    pub endpoint: String,
    pub role: NodeRole,
    pub commit_index: u64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SequencedEvent {
    pub sequence_number: u64,
    pub term: u64,
    pub payload: String,
    pub quorum_acks: u32,
    pub commit_latency_nanos: u128,
}

pub struct AeronClusterSequencer {
    pub current_term: u64,
    pub sequence_counter: u64,
    pub nodes: Vec<ClusterNode>,
    pub cluster_size: usize,
}

impl AeronClusterSequencer {
    pub fn new_3node_cluster() -> Self {
        Self {
            current_term: 1,
            sequence_counter: 0,
            nodes: vec![
                ClusterNode { node_id: 1, endpoint: "aeron:ipc?term-length=64k".to_string(), role: NodeRole::Leader, commit_index: 0 },
                ClusterNode { node_id: 2, endpoint: "aeron:udp?endpoint=10.0.1.2:40123".to_string(), role: NodeRole::Follower, commit_index: 0 },
                ClusterNode { node_id: 3, endpoint: "aeron:udp?endpoint=10.0.1.3:40123".to_string(), role: NodeRole::Follower, commit_index: 0 },
            ],
            cluster_size: 3,
        }
    }

    /// Replicates an inbound order/trade event across the 3-node cluster, reaching quorum (2 of 3 acks)
    pub fn sequence_and_commit(&mut self, payload: &str) -> SequencedEvent {
        let start = Instant::now();
        self.sequence_counter += 1;
        let seq = self.sequence_counter;

        // Leader increments commit index and replicates to followers
        let quorum_needed = (self.cluster_size / 2) + 1; // 2 of 3
        let mut acks = 1; // Leader is self-acked

        // Simulate zero-copy ring-buffer broadcast to followers
        for node in self.nodes.iter_mut().skip(1) {
            node.commit_index = seq;
            acks += 1;
            if acks >= quorum_needed as u32 {
                break;
            }
        }

        let nanos = start.elapsed().as_nanos();

        SequencedEvent {
            sequence_number: seq,
            term: self.current_term,
            payload: payload.to_string(),
            quorum_acks: acks,
            commit_latency_nanos: nanos,
        }
    }
}
