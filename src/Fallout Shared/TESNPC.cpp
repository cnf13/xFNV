#include "Fallout Shared/TESNPC.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMain/SettingCollectionMap.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBipedModelList.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSDefaultObjectManager.h"
#include "Fallout Shared/BGSHeadPart.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BGSVoiceType.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Fallout Shared/FaceGen/BSFaceGenModelExtraData.h"
#include "Fallout Shared/FaceGen/BSFaceGenNiNode.h"
#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"
#include "Fallout Shared/FaceGenNpcData.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageSchedule.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESAIForm.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESClass.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESEyes.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHair.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESLevItem.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatPlanner.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/DialoguePackage.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDefaultAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/exception.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

// The game settings this file defines, with their IDs and values, in their initialization order.
BSSimpleList<TESNPC*> TESNPC::CharGenPresetFaceList;
SettingT<GameSettingCollection> gs_fNPCGeneticVariation("fNPCGeneticVariation", 0.5f);
SettingT<GameSettingCollection> TESNPC::iVampirismAgeOffset("iVampirismAgeOffset", 20);
SettingT<GameSettingCollection> fAttributeClassPrimaryBonus("fAttributeClassPrimaryBonus", 5.0f);
SettingT<GameSettingCollection> fAttributeClassSecondaryBonus("fAttributeClassSecondaryBonus", 5.0f);

// The record's chunks: the components' chunks, the base stats, AI data, packages, factions, spells and inventory, the
// class, race, voice, template and death item IDs (kept in the pointers until InitItem resolves them), the hair, eyes
// and head parts (resolved here, with a warning when missing), the FaceGen coordinates, the face texture warning (once
// per record) and the hair, height and weight values. The player's base form then becomes the player's base object if
// it has none. When a chunk is empty (malformed data), the VTCK, TPLT, PNAM, HNAM, ENAM, LNAM, ZNAM and NAM4 locals are
// initialised to 0 and the EDID buffer to "".
bool TESNPC::Load(TESFile* apFile) {
    // The container entry the last CNTO chunk added, which a following COED chunk gives its extra data.
    static ContainerObject* plastloaded;
    if (static_cast<uint8_t>(apFile->GetTESForm()) != 0x2A) {
        return false;
    }
    iFileOffset = apFile->GetOffset();
    LoadForm(apFile);
    SetInitialized(false);
    bool bfacewarned = false;
    // Every chunk keeps the loop going and moves on; no case changes them.
    bool goodchunk;
    bool skipnext;
    do {
        uint32_t chunkid = apFile->GetTESChunk();
        if (chunkid == NO_CHUNK) {
            break;
        }
        // The FaceGen coordinate array a FGGS, FGGA or FGTS chunk fills: [etype][esymm].
        int32_t etype = -1;
        int32_t esymm = -1;
        goodchunk = true;
        skipnext = false;
        switch (chunkid) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pMessage = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // An empty chunk leaves the buffer unwritten (see above).
                pMessage[0] = '\0';
                apFile->GetChunkData(pMessage, 0x200);
                SetFormEditorID(pMessage);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case MODL_ID:
            case MODT_ID: TESModel::LoadModelChunk(this, apFile); break;
            // The ID chunks below hold form IDs, which are kept in the pointers (untyped until InitItem resolves them).
            case SCRI_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                TESScriptableForm::SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(tempint)));
                TESScriptableForm::InitItem(this);
                break;
            }
            case CNTO_ID:
            case COED_ID: {
                uint32_t uiChunk = apFile->GetTESChunk();
                if (uiChunk == COED_ID) {
                    if (plastloaded) {
                        plastloaded->pItemExtra = new ContainerItemExtra;
                        plastloaded->pItemExtra->Load(apFile);
                        plastloaded = nullptr;
                    }
                }
                else if (uiChunk == CNTO_ID) {
                    CONTAINER_OBJECT_FILE cfile;
                    ourMemset(&cfile, 0, sizeof(cfile));
                    apFile->GetChunkData(&cfile, 8);
                    if (apFile->QEndian()) {
                        cfile.Endian();
                    }
                    plastloaded = TESContainer::AddObject(&cfile);
                }
                break;
            }
            case SPLO_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                TESSpellList::AddSpell(reinterpret_cast<TESForm*>(static_cast<uintptr_t>(tempint)));
                break;
            }
            case EITM_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                BGSTouchSpellForm::SetFormTouchSpell(reinterpret_cast<MagicItemForm*>(static_cast<uintptr_t>(tempint)));
                break;
            }
            case EAMT_ID: {
                uint16_t stempval = 0;
                // LoadWord reads into a signed word; the animation group is the same 2 bytes, unsigned.
                apFile->LoadWord(reinterpret_cast<int16_t*>(&stempval));
                BGSTouchSpellForm::SetFormTouchSpellAnimation(stempval);
                break;
            }
            case AIDT_ID: {
                AIDATA kAIData;
                ourMemset(&kAIData, 0, sizeof(kAIData));
                apFile->GetChunkData(&kAIData, 0x14);
                if (apFile->QEndian()) {
                    kAIData.Endian();
                }
                // Before version 4 the aggression levels above 1 were one lower.
                if (apFile->GetFormVersion() < 4 && kAIData.cAggression > 1) {
                    ++kAIData.cAggression;
                }
                // Before version 6 one aggression value also stood for the assistance.
                if (apFile->GetFormVersion() < 6) {
                    switch (kAIData.cAggression) {
                        case 0: kAIData.cAssistance = 0; break;
                        case 1:
                            kAIData.cAggression = 0;
                            kAIData.cAssistance = 1;
                            break;
                        case 2:
                            kAIData.cAggression = 1;
                            kAIData.cAssistance = 1;
                            break;
                        case 3:
                            kAIData.cAggression = 1;
                            kAIData.cAssistance = 2;
                            break;
                        case 4:
                            kAIData.cAggression = 2;
                            kAIData.cAssistance = 2;
                            break;
                        case 5:
                            kAIData.cAggression = 3;
                            kAIData.cAssistance = 0;
                            break;
                        default: break;
                    }
                }
                // Before version 7 the confidence scale ran the other way.
                if (apFile->GetFormVersion() < 7) {
                    kAIData.cConfidence = static_cast<uint8_t>(4 - kAIData.cConfidence);
                }
                TESAIForm::SetAIData(&kAIData);
                break;
            }
            case PKID_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                TESAIForm::AddPackage(reinterpret_cast<TESPackage*>(static_cast<uintptr_t>(tempint)));
                break;
            }
            case SNAM_ID: {
                FACTION_RANK temp;
                ourMemset(&temp, 0, sizeof(temp));
                apFile->GetChunkData(&temp, 8);
                // The file's record is 8 bytes, the faction's 4-byte form ID first.
                TESFile::WidenFormIDRecords(&temp, 1, 8, sizeof(temp), {{0, offsetof(FACTION_RANK, pFaction)}});
                if (apFile->QEndian()) {
                    temp.Endian();
                }
                TESActorBaseData::SetFactionRank(temp.pFaction, static_cast<int8_t>(temp.cRank));
                break;
            }
            case INAM_ID: {
                uint32_t itempint = 0;
                apFile->GetChunkData(itempint);
                pDeathItem = reinterpret_cast<TESLevItem*>(static_cast<uintptr_t>(itempint));
                break;
            }
            case VTCK_ID: {
                uint32_t itempint = 0;
                apFile->GetChunkData(itempint);
                pVoiceType = reinterpret_cast<BGSVoiceType*>(static_cast<uintptr_t>(itempint));
                break;
            }
            case TPLT_ID: {
                uint32_t itempint = 0;
                apFile->GetChunkData(itempint);
                pTemplateForm = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(itempint));
                break;
            }
            case ACBS_ID:
                apFile->GetChunkData(&actorData, 0x18);
                if (apFile->QEndian()) {
                    actorData.Endian();
                }
                if (apFile->GetFormVersion() < 8) {
                    actorData.sTemplateUseFlags = 0;
                }
                actorData.usBartergold = 0;
                break;
            case RNAM_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                TESRaceForm::SetFormRace(reinterpret_cast<TESRace*>(static_cast<uintptr_t>(tempint)));
                break;
            }
            case KFFZ_ID: TESAnimation::LoadAnimationChunk(this, apFile); break;
            case DEST_ID:
            case DSTD_ID: BGSDestructibleObjectForm::LoadChunk(this, apFile); break;
            case DATA_ID: {
                uint32_t idatasize = apFile->GetChunkSize() - GetDataSizeAdded();
                if (idatasize) {
                    if (idatasize != 0x1C && idatasize * 2 != 0x1C) {
                        uint32_t uiFormID = GetFormID();
                        Warning("NPC '%s' (%08X) has invalid data format.", GetFormEditorID(), uiFormID);
                    }
                    // LoadData reads the chunk's bytes into an untyped buffer, which is how the skill data is passed.
                    LoadData(apFile, reinterpret_cast<char*>(&data), static_cast<uint16_t>(idatasize));
                }
                else {
                    LoadData(apFile, nullptr, 0);
                }
                break;
            }
            case DNAM_ID: apFile->GetChunkData(&data, 0x1C); break;
            case FGGS_ID:
                if (etype == -1) {
                    etype = 0;
                    esymm = 0;
                }
                [[fallthrough]];
            case FGGA_ID:
                if (etype == -1) {
                    etype = 0;
                    esymm = 1;
                }
                [[fallthrough]];
            case FGTS_ID: {
                if (etype == -1) {
                    etype = 1;
                    esymm = 0;
                }
                uint32_t imemsize = apFile->GetChunkSize();
                int32_t isize = static_cast<int32_t>(imemsize >> 2);
                RaceFaceOffsetCoord[etype][esymm].resize(isize, 1);
                // A 16-byte aligned stack buffer of the chunk's size.
                float* pfloats = static_cast<float*>(XFNV_ALLOCA16(imemsize));
                ourMemset(pfloats, 0, imemsize);
                apFile->GetChunkData(pfloats, imemsize);
                for (uint32_t i = 0; i < static_cast<uint32_t>(isize); ++i) {
                    if (apFile->QEndian()) {
                        ENDIAN(pfloats[i], false);
                    }
                    RaceFaceOffsetCoord[etype][esymm][i][0] = pfloats[i];
                }
                break;
            }
            case PNAM_ID: {
                uint32_t itempint = 0;
                apFile->GetChunkData(itempint);
                TESForm::AddCompileIndex(itempint, apFile);
                BGSHeadPart* pHeadPart = dynamic_cast<BGSHeadPart*>(TESForm::GetFormByNumericID(itempint));
                if (pHeadPart) {
                    GetHeadParts()->AddHead(pHeadPart);
                }
                else {
                    uint32_t uiFormID = GetFormID();
                    Warning(
                        "MASTERFILE: Could not find headpart (%08X) on NPC '%s' (%08X).",
                        itempint,
                        GetFormEditorID(),
                        uiFormID
                    );
                }
                break;
            }
            case HNAM_ID: {
                uint32_t itempint = 0;
                apFile->GetChunkData(itempint);
                TESForm::AddCompileIndex(itempint, apFile);
                TESHair* pHair = dynamic_cast<TESHair*>(TESForm::GetFormByNumericID(itempint));
                if (pHair) {
                    SetHair(pHair);
                }
                else {
                    uint32_t uiFormID = GetFormID();
                    Warning(
                        "MASTERFILE: Could not find hair (%08X) on NPC '%s' (%08X).",
                        itempint,
                        GetFormEditorID(),
                        uiFormID
                    );
                }
                break;
            }
            case LNAM_ID: {
                float tempfloat = 0.0f;
                apFile->GetChunkData(tempfloat);
                // Compares against double constants (0.0 and 1.0); a NaN length is kept.
                if (static_cast<double>(tempfloat) < 0.0) {
                    tempfloat = 0.0f;
                }
                if (static_cast<double>(tempfloat) > 1.0) {
                    tempfloat = 1.0f;
                }
                SetHairLength(tempfloat);
                break;
            }
            case ENAM_ID: {
                uint32_t itempint = 0;
                apFile->GetChunkData(itempint);
                TESForm::AddCompileIndex(itempint, apFile);
                TESEyes* pEyes = dynamic_cast<TESEyes*>(TESForm::GetFormByNumericID(itempint));
                if (pEyes) {
                    SetEyeColor(pEyes);
                }
                else {
                    uint32_t uiFormID = GetFormID();
                    Warning(
                        "MASTERFILE: Could not find eyes (%08X) on NPC '%s' (%08X).",
                        itempint,
                        GetFormEditorID(),
                        uiFormID
                    );
                }
                break;
            }
            case NAM9_ID:
            case HCLR_ID: apFile->GetChunkData(iHairColor); break;
            case CNAM_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                pCl = reinterpret_cast<TESClass*>(static_cast<uintptr_t>(tempint));
                break;
            }
            case ZNAM_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                SetCombatStyle(reinterpret_cast<TESCombatStyle*>(static_cast<uintptr_t>(tempint)));
                break;
            }
            case FNAM_ID:
                if (apFile->GetChunkSize() >= 2) {
                    // LoadWord reads into a signed word; the face number is the same 2 bytes, unsigned.
                    apFile->LoadWord(reinterpret_cast<int16_t*>(&sLastRaceFaceNum));
                    if (apFile->QEndian()) {
                        ENDIAN(sLastRaceFaceNum, false);
                    }
                }
                break;
            case NAM0_ID:
            case NAM1_ID:
            case NAM2_ID:
            case NAM3_ID:
                if (!bfacewarned) {
                    const char* pFilename = apFile->GetFilename();
                    uint32_t uiFormID = GetFormID();
                    Warning(
                        "MASTERFILE: Found face texture for NPC '%s' (%08X) in file '%s'",
                        GetFormEditorID(),
                        uiFormID,
                        pFilename
                    );
                    bfacewarned = true;
                }
                break;
            case NAM4_ID: {
                uint32_t tempint = 0;
                apFile->GetChunkData(tempint);
                eBloodImpactMaterial = static_cast<BGSImpactDataSet::IMPACT_MATERIAL_TYPE>(tempint);
                break;
            }
            case NAM6_ID: apFile->GetChunkData(fHeight); break;
            case NAM7_ID: apFile->GetChunkData(fWeight); break;
            default: break;
        }
    }
    while (goodchunk && (skipnext || apFile->NextChunk()));
    if (GetFormID() == 7 && pPlayer && !pPlayer->GetObjectReference()) {
        pPlayer->SetObjectReference(this);
    }
    if (!TESActorBaseData::GetSpeedMultiplier()) {
        uint32_t uiFormID = GetFormID();
        Warning("MASTERFILE: Speed multiplier is zero on NPC '%s' (%08X).", GetFormEditorID(), uiFormID);
    }
    return true;
}

// qsort's compare over the preset array's TESNPC* items: their full names in BSstrcmp order; 0 when either item or its
// NPC is null. The full name getter is called with no null test, after testing both NPCs.
int32_t PresetIndexSortFunction(const void* apPtr1, const void* apPtr2) {
    TESNPC* const* ppNPC1 = static_cast<TESNPC* const*>(apPtr1);
    TESNPC* const* ppNPC2 = static_cast<TESNPC* const*>(apPtr2);
    if (!ppNPC1 || !*ppNPC1 || !ppNPC2 || !*ppNPC2) {
        return 0;
    }
    const char* pName2 = (*ppNPC2)->TESFullName::GetFullName();
    const char* pName1 = (*ppNPC1)->TESFullName::GetFullName();
    return BSstrcmp(pName1, pName2);
}

TESNPC::TESNPC() : spHeadBiped(nullptr), spHeadSkinned(nullptr), spBodyModTexture(nullptr), FaceGenUndoStates(0, 1) {
    cFormType = 42;
    TESNPC::InitializeData();
    SetResponsibility(50, false);
    SetAggression(0, false);
    SetConfidence(2, false);
    SetEnergy(50, false);
    iFileOffset = 0;
}

TESNPC::~TESNPC() {
    TESNPC::ClearData();
    if (GetFlag(BASE_HAS_CHARGEN_FACE)) {
        TESNPC* pNPC = this;
        TESNPC::CharGenPresetFaceList.Remove(pNPC);
    }
}

// The defaults: no hair, eyes, class or combat style, the default hair colour, skills of 5 with no offsets, the
// race's face offsets zeroed and the manager's current face copied into the NPC's offsets. The face nodes are left
// alone while a save is loading.
void TESNPC::InitializeData() {
    fWeight = 0.0f;
    fHeight = 0.0f;
    pHair = nullptr;
    fHairLength = 0.0f;
    pEyeColor = nullptr;
    if (!pSaveLoadGameOLD->GetLoading()) {
        spHeadBiped = (spHeadSkinned = nullptr);
        spBodyModTexture = nullptr;
    }
    sLastRaceFaceNum = 255;
    pCl = nullptr;
    pCombatStyle = nullptr;
    iHairColor = 0x19324B;
    pAlternateFaceOffsetCoord = nullptr;
    // Takes the NPC's offsets before the manager's coordinates.
    FR2MatrixVTC<float>(*pOffsetFaceCoord)[2][2] = GetOffsetFaceCoord();
    FR2MatrixVTC<float>(*pFaceGenCoord)[2][2] = BSFaceGenManager::GetFaceGenCoord();
    BSFaceGenManager::CopyFaceGenCoord(pFaceGenCoord, pOffsetFaceCoord, false, false);
    for (int32_t i = 0; i < 2; ++i) {
        for (int32_t j = 0; j < 2; ++j) {
            if (RaceFaceOffsetCoord[i][j].numRows()) {
                RaceFaceOffsetCoord[i][j].setZero();
            }
        }
    }
    for (int32_t k = 0; k < 14; ++k) {
        data.cSkillValues[k] = 5;
        data.cSkillOffsets[k] = 0;
    }
    eBloodImpactMaterial = BGSImpactDataSet::IMP_MAT_ORGANIC;
    pOriginalRace = nullptr;
    pFaceNPC = nullptr;
    TESActorBaseData::SetRunsInLow(true, false);
    TESHealthForm::SetFormHealth(0x32);
}

