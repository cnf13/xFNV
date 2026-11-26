#include "Fallout Shared/BGSTalkingActivator.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSVoiceType.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/TeleportPath.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectACTI.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/DialoguePackage.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

#include <limits>

BSSimpleList<TESObjectREFR*> BGSTalkingActivator::RadioStationList;

BGSTalkingActivator::BGSTalkingActivator() {
    SetFormType(TACT_ID);
    BGSTalkingActivator::InitializeData();
}

BGSTalkingActivator::~BGSTalkingActivator() {}

void BGSTalkingActivator::InitializeData() {
    pVoiceType = nullptr;
    TESObjectACTI::InitializeData();
}

// Reads the full name, the model and texture-swap chunks, the script (SCRI), the destruction chunks, the editor ID,
// the object bound, the looping sound (SNAM), the radio template (INAM) and the voice type (VNAM).
bool BGSTalkingActivator::Load(TESFile* apFile) {
    LoadForm(apFile);
    SetInitialized(false);
    for (;;) {
        CHUNK_ID eChunk = static_cast<CHUNK_ID>(apFile->GetTESChunk());
        if (eChunk == NO_CHUNK) {
            break;
        }

        switch (eChunk) {
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
            case MODL_ID:
            case MODT_ID:
            case MOSD_ID:
            case MODD_ID: TESModel::LoadModelChunk(static_cast<TESModel*>(this), apFile); break;
            case MODS_ID: {
                uint32_t uiSize = apFile->GetChunkSize();
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiSize));
                apFile->GetChunkData(pBuffer, uiSize);
                uint16_t usFormVersion = apFile->GetFormVersion();
                static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiSize, usFormVersion);
                break;
            }
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pEditorID = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pEditorID, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pEditorID);
                break;
            }
            case OBND_ID: static_cast<TESBoundObject*>(this)->LoadObjectBound(apFile); break;
            case DSTD_ID:
            case DEST_ID:
                BGSDestructibleObjectForm::LoadChunk(static_cast<BGSDestructibleObjectForm*>(this), apFile);
                break;
            case SCRI_ID: {
                uint32_t uiScript = 0;
                apFile->GetChunkData<uint32_t>(uiScript);
                TESScriptableForm* pScriptableForm = static_cast<TESScriptableForm*>(this);
                // The chunk's value is stored in the pointer field unchanged.
                pScriptableForm->SetFormScript(reinterpret_cast<Script*>(uiScript));
                pScriptableForm->InitItem(this);
                break;
            }
            case SNAM_ID: {
                uint32_t uiSoundLoop = 0;
                apFile->GetChunkData<uint32_t>(uiSoundLoop);
                // The chunk's value is stored in the pointer field unchanged.
                SetLoopingSound(reinterpret_cast<TESSound*>(uiSoundLoop));
                break;
            }
            case INAM_ID: {
                uint32_t uiRadioTemplate = 0;
                apFile->GetChunkData<uint32_t>(uiRadioTemplate);
                // The chunk's value is stored in the pointer field unchanged.
                SetRadioTemplate(reinterpret_cast<TESSound*>(uiRadioTemplate));
                break;
            }
            case VNAM_ID: {
                uint32_t uiVoiceType = 0;
                apFile->GetChunkData<uint32_t>(uiVoiceType);
                // The chunk's value is stored in the pointer field unchanged.
                SetVoiceType(reinterpret_cast<BGSVoiceType*>(uiVoiceType));
                break;
            }
        }

        if (!apFile->NextChunk()) {
            break;
        }
    }
    return true;
}

void BGSTalkingActivator::Save() {
    TESForm::StartForm();
    static_cast<TESFullName*>(this)->Save();
    static_cast<TESModelTextureSwap*>(static_cast<TESModel*>(this))->Save(MODL_ID, MODT_ID, MODS_ID);
    static_cast<TESScriptableForm*>(this)->Save();
    static_cast<BGSDestructibleObjectForm*>(this)->Save();
    if (pSoundLoop) {
        uint32_t uiFormID = pSoundLoop->TESForm::GetFormID();
        TESForm::AddChunk(SNAM_ID, uiFormID);
    }
    if (pVoiceType) {
        uint32_t uiFormID = pVoiceType->TESForm::GetFormID();
        TESForm::AddChunk(VNAM_ID, uiFormID);
    }
    if (pRadioTemplate) {
        uint32_t uiFormID = pRadioTemplate->TESForm::GetFormID();
        TESForm::AddChunk(INAM_ID, uiFormID);
    }
    TESForm::CloseForm();
}

