#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiCloningProcess;
class NiPSysColliderManager;
class NiPSysData;
class NiPSysSpawnModifier;
class NiStream;

NiSmartPointer(NiPSysCollider);

// base of the particle colliders; the colliders of a manager form a linked list.
class NiPSysCollider : public NiObject {
public:
    NiPSysCollider(float fBounce, bool bSpawnOnCollide, bool bDieOnCollide, NiPSysSpawnModifier* pkSpawnModifier);
    ~NiPSysCollider() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void ProcessClone(NiCloningProcess& kCloning) override;
    // The collider the particle hits first, or null. Pure, but it has a body that derived colliders call last: it only
    // asks the rest of the list (m_spNext).
    virtual NiPSysCollider*
    Resolve(float fInitialTime, float& fCollisionTime, NiPSysData* pkData, int32_t usCurParticle) = 0;
    virtual void Update(float fTime, NiPSysData* pkData, int32_t usCurParticle);
    virtual void SetResolveConstants() = 0;

    // NiObject's copy, then the bounce, the spawn and die flags and a clone of the next
    // collider into pkDest when cloning.
    void CopyMembers(NiPSysCollider* pkDest, NiCloningProcess& kCloning);

    float m_fBounce;
    bool m_bSpawnOnCollide;
    bool m_bDieOnCollide;
    NiPSysSpawnModifier* m_pkSpawnModifier;
    NiPoint3 m_kCollisionPoint;
    float m_fCollisionTime;
    NiPSysColliderManager* m_pkManager;
    NiPointer<NiPSysCollider> m_spNext;

    static NiRTTI ms_RTTI;
};