// Drops the face nodes (unless a save is loading), the head parts and the alternate face offsets.
void TESNPC::ClearData() {
    if (!pSaveLoadGameOLD->GetLoading()) {
        spHeadSkinned = (spHeadBiped = nullptr);
        spBodyModTexture = nullptr;
    }
    sLastRaceFaceNum = 255;
    listHeadParts.RemoveAll();
    delete[] pAlternateFaceOffsetCoord;
    pAlternateFaceOffsetCoord = nullptr;
}

void TESNPC::Save() {
    StartForm();
    TESFullName::Save();
    TESModel::Save(MODL_ID, MODT_ID);
    TESActorBaseData::Save();
    TESRaceForm::SaveSubRecord();
    TESSpellList::Save();
    BGSTouchSpellForm::SaveSubRecord();
    BGSDestructibleObjectForm::Save();
    if (pCl) {
        uint32_t tempint = pCl->iFormID;
        TESForm::AddChunk(CNAM_ID, tempint);
    }
    SaveData();
    if (TESForm::QEndianSwapOnSave()) {}
    TESForm::__AddChunkData(DNAM_ID, &data, 28u);
    if (TESForm::QEndianSwapOnSave()) {}
    // The head part list holds BGSHeadPart pointers in a void* list.
    for (BSSimpleList<void*>* phplist = GetHeadParts(); phplist && !phplist->IsEmpty(); phplist = phplist->m_pkNext) {
        BGSHeadPart* ppart = static_cast<BGSHeadPart*>(phplist->m_item);
        TESForm::AddChunk(PNAM_ID, ppart->iFormID);
    }
    if (GetHair()) {
        TESForm::AddChunk(HNAM_ID, GetHair()->iFormID);
        float fLength = GetHairLength();
        TESForm::AddChunk(LNAM_ID, std::bit_cast<uint32_t>(fLength));
    }
    if (GetEyeColor()) {
        TESForm::AddChunk(ENAM_ID, GetEyeColor()->iFormID);
    }
    TESForm::AddChunk(HCLR_ID, iHairColor);
    if (GetCombatStyle()) {
        TESForm::AddChunk(ZNAM_ID, GetCombatStyle()->iFormID);
    }
    // The enum promotes to its int32_t underlying type and uses the 4-byte overload.
    TESForm::AddChunk(NAM4_ID, eBloodImpactMaterial);
    uint32_t isize = RaceFaceOffsetCoord[0][0].nrows;
    if (isize) {
        uint32_t imemsize = 4 * isize;
        float* pfloats = static_cast<float*>(ourMalloc(sizeof(float) * isize));
        ourMemset(pfloats, 0, imemsize);
        for (uint32_t i = 0; i < isize; ++i) {
            pfloats[i] = RaceFaceOffsetCoord[0][0][i][0];
        }
        TESForm::AddChunkArray(FGGS_ID, pfloats, isize);
        ourFree(pfloats);
        pfloats = nullptr;
    }
    isize = RaceFaceOffsetCoord[0][1].nrows;
    if (isize) {
        uint32_t imemsize = 4 * isize;
        float* pfloats = static_cast<float*>(ourMalloc(sizeof(float) * isize));
        ourMemset(pfloats, 0, imemsize);
        for (uint32_t i = 0; i < isize; ++i) {
            pfloats[i] = RaceFaceOffsetCoord[0][1][i][0];
        }
        TESForm::AddChunkArray(FGGA_ID, pfloats, isize);
        ourFree(pfloats);
        pfloats = nullptr;
    }
    isize = RaceFaceOffsetCoord[1][0].nrows;
    if (isize) {
        uint32_t imemsize = 4 * isize;
        float* pfloats = static_cast<float*>(ourMalloc(sizeof(float) * isize));
        ourMemset(pfloats, 0, imemsize);
        for (uint32_t i = 0; i < isize; ++i) {
            pfloats[i] = RaceFaceOffsetCoord[1][0][i][0];
        }
        TESForm::AddChunkArray(FGTS_ID, pfloats, isize);
        ourFree(pfloats);
        pfloats = nullptr;
    }
    TESForm::AddChunk(NAM5_ID, sLastRaceFaceNum);
    TESForm::AddChunk(NAM6_ID, std::bit_cast<uint32_t>(fHeight));
    TESForm::AddChunk(NAM7_ID, std::bit_cast<uint32_t>(fWeight));
    CloseForm();
}

uint16_t TESNPC::GetSaveSize(uint32_t aiFlags) {
    uint16_t saveSize = TESActorBase::GetSaveSize(aiFlags);
    if ((aiFlags & CHANGE_NPC_SKILLS) != 0) {
        saveSize += 14;
    }
    return saveSize;
}

void TESNPC::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    bool bfemale;
    float foffset;
    bool balternativeCoords;
    TESActorBase::SaveGame(apSaveGameBuffer);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_SKILLS)) {
        apSaveGameBuffer->SaveDataEndian(&data, sizeof(data), 0);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_CLASS)) {
        apSaveGameBuffer->SaveFormID(pCl, 0);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_RACE)) {
        TESRace* prace = pFormRace;
        apSaveGameBuffer->SaveFormID(prace, 0);
        apSaveGameBuffer->SaveFormID(pOriginalRace, 0);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_FACE)) {
        balternativeCoords = false;
        FR2MatrixVTC<float>* pfaceCoords = RaceFaceOffsetCoord[0];
        if (pAlternateFaceOffsetCoord) {
            pfaceCoords = (*pAlternateFaceOffsetCoord)[0];
            balternativeCoords = true;
        }
        apSaveGameBuffer->SaveDataEndian(&balternativeCoords, sizeof(balternativeCoords), 0);
        for (uint32_t i = 0; i < 2; ++i) {
            for (uint32_t j = 0; j < 2; ++j) {
                uint32_t inumrows = pfaceCoords[2 * i + j].nrows;
                uint32_t inumcolumns = pfaceCoords[2 * i + j].ncols;
                for (uint32_t k = 0; k < inumrows; ++k) {
                    for (uint32_t l = 0; l < inumcolumns; ++l) {
                        foffset = pfaceCoords[2 * i + j][k][l];
                        apSaveGameBuffer->SaveDataEndian(&foffset, sizeof(foffset), 0);
                    }
                }
            }
        }
        apSaveGameBuffer->SaveFormID(pHair, 0);
        apSaveGameBuffer->SaveFormID(pEyeColor, 0);
        apSaveGameBuffer->SaveDataEndian(&fHairLength, sizeof(fHairLength), 0);
        apSaveGameBuffer->SaveDataEndian(&iHairColor, sizeof(iHairColor), 0);
        uint32_t isaveGameCount = 0;
        uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
        for (BSSimpleList<void*>* plist = &listHeadParts; plist; plist = plist->m_pkNext) {
            BGSHeadPart* pForm = static_cast<BGSHeadPart*>(plist->m_item);
            if (pForm) {
                apSaveGameBuffer->SaveFormID(pForm, 0);
                ++isaveGameCount;
            }
        }
        apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_GENDER)) {
        bfemale = GetFlag(BASE_FEMALE);
        apSaveGameBuffer->SaveDataEndian(&bfemale, sizeof(bfemale), 0);
    }
}

void TESNPC::SaveGame(uint32_t aiFlags) {
    TESActorBase::SaveGame(aiFlags);
    if ((aiFlags & CHANGE_NPC_SKILLS) != 0) {
        SaveGameDataOLD(&data, 14);
    }
}

void TESNPC::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    bool bfemale;
    BGSHeadPart* pForm;
    BGSHeadPart* ppart;
    float foffset;
    bool balternativeCoords;
    // The hair colour before the load, converted to float as the game keeps it.
    float fcurrentHairColor;
    TESActorBase::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_SKILLS)) {
        apLoadGameBuffer->LoadData(&data, sizeof(data));
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_CLASS)) {
        TESForm* pClassForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
        pCl = dynamic_cast<TESClass*>(pClassForm);
    }
    uint8_t cresetFlags = 0;
    if (apLoadGameBuffer->GetReconstructedForm()) {
        cresetFlags |= RESET_MODEL | RESET_SKIN | RESET_FACE | RESET_SCALE;
    }
    bool bRaceHeight = fHeight == GetRaceHeight();
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_RACE)) {
        TESForm* pRaceForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
        TESRace* prace = dynamic_cast<TESRace*>(pRaceForm);
        if (prace != pFormRace) {
            pFormRace = prace;
            cresetFlags |= RESET_MODEL | RESET_SKIN | RESET_FACE | RESET_SCALE;
        }
        TESForm* pOriginalRaceForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
        pOriginalRace = dynamic_cast<TESRace*>(pOriginalRaceForm);
    }
    if (bRaceHeight) {
        fHeight = GetRaceHeight();
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_FACE)) {
        balternativeCoords = false;
        apLoadGameBuffer->LoadData(&balternativeCoords, sizeof(balternativeCoords));
        FR2MatrixVTC<float>* pfaceCoords = RaceFaceOffsetCoord[0];
        if (balternativeCoords) {
            if (!pAlternateFaceOffsetCoord) {
                pAlternateFaceOffsetCoord = new FR2MatrixVTC<float>[1][2][2];
                BSFaceGenManager::InitFaceGenCoord(pAlternateFaceOffsetCoord);
            }
            pfaceCoords = (*pAlternateFaceOffsetCoord)[0];
        }
        for (uint32_t i = 0; i < 2; ++i) {
            for (uint32_t j = 0; j < 2; ++j) {
                uint32_t inumrows = pfaceCoords[2 * i + j].nrows;
                uint32_t inumcolumns = pfaceCoords[2 * i + j].ncols;
                for (uint32_t k = 0; k < inumrows; ++k) {
                    for (uint32_t l = 0; l < inumcolumns; ++l) {
                        foffset = 0.0f;
                        apLoadGameBuffer->LoadData(&foffset, sizeof(foffset));
                        double dCurrentOffset = pfaceCoords[2 * i + j][k][l];
                        if (foffset != dCurrentOffset) {
                            pfaceCoords[2 * i + j][k][l] = foffset;
                            cresetFlags |= RESET_FACE;
                        }
                    }
                }
            }
        }
        TESHair* pcurrentHair = pHair;
        TESEyes* pcurrentEyes = pEyeColor;
        float fcurrentHairLength = fHairLength;
        fcurrentHairColor = static_cast<float>(iHairColor);
        TESForm* pHairForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
        pHair = dynamic_cast<TESHair*>(pHairForm);
        TESForm* pEyesForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
        pEyeColor = dynamic_cast<TESEyes*>(pEyesForm);
        apLoadGameBuffer->LoadData(&fHairLength, sizeof(fHairLength));
        apLoadGameBuffer->LoadData(&iHairColor, sizeof(iHairColor));
        bool bheadPartsChanged = false;
        if (apLoadGameBuffer->GetVersion() >= 14u) {
            BSSimpleList<void*>* plist;
            using HeadPartScrapArray = BSScrapArray<BGSHeadPart*, 1024>;
            HeadPartScrapArray oldHeadParts;
            for (plist = &listHeadParts; plist; plist = plist->m_pkNext) {
                ppart = static_cast<BGSHeadPart*>(plist->m_item);
                if (ppart) {
                    oldHeadParts.Add(ppart);
                }
            }
            listHeadParts.RemoveAll();
            uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
            for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
                TESForm* pHeadPartForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
                pForm = dynamic_cast<BGSHeadPart*>(pHeadPartForm);
                if (pForm) {
                    listHeadParts.AddHead(*reinterpret_cast<void**>(&pForm));
                }
            }
            for (plist = &listHeadParts; plist; plist = plist->m_pkNext) {
                ppart = static_cast<BGSHeadPart*>(plist->m_item);
                if (ppart) {
                    uint32_t iindex = oldHeadParts
                                          .Find(ppart, 0, &BSSimpleArray<BGSHeadPart*>::DefaultEqualFunctionByAddress);
                    if (iindex == -1) {
                        bheadPartsChanged = true;
                        break;
                    }
                    oldHeadParts.RemoveFast(iindex, 1u);
                }
            }
            if (oldHeadParts.iSize) {
                bheadPartsChanged = true;
            }
        }
        if (bheadPartsChanged ||
            pcurrentHair != pHair ||
            pcurrentEyes != pEyeColor ||
            fHairLength != fcurrentHairLength ||
            fcurrentHairColor != static_cast<double>(iHairColor)) {
            cresetFlags |= RESET_FACE;
        }
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_GENDER)) {
        bfemale = false;
        apLoadGameBuffer->LoadData(&bfemale, sizeof(bfemale));
        if (GetFlag(BASE_FEMALE) != bfemale) {
            SetFlagBit(BASE_FEMALE, bfemale, true);
            cresetFlags |= RESET_MODEL | RESET_SKIN | RESET_FACE | RESET_SCALE;
        }
    }
    if ((cresetFlags & RESET_FACE) != 0) {
        ClearHead();
    }
    if (cresetFlags) {
        Reset3DForLoadedActors(cresetFlags);
    }
}

void TESNPC::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESActorBase::LoadGame(aiFlags, aiCurrentFlags);
    if ((aiFlags & CHANGE_NPC_SKILLS) != 0) {
        LoadGameDataOLD(&data, 14);
    }
}

// Undoes the race, face and gender changes the save had but the loaded form has not: the original race comes back,
// the alternate face offsets and head parts are dropped (a face reset also clears the head), and the female flag is
// flipped back. A height that was the race's follows the race. Each change is tested in the old flags, then its
// absence in the current flags.
void TESNPC::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESActorBase::Revert(apLoadGameBuffer);
    uint8_t cResetFlags = 0;
    bool bRaceHeight = GetRaceHeight() == fHeight;
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_NPC_RACE) &&
        !apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_RACE) &&
        pOriginalRace) {
        TESRaceForm::SetFormRace(pOriginalRace);
        pOriginalRace = nullptr;
        cResetFlags |= RESET_MODEL | RESET_SKIN | RESET_FACE | RESET_SCALE;
    }
    if (bRaceHeight) {
        fHeight = GetRaceHeight();
    }
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_NPC_FACE) &&
        !apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_FACE)) {
        delete[] pAlternateFaceOffsetCoord;
        pAlternateFaceOffsetCoord = nullptr;
        cResetFlags |= RESET_FACE;
        listHeadParts.RemoveAll();
    }
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_NPC_GENDER) &&
        !apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_GENDER)) {
        TESActorBaseData::SetFlagBit(BASE_FEMALE, !TESActorBaseData::GetFlag(BASE_FEMALE), true);
        cResetFlags |= RESET_MODEL | RESET_SKIN | RESET_FACE | RESET_SCALE;
    }
    if (cResetFlags & RESET_FACE) {
        ClearHead();
    }
    if (cResetFlags) {
        Reset3DForLoadedActors(cResetFlags);
    }
}

// The flags overload only runs TESActorBase's (TESForm's empty one).
void TESNPC::Revert(uint32_t aiFlags) {
    TESActorBase::Revert(aiFlags);
}

bool TESNPC::FindInFileFast(TESFile* apFile) {
    if (!apFile) {
        return false;
    }
    if (!iFileOffset) {
        return false;
    }
    if (!apFile->OpenTES(NiFile::READ_ONLY, false)) {
        return false;
    }
    if (!apFile->SetOffset(iFileOffset)) {
        return false;
    }
    if (apFile->GetTESForm() != formEnumString[NPC__ID].cFormID) {
        return false;
    }
    int32_t* currentFormID = reinterpret_cast<int32_t*>(static_cast<uintptr_t>(apFile->TESFile::GetID()));
    if (currentFormID == reinterpret_cast<int32_t*>(static_cast<uintptr_t>(TESForm::GetFormID()))) {
        return true;
    }
    return false;
}

// Resolves the loaded form IDs of the components, the class and the combat style (the default one when there is none),
// recomputes an auto-calc NPC's stats, and takes the race's height when the NPC has none. A preset-face NPC is listed
// in CharGenPresetFaceList.
void TESNPC::InitItem() {
    if (GetInitialized()) {
        return;
    }
    if (GetFlag(BASE_HAS_CHARGEN_FACE)) {
        TESNPC* pNPC = this;
        CharGenPresetFaceList.AddHead(pNPC);
    }
    TESScriptableForm::InitItem(this);
    TESContainer::InitItem(this);
    TESAIForm::InitItem(this);
    TESRaceForm::InitItem(this);
    TESActorBaseData::InitItem(this);
    TESSpellList::InitItem(this);
    BGSTouchSpellForm::InitItem(this);
    BGSDestructibleObjectForm::InitItem(this);
    if (pCl) {
        uint32_t iClassID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCl));
        TESForm::AddCompileIndex(iClassID, GetFile(-1));
        pCl = dynamic_cast<TESClass*>(TESForm::GetFormByNumericID(iClassID));
    }
    if (IsAutoCalc()) {
        InitValues(false);
    }
    if (pCombatStyle) {
        uint32_t iCombatStyleID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetCombatStyle()));
        if (iCombatStyleID) {
            TESForm::AddCompileIndex(iCombatStyleID, GetFile(-1));
            pCombatStyle = dynamic_cast<TESCombatStyle*>(TESForm::GetFormByNumericID(iCombatStyleID));
            if (!pCombatStyle) {
                const char* pEditorID = GetFormEditorID();
                uint32_t iFormID = GetFormID();
                Warning(
                    "MASTERFILE: Combat Style (%08X) not found in InitItem for NPC (%08X) '%s'.",
                    iCombatStyleID,
                    iFormID,
                    pEditorID
                );
            }
        }
    }
    if (!GetCombatStyle()) {
        SetCombatStyle(TESCombatStyle::GetDefaultCombatStyle());
    }
    if (!TESHealthForm::GetFormHealth()) {
        SetHasTemp3D(true);
    }
    if (fHeight == 0.0) {
        fHeight = GetRaceHeight();
    }
    SetInitialized(true);
}