// Saves the activator's data, then the speaker when the save changed it.
void BGSTalkingActivator::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESObjectACTI::SaveGame(apSaveGameBuffer);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_TALKING_ACTIVATOR_SPEAKER)) {
        apSaveGameBuffer->SaveFormID(pTempRef, 0);
    }
}

// Loads the activator's data, then the speaker the save changed. The 4-byte form ID is read straight over pTempRef.
void BGSTalkingActivator::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectACTI::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_TALKING_ACTIVATOR_SPEAKER)) {
        apLoadGameBuffer->LoadFormID(pTempRef);
    }
}

// Calls the base form's empty Revert directly, then resolves the speaker form ID LoadGame stored over pTempRef: the
// field's 4 raw bytes are handed to GetFormByNumericID as they stand.
void BGSTalkingActivator::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::Revert(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_TALKING_ACTIVATOR_SPEAKER)) {
        Actor* pSpeaker = nullptr;
        if (pTempRef) {
            TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTempRef)));
            pSpeaker = dynamic_cast<Actor*>(pForm);
        }
        pTempRef = pSpeaker;
    }
}

void BGSTalkingActivator::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_TALKING_ACTIVATOR_SPEAKER)) {
        pTempRef = nullptr;
    }
}

void BGSTalkingActivator::InitItem() {
    if (!GetInitialized()) {
        TESObjectACTI::InitItem();
        pTempRef = nullptr;
        uint32_t uiTempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pVoiceType));
        if (uiTempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(uiTempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(uiTempInt);
            pVoiceType = dynamic_cast<BGSVoiceType*>(pForm);
            if (!pVoiceType) {
                int32_t iActivatorID = iFormID;
                const char* pEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find voice type (%08X) for activator '%s' (%08X).",
                    uiTempInt,
                    pEditorID,
                    iActivatorID
                );
            }
        }
        SetInitialized(true);
    }
}

void BGSTalkingActivator::Copy(TESForm* apCopy) {
    auto* pCopy = dynamic_cast<BGSTalkingActivator*>(apCopy);
    if (pCopy) {
        TESObjectACTI::Copy(apCopy);
        SetVoiceType(pCopy->GetVoiceType());
    }
}

// True when the forms differ: apCompare is no talking activator, the activator parts differ or the voice types do.
bool BGSTalkingActivator::Compare(TESForm* apCompare) {
    auto* pCompare = dynamic_cast<BGSTalkingActivator*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (TESObjectACTI::Compare(apCompare)) {
        return true;
    }
    return GetVoiceType() != pCompare->GetVoiceType();
}

