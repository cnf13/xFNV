#pragma once

#include "BSMain/BSMultiBoundShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// a sphere bound
class BSMultiBoundSphere : public BSMultiBoundShape {
public:
	BSMultiBoundSphere();
	BSMultiBoundSphere(NiPoint3* apCenter, float afRadius);
	~BSMultiBoundSphere() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	BSMultiBoundShape::BSMBShapeType QType() const override { return BS_SHAPE_SPHERE; }
	float QMultiBoundRadius() const override { return Radius; }
	BSMultiBoundShape::BSMBIntersectResult CheckBound(const NiBound& aTargetBound) override;
	BSMultiBoundShape::BSMBIntersectResult CheckBound(const BSMultiBound& aTargetBound) override;
	bool WithinFrustum(const NiFrustumPlanes& aPlanes) override;
	bool CompletelyWithinFrustum(const NiFrustumPlanes& aPlanes) override;
	void GetNiBound(NiBound& aBound) override;
	void MakeDebugGeom(NiLines*, NiTriShape*, NiColorA) override {}
	// shared "return 2" and "return 3" stubs
	int32_t QDebugGeomLineSize() override { return 2; }
	int32_t QDebugGeomShapeSize() override { return 3; }
	bool QPointWithin(NiPoint3& aPoint) override;
	void SetCenter(const NiPoint3& arCenter) override { Center = arCenter; }

	// Stores afRadius in Radius.
	void SetRadius(float afRadius);

protected:
	void CopyMembers(BSMultiBoundSphere* pDest, NiCloningProcess& arCloneData);

public:
	static NiObject* CreateObject();

	NiPoint3 Center;
	float Radius;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(BSMultiBoundSphere);
