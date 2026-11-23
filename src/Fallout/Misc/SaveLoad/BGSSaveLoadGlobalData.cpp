#include "Fallout/Misc/SaveLoad/BGSSaveLoadGlobalData.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSParticleSystemManager.h"
#include "BSMain/NiObjectGarbage.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorCause.h"
#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Dialogue/MenuTopic.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/MapMenu.h"
#include "Fallout/Interface/Menus/MessageMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Misc/GarbageCollector.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSReconstructFormsInAllFilesMap.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFile.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/rtti.h"

#include <cstdint>

bool BGSSaveLoadGlobalData::LoadGlobalData(uint32_t auiGlobalDataType, BGSLoadGameBuffer* apLoadGameBuffer) {
    switch (auiGlobalDataType) {
        case GLOBAL_DATA_MISC_STATS: {
            MiscStatManager::LoadGame(apLoadGameBuffer);
            return true;
        }
        case GLOBAL_DATA_LOCATION: {
            BGSSaveLoadGlobalData::LoadLocationData(apLoadGameBuffer);
            return true;
        }
        case GLOBAL_DATA_TES: {
            TES::pInstance->LoadGame(apLoadGameBuffer);
            return true;
        }
        case GLOBAL_DATA_GLOBALS: {
            BGSSaveLoadGlobalData::LoadGlobals(apLoadGameBuffer);
            return true;
        }
        case GLOBAL_DATA_PROCESS_LISTS: {
            ReferenceProcessLists.LoadGame(apLoadGameBuffer);
            return true;
        }
        case GLOBAL_DATA_COMBAT: {
            pCombatManager->LoadGame(apLoadGameBuffer);
            return true;
        }
        case GLOBAL_DATA_INTERFACE: {
            Interface::LoadGame(apLoadGameBuffer);
            return true;
        }
        case GLOBAL_DATA_EFFECTS: {
            BGSSaveLoadGlobalData::LoadEffects(apLoadGameBuffer);
            return true;
        }
        case GLOBAL_DATA_WEATHER: {
            Sky::GetInstance()->LoadGame(apLoadGameBuffer);
            return true;
        }
        case GLOBAL_DATA_ACTOR_CAUSES: {
            ActorCause::LoadActorCausesList(apLoadGameBuffer);
            return true;
        }
        case GLOBAL_DATA_RADIO: {
            FalloutRadio::LoadGame(apLoadGameBuffer);
            return true;
        }
        case GLOBAL_DATA_AUDIO: {
            FalloutAudio::LoadGame(apLoadGameBuffer);
            return true;
        }
        case GLOBAL_DATA_TEMP_EFFECTS: {
            return true;
        }
        default: {
            SaveGameWarning("SAVELOAD: Unknown global data type %i in LoadGlobalData()", auiGlobalDataType);
            return false;
        }
    }
}

void BGSSaveLoadGlobalData::LoadGlobalData(BGSSaveLoadFile* apFile, uint32_t aiCount) {
    uint32_t uiType;
    for (uint32_t i = 0; i < aiCount; ++i) {
        BGSLoadGameBuffer kBuffer;
        uiType = 0;
        LoadGameDataFromFileFunction(apFile, &uiType, 4u);
        kBuffer.Load(apFile);
        BGSSaveLoadGlobalData::LoadGlobalData(uiType, &kBuffer);
    }
}