// Copies every component, the skills, class, combat style, blood material, head data, height and weight of another NPC.
// Checks the form type rather than casting, and hands apCopy itself to each CopyComponent.
void TESNPC::Copy(TESForm* apCopy) {
    if (apCopy->GetFormType() != NPC__ID) {
        return;
    }
    TESNPC* pCopy = static_cast<TESNPC*>(apCopy);
    TESAIForm::CopyComponent(apCopy);
    TESSpellList::CopyComponent(apCopy);
    BGSTouchSpellForm::CopyComponent(apCopy);
    TESRaceForm::CopyComponent(apCopy);
    TESActorBaseData::CopyComponent(apCopy);
    TESHealthForm::CopyComponent(apCopy);
    TESAttributes::CopyComponent(apCopy);
    TESFullName::CopyComponent(apCopy);
    TESModel::CopyComponent(apCopy);
    TESScriptableForm::CopyComponent(apCopy);
    TESContainer::CopyComponent(apCopy);
    TESAnimation::CopyComponent(apCopy);
    BGSDestructibleObjectForm::CopyComponent(apCopy);
    ourMemcpy(&data, pCopy->GetSkills(), sizeof(TESNPC_Data));
    pCl = pCopy->GetClass();
    SetCombatStyle(pCopy->GetCombatStyle());
    TESActorBaseData* pCopyData = pCopy;
    TESActorBaseData* pThisData = this;
    pThisData->SetBloodImpactMaterial(pCopyData->GetBloodImpactMaterial());
    CopyHeadData(pCopy);
    fHeight = pCopy->fHeight;
    fWeight = pCopy->fWeight;
}

bool TESNPC::Compare(TESForm* apCompare) {
    TESNPC* pComp = dynamic_cast<TESNPC*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    if (std::memcmp(&data, pComp->GetSkills(), sizeof(data))) {
        return true;
    }
    if (pCl != pComp->GetClass()) {
        return true;
    }
    if (pHair != pComp->GetHair()) {
        return true;
    }
    if (fHairLength != pComp->GetHairLength()) {
        return true;
    }
    if (pEyeColor != pComp->GetEyeColor()) {
        return true;
    }
    if (iHairColor != pComp->GetHairColor()) {
        return true;
    }
    FR2MatrixVTC<float>(*pCompOffsetFaceCoord)[2][2] = pComp->GetOffsetFaceCoord();
    FR2MatrixVTC<float>(*pOffsetFaceCoord)[2][2] = GetOffsetFaceCoord();
    if (BSFaceGenManager::CompareFaceGenCoord(pOffsetFaceCoord, pCompOffsetFaceCoord)) {
        return true;
    }
    TESCombatStyle* pStyle = GetCombatStyle();
    if (pStyle != pComp->GetCombatStyle()) {
        return true;
    }
    BGSImpactDataSet::IMPACT_MATERIAL_TYPE eMaterial = GetBloodImpactMaterial();
    if (eMaterial != pComp->GetBloodImpactMaterial()) {
        return true;
    }
    int32_t iNumHeadParts = GetHeadParts()->ItemsInList();
    if (iNumHeadParts != pComp->GetHeadParts()->ItemsInList()) {
        return true;
    }
    for (BSSimpleList<void*>* phplist = GetHeadParts(); phplist && !phplist->IsEmpty(); phplist = phplist->m_pkNext) {
        if (!pComp->GetHeadParts()->IsInList(phplist->m_item)) {
            return true;
        }
    }
    return pComp->fHeight != fHeight || pComp->fWeight != fWeight;
}

