#pragma once

#include "BSMain/BSMultiBoundShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// a capsule bound between PointA and PointB; most of its tests are stubs in this build
class BSMultiBoundCapsule : public BSMultiBoundShape {
public:
    BSMultiBoundCapsule();
    ~BSMultiBoundCapsule() override;

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
        return BS_SHAPE_CAPSULE;
    }
    float QMultiBoundRadius() const override {
        return Radius;
    }
    BSMultiBoundShape::BSMBIntersectResult CheckBound(const NiBound& aBound) override;
    BSMultiBoundShape::BSMBIntersectResult CheckBound(const BSMultiBound& aTargetBound) override;
    bool WithinFrustum(const NiFrustumPlanes&) override {
        return false;
    }
    bool CompletelyWithinFrustum(const NiFrustumPlanes&) override {
        return false;
    }
    void GetNiBound(NiBound&) override {}
    void MakeDebugGeom(NiLines*, NiTriShape*, NiColorA) override {}
    // shared "return 2" and "return 3" stubs
    int32_t QDebugGeomLineSize() override {
        return 2;
    }
    int32_t QDebugGeomShapeSize() override {
        return 3;
    }

protected:
    void CopyMembers(BSMultiBoundCapsule* pDest, NiCloningProcess& arCloneData);

public:
    static NiObject* CreateObject();

    NiPoint3 PointA;
    NiPoint3 PointB;
    float Radius;

    static NiRTTI ms_RTTI;
};
