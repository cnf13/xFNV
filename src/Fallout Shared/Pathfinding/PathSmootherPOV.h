#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

float* ComputePerpendicular(float* a1, float* a2, int32_t a3);

// A point of view of the path smoother: a vertex, the start or goal, or an avoid node, with its circle and edges.
class PathSmootherPOV {
public:
	// Values from the Xbox PDB.
	enum POV_TYPE : int32_t {
		POV_TYPE_INVALID = -1,
		POV_TYPE_LEFT_VERTEX = 0x0,
		POV_TYPE_RIGHT_VERTEX = 0x1,
		POV_TYPE_START = 0x2,
		POV_TYPE_GOAL = 0x3,
		POV_TYPE_AVOIDNODE = 0x4,
		POV_TYPE_AVOIDNODE_NOCROSS = 0x5,
	};

	PathSmootherPOV();

	POV_TYPE eType;
	NiPoint3 Center;
	float fRadius;
	FastNavMeshEdgeHandle EdgeHandle;
	NiPoint3 Edge1;
	NiPoint3 Edge2;
	float fCostFactor;
	uint32_t iThisSideIndex;
	uint32_t iOtherSideIndex;
	uint32_t iAvoidNodeIndex;
	uint16_t iVertexIndex;
	uint8_t padding4A[2];
};

// The game settings PathSmootherPOV.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> fPOVSmootherAvoidNodeCost;
