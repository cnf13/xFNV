#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Reconstruction.hpp"

class Actor;
class GridCellArray;
class TESObjectCELL;
class TESObjectREFR;
class bhkAabbPhantom;
class bhkRigidBody;
class hkpPhantom;
class hkpRigidBody;
class hkpSimulationIsland;

// The PDB's compiler-generated hkAabb::operator=(__that), defined in LoadedAreaBound.cpp. The exe folds it with
// hkContactPoint::operator= (0x6281A0: hkVector4::operator= 0x4A3F10 on m_min, then on m_max; thiscall, ret 4,
// returns apThis), which LoadedAreaBound::Create and bhkAabbPhantom::GetAabb call out of line. A free forwarder: a
// user-provided operator= would make hkAabb non-trivially copyable for every src user.
hkAabb* hkAabb_operator_assign(hkAabb* apThis, const hkAabb& arThat);

// The Havok phantom bound around the loaded area: keeps the player's current cell and the six
// phantom bodies that stand in for the world outside the AABB.
class LoadedAreaBound : public NiRefObject {
public:
	LoadedAreaBound(float a1, float a2, float a3, float a4);
	~LoadedAreaBound() override;

	void Clear();
	// The AABB of the collision in apCell, in Havok units: in an interior, the broad phase AABBs of the entities of
	// the cell world's fixed island; in an exterior, those of the MOPP bodies of every loaded cell of apCellGrid
	// (a body outside a world contributes its shape's AABB at its transform). False when the cell has no world or
	// nothing was found.
	bool GetBoundFromCell(TESObjectCELL* apCell, GridCellArray* apCellGrid, hkAabb& arBound);
	void WarpActor(Actor* apActor);
	// Where to warp apRef back into the loaded area, in arWarpLoc.
	void FindWarpLocation(TESObjectREFR* apRef, const NiPoint3& arCurLoc, NiPoint3& arWarpLoc);
	void Update(TESObjectCELL* apCell, GridCellArray* apCellGrid);
	// Builds the six phantom walls around arboundAABB and adds them to apCell's world.
	void Create(TESObjectCELL* apCell, const hkAabb& arboundAABB);
	// Handles the phantoms and the bodies of the current cell's world that left the box of the six walls.
	void CheckForDynamicBodiesOutsideBound();
	// Warps the moving bodies of arSimIslandA's islands that left boundAABB back into the loaded area.
	void SetBodiesMotionOutsideAABB(hkAabb& boundAABB,
		const hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>& arSimIslandA);
	// Warps the character controllers among arPhantomA that left boundAABB back into the loaded area.
	void CheckPhantomsOutsideAABB(hkAabb& boundAABB, const hkArray<hkpPhantom*, hkContainerHeapAllocator>& arPhantomA,
		hkpWorld* apWorld);
	// Warps apBody, with the bodies constrained to it, back into the loaded area; the bodies it moved besides apBody
	// go on arWarpedBodies.
	void WarpBody(bhkRigidBody* apBody, hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arWarpedBodies);

	// The phantom bodies (PDB: NiPointer<bhkAabbPhantom>; Create 0x627980 stores the bhkAabbPhantoms it builds,
	// CheckForDynamicBodiesOutsideBound 0x6286F0 calls bhkAabbPhantom::GetAabb on them).
	NiPointer<bhkAabbPhantom> spPhantomA[6];   // 0x08

	// The cell the player is in.
	TESObjectCELL* pCurrentCell; // 0x20

	// The phantom hit map: each body hit and the border flags it hit (the constructor 0x626FD0 builds it with 37
	// buckets).
	NiTMap<bhkRigidBody*, uint16_t> HitMap; // 0x24

	// PDB names (IDA the same): the constructor 0x626FD0 stores its four float parameters here.
	float fBoundOffset;
	float fBoundDepth;
	float fSideWallHeightOffset;
	float fVelocityDampen;
};
inline constexpr fnvd::Vtable LoadedAreaBound_vftable{};
