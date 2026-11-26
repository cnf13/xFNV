#include "Fallout Shared/TESRace.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSHeadPart.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BGSTextureModel.h"
#include "Fallout Shared/BGSTextureSet.h"
#include "Fallout Shared/BGSVoiceType.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenNiNode.h"
#include "Fallout Shared/FaceGenNpcData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Settings/XGameSetting.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAttributes.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESEyes.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHair.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESReactionForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESSpellList.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCollisionSDM.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/FaceGen/MatrixVT.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

#include <cstdint>

static NiCollisionSDM NiCollisionSDMObject;
SettingT<GameSettingCollection> TESRace::fRaceGeneticVariation("fRaceGeneticVariation", 0.5f);
SettingT<INISettingCollection> TESRace::bFaceGenTexturing("bFaceGenTexturing:General", true);

// Reads the race record's chunks. The head and body part chunks (ICON, MODL...) apply to the part the last INDX named,
// in the head list after NAM0 or the body list after NAM1, for the sex the last MNAM/FNAM chose. The FaceGen chunks
// fill the mean face coordinates; without NAM2 the female ones skip the male matrices and the head part indices past 1
// are taken one lower. Save writes a 2-byte ATTR chunk while Load reads 14 bytes, so on every real RACE record 12 of
// the 14 bytes are not supplied by the chunk; the DNAM hair IDs, the EDID buffer's terminator and the ATTR buffer are
// initialised to 0 because GetChunkData writes nothing for an empty chunk and only the chunk's bytes for a short one.
bool TESRace::Load(TESFile* apFile) {
    int32_t iSex = 0;
    int32_t iIndex = 0;
    int32_t iFaceGenType = -1;
    int32_t iFaceGenPart = -1;
    bool bHeadParts = false;
    bool bHasNAM2 = true;
    if (apFile->GetTESForm() != RACE_ID) {
        return false;
    }
    LoadForm(apFile);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        iFaceGenPart = -1;
        iFaceGenType = iFaceGenPart;
        switch (uiChunk) {
            case EDID_ID: {
                uint32_t uiSize = apFile->GetChunkSize();
                // Allocates the chunk's size plus a terminator byte.
                char* pEditorID = static_cast<char*>(XFNV_ALLOCA16(uiSize + 1));
                pEditorID[uiSize] = '\0';
                apFile->GetChunkData(pEditorID, 0x200);
                SetFormEditorID(pEditorID);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
            case DESC_ID: TESDescription::LoadDescriptionData(static_cast<TESDescription*>(this), apFile); break;
            case SPLO_ID: {
                uint32_t uiSpellID = 0;
                apFile->GetChunkData<uint32_t>(uiSpellID);
                // The spell is kept as its form ID until InitItem: the raw ID is stored in the pointer.
                static_cast<TESSpellList*>(this)->AddSpell(
                    reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiSpellID))
                );
                break;
            }
            case XNAM_ID: {
                GROUP_REACTION* pReaction = static_cast<GROUP_REACTION*>(ourMalloc(sizeof(GROUP_REACTION)));
                apFile->GetChunkData(pReaction, 0xC);
                // The file's record is 0xC bytes, the form's 4-byte ID first.
                TESFile::WidenFormIDRecords(
                    pReaction,
                    1,
                    0xC,
                    sizeof(GROUP_REACTION),
                    {{0, offsetof(GROUP_REACTION, pForm)}}
                );
                if (apFile->QEndian()) {
                    pReaction->Endian();
                }
                static_cast<TESReactionForm*>(this)->QReactionList()->AddTail(pReaction);
                break;
            }
            case DATA_ID:
                // LoadData takes the chunk's raw bytes, which here are the RACE_DATA struct.
                LoadData(apFile, reinterpret_cast<char*>(&Data), sizeof(RACE_DATA));
                break;
            case ONAM_ID: {
                uint32_t uiRaceID = 0;
                apFile->GetChunkData<uint32_t>(uiRaceID);
                // Kept as the form ID until InitItem: the raw ID is stored in the pointer.
                pOldRace = reinterpret_cast<TESRace*>(static_cast<uintptr_t>(uiRaceID));
                break;
            }
            case YNAM_ID: {
                uint32_t uiRaceID = 0;
                apFile->GetChunkData<uint32_t>(uiRaceID);
                // Kept as the form ID until InitItem: the raw ID is stored in the pointer.
                pYoungRace = reinterpret_cast<TESRace*>(static_cast<uintptr_t>(uiRaceID));
                break;
            }
            case VTCK_ID:
                if (apFile->GetChunkSize() == 4) {
                    SetDefaultVoiceType(TESActorBase::SEX_MALE, pMaleVoice);
                    SetDefaultVoiceType(TESActorBase::SEX_FEMALE, pFemaleVoice);
                }
                else if (apFile->GetChunkSize() == 8) {
                    uint32_t auiVoiceIDs[2];
                    apFile->GetChunkData(auiVoiceIDs, 8);
                    if (apFile->QEndian()) {
                        for (uint32_t& uiVoiceID : auiVoiceIDs) {
                            BSEndian(uiVoiceID, false);
                        }
                    }
                    // Kept as form IDs until InitItem: the raw IDs are stored in the pointers.
                    SetDefaultVoiceType(
                        TESActorBase::SEX_MALE,
                        reinterpret_cast<BGSVoiceType*>(static_cast<uintptr_t>(auiVoiceIDs[0]))
                    );
                    SetDefaultVoiceType(
                        TESActorBase::SEX_FEMALE,
                        reinterpret_cast<BGSVoiceType*>(static_cast<uintptr_t>(auiVoiceIDs[1]))
                    );
                }
                break;
            case CNAM_ID:
                if (apFile->GetChunkSize() == 1) {
                    apFile->GetChunkData(&cDefaultHairColor[0], 2);
                    apFile->GetChunkData(&cDefaultHairColor[1], 2);
                }
                else if (apFile->GetChunkSize() == 2) {
                    uint8_t aucColors[2];
                    apFile->GetChunkData(aucColors, 2);
                    for (int32_t i = 0; i < 2; ++i) {
                        SetDefaultHairColor(static_cast<TESActorBase::SEX>(i), static_cast<char>(aucColors[i]));
                    }
                }
                break;
            case PNAM_ID: apFile->GetChunkData<float>(fClampFaceGeoValue); break;
            case UNAM_ID: apFile->GetChunkData<float>(fClampFaceGeoValue2); break;
            case DNAM_ID: {
                uint32_t auiHairIDs[2] = {0, 0};
                apFile->GetChunkData(auiHairIDs, 8);
                if (apFile->QEndian()) {
                    for (uint32_t& uiHairID : auiHairIDs) {
                        BSEndian(uiHairID, false);
                    }
                }
                // Kept as form IDs until InitItem: the raw IDs are stored in the pointers.
                SetDefaultHair(
                    TESActorBase::SEX_MALE,
                    reinterpret_cast<TESHair*>(static_cast<uintptr_t>(auiHairIDs[0]))
                );
                SetDefaultHair(
                    TESActorBase::SEX_FEMALE,
                    reinterpret_cast<TESHair*>(static_cast<uintptr_t>(auiHairIDs[1]))
                );
                break;
            }
            case ATTR_ID: {
                int32_t iSize = MaleAttributes.GetAttributeDataSize();
                char* pData = static_cast<char*>(XFNV_ALLOCA16(2 * iSize));
                for (int32_t i = 0; i < 2 * iSize; ++i) {
                    pData[i] = 0;
                }
                apFile->GetChunkData(pData, 2 * iSize);
                MaleAttributes.Load(pData);
                pData += iSize;
                FemaleAttributes.Load(pData);
                break;
            }
            case FGGS_ID:
                if (iFaceGenType == -1) {
                    iFaceGenType = 0;
                    iFaceGenPart = 0;
                }
                [[fallthrough]];
            case FGGA_ID:
                if (iFaceGenType == -1) {
                    iFaceGenType = 0;
                    iFaceGenPart = 1;
                }
                [[fallthrough]];
            case FGTS_ID: {
                if (iFaceGenType == -1) {
                    iFaceGenType = 1;
                    iFaceGenPart = 0;
                }
                uint32_t uiSize = apFile->GetChunkSize();
                uint32_t uiCount = uiSize >> 2;
                if (!iSex || bHasNAM2) {
                    MeanFaceCoordMale[iFaceGenType][iFaceGenPart].resize(static_cast<int32_t>(uiCount), 1);
                }
                MeanFaceCoordFemale[iFaceGenType][iFaceGenPart].resize(static_cast<int32_t>(uiCount), 1);
                float* pValues = static_cast<float*>(XFNV_ALLOCA16(uiSize));
                ourMemset(pValues, 0, uiSize);
                apFile->GetChunkData(pValues, uiSize);
                for (uint32_t i = 0; i < uiCount; ++i) {
                    if (apFile->QEndian()) {
                        BSEndian(pValues[i], false);
                    }
                    if (!iSex || bHasNAM2) {
                        MeanFaceCoordMale[iFaceGenType][iFaceGenPart][i][0] = pValues[i];
                    }
                    MeanFaceCoordFemale[iFaceGenType][iFaceGenPart][i][0] = pValues[i];
                }
                break;
            }
            case FNAM_ID: iSex = 1; break;
            case MNAM_ID: iSex = 0; break;
            case NAM0_ID: bHeadParts = true; break;
            case NAM1_ID: bHeadParts = false; break;
            case NAM2_ID: bHasNAM2 = false; break;
            case SNAM_ID:
                if (apFile->GetChunkSize() >= 2) {
                    // LoadWord takes the count as a signed short; the field is unsigned.
                    apFile->LoadWord(reinterpret_cast<int16_t*>(&sFaceCoordNum));
                }
                break;
            case HNAM_ID: {
                uint32_t* puiHairIDs = nullptr;
                uint32_t uiSize = apFile->GetChunkSize();
                if (uiSize % 4) {
                    break;
                }
                uint32_t uiCount = uiSize >> 2;
                if (uiCount) {
                    // A count whose byte size would overflow asks ourMalloc for SIZE_MAX bytes, as the game's new[]
                    // does.
                    puiHairIDs = static_cast<uint32_t*>(
                        ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
                    );
                    apFile->GetChunkData(puiHairIDs, uiSize);
                    for (uint32_t i = 0; i < uiCount; ++i) {
                        if (apFile->QEndian()) {
                            BSEndian(puiHairIDs[i], false);
                        }
                        TESForm::AddCompileIndex(puiHairIDs[i], apFile);
                        TESHair* pHair = dynamic_cast<TESHair*>(TESForm::GetFormByNumericID(puiHairIDs[i]));
                        if (pHair) {
                            AddHair(pHair);
                        }
                        else {
                            uint32_t uiFormID = GetFormID();
                            const char* pEditorID = GetFormEditorID();
                            Warning(
                                "MASTERFILE: Could not find hair (%08X) for race '%s' (%08X).",
                                puiHairIDs[i],
                                pEditorID,
                                uiFormID
                            );
                        }
                    }
                    ourFree(puiHairIDs);
                }
                break;
            }
            case ENAM_ID: {
                uint32_t* puiEyeIDs = nullptr;
                uint32_t uiSize = apFile->GetChunkSize();
                if (uiSize % 4) {
                    break;
                }
                uint32_t uiCount = uiSize >> 2;
                if (uiCount) {
                    // A count whose byte size would overflow asks ourMalloc for SIZE_MAX bytes, as the game's new[]
                    // does.
                    puiEyeIDs = static_cast<uint32_t*>(
                        ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
                    );
                    apFile->GetChunkData(puiEyeIDs, uiSize);
                    for (uint32_t i = 0; i < uiCount; ++i) {
                        if (apFile->QEndian()) {
                            BSEndian(puiEyeIDs[i], false);
                        }
                        TESForm::AddCompileIndex(puiEyeIDs[i], apFile);
                        TESEyes* pEyes = dynamic_cast<TESEyes*>(TESForm::GetFormByNumericID(puiEyeIDs[i]));
                        if (pEyes) {
                            AddEyeColor(pEyes);
                        }
                        else {
                            uint32_t uiFormID = GetFormID();
                            const char* pEditorID = GetFormEditorID();
                            Warning(
                                "MASTERFILE: Could not find eyes (%08X) for race '%s' (%08X).",
                                puiEyeIDs[i],
                                pEditorID,
                                uiFormID
                            );
                        }
                    }
                    ourFree(puiEyeIDs);
                }
                break;
            }
            case INDX_ID:
                apFile->GetChunkData<int32_t>(iIndex);
                if (bHeadParts) {
                    int32_t iSkip = (bHasNAM2 && iIndex > 1) ? 1 : 0;
                    if (iIndex < 8 && iIndex - iSkip != 1) {
                        TESModel* pModel = &ppHeadModelFiles[iSex][iIndex - iSkip];
                        pModel->InitializeDataComponent();
                        pModel->SetModel(pppHeadModelFileDefaults[iSex][iIndex - iSkip]);
                        TESTexture* pTexture = &ppHeadTextureFiles[iSex][iIndex - iSkip];
                        pTexture->InitializeDataComponent();
                        pTexture->SetTextureName(pppHeadTextureFileDefaults[iSex][iIndex - iSkip]);
                    }
                }
                else {
                    if (iIndex < 3) {
                        TESModel* pModel = &ppBodyModelFiles[iSex][iIndex];
                        pModel->InitializeDataComponent();
                        pModel->SetModel(pppBodyModelFileDefaults[iSex][iIndex]);
                        TESTexture* pTexture = &ppBodyTextureFiles[iSex][iIndex];
                        pTexture->InitializeDataComponent();
                    }
                    if (iIndex == 3) {
                        TESModel* pTextureModel = &pBodyTextureModels[iSex];
                        pTextureModel->InitializeDataComponent();
                        pTextureModel->SetModel(ppBodyTextureModelDefaults[iSex]);
                    }
                }
                break;
            default:
                // A part's texture or model chunk.
                if (bHeadParts) {
                    int32_t iSkip = (bHasNAM2 && iIndex > 1) ? 1 : 0;
                    if (iIndex - iSkip < 8 && iIndex - iSkip != 1) {
                        if (TESTexture::IsTextureChunk(static_cast<CHUNK_ID>(uiChunk))) {
                            TESTexture::LoadTextureChunk(&ppHeadTextureFiles[iSex][iIndex - iSkip], apFile);
                        }
                        else if (TESModel::IsModelChunk(static_cast<CHUNK_ID>(uiChunk))) {
                            TESModel::LoadModelChunk(&ppHeadModelFiles[iSex][iIndex - iSkip], apFile);
                        }
                    }
                }
                else {
                    if (iIndex < 3) {
                        if (TESTexture::IsTextureChunk(static_cast<CHUNK_ID>(uiChunk))) {
                            TESTexture::LoadTextureChunk(&ppBodyTextureFiles[iSex][iIndex], apFile);
                        }
                        else if (TESModel::IsModelChunk(static_cast<CHUNK_ID>(uiChunk))) {
                            TESModel::LoadModelChunk(&ppBodyModelFiles[iSex][iIndex], apFile);
                        }
                    }
                    if (iIndex == 3) {
                        TESModel::LoadModelChunk(&pBodyTextureModels[iSex], apFile);
                    }
                }
                break;
        }
        if (uiChunk == DATA_ID && apFile->QEndian()) {
            Data.Endian();
        }
    }
    while (apFile->NextChunk());
    return true;
}

