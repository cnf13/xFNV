#pragma once

#include "BSHavok/bhkSphereRepShape.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

struct hkMultiSphereShapeCinfo;

// A shape of several spheres (a rigid body's sphere representation). The Havok shape holds the spheres.
class bhkMultiSphereShape : public bhkSphereRepShape {
public:
    bhkMultiSphereShape();
    bhkMultiSphereShape(const hkMultiSphereShapeCinfo& arInfo);
    ~bhkMultiSphereShape() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void KillCinfo(bool abCreated) override;
    uint32_t QCinfoSize() const override;
    void Init(const hkSerializableCinfo& arInfo) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
    void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;

    // Copies the spheres of the wrapped Havok shape into arCinfo.
    void GetCinfo(hkMultiSphereShapeCinfo& arCinfo);

    // A bare shape with no creation info yet.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    static inline uint32_t iCount;
};

// The creation info of a multi-sphere shape: the sphere-rep shape's, then the spheres.
struct hkMultiSphereShapeCinfo : public hkSphereRepShapeCinfo {
    hkMultiSphereShapeCinfo();
    ~hkMultiSphereShapeCinfo();

    hkArray<hkVector4> m_spheres;
};
