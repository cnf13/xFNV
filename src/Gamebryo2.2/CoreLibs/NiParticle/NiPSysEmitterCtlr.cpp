#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendBoolInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSingleInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitterCtlrData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cfloat>

NiRTTI NiPSysEmitterCtlr::ms_RTTI("NiPSysEmitterCtlr", &NiPSysModifierCtlr::ms_RTTI);

// The last birth rate interpolator is deliberately left unset by the default constructor, as the game does.
NiPSysEmitterCtlr::NiPSysEmitterCtlr()
  : m_spEmitterActiveInterpolator(nullptr), m_fLastScaledTime(-3.4028235e+38f), m_bLastActive(false),
    m_kParticleAges() {}

NiPSysEmitterCtlr::NiPSysEmitterCtlr(const char* apcModifierName)
  : NiPSysModifierCtlr(NiFixedString(apcModifierName)), m_spEmitterActiveInterpolator(nullptr),
    m_pkLastBirthRateInterpolator(nullptr), m_fLastScaledTime(-3.4028235e+38f), m_bLastActive(false),
    m_kParticleAges() {}

// The particle ages' buffer is freed by NiTSet's destructor and the interpolator released by NiPointer's.
NiPSysEmitterCtlr::~NiPSysEmitterCtlr() {}

NiObject* NiPSysEmitterCtlr::CreateClone(NiCloningProcess& kCloning) {
    void* v3 = NiMemObject::operator new(sizeof(NiPSysEmitterCtlr));
    NiPSysEmitterCtlr* v4 = static_cast<NiPSysEmitterCtlr*>(v3);
    if (v3) {
        ::new (v3) NiPSysEmitterCtlr;
        CopyMembers(v4, kCloning);
        return v4;
    }

    CopyMembers(nullptr, kCloning);
    return nullptr;
}

// The emitter active link is read in every file version.
void NiPSysEmitterCtlr::LoadBinary(NiStream& kStream) {
    NiPSysModifierCtlr::LoadBinary(kStream);
    kStream.ReadLinkID();
}

// Files older than 10.1.0.104 link old-style emitter data; its keys move into a new birth rate interpolator and a
// new emitter active interpolator. Newer files link the emitter active interpolator. A looping controller with no
// phase gets a random one in [0, 100], so emitters loaded together do not pulse in step.
void NiPSysEmitterCtlr::LinkObject(NiStream& kStream) {
    NiPSysModifierCtlr::LinkObject(kStream);
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
        NiPointer<NiPSysEmitterCtlrData> spData = static_cast<NiPSysEmitterCtlrData*>(kStream.GetObjectFromLinkID());
        if (!spData) {
            return;
        }

        void* pvMem = NiMemObject::operator new(sizeof(NiFloatInterpolator));
        NiFloatInterpolator* pkBirthRate =
            pvMem ? ::new (pvMem) NiFloatInterpolator(static_cast<NiFloatData*>(nullptr)) : nullptr;
        pkBirthRate->ReplaceKeys(spData->m_pkBirthRateKeys, spData->m_uiNumBirthRateKeys, spData->m_eBirthRateKeyType);
        spData->m_uiNumBirthRateKeys = 0;
        spData->m_pkBirthRateKeys = nullptr;
        spData->m_eBirthRateKeyType = NiAnimationKey::NOINTERP;
        spData->m_ucBirthRateSize = 0;
        SetInterpolator(pkBirthRate, 0);

        pvMem = NiMemObject::operator new(sizeof(NiBoolInterpolator));
        NiBoolInterpolator* pkActive = pvMem ? ::new (pvMem) NiBoolInterpolator(nullptr) : nullptr;
        pkActive->ReplaceKeys(
            spData->m_pkEmitterActiveKeys, spData->m_eEmitterActiveKeyType, spData->m_uiNumEmitterActiveKeys
        );
        spData->m_uiNumEmitterActiveKeys = 0;
        spData->m_pkEmitterActiveKeys = nullptr;
        spData->m_ucEmitterActiveSize = 0;
        SetInterpolator(pkActive, 1);
    }
    else {
        if (!(m_usFlags & CYCLETYPE_MASK) && m_fPhase == 0.0f) {
            m_fPhase = static_cast<float>(rt::rand() / 32767.0 * 100.0);
        }
        m_spEmitterActiveInterpolator = static_cast<NiInterpolator*>(kStream.GetObjectFromLinkID());
    }
}