void RACE_DATA::Endian() {
    for (int32_t i = 0; i < 2; ++i) {
        BSEndian(fHeight[i], false);
        BSEndian(fWeight[i], false);
    }
    BSEndian(iFlags, false);
}

TESRace::TESRace() : FaceGenUndoStates(0, 1) {
    cFormType = RACE_ID;
    TESRace::InitializeData();
}

TESRace::~TESRace() {
    TESRace::ClearData();
}

// The component setters are called virtually.
void TESRace::InitializeData() {
    ourMemset(&Data, 0, sizeof(RACE_DATA));
    for (SKILL_BONUS& kSkillBonus : Data.eSkillBonus) {
        kSkillBonus.eSkill = -1;
    }
    for (uint32_t i = 0; i < 2; ++i) {
        Data.fWeight[i] = 1.0f;
        Data.fHeight[i] = 1.0f;
    }
    pDefaultHair[0] = nullptr;
    pDefaultHair[1] = nullptr;
    pDefaultVoiceType[0] = pMaleVoice;
    pDefaultVoiceType[1] = pFemaleVoice;
    fClampFaceGeoValue = 0.0f;
    fClampFaceGeoValue2 = 0.0f;
    cDefaultHairColor[0] = 0;
    cDefaultHairColor[1] = 0;
    FR2MatrixVTC<float>(*pMaleCoord)[2][2] = GetMeanFaceCoord(TESActorBase::SEX_MALE);
    BSFaceGenManager::CopyFaceGenCoord(BSFaceGenManager::GetFaceGenCoord(), pMaleCoord, false, false);
    BSFaceGenManager::InitFaceGenCoord(GetMeanFaceCoord(TESActorBase::SEX_MALE));
    FR2MatrixVTC<float>(*pFemaleCoord)[2][2] = GetMeanFaceCoord(TESActorBase::SEX_FEMALE);
    BSFaceGenManager::CopyFaceGenCoord(BSFaceGenManager::GetFaceGenCoord(), pFemaleCoord, false, false);
    BSFaceGenManager::InitFaceGenCoord(GetMeanFaceCoord(TESActorBase::SEX_FEMALE));
    sFaceCoordNum = 0;
    for (uint32_t isex = 0; isex < 2; ++isex) {
        for (uint32_t ipart = 0; ipart < RACE_HEAD_PART_COUNT; ++ipart) {
            TESModel* pModel = &ppHeadModelFiles[isex][ipart];
            pModel->InitializeDataComponent();
            pModel->SetModel(pppHeadModelFileDefaults[isex][ipart]);
            TESTexture* pTexture = &ppHeadTextureFiles[isex][ipart];
            pTexture->InitializeDataComponent();
            pTexture->SetTextureName(pppHeadTextureFileDefaults[isex][ipart]);
        }
    }
    for (uint32_t isex = 0; isex < 2; ++isex) {
        for (uint32_t ipart = 0; ipart < RACE_BODY_PART_COUNT; ++ipart) {
            TESTexture* pTexture = &ppBodyTextureFiles[isex][ipart];
            pTexture->InitializeDataComponent();
            pTexture->SetTextureName(pppBodyTextureFileDefaults[isex][ipart]);
            TESModel* pModel = &ppBodyModelFiles[isex][ipart];
            pModel->InitializeDataComponent();
            pModel->SetModel(pppBodyModelFileDefaults[isex][ipart]);
        }
        TESModel* pTextureModel = &pBodyTextureModels[isex];
        pTextureModel->InitializeDataComponent();
        pTextureModel->SetModel(ppBodyTextureModelDefaults[isex]);
    }
    pOldRace = nullptr;
    pYoungRace = nullptr;
}