bool BGSSaveLoadGlobalData::InitGlobalData(uint32_t aiGlobalDataType, BGSLoadGameBuffer* apLoadGameBuffer) {
    switch (aiGlobalDataType) {
        case GLOBAL_DATA_MISC_STATS: {
            return false;
        }
        case GLOBAL_DATA_LOCATION: {
            return false;
        }
        case GLOBAL_DATA_TES: {
            return false;
        }
        case GLOBAL_DATA_GLOBALS: {
            return false;
        }
        case GLOBAL_DATA_PROCESS_LISTS: {
            ReferenceProcessLists.InitLoadGame(apLoadGameBuffer);
            return true;
        }
        case GLOBAL_DATA_COMBAT: {
            pCombatManager->InitLoadGame(apLoadGameBuffer);
            return true;
        }
        case GLOBAL_DATA_INTERFACE: {
            return false;
        }
        case GLOBAL_DATA_EFFECTS: {
            return false;
        }
        case GLOBAL_DATA_WEATHER: {
            return false;
        }
        case GLOBAL_DATA_ACTOR_CAUSES: {
            ActorCause::InitLoadActorCausesList(apLoadGameBuffer);
            return true;
        }
        case GLOBAL_DATA_RADIO: {
            FalloutRadio::InitLoadGame(apLoadGameBuffer);
            return true;
        }
        case GLOBAL_DATA_AUDIO: {
            return false;
        }
        case GLOBAL_DATA_TEMP_EFFECTS: {
            return false;
        }
        default: {
            SaveGameWarning("DEFAULT: Unknown global data type %i in InitGlobalData()", aiGlobalDataType);
            return false;
        }
    }
}

void BGSSaveLoadGlobalData::InitGlobalDataRange(uint32_t auiStart, uint32_t auiEnd) {
    BGSLoadGameBuffer kBuffer;
    while (auiStart < auiEnd) {
        BGSSaveLoadGlobalData::InitGlobalData(auiStart++, &kBuffer);
    }
}

// Only TES, combat, interface and radio have work after the forms load; the temp effects block and the rest return
// false, an unknown type warns.
bool BGSSaveLoadGlobalData::FinishLoadGlobalData(uint32_t aiGlobalDataType, BGSLoadGameBuffer* apLoadGameBuffer) {
    switch (aiGlobalDataType) {
        case GLOBAL_DATA_TEMP_EFFECTS: return false;
        case GLOBAL_DATA_MISC_STATS:
        case GLOBAL_DATA_LOCATION: return false;
        case GLOBAL_DATA_TES: TES::pInstance->FinishLoadGame(apLoadGameBuffer); return true;
        case GLOBAL_DATA_GLOBALS:
        case GLOBAL_DATA_PROCESS_LISTS: return false;
        case GLOBAL_DATA_COMBAT: pCombatManager->FinishLoadGame(apLoadGameBuffer); return true;
        case GLOBAL_DATA_INTERFACE: Interface::FinishLoadGame(apLoadGameBuffer); return true;
        case GLOBAL_DATA_EFFECTS:
        case GLOBAL_DATA_WEATHER:
        case GLOBAL_DATA_ACTOR_CAUSES: return false;
        case GLOBAL_DATA_RADIO: FalloutRadio::FinishLoadGame(apLoadGameBuffer); return true;
        case GLOBAL_DATA_AUDIO: return false;
        default:
            SaveGameWarning("DEFAULT: Unknown global data type %i in FinishLoadGlobalData()", aiGlobalDataType);
            return false;
    }
}

// Every block but the location and weather ones has a revert; the globals revert only on a full revert. An unknown
// type warns.
bool BGSSaveLoadGlobalData::RevertGlobalData(
    uint32_t aiGlobalDataType,
    BGSLoadGameBuffer* apLoadGameBuffer,
    bool abFullRevert
) {
    switch (aiGlobalDataType) {
        case GLOBAL_DATA_TEMP_EFFECTS: ReferenceProcessLists.RevertTempEffects(apLoadGameBuffer); return true;
        case GLOBAL_DATA_MISC_STATS: MiscStatManager::Revert(apLoadGameBuffer); return true;
        case GLOBAL_DATA_LOCATION: return false;
        case GLOBAL_DATA_TES: TES::pInstance->Revert(apLoadGameBuffer); return true;
        case GLOBAL_DATA_GLOBALS:
            if (abFullRevert) {
                RevertGlobals();
                return true;
            }
            return false;
        case GLOBAL_DATA_PROCESS_LISTS: ReferenceProcessLists.Revert(apLoadGameBuffer); return true;
        case GLOBAL_DATA_COMBAT: pCombatManager->Revert(apLoadGameBuffer); return true;
        case GLOBAL_DATA_INTERFACE: RevertGlobalData::InterfaceManagerAndMenus(apLoadGameBuffer); return true;
        case GLOBAL_DATA_EFFECTS: RevertGlobalData::ImageSpaceModifiers(); return true;
        case GLOBAL_DATA_WEATHER: return false;
        case GLOBAL_DATA_ACTOR_CAUSES: ActorCause::RevertActorCausesList(apLoadGameBuffer); return true;
        case GLOBAL_DATA_RADIO: FalloutRadio::Revert(apLoadGameBuffer); return true;
        case GLOBAL_DATA_AUDIO: FalloutAudio::Revert(apLoadGameBuffer); return true;
        default:
            SaveGameWarning("DEFAULT: Unknown global data type %i in RevertGlobalData()", aiGlobalDataType);
            return false;
    }
}