bool NiPSysEmitterCtlr::RegisterStreamables(NiStream& kStream) {
    if (!NiPSysModifierCtlr::RegisterStreamables(kStream)) {
        return false;
    }

    if (m_spEmitterActiveInterpolator) {
        m_spEmitterActiveInterpolator->RegisterStreamables(kStream);
    }
    return true;
}

void NiPSysEmitterCtlr::SaveBinary(NiStream& kStream) {
    NiPSysModifierCtlr::SaveBinary(kStream);
    kStream.SaveLinkID(m_spEmitterActiveInterpolator);
}

// Both controllers need an emitter active interpolator, or neither; two interpolators must match.
bool NiPSysEmitterCtlr::IsEqual(NiObject* pkObject) {
    if (!NiPSysModifierCtlr::IsEqual(pkObject)) {
        return false;
    }

    NiPSysEmitterCtlr* pkOther = static_cast<NiPSysEmitterCtlr*>(pkObject);
    NiInterpolator* pkOtherInterp = pkOther->m_spEmitterActiveInterpolator;
    if (!pkOtherInterp != !m_spEmitterActiveInterpolator) {
        return false;
    }
    if (pkOtherInterp && !pkOtherInterp->IsEqual(m_spEmitterActiveInterpolator)) {
        return false;
    }
    return true;
}

void NiPSysEmitterCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifierCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    if (m_spEmitterActiveInterpolator) {
        m_spEmitterActiveInterpolator->GetViewerStrings(pkStrings);
    }
}

void NiPSysEmitterCtlr::ProcessClone(NiCloningProcess& kCloning) {
    NiPSysModifierCtlr::ProcessClone(kCloning);
    if (m_spEmitterActiveInterpolator) {
        m_spEmitterActiveInterpolator->ProcessClone(kCloning);
    }
}

// Forgets the last scaled time, so the next Update starts over instead of emitting for the gap.
void NiPSysEmitterCtlr::Stop() {
    m_fLastScaledTime = -FLT_MAX;
    NiTimeController::Stop();
}

