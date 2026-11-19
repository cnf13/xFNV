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
#include "BSCore/TLSData.h"
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

#include <bit>
#include <cstdint>
#include <memory>
#include <vector>

// The container entry the last CNTO chunk added, which a following COED chunk gives its extra data
// (TESNPC::Load's function-local static plastloaded).
static ContainerObject* plastloaded;

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
                char* pMessage = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
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
                        void* pMemory = ourMalloc(sizeof(ContainerItemExtra));
                        plastloaded->pItemExtra = pMemory ? ::new (pMemory) ContainerItemExtra : nullptr;
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
#if XFNV_64BIT
                // The file's record is 8 bytes, the faction's 4-byte form ID first.
                TESFile::WidenFormIDRecords(&temp, 1, 8, sizeof(temp), {{0, offsetof(FACTION_RANK, pFaction)}});
#endif
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
                float* pfloats = static_cast<float*>(__builtin_alloca_with_align(imemsize, 128));
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
                        "MASTERFILE: Could not find headpart (%08X) on NPC '%s' (%08X).", itempint, GetFormEditorID(),
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
                        "MASTERFILE: Could not find hair (%08X) on NPC '%s' (%08X).", itempint, GetFormEditorID(),
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
                        "MASTERFILE: Could not find eyes (%08X) on NPC '%s' (%08X).", itempint, GetFormEditorID(),
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
                        "MASTERFILE: Found face texture for NPC '%s' (%08X) in file '%s'", GetFormEditorID(), uiFormID,
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
    if (GetFlag(4u)) {
        TESNPC* item = this;
        TESNPC::CharGenPresetFaceList.Remove(item);
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
    FR2MatrixVTC<float>(*pCoord)[2][2] = pAlternateFaceOffsetCoord;
    if (pCoord) {
        (*pCoord)[0][0].DeleteArray(3);
    }
    pAlternateFaceOffsetCoord = nullptr;
}

void TESNPC::Save() {
    FR2MatrixVTC<float>::RowT v11;
    FR2MatrixVTC<float>::RowT result;
    FR2MatrixVTC<float>::RowT a2;
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
        TESHair* Hair = GetHair();
        TESForm::AddChunk(HNAM_ID, Hair->iFormID);
        float HairLength = GetHairLength();
        TESForm::AddChunk(LNAM_ID, std::bit_cast<uint32_t>(HairLength));
    }
    if (GetEyeColor()) {
        TESForm::AddChunk(ENAM_ID, GetEyeColor()->iFormID);
    }
    TESForm::AddChunk(HCLR_ID, iHairColor);
    if (GetCombatStyle()) {
        TESCombatStyle* v2 = GetCombatStyle();
        TESForm::AddChunk(ZNAM_ID, v2->iFormID);
    }
    // The enum promotes to its int32_t underlying type and uses the 4-byte overload.
    TESForm::AddChunk(NAM4_ID, eBloodImpactMaterial);
    uint32_t isize = RaceFaceOffsetCoord[0][0].nrows;
    if (isize) {
        uint32_t imemsize = 4 * isize;
        float* pfloats = static_cast<float*>(ourMalloc(sizeof(float) * isize));
        ourMemset(pfloats, 0, imemsize);
        for (uint32_t i = 0; i < isize; ++i) {
            FR2MatrixVTC<float>::RowT* v3 = ::new (&a2) FR2MatrixVTC<float>::RowT(RaceFaceOffsetCoord[0][0][i]);
            float* v4 = &(*v3)[0];
            pfloats[i] = *v4;
        }
        TESForm::AddChunkArray(FGGS_ID, pfloats, isize);
        ourFree(pfloats);
        pfloats = nullptr;
    }
    isize = RaceFaceOffsetCoord[0][1].nrows;
    if (isize) {
        uint32_t imemsize_2 = 4 * isize;
        float* pfloats_2 = static_cast<float*>(ourMalloc(sizeof(float) * isize));
        ourMemset(pfloats_2, 0, imemsize_2);
        for (uint32_t i_2 = 0; i_2 < isize; ++i_2) {
            FR2MatrixVTC<float>::RowT* v5 = ::new (&result) FR2MatrixVTC<float>::RowT(RaceFaceOffsetCoord[0][1][i_2]);
            float* v6 = &(*v5)[0];
            pfloats_2[i_2] = *v6;
        }
        TESForm::AddChunkArray(FGGA_ID, pfloats_2, isize);
        ourFree(pfloats_2);
        pfloats_2 = nullptr;
    }
    isize = RaceFaceOffsetCoord[1][0].nrows;
    if (isize) {
        uint32_t imemsize_3 = 4 * isize;
        float* pfloats_3 = static_cast<float*>(ourMalloc(sizeof(float) * isize));
        ourMemset(pfloats_3, 0, imemsize_3);
        for (uint32_t i_3 = 0; i_3 < isize; ++i_3) {
            FR2MatrixVTC<float>::RowT* v7 = ::new (&v11) FR2MatrixVTC<float>::RowT(RaceFaceOffsetCoord[1][0][i_3]);
            float* v8 = &(*v7)[0];
            pfloats_3[i_3] = *v8;
        }
        TESForm::AddChunkArray(FGTS_ID, pfloats_3, isize);
        ourFree(pfloats_3);
        pfloats_3 = nullptr;
    }
    TESForm::AddChunk(NAM5_ID, sLastRaceFaceNum);
    TESForm::AddChunk(NAM6_ID, std::bit_cast<uint32_t>(fHeight));
    TESForm::AddChunk(NAM7_ID, std::bit_cast<uint32_t>(fWeight));
    CloseForm();
}

uint16_t TESNPC::GetSaveSize(uint32_t aiFlags) {
    uint16_t saveSize = TESActorBase::GetSaveSize(aiFlags);
    if ((aiFlags & 512) != 0) {
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
        apSaveGameBuffer->SaveDataEndian(&balternativeCoords, 1u, 0);
        for (uint32_t i = 0; i < 2; ++i) {
            for (uint32_t j = 0; j < 2; ++j) {
                uint32_t inumrows = pfaceCoords[2 * i + j].nrows;
                uint32_t inumcolumns = pfaceCoords[2 * i + j].ncols;
                for (uint32_t k = 0; k < inumrows; ++k) {
                    for (uint32_t l = 0; l < inumcolumns; ++l) {
                        FR2MatrixVTC<float>::RowT a2;
                        foffset = (*(::new (&a2) FR2MatrixVTC<float>::RowT(pfaceCoords[2 * i + j][k])))[l];
                        apSaveGameBuffer->SaveDataEndian(&foffset, 4u, 0);
                    }
                }
            }
        }
        apSaveGameBuffer->SaveFormID(pHair, 0);
        apSaveGameBuffer->SaveFormID(pEyeColor, 0);
        apSaveGameBuffer->SaveDataEndian(&fHairLength, 4u, 0);
        apSaveGameBuffer->SaveDataEndian(&iHairColor, 4u, 0);
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
        bfemale = GetFlag(1u);
        apSaveGameBuffer->SaveDataEndian(&bfemale, 1u, 0);
    }
}

void TESNPC::SaveGame(uint32_t aiFlags) {
    TESActorBase::SaveGame(aiFlags);
    if ((aiFlags & 512) != 0) {
        SaveGameDataOLD(&data, 14);
    }
}

