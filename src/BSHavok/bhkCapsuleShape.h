#pragma once

#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkConvexShape.h"

struct hkCapsuleShapeCinfo;
class hkContactPoint;
class hkpCapsuleShape;
class hkSphere;

// A capsule shape. Only part of the class is declared; its hkpCapsuleShape is held by bhkShape.
class bhkCapsuleShape : public bhkConvexShape {
public:
    bhkCapsuleShape();
    bhkCapsuleShape(const hkCapsuleShapeCinfo& arInfo);
    bhkCapsuleShape(const NiPoint3& arTop, const NiPoint3& arBottom, float afRadius);
    bhkCapsuleShape(const hkVector4& arTop, const hkVector4& arBottom, float afRad);

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    uint32_t QCinfoSize() const override {
        return 0x30;
    }
    void Init(const hkSerializableCinfo& arInfo) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
    void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;
    void BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) override;
    ~bhkCapsuleShape() override;

    // The (top, bottom, radius) overload: builds the Havok capsule from the two axis end points.
    void Init(const hkVector4& arTop, const hkVector4& arBottom, float afRad);
    // Fills apCinfo from the wrapped capsule.
    void GetCinfo(hkCapsuleShapeCinfo* apCinfo);
    bool GetClosestPoints(const hkSphere& arSphere, const hkTransform& arTrans, hkContactPoint& arCPoint);
    hkpCapsuleShape* Gethk() const;
    // The wrapped capsule, or null for a null this.
    hkpCapsuleShape* operator*() const;

    // The class's factory: a new default-built capsule.
    static NiObject* CreateObject();

    static inline uint32_t iCount;
    static NiRTTI ms_RTTI;
};

// The creation info of a capsule shape: the two axis end points (bhkVector4, so the struct sits on Havok's 16-byte
// alignment). bhkCapsuleShape::CreateCinfo allocates the struct, whose size bhkCapsuleShape::QCinfoSize returns.
struct hkCapsuleShapeCinfo : public hkConvexShapeCinfo {
    hkCapsuleShapeCinfo();

    bhkVector4 m_vertexA;
    bhkVector4 m_vertexB;
};

// The info is its own serialized form (bhkSerialized.h): with no pointer member its layout is the file's, so
// bhkSerializable::LoadCInfo's raw read of QCinfoSize() bytes over it is right.
static_assert(offsetof(hkCapsuleShapeCinfo, m_vertexB) == 0x20 && sizeof(hkCapsuleShapeCinfo) == 0x30);

inline hkCapsuleShapeCinfo::hkCapsuleShapeCinfo() {
    m_vertexB.set(1.0, 0.0, 0.0, 0.0);
}