void TESRace::ClearData() {
    uint32_t i;
    uint32_t k;
    uint32_t j;
    uint32_t m;
    ClearHairList();
    ClearEyeColorList();
    for (i = 0; i < 2; ++i) {
        for (j = 0; j < 8; ++j) {
            ppHeadModelFiles[i][j].ClearDataComponent();
            ppHeadTextureFiles[i][j].ClearDataComponent();
        }
    }
    for (k = 0; k < 2; ++k) {
        for (m = 0; m < 3; ++m) {
            ppBodyTextureFiles[k][m].ClearDataComponent();
            ppBodyModelFiles[k][m].ClearDataComponent();
        }
        pBodyTextureModels[k].ClearDataComponent();
    }
}

// Saves the race's record chunks: the components', the race data, the age races, the default voice types and hairs,
// the face clamps, the attributes, the head and body parts, the hair and eye lists and the FaceGen coordinates.
void TESRace::Save() {
    StartForm();
    TESFullName::Save();
    TESDescription::Save();
    TESSpellList::Save();
    TESReactionForm::Save();
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    TESForm::__SaveData(&Data, sizeof(RACE_DATA));
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    uint32_t uiFormID = 0;
    if (pOldRace) {
        uiFormID = pOldRace->GetFormID();
        TESForm::AddChunk(ONAM_ID, uiFormID);
    }
    if (pYoungRace) {
        uiFormID = pYoungRace->GetFormID();
        TESForm::AddChunk(YNAM_ID, uiFormID);
    }
    TESForm::AddChunk(NAM2_ID);
    if (pDefaultVoiceType[0] || pDefaultVoiceType[1]) {
        uint32_t aVoiceTypes[2];
        for (int32_t i = 0; i < 2; ++i) {
            if (pDefaultVoiceType[i]) {
                aVoiceTypes[i] = pDefaultVoiceType[i]->GetFormID();
            }
            else {
                aVoiceTypes[i] = 0;
            }
        }
        TESForm::AddChunkArray(VTCK_ID, aVoiceTypes, 2);
    }
    if (pDefaultHair[0] || pDefaultHair[1]) {
        uint32_t aHairIDs[2];
        for (int32_t i = 0; i < 2; ++i) {
            if (pDefaultHair[i]) {
                aHairIDs[i] = pDefaultHair[i]->GetFormID();
            }
            else {
                aHairIDs[i] = 0;
            }
        }
        TESForm::AddChunkArray(DNAM_ID, aHairIDs, 2);
    }
    TESForm::AddChunkArray(CNAM_ID, cDefaultHairColor, 2);
    TESForm::AddChunk(PNAM_ID, fClampFaceGeoValue);
    TESForm::AddChunk(UNAM_ID, fClampFaceGeoValue2);
    int32_t iAttributeSize = MaleAttributes.GetAttributeDataSize();
    // A 16-byte aligned stack buffer of the chunk's size.
    char* pAttributes = static_cast<char*>(XFNV_ALLOCA16(2 * iAttributeSize));
    MaleAttributes.Save(pAttributes);
    FemaleAttributes.Save(pAttributes + iAttributeSize);
    TESForm::AddChunkArray(ATTR_ID, pAttributes, 2);
    TESForm::AddChunk(NAM0_ID);
    for (uint32_t k = 0; k < 2; ++k) {
        if (k == 0) {
            TESForm::AddChunk(MNAM_ID);
        }
        else if (k == 1) {
            TESForm::AddChunk(FNAM_ID);
        }
        for (uint32_t m = 0; m < RACE_HEAD_PART_COUNT; ++m) {
            TESForm::AddChunk(INDX_ID, m);
            ppHeadModelFiles[k][m].Save(MODL_ID, MODT_ID);
            ppHeadModelFiles[k][m].SaveFlags(MODD_ID);
            ppHeadTextureFiles[k][m].Save(ICON_ID);
        }
    }
    TESForm::AddChunk(NAM1_ID);
    for (uint32_t k = 0; k < 2; ++k) {
        if (k == 0) {
            TESForm::AddChunk(MNAM_ID);
        }
        else if (k == 1) {
            TESForm::AddChunk(FNAM_ID);
        }
        uint32_t m;
        for (m = 0; m < RACE_BODY_PART_COUNT; ++m) {
            TESForm::AddChunk(INDX_ID, m);
            ppBodyTextureFiles[k][m].Save(ICON_ID);
            ppBodyModelFiles[k][m].Save(MODL_ID, MODT_ID);
            ppBodyModelFiles[k][m].SaveFlags(MODD_ID);
        }
        TESForm::AddChunk(INDX_ID, m);
        pBodyTextureModels[k].Save(MODL_ID, MODT_ID);
    }
    BSSimpleList<TESHair*>* pHairList = GetHairList();
    uint32_t uiHairCount = static_cast<uint32_t>(pHairList->ItemsInList());
    uint32_t* pFormIDs = static_cast<uint32_t*>(ourMalloc((uiHairCount >> 30) != 0 ? 0xFFFFFFFF : 4 * uiHairCount));
    uint32_t uiIndex = 0;
    while (pHairList && pHairList->GetItem()) {
        pFormIDs[uiIndex++] = pHairList->GetItem()->GetFormID();
        pHairList = pHairList->GetNext();
    }
    TESForm::AddChunkArray(HNAM_ID, pFormIDs, uiHairCount);
    ourFree(pFormIDs);
    BSSimpleList<TESEyes*>* pEyeList = GetEyeColorList();
    uint32_t uiEyeCount = static_cast<uint32_t>(pEyeList->ItemsInList());
    pFormIDs = static_cast<uint32_t*>(ourMalloc((uiEyeCount >> 30) != 0 ? 0xFFFFFFFF : 4 * uiEyeCount));
    uiIndex = 0;
    while (pEyeList && pEyeList->GetItem()) {
        pFormIDs[uiIndex++] = pEyeList->GetItem()->GetFormID();
        pEyeList = pEyeList->GetNext();
    }
    TESForm::AddChunkArray(ENAM_ID, pFormIDs, uiEyeCount);
    ourFree(pFormIDs);
    // FGGS, FGGA and FGTS hold the first, second and third of the sex's mean face coordinate matrices, one float per
    // row.
    for (uint32_t k = 0; k < 2; ++k) {
        if (k == 0) {
            TESForm::AddChunk(MNAM_ID);
        }
        else if (k == 1) {
            TESForm::AddChunk(FNAM_ID);
        }
        for (uint32_t c = 0; c < 3; ++c) {
            FR2MatrixVTC<float>* pMatrix = (k == 0) ? &MeanFaceCoordMale[c / 2][c % 2]
                                                    : &MeanFaceCoordFemale[c / 2][c % 2];
            CHUNK_ID eChunk;
            if (c == 0) {
                eChunk = FGGS_ID;
            }
            else if (c == 1) {
                eChunk = FGGA_ID;
            }
            else {
                eChunk = FGTS_ID;
            }
            uint32_t uiParent = pMatrix->numRows();
            if (uiParent) {
                uint32_t uiSize = 4 * uiParent;
                float* pCoords = static_cast<float*>(ourMalloc(uiSize));
                ourMemset(pCoords, 0, uiSize);
                for (uint32_t i = 0; i < uiParent; ++i) {
                    pCoords[i] = (*pMatrix)[i][0];
                }
                // The float buffer is passed to the uint32_t chunk array as the raw bytes it writes.
                TESForm::AddChunkArray(eChunk, reinterpret_cast<const uint32_t*>(pCoords), uiParent);
                ourFree(pCoords);
                pCoords = nullptr;
            }
        }
        TESForm::AddChunk(SNAM_ID, sFaceCoordNum);
    }
    CloseForm();
}

