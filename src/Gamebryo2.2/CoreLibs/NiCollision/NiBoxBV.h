#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoundingVolume.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBox.h"

class NiStream;

// an oriented box bounding volume
class NiBoxBV : public NiBoundingVolume {
public:
	NiBoxBV(const NiPoint3& kExtent, const NiPoint3& kCenter, const NiPoint3& kXAvis, const NiPoint3& kYAvis,
		const NiPoint3& kZAxis);
	NiBoxBV(const NiBoxBV& kABV);

	// No destructor of its own; it uses the NiBoundingVolume one.
	void LoadBinary(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	// shared constant stub
	int32_t Type() const override { return BOX_BV; }
	void UpdateWorldData(const NiBoundingVolume& kModelABV, const NiTransform& kWorld) override;
	NiBoundingVolume* Create() const override;
	NiBoundingVolume* Clone() const override;
	void Copy(const NiBoundingVolume& kABV) override;
	bool operator==(const NiBoundingVolume& kABV) const override;
	// one body shared by every bounding volume: the negated operator==
	bool operator!=(const NiBoundingVolume& kABV) const override;
	void AddViewerStrings(const char* pcPrefix, NiTPrimitiveArray<char*>* pkStrings) const override;

	NiPoint3* GetNormal(NiPoint3* apResult, int32_t iSign0, int32_t iSign1, int32_t iSign2,
		const NiPoint3 kVelocity) const;

	// The loader of ms_apfnLoaders[BOX_BV]: a new unit box at the origin that LoadBinary fills from the stream. Hides
	// NiBoundingVolume's, as in the engine.
	static NiBoundingVolume* CreateFromStream(NiStream& kStream);
	// Whether the triangle (kVert0, kVert1, kVert2) moving from kV0 to kV1 over fTime meets the box kBV0 (an NiBoxBV),
	// by the separating-axis tests.
	static bool BoxTriTestIntersect(float fTime, const NiBoundingVolume& kBV0, const NiPoint3& kV0,
		const NiPoint3& kVert0, const NiPoint3& kVert1, const NiPoint3& kVert2, const NiPoint3& kV1);
	// Whether box kBV0 moving by kV0 and box kBV1 moving by kV1 over fTime meet (both NiBoxBVs), by the
	// separating-axis tests.
	static bool BoxBoxTestIntersect(float fTime, const NiBoundingVolume& kBV0, const NiPoint3& kV0,
		const NiBoundingVolume& kBV1, const NiPoint3& kV1);
	// Whether box kBV0 moving by kV0 and sphere kBV1 (an NiSphereBV) moving by kV1 over fTime meet: the sphere's
	// centre inside the box, else the squared distance from its centre (or its swept segment) to the box.
	static bool BoxSphereTestIntersect(float fTime, const NiBoundingVolume& kBV0, const NiPoint3& kV0,
		const NiBoundingVolume& kBV1, const NiPoint3& kV1);
	static bool BoxCapsuleTestIntersect(float fTime, const NiBoundingVolume& kBV0, const NiPoint3& kV0,
		const NiBoundingVolume& kBV1, const NiPoint3& kV1);
	static bool BoxSphereFindIntersect(float fTime, const NiBoundingVolume& kBV0, const NiPoint3& kV0,
		const NiBoundingVolume& kBV1, const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, bool bCalcNormals,
		NiPoint3& kNormal0, NiPoint3& kNormal1);
	static bool BoxCapsuleFindIntersect(float fTime, const NiBoundingVolume& kBV0, const NiPoint3& kV0,
		const NiBoundingVolume& kBV1, const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, bool bCalcNormals,
		NiPoint3& kNormal0, NiPoint3& kNormal1);
	static bool BoxTriFindIntersect(float fTime, const NiBoundingVolume& kBV0, const NiPoint3& kV0,
		const NiPoint3& kVert0, const NiPoint3& kVert1, const NiPoint3& kVert2, const NiPoint3& kV1, float& fIntrTime,
		NiPoint3& kIntrPt, bool bCalcNormals, NiPoint3& kNormal0, NiPoint3& kNormal1);
	// The sweep of box kBV0 moving by kV0 against box kBV1 moving by kV1 over fTime: false when they never meet,
	// else the first contact's time, point and (with bCalcNormals) unit normals.
	static bool BoxBoxFindIntersect(float fTime, const NiBoundingVolume& kBV0, const NiPoint3& kV0,
		const NiBoundingVolume& kBV1, const NiPoint3& kV1, float& fIntrTime, NiPoint3& kIntrPt, bool bCalcNormals,
		NiPoint3& kNormal0, NiPoint3& kNormal1);
	// Moves kIntrPt0 onto box pkBBV0, then onto box pkBBV1, each at its position at fIntrTime, unless a small sphere
	// around the point already meets that box.
	static void RefinePoint(float fTime, const NiBoxBV* pkBBV0, const NiPoint3* pkV0, const NiBoxBV* pkBBV1,
		const NiPoint3* pkV1, float& fIntrTime, NiPoint3& kIntrPt0);

	NiBox m_kBox;
};

inline float fOneThird = 0.33333334f;