void TESNPC::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    FR2MatrixVTC<float>::RowT v21;
    bool bfemale;
    BGSHeadPart* ppart_2;
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
        uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
        pCl = dynamic_cast<TESClass*>(FormByNumericID);
    }
    uint8_t cresetFlags = 0;
    if (apLoadGameBuffer->GetReconstructedForm()) {
        cresetFlags |= 27u;
    }
    bool v55 = fHeight == GetRaceHeight();
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_RACE)) {
        CombatPlanLink::Key* v53 = nullptr;
        uint32_t v4 = apLoadGameBuffer->LoadFormID();
        TESForm* v5 = TESForm::GetFormByNumericID(v4);
        TESRace* prace = dynamic_cast<TESRace*>(v5);
        TESRace* FormRace = pFormRace;
        if (prace != FormRace) {
            pFormRace = prace;
            cresetFlags |= 27u;
        }
        uint32_t v7 = apLoadGameBuffer->LoadFormID();
        TESForm* v8 = TESForm::GetFormByNumericID(v7);
        pOriginalRace = dynamic_cast<TESRace*>(v8);
    }
    if (v55) {
        fHeight = GetRaceHeight();
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_FACE)) {
        balternativeCoords = false;
        apLoadGameBuffer->LoadData(&balternativeCoords, 1u);
        FR2MatrixVTC<float>* pfaceCoords = RaceFaceOffsetCoord[0];
        if (balternativeCoords) {
            if (!pAlternateFaceOffsetCoord) {
                // new FR2MatrixVTC<float>[2][2]: a count cookie, then the four matrices.
                size_t* pBlock = static_cast<size_t*>(ourMalloc(sizeof(size_t) + sizeof(FR2MatrixVTC<float>[2][2])));
                FR2MatrixVTC<float>(*pNew)[2][2] = nullptr;
                if (pBlock) {
                    // new[]'s cookie: the element count stored just before the array.
                    pBlock[0] = 4;
                    // The matrices follow the cookie, built as one run of four and treated as the [2][2] array.
                    FR2MatrixVTC<float>* pMatrices = ::new (static_cast<void*>(pBlock + 1)) FR2MatrixVTC<float>[4];
                    pNew = reinterpret_cast<FR2MatrixVTC<float>(*)[2][2]>(pMatrices);
                }
                pAlternateFaceOffsetCoord = pNew;
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
                        FR2MatrixVTC<float>::RowT v22;
                        foffset = 0.0f;
                        apLoadGameBuffer->LoadData(&foffset, sizeof(foffset));
                        double v9 = (*(::new (&v22) FR2MatrixVTC<float>::RowT(pfaceCoords[2 * i + j][k])))[l];
                        if (foffset != v9) {
                            float* v10 = &(*(::new (&v21) FR2MatrixVTC<float>::RowT(pfaceCoords[2 * i + j][k])))[l];
                            *v10 = foffset;
                            cresetFlags |= 8u;
                        }
                    }
                }
            }
        }
        TESHair* pcurrentHair = pHair;
        TESEyes* pcurrentEyes = pEyeColor;
        float fcurrentHairLength = fHairLength;
        fcurrentHairColor = static_cast<float>(iHairColor);
        uint32_t v11 = apLoadGameBuffer->LoadFormID();
        TESForm* v12 = TESForm::GetFormByNumericID(v11);
        pHair = dynamic_cast<TESHair*>(v12);
        uint32_t v13 = apLoadGameBuffer->LoadFormID();
        TESForm* v14 = TESForm::GetFormByNumericID(v13);
        pEyeColor = dynamic_cast<TESEyes*>(v14);
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
                BGSHeadPart* a2 = nullptr;
                uint32_t v15 = apLoadGameBuffer->LoadFormID();
                TESForm* v16 = TESForm::GetFormByNumericID(v15);
                pForm = dynamic_cast<BGSHeadPart*>(v16);
                if (pForm) {
                    listHeadParts.AddHead(*reinterpret_cast<void**>(&pForm));
                }
            }
            for (plist = &listHeadParts; plist; plist = plist->m_pkNext) {
                ppart_2 = static_cast<BGSHeadPart*>(plist->m_item);
                if (ppart_2) {
                    uint32_t iindex =
                        oldHeadParts.Find(ppart_2, 0, &BSSimpleArray<BGSHeadPart*>::DefaultEqualFunctionByAddress);
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
            (pcurrentHair != pHair ||
             (pcurrentEyes != pEyeColor ||
              (fHairLength != fcurrentHairLength || fcurrentHairColor != static_cast<double>(iHairColor))))) {
            cresetFlags |= 8u;
        }
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_GENDER)) {
        bfemale = false;
        apLoadGameBuffer->LoadData(&bfemale, sizeof(bfemale));
        bool Flag = GetFlag(1u);
        if (Flag != bfemale) {
            SetFlagBit(1, bfemale, true);
            cresetFlags |= 27u;
        }
    }
    if ((cresetFlags & 8) != 0) {
        ClearHead();
    }
    if (cresetFlags) {
        Reset3DForLoadedActors(cresetFlags);
    }
}

void TESNPC::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESActorBase::LoadGame(aiFlags, aiCurrentFlags);
    if ((aiFlags & 512) != 0) {
        LoadGameDataOLD(&data, 14);
    }
}