bool BGSTalkingActivator::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIDFlag,
    TESBoundObject* apObjectToGet,
    int32_t aiCount
) {
    if (apActionRef == pPlayer) {
        ExtraTalkingActor* pExtraTalk = apItemActivated->m_Extra.GetTalkingActorExtra();
        if (pExtraTalk && pExtraTalk->pTalkObject) {
            const char* pMessage = sObjectInUse.String();
            Interface::ShowMessage(pMessage, 0, nullptr, nullptr, 2.0f, false);
            return false;
        }
    }
    MobileObject* pActor;
    if (apItemActivated->IsMobileObject()) {
        pActor = static_cast<MobileObject*>(apItemActivated);
    }
    else {
        apItemActivated->m_Extra.SetTalkingActorExtra(this, apItemActivated);
        apItemActivated->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
        pActor = apItemActivated->m_Extra.GetTalkingActorExtra()->pTalkObject;
    }
    MobileObject* pActionAct;
    if (apActionRef->IsActor()) {
        pActionAct = static_cast<MobileObject*>(apActionRef);
    }
    else {
        pActionAct = apActionRef->AddTalkingActivatorMobileObjectREFR();
    }
    if (pActor) {
        if (apActionRef == pPlayer) {
            pVATS.QuitVATSPlayback(false, false);
            Interface::QueueMenuCreate(
                Interface::OPEN_DIALOG,
                pActor,
                nullptr,
                pTempRef,
                Interface::CMM_STANDARD,
                false
            );
        }
        else {
            DialoguePackage* pDialogue = nullptr;
            if (pActor->GetCurrentPackage()) {
                DialoguePackage* pCurrentPackage = static_cast<DialoguePackage*>(pActor->GetCurrentPackage());
                if (pCurrentPackage->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                    pDialogue = static_cast<DialoguePackage*>(pActor->GetCurrentPackage());
                }
            }
            if (pDialogue && pDialogue->GetActorStartedConversation() == pActionAct) {
                return true;
            }
            pActor->InitiateDialogue(
                pActionAct,
                static_cast<PackageLocation*>(nullptr),
                static_cast<PackageLocation*>(nullptr),
                true,
                false,
                false,
                static_cast<TESTopic*>(nullptr),
                false,
                false
            );
        }
    }
    return true;
}

bool BGSTalkingActivator::IsRadioStation() {
    return TESForm::GetNeedtoChangeProcess();
}

bool BGSTalkingActivator::IsNonPipboyRadioStation() {
    return IsRadioStation() && TESObjectACTI::GetNonPipboyRadio();
}

void BGSTalkingActivator::SetVoiceType(BGSVoiceType* apVoiceType) {
    pVoiceType = apVoiceType;
}

BGSVoiceType* BGSTalkingActivator::GetVoiceType() {
    return pVoiceType;
}

MobileObject* BGSTalkingActivator::AddMobileObjectExtra(TESObjectREFR* apRef) {
    MobileObject* pObject = nullptr;
    apRef->m_Extra.SetTalkingActorExtra(this, apRef);
    apRef->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
    if (apRef->m_Extra.GetTalkingActorExtra()) {
        return apRef->m_Extra.GetTalkingActorExtra()->pTalkObject;
    }
    return pObject;
}

// Sets the speaker to pref when it is null or an actor, and marks or clears the speaker change flag.
void BGSTalkingActivator::SetTempRef(TESObjectREFR* pref) {
    if (!pref || pref->IsActor()) {
        // pTempRef is declared as Actor*, and the test above guards the cast.
        pTempRef = static_cast<Actor*>(pref);
        if (pref) {
            AddChange(CHANGE_TALKING_ACTIVATOR_SPEAKER);
        }
        else {
            RemoveChange(CHANGE_TALKING_ACTIVATOR_SPEAKER);
        }
    }
}

Actor* BGSTalkingActivator::GetTempRef() {
    return pTempRef;
}

bool BGSTalkingActivator::IsNonPipboyRadioStation(TESForm* apForm, int32_t aiData) {
    bool bResult = false;
    if (!apForm) {
        return bResult;
    }
    if (apForm->GetFormType() == TACT_ID && static_cast<BGSTalkingActivator*>(apForm)->IsRadioStation()) {
        return static_cast<BGSTalkingActivator*>(apForm)->IsNonPipboyRadioStation();
    }
    return bResult;
}

void BGSTalkingActivator::ClearRadioStationList() {
    RadioStationList.RemoveAll();
}

