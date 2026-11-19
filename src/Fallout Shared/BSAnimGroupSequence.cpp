#include "Fallout Shared/BSAnimGroupSequence.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSAudio/BSWin32Audio.h"
#include "BSCore/BSString.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/NiBSBoneLODController.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BSTransCon.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMultiTargetTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKeyExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDefaultAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

NiRTTI BSAnimGroupSequence::ms_RTTI("BSAnimGroupSequence", &NiControllerSequence::ms_RTTI);

BSAnimGroupSequence::BSAnimGroupSequence() : spAnimGroup(nullptr) {}

BSAnimGroupSequence::BSAnimGroupSequence(TESAnimGroup* apAnimGroup, NiControllerSequence* apSeq)
  : NiControllerSequence(apSeq->m_kName, 0, 1u), spAnimGroup(apAnimGroup) {
    m_spTextKeys = apSeq->GetTextKeys();
    m_pkAccumRoot = apSeq->m_pkAccumRoot;
    SetAccumRootName(apSeq->m_kAccumRootName);
    NiCloningProcess kCloning(257u);
    static_cast<BSAnimGroupSequence*>(apSeq)->BSAnimGroupSequence::CopyMembers(this, kCloning);
}

BSAnimGroupSequence::~BSAnimGroupSequence() {}

const NiRTTI* BSAnimGroupSequence::GetRTTI() const {
    return &ms_RTTI;
}

// The clone is a plain NiControllerSequence: the animation group is not cloned.
NiObject* BSAnimGroupSequence::CreateClone(NiCloningProcess& arCloneData) {
    return NiControllerSequence::CreateClone(arCloneData);
}

void BSAnimGroupSequence::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiControllerSequence::GetViewerStrings(pkStrings);
}

bool BSAnimGroupSequence::Deactivate(float afEaseOutTime, bool abTransition) {
    // The holster and unequip groups put back the objects their controllers drove, through the bone LOD controller
    // the sequence's accumulation root carries.
    if (m_pkAccumRoot) {
        const int32_t eType = spAnimGroup->GetType();
        if (eType >= 0x17 && eType <= 0x19) {
            NiBSBoneLODController* pBoneLOD =
                static_cast<NiBSBoneLODController*>(m_pkAccumRoot->GetController(&NiBSBoneLODController::ms_RTTI));
            if (pBoneLOD && pBoneLOD->GetBoneLOD() != 0) {
                NiTimeController* pController =
                    m_pkAccumRoot->GetParent()->GetController(&NiMultiTargetTransformController::ms_RTTI);
                // BSTransCon adds no data to the multi-target transform controller the RTTI test found.
                if (pController) {
                    static_cast<BSTransCon*>(pController)->UpdateAllButTarget(m_fLastTime, true);
                }
            }
        }
    }
    return NiControllerSequence::Deactivate(afEaseOutTime, abTransition);
}

void BSAnimGroupSequence::LoadGame(float afTime) {
    float apData;
    pSaveLoadGameOLD->LoadGameDataOLD(&apData, 4u);
    pSaveLoadGameOLD->LoadGameDataOLD(&m_eState, 4u);
    if (((uint32_t)((int32_t)pSaveLoadGameOLD->GetCurrentVersion())) < 0x71u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&apData, 4u);
    }
    m_fOffset = (apData - afTime);
    pSaveLoadGameOLD->LoadGameDataOLD(&m_fLastTime, 4u);
    pSaveLoadGameOLD->LoadGameDataOLD(&m_fWeightedLastTime, 4u);
    pSaveLoadGameOLD->LoadGameDataOLD(&m_fDestFrame, 4u);
    m_fStartTime = (-0x1.fffffe0000000p+127f);
    m_fEndTime = (-0x1.fffffe0000000p+127f);
    GetScaledTime(afTime);
}

void BSAnimGroupSequence::LoadGame(BGSLoadGameBuffer* apBuffer, float afTime) {
    float apData;
    uint8_t cstate;
    apData = 0.0f;
    apBuffer->LoadData(&apData, 4u);
    m_fOffset = apData - afTime;
    cstate = 0;
    apBuffer->LoadData(&cstate, 1u);
    m_eState = static_cast<NiControllerSequence::State>(cstate);
    apBuffer->LoadData(&m_fLastTime, 4u);
    apBuffer->LoadData(&m_fWeightedLastTime, 4u);
    apBuffer->LoadData(&m_fDestFrame, 4u);
    m_fStartTime = -3.4028235e+38f;
    m_fEndTime = -3.4028235e+38f;
    GetScaledTime(afTime);
}

TESAnimGroup* BSAnimGroupSequence::GetAnimGroup() {
    return spAnimGroup;
}

uint16_t BSAnimGroupSequence::GetSaveSize() {
    return BSAnimGroupSequence::GetSequenceSaveSize();
}

NiFixedString* BSAnimGroupSequence::GetRootNodeNamePtr() {
    return &m_kAccumRootName;
}

