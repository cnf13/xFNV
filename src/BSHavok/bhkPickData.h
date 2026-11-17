#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Reconstruction.hpp"

class CFilter;
class NiAVObject;
class NiPoint3;
class SpecificItemCollector;
class hkpAllRayHitCollector;
class hkpClosestRayHitCollector;
class hkpRigidBody;

// A ray pick: the ray (its end point is calculated from the start and kLength when kLength is not zero), the
// collision filter, the collectors the hits go to and the result. Not polymorphic. The PDB's source module is
// BGSDecalManager.cpp. IDA names the last member bFailed.
class bhkPickData : public hkpWorldRayCastInput, public hkpWorldRayCastOutput {
public:
	bhkPickData();

	// The end point of the ray.
	const hkVector4& GethkTo();
	// Calculates the end point of the ray from its start point and length.
	void CalcTo();
	// Copies arFrom into the ray's start point.
	void SetFrom(const hkVector4& arFrom);
	void SetAllCollector(hkpAllRayHitCollector* apCollector);
	void SetClosestCollector(hkpClosestRayHitCollector* apCollector);
	bool GetPickFailed();
	NiAVObject* GetNiAVObject();
	// The end point of the ray in game units, written to arTo.
	void GetTo(NiPoint3& arTo);
	// True when the ray has a length (kLength is not zero within 0.001).
	bool QLength();
	// kLength.
	hkVector4& GethkLength() { return hkLength; }
	// The ray's start point, through the exe's shared identity function (thiscall, ret 0,
	// 0x6815C0).
	hkVector4* GethkFrom();
	// The projectiles pass their pick cache's name.
	void SetCache(char* apCache) { pCache = apCache; }
	// pAllCollector.
	hkpAllRayHitCollector* GetAllCollector() { return pAllCollector; }
	// The ray's length vector in game units, written to arLength.
	void GetLength(NiPoint3& arLength);
	// Sets the ray's length vector; with a length the end point is calculated from it.
	void SetLength(const hkVector4& arLength);
	// Sets the ray's end point and clears its length.
	void SetTo(const hkVector4& arTo);
	// Sets the collision filter info of the ray.
	void SetFilter(CFilter aiFilter);
	// The rigid body the ray hit, or null.
	hkpRigidBody* GethkRigidBody();
	// Sets the ray's start point from a game-unit point.
	void NiSetFrom(const NiPoint3& arFrom);
	// Sets the ray's end point from a game-unit point.
	void NiSetTo(const NiPoint3& arTo);
	// Writes the ray's hit point, calculated by CalcHitPoint, in game units.
	void NiCalcHitPoint(NiPoint3& arPoint);
	// Sets the ray's length vector from a game-unit vector.
	void NiSetLength(const NiPoint3& arLength);
	// The ray's hit point in Havok units: the start interpolated towards the end by the hit fraction.
	void CalcHitPoint(hkVector4& arPoint);
	// The ray's start point in game units, written to arFrom (thiscall, ret 4, 0x4585F0). The exe leaves HK2NI's
	// result in eax; none of its callers reads it.
	void GetFrom(NiPoint3& arFrom);
	// The ray's filter, built from its filter info into apResult.
	CFilter* GetFilter(CFilter* apResult);

	hkVector4 hkLength;
	char* pCache;
	SpecificItemCollector* pCloseCollector;
	hkpAllRayHitCollector* pAllCollector;
	bool bPickFailed;
};
