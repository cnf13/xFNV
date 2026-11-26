#pragma once

#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkPhantom.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpSimpleShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include <stdint.h>

class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpLinearCastInput;
class NiTransform;

// The creation info of a shape phantom: the phantom's info and where the shape sits.
struct hkShapePhantomCinfo : public hkPhantomCinfo {
    // The serialized form (bhkSerialized.h): the 32-bit layout, 0x60 bytes (hkPhantomCinfo adds nothing to the world
    // object's form).
    struct Serialized : hkWorldObjectCinfo::Serialized {
        uint8_t padding18[8];
        bhkTransform m_transform;
    };

    hkShapePhantomCinfo();
    ~hkShapePhantomCinfo();

    // Fills the info from its serialized form; the shape is left null (LinkObject binds it).
    void ReadSerialized(const Serialized& arForm) {
        hkWorldObjectCinfo::ReadSerialized(arForm);
        m_transform = arForm.m_transform;
    }

    // The constructor builds it with bhkTransform's constructor and setIdentity, and the Init bodies convert it with
    // bhkTransform::ToHK.
    bhkTransform m_transform;
};

static_assert(offsetof(hkShapePhantomCinfo::Serialized, m_transform) == 0x20);
static_assert(sizeof(hkShapePhantomCinfo::Serialized) == 0x60);

// A phantom with a Havok shape. Only part of the class is declared.
class bhkShapePhantom : public bhkPhantom {
public:
    // The class does not override the base's virtual: its own CopyCinfo below takes the phantom's creation info.
    using bhkSerializable::CopyCinfo;

    bhkShapePhantom();

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    ~bhkShapePhantom() override;

    // The contact pairs of the phantom, stored in apCollector.
    void GetPenetrations(hkpCdBodyPairCollector* arCollector);
    // The phantom's world transform, or nothing without a Havok phantom.
    void GetTransform(hkTransform& arTrans);
    // The phantom's world transform in game units: the Havok transform converted into
    // arTrans's rotation and translation.
    void GetTransform(NiTransform& arTrans);

    // Moves the phantom to arTrans, converted to Havok units.
    void SetTransform(const NiTransform& arTrans);
    // Moves the Havok phantom to arPos inside the world's write marks.
    void SetPosition(const hkVector4& arPos);
    // Moves the Havok phantom to arPos, converted from game units.
    void SetPosition(const NiPoint3& arPos);
    // Runs the wrapped phantom's linear cast from arPos, collecting the start overlaps in arPnts and the hits in
    // apStart, inside the shape's critical section and the world's write marks.
    void SetPositionAndLinearCast(
        const hkVector4& arPos,
        const hkpLinearCastInput& arIn,
        hkpCdPointCollector& arPnts,
        hkpCdPointCollector* apStart
    );

protected:
    // Fills apInfo from the wrapped phantom.
    void CopyCinfo(hkShapePhantomCinfo& arInfo);

public:
    void setTransform(hkTransform* apTransform);

    // The wrapper apObj's user data points back to, or null for a null object.
    static bhkShapePhantom* Getbhk(const hkpWorldObject* apObj) {
        return apObj ? static_cast<bhkShapePhantom*>(apObj->getUserData()) : nullptr;
    }

    static NiRTTI ms_RTTI;
    static inline int32_t iCount;
};

inline void bhkShapePhantom::SetPosition(const hkVector4& arPos) {
    hkpShapePhantom* pPhantom = static_cast<hkpShapePhantom*>(**this);
    if (pPhantom) {
        MarkWrite();
        pPhantom->setPosition(arPos, 0.0f);
        UnMarkWrite();
    }
}

inline bhkShapePhantom::bhkShapePhantom() {
    ++iCount;
    ResetPhantomFlags();
}

inline void bhkShapePhantom::SetTransform(const NiTransform& arTrans) {
    hkTransform hkt;
    hkTransform* pTransform = &NI2HK(hkt, arTrans);
    bhkShapePhantom::setTransform(pTransform);
}

inline void bhkShapePhantom::GetPenetrations(hkpCdBodyPairCollector* arCollector) {
    hkpSimpleShapePhantom* pphantom = static_cast<hkpSimpleShapePhantom*>(**this);
    if (pphantom && pphantom->m_world) {
        pphantom->getPenetrations(*arCollector, nullptr);
    }
}
