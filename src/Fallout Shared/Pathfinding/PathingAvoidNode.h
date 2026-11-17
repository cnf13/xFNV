#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleArrayRefCounted.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;

// A place a path should keep away from: a sphere around Point1, or a cylinder from Point1 to Point2, and what it costs
// to cross it. IDA names the fields kPoint1, kPoint2 and eType.
class PathingAvoidNode {
public:
	enum AVOID_NODE_TYPE : int32_t {
		AVOID_NODE_SPHERE = 0x0,
		AVOID_NODE_CYLINDER = 0x1,
		AVOID_NODE_SPHERE_ACTOR = 0x2,
		AVOID_NODE_SPHERE_TARGET = 0x3,
		AVOID_NODE_SPHERE_THREAT = 0x4,
		AVOID_NODE_SPHERE_OBSTACLE = 0x5,
	};

	PathingAvoidNode();
	PathingAvoidNode(const NiPoint3& arPoint1, const NiPoint3& arPoint2, float afRadius, float afCost);
	PathingAvoidNode(const NiPoint3& arPoint, float afRadius, float afCost);
	PathingAvoidNode(const NiPoint3& arPoint, float afRadius, float afCost, AVOID_NODE_TYPE aeType);

	// The cost of passing at akPoint (thiscall, ret 4, 0x6DC780; the result is in ST0 as a
	// float).
	float GetCostForPoint(NiPoint3& arPoint);
	// Writes the type as a byte, then the radius, cost and points; Point2 only for a cylinder.
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
	// Reads what SaveGame writes.
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	// Not named in the PDB or IDA; name chosen here. The node itself, read as Point1: the exe calls the shared
	// "return this" body. Same function as NiNode::IsNode (0x6815C0), registered there.
	NiPoint3* GetPoint1() { return &Point1; }
	// Not named in the PDB or IDA; name chosen here. fRadius, through the shared float getter (ST0). Same function
	// as BSMultiBoundSphere::QMultiBoundRadius (0x4A7BD0), registered there.
	float GetRadius() { return fRadius; }
	// Not named in the PDB or IDA; name chosen here. eType, through the shared +0x20 getter. Same function as
	// TESObjectREFR::GetObjectReference (0x7AF430), registered there.
	AVOID_NODE_TYPE GetType() { return eType; }
	// Not named in the PDB or IDA; name chosen here. Sets fRadius through the shared fld/fstp +0x18 setter (thiscall,
	// ret 4). Same function as CombatLocation::SetRadius (0x819A50), registered there.
	void SetRadius(float afRadius);
	// Not named in the PDB or IDA; name chosen here. Sets fCost through the shared fld/fstp +0x1C setter (thiscall,
	// ret 4). Same function as ActiveEffect::SetMagnitude (0x804CB0), registered there.
	void SetCost(float afCost);

	NiPoint3 Point1;
	NiPoint3 Point2;
	float fRadius;
	float fCost;
	AVOID_NODE_TYPE eType;
};

// A pathing request's places to avoid (PDB: derives from BSSimpleArrayRefCounted<PathingAvoidNode,1024>: the array
// at +0, the NiRefObject at +0x10). Only the members the ported code calls are declared.
class PathingAvoidNodeArray : public BSSimpleArrayRefCounted<PathingAvoidNode, 1024> {
public:
	// An empty array.
	PathingAvoidNodeArray();
	~PathingAvoidNodeArray() override;

	// The node at auiIndex, through GetAtInternal.
	PathingAvoidNode& GetAt(uint32_t auiIndex);
	// The radius sum of the nodes; arCenter gets their cost-weighted center, each cylinder counted at the point of
	// its axis closest to arStartPoint.
	float ComputeCenter(const NiPoint3& arStartPoint, NiPoint3& arCenter);
	float GetCostForPoint(NiPoint3& arPoint);
	// Not named in the PDB or IDA; name chosen here. The node at a1 (thiscall, ret 4, 0x6A1490): the PDB's
	// BSSimpleArray<PathingAvoidNode, 1024>::GetAt(int)
	// (?GetAt@?$BSSimpleArray@VPathingAvoidNode@@$0EAA@@@QAAAAVPathingAvoidNode@@H@Z).
	PathingAvoidNode& GetAtInternal(int32_t a1);
};

NiSmartPointer(PathingAvoidNodeArray);

// This instance's out-of-line copy of NiPointer's constructor from a raw pointer,
// a per-address body for a duplicated template instance; defined in
// Pathfinding/PathSmootherPOV.cpp.
NiPointer<PathingAvoidNodeArray>* NiPointer_PathingAvoidNodeArray_Construct(void* apMemory,
	PathingAvoidNodeArray* apObject);

using BSSimpleArray_PathingAvoidNode_1024 = BSSimpleArray<PathingAvoidNode, 1024>;

inline constexpr fnvd::Vtable PathingAvoidNodeArray_vftable{};
inline constexpr fnvd::Vtable PathingAvoidNodeArray_vftable_NiRefObject{};
