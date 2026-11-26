#pragma once

#include "BSHavok/bhkCharacterPointCollector.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkSerializable.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxy.h"

#include <stdint.h>

class hkpShapePhantom;
class hkpSurfaceInfo;
struct hkStepInfo;

// The Gamebryo wrapper around a Havok character proxy.
class bhkCharacterProxy : public bhkSerializable {
public:
    bhkCharacterProxy();
    ~bhkCharacterProxy() override;

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
    void Link(bool abAttach) override;

    // The world the proxy's phantom is in, or null.
    const hkpWorld* GethkWorld() const override;
    // Same as GethkWorld.
    hkpWorld* GethkWorldRw() override;

    void KillCinfo(bool abCreated) override;
    uint32_t QCinfoSize() const override {
        return 0x80;
    }
    void Init(const hkSerializableCinfo& arInfo) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

    // The Havok proxy's shape phantom, null without a proxy.
    hkpShapePhantom* GethkShapePhantom();
    // Drops the reference the proxy holds on its shape phantom's wrapper.
    void RembhkRefs();
    // Fills arCinfo from the proxy.
    void GetCinfo(hkSerializableCinfo* apCinfo);
    // Drops the proxy's shape phantom and the manifold's contact points.
    void CleanUp();

    // The proxy's position and velocity in Havok units; both fall back to the zero vector without a proxy.
    const hkVector4& GethkPosition();
    const hkVector4& GethkLinearVelocity();
    // Null-safe wrapped-object getter, typed as the proxy this wrapper holds; returns null for a null this.
    hkpCharacterProxy* operator*();

    // The threading-check marks around a write to the Havok world: each reads the world and does nothing else.
    void MarkWrite();
    void UnMarkWrite();
    // Moves the proxy to arPos and clears its last displacement, inside the write marks.
    void SetPosition(const hkVector4& arPos) {
        hkpCharacterProxy* pProxy = static_cast<hkpCharacterProxy*>(phkObject);
        if (pProxy) {
            MarkWrite();
            pProxy->setPosition(arPos);
            pProxy->m_oldDisplacement.setZero4();
            UnMarkWrite();
        }
    }
    // Gives the proxy a new velocity.
    void SetLinearVelocity(const hkVector4& arVel);
    // Fills arGround with the surface the proxy stands on, probing along arDirection.
    void CheckSupport(const hkVector4& arDirection, hkpSurfaceInfo& arGround);
    // Integrates the proxy over the step arInfo.
    void Integrate(hkStepInfo& arInfo);
    void GetLinearVelocity(NiPoint3& arVel) const;

    bhkCharacterPointCollector m_ignoredCollisionStartCollector;

    static NiRTTI ms_RTTI;
    static inline int32_t iNumObjects;
};

// This getter is const but GethkWorldRw is not; it is called on the const proxy.
inline void bhkCharacterProxy::GetLinearVelocity(NiPoint3& arVel) const {
    hkpCharacterProxy* pProxy = static_cast<hkpCharacterProxy*>(GethkObject());
    if (pProxy) {
        const_cast<bhkCharacterProxy*>(this)->bhkCharacterProxy::GethkWorldRw();
        HK2NI(arVel, pProxy->getLinearVelocity());
        const_cast<bhkCharacterProxy*>(this)->bhkCharacterProxy::GethkWorldRw();
    }
}