void BGSSaveLoadGlobalData::RevertGlobalData(bool abFullRevert) {
    BGSSaveLoadGlobalData::RevertGlobalDataRange(0, 12, abFullRevert);
    BGSSaveLoadGlobalData::RevertGlobalDataRange(1000, 1001, abFullRevert);
}

void BGSSaveLoadGlobalData::RevertGlobals() {
    BGSReconstructFormsInAllFilesMap* preconstructForms = pSaveLoadGame->pReconstructForms;
    for (BSSimpleList<TESGlobal*>* plist = pDataHandler->GetGlobalList(); plist; plist = plist->m_pkNext) {
        preconstructForms->AddForm(plist->m_item, 0);
    }
}

void BGSSaveLoadGlobalData::InitInitialGlobalData() {
    BGSSaveLoadGlobalData::InitGlobalDataRange(0, 0xCu);
}

// Finishes the initial blocks (0-11) after the forms load.
void BGSSaveLoadGlobalData::FinishLoadInitialGlobalData() {
    BGSSaveLoadGlobalData::FinishLoadGlobalDataRange(GLOBAL_DATA_INITIAL_START, GLOBAL_DATA_INITIAL_COUNT);
}

void BGSSaveLoadGlobalData::FinishLoadGlobalDataRange(uint32_t aiStart, uint32_t aiEnd) {
    BGSLoadGameBuffer kBuffer;
    while (aiStart < aiEnd) {
        BGSSaveLoadGlobalData::FinishLoadGlobalData(aiStart++, &kBuffer);
    }
}

void BGSSaveLoadGlobalData::RevertGlobalDataRange(uint32_t aiStart, uint32_t aiEnd, bool abFullRevert) {
    BGSLoadGameBuffer kBuffer;
    while (aiStart < aiEnd) {
        BGSSaveLoadGlobalData::RevertGlobalData(aiStart++, &kBuffer, abFullRevert);
    }
}