bool TESNPC::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjecttoGet,
    int32_t aicount
) {
    Actor* pActionActor = dynamic_cast<Actor*>(apActionRef);
    Actor* pactor = dynamic_cast<Actor*>(apItemActivated);
    if (!pactor || pactor->IsDying()) {
        return false;
    }
    BaseProcess* pprocess = pactor->pCurrentProcess;
    if (!pprocess) {
        return false;
    }
    if (pprocess->GetCommandingActor() && !pactor->IsDead(false)) {
        return false;
    }
    if (pprocess->GetFadeState()) {
        return false;
    }
    TESPackage* pactionpackage = pActionActor->pCurrentProcess->GetPackageThatIsRunning();
    TESPackage* potherpackage = pprocess->GetPackageThatIsRunning();
    if (pactor == pPlayer && pPlayer->btransporting) {
        if (pactor->IsActoraGuard() && pactor->IsAlarmed()) {
            float fHour = appCalendar.GetHour() - 1.0;
            pprocess->SetLastTimeProcessed(fHour);
        }
        return false;
    }

    char msg[304];
    if (!pactor->IsDead(false) || pactor->GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN) {
        if (pactor->IsUnConscious() ||
            pactor->IsKnockedOut() ||
            pactor->GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN) {
            const char* pMessage = sNoTalkUnConscious.String();
            std::sprintf(msg, "%s %s", pactor->GetFullName(), pMessage);
            Interface::ShowMessage(
                msg,
                0,
                "Interfac\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds",
                nullptr,
                2.0f,
                false
            );
            return false;
        }
        if (pactor->IsFleeing(false)) {
            CombatController* pcombat = pactor->GetCombatController();
            if (!pcombat || !pcombat->IsActoraCombatTarget(pPlayer) || pPlayer->GetBlocked()) {
                const char* pMessage = sNoTalkFleeing.String();
                std::sprintf(msg, "%s %s", pactor->GetFullName(), pMessage);
                Interface::ShowMessage(
                    msg,
                    0,
                    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                    nullptr,
                    2.0f,
                    false
                );
                return false;
            }
        }
    }
    if (pactor->IsPlayerTeammate() && apActionRef == pPlayer) {
        if (Actor::IsConsciousAndNotAttackingPlayer(pactor)) {
            Interface::QueueMenuCreate(
                Interface::OPEN_COMPANION_WHEEL,
                apItemActivated,
                nullptr,
                nullptr,
                Interface::CMM_NONE,
                false
            );
        }
        return false;
    }

    if (pActionActor) {
        TESPackage* peditorpack = pActionActor->m_Extra.GetPackageExtra();
        if (pActionActor != pPlayer && peditorpack && peditorpack->GetPackType() == PACKAGE_TYPE_DIALOGUE) {
            TESObjectREFR* pTarget = pactor->m_Extra.GetPackageExtraTarget();
            pActionActor->m_Extra.SetPackageExtra(peditorpack, 4, pTarget, true, true, false);
            if (peditorpack->GetOncePerDay()) {
                pActionActor->SetRunOnceExtraPackage(peditorpack, appCalendar.GetDay());
            }
            pactor->pCurrentProcess->PackageDone(pActionActor, peditorpack);
            pactor->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
        }

        if (pActionActor != pPlayer && pActionActor->pCurrentProcess) {
            pActionActor->pCurrentProcess->SetTargetActivated(true);
        }

        if (pactor->IsInCombat() && pActionActor == pPlayer && pactor->IsPlayerTeammate()) {
            pVATS.QuitVATSPlayback(false, false);
            Interface::QueueMenuCreate(
                Interface::OPEN_DIALOG,
                pactor,
                nullptr,
                nullptr,
                Interface::CMM_STANDARD,
                false
            );
        }
        else {
            if ((!pactor->IsInCombat() || pactor->IsKnockedOut()) &&
                (pActionActor != pPlayer ||
                 pactor->GetSitSleepState() == SIT_SLEEP_NORMAL ||
                 pactor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING ||
                 pactor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
                 (pactor->pCurrentProcess->GetFurnitureMarker() &&
                  pactor->pCurrentProcess->GetFurnitureMarker()->cNumber > 19u))) {
                if (pactor->IsReanimated()) {
                    return false;
                }
                if (pactor->IsDead(false)) {
                    if (pActionActor == pPlayer) {
                        Interface::QueueMenuCreate(
                            Interface::OPEN_CONTAINER,
                            apItemActivated,
                            nullptr,
                            nullptr,
                            Interface::CMM_STANDARD,
                            false
                        );
                    }
                    else if (apObjecttoGet && !dynamic_cast<TESNPC*>(apObjecttoGet)) {
                        apItemActivated->RemoveItem(
                            apObjecttoGet,
                            nullptr,
                            aicount,
                            false,
                            false,
                            pActionActor,
                            nullptr,
                            nullptr,
                            true,
                            false
                        );
                    }
                }
                else {
                    FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
                    if (!pactor->IsRestrained() && !pactor->IsParalyzed()) {
                        if (pActionActor == pPlayer ||
                            !pActionActor->GetShouldAttackActor(pactor, false, ereaction, false)) {
                            if (!pactor->IsActoraGuard() &&
                                pactor->GetShouldAttackActor(pActionActor, false, ereaction, false) &&
                                !pActionActor->IsSneaking()) {
                                if (pactor->pCurrentProcess->EnterCombat(
                                        pactor,
                                        pActionActor,
                                        true,
                                        true,
                                        nullptr,
                                        false,
                                        false,
                                        true,
                                        0,
                                        false,
                                        false,
                                        true,
                                        nullptr
                                    )) {
                                    return true;
                                }
                            }
                        }
                        else {
                            if (pActionActor->pCurrentProcess->EnterCombat(
                                    pActionActor,
                                    pactor,
                                    false,
                                    true,
                                    nullptr,
                                    false,
                                    false,
                                    true,
                                    0,
                                    false,
                                    false,
                                    true,
                                    nullptr
                                )) {
                                return true;
                            }
                        }
                    }
                    if (pActionActor == pPlayer && pactor->IsKnockedOut() && pactor->GetEssential()) {
                        return false;
                    }
                    if (pActionActor == pPlayer ||
                        (pactor == pPlayer &&
                         (pActionActor->GetCurrentPackage()->IsInterruptPackage() ||
                          pActionActor->GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_EXPLORE ||
                          pActionActor->GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_AMBUSH))) {
                        if (pActionActor == pPlayer &&
                            (pactor->IsKnockedOut() || (pPlayer->IsSneaking() && !pactor->IsPlayerTeammate())) &&
                            !pactor->GetGhost()) {
                            if (!pactor->IsKnockedOut()) {
                                if (pactor->pCurrentProcess->GetPickpocketed()) {
                                    const char* pMessage = gs_sNoPickPocketAgain.String();
                                    std::sprintf(msg, "%s %s", pactor->GetFullName(), pMessage);
                                    Interface::ShowMessage(
                                        msg,
                                        0,
                                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                                        nullptr,
                                        2.0f,
                                        false
                                    );
                                    return false;
                                }
                            }
                            Interface::QueueMenuCreate(
                                Interface::OPEN_CONTAINER,
                                apItemActivated,
                                nullptr,
                                nullptr,
                                Interface::CMM_PICKPOCKET,
                                false
                            );
                        }
                        else {
                            TESTopic* pgreetTopic = nullptr;
                            if (pActionActor == pPlayer || pActionActor->pCurrentProcess->GetNeedTalkPlayer()) {
                                pgreetTopic = TESTopic::GetTopic(DT_TOPIC, 0);
                            }
                            DialogueItem* pgreetDialogue = nullptr;
                            if (pgreetTopic) {
                                if (pActionActor == pPlayer) {
                                    pgreetDialogue = pgreetTopic
                                                         ->CreateDialogueItem(pactor, pPlayer, nullptr, nullptr, false);
                                }
                                else if (pActionActor->pCurrentProcess->GetNeedTalkPlayer()) {
                                    pgreetDialogue = pgreetTopic->CreateDialogueItem(
                                        pActionActor,
                                        pPlayer,
                                        nullptr,
                                        nullptr,
                                        false
                                    );
                                }
                            }
                            if (pgreetDialogue &&
                                pgreetDialogue->pInfo->IsGoodbye() &&
                                pgreetDialogue->FirstResponse() &&
                                !pgreetDialogue->NextResponse()) {
                                if (pactor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                                    pactor->pCurrentProcess->SetNeedTalkPlayer(true);
                                    pactor->InitiateGetUpPackage();
                                    return true;
                                }
                                if (pactor == pPlayer) {
                                    pActionActor->SayForceGoodbyeGreetingToPlayer(pgreetTopic, false, false);
                                }
                                else {
                                    pactor->SayForceGoodbyeGreetingToPlayer(pgreetTopic, false, false);
                                }
                            }
                            else {
                                if (pactor->pCurrentProcess->GetFurnitureMarker()) {
                                    if (pactor->pCurrentProcess->GetFurnitureMarker()->cNumber > 19u) {
                                        pactor->pCurrentProcess->AddPostAnimationAction(
                                            BaseProcess::POSTANIM_ACTION_GETOUT_OF_FURNITURE
                                        );
                                    }
                                }
                                if (pactor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
                                    NiPoint3 kActionPos = pActionActor->GetLocationOnReference();
                                    pactor->RequestRotate(kActionPos, false);
                                }
                                TESTopic* ptopic = nullptr;
                                if (pActionActor == pPlayer) {
                                    if (!pactor->IsPathingComplete() && !pactor->IsRotating()) {
                                        pactor->pCurrentProcess->EndMoveMessage(pactor);
                                    }
                                    pActionActor->StopAttack();
                                    pactor->StopAttack();
                                    DialoguePackage* ppack = dynamic_cast<DialoguePackage*>(
                                        pActionActor->GetCurrentPackage()
                                    );
                                    if (ppack) {
                                        ptopic = ppack->ptopic;
                                    }
                                    if (pactor->GetCurrentTarget() == pPlayer &&
                                        potherpackage &&
                                        potherpackage->GetPackType() == PACKAGE_TYPE_EXPLORE) {
                                        if (!potherpackage->GetPackageSchedule() ||
                                            potherpackage->GetPackageSchedule()->PSData.iDuration) {
                                            pprocess->AddToProcedureIndexRunning(pactor, 1);
                                        }
                                        else {
                                            pprocess->AddToProcedureIndexRunning(pActionActor, 2);
                                        }
                                    }

                                    BSAudio::QInstance().EndAllSoundsOfType(4u);
                                    pactor->InterruptCast();
                                    if (pactor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                                        pactor->pCurrentProcess->SetNeedTalkPlayer(true);
                                        pactor->InitiateGetUpPackage();
                                    }
                                    else {
                                        Actor* pact = pActionActor;
                                        if (pActionActor == pPlayer) {
                                            pact = pactor;
                                        }
                                        pVATS.QuitVATSPlayback(false, false);
                                        Interface::QueueMenuCreate(
                                            Interface::OPEN_DIALOG,
                                            pact,
                                            ptopic,
                                            nullptr,
                                            Interface::CMM_STANDARD,
                                            false
                                        );
                                    }
                                }
                                else if (Interface::GetMenuModeType() != Interface::eDialogMenu) {
                                    if (!pPlayer->pCurrentProcess->CanForceGreet(pActionActor)) {
                                        return false;
                                    }
                                    BSAudio::QInstance().EndAllSoundsOfType(4u);
                                    pActionActor->StopAttack();
                                    pactor->StopAttack();
                                    if (!pActionActor->IsInCombat()) {
                                        DialoguePackage* ppack = dynamic_cast<DialoguePackage*>(
                                            pActionActor->GetCurrentPackage()
                                        );
                                        if (ppack) {
                                            ptopic = ppack->ptopic;
                                        }
                                    }
                                    if (!pactionpackage->GetPackageSchedule() ||
                                        pactionpackage->GetPackageSchedule()->PSData.iDuration) {
                                        pActionActor->pCurrentProcess->AddToProcedureIndexRunning(pActionActor, 1);
                                    }
                                    else {
                                        pActionActor->pCurrentProcess->AddToProcedureIndexRunning(pActionActor, 2);
                                    }
                                    pactor->InterruptCast();
                                    if (pactor == pPlayer) {
                                        pactor->ForceIdleAnimation();
                                    }
                                    else {
                                        pactor->pCurrentProcess->AddPostAnimationAction(
                                            BaseProcess::POSTANIM_FORCEIDLE_ANIMATION
                                        );
                                    }
                                    if (pActionActor == pPlayer) {
                                        pActionActor->ForceIdleAnimation();
                                    }
                                    else {
                                        pActionActor->pCurrentProcess->AddPostAnimationAction(
                                            BaseProcess::POSTANIM_FORCEIDLE_ANIMATION
                                        );
                                    }
                                    pVATS.QuitVATSPlayback(false, false);
                                    Actor* pact = pActionActor;
                                    if (pActionActor == pPlayer) {
                                        pact = pactor;
                                    }
                                    Interface::QueueMenuCreate(
                                        Interface::OPEN_DIALOG,
                                        pact,
                                        ptopic,
                                        nullptr,
                                        Interface::CMM_STANDARD,
                                        false
                                    );
                                }
                            }
                            if (pgreetDialogue) {
                                delete pgreetDialogue;
                            }
                        }
                    }
                    else {
                        if (apObjecttoGet && !dynamic_cast<TESNPC*>(apObjecttoGet)) {
                            pactor->RemoveItem(
                                apObjecttoGet,
                                nullptr,
                                aicount,
                                true,
                                false,
                                pActionActor,
                                nullptr,
                                nullptr,
                                true,
                                false
                            );
                            pActionActor->PickpocketAlarm(pactor, apObjecttoGet, aicount);
                            return true;
                        }

                        if (pActionActor->GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_ESCORT) {
                            pActionActor->pCurrentProcess->AddToProcedureIndexRunning(pActionActor, 2);
                        }
                        else {
                            if (pActionActor->GetCurrentPackage()) {
                                if (pActionActor->GetCurrentPackage()->GetPackType() != PACKAGE_TYPE_FOLLOW) {
                                    pActionActor->pCurrentProcess->AddToProcedureIndexRunning(pActionActor, 1);
                                }
                            }
                            if (pactor != pPlayer) {
                                if (pactor != pActionActor && pactor->GetCurrentPackage()) {
                                    if (pactor->GetCurrentPackage()->GetPackType() != PACKAGE_TYPE_FOLLOW) {
                                        if (pactor->GetCurrentPackage()->GetPackType() != PACKAGE_TYPE_ESCORT) {
                                            pprocess->AddToProcedureIndexRunning(pactor, 1);
                                        }
                                    }
                                }

                                if (pactor != pActionActor && pactor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
                                    if (pActionActor->IsinDialogue()) {
                                        DialoguePackage* ppack = static_cast<DialoguePackage*>(
                                            pActionActor->GetCurrentPackage()
                                        );
                                        if (!ppack->GetDialogueNoHeadtrack() &&
                                            !ppack->GetDialogueDoNotControlTarget()) {
                                            NiPoint3 kActionPos = pActionActor->GetLocationOnReference();
                                            pactor->RequestRotate(kActionPos, false);
                                        }
                                    }
                                    else {
                                        NiPoint3 kActionPos = pActionActor->GetLocationOnReference();
                                        pactor->RequestRotate(kActionPos, false);
                                    }
                                }
                                if (!pActionActor->IsinDialogue() && pActionActor->IsNpc() && pactor->IsNpc()) {
                                    pActionActor->pCurrentProcess->AddToProcedureIndexRunning(pActionActor, 1);
                                    if (pActionActor->InitiateDialogue(
                                            pactor,
                                            nullptr,
                                            nullptr,
                                            true,
                                            false,
                                            false,
                                            nullptr,
                                            false,
                                            false
                                        )) {
                                        pActionActor->pCurrentProcess->AddToProcedureIndexRunning(pActionActor, 2);
                                        pprocess->AddToProcedureIndexRunning(pactor, 2);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else if (
                pactor->GetCombatController() &&
                pActionActor == pPlayer &&
                pactor->GetCombatController()->QSwimAllowed()
            ) {
                char apText[204];
                const char* pMessage = sActivateNPCCalmed.String();
                std::sprintf(apText, "%s %s", GetFullName(), pMessage);
                Interface::ShowMessage(apText, 0, nullptr, nullptr, 2.0f, false);
                return false;
            }
        }
        return true;
    }

    return false;
}

void TESNPC::UnClone3D(TESObjectREFR* apRequester) {
    TESObject::UnClone3D(apRequester);
}

// A leveled-template NPC whose requester has not calculated its template yet calculates it and clones the chosen
// base. Otherwise the 3D is cloned and, when it has a Bip01, the requester's biped is rooted on it (made first if
// the requester has none) and the 3D updated.
NiAVObject* TESNPC::Clone3D(TESObjectREFR* apRequester) {
    if (GetUsesLeveledTemplate() && apRequester && !apRequester->HasLeveledCreatureCalced()) {
        CalcTemplateForRef(apRequester);
        TESBoundObject* pBase = apRequester->GetObjectReference();
        return pBase->Clone3D(apRequester);
    }
    NiAVObject* p3D = nullptr;
    if (!apRequester) {
        return p3D;
    }
    BipedAnim* pBipedAnim = apRequester->GetBiped();
    p3D = TESBoundObject::Clone3D(apRequester, false);
    if (pBipedAnim) {
        if (BSUtilities::GetObjectByName(p3D, "Bip01")) {
            pBipedAnim->InitRoot(static_cast<NiNode*>(p3D));
            p3D->UpdateProperties();
            NiUpdateData kUpdateData(0.0f, false, false);
            p3D->Update(kUpdateData);
        }
    }
    else if (BSUtilities::GetObjectByName(p3D, "Bip01")) {
        pBipedAnim = new BipedAnim(apRequester, static_cast<NiNode*>(p3D));
        apRequester->SetBiped(pBipedAnim);
        p3D->UpdateProperties();
        NiUpdateData kUpdateData(0.0f, false, false);
        p3D->Update(kUpdateData);
    }
    return p3D;
}

TESCombatStyle* TESNPC::GetCombatStyle() const {
    return pCombatStyle;
}

void TESNPC::SetCombatStyle(TESCombatStyle* apCombatStyle) {
    pCombatStyle = apCombatStyle;
}

void TESNPC::SetActorValue(ActorValue::Index aiWhich, int32_t aiValue) {
    if (ActorValue::IsSkill(aiWhich)) {
        data.cSkillValues[ActorValue::ToArrayIndex(ActorValue::SKILL, aiWhich)] = aiValue;
        AddChange(CHANGE_NPC_SKILLS);
    }
    else {
        TESActorBase::SetActorValue(aiWhich, aiValue);
    }
}

// Copies the parts the template use flags select from the template: apForceTemplate when it is an NPC, else this base's
// template unless that is leveled, else a default NPC built on the stack with the first race and class. A template that
// has not copied from its own template does so first. The base disposition is read back from this NPC, not the
// template, and each CopyComponent is handed the template's component (null-checked).
void TESNPC::CopyFromTemplateForm(TESActorBase* apForceTemplate) {
    if (!QTemplateForm() && !apForceTemplate) {
        return;
    }
    alignas(TESNPC) uint8_t aDefaultNPCStorage[sizeof(TESNPC)];
    TESNPC* defaultnpc = ::new (aDefaultNPCStorage) TESNPC;
    TESNPC* pactor = nullptr;
    if (!IsTemplateLeveled()) {
        pactor = static_cast<TESNPC*>(QTemplateForm());
    }
    if (apForceTemplate && apForceTemplate->GetFormType() == NPC__ID) {
        pactor = static_cast<TESNPC*>(apForceTemplate);
    }
    if (!pactor) {
        pactor = defaultnpc;
        pactor->SetFormRace(pDataHandler->GetRaceList()->GetItem());
        pactor->SetClass(static_cast<TESClass*>(pDataHandler->GetClassList()->GetItem()));
    }
    else if (pactor->QTemplateForm() && !pactor->QCopiedTemplate()) {
        TESActorBaseData* ptemplate = pactor;
        ptemplate->CopyFromTemplateForm(nullptr);
    }
    if (GetTemplateUseFlag(TU_SCRIPT)) {
        TESScriptableForm::CopyComponent(static_cast<TESScriptableForm*>(pactor));
    }
    if (GetTemplateUseFlag(TU_INVENTORY)) {
        TESContainer::CopyComponent(static_cast<TESContainer*>(pactor));
    }
    if (GetTemplateUseFlag(TU_BASEDATA)) {
        TESFullName::CopyComponent(static_cast<TESFullName*>(pactor));
        uint32_t iFlags = *GetFlags() &
            ~(BASE_ESSENTIAL | BASE_RESPAWN | BASE_NO_LOWLEVEL_PROCESSING | BASE_NO_RUMORS | BASE_CREATURE_NO_HEAD |
              BASE_CREATURE_ALLOW_PC_DIALOGUE | BASE_CAN_BE_ALL_RACES | BASE_CREATURE_ALLOW_PICKPOCKET);
        uint32_t iCopyFlags = *pactor->GetFlags() &
            (BASE_ESSENTIAL | BASE_RESPAWN | BASE_NO_LOWLEVEL_PROCESSING | BASE_NO_RUMORS | BASE_CREATURE_NO_HEAD |
             BASE_CREATURE_ALLOW_PC_DIALOGUE | BASE_CAN_BE_ALL_RACES | BASE_CREATURE_ALLOW_PICKPOCKET);
        TESActorBaseData::SetFlags(iCopyFlags | iFlags);
    }
    if (GetTemplateUseFlag(TU_MODEL)) {
        TESAnimation::CopyComponent(static_cast<TESAnimation*>(pactor));
        CopyBoundData(pactor);
        TESActorBaseData* pCopyData = pactor;
        TESActorBaseData* pThisData = this;
        pThisData->SetBloodImpactMaterial(pCopyData->GetBloodImpactMaterial());
        CopyHeadData(pactor);
        BGSDestructibleObjectForm::CopyComponent(static_cast<BGSDestructibleObjectForm*>(pactor));
    }
    if (GetTemplateUseFlag(TU_TRAITS)) {
        SetDeathItem(pactor->GetDeathItem());
        TESActorBaseData* pCopyData = pactor;
        SetVoiceType(pCopyData->GetVoiceType());
        SetFlagBit(BASE_FEMALE, pactor->GetFlag(BASE_FEMALE), true);
        SetFormRace(pactor->GetFormRace());
        SetHeight(pactor->GetHeight());
        SetWeight(pactor->GetWeight());
        SetClass(pactor->GetClass());
        SetKarma(pCopyData->GetKarma());
        SetBaseDisposition(GetBaseDisposition());
        SetCombatStyle(pactor->GetCombatStyle());
    }
    if (GetTemplateUseFlag(TU_STATS)) {
        TESActorBaseData* pCopyData = pactor;
        SetLevel(pactor->GetLevelExact());
        SetMatchesPCLevel(pactor->GetMatchesPCLevel());
        SetCalcLevelMin(pactor->GetCalcMin());
        SetCalcLevelMax(pactor->GetCalcLevelMax());
        TESAttributes::CopyComponent(static_cast<TESAttributes*>(pactor));
        TESHealthForm::CopyComponent(static_cast<TESHealthForm*>(pactor));
        SetSpeedMult(pactor->GetSpeedMultiplier());
        SetFatigue(pCopyData->GetFatigue());
        ourMemcpy(data.cSkillValues, pactor->data.cSkillValues, sizeof(data.cSkillValues));
        ourMemcpy(data.cSkillOffsets, pactor->data.cSkillOffsets, sizeof(data.cSkillOffsets));
        if (GetMatchesPCLevel()) {
            SetAutoCalc(true);
        }
        else {
            SetAutoCalc(pactor->IsAutoCalc());
        }
    }
    if (GetTemplateUseFlag(TU_FACTIONS)) {
        CopyFactionList(static_cast<TESActorBaseData*>(pactor));
    }
    if (GetTemplateUseFlag(TU_SPELLS)) {
        TESSpellList::CopyComponent(static_cast<TESSpellList*>(pactor));
        BGSTouchSpellForm::CopyComponent(static_cast<BGSTouchSpellForm*>(pactor));
    }
    if (GetTemplateUseFlag(TU_AIDATA)) {
        CopyAIData(static_cast<TESAIForm*>(pactor));
    }
    if (GetTemplateUseFlag(TU_AIPACKAGES)) {
        CopyAIPackageList(static_cast<TESAIForm*>(pactor));
    }
    if (IsAutoCalc()) {
        InitValues(false);
    }
    uint16_t scompositeflags = 0;
    TESForm* pleveledtemplate = GetLeveledTemplateInHierarchy(this, scompositeflags);
    if (pleveledtemplate) {
        SetTemplateForm(pleveledtemplate);
        SetTemplateUseFlags(scompositeflags);
    }
    SetCopiedTemplate(true);
    defaultnpc->~TESNPC();
}

bool TESNPC::GetNoPersuasion() const {
    return GetFlag(BASE_CREATURE_NO_HEAD);
}

bool TESNPC::GetNoBloodSpray() const {
    return GetFlag(BASE_NO_BLOOD_SPRAY);
}

void TESNPC::SetNoBloodSpray(bool abVal) {
    SetFlagBit(BASE_NO_BLOOD_SPRAY, abVal, true);
}

bool TESNPC::GetNoBloodDecal() const {
    return GetFlag(BASE_NO_BLOOD_DECAL);
}

void TESNPC::SetNoBloodDecal(bool abVal) {
    SetFlagBit(BASE_NO_BLOOD_DECAL, abVal, true);
}

BGSVoiceType* TESNPC::GetVoiceType() const {
    TESNPC* pNPC = const_cast<TESNPC*>(this);
    if (this == pPlayer->GetObjectReference()) {
        if (GetSex()) {
            if (pPlayer->IsPCYoung()) {
                pNPC->pVoiceType = static_cast<BGSVoiceType*>(
                    BGSDefaultObjectManager::GetDefaultObject(DO_PLAYERVOICE_FEMALE_CHILD)
                );
            }
            else {
                pNPC->pVoiceType = static_cast<BGSVoiceType*>(
                    BGSDefaultObjectManager::GetDefaultObject(DO_PLAYERVOICE_FEMALE)
                );
            }
        }
        else if (pPlayer->IsPCYoung()) {
            pNPC->pVoiceType = static_cast<BGSVoiceType*>(
                BGSDefaultObjectManager::GetDefaultObject(DO_PLAYERVOICE_MALE_CHILD)
            );
        }
        else {
            pNPC->pVoiceType = static_cast<BGSVoiceType*>(
                BGSDefaultObjectManager::GetDefaultObject(DO_PLAYERVOICE_MALE)
            );
        }
    }
    if (pVoiceType) {
        return pVoiceType;
    }
    TESRace* pRace = GetFormRace();
    if (!pRace) {
        return nullptr;
    }
    return pRace->GetDefaultVoiceType(GetSex());
}

uint32_t TESNPC::GetFormHealth() const {
    int32_t iResult = static_cast<int32_t>(TESHealthForm::GetFormHealth());
    if (iResult < 0) {
        iResult = 0;
    }
    return static_cast<uint32_t>(iResult);
}

const char* TESNPC::GetModel() {
    if (GetSex() == SEX_FEMALE) {
        return sNPCModelFemale.String();
    }
    return sNPCModelMale.String();
}

int32_t TESNPC::GetActorValue(ActorValue::Index aiWhich) {
    if (!ActorValue::IsSkill(aiWhich)) {
        return TESActorBase::GetActorValue(aiWhich);
    }
    int32_t i = ActorValue::ToArrayIndex(ActorValue::SKILL, aiWhich);
    int32_t iresult = data.cSkillValues[i];
    if (!IsAutoCalc()) {
        iresult += data.cSkillOffsets[i];
    }
    return iresult;
}

void TESNPC::SwapEyes(TESEyes* apSwapToEyes) {
    NiPointer<NiTriBasedGeom> spLeftEye(static_cast<NiTriBasedGeom*>(nullptr));
    NiPointer<NiTriBasedGeom> spRightEye(static_cast<NiTriBasedGeom*>(nullptr));
    if (spHeadBiped.m_pObject) {
        NiAVObject* pobj;
        uint32_t isizeArray = spHeadBiped->GetArrayCount();
        for (uint32_t ichild = 0; ichild < isizeArray; ++ichild) {
            pobj = spHeadBiped->GetAt(ichild);
            if (!BSstrcmp(pobj->m_kName.GetHandleString(), "FaceGenEyeLeft")) {
                spLeftEye = pobj->IsTriBasedGeom();
            }
            if (!BSstrcmp(pobj->m_kName.GetHandleString(), "FaceGenEyeRight")) {
                spRightEye = pobj->IsTriBasedGeom();
            }
        }
        if (spLeftEye.m_pObject && spRightEye.m_pObject) {
            BSStringT<char> filename;
            if (apSwapToEyes) {
                filename.SPrintF("Data\\Textures\\%s", apSwapToEyes->GetTextureName());
            }
            else {
                filename.SPrintF("Data\\Textures\\Characters\\Eyes\\EyeDefault.dds");
            }
            NiPointer<NiSourceTexture> sptexBase(static_cast<NiSourceTexture*>(nullptr));
            TES::pInstance->CreateTextureImage(filename.pString, sptexBase, false, false);
            if (sptexBase.m_pObject) {
                NiTexturingProperty* ptexprop;
                NiPropertyState* pstateleft = &spLeftEye->m_PropertyState;
                NiPropertyState* pstateright = &spRightEye->m_PropertyState;
                if (pstateleft && pstateright) {
                    NiShadeProperty* pshadeleft = pstateleft->GetShade();
                    if (pshadeleft && pshadeleft->iShaderPropertyType >= 8) {}

                    int32_t iLeftLighting = 0;
                    if (pshadeleft) {
                        iLeftLighting = pshadeleft->iShaderPropertyType >= 8 && pshadeleft->iShaderPropertyType <= 12;
                    }
                    BSShaderPPLightingProperty* pshaderpropLightingLeft = iLeftLighting
                        ? static_cast<BSShaderPPLightingProperty*>(pshadeleft)
                        : nullptr;
                    NiShadeProperty* pshaderight = pstateright->GetShade();
                    int32_t iRightLighting = 0;
                    if (pshaderight) {
                        iRightLighting = pshaderight->iShaderPropertyType >= 8 &&
                            pshaderight->iShaderPropertyType <= 12;
                    }
                    BSShaderPPLightingProperty* pshaderpropLightingRight = iRightLighting
                        ? static_cast<BSShaderPPLightingProperty*>(pshaderight)
                        : nullptr;
                    if (pshaderpropLightingLeft && pshaderpropLightingRight) {
                        pshaderpropLightingLeft->SetDiffuseTexture(0, sptexBase.m_pObject);
                        pshaderpropLightingRight->SetDiffuseTexture(0, sptexBase.m_pObject);
                        return;
                    }
                }

                ptexprop = new NiTexturingProperty;
                ptexprop->SetBaseTexture(sptexBase.m_pObject);
                ptexprop->SetBaseClampMode(NiTexturingProperty::WRAP_S_WRAP_T);
                ptexprop->SetBaseFilterMode(NiTexturingProperty::FILTER_TRILERP);
                if (spLeftEye->GetProperty(5)) {
                    spLeftEye->RemoveProperty(5);
                }
                spLeftEye->AttachProperty(ptexprop);
                if (spRightEye->GetProperty(5)) {
                    spRightEye->RemoveProperty(5);
                }
                spRightEye->AttachProperty(ptexprop);
            }
        }
        pobj = spHeadBiped->GetObjectByName("FaceGenEyeLeft");
        if (pobj) {
            BSShaderManager::PrepareObject(pobj, 0, 0);
        }
        pobj = spHeadBiped->GetObjectByName("FaceGenEyeRight");
        if (pobj) {
            BSShaderManager::PrepareObject(pobj, 0, 0);
        }
    }
}

TESClass* TESNPC::GetClass() const {
    return pCl;
}

TESNPC* TESNPC::GetFaceNPC() const {
    return pFaceNPC;
}

float TESNPC::GetHeight() const {
    return fHeight;
}

TESRace* TESNPC::GetRace() const {
    return pFormRace;
}

// The face-gen offsets: the alternate set when there is one, else the race's.
FR2MatrixVTC<float> (*TESNPC::GetOffsetFaceCoord())[2][2] {
    if (pAlternateFaceOffsetCoord) {
        return pAlternateFaceOffsetCoord;
    }

    return &RaceFaceOffsetCoord;
}

BSSimpleList<void*>* TESNPC::GetHeadParts() {
    return &listHeadParts;
}

TESNPC_Data* TESNPC::GetSkills() {
    return &data;
}

float TESNPC::GetHairLength() const {
    return fHairLength;
}

TESHair* TESNPC::GetHair() {
    TESHair* pResult = pHair;
    if (pOriginalRace && this != pPlayer->GetObjectReference()) {
        int32_t iSex = GetSex();
        pResult = GetFormRace()->GetDefaultHair(static_cast<TESActorBase::SEX>(iSex));
    }
    return pResult;
}

// Returns the race helper's value, or 0 when there is no race; the sex is read before the second race lookup.
float TESNPC::GetRaceHeight() {
    if (!GetRace()) {
        return 0.0f;
    }
    int32_t iSex = GetSex();
    return GetRace()->GetHeight(static_cast<TESActorBase::SEX>(iSex));
}

void TESNPC::ClearHead() {
    spHeadSkinned = spHeadBiped = nullptr;
}

void TESNPC::InitHead(BSFaceGenNiNode*& apOutNodeBiped, BSFaceGenNiNode*& apOutNodeSkinned) {
    spHeadBiped = static_cast<BSFaceGenNiNode*>(nullptr);
    spHeadSkinned = static_cast<BSFaceGenNiNode*>(nullptr);
    if (!spHeadBiped.m_pObject && !spHeadSkinned.m_pObject && GetFormRace()) {
        GetFormRace()->CreateHead(apOutNodeBiped, apOutNodeSkinned, this, true, false, TESActorBase::SEX_MALE);
        sLastRaceFaceNum = GetFormRace()->GetFaceCoordNum();
    }
}

void TESNPC::AttachHead(
    TESObjectREFR* apRef,
    BipedAnim* apBipedAnim,
    BSFaceGenNiNode* apOutNodeBiped,
    BSFaceGenNiNode* apOutNodeSkinned
) {
    BSFaceGenAnimationData* pAnimData = nullptr;
    NiNode* p3D = apRef->TESObjectREFR::Get3D();
    NiNode* pRoot = nullptr;
    if (p3D) {
        pRoot = p3D->IsNode();
    }
    NiNode* pHeadBone = apBipedAnim->GetParentBone(BIPED_BONE_HEAD);
    NiMatrix3 kRot;
    NiDefaultAVObjectPalette* pPalette = nullptr;
    if (apRef->GetAnimation() && apRef->GetAnimation()->GetControllerManager()) {
        pPalette = static_cast<NiDefaultAVObjectPalette*>(
            apRef->GetAnimation()->GetControllerManager()->GetObjectPalette()
        );
    }
    if (pHeadBone && pRoot) {
        if (apOutNodeBiped) {
            if (apOutNodeBiped->GetAnimationData() && apRef->GetHealth() <= 0.0) {
                apOutNodeBiped->GetAnimationData()->SetDead(true, true);
                apOutNodeBiped->GetAnimationData()->SetMustUpdate(true);
            }
            apOutNodeBiped->SetAnimationUpdate(true);
            apOutNodeBiped->SetApplyRotToParent(true);
            apOutNodeBiped->SetTranslate(NiPoint3::ZERO);
            apOutNodeBiped->SetRotate(NiMatrix3::IDENTITY);
            pHeadBone->AttachChild(apOutNodeBiped, true);
            RecurseAndAddObjectsToPalette(apOutNodeBiped, pPalette);
        }
        if (apOutNodeSkinned) {
            kRot = NiMatrix3::IDENTITY;
            apOutNodeSkinned->SetAnimationUpdate(apOutNodeBiped == nullptr);
            apOutNodeSkinned->SetApplyRotToParent(apOutNodeBiped == nullptr);
            apOutNodeSkinned->SetTranslate(NiPoint3::ZERO);
            pRoot->AttachChild(apOutNodeSkinned, true);
            RecurseAndAddObjectsToPalette(apOutNodeSkinned, pPalette);
            apOutNodeSkinned->FixSkinInstances(pRoot, true);
        }
        pAnimData = apOutNodeBiped ? apOutNodeBiped->GetAnimationData() : nullptr;
        if (!pAnimData) {
            pAnimData = apOutNodeSkinned ? apOutNodeSkinned->GetAnimationData() : nullptr;
        }
        if (pAnimData) {
            pAnimData->ResetCurrentMorphs(0.0f, true, true, true, true, false);
            if (apRef->IsActor()) {
                Actor* pActor = static_cast<Actor*>(apRef);
                if (pActor->pRagdollController) {
                    float fMin;
                    float fHeading;
                    float fPitch;
                    BSFaceGenAnimationData::GetTrackEyeXYMinMax(fMin, fHeading);
                    BSFaceGenAnimationData::GetTrackEyeZMinMax(fMin, fPitch);
                    pActor->pRagdollController->SetMaxEyePitch(fPitch);
                    pActor->pRagdollController->SetLookAtMaxAngle(bhkRagdollController::EYE_BONE, fHeading);
                    BSFaceGenAnimationData::GetTrackDeadZoneXYMinMax(fMin, fHeading);
                    BSFaceGenAnimationData::GetTrackDeadZoneZMinMax(fMin, fPitch);
                    pActor->pRagdollController->SetLookIKDeadZone(bhkRagdollController::EYE_BONE, fHeading, fPitch);
                }
            }
        }
        spHeadBiped = apOutNodeBiped;
        spHeadSkinned = apOutNodeSkinned;
        pRoot->UpdateProperties();
        NiUpdateData kUpdateData(0.0f, false, false);
        pRoot->Update(kUpdateData);
    }
    else {
        Warning("MODELS: Cannot create a head for an NPC (%d) that does not have a biped-head node.", GetFormID());
    }
    FixDisplayedHeadParts(apRef, nullptr);
}

float TESNPC::GetWeight() {
    if (GetFormID() != 7) {
        return GetHeight();
    }
    float fResult;
    if (fWeight == 0.0 && GetRace()) {
        int32_t iSex = GetSex();
        fResult = GetRace()->GetWeight(static_cast<TESActorBase::SEX>(iSex));
    }
    else {
        fResult = fWeight;
    }
    return fResult;
}

// It takes the offsets, then the sex, then the race's mean face.
void TESNPC::GetFaceCoord(FR2MatrixVTC<float> (&aOutCoord)[2][2]) {
    BSFaceGenManager::InitFaceGenCoord(&aOutCoord);
    if (!GetFormRace()) {
        BSFaceGenManager::CopyFaceGenCoord(BSFaceGenManager::GetFaceGenCoord(), &aOutCoord, false, false);
        return;
    }
    FR2MatrixVTC<float>(*pOffsetFaceCoord)[2][2] = GetOffsetFaceCoord();
    TESActorBase::SEX eSex = GetSex();
    FR2MatrixVTC<float>(*pMeanFaceCoord)[2][2] = GetFormRace()->GetMeanFaceCoord(eSex);
    BSFaceGenManager::MergeFaceGenCoord(pMeanFaceCoord, pOffsetFaceCoord, &aOutCoord, false, 0.0f);
}

// Through the template chain the NPC takes its textures from; otherwise the texture is named after the owning master
// (FalloutNV.ESM for an update file) and the NPC's form ID, with the race's form ID and an M/F prefix for an NPC that
// can be any race. Without an owning master there is no file name. The master's name is re-read for the format after
// the "update" test. The function calls itself for the face NPC; disable_tail_calls keeps clang from turning that
// recursion into a loop.
[[clang::disable_tail_calls]] bool
TESNPC::GetHeadPartModTextureFileName(RACE_HEAD_PART aePart, char* apFileName, uint32_t aiSize) {
    if (pFaceNPC) {
        TESNPC* pfacenpc = pFaceNPC;
        while (pfacenpc && pfacenpc->GetFaceNPC()) {
            pfacenpc = pfacenpc->GetFaceNPC();
        }
        return pfacenpc->GetHeadPartModTextureFileName(aePart, apFileName, aiSize);
    }
    TESFile* pmaster = GetOwnerMaster();
    if (!pmaster) {
        return false;
    }
    uint32_t imyID = GetFormID() & 0xFFFFFF;
    uint32_t iraceID = GetRace()->GetFormID() & 0xFFFFFF;
    bool bUpdate = false;
    if (!BSstrnicmp(pmaster->GetFilename(), "update", 6)) {
        bUpdate = true;
    }
    if (GetCanBeAllRaces()) {
        if (GetSex() == 1) {
            BSsprintf(
                apFileName,
                aiSize,
                "data\\Textures\\Characters\\FaceMods\\%s\\F%08X_%08X_%i.dds",
                bUpdate ? "FalloutNV.ESM" : pmaster->GetFilename(),
                iraceID,
                imyID,
                aePart
            );
        }
        else {
            BSsprintf(
                apFileName,
                aiSize,
                "data\\Textures\\Characters\\FaceMods\\%s\\M%08X_%08X_%i.dds",
                bUpdate ? "FalloutNV.ESM" : pmaster->GetFilename(),
                iraceID,
                imyID,
                aePart
            );
        }
    }
    else {
        BSsprintf(
            apFileName,
            aiSize,
            "data\\Textures\\Characters\\FaceMods\\%s\\%08X_%i.dds",
            bUpdate ? "FalloutNV.ESM" : pmaster->GetFilename(),
            imyID,
            aePart
        );
    }
    return true;
}

void TESNPC::GetHeadPartModTexture(RACE_HEAD_PART aePart, NiPointer<NiTexture>& aspTexture) {
    char acFileName[0x104];
    if (BSFaceGenManager::bLoadFaceGenHeadEGTFiles.Bool() || this == pPlayer->GetObjectReference()) {
        aspTexture = nullptr;
    }
    else if (aePart >= 1) {
        NiSourceTexture* pTexture = static_cast<NiSourceTexture*>(BSFaceGenManager::GetDefaultBaseModulationTexture());
        aspTexture = pTexture;
    }
    else if (!GetHeadPartModTextureFileName(aePart, acFileName, sizeof(acFileName))) {
        aspTexture = nullptr;
    }
    else {
        NiPointer<NiSourceTexture> spTemp(nullptr);
        TES::pInstance->CreateTextureImage(acFileName, spTemp, true, false);
        aspTexture = spTemp;
    }
}

// Gives apRef a biped with no root yet, under the animation heap context; a reference that already has one keeps it,
// with a warning.
BipedAnim* TESNPC::CreateBipedAnim(TESObjectREFR* apRef) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_ANIMATION,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESNPC.cpp",
        5585
    );
    BipedAnim* pBipedAnim = apRef->GetBiped();
    if (pBipedAnim) {
        Warning(
            "ANIMATION: This npc \"%s\" has already been used.\r\nOnly the first reference will be used.\r\n",
            GetFormEditorID()
        );
    }
    else {
        pBipedAnim = new BipedAnim(apRef, nullptr);
        apRef->SetBiped(pBipedAnim);
    }
    pContext->~AutoMemContext();
    return pBipedAnim;
}

TESEyes* TESNPC::GetEyeColor() const {
    return pEyeColor;
}

uint32_t TESNPC::GetHairColor() {
    return iHairColor;
}

void TESNPC::CopyHeadData(TESNPC* apNPC) {
    GetHeadParts()->RemoveAll();
    pHair = apNPC->GetHair();
    fHairLength = apNPC->GetHairLength();
    pEyeColor = apNPC->GetEyeColor();
    iHairColor = apNPC->GetHairColor();
    for (BSSimpleList<void*>* pIter = apNPC->GetHeadParts(); pIter && !pIter->IsEmpty(); pIter = pIter->GetNext()) {
        void*& rItem = pIter->GetItem();
        GetHeadParts()->AddTail(rItem);
    }
    // Takes this NPC's offsets before apNPC's.
    FR2MatrixVTC<float>(*pCopyTo)[2][2] = GetOffsetFaceCoord();
    FR2MatrixVTC<float>(*pCopyFrom)[2][2] = apNPC->GetOffsetFaceCoord();
    BSFaceGenManager::CopyFaceGenCoord(pCopyFrom, pCopyTo, false, false);
    if (!pPlayer || this != pPlayer->GetObjectReference()) {
        SetFaceNPC(apNPC);
    }
}

void TESNPC::SetFaceNPC(TESNPC* apNPC) {
    pFaceNPC = apNPC;
}

void TESNPC::SetHairLength(float afHairLength) {
    fHairLength = afHairLength;
}

void TESNPC::SetHair(TESHair* apHair) {
    pHair = apHair;
}

void TESNPC::SetEyeColor(TESEyes* apEyeColor) {
    pEyeColor = apEyeColor;
}

void TESNPC::SetWeight(float afWeight) {
    fWeight = afWeight;
}

void TESNPC::SetHeight(float afHeight) {
    fHeight = afHeight;
}

void TESNPC::SetClass(TESClass* apClass) {
    pCl = apClass;
    AddChange(CHANGE_NPC_CLASS);
}

// The auto-calc stats of an NPC with a race and a class: the class's attributes (capped at 10) unless this is the
// player with the character-generation class, then each skill from its primary attribute and luck, the tag skills
// sharing the level's skill points (whatever a tag skill gets past 100 carries to the next one), and the class's
// service flags. For both caps, only a greater value falls through.
void TESNPC::InitValues(bool abPC) {
    float fvalue = 0.0f;
    if (!GetFormRace() || !pCl) {
        return;
    }
    bool isPC = false;
    if (GetFormID() == 7) {
        isPC = true;
    }
    bool bUseclass = true;
    if (isPC) {
        const int32_t& iCharGenClassID = iClassCharactergenClass.UInt();
        if (static_cast<uint32_t>(iCharGenClassID) == GetClass()->GetFormID()) {
            bUseclass = false;
        }
    }
    if (!IsAutoCalc()) {
        return;
    }
    for (int32_t iattribute = 0; iattribute < 7; ++iattribute) {
        int32_t eattribute = ActorValue::ToActorValue(ActorValue::ATTRIBUTE, static_cast<int8_t>(iattribute));
        if (bUseclass) {
            fvalue = GetClass()->GetAttributeValue(static_cast<ActorValue::Index>(iattribute + ActorValue::STRENGTH));
        }
        if (fvalue > 10.0) {
            fvalue = 10.0f;
        }
        SetAttributeValue(static_cast<ActorValue::Index>(eattribute), static_cast<uint8_t>(FloatToInt(fvalue)), false);
    }
    uint8_t ucluck = GetAttributeValue(ActorValue::LUCK);
    int32_t ilevelBonus = GamePlayFormulas::GetLevelUpSkillPoints(GetAttributeValue(ActorValue::INTELLIGENCE), 1);
    int32_t ilevel = GetLevel();
    ilevelBonus = (ilevel - 1) * ilevelBonus;
    int32_t itagSkillCount = abPC + 3;
    float fperTagSkill = static_cast<float>(static_cast<double>(ilevelBonus) / itagSkillCount);
    double dfloor = FloatFloor(fperTagSkill);
    float ftagSkillBonus = static_cast<float>(static_cast<int32_t>(iSkillPointsTagSkillMult.UInt()) * dfloor);
    float fextraBonus = 0.0f;
    --itagSkillCount;
    for (int32_t iskill = 0; iskill < 14; ++iskill) {
        ActorValue::Index eskill = static_cast<ActorValue::Index>(
            ActorValue::ToActorValue(ActorValue::SKILL, static_cast<int8_t>(iskill))
        );
        // Tests only the low byte of IsFlagSet's result.
        if (static_cast<uint8_t>(ActorValue::IsFlagSet(eskill, ActorValue::DEPRECATED)) ||
            !ActorValue::IsSkill(eskill)) {
            continue;
        }
        fvalue = 0.0f;
        ActorValue::Index eprimaryAttribute = ActorValue::ATTRIBUTE_NONE;
        AiFormulas::AVGetDerivedSkillPrimaryAttributeAndBase(eskill, eprimaryAttribute, fvalue);
        fvalue = AiFormulas::CalculateDerivedSkillValue(
            fvalue,
            cAttribute[eprimaryAttribute - ActorValue::STRENGTH],
            ucluck
        );
        if (bUseclass && pCl && pCl->IsTagSkill(eskill)) {
            double dbonus = static_cast<double>(ftagSkillBonus) + fextraBonus;
            fvalue = static_cast<float>(fAVDTagSkillBonus.Float() + dbonus + fvalue);
            fextraBonus = 0.0f;
            if (fvalue > 100.0) {
                fextraBonus = static_cast<float>(fvalue - 100.0);
                if (itagSkillCount) {
                    fextraBonus = static_cast<float>(fextraBonus / static_cast<double>(itagSkillCount));
                }
            }
            --itagSkillCount;
        }
        if (fvalue > 100.0) {
            fvalue = 100.0f;
        }
        data.cSkillValues[iskill] = static_cast<uint8_t>(FloatToInt(fvalue));
    }
    if (pCl) {
        if (GetAutoCalcServiceFlags()) {
            TESAIForm::SetServiceFlags(pCl->data.iServiceFlags, false);
        }
    }
    else {
        TESAIForm::SetServiceFlags(0, false);
    }
    AddChange(CHANGE_ACTOR_BASE_ATTRIBUTES | CHANGE_ACTOR_BASE_AIDATA | CHANGE_NPC_SKILLS);
}

// Strips apRef and its process's equipment, then wears the best armor per slot (skipping slots the chosen upper body
// model fills, and any other item that also covers the upper body) and, for a living actor with abWeapon, equips the
// best weapon or makes the worn one current. abArmor and abOnlyifClothes are not read. Without abAllowChanges, the save
// game's thread change flag is off meanwhile. apRef is treated as an actor without a cast, and the weapon change is
// freed only when it is equipped without extra data.
void TESNPC::InitDefaultWorn(
    TESObjectREFR* apRef,
    [[maybe_unused]] bool abArmor,
    bool abWeapon,
    [[maybe_unused]] bool abOnlyifClothes,
    bool abAllowChanges
) {
    bool brestoreAllowChanges = true;
    if (!abAllowChanges) {
        brestoreAllowChanges = pSaveLoadGame->SetThreadAllowChanges(false);
    }
    if (apRef->IsActor()) {
        apRef->RemoveWeapon();
    }
    InventoryChanges* pinv = InventoryChanges::GetInventoryChanges(apRef);
    pinv->RemoveAllObjectsWorn(true, static_cast<Actor*>(apRef));
    Actor* pactor = static_cast<Actor*>(apRef);
    if (pactor->GetCurrentAIProcess()) {
        pactor->GetCurrentAIProcess()->SetCurrentAmmo(nullptr);
        pactor->GetCurrentAIProcess()->SetCurrentShield(nullptr);
        pactor->GetCurrentAIProcess()->SetCurrentTorch(nullptr);
        pactor->GetCurrentAIProcess()->SetCurrentWeapon(nullptr, nullptr, false);
    }
    TESBipedModelForm* pupperModel = nullptr;
    ExtraDataList* pextra = nullptr;
    for (int32_t n = BO_HEAD; n < BIPED_OBJECT_COUNT; ++n) {
        ItemChange* pitemChange = pinv->GetBestArmor(this, static_cast<BIPED_OBJECT>(n), true);
        if (!pitemChange) {
            continue;
        }
        bool bwear = true;
        TESBoundObject* pobj = pitemChange->GetContainerObject();
        if ((!pupperModel || !pupperModel->FillsBipedSlot(static_cast<BIPED_OBJECT>(n), false, nullptr)) &&
            !apRef->WearingObject(pobj)) {
            if (pitemChange->GetExtraDataList()) {
                pextra = pitemChange->GetExtraDataList()->GetItem();
            }
            else {
                pextra = nullptr;
            }
            if (n == BO_UPPER_BODY) {
                pupperModel = dynamic_cast<TESBipedModelForm*>(pobj);
            }
            else {
                TESBipedModelForm* pmodel = TESBipedModelForm::GetFormAsBipedModel(pobj);
                if (pmodel->FillsBipedSlot(BO_UPPER_BODY, false, nullptr)) {
                    bwear = false;
                }
            }
            if (bwear) {
                apRef->AddWornItem(pobj, 1, pextra, false);
            }
        }
        if (pitemChange) {
            delete pitemChange;
        }
    }
    if (abWeapon && !apRef->IsDead(false)) {
        float fval = 0.0f;
        ItemChange* pweapon = pinv->GetBestWeapon(this, fval, COMBAT_WEAPON_TYPE_COUNT, true);
        if (pweapon) {
            bool bdelete = true;
            if (!apRef->WearingObject(pweapon->GetContainerObject())) {
                ExtraDataList* pextraWeapon = nullptr;
                if (pweapon->GetExtraDataList()) {
                    pextraWeapon = pweapon->GetExtraDataList()->GetItem();
                    bdelete = false;
                }
                else {
                    pextraWeapon = nullptr;
                }
                Actor* pact = static_cast<Actor*>(apRef);
                pact->EquipObject(pweapon->GetContainerObject(), pweapon->GetCount(), pextraWeapon, true, false, true);
                if (bdelete && pweapon) {
                    delete pweapon;
                }
            }
            else if (static_cast<Actor*>(apRef)->GetCurrentAIProcess()) {
                TESBoundObject* pobj = pweapon->GetContainerObject();
                if (pobj != static_cast<Actor*>(apRef)->GetCurrentWeapon()) {
                    BaseProcess* pprocess = static_cast<Actor*>(apRef)->GetCurrentAIProcess();
                    pprocess->SetCurrentWeapon(pweapon, apRef->Get3D(), false);
                }
            }
        }
    }
    if (!abAllowChanges) {
        pSaveLoadGame->SetThreadAllowChanges(brestoreAllowChanges);
    }
    if (pactor->GetCurrentAIProcess()) {
        pactor->CastPermanentMagic(true);
    }
}

// The left hand (3) and the weapon (5) are skipped in order and done last, as slots 20 and 21.
void TESNPC::InitWorn(TESObjectREFR* apRef, BipedAnim* apBipedAnim) {
    InventoryChanges* pinv = InventoryChanges::GetInventoryChanges(apRef);
    if (!apRef || !apBipedAnim) {
        return;
    }
    for (int32_t ePart = BO_HEAD; ePart <= BIPED_OBJECT_COUNT + 1; ++ePart) {
        if (ePart == BO_LEFT_HAND || ePart == BO_WEAPON) {
            ++ePart;
        }
        else if (ePart == BIPED_OBJECT_COUNT) {
            ePart = BO_LEFT_HAND;
        }
        else if (ePart == BIPED_OBJECT_COUNT + 1) {
            ePart = BO_WEAPON;
        }
        ItemChange* pitem = pinv->GetWornItem(static_cast<BIPED_OBJECT>(ePart), false);
        if (pitem) {
            TESBoundObject* pobj = pitem->GetContainerObject();
            delete pitem;
            if (pobj && !InitWornObject(apRef, apBipedAnim, pobj)) {
                Warning(
                    "MODELS: Need to add support for BipedObject '%s' object name '%s' in "
                    "TESNPC::InitWornObject.",
                    bipedObjectName[ePart],
                    TESFullName::GetFullName(pobj)
                );
            }
        }
        if (ePart == BO_LEFT_HAND) {
            ePart = BIPED_OBJECT_COUNT;
        }
        else if (ePart == BO_WEAPON) {
            ePart = BIPED_OBJECT_COUNT + 1;
        }
    }
}

void TESNPC::ReplaceRefModel(TESObjectREFR* apRef) {
    uint8_t b3rdPerson;
    BipedAnim* pBiped = apRef->GetBiped();
    NiNode* pRoot = apRef->TESObjectREFR::Get3D();
    int32_t iPasses = 1;
    if (static_cast<const void*>(apRef) == static_cast<const void*>(pPlayer)) {
        iPasses = 2;
    }
    while (iPasses) {
        if (static_cast<const void*>(apRef) == static_cast<const void*>(pPlayer) && iPasses == 1) {
            b3rdPerson = pPlayer->Is3rdPerson();
            pBiped = pPlayer->GetBiped(b3rdPerson);
            pRoot = static_cast<NiNode*>(pPlayer->Get3D(true));
        }
        InitParts(apRef, pBiped, false);
        pBiped->LoadBipedParts(false);
        if (pRoot) {
            pRoot->UpdateProperties();
            pRoot->UpdateWorldBound();
            NiUpdateData kUpdateData(0.0, false, false);
            pRoot->Update(kUpdateData);
        }
        --iPasses;
    }
}

bool TESNPC::InitWornObject([[maybe_unused]] TESObjectREFR* apRef, BipedAnim* apBipedAnim, TESBoundObject* apObject) {
    bool bResult = true;
    TESBipedModelForm* pbipedModel = TESBipedModelForm::GetFormAsBipedModel(apObject);
    if (apObject->GetFormType() == WEAP_ID) {
        TESObjectWEAP* pweapon = static_cast<TESObjectWEAP*>(apObject);
        apBipedAnim->SetBipedWeapon(pweapon, 0);
    }
    else if (pbipedModel) {
        pbipedModel->AddToBiped(
            apBipedAnim,
            static_cast<TESActorBase::SEX>((actorData.iActorBaseFlags & BASE_FEMALE) != 0),
            BO_NONE
        );
    }
    else {
        Warning("MODELS: Bad part '%s' in TESNPC::InitWornObject.", TESFullName::GetFullName(apObject));
    }
    BGSBipedModelList* pformList = BGSBipedModelList::GetFormAsBipedModelList(apObject);
    if (pformList && pformList->GetBipedModelList()) {
        for (BSSimpleList<TESForm*>* pIter = pformList->GetBipedModelList()->GetFormList(); pIter && !pIter->IsEmpty();
             pIter = pIter->GetNext()) {
            pbipedModel = TESBipedModelForm::GetFormAsBipedModel(pIter->GetItem());
            if (pbipedModel) {
                pbipedModel->AddToBiped(
                    apBipedAnim,
                    static_cast<TESActorBase::SEX>((actorData.iActorBaseFlags & BASE_FEMALE) != 0),
                    BO_NONE
                );
            }
        }
    }
    return bResult;
}

// Hides the FaceGen head under a worn head item. Otherwise the head shows and the ears and normal hair hide under a
// worn hair item; with a hat-hair node, a worn hat also hides them and shows the hat hair instead.
void TESNPC::FixDisplayedHeadParts(TESObjectREFR* apRef, NiNode* apNode) {
    if (!apNode) {
        apNode = apRef->TESObjectREFR::Get3D();
    }
    if (!apNode) {
        return;
    }
    NiAVObject* pheadBiped = BSUtilities::GetObjectByName(apNode, "BSFaceGenNiNodeBiped");
    NiAVObject* pheadSkinned = BSUtilities::GetObjectByName(apNode, "BSFaceGenNiNodeSkinned");
    if (!pheadBiped || !pheadSkinned) {
        return;
    }
    InventoryChanges* pinv = InventoryChanges::GetInventoryChanges(apRef);
    ItemChange* pitem = pinv->GetWornItem(BO_HEAD, false);
    if (pitem) {
        pheadBiped->SetAppCulled(true);
        pheadSkinned->SetAppCulled(true);
        if (pitem) {
            delete pitem;
        }
        return;
    }
    pheadBiped->SetAppCulled(false);
    pheadSkinned->SetAppCulled(false);
    NiAVObject* pearsObj = BSUtilities::GetObjectByName(apNode, TESRace::GetHeadPartNodeName(RHP_EAR));
    char tempHairName[32];
    NiAVObject* phairObj[HAIR_PART_COUNT];
    for (int32_t ehairPart = HAIR_PART_NORMAL; ehairPart < HAIR_PART_COUNT; ++ehairPart) {
        phairObj[ehairPart] = BSUtilities::GetObjectByName(
            apNode,
            BSFaceGenManager::GetHairName(tempHairName, static_cast<HAIR_PART>(ehairPart))
        );
    }
    bool bhideHair = false;
    bool bhideHatHair = true;
    pitem = pinv->GetWornItem(BO_HAIR, false);
    if (pitem) {
        if (pitem) {
            delete pitem;
        }
        bhideHair = true;
    }
    if (phairObj[HAIR_PART_HAT]) {
        pitem = pinv->GetWornItem(BO_HAT, false);
        if (pitem) {
            if (pitem) {
                delete pitem;
            }
            bhideHatHair = false;
            bhideHair = true;
        }
    }
    if (pearsObj) {
        pearsObj->SetAppCulled(bhideHair);
    }
    for (int32_t ehairPart = HAIR_PART_NORMAL; ehairPart < HAIR_PART_COUNT; ++ehairPart) {
        if (phairObj[ehairPart]) {
            if (ehairPart == HAIR_PART_NORMAL) {
                phairObj[ehairPart]->SetAppCulled(bhideHair);
            }
            else {
                phairObj[ehairPart]->SetAppCulled(bhideHatHair);
            }
        }
    }
}

// Unless forced, a biped still holding a buffered object is left alone; otherwise it gets the race's naked body. An
// actor with a process is flagged for a 3D update unless its head (limb 0) was blown off. Without 3D, or when forced,
// the default outfit is equipped (not on the player's base form while the player wears anything), keeping the weapon
// unless the current package unequips it. That package is read from the cast actor even when apRef is not an actor.
// The head loads here without FaceGen multithreading; the worn items follow, and every biped but the
// first-person one gets its head parts fixed.
void TESNPC::InitParts(TESObjectREFR* apRef, BipedAnim* apBipedAnim, bool abForceInitDefaultWorn) {
    if (apBipedAnim) {
        if (!abForceInitDefaultWorn) {
            for (uint32_t i = 0; i < 20; ++i) {
                if (apBipedAnim->QBufferedObjects()[i].pParent) {
                    return;
                }
            }
        }
        apBipedAnim->InitNaked(GetFormRace(), static_cast<SEX>((actorData.iActorBaseFlags & BASE_FEMALE) != 0));
    }
    Actor* pactor = dynamic_cast<Actor*>(apRef);
    if (pactor && pactor->GetCurrentAIProcess()) {
        bool bupdate3D = true;
        ExtraDismemberedLimbs* plimbs = pactor->GetExtra()->GetDismembermentExtra();
        if (plimbs && plimbs->DismemberedLimbs.GetSize() && plimbs->GetDismemberedLimb(0)) {
            for (uint32_t i = 0; i < plimbs->DismemberedLimbs.GetSize(); ++i) {
                DismemberedLimb* plimb = plimbs->GetDismemberedLimb(i);
                if (!plimb->cLimb && plimb->bLimbExploded) {
                    bupdate3D = false;
                }
            }
        }
        if (bupdate3D) {
            pactor->GetCurrentAIProcess()->Set3DUpdateFlag(RESET_MODEL);
        }
    }
    if (!apRef->TESObjectREFR::Get3D() || abForceInitDefaultWorn) {
        bool binitDefaultWorn = ShouldRunInitDefaultWorn(apRef) | abForceInitDefaultWorn;
        if (this == pPlayer->GetObjectReference()) {
            InventoryChanges* pinv = InventoryChanges::GetInventoryChanges(apRef);
            if (pinv) {
                for (int32_t i = BO_HEAD; i < BIPED_OBJECT_COUNT; ++i) {
                    ItemChange* pitem = pinv->GetWornItem(static_cast<BIPED_OBJECT>(i), false);
                    if (pitem) {
                        binitDefaultWorn = false;
                        if (pitem) {
                            delete pitem;
                        }
                        break;
                    }
                }
            }
        }
        if (binitDefaultWorn) {
            bool bweapon = true;
            TESPackage* ppackage = pactor->GetCurrentPackage();
            if (ppackage && ppackage->GetForceUnequipWeapon()) {
                bweapon = false;
            }
            InitDefaultWorn(apRef, true, bweapon, false, false);
        }
    }
    if (!BSFaceGenManager::GetUseMultiThreading()) {
        LinearFaceGenHeadLoad(apRef, apBipedAnim);
    }
    InitWorn(apRef, apBipedAnim);
    if (apBipedAnim != pPlayer->GetBiped(true)) {
        FixDisplayedHeadParts(apRef, nullptr);
    }
}

void TESNPC::LinearFaceGenHeadLoad(TESObjectREFR* apRef, BipedAnim* apBipedAnim) {
    NiMatrix3 matrot;
    NiPointer<NiObject> spTempObject(nullptr);
    if (BSFaceGenManager::GetInstance() &&
        apBipedAnim->root &&
        BSFaceGenManager::GetUseFaceGenHeads() &&
        (apRef != pPlayer || !pPlayer->Is1stPersonBiped(apBipedAnim))) {
        NiAVObject* pobjTemp = apRef->TESObjectREFR::Get3D();
        NiNode* pnodeBase = nullptr;
        if (pobjTemp) {
            pnodeBase = pobjTemp->IsNode();
        }
        NiNode* pHeadNode = apBipedAnim->GetParentBone(BIPED_BONE_HEAD);
        NiAVObjectPalette* ptmpPal = nullptr;
        if (apRef->GetAnimation()) {
            if (apRef->GetAnimation()->GetControllerManager()) {
                ptmpPal = apRef->GetAnimation()->GetControllerManager()->GetObjectPalette();
            }
        }
        NiDefaultAVObjectPalette* ppal = static_cast<NiDefaultAVObjectPalette*>(ptmpPal);
        if (pHeadNode && pnodeBase) {
            if (!apRef->GetFaceNodeBiped(pHeadNode) && !apRef->GetFaceNodeSkinned(pHeadNode)) {
                NiAVObject* pobjTemp2;
                uint32_t ichild;
                uint32_t ichildcount;
                BSFaceGenNiNode* pfaceBiped = nullptr;
                BSFaceGenNiNode* pfaceSkinned = nullptr;
                BSFaceGenAnimationData* panimdata = nullptr;
                NiPointer<NiGeometryData> spdata(nullptr);
                NiPointer<NiSkinPartition> spskinpart(nullptr);
                if (spHeadBiped.m_pObject) {
                    if (spHeadSkinned.m_pObject && spHeadSkinned->m_uiRefCount <= 1) {
                        pfaceBiped = spHeadBiped.m_pObject;
                    }
                    else {
                        BSCloningProcess cloneData(1.0f);
                        pfaceBiped = static_cast<BSFaceGenNiNode*>(spHeadBiped->Clone(cloneData));
                    }
                    ichildcount = pfaceBiped->GetArrayCount();
                    for (ichild = 0; ichild < ichildcount; ++ichild) {
                        pobjTemp2 = pfaceBiped->GetAt(ichild);
                        NiTriBasedGeom* pgeom = pobjTemp2 ? pobjTemp2->IsTriBasedGeom() : nullptr;
                        if (pgeom && spHeadBiped != pfaceBiped) {
                            pgeom->GetModelData()->CreateDeepCopy(spTempObject);
                            spdata = static_cast<NiGeometryData*>(spTempObject.m_pObject);
                            if (spdata.m_pObject) {
                                pgeom->SetModelData(spdata.m_pObject);
                            }
                        }
                    }
                    if (spHeadBiped != pfaceBiped) {
                        panimdata = pfaceBiped->GetAnimationData()->CreateCopyBS();
                        if (panimdata) {
                            pfaceBiped->SetAnimationData(panimdata);
                        }
                    }
                }
                if (spHeadSkinned.m_pObject) {
                    if (spHeadSkinned->m_uiRefCount > 1) {
                        BSCloningProcess cloneData(1.0f);
                        pfaceSkinned = static_cast<BSFaceGenNiNode*>(spHeadSkinned->Clone(cloneData));
                    }
                    else {
                        pfaceSkinned = spHeadSkinned.m_pObject;
                    }
                    ichildcount = pfaceSkinned->GetArrayCount();
                    for (ichild = 0; ichild < ichildcount; ++ichild) {
                        pobjTemp2 = pfaceSkinned->GetAt(ichild);
                        NiTriBasedGeom* pgeom = pobjTemp2 ? pobjTemp2->IsTriBasedGeom() : nullptr;
                        if (pgeom) {
                            if (spHeadSkinned != pfaceSkinned) {
                                pgeom->GetModelData()->CreateDeepCopy(spTempObject);
                                spdata = static_cast<NiGeometryData*>(spTempObject.m_pObject);
                                if (spdata.m_pObject) {
                                    pgeom->SetModelData(spdata.m_pObject);
                                }
                            }
                            if (pgeom->GetSkinInstance() && pgeom->GetSkinInstance()->GetSkinPartition()) {
                                BSFaceGenModelExtraData* pextraModel = BSFaceGenManager::GetFaceGenModelExtraData(
                                    pgeom
                                );
                                if (pextraModel &&
                                    pextraModel->GetModel() &&
                                    pextraModel->GetModel()->GetSkinInstance()) {
                                    NiSkinInstance* pskinModel = pextraModel->GetModel()->GetSkinInstance();
                                    NiSkinInstance* pskinNew = pgeom->GetSkinInstance();
                                    NiAVObject** pbones = pskinModel->m_ppkBones;
                                    uint32_t ibonecount = pskinModel->GetSkinData()->m_uiBones;
                                    for (uint32_t ibone = 0; ibone < ibonecount; ++ibone) {
                                        pskinNew->SetBone(ibone, pbones[ibone]);
                                    }
                                }
                                else {
                                    int32_t iNPCID = iFormID;
                                    Warning(
                                        "MODELS: Could not correctly handle skinning for duplicate NPC "
                                        "\"%s\" (%08X).",
                                        GetFormEditorID(),
                                        iNPCID
                                    );
                                }
                                pgeom->GetSkinInstance()->GetSkinPartition()->CreateDeepCopy(spTempObject);
                                spskinpart = static_cast<NiSkinPartition*>(spTempObject.m_pObject);
                                if (spskinpart.m_pObject) {
                                    pgeom->GetSkinInstance()->SetSkinPartition(spskinpart.m_pObject);
                                }
                            }
                        }
                    }
                    if (spHeadSkinned != pfaceSkinned) {
                        if (panimdata) {
                            pfaceSkinned->SetAnimationData(panimdata);
                        }
                        else {
                            panimdata = pfaceSkinned->GetAnimationData()->CreateCopyBS();
                            if (panimdata) {
                                pfaceSkinned->SetAnimationData(panimdata);
                            }
                        }
                    }
                }
                if (!spHeadBiped.m_pObject && !spHeadSkinned.m_pObject && pFormRace) {
                    pFormRace->CreateHead(pfaceBiped, pfaceSkinned, this, false, false, TESActorBase::SEX_MALE);
                    sLastRaceFaceNum = pFormRace->GetFaceCoordNum();
                    spHeadBiped = pfaceBiped;
                    spHeadSkinned = pfaceSkinned;
                }

                if (!pfaceBiped && !pfaceSkinned) {
                    Warning("MODELS: Cannot create a head for an NPC (%d) (no race or bad race data).", iFormID);
                }
                if (pfaceBiped) {
                    if (pfaceBiped->GetAnimationData() && apRef->GetHealth() <= 0.0) {
                        pfaceBiped->GetAnimationData()->SetDead(true, true);
                        pfaceBiped->GetAnimationData()->SetMustUpdate(true);
                    }

                    pfaceBiped->SetAnimationUpdate(true);
                    pfaceBiped->SetApplyRotToParent(true);
                    pfaceBiped->SetTranslate(NiPoint3::ZERO);
                    pfaceBiped->SetRotate(NiMatrix3::IDENTITY);
                    pHeadNode->AttachChild(pfaceBiped, 1);
                    pfaceBiped->pActor = static_cast<Actor*>(apRef);
                    RecurseAndAddObjectsToPalette(pfaceBiped, ppal);
                }
                if (pfaceSkinned) {
                    matrot = NiMatrix3::IDENTITY;
                    pfaceSkinned->SetAnimationUpdate(pfaceBiped == nullptr);
                    pfaceSkinned->SetApplyRotToParent(pfaceBiped == nullptr);
                    pfaceSkinned->SetTranslate(NiPoint3::ZERO);
                    pnodeBase->AttachChild(pfaceSkinned, 1);
                    pfaceSkinned->pActor = static_cast<Actor*>(apRef);
                    RecurseAndAddObjectsToPalette(pfaceSkinned, ppal);
                    pfaceSkinned->FixSkinInstances(pnodeBase, true);
                }
                panimdata = pfaceBiped ? pfaceBiped->GetAnimationData() : nullptr;
                if (!panimdata) {
                    panimdata = pfaceSkinned ? pfaceSkinned->GetAnimationData() : nullptr;
                }
                if (panimdata) {
                    panimdata->ResetCurrentMorphs(0.0f, true, true, true, true, false);
                }
            }
            NiUpdateData kUpdateData(0.0f, false, false);
            pnodeBase->Update(kUpdateData);
        }
        else {
            Warning("MODELS: Cannot create a head for an NPC (%d) that does not have a biped-head node.", iFormID);
        }
    }
}

// The two beast-race flags compared before rebuilding the head both stay 0, so the branch for differing flags (a full
// model swap: the beast or sex body model through TESModel::SetModel, Set3D(null), Load3D, a QueuedHead and
// MiddleHighProcess::RemoveAndAttachItems) never runs and is left out.
void TESNPC::LoadFaceGen(Character* apCharacter) {
    bool bChanged = false;
    for (uint32_t i = 0; i < 2; ++i) {
        for (uint32_t j = 0; j < 2; ++j) {
            uint32_t uiRows = RaceFaceOffsetCoord[i][j].numRows();
            uint32_t uiCols = RaceFaceOffsetCoord[i][j].numCols();
            for (uint32_t k = 0; k < uiRows; ++k) {
                for (uint32_t m = 0; m < uiCols; ++m) {
                    float fValue;
                    // The save holds the raw bytes of the float.
                    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&fValue), sizeof(fValue));
                    // A NaN counts as a change.
                    if (fValue != RaceFaceOffsetCoord[i][j][k][m]) {
                        RaceFaceOffsetCoord[i][j][k][m] = fValue;
                        bChanged = true;
                    }
                }
            }
        }
    }
    if (!GetRace()) {
        Warning("MODELS: %s has a bad current race when loading facegen", TESFullName::GetFullName());
    }
    uint32_t uiRaceID;
    LoadNumericID(&uiRaceID, 4);
    TESRace* pRace = dynamic_cast<TESRace*>(TESForm::GetFormByNumericID(uiRaceID));
    if (uiRaceID && !pRace) {
        Warning("MODELS: NPC %s could not find race %08X", TESFullName::GetFullName(), uiRaceID);
    }
    if (TESRaceForm::GetFormRace() != pRace) {
        bChanged = true;
    }
    TESRaceForm::SetFormRace(pRace);
    if (!GetRace()) {
        Warning("MODELS: %s has a bad new race when loading facegen", TESFullName::GetFullName());
    }
    uint32_t uiHairID;
    LoadNumericID(&uiHairID, 4);
    TESHair* pNewHair = dynamic_cast<TESHair*>(TESForm::GetFormByNumericID(uiHairID));
    if (uiHairID && !pNewHair) {
        Warning("MODELS: NPC %s could not find hair %08X", TESFullName::GetFullName(), uiHairID);
    }
    if (pHair != pNewHair) {
        bChanged = true;
    }
    pHair = pNewHair;
    uint32_t uiEyesID;
    LoadNumericID(&uiEyesID, 4);
    TESEyes* pNewEyes = dynamic_cast<TESEyes*>(TESForm::GetFormByNumericID(uiEyesID));
    if (uiEyesID && !pNewEyes) {
        Warning("MODELS: NPC %s could not find eyes %08X", TESFullName::GetFullName(), uiEyesID);
    }
    if (pEyeColor != pNewEyes) {
        bChanged = true;
    }
    pEyeColor = pNewEyes;
    float fNewHairLength;
    // The save holds the raw bytes of the float and of the color.
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&fNewHairLength), sizeof(fNewHairLength));
    uint32_t uiNewHairColor;
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&uiNewHairColor), sizeof(uiNewHairColor));
    // A NaN counts as a change.
    if (fNewHairLength != fHairLength || iHairColor != uiNewHairColor) {
        bChanged = true;
    }
    fHairLength = fNewHairLength;
    iHairColor = uiNewHairColor;
    char cFemale;
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cFemale), sizeof(cFemale));
    if (static_cast<uint8_t>(TESActorBaseData::GetFlag(BASE_FEMALE)) != static_cast<uint8_t>(cFemale)) {
        bChanged = true;
    }
    // The saved byte is passed as the bool unchanged, whatever its value.
    TESActorBaseData::SetFlagBit(BASE_FEMALE, std::bit_cast<bool>(cFemale), true);
    if (!bChanged) {
        return;
    }
    NiNode* p3D = apCharacter->TESObjectREFR::Get3D();
    NiNode* pRoot = p3D ? p3D->IsNode() : nullptr;
    if (pRoot) {
        NiDefaultAVObjectPalette* pPalette = nullptr;
        if (apCharacter->Actor::GetAnimation() && apCharacter->Actor::GetAnimation()->GetControllerManager()) {
            pPalette = static_cast<NiDefaultAVObjectPalette*>(
                apCharacter->Actor::GetAnimation()->GetControllerManager()->GetObjectPalette()
            );
        }
        BSFaceGenNiNode* pFaceNode = apCharacter->GetFaceNodeSkinned(nullptr);
        if (pFaceNode && pFaceNode->GetParent()) {
            RecurseAndRemoveObjectsFromPalette(pFaceNode, pPalette);
            pFaceNode->GetParent()->DetachChild(pFaceNode);
        }
        pFaceNode = apCharacter->GetFaceNodeBiped(nullptr);
        if (pFaceNode && pFaceNode->GetParent()) {
            RecurseAndRemoveObjectsFromPalette(pFaceNode, pPalette);
            pFaceNode->GetParent()->DetachChild(pFaceNode);
        }
    }
    ClearHead();
    if (apCharacter->Character::GetBiped()) {
        LinearFaceGenHeadLoad(apCharacter, apCharacter->Character::GetBiped());
    }
    if (apCharacter->GetFaceNodeBiped(nullptr) && apCharacter->GetFaceNodeSkinned(nullptr)) {
        alignas(FaceGenNpcData) char kNpcDataMem[sizeof(FaceGenNpcData)];
        FaceGenNpcData* pNpcData = ::new (kNpcDataMem) FaceGenNpcData;
        TESRaceForm::GetFormRace()->GetFaceGenData(this, *pNpcData, TESActorBase::SEX_MALE, false);
        BSFaceGenManager::PrepareHeadForShaders(apCharacter->GetFaceNodeBiped(nullptr), *pNpcData);
        BSFaceGenManager::PrepareHeadForShaders(apCharacter->GetFaceNodeSkinned(nullptr), *pNpcData);
        pNpcData->~FaceGenNpcData();
    }
}

