#pragma once

#include "Reconstruction.hpp"

class hkStatisticsCollector;
class hkpAgentNnTrack;
class hkpCollisionInput;

typedef void hkpAgentData;

// Havok's static helpers reporting the agents of Nn and 1n tracks to a statistics collector (calcNnStatistics,
// calcNnStatisticsContactMgrsOnly, calc1nStatistics; the game PDB places them in
// .\Agent3\Machine\hkpAgentMachineUtil.cpp). No data: sizeof 1 in Havok's release hkpcollide.pdb.
class hkpAgentMachineUtil {
public:
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
	static void calcNnStatistics(const hkpAgentNnTrack& track, const hkpCollisionInput& input,
		hkStatisticsCollector* collector);
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
	static void calcNnStatisticsContactMgrsOnly(const hkpAgentNnTrack& track, hkStatisticsCollector* collector);
	// Reports the track's child agents to the collector, which the client data (the collision input) is handed on with
	// (cdecl, ret 0, 0xD97250). IDA name; not in the PDB. The exe only reads the track (no store through it), so it is
	// const: hkpBvTreeStreamAgent::calcContentStatistics (const) passes its own track.
	static void calc1nStatistics(const hkpAgentData* agentData, const void* clientData,
		hkStatisticsCollector* collector);
};
