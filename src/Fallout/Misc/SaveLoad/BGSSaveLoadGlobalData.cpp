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
    bool result;
    if (auiGlobalDataType > 1000) {
    LABEL_18:;
        SaveGameWarning("SAVELOAD: Unknown global data type %i in LoadGlobalData()", auiGlobalDataType);
        return false;
    }

    if (auiGlobalDataType == 1000) {
        return true;
    }

    switch (auiGlobalDataType) {
        case 0u: {
            MiscStatManager::LoadGame(apLoadGameBuffer);
            result = true;
            break;
        }
        case 1u: {
            BGSSaveLoadGlobalData::LoadLocationData(apLoadGameBuffer);
            result = true;
            break;
        }
        case 2u: {
            TES::pInstance->LoadGame(apLoadGameBuffer);
            result = true;
            break;
        }
        case 3u: {
            BGSSaveLoadGlobalData::LoadGlobals(apLoadGameBuffer);
            result = true;
            break;
        }
        case 4u: {
            ReferenceProcessLists.LoadGame(apLoadGameBuffer);
            result = true;
            break;
        }
        case 5u: {
            pCombatManager->LoadGame(apLoadGameBuffer);
            result = true;
            break;
        }
        case 6u: {
            Interface::LoadGame(apLoadGameBuffer);
            result = true;
            break;
        }
        case 7u: {
            BGSSaveLoadGlobalData::LoadEffects(apLoadGameBuffer);
            result = true;
            break;
        }
        case 8u: {
            Sky* Instance = Sky::GetInstance();
            Instance->LoadGame(apLoadGameBuffer);
            result = true;
            break;
        }
        case 9u: {
            ActorCause::LoadActorCausesList(apLoadGameBuffer);
            result = true;
            break;
        }
        case 10u: {
            FalloutRadio::LoadGame(apLoadGameBuffer);
            result = true;
            break;
        }
        case 11u: {
            FalloutAudio::LoadGame(apLoadGameBuffer);
            result = true;
            break;
        }
        default: {
            goto LABEL_18;
        }
    }

    return result;
}

void BGSSaveLoadGlobalData::LoadGlobalData(BGSSaveLoadFile* apFile, uint32_t aiCount) {
    uint32_t apData;
    BGSLoadGameBuffer apMemory;
    uint32_t i;
    int32_t v5;
    for (i = 0; i < aiCount; ++i) {
        apMemory.~BGSLoadGameBuffer();
        ::new (&apMemory) BGSLoadGameBuffer;
        v5 = 0;
        apData = 0;
        LoadGameDataFromFileFunction(apFile, &apData, 4u);
        (&apMemory)->Load(apFile);
        BGSSaveLoadGlobalData::LoadGlobalData(apData, &apMemory);
        v5 = (-1);
    }
}

bool BGSSaveLoadGlobalData::InitGlobalData(uint32_t aiGlobalDataType, BGSLoadGameBuffer* apLoadGameBuffer) {
    bool result;
    if (aiGlobalDataType > 1000) {
    LABEL_18:;
        SaveGameWarning("DEFAULT: Unknown global data type %i in InitGlobalData()", aiGlobalDataType);
        return false;
    }

    if (aiGlobalDataType == 1000) {
        return false;
    }

    switch (aiGlobalDataType) {
        case 0u: {
            result = false;
            break;
        }
        case 1u: {
            result = false;
            break;
        }
        case 2u: {
            result = false;
            break;
        }
        case 3u: {
            result = false;
            break;
        }
        case 4u: {
            ReferenceProcessLists.InitLoadGame(apLoadGameBuffer);
            result = true;
            break;
        }
        case 5u: {
            pCombatManager->InitLoadGame(apLoadGameBuffer);
            result = true;
            break;
        }
        case 6u: {
            result = false;
            break;
        }
        case 7u: {
            result = false;
            break;
        }
        case 8u: {
            result = false;
            break;
        }
        case 9u: {
            ActorCause::InitLoadActorCausesList(apLoadGameBuffer);
            result = true;
            break;
        }
        case 10u: {
            FalloutRadio::InitLoadGame(apLoadGameBuffer);
            result = true;
            break;
        }
        case 11u: {
            result = false;
            break;
        }
        default: {
            goto LABEL_18;
        }
    }

    return result;
}

