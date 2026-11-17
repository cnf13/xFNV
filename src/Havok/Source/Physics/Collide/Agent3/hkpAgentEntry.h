#pragma once

#include <cstdint>

// The header of one agent's record in a collision agent stream: the stream command, the agent type, how many contact
// points it has and its size in bytes.
struct hkpAgentEntry {
	uint8_t m_streamCommand;
	uint8_t m_agentType;
	uint8_t m_numContactPoints;
	uint8_t m_size;
};