// Resolves the voice types, default hairs and age races loaded as form IDs, then the spell and reaction lists.
void TESRace::InitItem() {
    if (GetInitialized()) {
        return;
    }
    if (pDefaultVoiceType[0]) {
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDefaultVoiceType[0]));
        TESForm::AddCompileIndex(itempint, GetFile(-1));
        pDefaultVoiceType[0] = dynamic_cast<BGSVoiceType*>(TESForm::GetFormByNumericID(itempint));
        if (!pDefaultVoiceType[0]) {
            uint32_t iFormID = GetFormID();
            const char* pEditorID = GetFormEditorID();
            Warning(
                "MASTERFILE: Could not find male voice type (%08X) for race '%s' (%08X).",
                itempint,
                pEditorID,
                iFormID
            );
        }
    }
    if (pDefaultVoiceType[1]) {
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDefaultVoiceType[1]));
        TESForm::AddCompileIndex(itempint, GetFile(-1));
        pDefaultVoiceType[1] = dynamic_cast<BGSVoiceType*>(TESForm::GetFormByNumericID(itempint));
        if (!pDefaultVoiceType[1]) {
            uint32_t iFormID = GetFormID();
            const char* pEditorID = GetFormEditorID();
            Warning(
                "MASTERFILE: Could not find female voice type (%08X) for race '%s' (%08X).",
                itempint,
                pEditorID,
                iFormID
            );
        }
    }
    if (pDefaultHair[0]) {
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDefaultHair[0]));
        TESForm::AddCompileIndex(itempint, GetFile(-1));
        pDefaultHair[0] = dynamic_cast<TESHair*>(TESForm::GetFormByNumericID(itempint));
        if (!pDefaultHair[0]) {
            uint32_t iFormID = GetFormID();
            const char* pEditorID = GetFormEditorID();
            Warning(
                "MASTERFILE: Could not find male default hair (%08X) for race '%s' (%08X).",
                itempint,
                pEditorID,
                iFormID
            );
        }
    }
    if (pDefaultHair[1]) {
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDefaultHair[1]));
        TESForm::AddCompileIndex(itempint, GetFile(-1));
        pDefaultHair[1] = dynamic_cast<TESHair*>(TESForm::GetFormByNumericID(itempint));
        if (!pDefaultHair[1]) {
            uint32_t iFormID = GetFormID();
            const char* pEditorID = GetFormEditorID();
            Warning(
                "MASTERFILE: Could not find female default hair (%08X) for race '%s' (%08X).",
                itempint,
                pEditorID,
                iFormID
            );
        }
    }
    if (pOldRace) {
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pOldRace));
        TESForm::AddCompileIndex(itempint, GetFile(-1));
        pOldRace = dynamic_cast<TESRace*>(TESForm::GetFormByNumericID(itempint));
        if (!pOldRace) {
            uint32_t iFormID = GetFormID();
            const char* pEditorID = GetFormEditorID();
            Warning("MASTERFILE: Could not find older race (%08X) for race '%s' (%08X).", itempint, pEditorID, iFormID);
        }
    }
    if (pYoungRace) {
        uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pYoungRace));
        TESForm::AddCompileIndex(itempint, GetFile(-1));
        pYoungRace = dynamic_cast<TESRace*>(TESForm::GetFormByNumericID(itempint));
        if (!pYoungRace) {
            uint32_t iFormID = GetFormID();
            const char* pEditorID = GetFormEditorID();
            Warning(
                "MASTERFILE: Could not find younger race (%08X) for race '%s' (%08X).",
                itempint,
                pEditorID,
                iFormID
            );
        }
    }
    TESSpellList::InitItem(this);
    TESReactionForm::InitItem(this);
    SetInitialized(true);
}

void TESRace::Copy(TESForm* apCopy) {
    TESRace* prace = dynamic_cast<TESRace*>(apCopy);
    if (prace) {
        CopyAllComponents(apCopy);
        pOldRace = prace->GetOlderRace();
        pYoungRace = prace->GetYoungerRace();
        pDefaultVoiceType[0] = prace->GetDefaultVoiceType(TESActorBase::SEX_MALE);
        pDefaultVoiceType[1] = prace->GetDefaultVoiceType(TESActorBase::SEX_FEMALE);
        pDefaultHair[0] = prace->GetDefaultHair(TESActorBase::SEX_MALE);
        pDefaultHair[1] = prace->GetDefaultHair(TESActorBase::SEX_FEMALE);
        cDefaultHairColor[0] = prace->GetDefaultHairColor(TESActorBase::SEX_MALE);
        cDefaultHairColor[1] = prace->GetDefaultHairColor(TESActorBase::SEX_FEMALE);
        fClampFaceGeoValue = prace->GetClampFaceGeoValue();
        fClampFaceGeoValue2 = prace->GetClampFaceGeoValue2();
        // Both attribute sets are copied through the virtual CopyComponent via a BaseFormComponent pointer; a call on
        // the member itself would be devirtualised.
        BaseFormComponent* pAttributes = &MaleAttributes;
        pAttributes->CopyComponent(&prace->MaleAttributes);
        pAttributes = &FemaleAttributes;
        pAttributes->CopyComponent(&prace->FemaleAttributes);
        for (uint32_t i = 0; i < 7; ++i) {
            Data.eSkillBonus[i].eSkill = prace->GetSkillBonus(i)->eSkill;
            Data.eSkillBonus[i].cBonus = prace->GetSkillBonus(i)->cBonus;
        }
        for (int32_t isex = 0; isex < 2; ++isex) {
            Data.fHeight[isex] = prace->GetHeight(static_cast<TESActorBase::SEX>(isex));
            Data.fWeight[isex] = prace->GetWeight(static_cast<TESActorBase::SEX>(isex));
        }
        Data.iFlags = prace->Data.iFlags;
        // The sex and part counters are unsigned, passed as the callees' enums.
        for (uint32_t isex = 0; isex < 2; ++isex) {
            TESActorBase::SEX eSex = static_cast<TESActorBase::SEX>(isex);
            for (uint32_t i = 0; i < 8; ++i) {
                RACE_HEAD_PART ePart = static_cast<RACE_HEAD_PART>(i);
                TESModel* pModel = prace->GetHeadPartModel(eSex, ePart);
                SetHeadPartModel(eSex, ePart, pModel->GetModel());
                TESTexture* pTexture = prace->GetHeadPartTexture(eSex, ePart);
                SetHeadTextureName(isex, i, pTexture->GetTextureName());
            }
        }
        for (uint32_t isex = 0; isex < 2; ++isex) {
            TESActorBase::SEX eSex = static_cast<TESActorBase::SEX>(isex);
            for (uint32_t j = 0; j < 3; ++j) {
                TESTexture* pTexture = prace->GetBodyPartTexture(eSex, static_cast<RACE_BODY_PART>(j));
                SetBodyTextureName(isex, j, pTexture->GetTextureName());
            }
            for (uint32_t j = 0; j < 3; ++j) {
                RACE_BODY_PART ePart = static_cast<RACE_BODY_PART>(j);
                TESModel* pModel = prace->GetBodyPartModel(eSex, ePart);
                SetBodyPartModel(eSex, ePart, pModel->GetModel());
            }
            TESModel* pModel = prace->GetBodyTextureModel(eSex);
            SetBodyTextureModel(eSex, pModel->GetModel());
        }
        BSSimpleList<TESHair*>* phairlist = &prace->HairList;
        ClearHairList();
        while (phairlist && phairlist->m_item) {
            HairList.AddTail(phairlist->m_item);
            phairlist = phairlist->m_pkNext;
        }
        BSSimpleList<TESEyes*>* peyeslist = prace->GetEyeColorList();
        ClearEyeColorList();
        while (peyeslist && peyeslist->m_item) {
            GetEyeColorList()->AddTail(peyeslist->m_item);
            peyeslist = peyeslist->m_pkNext;
        }
        FR2MatrixVTC<float>(*pMaleCoord)[2][2] = GetMeanFaceCoord(TESActorBase::SEX_MALE);
        BSFaceGenManager::CopyFaceGenCoord(prace->GetMeanFaceCoord(TESActorBase::SEX_MALE), pMaleCoord, false, false);
        FR2MatrixVTC<float>(*pFemaleCoord)[2][2] = GetMeanFaceCoord(TESActorBase::SEX_FEMALE);
        BSFaceGenManager::CopyFaceGenCoord(
            prace->GetMeanFaceCoord(TESActorBase::SEX_FEMALE),
            pFemaleCoord,
            false,
            false
        );
        sFaceCoordNum = prace->GetFaceCoordNum();
    }
}

