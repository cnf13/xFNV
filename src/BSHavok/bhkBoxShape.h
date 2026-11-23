#pragma once

#include "BSHavok/bhkConvexShape.h"
#include "Fallout Shared/GlobalFunc.h"

#include <stdint.h>

// A box shape. Only part of the class is declared.
class bhkBoxShape : public bhkConvexShape {
public:
    bhkBoxShape();
    bhkBoxShape(const hkVector4& arExt);
    bhkBoxShape(const NiPoint3& arExt);
    ~bhkBoxShape() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void KillCinfo(bool abCreated) override;
    uint32_t QCinfoSize() const override {
        return 0x20;
    }
    void Init(const hkSerializableCinfo& arInfo) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
    void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;
    void BuildDisplayGeometry(NiNode* apNode, const NiColor& apColor) override;

    // The half extents arExt, in Havok units, as a new creation info handed to the virtual Init.
    void Init(const hkVector4& arExt);
    NiPoint3 GetHalfExtents();
    const hkVector4& GethkHalfExtents();

    // Factory: returns a new default-constructed box shape, or null when the allocation fails.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    // The initialisation guard of bhkBoxShape::BuildDisplayGeometry's dbgGeomNameSC, the debug geometry's name (bit 0).
    static inline uint32_t iCount;
};

// The creation info of a box shape: the half extents (a bhkVector4, so the struct sits on Havok's 16-byte alignment).
struct hkBoxShapeCinfo : public hkConvexShapeCinfo {
    hkBoxShapeCinfo();

    void Scale(float afScale) {
        m_halfExtents.fX = m_halfExtents.fX * afScale;
        m_halfExtents.fY = m_halfExtents.fY * afScale;
        m_halfExtents.fZ = m_halfExtents.fZ * afScale;
        m_halfExtents.fW = afScale * m_halfExtents.fW;
    }

    bhkVector4 m_halfExtents;
};

// The info is its own serialized form (bhkSerialized.h): with no pointer member its layout is the file's, so
// bhkSerializable::LoadCInfo's raw read of QCinfoSize() bytes over it is right.
static_assert(offsetof(hkBoxShapeCinfo, m_halfExtents) == 0x10 && sizeof(hkBoxShapeCinfo) == 0x20);