bool BGSSaveLoadGlobalData::SaveGlobalData(uint32_t aiGlobalDataType, BGSSaveGameBuffer* apSaveGameBuffer) {
    switch (aiGlobalDataType) {
        case GLOBAL_DATA_MISC_STATS: {
            MiscStatManager::SaveGame(apSaveGameBuffer);
            return true;
        }
        case GLOBAL_DATA_LOCATION: {
            BGSSaveLoadGlobalData::SaveLocationData(apSaveGameBuffer);
            return true;
        }
        case GLOBAL_DATA_TES: {
            TES::pInstance->SaveGameBGS(apSaveGameBuffer);
            return true;
        }
        case GLOBAL_DATA_GLOBALS: {
            BGSSaveLoadGlobalData::SaveGlobals(apSaveGameBuffer);
            return true;
        }
        case GLOBAL_DATA_PROCESS_LISTS: {
            ReferenceProcessLists.SaveGame(apSaveGameBuffer);
            return true;
        }
        case GLOBAL_DATA_COMBAT: {
            pCombatManager->SaveGame(apSaveGameBuffer);
            return true;
        }
        case GLOBAL_DATA_INTERFACE: {
            Interface::SaveGame(apSaveGameBuffer);
            return true;
        }
        case GLOBAL_DATA_EFFECTS: {
            BGSSaveLoadGlobalData::SaveEffects(apSaveGameBuffer);
            return true;
        }
        case GLOBAL_DATA_WEATHER: {
            Sky::GetInstance()->SaveGame(apSaveGameBuffer);
            return true;
        }
        case GLOBAL_DATA_ACTOR_CAUSES: {
            ActorCause::SaveActorCausesList(apSaveGameBuffer);
            return true;
        }
        case GLOBAL_DATA_RADIO: {
            FalloutRadio::SaveGame(apSaveGameBuffer);
            return true;
        }
        case GLOBAL_DATA_AUDIO: {
            FalloutAudio::SaveGame(apSaveGameBuffer);
            return true;
        }
        case GLOBAL_DATA_TEMP_EFFECTS: {
            return true;
        }
        default: {
            SaveGameWarning("SAVELOAD: Unknown global data type %i in SaveGlobalData()", aiGlobalDataType);
            return false;
        }
    }
}

void BGSSaveLoadGlobalData::SaveLocationData(BGSSaveGameBuffer* apSaveGameBuffer) {
    int32_t iCurrentGridY;
    uint32_t inextID;
    int32_t iCurrentGridX;
    NiPoint3 playerLocation;
    inextID = pDataHandler->GetNextFormID();
    apSaveGameBuffer->SaveDataEndian(&inextID, sizeof(inextID), 0);
    apSaveGameBuffer->SaveFormID(TES::pInstance->pWorldSpace, 0);
    iCurrentGridX = TES::pInstance->iCurrentGridX;
    iCurrentGridY = TES::pInstance->iCurrentGridY;
    apSaveGameBuffer->SaveDataEndian(&iCurrentGridX, sizeof(iCurrentGridX), 0);
    apSaveGameBuffer->SaveDataEndian(&iCurrentGridY, sizeof(iCurrentGridY), 0);
    TESForm* plocation = pPlayer->GetWorldSpace();
    if (!plocation) {
        plocation = pPlayer->GetParentCell();
    }
    apSaveGameBuffer->SaveFormID(plocation, 0);
    playerLocation = *pPlayer->QLocationOnReference();
    apSaveGameBuffer->SaveDataEndian(&playerLocation, sizeof(playerLocation), 0);
    LoadingMenu::SaveGame(apSaveGameBuffer);
}