// Emits the particles born since the last update. With a posed emitter active value the whole time since the last
// update counts while it is true. With active keys every run of true keys between the last time and now emits at
// the birth rate at the middle of the run, following wraps of a looping controller, for at most 20 runs.
void NiPSysEmitterCtlr::Update(NiUpdateData& UpdateData) {
    if (UpdateData.fTime < m_fLastTime) {
        m_fLastScaledTime = -FLT_MAX;
    }

    if (!m_pkTarget) {
        return;
    }
    NiPSysEmitter* pkEmitter = static_cast<NiPSysEmitter*>(m_pkModifier);
    if (!pkEmitter) {
        return;
    }

    if (m_usFlags & MANAGERCONTROLLED_MASK) {
        m_fScaledTime = NiInterpController::INVALID_TIME;
    }

    if (m_fLastTime != -FLT_MAX && !(m_usFlags & CYCLETYPE_MASK) &&
        UpdateData.fTime - m_fLastTime > pkEmitter->m_fLifeSpan) {
        m_fLastTime = UpdateData.fTime;
    }

    if (DontDoUpdate(UpdateData.fTime)) {
        return;
    }
    if (!m_spEmitterActiveInterpolator || !m_spInterpolator) {
        return;
    }

    float fActiveTime = -FLT_MAX;
    if (NiObject::IsKindOf_0(&NiBlendBoolInterpolator::ms_RTTI, m_spEmitterActiveInterpolator.m_pObject)) {
        NiBlendInterpolator* pkBlend = static_cast<NiBlendInterpolator*>(m_spEmitterActiveInterpolator.m_pObject);
        pkBlend->ComputeNormalizedWeights();
        uint8_t ucIndex = pkBlend->GetHighestWeightedIndex();
        if (ucIndex != NiBlendInterpolator::INVALID_INDEX) {
            fActiveTime = pkBlend->GetTime(ucIndex);
        }
    }
    NiBoolInterpolator* pkActive = GetEmitterActiveInterpolator();
    bool bActiveTime = fActiveTime != -FLT_MAX;

    float fBirthTime = -FLT_MAX;
    if (NiObject::IsKindOf_0(&NiBlendFloatInterpolator::ms_RTTI, m_spInterpolator.m_pObject)) {
        NiBlendInterpolator* pkBlend = static_cast<NiBlendInterpolator*>(m_spInterpolator.m_pObject);
        pkBlend->ComputeNormalizedWeights();
        uint8_t ucIndex = pkBlend->GetHighestWeightedIndex();
        if (ucIndex != NiBlendInterpolator::INVALID_INDEX) {
            fBirthTime = pkBlend->GetTime(ucIndex);
        }
    }
    NiFloatInterpolator* pkBirthRate = GetBirthRateInterpolator();
    bool bBirthTime = fBirthTime != -FLT_MAX;

    if (!pkActive || !pkBirthRate) {
        return;
    }

    // A blended controller runs at the blend's time; the birth rate's time stands for both.
    float fTime = m_fScaledTime;
    if (bActiveTime || bBirthTime) {
        fTime = fBirthTime;
    }

    // A manager switching birth rate interpolators only resyncs, so the switch emits nothing.
    if ((m_usFlags & MANAGERCONTROLLED_MASK) && m_pkLastBirthRateInterpolator != pkBirthRate) {
        m_fLastScaledTime = fTime;
        pkActive->Update(fTime, m_pkTarget, m_bLastActive);
        m_pkLastBirthRateInterpolator = pkBirthRate;
        return;
    }
    m_pkLastBirthRateInterpolator = pkBirthRate;

    uint32_t uiNumKeys;
    NiAnimationKey::KeyType eType;
    uint8_t ucSize;
    NiBoolKey* pkKeys = pkActive->GetKeys(uiNumKeys, eType, ucSize);
    if (uiNumKeys == 0 && !pkActive->GetChannelPosed(0)) {
        return;
    }

    if (pkActive->GetChannelPosed(0)) {
        bool bActive;
        pkActive->Update(0.0f, m_pkTarget, bActive);
        if (!bActive) {
            return;
        }

        float fLastTime = m_fLastScaledTime;
        float fSystemTime = static_cast<NiParticleSystem*>(m_pkTarget)->m_fLastTime;
        m_fLastScaledTime = fTime;

        if (pkBirthRate->GetChannelPosed(0)) {
            float fDeltaTime = fTime - fLastTime;
            if (fLastTime > fTime) {
                fLastTime = 0.0f;
                fDeltaTime = fTime;
            }
            float fBirthRate;
            if (!pkBirthRate->Update(0.0f, m_pkTarget, fBirthRate)) {
                return;
            }
            EmitParticles(fDeltaTime + fSystemTime, fTime, fLastTime, 0.0f, fTime, fBirthRate);
            return;
        }

        uint32_t uiNumBirthKeys;
        NiAnimationKey::KeyType eBirthType;
        uint8_t ucBirthSize;
        NiFloatKey* pkBirthKeys = pkBirthRate->GetKeys(uiNumBirthKeys, eBirthType, ucBirthSize);
        float fFirstKeyTime = pkBirthKeys->m_fTime;
        float fLastKeyTime = pkBirthKeys->GetKeyAt(uiNumBirthKeys - 1, ucBirthSize)->m_fTime;
        float fWrapDelta = 0.0f;
        float fBirthRate;
        // A wrap first emits up to the last key, then from the first key on.
        if (fTime < fLastTime) {
            pkBirthRate->Update(
                static_cast<float>(fLastTime + (fLastKeyTime - fLastTime) * 0.5), m_pkTarget, fBirthRate
            );
            fWrapDelta = fLastKeyTime - fLastTime;
            EmitParticles(fWrapDelta + fSystemTime, fLastKeyTime, fLastTime, fFirstKeyTime, fLastKeyTime, fBirthRate);
            fLastTime = pkBirthKeys->m_fTime;
        }
        pkBirthRate->Update(static_cast<float>(fLastTime + (fTime - fLastTime) * 0.5), m_pkTarget, fBirthRate);
        EmitParticles(
            (fSystemTime + (fTime - fLastTime) + fWrapDelta), fTime, fLastTime, fFirstKeyTime, fTime, fBirthRate
        );
        return;
    }

    // GenInterp's raw byte is kept unnormalised as the run's last state; m_bLastActive is a bool and stores it as 0/1,
    // which differs from the game only for a key byte other than 0 or 1.
    uint32_t uiLastIdx = 0;
    uint8_t ucActive = NiBoolKey::GenInterp(fTime, pkKeys, eType, uiNumKeys, uiLastIdx, ucSize);
    if (m_fLastScaledTime == -FLT_MAX) {
        m_fLastScaledTime = fTime;
        m_bLastActive = ucActive;
        return;
    }

    float fLastTime = m_fLastScaledTime;
    uint8_t ucLastActive = m_bLastActive;
    m_bLastActive = ucActive;
    m_fLastScaledTime = fTime;

    for (int32_t iLoop = 0; iLoop < 20; iLoop++) {
        bool bWrapped = fLastTime > fTime;

        // The run starts at the last key that turned the emitter on at or before the last time, or else at the
        // first key turning it on after the last time.
        uint32_t uiStart;
        if (ucLastActive) {
            for (uiStart = uiNumKeys; uiStart > 0; uiStart--) {
                NiBoolKey* pkKey = pkKeys->GetKeyAt(uiStart - 1, ucSize);
                if (pkKey->m_fTime <= fLastTime && !pkKey->GetBool()) {
                    break;
                }
            }
        }
        else {
            for (uiStart = 0; uiStart < uiNumKeys; uiStart++) {
                NiBoolKey* pkKey = pkKeys->GetKeyAt(uiStart, ucSize);
                if (pkKey->m_fTime > fLastTime && (bWrapped || pkKey->m_fTime <= fTime) && pkKey->GetBool()) {
                    break;
                }
            }
            if (uiStart == uiNumKeys) {
                if (!bWrapped) {
                    return;
                }
                for (uiStart = 0; uiStart < uiNumKeys; uiStart++) {
                    NiBoolKey* pkKey = pkKeys->GetKeyAt(uiStart, ucSize);
                    if (pkKey->m_fTime < fTime && pkKey->GetBool()) {
                        break;
                    }
                }
                if (uiStart == uiNumKeys) {
                    return;
                }
            }
        }

        uint32_t uiEnd = uiStart;
        while (uiEnd < uiNumKeys && pkKeys->GetKeyAt(uiEnd, ucSize)->GetBool()) {
            uiEnd++;
        }
        if (uiEnd == uiNumKeys) {
            uiEnd--;
        }

        NiBoolKey* pkStartKey = pkKeys->GetKeyAt(uiStart, ucSize);
        NiBoolKey* pkEndKey = pkKeys->GetKeyAt(uiEnd, ucSize);

        float fCurTime;
        float fRunLastTime;
        float fEmitStart = pkStartKey->m_fTime;
        float fEmitStop;
        float fBirthRate;
        if (bWrapped) {
            if (pkEndKey->m_fTime > fLastTime) {
                fCurTime = pkKeys->GetKeyAt(uiNumKeys - 1, ucSize)->m_fTime;
                fRunLastTime = fLastTime;
                fEmitStop = pkEndKey->m_fTime;
            }
            else {
                fCurTime = fTime;
                fRunLastTime = pkKeys->m_fTime;
                fEmitStop = pkEndKey->m_fTime < fTime ? pkEndKey->m_fTime : fTime;
            }
            pkBirthRate->Update(
                static_cast<float>(fEmitStart + (fEmitStop - fEmitStart) * 0.5), m_pkTarget, fBirthRate
            );
        }
        else if (pkEndKey->m_fTime < fTime) {
            fCurTime = pkEndKey->m_fTime;
            fRunLastTime = fLastTime;
            fEmitStop = pkEndKey->m_fTime;
            pkBirthRate->Update(
                static_cast<float>(fEmitStart + (fEmitStop - fEmitStart) * 0.5), m_pkTarget, fBirthRate
            );
        }
        else {
            fCurTime = fTime;
            fRunLastTime = fLastTime;
            fEmitStop = pkEndKey->m_fTime;
            pkBirthRate->Update(fTime, m_pkTarget, fBirthRate);
        }

        float fSystemTime = static_cast<NiParticleSystem*>(m_pkTarget)->m_fLastTime;
        if (bWrapped) {
            fSystemTime = m_fHiKeyTime - m_fLoKeyTime + fSystemTime;
        }
        EmitParticles((fSystemTime + fTime - fLastTime), fCurTime, fRunLastTime, fEmitStart, fEmitStop, fBirthRate);

        if (!(fTime > pkEndKey->m_fTime)) {
            return;
        }

        fLastTime = fCurTime;
        ucLastActive = NiBoolKey::GenInterp(fCurTime, pkKeys, eType, uiNumKeys, uiLastIdx, ucSize);
    }
}