// Each placed radio station (a disabled one only for a listener outside any worldspace) is heard by its range type:
// within its radius (the straight distance in the listener's worldspace, or the teleport door path length from an
// interior), everywhere, in its worldspace and the interiors whose door path leads only there, in interiors linked by
// doors, or in the listener's cell. A heard station goes to apOutList and its distance (-1 for the range types without
// one) to apDistancesList. The conditionally constructed locations of the linked-interior case are destroyed in that
// branch.
void BGSTalkingActivator::GetRadioStationsInRangeOfRef(
    TESObjectREFR* apListenRef,
    BSSimpleList<TESObjectREFR*>* apOutList,
    BSSimpleList<float>* apDistancesList
) {
    if (!apOutList || !apListenRef || !apListenRef->GetParentCell()) {
        return;
    }
    TESWorldSpace* pWorldSpace = apListenRef->GetWorldSpace();
    TESObjectCELL* pInteriorCell = nullptr;
    if (!pWorldSpace) {
        pInteriorCell = apListenRef->GetParentCell();
    }
    for (BSSimpleList<TESObjectREFR*>* pNode = &RadioStationList; pNode && !pNode->IsEmpty();
         pNode = pNode->GetNext()) {
        TESObjectREFR* pReference = pNode->GetItem();
        bool bInRange = false;
        float fDistance = std::numeric_limits<float>::max();
        if (pReference->GetDisabled() && pWorldSpace) {
            continue;
        }
        switch (pReference->GetRadioStationRangeType()) {
            case RADIO_RANGE_RADIUS: {
                TESObjectREFR* pPositionRef = pReference->GetRadioStationPositionRef();
                if (!pPositionRef) {
                    pPositionRef = pReference;
                }
                TESWorldSpace* pRadioWorldSpace = pPositionRef->GetWorldSpace();
                if (!pRadioWorldSpace) {
                    uint32_t uiFormID = pPositionRef->GetFormID();
                    Warning(
                        "DEFAULT: Radio station exterior position ref '%s' (%08X) is not placed in an exterior.",
                        pPositionRef->GetFormEditorID(),
                        uiFormID
                    );
                    break;
                }
                if (pWorldSpace && pWorldSpace == pRadioWorldSpace) {
                    fDistance = apListenRef->GetDistanceFromPoint(pPositionRef->GetLocationOnReference());
                }
                else if (!pWorldSpace) {
                    // Gets the radius before it builds the two locations.
                    float fRadius = pReference->GetRadioStationRadius();
                    alignas(PathingLocation) uint8_t aListenerStorage[sizeof(PathingLocation)];
                    PathingLocation* pListenerLocation = ::new (aListenerStorage) PathingLocation(apListenRef);
                    alignas(PathingLocation) uint8_t aTargetStorage[sizeof(PathingLocation)];
                    PathingLocation* pTargetLocation = ::new (aTargetStorage) PathingLocation(pReference);
                    fDistance = Pathing::ComputeTeleportDoorPathLength(
                        *pTargetLocation,
                        *pListenerLocation,
                        fRadius,
                        nullptr,
                        Pathing::DOOR_LOCK_DISPOSITION_RADIO
                    );
                    pTargetLocation->~PathingLocation();
                    pListenerLocation->~PathingLocation();
                }
                // An unordered (NaN) distance or one not below the radius is out of range.
                if (fDistance < pReference->GetRadioStationRadius()) {
                    bInRange = true;
                }
                break;
            }
            case RADIO_RANGE_EVERYWHERE:
                bInRange = true;
                fDistance = -1.0f;
                break;
            case RADIO_RANGE_WORLDSPACE: {
                TESWorldSpace* pRadioWorldSpace = pReference->GetWorldSpace();
                if (!pRadioWorldSpace) {
                    uint32_t uiFormID = pReference->GetFormID();
                    Warning(
                        "DEFAULT: Radio station ref '%s' (%08X) is set to worldspace and connected interiors but is "
                        "not "
                        "placed in an exterior.",
                        pReference->GetFormEditorID(),
                        uiFormID
                    );
                    break;
                }
                if (pWorldSpace) {
                    if (pWorldSpace == pRadioWorldSpace) {
                        bInRange = true;
                        fDistance = -1.0f;
                    }
                    break;
                }
                alignas(TeleportPath) uint8_t aPathStorage[sizeof(TeleportPath)];
                TeleportPath* pPath = ::new (aPathStorage) TeleportPath;
                alignas(PathingLocation) uint8_t aListenerStorage[sizeof(PathingLocation)];
                PathingLocation* pListenerLocation = ::new (aListenerStorage) PathingLocation(apListenRef);
                alignas(PathingLocation) uint8_t aTargetStorage[sizeof(PathingLocation)];
                PathingLocation* pTargetLocation = ::new (aTargetStorage) PathingLocation(pReference);
                bool bFound = Pathing::BuildTeleportDoorPath(
                    *pTargetLocation,
                    *pListenerLocation,
                    pPath,
                    Pathing::TELEPORT_DOOR_PATH_START_SPACE_ONLY,
                    0.0f,
                    nullptr,
                    Pathing::DOOR_LOCK_DISPOSITION_IGNORELOCKS
                );
                pTargetLocation->~PathingLocation();
                pListenerLocation->~PathingLocation();
                if (bFound) {
                    bInRange = true;
                    fDistance = -1.0f;
                    uint32_t uiSpaceCount = pPath->Spaces.GetSize();
                    for (uint32_t i = 0; i < uiSpaceCount; ++i) {
                        TESWorldSpace* pSpaceWorldSpace = pPath->Spaces[i].pWorldSpace;
                        if (pSpaceWorldSpace && pSpaceWorldSpace != pRadioWorldSpace) {
                            bInRange = false;
                            break;
                        }
                    }
                }
                pPath->~TeleportPath();
                break;
            }
            case RADIO_RANGE_LINKEDINT: {
                if (!pInteriorCell) {
                    break;
                }
                alignas(TeleportPath) uint8_t aPathStorage[sizeof(TeleportPath)];
                TeleportPath* pPath = ::new (aPathStorage) TeleportPath;
                bool bConnected;
                if (apListenRef->GetParentCell() &&
                    pReference->GetParentCell() &&
                    apListenRef->GetParentCell() == pReference->GetParentCell()) {
                    bConnected = true;
                }
                else {
                    alignas(PathingLocation) uint8_t aListenerStorage[sizeof(PathingLocation)];
                    PathingLocation* pListenerLocation = ::new (aListenerStorage) PathingLocation(apListenRef);
                    alignas(PathingLocation) uint8_t aTargetStorage[sizeof(PathingLocation)];
                    PathingLocation* pTargetLocation = ::new (aTargetStorage) PathingLocation(pReference);
                    bConnected = Pathing::BuildTeleportDoorPath(
                        *pTargetLocation,
                        *pListenerLocation,
                        pPath,
                        Pathing::TELEPORT_DOOR_PATH_INTERIORS_ONLY,
                        0.0f,
                        nullptr,
                        Pathing::DOOR_LOCK_DISPOSITION_IGNORELOCKS
                    );
                    pTargetLocation->~PathingLocation();
                    pListenerLocation->~PathingLocation();
                }
                if (bConnected) {
                    bInRange = true;
                    fDistance = -1.0f;
                    uint32_t uiSpaceCount = pPath->Spaces.GetSize();
                    for (uint32_t i = 0; i < uiSpaceCount; ++i) {
                        if (pPath->Spaces[i].pWorldSpace) {
                            bInRange = false;
                            break;
                        }
                    }
                }
                pPath->~TeleportPath();
                break;
            }
            case RADIO_RANGE_CURRENT_CELL:
                if (apListenRef->GetParentCell() &&
                    pReference->GetParentCell() &&
                    apListenRef->GetParentCell() == pReference->GetParentCell()) {
                    bInRange = true;
                    fDistance = -1.0f;
                }
                break;
            default: break;
        }
        if (bInRange) {
            apOutList->AddHead(pReference);
            if (apDistancesList) {
                apDistancesList->AddHead(fDistance);
            }
        }
    }
}

bool BGSTalkingActivator::IsRadioStation(TESForm* apForm, int32_t aiData) {
    bool bIsRadio = false;
    if (apForm &&
        apForm->GetFormType() == TACT_ID &&
        static_cast<BGSTalkingActivator*>(apForm)->BGSTalkingActivator::IsRadioStation()) {
        bIsRadio = true;
    }
    return bIsRadio;
}

void BGSTalkingActivator::AddRadioStation(TESObjectREFR* apRef) {
    if (apRef && IsRadioStation(apRef->GetObjectReference(), 0)) {
        RadioStationList.AddHead(apRef);
    }
}

void TESObjectACTI::SetRadioTemplate(TESSound* apSound) {
    pRadioTemplate = apSound;
}
