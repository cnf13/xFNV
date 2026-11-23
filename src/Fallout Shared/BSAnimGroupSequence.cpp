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

#include <limits>

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
            NiBSBoneLODController* pBoneLOD = static_cast<NiBSBoneLODController*>(
                m_pkAccumRoot->GetController(&NiBSBoneLODController::ms_RTTI)
            );
            if (pBoneLOD && pBoneLOD->GetBoneLOD() != 0) {
                NiTimeController* pController = m_pkAccumRoot->GetParent()->GetController(
                    &NiMultiTargetTransformController::ms_RTTI
                );
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
    float fOffsetTime;
    pSaveLoadGameOLD->LoadGameDataOLD(&fOffsetTime, sizeof(fOffsetTime));
    pSaveLoadGameOLD->LoadGameDataOLD(&m_eState, sizeof(m_eState));
    if (pSaveLoadGameOLD->GetCurrentVersion() < 0x71u) {
        pSaveLoadGameOLD->LoadGameDataOLD(&fOffsetTime, sizeof(fOffsetTime));
    }
    m_fOffset = fOffsetTime - afTime;
    pSaveLoadGameOLD->LoadGameDataOLD(&m_fLastTime, sizeof(m_fLastTime));
    pSaveLoadGameOLD->LoadGameDataOLD(&m_fWeightedLastTime, sizeof(m_fWeightedLastTime));
    pSaveLoadGameOLD->LoadGameDataOLD(&m_fDestFrame, sizeof(m_fDestFrame));
    m_fStartTime = -std::numeric_limits<float>::max();
    m_fEndTime = -std::numeric_limits<float>::max();
    GetScaledTime(afTime);
}

void BSAnimGroupSequence::LoadGame(BGSLoadGameBuffer* apBuffer, float afTime) {
    float fOffsetTime = 0.0f;
    apBuffer->LoadData(&fOffsetTime, sizeof(fOffsetTime));
    m_fOffset = fOffsetTime - afTime;
    uint8_t cstate = 0;
    apBuffer->LoadData(&cstate, sizeof(cstate));
    m_eState = static_cast<NiControllerSequence::State>(cstate);
    apBuffer->LoadData(&m_fLastTime, sizeof(m_fLastTime));
    apBuffer->LoadData(&m_fWeightedLastTime, sizeof(m_fWeightedLastTime));
    apBuffer->LoadData(&m_fDestFrame, sizeof(m_fDestFrame));
    m_fStartTime = -std::numeric_limits<float>::max();
    m_fEndTime = -std::numeric_limits<float>::max();
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
    NiAVObjectPalette* pPalette = m_pkOwner->GetObjectPalette();
    return NiControllerSequence::ResolveTransformInterpolators(apNode, pPalette, aiTransformControllerCount);
}

double BSAnimGroupSequence::GetKeyLengthOverFrequency() {
    return (m_fEndKeyTime - m_fBeginKeyTime) / m_fFrequency;
}