bool TESRace::Compare(TESForm* apCompare) {
    TESRace* prace = dynamic_cast<TESRace*>(apCompare);
    if (!prace) {
        return true;
    }
    if (CompareAllComponents(prace)) {
        return true;
    }
    if (std::memcmp(&Data, &prace->Data, sizeof(Data))) {
        return true;
    }
    if (this->GetOlderRace() != prace->GetOlderRace()) {
        return true;
    }
    if (this->GetYoungerRace() != prace->GetYoungerRace()) {
        return true;
    }
    BGSVoiceType* pVoiceType = GetDefaultVoiceType(TESActorBase::SEX_MALE);
    if (pVoiceType != prace->GetDefaultVoiceType(TESActorBase::SEX_MALE)) {
        return true;
    }
    pVoiceType = GetDefaultVoiceType(TESActorBase::SEX_FEMALE);
    if (pVoiceType != prace->GetDefaultVoiceType(TESActorBase::SEX_FEMALE)) {
        return true;
    }
    TESHair* pHair = GetDefaultHair(TESActorBase::SEX_MALE);
    if (pHair != prace->GetDefaultHair(TESActorBase::SEX_MALE)) {
        return true;
    }
    pHair = GetDefaultHair(TESActorBase::SEX_FEMALE);
    if (pHair != prace->GetDefaultHair(TESActorBase::SEX_FEMALE)) {
        return true;
    }
    int32_t iHairColor = GetDefaultHairColor(TESActorBase::SEX_MALE);
    if (iHairColor != prace->GetDefaultHairColor(TESActorBase::SEX_MALE)) {
        return true;
    }
    iHairColor = GetDefaultHairColor(TESActorBase::SEX_FEMALE);
    if (iHairColor != prace->GetDefaultHairColor(TESActorBase::SEX_FEMALE)) {
        return true;
    }
    double dClampValue = GetClampFaceGeoValue();
    if (dClampValue != prace->GetClampFaceGeoValue()) {
        return true;
    }
    dClampValue = GetClampFaceGeoValue2();
    if (dClampValue != prace->GetClampFaceGeoValue2()) {
        return true;
    }
    // Every component is compared through the virtual CompareComponent of the member, called through a
    // BaseFormComponent pointer; a call on the member itself would be devirtualised.
    BaseFormComponent* pComponent = &MaleAttributes;
    if (pComponent->CompareComponent(&prace->MaleAttributes)) {
        return true;
    }
    pComponent = &FemaleAttributes;
    if (pComponent->CompareComponent(&prace->FemaleAttributes)) {
        return true;
    }
    // The sex and part counters are unsigned, passed as the callees' enums.
    for (uint32_t isex = 0; isex < 2; ++isex) {
        TESActorBase::SEX eSex = static_cast<TESActorBase::SEX>(isex);
        for (uint32_t i = 0; i < 8; ++i) {
            RACE_HEAD_PART ePart = static_cast<RACE_HEAD_PART>(i);
            TESModel* pModel = prace->GetHeadPartModel(eSex, ePart);
            pComponent = &ppHeadModelFiles[isex][i];
            if (!pComponent->CompareComponent(pModel)) {
                TESTexture* pTexture = prace->GetHeadPartTexture(eSex, ePart);
                pComponent = &ppHeadTextureFiles[isex][i];
                if (!pComponent->CompareComponent(pTexture)) {
                    continue;
                }
            }
            return true;
        }
    }
    for (uint32_t isex = 0; isex < 2; ++isex) {
        for (uint32_t j = 0; j < 3; ++j) {
            pComponent = &ppBodyTextureFiles[isex][j];
            if (pComponent->CompareComponent(&prace->ppBodyTextureFiles[isex][j])) {
                return true;
            }
            pComponent = &ppBodyModelFiles[isex][j];
            if (pComponent->CompareComponent(&prace->ppBodyModelFiles[isex][j])) {
                return true;
            }
        }
        pComponent = &pBodyTextureModels[isex];
        if (pComponent->CompareComponent(&prace->pBodyTextureModels[isex])) {
            return true;
        }
    }
    BSSimpleList<TESHair*>* potherList = &prace->HairList;
    int32_t iCount = HairList.ItemsInList();
    if (iCount != potherList->ItemsInList()) {
        return true;
    }
    while (potherList && potherList->m_item) {
        uint32_t uiFormID = potherList->m_item->iFormID;
        if (!GetRaceHair(uiFormID)) {
            return true;
        }
        potherList = potherList->m_pkNext;
    }
    BSSimpleList<TESEyes*>* potherEyeList = prace->GetEyeColorList();
    iCount = GetEyeColorList()->ItemsInList();
    if (iCount != potherEyeList->ItemsInList()) {
        return true;
    }
    while (potherEyeList && potherEyeList->m_item) {
        uint32_t uiFormID = potherEyeList->m_item->iFormID;
        if (!GetRaceEyeColor(uiFormID)) {
            return true;
        }
        potherEyeList = potherEyeList->m_pkNext;
    }
    FR2MatrixVTC<float>(*pmaleCoord)[2][2] = GetMeanFaceCoord(TESActorBase::SEX_MALE);
    FR2MatrixVTC<float>(*potherMaleCoord)[2][2] = prace->GetMeanFaceCoord(TESActorBase::SEX_MALE);
    if (!BSFaceGenManager::CompareFaceGenCoord(potherMaleCoord, pmaleCoord)) {
        FR2MatrixVTC<float>(*pfemaleCoord)[2][2] = GetMeanFaceCoord(TESActorBase::SEX_FEMALE);
        FR2MatrixVTC<float>(*potherFemaleCoord)[2][2] = prace->GetMeanFaceCoord(TESActorBase::SEX_FEMALE);
        if (!BSFaceGenManager::CompareFaceGenCoord(potherFemaleCoord, pfemaleCoord)) {
            int32_t iFaceCoordNum = GetFaceCoordNum();
            return iFaceCoordNum != prace->GetFaceCoordNum();
        }
    }
    sFaceCoordNum = Random();
    return true;
}

const char* TESRace::GetFormEditorID() {
    if (strEditorID.pString) {
        return strEditorID.pString;
    }
    return "";
}

// The ID is copied to a stack buffer first; returns false when it is unchanged (ignoring case).
bool TESRace::SetFormEditorID(const char* apEditorID) {
    uint32_t uiSize = BSstrlen(apEditorID) + 1;
    char* pEditorID = static_cast<char*>(XFNV_ALLOCA(uiSize));
    BSstrcpy(pEditorID, uiSize, apEditorID);
    if (!strEditorID.StrCmp(pEditorID, true)) {
        return false;
    }
    strEditorID.Set(pEditorID, 0);
    return true;
}

void TESRace::ClearEyeColorList() {
    GetEyeColorList()->RemoveAll();
}