bool BSAnimGroupSequence::ResolveTransformInterpolators(NiAVObject* apNode, uint32_t aiTransformControllerCount) {
    NiAVObjectPalette* objectPalette = m_pkOwner->GetObjectPalette();
    return (bool)((uint8_t)NiControllerSequence::ResolveTransformInterpolators(
        apNode, objectPalette, aiTransformControllerCount
    ));
}

double BSAnimGroupSequence::GetKeyLengthOverFrequency() {
    return ((m_fEndKeyTime - m_fBeginKeyTime) / m_fFrequency);
}

uint8_t BSAnimGroupSequence::GetPriority(const char* apBoneName, uint32_t* apReturnIndex) {
    uint32_t i;
    for (i = 0;; ++i) {
        if (i >= m_uiArraySize) {
            return 0;
        }
        NiFixedString* v3 = &(&m_pkIDTagArray[i])->m_kAVObjectName;
        const char* pname = v3->GetHandleString();
        if (!pname) {
            Warning("NULL object name found in tag array for sequence '%s'.", (&m_kName)->GetHandleString());
            continue;
        }
        if (!BSstricmp(pname, apBoneName)) {
            break;
        }
    }
    if (apReturnIndex) {
        *apReturnIndex = i;
    }
    return m_pkInterpArray[i].m_ucPriority;
}

void BSAnimGroupSequence::SetInterpolatorPriority(uint32_t a2, uint8_t a3) {
    if (a2 < m_uiArraySize) {
        m_pkInterpArray[a2].m_ucPriority = a3;
    }
}

uint8_t BSAnimGroupSequence::GetInterpolatorPriority(uint32_t a2) {
    if (a2 >= m_uiArraySize) {
        return (uint8_t)(-1);
    }

    return m_pkInterpArray[a2].m_ucPriority;
}

void BSAnimGroupSequence::SetAnimGroup(TESAnimGroup* apAnimGroup) {
    spAnimGroup = apAnimGroup;
}

void BSAnimGroupSequence::ClearBlendInterp() {
    uint32_t v3 = 0;
    for (uint32_t i = m_uiArraySize; v3 < i; i = m_uiArraySize) {
        m_pkInterpArray[v3++].m_pkBlendInterp = nullptr;
    }
}

float BSAnimGroupSequence::GetScaledTime(float afTime) {
    int32_t eState = (int32_t)m_eState;
    if ((eState <= 0) || (eState > 3)) {
        return 0.0;
    }
    float fTime = afTime + m_fOffset;
    return NiControllerSequence::ComputeScaledTime(fTime, true);
}

void BSAnimGroupSequence::ValidateTargets() {
    if (m_pkOwner) {
        NiDefaultAVObjectPalette* pdefaultObjectPalette =
            static_cast<NiDefaultAVObjectPalette*>(m_pkOwner->GetObjectPalette());
        if (m_pkAccumRoot) {
            m_pkAccumRoot = pdefaultObjectPalette->GetAVObject(*&m_pkAccumRoot->m_kName);
        }
        for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
            NiControllerSequence::IDTag* kIDTag = &m_pkIDTagArray[ui];
            NiAVObject* v1 = pdefaultObjectPalette->GetScene();
            if (!v1->GetObjectByName(kIDTag->m_kAVObjectName)) {
                pdefaultObjectPalette->SetAVObject(kIDTag->m_kAVObjectName, nullptr);
                m_pkInterpArray[ui].m_spInterpCtlr = static_cast<NiInterpController*>(nullptr);
                m_pkInterpArray[ui].m_pkBlendInterp = nullptr;
                m_pkInterpArray[ui].m_ucBlendIdx = -1;
            }
        }
    }
}

void BSAnimGroupSequence::ValidateTargetsFromPalette() {
    NiDefaultAVObjectPalette* pdefaultObjectPalette =
        static_cast<NiDefaultAVObjectPalette*>(m_pkOwner->GetObjectPalette());
    if (m_pkAccumRoot) {
        m_pkAccumRoot = pdefaultObjectPalette->GetAVObject(m_kAccumRootName);
    }
    for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
        const NiControllerSequence::IDTag* kIDTag = &m_pkIDTagArray[ui];
        if (!kIDTag->m_kAVObjectName.GetHandleString() ||
            !pdefaultObjectPalette->GetAVObject(kIDTag->m_kAVObjectName)) {
            m_pkInterpArray[ui].m_spInterpCtlr = static_cast<NiInterpController*>(nullptr);
            m_pkInterpArray[ui].m_pkBlendInterp = nullptr;
            m_pkInterpArray[ui].m_ucBlendIdx = -1;
        }
    }
}

void BSAnimGroupSequence::SaveGame(float a2) {
    float apData;
    apData = m_fOffset + a2;
    pSaveLoadGameOLD->SaveGameDataOLD(&apData, 4u);
    pSaveLoadGameOLD->SaveGameDataOLD(&m_eState, 4u);
    if (pSaveLoadGameOLD->GetCurrentVersion() < 113u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&apData, 4u);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&m_fLastTime, 4u);
    pSaveLoadGameOLD->SaveGameDataOLD(&m_fWeightedLastTime, 4u);
    pSaveLoadGameOLD->SaveGameDataOLD(&m_fDestFrame, 4u);
}

