#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoundingVolume.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCapsule.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

// a capsule bounding volume, kept both as centre, axis and extent and as a capsule
class NiCapsuleBV : public NiBoundingVolume {
public:
	NiCapsuleBV(const NiCapsuleBV& arCapsule);
	NiCapsuleBV(float fRadius, float fExtent, const NiPoint3& kCenter, const NiPoint3& kAxis);

	// no destructor of its own: the slot folds with the NiBoundingVolume one
	void LoadBinary(NiStream& stream) override;
	void SaveBinary(NiStream& stream) override;
	// shared constant stub
	int32_t Type() const override { return CAPSULE_BV; }
	void UpdateWorldData(const NiBoundingVolume& kModelABV, const NiTransform& kWorld) override;
	NiBoundingVolume* Create() const override;
	NiBoundingVolume* Clone() const override;
	void Copy(const NiBoundingVolume& kABV) override;
	bool operator==(const NiBoundingVolume& kABV) const override;
	// one body shared by every bounding volume: the negated operator==
	bool operator!=(const NiBoundingVolume& kABV) const override;
	void AddViewerStrings(const char* pcPrefix, NiTPrimitiveArray<char*>* pkStrings) const override;

protected:
	// Rebuilds the capsule's segment from the centre, axis, extent and radius.
	void CalculateCapsule();

public:
	// The loader of ms_apfnLoaders[CAPSULE_BV]: a new unit capsule (0x3C bytes) that
	// LoadBinary fills from arStream. Hides NiBoundingVolume's, as in the engine.
	static NiBoundingVolume* CreateFromStream(NiStream& stream);
	// The exe's out-of-line sweep tests of a capsule against a sphere and against a capsule (cdecl, ret 0).
	static bool CapsuleSphereFindIntersect(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
		const NiBoundingVolume& kABV1, const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, bool bCalcNormals,
		NiPoint3& kNormal0, NiPoint3& kNormal1);
	static bool CapsuleCapsuleFindIntersect(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
		const NiBoundingVolume& kABV1, const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, bool bCalcNormals,
		NiPoint3& kNormal0, NiPoint3& kNormal1);
	static bool CapsuleSphereTestIntersect(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
		const NiBoundingVolume& kABV1, const NiPoint3& kV1);
	static bool CapsuleCapsuleTestIntersect(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
		const NiBoundingVolume& kABV1, const NiPoint3& kV1);
	static bool CapsuleTriTestIntersect(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
		const NiPoint3& kVert0, const NiPoint3& kVert1, const NiPoint3& kVert2, const NiPoint3& kV1);
	static bool CapsuleTriFindIntersect(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
		const NiPoint3& kVert0, const NiPoint3& kVert1, const NiPoint3& kVert2, const NiPoint3& kV1, float& fIntrTime,
		NiPoint3& kIntrPt, bool bCalcNormals, NiPoint3& kNormal0, NiPoint3& kNormal1);

	NiPoint3 m_kCenter;
	NiPoint3 m_kAxis;
	float m_fExtent;
	NiCapsule m_kCapsule;
};
inline constexpr fnvd::Vtable NiCapsuleBV_vftable{};