void TESRace::GetBodyModTextureName(TESNPC* apNPC, char* apName) {
    TESNPC* pFaceNPC = apNPC;
    if (apNPC->GetFaceNPC()) {
        for (pFaceNPC = apNPC->GetFaceNPC(); pFaceNPC && pFaceNPC->GetFaceNPC(); pFaceNPC = pFaceNPC->GetFaceNPC()) {}
    }
    char acSex[16];
    if (apNPC->GetSex() == TESActorBase::SEX_FEMALE) {
        std::sprintf(acSex, "Female");
    }
    else {
        std::sprintf(acSex, "Male");
    }
    uint32_t uiRaceID = GetFormID() & 0xFFFFFF;
    uint32_t uiNPCID = pFaceNPC->GetFormID() & 0xFFFFFF;
    // apNPC is converted to its TESActorBaseData base by reference, not by pointer, so there is no null check.
    TESActorBaseData& rBaseData = *apNPC;
    if (rBaseData.GetCanBeAllRaces()) {
        BSsprintf(apName, 0x104u, "%08x_%08XModBody%s", uiRaceID, uiNPCID, acSex);
    }
    else {
        BSsprintf(apName, 0x104u, "%08XModBody%s", uiNPCID, acSex);
    }
}

bool TESRace::GetBodyModTextureFileName(TESNPC* apNPC, const char* apName, char* apFileName) {
    TESNPC* pFaceNPC = apNPC;
    if (apNPC->GetFaceNPC()) {
        for (pFaceNPC = apNPC->GetFaceNPC(); pFaceNPC && pFaceNPC->GetFaceNPC(); pFaceNPC = pFaceNPC->GetFaceNPC()) {}
    }
    TESFile* pOwnerMaster = pFaceNPC->GetOwnerMaster();
    if (!pOwnerMaster) {
        return false;
    }
    const char* pPluginName;
    if (!BSstrnicmp(pOwnerMaster->GetFilename(), "update", 6u)) {
        pPluginName = "FalloutNV.ESM";
    }
    else {
        pPluginName = pOwnerMaster->GetFilename();
    }
    BSsprintf(apFileName, 0x104u, "data\\Textures\\Characters\\BodyMods\\%s\\%s.dds", pPluginName, apName);
    return true;
}

TESHair* TESRace::GetDefaultHair(TESActorBase::SEX aeSex) {
    return pDefaultHair[aeSex];
}

// A null voice type sets the sex's default one.
void TESRace::SetDefaultVoiceType(TESActorBase::SEX aeSex, BGSVoiceType* apVoiceType) {
    if (apVoiceType) {
        pDefaultVoiceType[aeSex] = apVoiceType;
    }
    else {
        if (aeSex == 0) {
            pDefaultVoiceType[aeSex] = pMaleVoice;
        }
        if (aeSex == 1) {
            pDefaultVoiceType[aeSex] = pFemaleVoice;
        }
    }
}

BSSimpleList<TESHair*>* TESRace::GetHairList() {
    return &HairList;
}

// The first hair of the race's list with the form ID; the walk stops at an empty node.
TESHair* TESRace::GetRaceHair(uint32_t aiFormID) {
    for (BSSimpleList<TESHair*>* pIter = GetHairList(); pIter && pIter->GetItem(); pIter = pIter->GetNext()) {
        if (pIter->GetItem()->GetFormID() == aiFormID) {
            return pIter->GetItem();
        }
    }
    return nullptr;
}

// Appends the hair unless the list already has its form ID.
void TESRace::AddHair(TESHair* apHair) {
    if (apHair && !GetRaceHair(apHair->GetFormID())) {
        GetHairList()->AddTail(apHair);
    }
}

void TESRace::SetDefaultHair(TESActorBase::SEX aeSex, TESHair* apHair) {
    pDefaultHair[aeSex] = apHair;
}

void TESRace::ClearHairList() {
    GetHairList()->RemoveAll();
}

// Compares with the double 0.0; a NaN clamp takes the default.
float TESRace::GetClampFaceGeoValue() {
    if (fClampFaceGeoValue > 0.0) {
        return fClampFaceGeoValue;
    }
    return 5.0f;
}

float TESRace::GetClampFaceGeoValue2() {
    if (fClampFaceGeoValue2 > 0.0) {
        return fClampFaceGeoValue2;
    }
    return 3.0f;
}

void TESRace::SetDefaultHairColor(TESActorBase::SEX aeSex, char acDefaultHairColor) {
    cDefaultHairColor[aeSex] = acDefaultHairColor;
}

// The first eye color of the race's list with the form ID; the walk stops at an empty node.
TESEyes* TESRace::GetRaceEyeColor(uint32_t aiFormID) {
    for (BSSimpleList<TESEyes*>* pIter = GetEyeColorList(); pIter && pIter->GetItem(); pIter = pIter->GetNext()) {
        if (pIter->GetItem()->GetFormID() == aiFormID) {
            return pIter->GetItem();
        }
    }
    return nullptr;
}

// Appends the eye color unless the list already has its form ID.
void TESRace::AddEyeColor(TESEyes* apEyes) {
    if (apEyes && !GetRaceEyeColor(apEyes->GetFormID())) {
        GetEyeColorList()->AddTail(apEyes);
    }
}

TESModel* TESRace::GetHeadPartModel(TESActorBase::SEX aeSex, RACE_HEAD_PART aePart) {
    if (aePart < RHP_HEAD || aePart >= RACE_HEAD_PART_COUNT) {
        return nullptr;
    }
    return &ppHeadModelFiles[aeSex][aePart];
}

TESTexture* TESRace::GetHeadPartTexture(TESActorBase::SEX aeSex, RACE_HEAD_PART aePart) {
    if (aePart < RHP_HEAD || aePart >= RACE_HEAD_PART_COUNT) {
        return nullptr;
    }
    return &ppHeadTextureFiles[aeSex][aePart];
}

TESModel* TESRace::GetBodyPartModel(TESActorBase::SEX aeSex, RACE_BODY_PART aePart) {
    if (aePart < RBP_UPPER_BODY || aePart >= RACE_BODY_PART_COUNT) {
        return nullptr;
    }
    return &ppBodyModelFiles[aeSex][aePart];
}

TESModel* TESRace::GetBodyTextureModel(TESActorBase::SEX aeSex) {
    return &pBodyTextureModels[aeSex];
}

// Builds "Textures\<body part texture>" for the NPC's sex (male without an NPC); a missing right hand texture falls
// back to the left hand. The player's hands use the first-person "_1st" variant.
bool TESRace::GetBodyTextureFileName(TESNPC* apNPC, BIPED_OBJECT aePart, char* apFileName) {
    RACE_BODY_PART epart = BodyPartToRaceBodyPart(aePart);
    if (epart < RBP_UPPER_BODY || epart >= RACE_BODY_PART_COUNT) {
        return false;
    }
    int32_t esex = apNPC ? apNPC->GetSex() : 0;
    if (!ppBodyTextureFiles[esex][epart].GetTextureNameLength() && epart == RBP_RIGHT_HAND) {
        epart = RBP_LEFT_HAND;
    }
    if (!ppBodyTextureFiles[esex][epart].GetTextureNameLength()) {
        return false;
    }
    BSstrcpy(apFileName, 0x104, "Textures\\");
    BSstrcat(apFileName, 0x104, ppBodyTextureFiles[esex][epart].GetTextureName());
    if (apNPC && apNPC == pPlayer->GetObjectReference() && (epart == RBP_LEFT_HAND || epart == RBP_RIGHT_HAND)) {
        const char* porigext = std::strrchr(ppBodyTextureFiles[esex][epart].GetTextureName(), '.');
        char* pext = std::strrchr(apFileName, '.');
        if (pext && porigext) {
            *pext = '\0';
            BSstrcat(apFileName, 0x104, "_1st");
            BSstrcat(apFileName, 0x104, porigext);
        }
    }
    return true;
}