void BSAnimGroupSequence::SaveGame(BGSSaveGameBuffer* a2, float a3) {
    float apData;
    uint8_t cstate;
    apData = m_fOffset + a3;
    a2->SaveDataEndian(&apData, 4u, 0);
    cstate = m_eState;
    a2->SaveDataEndian(&cstate, 1u, 0);
    a2->SaveDataEndian(&m_fLastTime, 4u, 0);
    a2->SaveDataEndian(&m_fWeightedLastTime, 4u, 0);
    a2->SaveDataEndian(&m_fDestFrame, 4u, 0);
}

void BSAnimGroupSequence::CopyMembers(BSAnimGroupSequence* pDest, NiCloningProcess& arCloneData) {
    for (uint32_t i = 0; i < pDest->m_uiArraySize; ++i) {
        NiPointer<NiInterpolator> spInterpolator;
        pDest->RemoveInterpolator(i, spInterpolator);
    }
    NiControllerSequence::InterpArrayItem* m_pkInterpArray = pDest->m_pkInterpArray;
    if (m_pkInterpArray) {
        m_pkInterpArray->Free(3);
    }
    NiControllerSequence::IDTag* m_pkIDTagArray = pDest->m_pkIDTagArray;
    if (m_pkIDTagArray) {
        m_pkIDTagArray->Free(3);
    }
    NiControllerSequence::CopyMembers(pDest, arCloneData);
}

uint16_t BSAnimGroupSequence::GetSequenceSaveSize() {
    uint16_t usSize = 8;
    if (pSaveLoadGameOLD->GetCurrentVersion() < 0x71) {
        usSize += 4;
    }
    return usSize + 12;
}

