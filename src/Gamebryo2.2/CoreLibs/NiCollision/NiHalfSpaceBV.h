#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoundingVolume.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

// a half space bounding volume: everything behind a plane
class NiHalfSpaceBV : public NiBoundingVolume {
public:
	NiHalfSpaceBV(const NiPoint3& kPoint, const NiPoint3& kNormal);
	NiHalfSpaceBV(const NiHalfSpaceBV& kABV);

	// no destructor of its own: the slot folds with the NiBoundingVolume one
	void LoadBinary(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	// shared constant stub
	int32_t Type() const override { return HALFSPACE_BV; }
	void UpdateWorldData(const NiBoundingVolume& kModelABV, const NiTransform& kWorld) override;
	NiBoundingVolume* Create() const override;
	NiBoundingVolume* Clone() const override;
	void Copy(const NiBoundingVolume& kABV) override;
	bool operator==(const NiBoundingVolume& kABV) const override;
	// one body shared by every bounding volume: the negated operator==
	bool operator!=(const NiBoundingVolume& kABV) const override;
	void AddViewerStrings(const char* pcPrefix, NiTPrimitiveArray<char*>* pkStrings) const override;

	// Takes the plane normalized: unit normal, constant divided by the normal's length.
	void SetPlane(const NiPlane& kPlane);

	// The loader of ms_apfnLoaders[HALFSPACE_BV]: a new half space through NiPoint3::ZERO
	// along UNIT_Y (0x20 bytes) that LoadBinary fills from arStream. Hides
	// NiBoundingVolume's, as in the engine.
	static NiBoundingVolume* CreateFromStream(NiStream& kStream);
	// The swept test of the half space kABV0 against the sphere kABV1.
	static bool HalfSpaceSphereFindIntersect(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
		const NiBoundingVolume& kABV1, const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, bool bCalcNormals,
		NiPoint3& kNormal0, NiPoint3& kNormal1);
	// The swept test of the half space kABV0 against the capsule kABV1.
	static bool HalfSpaceCapsuleFindIntersect(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
		const NiBoundingVolume& kABV1, const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, bool bCalcNormals,
		NiPoint3& kNormal0, NiPoint3& kNormal1);
	static bool HalfSpaceHalfSpaceFindIntersect(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
		const NiBoundingVolume& kABV1, const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, bool bCalcNormals,
		NiPoint3& kNormal0, NiPoint3& kNormal1);
	static bool HalfSpaceSphereTestIntersect(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
		const NiBoundingVolume& kABV1, const NiPoint3& kV1);
	static bool HalfSpaceBoxTestIntersect(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
		const NiBoundingVolume& kABV1, const NiPoint3& kV1);
	static bool HalfSpaceCapsuleTestIntersect(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
		const NiBoundingVolume& kABV1, const NiPoint3& kV1);
	static bool HalfSpaceHalfSpaceTestIntersect(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
		const NiBoundingVolume& kABV1, const NiPoint3& kV1);
	static bool HalfSpaceTriTestIntersect(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
		const NiPoint3& kVert0, const NiPoint3& kVert1, const NiPoint3& kVert2, const NiPoint3& kV1);
	static bool HalfSpaceBoxFindIntersect(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
		const NiBoundingVolume& kABV1, const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, bool bCalcNormals,
		NiPoint3& kNormal0, NiPoint3& kNormal1);
	static bool HalfSpaceTriFindIntersect(float fTime, const NiBoundingVolume& kABV0, const NiPoint3& kV0,
		const NiPoint3& kVert0, const NiPoint3& kVert1, const NiPoint3& kVert2, const NiPoint3& kV1, float& fIntrTime,
		NiPoint3& kIntrPt, bool bCalcNormals, NiPoint3& kNormal0, NiPoint3& kNormal1);

	NiPlane m_kPlane;
	NiPoint3 m_kCenter;
};
inline constexpr fnvd::Vtable NiHalfSpaceBV_vftable{};