uint8_t BSAnimGroupSequence::GetPriority(const char* apBoneName, uint32_t* apReturnIndex) {
    uint32_t i;
    for (i = 0;; ++i) {
        if (i >= m_uiArraySize) {
            return 0;
        }
        const char* pname = m_pkIDTagArray[i].m_kAVObjectName.GetHandleString();
        if (!pname) {
            Warning("NULL object name found in tag array for sequence '%s'.", m_kName.GetHandleString());
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

void BSAnimGroupSequence::SetInterpolatorPriority(uint32_t auiIndex, uint8_t aucPriority) {
    if (auiIndex < m_uiArraySize) {
        m_pkInterpArray[auiIndex].m_ucPriority = aucPriority;
    }
}

uint8_t BSAnimGroupSequence::GetInterpolatorPriority(uint32_t auiIndex) {
    if (auiIndex >= m_uiArraySize) {
        return 0xFF;
    }

    return m_pkInterpArray[auiIndex].m_ucPriority;
}

void BSAnimGroupSequence::SetAnimGroup(TESAnimGroup* apAnimGroup) {
    spAnimGroup = apAnimGroup;
}

void BSAnimGroupSequence::ClearBlendInterp() {
    for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
        m_pkInterpArray[ui].m_pkBlendInterp = nullptr;
    }
}

float BSAnimGroupSequence::GetScaledTime(float afTime) {
    int32_t eState = m_eState;
    if (eState <= 0 || eState > 3) {
        return 0.0;
    }
    float fTime = afTime + m_fOffset;
    return NiControllerSequence::ComputeScaledTime(fTime, true);
}

void BSAnimGroupSequence::ValidateTargets() {
    if (m_pkOwner) {
        NiDefaultAVObjectPalette* pdefaultObjectPalette = static_cast<NiDefaultAVObjectPalette*>(
            m_pkOwner->GetObjectPalette()
        );
        if (m_pkAccumRoot) {
            m_pkAccumRoot = pdefaultObjectPalette->GetAVObject(m_pkAccumRoot->m_kName);
        }
        for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
            NiControllerSequence::IDTag* pIDTag = &m_pkIDTagArray[ui];
            NiAVObject* pScene = pdefaultObjectPalette->GetScene();
            if (!pScene->GetObjectByName(pIDTag->m_kAVObjectName)) {
                pdefaultObjectPalette->SetAVObject(pIDTag->m_kAVObjectName, nullptr);
                m_pkInterpArray[ui].m_spInterpCtlr = static_cast<NiInterpController*>(nullptr);
                m_pkInterpArray[ui].m_pkBlendInterp = nullptr;
                m_pkInterpArray[ui].m_ucBlendIdx = -1;
            }
        }
    }
}

void BSAnimGroupSequence::ValidateTargetsFromPalette() {
    NiDefaultAVObjectPalette* pdefaultObjectPalette = static_cast<NiDefaultAVObjectPalette*>(
        m_pkOwner->GetObjectPalette()
    );
    if (m_pkAccumRoot) {
        m_pkAccumRoot = pdefaultObjectPalette->GetAVObject(m_kAccumRootName);
    }
    for (uint32_t ui = 0; ui < m_uiArraySize; ++ui) {
        const NiControllerSequence::IDTag* pIDTag = &m_pkIDTagArray[ui];
        if (!pIDTag->m_kAVObjectName.GetHandleString() ||
            !pdefaultObjectPalette->GetAVObject(pIDTag->m_kAVObjectName)) {
            m_pkInterpArray[ui].m_spInterpCtlr = static_cast<NiInterpController*>(nullptr);
            m_pkInterpArray[ui].m_pkBlendInterp = nullptr;
            m_pkInterpArray[ui].m_ucBlendIdx = -1;
        }
    }
}

void BSAnimGroupSequence::SaveGame(float afTime) {
    float fOffsetTime = m_fOffset + afTime;
    pSaveLoadGameOLD->SaveGameDataOLD(&fOffsetTime, sizeof(fOffsetTime));
    pSaveLoadGameOLD->SaveGameDataOLD(&m_eState, sizeof(m_eState));
    if (pSaveLoadGameOLD->GetCurrentVersion() < 113u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&fOffsetTime, sizeof(fOffsetTime));
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&m_fLastTime, sizeof(m_fLastTime));
    pSaveLoadGameOLD->SaveGameDataOLD(&m_fWeightedLastTime, sizeof(m_fWeightedLastTime));
    pSaveLoadGameOLD->SaveGameDataOLD(&m_fDestFrame, sizeof(m_fDestFrame));
}

void BSAnimGroupSequence::SaveGame(BGSSaveGameBuffer* apBuffer, float afTime) {
    float fOffsetTime = m_fOffset + afTime;
    apBuffer->SaveDataEndian(&fOffsetTime, sizeof(fOffsetTime), 0);
    uint8_t cstate = m_eState;
    apBuffer->SaveDataEndian(&cstate, sizeof(cstate), 0);
    apBuffer->SaveDataEndian(&m_fLastTime, sizeof(m_fLastTime), 0);
    apBuffer->SaveDataEndian(&m_fWeightedLastTime, sizeof(m_fWeightedLastTime), 0);
    apBuffer->SaveDataEndian(&m_fDestFrame, sizeof(m_fDestFrame), 0);
}

