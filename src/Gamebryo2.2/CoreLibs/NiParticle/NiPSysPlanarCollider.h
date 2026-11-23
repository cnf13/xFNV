#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysCollider.h"

class NiAVObject;
class NiCloningProcess;
class NiPSysData;
class NiStream;

// bounces particles off a rectangle.
class NiPSysPlanarCollider : public NiPSysCollider {
public:
    NiPSysPlanarCollider(
        float fBounce,
        bool bSpawnOnCollide,
        bool bDieOnCollide,
        NiPSysSpawnModifier* pkSpawnModifier,
        NiAVObject* pkColliderObj,
        float fWidth,
        float fHeight,
        NiPoint3 kXAxis,
        NiPoint3 kYAxis
    );
    ~NiPSysPlanarCollider() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void ProcessClone(NiCloningProcess& kCloning) override;
    NiPSysCollider*
    Resolve(float fInitialTime, float& fCollisionTime, NiPSysData* pkData, int32_t usCurParticle) override;
    void Update(float fTime, NiPSysData* pkData, int32_t usCurParticle) override;
    void SetResolveConstants() override;

    static NiObject* CreateObject();

    NiAVObject* m_pkColliderObj;
    float m_fWidth;
    float m_fHeight;
    NiPoint3 m_kXAxis;
    NiPoint3 m_kYAxis;
    float m_fHalfSqScaledWidth;
    float m_fHalfSqScaledHeight;
    NiPlane m_kPlane;
    NiPoint3 m_kPosition;
    NiMatrix3 m_kRotation;
    NiTransform m_kLastColliderTransform;
    NiTransform m_kLastPSysTransform;

    static NiRTTI ms_RTTI;
};
