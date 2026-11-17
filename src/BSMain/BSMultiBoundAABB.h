#pragma once

#include "BSMain/BSMultiBoundShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// an axis-aligned box bound given by its center and half extents
class BSMultiBoundAABB : public BSMultiBoundShape {
public:
	BSMultiBoundAABB();
	BSMultiBoundAABB(NiPoint3* Center, NiPoint3* HalfExtents);
	~BSMultiBoundAABB() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	BSMultiBoundShape::BSMBShapeType QType() const override { return BS_SHAPE_AABB; }
	float QMultiBoundRadius() const override;
	BSMultiBoundShape::BSMBIntersectResult CheckBound(const NiBound& aTargetBound) override;
	BSMultiBoundShape::BSMBIntersectResult CheckBound(const BSMultiBound& aTargetBound) override;
	bool WithinFrustum(const NiFrustumPlanes& aPlanes) override;
	bool CompletelyWithinFrustum(const NiFrustumPlanes& aPlanes) override;
	void GetNiBound(NiBound& aBound) override;
	void MakeDebugGeom(NiLines* apLines, NiTriShape* apShape, NiColorA akBaseColor) override;
	int32_t QDebugGeomLineSize() override { return 24; }
	int32_t QDebugGeomShapeSize() override { return 36; }
	bool QPointWithin(NiPoint3& kpoint) override;
	void SetCenter(const NiPoint3& arCenter) override { Center = arCenter; }

	// the eight corners, +X+Y+Z first, then with -Z, -Y and -X varying fastest to slowest
	void GetVertices(NiPoint3* apPoints);
	bool FastPointWithin(NiPoint3 kpoint);
	// Defined in TES.cpp.
	const NiPoint3& QCenter() const;
	// Copies aHalfExtents into HalfExtents.
	void SetHalfExtents(const NiPoint3& arHalfExtents);

protected:
	void CopyMembers(BSMultiBoundAABB* pDest, NiCloningProcess& arCloneData);

public:
	static NiObject* CreateObject();

	NiPoint3 Center;
	NiPoint3 HalfExtents;

	static NiRTTI ms_RTTI;
};