void BSAnimGroupSequence::CopyMembers(BSAnimGroupSequence* pDest, NiCloningProcess& arCloneData) {
    for (uint32_t i = 0; i < pDest->m_uiArraySize; ++i) {
        NiPointer<NiInterpolator> spInterpolator;
        pDest->RemoveInterpolator(i, spInterpolator);
    }
    delete[] pDest->m_pkInterpArray;
    delete[] pDest->m_pkIDTagArray;
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
    bool bdone = false;
    if (apSeq && apSeq->GetState() == NiControllerSequence::kAnimating) {
        float fstartTime = apSeq->m_fStartTime;
        float fstopTime = apSeq->m_fLastScaledTime;
        if (fstartTime != -std::numeric_limits<float>::max() &&
            fstopTime != -std::numeric_limits<float>::max() &&
            fstopTime != fstartTime) {
            bool blooped = fstartTime > static_cast<double>(fstopTime);
            uint32_t uiNumKeys = 0;
            NiTextKey* ptextKeys = nullptr;
            if (apSeq->GetTextKeys()) {
                ptextKeys = apSeq->GetTextKeys()->GetKeys(uiNumKeys);
            }
            for (uint32_t n = 0; n < uiNumKeys; ++n) {
                // The key's text is written into (the line ends and the attach node's space are cut off and put
                // back), so it is held as a char*.
                char* pnote = const_cast<char*>(ptextKeys[n].m_kText.GetHandleString());
                if (pnote) {
                    char soundAttachNode[32];
                    float ftestTime = ptextKeys[n].m_fTime;
                    bool bplay = false;
                    NiAVObject* pmovingSound = nullptr;
                    if (blooped &&
                        (fstartTime < static_cast<double>(ftestTime) || fstopTime >= static_cast<double>(ftestTime))) {
                        bplay = true;
                    }
                    else if (
                        fstartTime <= static_cast<double>(ftestTime) && fstopTime > static_cast<double>(ftestTime)
                    ) {
                        bplay = true;
                    }
                    if (bplay && _strnicmp("Sound: ", pnote, 7u)) {
                        bplay = false;
                        if (!_strnicmp("Enum: StopSounds ", pnote, 17u)) {
                            soundAttachNode[0] = 0;
                            char* pcr = std::strchr(pnote, 13);
                            if (pcr) {
                                *pcr = 0;
                            }
                            const char* psp = pnote + 16;
                            if (pnote[16] && psp[1]) {
                                ourStrcpy(soundAttachNode, psp + 1);
                            }

                            if (pcr) {
                                *pcr = 13;
                            }
                            if (apRef->Get3D()) {
                                if (soundAttachNode[0]) {
                                    NiNode* pRoot = apRef->Get3D();
                                    NiFixedString kAttachName(static_cast<const char*>(soundAttachNode));
                                    pmovingSound = pRoot->GetObjectByName(kAttachName);
                                }
                                else {
                                    NiNode* pRoot = apRef->Get3D();
                                    pmovingSound = pRoot->GetObjectByName(FixedStrings::QAttachSound());
                                }
                                if (!pmovingSound) {
                                    pmovingSound = apRef->Get3D();
                                }
                            }
                            if (pmovingSound) {
                                (&BSAudio::QInstance())
                                    ->StopMovingSounds(pmovingSound, 1.0f, BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
                            }
                        }
                        else if (!_strnicmp("Enum: HitShader", pnote, 15u)) {
                            pPlayer->PlayImpactBasedHitShader(apRef);
                        }
                    }

                    TESSound* psound = nullptr;
                    char* psoundSpace = nullptr;
                    soundAttachNode[0] = 0;
                    if (bplay) {
                        char* pcr = std::strchr(pnote, 13);
                        if (pcr) {
                            *pcr = 0;
                        }
                        psoundSpace = std::strchr(pnote + 7, 32);
                        if (psoundSpace) {
                            ourStrcpy(soundAttachNode, psoundSpace + 1);
                            *psoundSpace = 0;
                        }
                        psound = pDataHandler->GetSound(pnote + 7);
                        if (pcr) {
                            *pcr = 13;
                        }
                    }
                    if (psound) {
                        BSSoundHandle soundHand;
                        NiPoint3 loc = NiPoint3::ZERO;
                        uint32_t uiFlags = TESSound::ConvertLoopFlag(psound->GetData().iFlags);
                        bool b2dSound = (psound->GetData().iFlags & TESSound::FLAG_2D) != 0 &&
                            (psound->GetData().iFlags & TESSound::FLAG_2D_RADIUS) == 0;
                        bool bloopingSound = (uiFlags & BSGameSound::SOUND_FLAG_LOOP) != 0;
                        uiFlags |= pPlayer != apRef ? 0 : BSGameSound::SOUND_FLAG_IGNORE_TIMESCALE;
                        if (apRef && apRef->Get3D()) {
                            const NiPoint3* pTranslate;
                            if (soundAttachNode[0]) {
                                NiNode* pRoot = apRef->Get3D();
                                NiFixedString kAttachName(static_cast<const char*>(soundAttachNode));
                                pmovingSound = pRoot->GetObjectByName(kAttachName);
                            }
                            else {
                                NiNode* pRoot = apRef->Get3D();
                                pmovingSound = pRoot->GetObjectByName(FixedStrings::QAttachSound());
                            }
                            if (bloopingSound && !pmovingSound) {
                                pmovingSound = apRef->Get3D();
                            }
                            if (pmovingSound) {
                                pmovingSound->SetHasMovingSound(true);
                            }
                            apRef->Get3D()->SetHasMovingSound(true);
                            if (pmovingSound) {
                                pTranslate = &pmovingSound->GetWorldTranslate();
                            }
                            else {
                                pTranslate = &apRef->Get3D()->GetWorldTranslate();
                            }
                            loc = *pTranslate;
                        }

                        if (apRef &&
                            (bloopingSound || b2dSound || FalloutAudio::SoundDistanceTest(psound, loc, 1.0f))) {
                            if (bloopingSound) {
                                uint32_t uiSoundFlags = uiFlags |
                                    (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER |
                                     BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
                                uint32_t uiSoundID = psound->iFormID;
                                soundHand = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, uiSoundFlags);
                            }
                            else if ((psound->GetData().iFlags & TESSound::FLAG_2D) != 0) {
                                uint32_t uiSoundFlags = uiFlags |
                                    (BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                                     BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER |
                                     BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
                                uint32_t uiSoundID = psound->iFormID;
                                soundHand = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, uiSoundFlags);
                            }
                            else {
                                uint32_t uiSoundFlags = uiFlags |
                                    (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                                     BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER |
                                     BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
                                uint32_t uiSoundID = psound->iFormID;
                                soundHand = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, uiSoundFlags);
                            }
                        }
                        else if (!apRef) {
                            if (bloopingSound) {
                                uint32_t uiSoundFlags = uiFlags |
                                    (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                     BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
                                uint32_t uiSoundID = psound->iFormID;
                                soundHand = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, uiSoundFlags);
                            }
                            else if ((psound->GetData().iFlags & TESSound::FLAG_2D) != 0) {
                                uint32_t uiSoundFlags = uiFlags |
                                    (BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                     BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
                                uint32_t uiSoundID = psound->iFormID;
                                soundHand = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, uiSoundFlags);
                            }
                            else {
                                uint32_t uiSoundFlags = uiFlags |
                                    (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                     BSGameSound::SOUND_FLAG_ONE_SHOT | BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN);
                                uint32_t uiSoundID = psound->iFormID;
                                soundHand = BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, uiSoundFlags);
                            }
                        }
                        if (pmovingSound) {
                            soundHand.SetObjectToFollow(pmovingSound);
                        }
                        soundHand.SetPosition(loc.x, loc.y, loc.z);
                        soundHand.Play(bloopingSound);
                    }
                    if (psoundSpace) {
                        *psoundSpace = 32;
                    }
                }
            }
        }
        if (apSeq->m_eCycleType) {
            bdone = true;
            if (-std::numeric_limits<float>::max() == apSeq->GetOffset()) {
                return false;
            }

            double dLastScaledTime = apSeq->m_fLastScaledTime;
            if (apSeq->m_fEndKeyTime > dLastScaledTime) {
                return false;
            }
        }
    }

    return bdone;
}