void TESNPC::RandomizeHead(bool abKeepAge, bool abKeepGender, bool abKeepHairLength) {
    float fGeoValue;
    FR2MatrixVTC<float> kFaceCoord[2][2];
    FR2MatrixVTC<float> kOffsetCoord[2][2];
    BSFaceGenManager::InitFaceGenCoord(&kOffsetCoord);
    TESActorBase::SEX eSex = GetSex();
    FR2MatrixVTC<float>(*pMeanFaceCoord)[2][2] = pFormRace->GetMeanFaceCoord(eSex);
    BSFaceGenManager::RandomizeFaceGenetic(gs_fNPCGeneticVariation.Float(), pMeanFaceCoord, &kFaceCoord);
    float fGeoAge = BSFaceGenManager::GetFaceCoordAttribute(kFaceCoord, FANATT_AGE, FANTYPE_GEO);
    float fTexAge = BSFaceGenManager::GetFaceCoordAttribute(kFaceCoord, FANATT_AGE, FANTYPE_TEX);
    if (abKeepAge) {
        fGeoValue = fGeoAge;
    }
    else {
        fGeoValue = RandomFloat0To1() * 50.0 + 15.0;
    }
    float fTexValue = fTexAge - (fGeoAge - fGeoValue);
    fGeoValue = BSMIN(BSMAX(fGeoValue, 15.0f), 65.0f);
    fTexValue = BSMIN(BSMAX(fTexValue, 15.0f), 65.0f);
    BSFaceGenManager::SetFaceCoordAttribute(kFaceCoord, FANATT_AGE, FANTYPE_GEO, fGeoValue);
    BSFaceGenManager::SetFaceCoordAttribute(kFaceCoord, FANATT_AGE, FANTYPE_TEX, fTexValue);
    float fGeoGender = BSFaceGenManager::GetFaceCoordAttribute(kFaceCoord, FANATT_GENDER, FANTYPE_GEO);
    float fTexGender = BSFaceGenManager::GetFaceCoordAttribute(kFaceCoord, FANATT_GENDER, FANTYPE_TEX);
    if (abKeepGender) {
        fGeoValue = fGeoGender;
        fTexValue = fTexGender;
    }
    else {
        fGeoValue = RandomFloat0To1() * 4.0 - 2.0;
        fTexValue = fTexGender - (fGeoGender - fGeoValue);
    }
    fGeoValue = BSMIN(BSMAX(fGeoValue, -2.0f), 2.0f);
    fTexValue = BSMIN(BSMAX(fTexValue, -2.0f), 2.0f);
    BSFaceGenManager::SetFaceCoordAttribute(kFaceCoord, FANATT_GENDER, FANTYPE_GEO, fGeoValue);
    BSFaceGenManager::SetFaceCoordAttribute(kFaceCoord, FANATT_GENDER, FANTYPE_TEX, fTexValue);
    if (!abKeepHairLength) {
        SetHairLength(RandomFloat0To1());
    }
    eSex = GetSex();
    pMeanFaceCoord = pFormRace->GetMeanFaceCoord(eSex);
    BSFaceGenManager::OffsetFaceGenCoord(pMeanFaceCoord, &kFaceCoord, &kOffsetCoord, false);
    if (BSFaceGenManager::CompareFaceGenCoord(&kOffsetCoord, GetOffsetFaceCoord())) {
        BSFaceGenManager::CopyFaceGenCoord(&kOffsetCoord, GetOffsetFaceCoord(), false, false);
        spBodyModTexture = static_cast<BSFaceGenNiNode*>(nullptr);
    }
}