const char* NiPSysEmitterCtlr::GetInterpolatorID(uint16_t usIndex) {
    if (usIndex == 0) {
        return ms_pcBirthRate;
    }
    if (usIndex == 1) {
        return ms_pcEmitterActive;
    }
    return nullptr;
}

uint16_t NiPSysEmitterCtlr::GetInterpolatorIndex(const char* pcID) const {
    if (NiStricmp(pcID, ms_pcBirthRate) == 0) {
        return 0;
    }
    if (NiStricmp(pcID, ms_pcEmitterActive) == 0) {
        return 1;
    }
    return NiInterpController::INVALID_INDEX;
}

NiInterpolator* NiPSysEmitterCtlr::GetInterpolator(uint16_t usIndex) const {
    if (usIndex == 0) {
        return m_spInterpolator;
    }
    if (usIndex == 1) {
        return m_spEmitterActiveInterpolator;
    }
    return nullptr;
}

void NiPSysEmitterCtlr::SetInterpolator(NiInterpolator* pkInterpolator, uint16_t usIndex) {
    if (usIndex == 0) {
        m_spInterpolator = pkInterpolator;
    }
    else if (usIndex == 1) {
        m_spEmitterActiveInterpolator.SetNew(pkInterpolator);
    }
}

// The key range covers both interpolators; a manager-controlled controller keeps its range.
void NiPSysEmitterCtlr::ResetTimeExtrema() {
    if (m_usFlags & MANAGERCONTROLLED_MASK) {
        return;
    }

    m_spInterpolator->GetActiveTimeRange(m_fLoKeyTime, m_fHiKeyTime);
    float fBeginKeyTime;
    float fEndKeyTime;
    m_spEmitterActiveInterpolator->GetActiveTimeRange(fBeginKeyTime, fEndKeyTime);
    if (fBeginKeyTime < m_fLoKeyTime) {
        m_fLoKeyTime = fBeginKeyTime;
    }
    if (fEndKeyTime > m_fHiKeyTime) {
        m_fHiKeyTime = fEndKeyTime;
    }
}