bool BSAnimGroupSequence::PlaySounds(NiControllerSequence* apSeq, TESObjectREFR* apRef) {
    uint32_t inumKeys;
    bool bdone = false;
    if ((apSeq) && (apSeq->GetState() == NiControllerSequence::kAnimating)) {
        float fstartTime = apSeq->m_fStartTime;
        float fstopTime = apSeq->m_fLastScaledTime;
        if (fstartTime != -3.4028235e+38f && fstopTime != -3.4028235e+38f && fstopTime != fstartTime) {
            bool blooped = fstartTime > static_cast<double>(fstopTime);
            inumKeys = 0;
            NiTextKey* ptextKeys = nullptr;
            if (apSeq->GetTextKeys()) {
                NiTextKeyExtraData* TextKeys = apSeq->GetTextKeys();
                ptextKeys = TextKeys->GetKeys(inumKeys);
            }
            for (uint32_t n = 0; n < inumKeys; ++n) {
                // The key's text is written into (the line ends and the attach node's space are cut off and put
                // back), so it is held as a char*.
                char* pnote = const_cast<char*>((&ptextKeys[n].m_kText)->GetHandleString());
                if (pnote) {
                    char soundAttachNode_2[32];
                    float ftestTime = ptextKeys[n].m_fTime;
                    bool bplay = false;
                    NiAVObject* pmovingSound = nullptr;
                    if (blooped &&
                        (fstartTime < static_cast<double>(ftestTime) || fstopTime >= static_cast<double>(ftestTime))) {
                        bplay = true;
                    }
                    else {
                        if (fstartTime <= static_cast<double>(ftestTime) &&
                            fstopTime > static_cast<double>(ftestTime)) {
                            bplay = true;
                        }
                    }
                    if ((bplay) && (_strnicmp("Sound: ", pnote, 7u))) {
                        bplay = false;
                        if (!_strnicmp("Enum: StopSounds ", pnote, 17u)) {
                            char soundAttachNode[32];
                            const char* psp = nullptr;
                            soundAttachNode[0] = 0;
                            char* pcr = std::strchr(pnote, 13);
                            if (pcr) {
                                *pcr = 0;
                            }
                            psp = pnote + 16;
                            if ((pnote[16]) && (psp[1])) {
                                ourStrcpy(soundAttachNode, psp + 1);
                            }

                            if (pcr) {
                                *pcr = 13;
                            }
                            if (apRef->Get3D()) {
                                if (soundAttachNode[0]) {
                                    NiNode* v40 = apRef->Get3D();
                                    NiFixedString apString(static_cast<const char*>(soundAttachNode));
                                    NiFixedString* v39 = &apString;
                                    pmovingSound = v40->GetObjectByName(*v39);
                                }
                                else {
                                    NiNode* v38 = apRef->Get3D();
                                    NiFixedString* v3 = &FixedStrings::QAttachSound();
                                    pmovingSound = v38->GetObjectByName(*v3);
                                }
                                if (!pmovingSound) {
                                    pmovingSound = (NiAVObject*)apRef->Get3D();
                                }
                            }
                            if (pmovingSound) {
                                (&BSAudio::QInstance())
                                    ->StopMovingSounds(pmovingSound, 1.0f, BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
                            }
                        }
                        else {
                            if (!_strnicmp("Enum: HitShader", pnote, 15u)) {
                                pPlayer->PlayImpactBasedHitShader(apRef);
                            }
                        }
                    }

                    TESSound* psound = nullptr;
                    char* psp_2 = nullptr;
                    soundAttachNode_2[0] = 0;
                    if (bplay) {
                        char* pcr_2 = std::strchr(pnote, 13);
                        if (pcr_2) {
                            *pcr_2 = 0;
                        }
                        psp_2 = std::strchr(pnote + 7, 32);
                        if (psp_2) {
                            ourStrcpy(soundAttachNode_2, psp_2 + 1);
                            *psp_2 = 0;
                        }
                        psound = pDataHandler->GetSound(pnote + 7);
                        if (pcr_2) {
                            *pcr_2 = 13;
                        }
                    }
                    if (psound) {
                        BSSoundHandle soundHand;
                        NiPoint3 loc = NiPoint3_ZERO;
                        uint32_t iFlags = TESSound::ConvertLoopFlag(psound->GetData().iFlags);
                        bool v37 = (psound->GetData().iFlags & 64) != 0 && (psound->GetData().iFlags & 2048) == 0;
                        bool b2dSound = v37;
                        bool bloopingSound = (iFlags & BSGameSound::SOUND_FLAG_LOOP) != 0;
                        iFlags |= pPlayer != apRef ? 0 : BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE;
                        if ((apRef) && (apRef->Get3D())) {
                            const NiPoint3* WorldTranslate;
                            if (soundAttachNode_2[0]) {
                                NiNode* v36 = apRef->Get3D();
                                NiFixedString v49(static_cast<const char*>(soundAttachNode_2));
                                NiFixedString* v35 = &v49;
                                pmovingSound = v36->GetObjectByName(*v35);
                            }
                            else {
                                NiNode* v34 = apRef->Get3D();
                                NiFixedString* v5 = &FixedStrings::QAttachSound();
                                pmovingSound = v34->GetObjectByName(*v5);
                            }
                            if (bloopingSound && !pmovingSound) {
                                pmovingSound = (NiAVObject*)apRef->Get3D();
                            }
                            if (pmovingSound) {
                                pmovingSound->SetHasMovingSound(true);
                            }
                            NiAVObject* pmovingSound_2 = (NiAVObject*)apRef->Get3D();
                            pmovingSound_2->SetHasMovingSound(true);
                            if (pmovingSound) {
                                WorldTranslate = &pmovingSound->GetWorldTranslate();
                            }
                            else {
                                WorldTranslate = &apRef->Get3D()->GetWorldTranslate();
                            }
                            loc = *WorldTranslate;
                        }

                        if (apRef &&
                            (bloopingSound || (b2dSound || FalloutAudio::SoundDistanceTest(psound, loc, 1.0f)))) {
                            if (bloopingSound) {
                                uint32_t a4 = iFlags |
                                    (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER |
                                     BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
                                uint32_t aPosition_8 = psound->iFormID;
                                BSAudio* v8 = &BSAudio::QInstance();
                                soundHand = v8->GetSoundHandleByNumericID(aPosition_8, a4);
                            }
                            else {
                                if ((psound->GetData().iFlags & 64) != 0) {
                                    uint32_t a4a = iFlags |
                                        (BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                                         BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER |
                                         BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
                                    uint32_t aPosition_8a = psound->iFormID;
                                    BSAudio* v9 = &BSAudio::QInstance();
                                    soundHand = v9->GetSoundHandleByNumericID(aPosition_8a, a4a);
                                }
                                else {
                                    uint32_t a4b = iFlags |
                                        (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                                         BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER |
                                         BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
                                    uint32_t aPosition_8b = psound->iFormID;
                                    BSAudio* v10 = &BSAudio::QInstance();
                                    soundHand = v10->GetSoundHandleByNumericID(aPosition_8b, a4b);
                                }
                            }
                        }
                        else {
                            if (!apRef) {
                                if (bloopingSound) {
                                    uint32_t a4c = iFlags |
                                        (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                         BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
                                    uint32_t aPosition_8c = psound->iFormID;
                                    BSAudio* v11 = &BSAudio::QInstance();
                                    soundHand = v11->GetSoundHandleByNumericID(aPosition_8c, a4c);
                                }
                                else {
                                    if ((psound->GetData().iFlags & 64) != 0) {
                                        uint32_t a4d = iFlags |
                                            (BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                             BSGameSound::SOUND_FLAG_ONE_SHOT |
                                             BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
                                        uint32_t aPosition_8d = psound->iFormID;
                                        BSAudio* v12 = &BSAudio::QInstance();
                                        soundHand = v12->GetSoundHandleByNumericID(aPosition_8d, a4d);
                                    }
                                    else {
                                        uint32_t a4e = iFlags |
                                            (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                             BSGameSound::SOUND_FLAG_ONE_SHOT |
                                             BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
                                        uint32_t aPosition_8e = psound->iFormID;
                                        BSAudio* v13 = &BSAudio::QInstance();
                                        soundHand = v13->GetSoundHandleByNumericID(aPosition_8e, a4e);
                                    }
                                }
                            }
                        }
                        if (pmovingSound) {
                            soundHand.SetObjectToFollow(pmovingSound);
                        }
                        soundHand.SetPosition(loc.x, loc.y, loc.z);
                        soundHand.Play(bloopingSound);
                    }
                    if (psp_2) {
                        *psp_2 = 32;
                    }
                }
            }
        }
        if (apSeq->m_eCycleType) {
            bdone = true;
            if (-3.4028235e+38f == apSeq->GetOffset()) {
                return false;
            }

            double DataFogPower = apSeq->m_fLastScaledTime;
            if (apSeq->m_fEndKeyTime > DataFogPower) {
                return false;
            }
        }
    }

    return bdone;
}

void* BSAnimGroupSequence::InitAnimGroup(BSAnimGroupSequence* apSequence, const char* a2) {
    uint32_t uiNumKeys;
    ANIM_GROUP_WEAPON_ENUM aeWeapon;
    bool a4;
    ANIM_GROUP_MOVE_ENUM emove;
    [[maybe_unused]] const char* plastSlash;
    int32_t uiIndex;
    char apDest[68];
    char v79 = 0;
    TESAnimGroup* result = nullptr;
    ANIM_GROUP_ENUM v70 = ANIM_GROUP_NONE;
    int32_t v68 = 0;
    a4 = false;
    emove = AGM_NONE;
    aeWeapon = AGW_NONE;
    // The result of strrchr is kept in plastSlash, which is never read, as the game does.
    plastSlash = std::strrchr(a2, 92);
    MessageHandler::IncDisableWarningCount(true);
    NiFixedString* RootNodeNamePtr = apSequence->GetRootNodeNamePtr();
    const char* apString1 = RootNodeNamePtr->GetHandleString();
    uint32_t v77 = -1;
    uint32_t v75 = -1;
    BSsprintf(apDest, 64, "%s NonAccum", apString1);
    for (uiIndex = 0;; ++uiIndex) {
        char pcObjectName[264];
        uint32_t CastingType = apSequence->m_uiArraySize;
        if (uiIndex >= CastingType) {
            break;
        }
        apSequence->GetObjectNameAt(uiIndex, pcObjectName);
        if (pcObjectName[0]) {
            if (BSstricmp(apString1, pcObjectName)) {
                if (!BSstricmp(apDest, pcObjectName)) {
                    v75 = uiIndex;
                }
            }
            else {
                v77 = uiIndex;
            }
        }
        else {
            const char* a3 = (&apSequence->m_kName)->GetHandleString();
            Warning("No object name at index %i for sequence '%s'.", uiIndex, a3);
        }
        if (v77 != -1 && v75 != -1) {
            uint8_t InterpolatorPriority = apSequence->GetInterpolatorPriority(v77);
            apSequence->SetInterpolatorPriority(v75, InterpolatorPriority);
            break;
        }
    }
    GetMoveHandAndPowerArmorTypeFromAnimName(a2, &emove, &aeWeapon, &a4);
    if (apSequence && apSequence->GetTextKeys()) {
        NiTextKeyExtraData* TextKeys = apSequence->GetTextKeys();
        NiTextKey* Keys = TextKeys->GetKeys(uiNumKeys);
        for (uiIndex = 0; uiIndex < uiNumKeys; ++uiIndex) {
            NiFixedString apString(*&Keys[uiIndex].m_kText);
            float DecalMinWidth = Keys[uiIndex].m_fTime;
            if (apString.Bool()) {
                const char* HandleString = apString.GetHandleString();
                if (!_strnicmp(HandleString, "prn:", 4u)) {
                    if (apString.GetHandleString()[4] == 32) {
                        const char* v8 = apString.GetHandleString();
                        result->SetParentRootNode(v8 + 5);
                    }
                    else {
                        const char* v7 = apString.GetHandleString();
                        result->SetParentRootNode(v7 + 4);
                    }
                }
                else {
                    char Dst[264];
                    const char* v9 = apString.GetHandleString();
                    BSstrcpy(Dst, 260, v9);
                    char* pTextKey = Dst;
                    while (pTextKey && BSstrlen(pTextKey)) {
                        if (*pTextKey == 13) {
                            pTextKey = GetNextLineInString(pTextKey);
                        }
                        uint32_t v10 = BSstrlen(NiControllerSequence::pMorphTag);
                        if (!_strnicmp(pTextKey, NiControllerSequence::pMorphTag, v10)) {
                            if (!result->cMorphKey) {
                                result->cMorphKey = pTextKey[2];
                            }
                        }
                        else {
                            if (!_strnicmp(pTextKey, "BlendIn:", 8u)) {
                                char v11;
                                if (pTextKey[8] == 32) {
                                    v11 = std::atoi(pTextKey + 9);
                                }
                                else {
                                    v11 = std::atoi(pTextKey + 8);
                                }
                                result->cBlendInFrames = v11;
                            }
                            else {
                                if (!_strnicmp(pTextKey, "BlendOut:", 9u)) {
                                    char v12;
                                    if (pTextKey[9] == 32) {
                                        v12 = std::atoi(pTextKey + 10);
                                    }
                                    else {
                                        v12 = std::atoi(pTextKey + 9);
                                    }
                                    result->SetBlendOutFrames(static_cast<uint8_t>(v12));
                                }
                                else {
                                    if (!_strnicmp(pTextKey, "Blend:", 6u)) {
                                        uint8_t v13;
                                        if (pTextKey[6] == 32) {
                                            v13 = std::atoi(pTextKey + 7);
                                        }
                                        else {
                                            v13 = std::atoi(pTextKey + 6);
                                        }
                                        result->SetBlend(v13);
                                    }
                                    else {
                                        if (!_strnicmp(pTextKey, "Decal:", 6u)) {
                                            uint8_t v14;
                                            if (pTextKey[6] == 32) {
                                                v14 = std::atoi(pTextKey + 7);
                                            }
                                            else {
                                                v14 = std::atoi(pTextKey + 6);
                                            }
                                            result->SetDecal(v14);
                                        }
                                        else {
                                            uint32_t v16;
                                            uint32_t v15 = BSstrlen("Sound:");
                                            if (!_strnicmp(pTextKey, "Sound:", v15) ||
                                                (v16 = BSstrlen("Enum:"), !_strnicmp(pTextKey, "Enum:", v16))) {
                                                uint32_t v17 = BSstrlen("Enum:");
                                                bool bEnum = _strnicmp(pTextKey, "Enum:", v17) == 0;
                                                if (result) {
                                                    char* pKeyValue;
                                                    char cKeyValue[268];
                                                    char* v53 = std::strchr(pTextKey, 13);
                                                    if (v53) {
                                                        *v53 = 0;
                                                    }
                                                    if (bEnum) {
                                                        pKeyValue = &pTextKey[BSstrlen("Enum: ")];
                                                    }
                                                    else {
                                                        pKeyValue = &pTextKey[BSstrlen("Sound: ")];
                                                    }
                                                    BSstrcpy(cKeyValue, 260, pKeyValue);
                                                    char* String = nullptr;
                                                    char* v54 = nullptr;
                                                    char* v56 = std::strchr(cKeyValue, 44);
                                                    if (v56) {
                                                        String = v56 + 1;
                                                    }
                                                    else {
                                                        v56 = std::strchr(cKeyValue, 32);
                                                        if (v56) {
                                                            String = v56 + 1;
                                                        }
                                                    }
                                                    if (v56) {
                                                        *v56 = 0;
                                                    }
                                                    if (String) {
                                                        while (*String == 32) {
                                                            ++String;
                                                        }
                                                        char* v57 = std::strchr(String, 44);
                                                        if (v57) {
                                                            v54 = v57 + 1;
                                                        }
                                                        else {
                                                            v57 = std::strchr(String, 32);
                                                            if (v57) {
                                                                v54 = v57 + 1;
                                                            }
                                                        }
                                                        if (v57) {
                                                            *v57 = 0;
                                                        }
                                                    }
                                                    char v52 = 0;
                                                    if (bEnum) {
                                                        ANIM_ENUM i;
                                                        for (i = ANIM_ENUM_LEFT; i < ANIM_ENUM_COUNT &&
                                                             BSstricmp(animEnumNames[i], cKeyValue);
                                                             i = static_cast<ANIM_ENUM>(i + 1)) {
                                                            ;
                                                        }
                                                        if (i >= ANIM_ENUM_COUNT) {
                                                            v52 = 1;
                                                        }
                                                        else {
                                                            uint32_t aiEnumData = uiIndex | (pTextKey - Dst) << 8;
                                                            uint32_t uiSoundCount = result->soundCount;
                                                            result->SetSoundCount(uiSoundCount + 1);
                                                            result->SetSoundTime(uiSoundCount, DecalMinWidth);
                                                            result->SetSoundEnum(uiSoundCount, i, aiEnumData);
                                                        }
                                                    }
                                                    else {
                                                        TESSound* Sound = pDataHandler->GetSound(cKeyValue);
                                                        if (Sound) {
                                                            int32_t someFloata = Sound->iFormID;
                                                            BSWin32Audio* v18 =
                                                                static_cast<BSWin32Audio*>(&BSAudio::QInstance());
                                                            v18->PrecacheSoundFromID(someFloata, 0);
                                                            uint32_t Interior = result->soundCount;
                                                            result->SetSoundCount(Interior + 1);
                                                            result->SetSoundTime(Interior, DecalMinWidth);
                                                            result->SetSoundObject(Interior, Sound);
                                                            if ((String) && (*String)) {
                                                                float v41 = std::atof(String);
                                                                result->SetSoundVolume(Interior, v41);
                                                            }

                                                            if (v54) {
                                                                float v40 = std::atof(v54);
                                                                result->SetSoundPitch(Interior, v40);
                                                            }
                                                        }
                                                        else {
                                                            v52 = 1;
                                                        }
                                                    }
                                                    if (v52) {
                                                        float v39 = DecalMinWidth * 30.0;
                                                        Warning(
                                                            "ANIMATION: Bad note string \"%s\" frame %d in \"%s\".",
                                                            pTextKey, FloatToInt(v39), a2
                                                        );
                                                        v79 = 1;
                                                    }
                                                    if (v53) {
                                                        *v53 = 13;
                                                    }
                                                }
                                            }
                                            else {
                                                ANIM_GROUP_ENUM aeType;
                                                for (aeType = ANIM_GROUP_IDLE; aeType < ANIM_GROUP_COUNT;
                                                     aeType = static_cast<ANIM_GROUP_ENUM>(aeType + 1)) {
                                                    if (v70 != ANIM_GROUP_NONE) {
                                                        aeType = v70;
                                                    }
                                                    const char* a3a = animGroupInfo[aeType].pName;
                                                    const char* v19 = (&apSequence->m_kName)->GetHandleString();
                                                    if (!BSstricmp(v19, a3a)) {
                                                        v70 = aeType;
                                                        uint32_t aiIndex = animGroupInfo[aeType].eActions[v68];
                                                        if (aiIndex != -1) {
                                                            if (aeType == ANIM_GROUP_SPECIAL_IDLE && v68 == 1) {
                                                                uint32_t v20 = BSstrlen(
                                                                    animGroupActionNames[3]
                                                                                        [animGroupInfo[aeType].eAction]
                                                                );
                                                                if (!_strnicmp(
                                                                        pTextKey,
                                                                        animGroupActionNames[3][animGroupInfo[aeType]
                                                                                                    .eAction],
                                                                        v20
                                                                    )) {
                                                                    aiIndex = 3;
                                                                    v68 = 3;
                                                                }
                                                            }
                                                            if (animGroupInfo[aeType].eAction == AGAT_ATTACK &&
                                                                v68 == 2) {
                                                                uint32_t v21 = BSstrlen(
                                                                    animGroupActionNames[3]
                                                                                        [animGroupInfo[aeType].eAction]
                                                                );
                                                                if (!_strnicmp(
                                                                        pTextKey,
                                                                        animGroupActionNames[3][animGroupInfo[aeType]
                                                                                                    .eAction],
                                                                        v21
                                                                    )) {
                                                                    aiIndex = 3;
                                                                    v68 = 3;
                                                                }
                                                            }
                                                            uint32_t v22 = BSstrlen(
                                                                animGroupActionNames[aiIndex]
                                                                                    [animGroupInfo[aeType].eAction]
                                                            );
                                                            if (!_strnicmp(
                                                                    pTextKey,
                                                                    animGroupActionNames[aiIndex]
                                                                                        [animGroupInfo[aeType].eAction],
                                                                    v22
                                                                )) {
                                                                if (!result) {
                                                                    if (aeType == ANIM_GROUP_SPECIAL_IDLE) {
                                                                        TESAnimGroup* v38;
                                                                        v38 = new TESAnimGroup(aeType);
                                                                        result = v38;
                                                                    }
                                                                    else {
                                                                        TESAnimGroup* v37;
                                                                        TESAnimGroup* v42 = static_cast<TESAnimGroup*>(
                                                                            NiMemObject::operator new(
                                                                                sizeof(TESAnimGroup)
                                                                            )
                                                                        );
                                                                        if (v42) {
                                                                            uint16_t v23 = TESAnimGroup::AnimGroup(
                                                                                emove, aeWeapon, aeType, a4
                                                                            );
                                                                            v37 = ::new (v42) TESAnimGroup(v23);
                                                                        }
                                                                        else {
                                                                            v37 = nullptr;
                                                                        }
                                                                        result = v37;
                                                                    }
                                                                }
                                                                if (DecalMinWidth > -1.0) {
                                                                    result->SetTime(aiIndex, DecalMinWidth);
                                                                }
                                                                if (animGroupInfo[aeType].eAction == AGAT_LOOP &&
                                                                    apSequence->m_eCycleType) {
                                                                    if (animGroupInfo[aeType].eSectionLayer !=
                                                                        AGS_MOVEMENT) {
                                                                        Warning(
                                                                            "ANIMATION: '%s' should be a looping "
                                                                            "animation.",
                                                                            a2
                                                                        );
                                                                        v79 = 1;
                                                                    }
                                                                }
                                                                else {
                                                                    if (animGroupInfo[aeType].eAction != AGAT_LOOP &&
                                                                        apSequence->m_eCycleType ==
                                                                            NiControllerSequence::CYCLE_LOOP) {
                                                                        Warning(
                                                                            "ANIMATION: '%s' should NOT be a "
                                                                            "looping animation.",
                                                                            a2
                                                                        );
                                                                        v79 = 1;
                                                                    }
                                                                }
                                                                ++v68;
                                                            }
                                                            else {
                                                                if (v68 == 5 ||
                                                                    animGroupInfo[aeType].eActions[v68] == AGA_NONE) {
                                                                    char* v45 = std::strchr(pTextKey, 13);
                                                                    if (v45) {
                                                                        *v45 = 0;
                                                                    }
                                                                    float v36 = DecalMinWidth * 30.0;
                                                                    Warning(
                                                                        "ANIMATION: Bad note string \"%s\" frame "
                                                                        "%d in \"%s\".",
                                                                        pTextKey, FloatToInt(v36), a2
                                                                    );
                                                                    if (v45) {
                                                                        *v45 = 13;
                                                                    }
                                                                    v79 = 1;
                                                                }
                                                            }
                                                        }
                                                        break;
                                                    }
                                                    if (aeType == v70) {
                                                        break;
                                                    }
                                                }
                                                if (aeType == ANIM_GROUP_COUNT) {
                                                    Warning(
                                                        "ANIMATION: AnimGroup unable to find sequence '%s' in "
                                                        "model '%s'.",
                                                        (&apSequence->m_kName)->GetHandleString(), a2
                                                    );
                                                    v79 = 1;
                                                    pTextKey = nullptr;
                                                    uiIndex = uiNumKeys;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (pTextKey) {
                            pTextKey = GetNextLineInString(pTextKey);
                        }
                    }
                }
            }
            else {
                const char* someFloat = (&apSequence->m_kName)->GetHandleString();
                Warning(
                    "ANIMATION: AnimGroup empty note key at time %.2f in sequence '%s' in model '%s'.", DecalMinWidth,
                    someFloat, a2
                );
                v79 = 1;
            }
        }
        if (result) {
            double v24;
            result->SetupPriorities(apSequence);
            ANIM_GROUP_ACTION groupAction = AGA_STOP;
            switch (animGroupInfo[result->GetType()].eAction) {
                case AGAT_SPECIAL_IDLE_LOOP: {
                    double Time = result->GetTime(AGA_STOP);
                    if (result->GetTime(AGA_START) > Time) {
                        float a3c = result->GetTime(AGA_START);
                        result->SetTime(1u, a3c);
                    }
                    double v33 = result->GetTime(AGA_SPECIAL_IDLE_LOOP_STOP);
                    if (result->GetTime(AGA_STOP) >= v33) {
                        float a3d = result->GetTime(AGA_SPECIAL_IDLE_STOP);
                        result->SetTime(2u, a3d);
                    }
                    goto LABEL_160;
                }
                case AGAT_EQUIP:
                case AGAT_UNEQUIP: {
                    groupAction = AGA_SPECIAL_IDLE_LOOP_STOP;
                    goto LABEL_162;
                }
                case AGAT_ATTACK: {
                    double v35 = result->GetTime(AGA_SPECIAL_IDLE_LOOP_STOP);
                    if (result->GetTime(AGA_STOP) > v35) {
                        float a3b = result->GetTime(AGA_STOP);
                        result->SetTime(2u, a3b);
                    }
                    goto LABEL_151;
                }
                case AGAT_ATTACK_POWER:
                case AGAT_LOOPING_RELOAD: {
                    v24 = result->GetTime(AGA_SPECIAL_IDLE_LOOP_STOP);
                    break;
                }
                case AGAT_ATTACK_THROW: {
                LABEL_151:;
                    v24 = result->GetTime(AGA_ATTACK_STOP);
                    break;
                }
                case AGAT_ATTACK_MINE:
                case AGAT_ATTACK_SPIN: {
                LABEL_160:;
                    v24 = result->GetTime(AGA_SPECIAL_IDLE_STOP);
                    break;
                }
                default: {
                LABEL_162:;
                    v24 = result->GetTime(groupAction);
                    break;
                }
            }
            if (result->GetTime(AGA_START) >= v24) {
                Warning(
                    "ANIMATION: (%s) End frame is less than or equal to Start frame in \"%s\".",
                    animGroupInfo[result->GetType()].pName, a2
                );
                v79 = 1;
            }
            if ((result->cBlendFrames) && (result->cBlendInFrames || result->cBlendOutFrames)) {
                Warning(
                    "ANIMATION: (%s) too many blend values in \"%s\". Blend %d, BlendIn %d, BlendOut %d",
                    animGroupInfo[result->GetType()].pName, a2, result->cBlendFrames, result->cBlendInFrames,
                    result->cBlendOutFrames
                );
                v79 = 1;
            }
        }
    }
    MessageHandler::IncDisableWarningCount(false);
    if (v79) {
        Warning("ANIMATION: Animation group note problem. See %s file.", MessageHandler::GetWarningFileName());
    }
    return (void*)result;
}

void BSAnimGroupSequence::CreateSequenceFromFile(
    const NiStream& kStream,
    uint32_t uiIndex,
    NiPointer<NiControllerSequence>& aspControllerSequence
) {
    BSAnimGroupSequence* v3;
    NiPointer<NiControllerSequence> spSequence;
    NiControllerSequence::CreateSequenceFromFile(kStream, uiIndex, spSequence);
    v3 = new BSAnimGroupSequence(nullptr, spSequence.m_pObject);
    aspControllerSequence = static_cast<NiControllerSequence*>(v3);
}

NiFixedString& FixedStrings::QAttachSound() {
    return *FixedStrings::AttachSound;
}

void NiControllerSequence::SetAccumRootName(const NiFixedString& kAccumRootName) {
    m_kAccumRootName.Assign(kAccumRootName);
}