// Undoes the race, face and gender changes the save had but the loaded form has not: the original race comes back
// (reset flags 0x1B), the alternate face offsets and head parts are dropped (flag 8: the head is cleared), and the
// female flag is flipped back (0x1B). A height that was the race's follows the race. Each change is tested in the old
// flags, then its absence in the current flags.
void TESNPC::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESActorBase::Revert(apLoadGameBuffer);
    uint8_t cResetFlags = 0;
    bool bRaceHeight = GetRaceHeight() == fHeight;
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_NPC_RACE) &&
        !apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_RACE) && pOriginalRace) {
        TESRaceForm::SetFormRace(pOriginalRace);
        pOriginalRace = nullptr;
        cResetFlags |= 0x1B;
    }
    if (bRaceHeight) {
        fHeight = GetRaceHeight();
    }
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_NPC_FACE) &&
        !apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_FACE)) {
        FR2MatrixVTC<float>(*pCoord)[2][2] = pAlternateFaceOffsetCoord;
        if (pCoord) {
            (*pCoord)[0][0].DeleteArray(3);
        }
        pAlternateFaceOffsetCoord = nullptr;
        cResetFlags |= 8;
        listHeadParts.RemoveAll();
    }
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_NPC_GENDER) &&
        !apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NPC_GENDER)) {
        TESActorBaseData::SetFlagBit(BASE_FEMALE, !TESActorBaseData::GetFlag(BASE_FEMALE), true);
        cResetFlags |= 0x1B;
    }
    if (cResetFlags & 8) {
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
    uint8_t result = 0;
    if (!apFile) {
        return (bool)result;
    }
    if (!iFileOffset) {
        return (bool)result;
    }
    if (!apFile->OpenTES(NiFile::READ_ONLY, false)) {
        return (bool)result;
    }
    if (!apFile->SetOffset(iFileOffset)) {
        return (bool)result;
    }
    if (((uint32_t)((int32_t)apFile->GetTESForm())) != ((uint32_t)formEnumString[42].cFormID)) {
        return (bool)result;
    }
    int32_t* currentFormID = reinterpret_cast<int32_t*>(static_cast<uintptr_t>(apFile->TESFile::GetID()));
    if (currentFormID == reinterpret_cast<int32_t*>(static_cast<uintptr_t>(TESForm::GetFormID()))) {
        return true;
    }
    return (bool)result;
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
                    "MASTERFILE: Combat Style (%08X) not found in InitItem for NPC (%08X) '%s'.", iCombatStyleID,
                    iFormID, pEditorID
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
    TESNPC_Data* Skills = pComp->GetSkills();
    if (std::memcmp(&data, Skills, sizeof(data))) {
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
    FR2MatrixVTC<float>(*OffsetFaceCoord)[2][2] = pComp->GetOffsetFaceCoord();
    FR2MatrixVTC<float>(*v4)[2][2] = GetOffsetFaceCoord();
    if (BSFaceGenManager::CompareFaceGenCoord(v4, OffsetFaceCoord)) {
        return true;
    }
    TESCombatStyle* v5 = GetCombatStyle();
    if (v5 != pComp->GetCombatStyle()) {
        return true;
    }
    BGSImpactDataSet::IMPACT_MATERIAL_TYPE v6 = GetBloodImpactMaterial();
    if (v6 != pComp->GetBloodImpactMaterial()) {
        return true;
    }
    int32_t v7 = GetHeadParts()->ItemsInList();
    if (v7 != pComp->GetHeadParts()->ItemsInList()) {
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
    BaseProcess* CurrentAIProcess = pActionActor->pCurrentProcess;
    TESPackage* pactionpackage = CurrentAIProcess->GetPackageThatIsRunning();
    TESPackage* potherpackage = pprocess->GetPackageThatIsRunning();
    if (pactor == pPlayer && pPlayer->btransporting) {
        if (pactor->IsActoraGuard() && pactor->IsAlarmed()) {
            float v48 = appCalendar.GetHour() - 1.0;
            pprocess->SetLastTimeProcessed(v48);
        }
        return false;
    }

    char msg[304];
    if (!pactor->IsDead(false) || pactor->GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN) {
        if (pactor->IsUnConscious() ||
            (pactor->IsKnockedOut() || pactor->GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN)) {
            const char* a2 = sNoTalkUnConscious.String();
            std::sprintf(msg, "%s %s", pactor->GetFullName(), a2);
            Interface::ShowMessage(
                msg, 0, "Interfac\\Icons\\Message Icons\\glow_message_vaultboy_surprised.dds", nullptr, 2.0f, false
            );
            return false;
        }
        if (pactor->IsFleeing(false)) {
            CombatController* pcombat = pactor->GetCombatController();
            if (!pcombat || (!pcombat->IsActoraCombatTarget(pPlayer) || pPlayer->GetBlocked())) {
                const char* a2a = sNoTalkFleeing.String();
                std::sprintf(msg, "%s %s", pactor->GetFullName(), a2a);
                Interface::ShowMessage(
                    msg, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", nullptr, 2.0f, false
                );
                return false;
            }
        }
    }
    if (pactor->IsPlayerTeammate() && apActionRef == pPlayer) {
        if (Actor::IsConsciousAndNotAttackingPlayer(pactor)) {
            Interface::QueueMenuCreate(
                Interface::OPEN_COMPANION_WHEEL, apItemActivated, nullptr, nullptr, Interface::CMM_NONE, false
            );
        }
        return false;
    }

    if (pActionActor) {
        TESPackage* peditorpack = (&pActionActor->m_Extra)->GetPackageExtra();
        if ((pActionActor != pPlayer) && (peditorpack) && (peditorpack->GetPackType() == PACKAGE_TYPE_DIALOGUE)) {
            ExtraDataList* pextra = &pactor->m_Extra;
            TESObjectREFR* PackageExtraTarget = pextra->GetPackageExtraTarget();
            (&pActionActor->m_Extra)->SetPackageExtra(peditorpack, 4, PackageExtraTarget, true, true, false);
            if (peditorpack->GetOncePerDay()) {
                char Day = appCalendar.GetDay();
                pActionActor->SetRunOnceExtraPackage(peditorpack, Day);
            }
            BaseProcess* v47 = pactor->pCurrentProcess;
            v47->PackageDone(pActionActor, peditorpack);
            pactor->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
        }

        if ((pActionActor != pPlayer) && (pActionActor->pCurrentProcess)) {
            BaseProcess* v46 = pActionActor->pCurrentProcess;
            v46->SetTargetActivated(true);
        }

        if (pactor->IsInCombat() && (pActionActor == pPlayer && pactor->IsPlayerTeammate())) {
            pVATS.QuitVATSPlayback(false, false);
            Interface::QueueMenuCreate(
                Interface::OPEN_DIALOG, pactor, nullptr, nullptr, Interface::CMM_STANDARD, false
            );
        }
        else {
            BaseProcess* v44;
            BaseProcess* v45;
            if ((!pactor->IsInCombat() || pactor->IsKnockedOut()) &&
                (pActionActor != pPlayer ||
                 (pactor->GetSitSleepState() == SIT_SLEEP_NORMAL ||
                  (pactor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING ||
                   (pactor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
                    ((v45 = pactor->pCurrentProcess, v45->GetFurnitureMarker()) &&
                     (v44 = pactor->pCurrentProcess, v44->GetFurnitureMarker()->cNumber > 19u))))))) {
                if (pactor->IsReanimated()) {
                    return false;
                }
                if (pactor->IsDead(false)) {
                    if (pActionActor == pPlayer) {
                        Interface::QueueMenuCreate(
                            Interface::OPEN_CONTAINER, apItemActivated, nullptr, nullptr, Interface::CMM_STANDARD, false
                        );
                    }
                    else {
                        if ((apObjecttoGet) && (!dynamic_cast<TESNPC*>(apObjecttoGet))) {
                            apItemActivated->RemoveItem(
                                apObjecttoGet, nullptr, aicount, false, false, pActionActor, nullptr, nullptr, true,
                                false
                            );
                        }
                    }
                }
                else {
                    TESPackage* v9;
                    TESPackage* v10;
                    TESPackage* v11;
                    FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
                    if (!pactor->IsRestrained() && !pactor->IsParalyzed()) {
                        if (pActionActor == pPlayer ||
                            !pActionActor->GetShouldAttackActor(pactor, false, ereaction, false)) {
                            if (!pactor->IsActoraGuard() &&
                                pactor->GetShouldAttackActor(pActionActor, false, ereaction, false) &&
                                !pActionActor->IsSneaking()) {
                                BaseProcess* v42 = pactor->pCurrentProcess;
                                if (v42->EnterCombat(
                                        pactor, pActionActor, true, true, nullptr, false, false, true, 0, false, false,
                                        true, nullptr
                                    )) {
                                    return true;
                                }
                            }
                        }
                        else {
                            BaseProcess* v43 = pActionActor->pCurrentProcess;
                            if (v43->EnterCombat(
                                    pActionActor, pactor, false, true, nullptr, false, false, true, 0, false, false,
                                    true, nullptr
                                )) {
                                return true;
                            }
                        }
                    }
                    if (pActionActor == pPlayer && (pactor->IsKnockedOut() && pactor->GetEssential())) {
                        return false;
                    }
                    if (pActionActor == pPlayer ||
                        (pactor == pPlayer &&
                         ((v9 = pActionActor->GetCurrentPackage(), v9->IsInterruptPackage()) ||
                          ((v10 = pActionActor->GetCurrentPackage(), v10->GetPackType() == PACKAGE_TYPE_EXPLORE) ||
                           (v11 = pActionActor->GetCurrentPackage(), v11->GetPackType() == PACKAGE_TYPE_AMBUSH))))) {
                        if (pActionActor == pPlayer &&
                            ((pactor->IsKnockedOut() || (pPlayer->IsSneaking() && !pactor->IsPlayerTeammate())) &&
                             !pactor->GetGhost())) {
                            if (!pactor->IsKnockedOut()) {
                                BaseProcess* v41 = pactor->pCurrentProcess;
                                if (v41->GetPickpocketed()) {
                                    const char* a2b = gs_sNoPickPocketAgain.String();
                                    std::sprintf(msg, "%s %s", pactor->GetFullName(), a2b);
                                    Interface::ShowMessage(
                                        msg, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                                        nullptr, 2.0f, false
                                    );
                                    return false;
                                }
                            }
                            Interface::QueueMenuCreate(
                                Interface::OPEN_CONTAINER, apItemActivated, nullptr, nullptr, Interface::CMM_PICKPOCKET,
                                false
                            );
                        }
                        else {
                            BaseProcess* v40;
                            TESTopic* pgreetTopic = nullptr;
                            if (pActionActor == pPlayer ||
                                (v40 = pActionActor->pCurrentProcess, v40->GetNeedTalkPlayer())) {
                                pgreetTopic = TESTopic::GetTopic(DT_TOPIC, 0);
                            }
                            DialogueItem* pgreetDialogue = nullptr;
                            if (pgreetTopic) {
                                if (pActionActor == pPlayer) {
                                    pgreetDialogue =
                                        pgreetTopic->CreateDialogueItem(pactor, pPlayer, nullptr, nullptr, false);
                                }
                                else {
                                    BaseProcess* v39 = pActionActor->pCurrentProcess;
                                    if (v39->GetNeedTalkPlayer()) {
                                        pgreetDialogue = pgreetTopic->CreateDialogueItem(
                                            pActionActor, pPlayer, nullptr, nullptr, false
                                        );
                                    }
                                }
                            }
                            if (pgreetDialogue &&
                                (pgreetDialogue->pInfo->IsGoodbye() &&
                                 (pgreetDialogue->FirstResponse() && !pgreetDialogue->NextResponse()))) {
                                if (pactor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                                    BaseProcess* v38 = pactor->pCurrentProcess;
                                    v38->SetNeedTalkPlayer(true);
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
                                BaseProcess* v37 = pactor->pCurrentProcess;
                                if (v37->GetFurnitureMarker()) {
                                    BaseProcess* v36 = pactor->pCurrentProcess;
                                    if (v36->GetFurnitureMarker()->cNumber > 19u) {
                                        BaseProcess* v35 = pactor->pCurrentProcess;
                                        v35->AddPostAnimationAction(BaseProcess::POSTANIM_ACTION_GETOUT_OF_FURNITURE);
                                    }
                                }
                                if (pactor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
                                    NiPoint3 v73 = pActionActor->GetLocationOnReference();
                                    pactor->RequestRotate(v73, false);
                                }
                                TESTopic* ptopic = nullptr;
                                Actor* pspeaker = nullptr;
                                if (pActionActor == pPlayer) {
                                    if (!pactor->IsPathingComplete() && !pactor->IsRotating()) {
                                        BaseProcess* v34 = pactor->pCurrentProcess;
                                        v34->EndMoveMessage(pactor);
                                    }
                                    pActionActor->StopAttack();
                                    pactor->StopAttack();
                                    TESPackage* CurrentPackage = pActionActor->GetCurrentPackage();
                                    DialoguePackage* ppack = dynamic_cast<DialoguePackage*>(CurrentPackage);
                                    if (ppack) {
                                        ptopic = ppack->ptopic;
                                    }
                                    pspeaker = static_cast<Actor*>(apItemActivated);
                                    if ((pactor->GetCurrentTarget() == pPlayer) && (potherpackage) &&
                                        (potherpackage->GetPackType() == PACKAGE_TYPE_EXPLORE)) {
                                        if (!potherpackage->GetPackageSchedule() ||
                                            potherpackage->GetPackageSchedule()->PSData.iDuration) {
                                            pprocess->AddToProcedureIndexRunning(pactor, 1);
                                        }
                                        else {
                                            pprocess->AddToProcedureIndexRunning(pActionActor, 2);
                                        }
                                    }

                                    BSAudio* v13 = &BSAudio::QInstance();
                                    v13->EndAllSoundsOfType(4u);
                                    pactor->InterruptCast();
                                    if (pactor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                                        BaseProcess* v33 = pactor->pCurrentProcess;
                                        v33->SetNeedTalkPlayer(true);
                                        pactor->InitiateGetUpPackage();
                                    }
                                    else {
                                        Actor* pact = pActionActor;
                                        if (pActionActor == pPlayer) {
                                            pact = pactor;
                                        }
                                        pVATS.QuitVATSPlayback(false, false);
                                        Interface::QueueMenuCreate(
                                            Interface::OPEN_DIALOG, pact, ptopic, nullptr, Interface::CMM_STANDARD,
                                            false
                                        );
                                    }
                                }
                                else {
                                    if (Interface::GetMenuModeType() != Interface::eDialogMenu) {
                                        BaseProcess* v32 = pPlayer->pCurrentProcess;
                                        if (!v32->CanForceGreet(pActionActor)) {
                                            return false;
                                        }
                                        BSAudio* v14 = &BSAudio::QInstance();
                                        v14->EndAllSoundsOfType(4u);
                                        pActionActor->StopAttack();
                                        pactor->StopAttack();
                                        pspeaker = pActionActor;
                                        if (!pActionActor->IsInCombat()) {
                                            TESPackage* v15 = pActionActor->GetCurrentPackage();
                                            DialoguePackage* ppack_2 = dynamic_cast<DialoguePackage*>(v15);
                                            if (ppack_2) {
                                                ptopic = ppack_2->ptopic;
                                            }
                                        }
                                        if (!pactionpackage->GetPackageSchedule() ||
                                            pactionpackage->GetPackageSchedule()->PSData.iDuration) {
                                            BaseProcess* v30 = pActionActor->pCurrentProcess;
                                            v30->AddToProcedureIndexRunning(pActionActor, 1);
                                        }
                                        else {
                                            BaseProcess* v31 = pActionActor->pCurrentProcess;
                                            v31->AddToProcedureIndexRunning(pActionActor, 2);
                                        }
                                        pactor->InterruptCast();
                                        if (pactor == pPlayer) {
                                            pactor->ForceIdleAnimation();
                                        }
                                        else {
                                            BaseProcess* v29 = pactor->pCurrentProcess;
                                            v29->AddPostAnimationAction(BaseProcess::POSTANIM_FORCEIDLE_ANIMATION);
                                        }
                                        if (pActionActor == pPlayer) {
                                            pActionActor->ForceIdleAnimation();
                                        }
                                        else {
                                            BaseProcess* v28 = pActionActor->pCurrentProcess;
                                            v28->AddPostAnimationAction(BaseProcess::POSTANIM_FORCEIDLE_ANIMATION);
                                        }
                                        pVATS.QuitVATSPlayback(false, false);
                                        Actor* pact_2 = pActionActor;
                                        if (pActionActor == pPlayer) {
                                            pact_2 = pactor;
                                        }
                                        Interface::QueueMenuCreate(
                                            Interface::OPEN_DIALOG, pact_2, ptopic, nullptr, Interface::CMM_STANDARD,
                                            false
                                        );
                                    }
                                }
                            }
                            if (pgreetDialogue) {
                                delete pgreetDialogue;
                            }
                        }
                    }
                    else {
                        if ((apObjecttoGet) && (!dynamic_cast<TESNPC*>(apObjecttoGet))) {
                            pactor->RemoveItem(
                                apObjecttoGet, nullptr, aicount, true, false, pActionActor, nullptr, nullptr, true,
                                false
                            );
                            pActionActor->PickpocketAlarm(pactor, apObjecttoGet, aicount);
                            return true;
                        }

                        TESPackage* v16 = pActionActor->GetCurrentPackage();
                        if (v16->GetPackType() == PACKAGE_TYPE_ESCORT) {
                            BaseProcess* v27 = pActionActor->pCurrentProcess;
                            v27->AddToProcedureIndexRunning(pActionActor, 2);
                        }
                        else {
                            if (pActionActor->GetCurrentPackage()) {
                                TESPackage* v17 = pActionActor->GetCurrentPackage();
                                if (v17->GetPackType() != PACKAGE_TYPE_FOLLOW) {
                                    BaseProcess* v26 = pActionActor->pCurrentProcess;
                                    v26->AddToProcedureIndexRunning(pActionActor, 1);
                                }
                            }
                            if (pactor != pPlayer) {
                                if ((pactor != pActionActor) && (pactor->GetCurrentPackage())) {
                                    TESPackage* v18 = pactor->GetCurrentPackage();
                                    if (v18->GetPackType() != PACKAGE_TYPE_FOLLOW) {
                                        TESPackage* v19 = pactor->GetCurrentPackage();
                                        if (v19->GetPackType() != PACKAGE_TYPE_ESCORT) {
                                            pprocess->AddToProcedureIndexRunning(pactor, 1);
                                        }
                                    }
                                }

                                if (pactor != pActionActor && pactor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
                                    if (pActionActor->IsinDialogue()) {
                                        DialoguePackage* ppack_3 =
                                            static_cast<DialoguePackage*>(pActionActor->GetCurrentPackage());
                                        if (!ppack_3->GetDialogueNoHeadtrack() &&
                                            !ppack_3->GetDialogueDoNotControlTarget()) {
                                            NiPoint3 v74 = pActionActor->GetLocationOnReference();
                                            pactor->RequestRotate(v74, false);
                                        }
                                    }
                                    else {
                                        NiPoint3 v75 = pActionActor->GetLocationOnReference();
                                        pactor->RequestRotate(v75, false);
                                    }
                                }
                                if (!pActionActor->IsinDialogue() && pActionActor->IsNpc() && pactor->IsNpc()) {
                                    BaseProcess* v25 = pActionActor->pCurrentProcess;
                                    v25->AddToProcedureIndexRunning(pActionActor, 1);
                                    if (pActionActor->InitiateDialogue(
                                            pactor, nullptr, nullptr, true, false, false, nullptr, false, false
                                        )) {
                                        BaseProcess* v24 = pActionActor->pCurrentProcess;
                                        v24->AddToProcedureIndexRunning(pActionActor, 2);
                                        pprocess->AddToProcedureIndexRunning(pactor, 2);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else {
                if (pactor->GetCombatController() &&
                    (pActionActor == pPlayer && pactor->GetCombatController()->QSwimAllowed())) {
                    char apText[204];
                    const char* v20 = sActivateNPCCalmed.String();
                    std::sprintf(apText, "%s %s", GetFullName(), v20);
                    Interface::ShowMessage(apText, 0, nullptr, nullptr, 2.0f, false);
                    return false;
                }
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
            NiUpdateData kUpdateData;
            kUpdateData.~NiUpdateData();
            ::new (&kUpdateData) NiUpdateData(0.0f, false, false);
            p3D->Update(kUpdateData);
        }
    }
    else if (BSUtilities::GetObjectByName(p3D, "Bip01")) {
        void* pMemory = ourMalloc(sizeof(BipedAnim));
        pBipedAnim = pMemory ? ::new (pMemory) BipedAnim(apRequester, static_cast<NiNode*>(p3D)) : nullptr;
        apRequester->SetBiped(pBipedAnim);
        p3D->UpdateProperties();
        NiUpdateData kUpdateData;
        kUpdateData.~NiUpdateData();
        ::new (&kUpdateData) NiUpdateData(0.0f, false, false);
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
        AddChange(512);
    }
    else {
        TESActorBase::SetActorValue(aiWhich, aiValue);
    }
}

// Copies the parts the template use flags select from the template: apForceTemplate when it is an NPC, else this base's
// template unless that is leveled, else a default NPC built on the stack with the first race and class. A template that
// has not copied from its own template does so first. The base disposition is read back from this NPC, not the
// template, as the game does, and each CopyComponent is handed the template's component (null-checked).
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
        uint32_t iFlags = *GetFlags() & 0xEF9F5DF5;
        uint32_t iCopyFlags = *pactor->GetFlags() & 0x1060A20A;
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
                pNPC->pVoiceType =
                    static_cast<BGSVoiceType*>(BGSDefaultObjectManager::GetDefaultObject(DO_PLAYERVOICE_FEMALE_CHILD));
            }
            else {
                pNPC->pVoiceType =
                    static_cast<BGSVoiceType*>(BGSDefaultObjectManager::GetDefaultObject(DO_PLAYERVOICE_FEMALE));
            }
        }
        else if (pPlayer->IsPCYoung()) {
            pNPC->pVoiceType =
                static_cast<BGSVoiceType*>(BGSDefaultObjectManager::GetDefaultObject(DO_PLAYERVOICE_MALE_CHILD));
        }
        else {
            pNPC->pVoiceType =
                static_cast<BGSVoiceType*>(BGSDefaultObjectManager::GetDefaultObject(DO_PLAYERVOICE_MALE));
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
        uint32_t isizeArray = spHeadBiped.m_pObject->GetArrayCount();
        for (uint32_t ichild = 0; ichild < isizeArray; ++ichild) {
            pobj = spHeadBiped.m_pObject->GetAt(ichild);
            const char* HandleString = (&pobj->m_kName)->GetHandleString();
            if (!BSstrcmp(HandleString, "FaceGenEyeLeft")) {
                spLeftEye = pobj->IsTriBasedGeom();
            }
            const char* v3 = (&pobj->m_kName)->GetHandleString();
            if (!BSstrcmp(v3, "FaceGenEyeRight")) {
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
            char* v4 = filename.pString;
            TES::pInstance->CreateTextureImage(v4, sptexBase, false, false);
            if (sptexBase.m_pObject) {
                NiTexturingProperty* ptexprop;
                NiPropertyState* pstateleft = &spLeftEye.m_pObject->m_PropertyState;
                NiPropertyState* pstateright = &spRightEye.m_pObject->m_PropertyState;
                if ((pstateleft) && (pstateright)) {
                    int32_t v11;
                    int32_t v13;
                    NiShadeProperty* pshadeleft = pstateleft->GetShade();
                    if ((pshadeleft) && (pshadeleft->iShaderPropertyType >= 8)) {}

                    if (pshadeleft) {
                        int32_t v12 = pshadeleft->iShaderPropertyType >= 8 && pshadeleft->iShaderPropertyType <= 12;
                        v13 = v12;
                    }
                    else {
                        v13 = 0;
                    }
                    BSShaderPPLightingProperty* pshaderpropLightingLeft =
                        v13 ? static_cast<BSShaderPPLightingProperty*>(pshadeleft) : nullptr;
                    NiShadeProperty* pshaderight = pstateright->GetShade();
                    if (pshaderight) {
                        int32_t v10 = pshaderight->iShaderPropertyType >= 8 && pshaderight->iShaderPropertyType <= 12;
                        v11 = v10;
                    }
                    else {
                        v11 = 0;
                    }
                    BSShaderPPLightingProperty* pshaderpropLightingRight =
                        v11 ? static_cast<BSShaderPPLightingProperty*>(pshaderight) : nullptr;
                    if ((pshaderpropLightingLeft) && (pshaderpropLightingRight)) {
                        pshaderpropLightingLeft->SetDiffuseTexture(0, sptexBase.m_pObject);
                        pshaderpropLightingRight->SetDiffuseTexture(0, sptexBase.m_pObject);
                        return;
                    }
                }

                ptexprop = new NiTexturingProperty;
                ptexprop->SetBaseTexture(sptexBase.m_pObject);
                ptexprop->SetBaseClampMode(NiTexturingProperty::WRAP_S_WRAP_T);
                ptexprop->SetBaseFilterMode(NiTexturingProperty::FILTER_TRILERP);
                if (spLeftEye.m_pObject->GetProperty(5)) {
                    spLeftEye.m_pObject->RemoveProperty(5);
                }
                spLeftEye.m_pObject->AttachProperty(ptexprop);
                if (spRightEye.m_pObject->GetProperty(5)) {
                    spRightEye.m_pObject->RemoveProperty(5);
                }
                spRightEye.m_pObject->AttachProperty(ptexprop);
            }
        }
        BSFaceGenNiNode* v8 = spHeadBiped.m_pObject;
        pobj = v8->GetObjectByName("FaceGenEyeLeft");
        if (pobj) {
            BSShaderManager::PrepareObject(pobj, 0, 0);
        }
        BSFaceGenNiNode* v6 = spHeadBiped.m_pObject;
        pobj = v6->GetObjectByName("FaceGenEyeRight");
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
    NiPointer<BSFaceGenNiNode>* v1 = &(spHeadBiped = nullptr);
    spHeadSkinned = *v1;
}

void TESNPC::InitHead(BSFaceGenNiNode*& apOutNodeBiped, BSFaceGenNiNode*& apOutNodeSkinned) {
    spHeadBiped = static_cast<BSFaceGenNiNode*>(nullptr);
    spHeadSkinned = static_cast<BSFaceGenNiNode*>(nullptr);
    if ((!spHeadBiped.m_pObject && !spHeadSkinned.m_pObject) && (GetFormRace())) {
        TESRace* FormHealth_0 = GetFormRace();
        FormHealth_0->CreateHead(apOutNodeBiped, apOutNodeSkinned, this, true, false, TESActorBase::SEX_MALE);
        TESRace* v4 = GetFormRace();
        sLastRaceFaceNum = v4->GetFaceCoordNum();
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
    kRot.~NiMatrix3();
    ::new (&kRot) NiMatrix3;
    NiDefaultAVObjectPalette* pPalette = nullptr;
    if (apRef->GetAnimation() && apRef->GetAnimation()->GetControllerManager()) {
        pPalette =
            static_cast<NiDefaultAVObjectPalette*>(apRef->GetAnimation()->GetControllerManager()->GetObjectPalette());
    }
    if (pHeadBone && pRoot) {
        if (apOutNodeBiped) {
            if (apOutNodeBiped->GetAnimationData() && apRef->GetHealth() <= 0.0) {
                apOutNodeBiped->GetAnimationData()->SetDead(true, true);
                apOutNodeBiped->GetAnimationData()->SetMustUpdate(true);
            }
            apOutNodeBiped->SetAnimationUpdate(true);
            apOutNodeBiped->SetApplyRotToParent(true);
            apOutNodeBiped->SetTranslate(NiPoint3_ZERO);
            apOutNodeBiped->SetRotate(NiMatrix3_IDENTITY);
            pHeadBone->AttachChild(apOutNodeBiped, true);
            RecurseAndAddObjectsToPalette(apOutNodeBiped, pPalette);
        }
        if (apOutNodeSkinned) {
            kRot = NiMatrix3_IDENTITY;
            apOutNodeSkinned->SetAnimationUpdate(apOutNodeBiped == nullptr);
            apOutNodeSkinned->SetApplyRotToParent(apOutNodeBiped == nullptr);
            apOutNodeSkinned->SetTranslate(NiPoint3_ZERO);
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
        NiUpdateData kUpdateData;
        kUpdateData.~NiUpdateData();
        ::new (&kUpdateData) NiUpdateData(0.0f, false, false);
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
                apFileName, aiSize, "data\\Textures\\Characters\\FaceMods\\%s\\F%08X_%08X_%i.dds",
                bUpdate ? "FalloutNV.ESM" : pmaster->GetFilename(), iraceID, imyID, aePart
            );
        }
        else {
            BSsprintf(
                apFileName, aiSize, "data\\Textures\\Characters\\FaceMods\\%s\\M%08X_%08X_%i.dds",
                bUpdate ? "FalloutNV.ESM" : pmaster->GetFilename(), iraceID, imyID, aePart
            );
        }
    }
    else {
        BSsprintf(
            apFileName, aiSize, "data\\Textures\\Characters\\FaceMods\\%s\\%08X_%i.dds",
            bUpdate ? "FalloutNV.ESM" : pmaster->GetFilename(), imyID, aePart
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
        MEM_CONTEXT_ANIMATION, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESNPC.cpp", 5585
    );
    BipedAnim* pBipedAnim = apRef->GetBiped();
    if (pBipedAnim) {
        Warning(
            "ANIMATION: This npc \"%s\" has already been used.\r\nOnly the first reference will be used.\r\n",
            GetFormEditorID()
        );
    }
    else {
        void* pMemory = ourMalloc(sizeof(BipedAnim));
        pBipedAnim = pMemory ? ::new (pMemory) BipedAnim(apRef, nullptr) : nullptr;
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
    AddChange(0x400);
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
        ActorValue::Index eskill =
            static_cast<ActorValue::Index>(ActorValue::ToActorValue(ActorValue::SKILL, static_cast<int8_t>(iskill)));
        // Tests only the low byte of IsFlagSet's result.
        if (static_cast<uint8_t>(ActorValue::IsFlagSet(eskill, ActorValue::DEPRECATED)) ||
            !ActorValue::IsSkill(eskill)) {
            continue;
        }
        fvalue = 0.0f;
        ActorValue::Index eprimaryAttribute = static_cast<ActorValue::Index>(-1);
        AiFormulas::AVGetDerivedSkillPrimaryAttributeAndBase(eskill, eprimaryAttribute, fvalue);
        fvalue = AiFormulas::CalculateDerivedSkillValue(
            fvalue, cAttribute[eprimaryAttribute - ActorValue::STRENGTH], ucluck
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
// freed only when it is equipped without extra data, as the game does.
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

// apRef is cast to Actor without the result being used, as the game does. The left hand (3) and the weapon (5) are
// skipped in order and done last, as slots 20 and 21.
void TESNPC::InitWorn(TESObjectREFR* apRef, BipedAnim* apBipedAnim) {
    InventoryChanges* pinv = InventoryChanges::GetInventoryChanges(apRef);
    [[maybe_unused]] Actor* apactor = dynamic_cast<Actor*>(apRef);
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
                    bipedObjectName[ePart], TESFullName::GetFullName(pobj)
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
    NiUpdateData a2;
    BipedAnim* pBiped = apRef->GetBiped();
    NiNode* pRoot = apRef->TESObjectREFR::Get3D();
    int32_t v5 = 1;
    if (static_cast<const void*>(apRef) == static_cast<const void*>(pPlayer)) {
        v5 = 2;
    }
    while (v5) {
        if ((static_cast<const void*>(apRef) == static_cast<const void*>(pPlayer)) && (v5 == 1)) {
            b3rdPerson = (uint8_t)pPlayer->Is3rdPerson();
            pBiped = pPlayer->GetBiped((bool)b3rdPerson);
            pRoot = static_cast<NiNode*>(pPlayer->Get3D(true));
        }
        InitParts(apRef, pBiped, false);
        pBiped->LoadBipedParts(false);
        if (pRoot) {
            pRoot->UpdateProperties();
            pRoot->UpdateWorldBound();
            a2.~NiUpdateData();
            ::new (&a2) NiUpdateData(0.0, false, false);
            pRoot->Update(a2);
        }
        --v5;
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
            apBipedAnim, static_cast<TESActorBase::SEX>((actorData.iActorBaseFlags & 1) != 0), BO_NONE
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
                    apBipedAnim, static_cast<TESActorBase::SEX>((actorData.iActorBaseFlags & 1) != 0), BO_NONE
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
            apNode, BSFaceGenManager::GetHairName(tempHairName, static_cast<HAIR_PART>(ehairPart))
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
// unless the current package unequips it. That package is read from the cast actor even when apRef is not an actor, as
// the game does. The head loads here without FaceGen multithreading; the worn items follow, and every biped but the
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
    if ((BSFaceGenManager::GetInstance()) && (apBipedAnim->root) &&
        (BSFaceGenManager::GetUseFaceGenHeads() && (apRef != pPlayer || !pPlayer->Is1stPersonBiped(apBipedAnim)))) {
        NiAVObject* pobjTemp = apRef->TESObjectREFR::Get3D();
        NiNode* pnodeBase = nullptr;
        if (pobjTemp) {
            pnodeBase = pobjTemp->IsNode();
        }
        NiNode* pHeadNode = apBipedAnim->GetParentBone(BIPED_BONE_HEAD);
        NiAVObjectPalette* ptmpPal = nullptr;
        if (apRef->GetAnimation()) {
            Animation* v3 = apRef->GetAnimation();
            if (v3->GetControllerManager()) {
                Animation* v4 = apRef->GetAnimation();
                NiControllerManager* ControllerManager = v4->GetControllerManager();
                ptmpPal = ControllerManager->GetObjectPalette();
            }
        }
        NiDefaultAVObjectPalette* ppal = static_cast<NiDefaultAVObjectPalette*>(ptmpPal);
        if (pHeadNode && pnodeBase) {
            if (!apRef->GetFaceNodeBiped(pHeadNode) && !apRef->GetFaceNodeSkinned(pHeadNode)) {
                BSFaceGenAnimationData* panimdata_3;
                NiAVObject* pobjTemp2;
                uint32_t ichild;
                NiTriBasedGeom* pgeom_2;
                uint32_t ichildcount;
                BSFaceGenNiNode* pfaceBiped = nullptr;
                BSFaceGenNiNode* pfaceSkinned = nullptr;
                BSFaceGenAnimationData* panimdata = nullptr;
                NiPointer<NiGeometryData> spdata(nullptr);
                NiPointer<NiSkinPartition> spskinpart(nullptr);
                if (spHeadBiped.m_pObject) {
                    if (spHeadSkinned.m_pObject && spHeadSkinned.m_pObject->m_uiRefCount <= 1) {
                        pfaceBiped = spHeadBiped.m_pObject;
                    }
                    else {
                        BSCloningProcess cloneData(1.0f);
                        pfaceBiped = static_cast<BSFaceGenNiNode*>(spHeadBiped.m_pObject->Clone(cloneData));
                    }
                    ichildcount = pfaceBiped->GetArrayCount();
                    for (ichild = 0; ichild < ichildcount; ++ichild) {
                        NiTriBasedGeom* pgeom;
                        pobjTemp2 = pfaceBiped->GetAt(ichild);
                        if (pobjTemp2) {
                            pgeom = pobjTemp2->IsTriBasedGeom();
                        }
                        else {
                            pgeom = nullptr;
                        }
                        pgeom_2 = pgeom;
                        if ((pgeom) && (spHeadBiped != pfaceBiped)) {
                            pgeom_2->GetModelData()->CreateDeepCopy(spTempObject);
                            spdata = static_cast<NiGeometryData*>(spTempObject.m_pObject);
                            if (spdata.m_pObject) {
                                NiGeometryData* fScale = spdata.m_pObject;
                                pgeom_2->SetModelData(fScale);
                            }
                        }
                    }
                    if (spHeadBiped != pfaceBiped) {
                        BSFaceGenAnimationData* v6 = pfaceBiped->GetAnimationData();
                        panimdata = v6->CreateCopyBS();
                        if (panimdata) {
                            pfaceBiped->SetAnimationData(panimdata);
                        }
                    }
                }
                if (spHeadSkinned.m_pObject) {
                    if (spHeadSkinned.m_pObject->m_uiRefCount > 1) {
                        BSCloningProcess cloneData_2(1.0f);
                        pfaceSkinned = static_cast<BSFaceGenNiNode*>(spHeadSkinned.m_pObject->Clone(cloneData_2));
                    }
                    else {
                        pfaceSkinned = spHeadSkinned.m_pObject;
                    }
                    ichildcount = pfaceSkinned->GetArrayCount();
                    for (ichild = 0; ichild < ichildcount; ++ichild) {
                        NiTriBasedGeom* pgeom_3;
                        pobjTemp2 = pfaceSkinned->GetAt(ichild);
                        if (pobjTemp2) {
                            pgeom_3 = pobjTemp2->IsTriBasedGeom();
                        }
                        else {
                            pgeom_3 = nullptr;
                        }
                        pgeom_2 = pgeom_3;
                        if (pgeom_3) {
                            if (spHeadSkinned != pfaceSkinned) {
                                pgeom_2->GetModelData()->CreateDeepCopy(spTempObject);
                                spdata = static_cast<NiGeometryData*>(spTempObject.m_pObject);
                                if (spdata.m_pObject) {
                                    NiGeometryData* afScale = spdata.m_pObject;
                                    pgeom_2->SetModelData(afScale);
                                }
                            }
                            if ((pgeom_2->GetSkinInstance()) && (pgeom_2->GetSkinInstance()->GetSkinPartition())) {
                                BSFaceGenModel* v7;
                                BSFaceGenModelExtraData* pextraModel =
                                    BSFaceGenManager::GetFaceGenModelExtraData(pgeom_2);
                                if (pextraModel &&
                                    (pextraModel->GetModel() &&
                                     (v7 = pextraModel->GetModel(), v7->GetSkinInstance()))) {
                                    BSFaceGenModel* v8 = pextraModel->GetModel();
                                    NiSkinInstance* pskinModel = v8->GetSkinInstance();
                                    NiSkinInstance* pskinNew = pgeom_2->GetSkinInstance();
                                    NiAVObject** pbones = pskinModel->m_ppkBones;
                                    uint32_t ibonecount = pskinModel->GetSkinData()->m_uiBones;
                                    for (uint32_t ibone = 0; ibone < ibonecount; ++ibone) {
                                        pskinNew->SetBone(ibone, pbones[ibone]);
                                    }
                                }
                                else {
                                    int32_t CastingType = iFormID;
                                    Warning(
                                        "MODELS: Could not correctly handle skinning for duplicate NPC "
                                        "\"%s\" (%08X).",
                                        GetFormEditorID(), CastingType
                                    );
                                }
                                pgeom_2->GetSkinInstance()->GetSkinPartition()->CreateDeepCopy(spTempObject);
                                spskinpart = static_cast<NiSkinPartition*>(spTempObject.m_pObject);
                                if (spskinpart.m_pObject) {
                                    NiSkinPartition* v11 = spskinpart.m_pObject;
                                    pgeom_2->GetSkinInstance()->SetSkinPartition(v11);
                                }
                            }
                        }
                    }
                    if (spHeadSkinned != pfaceSkinned) {
                        if (panimdata) {
                            pfaceSkinned->SetAnimationData(panimdata);
                        }
                        else {
                            BSFaceGenAnimationData* panimdata_2 = pfaceSkinned->GetAnimationData();
                            panimdata = panimdata_2->CreateCopyBS();
                            if (panimdata) {
                                pfaceSkinned->SetAnimationData(panimdata);
                            }
                        }
                    }
                }
                if ((!spHeadBiped.m_pObject && !spHeadSkinned.m_pObject) && (pFormRace)) {
                    pFormRace->CreateHead(pfaceBiped, pfaceSkinned, this, false, false, TESActorBase::SEX_MALE);
                    sLastRaceFaceNum = pFormRace->GetFaceCoordNum();
                    spHeadBiped = pfaceBiped;
                    spHeadSkinned = pfaceSkinned;
                }

                if (!pfaceBiped && !pfaceSkinned) {
                    Warning("MODELS: Cannot create a head for an NPC (%d) (no race or bad race data).", iFormID);
                }
                if (pfaceBiped) {
                    if ((pfaceBiped->GetAnimationData()) && (apRef->GetHealth() <= 0.0)) {
                        BSFaceGenAnimationData* v17 = pfaceBiped->GetAnimationData();
                        v17->SetDead(true, true);
                        BSFaceGenAnimationData* v16 = pfaceBiped->GetAnimationData();
                        v16->SetMustUpdate(true);
                    }

                    pfaceBiped->SetAnimationUpdate(true);
                    pfaceBiped->SetApplyRotToParent(true);
                    pfaceBiped->SetTranslate(NiPoint3_ZERO);
                    pfaceBiped->SetRotate(NiMatrix3_IDENTITY);
                    pHeadNode->AttachChild(pfaceBiped, 1);
                    pfaceBiped->pActor = static_cast<Actor*>(apRef);
                    RecurseAndAddObjectsToPalette(pfaceBiped, ppal);
                }
                if (pfaceSkinned) {
                    matrot = NiMatrix3_IDENTITY;
                    pfaceSkinned->SetAnimationUpdate(pfaceBiped == nullptr);
                    pfaceSkinned->SetApplyRotToParent(pfaceBiped == nullptr);
                    pfaceSkinned->SetTranslate(NiPoint3_ZERO);
                    pnodeBase->AttachChild(pfaceSkinned, 1);
                    pfaceSkinned->pActor = static_cast<Actor*>(apRef);
                    RecurseAndAddObjectsToPalette(pfaceSkinned, ppal);
                    pfaceSkinned->FixSkinInstances(pnodeBase, true);
                }
                if (pfaceBiped) {
                    panimdata_3 = pfaceBiped->GetAnimationData();
                }
                else {
                    panimdata_3 = nullptr;
                }
                panimdata = panimdata_3;
                if (!panimdata_3) {
                    BSFaceGenAnimationData* panimdata_4;
                    if (pfaceSkinned) {
                        panimdata_4 = pfaceSkinned->GetAnimationData();
                    }
                    else {
                        panimdata_4 = nullptr;
                    }
                    panimdata = panimdata_4;
                }
                if (panimdata) {
                    panimdata->ResetCurrentMorphs(0.0f, true, true, true, true, false);
                }
            }
            NiUpdateData UpdateData(0.0f, false, false);
            pnodeBase->Update(UpdateData);
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
                    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&fValue), 4);
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
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&fNewHairLength), 4);
    uint32_t uiNewHairColor;
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&uiNewHairColor), 4);
    // A NaN counts as a change.
    if (fNewHairLength != fHairLength || iHairColor != uiNewHairColor) {
        bChanged = true;
    }
    fHairLength = fNewHairLength;
    iHairColor = uiNewHairColor;
    char cFemale;
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cFemale), 1);
    if (static_cast<uint8_t>(TESActorBaseData::GetFlag(BASE_FEMALE)) != static_cast<uint8_t>(cFemale)) {
        bChanged = true;
    }
    // The saved byte is passed as the bool unchanged, whatever its value, as the game does.
    TESActorBaseData::SetFlagBit(BASE_FEMALE, __builtin_bit_cast(bool, cFemale), true);
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

void TESNPC::RandomizeHead(bool a2, bool a3, bool a4) {
    float v24;
    FR2MatrixVTC<float> addr[2][2];
    FR2MatrixVTC<float> v21[2][2];
    BSFaceGenManager::InitFaceGenCoord(&v21);
    TESActorBase::SEX Sex = GetSex();
    FR2MatrixVTC<float>(*afRight)[2][2] = pFormRace->GetMeanFaceCoord(Sex);
    float* v5 = &gs_fNPCGeneticVariation.Float();
    BSFaceGenManager::RandomizeFaceGenetic(*v5, afRight, &addr);
    float FaceCoordAttribute = BSFaceGenManager::GetFaceCoordAttribute(addr, FANATT_AGE, FANTYPE_GEO);
    float v19 = BSFaceGenManager::GetFaceCoordAttribute(addr, FANATT_AGE, FANTYPE_TEX);
    if (a2) {
        v24 = FaceCoordAttribute;
    }
    else {
        v24 = RandomFloat0To1() * 50.0 + 15.0;
    }
    float v22 = v19 - (FaceCoordAttribute - v24);
    float afRighta = BSMAX(v24, 15.0f);
    v24 = BSMIN(afRighta, 65.0f);
    float afRightb = BSMAX(v22, 15.0f);
    v22 = BSMIN(afRightb, 65.0f);
    BSFaceGenManager::SetFaceCoordAttribute(addr, FANATT_AGE, FANTYPE_GEO, v24);
    BSFaceGenManager::SetFaceCoordAttribute(addr, FANATT_AGE, FANTYPE_TEX, v22);
    float v18 = BSFaceGenManager::GetFaceCoordAttribute(addr, FANATT_GENDER, FANTYPE_GEO);
    float v20 = BSFaceGenManager::GetFaceCoordAttribute(addr, FANATT_GENDER, FANTYPE_TEX);
    if (a3) {
        v24 = v18;
        v22 = v20;
    }
    else {
        v24 = RandomFloat0To1() * 4.0 - 2.0;
        v22 = v20 - (v18 - v24);
    }
    float afRightc = BSMAX(v24, -2.0f);
    v24 = BSMIN(afRightc, 2.0f);
    float afRightd = BSMAX(v22, -2.0f);
    v22 = BSMIN(afRightd, 2.0f);
    BSFaceGenManager::SetFaceCoordAttribute(addr, FANATT_GENDER, FANTYPE_GEO, v24);
    BSFaceGenManager::SetFaceCoordAttribute(addr, FANATT_GENDER, FANTYPE_TEX, v22);
    if (!a4) {
        float afValue = RandomFloat0To1();
        SetHairLength(afValue);
    }
    TESActorBase::SEX v6 = GetSex();
    FR2MatrixVTC<float>(*MeanFaceCoord)[2][2] = pFormRace->GetMeanFaceCoord(v6);
    BSFaceGenManager::OffsetFaceGenCoord(MeanFaceCoord, &addr, &v21, false);
    FR2MatrixVTC<float>(*OffsetFaceCoord)[2][2] = GetOffsetFaceCoord();
    if (BSFaceGenManager::CompareFaceGenCoord(&v21, OffsetFaceCoord)) {
        FR2MatrixVTC<float>(*v9)[2][2] = GetOffsetFaceCoord();
        BSFaceGenManager::CopyFaceGenCoord(&v21, v9, false, false);
        spBodyModTexture = static_cast<BSFaceGenNiNode*>(nullptr);
    }
}

void TESNPC::LoadObjectArray(
    TESObjectREFR* apRef,
    BipedAnim* apBipedAnim,
    BSSimpleArray<TESBoundObject*, 1024>* apObjectArray
) {
    apBipedAnim->InitNaked(pFormRace, static_cast<TESActorBase::SEX>((actorData.iActorBaseFlags & 1) != 0));
    Actor* pactor = static_cast<Actor*>(apRef);
    if ((apRef) && (pactor->pCurrentProcess)) {
        BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
        CurrentAIProcess->Set3DUpdateFlag(RESET_MODEL);
    }

    for (uint32_t i = 0; i < apObjectArray->iSize; ++i) {
        TESBoundObject* pboundObject = (*apObjectArray)[i];
        InitWornObject(apRef, apBipedAnim, pboundObject);
    }
    FixDisplayedHeadParts(apRef, nullptr);
    if ((pactor) && (pactor->pCurrentProcess)) {
        BaseProcess* v4 = pactor->pCurrentProcess;
        v4->ClearAll3DUpdateFlags();
    }

    apBipedAnim->LoadBipedParts(true);
    NiAVObject* p3D = apRef->Get3D();
    if (p3D) {
        p3D->UpdateProperties();
        p3D->UpdateWorldBound();
        NiUpdateData UpdateData(0.0f, false, false);
        p3D->Update(UpdateData);
    }
}

void TESNPC::BuildObjectArray(
    TESObjectREFR* apRef,
    BipedAnim* apBipedAnim,
    BSSimpleArray<TESBoundObject*, 1024>* apObjectArray
) {
    for (uint32_t i = 0; i < 20; ++i) {
        BIPOBJECT* pbipObject = apBipedAnim->GetBipObject(static_cast<BIPED_OBJECT>(i));
        if ((pbipObject->pParent) && (pbipObject->pParent->IsBoundObject())) {
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
    if ((apRef) && (BSFaceGenManager::GetFixNormals())) {
        NiAVObject* pobj = (NiAVObject*)apRef->Get3D();
        uint32_t ichildren = 0;
        BSFaceGenNiNode* phead = (&(spHeadSkinned = apRef->GetFaceNode(nullptr)))->m_pObject;
        const char* HeadPartNodeName = TESRace::GetHeadPartNodeName(RHP_HEAD);
        NiAVObject* ptemp = BSUtilities::GetObjectByName(pobj, HeadPartNodeName);
        NiTriBasedGeom* pface = ptemp ? ptemp->IsTriBasedGeom() : nullptr;
        if ((pface) && (phead)) {
            if (!phead->GetFixedNormals()) {
                ptemp = BSUtilities::GetObjectByName(pobj, "UpperBody");
                if (ptemp) {
                    NiNode* pupperbody = ptemp->IsNode();
                    if (pupperbody) {
                        NiTriBasedGeom* pshape_2;
                        NiMaterialProperty* pmat;
                        NiProperty* pprop;
                        ptemp = pupperbody->GetAtChecked(0);
                        if (ptemp) {
                            NiNode* pnode = ptemp->IsNode();
                            if (pnode) {
                                ichildren = pnode->GetChildCount();
                                for (uint32_t i = 0; i < ichildren; ++i) {
                                    NiTriBasedGeom* pshape;
                                    ptemp = pnode->GetAt(i);
                                    if (ptemp) {
                                        pshape = ptemp->IsTriBasedGeom();
                                    }
                                    else {
                                        pshape = nullptr;
                                    }
                                    pshape_2 = pshape;
                                    if (pshape) {
                                        pprop = pshape_2->GetProperty(2);
                                        pmat = static_cast<NiMaterialProperty*>(
                                            NiObject::DynamicCastStatic(&NiMaterialProperty::ms_RTTI, pprop)
                                        );
                                        if (pmat) {
                                            const char* HandleString = (&pmat->m_kName)->GetHandleString();
                                            if (!_strnicmp(HandleString, "Skin", 4u) &&
                                                FixNormals(pshape_2, pface, 0.5f, true, false)) {
                                                goto LABEL_35;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else {
                            uint32_t i_2;
                            ichildren = pupperbody->GetChildCount();
                            for (i_2 = 0; i_2 < ichildren; ++i_2) {
                                NiTriBasedGeom* pshape_3;
                                ptemp = pupperbody->GetAt(i_2);
                                if (ptemp) {
                                    pshape_3 = ptemp->IsTriBasedGeom();
                                }
                                else {
                                    pshape_3 = nullptr;
                                }
                                pshape_2 = pshape_3;
                                if (pshape_3) {
                                    pprop = pshape_2->GetProperty(2);
                                    pmat = static_cast<NiMaterialProperty*>(
                                        NiObject::DynamicCastStatic(&NiMaterialProperty::ms_RTTI, pprop)
                                    );
                                    if (pmat) {
                                        const char* v4;
                                        v4 = (&pmat->m_kName)->GetHandleString();
                                        if (!_strnicmp(v4, "Skin", 4u) &&
                                            FixNormals(pshape_2, pface, 0.5f, true, false)) {
                                        LABEL_35:;
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
                GetFaceCoord((&npcdata)->FaceCoord);
                if ((!npcdata.pEyes) && (pFormRace->GetEyeColorList()) && (!pFormRace->GetEyeColorList()->IsEmpty())) {
                    npcdata.pEyes = pFormRace->GetEyeColorList()->m_item;
                }

                // ipart is unsigned and is passed as the head part.
                for (uint32_t ipart = 0; ipart < 8; ++ipart) {
                    RACE_HEAD_PART v7 = static_cast<RACE_HEAD_PART>(ipart);
                    TESActorBase::SEX Sex = GetSex();
                    TESModel* element = pFormRace->GetHeadPartModel(Sex, v7);
                    npcdata.pModel.Add(element);
                    RACE_HEAD_PART v8 = static_cast<RACE_HEAD_PART>(ipart);
                    TESActorBase::SEX v6 = GetSex();
                    TESTexture* HeadPartTexture = pFormRace->GetHeadPartTexture(v6, v8);
                    npcdata.pTexture.Add(HeadPartTexture);
                    const char* v13 = TESRace::GetHeadPartNodeName(static_cast<RACE_HEAD_PART>(ipart));
                    npcdata.pHeadPartNodeName.Add(v13);
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
    NiMatrix3* v2;
    NiAVObject* v3;
    NiMatrix3 arOut;
    NiAVObject* v6;
    NiNode* v7;
    NiAVObject* v8;
    NiMatrix3 apRotIn;
    bBipedAnimUpdateSkins = true;
    BaseProcess* currentAIProcess = apCharacter->MobileObject::GetCurrentAIProcess();
    currentAIProcess->Update3dModel(apCharacter);
    bBipedAnimUpdateSkins = false;
    if (static_cast<const void*>(apCharacter) == static_cast<const void*>(pPlayer)) {
        v6 = pPlayer->Get3D(true);
        v7 = (v6 != nullptr) ? static_cast<NiNode*>(v6) : nullptr;
        if (v7) {
            v3 = v7->GetAtChecked(0);
        }
        else {
            v3 = nullptr;
        }
        v8 = v3;
        __builtin_memcpy(&apRotIn, &NiMatrix3_IDENTITY, sizeof(apRotIn));
        if (v3) {
            v2 = &(arOut = pPlayer->MultipleMatrixByRace(apRotIn));
            v8->SetRotate(*v2);
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
                    FR2MatrixVTC<float>::RowT result;
                    foffset = (result = RaceFaceOffsetCoord[i][j][k])[l];
                    SaveGameDataOLD(&foffset, 4);
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
    SaveGameDataOLD(&fHairLength, 4);
    SaveGameDataOLD(&iHairColor, 4);
    bfemale = GetSex() == TESActorBase::SEX_FEMALE;
    SaveGameDataOLD(&bfemale, 1);
}

void TESNPC::ClearFaceGenModels() {
    spHeadBiped = static_cast<BSFaceGenNiNode*>(nullptr);
    spHeadSkinned = static_cast<BSFaceGenNiNode*>(nullptr);
}

uint16_t TESNPC::GetFaceGenSaveSize(Character* apCharacter) {
    int16_t v7 = 0;
    for (uint32_t i = 0; i < 2; ++i) {
        for (uint32_t j = 0; j < 2; ++j) {
            int16_t Count = RaceFaceOffsetCoord[i][j].nrows;
            v7 += 4 * (RaceFaceOffsetCoord[i][j].ncols * Count);
        }
    }
    return v7 + 21;
}

BSSimpleList<const char*>* TESNPC::BuildDefaultModelList(bool abArmor, bool abWeapon) {
    BSSimpleList<const char*>* plist;
    const char* pfilename_2;
    const char* pfilename;
    BSSimpleList<const char*>* v7 =
        static_cast<BSSimpleList<const char*>*>(ourMalloc(sizeof(BSSimpleList<const char*>)));
    if (v7) {
        plist = ::new (v7) BSSimpleList<const char*>;
    }
    else {
        plist = nullptr;
    }
    if (abArmor) {
        TESBipedModelForm* pModels[BIPED_OBJECT_COUNT];
        TESBipedModelForm* pupperModel = nullptr;
        TESActorBase::SEX esex = GetSex();
        ourMemset(pModels, 0, sizeof(pModels));
        for (BIPED_OBJECT n = BO_HEAD; n < BIPED_OBJECT_COUNT; n = static_cast<BIPED_OBJECT>(n + 1)) {
            int32_t pextra = 0;
            TESObjectARMO* parmorObj = GetBestArmor(this, n);
            if ((parmorObj) && ((!pupperModel || !pupperModel->FillsBipedSlot(n, false, nullptr)) && !pModels[n])) {
                TESBipedModelForm* v4;
                if (parmorObj) {
                    v4 = static_cast<TESBipedModelForm*>(parmorObj);
                }
                else {
                    v4 = nullptr;
                }
                pModels[n] = v4;
                if (pModels[n]) {
                    pfilename = pModels[n]->GetBipedModel(esex);
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
                pfilename_2 = pmodel->GetModel();
                if (pfilename_2) {
                    plist->AddHead(pfilename_2);
                }
            }
        }
    }
    return plist;
}

void TESNPC::SwitchRace(TESRace* apRace, Actor* apActor) {
    TESRace* pcurrentRace = pFormRace;
    if (apRace != pcurrentRace) {
        bool v4 = fHeight == GetRaceHeight();
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
        if (v4) {
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
    int32_t iResult = iFatigue;
    if (iFatigue < 0) {
        return 0;
    }
    return iResult;
}

void TESNPC::FillCharGenPresetList(TESRace* apRace, TESActorBase::SEX aeSex, BSSimpleArray<TESNPC*, 1024>* apOutList) {
    if (apOutList) {
        for (char i = 1;; i = 0) {
            for (TESNPC* pnpc = static_cast<TESNPC*>(pDataHandler->pObjectList->lnk_lpRoot); pnpc;
                 pnpc = static_cast<TESNPC*>(pnpc->nod_lpNext)) {
                if (pnpc->GetFormType() == NPC__ID) {
                    TESNPC* pnpc_2 = pnpc;
                    if (pnpc->GetRace() == apRace && pnpc_2->GetSex() == aeSex && (!i || pnpc_2->GetFlag(4u))) {
                        TESNPC* ObjectReference = static_cast<TESNPC*>(pPlayer->GetObjectReference());
                        if (pnpc_2 != ObjectReference) {
                            apOutList->Add(pnpc_2);
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

// Without a manager this builds a null temporary smart pointer, reads it through operator-> and releases it under a
// construction flag. The temporary is never written, so operator-> reads uninitialised storage, as the game does.
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
    NiSourceTexture* pTexture = pspTexture->operator->();
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
        void* pvMemory = ourMalloc(sizeof(Map));
        pkMap = pvMemory ? ::new (pvMemory) Map : nullptr;
        m_kMaps.SetAt(BASE_INDEX, pkMap);
    }
    pkMap->SetFilterMode(eMode);
}
