#pragma once

#include "BSCore/BSReadWriteLock.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Reconstruction.hpp"

class BGSLoadGameBuffer;
class PathingLOSGrid;
class PathingLOSMap;

// What a group has seen of the navmeshes around Center: per navmesh, its line-of-sight grid. IDA names the fields
// kGridMap, kPathinglocation, float44 and kLock.
class PathingLOSGridMap : public NiRefObject {
public:
	PathingLOSGridMap();
	~PathingLOSGridMap() override;

	// Sets the grid map up for aNavMeshes (thiscall, ret 4, 0x6E05F0); IDA types the argument
	// TESMain*.
	bool Setup(const BSSimpleArray<NavMeshPtr, 1024>& aNavMeshes);
	bool IsValid();
	void Clear();
	void Initialize(const PathingLocation& arCenter, float afRadius);
	void ClearSeen();
	void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	// fPercentSeen.
	float GetSeenPercent() const { return fPercentSeen; }
	// Stores afRadius in fSightRadius.
	void SetSightRadius(float afSightRadius);
	void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	NiPoint3* GetLocation(NiPoint3* a2);
	bool SetLOSMapFromSeenTriangles(NiPointer<PathingLOSMap>& aspLOSMap);
	bool SetSeenTrianglesFromLOSMap(NiPointer<PathingLOSMap>& aspLOSMap);
	bool IsLocationSeen(const PathingLocation& arLocation);
	bool SetLocationSeen(const PathingLocation& arLocation, bool abSeen);

	// The navmesh form ID to that navmesh's grid.
	NiTMap<uint32_t, PathingLOSGrid*> NavMeshToLOSGridMap;
	float fPercentSeen;
	PathingLocation Center;
	float fRadius;
	float fSightRadius;
	BSReadWriteLock Lock;
};

inline constexpr fnvd::Vtable PathingLOSGridMap_vftable{};