// Poses a zero birth rate or an inactive emitter.
NiInterpolator* NiPSysEmitterCtlr::CreatePoseInterpolator(uint16_t usIndex) {
    if (usIndex == 0) {
        void* pvMem = NiMemObject::operator new(sizeof(NiFloatInterpolator));
        return pvMem ? ::new (pvMem) NiFloatInterpolator(0.0f) : nullptr;
    }
    if (usIndex == 1) {
        void* pvMem = NiMemObject::operator new(sizeof(NiBoolInterpolator));
        return pvMem ? ::new (pvMem) NiBoolInterpolator(false) : nullptr;
    }
    return nullptr;
}

NiBlendInterpolator* NiPSysEmitterCtlr::CreateBlendInterpolator(
    uint16_t usIndex,
    int32_t bManagerControlled,
    bool,
    float fWeightThreshold,
    int32_t ucArraySize
) const {
    if (usIndex == 0) {
        void* pvMem = NiMemObject::operator new(sizeof(NiBlendFloatInterpolator));
        return pvMem ? ::new (pvMem) NiBlendFloatInterpolator(bManagerControlled, fWeightThreshold, ucArraySize)
                     : nullptr;
    }
    if (usIndex == 1) {
        void* pvMem = NiMemObject::operator new(sizeof(NiBlendBoolInterpolator));
        return pvMem ? ::new (pvMem) NiBlendBoolInterpolator(bManagerControlled, fWeightThreshold, ucArraySize)
                     : nullptr;
    }
    return nullptr;
}