void TESNPC::LoadObjectArray(
    TESObjectREFR* apRef,
    BipedAnim* apBipedAnim,
    BSSimpleArray<TESBoundObject*, 1024>* apObjectArray
) {
    apBipedAnim->InitNaked(pFormRace, static_cast<TESActorBase::SEX>((actorData.iActorBaseFlags & BASE_FEMALE) != 0));
    Actor* pactor = static_cast<Actor*>(apRef);
    if (apRef && pactor->pCurrentProcess) {
        pactor->pCurrentProcess->Set3DUpdateFlag(RESET_MODEL);
    }

    for (uint32_t i = 0; i < apObjectArray->iSize; ++i) {
        TESBoundObject* pboundObject = (*apObjectArray)[i];
        InitWornObject(apRef, apBipedAnim, pboundObject);
    }
    FixDisplayedHeadParts(apRef, nullptr);
    if (pactor && pactor->pCurrentProcess) {
        pactor->pCurrentProcess->ClearAll3DUpdateFlags();
    }

    apBipedAnim->LoadBipedParts(true);
    NiAVObject* p3D = apRef->Get3D();
    if (p3D) {
        p3D->UpdateProperties();
        p3D->UpdateWorldBound();
        NiUpdateData kUpdateData(0.0f, false, false);
        p3D->Update(kUpdateData);
    }
}

