#pragma once

#include "BSHavok/bhkConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"

#include <stdint.h>

// A sphere shape. Only part of the class is declared.
class bhkSphereShape : public bhkConvexShape {
public:
    bhkSphereShape();
    bhkSphereShape(float afRadius, bool abBS);
    ~bhkSphereShape() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    uint32_t QCinfoSize() const override;
    // The virtual one takes a creation info; the overload below makes one from a radius. Register either with the
    // _OVERLOAD form: `&bhkSphereShape::Init` alone is ambiguous.
    void Init(const hkSerializableCinfo& arInfo) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
    void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;
    void BuildDisplayGeometry(NiNode* apNode, const NiColor& apColor) override;

    void Init(float afRadius);
    hkpSphereShape* Gethk() const;
    // The wrapped sphere, or null for a null this.
    hkpSphereShape* operator*() const;

    // A bare shape with no creation info.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    static inline int32_t iCount;
};

// The creation info of a sphere shape; it adds no fields to hkConvexShapeCinfo.
struct hkSphereShapeCinfo : public hkConvexShapeCinfo {
    hkSphereShapeCinfo();
};

// The info is its own serialized form (bhkSerialized.h): with no pointer member its layout is the file's, so
// bhkSerializable::LoadCInfo's raw read of QCinfoSize() bytes over it is right.
static_assert(sizeof(hkSphereShapeCinfo) == 0x8);