void BGSSaveLoadGlobalData::LoadLocationData(BGSLoadGameBuffer* apLoadGameBuffer) {
    NiPoint3 centerGridLocation;
    int32_t icurrentGridY;
    uint32_t inextID;
    int32_t icurrentGridX;
    NiPoint3 playerLocation;
    apLoadGameBuffer->LoadData(&inextID, sizeof(inextID));
    uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
    TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
    TESWorldSpace* ptesWorldSpace = dynamic_cast<TESWorldSpace*>(pForm);
    apLoadGameBuffer->LoadData(&icurrentGridX, sizeof(icurrentGridX));
    apLoadGameBuffer->LoadData(&icurrentGridY, sizeof(icurrentGridY));
    uiFormID = apLoadGameBuffer->LoadFormID();
    TESForm* pworld = TESForm::GetFormByNumericID(uiFormID);
    apLoadGameBuffer->LoadData(&playerLocation, sizeof(playerLocation));
    LoadingMenu::LoadGame(apLoadGameBuffer);
    bool bvalidPlayerLocation = true;
    if (pworld) {
        if (pworld->GetFormType() == CELL_ID) {
            if (pPlayer->GetParentCell() != pworld) {
                NiPoint3 kAngle = pPlayer->GetAngleOnReference();
                pPlayer->PositionPlayer(playerLocation, kAngle, static_cast<TESObjectCELL*>(pworld), false);
            }
            if (ptesWorldSpace) {
                TES::pInstance->SetWorldSpace(ptesWorldSpace);
            }
            TES::pInstance->iCurrentGridX = icurrentGridX;
            TES::pInstance->iCurrentGridY = icurrentGridY;
        }
        else if (pworld->GetFormType() == WRLD_ID) {
            if (ptesWorldSpace) {
                TES::pInstance->SetWorldSpace(ptesWorldSpace);
            }
            TESWorldSpace* pworldSpace = static_cast<TESWorldSpace*>(pworld);
            if (static_cast<TESWorldSpace*>(pworld)->TESWorldSpace::GetHasBorderRegion() &&
                gs_bBorderRegionsEnabled_General.Bool() &&
                !pworldSpace->IsPointInBorderRegion(playerLocation)) {
                bvalidPlayerLocation = false;
            }
            if (bvalidPlayerLocation) {
                centerGridLocation.x = static_cast<float>(icurrentGridX << 12);
                centerGridLocation.y = static_cast<float>(icurrentGridY << 12);
                centerGridLocation.z = 0.0f;
                TES::pInstance->UpdateCurrentGridCell(&centerGridLocation, false);
                TES::pInstance->ShowLoadingMenu(true, pworld, false);
                int32_t icellX = FloatToInt(playerLocation.x) >> 12;
                int32_t icellY = FloatToInt(playerLocation.y) >> 12;
                TESObjectCELL* pcell = pDataHandler->GetCellFromCellCoord(icellX, icellY, pworldSpace, false);
                if (!pcell || !pcell->Get3D()) {
                    if (pPlayer->GetParentCell()) {
                        pPlayer->GetParentCell()->RemoveReference(pPlayer);
                    }
                    TES::pInstance->LeaveInterior(&centerGridLocation);
                    pcell = pDataHandler->GetCellFromCellCoord(icellX, icellY, pworldSpace, true);
                }
                if (pcell) {
                    pSaveLoadGame->TogglePositioningPlayer(1);
                    NiPoint3 kAngle = pPlayer->GetAngleOnReference();
                    pPlayer->PositionPlayer(playerLocation, kAngle, pcell, false);
                    pSaveLoadGame->TogglePositioningPlayer(0);
                }
                else {
                    bvalidPlayerLocation = false;
                }
            }
        }
        else {
            bvalidPlayerLocation = false;
        }
    }
    else {
        bvalidPlayerLocation = false;
    }
    if (!bvalidPlayerLocation) {
        pSaveLoadGame->SetPlayerLocationInvalid(true);
        for (BSSimpleList<TESQuest*>* plist = pDataHandler->GetQuestList(); plist && plist->m_item;
             plist = plist->m_pkNext) {
            int32_t iFormID = plist->m_item->iFormID;
            BSprintf("Quest %s (%08x) compared to Patch04Debug\n", plist->m_item->GetFullName(), iFormID);
            if (plist->m_item->GetFullNameLength() > 8 && std::strstr(plist->m_item->GetFullName(), "Patch04Debug")) {
                pPlayer->patch04DebugQuest = plist->m_item;
                break;
            }
        }
        pPlayer->SetControlsDisabled(false, PlayerCharacter::DPC_ALL);
        uint32_t isafeMarkerID = std::strtol(sSaveGameSafeMarkerID.String(), nullptr, 16);
        pForm = TESForm::GetFormByNumericID(isafeMarkerID);
        TESObjectREFR* pref = dynamic_cast<TESObjectREFR*>(pForm);
        if (pref) {
            NiPoint3 loc = pref->GetLocationOnReference();
            NiPoint3 angle = pref->GetAngleOnReference();
            TESObjectCELL* pcell = pref->GetParentCell();
            TESWorldSpace* pworldSpace = pref->GetWorldSpace();
            bvalidPlayerLocation = true;
            if (pworldSpace) {
                pPlayer->PositionPlayerExterior(loc, angle, pworldSpace, true);
            }
            else if (pcell) {
                pPlayer->PositionPlayer(loc, angle, pcell, true);
            }
            else {
                bvalidPlayerLocation = false;
            }
        }
        if (!bvalidPlayerLocation) {
            pForm = TESForm::GetFormByNumericID(0xDA726u);
            TESWorldSpace* pworldSpace = dynamic_cast<TESWorldSpace*>(pForm);
            if (pworldSpace) {
                TESObjectCELL* pcell = pworldSpace->GetCellFromCellCoord(-18, 0);
                if (!pcell) {
                    pcell = pworldSpace->LoadCell(-18, 0);
                }
                if (!pcell) {
                    pcell = pworldSpace->GetCellFromCellCoord(0, 0);
                }
                if (!pcell) {
                    pcell = pworldSpace->LoadCell(0, 0);
                }
                if (!pcell) {
                    pcell = pDataHandler->NewCell(nullptr, 0, 0, pworldSpace);
                }
                if (pcell) {
                    pPlayer->CenterOnCell(nullptr, pcell);
                }
            }
        }
    }
    TES::pInstance->ClearAllCells(true, false);
    GarbageCollector::ClearAll(false);
    NiObjectGarbage::ClearAll(false);
}

