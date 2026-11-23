#pragma once

#include "BSHavok/bhkPhantom.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"

class hkpAabbPhantom;

// The creation info of a box phantom: the phantom's info and the box it covers.
struct hkAabbPhantomCinfo : public hkPhantomCinfo {
    hkAabbPhantomCinfo();
    ~hkAabbPhantomCinfo();

    hkAabb aabb;
};

// A phantom covering an axis aligned box. Only part of the class is declared.
class bhkAabbPhantom : public bhkPhantom {
public:
    bhkAabbPhantom();
    bhkAabbPhantom(const hkAabbPhantomCinfo& arInfo);
    ~bhkAabbPhantom() override;

    uint32_t QCinfoSize() const override {
        return 0x40;
    }
    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void Init(const hkSerializableCinfo& arInfo) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    // Deletes the creation info when this wrapper made it, inside the Havok memory context.
    void KillCinfo(bool abCreated) override;
    void LoadBinary(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

    // The wrapped phantom's box, or an empty box at the origin without one.
    void GetAabb(hkAabb& arB);
    // The wrapped phantom.
    hkpAabbPhantom* operator*() const;
    // The wrapped phantom, without a null test; defined in LoadedAreaBound.cpp.
    hkpAabbPhantom* Gethk();
    // The wrapped phantom's overlapping collidables, or null without one.
    hkArray<hkpCollidable*, hkContainerHeapAllocator>* GetOverlappingCollidables();
    // Fills arInfo's world-object part, then its box from the wrapped phantom. Not
    // virtual: it hides bhkWorldObject::CopyCinfo, which it calls.
    void CopyCinfo(hkAabbPhantomCinfo& arInfo);

    // Builds a bare wrapper.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    // Live wrappers of this class, counted by the constructors.
    static inline uint32_t iCount;
};
