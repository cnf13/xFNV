#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterCtlr.h"

class BSMasterParticleSystem;
class NiCloningProcess;
class NiStream;
struct NiUpdateData;

// an emitter controller that spreads its particles over the emitters of a master particle system.
class BSPSysMultiTargetEmitterCtlr : public NiPSysEmitterCtlr {
public:
    // Used for streaming: no master particle system, no full-emitter cap.
    BSPSysMultiTargetEmitterCtlr();
    BSPSysMultiTargetEmitterCtlr(
        const char* pcModifierName,
        BSMasterParticleSystem* apMasterParticleSystem,
        uint16_t ausMaxFullEmitters
    );
    ~BSPSysMultiTargetEmitterCtlr() override;

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
    void Update(NiUpdateData& UpdateData) override;

protected:
    // Spreads the particles born over [afEmitStart, afEmitStop) between afLastTime and afCurTime over the master's
    // active emitter objects, at most usMaxFullEmitters emitters' worth, through the modifier's volume emitter.
    void
    EmitParticles(float fTime, float fCurTime, float fLastTime, float fEmitStart, float fEmitStop, float fBirthRate);
    void CopyMembers(BSPSysMultiTargetEmitterCtlr* pDest, NiCloningProcess& arCloneData);

public:
    static void SetMaxParticlesPerEmitter(uint16_t asValue);
    // The class loader's factory: a new controller, or null when the allocation fails.
    static NiObject* CreateObject();

    BSMasterParticleSystem* pMasterParticleSystem;
    uint16_t usMaxFullEmitters;

    static NiRTTI ms_RTTI;
    // Cap on the particles one emitter object gets per update.
    static inline uint16_t usMaxParticlesPerEmitterS = 100;
};
