#pragma once

#include "BSHavok/bhkPhantom.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"

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

inline void bhkAabbPhantom::GetAabb(hkAabb& arB) {
    hkpAabbPhantom* pPhantom = **this;
    if (pPhantom) {
        MarkRead();
        arB = pPhantom->m_aabb;
        UnMarkRead();
    }
    else {
        const hkVector4& rMax = hkVector4::getZero();
        const hkVector4& rMin = hkVector4::getZero();
        hkAabb kZeroAabb(rMin, rMax);
        arB = kZeroAabb;
    }
}

inline hkpAabbPhantom* bhkAabbPhantom::Gethk() {
    return static_cast<hkpAabbPhantom*>(phkObject);
}

// A box phantom wraps an hkpAabbPhantom; its overlap array's getter is a trivial accessor (this + 0xD0).
inline hkArray<hkpCollidable*, hkContainerHeapAllocator>* bhkAabbPhantom::GetOverlappingCollidables() {
    hkpAabbPhantom* pphantom = static_cast<hkpAabbPhantom*>(**this);
    if (pphantom) {
        return &pphantom->m_overlappingCollidables;
    }

    return nullptr;
}
