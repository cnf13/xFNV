#pragma once

#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkConvexShape.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

#include <stdint.h>

struct hkCylinderShapeCinfo;

// The wrapper of an hkpCylinderShape. Only its RTTI and the virtuals it overrides are modelled.
class bhkCylinderShape : public bhkConvexShape {
public:
    // The default constructor, run in place on apMemory.
    bhkCylinderShape();
    ~bhkCylinderShape() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void KillCinfo(bool abCreated) override;
    uint32_t QCinfoSize() const override;
    void Init(const hkSerializableCinfo& arInfo) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
    void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;
    void BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) override;

    // Creates the hkpCylinderShape from arTop to arBottom with radius afRad and sets it as
    // this wrapper's object; the virtual Init calls it.
    void Init(const hkVector4& arTop, const hkVector4& arBottom, float afRad);
    // Calls bhkConvexShape::CopyCinfo, then copies the two end points and the cylinder radius into arInfo. CreateCinfo,
    // BuildDisplayGeometry and GetViewerStrings call it.
    void CopyCinfo(hkCylinderShapeCinfo& arInfo);

    // The stream's object factory: a new, unbuilt wrapper.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    static inline int32_t iCount;
};

// The creation info of a cylinder shape: its two end points and the cylinder's radius after the convex radius. The
// vertices are 16-byte aligned.
struct hkCylinderShapeCinfo : public hkConvexShapeCinfo {
    hkCylinderShapeCinfo();

    // Multiplies the two vertices (all four words each) and fCylinderRadius by afScale;
    // bhkCylinderShape::CopyMembers calls it on the clone's cinfo when the cloning
    // scale is not 1.
    void Scale(float afScale);

    bhkVector4 m_vertexA;
    bhkVector4 m_vertexB;
    float fCylinderRadius;
};
