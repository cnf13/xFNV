#pragma once

#include "BSHavok/bhkShapePhantom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpCachingShapePhantom.h"

#include <stdint.h>

struct hkpWorldRayCastInput;

// A shape phantom that caches its overlaps. Its members and overrides are not declared yet.
class bhkCachingShapePhantom : public bhkShapePhantom {
public:
    bhkCachingShapePhantom();
    bhkCachingShapePhantom(hkShapePhantomCinfo* arInfo);

    uint32_t QCinfoSize() const override {
        return 0x60;
    }
    // Builds the wrapped phantom from the creation info and hands it to the base's SethkObject and PostInit.
    void Init(const hkSerializableCinfo& arInfo) override;
    // Frees the creation info when this phantom made it.
    void KillCinfo(bool abCreated) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
    const NiRTTI* GetRTTI() const override;
    ~bhkCachingShapePhantom() override;
    // Clones into memory charged to the Havok context, under the engine's clone critical section.
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

    // The class's factory: a bare phantom in fresh memory.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    static inline int32_t iCount;
};

NiSmartPointer(bhkCachingShapePhantom);

// The caching phantom's ray-casting form: casts one ray against the cached collision details. It adds no members, and
// only castRay is declared.
class hkpCachingShapePhantomRay : public hkpCachingShapePhantom {
public:
    // Casts arInput against each cached collidable whose filter passes it, the ray and the hit normal transformed
    // into and out of each collidable's space, and keeps the closest hit in arOutput with that collidable as its
    // m_rootCollidable.
    void castRay(const hkpWorldRayCastInput& arInput, hkpWorldRayCastOutput& arOutput);
};