void TESNPC::BuildObjectArray(
    TESObjectREFR* apRef,
    BipedAnim* apBipedAnim,
    BSSimpleArray<TESBoundObject*, 1024>* apObjectArray
) {
    for (uint32_t i = 0; i < 20; ++i) {
        BIPOBJECT* pbipObject = apBipedAnim->GetBipObject(static_cast<BIPED_OBJECT>(i));
        if (pbipObject->pParent && pbipObject->pParent->IsBoundObject()) {
            TESBoundObject* pboundObject = static_cast<TESBoundObject*>(pbipObject->pParent);
            if (!apObjectArray->IsInArray(pboundObject)) {
                apObjectArray->Add(pboundObject);
            }
        }
    }
}

void TESNPC::SetHairColor(uint32_t aiVal) {
    iHairColor = aiVal;
}

void TESNPC::FixNPCNormals(TESObjectREFR* apRef) {
    if (apRef && BSFaceGenManager::GetFixNormals()) {
        NiAVObject* pobj = apRef->Get3D();
        uint32_t ichildren = 0;
        BSFaceGenNiNode* phead = (spHeadSkinned = apRef->GetFaceNode(nullptr)).m_pObject;
        NiAVObject* ptemp = BSUtilities::GetObjectByName(pobj, TESRace::GetHeadPartNodeName(RHP_HEAD));
        NiTriBasedGeom* pface = ptemp ? ptemp->IsTriBasedGeom() : nullptr;
        if (pface && phead) {
            if (!phead->GetFixedNormals()) {
                ptemp = BSUtilities::GetObjectByName(pobj, "UpperBody");
                if (ptemp) {
                    NiNode* pupperbody = ptemp->IsNode();
                    if (pupperbody) {
                        NiMaterialProperty* pmat;
                        NiProperty* pprop;
                        ptemp = pupperbody->GetAtChecked(0);
                        if (ptemp) {
                            NiNode* pnode = ptemp->IsNode();
                            if (pnode) {
                                ichildren = pnode->GetChildCount();
                                for (uint32_t i = 0; i < ichildren; ++i) {
                                    ptemp = pnode->GetAt(i);
                                    NiTriBasedGeom* pshape = ptemp ? ptemp->IsTriBasedGeom() : nullptr;
                                    if (pshape) {
                                        pprop = pshape->GetProperty(2);
                                        pmat = static_cast<NiMaterialProperty*>(
                                            NiObject::DynamicCastStatic(&NiMaterialProperty::ms_RTTI, pprop)
                                        );
                                        if (pmat) {
                                            if (!_strnicmp(pmat->m_kName.GetHandleString(), "Skin", 4u) &&
                                                FixNormals(pshape, pface, 0.5f, true, false)) {
                                                phead->SetFixedNormals(true);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else {
                            uint32_t i;
                            ichildren = pupperbody->GetChildCount();
                            for (i = 0; i < ichildren; ++i) {
                                NiTriBasedGeom* pshape;
                                ptemp = pupperbody->GetAt(i);
                                if (ptemp) {
                                    pshape = ptemp->IsTriBasedGeom();
                                }
                                else {
                                    pshape = nullptr;
                                }
                                if (pshape) {
                                    pprop = pshape->GetProperty(2);
                                    pmat = static_cast<NiMaterialProperty*>(
                                        NiObject::DynamicCastStatic(&NiMaterialProperty::ms_RTTI, pprop)
                                    );
                                    if (pmat) {
                                        if (!_strnicmp(pmat->m_kName.GetHandleString(), "Skin", 4u) &&
                                            FixNormals(pshape, pface, 0.5f, true, false)) {
                                            phead->SetFixedNormals(true);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (pFormRace) {
                FaceGenNpcData npcdata;
                npcdata.pHair = GetHair();
                npcdata.fHairLength = GetHairLength();
                npcdata.pEyes = GetEyeColor();
                npcdata.eSex = GetSex();
                npcdata.iHairColor = GetHairColor();
                GetFaceCoord(npcdata.FaceCoord);
                if (!npcdata.pEyes && pFormRace->GetEyeColorList() && !pFormRace->GetEyeColorList()->IsEmpty()) {
                    npcdata.pEyes = pFormRace->GetEyeColorList()->m_item;
                }

                // ipart is unsigned and is passed as the head part.
                for (uint32_t ipart = 0; ipart < 8; ++ipart) {
                    TESActorBase::SEX eSex = GetSex();
                    TESModel* pModel = pFormRace->GetHeadPartModel(eSex, static_cast<RACE_HEAD_PART>(ipart));
                    npcdata.pModel.Add(pModel);
                    eSex = GetSex();
                    TESTexture* pTexture = pFormRace->GetHeadPartTexture(eSex, static_cast<RACE_HEAD_PART>(ipart));
                    npcdata.pTexture.Add(pTexture);
                    const char* pNodeName = TESRace::GetHeadPartNodeName(static_cast<RACE_HEAD_PART>(ipart));
                    npcdata.pHeadPartNodeName.Add(pNodeName);
                    if (TESRace::GetFaceGenTexturing()) {
                        NiPointer<NiTexture> spTempTexture(static_cast<NiTexture*>(nullptr));
                        GetHeadPartModTexture(static_cast<RACE_HEAD_PART>(ipart), spTempTexture);
                        npcdata.spTexBaseMod.Add(spTempTexture);
                    }
                }
                npcdata.bFaceGenTexturing = TESRace::GetFaceGenTexturing();
                BSFaceGenManager::PrepareHeadForShaders(spHeadBiped.m_pObject, npcdata);
                BSFaceGenManager::PrepareHeadForShaders(spHeadSkinned.m_pObject, npcdata);
            }
        }
    }
}

void TESNPC::InitLoadFaceGen(Character* apCharacter) {
    NiMatrix3 kRaceRot;
    NiMatrix3 kRot;
    bBipedAnimUpdateSkins = true;
    apCharacter->MobileObject::GetCurrentAIProcess()->Update3dModel(apCharacter);
    bBipedAnimUpdateSkins = false;
    if (static_cast<const void*>(apCharacter) == static_cast<const void*>(pPlayer)) {
        NiNode* pRoot = static_cast<NiNode*>(pPlayer->Get3D(true));
        NiAVObject* pChild = pRoot ? pRoot->GetAtChecked(0) : nullptr;
        std::memcpy(&kRot, &NiMatrix3::IDENTITY, sizeof(kRot));
        if (pChild) {
            kRaceRot = pPlayer->MultipleMatrixByRace(kRot);
            pChild->SetRotate(kRaceRot);
        }
    }
}

void TESNPC::SaveFaceGen(Character* apCharacter) {
    float foffset;
    uint32_t ieyeID;
    bool bfemale;
    uint32_t ihairID;
    uint32_t iraceID;
    for (uint32_t i = 0; i < 2; ++i) {
        for (uint32_t j = 0; j < 2; ++j) {
            uint32_t inumrows = RaceFaceOffsetCoord[i][j].nrows;
            uint32_t inumcolumns = RaceFaceOffsetCoord[i][j].ncols;
            for (uint32_t k = 0; k < inumrows; ++k) {
                for (uint32_t l = 0; l < inumcolumns; ++l) {
                    foffset = RaceFaceOffsetCoord[i][j][k][l];
                    SaveGameDataOLD(&foffset, sizeof(foffset));
                }
            }
        }
    }
    iraceID = 0;
    if (pFormRace) {
        iraceID = pFormRace->iFormID;
    }
    SaveNumericID(&iraceID, 4);
    ihairID = 0;
    if (pHair) {
        ihairID = pHair->iFormID;
    }
    SaveNumericID(&ihairID, 4);
    ieyeID = 0;
    if (pEyeColor) {
        ieyeID = pEyeColor->iFormID;
    }
    SaveNumericID(&ieyeID, 4);
    SaveGameDataOLD(&fHairLength, sizeof(fHairLength));
    SaveGameDataOLD(&iHairColor, sizeof(iHairColor));
    bfemale = GetSex() == TESActorBase::SEX_FEMALE;
    SaveGameDataOLD(&bfemale, sizeof(bfemale));
}

void TESNPC::ClearFaceGenModels() {
    spHeadBiped = static_cast<BSFaceGenNiNode*>(nullptr);
    spHeadSkinned = static_cast<BSFaceGenNiNode*>(nullptr);
}

uint16_t TESNPC::GetFaceGenSaveSize(Character* apCharacter) {
    int16_t sSaveSize = 0;
    for (uint32_t i = 0; i < 2; ++i) {
        for (uint32_t j = 0; j < 2; ++j) {
            int16_t sRows = RaceFaceOffsetCoord[i][j].nrows;
            sSaveSize += 4 * (RaceFaceOffsetCoord[i][j].ncols * sRows);
        }
    }
    return sSaveSize + 21;
}

BSSimpleList<const char*>* TESNPC::BuildDefaultModelList(bool abArmor, bool abWeapon) {
    BSSimpleList<const char*>* plist = new BSSimpleList<const char*>;
    if (abArmor) {
        TESBipedModelForm* pModels[BIPED_OBJECT_COUNT];
        TESBipedModelForm* pupperModel = nullptr;
        TESActorBase::SEX esex = GetSex();
        ourMemset(pModels, 0, sizeof(pModels));
        for (BIPED_OBJECT n = BO_HEAD; n < BIPED_OBJECT_COUNT; n = static_cast<BIPED_OBJECT>(n + 1)) {
            TESObjectARMO* parmorObj = GetBestArmor(this, n);
            if (parmorObj && (!pupperModel || !pupperModel->FillsBipedSlot(n, false, nullptr)) && !pModels[n]) {
                pModels[n] = static_cast<TESBipedModelForm*>(parmorObj);
                if (pModels[n]) {
                    const char* pfilename = pModels[n]->GetBipedModel(esex);
                    if (pfilename) {
                        plist->AddHead(pfilename);
                    }
                }
                if (n == BO_UPPER_BODY) {
                    pupperModel = pModels[2];
                }
            }
        }
    }
    if (abWeapon) {
        TESObjectWEAP* pweapon = GetBestWeapon(this);
        if (pweapon) {
            TESModel* pmodel = static_cast<TESModelTextureSwap*>(pweapon);
            if (static_cast<TESModelTextureSwap*>(pweapon) != nullptr) {
                const char* pfilename = pmodel->GetModel();
                if (pfilename) {
                    plist->AddHead(pfilename);
                }
            }
        }
    }
    return plist;
}

void TESNPC::SwitchRace(TESRace* apRace, Actor* apActor) {
    TESRace* pcurrentRace = pFormRace;
    if (apRace != pcurrentRace) {
        bool bRaceHeight = fHeight == GetRaceHeight();
        if (pOriginalRace == apRace) {
            pFormRace = apRace;
            pOriginalRace = nullptr;
            RemoveChange(CHANGE_NPC_RACE);
        }
        else {
            pFormRace = apRace;
            AddChange(CHANGE_NPC_RACE);
            if (!pOriginalRace) {
                pOriginalRace = pcurrentRace;
            }
        }
        if (bRaceHeight) {
            fHeight = GetRaceHeight();
        }
        if (apActor) {
            apActor->QueueReplacementLocomotions(false);
        }
    }
}

uint32_t TESNPC::GetFormHealthLeveled(bool abAddBase) {
    int32_t iHealth;
    uint16_t usLevel = GetLevel();
    int32_t iLevel = usLevel;
    if (!usLevel) {
        iLevel = 1;
    }
    if (TESHealthForm::GetFormHealth()) {
        uint32_t uiBase;
        if (abAddBase) {
            uiBase = TESHealthForm::GetFormHealth();
        }
        else {
            uiBase = 0;
        }
        double dEndurance = static_cast<double>(GetEndurance());
        double dEnduranceOffset = fAVDNPCHealthEnduranceOffset.Float() + dEndurance;
        double dBase = fAVDNPCHealthEnduranceMult.Float() * dEnduranceOffset + static_cast<double>(uiBase);
        iHealth = static_cast<int32_t>(fAVDNPCHealthLevelMult.Float() * static_cast<double>(iLevel - 1) + dBase);
    }
    else {
        iHealth = 0;
    }
    if (iHealth < 0) {
        return 0;
    }
    return iHealth;
}

uint32_t TESNPC::GetFormFatigueLeveled(bool abAddBase) {
    int32_t iFatigue;
    uint16_t usLevel = GetLevel();
    int32_t iLevel = usLevel;
    if (!usLevel) {
        iLevel = 1;
    }
    if (GetFatigue()) {
        float fBase;
        if (abAddBase) {
            fBase = static_cast<float>(GetFatigue());
        }
        else {
            fBase = fAVDNPCFatigueBase.Float();
        }
        double dEndurance = static_cast<double>(GetEndurance());
        double dBase = fAVDNPCFatigueEnduranceMult.Float() * dEndurance + fBase;
        iFatigue = static_cast<int32_t>(fAVDNPCFatigueLevelMult.Float() * static_cast<double>(iLevel) + dBase);
    }
    else {
        iFatigue = 0;
    }
    if (iFatigue < 0) {
        return 0;
    }
    return iFatigue;
}

void TESNPC::FillCharGenPresetList(TESRace* apRace, TESActorBase::SEX aeSex, BSSimpleArray<TESNPC*, 1024>* apOutList) {
    if (apOutList) {
        for (char i = 1;; i = 0) {
            for (TESNPC* pnpc = static_cast<TESNPC*>(pDataHandler->pObjectList->lnk_lpRoot); pnpc;
                 pnpc = static_cast<TESNPC*>(pnpc->nod_lpNext)) {
                if (pnpc->GetFormType() == NPC__ID) {
                    if (pnpc->GetRace() == apRace &&
                        pnpc->GetSex() == aeSex &&
                        (!i || pnpc->GetFlag(BASE_HAS_CHARGEN_FACE))) {
                        if (pnpc != static_cast<TESNPC*>(pPlayer->GetObjectReference())) {
                            apOutList->Add(pnpc);
                        }
                    }
                }
            }
            if (!apOutList->IsEmpty() || !i) {
                break;
            }
        }
        apOutList->Sort(&PresetIndexSortFunction);
    }
}

void bhkRagdollController::SetLookAtMaxAngle(bhkRagdollController::LookIKBone aeBone, float afMaxAngle) {
    BoneParamA[aeBone].fLookAtMaxAngle = afMaxAngle;
}

BIPOBJECT* BipedAnim::QBufferedObjects() {
    return this->bufferedObjects;
}

// Without a manager, builds a null temporary smart pointer, reads its pointer and releases it under a construction
// flag.
NiTexture* BSFaceGenManager::GetDefaultBaseModulationTexture() {
    alignas(NiPointer<NiSourceTexture>) uint8_t aNullStorage[sizeof(NiPointer<NiSourceTexture>)];
    bool bConstructed = false;
    NiPointer<NiSourceTexture>* pspTexture;
    if (pInstance) {
        pspTexture = &pInstance->spDefaultBaseMod;
    }
    else {
        pspTexture = ::new (aNullStorage) NiPointer<NiSourceTexture>(nullptr);
        bConstructed = true;
    }
    NiSourceTexture* pTexture = *pspTexture;
    if (bConstructed) {
        reinterpret_cast<NiPointer<NiSourceTexture>*>(aNullStorage)->~NiPointer<NiSourceTexture>();
    }
    return pTexture;
}

BGSVoiceType* TESRace::GetDefaultVoiceType(TESActorBase::SEX aeSex) {
    return pDefaultVoiceType[aeSex];
}

float TESRace::GetHeight(TESActorBase::SEX aeSex) const {
    float fHeight;
    if (aeSex >= 0 && aeSex < 2) {
        fHeight = Data.fHeight[aeSex];
    }
    else {
        fHeight = 0.0f;
    }
    return fHeight;
}

float TESRace::GetWeight(TESActorBase::SEX aeSex) const {
    float fWeight;
    if (aeSex >= 0 && aeSex < 2) {
        fWeight = Data.fWeight[aeSex];
    }
    else {
        fWeight = 0.0f;
    }
    return fWeight;
}

uint16_t TESRace::GetFaceCoordNum() const {
    return sFaceCoordNum;
}

const char* TESRace::GetHeadPartNodeName(RACE_HEAD_PART aePart) {
    return ppHeadPartNames[aePart];
}

bool TESRace::GetFaceGenTexturing() {
    return TESRace::bFaceGenTexturing.Bool();
}

NiFixedString* FixedStrings::QSkin() {
    return FixedStrings::pSkin;
}

// Allocate the base map only when its slot is empty.
void NiTexturingProperty::SetBaseFilterMode(FilterMode eMode) {
    Map* pkMap = m_kMaps[BASE_INDEX];
    if (!pkMap) {
        pkMap = ::new Map;
        m_kMaps.SetAt(BASE_INDEX, pkMap);
    }
    pkMap->SetFilterMode(eMode);
}