// Deliberately keeps an uninitialised read: GetBodyModTextureFileName returns false without writing
// the file name when the NPC has no owner master; with abForceCreate set, the unwritten buffer is then read by
// BSTexturePalette's RemoveTexture, the NiFixedString constructor and SetFilename.
bool TESRace::GetBodyTexture(
    NiPointer<NiTexture>& aspOutBaseTexture,
    NiPointer<NiTexture>& aspOutModTexture,
    TESNPC* apNPC,
    BIPED_OBJECT aePart,
    bool abForceCreate
) {
    TESActorBase::SEX esex;
    char path[264];
    char pcString[264];
    char Path[268];
    NiPointer<BSFaceGenModel> pmodelFG(static_cast<BSFaceGenModel*>(nullptr));
    NiPointer<NiTriBasedGeom> spshape(static_cast<NiTriBasedGeom*>(nullptr));
    NiPointer<NiSourceTexture> sptexBase(static_cast<NiSourceTexture*>(nullptr));
    NiPointer<NiTexture> sptexBaseMod(static_cast<NiTexture*>(nullptr));
    FR2MatrixVTC<float> facecoord[2][2];
    if (apNPC) {
        esex = apNPC->GetSex();
    }
    else {
        esex = TESActorBase::SEX_MALE;
    }
    BSStringT<char> fileEGT;
    aspOutBaseTexture = static_cast<NiTexture*>(nullptr);
    if (!abForceCreate && apNPC != pPlayer->GetObjectReference()) {
        aspOutModTexture = static_cast<NiTexture*>(BSFaceGenManager::GetBaseTexture());
    }
    if (!GetBodyTextureFileName(apNPC, aePart, path)) {
        return false;
    }
    // The texture is a NiPointer<NiTexture> here, but CreateTextureImage takes a NiPointer<NiSourceTexture>.
    TES::pInstance
        ->CreateTextureImage(path, reinterpret_cast<NiPointer<NiSourceTexture>&>(aspOutBaseTexture), false, false);
    if (!aspOutBaseTexture.m_pObject) {
        return false;
    }
    if (!TESRace::bFaceGenTexturing.Bool()) {
        return true;
    }
    GetBodyModTextureName(apNPC, Path);
    if (GetBodyModTextureFileName(apNPC, Path, pcString) && !abForceCreate) {
        TES::pInstance->CreateTextureImage(
            pcString,
            reinterpret_cast<NiPointer<NiSourceTexture>&>(aspOutModTexture),
            true,
            false
        );
    }
    if (aspOutModTexture == static_cast<NiTexture*>(nullptr) || abForceCreate) {
        if (!abForceCreate && apNPC != pPlayer->GetObjectReference()) {
            Warning(
                "Failed to find body mod texture '%s' for '%s' (%08X). Creating from scratch.",
                Path,
                GetFormEditorID(),
                iFormID
            );
        }
        TESModel* pmodel = GetBodyTextureModel(apNPC->GetSex());
        if (!pmodel) {
            return false;
        }
        if (!abForceCreate && apNPC != pPlayer->GetObjectReference()) {
            BSTexturePalette::GetTexture(Path, aspOutModTexture);
            if (aspOutModTexture.m_pObject) {
                return true;
            }
        }
        char acModelPath[264];
        BSstrcpy(acModelPath, 260, "Meshes\\");
        BSstrcat(acModelPath, 260, pmodel->GetModel());
        const char* pEGTFile = BSFaceGenManager::GetAsEGTFile(fileEGT, acModelPath);
        pmodelFG = BSFaceGenManager::GetFaceGenModel(nullptr, nullptr, nullptr, pEGTFile, true, HAIR_PART_NONE, false);
        if (!pmodelFG.m_pObject) {
            return false;
        }
        if (apNPC) {
            FR2MatrixVTC<float>(*pOffsetCoord)[2][2] = apNPC->GetOffsetFaceCoord();
            BSFaceGenManager::MergeFaceGenCoord(
                GetMeanFaceCoord(apNPC->GetSex()),
                pOffsetCoord,
                &facecoord,
                false,
                0.0f
            );
        }
        else {
            FR2MatrixVTC<float>(*pcoord)[2][2] = GetMeanFaceCoord(esex);
            if (!pcoord) {
                pcoord = BSFaceGenManager::GetFaceGenCoord();
            }
            BSFaceGenManager::CopyFaceGenCoord(pcoord, &facecoord, false, false);
        }
        if (abForceCreate) {
            BSTexturePalette::RemoveTexture_0(pcString);
        }
        uint32_t uiEGTClampSize = BSFaceGenManager::GetEGTClampSize();
        pmodelFG->ApplyCoordinateTexturingToMesh(&facecoord, aspOutModTexture, uiEGTClampSize);
        if (!aspOutModTexture.m_pObject) {
            return false;
        }
        static_cast<NiSourceTexture*>(aspOutModTexture.m_pObject)->NiSourceTexture::SetFilename(pcString);
        BSTexturePalette::SetTexture(aspOutModTexture.m_pObject, nullptr);
    }
    return true;
}

// Drops the cached FaceGen texture data of every head part model: the male one, then the female one when it has a model
// that differs from the male one.
void TESRace::KillEGTData() {
    BSStringT<char> fileEGT;
    char ptemp[0x104];
    for (int32_t i = 0; i < RACE_HEAD_PART_COUNT; ++i) {
        TESModel* pmodel = GetHeadPartModel(TESActorBase::SEX_MALE, static_cast<RACE_HEAD_PART>(i));
        if (pmodel && *pmodel->GetModel()) {
            BSstrcpy(ptemp, 0x104, "Meshes\\");
            BSstrcat(ptemp, 0x104, pmodel->GetModel());
            BSFaceGenManager::GetAsEGTFile(fileEGT, ptemp);
            const char* pKey = fileEGT.pString;
            BSFaceGenManager::GetModelCache()->KillModelTextureData(pKey);
        }
        TESModel* pmodel2 = GetHeadPartModel(TESActorBase::SEX_FEMALE, static_cast<RACE_HEAD_PART>(i));
        if (pmodel2 && *pmodel2->GetModel() && pmodel2->CompareComponent(pmodel)) {
            BSstrcpy(ptemp, 0x104, "Meshes\\");
            BSstrcat(ptemp, 0x104, pmodel2->GetModel());
            BSFaceGenManager::GetAsEGTFile(fileEGT, ptemp);
            const char* pKey = fileEGT.pString;
            BSFaceGenManager::GetModelCache()->KillModelTextureData(pKey);
        }
    }
}

// The body part model for the biped slot; one without a model file falls back to the other sex's.
TESModel* TESRace::GetPartModel(TESActorBase::SEX aeSex, BIPED_OBJECT aePart) {
    RACE_BODY_PART epart = BodyPartToRaceBodyPart(aePart);
    if (aeSex < 0 || aeSex >= 2 || epart < RBP_UPPER_BODY || epart >= RACE_BODY_PART_COUNT) {
        return nullptr;
    }
    if (GetBodyPartModel(aeSex, epart) && GetBodyPartModel(aeSex, epart)->GetModelLength()) {
        return GetBodyPartModel(aeSex, epart);
    }
    switch (aeSex) {
        case TESActorBase::SEX_MALE:
            if (GetBodyPartModel(TESActorBase::SEX_FEMALE, epart) &&
                GetBodyPartModel(TESActorBase::SEX_FEMALE, epart)->GetModelLength()) {
                return GetBodyPartModel(TESActorBase::SEX_FEMALE, epart);
            }
            break;
        case TESActorBase::SEX_FEMALE:
            if (GetBodyPartModel(TESActorBase::SEX_MALE, epart) &&
                GetBodyPartModel(TESActorBase::SEX_MALE, epart)->GetModelLength()) {
                return GetBodyPartModel(TESActorBase::SEX_MALE, epart);
            }
            break;
    }
    return nullptr;
}

char TESRace::GetDefaultHairColor(TESActorBase::SEX aeSex) {
    return cDefaultHairColor[aeSex];
}

