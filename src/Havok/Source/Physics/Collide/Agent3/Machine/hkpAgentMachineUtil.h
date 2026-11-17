#pragma once

class hkpAgentNnTrack;
class hkpCollisionInput;
class hkStatisticsCollector;

typedef void hkpAgentData;

// Havok's static helpers reporting the agents of Nn and 1n tracks to a statistics collector (calcNnStatistics,
// calcNnStatisticsContactMgrsOnly, calc1nStatistics). No data.
class hkpAgentMachineUtil {
public:
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
	static void calcNnStatistics(const hkpAgentNnTrack& track, const hkpCollisionInput& input,
		hkStatisticsCollector* collector);
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
	static void calcNnStatisticsContactMgrsOnly(const hkpAgentNnTrack& track, hkStatisticsCollector* collector);
	// Reports the track's child agents to the collector, which the client data (the collision input) is handed on with.
	// Only reads the track (no store through it), so it is const: hkpBvTreeStreamAgent::calcContentStatistics (const)
	// passes its own track.
	static void calc1nStatistics(const hkpAgentData* agentData, const void* clientData,
		hkStatisticsCollector* collector);
};