void BGSSaveLoadGlobalData::SaveGlobals(BGSSaveGameBuffer* apSaveGameBuffer) {
    float fvalue;
    BSSimpleList<TESGlobal*>* pglobals = pDataHandler->GetGlobalList();
    uint32_t icount = 0;
    uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
    for (TESGlobal* pglobal : *pglobals) {
        if (pglobal && !pglobal->TESForm::GetHasSpokenFlag()) {
            apSaveGameBuffer->SaveFormID(pglobal, 0);
            fvalue = pglobal->fValue;
            apSaveGameBuffer->SaveDataEndian(&fvalue, sizeof(fvalue), 0);
            ++icount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(icount, isaveGameBufferPos);
}

void BGSSaveLoadGlobalData::LoadGlobals(BGSLoadGameBuffer* apLoadGameBuffer) {
    float fvalue;
    BGSReconstructFormsInAllFilesMap* preconstructForms = pSaveLoadGame->pReconstructForms;
    using GlobalsMap_t = NiTMap<TESGlobal*, bool>;
    GlobalsMap_t GlobalsMap(37u);
    for (TESGlobal* pglobal : *pDataHandler->GetGlobalList()) {
        if (pglobal) {
            GlobalsMap.SetAt(pglobal, true);
        }
    }
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
        uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
        TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
        TESGlobal* pglobal = dynamic_cast<TESGlobal*>(pForm);
        fvalue = 0.0f;
        apLoadGameBuffer->LoadData(&fvalue, sizeof(fvalue));
        if (pglobal) {
            if (!pglobal->TESForm::GetHasSpokenFlag()) {
                pglobal->SetValue(fvalue);
            }
            GlobalsMap.RemoveAt(pglobal);
        }
    }
    NiTMapItem<TESGlobal*, bool>* pos = GlobalsMap.GetFirstPos();
    while (pos) {
        TESGlobal* pglobal = nullptr;
        bool bvalue = false;
        GlobalsMap.GetNext(pos, pglobal, bvalue);
        if (!pglobal->TESForm::GetHasSpokenFlag()) {
            preconstructForms->AddForm(pglobal, 0);
        }
    }
}

void BGSSaveLoadGlobalData::SaveEffects(BGSSaveGameBuffer* apSaveGameBuffer) {
    float foffset;
    float fstrength;
    uint32_t icount = 0;
    uint32_t icountPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<NiPointer<ImageSpaceModifierInstance>>* plist = &TES::pInstance->listActiveImageSpaceModifiers;
         plist;
         plist = plist->m_pkNext) {
        ImageSpaceModifierInstance* pmod = plist->m_item.m_pObject;
        if (pmod && !pmod->QPermanent()) {
            ImageSpaceModifierInstanceForm* pinstanceForm = pmod->IsForm();
            if (pinstanceForm) {
                fstrength = pinstanceForm->fStrength;
                apSaveGameBuffer->SaveDataEndian(&fstrength, sizeof(fstrength), 0);
                double dTickCount = TESImageSpaceModifier::GetTickCount();
                foffset = dTickCount - pinstanceForm->fStartTime;
                apSaveGameBuffer->SaveDataEndian(&foffset, sizeof(foffset), 0);
                apSaveGameBuffer->SaveFormID(pinstanceForm->pImageSpaceMod, 0);
                ++icount;
            }
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(icount, icountPosition);
}

void BGSSaveLoadGlobalData::LoadEffects(BGSLoadGameBuffer* apLoadGameBuffer) {
    float foffset;
    float fstrength;
    uint32_t iextraCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < iextraCount; ++i) {
        fstrength = 0.0f;
        apLoadGameBuffer->LoadData(&fstrength, sizeof(fstrength));
        foffset = 0.0f;
        if (apLoadGameBuffer->GetVersion() >= 14u) {
            apLoadGameBuffer->LoadData(&foffset, sizeof(foffset));
        }
        uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
        TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
        TESImageSpaceModifier* pmod = dynamic_cast<TESImageSpaceModifier*>(pForm);
        if (pmod) {
            ImageSpaceModifierInstanceForm* pinstance = ImageSpaceModifierInstanceForm::Trigger(
                pmod,
                fstrength,
                nullptr
            );
            if (pinstance) {
                float fAge = fCellAppTime - foffset;
                pinstance->SetAge(fAge);
            }
        }
    }
}

void RevertGlobalData::InterfaceManagerAndMenus(BGSLoadGameBuffer* apLoadGameBuffer) {
    bIsReverting = true;
    Interface::ClearAllSubtitlesAndMessages();
    HUDMainMenu::Revert();
    if (Interface::InDialog()) {
        MobileObject* pPartner = static_cast<MobileObject*>(GetDialogMenuParterRef());
        pPlayer->FocusOnActor(pPartner, 0.0f, true);
        MenuTopicManager::Destroy();
    }
    Interface::CloseVATSMenu();
    VATSMenu::ClearTargetList();
    Interface::CloseAllSupportMenus(false);
    MessageMenu::ClearMessageQueue();
    StatsMenu::ClearPerks();
    MapMenu::ClearSelectedNote();
    MapMenu::ClearAudioLogSubtites();
    Interface::PopFromEnterStack(1, true);
    InterfaceManager::Instance()->EnterRenderedMenu(nullptr);
    InterfaceManager::Instance()->ePipboyState = InterfaceManager::PS_CLOSED;
    InterfaceManager::Instance()->xTutorialManager.Reset();
}

void RevertGlobalData::ImageSpaceModifiers() {
    ScreenSplatter::Clear();
    CustomBloodSplatter();
    Main::GetMasterParticleSystemManager()->ClearAllParticles();
    // The list is walked from its head node; a removed node's successor moves into it, so the walk stays put there.
    BSSimpleList<NiPointer<ImageSpaceModifierInstance>>* pnode = &TES::pInstance->listActiveImageSpaceModifiers;
    while (pnode) {
        ImageSpaceModifierInstance* pinstance = pnode->m_item.m_pObject;
        if (!pinstance || pinstance->QPermanent()) {
            if (pinstance) {
                pinstance->SetStrength(0.0f);
            }
            pnode = pnode->m_pkNext;
        }
        else {
            NiPointer<ImageSpaceModifierInstance> spinstance(pinstance);
            pnode->Remove(spinstance);
        }
    }
}

bool ImageSpaceModifierInstance::QPermanent() {
    return (iFlags & PERMANENT) != 0;
}

float ImageSpaceModifierInstance::QStrength() {
    return fStrength;
}