void BGSSaveLoadGlobalData::InitGlobalDataRange(uint32_t auiStart, uint32_t auiEnd) {
    BGSLoadGameBuffer v3;
    v3.~BGSLoadGameBuffer();
    ::new (&v3) BGSLoadGameBuffer;
    int32_t v4 = 0;
    while (auiStart < auiEnd) {
        BGSSaveLoadGlobalData::InitGlobalData(auiStart++, &v3);
    }
    v4 = (-1);
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
    BGSReconstructFormsInAllFilesMap* ObjectReference = pSaveLoadGame->pReconstructForms;
    for (BSSimpleList<TESGlobal*>* ecx0 = pDataHandler->GetGlobalList(); ecx0; ecx0 = ecx0->m_pkNext) {
        TESForm* apForm = ecx0->m_item;
        ObjectReference->AddForm(apForm, 0);
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
    BGSLoadGameBuffer v3;
    v3.~BGSLoadGameBuffer();
    ::new (&v3) BGSLoadGameBuffer;
    int32_t v4 = 0;
    while (aiStart < aiEnd) {
        BGSSaveLoadGlobalData::FinishLoadGlobalData(aiStart++, &v3);
    }
    v4 = (-1);
}

void BGSSaveLoadGlobalData::RevertGlobalDataRange(uint32_t aiStart, uint32_t aiEnd, bool abFullRevert) {
    BGSLoadGameBuffer v4;
    v4.~BGSLoadGameBuffer();
    ::new (&v4) BGSLoadGameBuffer;
    int32_t v5 = 0;
    while (aiStart < aiEnd) {
        BGSSaveLoadGlobalData::RevertGlobalData(aiStart++, &v4, abFullRevert);
    }
    v5 = (-1);
}

bool BGSSaveLoadGlobalData::SaveGlobalData(uint32_t aiGlobalDataType, BGSSaveGameBuffer* apSaveGameBuffer) {
    bool result;
    if (aiGlobalDataType > 1000) {
    LABEL_18:;
        SaveGameWarning("SAVELOAD: Unknown global data type %i in SaveGlobalData()", aiGlobalDataType);
        return false;
    }

    if (aiGlobalDataType == 1000) {
        return true;
    }

    switch (aiGlobalDataType) {
        case 0u: {
            MiscStatManager::SaveGame(apSaveGameBuffer);
            result = true;
            break;
        }
        case 1u: {
            BGSSaveLoadGlobalData::SaveLocationData(apSaveGameBuffer);
            result = true;
            break;
        }
        case 2u: {
            TES::pInstance->SaveGameBGS(apSaveGameBuffer);
            result = true;
            break;
        }
        case 3u: {
            BGSSaveLoadGlobalData::SaveGlobals(apSaveGameBuffer);
            result = true;
            break;
        }
        case 4u: {
            ReferenceProcessLists.SaveGame(apSaveGameBuffer);
            result = true;
            break;
        }
        case 5u: {
            pCombatManager->SaveGame(apSaveGameBuffer);
            result = true;
            break;
        }
        case 6u: {
            Interface::SaveGame(apSaveGameBuffer);
            result = true;
            break;
        }
        case 7u: {
            BGSSaveLoadGlobalData::SaveEffects(apSaveGameBuffer);
            result = true;
            break;
        }
        case 8u: {
            Sky* Instance = Sky::GetInstance();
            Instance->SaveGame(apSaveGameBuffer);
            result = true;
            break;
        }
        case 9u: {
            ActorCause::SaveActorCausesList(apSaveGameBuffer);
            result = true;
            break;
        }
        case 10u: {
            FalloutRadio::SaveGame(apSaveGameBuffer);
            result = true;
            break;
        }
        case 11u: {
            FalloutAudio::SaveGame(apSaveGameBuffer);
            result = true;
            break;
        }
        default: {
            goto LABEL_18;
        }
    }

    return result;
}

void BGSSaveLoadGlobalData::SaveLocationData(BGSSaveGameBuffer* apSaveGameBuffer) {
    int32_t iCurrentGridY;
    uint32_t inextID;
    int32_t iCurrentGridX;
    NiPoint3 playerLocation;
    inextID = pDataHandler->GetNextFormID();
    apSaveGameBuffer->SaveDataEndian(&inextID, 4u, 0);
    apSaveGameBuffer->SaveFormID(TES::pInstance->pWorldSpace, 0);
    iCurrentGridX = TES::pInstance->iCurrentGridX;
    iCurrentGridY = TES::pInstance->iCurrentGridY;
    apSaveGameBuffer->SaveDataEndian(&iCurrentGridX, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&iCurrentGridY, 4u, 0);
    TESForm* plocation_2 = (TESForm*)pPlayer->GetWorldSpace();
    if (!plocation_2) {
        plocation_2 = (TESForm*)pPlayer->GetParentCell();
    }
    apSaveGameBuffer->SaveFormID(plocation_2, 0);
    playerLocation = *pPlayer->QLocationOnReference();
    apSaveGameBuffer->SaveDataEndian(&playerLocation, 12u, 0);
    LoadingMenu::SaveGame(apSaveGameBuffer);
}

void BGSSaveLoadGlobalData::LoadLocationData(BGSLoadGameBuffer* apLoadGameBuffer) {
    NiPoint3 centerGridLocation;
    int32_t icurrentGridY;
    uint32_t inextID;
    int32_t icurrentGridX;
    NiPoint3 playerLocation;
    apLoadGameBuffer->LoadData(&inextID, 4u);
    uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
    TESWorldSpace* ptesWorldSpace = dynamic_cast<TESWorldSpace*>(FormByNumericID);
    apLoadGameBuffer->LoadData(&icurrentGridX, 4u);
    apLoadGameBuffer->LoadData(&icurrentGridY, 4u);
    uint32_t v3 = apLoadGameBuffer->LoadFormID();
    TESForm* pworld = TESForm::GetFormByNumericID(v3);
    apLoadGameBuffer->LoadData(&playerLocation, 12u);
    LoadingMenu::LoadGame(apLoadGameBuffer);
    bool bvalidPlayerLocation = true;
    if (pworld) {
        if (pworld->GetFormType() == CELL_ID) {
            if (pPlayer->GetParentCell() != pworld) {
                NiPoint3 v32 = pPlayer->GetAngleOnReference();
                pPlayer->PositionPlayer(playerLocation, v32, static_cast<TESObjectCELL*>(pworld), false);
            }
            if (ptesWorldSpace) {
                TES::pInstance->SetWorldSpace(ptesWorldSpace);
            }
            TES::pInstance->iCurrentGridX = icurrentGridX;
            TES::pInstance->iCurrentGridY = icurrentGridY;
        }
        else {
            if (pworld->GetFormType() == WRLD_ID) {
                if (ptesWorldSpace) {
                    TES::pInstance->SetWorldSpace(ptesWorldSpace);
                }
                TESWorldSpace* pworld_2 = static_cast<TESWorldSpace*>(pworld);
                if (static_cast<TESWorldSpace*>(pworld)->TESWorldSpace::GetHasBorderRegion() &&
                    gs_bBorderRegionsEnabled_General.Bool() && !pworld_2->IsPointInBorderRegion(playerLocation)) {
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
                    TESObjectCELL* pcell = pDataHandler->GetCellFromCellCoord(icellX, icellY, pworld_2, false);
                    if (!pcell || !pcell->Get3D()) {
                        if (pPlayer->GetParentCell()) {
                            pPlayer->GetParentCell()->RemoveReference(pPlayer);
                        }
                        TES::pInstance->LeaveInterior(&centerGridLocation);
                        pcell = pDataHandler->GetCellFromCellCoord(icellX, icellY, pworld_2, true);
                    }
                    if (pcell) {
                        pSaveLoadGame->TogglePositioningPlayer(1);
                        NiPoint3 v33 = pPlayer->GetAngleOnReference();
                        pPlayer->PositionPlayer(playerLocation, v33, pcell, false);
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
    }
    else {
        bvalidPlayerLocation = false;
    }
    if (!bvalidPlayerLocation) {
        pSaveLoadGame->SetPlayerLocationInvalid(true);
        for (BSSimpleList<TESQuest*>* ecx0 = pDataHandler->GetQuestList(); ecx0 && ecx0->m_item;
             ecx0 = ecx0->m_pkNext) {
            int32_t CastingType = ecx0->m_item->iFormID;
            BSprintf("Quest %s (%08x) compared to Patch04Debug\n", ecx0->m_item->GetFullName(), CastingType);
            if (ecx0->m_item->GetFullNameLength() > 8 && std::strstr(ecx0->m_item->GetFullName(), "Patch04Debug")) {
                BSSimpleList<TESQuest*>* v5 = ecx0;
                pPlayer->patch04DebugQuest = v5->m_item;
                break;
            }
        }
        pPlayer->SetControlsDisabled(false, PlayerCharacter::DPC_ALL);
        const char* v6 = sSaveGameSafeMarkerID.String();
        uint32_t isafeMarkerID = std::strtol(v6, nullptr, 16);
        TESForm* v7 = TESForm::GetFormByNumericID(isafeMarkerID);
        TESObjectREFR* pref = dynamic_cast<TESObjectREFR*>(v7);
        if (pref) {
            NiPoint3 loc = pref->GetLocationOnReference();
            NiPoint3 angle = pref->GetAngleOnReference();
            TESObjectCELL* pcell_2 = pref->GetParentCell();
            TESWorldSpace* pworld_3 = pref->GetWorldSpace();
            bvalidPlayerLocation = true;
            if (pworld_3) {
                pPlayer->PositionPlayerExterior(loc, angle, pworld_3, true);
            }
            else {
                if (pcell_2) {
                    pPlayer->PositionPlayer(loc, angle, pcell_2, true);
                }
                else {
                    bvalidPlayerLocation = false;
                }
            }
        }
        if (!bvalidPlayerLocation) {
            TESForm* v8 = TESForm::GetFormByNumericID(0xDA726u);
            TESWorldSpace* pworld_4 = dynamic_cast<TESWorldSpace*>(v8);
            if (pworld_4) {
                TESObjectCELL* pcell_3 = pworld_4->GetCellFromCellCoord(-18, 0);
                if (!pcell_3) {
                    pcell_3 = pworld_4->LoadCell(-18, 0);
                }
                if (!pcell_3) {
                    pcell_3 = pworld_4->GetCellFromCellCoord(0, 0);
                }
                if (!pcell_3) {
                    pcell_3 = pworld_4->LoadCell(0, 0);
                }
                if (!pcell_3) {
                    pcell_3 = pDataHandler->NewCell(nullptr, 0, 0, pworld_4);
                }
                if (pcell_3) {
                    pPlayer->CenterOnCell(nullptr, pcell_3);
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
        if ((pglobal) && (!pglobal->TESForm::GetHasSpokenFlag())) {
            apSaveGameBuffer->SaveFormID(pglobal, 0);
            fvalue = pglobal->fValue;
            apSaveGameBuffer->SaveDataEndian(&fvalue, 4u, 0);
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
        uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
        TESGlobal* pglobal = dynamic_cast<TESGlobal*>(FormByNumericID);
        fvalue = 0.0f;
        apLoadGameBuffer->LoadData(&fvalue, 4u);
        if (pglobal) {
            if (!pglobal->TESForm::GetHasSpokenFlag()) {
                pglobal->SetValue(fvalue);
            }
            GlobalsMap.RemoveAt(pglobal);
        }
    }
    NiTMapItem<TESGlobal*, bool>* pos = GlobalsMap.GetFirstPos();
    while (pos) {
        TESGlobal* pglobal_2 = nullptr;
        bool bvalue = false;
        GlobalsMap.GetNext(pos, pglobal_2, bvalue);
        if (!pglobal_2->TESForm::GetHasSpokenFlag()) {
            preconstructForms->AddForm(pglobal_2, 0);
        }
    }
}

void BGSSaveLoadGlobalData::SaveEffects(BGSSaveGameBuffer* apSaveGameBuffer) {
    float foffset;
    float fstrength;
    uint32_t icount = 0;
    uint32_t icountPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<NiPointer<ImageSpaceModifierInstance>>* plist = &TES::pInstance->listActiveImageSpaceModifiers;
         plist; plist = plist->m_pkNext) {
        ImageSpaceModifierInstance* pmod = plist->m_item.m_pObject;
        if ((pmod) && (!pmod->QPermanent())) {
            ImageSpaceModifierInstanceForm* pinstanceForm = pmod->IsForm();
            if (pinstanceForm) {
                fstrength = pinstanceForm->fStrength;
                apSaveGameBuffer->SaveDataEndian(&fstrength, 4u, 0);
                double TickCount = TESImageSpaceModifier::GetTickCount();
                foffset = TickCount - pinstanceForm->fStartTime;
                apSaveGameBuffer->SaveDataEndian(&foffset, 4u, 0);
                apSaveGameBuffer->SaveFormID(pinstanceForm->pImageSpaceMod, 0);
                ++icount;
            }
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(icount, icountPosition);
}

void BGSSaveLoadGlobalData::LoadEffects(BGSLoadGameBuffer* apLoadGameBuffer) {
    float v5;
    float fstrength;
    uint32_t iextraCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < iextraCount; ++i) {
        fstrength = 0.0f;
        apLoadGameBuffer->LoadData(&fstrength, 4u);
        v5 = 0.0f;
        if (apLoadGameBuffer->GetVersion() >= 14u) {
            apLoadGameBuffer->LoadData(&v5, 4u);
        }
        TESImageSpaceModifier* apMod = nullptr;
        uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
        TESImageSpaceModifier* pmod = dynamic_cast<TESImageSpaceModifier*>(FormByNumericID);
        if (pmod) {
            ImageSpaceModifierInstanceForm* pinstance =
                ImageSpaceModifierInstanceForm::Trigger(pmod, fstrength, nullptr);
            if (pinstance) {
                float a2 = fCellAppTime - v5;
                pinstance->SetAge(a2);
            }
        }
    }
}

void RevertGlobalData::InterfaceManagerAndMenus(BGSLoadGameBuffer* apLoadGameBuffer) {
    bIsReverting = true;
    Interface::ClearAllSubtitlesAndMessages();
    HUDMainMenu::Revert();
    if (Interface::InDialog()) {
        MobileObject* DialogMenuParterRef = static_cast<MobileObject*>(GetDialogMenuParterRef());
        pPlayer->FocusOnActor(DialogMenuParterRef, 0.0f, true);
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
    InterfaceManager* v2 = InterfaceManager::Instance();
    v2->EnterRenderedMenu(nullptr);
    InterfaceManager::Instance()->ePipboyState = InterfaceManager::PS_CLOSED;
    InterfaceManager* v3 = InterfaceManager::Instance();
    v3->xTutorialManager.Reset();
}

void RevertGlobalData::ImageSpaceModifiers() {
    ScreenSplatter::Clear();
    CustomBloodSplatter();
    BSParticleSystemManager* MasterParticleSystemManager = Main::GetMasterParticleSystemManager();
    MasterParticleSystemManager->ClearAllParticles();
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
    return (iFlags & 1) != 0;
}

float ImageSpaceModifierInstance::QStrength() {
    return fStrength;
}
