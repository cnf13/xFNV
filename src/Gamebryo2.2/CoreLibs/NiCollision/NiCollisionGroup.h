#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiAVObject;

// Gamebryo's collision group: the colliders tested against the collidees. The exe has none of its code; the layout
// is the PDB's (sizeof 0x20: two record arrays), named here for NiCollisionData's collide callback.
class NiCollisionGroup {
public:
	class Record;

	// What a collide callback is told of one collision: the two trees' roots, the two objects, the time, the point,
	// the two normals and the two triangles (the PDB's members; sizeof 0x40).
	class Intersect {
	public:
		NiAVObject* pkRoot0;
		NiAVObject* pkRoot1;
		NiAVObject* pkObj0;
		NiAVObject* pkObj1;
		float fTime;
		NiPoint3 kPoint;
		NiPoint3 kNormal0;
		NiPoint3 kNormal1;
		const NiPoint3** appkTri1;
		const NiPoint3** appkTri2;
	};

	// What a collide callback returns (the PDB's unnamed enum).
	enum {
		CONTINUE_COLLISIONS = 0,
		TERMINATE_COLLISIONS = 1,
		BREAKOUT_COLLISIONS = 2,
	};

	// The collide callback: what to do after the collision (the PDB's Callback, cdecl).
	typedef int32_t (*Callback)(Intersect& arIntersect);
	typedef NiTPrimitiveArray<Record*> NiRecordArray;

protected:
	NiRecordArray m_kColliders;
	NiRecordArray m_kCollidees;
};
