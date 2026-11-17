#pragma once

#include "BSMain/BSMultiBoundAABB.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

class NiPlane;

// an oriented box bound: the AABB extents along the axes in Axis
class BSMultiBoundOBB : public BSMultiBoundAABB {
public:
    BSMultiBoundOBB();
    BSMultiBoundOBB(NiPoint3* a2, NiPoint3* a3, NiMatrix3* axis);
    ~BSMultiBoundOBB() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    BSMultiBoundShape::BSMBShapeType QType() const override {
        return BS_SHAPE_OBB;
    }
    BSMultiBoundShape::BSMBIntersectResult CheckBound(const NiBound& aTargetBound) override;
    BSMultiBoundShape::BSMBIntersectResult CheckBound(const BSMultiBound& aTargetBound) override;
    bool WithinFrustum(const NiFrustumPlanes& aPlanes) override;
    bool CompletelyWithinFrustum(const NiFrustumPlanes& aPlanes) override;
    void GetNiBound(NiBound& aBound) override;
    void MakeDebugGeom(NiLines* apLines, NiTriShape* apShape, NiColorA akBaseColor) override;
    bool QPointWithin(NiPoint3& aPoint) override;

    // the eight corners in BSMultiBoundAABB::GetVertices' order, along the Axis columns
    void GetVertices(NiPoint3* apPoints);
    // the three face planes through the center, one per Axis column
    void GetPlanes(NiPlane* apPlanes);
    // Copies aAxis into Axis.
    void SetAxis(const NiMatrix3& arAxis);

protected:
    void CopyMembers(BSMultiBoundOBB* pDest, NiCloningProcess& arCloneData);

public:
    static NiObject* CreateObject();

    NiMatrix3 Axis;

    static NiRTTI ms_RTTI;
};