void TESRace::GetFaceGenData(TESNPC* apNPC, FaceGenNpcData& arNpcData, TESActorBase::SEX aeSex, bool abNoModTextures) {
    bool byoungplayer = false;
    if (apNPC) {
        apNPC->GetFaceCoord(arNpcData.FaceCoord);
        aeSex = apNPC->GetSex();
        if (apNPC == pPlayer->GetObjectReference()) {
            arNpcData.bPlayer = true;
            if (pPlayer->IsPCYoung()) {
                byoungplayer = true;
            }
        }
    }
    else {
        FR2MatrixVTC<float>(*pcoord)[2][2] = GetMeanFaceCoord(aeSex);
        if (!pcoord) {
            pcoord = BSFaceGenManager::GetFaceGenCoord();
        }
        BSFaceGenManager::CopyFaceGenCoord(pcoord, &arNpcData.FaceCoord, false, false);
    }
    if (apNPC) {
        if (!byoungplayer && apNPC->GetHair() && apNPC->GetHair()->CanBeHairForNPC(apNPC)) {
            arNpcData.pHair = apNPC->GetHair();
        }
        else {
            arNpcData.pHair = GetDefaultHair(aeSex);
        }
        arNpcData.fHairLength = apNPC->GetHairLength();
        arNpcData.iHairColor = apNPC->GetHairColor();
        arNpcData.pEyes = apNPC->GetEyeColor();
        arNpcData.eSex = apNPC->GetSex();
    }
    else {
        arNpcData.pHair = GetDefaultHair(aeSex);
        arNpcData.iHairColor = GetDefaultHairColor(aeSex);
    }
    // The parts are counted with an unsigned counter and the count is passed as the part.
    for (uint32_t ipart = 0; ipart < static_cast<uint32_t>(RACE_HEAD_PART_COUNT); ++ipart) {
        RACE_HEAD_PART epart = static_cast<RACE_HEAD_PART>(ipart);
        TESModel* pFaceModel = GetHeadPartModel(aeSex, epart);
        arNpcData.pModel.Add(pFaceModel);
        TESTexture* pFaceTexture = GetHeadPartTexture(aeSex, epart);
        arNpcData.pTexture.Add(pFaceTexture);
        const char* pNodeName = TESRace::GetHeadPartNodeName(epart);
        arNpcData.pHeadPartNodeName.Add(pNodeName);
        if (apNPC && (TESRace::bFaceGenTexturing.Bool() && !abNoModTextures)) {
            NiPointer<NiTexture> spTempTexture(static_cast<NiTexture*>(nullptr));
            apNPC->GetHeadPartModTexture(epart, spTempTexture);
            arNpcData.spTexBaseMod.Add(spTempTexture);
        }
    }
    arNpcData.bFaceGenTexturing = TESRace::bFaceGenTexturing.Bool();
    arNpcData.eLod = BSFaceGenManager::GetForcedLod();
    if (apNPC && !byoungplayer) {
        // TESNPC::listHeadParts holds void* items; they are BGSHeadPart pointers.
        BSSimpleList<void*>* plist = apNPC->GetHeadParts();
        while (plist && !plist->IsEmpty()) {
            BGSHeadPart* ppart = static_cast<BGSHeadPart*>(plist->m_item);
            plist = plist->m_pkNext;
            arNpcData.Accessories.AddHead(ppart);
            BSSimpleList<BGSHeadPart*>* pextralist = &ppart->ExtraParts;
            while (pextralist && !pextralist->IsEmpty()) {
                BGSHeadPart* pextra = pextralist->m_item;
                pextralist = pextralist->m_pkNext;
                arNpcData.Accessories.AddHead(pextra);
            }
        }
    }

    arNpcData.pLeftEye = GetHeadPartModel(aeSex, RHP_EYE_LEFT);
    arNpcData.pRightEye = GetHeadPartModel(aeSex, RHP_EYE_RIGHT);
    if (!arNpcData.pEyes) {
        BSSimpleList<TESEyes*>* peyesList = GetEyeColorList();
        if (!peyesList || peyesList->IsEmpty()) {
            for (BSSimpleList<TESEyes*>* pAllEyes = pDataHandler->GetEyesList(); pAllEyes;
                 pAllEyes = pAllEyes->m_pkNext) {
                TESEyes* peyeItem = pAllEyes->m_item;
                if (peyeItem && !BSstricmp(peyeItem->GetTextureName(), "Characters\\Eyes\\EyeDefault.dds")) {
                    arNpcData.pEyes = peyeItem;
                    return;
                }
            }
        }
        else {
            arNpcData.pEyes = peyesList->m_item;
        }
    }
}

TESEyes* TESRace::GetDefaultEyeColor() {
    return EyeList.m_item;
}

SKILL_BONUS* TESRace::GetSkillBonus(int32_t aiIndex) {
    return &Data.eSkillBonus[aiIndex];
}

TESTexture* TESRace::GetBodyPartTexture(TESActorBase::SEX aeSex, RACE_BODY_PART aePart) {
    if (aeSex >= TESActorBase::SEX_MALE &&
        aeSex <= TESActorBase::SEX_FEMALE &&
        aePart >= RBP_UPPER_BODY &&
        aePart < RACE_BODY_PART_COUNT) {
        return &ppBodyTextureFiles[aeSex][aePart];
    }

    return nullptr;
}

void TESRace::SetHeadPartModel(TESActorBase::SEX aeSex, RACE_HEAD_PART aePart, const char* apFilename) {
    if (aePart >= RHP_HEAD &&
        aePart < RACE_HEAD_PART_COUNT &&
        aeSex >= TESActorBase::SEX_MALE &&
        aeSex <= TESActorBase::SEX_FEMALE) {
        TESModel* pModel = &ppHeadModelFiles[aeSex][aePart];
        pModel->SetModel(apFilename);
    }
}

// Sets the model through the model's virtual SetModel.
void TESRace::SetBodyPartModel(TESActorBase::SEX aeSex, RACE_BODY_PART aePart, const char* apFilename) {
    if (aePart >= RBP_UPPER_BODY &&
        aePart < RACE_BODY_PART_COUNT &&
        aeSex >= TESActorBase::SEX_MALE &&
        aeSex <= TESActorBase::SEX_FEMALE) {
        TESModel* pModel = &ppBodyModelFiles[aeSex][aePart];
        pModel->SetModel(apFilename);
    }
}

// Sets the model through the model's virtual SetModel.
void TESRace::SetBodyTextureModel(TESActorBase::SEX aeSex, const char* apFilename) {
    if (aeSex >= TESActorBase::SEX_MALE && aeSex <= TESActorBase::SEX_FEMALE) {
        TESModel* pModel = &pBodyTextureModels[aeSex];
        pModel->SetModel(apFilename);
    }
}

// The sex index is tested against 2, one past the female entry, deliberately.
void TESRace::SetHeadTextureName(uint32_t auiSex, uint32_t auiPart, const char* apFilename) {
    if (auiPart < RACE_HEAD_PART_COUNT && auiSex <= 2) {
        ppHeadTextureFiles[auiSex][auiPart].SetTextureName(apFilename);
    }
}

void TESRace::SetBodyTextureName(uint32_t auiSex, uint32_t auiPart, const char* apFilename) {
    if (auiSex <= TESActorBase::SEX_FEMALE && auiPart < RACE_BODY_PART_COUNT) {
        ppBodyTextureFiles[auiSex][auiPart].SetTextureName(apFilename);
    }
}

bool TESRace::CreateHead(
    BSFaceGenNiNode*& apOutNodeBiped,
    BSFaceGenNiNode*& apOutNodeSkinned,
    TESNPC* apNPC,
    bool abPrepareObjects,
    bool abForceNoSkinning,
    TESActorBase::SEX aeSex
) {
    BSFaceGenAnimationData* panimData;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_FACEGEN_SYSTEM,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESRace.cpp",
        1571
    );
    apOutNodeBiped = nullptr;
    apOutNodeSkinned = nullptr;
    panimData = new BSFaceGenAnimationData;
    apOutNodeBiped = new BSFaceGenNiNode;
    apOutNodeBiped->SetName("BSFaceGenNiNodeBiped");
    apOutNodeBiped->SetAnimationData(panimData);
    apOutNodeBiped->SetAnimationUpdate(true);
    apOutNodeBiped->SetApplyRotToParent(true);
    apOutNodeSkinned = new BSFaceGenNiNode;
    apOutNodeSkinned->SetName("BSFaceGenNiNodeSkinned");
    apOutNodeSkinned->SetAnimationData(panimData);
    apOutNodeSkinned->SetAnimationUpdate(false);
    apOutNodeSkinned->SetApplyRotToParent(false);
    FaceGenNpcData npcData;
    GetFaceGenData(apNPC, npcData, aeSex, false);
    BSFaceGenManager::CreateFaceGenHead(apOutNodeBiped, apOutNodeSkinned, npcData, abForceNoSkinning);
    if (abPrepareObjects) {
        BSFaceGenManager::PrepareHeadForShaders(apOutNodeBiped, npcData);
        BSFaceGenManager::PrepareHeadForShaders(apOutNodeSkinned, npcData);
    }
    if (apNPC && apNPC->iFormID == 7) {
        apOutNodeBiped->SetIAmPlayerCharacter(true);
        apOutNodeSkinned->SetIAmPlayerCharacter(true);
    }

    return true;
}

RACE_BODY_PART TESRace::BodyPartToRaceBodyPart(BIPED_OBJECT aePart) {
    if (static_cast<uint32_t>(aePart - BO_UPPER_BODY) <= RBP_RIGHT_HAND) {
        return static_cast<RACE_BODY_PART>(aePart - BO_UPPER_BODY);
    }

    return static_cast<RACE_BODY_PART>(-1);
}

BGSTextureModel::~BGSTextureModel() {}