void NiPSysEmitterCtlr::GuaranteeTimeRange(float fStartTime, float fEndTime) {
    NiSingleInterpController::GuaranteeTimeRange(fStartTime, fEndTime);
    if (m_spEmitterActiveInterpolator) {
        m_spEmitterActiveInterpolator->GuaranteeTimeRange(fStartTime, fEndTime);
    }
}

// The target must also hold the named modifier, and it must be an emitter.
bool NiPSysEmitterCtlr::TargetIsRequiredType() const {
    if (!NiSingleInterpController::TargetIsRequiredType()) {
        return false;
    }

    NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(m_pkTarget)->GetModifierByName(m_kModifierName);
    return NiObject::IsKindOf(&NiPSysEmitter::ms_RTTI, pkModifier);
}

// The birth rate takes float interpolators, the emitter active flag bool interpolators, or blends of them.
bool NiPSysEmitterCtlr::InterpolatorIsCorrectType(NiInterpolator* pkInterp, uint16_t usIndex) const {
    if (usIndex == 0) {
        return pkInterp &&
            (pkInterp->IsKindOf(&NiFloatInterpolator::ms_RTTI) ||
             pkInterp->IsKindOf(&NiBlendFloatInterpolator::ms_RTTI));
    }
    if (usIndex == 1) {
        if (pkInterp && pkInterp->IsKindOf(&NiBoolInterpolator::ms_RTTI)) {
            return true;
        }
        return NiObject::IsKindOf_0(&NiBlendBoolInterpolator::ms_RTTI, pkInterp);
    }
    return false;
}

// The target must also hold the named modifier, and it must be an emitter.
bool NiPSysEmitterCtlr::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    if (!NiPSysModifierCtlr::InterpTargetIsCorrectType(pkTarget)) {
        return false;
    }

    NiPSysModifier* pkModifier = static_cast<NiParticleSystem*>(pkTarget)->GetModifierByName(m_kModifierName);
    return NiObject::IsKindOf(&NiPSysEmitter::ms_RTTI, pkModifier);
}

// A debug-built copy of the inline from Precipitation.cpp: the casts and the blend lookup are calls.
NiFloatInterpolator* NiPSysEmitterCtlr::GetBirthRateInterpolator() {
    NiFloatInterpolator* pkFloat = static_cast<NiFloatInterpolator*>(
        NiObject::DynamicCastStatic(&NiFloatInterpolator::ms_RTTI, m_spInterpolator.m_pObject)
    );
    if (pkFloat) {
        return pkFloat;
    }

    NiBlendInterpolator* pkBlend = static_cast<NiBlendInterpolator*>(
        NiObject::DynamicCastStatic(&NiBlendFloatInterpolator::ms_RTTI, m_spInterpolator.m_pObject)
    );
    if (!pkBlend) {
        return nullptr;
    }
    uint8_t ucIndex = pkBlend->GetHighestWeightedIndex();
    if (ucIndex == NiBlendInterpolator::INVALID_INDEX) {
        return nullptr;
    }
    return static_cast<NiFloatInterpolator*>(pkBlend->GetInterpolator(ucIndex));
}