void* BSAnimGroupSequence::InitAnimGroup(BSAnimGroupSequence* apSequence, const char* apFilename) {
    uint32_t uiNumKeys;
    int32_t iIndex;
    char cNonAccumName[68];
    char bNoteProblem = 0;
    TESAnimGroup* pAnimGroup = nullptr;
    ANIM_GROUP_ENUM eGroupType = ANIM_GROUP_NONE;
    int32_t iActionSlot = 0;
    bool bPowerArmor = false;
    ANIM_GROUP_MOVE_ENUM emove = AGM_NONE;
    ANIM_GROUP_WEAPON_ENUM eWeapon = AGW_NONE;
    MessageHandler::IncDisableWarningCount(true);
    const char* pcRootName = apSequence->GetRootNodeNamePtr()->GetHandleString();
    uint32_t uiRootIndex = -1;
    uint32_t uiNonAccumIndex = -1;
    BSsprintf(cNonAccumName, 64, "%s NonAccum", pcRootName);
    for (iIndex = 0; iIndex < apSequence->m_uiArraySize; ++iIndex) {
        char pcObjectName[264];
        apSequence->GetObjectNameAt(iIndex, pcObjectName);
        if (pcObjectName[0]) {
            if (BSstricmp(pcRootName, pcObjectName)) {
                if (!BSstricmp(cNonAccumName, pcObjectName)) {
                    uiNonAccumIndex = iIndex;
                }
            }
            else {
                uiRootIndex = iIndex;
            }
        }
        else {
            Warning("No object name at index %i for sequence '%s'.", iIndex, apSequence->m_kName.GetHandleString());
        }
        if (uiRootIndex != -1 && uiNonAccumIndex != -1) {
            uint8_t ucPriority = apSequence->GetInterpolatorPriority(uiRootIndex);
            apSequence->SetInterpolatorPriority(uiNonAccumIndex, ucPriority);
            break;
        }
    }
    GetMoveHandAndPowerArmorTypeFromAnimName(apFilename, &emove, &eWeapon, &bPowerArmor);
    if (apSequence && apSequence->GetTextKeys()) {
        NiTextKey* pKeys = apSequence->GetTextKeys()->GetKeys(uiNumKeys);
        for (iIndex = 0; iIndex < uiNumKeys; ++iIndex) {
            NiFixedString kText(pKeys[iIndex].m_kText);
            float fKeyTime = pKeys[iIndex].m_fTime;
            if (kText.Bool()) {
                if (!_strnicmp(kText.GetHandleString(), "prn:", 4u)) {
                    if (kText.GetHandleString()[4] == 32) {
                        pAnimGroup->SetParentRootNode(kText.GetHandleString() + 5);
                    }
                    else {
                        pAnimGroup->SetParentRootNode(kText.GetHandleString() + 4);
                    }
                }
                else {
                    char cKeyText[264];
                    BSstrcpy(cKeyText, 260, kText.GetHandleString());
                    char* pTextKey = cKeyText;
                    while (pTextKey && BSstrlen(pTextKey)) {
                        if (*pTextKey == 13) {
                            pTextKey = GetNextLineInString(pTextKey);
                        }
                        uint32_t uiTagLength = BSstrlen(NiControllerSequence::pMorphTag);
                        if (!_strnicmp(pTextKey, NiControllerSequence::pMorphTag, uiTagLength)) {
                            if (!pAnimGroup->cMorphKey) {
                                pAnimGroup->cMorphKey = pTextKey[2];
                            }
                        }
                        else if (!_strnicmp(pTextKey, "BlendIn:", 8u)) {
                            char cBlendInFrames;
                            if (pTextKey[8] == 32) {
                                cBlendInFrames = std::atoi(pTextKey + 9);
                            }
                            else {
                                cBlendInFrames = std::atoi(pTextKey + 8);
                            }
                            pAnimGroup->cBlendInFrames = cBlendInFrames;
                        }
                        else if (!_strnicmp(pTextKey, "BlendOut:", 9u)) {
                            char cBlendOutFrames;
                            if (pTextKey[9] == 32) {
                                cBlendOutFrames = std::atoi(pTextKey + 10);
                            }
                            else {
                                cBlendOutFrames = std::atoi(pTextKey + 9);
                            }
                            pAnimGroup->SetBlendOutFrames(static_cast<uint8_t>(cBlendOutFrames));
                        }
                        else if (!_strnicmp(pTextKey, "Blend:", 6u)) {
                            uint8_t ucBlendFrames;
                            if (pTextKey[6] == 32) {
                                ucBlendFrames = std::atoi(pTextKey + 7);
                            }
                            else {
                                ucBlendFrames = std::atoi(pTextKey + 6);
                            }
                            pAnimGroup->SetBlend(ucBlendFrames);
                        }
                        else if (!_strnicmp(pTextKey, "Decal:", 6u)) {
                            uint8_t ucDecal;
                            if (pTextKey[6] == 32) {
                                ucDecal = std::atoi(pTextKey + 7);
                            }
                            else {
                                ucDecal = std::atoi(pTextKey + 6);
                            }
                            pAnimGroup->SetDecal(ucDecal);
                        }
                        else if (
                            !_strnicmp(pTextKey, "Sound:", BSstrlen("Sound:")) ||
                            !_strnicmp(pTextKey, "Enum:", BSstrlen("Enum:"))
                        ) {
                            bool bEnum = _strnicmp(pTextKey, "Enum:", BSstrlen("Enum:")) == 0;
                            if (pAnimGroup) {
                                char* pKeyValue;
                                char cKeyValue[268];
                                char* pcr = std::strchr(pTextKey, 13);
                                if (pcr) {
                                    *pcr = 0;
                                }
                                if (bEnum) {
                                    pKeyValue = &pTextKey[BSstrlen("Enum: ")];
                                }
                                else {
                                    pKeyValue = &pTextKey[BSstrlen("Sound: ")];
                                }
                                BSstrcpy(cKeyValue, 260, pKeyValue);
                                char* pVolume = nullptr;
                                char* pPitch = nullptr;
                                char* pSeparator = std::strchr(cKeyValue, 44);
                                if (pSeparator) {
                                    pVolume = pSeparator + 1;
                                }
                                else {
                                    pSeparator = std::strchr(cKeyValue, 32);
                                    if (pSeparator) {
                                        pVolume = pSeparator + 1;
                                    }
                                }
                                if (pSeparator) {
                                    *pSeparator = 0;
                                }
                                if (pVolume) {
                                    while (*pVolume == 32) {
                                        ++pVolume;
                                    }
                                    pSeparator = std::strchr(pVolume, 44);
                                    if (pSeparator) {
                                        pPitch = pSeparator + 1;
                                    }
                                    else {
                                        pSeparator = std::strchr(pVolume, 32);
                                        if (pSeparator) {
                                            pPitch = pSeparator + 1;
                                        }
                                    }
                                    if (pSeparator) {
                                        *pSeparator = 0;
                                    }
                                }
                                char bBadNote = 0;
                                if (bEnum) {
                                    ANIM_ENUM i;
                                    for (i = ANIM_ENUM_LEFT;
                                         i < ANIM_ENUM_COUNT && BSstricmp(animEnumNames[i], cKeyValue);
                                         i = static_cast<ANIM_ENUM>(i + 1)) {
                                        ;
                                    }
                                    if (i >= ANIM_ENUM_COUNT) {
                                        bBadNote = 1;
                                    }
                                    else {
                                        uint32_t uiEnumData = iIndex | (pTextKey - cKeyText) << 8;
                                        uint32_t uiSoundCount = pAnimGroup->soundCount;
                                        pAnimGroup->SetSoundCount(uiSoundCount + 1);
                                        pAnimGroup->SetSoundTime(uiSoundCount, fKeyTime);
                                        pAnimGroup->SetSoundEnum(uiSoundCount, i, uiEnumData);
                                    }
                                }
                                else {
                                    TESSound* pSound = pDataHandler->GetSound(cKeyValue);
                                    if (pSound) {
                                        int32_t iSoundID = pSound->iFormID;
                                        BSWin32Audio* pAudio = static_cast<BSWin32Audio*>(&BSAudio::QInstance());
                                        pAudio->PrecacheSoundFromID(iSoundID, 0);
                                        uint32_t uiSoundCount = pAnimGroup->soundCount;
                                        pAnimGroup->SetSoundCount(uiSoundCount + 1);
                                        pAnimGroup->SetSoundTime(uiSoundCount, fKeyTime);
                                        pAnimGroup->SetSoundObject(uiSoundCount, pSound);
                                        if (pVolume && *pVolume) {
                                            float fVolume = std::atof(pVolume);
                                            pAnimGroup->SetSoundVolume(uiSoundCount, fVolume);
                                        }

                                        if (pPitch) {
                                            float fPitch = std::atof(pPitch);
                                            pAnimGroup->SetSoundPitch(uiSoundCount, fPitch);
                                        }
                                    }
                                    else {
                                        bBadNote = 1;
                                    }
                                }
                                if (bBadNote) {
                                    float fFrame = fKeyTime * 30.0;
                                    Warning(
                                        "ANIMATION: Bad note string \"%s\" frame %d in \"%s\".",
                                        pTextKey,
                                        FloatToInt(fFrame),
                                        apFilename
                                    );
                                    bNoteProblem = 1;
                                }
                                if (pcr) {
                                    *pcr = 13;
                                }
                            }
                        }
                        else {
                            ANIM_GROUP_ENUM aeType;
                            for (aeType = ANIM_GROUP_IDLE; aeType < ANIM_GROUP_COUNT;
                                 aeType = static_cast<ANIM_GROUP_ENUM>(aeType + 1)) {
                                if (eGroupType != ANIM_GROUP_NONE) {
                                    aeType = eGroupType;
                                }
                                const char* pcGroupName = animGroupInfo[aeType].pName;
                                if (!BSstricmp(apSequence->m_kName.GetHandleString(), pcGroupName)) {
                                    eGroupType = aeType;
                                    uint32_t uiAction = animGroupInfo[aeType].eActions[iActionSlot];
                                    if (uiAction != -1) {
                                        if (aeType == ANIM_GROUP_SPECIAL_IDLE && iActionSlot == 1) {
                                            uint32_t uiNameLength = BSstrlen(
                                                animGroupActionNames[3][animGroupInfo[aeType].eAction]
                                            );
                                            if (!_strnicmp(
                                                    pTextKey,
                                                    animGroupActionNames[3][animGroupInfo[aeType].eAction],
                                                    uiNameLength
                                                )) {
                                                uiAction = 3;
                                                iActionSlot = 3;
                                            }
                                        }
                                        if (animGroupInfo[aeType].eAction == AGAT_ATTACK && iActionSlot == 2) {
                                            uint32_t uiNameLength = BSstrlen(
                                                animGroupActionNames[3][animGroupInfo[aeType].eAction]
                                            );
                                            if (!_strnicmp(
                                                    pTextKey,
                                                    animGroupActionNames[3][animGroupInfo[aeType].eAction],
                                                    uiNameLength
                                                )) {
                                                uiAction = 3;
                                                iActionSlot = 3;
                                            }
                                        }
                                        uint32_t uiNameLength = BSstrlen(
                                            animGroupActionNames[uiAction][animGroupInfo[aeType].eAction]
                                        );
                                        if (!_strnicmp(
                                                pTextKey,
                                                animGroupActionNames[uiAction][animGroupInfo[aeType].eAction],
                                                uiNameLength
                                            )) {
                                            if (!pAnimGroup) {
                                                if (aeType == ANIM_GROUP_SPECIAL_IDLE) {
                                                    pAnimGroup = new TESAnimGroup(aeType);
                                                }
                                                else {
                                                    pAnimGroup = new TESAnimGroup(
                                                        TESAnimGroup::AnimGroup(emove, eWeapon, aeType, bPowerArmor)
                                                    );
                                                }
                                            }
                                            if (fKeyTime > -1.0) {
                                                pAnimGroup->SetTime(uiAction, fKeyTime);
                                            }
                                            if (animGroupInfo[aeType].eAction == AGAT_LOOP &&
                                                apSequence->m_eCycleType) {
                                                if (animGroupInfo[aeType].eSectionLayer != AGS_MOVEMENT) {
                                                    Warning(
                                                        "ANIMATION: '%s' should be a looping "
                                                        "animation.",
                                                        apFilename
                                                    );
                                                    bNoteProblem = 1;
                                                }
                                            }
                                            else if (
                                                animGroupInfo[aeType].eAction != AGAT_LOOP &&
                                                apSequence->m_eCycleType == NiControllerSequence::CYCLE_LOOP
                                            ) {
                                                Warning(
                                                    "ANIMATION: '%s' should NOT be a "
                                                    "looping animation.",
                                                    apFilename
                                                );
                                                bNoteProblem = 1;
                                            }
                                            ++iActionSlot;
                                        }
                                        else if (
                                            iActionSlot == 5 || animGroupInfo[aeType].eActions[iActionSlot] == AGA_NONE
                                        ) {
                                            char* pcr = std::strchr(pTextKey, 13);
                                            if (pcr) {
                                                *pcr = 0;
                                            }
                                            float fFrame = fKeyTime * 30.0;
                                            Warning(
                                                "ANIMATION: Bad note string \"%s\" frame "
                                                "%d in \"%s\".",
                                                pTextKey,
                                                FloatToInt(fFrame),
                                                apFilename
                                            );
                                            if (pcr) {
                                                *pcr = 13;
                                            }
                                            bNoteProblem = 1;
                                        }
                                    }
                                    break;
                                }
                                if (aeType == eGroupType) {
                                    break;
                                }
                            }
                            if (aeType == ANIM_GROUP_COUNT) {
                                Warning(
                                    "ANIMATION: AnimGroup unable to find sequence '%s' in "
                                    "model '%s'.",
                                    apSequence->m_kName.GetHandleString(),
                                    apFilename
                                );
                                bNoteProblem = 1;
                                pTextKey = nullptr;
                                iIndex = uiNumKeys;
                            }
                        }
                        if (pTextKey) {
                            pTextKey = GetNextLineInString(pTextKey);
                        }
                    }
                }
            }
            else {
                Warning(
                    "ANIMATION: AnimGroup empty note key at time %.2f in sequence '%s' in model '%s'.",
                    fKeyTime,
                    apSequence->m_kName.GetHandleString(),
                    apFilename
                );
                bNoteProblem = 1;
            }
        }
        if (pAnimGroup) {
            double dEndTime;
            pAnimGroup->SetupPriorities(apSequence);
            ANIM_GROUP_ACTION groupAction = AGA_STOP;
            switch (animGroupInfo[pAnimGroup->GetType()].eAction) {
                case AGAT_ATTACK_POWER:
                case AGAT_LOOPING_RELOAD: {
                    dEndTime = pAnimGroup->GetTime(AGA_SPECIAL_IDLE_LOOP_STOP);
                    break;
                }
                case AGAT_ATTACK: {
                    double dLoopStopTime = pAnimGroup->GetTime(AGA_SPECIAL_IDLE_LOOP_STOP);
                    if (pAnimGroup->GetTime(AGA_STOP) > dLoopStopTime) {
                        pAnimGroup->SetTime(2u, pAnimGroup->GetTime(AGA_STOP));
                    }
                    [[fallthrough]];
                }
                case AGAT_ATTACK_THROW: {
                    dEndTime = pAnimGroup->GetTime(AGA_ATTACK_STOP);
                    break;
                }
                case AGAT_SPECIAL_IDLE_LOOP: {
                    double dStopTime = pAnimGroup->GetTime(AGA_STOP);
                    if (pAnimGroup->GetTime(AGA_START) > dStopTime) {
                        pAnimGroup->SetTime(1u, pAnimGroup->GetTime(AGA_START));
                    }
                    double dLoopStopTime = pAnimGroup->GetTime(AGA_SPECIAL_IDLE_LOOP_STOP);
                    if (pAnimGroup->GetTime(AGA_STOP) >= dLoopStopTime) {
                        pAnimGroup->SetTime(2u, pAnimGroup->GetTime(AGA_SPECIAL_IDLE_STOP));
                    }
                    [[fallthrough]];
                }
                case AGAT_ATTACK_MINE:
                case AGAT_ATTACK_SPIN: {
                    dEndTime = pAnimGroup->GetTime(AGA_SPECIAL_IDLE_STOP);
                    break;
                }
                case AGAT_EQUIP:
                case AGAT_UNEQUIP: {
                    groupAction = AGA_SPECIAL_IDLE_LOOP_STOP;
                    [[fallthrough]];
                }
                default: {
                    dEndTime = pAnimGroup->GetTime(groupAction);
                    break;
                }
            }
            if (pAnimGroup->GetTime(AGA_START) >= dEndTime) {
                Warning(
                    "ANIMATION: (%s) End frame is less than or equal to Start frame in \"%s\".",
                    animGroupInfo[pAnimGroup->GetType()].pName,
                    apFilename
                );
                bNoteProblem = 1;
            }
            if (pAnimGroup->cBlendFrames && (pAnimGroup->cBlendInFrames || pAnimGroup->cBlendOutFrames)) {
                Warning(
                    "ANIMATION: (%s) too many blend values in \"%s\". Blend %d, BlendIn %d, BlendOut %d",
                    animGroupInfo[pAnimGroup->GetType()].pName,
                    apFilename,
                    pAnimGroup->cBlendFrames,
                    pAnimGroup->cBlendInFrames,
                    pAnimGroup->cBlendOutFrames
                );
                bNoteProblem = 1;
            }
        }
    }
    MessageHandler::IncDisableWarningCount(false);
    if (bNoteProblem) {
        Warning("ANIMATION: Animation group note problem. See %s file.", MessageHandler::GetWarningFileName());
    }
    return pAnimGroup;
}

void BSAnimGroupSequence::CreateSequenceFromFile(
    const NiStream& kStream,
    uint32_t uiIndex,
    NiPointer<NiControllerSequence>& aspControllerSequence
) {
    NiPointer<NiControllerSequence> spSequence;
    NiControllerSequence::CreateSequenceFromFile(kStream, uiIndex, spSequence);
    BSAnimGroupSequence* pAnimSequence = new BSAnimGroupSequence(nullptr, spSequence.m_pObject);
    aspControllerSequence = static_cast<NiControllerSequence*>(pAnimSequence);
}

NiFixedString& FixedStrings::QAttachSound() {
    return *FixedStrings::AttachSound;
}

void NiControllerSequence::SetAccumRootName(const NiFixedString& kAccumRootName) {
    m_kAccumRootName.Assign(kAccumRootName);
}