NiBoolInterpolator* NiPSysEmitterCtlr::GetEmitterActiveInterpolator() {
    if (NiBoolInterpolator* pkBool = m_spEmitterActiveInterpolator->IsNiBoolInterpolator()) {
        return pkBool;
    }

    NiInterpolator* pkInterp = m_spEmitterActiveInterpolator.m_pObject;
    if (!pkInterp) {
        return nullptr;
    }
    NiBlendInterpolator* pkBlend =
        static_cast<NiBlendInterpolator*>(pkInterp->DynamicCastMember(&NiBlendBoolInterpolator::ms_RTTI));
    if (!pkBlend) {
        return nullptr;
    }
    uint8_t ucIndex = pkBlend->GetHighestWeightedIndex();
    if (ucIndex == NiBlendInterpolator::INVALID_INDEX) {
        return nullptr;
    }
    // EXE-INLINED 0x495D50
    if (pkBlend->m_ucInterpCount == 1 && ucIndex == pkBlend->m_ucSingleIdx) {
        return static_cast<NiBoolInterpolator*>(pkBlend->m_pkSingleInterpolator);
    }
    return static_cast<NiBoolInterpolator*>(pkBlend->m_pkInterpArray[ucIndex].m_spInterpolator.m_pObject);
}

void NiPSysEmitterCtlr::CopyMembers(NiPSysEmitterCtlr* pkDest, NiCloningProcess& kCloning) {
    NiPSysModifierCtlr::CopyMembers(pkDest, kCloning);
    NiInterpolator* pkInterpolator = m_spEmitterActiveInterpolator.m_pObject;
    if (pkInterpolator) {
        NiInterpolator* pkClone = static_cast<NiInterpolator*>(pkInterpolator->CreateClone(kCloning));
        pkDest->m_spEmitterActiveInterpolator = pkClone;
    }
    if (!(pkDest->m_usFlags & CYCLETYPE_MASK) && pkDest->m_fPhase == 0.0f) {
        pkDest->m_fPhase = static_cast<float>(rt::rand() / 32767.0 * 100.0);
    }
}

// Ages are spaced 1 / fBirthRate apart, the newest born at fCurTime.
void NiPSysEmitterCtlr::EmitParticles(
    float fTime,
    float fCurTime,
    float fLastTime,
    float fEmitStart,
    float fEmitStop,
    float fBirthRate
) {
    if (!(fBirthRate > 0.0f) || !(fEmitStop > fEmitStart) || !(fCurTime > fEmitStart) || !(fLastTime < fEmitStop)) {
        return;
    }

    if (fCurTime > fEmitStop) {
        fCurTime = fEmitStop;
    }
    float fCurDelta = fCurTime - fEmitStart;
    float fLastDelta = 0.0f;
    if (fLastTime >= fEmitStart) {
        fLastDelta = fLastTime - fEmitStart;
    }

    uint16_t usLast = static_cast<uint16_t>(static_cast<int32_t>(fLastDelta * fBirthRate));
    uint16_t usCur = static_cast<uint16_t>(static_cast<int32_t>(fCurDelta * fBirthRate));
    uint16_t usNumParticles = static_cast<uint16_t>(usCur - usLast);
    if (usNumParticles > sLimitParticlesS) {
        usNumParticles = sLimitParticlesS;
    }

    float fInvBirthRate = 1.0f / fBirthRate;
    m_kParticleAges.RemoveAll();
    for (uint16_t us = 0; us < usNumParticles; us++) {
        usLast++;
        m_kParticleAges.Add(fCurDelta - usLast * fInvBirthRate);
    }

    static_cast<NiPSysEmitter*>(m_pkModifier)->EmitParticles(fTime, usNumParticles, m_kParticleAges.GetBase());
}

void NiPSysEmitterCtlr::SetParticlesLimit(uint16_t asLimit) {
    NiPSysEmitterCtlr::sLimitParticlesS = (uint16_t)((int16_t)asLimit);
}

NiObject* NiPSysEmitterCtlr::CreateObject() {
    void* v0 = NiMemObject::operator new(sizeof(NiPSysEmitterCtlr));
    if (!v0) {
        return nullptr;
    }
    return ::new (v0) NiPSysEmitterCtlr;
}
