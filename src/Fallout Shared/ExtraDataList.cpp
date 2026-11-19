#include "Fallout Shared/ExtraDataList.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkTTNode.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSMultiBoundRoom.h"
#include "BSMain/BSOcclusionPlane.h"
#include "BSMain/BSPortal.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActorCause.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/AudioMarkerData.h"
#include "Fallout Shared/BGSAcousticSpace.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEncounterZone.h"
#include "Fallout Shared/BGSImpactDataSet.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSMusicType.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSPrimitive.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/ExtraGuardedRefData.h"
#include "Fallout Shared/ExtraSeenData.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/Havok/TESWaterListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/MediaLocationController.h"
#include "Fallout Shared/MultiBoundMarkerData.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESChildCell.h"
#include "Fallout Shared/TESClimate.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESImageSpace.h"
#include "Fallout Shared/TESKey.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TeleportData.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ActorPackageData.h"
#include "Fallout/AI/AlarmPackage.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/AI/TrespassPackage.h"
#include "Fallout/Audio/FalloutAudioMedia.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Dialogue/MenuTopic.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/MenuManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/NonActorMagicCaster.h"
#include "Fallout/Magic/NonActorMagicTarget.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/SeenData.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>
#include <cstdint>

BSSpinLock BaseExtraList::ExtraCritSection;

// Reads one extra-data chunk of a reference's or a cell's record into the list; apOwner is the form being loaded. Form
// IDs are stored as the raw pointer values until the forms are resolved (InitItem). Chunk data read into a local is
// initialised to 0 here, so an empty chunk (malformed data) gives 0 rather than an uninitialised value.
void ExtraDataList::Load(TESFile* apFile, TESForm* apOwner) {
    switch (apFile->GetTESChunk()) {
        case XACT_ID: {
            uint32_t uiAction = 0;
            apFile->GetChunkData<uint32_t>(uiAction);
            SetActionFlag(static_cast<OBJECT_ACTION>(uiAction));
            break;
        }
        case XOWN_ID: {
            uint32_t uiOwner = 0;
            apFile->GetChunkData<uint32_t>(uiOwner);
            SetOwnership(reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiOwner)));
            break;
        }
        case XGLB_ID: {
            uint32_t uiGlobal = 0;
            apFile->GetChunkData<uint32_t>(uiGlobal);
            SetGlobal(reinterpret_cast<TESGlobal*>(static_cast<uintptr_t>(uiGlobal)));
            break;
        }
        case XRNK_ID: {
            uint32_t uiRank = 0;
            apFile->GetChunkData<uint32_t>(uiRank);
            SetRank(static_cast<int32_t>(uiRank));
            break;
        }
        case XHLT_ID: {
            // The health is stored as an integer and converted to the float argument.
            uint32_t uiHealth = 0;
            apFile->GetChunkData<uint32_t>(uiHealth);
            SetHealth(static_cast<float>(static_cast<int32_t>(uiHealth)));
            break;
        }
        case XHLP_ID: {
            float fHealthPerc = 0.0f;
            apFile->GetChunkData<float>(fHealthPerc);
            SetHealthPerc(fHealthPerc);
            break;
        }
        case XUSE_ID: {
            // Only the low byte of the 32-bit value is passed on.
            uint32_t uiUses = 0;
            apFile->GetChunkData<uint32_t>(uiUses);
            SetUses(static_cast<uint8_t>(uiUses));
            break;
        }
        case XTIM_ID: {
            float fTimeLeft = 0.0f;
            apFile->GetChunkData<float>(fTimeLeft);
            SetTimeLeft(fTimeLeft);
            break;
        }
        case XCNT_ID: {
            // Only the low 16 bits of the 32-bit chunk value are passed on.
            uint32_t uiCount = 0;
            apFile->GetChunkData<uint32_t>(uiCount);
            SetCount(static_cast<uint16_t>(uiCount));
            break;
        }
        case XAMC_ID: {
            uint32_t uiCount = 0;
            apFile->GetChunkData<uint32_t>(uiCount);
            if (GetAmmoExtra()) {
                SetAmmoExtra(GetAmmoExtra()->pAmmo, uiCount);
            }
            break;
        }
        case XAMT_ID: {
            uint32_t uiAmmo = 0;
            apFile->GetChunkData<uint32_t>(uiAmmo);
            SetAmmoExtra(reinterpret_cast<TESAmmo*>(static_cast<uintptr_t>(uiAmmo)), 999);
            break;
        }
        case XCHG_ID: {
            float fCharge = 0.0f;
            apFile->GetChunkData<float>(fCharge);
            SetCharge(fCharge);
            break;
        }
        case XPSN_ID: {
            uint32_t uiPoison = 0;
            apFile->GetChunkData<uint32_t>(uiPoison);
            SetPoison(reinterpret_cast<AlchemyItem*>(static_cast<uintptr_t>(uiPoison)));
            break;
        }
        case XLOC_ID: {
            auto* pExtra = static_cast<ExtraLock*>(GetExtraData(EXTRA_LOCK));
            LockData* pLock = nullptr;
            if (!pExtra) {
                void* pLockMemory = ourMalloc(sizeof(LockData));
                pLock = pLockMemory ? ::new (pLockMemory) LockData : nullptr;
                void* pExtraMemory = ourMalloc(sizeof(ExtraLock));
                pExtra = pExtraMemory ? ::new (pExtraMemory) ExtraLock(pLock) : nullptr;
                AddExtra(pExtra);
            }
            else {
                pLock = pExtra->pLock;
            }
            apFile->GetChunkData(pLock, 0x14);
#if XFNV_64BIT
            // The file's record is 0x14 bytes, the key's 4-byte form ID at 4.
            TESFile::WidenFormIDRecords(pLock, 1, 0x14, sizeof(LockData), {{4, offsetof(LockData, pKey)}});
#endif
            if (apFile->QEndian()) {
                pLock->Endian();
            }
            pLock->SetLocked(true);
            break;
        }
        case XRDO_ID: {
            RADIO_DATA kRadioData;
            kRadioData.~RADIO_DATA();
            ::new (&kRadioData) RADIO_DATA;
            apFile->GetChunkData(&kRadioData, 0x10);
#if XFNV_64BIT
            // The file's record is 0x10 bytes, the position reference's 4-byte form ID at 0xC.
            TESFile::WidenFormIDRecords(
                &kRadioData, 1, 0x10, sizeof(kRadioData), {{0xC, offsetof(RADIO_DATA, pPositionRef)}}
            );
#endif
            if (apFile->QEndian()) {
                kRadioData.Endian();
            }
            SetRadioData(&kRadioData);
            break;
        }
        case XTEL_ID: {
            DoorTeleportData* pTeleport = GetTeleport();
            if (!pTeleport) {
                void* pMemory = ourMalloc(sizeof(DoorTeleportData));
                pTeleport = pMemory ? ::new (pMemory) DoorTeleportData : nullptr;
                SetTeleportPtr(pTeleport);
            }
            pTeleport->Load(apFile);
            break;
        }
        case XMRK_ID: {
            MapMarkerData* pMapMarker = GetMapMarkerData();
            if (!pMapMarker) {
                void* pMemory = ourMalloc(sizeof(MapMarkerData));
                pMapMarker = pMemory ? ::new (pMemory) MapMarkerData : nullptr;
                SetMapMarkerPtr(pMapMarker);
            }
            pMapMarker->Load(apFile);
            break;
        }
        case MMRK_ID: {
            AudioMarker* pAudioMarker = GetAudioMarkerData();
            if (!pAudioMarker) {
                void* pMemory = ourMalloc(sizeof(AudioMarker));
                pAudioMarker = pMemory ? ::new (pMemory) AudioMarker : nullptr;
                SetAudioMarkerPtr(pAudioMarker);
            }
            pAudioMarker->Load(apFile);
            break;
        }
        case AMRK_ID: {
            AudioBuoyMarkerData* pAudioBuoyData = GetAudioBuoyMarkerData();
            if (!pAudioBuoyData) {
                void* pMemory = ourMalloc(sizeof(AudioBuoyMarkerData));
                pAudioBuoyData = pMemory ? ::new (pMemory) AudioBuoyMarkerData : nullptr;
                SetAudioBuoyMarkerPtr(pAudioBuoyData);
            }
            pAudioBuoyData->Load(apFile);
            break;
        }
        case XSED_ID: {
            // A 4-byte chunk holds the seed in its low byte; any other size is read as the byte itself.
            uint8_t cSeed = 0;
            if (apFile->GetChunkSize() == 4) {
                uint32_t uiSeed = 0;
                apFile->GetChunkData<uint32_t>(uiSeed);
                cSeed = static_cast<uint8_t>(uiSeed);
            }
            else {
                apFile->GetChunkData(&cSeed, 1);
            }
            SetSeedIndex(cSeed);
            break;
        }
        case XCLR_ID: {
            uint32_t uiSize = apFile->GetChunkSize();
            uint32_t uiCount = uiSize >> 2;
            if (uiSize % 4) {
                Warning("MASTERFILE: Invalid Extra Data - Region List in file \"%s\".", apFile->GetFilename());
                break;
            }
            void* pListMemory = ourMalloc(sizeof(TESRegionList));
            TESRegionList* pRegionList = pListMemory ? ::new (pListMemory) TESRegionList(false) : nullptr;
            // EXE-INLINED none: the array allocation's size, all ones when the byte count overflows
            auto* pRegionIDs = static_cast<uint32_t*>(
                ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
            );
            apFile->GetChunkData(pRegionIDs, uiSize);
            for (uint32_t i = 0; i < uiCount; ++i) {
                if (apFile->QEndian()) {
                    ENDIAN(pRegionIDs[i], false);
                }
                TESForm::AddCompileIndex(pRegionIDs[i], apFile);
                TESRegion* pRegion = pDataHandler->GetRegionList()->HasRegion(pRegionIDs[i]);
                if (pRegion) {
                    pRegionList->Add(pRegion);
                }
            }
            SetRegionList(pRegionList);
            ourFree(pRegionIDs);
            break;
        }
        case XCMT_ID: {
            // The obsolete chunk is read and dropped.
            uint8_t cUnused = 0;
            apFile->GetChunkData(&cUnused, 1);
            break;
        }
        case XCMO_ID: {
            uint32_t uiMusicType = 0;
            apFile->GetChunkData<uint32_t>(uiMusicType);
            SetMusicType(reinterpret_cast<BGSMusicType*>(static_cast<uintptr_t>(uiMusicType)));
            break;
        }
        case XCET_ID: {
            // The obsolete chunk is read and dropped.
            uint8_t cUnused = 0;
            apFile->GetChunkData(&cUnused, 1);
            break;
        }
        case XCAS_ID: {
            uint32_t uiSpace = 0;
            apFile->GetChunkData<uint32_t>(uiSpace);
            SetAcousticSpace(reinterpret_cast<BGSAcousticSpace*>(static_cast<uintptr_t>(uiSpace)));
            break;
        }
        case XCCM_ID: {
            uint32_t uiClimate = 0;
            apFile->GetChunkData<uint32_t>(uiClimate);
            SetClimate(reinterpret_cast<TESClimate*>(static_cast<uintptr_t>(uiClimate)));
            break;
        }
        case XCIM_ID: {
            uint32_t uiImageSpace = 0;
            apFile->GetChunkData<uint32_t>(uiImageSpace);
            SetImageSpace(reinterpret_cast<TESImageSpace*>(static_cast<uintptr_t>(uiImageSpace)));
            break;
        }
        case XCWT_ID: {
            uint32_t uiWater = 0;
            apFile->GetChunkData<uint32_t>(uiWater);
            SetWaterType(reinterpret_cast<TESWaterForm*>(static_cast<uintptr_t>(uiWater)));
            break;
        }
        case XPSL_ID: {
            WORLD_LOCATION kLocation;
            kLocation.~WORLD_LOCATION();
            ::new (&kLocation) WORLD_LOCATION;
            ourMemset(&kLocation, 0, sizeof(WORLD_LOCATION));
            apFile->GetChunkData(&kLocation, 0x14);
#if XFNV_64BIT
            // The file's record is 0x14 bytes, the location's 4-byte form ID first.
            TESFile::WidenFormIDRecords(
                &kLocation, 1, 0x14, sizeof(kLocation), {{0, offsetof(WORLD_LOCATION, pLocationForm)}}
            );
#endif
            if (apFile->QEndian()) {
                kLocation.Endian();
            }
            SetPackageStartLocation(
                static_cast<TESWorldSpace*>(kLocation.pLocationForm), nullptr, kLocation.locPt, kLocation.fZRot
            );
            break;
        }
        case XRGD_ID:
        case XRGB_ID: {
            RagDollData* pRagDollData = nullptr;
            auto* pExtra = static_cast<ExtraRagDollData*>(GetExtraData(EXTRA_RAGDOLLDATA));
            if (pExtra) {
                pRagDollData = pExtra->pRagDollData;
            }
            else {
                void* pExtraMemory = ourMalloc(sizeof(ExtraRagDollData));
                pExtra = pExtraMemory ? ::new (pExtraMemory) ExtraRagDollData : nullptr;
                void* pDataMemory = ourMalloc(sizeof(RagDollData));
                pExtra->pRagDollData = pDataMemory ? ::new (pDataMemory) RagDollData : nullptr;
                AddExtra(pExtra);
                pRagDollData = pExtra->pRagDollData;
            }
            if (!pRagDollData->Load(apFile)) {
                Warning("MASTERFILE: Failed to load RagDoll Data.");
                RemoveExtra(pExtra, true);
            }
            break;
        }
        case XLOD_ID: {
            NiPoint3 kNormal;
            ::new (&kNormal) NiPoint3(0.0f, 0.0f, 0.97f);
            apFile->GetChunkData(&kNormal, 0xC);
            if (apFile->QEndian()) {
                ENDIAN(kNormal.x, false);
                ENDIAN(kNormal.y, false);
                ENDIAN(kNormal.z, false);
            }
            SetDistantDataNormal(kNormal);
            break;
        }
        case XESP_ID: {
            REF_ENABLE_PARENT_DATA kParentData;
            ourMemset(&kParentData, 0, sizeof(REF_ENABLE_PARENT_DATA));
            apFile->GetChunkData(&kParentData, 8);
#if XFNV_64BIT
            // The file's record is 8 bytes, the parent's 4-byte form ID first.
            TESFile::WidenFormIDRecords(
                &kParentData, 1, 8, sizeof(kParentData), {{0, offsetof(REF_ENABLE_PARENT_DATA, pParent)}}
            );
#endif
            if (apFile->QEndian()) {
                kParentData.Endian();
            }
            SetEnableStateParent(kParentData.pParent);
            SetEnableStateParentFlags(kParentData.cFlags);
            break;
        }
        case XRTM_ID: {
            uint32_t uiMarker = 0;
            apFile->GetChunkData<uint32_t>(uiMarker);
            SetRandomTeleportMarker(reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiMarker)));
            break;
        }
        case XLKR_ID: {
            uint32_t uiLinkedRef = 0;
            apFile->GetChunkData<uint32_t>(uiLinkedRef);
            SetLinkedRef(reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiLinkedRef)));
            break;
        }
        case XAPD_ID: {
            auto* pExtra = static_cast<ExtraActivateRef*>(GetExtraData(EXTRA_ACTIVATE_REF));
            if (!pExtra) {
                void* pMemory = ourMalloc(sizeof(ExtraActivateRef));
                pExtra = pMemory ? ::new (pMemory) ExtraActivateRef : nullptr;
                AddExtra(pExtra);
            }
            // A 4-byte chunk holds the flags in its low byte; any other size is read as the byte itself.
            if (apFile->GetChunkSize() == 4) {
                uint32_t uiFlags = 0;
                apFile->GetChunkData<uint32_t>(uiFlags);
                pExtra->cActivateFlags = static_cast<uint8_t>(uiFlags);
            }
            else {
                apFile->GetChunkData(&pExtra->cActivateFlags, 1);
            }
            break;
        }
        case XATO_ID: {
            auto* pExtra = static_cast<ExtraActivateRef*>(GetExtraData(EXTRA_ACTIVATE_REF));
            if (!pExtra) {
                void* pMemory = ourMalloc(sizeof(ExtraActivateRef));
                pExtra = pMemory ? ::new (pMemory) ExtraActivateRef : nullptr;
                AddExtra(pExtra);
            }
            // The size compare is signed.
            int32_t iSize = static_cast<int32_t>(apFile->GetChunkSize());
            if (iSize > 1) {
                auto* pText = static_cast<char*>(ourMalloc(iSize));
                apFile->GetChunkData(pText, iSize);
                SetActivateTextOverride(pText);
                ourFree(pText);
            }
            break;
        }
        case XAPR_ID: {
            auto* pExtra = static_cast<ExtraActivateRef*>(GetExtraData(EXTRA_ACTIVATE_REF));
            if (!pExtra) {
                void* pMemory = ourMalloc(sizeof(ExtraActivateRef));
                pExtra = pMemory ? ::new (pMemory) ExtraActivateRef : nullptr;
                AddExtra(pExtra);
            }
            void* pDataMemory = ourMalloc(sizeof(REF_ACTIVATE_DATA));
            REF_ACTIVATE_DATA* pData = pDataMemory ? ::new (pDataMemory) REF_ACTIVATE_DATA : nullptr;
            apFile->GetChunkData(pData, 8);
#if XFNV_64BIT
            // The file's record is 8 bytes, the parent's 4-byte form ID first.
            TESFile::WidenFormIDRecords(
                pData, 1, 8, sizeof(REF_ACTIVATE_DATA), {{0, offsetof(REF_ACTIVATE_DATA, pActivateRef)}}
            );
#endif
            if (apFile->QEndian()) {
                pData->Endian();
            }
            pExtra->ParentList.AddHead(pData);
            break;
        }
        case XACR_ID: {
            // Record of the old activate parent chunk.
            struct OLD_ACTIVATE_DATA {
                TESObjectREFR* pActivateRef;
                float fActivateDelay;
                uint8_t cActivateFlags;
                uint8_t padding09[3];
            };

            auto* pExtra = static_cast<ExtraActivateRef*>(GetExtraData(EXTRA_ACTIVATE_REF));
            if (!pExtra) {
                void* pMemory = ourMalloc(sizeof(ExtraActivateRef));
                pExtra = pMemory ? ::new (pMemory) ExtraActivateRef : nullptr;
                AddExtra(pExtra);
            }
            OLD_ACTIVATE_DATA kOldData;
            ourMemset(&kOldData, 0, sizeof(OLD_ACTIVATE_DATA));
            apFile->GetChunkData(&kOldData, 0xC);
#if XFNV_64BIT
            // The file's record is 0xC bytes, the parent's 4-byte form ID first.
            TESFile::WidenFormIDRecords(
                &kOldData, 1, 0xC, sizeof(kOldData), {{0, offsetof(OLD_ACTIVATE_DATA, pActivateRef)}}
            );
#endif
            if (apFile->QEndian()) {
                // Swaps the form ID slot as a raw 32-bit value.
                ENDIAN(reinterpret_cast<uint32_t&>(kOldData.pActivateRef), false);
                ENDIAN(kOldData.fActivateDelay, false);
            }
            void* pDataMemory = ourMalloc(sizeof(REF_ACTIVATE_DATA));
            REF_ACTIVATE_DATA* pData = pDataMemory ? ::new (pDataMemory) REF_ACTIVATE_DATA : nullptr;
            pData->pActivateRef = kOldData.pActivateRef;
            pData->fActivateDelay = kOldData.fActivateDelay;
            pExtra->ParentList.AddHead(pData);
            pExtra->cActivateFlags = kOldData.cActivateFlags;
            break;
        }
        case XDCR_ID: {
            REF_DECAL_DATA kDecal;
            kDecal.~REF_DECAL_DATA();
            ::new (&kDecal) REF_DECAL_DATA;
            ourMemset(&kDecal, 0, sizeof(REF_DECAL_DATA));
            apFile->GetChunkData(&kDecal, 0x1C);
#if XFNV_64BIT
            // The file's record is 0x1C bytes, the reference's 4-byte form ID first.
            TESFile::WidenFormIDRecords(&kDecal, 1, 0x1C, sizeof(kDecal), {{0, offsetof(REF_DECAL_DATA, pDecalRef)}});
#endif
            if (apFile->QEndian()) {
                // Swaps the form ID slot as a raw 32-bit value.
                ENDIAN(reinterpret_cast<uint32_t&>(kDecal.pDecalRef), false);
                ENDIAN(kDecal.Intersect.x, false);
                ENDIAN(kDecal.Intersect.y, false);
                ENDIAN(kDecal.Intersect.z, false);
                ENDIAN(kDecal.Normal.x, false);
                ENDIAN(kDecal.Normal.y, false);
                ENDIAN(kDecal.Normal.z, false);
            }
            AddDecalRef(kDecal.pDecalRef, kDecal.Intersect, kDecal.Normal);
            break;
        }
        case XMRC_ID: {
            uint32_t uiContainer = 0;
            apFile->GetChunkData<uint32_t>(uiContainer);
            SetMerchantContainer(reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiContainer)));
            break;
        }
        case XTRG_ID: {
            uint32_t uiTarget = 0;
            apFile->GetChunkData<uint32_t>(uiTarget);
            SetXTarget(reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiTarget)));
            break;
        }
        case XEZN_ID: {
            uint32_t uiZone = 0;
            apFile->GetChunkData<uint32_t>(uiZone);
            SetEncounterZone(reinterpret_cast<BGSEncounterZone*>(static_cast<uintptr_t>(uiZone)));
            break;
        }
        case XEMI_ID: {
            uint32_t uiSource = 0;
            apFile->GetChunkData<uint32_t>(uiSource);
            SetEmittanceSource(reinterpret_cast<TESRegion*>(static_cast<uintptr_t>(uiSource)));
            break;
        }
        case XMBR_ID: {
            uint32_t uiMultiBoundRef = 0;
            apFile->GetChunkData<uint32_t>(uiMultiBoundRef);
            SetMultiBoundRef(reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiMultiBoundRef)));
            break;
        }
        case XMBO_ID: {
            void* pMemory = ourMalloc(sizeof(MultiBoundMarkerData));
            MultiBoundMarkerData* pData = pMemory ? ::new (pMemory) MultiBoundMarkerData : nullptr;
            pData->Load(apFile);
            SetMultiBoundData(pData);
            break;
        }
        case XLCM_ID: {
            uint32_t uiModifier = 0;
            apFile->GetChunkData<uint32_t>(uiModifier);
            SetLevCreaModifier(static_cast<int32_t>(uiModifier));
            break;
        }
        case XPCI_ID:
            // The chunk after it (FULL) belongs to the editor; it is skipped.
            apFile->NextChunk();
            break;
        case XNVP_ID: {
            NavMeshTriangleEdgePortal kPortal;
            kPortal.~NavMeshTriangleEdgePortal();
            ::new (&kPortal) NavMeshTriangleEdgePortal;
            uint32_t uiSize = apFile->GetChunkSize();
            auto* pBuffer = static_cast<char*>(ourMalloc(uiSize));
            apFile->GetChunkData(pBuffer, uiSize);
            kPortal.Load(apOwner, apFile, pBuffer);
            ourFree(pBuffer);
            break;
        }
        case XNDP_ID: {
            NAVMESH_PORTAL kHandle;
            kHandle.~NAVMESH_PORTAL();
            ::new (&kHandle) NAVMESH_PORTAL;
            apFile->GetChunkData(&kHandle, 8);
#if XFNV_64BIT
            // The file's record is 8 bytes, the navmesh's 4-byte form ID first.
            TESFile::WidenFormIDRecords(&kHandle, 1, 8, sizeof(kHandle), {{0, offsetof(NAVMESH_PORTAL, pNavMesh)}});
#endif
            if (apFile->QEndian()) {
                kHandle.Endian();
            }
            NAVMESH_PORTAL kArgument;
            kArgument.~NAVMESH_PORTAL();
            ::new (&kArgument) NAVMESH_PORTAL(kHandle);
            alignas(ExtraNavMeshPortal) uint8_t aucPortal[sizeof(ExtraNavMeshPortal)];
            ExtraNavMeshPortal* pPortal = ::new (aucPortal) ExtraNavMeshPortal(kArgument);
            SetNavMeshPortal(pPortal);
            pPortal->~ExtraNavMeshPortal();
            break;
        }
        case XRDS_ID: {
            float fRadius = 0.0f;
            apFile->GetChunkData<float>(fRadius);
            SetRadius(fRadius);
            break;
        }
        case XRAD_ID: {
            float fRadiation = 0.0f;
            apFile->GetChunkData<float>(fRadiation);
            SetRadiation(fRadiation);
            break;
        }
        case XPWR_ID: {
            // A placeable water reference reads no reflection chunk. The base object is taken from the owner as a
            // TESObjectREFR: these chunks belong to references.
            TESObjectREFR* pOwnerRef = static_cast<TESObjectREFR*>(apOwner);
            if (pOwnerRef->GetObjectReference() && pOwnerRef->GetObjectReference()->GetFormType() == PWAT_ID) {
                break;
            }
            REF_REFLECTED_DATA kReflected;
            kReflected.~REF_REFLECTED_DATA();
            ::new (&kReflected) REF_REFLECTED_DATA;
            ourMemset(&kReflected, 0, sizeof(REF_REFLECTED_DATA));
            apFile->GetChunkData(&kReflected, 8);
#if XFNV_64BIT
            // The file's record is 8 bytes, the reference's 4-byte form ID first.
            TESFile::WidenFormIDRecords(
                &kReflected, 1, 8, sizeof(kReflected), {{0, offsetof(REF_REFLECTED_DATA, pRef)}}
            );
#endif
            if (apFile->QEndian()) {
                // Swaps the form ID slot as a raw 32-bit value.
                ENDIAN(reinterpret_cast<uint32_t&>(kReflected.pRef), false);
                ENDIAN(kReflected.iEffectFlags, false);
            }
            if (kReflected.iEffectFlags & 1) {
                SetReflectedByRef(kReflected.pRef, true);
            }
            if (kReflected.iEffectFlags & 2) {
                SetRefractedByRef(kReflected.pRef, true);
            }
            break;
        }
        case XLTW_ID: {
            // A placeable water reference reads no lit water chunk (as XPWR above).
            TESObjectREFR* pOwnerRef = static_cast<TESObjectREFR*>(apOwner);
            if (pOwnerRef->GetObjectReference() && pOwnerRef->GetObjectReference()->GetFormType() == PWAT_ID) {
                break;
            }
            uint32_t uiWaterRef = 0;
            apFile->GetChunkData<uint32_t>(uiWaterRef);
            SetLitWaterRef(reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiWaterRef)), true);
            break;
        }
        case XPRM_ID: {
            // PRIMITIVE_DATA's colour has an inline C++ constructor; the constructor runs on raw storage instead.
            alignas(PRIMITIVE_DATA) uint8_t aucPrimitive[sizeof(PRIMITIVE_DATA)];
            PRIMITIVE_DATA* pPrimitiveData = ::new (aucPrimitive) PRIMITIVE_DATA;
            ourMemset(pPrimitiveData, 0, sizeof(PRIMITIVE_DATA));
            apFile->GetChunkData(pPrimitiveData, 0x20);
            if (apFile->QEndian()) {
                pPrimitiveData->Endian();
            }
            BGSPrimitive* pPrimitive =
                BGSPrimitive::CreateAbstract(pPrimitiveData->eType, pPrimitiveData->aRadii, pPrimitiveData->aColor);
            if (pPrimitive) {
                AddPrimitive(pPrimitive);
            }
            break;
        }
        case XOCP_ID: {
            OCCLUSION_PLANE_DATA kPlaneData;
            kPlaneData.~OCCLUSION_PLANE_DATA();
            ::new (&kPlaneData) OCCLUSION_PLANE_DATA;
            ourMemset(&kPlaneData, 0, sizeof(OCCLUSION_PLANE_DATA));
            apFile->GetChunkData(&kPlaneData, 0x24);
            if (apFile->QEndian()) {
                kPlaneData.Endian();
            }
            BSOcclusionPlane* pPlane = new BSOcclusionPlane;
            pPlane->SetHalfExtents(kPlaneData.rHalfExtents);
            pPlane->SetCenter(kPlaneData.rCenter);
            NiMatrix3 kRotation;
            kRotation.~NiMatrix3();
            ::new (&kRotation) NiMatrix3;
            kRotation.MakeRotation(kPlaneData.fAngle, kPlaneData.rAxis.x, kPlaneData.rAxis.y, kPlaneData.rAxis.z);
            pPlane->SetRotation(kRotation);
            SetOcclusionPlane(pPlane);
            break;
        }
        case XPTL_ID: {
            // The portal's shape is read and dropped.
            OCCLUSION_PLANE_DATA kPortalData;
            kPortalData.~OCCLUSION_PLANE_DATA();
            ::new (&kPortalData) OCCLUSION_PLANE_DATA;
            ourMemset(&kPortalData, 0, sizeof(OCCLUSION_PLANE_DATA));
            apFile->GetChunkData(&kPortalData, 0x24);
            if (apFile->QEndian()) {
                kPortalData.Endian();
            }
            break;
        }
        case XPRD_ID: {
            // The data is constructed through the empty constructor only, then cleared (see above).
            REF_PATROL_DATA kPatrolData;
            kPatrolData.~REF_PATROL_DATA();
            ::new (&kPatrolData) REF_PATROL_DATA;
            kPatrolData.fTimeAtRef = 0.0f;
            apFile->GetChunkData(&kPatrolData, 4);
            if (apFile->QEndian()) {
                kPatrolData.Endian();
            }
            void* pMemory = ourMalloc(sizeof(PatrolRefData));
            PatrolRefData* pPatrolRefData = pMemory ? ::new (pMemory) PatrolRefData(kPatrolData) : nullptr;
            SetPatrolRefData(pPatrolRefData);
            break;
        }
        case XORD_ID: {
            void* pMemory = ourMalloc(sizeof(OcclusionPlaneLinkedRefData));
            OcclusionPlaneLinkedRefData* pLinkedRefData =
                pMemory ? ::new (pMemory) OcclusionPlaneLinkedRefData : nullptr;
            apFile->GetChunkData(pLinkedRefData, 0x10);
#if XFNV_64BIT
            // The file's record is four 4-byte form IDs.
            TESFile::WidenFormIDRecords(pLinkedRefData->pLinkedRefs, 4, 4, sizeof(TESObjectREFR*), {{0, 0}});
#endif
            if (apFile->QEndian()) {
                pLinkedRefData->Endian();
            }
            SetOcclusionPlaneLinkedRefData(pLinkedRefData);
            break;
        }
        case XPOD_ID: {
            void* pMemory = ourMalloc(sizeof(PortalLinkedRefData));
            PortalLinkedRefData* pLinkedRefData = pMemory ? ::new (pMemory) PortalLinkedRefData : nullptr;
            apFile->GetChunkData(pLinkedRefData, 8);
#if XFNV_64BIT
            // The file's record is two 4-byte form IDs.
            TESFile::WidenFormIDRecords(pLinkedRefData->pLinkedRefs, 2, 4, sizeof(TESObjectREFR*), {{0, 0}});
#endif
            if (apFile->QEndian()) {
                pLinkedRefData->Endian();
            }
            SetPortalLinkedRefData(pLinkedRefData);
            break;
        }
        case XRMR_ID: {
            void* pMemory = ourMalloc(sizeof(RoomRefData));
            RoomRefData* pRoomRefData = pMemory ? ::new (pMemory) RoomRefData : nullptr;
            // The low word is the number of linked rooms that follow, the third byte the room count.
            uint32_t uiData = 0;
            apFile->GetChunkData<uint32_t>(uiData);
            pRoomRefData->ucRoomCount = static_cast<uint8_t>(uiData >> 16);
            uiData &= 0xFFFF;
            for (uint32_t i = 0; i < uiData; ++i) {
                uint32_t uiRoom = 0;
                apFile->NextChunk();
                apFile->GetChunkData<uint32_t>(uiRoom);
                TESObjectREFR* pRoom = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiRoom));
                pRoomRefData->kRoomList.AddHead(pRoom);
            }
            SetRoomLinkedRefData(pRoomRefData);
            break;
        }
        case XTRI_ID: {
            void* pMemory = ourMalloc(sizeof(CollisionData));
            CollisionData* pCollisionData = pMemory ? ::new (pMemory) CollisionData : nullptr;
            apFile->GetChunkData(pCollisionData, 4);
            if (apFile->QEndian()) {
                pCollisionData->Endian();
            }
            SetCollisionData(pCollisionData);
            break;
        }
        case XPPA_ID: {
            // The patrol's action chunks follow the marker chunk.
            PatrolRefData* pPatrolRefData = GetPatrolRefData();
            apFile->NextChunk();
            pPatrolRefData->OnPatrol.Load(apFile);
            break;
        }
        case XIBS_ID: SetIgnoredBySandbox(true); break;
        case XSRF_ID: {
            auto* pExtra = static_cast<ExtraSpecialRenderFlags*>(GetExtraData(EXTRA_SPECIAL_RENDER_FLAGS));
            if (!pExtra) {
                void* pMemory = ourMalloc(sizeof(ExtraSpecialRenderFlags));
                BSExtraData* pNew = pMemory ? ::new (pMemory) ExtraSpecialRenderFlags : nullptr;
                pExtra = static_cast<ExtraSpecialRenderFlags*>(AddExtra(pNew));
            }
            uint32_t uiFlags = 0;
            apFile->GetChunkData<uint32_t>(uiFlags);
            pExtra->SetFlags(uiFlags);
            break;
        }
        case XSRD_ID: {
            auto* pExtra = static_cast<ExtraSpecialRenderFlags*>(GetExtraData(EXTRA_SPECIAL_RENDER_FLAGS));
            if (!pExtra) {
                void* pMemory = ourMalloc(sizeof(ExtraSpecialRenderFlags));
                BSExtraData* pNew = pMemory ? ::new (pMemory) ExtraSpecialRenderFlags : nullptr;
                pExtra = static_cast<ExtraSpecialRenderFlags*>(AddExtra(pNew));
            }
            float fFadeDistance = 0.0f;
            apFile->GetChunkData<float>(fFadeDistance);
            pExtra->m_fNodeMaxFadeDistance = fFadeDistance;
            break;
        }
        default: break;
    }
}

uint32_t BSGetCurrentThreadID() {
    return GetCurrentThreadId();
}

// Every field is deliberately left unset, as the game does.
REF_DECAL_DATA::REF_DECAL_DATA() {}

// Swaps the decal reference and the hit point's and normal's floats of a record read from a big-endian file.
void REF_DECAL_DATA::Endian() {
    ENDIAN(pDecalRef, false);
    ENDIAN(Intersect.x, false);
    ENDIAN(Intersect.y, false);
    ENDIAN(Intersect.z, false);
    ENDIAN(Normal.x, false);
    ENDIAN(Normal.y, false);
    ENDIAN(Normal.z, false);
}

// Every field is deliberately left unset, as the game does.
OCCLUSION_PLANE_DATA::OCCLUSION_PLANE_DATA() {}

// Swaps the nine floats read from a big-endian file, in field order.
void OCCLUSION_PLANE_DATA::Endian() {
    ENDIAN(rHalfExtents.x, false);
    ENDIAN(rHalfExtents.y, false);
    ENDIAN(rCenter.x, false);
    ENDIAN(rCenter.y, false);
    ENDIAN(rCenter.z, false);
    ENDIAN(rAxis.x, false);
    ENDIAN(rAxis.y, false);
    ENDIAN(rAxis.z, false);
    ENDIAN(fAngle, false);
}

BSExtraData::BSExtraData(uint8_t acType) : cEtype(acType), pNext(nullptr) {}

bool BSExtraData::Compare(BSExtraData* apCompare) {
    if (!apCompare) {
        return true;
    }
    int32_t extraType = (int32_t)GetExtraType();
    return (bool)((uint8_t)(((uint32_t)extraType) != ((uint32_t)apCompare->GetExtraType())));
}

BaseExtraList::BaseExtraList() : pHead(nullptr) {
    ourMemset(iFlags, 0, sizeof(iFlags));
}

BaseExtraList::~BaseExtraList() {
    RemoveAll(true);
}

void BaseExtraList::RemoveExtra() {
    BSExtraData* pExtraData = GetExtraData(EXTRA_SAY_ONCE_A_DAY_TOPIC_INFO);
    if (pExtraData) {
        RemoveExtra(pExtraData, true);
    }
}

uint32_t BaseExtraList::ItemsInList() {
    ExtraCritSection.Lock("BaseExtraList::ItemsInList()");
    int32_t iCount = 0;
    for (BSExtraData* pItem = pHead; pItem; pItem = pItem->GetNext()) {
        ++iCount;
    }
    ExtraCritSection.Unlock();
    return iCount;
}

bool BaseExtraList::IsExtraListEmpty() const {
    return pHead == nullptr;
}

bool BaseExtraList::HasExtra(uint8_t acType) const {
    uint32_t uiByte = acType >> 3;
    if (uiByte >= sizeof(iFlags)) {
        return false;
    }
    return (iFlags[uiByte] & (1u << (acType & 7))) != 0;
}

void BaseExtraList::SetHasExtra(uint8_t acType, bool abVal) {
    uint32_t uiByte = acType >> 3;
    if (uiByte >= sizeof(iFlags)) {
        return;
    }
    uint32_t uiMask = 1u << (acType & 7);
    uint32_t uiBits = iFlags[uiByte];
    if (abVal) {
        uiBits |= uiMask;
    }
    else {
        uiBits &= ~uiMask;
    }
    iFlags[uiByte] = static_cast<uint8_t>(uiBits);
}

// Tests the presence bit, then this thread's cache, and only then walks the list under the lock, caching the hit.
BSExtraData* BaseExtraList::GetExtraData(uint8_t acType) {
    if (!HasExtra(acType)) {
        return nullptr;
    }
    BSExtraData* pFound = GetLastExtra(this, acType);
    if (!pFound) {
        ExtraCritSection.Lock("BaseExtraList::GetExtraData()");
        for (BSExtraData* pExtra = pHead; pExtra; pExtra = pExtra->GetNext()) {
            if (pExtra->GetExtraType() == acType) {
                SetLastExtra(this, pExtra);
                pFound = pExtra;
                break;
            }
        }
        ExtraCritSection.Unlock();
    }
    return pFound;
}

// The presence bit is cleared even when no item of aeCode was found.
void BaseExtraList::RemoveExtra(uint8_t aeCode) {
    ExtraCritSection.Lock("BaseExtraList::RemoveExtra()");
    BSExtraData* pPrev = nullptr;
    BSExtraData* pExtra = GetHead();
    while (pExtra && pExtra->GetExtraType() != aeCode) {
        pPrev = pExtra;
        pExtra = pExtra->GetNext();
    }
    if (pExtra) {
        if (pPrev) {
            pPrev->SetNext(pExtra->GetNext());
        }
        else {
            pHead = pExtra->GetNext();
        }
        ClearLastExtra(this, aeCode);
        delete pExtra;
    }
    SetHasExtra(aeCode, false);
    ExtraCritSection.Unlock();
}

void BaseExtraList::RemoveExtra(BSExtraData* apExtra, bool abDelete) {
    if (apExtra) {
        BaseExtraList::ExtraCritSection.Lock("BaseExtraList::RemoveExtra()");
        uint8_t ExtraType = apExtra->cEtype;
        uint32_t etype = ExtraType;
        BSExtraData* pprev = GetPrevExtraData(ExtraType);
        if (pprev) {
            pprev->pNext = apExtra->pNext;
        }
        else {
            pHead = apExtra->pNext;
        }
        BaseExtraList::ClearLastExtra(this, etype);
        if (abDelete) {
            delete apExtra;
        }
        else {
            apExtra->pNext = nullptr;
        }
        SetHasExtra(etype, false);
        ++BaseExtraList::iDirty;
        ++GetThreadDirty();
        BaseExtraList::ExtraCritSection.Unlock();
    }
}

BSExtraData* BaseExtraList::GetHead() {
    return pHead;
}

void BaseExtraList::RemoveAll(bool abDelete) {
    ExtraCritSection.Lock("BaseExtraList::RemoveAll()");
    ClearLastExtraAll(this);
    if (abDelete) {
        BSExtraData* pExtra = pHead;
        while (pExtra) {
            BSExtraData* pDelete = pExtra;
            pExtra = pExtra->GetNext();
            pHead = pExtra;
            if (abDelete) {
                delete pDelete;
            }
        }
    }
    else {
        pHead = nullptr;
    }
    ourMemset(iFlags, 0, sizeof(iFlags));
    ExtraCritSection.Unlock();
}

void BaseExtraList::RemoveAllDefault(bool abDelete) {
    BaseExtraList::ExtraCritSection.Lock("BaseExtraList::RemoveAllDefault()");
    BaseExtraList::ClearLastExtraAll(this);
    BSExtraData* pwork = pHead;
    BSExtraData* pprev = nullptr;
    while (pwork) {
        uint8_t cextraType = pwork->cEtype;
        bool bremove = true;
        switch (cextraType) {
            case 46:
            case 56:
            case 79:
            case 82:
            case 84:
            case 88:
            case 101:
            case 123:
            case 132: {
                bremove = false;
                break;
            }
            default: {
                break;
            }
        }
        BSExtraData* pnext = pwork->pNext;
        if (bremove) {
            if (pprev) {
                pprev->pNext = pnext;
            }
            else {
                pHead = pnext;
            }
            if (abDelete) {
                delete pwork;
            }
            SetHasExtra(cextraType, false);
        }
        else {
            pprev = pwork;
        }
        pwork = pnext;
    }
    BaseExtraList::ExtraCritSection.Unlock();
}

// A non-high-use item is linked after the tail without clearing its own next link.
BSExtraData* BaseExtraList::AddExtra(BSExtraData* apExtra) {
    ExtraCritSection.Lock("BaseExtraList::AddExtra()");
    if (IsHighUseExtra(apExtra->GetExtraType())) {
        if (pHead) {
            apExtra->SetNext(pHead);
        }
        pHead = apExtra;
    }
    else {
        BSExtraData* pTail = pHead;
        if (pTail) {
            while (pTail->GetNext()) {
                pTail = pTail->GetNext();
            }
            pTail->SetNext(apExtra);
        }
        else {
            pHead = apExtra;
        }
    }
    SetHasExtra(static_cast<EXTRA_DATA_TYPE>(apExtra->GetExtraType()), true);
    ExtraCritSection.Unlock();
    return apExtra;
}

BSExtraData* BaseExtraList::GetPrevExtraData(uint8_t acType) {
    if (!HasExtra(acType)) {
        return nullptr;
    }
    ExtraCritSection.Lock("BaseExtraList::GetPrevExtraData()");
    BSExtraData* pExtra = pHead;
    BSExtraData* pPrev = nullptr;
    while (pExtra) {
        if (pExtra->GetExtraType() == acType) {
            break;
        }
        pPrev = pExtra;
        pExtra = pExtra->GetNext();
    }
    ExtraCritSection.Unlock();
    return pPrev;
}

void BaseExtraList::RemoveCount() {
    RemoveExtra(EXTRA_COUNT);
}

void BaseExtraList::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// The cached item of aeType, or null when apList is not the cached list. The cache may be dropped by CheckDirty after
// the list compare; the list is deliberately not compared again, as the game does.
BSExtraData* BaseExtraList::GetLastExtra(BaseExtraList* apList, int32_t aeType) {
    BSExtraData* pExtra = nullptr;
    if (apList == GetThreadLastExtraList()) {
        CheckDirty();
        if (aeType >= 0 && aeType < EXTRA_DATATYPECOUNT) {
            pExtra = GetThreadLastExtraData()[aeType];
        }
    }
    return pExtra;
}

// Switching lists drops the whole cache. The type indexes the cache unchecked.
void BaseExtraList::SetLastExtra(BaseExtraList* apList, BSExtraData* apExtra) {
    if (apList && apExtra) {
        if (apList != GetThreadLastExtraList()) {
            ourMemset(GetThreadLastExtraData(), 0, sizeof(BSExtraData*) * EXTRA_DATATYPECOUNT);
            GetThreadLastExtraList() = apList;
        }
        BSExtraData** ppLastExtraData = GetThreadLastExtraData();
        ppLastExtraData[apExtra->GetExtraType()] = apExtra;
    }
}

// Invalidates every other thread's cache (iDirty) but keeps this thread's in step, then forgets this list's item
// of aeType.
void BaseExtraList::ClearLastExtra(BaseExtraList* apList, int32_t aeType) {
    ++iDirty;
    ++GetThreadDirty();
    if (apList == GetThreadLastExtraList() && aeType >= 0 && aeType < EXTRA_DATATYPECOUNT) {
        GetThreadLastExtraData()[aeType] = nullptr;
    }
}

void BaseExtraList::ClearLastExtraAll(BaseExtraList* apList) {
    ++iDirty;
    ++GetThreadDirty();
    if (apList == GetThreadLastExtraList()) {
        ourMemset(GetThreadLastExtraData(), 0, sizeof(BSExtraData*) * EXTRA_DATATYPECOUNT);
        GetThreadLastExtraList() = nullptr;
    }
}

void BaseExtraList::CheckDirty() {
    if (GetThreadDirty() != iDirty) {
        GetThreadLastExtraList() = nullptr;
        ourMemset(GetThreadLastExtraData(), 0, sizeof(BSExtraData*) * EXTRA_DATATYPECOUNT);
        GetThreadDirty() = iDirty;
    }
}

bool BaseExtraList::IsHighUseExtra(int32_t aeType) {
    switch (aeType) {
        case EXTRA_PERSISTENT_CELL:
        case EXTRA_SCRIPT:
        case EXTRA_ACTION:
        case EXTRA_CONTAINER_CHANGES:
        case EXTRA_TELEPORT: return true;
        default: return false;
    }
}

BaseExtraList*& BaseExtraList::GetThreadLastExtraList() {
    static thread_local BaseExtraList* pLastExtraList = nullptr;
    return pLastExtraList;
}

int32_t& BaseExtraList::GetThreadDirty() {
    static thread_local int32_t iThreadDirty = 0;
    return iThreadDirty;
}

BSExtraData** BaseExtraList::GetThreadLastExtraData() {
    static thread_local BSExtraData* ppLastExtraData[EXTRA_DATATYPECOUNT] = {};
    return ppLastExtraData;
}

ExtraDataList::ExtraDataList() {}

ExtraDataList::~ExtraDataList() {}

void ExtraDataList::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer, TESForm* apBaseObject) {
    TESObjectREFR* pref = apLoadGameBuffer->GetReference();
    Actor* pactor = apLoadGameBuffer->GetActor();
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[28])) {
        ExtraReferencePointer* pextraReferencePointer = static_cast<ExtraReferencePointer*>(GetExtraData(28));
        if (pextraReferencePointer) {
            TESObjectREFR* v30;
            if (pextraReferencePointer->pRef) {
                TESForm* FormByNumericID = TESForm::GetFormByNumericID(
                    static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraReferencePointer->pRef))
                );
                v30 = dynamic_cast<TESObjectREFR*>(FormByNumericID);
            }
            else {
                v30 = nullptr;
            }
            pextraReferencePointer->pRef = v30;
        }
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[13])) {
        ExtraScript* pscriptExtra = static_cast<ExtraScript*>(GetExtraData(13));
        if ((pscriptExtra) && (apBaseObject)) {
            Script* FormScript;
            TESScriptableForm* pscriptableForm = nullptr;
            if (pactor) {
                TESActorBase* pactorBase = pactor->GetActorBase();
                if (pactorBase) {
                    pscriptableForm = static_cast<TESScriptableForm*>(pactorBase);
                }
            }
            else {
                pscriptableForm = dynamic_cast<TESScriptableForm*>(apBaseObject);
            }
            if (!pscriptableForm ||
                (!pscriptExtra->pScript ||
                 (FormScript = pscriptableForm->pFormScript, FormScript != pscriptExtra->pScript))) {
                RemoveExtra(13);
            }
        }
    }
    if (pref) {
        if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[108])) {
            ExtraOpenCloseActivateRef* pextraActivateRef = static_cast<ExtraOpenCloseActivateRef*>(GetExtraData(108));
            if (pextraActivateRef) {
                TESObjectREFR* v29;
                if (pextraActivateRef->pActivateRef) {
                    TESForm* v5 = TESForm::GetFormByNumericID(
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraActivateRef->pActivateRef))
                    );
                    v29 = dynamic_cast<TESObjectREFR*>(v5);
                }
                else {
                    v29 = nullptr;
                }
                pextraActivateRef->pActivateRef = v29;
                if (!pextraActivateRef->pActivateRef) {
                    RemoveExtra(108);
                }
            }
        }
        if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[50])) {
            NonActorMagicCaster* pextraNonActorMagicCaster = static_cast<NonActorMagicCaster*>(GetExtraData(50));
            if (pextraNonActorMagicCaster) {
                TESObjectREFR* v26;
                MagicTarget* MagicTargetByNumericID;
                MagicItem* MagicItemByNumericID;
                if (pextraNonActorMagicCaster->pCurrentSpell) {
                    MagicItemByNumericID = MagicItem::GetMagicItemByNumericID(
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraNonActorMagicCaster->pCurrentSpell))
                    );
                }
                else {
                    MagicItemByNumericID = nullptr;
                }
                pextraNonActorMagicCaster->pCurrentSpell = MagicItemByNumericID;
                if (pextraNonActorMagicCaster->pDesiredTarget) {
                    MagicTargetByNumericID = MagicTarget::GetMagicTargetByNumericID(
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraNonActorMagicCaster->pDesiredTarget))
                    );
                }
                else {
                    MagicTargetByNumericID = nullptr;
                }
                pextraNonActorMagicCaster->pDesiredTarget = MagicTargetByNumericID;
                if (pextraNonActorMagicCaster->pCasterObject) {
                    TESForm* v6 = TESForm::GetFormByNumericID(
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraNonActorMagicCaster->pCasterObject))
                    );
                    v26 = dynamic_cast<TESObjectREFR*>(v6);
                }
                else {
                    v26 = nullptr;
                }
                pextraNonActorMagicCaster->pCasterObject = v26;
            }
        }
        if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[51])) {
            NonActorMagicTarget* pextraNonActorMagicTarget = static_cast<NonActorMagicTarget*>(GetExtraData(51));
            if (pextraNonActorMagicTarget) {
                TESObjectREFR* v25;
                if (pextraNonActorMagicTarget->pTargetObject) {
                    TESForm* v7 = TESForm::GetFormByNumericID(
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraNonActorMagicTarget->pTargetObject))
                    );
                    v25 = dynamic_cast<TESObjectREFR*>(v7);
                }
                else {
                    v25 = nullptr;
                }
                pextraNonActorMagicTarget->pTargetObject = v25;
                ActiveEffect::InitLoadActiveEffectList(apLoadGameBuffer, &pextraNonActorMagicTarget->lActiveEffectList);
            }
        }
        if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[57])) {
            ExtraItemDropper* pextraItemDropper = static_cast<ExtraItemDropper*>(GetExtraData(57));
            if (pextraItemDropper) {
                TESObjectREFR* v24;
                if (pextraItemDropper->pDropper) {
                    TESForm* v8 = TESForm::GetFormByNumericID(
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraItemDropper->pDropper))
                    );
                    v24 = dynamic_cast<TESObjectREFR*>(v8);
                }
                else {
                    v24 = nullptr;
                }
                pextraItemDropper->pDropper = v24;
                if (pextraItemDropper->pDropper) {
                    (&pextraItemDropper->pDropper->m_Extra)->AddDroppedItem(pref);
                }
                else {
                    RemoveExtra(57);
                }
            }
        }
        if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[85])) {
            ExtraTalkingActor* pextraTalkingActor = static_cast<ExtraTalkingActor*>(GetExtraData(85));
            if (pextraTalkingActor) {
                MobileObject* v23;
                if (pextraTalkingActor->pTalkObject) {
                    TESForm* v9 = TESForm::GetFormByNumericID(
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraTalkingActor->pTalkObject))
                    );
                    v23 = dynamic_cast<MobileObject*>(v9);
                }
                else {
                    v23 = nullptr;
                }
                pextraTalkingActor->pTalkObject = v23;
            }
        }
        if (pactor) {
            if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[25])) {
                ExtraPackage* pextraPackage = static_cast<ExtraPackage*>(GetExtraData(25));
                if (pextraPackage) {
                    TESObjectREFR* v21;
                    TESPackage* v22;
                    if (pextraPackage->pPack) {
                        TESForm* v10 = TESForm::GetFormByNumericID(
                            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraPackage->pPack))
                        );
                        v22 = dynamic_cast<TESPackage*>(v10);
                    }
                    else {
                        v22 = nullptr;
                    }
                    pextraPackage->pPack = v22;
                    if (pextraPackage->pTarg) {
                        TESForm* v11 = TESForm::GetFormByNumericID(
                            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraPackage->pTarg))
                        );
                        v21 = dynamic_cast<TESObjectREFR*>(v11);
                    }
                    else {
                        v21 = nullptr;
                    }
                    pextraPackage->pTarg = v21;
                    if (pextraPackage->pPack) {
                        if (pextraPackage->pPack->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_NONE) {
                            pextraPackage->pPack->CalculateProcedureType(pextraPackage->pTarg);
                        }
                    }
                    else {
                        RemoveExtra(25);
                    }
                }
            }
            if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[29])) {
                ExtraFollower* pextraFollower = static_cast<ExtraFollower*>(GetExtraData(29));
                if (pextraFollower) {
                    BSSimpleList<Actor*>* plist = pextraFollower->pActorlist;
                    BSSimpleList<Actor*>* plastList = nullptr;
                    while (plist && !plist->IsEmpty()) {
                        Actor* pForm = plist->m_item;
                        bool bremove = false;
                        if (pForm) {
                            TESForm* v12 =
                                TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm)));
                            pForm = dynamic_cast<Actor*>(v12);
                            if (pForm) {
                                plist->SetItem(pForm);
                            }
                            else {
                                bremove = true;
                            }
                        }
                        else {
                            bremove = true;
                        }
                        if (bremove) {
                            if (plastList) {
                                BSSimpleList<Actor*>* v16 = plist;
                                plastList->Remove(v16->m_item);
                                plist = plastList->m_pkNext;
                            }
                            else {
                                plist->RemoveHead();
                            }
                        }
                        else {
                            plastList = plist;
                            plist = plist->m_pkNext;
                        }
                    }
                    if (apLoadGameBuffer->GetVersion() < 14u) {
                        RemoveFollower(pPlayer);
                    }
                }
            }
            if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[112])) {
                ExtraPackageData* pextraPackageData = static_cast<ExtraPackageData*>(GetExtraData(112));
                if ((pextraPackageData) && (pextraPackageData->pActorPackageData)) {
                    TESPackage* ppackage = GetPackageExtra();
                    pextraPackageData->pActorPackageData->InitLoadGame(apLoadGameBuffer);
                }
            }
            if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[60])) {
                ExtraMerchantContainer* pextraMerchantContainer =
                    static_cast<ExtraMerchantContainer*>(GetExtraData(60));
                if (pextraMerchantContainer) {
                    TESObjectREFR* v20;
                    if (pextraMerchantContainer->pContainer) {
                        TESForm* v13 = TESForm::GetFormByNumericID(
                            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraMerchantContainer->pContainer))
                        );
                        v20 = dynamic_cast<TESObjectREFR*>(v13);
                    }
                    else {
                        v20 = nullptr;
                    }
                    pextraMerchantContainer->pContainer = v20;
                }
            }
            if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[26])) {
                ExtraTresPassPackage* pextraTresPassPackage = static_cast<ExtraTresPassPackage*>(GetExtraData(26));
                if ((pextraTresPassPackage) && (pextraTresPassPackage->pPack)) {
                    pextraTresPassPackage->pPack->InitLoadGame(apLoadGameBuffer);
                }
            }
            if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[70])) {
                ExtraHeadingTarget* pextraHeadingTarget = static_cast<ExtraHeadingTarget*>(GetExtraData(70));
                if (pextraHeadingTarget) {
                    TESObjectREFR* v19;
                    if (pextraHeadingTarget->pHeadingTarget) {
                        TESForm* v14 = TESForm::GetFormByNumericID(
                            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraHeadingTarget->pHeadingTarget))
                        );
                        v19 = dynamic_cast<TESObjectREFR*>(v14);
                    }
                    else {
                        v19 = nullptr;
                    }
                    pextraHeadingTarget->pHeadingTarget = v19;
                }
            }
            if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[95])) {
                ExtraDismemberedLimbs* pextraDismemberedLimbs = static_cast<ExtraDismemberedLimbs*>(GetExtraData(95));
                if (pextraDismemberedLimbs) {
                    TESForm* v18;
                    if (pextraDismemberedLimbs->pDeathObject) {
                        v18 = TESForm::GetFormByNumericID(
                            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraDismemberedLimbs->pDeathObject))
                        );
                    }
                    else {
                        v18 = nullptr;
                    }
                    pextraDismemberedLimbs->pDeathObject = v18;
                }
            }
            if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[137])) {
                ExtraAshPileRef* pextraAshPileRef = static_cast<ExtraAshPileRef*>(GetExtraData(137));
                if (pextraAshPileRef) {
                    TESObjectREFR* v17;
                    if (pextraAshPileRef->pAshPileRef) {
                        TESForm* v15 = TESForm::GetFormByNumericID(
                            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraAshPileRef->pAshPileRef))
                        );
                        v17 = dynamic_cast<TESObjectREFR*>(v15);
                    }
                    else {
                        v17 = nullptr;
                    }
                    pextraAshPileRef->pAshPileRef = v17;
                    if (pextraAshPileRef->pAshPileRef) {
                        (&pextraAshPileRef->pAshPileRef->m_Extra)->SetAshPileRef(pactor);
                    }
                }
            }
            if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[53])) {
                ExtraPlayerCrimeList* pextraPlayerCrimeList = static_cast<ExtraPlayerCrimeList*>(GetExtraData(53));
                if (pextraPlayerCrimeList) {
                    for (Crime* pcrime : *pextraPlayerCrimeList->pCrime) {
                        if (pcrime) {
                            pcrime->AddtoActorKnowList(pactor);
                        }
                    }
                }
            }
        }
        else {
            if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[43])) {
                ExtraTeleport* pextraTeleport = static_cast<ExtraTeleport*>(GetExtraData(43));
                if (pextraTeleport) {
                    pextraTeleport->pTeleportData->InitLoadGame(apLoadGameBuffer);
                }
            }
        }
    }
}

// Writes every extra the form's change flags call for, each as its type byte and its data, then their count.
void ExtraDataList::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint32_t iextraCount = 0;
    uint32_t icountPosition = apSaveGameBuffer->StartVariableSizedValue();
    TESObjectREFR* pref = apSaveGameBuffer->GetReference();
    Actor* pactor = apSaveGameBuffer->GetActor();
    bool bcreated;
    if (pref) {
        bcreated = pDataHandler->IsFormIDCreated(pref->iFormID);
    }
    else {
        bcreated = false;
    }
    BSExtraData* pExtra = pHead;
    while (pExtra) {
        uint8_t cextraType = pExtra->GetExtraType();
        bool bsaveExtra = false;
        if ((xDataTypeToChangeFlag[cextraType]) &&
            (apSaveGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[cextraType]))) {
            bsaveExtra = true;
        }

        // Extras whose change flag has bit 30 are saved only for created forms.
        if (!bcreated && bsaveExtra && (xDataTypeToChangeFlag[cextraType] & 0x40000000) != 0) {
            bsaveExtra = false;
        }
        if (!pactor && cextraType == EXTRA_ASHPILE_REF) {
            bsaveExtra = false;
        }
        if (bsaveExtra) {
            apSaveGameBuffer->SaveDataEndian(&cextraType, 1, 0);
            switch (cextraType) {
                case EXTRA_SCRIPT: {
                    auto* pextraScript = static_cast<ExtraScript*>(pExtra);
                    apSaveGameBuffer->SaveFormID(pextraScript->pScript, 0);
                    pextraScript->pScriptVars->SaveGame(apSaveGameBuffer);
                    break;
                }
                case EXTRA_WORN:
                case EXTRA_GHOST:
                case EXTRA_CANNOTWEAR:
                case EXTRA_AUDIOMARKER:
                case EXTRA_AUDIOBUOYMARKER: break;
                case EXTRA_PACKAGESTARTLOC: {
                    auto* pextraStartLoc = static_cast<ExtraPackageStartLocation*>(pExtra);
                    apSaveGameBuffer->SaveFormID(pextraStartLoc->worldLoc.pLocationForm, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraStartLoc->worldLoc.locPt, 12, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraStartLoc->worldLoc.fZRot, 4, 0);
                    break;
                }
                case EXTRA_PACKAGE: {
                    auto* pextraPackage = static_cast<ExtraPackage*>(pExtra);
                    apSaveGameBuffer->SaveFormID(pextraPackage->pPack, 0);
                    apSaveGameBuffer->SaveFormID(pextraPackage->pTarg, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraPackage->iindex, 4, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraPackage->bActionComplete, 1, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraPackage->bActivated, 1, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraPackage->bDoneOnce, 1, 0);
                    break;
                }
                case EXTRA_TRESPASS_PACKAGE: {
                    auto* pextraTrespassPackage = static_cast<ExtraTresPassPackage*>(pExtra);
                    apSaveGameBuffer->SaveFormID(pextraTrespassPackage->pPack, 0);
                    if (pextraTrespassPackage->pPack) {
                        pextraTrespassPackage->pPack->SaveGame(apSaveGameBuffer);
                    }
                    break;
                }
                case EXTRA_RUN_ONCE_PACKAGES: {
                    auto* pextraRunOncePackages = static_cast<ExtraRunOncePacks*>(pExtra);
                    uint32_t isaveGameCount = 0;
                    uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
                    for (BSSimpleList<RunOncePackage*>* plist = pextraRunOncePackages->pPackageList; plist;
                         plist = plist->m_pkNext) {
                        RunOncePackage* prunOncePackage = plist->m_item;
                        if (prunOncePackage) {
                            apSaveGameBuffer->SaveFormID(prunOncePackage->pPackage, 0);
                            apSaveGameBuffer->SaveDataEndian(&prunOncePackage->cday, 1, 0);
                            ++isaveGameCount;
                        }
                    }
                    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
                    break;
                }
                case EXTRA_REFERENCE_POINTER:
                    apSaveGameBuffer->SaveFormID(static_cast<ExtraReferencePointer*>(pExtra)->pRef, 0);
                    break;
                case EXTRA_FOLLOWER: {
                    auto* pextraFollower = static_cast<ExtraFollower*>(pExtra);
                    uint32_t isaveGameCount = 0;
                    uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
                    for (BSSimpleList<Actor*>* plist = pextraFollower->pActorlist; plist; plist = plist->m_pkNext) {
                        Actor* pForm = plist->m_item;
                        if (pForm) {
                            apSaveGameBuffer->SaveFormID(pForm, 0);
                            ++isaveGameCount;
                        }
                    }
                    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
                    break;
                }
                case EXTRA_LEVCREA_MOD:
                    apSaveGameBuffer->SaveDataEndian(&static_cast<ExtraLevCreaModifier*>(pExtra)->eModifier, 4, 0);
                    break;
                case EXTRA_OWNERSHIP:
                    apSaveGameBuffer->SaveFormID(static_cast<ExtraOwnership*>(pExtra)->pOwner, 0);
                    break;
                case EXTRA_GLOBAL: apSaveGameBuffer->SaveFormID(static_cast<ExtraGlobal*>(pExtra)->pGlobal, 0); break;
                case EXTRA_RANK: apSaveGameBuffer->SaveDataEndian(&static_cast<ExtraRank*>(pExtra)->iRank, 4, 0); break;
                case EXTRA_COUNT:
                    apSaveGameBuffer->SaveDataEndian(&static_cast<ExtraCount*>(pExtra)->iCount, 2, 0);
                    break;
                case EXTRA_HEALTH:
                    apSaveGameBuffer->SaveDataEndian(&static_cast<ExtraHealth*>(pExtra)->fHealth, 4, 0);
                    break;
                case EXTRA_USES: apSaveGameBuffer->SaveDataEndian(&static_cast<ExtraUses*>(pExtra)->cUses, 1, 0); break;
                case EXTRA_TIMELEFT:
                    apSaveGameBuffer->SaveDataEndian(&static_cast<ExtraTimeLeft*>(pExtra)->fTime, 4, 0);
                    break;
                case EXTRA_CHARGE:
                    apSaveGameBuffer->SaveDataEndian(&static_cast<ExtraCharge*>(pExtra)->fCharge, 4, 0);
                    break;
                case EXTRA_LOCK: {
                    auto* pextraLock = static_cast<ExtraLock*>(pExtra);
                    apSaveGameBuffer->SaveDataEndian(&pextraLock->pLock->cLockLevel, 1, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraLock->pLock->cFlags, 1, 0);
                    apSaveGameBuffer->SaveFormID(pextraLock->pLock->pKey, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraLock->pLock->iNumTries, 4, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraLock->pLock->iTimesUnlocked, 4, 0);
                    break;
                }
                case EXTRA_TELEPORT:
                    static_cast<ExtraTeleport*>(pExtra)->pTeleportData->SaveGame(apSaveGameBuffer);
                    break;
                case EXTRA_MAPMARKER: {
                    auto* pextraMapMarker = static_cast<ExtraMapMarker*>(pExtra);
                    uint8_t cFlags = 0;
                    if (pextraMapMarker->pMapData) {
                        cFlags = pextraMapMarker->pMapData->cFlags;
                    }
                    apSaveGameBuffer->SaveDataEndian(&cFlags, 1, 0);
                    break;
                }
                case EXTRA_LEVELEDCREATURE: {
                    auto* pleveledCreature = static_cast<ExtraLeveledCreature*>(pExtra);
                    apSaveGameBuffer->SaveFormID(pleveledCreature->pOriginalBase, 0);
                    apSaveGameBuffer->SaveFormID(pleveledCreature->pTemplate, 0);
                    TESActorBase* pactorBase = pactor->GetActorBase();
                    int32_t iactorBaseChangeFlags = static_cast<int32_t>(pactorBase->iChangeFlags);
                    apSaveGameBuffer->SaveDataEndian(&iactorBaseChangeFlags, 4, 0);
                    // The base form is saved under its own change flags, then the buffer's are put back.
                    BGSChangeFlags kOldFlags = apSaveGameBuffer->GetChangeFlags();
                    BGSChangeFlags kBaseFlags(iactorBaseChangeFlags);
                    apSaveGameBuffer->SetChangeFlags(kBaseFlags);
                    pactorBase->SaveGame(apSaveGameBuffer);
                    apSaveGameBuffer->SetChangeFlags(kOldFlags);
                    break;
                }
                case EXTRA_LEVELITEM: {
                    auto* pextraLeveledItem = static_cast<ExtraLeveledItem*>(pExtra);
                    apSaveGameBuffer->SaveDataEndian(&pextraLeveledItem->iIndex, 4, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraLeveledItem->bdefault, 1, 0);
                    break;
                }
                case EXTRA_SCALE:
                    apSaveGameBuffer->SaveDataEndian(&static_cast<ExtraScale*>(pExtra)->fScale, 4, 0);
                    break;
                case EXTRA_MAGICCASTER: {
                    auto* pextraNonActorMagicCaster = static_cast<NonActorMagicCaster*>(pExtra);
                    uint32_t aiFormID;
                    if (pextraNonActorMagicCaster->pCurrentSpell) {
                        aiFormID = pextraNonActorMagicCaster->pCurrentSpell->GetMagicItemFormID();
                    }
                    else {
                        aiFormID = 0;
                    }
                    apSaveGameBuffer->SaveFormID(aiFormID, 0);
                    uint32_t MagicTargetFormID;
                    if (pextraNonActorMagicCaster->pDesiredTarget) {
                        MagicTargetFormID = pextraNonActorMagicCaster->pDesiredTarget->GetMagicTargetFormID();
                    }
                    else {
                        MagicTargetFormID = 0;
                    }
                    apSaveGameBuffer->SaveFormID(MagicTargetFormID, 0);
                    apSaveGameBuffer->SaveFormID(pextraNonActorMagicCaster->pCasterObject, 0);
                    break;
                }
                case EXTRA_MAGICTARGET: {
                    auto* pextraNonActorMagicTarget = static_cast<NonActorMagicTarget*>(pExtra);
                    apSaveGameBuffer->SaveFormID(pextraNonActorMagicTarget->pTargetObject, 0);
                    ActiveEffect::SaveActiveEffectList(apSaveGameBuffer, &pextraNonActorMagicTarget->lActiveEffectList);
                    break;
                }
                case EXTRA_PLAYERCRIMELIST: {
                    auto* pextraPlayerCrimeList = static_cast<ExtraPlayerCrimeList*>(pExtra);
                    uint32_t isaveGameCount = 0;
                    uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
                    for (BSSimpleList<Crime*>* plist = pextraPlayerCrimeList->pCrime; plist; plist = plist->m_pkNext) {
                        Crime* pcrime = plist->m_item;
                        if (pcrime) {
                            uint32_t iCrimeNumber = pcrime->iCrimeNumber;
                            PackageNS::CRIME_TYPE ecrimeType = pcrime->ecrime;
                            apSaveGameBuffer->SaveDataEndian(&iCrimeNumber, 4, 0);
                            apSaveGameBuffer->SaveDataEndian(&ecrimeType, 4, 0);
                            ++isaveGameCount;
                        }
                    }
                    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
                    break;
                }
                case EXTRA_ITEMDROPPER:
                    apSaveGameBuffer->SaveFormID(static_cast<ExtraItemDropper*>(pExtra)->pDropper, 0);
                    break;
                case EXTRA_MERCHANTCONTAINER:
                    apSaveGameBuffer->SaveFormID(static_cast<ExtraMerchantContainer*>(pExtra)->pContainer, 0);
                    break;
                case EXTRA_POISON: {
                    auto* pextraPoison = static_cast<ExtraPoison*>(pExtra);
                    uint32_t MagicItemFormID;
                    if (pextraPoison->pPoison) {
                        MagicItemFormID = static_cast<MagicItem*>(pextraPoison->pPoison)->GetMagicItemFormID();
                    }
                    else {
                        MagicItemFormID = 0;
                    }
                    apSaveGameBuffer->SaveFormID(MagicItemFormID, 0);
                    break;
                }
                case EXTRA_FRIEND_HITS: {
                    auto* pextraFriendHits = static_cast<ExtraFriendHits*>(pExtra);
                    pextraFriendHits->RemoveOldHits();
                    BSSimpleArray<CombatTimeStamp>* pHits = &pextraFriendHits->Hits;
                    uint32_t isaveGameCount = pHits ? pHits->iSize : 0;
                    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
                    for (uint32_t i = 0; i < isaveGameCount; ++i) {
                        CombatTimeStamp* ptimeStamp = &pextraFriendHits->Hits[i];
                        ptimeStamp->SaveGame(apSaveGameBuffer);
                    }
                    break;
                }
                case EXTRA_HEAD_TRACK_TARGET:
                    apSaveGameBuffer->SaveFormID(static_cast<ExtraHeadingTarget*>(pExtra)->pHeadingTarget, 0);
                    break;
                case EXTRA_STARTINGWORLDORCELL:
                    apSaveGameBuffer->SaveFormID(
                        static_cast<ExtraStartingWorldOrCell*>(pExtra)->pStartingWorldOrCell, 0
                    );
                    break;
                case EXTRA_HOT_KEY:
                    apSaveGameBuffer->SaveDataEndian(&static_cast<ExtraHotKey*>(pExtra)->chotkey, 1, 0);
                    break;
                case EXTRA_INFO_GENERAL_TOPIC:
                    static_cast<ExtraInfoGeneralTopic*>(pExtra)->pInfoGen->SaveGame_0(apSaveGameBuffer);
                    break;
                case EXTRA_NO_RUMORS:
                    apSaveGameBuffer->SaveDataEndian(&static_cast<ExtraHasNoRumors*>(pExtra)->bNoRumors, 1, 0);
                    break;
                case EXTRA_TERMINALSTATE: {
                    auto* pextraTerminalState = static_cast<ExtraTerminalState*>(pExtra);
                    apSaveGameBuffer->SaveDataEndian(&pextraTerminalState->cState, 1, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraTerminalState->cLockOverride, 1, 0);
                    break;
                }
                case EXTRA_ACTIVATE_REF_CHILDREN:
                    apSaveGameBuffer->SaveDataEndian(
                        &static_cast<ExtraActivateRefChildren*>(pExtra)->fActivateChildrenTimer, 4, 0
                    );
                    break;
                case EXTRA_TALKING_ACTOR:
                    apSaveGameBuffer->SaveFormID(static_cast<ExtraTalkingActor*>(pExtra)->pTalkObject, 0);
                    break;
                case EXTRA_OBJECT_HEALTH:
                    apSaveGameBuffer->SaveDataEndian(&static_cast<ExtraObjectHealth*>(pExtra)->fHealth, 4, 0);
                    break;
                case EXTRA_MODEL_SWAP: {
                    auto* pextraModelSwap = static_cast<ExtraModelSwap*>(pExtra);
                    apSaveGameBuffer->SaveFormID(pextraModelSwap->pModelSwapForm, 0);
                    int32_t ModelSwapIndex = BGSDestructibleObjectForm::GetModelSwapIndex(
                        pextraModelSwap->pModelSwapForm, pextraModelSwap->pModelSwap
                    );
                    apSaveGameBuffer->SaveDataEndian(&ModelSwapIndex, 4, 0);
                    break;
                }
                case EXTRA_RADIUS:
                    apSaveGameBuffer->SaveDataEndian(&static_cast<ExtraRadius*>(pExtra)->fRadius, 4, 0);
                    break;
                case EXTRA_RADIATION:
                    apSaveGameBuffer->SaveDataEndian(&static_cast<ExtraRadiation*>(pExtra)->fRadiation, 4, 0);
                    break;
                case EXTRA_FACTION_CHANGES: {
                    auto* pextraFactionChanges = static_cast<ExtraFactionChanges*>(pExtra);
                    uint32_t isaveGameCount = 0;
                    uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
                    for (BSSimpleList<FACTION_RANK*>* plist = pextraFactionChanges->pFactionChanges; plist;
                         plist = plist->m_pkNext) {
                        FACTION_RANK* pfactionRank = plist->m_item;
                        if (pfactionRank) {
                            apSaveGameBuffer->SaveFormID(pfactionRank->pFaction, 0);
                            apSaveGameBuffer->SaveDataEndian(&pfactionRank->cRank, 1, 0);
                            ++isaveGameCount;
                        }
                    }
                    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
                    break;
                }
                case EXTRA_DISMEMBERED_LIMBS: {
                    auto* pextraDismemberedLimbs = static_cast<ExtraDismemberedLimbs*>(pExtra);
                    apSaveGameBuffer->SaveDataEndian(&pextraDismemberedLimbs->sLimbs, 2, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraDismemberedLimbs->eCauseofDeath, 4, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraDismemberedLimbs->eLastHitLimb, 4, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraDismemberedLimbs->bEaten, 1, 0);
                    apSaveGameBuffer->SaveFormID(pextraDismemberedLimbs->pDeathObject, 0);
                    BSSimpleArray<DismemberedLimb*>* pLimbs = &pextraDismemberedLimbs->DismemberedLimbs;
                    uint32_t isaveGameCount = pLimbs ? pLimbs->GetSize() : 0;
                    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
                    for (uint32_t j = 0; j < isaveGameCount; ++j) {
                        DismemberedLimb** ppLimb = &pextraDismemberedLimbs->DismemberedLimbs[j];
                        apSaveGameBuffer->SaveDataEndian(&(*ppLimb)->cLimb, 1, 0);
                        apSaveGameBuffer->SaveDataEndian(&(*ppLimb)->bLimbExploded, 1, 0);
                        apSaveGameBuffer->SaveDataEndian(&(*ppLimb)->bObjectArrayIdentical, 1, 0);
                        apSaveGameBuffer->SaveDataEndian(&(*ppLimb)->bLimbRemoved, 1, 0);
                        BSSimpleArray<TESBoundObject*>* pObjects = &(*ppLimb)->ObjectArray;
                        uint32_t iobjectCount = pObjects ? pObjects->GetSize() : 0;
                        apSaveGameBuffer->SaveVariableSizedValue(iobjectCount);
                        for (uint32_t k = 0; k < iobjectCount; ++k) {
                            TESBoundObject** ppForm = &(*ppLimb)->ObjectArray[k];
                            apSaveGameBuffer->SaveFormID(*ppForm, 0);
                        }
                    }
                    break;
                }
                case EXTRA_ACTOR_CAUSE: {
                    auto* pextraActorCause = static_cast<ExtraActorCause*>(pExtra);
                    int32_t iactorCauseID;
                    if (pextraActorCause->spActorCause.m_pObject) {
                        iactorCauseID = static_cast<int32_t>(pextraActorCause->spActorCause->iActorCauseID);
                    }
                    else {
                        iactorCauseID = -1;
                    }
                    apSaveGameBuffer->SaveDataEndian(&iactorCauseID, 4, 0);
                    break;
                }
                case EXTRA_OPENCLOSEACTIVATE_REF:
                    apSaveGameBuffer->SaveFormID(static_cast<ExtraOpenCloseActivateRef*>(pExtra)->pActivateRef, 0);
                    break;
                case EXTRA_AMMO: {
                    auto* pextraAmmo = static_cast<ExtraAmmo*>(pExtra);
                    apSaveGameBuffer->SaveFormID(pextraAmmo->pAmmo, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraAmmo->iCount, 4, 0);
                    break;
                }
                case EXTRA_PACKAGE_DATA: {
                    auto* pextraPackageData = static_cast<ExtraPackageData*>(pExtra);
                    PACKAGE_TYPE epackDataType;
                    if (pextraPackageData->pActorPackageData) {
                        epackDataType = pextraPackageData->pActorPackageData->GetPackageType();
                    }
                    else {
                        epackDataType = PACKAGE_TYPE_NONE;
                    }
                    uint8_t cpackDataType = static_cast<uint8_t>(epackDataType);
                    apSaveGameBuffer->SaveDataEndian(&cpackDataType, 1, 0);
                    if (pextraPackageData->pActorPackageData) {
                        pextraPackageData->pActorPackageData->SaveGame(apSaveGameBuffer);
                    }
                    break;
                }
                case EXTRA_SAY_ONCE_A_DAY_TOPIC_INFO: {
                    auto* pextraTopicInfo = static_cast<ExtraSayTopicInfoOnceADay*>(pExtra);
                    uint32_t isaveGameCount = 0;
                    uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
                    for (BSSimpleList<SayOnceTopicInfos*>* plist = pextraTopicInfo->pListofSaidOnceTopicInfos; plist;
                         plist = plist->m_pkNext) {
                        SayOnceTopicInfos* psayOnceTopicInfo = plist->m_item;
                        if (psayOnceTopicInfo) {
                            apSaveGameBuffer->SaveFormID(psayOnceTopicInfo->pInfo, 0);
                            apSaveGameBuffer->SaveDataEndian(&psayOnceTopicInfo->iDateStamp, 4, 0);
                            apSaveGameBuffer->SaveDataEndian(&psayOnceTopicInfo->fHourStamp, 4, 0);
                            ++isaveGameCount;
                        }
                    }
                    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
                    break;
                }
                case EXTRA_ENCOUNTERZONE:
                    apSaveGameBuffer->SaveFormID(static_cast<ExtraEncounterZone*>(pExtra)->pZone, 0);
                    break;
                case EXTRA_SAY_TO_TOPIC_INFO: {
                    auto* pextraSayToTopicInfo = static_cast<ExtraSayToTopicInfo*>(pExtra);
                    apSaveGameBuffer->SaveFormID(pextraSayToTopicInfo->pTopic, 0);
                    apSaveGameBuffer->SaveFormID(pextraSayToTopicInfo->pInfo, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraSayToTopicInfo->bVoiceDone, 1, 0);
                    break;
                }
                case EXTRA_GUARDED_REF_DATA: {
                    auto* pextraGuardedRefData = static_cast<ExtraGuardedRefData*>(pExtra);
                    BSSimpleArray<uint32_t>* pGuards = &pextraGuardedRefData->Guards;
                    uint32_t auiSize = pGuards ? pGuards->iSize : 0;
                    apSaveGameBuffer->SaveVariableSizedValue(auiSize);
                    for (uint32_t m = 0; m < auiSize; ++m) {
                        uint32_t* iguardID = &pextraGuardedRefData->Guards[m];
                        apSaveGameBuffer->SaveFormID(*iguardID, 0);
                    }
                    break;
                }
                case EXTRA_ASHPILE_REF:
                    apSaveGameBuffer->SaveFormID(static_cast<ExtraAshPileRef*>(pExtra)->pAshPileRef, 0);
                    break;
                case EXTRA_FOLLOWER_SWIM_BREADCRUMBS: {
                    auto* pextraCrumbs = static_cast<ExtraFollowerSwimBreadcrumbs*>(pExtra);
                    apSaveGameBuffer->SaveDataEndian(&pextraCrumbs->LeaderLocation, 12, 0);
                    apSaveGameBuffer->SaveFormID(pextraCrumbs->LeaderNavMeshID, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraCrumbs->eLeaderState, 4, 0);
                    uint32_t isaveGameCount = 0;
                    uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
                    for (BSSimpleList<ExtraFollowerSwimBreadcrumb*>* plist = &pextraCrumbs->CrumbList; plist;
                         plist = plist->m_pkNext) {
                        ExtraFollowerSwimBreadcrumb* pcrumb = plist->m_item;
                        if (pcrumb) {
                            apSaveGameBuffer->SaveDataEndian(&pcrumb->StartLocation, 12, 0);
                            apSaveGameBuffer->SaveFormID(pcrumb->StartNavMeshID, 0);
                            apSaveGameBuffer->SaveDataEndian(&pcrumb->EndLocation, 12, 0);
                            apSaveGameBuffer->SaveFormID(pcrumb->EndNavMeshID, 0);
                            apSaveGameBuffer->SaveDataEndian(&pcrumb->bEnteringWater, 1, 0);
                            ++isaveGameCount;
                        }
                    }
                    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
                    break;
                }
                case EXTRA_WEAPON_MOD_SLOTS:
                    apSaveGameBuffer->SaveDataEndian(
                        &static_cast<ExtraWeaponModFlags*>(pExtra)->cWeaponModsActive, 1, 0
                    );
                    break;
                case EXTRA_SECURITRON_FACE: {
                    auto* pFaceExtra = static_cast<ExtraSecuritronFace*>(pExtra);
                    apSaveGameBuffer->SaveString(pFaceExtra->GetPersonality().pString, 0);
                    apSaveGameBuffer->SaveString(pFaceExtra->GetMood().pString, 0);
                    break;
                }
                case EXTRA_SPECIAL_RENDER_FLAGS: {
                    auto* pextraRenderFlags = static_cast<ExtraSpecialRenderFlags*>(pExtra);
                    uint32_t uiFlags = pextraRenderFlags->m_flags;
                    apSaveGameBuffer->SaveDataEndian(&uiFlags, 4, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraRenderFlags->m_fNodeMaxFadeDistance, 4, 0);
                    break;
                }
                default:
                    SaveGameWarning(
                        "SAVELOAD: Unknown extra data type %i found in ExtraDataList::SaveGame", cextraType
                    );
                    break;
            }
            ++iextraCount;
        }
        pExtra = pExtra->GetNext();
    }
    apSaveGameBuffer->SaveVariableSizedValue(iextraCount, icountPosition);
}

// The save keeps form IDs in the pointer fields and list items; this resolves them to the forms.
void ExtraDataList::InitLoadGameTES(
    uint32_t aiFlags,
    uint32_t aiCurrentFlags,
    TESObjectREFR* apRef,
    TESBoundObject* apBaseObject
) {
    bool bIsActor = false;
    if (apRef) {
        bIsActor = apRef->IsActor();
    }
    if (bIsActor) {
        ExtraPackage* pextraPackage = static_cast<ExtraPackage*>(GetExtraData(25));
        if (pextraPackage) {
            uint32_t CastingType = pextraPackage->pPack->iFormID;
            if (pDataHandler->IsFormIDCreated(CastingType)) {
                pextraPackage->pPack->InitLoadGame();
            }
            if (pextraPackage->pTarg) {
                uint32_t itargetID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraPackage->pTarg));
                TESForm* FormByNumericID = TESForm::GetFormByNumericID(itargetID);
                pextraPackage->pTarg = dynamic_cast<TESObjectREFR*>(FormByNumericID);
            }
            pextraPackage->pPack->CalculateProcedureType(pextraPackage->pTarg);
        }
        ExtraFollower* pfollow = GetFollowerExtra();
        if (pfollow) {
            BSSimpleList<Actor*>* plist = pfollow->pActorlist;
            while (plist && !plist->IsEmpty()) {
                Actor* pactor = nullptr;
                uint32_t iactorID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(plist->m_item));
                if (iactorID) {
                    TESForm* v7 = TESForm::GetFormByNumericID(iactorID);
                    pactor = dynamic_cast<Actor*>(v7);
                }
                if (pactor) {
                    plist->SetItem(pactor);
                    plist = plist->m_pkNext;
                }
                else {
                    plist->RemoveHead();
                }
            }
        }
    }
    else {
        if ((aiFlags & CHANGE_DOOR_EXTRA_TELEPORT) != 0) {
            DoorTeleportData* pteleport = GetTeleport();
            pteleport->FinishInitLoadGame();
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 70u) {
        BSSimpleList<TESObjectREFR*>* plist_2 = GetDroppedItemList();
        BSSimpleList<TESObjectREFR*>* plastList = nullptr;
        while (plist_2 && !plist_2->IsEmpty()) {
            TESObjectREFR* pref = plist_2->m_item;
            uint32_t irefID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pref));
            if (pref) {
                TESForm* v8 = TESForm::GetFormByNumericID(irefID);
                pref = dynamic_cast<TESObjectREFR*>(v8);
            }
            if (pref) {
                plist_2->SetItem(pref);
                (&pref->m_Extra)->SetItemDropper(apRef);
                plastList = plist_2;
                plist_2 = plist_2->m_pkNext;
            }
            else {
                if (plastList) {
                    TESObjectREFR* pstaleItem = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(irefID));
                    plastList->Remove(pstaleItem);
                    plist_2 = plastList->m_pkNext;
                }
                else {
                    plist_2->RemoveHead();
                    if (plist_2->IsEmpty()) {
                        plist_2 = nullptr;
                    }
                }
            }
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 70u) {
        uint32_t idropperID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetItemDropper()));
        if (idropperID) {
            TESForm* v9 = TESForm::GetFormByNumericID(idropperID);
            TESObjectREFR* pref_2 = dynamic_cast<TESObjectREFR*>(v9);
            SetItemDropper(pref_2);
            if (pref_2) {
                (&pref_2->m_Extra)->AddDroppedItem(apRef);
            }
        }
    }
    TESObjectREFR* ptarget = GetHeadTrackTargetExtra();
    if (ptarget) {
        uint32_t itargetID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ptarget));
        TESForm* v10 = TESForm::GetFormByNumericID(itargetID);
        ptarget = dynamic_cast<TESObjectREFR*>(v10);
        SetHeadTrackTargetExtra(ptarget);
    }
}

void ExtraDataList::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags, TESObjectREFR* apRef) {
    bool pActor = false;
    if (apRef) {
        pActor = apRef->IsActor();
    }
    if (!pActor && (aiFlags & CHANGE_DOOR_EXTRA_TELEPORT) != 0) {
        DoorTeleportData* pteleport = GetTeleport();
    }
}

// KEEP: the exe writes stack bytes it never stores into the form buffer: the XESP chunk is 8 bytes from [ebp-0xD8]
// (push 8 at 0x413086) of which only the dword and the byte at +4 are written, and likewise XPSL's fZRot (0x412FB4),
// XNDP's last two bytes (0x4136DC) and XLOC's padding (0x412A7A). An uninitialised read: unregistered.
// Saves each extra's chunks, under the extra data lock. Form pointers are saved as their form IDs (the exe's getter
// 0x84E3A0 is a trivial accessor). The exe's cleanup-only C++ EH frame (the XATO strings') is left out.
void ExtraDataList::Save() {
    BaseExtraList::ExtraCritSection.Lock(nullptr);
    for (BSExtraData* pExtra = pHead; pExtra; pExtra = pExtra->pNext) {
        switch (pExtra->cEtype) {
            case EXTRA_WATERTYPE:
                TESForm::AddChunk(XCWT_ID, static_cast<ExtraCellWaterType*>(pExtra)->pWater->iFormID);
                break;
            case EXTRA_REGIONLIST: {
                BSSimpleList<TESRegion*>* pList = static_cast<ExtraRegionList*>(pExtra)->pList;
                uint32_t iSize = pList->ItemsInList();
                uint32_t* pArray = nullptr;
                int32_t i = 0;
                if (iSize) {
                    pArray = static_cast<uint32_t*>(
                        ourMalloc(iSize > SIZE_MAX / sizeof(uint32_t) ? SIZE_MAX : sizeof(uint32_t) * iSize)
                    );
                    for (; pList; pList = pList->m_pkNext) {
                        pArray[i++] = pList->m_item->iFormID;
                    }
                    TESForm::AddChunkArray(XCLR_ID, pArray, iSize);
                    ourFree(pArray);
                }
                break;
            }
            case EXTRA_CELLMUSICTYPE:
                TESForm::AddChunk(XCMO_ID, static_cast<ExtraCellMusicType*>(pExtra)->pType->iFormID);
                break;
            case EXTRA_CLIMATE:
                TESForm::AddChunk(XCCM_ID, static_cast<ExtraCellClimate*>(pExtra)->pClimate->iFormID);
                break;
            case EXTRA_ACTION:
                TESForm::AddChunk(XACT_ID, static_cast<uint32_t>(static_cast<ExtraAction*>(pExtra)->eAction));
                break;
            case EXTRA_DISTANTDATA: {
                NiPoint3 kNormal = static_cast<ExtraDistantData*>(pExtra)->kNormal;
                TESForm::AddChunkArray(XLOD_ID, &kNormal.x, 3u);
                break;
            }
            case EXTRA_RAGDOLLDATA: static_cast<ExtraRagDollData*>(pExtra)->pRagDollData->Save(); break;
            case EXTRA_PACKAGESTARTLOC: {
                ExtraPackageStartLocation* pStart = static_cast<ExtraPackageStartLocation*>(pExtra);
                WORLD_LOCATION saveData;
                saveData.pLocationForm =
                    reinterpret_cast<TESForm*>(static_cast<uintptr_t>(pStart->worldLoc.pLocationForm->iFormID));
                saveData.locPt = pStart->worldLoc.locPt;
                if (TESForm::QEndianSwapOnSave()) {
                    saveData.Endian();
                }
                TESForm::__AddChunkData(XPSL_ID, &saveData, 20u);
                if (TESForm::QEndianSwapOnSave()) {
                    saveData.Endian();
                }
                break;
            }
            case EXTRA_LEVCREA_MOD:
                TESForm::AddChunk(
                    XLCM_ID, static_cast<uint32_t>(static_cast<ExtraLevCreaModifier*>(pExtra)->eModifier)
                );
                break;
            case EXTRA_OWNERSHIP:
                TESForm::AddChunk(XOWN_ID, static_cast<ExtraOwnership*>(pExtra)->pOwner->iFormID);
                break;
            case EXTRA_GLOBAL: TESForm::AddChunk(XGLB_ID, static_cast<ExtraGlobal*>(pExtra)->pGlobal->iFormID); break;
            case EXTRA_RANK:
                TESForm::AddChunk(XRNK_ID, static_cast<uint32_t>(static_cast<ExtraRank*>(pExtra)->iRank));
                break;
            case EXTRA_COUNT: {
                // The count is sign-extended.
                int32_t iCount = static_cast<ExtraCount*>(pExtra)->iCount;
                TESForm::AddChunk(XCNT_ID, static_cast<uint32_t>(iCount));
                break;
            }
            case EXTRA_HEALTH: {
                int32_t iHealth = static_cast<int32_t>(static_cast<ExtraHealth*>(pExtra)->fHealth);
                TESForm::AddChunk(XHLT_ID, static_cast<uint32_t>(iHealth));
                break;
            }
            case EXTRA_USES:
                TESForm::AddChunk(XUSE_ID, static_cast<uint32_t>(static_cast<ExtraUses*>(pExtra)->cUses));
                break;
            case EXTRA_TIMELEFT: {
                float fTime = static_cast<ExtraTimeLeft*>(pExtra)->fTime;
                TESForm::AddChunk(XTIM_ID, fTime);
                break;
            }
            case EXTRA_CHARGE: {
                float fCharge = static_cast<ExtraCharge*>(pExtra)->fCharge;
                TESForm::AddChunk(XCHG_ID, fCharge);
                break;
            }
            case EXTRA_LOCK: {
                LockData* pLock = static_cast<ExtraLock*>(pExtra)->pLock;
                LockData data;
                data.cFlags = pLock->cFlags;
                data.cLockLevel = pLock->cLockLevel;
                if (pLock->pKey) {
                    data.pKey = reinterpret_cast<TESKey*>(static_cast<uintptr_t>(pLock->pKey->iFormID));
                }
                else {
                    data.pKey = nullptr;
                }
                if (TESForm::QEndianSwapOnSave()) {
                    data.Endian();
                }
                TESForm::__AddChunkData(XLOC_ID, &data, 20u);
                if (TESForm::QEndianSwapOnSave()) {
                    data.Endian();
                }
                break;
            }
            case EXTRA_TELEPORT: static_cast<ExtraTeleport*>(pExtra)->pTeleportData->Save(); break;
            case EXTRA_MAPMARKER:
                TESForm::AddChunk(XMRK_ID);
                static_cast<ExtraMapMarker*>(pExtra)->pMapData->Save();
                break;
            case EXTRA_SEED: {
                uint8_t iSeed = GetSeedIndex();
                TESForm::AddChunk(XSED_ID, iSeed);
                break;
            }
            case EXTRA_ENABLESTATEPARENT: {
                ExtraEnableStateParent* pParentExtra = static_cast<ExtraEnableStateParent*>(pExtra);
                if (pParentExtra->pParent) {
                    REF_ENABLE_PARENT_DATA data;
                    data.pParent =
                        reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(pParentExtra->pParent->iFormID));
                    data.cFlags = pParentExtra->cFlags;
                    if (TESForm::QEndianSwapOnSave()) {
                        data.Endian();
                    }
                    TESForm::__AddChunkData(XESP_ID, &data, 8u);
                    if (TESForm::QEndianSwapOnSave()) {
                        data.Endian();
                    }
                }
                break;
            }
            case EXTRA_TELEPORTMARKER: {
                TESObjectREFR* pMarker = static_cast<ExtraRandomTeleportMarker*>(pExtra)->pMarker;
                if (pMarker) {
                    TESForm::AddChunk(XRTM_ID, pMarker->iFormID);
                }
                break;
            }
            case EXTRA_MERCHANTCONTAINER: {
                TESObjectREFR* pContainer = static_cast<ExtraMerchantContainer*>(pExtra)->pContainer;
                if (pContainer) {
                    TESForm::AddChunk(XMRC_ID, pContainer->iFormID);
                }
                break;
            }
            case EXTRA_POISON: TESForm::AddChunk(XPSN_ID, static_cast<ExtraPoison*>(pExtra)->pPoison->iFormID); break;
            case EXTRA_XTARGET: {
                TESObjectREFR* pTarget = static_cast<ExtraXTarget*>(pExtra)->pTarget;
                if (pTarget) {
                    TESForm::AddChunk(XTRG_ID, pTarget->iFormID);
                }
                break;
            }
            case EXTRA_LINKED_REF: {
                TESObjectREFR* pLinkedRef = static_cast<ExtraLinkedRef*>(pExtra)->pLinkedRef;
                if (pLinkedRef) {
                    TESForm::AddChunk(XLKR_ID, pLinkedRef->iFormID);
                }
                break;
            }
            case EXTRA_ACTIVATE_REF: {
                ExtraActivateRef* pActivate = static_cast<ExtraActivateRef*>(pExtra);
                TESForm::AddChunk(XAPD_ID, pActivate->cActivateFlags);
                // The next node is fetched before the item is saved.
                BSSimpleList<REF_ACTIVATE_DATA*>* pList = &pActivate->ParentList;
                while (pList && !pList->IsEmpty()) {
                    REF_ACTIVATE_DATA* pData = pList->m_item;
                    pList = pList->m_pkNext;
                    REF_ACTIVATE_DATA saveData;
                    ourMemcpy(&saveData, pData, sizeof(REF_ACTIVATE_DATA));
                    // The form ID is taken from the copy's reference.
                    saveData.pActivateRef =
                        reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(saveData.pActivateRef->iFormID));
                    if (TESForm::QEndianSwapOnSave()) {
                        saveData.Endian();
                    }
                    TESForm::__AddChunkData(XAPR_ID, &saveData, 8u);
                    if (TESForm::QEndianSwapOnSave()) {
                        saveData.Endian();
                    }
                }
                if (pActivate->ActivateTextOverride.GetLength()) {
                    // Two copies of the text: the length is taken from the first, the characters from the second.
                    BSStringT<char> kLengthText = GetActivateTextOverride();
                    uint32_t iLength = kLengthText.GetLength() + 1;
                    BSStringT<char> kText = GetActivateTextOverride();
                    TESForm::AddChunkArray(XATO_ID, kText.pString, iLength);
                }
                break;
            }
            case EXTRA_DECAL_REFS: {
                BSSimpleList<REF_DECAL_DATA*>* pList = &static_cast<ExtraDecalRefs*>(pExtra)->DecalRefList;
                for (; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
                    REF_DECAL_DATA* pData = pList->m_item;
                    REF_DECAL_DATA saveData;
                    ourMemcpy(&saveData, pData, sizeof(REF_DECAL_DATA));
                    // The form ID is taken from the list's item.
                    saveData.pDecalRef =
                        reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(pData->pDecalRef->iFormID));
                    if (TESForm::QEndianSwapOnSave()) {
                        saveData.Endian();
                    }
                    TESForm::__AddChunkData(XDCR_ID, &saveData, 28u);
                    if (TESForm::QEndianSwapOnSave()) {
                        saveData.Endian();
                    }
                }
                break;
            }
            case EXTRA_IMAGESPACE:
                TESForm::AddChunk(XCIM_ID, static_cast<ExtraCellImageSpace*>(pExtra)->pImageSpace->iFormID);
                break;
            case EXTRA_NAVMESH_PORTAL: {
                NAVMESH_PORTAL* pPortal = &static_cast<ExtraNavMeshPortal*>(pExtra)->Portal;
                NAVMESH_PORTAL saveData(*pPortal);
                if (pPortal->pNavMesh) {
                    saveData.NavMeshID = pPortal->pNavMesh->iFormID;
                    if (TESForm::QEndianSwapOnSave()) {
                        saveData.Endian();
                    }
                    TESForm::__AddChunkData(XNDP_ID, &saveData, 8u);
                    if (TESForm::QEndianSwapOnSave()) {
                        saveData.Endian();
                    }
                }
                break;
            }
            case EXTRA_RADIUS: TESForm::AddChunk(XRDS_ID, static_cast<ExtraRadius*>(pExtra)->fRadius); break;
            case EXTRA_RADIATION: TESForm::AddChunk(XRAD_ID, static_cast<ExtraRadiation*>(pExtra)->fRadiation); break;
            case EXTRA_MULTIBOUND: TESForm::AddChunk(XMBP_ID); break;
            case EXTRA_MULTIBOUND_DATA: {
                MultiBoundMarkerData* pBound = static_cast<ExtraMultiBoundData*>(pExtra)->pBound;
                if (pBound) {
                    pBound->Save();
                }
                break;
            }
            case EXTRA_MULTIBOUND_REF: {
                TESObjectREFR* pBoundRef = static_cast<ExtraMultiBoundRef*>(pExtra)->pBoundRef;
                if (pBoundRef) {
                    TESForm::AddChunk(XMBR_ID, pBoundRef->iFormID);
                }
                break;
            }
            case EXTRA_REFLECTOR_REFS: {
                BSSimpleList<REF_REFLECTOR_DATA*>* pList = &static_cast<ExtraReflectorRefs*>(pExtra)->RefList;
                for (; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
                    REF_REFLECTOR_DATA* pData = pList->m_item;
                    REF_REFLECTOR_DATA saveData;
                    ourMemcpy(&saveData, pData, sizeof(REF_REFLECTOR_DATA));
                    // The form ID is taken from the list's item.
                    saveData.pRef = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(pData->pRef->iFormID));
                    if (TESForm::QEndianSwapOnSave()) {
                        saveData.Endian();
                    }
                    TESForm::__AddChunkData(XPWR_ID, &saveData, 8u);
                    if (TESForm::QEndianSwapOnSave()) {
                        saveData.Endian();
                    }
                }
                break;
            }
            case EXTRA_EMITTANCE_SOURCE: {
                TESForm* pSource = static_cast<ExtraEmittanceSource*>(pExtra)->pSource;
                if (pSource) {
                    TESForm::AddChunk(XEMI_ID, pSource->iFormID);
                }
                break;
            }
            case EXTRA_RADIO_DATA: {
                // The extra's own data is saved: its reference is swapped for its form ID, then put back.
                RADIO_DATA* pData = &static_cast<ExtraRadioData*>(pExtra)->Data;
                TESObjectREFR* pRef = pData->pPositionRef;
                if (pRef) {
                    pData->pPositionRef = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(pRef->iFormID));
                }
                if (TESForm::QEndianSwapOnSave()) {
                    pData->Endian();
                }
                TESForm::__AddChunkData(XRDO_ID, pData, 16u);
                if (TESForm::QEndianSwapOnSave()) {
                    pData->Endian();
                }
                pData->pPositionRef = pRef;
                break;
            }
            case EXTRA_PRIMITIVE: {
                PRIMITIVE_DATA saveData;
                BGSPrimitive* pPrimitive = static_cast<ExtraPrimitive*>(pExtra)->pPrimitive;
                saveData.aRadii = pPrimitive->GetRadii();
                saveData.aColor = pPrimitive->GetColor();
                saveData.eType = pPrimitive->eType;
                if (TESForm::QEndianSwapOnSave()) {
                    saveData.Endian();
                }
                TESForm::__AddChunkData(XPRM_ID, &saveData, 32u);
                if (TESForm::QEndianSwapOnSave()) {
                    saveData.Endian();
                }
                break;
            }
            case EXTRA_AMMO: {
                ExtraAmmo* pAmmoExtra = static_cast<ExtraAmmo*>(pExtra);
                int32_t iCount = pAmmoExtra->iCount;
                uint32_t iAmmoID = pAmmoExtra->pAmmo ? pAmmoExtra->pAmmo->iFormID : 0;
                TESForm::AddChunk(XAMT_ID, iAmmoID);
                TESForm::AddChunk(XAMC_ID, static_cast<uint32_t>(iCount));
                break;
            }
            case EXTRA_PATROL_REF_DATA: {
                ExtraPatrolRefData* pPatrolExtra = static_cast<ExtraPatrolRefData*>(pExtra);
                REF_PATROL_DATA saveData;
                saveData.SetTimeAtRef(&pPatrolExtra->pPatrolData->fTimeAtRef);
                if (TESForm::QEndianSwapOnSave()) {
                    saveData.Endian();
                }
                TESForm::__AddChunkData(XPRD_ID, &saveData, 4u);
                if (TESForm::QEndianSwapOnSave()) {
                    saveData.Endian();
                }
                TESPackageOnBeginEndOrChange_Save(&pPatrolExtra->pPatrolData->OnPatrol, nullptr, XPPA_ID);
                break;
            }
            case EXTRA_OCCLUSION_PLANE: {
                OCCLUSION_PLANE_DATA saveData;
                BSOcclusionPlane* pPlane = static_cast<ExtraOcclusionPlane*>(pExtra)->spPlane;
                saveData.rHalfExtents = pPlane->kHalfExtents;
                saveData.rCenter = pPlane->kCenter;
                pPlane->QRotation().ExtractAngleAndAxis(
                    saveData.fAngle, saveData.rAxis.x, saveData.rAxis.y, saveData.rAxis.z
                );
                if (TESForm::QEndianSwapOnSave()) {
                    saveData.Endian();
                }
                TESForm::__AddChunkData(XOCP_ID, &saveData, 36u);
                if (TESForm::QEndianSwapOnSave()) {
                    saveData.Endian();
                }
                break;
            }
            case EXTRA_COLLISION_DATA: {
                ExtraCollisionData* pCollision = static_cast<ExtraCollisionData*>(pExtra);
                if (TESForm::QEndianSwapOnSave()) {
                    pCollision->pCollisionData->Endian();
                }
                TESForm::__AddChunkData(XTRI_ID, pCollision->pCollisionData, 4u);
                if (TESForm::QEndianSwapOnSave()) {
                    pCollision->pCollisionData->Endian();
                }
                break;
            }
            case EXTRA_ENCOUNTERZONE: {
                BGSEncounterZone* pZone = static_cast<ExtraEncounterZone*>(pExtra)->pZone;
                if (pZone) {
                    TESForm::AddChunk(XEZN_ID, pZone->iFormID);
                }
                break;
            }
            case EXTRA_OCCLUSION_PLANE_REF_DATA: {
                ExtraOcclusionPlaneRefData* pRefData = static_cast<ExtraOcclusionPlaneRefData*>(pExtra);
                OcclusionPlaneLinkedRefData saveData;
                for (int32_t i = 0; i < 4; ++i) {
                    TESObjectREFR* pLinked;
                    if (pRefData->pData->pLinkedRefs[i]) {
                        pLinked = reinterpret_cast<TESObjectREFR*>(
                            static_cast<uintptr_t>(pRefData->pData->pLinkedRefs[i]->iFormID)
                        );
                    }
                    else {
                        pLinked = nullptr;
                    }
                    saveData.pLinkedRefs[i] = pLinked;
                }
                if (TESForm::QEndianSwapOnSave()) {
                    saveData.Endian();
                }
                TESForm::__AddChunkData(XORD_ID, &saveData, 16u);
                if (TESForm::QEndianSwapOnSave()) {
                    saveData.Endian();
                }
                break;
            }
            case EXTRA_PORTAL_REF_DATA: {
                ExtraPortalRefData* pRefData = static_cast<ExtraPortalRefData*>(pExtra);
                PortalLinkedRefData saveData;
                for (int32_t i = 0; i < 2; ++i) {
                    TESObjectREFR* pLinked;
                    if (pRefData->pData->pLinkedRefs[i]) {
                        pLinked = reinterpret_cast<TESObjectREFR*>(
                            static_cast<uintptr_t>(pRefData->pData->pLinkedRefs[i]->iFormID)
                        );
                    }
                    else {
                        pLinked = nullptr;
                    }
                    saveData.pLinkedRefs[i] = pLinked;
                }
                if (TESForm::QEndianSwapOnSave()) {
                    saveData.Endian();
                }
                TESForm::__AddChunkData(XPOD_ID, &saveData, 8u);
                if (TESForm::QEndianSwapOnSave()) {
                    saveData.Endian();
                }
                break;
            }
            case EXTRA_PORTAL: {
                OCCLUSION_PLANE_DATA saveData;
                BSPortal* pPortal = static_cast<ExtraPortal*>(pExtra)->spPortal;
                saveData.rHalfExtents = pPortal->kHalfExtents;
                saveData.rCenter = pPortal->kCenter;
                pPortal->QRotation().ExtractAngleAndAxis(
                    saveData.fAngle, saveData.rAxis.x, saveData.rAxis.y, saveData.rAxis.z
                );
                if (TESForm::QEndianSwapOnSave()) {
                    saveData.Endian();
                }
                TESForm::__AddChunkData(XPTL_ID, &saveData, 36u);
                if (TESForm::QEndianSwapOnSave()) {
                    saveData.Endian();
                }
                break;
            }
            case EXTRA_HEALTH_PERC:
                TESForm::AddChunk(XHLP_ID, static_cast<ExtraHealthPerc*>(pExtra)->fHealthPerc);
                break;
            case EXTRA_ROOM_REF_DATA: {
                ExtraRoomRefData* pRoomExtra = static_cast<ExtraRoomRefData*>(pExtra);
                uint32_t iItems = static_cast<uint32_t>(pRoomExtra->pData->kRoomList.ItemsInList());
                if (pRoomExtra->pData->ucRoomCount) {
                    iItems |= 0x10000u;
                }
                TESForm::AddChunk(XRMR_ID, iItems);
                // The walk stops at the first empty item.
                for (BSSimpleList<TESObjectREFR*>* pList = &pRoomExtra->pData->kRoomList; pList && pList->m_item;
                     pList = pList->m_pkNext) {
                    TESForm::AddChunk(XLRM_ID, pList->m_item->iFormID);
                }
                break;
            }
            case EXTRA_IGNORED_BY_SANDBOX: TESForm::AddChunk(XIBS_ID); break;
            case EXTRA_CELL_ACOUSTIC_SPACE:
                TESForm::AddChunk(XCAS_ID, static_cast<ExtraCellAcousticSpace*>(pExtra)->pSpace->iFormID);
                break;
            case EXTRA_LIT_WATER_REFS: {
                BSSimpleList<TESObjectREFR*>* pList = &static_cast<ExtraLitWaterRefs*>(pExtra)->RefList;
                for (; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
                    TESForm::AddChunk(XLTW_ID, pList->m_item->iFormID);
                }
                break;
            }
            case EXTRA_WEAPON_MOD_SLOTS: {
                uint8_t cFlags = static_cast<ExtraWeaponModFlags*>(pExtra)->cWeaponModsActive;
                TESForm::AddChunk(XHLT_ID, cFlags);
                break;
            }
            case EXTRA_AUDIOMARKER:
                TESForm::AddChunk(MMRK_ID);
                static_cast<ExtraAudioMarker*>(pExtra)->pAudioData->Save();
                break;
            case EXTRA_AUDIOBUOYMARKER: TESForm::AddChunk(AMRK_ID); break;
            case EXTRA_SPECIAL_RENDER_FLAGS: {
                ExtraSpecialRenderFlags* pFlags = static_cast<ExtraSpecialRenderFlags*>(pExtra);
                TESForm::AddChunk(XSRF_ID, pFlags->m_flags);
                TESForm::AddChunk(XSRD_ID, pFlags->m_fNodeMaxFadeDistance);
                break;
            }
            default: break;
        }
    }
    BaseExtraList::ExtraCritSection.Unlock();
}

void ExtraDataList::SaveGame(uint32_t aiFlags, TESObjectREFR* apRef) {
    uint32_t v18;
    uint32_t v20;
    uint32_t CastingType;
    uint8_t clength;
    uint32_t iactorID;
    uint32_t ilocationID;
    uint32_t ipackageID;
    uint8_t cpackType;
    uint32_t itargetID;
    uint32_t ipackID;
    uint32_t a2;
    uint8_t cextratype;
    uint32_t iblockHeader;
    uint16_t icount;
    uint16_t iblockSize;
    iblockSize = 0;
    // The save buffer position where the block's 16-bit size goes, filled in at the end.
    void* pblockSize = nullptr;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // 'BLOK'
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
        pblockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
    }
    BaseExtraList::ExtraCritSection.Lock(nullptr);
    icount = 0;
    uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&icount, 2u);
    for (BSExtraData* work = pHead; work; work = work->pNext) {
        cextratype = work->cEtype;
        void* ptempBuffer = pSaveLoadGameOLD->m_pBuffer;
        switch (work->cEtype) {
            case EXTRA_PERSISTENT_CELL: {
                if ((apRef) && (apRef->IsActor())) {
                    pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, 1u);
                    ExtraPersistentCell* ppersistentCell = dynamic_cast<ExtraPersistentCell*>(work);
                    CastingType = 0;
                    if (ppersistentCell->pPersistentCell) {
                        TESWorldSpace* WorldSpace = ppersistentCell->pPersistentCell->GetWorldSpace();
                        CastingType = WorldSpace->iFormID;
                    }
                    pSaveLoadGameOLD->SaveNumericID(&CastingType, 4u);
                }

                break;
            }
            case EXTRA_PACKAGESTARTLOC: {
                pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, 1u);
                ExtraPackageStartLocation* pstartLocation = dynamic_cast<ExtraPackageStartLocation*>(work);
                ilocationID = 0;
                if (pstartLocation->worldLoc.pLocationForm) {
                    ilocationID = pstartLocation->worldLoc.pLocationForm->iFormID;
                }
                else {
                    const char* v8 = apRef->GetFormEditorID();
                    int32_t v3 = apRef->iFormID;
                    Warning("SAVELOAD: Saving package start location on %08X %s, but the location is NULL.", v3, v8);
                }
                pSaveLoadGameOLD->SaveNumericID(&ilocationID, 4u);
                pSaveLoadGameOLD->SaveGameDataOLD(&pstartLocation->worldLoc.locPt, 12u);
                pSaveLoadGameOLD->SaveGameDataOLD(&pstartLocation->worldLoc.fZRot, 4u);
                break;
            }
            case EXTRA_PACKAGE: {
                pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, 1u);
                ExtraPackage* ppackage = dynamic_cast<ExtraPackage*>(work);
                ipackID = ppackage->pPack->iFormID;
                itargetID = 0;
                if (ppackage->pTarg) {
                    itargetID = ppackage->pTarg->iFormID;
                }
                pSaveLoadGameOLD->SaveNumericID(&ipackID, 4u);
                pSaveLoadGameOLD->SaveGameDataOLD(&ppackage->iindex, 4u);
                pSaveLoadGameOLD->SaveNumericID(&itargetID, 4u);
                pSaveLoadGameOLD->SaveGameDataOLD(&ppackage->bActionComplete, 1u);
                pSaveLoadGameOLD->SaveGameDataOLD(&ppackage->bActivated, 1u);
                if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u && pDataHandler->IsFormIDCreated(ipackID)) {
                    cpackType = ppackage->pPack->GetPackType();
                    pSaveLoadGameOLD->SaveGameDataOLD(&cpackType, 1u);
                    ppackage->pPack->SaveGame();
                }
                break;
            }
            case EXTRA_RUN_ONCE_PACKAGES: {
                uint16_t icount_2;
                pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, 1u);
                icount_2 = 0;
                uint16_t* pcount_2 = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
                pSaveLoadGameOLD->SaveGameDataOLD(&icount_2, 2u);
                ExtraRunOncePacks* ppackages = dynamic_cast<ExtraRunOncePacks*>(work);
                for (BSSimpleList<RunOncePackage*>* plist = ppackages->pPackageList; plist && !plist->IsEmpty();
                     plist = plist->m_pkNext) {
                    RunOncePackage* ppackage_2 = plist->m_item;
                    if (ppackage_2) {
                        ipackageID = ppackage_2->pPackage->iFormID;
                        pSaveLoadGameOLD->SaveNumericID(&ipackageID, 4u);
                        pSaveLoadGameOLD->SaveGameDataOLD(&ppackage_2->cday, 1u);
                        ++icount_2;
                    }
                }
                *pcount_2 = icount_2;
                break;
            }
            case EXTRA_FOLLOWER: {
                uint16_t icount_3;
                pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, 1u);
                ExtraFollower* pfollow = dynamic_cast<ExtraFollower*>(work);
                icount_3 = 0;
                uint16_t* pcount_3 = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
                pSaveLoadGameOLD->SaveGameDataOLD(&icount_3, 2u);
                for (BSSimpleList<Actor*>* plist_2 = pfollow->pActorlist; plist_2 && !plist_2->IsEmpty();
                     plist_2 = plist_2->m_pkNext) {
                    Actor* pactor = plist_2->m_item;
                    if (pactor) {
                        iactorID = pactor->iFormID;
                        pSaveLoadGameOLD->SaveNumericID(&iactorID, 4u);
                        ++icount_3;
                    }
                }
                *pcount_3 = icount_3;
                break;
            }
            case EXTRA_GHOST: {
                pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, 1u);
                break;
            }
            case EXTRA_LOCK: {
                if ((aiFlags & CHANGE_OBJECT_EXTRA_LOCK) != 0) {
                    pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, 1u);
                    // Reads the lock without a dynamic cast: type 42 is ExtraLock.
                    LockData* plock = static_cast<ExtraLock*>(work)->pLock;
                    a2 = 0;
                    if (plock->pKey) {
                        a2 = plock->pKey->iFormID;
                    }
                    pSaveLoadGameOLD->SaveGameDataOLD(&plock->cLockLevel, 1u);
                    pSaveLoadGameOLD->SaveNumericID(&a2, 4u);
                    pSaveLoadGameOLD->SaveGameDataOLD(&plock->cFlags, 1u);
                }
                break;
            }
            case EXTRA_TELEPORT: {
                if ((aiFlags & CHANGE_DOOR_EXTRA_TELEPORT) != 0) {
                    pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, 1u);
                    ExtraTeleport* pteleport = dynamic_cast<ExtraTeleport*>(work);
                    pteleport->pTeleportData->SaveGame();
                }
                break;
            }
            case EXTRA_ITEMDROPPER: {
                pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, 1u);
                ExtraItemDropper* pdropper = dynamic_cast<ExtraItemDropper*>(work);
                v20 = 0;
                if (pdropper->pDropper) {
                    v20 = pdropper->pDropper->iFormID;
                }
                pSaveLoadGameOLD->SaveNumericID(&v20, 4u);
                break;
            }
            case EXTRA_LASTFINISHEDSEQUENCE: {
                if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
                    pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, 1u);
                    ExtraLastFinishedSequence* plast = dynamic_cast<ExtraLastFinishedSequence*>(work);
                    clength = BSstrlen(plast->pLastSequenceName);
                    pSaveLoadGameOLD->SaveGameDataOLD(&clength, 1u);
                    pSaveLoadGameOLD->SaveGameDataOLD(plast->pLastSequenceName, clength);
                }
                break;
            }
            case EXTRA_HEAD_TRACK_TARGET: {
                pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, 1u);
                ExtraHeadingTarget* pheading = dynamic_cast<ExtraHeadingTarget*>(work);
                v18 = 0;
                if (pheading->pHeadingTarget) {
                    v18 = pheading->pHeadingTarget->iFormID;
                }
                pSaveLoadGameOLD->SaveNumericID(&v18, 4u);
                break;
            }
            case EXTRA_INFO_GENERAL_TOPIC: {
                ExtraInfoGeneralTopic* pinfoGeneralTopic = dynamic_cast<ExtraInfoGeneralTopic*>(work);
                if (pinfoGeneralTopic->pInfoGen) {
                    pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, 1u);
                    pinfoGeneralTopic->pInfoGen->SaveGame();
                }
                break;
            }
            case EXTRA_NO_RUMORS: {
                pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, 1u);
                ExtraHasNoRumors* phasNoRumors = dynamic_cast<ExtraHasNoRumors*>(work);
                pSaveLoadGameOLD->SaveGameDataOLD(&phasNoRumors->bNoRumors, 1u);
                break;
            }
            default: {
                break;
            }
        }
        void* BufferPointer = pSaveLoadGameOLD->m_pBuffer;
        if (ptempBuffer != BufferPointer) {
            ++icount;
        }
    }
    *pcount = icount;
    // 11295 is the source line number this check reports, as the game does.
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* v13 = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* v6 = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                v13 - itempStartPosition, pheader->iFormID, v6, iFlags, 11295,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s", v13 - itempStartPosition, 11295,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pblockEnd = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pblockStart = static_cast<uint8_t*>(pblockSize);
        if (pblockEnd > pblockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp", 11295
            );
        }
        *static_cast<uint16_t*>(pblockSize) = static_cast<uint16_t>(pblockEnd - pblockStart);
    }
    BaseExtraList::ExtraCritSection.Unlock();
}

// Reads the extra data of an old-format save (TESSaveLoadGame): a count, then per extra its type byte and data. The
// form IDs LoadNumericID reads are passed on as the pointers the setters take: the raw ID is kept and resolved later.
// The change flags of many types are tested against 0, so their branches never run; they are written out deliberately,
// as the game has them. Types 18, 46, 69, 144 and 145 read nothing and warn nothing.
void ExtraDataList::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags, TESObjectREFR* apRef) {
    uint16_t usSize = 0;
    char* pStart = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t uiMarker;
        pSaveLoadGameOLD->LoadGameDataOLD(&uiMarker, 4);
        if (uiMarker != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                uint32_t uiFlags = pHeader->iFlags;
                uint8_t ucVersion = pHeader->cVersion;
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp", 11308,
                    pHeader->iFormID, pForm->GetFormEditorID(), ucVersion, uiFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp", 11308,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->LoadGameDataOLD(&usSize, 2);
    }
    bool bScriptLoaded = false;
    // Casts the reference and deliberately leaves the result unused, as the game does.
    [[maybe_unused]] Actor* pActor = dynamic_cast<Actor*>(apRef);
    uint16_t usCount;
    pSaveLoadGameOLD->LoadGameDataOLD(&usCount, 2);
    for (int32_t i = 0; i < usCount; ++i) {
        uint8_t ucType;
        pSaveLoadGameOLD->LoadGameDataOLD(&ucType, 1);
        switch (ucType) {
            case EXTRA_COUNT:
                if ((aiFlags & 0) != 0) {
                    uint16_t usItemCount;
                    pSaveLoadGameOLD->LoadGameDataOLD(&usItemCount, 2);
                    SetCount(usItemCount);
                }
                break;
            case EXTRA_HEALTH:
                if ((aiFlags & 0) != 0) {
                    float fHealth;
                    pSaveLoadGameOLD->LoadGameDataOLD(&fHealth, 4);
                    SetHealth(fHealth);
                }
                break;
            case EXTRA_USES:
                if ((aiFlags & 0) != 0) {
                    uint8_t ucUses;
                    pSaveLoadGameOLD->LoadGameDataOLD(&ucUses, 1);
                    SetUses(ucUses);
                }
                break;
            case EXTRA_WEAPON_MOD_SLOTS:
                if ((aiFlags & 0) != 0) {
                    uint8_t ucModFlags;
                    pSaveLoadGameOLD->LoadGameDataOLD(&ucModFlags, 1);
                    SetWeaponModFlags(ucModFlags);
                }
                break;
            case EXTRA_TIMELEFT:
                if ((aiFlags & 0) != 0) {
                    float fTimeLeft;
                    pSaveLoadGameOLD->LoadGameDataOLD(&fTimeLeft, 4);
                    SetTimeLeft(fTimeLeft);
                }
                break;
            case EXTRA_CHARGE:
                if ((aiFlags & 0) != 0) {
                    float fCharge;
                    pSaveLoadGameOLD->LoadGameDataOLD(&fCharge, 4);
                    SetCharge(fCharge);
                }
                break;
            case EXTRA_LOCK:
                if ((aiFlags & CHANGE_OBJECT_EXTRA_LOCK) != 0) {
                    void* pMemory = ourMalloc(sizeof(LockData));
                    LockData* pLock = pMemory ? ::new (pMemory) LockData : nullptr;
                    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x15) {
                        pSaveLoadGameOLD->LoadGameDataOLD(&pLock->cLockLevel, 1);
                        uint32_t uiKeyID;
                        pSaveLoadGameOLD->LoadNumericID(&uiKeyID, 4);
                        if (pSaveLoadGameOLD->GetCurrentVersion() < 0x1A) {
                            // Older saves hold a second form ID that is read and dropped.
                            uint32_t uiUnusedID;
                            pSaveLoadGameOLD->LoadNumericID(&uiUnusedID, 4);
                        }
                        pSaveLoadGameOLD->LoadGameDataOLD(&pLock->cFlags, 1);
                        if (uiKeyID) {
                            pLock->pKey = dynamic_cast<TESKey*>(TESForm::GetFormByNumericID(uiKeyID));
                        }
                    }
                    SetLockPtr(pLock);
                }
                break;
            case EXTRA_SCALE:
                if (pSaveLoadGameOLD->GetCurrentVersion() < 0x43 && (aiFlags & CHANGE_REFR_SCALE) != 0) {
                    float fScale;
                    pSaveLoadGameOLD->LoadGameDataOLD(&fScale, 4);
                    if (apRef) {
                        apRef->SetScale(fScale);
                    }
                    else {
                        SetScale(fScale);
                    }
                }
                if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x43 && (aiFlags & 0) != 0) {
                    float fScale;
                    pSaveLoadGameOLD->LoadGameDataOLD(&fScale, 4);
                    SetScale(fScale);
                }
                break;
            case EXTRA_OWNERSHIP:
                if ((aiFlags & 0) != 0) {
                    uint32_t uiOwnerID;
                    pSaveLoadGameOLD->LoadNumericID(&uiOwnerID, 4);
                    TESForm* pOwner = TESForm::GetFormByNumericID(uiOwnerID);
                    if (pOwner) {
                        SetOwnership(pOwner);
                    }
                }
                break;
            case EXTRA_GLOBAL:
                if ((aiFlags & 0) != 0) {
                    uint32_t uiGlobalID;
                    pSaveLoadGameOLD->LoadNumericID(&uiGlobalID, 4);
                    TESGlobal* pGlobal = dynamic_cast<TESGlobal*>(TESForm::GetFormByNumericID(uiGlobalID));
                    if (pGlobal) {
                        SetGlobal(pGlobal);
                    }
                }
                break;
            case EXTRA_RANK:
                if ((aiFlags & 0) != 0) {
                    int32_t iRank;
                    pSaveLoadGameOLD->LoadGameDataOLD(&iRank, 4);
                    SetRank(iRank);
                }
                break;
            case EXTRA_WORN:
                if ((aiFlags & 0) != 0) {
                    SetWorn(true, false);
                }
                break;
            case EXTRA_WORN_LEFT:
                if ((aiFlags & 0) != 0) {
                    SetWorn(true, true);
                }
                break;
            case EXTRA_CANNOTWEAR:
                if ((aiFlags & 0) != 0) {
                    SetCanNotWear(true);
                }
                break;
            case EXTRA_GHOST: SetGhost(true); break;
            case EXTRA_SCRIPT:
                if ((aiFlags & 0) != 0) {
                    bScriptLoaded = true;
                    uint32_t uiScriptID;
                    pSaveLoadGameOLD->LoadNumericID(&uiScriptID, 4);
                    Script* pScript = dynamic_cast<Script*>(TESForm::GetFormByNumericID(uiScriptID));
                    if (pScript) {
                        SetScript(pScript);
                    }
                    ScriptLocals* pLocals = GetScriptLocals();
                    if (!pLocals) {
                        if (pScript) {
                            pLocals = pScript->CloneLocals();
                        }
                        SetScriptLocals(pLocals);
                    }
                    if (pLocals) {
                        pLocals->LoadGame();
                    }
                    else {
                        // No locals to load into: the saved ones are read into a scratch set and dropped.
                        void* pMemory = ourMalloc(sizeof(ScriptLocals));
                        ScriptLocals* pScratch = pMemory ? ::new (pMemory) ScriptLocals : nullptr;
                        pScratch->LoadGame();
                        if (pScratch) {
                            delete pScratch;
                        }
                    }
                }
                break;
            case EXTRA_LEVELITEM:
                if ((aiFlags & 0) != 0) {
                    int32_t iIndex;
                    pSaveLoadGameOLD->LoadGameDataOLD(&iIndex, 4);
                    uint8_t ucLevelFlag;
                    pSaveLoadGameOLD->LoadGameDataOLD(&ucLevelFlag, 1);
                    SetLevelItemExtra(iIndex);
                    SetLevelItemExtraFlag(ucLevelFlag);
                }
                break;
            case EXTRA_PACKAGE: {
                uint32_t uiPackageID;
                pSaveLoadGameOLD->LoadNumericID(&uiPackageID, 4);
                int32_t iIndex;
                pSaveLoadGameOLD->LoadGameDataOLD(&iIndex, 4);
                uint32_t uiTargetID;
                pSaveLoadGameOLD->LoadNumericID(&uiTargetID, 4);
                // Bytes are passed on to bool parameters as read, as the game does.
                bool bComplete;
                pSaveLoadGameOLD->LoadGameDataOLD(&bComplete, 1);
                bool bActivated;
                pSaveLoadGameOLD->LoadGameDataOLD(&bActivated, 1);
                TESPackage* pPackage = nullptr;
                if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x40 && pDataHandler->IsFormIDCreated(uiPackageID)) {
                    uint8_t ucPackType;
                    pSaveLoadGameOLD->LoadGameDataOLD(&ucPackType, 1);
                    pPackage = pSaveLoadGameOLD->CreatePackage(uiPackageID, ucPackType);
                    pPackage->LoadGame();
                }
                if (!pPackage) {
                    pPackage = dynamic_cast<TESPackage*>(TESForm::GetFormByNumericID(uiPackageID));
                }
                if (pPackage) {
                    SetPackageExtra(
                        pPackage, iIndex, reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiTargetID)),
                        bComplete, bActivated, false
                    );
                }
                break;
            }
            case EXTRA_TRESPASS_PACKAGE:
                if ((aiFlags & 0) != 0) {
                    uint32_t uiPackageID;
                    pSaveLoadGameOLD->LoadNumericID(&uiPackageID, 4);
                    if (uiPackageID) {
                        TESPackage* pPackage = pSaveLoadGameOLD->CreatePackage(uiPackageID, PACKAGE_TYPE_TRESPASS);
                        pPackage->LoadGame();
                        SetTrespassPackageExtra(static_cast<TrespassPackage*>(pPackage));
                    }
                }
                break;
            case EXTRA_USEDMARKERS:
                if ((aiFlags & 0) != 0) {
                    // Reads the 4-byte value and drops it.
                    uint32_t uiUnused;
                    pSaveLoadGameOLD->LoadGameDataOLD(&uiUnused, 4);
                }
                break;
            case EXTRA_REFERENCE_POINTER:
                if ((aiFlags & 0) != 0) {
                    uint32_t uiRefID;
                    pSaveLoadGameOLD->LoadNumericID(&uiRefID, 4);
                    SetReferencePointer(reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiRefID)));
                }
                break;
            case EXTRA_LEVELEDCREATURE: break;
            case EXTRA_RUN_ONCE_PACKAGES: {
                uint16_t usPackages;
                pSaveLoadGameOLD->LoadGameDataOLD(&usPackages, 2);
                for (int32_t j = 0; j < usPackages; ++j) {
                    uint32_t uiPackageID;
                    pSaveLoadGameOLD->LoadNumericID(&uiPackageID, 4);
                    uint8_t ucDay;
                    pSaveLoadGameOLD->LoadGameDataOLD(&ucDay, 1);
                    TESPackage* pPackage = dynamic_cast<TESPackage*>(TESForm::GetFormByNumericID(uiPackageID));
                    if (pPackage) {
                        SetRunOnceExtraPackage(pPackage, ucDay);
                    }
                }
                break;
            }
            case EXTRA_PACKAGESTARTLOC: {
                WORLD_LOCATION kLocation;
                kLocation.~WORLD_LOCATION();
                ::new (&kLocation) WORLD_LOCATION;
                uint32_t uiLocationID;
                pSaveLoadGameOLD->LoadNumericID(&uiLocationID, 4);
                pSaveLoadGameOLD->LoadGameDataOLD(&kLocation.locPt, 0xC);
                pSaveLoadGameOLD->LoadGameDataOLD(&kLocation.fZRot, 4);
                kLocation.pLocationForm = TESForm::GetFormByNumericID(uiLocationID);
                TESWorldSpace* pWorld = dynamic_cast<TESWorldSpace*>(kLocation.pLocationForm);
                TESObjectCELL* pCell = dynamic_cast<TESObjectCELL*>(kLocation.pLocationForm);
                if (pWorld || pCell) {
                    SetPackageStartLocation(pWorld, pCell, kLocation.locPt, kLocation.fZRot);
                }
                break;
            }
            case EXTRA_FOLLOWER: {
                uint16_t usFollowers;
                pSaveLoadGameOLD->LoadGameDataOLD(&usFollowers, 2);
                for (int32_t j = 0; j < usFollowers; ++j) {
                    uint32_t uiFollowerID;
                    pSaveLoadGameOLD->LoadNumericID(&uiFollowerID, 4);
                    AddFollower(reinterpret_cast<Actor*>(static_cast<uintptr_t>(uiFollowerID)));
                }
                break;
            }
            case EXTRA_MAGICCASTER:
                if ((aiFlags & 0) != 0) {
                    uint32_t uiSpellID;
                    pSaveLoadGameOLD->LoadNumericID(&uiSpellID, 4);
                    uint32_t uiTargetID;
                    pSaveLoadGameOLD->LoadNumericID(&uiTargetID, 4);
                    uint32_t uiCasterID;
                    pSaveLoadGameOLD->LoadNumericID(&uiCasterID, 4);
                    void* pMemory = ourMalloc(sizeof(NonActorMagicCaster));
                    NonActorMagicCaster* pCaster = pMemory
                        ? ::new (pMemory)
                              NonActorMagicCaster(reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiCasterID)))
                        : nullptr;
                    MagicCaster* pMagicCaster = pCaster;
                    pMagicCaster->SetCurrentSpell(reinterpret_cast<MagicItem*>(static_cast<uintptr_t>(uiSpellID)));
                    pMagicCaster->SetDesiredTarget(reinterpret_cast<MagicTarget*>(static_cast<uintptr_t>(uiTargetID)));
                    AddExtra(pCaster);
                }
                break;
            case EXTRA_MAGICTARGET:
                if ((aiFlags & 0) != 0) {
                    uint32_t uiTargetID;
                    pSaveLoadGameOLD->LoadNumericID(&uiTargetID, 4);
                    void* pMemory = ourMalloc(sizeof(NonActorMagicTarget));
                    NonActorMagicTarget* pTarget = pMemory
                        ? ::new (pMemory)
                              NonActorMagicTarget(reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiTargetID)))
                        : nullptr;
                    MagicTarget* pMagicTarget = pTarget;
                    ActiveEffect::LoadActiveEffectListTES(pMagicTarget->GetActiveEffectList(), nullptr);
                    AddExtra(pTarget);
                }
                break;
            case EXTRA_TELEPORT:
                if ((aiFlags & CHANGE_DOOR_EXTRA_TELEPORT) != 0) {
                    void* pMemory = ourMalloc(sizeof(DoorTeleportData));
                    DoorTeleportData* pTeleport = pMemory ? ::new (pMemory) DoorTeleportData : nullptr;
                    pTeleport->LoadGameTES();
                    SetTeleportPtr(pTeleport);
                }
                break;
            case EXTRA_POISON:
                if ((aiFlags & 0) != 0) {
                    uint32_t uiPoisonID;
                    pSaveLoadGameOLD->LoadNumericID(&uiPoisonID, 4);
                    SetPoison(reinterpret_cast<AlchemyItem*>(static_cast<uintptr_t>(uiPoisonID)));
                }
                break;
            case EXTRA_LASTFINISHEDSEQUENCE:
                if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
                    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x15 && pSaveLoadGameOLD->GetCurrentVersion() < 0x17) {
                        // An animation group index; 245 is the animation group count, compared signed.
                        int32_t iGroup;
                        pSaveLoadGameOLD->LoadGameDataOLD(&iGroup, 4);
                        if (iGroup < 245) {
                            SetLastFinishedSequence(animGroupInfo[iGroup].pName);
                        }
                    }
                    if (pSaveLoadGameOLD->GetCurrentVersion() < 0x15 || pSaveLoadGameOLD->GetCurrentVersion() >= 0x17) {
                        uint8_t ucLength;
                        pSaveLoadGameOLD->LoadGameDataOLD(&ucLength, 1);
                        char acSequence[0x104];
                        ourMemset(acSequence, 0, 0x104);
                        pSaveLoadGameOLD->LoadGameDataOLD(acSequence, ucLength);
                        SetLastFinishedSequence(acSequence);
                    }
                }
                break;
            case EXTRA_MAPMARKER:
                if ((aiFlags & 0) != 0) {
                    uint8_t ucMarkerFlags;
                    pSaveLoadGameOLD->LoadGameDataOLD(&ucMarkerFlags, 1);
                    ExtraMapMarker* pMarker = static_cast<ExtraMapMarker*>(GetExtraData(EXTRA_MAPMARKER));
                    if (pMarker) {
                        pMarker->pMapData->SetFlags(ucMarkerFlags);
                    }
                }
                break;
            case EXTRA_AUDIOMARKER:
            case EXTRA_AUDIOBUOYMARKER: break;
            case EXTRA_PERSISTENT_CELL:
                if (apRef && apRef->IsActor()) {
                    TESObjectCELL* pOldCell = GetPersistentCell();
                    if (pOldCell) {
                        SetPersistentCell(nullptr);
                        pOldCell->RemoveReference(apRef);
                    }
                    uint32_t uiWorldID;
                    pSaveLoadGameOLD->LoadNumericID(&uiWorldID, 4);
                    if (apRef->GetRefPersists()) {
                        TESWorldSpace* pWorld = dynamic_cast<TESWorldSpace*>(TESForm::GetFormByNumericID(uiWorldID));
                        if (pWorld) {
                            SetPersistentCell(pWorld->QPersistentCell());
                            if (apRef) {
                                pWorld->QPersistentCell()->AddReference(apRef, false);
                                if (apRef->GetParentCell() && apRef->GetParentCell()->GetInterior()) {
                                    apRef->GetParentCell()->RemoveReference(apRef);
                                }
                            }
                        }
                    }
                }
                break;
            case EXTRA_DROPPEDITEMLIST: {
                uint8_t ucItems;
                pSaveLoadGameOLD->LoadGameDataOLD(&ucItems, 1);
                for (uint32_t j = 0; j < ucItems; ++j) {
                    uint32_t uiItemID;
                    pSaveLoadGameOLD->LoadNumericID(&uiItemID, 4);
                    if (uiItemID) {
                        AddDroppedItem(reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiItemID)));
                    }
                }
                break;
            }
            case EXTRA_ITEMDROPPER: {
                uint32_t uiDropperID;
                pSaveLoadGameOLD->LoadNumericID(&uiDropperID, 4);
                if (uiDropperID) {
                    SetItemDropper(reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiDropperID)));
                }
                break;
            }
            case EXTRA_FRIEND_HITS: break;
            case EXTRA_HEAD_TRACK_TARGET: {
                uint32_t uiTargetID;
                pSaveLoadGameOLD->LoadNumericID(&uiTargetID, 4);
                if (uiTargetID) {
                    SetHeadTrackTargetExtra(reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiTargetID)));
                }
                break;
            }
            case EXTRA_STARTINGWORLDORCELL:
                if ((aiFlags & 0) != 0) {
                    uint32_t uiWorldOrCellID;
                    pSaveLoadGameOLD->LoadNumericID(&uiWorldOrCellID, 4);
                    if (uiWorldOrCellID) {
                        TESForm* pWorldOrCell = TESForm::GetFormByNumericID(uiWorldOrCellID);
                        if (dynamic_cast<TESObjectCELL*>(pWorldOrCell) || dynamic_cast<TESWorldSpace*>(pWorldOrCell)) {
                            SetStartingWorldOrCell(pWorldOrCell);
                        }
                    }
                }
                break;
            case EXTRA_HOT_KEY:
                if ((aiFlags & 0) != 0) {
                    uint8_t ucHotKey;
                    pSaveLoadGameOLD->LoadGameDataOLD(&ucHotKey, 1);
                    SetHotKey(ucHotKey);
                }
                break;
            case EXTRA_INFO_GENERAL_TOPIC: {
                void* pMemory = ourMalloc(sizeof(MenuTopic));
                MenuTopic* pTopic = pMemory ? ::new (pMemory) MenuTopic : nullptr;
                pTopic->LoadGame(apRef);
                if (pTopic->GetInfo()) {
                    SetInfoGeneralTopic(pTopic);
                }
                else if (pTopic) {
                    delete pTopic;
                }
                break;
            }
            case EXTRA_NO_RUMORS: {
                // The byte goes on to the bool parameter as read, as the game does.
                bool bNoRumors;
                pSaveLoadGameOLD->LoadGameDataOLD(&bNoRumors, 1);
                SetNoRumors(bNoRumors);
                break;
            }
            default:
                Warning(
                    "SAVELOAD: No loading code found in ExtraDataList::LoadGame() for type %i.  The order of the extra "
                    "data enum probably changed.  Errors may occur.",
                    ucType
                );
                break;
        }
    }
    if (!bScriptLoaded && (aiFlags & 0) != 0) {
        RemoveScript();
    }
    if (!pSaveLoadGameOLD->UseSaveGameBlocks()) {
        return;
    }
    char* pNow = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
    LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
    if (pHeader) {
        TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
        if (pNow > pStart + usSize) {
            uint32_t uiFlags = pHeader->iFlags;
            uint8_t ucVersion = pHeader->cVersion;
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form is "
                "%08X %s with version %i and flags %08X",
                static_cast<int32_t>(pNow - (pStart + usSize)),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp", 12009, pHeader->iFormID,
                pForm->GetFormEditorID(), ucVersion, uiFlags
            );
        }
        else if (pNow < pStart + usSize) {
            uint32_t uiFlags = pHeader->iFlags;
            uint8_t ucVersion = pHeader->cVersion;
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading form is "
                "%08X %s with version %i and flags %08X",
                static_cast<int32_t>((pStart + usSize) - pNow),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp", 12009, pHeader->iFormID,
                pForm->GetFormEditorID(), ucVersion, uiFlags
            );
        }
    }
    else if (pNow > pStart + usSize) {
        Warning(
            "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
            static_cast<int32_t>(pNow - (pStart + usSize)),
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp", 12009,
            pSaveLoadGameOLD->GetCurrentVersion()
        );
    }
    else if (pNow < pStart + usSize) {
        Warning(
            "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
            static_cast<int32_t>((pStart + usSize) - pNow),
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp", 12009,
            pSaveLoadGameOLD->GetCurrentVersion()
        );
    }
}

void ExtraDataList::Revert(uint32_t aiFlags, TESObjectREFR* apRef) {
    ExtraDataList* extra;
    TESObjectREFR* itemDropper;
    uint8_t bActor = 0;
    if (apRef) {
        bActor = (uint8_t)apRef->IsActor();
    }
    if ((((aiFlags & 131072) != 0) && (!bActor)) && (GetTeleport())) {
        RemoveTeleportPtr();
    }

    if ((aiFlags & 268435456) != 0) {
        RemoveLastFinishedSequence();
    }
    if (pSaveLoadGameOLD->IsUnk048Zero()) {
        if (bActor) {
            RemovePackageExtra();
            RemoveExtraRunOncePackageList();
            RemovePackageStartLocation();
            RemoveFollowerExtra();
            RemoveFriendHitsExtra();
            BaseExtraList::RemoveExtra((uint8_t)0x3Au);
            SetHeadTrackTargetExtra(nullptr);
            RemoveGhost();
            RemoveInfoGeneral();
            ClearRumorsExtra();
        }
        else {
            itemDropper = GetItemDropper();
            if (itemDropper) {
                extra = itemDropper->TESObjectREFR::GetExtra();
                extra->RemoveDroppedItem(apRef);
                SetItemDropper(nullptr);
            }
        }
    }
}

void ExtraDataList::SetRoomLinkedRefData(RoomRefData* apData) {
    BSExtraData* ExtraData = GetExtraData(123u);
    if (ExtraData) {
        RemoveExtra(ExtraData, true);
    }
    if (apData) {
        ExtraRoomRefData* v2;
        ExtraRoomRefData* apMemory = static_cast<ExtraRoomRefData*>(ourMalloc(sizeof(ExtraRoomRefData)));
        if (apMemory) {
            v2 = ::new (apMemory) ExtraRoomRefData;
        }
        else {
            v2 = nullptr;
        }
        v2->pData = apData;
        AddExtra(v2);
    }
}

// Stores the form through the constructor's result even when the allocation failed, as the game does.
void ExtraDataList::SetStartingWorldOrCell(TESForm* apWorldOrCell) {
    if (apWorldOrCell) {
        auto* pExtra = static_cast<ExtraStartingWorldOrCell*>(GetExtraData(EXTRA_STARTINGWORLDORCELL));
        if (!pExtra) {
            void* pMemory = ourMalloc(sizeof(ExtraStartingWorldOrCell));
            pExtra = pMemory ? ::new (pMemory) ExtraStartingWorldOrCell : nullptr;
            AddExtra(pExtra);
        }
        pExtra->pStartingWorldOrCell = apWorldOrCell;
    }
    else {
        RemoveStartingWorldOrCell();
    }
}

void ExtraDataList::RemovePackageExtra() {
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_PACKAGE);
    if (extraData) {
        BaseExtraList::RemoveExtra(extraData, true);
    }
}

void ExtraDataList::SetOcclusionPlaneLinkedRefData(OcclusionPlaneLinkedRefData* apData) {
    BSExtraData* ExtraData = GetExtraData(118u);
    if (ExtraData) {
        RemoveExtra(ExtraData, true);
    }
    if (apData) {
        ExtraOcclusionPlaneRefData* v2;
        ExtraOcclusionPlaneRefData* apMemory =
            static_cast<ExtraOcclusionPlaneRefData*>(ourMalloc(sizeof(ExtraOcclusionPlaneRefData)));
        if (apMemory) {
            v2 = ::new (apMemory) ExtraOcclusionPlaneRefData;
        }
        else {
            v2 = nullptr;
        }
        v2->pData = apData;
        AddExtra(v2);
    }
}

TESObjectREFR* ExtraDataList::GetMasterRoomRecurse(NiTMap<TESObjectREFR*, bool>* apRoomsTouchedMap) {
    ExtraRoomRefData* pextra = static_cast<ExtraRoomRefData*>(GetExtraData(123));
    if (!pextra) {
        return nullptr;
    }
    for (TESObjectREFR* pref : pextra->pData->kRoomList.ItemsUntilNull()) {
        if ((&pref->m_Extra)->GetRoomIsMaster()) {
            return pref;
        }
    }
    for (TESObjectREFR* pref_2 : pextra->pData->kRoomList.ItemsUntilNull()) {
        bool btest;
        if (!apRoomsTouchedMap->GetAt(pref_2, btest)) {
            apRoomsTouchedMap->SetAt(pref_2, btest);
            TESObjectREFR* pmaster = (&pref_2->m_Extra)->GetMasterRoomRecurse(apRoomsTouchedMap);
            if (pmaster) {
                return pmaster;
            }
        }
    }
    return nullptr;
}

void ExtraDataList::SetPortalLinkedRefData(PortalLinkedRefData* apLinkedRefData) {
    BSExtraData* ExtraData = GetExtraData(119u);
    if (ExtraData) {
        RemoveExtra(ExtraData, true);
    }
    if (apLinkedRefData) {
        ExtraPortalRefData* v2;
        ExtraPortalRefData* apMemory = static_cast<ExtraPortalRefData*>(ourMalloc(sizeof(ExtraPortalRefData)));
        if (apMemory) {
            v2 = ::new (apMemory) ExtraPortalRefData;
        }
        else {
            v2 = nullptr;
        }
        v2->pData = apLinkedRefData;
        AddExtra(v2);
    }
}

void ExtraDataList::SetReflectedByRef(TESObjectREFR* apRef, bool bValue) {
    ExtraReflectorRefs* pextra = static_cast<ExtraReflectorRefs*>(GetExtraData(102));
    if (pextra) {
        for (REF_REFLECTOR_DATA*& rpItem : pextra->RefList.ItemsUntilNull()) {
            if (rpItem->pRef == apRef) {
                if (bValue) {
                    uint32_t* p_iEffectFlags = &rpItem->iEffectFlags;
                    *p_iEffectFlags |= 1u;
                }
                else {
                    uint32_t* v4 = &rpItem->iEffectFlags;
                    *v4 &= ~1u;
                    if (!rpItem->iEffectFlags) {
                        pextra->RemoveReference(apRef);
                        if (pextra->RefList.IsEmpty()) {
                            RemoveExtra(pextra, true);
                        }
                    }
                }
                return;
            }
        }
    }
    if (bValue) {
        if (!pextra) {
            ExtraReflectorRefs* pextra_2;
            ExtraReflectorRefs* apMemory = static_cast<ExtraReflectorRefs*>(ourMalloc(sizeof(ExtraReflectorRefs)));
            if (apMemory) {
                pextra_2 = ::new (apMemory) ExtraReflectorRefs;
            }
            else {
                pextra_2 = nullptr;
            }
            pextra = pextra_2;
            AddExtra(pextra_2);
        }
        pextra->SetReference(apRef, 1u);
    }
}

void ExtraDataList::SetLitWaterRef(TESObjectREFR* apRef, bool bValue) {
    ExtraLitWaterRefs* pextra = static_cast<ExtraLitWaterRefs*>(GetExtraData(133));
    if (pextra) {
        BSSimpleList<TESObjectREFR*>* preflist = &pextra->RefList;
        if (bValue) {
            if (!preflist->IsInList(apRef)) {
                preflist->AddHead(apRef);
            }
        }
        else {
            preflist->Remove(apRef);
            if (pextra->RefList.IsEmpty()) {
                RemoveExtra(pextra, true);
            }
        }
    }
    else {
        if (bValue) {
            ExtraLitWaterRefs* pextra_2;
            ExtraLitWaterRefs* apMemory = static_cast<ExtraLitWaterRefs*>(ourMalloc(sizeof(ExtraLitWaterRefs)));
            if (apMemory) {
                pextra_2 = ::new (apMemory) ExtraLitWaterRefs;
            }
            else {
                pextra_2 = nullptr;
            }
            AddExtra(pextra_2);
            pextra_2->RefList.AddHead(apRef);
        }
    }
}

// Frees the old lock unchecked (ourFree accepts null).
ExtraLock* ExtraDataList::SetLockPtr(LockData* apLock) {
    auto* pExtra = static_cast<ExtraLock*>(GetExtraData(EXTRA_LOCK));
    if (pExtra) {
        ourFree(pExtra->pLock);
        pExtra->pLock = apLock;
        return pExtra;
    }
    void* pMemory = ourMalloc(sizeof(ExtraLock));
    pExtra = pMemory ? ::new (pMemory) ExtraLock(apLock) : nullptr;
    AddExtra(pExtra);
    return pExtra;
}

void ExtraDataList::AddDecalRef(TESObjectREFR* apRef, const NiPoint3& aIntersect, const NiPoint3& aNormal) {
    void* v6;
    ExtraDecalRefs* pExtra;
    if (apRef) {
        pExtra = static_cast<ExtraDecalRefs*>(BaseExtraList::GetExtraData(EXTRA_DECAL_REFS));
        if (!pExtra) {
            v6 = ourMalloc(sizeof(ExtraDecalRefs));
            if (v6) {
                pExtra = ::new (v6) ExtraDecalRefs;
            }
            else {
                pExtra = nullptr;
            }
            BaseExtraList::AddExtra(pExtra);
        }
        pExtra->SetRefDecalData(apRef, aIntersect, aNormal);
    }
}

ExtraLight* ExtraDataList::SetSpellEffectLightPtr(LightRefData* apLight) {
    ExtraLight* ExtraData = static_cast<ExtraLight*>(GetExtraData(64));
    if (ExtraData) {
        if (ExtraData->pLight) {
            delete ExtraData->pLight;
        }
        ExtraData->pLight = apLight;
        return ExtraData;
    }

    ExtraLight* v3;
    ExtraLight* apMemory = static_cast<ExtraLight*>(ourMalloc(sizeof(ExtraLight)));
    if (apMemory) {
        v3 = ::new (apMemory) ExtraLight(apLight);
    }
    else {
        v3 = nullptr;
    }
    v3->cEtype = 64;
    AddExtra(v3);
    return v3;
}

NiPoint3 ExtraDataList::GetStartingAngle(TESObjectREFR* apObject) {
    NiPoint3 result;
    ExtraStartingPosition* ExtraData = static_cast<ExtraStartingPosition*>(GetExtraData(15u));
    if (!ExtraData) {
        ExtraData = SetStartingPositionPtr(apObject);
    }
    result = ExtraData->startRotation;
    return result;
}

// The start position stored in the starting-position extra data, created from apRef when the list has none.
NiPoint3 ExtraDataList::GetStartingLocation(TESObjectREFR* apObject) {
    ExtraStartingPosition* pStart = static_cast<ExtraStartingPosition*>(GetExtraData(EXTRA_STARTINGPOSITION));
    if (!pStart) {
        pStart = SetStartingPositionPtr(apObject);
    }
    return pStart->startPosition;
}

int32_t ExtraDataList::GetFriendHitCount() {
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_FRIEND_HITS);
    if (extraData) {
        return static_cast<ExtraFriendHits*>(extraData)->GetHitCount();
    }

    return 0;
}

void ExtraDataList::SetSeenData(SeenData* apData) {
    if (apData) {
        ExtraSeenData* ExtraData = static_cast<ExtraSeenData*>(GetExtraData(5));
        if (ExtraData) {
            if (ExtraData->pSeenData) {
                delete ExtraData->pSeenData;
            }
            ExtraData->pSeenData = apData;
        }
        else {
            ExtraSeenData* v2;
            ExtraSeenData* apMemory = static_cast<ExtraSeenData*>(ourMalloc(sizeof(ExtraSeenData)));
            if (apMemory) {
                v2 = ::new (apMemory) ExtraSeenData;
            }
            else {
                v2 = nullptr;
            }
            v2->pSeenData = apData;
            AddExtra(v2);
        }
    }
    else {
        RemoveExtra(5);
    }
}

bool ExtraDataList::GetInWaterZone(WaterZone* apZone) {
    auto* pExtra = static_cast<ExtraWaterZoneMap*>(GetExtraData(EXTRA_WATER_ZONE_MAP));
    if (!pExtra) {
        return false;
    }
    int32_t iCount = 0;
    pExtra->WaterZoneMap.GetAt(apZone, iCount);
    return iCount > 0;
}

// Takes the first entry's key; the value is unused.
WaterZone* ExtraDataList::GetFirstWaterZone() {
    NiTMap<WaterZone*, int32_t>* pMap = QWaterZoneMap();
    if (!pMap) {
        return nullptr;
    }
    NiTMapItem<WaterZone*, int32_t>* pPos = pMap->GetFirstPos();
    if (!pPos) {
        return nullptr;
    }
    WaterZone* pZone;
    int32_t iCount;
    pMap->GetNext(pPos, pZone, iCount);
    return pZone;
}

bool ExtraDataList::GetPatrolRefCanBeUsedBy(const TESObjectREFR* apUser) {
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_PATROL_REF_IN_USE_DATA);
    return ((!extraData) || (((int8_t)static_cast<ExtraPatrolRefInUseData*>(extraData)->GetCanBeUsedBy(apUser)) != 0));
}

char ExtraDataList::GetTerminalLockState() {
    auto* pExtra = static_cast<ExtraTerminalState*>(GetExtraData(EXTRA_TERMINALSTATE));
    if (!pExtra) {
        return -1;
    }
    return static_cast<int8_t>(pExtra->cLockOverride);
}

// Stores the override through the constructor's result even when the allocation failed, as the game does.
void ExtraDataList::SetTerminalLockState(char acLockState) {
    auto* pExtra = static_cast<ExtraTerminalState*>(GetExtraData(EXTRA_TERMINALSTATE));
    if (acLockState) {
        if (!pExtra) {
            void* pMemory = ourMalloc(sizeof(ExtraTerminalState));
            pExtra = pMemory ? ::new (pMemory) ExtraTerminalState : nullptr;
            AddExtra(pExtra);
        }
        pExtra->cLockOverride = acLockState;
    }
    else if (pExtra) {
        pExtra->cLockOverride = 0;
        if (pExtra->IsLockedState()) {
            RemoveExtra(pExtra, true);
        }
    }
}

// Stores the state through the constructor's result even when the allocation failed, as the game does.
void ExtraDataList::SetTerminalState(uint8_t acState) {
    auto* pExtra = static_cast<ExtraTerminalState*>(GetExtraData(EXTRA_TERMINALSTATE));
    if (acState) {
        if (!pExtra) {
            void* pMemory = ourMalloc(sizeof(ExtraTerminalState));
            pExtra = pMemory ? ::new (pMemory) ExtraTerminalState : nullptr;
            AddExtra(pExtra);
        }
        pExtra->cState = acState;
    }
    else if (pExtra) {
        pExtra->cState = 0;
        if (pExtra->IsLockedState()) {
            RemoveExtra(pExtra, true);
        }
    }
}

void ExtraDataList::SetCombatStyle(TESCombatStyle* apCombatStyle) {
    if (apCombatStyle) {
        auto* pExtra = static_cast<ExtraCombatStyle*>(GetExtraData(EXTRA_COMBAT_STYLE));
        if (pExtra) {
            pExtra->pCombatStyle = apCombatStyle;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraCombatStyle));
            pExtra = pMemory ? ::new (pMemory) ExtraCombatStyle(apCombatStyle) : nullptr;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_COMBAT_STYLE);
    }
}

void ExtraDataList::SetPatrolRefInUse(TESObjectREFR* apUser) {
    auto* pExtra = static_cast<ExtraPatrolRefInUseData*>(GetExtraData(EXTRA_PATROL_REF_IN_USE_DATA));
    if (pExtra || !apUser) {
        if (pExtra && !apUser) {
            RemoveExtra(pExtra, true);
        }
    }
    else {
        void* pMemory = ourMalloc(sizeof(ExtraPatrolRefInUseData));
        pExtra = pMemory ? ::new (pMemory) ExtraPatrolRefInUseData(apUser) : nullptr;
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetGhost(bool abGhost) {
    bool bHasExtra = HasExtra(EXTRA_GHOST);
    if (abGhost && !bHasExtra) {
        void* pMemory = ourMalloc(sizeof(ExtraGhost));
        ExtraGhost* pExtra = pMemory ? ::new (pMemory) ExtraGhost : nullptr;
        AddExtra(pExtra);
    }
    else if (!abGhost && bHasExtra) {
        RemoveExtra(EXTRA_GHOST);
    }
}

// Stores the reference through the constructor's result even when the allocation failed, as the game does.
void ExtraDataList::SetOpenCloseActivateRef(TESObjectREFR* apRef) {
    if (apRef) {
        auto* pExtra = static_cast<ExtraOpenCloseActivateRef*>(GetExtraData(EXTRA_OPENCLOSEACTIVATE_REF));
        if (pExtra) {
            pExtra->pActivateRef = apRef;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraOpenCloseActivateRef));
            pExtra = pMemory ? ::new (pMemory) ExtraOpenCloseActivateRef : nullptr;
            pExtra->pActivateRef = apRef;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_OPENCLOSEACTIVATE_REF);
    }
}

// Stores the reference through the constructor's result even when the allocation failed, as the game does.
void ExtraDataList::SetAshPileRef(TESObjectREFR* apRef) {
    auto* pExtra = static_cast<ExtraAshPileRef*>(GetExtraData(EXTRA_ASHPILE_REF));
    if (apRef) {
        if (!pExtra) {
            void* pMemory = ourMalloc(sizeof(ExtraAshPileRef));
            pExtra = pMemory ? ::new (pMemory) ExtraAshPileRef : nullptr;
            AddExtra(pExtra);
        }
        pExtra->pAshPileRef = apRef;
    }
    else if (pExtra) {
        RemoveExtra(pExtra, true);
    }
}

void ExtraDataList::SetHotKey(char acValue) {
    auto* pExtra = static_cast<ExtraHotKey*>(GetExtraData(EXTRA_HOT_KEY));
    if (pExtra) {
        pExtra->chotkey = acValue;
        return;
    }
    void* pMemory = ourMalloc(sizeof(ExtraHotKey));
    pExtra = pMemory ? ::new (pMemory) ExtraHotKey(static_cast<uint8_t>(acValue)) : nullptr;
    AddExtra(pExtra);
}

void ExtraDataList::SetRunOnceExtraPackage(TESPackage* apPack, char acDay) {
    auto* pExtra = static_cast<ExtraRunOncePacks*>(GetExtraData(EXTRA_RUN_ONCE_PACKAGES));
    if (!pExtra) {
        void* pMemory = ourMalloc(sizeof(ExtraRunOncePacks));
        pExtra = pMemory ? ::new (pMemory) ExtraRunOncePacks : nullptr;
        AddExtra(pExtra);
    }
    pExtra->AddRunOncePack(apPack, static_cast<uint8_t>(acDay));
}

uint32_t ExtraDataList::GetExtraSayToResponseID() {
    auto* pExtra = static_cast<ExtraSayToTopicInfo*>(GetExtraData(EXTRA_SAY_TO_TOPIC_INFO));
    if (!pExtra) {
        return 0;
    }
    return pExtra->nResponseID;
}

void ExtraDataList::RemoveSayToInfoExtra() {
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_SAY_TO_TOPIC_INFO);
    if (extraData) {
        BaseExtraList::RemoveExtra(extraData, true);
    }
}

TESTopicInfo* ExtraDataList::GetExtraSayToTopicInfo() {
    int32_t v3 = 0;
    ExtraSayToTopicInfo* extraData =
        static_cast<ExtraSayToTopicInfo*>(BaseExtraList::GetExtraData(EXTRA_SAY_TO_TOPIC_INFO));
    if (extraData) {
        return extraData->pInfo;
    }
    return reinterpret_cast<TESTopicInfo*>(static_cast<uintptr_t>(v3));
}

bool ExtraDataList::HasFollower(Actor* apActor) {
    const ExtraFollower* pextra = static_cast<const ExtraFollower*>(GetExtraData(29));
    return pextra && pextra->pActorlist->IsInList(apActor);
}

void ExtraDataList::RemoveDroppedItem(TESObjectREFR* apItem) {
    if (apItem) {
        auto* pExtra = static_cast<ExtraDroppedItemList*>(GetExtraData(EXTRA_DROPPEDITEMLIST));
        if (pExtra) {
            pExtra->DroppedItemList.Remove(apItem);
            if (pExtra->DroppedItemList.IsEmpty()) {
                RemoveExtra(pExtra, true);
            }
        }
    }
}

void ExtraDataList::RemoveLinkedRefChild(TESObjectREFR* apChild) {
    if (apChild) {
        auto* pExtra = static_cast<ExtraLinkedRefChildren*>(GetExtraData(EXTRA_LINKED_REF_CHILDREN));
        if (pExtra) {
            pExtra->ChildList.Remove(apChild);
            if (pExtra->ChildList.IsEmpty()) {
                RemoveExtra(pExtra, true);
            }
        }
    }
}

bool ExtraDataList::HasLeveledItem() {
    return HasExtra(EXTRA_LEVELITEM);
}

bool ExtraDataList::HasLevCreaCalced() {
    return HasExtra(EXTRA_LEVELEDCREATURE);
}

bool ExtraDataList::GetGhost() {
    return HasExtra(EXTRA_GHOST);
}

bool ExtraDataList::GetCanNotWear() {
    return HasExtra(EXTRA_CANNOTWEAR);
}

void ExtraDataList::RemoveContainerChanges() {
    RemoveExtra(EXTRA_CONTAINER_CHANGES);
}

void ExtraDataList::RemoveScript() {
    RemoveExtra(EXTRA_SCRIPT);
}

void ExtraDataList::RemoveLastFinishedSequence() {
    RemoveExtra(EXTRA_LASTFINISHEDSEQUENCE);
}

void ExtraDataList::RemoveLeveledItem() {
    BSExtraData* pExtra = GetExtraData(EXTRA_LEVELITEM);
    if (pExtra) {
        RemoveExtra(pExtra, true);
    }
}

uint32_t ExtraDataList::GetActionFlag() {
    auto* pExtra = static_cast<ExtraAction*>(GetExtraData(EXTRA_ACTION));
    if (!pExtra) {
        return 1;
    }
    return pExtra->eAction;
}

bool ExtraDataList::GetAction(OBJECT_ACTION aeType) {
    return (GetActionFlag() & aeType) != 0;
}

void ExtraDataList::SetAction(OBJECT_ACTION aeType) {
    SetActionFlag(static_cast<OBJECT_ACTION>(GetActionFlag() | aeType));
}

void ExtraDataList::ClearAction(OBJECT_ACTION aeType) {
    SetActionFlag(static_cast<OBJECT_ACTION>(GetActionFlag() & ~aeType));
}

ExtraAnimSave* ExtraDataList::GetAnimSave() {
    return static_cast<ExtraAnimSave*>(GetExtraData(EXTRA_ANIM_SAVE));
}

ExtraFactionChanges* ExtraDataList::GetFactionChangesExtra() {
    return static_cast<ExtraFactionChanges*>(GetExtraData(EXTRA_FACTION_CHANGES));
}

ExtraDismemberedLimbs* ExtraDataList::GetDismembermentExtra() {
    return static_cast<ExtraDismemberedLimbs*>(GetExtraData(EXTRA_DISMEMBERED_LIMBS));
}

ExtraAmmo* ExtraDataList::GetAmmoExtra() {
    return static_cast<ExtraAmmo*>(GetExtraData(EXTRA_AMMO));
}

ExtraSayToTopicInfo* ExtraDataList::GetSayToExtra() {
    return static_cast<ExtraSayToTopicInfo*>(GetExtraData(EXTRA_SAY_TO_TOPIC_INFO));
}

ExtraSayTopicInfoOnceADay* ExtraDataList::GetExtraSayTopicInfoOnceADayExtra() {
    return reinterpret_cast<ExtraSayTopicInfoOnceADay*>(GetExtraData(EXTRA_SAY_ONCE_A_DAY_TOPIC_INFO));
}

ExtraLeveledItem* ExtraDataList::GetLeveledItem() {
    return static_cast<ExtraLeveledItem*>(GetExtraData(EXTRA_LEVELITEM));
}

ExtraRefractionProperty* ExtraDataList::GetRefractionPropertyExtra() {
    return static_cast<ExtraRefractionProperty*>(GetExtraData(EXTRA_REFRACTION_PROPERTY));
}

Animation* ExtraDataList::GetAnimation() {
    auto* pExtra = static_cast<ExtraAnim*>(GetExtraData(EXTRA_ANIM));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pAnimation;
}

ExtraAnim* ExtraDataList::SetAnimPtr(Animation* apAnim) {
    ExtraAnim* apExtra = static_cast<ExtraAnim*>(GetExtraData(16u));
    if (apExtra) {
        if (apExtra->pAnimation) {
            delete apExtra->pAnimation;
        }
        apExtra->pAnimation = apAnim;
        return apExtra;
    }

    ExtraAnim* v3;
    ExtraAnim* apMemory = static_cast<ExtraAnim*>(ourMalloc(sizeof(ExtraAnim)));
    if (apMemory) {
        v3 = ::new (apMemory) ExtraAnim(apAnim);
    }
    else {
        v3 = nullptr;
    }
    AddExtra(v3);
    return v3;
}

LightRefData* ExtraDataList::GetLight() {
    auto* pExtra = static_cast<ExtraLight*>(GetExtraData(EXTRA_LIGHT));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pLight;
}

LockData* ExtraDataList::GetLock() {
    auto* pExtra = static_cast<ExtraLock*>(GetExtraData(EXTRA_LOCK));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pLock;
}

RADIO_DATA* ExtraDataList::GetRadioData() {
    auto* pExtra = static_cast<ExtraRadioData*>(GetExtraData(EXTRA_RADIO_DATA));
    if (!pExtra) {
        return nullptr;
    }
    return &pExtra->Data;
}

DoorTeleportData* ExtraDataList::GetTeleport() {
    auto* pExtra = static_cast<ExtraTeleport*>(GetExtraData(EXTRA_TELEPORT));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pTeleportData;
}

MapMarkerData* ExtraDataList::GetMapMarkerData() {
    auto* pExtra = static_cast<ExtraMapMarker*>(GetExtraData(EXTRA_MAPMARKER));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pMapData;
}

BGSMusicType* ExtraDataList::GetMusicType() {
    auto* pExtra = static_cast<ExtraCellMusicType*>(GetExtraData(EXTRA_CELLMUSICTYPE));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pType;
}

TESImageSpace* ExtraDataList::GetImageSpace() {
    auto* pExtra = static_cast<ExtraCellImageSpace*>(GetExtraData(EXTRA_IMAGESPACE));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pImageSpace;
}

TESObjectREFR* ExtraDataList::GetEnableStateParent() {
    auto* pExtra = static_cast<ExtraEnableStateParent*>(GetExtraData(EXTRA_ENABLESTATEPARENT));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pParent;
}

BSSimpleList<TESObjectREFR*>* ExtraDataList::GetEnableStateChildren() {
    auto* pExtra = static_cast<ExtraEnableStateChildren*>(GetExtraData(EXTRA_ENABLESTATECHILDREN));
    if (!pExtra) {
        return nullptr;
    }
    return &pExtra->ChildList;
}

TESObjectREFR* ExtraDataList::GetLinkedRef() {
    auto* pExtra = static_cast<ExtraLinkedRef*>(GetExtraData(EXTRA_LINKED_REF));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pLinkedRef;
}

BSSimpleList<TESObjectREFR*>* ExtraDataList::GetLinkedRefChildren() {
    auto* pExtra = static_cast<ExtraLinkedRefChildren*>(GetExtraData(EXTRA_LINKED_REF_CHILDREN));
    if (!pExtra) {
        return nullptr;
    }
    return &pExtra->ChildList;
}

BSSimpleList<REF_ACTIVATE_DATA*>* ExtraDataList::GetActivateParentList() {
    auto* pExtra = static_cast<ExtraActivateRef*>(GetExtraData(EXTRA_ACTIVATE_REF));
    if (!pExtra) {
        return nullptr;
    }
    return &pExtra->ParentList;
}

BSSimpleList<REF_ACTIVATE_DATA*>* ExtraDataList::GetActivateRefChildren() {
    auto* pExtra = static_cast<ExtraActivateRefChildren*>(GetExtraData(EXTRA_ACTIVATE_REF_CHILDREN));
    if (!pExtra) {
        return nullptr;
    }
    return &pExtra->ChildList;
}

BSSimpleList<REF_DECAL_DATA*>* ExtraDataList::GetDecalRefs() {
    auto* pExtra = static_cast<ExtraDecalRefs*>(GetExtraData(EXTRA_DECAL_REFS));
    if (!pExtra) {
        return nullptr;
    }
    return &pExtra->DecalRefList;
}

BGSPrimitive* ExtraDataList::GetPrimitive() {
    auto* pExtra = static_cast<ExtraPrimitive*>(GetExtraData(EXTRA_PRIMITIVE));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pPrimitive;
}

TESActorBase* ExtraDataList::GetLevCreaOriginalBase() {
    auto* pExtra = static_cast<ExtraLeveledCreature*>(GetExtraData(EXTRA_LEVELEDCREATURE));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pOriginalBase;
}

uint32_t ExtraDataList::GetDetachTime() {
    auto* pExtra = static_cast<ExtraDetachTime*>(GetExtraData(EXTRA_CELLDETACHTIME));
    if (!pExtra) {
        return 0;
    }
    return pExtra->iTime;
}

BGSEncounterZone* ExtraDataList::GetEncounterZone() {
    auto* pExtra = static_cast<ExtraEncounterZone*>(GetExtraData(EXTRA_ENCOUNTERZONE));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pZone;
}

TESForm* ExtraDataList::GetEmittanceSource() {
    auto* pExtra = static_cast<ExtraEmittanceSource*>(GetExtraData(EXTRA_EMITTANCE_SOURCE));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pSource;
}

TESObjectREFR* ExtraDataList::GetMultiBoundRef() {
    auto* pExtra = static_cast<ExtraMultiBoundRef*>(GetExtraData(EXTRA_MULTIBOUND_REF));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pBoundRef;
}

// The portal list is the room data's first member.
BSSimpleList<TESObjectREFR*>* ExtraDataList::GetRoomLinkedPortalRefList() {
    RoomRefData* pData = GetRoomLinkedRefData();
    return pData ? &pData->kPortalList : nullptr;
}

TESObjectREFR* ExtraDataList::GetPortalLinkedRoomRef(int32_t aiWhich) {
    ExtraPortalRefData* pExtraData =
        static_cast<ExtraPortalRefData*>(BaseExtraList::GetExtraData(EXTRA_PORTAL_REF_DATA));
    if (!pExtraData) {
        return nullptr;
    }
    if (pExtraData->pData) {
        return pExtraData->pData->pLinkedRefs[aiWhich];
    }

    return nullptr;
}

MultiBoundMarkerData* ExtraDataList::GetMultiBoundData() {
    auto* pExtra = static_cast<ExtraMultiBoundData*>(GetExtraData(EXTRA_MULTIBOUND_DATA));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pBound;
}

TESModel* ExtraDataList::GetModelSwap() {
    auto* pExtra = static_cast<ExtraModelSwap*>(GetExtraData(EXTRA_MODEL_SWAP));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pModelSwap;
}

TESCombatStyle* ExtraDataList::GetCombatStyle() {
    auto* pExtra = static_cast<ExtraCombatStyle*>(GetExtraData(EXTRA_COMBAT_STYLE));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pCombatStyle;
}

InventoryChanges* ExtraDataList::GetContainerChanges() {
    auto* pExtra = static_cast<ExtraContainerChanges*>(GetExtraData(EXTRA_CONTAINER_CHANGES));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pChanges;
}

uint16_t ExtraDataList::GetSaveSize(uint32_t aiFlags, TESObjectREFR* apRef) {
    BaseExtraList::ExtraCritSection.Lock(nullptr);
    int16_t sHeaderSize = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        sHeaderSize = 6;
    }
    uint16_t isize = sHeaderSize + 2;
    for (BSExtraData* work = pHead; work; work = work->pNext) {
        uint16_t itempsize = isize;
        switch (work->cEtype) {
            case EXTRA_PERSISTENT_CELL: {
                if ((apRef) && (apRef->IsActor())) {
                    isize += 4;
                }

                break;
            }
            case EXTRA_PACKAGESTARTLOC: {
                isize += 20;
                break;
            }
            case EXTRA_PACKAGE: {
                ExtraPackage* pPackage = dynamic_cast<ExtraPackage*>(work);
                isize += 14;
                if ((pSaveLoadGameOLD->GetCurrentVersion() >= 64) &&
                    (pDataHandler->IsFormIDCreated(pPackage->pPack->iFormID))) {
                    isize += 1;
                    isize += pPackage->pPack->GetSaveSize();
                }

                break;
            }
            case EXTRA_RUN_ONCE_PACKAGES: {
                ExtraRunOncePacks* pRunOnce = dynamic_cast<ExtraRunOncePacks*>(work);
                isize += 2;
                isize += 5 * pRunOnce->pPackageList->ItemsInList();
                break;
            }
            case EXTRA_FOLLOWER: {
                ExtraFollower* pFollower = dynamic_cast<ExtraFollower*>(work);
                isize += 4 * pFollower->pActorlist->ItemsInList() + 2;
                break;
            }
            case EXTRA_LOCK: {
                if ((aiFlags & CHANGE_OBJECT_EXTRA_LOCK) != 0) {
                    isize += 6;
                }
                break;
            }
            case EXTRA_TELEPORT: {
                if ((aiFlags & CHANGE_DOOR_EXTRA_TELEPORT) != 0) {
                    ExtraTeleport* pteleport = dynamic_cast<ExtraTeleport*>(work);
                    isize += pteleport->pTeleportData->GetSaveSize();
                }
                break;
            }
            case EXTRA_ITEMDROPPER: {
                isize += 4;
                break;
            }
            case EXTRA_LASTFINISHEDSEQUENCE: {
                if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
                    ExtraLastFinishedSequence* plast = dynamic_cast<ExtraLastFinishedSequence*>(work);
                    isize += BSstrlen(plast->pLastSequenceName) + 1;
                }
                break;
            }
            case EXTRA_HEAD_TRACK_TARGET: {
                isize += 4;
                break;
            }
            case EXTRA_INFO_GENERAL_TOPIC: {
                ExtraInfoGeneralTopic* pinfoGeneralTopic = dynamic_cast<ExtraInfoGeneralTopic*>(work);
                if (pinfoGeneralTopic->pInfoGen) {
                    isize += pinfoGeneralTopic->pInfoGen->GetSaveSize();
                }
                break;
            }
            case EXTRA_NO_RUMORS: {
                ++isize;
                break;
            }
            default: {
                break;
            }
        }
        if (itempsize == isize) {
            if (work->cEtype == EXTRA_GHOST) {
                ++isize;
            }
        }
        else {
            ++isize;
        }
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* pEditorID = pform->GetFormEditorID();
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize,
                pheader->iFormID, pEditorID, iFlags, 10677,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize, 10677,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp"
            );
        }
    }
    BaseExtraList::ExtraCritSection.Unlock();
    return isize;
}

TESForm* ExtraDataList::GetOwner() {
    auto* pExtra = static_cast<ExtraOwnership*>(GetExtraData(EXTRA_OWNERSHIP));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pOwner;
}

TESGlobal* ExtraDataList::GetGlobal() {
    auto* pExtra = static_cast<ExtraGlobal*>(GetExtraData(EXTRA_GLOBAL));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pGlobal;
}

ExtraFollower* ExtraDataList::GetFollowerExtra() {
    return reinterpret_cast<ExtraFollower*>(GetExtraData(EXTRA_FOLLOWER));
}

Script* ExtraDataList::GetScript() {
    auto* pExtra = static_cast<ExtraScript*>(GetExtraData(EXTRA_SCRIPT));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pScript;
}

ScriptLocals* ExtraDataList::GetScriptLocals() {
    auto* pExtra = static_cast<ExtraScript*>(GetExtraData(EXTRA_SCRIPT));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pScriptVars;
}

void ExtraDataList::SetScriptLocals(ScriptLocals* apScriptlocals) {
    auto* pExtra = static_cast<ExtraScript*>(GetExtraData(EXTRA_SCRIPT));
    if (pExtra) {
        pExtra->pScriptVars = apScriptlocals;
    }
}

WORLD_LOCATION* ExtraDataList::GetPackageStartLocation() {
    auto* pExtra = static_cast<ExtraPackageStartLocation*>(GetExtraData(EXTRA_PACKAGESTARTLOC));
    if (!pExtra) {
        return nullptr;
    }
    return &pExtra->worldLoc;
}

TESObjectREFR* ExtraDataList::GetReferencePointer() {
    auto* pExtra = static_cast<ExtraReferencePointer*>(GetExtraData(EXTRA_REFERENCE_POINTER));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pRef;
}

TESPackage* ExtraDataList::GetPackageExtra() {
    auto* pExtra = static_cast<ExtraPackage*>(GetExtraData(EXTRA_PACKAGE));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pPack;
}

bool ExtraDataList::GetPackageExtraComplete() {
    auto* pExtra = static_cast<ExtraPackage*>(GetExtraData(EXTRA_PACKAGE));
    if (pExtra) {
        return pExtra->bActionComplete;
    }
    return false;
}

BSSimpleList<Crime*>* ExtraDataList::GetPlayerCrimeList() {
    auto* pExtra = static_cast<ExtraPlayerCrimeList*>(GetExtraData(EXTRA_PLAYERCRIMELIST));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pCrime;
}

TESObjectCELL* ExtraDataList::GetPersistentCell() {
    auto* pExtra = static_cast<ExtraPersistentCell*>(GetExtraData(EXTRA_PERSISTENT_CELL));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pPersistentCell;
}

RagDollData* ExtraDataList::GetRagDollData() {
    auto* pExtra = static_cast<ExtraRagDollData*>(GetExtraData(EXTRA_RAGDOLLDATA));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pRagDollData;
}

void ExtraDataList::SetRagDollDataFromCopy(RagDollData* apRagData) {
    ExtraRagDollData* pextra = static_cast<ExtraRagDollData*>(GetExtraData(20));
    if (pextra) {
        if (apRagData) {
            pextra->pRagDollData->Copy(apRagData);
        }
        else {
            RemoveExtra(pextra, true);
        }
    }
    else {
        if (apRagData) {
            RagDollData* v2;
            ExtraRagDollData* pextra_2;
            ExtraRagDollData* apMemory = static_cast<ExtraRagDollData*>(ourMalloc(sizeof(ExtraRagDollData)));
            if (apMemory) {
                pextra_2 = ::new (apMemory) ExtraRagDollData;
            }
            else {
                pextra_2 = nullptr;
            }
            RagDollData* v5 = static_cast<RagDollData*>(ourMalloc(sizeof(RagDollData)));
            if (v5) {
                v2 = ::new (v5) RagDollData;
            }
            else {
                v2 = nullptr;
            }
            pextra_2->pRagDollData = v2;
            pextra_2->pRagDollData->Copy(apRagData);
            AddExtra(pextra_2);
        }
    }
}

char* ExtraDataList::GetSavedHavokData() {
    auto* pExtra = static_cast<ExtraSavedHavokData*>(GetExtraData(EXTRA_SAVED_HAVOK_DATA));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pHavokBuffer;
}

// Stores the save game's Havok sub-buffer: on the existing saved-Havok extra after releasing its buffer, or on a fresh
// one. A null buffer drops the extra instead.
void ExtraDataList::SetSavedHavokData(char* apBuffer) {
    if (!apBuffer) {
        BaseExtraList::RemoveExtra(EXTRA_SAVED_HAVOK_DATA);
        return;
    }
    auto* pExtra = static_cast<ExtraSavedHavokData*>(GetExtraData(EXTRA_SAVED_HAVOK_DATA));
    if (pExtra) {
        SaveGameWarning("SAVELOAD: SetSavedHavokData() is being called when there is already saved havok data.");
        pExtra->DeleteBuffer();
    }
    else {
        void* pMemory = ourMalloc(sizeof(ExtraSavedHavokData));
        ExtraSavedHavokData* pNew = nullptr;
        if (pMemory) {
            pNew = ::new (pMemory) ExtraSavedHavokData;
        }
        pExtra = pNew;
        AddExtra(pNew);
    }
    pExtra->pHavokBuffer = apBuffer;
}

// Stores the save game's animation sub-buffer: on the existing saved-animation extra after releasing its buffer, or on
// a fresh one. A null buffer drops the extra instead.
void ExtraDataList::SetSavedAnimation(char* apBuffer) {
    if (!apBuffer) {
        BaseExtraList::RemoveExtra(EXTRA_SAVED_ANIMATION);
        return;
    }
    auto* pExtra = static_cast<ExtraSavedAnimation*>(GetExtraData(EXTRA_SAVED_ANIMATION));
    if (pExtra) {
        SaveGameWarning("SAVELOAD: SetSavedAnimation() is being called when there is already a saved animation.");
        pExtra->DeleteBuffer();
    }
    else {
        void* pMemory = ourMalloc(sizeof(ExtraSavedAnimation));
        ExtraSavedAnimation* pNew = nullptr;
        if (pMemory) {
            pNew = ::new (pMemory) ExtraSavedAnimation;
        }
        pExtra = pNew;
        AddExtra(pNew);
    }
    pExtra->pAnimationBuffer = apBuffer;
}

float ExtraDataList::GetObjectHealth() {
    auto* pExtra = static_cast<ExtraObjectHealth*>(GetExtraData(EXTRA_OBJECT_HEALTH));
    if (!pExtra) {
        return -1.0f;
    }
    return pExtra->fHealth;
}

PatrolRefData* ExtraDataList::GetPatrolRefData() {
    auto* pExtra = static_cast<ExtraPatrolRefData*>(GetExtraData(EXTRA_PATROL_REF_DATA));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pPatrolData;
}

CollisionData* ExtraDataList::GetCollisionData() {
    auto* pExtra = static_cast<ExtraCollisionData*>(GetExtraData(EXTRA_COLLISION_DATA));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pCollisionData;
}

AudioMarker* ExtraDataList::GetAudioMarkerData() {
    auto* pExtra = static_cast<ExtraAudioMarker*>(GetExtraData(EXTRA_AUDIOMARKER));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pAudioData;
}

ImpactSwap* ExtraDataList::GetImpactSwap() {
    auto* pExtra = static_cast<ExtraCellImpactSwap*>(GetExtraData(EXTRA_IMPACTSWAP));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pImpactSwap;
}

BSSimpleList<TESObjectREFR*>* ExtraDataList::GetLitWaterRefs() {
    auto* pExtra = static_cast<ExtraLitWaterRefs*>(GetExtraData(EXTRA_LIT_WATER_REFS));
    if (!pExtra) {
        return nullptr;
    }
    return &pExtra->RefList;
}

BSSimpleList<TESObjectREFR*>* ExtraDataList::GetDroppedItemList() {
    auto* pExtra = static_cast<ExtraDroppedItemList*>(GetExtraData(EXTRA_DROPPEDITEMLIST));
    if (!pExtra) {
        return nullptr;
    }
    return &pExtra->DroppedItemList;
}

// Stores the dropper through the constructor's result even when the allocation failed, as the game does.
void ExtraDataList::SetItemDropper(TESObjectREFR* apDropper) {
    if (apDropper) {
        auto* pExtra = static_cast<ExtraItemDropper*>(GetExtraData(EXTRA_ITEMDROPPER));
        if (!pExtra) {
            void* pMemory = ourMalloc(sizeof(ExtraItemDropper));
            pExtra = pMemory ? ::new (pMemory) ExtraItemDropper : nullptr;
            AddExtra(pExtra);
        }
        pExtra->pDropper = apDropper;
    }
    else {
        RemoveExtra(EXTRA_ITEMDROPPER);
    }
}

void ExtraDataList::AddDroppedItem(TESObjectREFR* apItem) {
    if (apItem) {
        ExtraDroppedItemList* pextra = static_cast<ExtraDroppedItemList*>(GetExtraData(58));
        if (!pextra) {
            ExtraDroppedItemList* pextra_2;
            ExtraDroppedItemList* apMemory =
                static_cast<ExtraDroppedItemList*>(ourMalloc(sizeof(ExtraDroppedItemList)));
            if (apMemory) {
                pextra_2 = ::new (apMemory) ExtraDroppedItemList;
            }
            else {
                pextra_2 = nullptr;
            }
            pextra = pextra_2;
            AddExtra(pextra_2);
        }
        if (!pextra->DroppedItemList.IsInList(apItem)) {
            pextra->DroppedItemList.AddHead(apItem);
        }
    }
}

void ExtraDataList::RemoveDroppedItemList() {
    if (BaseExtraList::GetExtraData(EXTRA_DROPPEDITEMLIST)) {
        BaseExtraList::RemoveExtra(EXTRA_DROPPEDITEMLIST);
    }
}

TESObjectREFR* ExtraDataList::GetAshPileRef() {
    auto* pExtra = static_cast<ExtraAshPileRef*>(GetExtraData(EXTRA_ASHPILE_REF));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pAshPileRef;
}

TESObjectREFR* ExtraDataList::GetDroppedWeapon() {
    TESObjectREFR* pRef = nullptr;
    ExtraDroppedItemList* pextra = static_cast<ExtraDroppedItemList*>(GetExtraData(58));
    if (pextra) {
        for (BSSimpleList<TESObjectREFR*>* pretlist = &pextra->DroppedItemList; !pRef && (pretlist && pretlist->m_item);
             pretlist = pretlist->m_pkNext) {
            TESObjectREFR* ptemp = pretlist->m_item;
            if (ptemp->GetObjectReference()->GetFormType() == WEAP_ID) {
                pRef = ptemp;
            }
        }
    }
    return pRef;
}

char* ExtraDataList::GetExtraEditorID() {
    auto* pExtra = static_cast<ExtraEditorID*>(GetExtraData(EXTRA_EDITOR_ID));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->cEditorID.pString;
}

BSOcclusionPlane* ExtraDataList::GetOcclusionPlane() {
    auto* pExtra = static_cast<ExtraOcclusionPlane*>(GetExtraData(EXTRA_OCCLUSION_PLANE));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->spPlane;
}

TESObjectREFR* ExtraDataList::GetXTarget() {
    auto* pExtra = static_cast<ExtraXTarget*>(GetExtraData(EXTRA_XTARGET));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pTarget;
}

uint8_t ExtraDataList::GetWeaponModFlags() {
    auto* pExtra = static_cast<ExtraWeaponModFlags*>(GetExtraData(EXTRA_WEAPON_MOD_SLOTS));
    if (!pExtra) {
        return 0;
    }
    return pExtra->cWeaponModsActive;
}

bool ExtraDataList::GetWeaponModSlotActive(uint8_t slot) {
    auto* pExtra = static_cast<ExtraWeaponModFlags*>(GetExtraData(EXTRA_WEAPON_MOD_SLOTS));
    if (!pExtra) {
        return false;
    }
    return pExtra->GetWeaponModActive(slot) != 0;
}

bool ExtraDataList::GetWorn(bool abwornleft) {
    if (abwornleft) {
        return HasExtra(EXTRA_WORN_LEFT);
    }
    return HasExtra(EXTRA_WORN) || HasExtra(EXTRA_WORN_LEFT);
}

bool ExtraDataList::GetRoomIsMaster() {
    auto* pExtra = static_cast<ExtraRoomRefData*>(GetExtraData(EXTRA_ROOM_REF_DATA));
    if (!pExtra) {
        return true;
    }
    RoomRefData* pData = pExtra->pData;
    if (pData->ucRoomCount != 0) {
        return true;
    }
    // BSSimpleList::IsEmpty: no next node and no item
    return pData->kRoomList.m_pkNext == nullptr && pData->kRoomList.m_item == nullptr;
}

void ExtraDataList::GetLevCreaMultiplier(float& afOutMult, int32_t& aiOutAllBelowForce) {
    afOutMult = 1.0f;
    aiOutAllBelowForce = 0;
    auto* pExtra = static_cast<ExtraLevCreaModifier*>(GetExtraData(EXTRA_LEVCREA_MOD));
    if (pExtra) {
        afOutMult = pExtra->GetMultiplier();
        aiOutAllBelowForce = pExtra->GetAllBelowForce();
    }
}

RoomRefData* ExtraDataList::GetRoomLinkedRefData() {
    auto* pExtra = static_cast<ExtraRoomRefData*>(GetExtraData(EXTRA_ROOM_REF_DATA));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pData;
}

void ExtraDataList::RemoveNonPersistentCellData() {
    BaseExtraList::ExtraCritSection.Lock("ExtraDataList::RemoveNonPersistentCellData()");
    BaseExtraList::ClearLastExtraAll(this);
    BSExtraData* pwork = pHead;
    BSExtraData* plast = nullptr;
    while (pwork) {
        BSExtraData* ptemp = pwork;
        pwork = pwork->pNext;
        uint8_t ExtraType = ptemp->cEtype;
        if (!ExtraType) {
            goto LABEL_9;
        }
        if (ExtraType <= 2u) {
            if (pSaveLoadGame->GetReconstructingForms()) {
                plast = ptemp;
            }
            else {
            LABEL_9:;
                if (ptemp == pHead) {
                    pHead = pwork;
                }
                if (plast) {
                    plast->pNext = pwork;
                }
                delete ptemp;
            }
        }
        else {
            if (ExtraType != 4) {
                goto LABEL_9;
            }
            plast = ptemp;
        }
    }
    BaseExtraList::ExtraCritSection.Unlock();
}

BGSAcousticSpace* ExtraDataList::GetAcousticSpace() {
    auto* pExtra = static_cast<ExtraCellAcousticSpace*>(GetExtraData(EXTRA_CELL_ACOUSTIC_SPACE));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pSpace;
}

TESWaterForm* ExtraDataList::GetWaterType() {
    auto* pExtra = static_cast<ExtraCellWaterType*>(GetExtraData(EXTRA_WATERTYPE));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pWater;
}

int32_t ExtraDataList::GetCanopyShadowMask(NiRenderedTexture*& apOutTex, _D3DLOCKED_RECT*& apOutRect) {
    ExtraCellCanopyShadowMask* pExtra = static_cast<ExtraCellCanopyShadowMask*>(GetExtraData(10));
    apOutTex = nullptr;
    apOutRect = nullptr;
    if (!pExtra) {
        return 0;
    }
    apOutTex = pExtra->spMask.m_pObject;
    apOutRect = &pExtra->Rect;
    return pExtra->eState;
}

SeenData* ExtraDataList::GetSeenData() {
    ExtraSeenData* extraData = static_cast<ExtraSeenData*>(BaseExtraList::GetExtraData(EXTRA_SEENDATA));
    if (extraData) {
        return extraData->pSeenData;
    }

    return nullptr;
}

TESActorBase* ExtraDataList::GetLevCreaTemplate() {
    auto* pExtra = static_cast<ExtraLeveledCreature*>(GetExtraData(EXTRA_LEVELEDCREATURE));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pTemplate;
}

ExtraActorCause* ExtraDataList::GetActorCauseExtra() {
    return static_cast<ExtraActorCause*>(BaseExtraList::GetExtraData(EXTRA_ACTOR_CAUSE));
}

BSMultiBound* ExtraDataList::GetMultiBound() {
    auto* pExtra = static_cast<ExtraMultiBound*>(GetExtraData(EXTRA_MULTIBOUND));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->spBound;
}

void ExtraDataList::SetMultiBound(BSMultiBound* apBound) {
    if (apBound) {
        auto* pExtra = static_cast<ExtraMultiBound*>(GetExtraData(EXTRA_MULTIBOUND));
        if (pExtra) {
            pExtra->spBound = apBound;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraMultiBound));
            pExtra = pMemory ? ::new (pMemory) ExtraMultiBound : nullptr;
            pExtra->spBound = apBound;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_MULTIBOUND);
    }
}

TESObjectREFR* ExtraDataList::GetRandomTeleportMarker() {
    auto* pExtra = static_cast<ExtraRandomTeleportMarker*>(GetExtraData(EXTRA_TELEPORTMARKER));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pMarker;
}

// Stores the marker through the constructor's result even when the allocation failed, as the game does.
void ExtraDataList::SetRandomTeleportMarker(TESObjectREFR* apMarker) {
    if (apMarker) {
        auto* pExtra = static_cast<ExtraRandomTeleportMarker*>(GetExtraData(EXTRA_TELEPORTMARKER));
        if (pExtra) {
            pExtra->pMarker = apMarker;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraRandomTeleportMarker));
            pExtra = pMemory ? ::new (pMemory) ExtraRandomTeleportMarker : nullptr;
            pExtra->pMarker = apMarker;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_TELEPORTMARKER);
    }
}

TESObjectREFR* ExtraDataList::GetMerchantContainer() {
    auto* pExtra = static_cast<ExtraMerchantContainer*>(GetExtraData(EXTRA_MERCHANTCONTAINER));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pContainer;
}

void ExtraDataList::RemoveEnableStateChild(TESObjectREFR* apChild) {
    if (apChild) {
        auto* pExtra = static_cast<ExtraEnableStateChildren*>(GetExtraData(EXTRA_ENABLESTATECHILDREN));
        if (pExtra) {
            pExtra->ChildList.Remove(apChild);
            if (pExtra->ChildList.IsEmpty()) {
                RemoveExtra(pExtra, true);
            }
        }
    }
}

void ExtraDataList::RemoveActivateRefChild(TESObjectREFR* apChild) {
    if (apChild) {
        ExtraActivateRefChildren* pextra = static_cast<ExtraActivateRefChildren*>(GetExtraData(84));
        if (pextra) {
            BSSimpleList<REF_ACTIVATE_DATA*>* plist = &pextra->ChildList;
            BSSimpleList<ListBoxBase*>* plastlist = nullptr;
            while (plist && !plist->IsEmpty()) {
                REF_ACTIVATE_DATA* pdata = plist->m_item;
                if (pdata->pActivateRef == apChild) {
                    plist->RemoveHead();
                    ourFree(pdata);
                    break;
                }
                plist = plist->m_pkNext;
            }
            if (pextra->ChildList.IsEmpty()) {
                RemoveExtra(pextra, true);
            }
        }
    }
}

BSSimpleList<REF_REFLECTOR_DATA*>* ExtraDataList::GetReflectorRefs() {
    ExtraReflectorRefs* pextra = static_cast<ExtraReflectorRefs*>(GetExtraData(102));
    if (pextra) {
        return &pextra->RefList;
    }

    return nullptr;
}

// The package is deleted from the save only when GetLoading (a stub that returns false) says a save is loading.
void ExtraDataList::RemoveTrespassPackage() {
    auto* pExtra = static_cast<ExtraTresPassPackage*>(GetExtraData(EXTRA_TRESPASS_PACKAGE));
    if (pExtra) {
        pExtra->pPack->SetIsCreated(true);
        if (pSaveLoadGameOLD->GetLoading()) {
            pSaveLoadGameOLD->DeleteForm(pExtra->pPack);
        }
        pExtra->pPack = nullptr;
        RemoveExtra(pExtra, true);
    }
}

void ExtraDataList::RemoveActivateParent(TESObjectREFR* apRef) {
    if (apRef) {
        auto* pExtra = static_cast<ExtraActivateRef*>(GetExtraData(EXTRA_ACTIVATE_REF));
        if (pExtra) {
            REF_ACTIVATE_DATA* pData = pExtra->GetData(apRef);
            if (pData) {
                pExtra->ParentList.Remove(pData);
                ourFree(pData);
                if (pExtra->ParentList.IsEmpty()) {
                    RemoveExtra(EXTRA_ACTIVATE_REF);
                }
            }
        }
    }
}

void ExtraDataList::RemoveFollower(Actor* apActor) {
    auto* pExtra = static_cast<ExtraFollower*>(GetExtraData(EXTRA_FOLLOWER));
    if (pExtra) {
        pExtra->pActorlist->Remove(apActor);
        if (!pSaveLoadGameOLD->GetLoading()) {
            pDataHandler->GetClearingData();
        }
    }
}

void ExtraDataList::SetEnableStateInverse(bool abVal) {
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_ENABLESTATEPARENT);
    if (extraData) {
        static_cast<ExtraEnableStateParent*>(extraData)->SetInverse(abVal);
    }
}

TESObjectREFR* ExtraDataList::GetItemDropper() {
    auto* pExtra = static_cast<ExtraItemDropper*>(GetExtraData(EXTRA_ITEMDROPPER));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pDropper;
}

TESObjectREFR* ExtraDataList::GetActionRef() {
    ExtraAction* extraData = static_cast<ExtraAction*>(BaseExtraList::GetExtraData(EXTRA_ACTION));
    if (extraData) {
        return extraData->pActionRef;
    }

    return nullptr;
}

ExtraTalkingActor* ExtraDataList::GetTalkingActorExtra() {
    return static_cast<ExtraTalkingActor*>(BaseExtraList::GetExtraData(EXTRA_TALKING_ACTOR));
}

TESObjectREFR* ExtraDataList::SetInWaterZone(WaterZone* apZone, bool abValue) {
    ExtraWaterZoneMap* pextra = static_cast<ExtraWaterZoneMap*>(GetExtraData(126));
    if (!pextra) {
        ExtraWaterZoneMap* pextra_2;
        ExtraWaterZoneMap* apMemory = static_cast<ExtraWaterZoneMap*>(ourMalloc(sizeof(ExtraWaterZoneMap)));
        if (apMemory) {
            pextra_2 = ::new (apMemory) ExtraWaterZoneMap;
        }
        else {
            pextra_2 = nullptr;
        }
        pextra = pextra_2;
        AddExtra(pextra_2);
    }
    pextra->SetInWaterZone(apZone, static_cast<uint8_t>(abValue));
    if (pextra->WaterZoneMap.m_kAllocator.m_uiCount) {
        if (pextra->pHighestWaterZone) {
            return pextra->pHighestWaterZone->pRef;
        }

        return nullptr;
    }

    RemoveExtra(pextra, true);
    return nullptr;
}

bool ExtraDataList::GetIgnoredBySandbox() {
    return (BaseExtraList::GetExtraData(EXTRA_IGNORED_BY_SANDBOX) != nullptr);
}

TESTopic* ExtraDataList::GetExtraSayToTopic() {
    int32_t v3 = 0;
    ExtraSayToTopicInfo* extraData =
        static_cast<ExtraSayToTopicInfo*>(BaseExtraList::GetExtraData(EXTRA_SAY_TO_TOPIC_INFO));
    if (extraData) {
        return extraData->pTopic;
    }
    return reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(v3));
}

void ExtraDataList::SetActionRef(TESObjectREFR* apRefIn) {
    auto* pExtra = static_cast<ExtraAction*>(GetExtraData(EXTRA_ACTION));
    if (!pExtra) {
        if (apRefIn) {
            pExtra = SetActionPtr();
        }
    }
    else if (pExtra->eAction == 1 && !apRefIn) {
        // only the default action bits are left, so the extra is no longer needed
        RemoveExtra(pExtra, true);
        pExtra = nullptr;
    }
    if (pExtra) {
        pExtra->pActionRef = apRefIn;
    }
}

void ExtraDataList::SetEncounterZone(BGSEncounterZone* apZone) {
    if (apZone) {
        auto* pExtra = static_cast<ExtraEncounterZone*>(GetExtraData(EXTRA_ENCOUNTERZONE));
        if (pExtra) {
            pExtra->pZone = apZone;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraEncounterZone));
            pExtra = pMemory ? ::new (pMemory) ExtraEncounterZone : nullptr;
            pExtra->pZone = apZone;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_ENCOUNTERZONE);
    }
}

// Replaces the copyable extras with copies of apCopy's.
void ExtraDataList::CopyList(ExtraDataList* apCopy) {
    RemoveAllCopyableExtra(true);
    ExtraCritSection.Lock("ExtraDataList::CopyList()");
    if (apCopy) {
        for (BSExtraData* pExtra = apCopy->pHead; pExtra; pExtra = pExtra->GetNext()) {
            AddExtraCopy(pExtra);
        }
    }
    ExtraCritSection.Unlock();
}

// Adds a copy of apCopy through the setter of its type; the list-holding extras get one created when missing and copy
// apCopy's items into it. A talking actor is not copied, and any other type only gets a warning. For a canopy shadow
// mask the copy's Rect is stored through the pointer SetCanopyShadowMask returns in apOutRect, which it writes only
// when the copy's eState is set; with a zero eState it writes through a never-initialised pointer, as the game does.
// The path is believed unreachable: every ExtraCellCanopyShadowMask is created by SetCanopyShadowMask with a nonzero
// state.
void ExtraDataList::AddExtraCopy(BSExtraData* apCopy) {
    if (!apCopy || !IsCopyableExtra(apCopy)) {
        return;
    }

    switch (apCopy->GetExtraType()) {
        case EXTRA_EDITOR_REFMOVE_DATA: {
            auto* pExtra = static_cast<ExtraEditorRefMoveData*>(GetExtraData(EXTRA_EDITOR_REFMOVE_DATA));
            if (!pExtra) {
                void* pMemory = ourMalloc(sizeof(ExtraEditorRefMoveData));
                pExtra = pMemory ? ::new (pMemory) ExtraEditorRefMoveData(nullptr) : nullptr;
                AddExtra(pExtra);
            }
            auto* pCopy = static_cast<ExtraEditorRefMoveData*>(apCopy);
            pExtra->oldLocation = pCopy->oldLocation;
            pExtra->realAngle = pCopy->realAngle;
            pExtra->realLocation = pCopy->realLocation;
            break;
        }
        case EXTRA_ACTION: SetActionFlag(static_cast<OBJECT_ACTION>(static_cast<ExtraAction*>(apCopy)->eAction)); break;
        case EXTRA_OWNERSHIP: SetOwnership(static_cast<ExtraOwnership*>(apCopy)->pOwner); break;
        case EXTRA_GLOBAL: SetGlobal(static_cast<ExtraGlobal*>(apCopy)->pGlobal); break;
        case EXTRA_RANK: SetRank(static_cast<ExtraRank*>(apCopy)->iRank); break;
        case EXTRA_COUNT:
            // The count is zero-extended, so it is passed as unsigned.
            SetCount(static_cast<uint16_t>(static_cast<ExtraCount*>(apCopy)->iCount));
            break;
        case EXTRA_AMMO: {
            auto* pCopy = static_cast<ExtraAmmo*>(apCopy);
            SetAmmoExtra(pCopy->pAmmo, pCopy->iCount);
            break;
        }
        case EXTRA_WORN: SetWorn(true, false); break;
        case EXTRA_HOT_KEY: SetHotKey(static_cast<ExtraHotKey*>(apCopy)->chotkey); break;
        case EXTRA_WORN_LEFT: SetWorn(true, true); break;
        case EXTRA_SEED: SetSeedIndex(static_cast<ExtraSeed*>(apCopy)->iSeed); break;
        case EXTRA_LEVELITEM:
            SetLevelItemExtra(static_cast<ExtraLeveledItem*>(apCopy)->iIndex);
            SetLevelItemExtraFlag(static_cast<ExtraLeveledItem*>(apCopy)->bdefault);
            break;
        case EXTRA_HEALTH: SetHealth(static_cast<ExtraHealth*>(apCopy)->fHealth); break;
        case EXTRA_HEALTH_PERC: SetHealthPerc(static_cast<ExtraHealthPerc*>(apCopy)->fHealthPerc); break;
        case EXTRA_USES: SetUses(static_cast<ExtraUses*>(apCopy)->cUses); break;
        case EXTRA_TIMELEFT: SetTimeLeft(static_cast<ExtraTimeLeft*>(apCopy)->fTime); break;
        case EXTRA_CHARGE: SetCharge(static_cast<ExtraCharge*>(apCopy)->fCharge); break;
        case EXTRA_SCRIPT:
            SetScript(static_cast<ExtraScript*>(apCopy)->pScript);
            SetScriptLocals(static_cast<ExtraScript*>(apCopy)->pScriptVars);
            break;
        case EXTRA_SCALE: SetScale(static_cast<ExtraScale*>(apCopy)->fScale); break;
        case EXTRA_LOCK: {
            void* pMemory = ourMalloc(sizeof(LockData));
            LockData* pLock = pMemory ? ::new (pMemory) LockData : nullptr;
            ourMemcpy(pLock, static_cast<ExtraLock*>(apCopy)->pLock, sizeof(LockData));
            SetLockPtr(pLock);
            break;
        }
        case EXTRA_RADIO_DATA: SetRadioData(&static_cast<ExtraRadioData*>(apCopy)->Data); break;
        case EXTRA_TELEPORT: {
            void* pMemory = ourMalloc(sizeof(DoorTeleportData));
            DoorTeleportData* pTeleport = pMemory ? ::new (pMemory) DoorTeleportData : nullptr;
            pTeleport->Copy(static_cast<ExtraTeleport*>(apCopy)->pTeleportData);
            SetTeleportPtr(pTeleport);
            break;
        }
        case EXTRA_MAPMARKER: {
            void* pMemory = ourMalloc(sizeof(MapMarkerData));
            MapMarkerData* pMapData = pMemory ? ::new (pMemory) MapMarkerData : nullptr;
            pMapData->Copy(static_cast<ExtraMapMarker*>(apCopy)->pMapData);
            SetMapMarkerPtr(pMapData);
            break;
        }
        case EXTRA_AUDIOMARKER: {
            void* pMemory = ourMalloc(sizeof(AudioMarker));
            AudioMarker* pAudioData = pMemory ? ::new (pMemory) AudioMarker : nullptr;
            pAudioData->Copy(static_cast<ExtraAudioMarker*>(apCopy)->pAudioData);
            SetAudioMarkerPtr(pAudioData);
            break;
        }
        case EXTRA_AUDIOBUOYMARKER: {
            void* pMemory = ourMalloc(sizeof(AudioBuoyMarkerData));
            AudioBuoyMarkerData* pBuoyData = pMemory ? ::new (pMemory) AudioBuoyMarkerData : nullptr;
            pBuoyData->Copy(static_cast<ExtraAudioBuoyMarker*>(apCopy)->pAudioBuoyData);
            SetAudioBuoyMarkerPtr(pBuoyData);
            break;
        }
        case EXTRA_PACKAGESTARTLOC: {
            WORLD_LOCATION& rLoc = static_cast<ExtraPackageStartLocation*>(apCopy)->worldLoc;
            auto* pWorld = dynamic_cast<TESWorldSpace*>(rLoc.pLocationForm);
            auto* pCell = dynamic_cast<TESObjectCELL*>(rLoc.pLocationForm);
            SetPackageStartLocation(pWorld, pCell, rLoc.locPt, rLoc.fZRot);
            break;
        }
        case EXTRA_RAGDOLLDATA: SetRagDollDataFromCopy(static_cast<ExtraRagDollData*>(apCopy)->pRagDollData); break;
        case EXTRA_DISTANTDATA: SetDistantDataNormal(static_cast<ExtraDistantData*>(apCopy)->kNormal); break;
        case EXTRA_FOLLOWER:
            for (BSSimpleList<Actor*>* pNode = static_cast<ExtraFollower*>(apCopy)->pActorlist;
                 pNode && pNode->GetItem(); pNode = pNode->GetNext()) {
                AddFollower(pNode->GetItem());
            }
            break;
        case EXTRA_HEAD_TRACK_TARGET:
            SetHeadTrackTargetExtra(static_cast<ExtraHeadingTarget*>(apCopy)->pHeadingTarget);
            break;
        case EXTRA_POISON: SetPoison(static_cast<ExtraPoison*>(apCopy)->pPoison); break;
        case EXTRA_CELL3D: SetCell3D(static_cast<ExtraCell3D*>(apCopy)->spCellNode); break;
        case EXTRA_WATERTYPE: SetWaterType(static_cast<ExtraCellWaterType*>(apCopy)->pWater); break;
        case EXTRA_CELLMUSICTYPE: SetMusicType(static_cast<ExtraCellMusicType*>(apCopy)->pType); break;
        case EXTRA_CELL_ACOUSTIC_SPACE: SetAcousticSpace(static_cast<ExtraCellAcousticSpace*>(apCopy)->pSpace); break;
        case EXTRA_CELL_CANOPYSHADOW_MASK: {
            auto* pCopy = static_cast<ExtraCellCanopyShadowMask*>(apCopy);
            // Deliberately left unset, as the game does: SetCanopyShadowMask writes it only for a nonzero state.
            D3DLOCKED_RECT* pRect;
            SetCanopyShadowMask(pCopy->eState, pCopy->spMask, pRect);
            pRect->pBits = pCopy->Rect.pBits;
            pRect->Pitch = pCopy->Rect.Pitch;
            break;
        }
        case EXTRA_CLIMATE: SetClimate(static_cast<ExtraCellClimate*>(apCopy)->pClimate); break;
        case EXTRA_IMAGESPACE: SetImageSpace(static_cast<ExtraCellImageSpace*>(apCopy)->pImageSpace); break;
        case EXTRA_IMPACTSWAP: {
            void* pMemory = ourMalloc(sizeof(ImpactSwap));
            ImpactSwap* pSwap = pMemory ? ::new (pMemory) ImpactSwap : nullptr;
            pSwap->Copy(static_cast<ExtraCellImpactSwap*>(apCopy)->pImpactSwap);
            SetImpactSwap(pSwap);
            break;
        }
        case EXTRA_REFERENCE_POINTER: SetReferencePointer(static_cast<ExtraReferencePointer*>(apCopy)->pRef); break;
        case EXTRA_PACKAGE: {
            auto* pCopy = static_cast<ExtraPackage*>(apCopy);
            SetPackageExtra(
                pCopy->pPack, pCopy->iindex, pCopy->pTarg, pCopy->bActionComplete, pCopy->bActivated, pCopy->bDoneOnce
            );
            break;
        }
        case EXTRA_TRESPASS_PACKAGE: SetTrespassPackageExtra(static_cast<ExtraTresPassPackage*>(apCopy)->pPack); break;
        case EXTRA_RUN_ONCE_PACKAGES:
            for (BSSimpleList<RunOncePackage*>* pNode = static_cast<ExtraRunOncePacks*>(apCopy)->pPackageList; pNode;
                 pNode = pNode->GetNext()) {
                RunOncePackage* pPackage = pNode->GetItem();
                if (pPackage) {
                    SetRunOnceExtraPackage(pPackage->pPackage, pPackage->cday);
                }
            }
            break;
        case EXTRA_ORIGINAL_REFERENCE:
            SetOriginalReferenceExtra(static_cast<ExtraOriginalReference*>(apCopy)->pReference);
            break;
        case EXTRA_ENABLESTATEPARENT:
            SetEnableStateParent(static_cast<ExtraEnableStateParent*>(apCopy)->pParent);
            SetEnableStateParentFlags(static_cast<ExtraEnableStateParent*>(apCopy)->cFlags);
            break;
        case EXTRA_TELEPORTMARKER:
            SetRandomTeleportMarker(static_cast<ExtraRandomTeleportMarker*>(apCopy)->pMarker);
            break;
        case EXTRA_LINKED_REF: SetLinkedRef(static_cast<ExtraLinkedRef*>(apCopy)->pLinkedRef); break;
        case EXTRA_ACTIVATE_REF: {
            auto* pExtra = static_cast<ExtraActivateRef*>(GetExtraData(EXTRA_ACTIVATE_REF));
            if (!pExtra) {
                void* pMemory = ourMalloc(sizeof(ExtraActivateRef));
                pExtra = pMemory ? ::new (pMemory) ExtraActivateRef : nullptr;
                AddExtra(pExtra);
            }
            pExtra->Copy(apCopy);
            break;
        }
        case EXTRA_DECAL_REFS: {
            auto* pExtra = static_cast<ExtraDecalRefs*>(GetExtraData(EXTRA_DECAL_REFS));
            if (!pExtra) {
                void* pMemory = ourMalloc(sizeof(ExtraDecalRefs));
                pExtra = pMemory ? ::new (pMemory) ExtraDecalRefs : nullptr;
                AddExtra(pExtra);
            }
            pExtra->Copy(apCopy);
            break;
        }
        case EXTRA_REFLECTED_REFS: {
            auto* pExtra = static_cast<ExtraReflectedRefs*>(GetExtraData(EXTRA_REFLECTED_REFS));
            if (!pExtra) {
                void* pMemory = ourMalloc(sizeof(ExtraReflectedRefs));
                pExtra = pMemory ? ::new (pMemory) ExtraReflectedRefs : nullptr;
                AddExtra(pExtra);
            }
            pExtra->Copy(apCopy);
            break;
        }
        case EXTRA_REFLECTOR_REFS: {
            auto* pExtra = static_cast<ExtraReflectorRefs*>(GetExtraData(EXTRA_REFLECTOR_REFS));
            if (!pExtra) {
                void* pMemory = ourMalloc(sizeof(ExtraReflectorRefs));
                pExtra = pMemory ? ::new (pMemory) ExtraReflectorRefs : nullptr;
                AddExtra(pExtra);
            }
            pExtra->Copy(apCopy);
            break;
        }
        case EXTRA_LIT_WATER_REFS: {
            auto* pExtra = static_cast<ExtraLitWaterRefs*>(GetExtraData(EXTRA_LIT_WATER_REFS));
            if (!pExtra) {
                void* pMemory = ourMalloc(sizeof(ExtraLitWaterRefs));
                pExtra = pMemory ? ::new (pMemory) ExtraLitWaterRefs : nullptr;
                AddExtra(pExtra);
            }
            pExtra->Copy(apCopy);
            break;
        }
        case EXTRA_WATER_LIGHT_REFS: {
            auto* pExtra = static_cast<ExtraWaterLightRefs*>(GetExtraData(EXTRA_WATER_LIGHT_REFS));
            if (!pExtra) {
                void* pMemory = ourMalloc(sizeof(ExtraWaterLightRefs));
                pExtra = pMemory ? ::new (pMemory) ExtraWaterLightRefs : nullptr;
                AddExtra(pExtra);
            }
            pExtra->Copy(apCopy);
            break;
        }
        case EXTRA_MERCHANTCONTAINER:
            SetMerchantContainer(static_cast<ExtraMerchantContainer*>(apCopy)->pContainer);
            break;
        case EXTRA_XTARGET: SetXTarget(static_cast<ExtraXTarget*>(apCopy)->pTarget); break;
        case EXTRA_ENCOUNTERZONE: SetEncounterZone(static_cast<ExtraEncounterZone*>(apCopy)->pZone); break;
        case EXTRA_EMITTANCE_SOURCE: SetEmittanceSource(static_cast<ExtraEmittanceSource*>(apCopy)->pSource); break;
        case EXTRA_MULTIBOUND_REF: SetMultiBoundRef(static_cast<ExtraMultiBoundRef*>(apCopy)->pBoundRef); break;
        case EXTRA_MULTIBOUND_DATA: {
            MultiBoundMarkerData* pData = nullptr;
            MultiBoundMarkerData* pCopyData = static_cast<ExtraMultiBoundData*>(apCopy)->pBound;
            if (pCopyData) {
                void* pMemory = ourMalloc(sizeof(MultiBoundMarkerData));
                pData = pMemory ? ::new (pMemory) MultiBoundMarkerData : nullptr;
                pData->Copy(pCopyData);
            }
            SetMultiBoundData(pData);
            break;
        }
        case EXTRA_LEVCREA_MOD: SetLevCreaModifier(static_cast<ExtraLevCreaModifier*>(apCopy)->eModifier); break;
        case EXTRA_REFRACTION_PROPERTY:
            SetRefractionPropertyExtra(true, static_cast<ExtraRefractionProperty*>(apCopy)->fRefractionPower);
            break;
        case EXTRA_CANNOTWEAR: SetCanNotWear(true); break;
        case EXTRA_TALKING_ACTOR: return;
        case EXTRA_RADIUS: SetRadius(static_cast<ExtraRadius*>(apCopy)->fRadius); break;
        case EXTRA_RADIATION: SetRadiation(static_cast<ExtraRadiation*>(apCopy)->fRadiation); break;
        case EXTRA_FACTION_CHANGES: {
            AddFactionChangesExtra();
            ExtraFactionChanges* pChanges = GetFactionChangesExtra();
            for (BSSimpleList<FACTION_RANK*>* pNode = static_cast<ExtraFactionChanges*>(apCopy)->pFactionChanges;
                 pNode && pNode->GetItem(); pNode = pNode->GetNext()) {
                FACTION_RANK* pRank = pNode->GetItem();
                // The rank byte is sign-extended.
                if (pRank) {
                    pChanges->AddToFaction(pRank->pFaction, static_cast<int8_t>(pRank->cRank));
                }
            }
            break;
        }
        case EXTRA_NAVMESH_PORTAL: SetNavMeshPortal(static_cast<ExtraNavMeshPortal*>(apCopy)); break;
        case EXTRA_PRIMITIVE: AddPrimitive(BGSPrimitive::Copy(static_cast<ExtraPrimitive*>(apCopy)->pPrimitive)); break;
        case EXTRA_MULTIBOUND: SetMultiBound(static_cast<ExtraMultiBound*>(apCopy)->spBound); break;
        case EXTRA_ROOM: SetRoom(static_cast<ExtraRoom*>(apCopy)->spRoom); break;
        case EXTRA_PATROL_REF_DATA: {
            void* pMemory = ourMalloc(sizeof(PatrolRefData));
            PatrolRefData* pData = pMemory ? ::new (pMemory) PatrolRefData : nullptr;
            pData->Copy(static_cast<ExtraPatrolRefData*>(apCopy)->pPatrolData);
            SetPatrolRefData(pData);
            break;
        }
        case EXTRA_OCCLUSION_PLANE: SetOcclusionPlane(static_cast<ExtraOcclusionPlane*>(apCopy)->spPlane); break;
        case EXTRA_OCCLUSION_PLANE_REF_DATA: {
            void* pMemory = ourMalloc(sizeof(OcclusionPlaneLinkedRefData));
            OcclusionPlaneLinkedRefData* pData = pMemory ? ::new (pMemory) OcclusionPlaneLinkedRefData : nullptr;
            OcclusionPlaneLinkedRefData* pCopyData = static_cast<ExtraOcclusionPlaneRefData*>(apCopy)->pData;
            for (int32_t i = 0; i < 4; ++i) {
                pData->pLinkedRefs[i] = pCopyData->pLinkedRefs[i];
            }
            SetOcclusionPlaneLinkedRefData(pData);
            break;
        }
        case EXTRA_PORTAL_REF_DATA: {
            PortalLinkedRefData* pCopyData = static_cast<ExtraPortalRefData*>(apCopy)->pData;
            for (int32_t i = 0; i < 2; ++i) {
                SetPortalLinkedRoomRef(i, pCopyData->pLinkedRefs[i]);
            }
            break;
        }
        case EXTRA_ROOM_REF_DATA: {
            RoomRefData* pCopyData = static_cast<ExtraRoomRefData*>(apCopy)->pData;
            for (BSSimpleList<TESObjectREFR*>* pNode = &pCopyData->kRoomList; pNode && pNode->GetItem();
                 pNode = pNode->GetNext()) {
                AddRoomLinkedRoomRef(pNode->GetItem());
            }
            SetRoomIsMaster(pCopyData->ucRoomCount != 0);
            break;
        }
        case EXTRA_COLLISION_DATA: {
            void* pMemory = ourMalloc(sizeof(CollisionData));
            CollisionData* pData = pMemory ? ::new (pMemory) CollisionData : nullptr;
            pData->SetLayer(static_cast<ExtraCollisionData*>(apCopy)->pCollisionData->QLayer());
            SetCollisionData(pData);
            break;
        }
        case EXTRA_SAY_ONCE_A_DAY_TOPIC_INFO:
            for (BSSimpleList<SayOnceTopicInfos*>* pNode =
                     static_cast<ExtraSayTopicInfoOnceADay*>(apCopy)->pListofSaidOnceTopicInfos;
                 pNode && pNode->GetItem();) {
                SetExtraSayTopicInfoOnceADayExtra(pNode->GetItem());
                // The result of GetNext is deliberately dropped, as the game does, so the walk never advances past a
                // non-empty first node.
                static_cast<void>(pNode->GetNext());
            }
            break;
        case EXTRA_PORTAL:
            // apCopy is never null here, so there is no fallback for a null apCopy.
            SetPortal(static_cast<ExtraPortal*>(apCopy)->spPortal);
            break;
        case EXTRA_IGNORED_BY_SANDBOX: SetIgnoredBySandbox(true); break;
        case EXTRA_FOLLOWER_SWIM_BREADCRUMBS: {
            ExtraFollowerSwimBreadcrumbs* pCrumbs = GetOrCreateExtraFollowerSwimBreadcrumbs();
            auto* pCopy = static_cast<ExtraFollowerSwimBreadcrumbs*>(apCopy);
            pCrumbs->LeaderLocation = pCopy->LeaderLocation;
            pCrumbs->LeaderNavMeshID = pCopy->LeaderNavMeshID;
            pCrumbs->eLeaderState = pCopy->eLeaderState;
            pCrumbs->CrumbList.RemoveAll();
            for (BSSimpleList<ExtraFollowerSwimBreadcrumb*>* pNode = &pCopy->CrumbList; pNode;
                 pNode = pNode->GetNext()) {
                if (pNode->GetItem()) {
                    pCrumbs->CrumbList.AddTail(pNode->GetItem());
                }
            }
            break;
        }
        case EXTRA_WEAPON_MOD_SLOTS: {
            auto* pCopy = static_cast<ExtraWeaponModFlags*>(apCopy);
            if (pCopy->GetWeaponModActive(1)) {
                SetWeaponModSlot(1);
            }
            if (pCopy->GetWeaponModActive(2)) {
                SetWeaponModSlot(2);
            }
            if (pCopy->GetWeaponModActive(4)) {
                SetWeaponModSlot(4);
            }
            break;
        }
        case EXTRA_SPECIAL_RENDER_FLAGS: {
            auto* pExtra = static_cast<ExtraSpecialRenderFlags*>(GetExtraData(EXTRA_SPECIAL_RENDER_FLAGS));
            if (!pExtra) {
                void* pMemory = ourMalloc(sizeof(ExtraSpecialRenderFlags));
                pExtra = pMemory ? ::new (pMemory) ExtraSpecialRenderFlags : nullptr;
                AddExtra(pExtra);
            }
            // Slot 3 (+0xC), called through the vtable.
            pExtra->Copy(apCopy);
            break;
        }
        default: Warning("AI: No Copy function available for Extra Data type %i.", apCopy->GetExtraType()); break;
    }
}

void ExtraDataList::CopyListForReference(ExtraDataList* apCopy, bool abremove) {
    ExtraCritSection.Lock("ExtraDataList::CopyListForReference()");
    BSExtraData* pExtra = apCopy->pHead;
    while (pExtra) {
        bool bAdvance = true;
        switch (pExtra->GetExtraType()) {
            case EXTRA_SCRIPT:
                if (abremove) {
                    AddExtraCopy(pExtra);
                    apCopy->RemoveExtra(pExtra, false);
                    pExtra = apCopy->pHead;
                    bAdvance = false;
                }
                else {
                    Script* pScript = apCopy->GetScript();
                    void* pMemory = ourMalloc(sizeof(ExtraScript));
                    ExtraScript* pExtraScript = pMemory ? ::new (pMemory) ExtraScript(pScript) : nullptr;
                    AddExtra(pExtraScript);
                    SetScriptLocals(apCopy->GetScript()->CloneLocals());
                }
                break;
            case EXTRA_WORN:
            case EXTRA_REFERENCE_POINTER:
            case EXTRA_OWNERSHIP:
            case EXTRA_GLOBAL:
            case EXTRA_RANK:
            case EXTRA_HEALTH:
            case EXTRA_USES:
            case EXTRA_TIMELEFT:
            case EXTRA_CHARGE:
            case EXTRA_LEVELITEM:
            case EXTRA_SCALE:
            case EXTRA_POISON:
            case EXTRA_WEAPON_MOD_SLOTS:
                AddExtraCopy(pExtra);
                if (abremove) {
                    apCopy->RemoveExtra(pExtra, true);
                    pExtra = apCopy->pHead;
                    bAdvance = false;
                }
                break;
            default: break;
        }
        if (pExtra && bAdvance) {
            pExtra = pExtra->GetNext();
        }
    }
    ExtraCritSection.Unlock();
}

// A null extra counts as copyable.
bool ExtraDataList::IsCopyableExtra(BSExtraData* apExtra) {
    bool bCopyable = true;
    if (apExtra) {
        switch (apExtra->GetExtraType()) {
            case EXTRA_HAVOK:
            case EXTRA_CELL3D:
            case EXTRA_REGIONLIST:
            case EXTRA_PERSISTENT_CELL:
            case EXTRA_STARTINGPOSITION:
            case EXTRA_ANIM:
            case EXTRA_BIPED:
            case EXTRA_CONTAINER_CHANGES:
            case EXTRA_LIGHT:
            case EXTRA_MASTERFILECELL:
            case EXTRA_MAGIC_LIGHT:
            case EXTRA_EDITOR_REF3D_DATA:
            case EXTRA_LINKED_REF_CHILDREN:
            case EXTRA_ACTIVATE_REF_CHILDREN:
            case EXTRA_TALKING_ACTOR:
            case EXTRA_DECAL_TARGETS: bCopyable = false; break;
            default: break;
        }
    }
    return bCopyable;
}

void ExtraDataList::SetContainerChanges(InventoryChanges* apChanges) {
    if (!apChanges) {
        return;
    }
    auto* pExtra = static_cast<ExtraContainerChanges*>(GetExtraData(EXTRA_CONTAINER_CHANGES));
    if (pExtra) {
        pExtra->pChanges = apChanges;
    }
    else {
        void* pMemory = ourMalloc(sizeof(ExtraContainerChanges));
        pExtra = pMemory ? ::new (pMemory) ExtraContainerChanges(apChanges) : nullptr;
        AddExtra(pExtra);
    }
}

// true when every extra is of a type the container keeps on a default instance: script, owner, reference pointer,
// original reference, count, time left, leveled item, scale and hotkey, plus worn when bwornignore is set.
bool ExtraDataList::IsExtraDefaultforContainer(bool bwornignore) {
    ExtraCritSection.Lock(nullptr);
    for (BSExtraData* pExtra = pHead; pExtra; pExtra = pExtra->GetNext()) {
        if (bwornignore) {
            switch (pExtra->GetExtraType()) {
                case EXTRA_SCRIPT:
                case EXTRA_WORN:
                case EXTRA_REFERENCE_POINTER:
                case EXTRA_ORIGINAL_REFERENCE:
                case EXTRA_OWNERSHIP:
                case EXTRA_COUNT:
                case EXTRA_TIMELEFT:
                case EXTRA_LEVELITEM:
                case EXTRA_SCALE:
                case EXTRA_HOT_KEY: break;
                default: ExtraCritSection.Unlock(); return false;
            }
        }
        else {
            switch (pExtra->GetExtraType()) {
                case EXTRA_SCRIPT:
                case EXTRA_REFERENCE_POINTER:
                case EXTRA_ORIGINAL_REFERENCE:
                case EXTRA_OWNERSHIP:
                case EXTRA_COUNT:
                case EXTRA_TIMELEFT:
                case EXTRA_LEVELITEM:
                case EXTRA_SCALE:
                case EXTRA_HOT_KEY: break;
                default: ExtraCritSection.Unlock(); return false;
            }
        }
    }
    ExtraCritSection.Unlock();
    return true;
}

void ExtraDataList::SetWorn(bool abWorn, bool abwearleft) {
    if (abwearleft) {
        bool bHasWorn = HasExtra(EXTRA_WORN_LEFT);
        if (abWorn && !bHasWorn) {
            void* pMemory = ourMalloc(sizeof(ExtraWornLeft));
            AddExtra(pMemory ? ::new (pMemory) ExtraWornLeft : nullptr);
        }
        else if (!abWorn && bHasWorn) {
            RemoveExtra(EXTRA_WORN_LEFT);
        }
    }
    else {
        bool bHasWorn = HasExtra(EXTRA_WORN);
        if (abWorn && !bHasWorn) {
            void* pMemory = ourMalloc(sizeof(ExtraWorn));
            AddExtra(pMemory ? ::new (pMemory) ExtraWorn : nullptr);
        }
        else if (!abWorn && bHasWorn) {
            RemoveExtra(EXTRA_WORN);
        }
    }
}

// true when apCompare holds an extra this list lacks or holds differently (BSExtraData::Compare). Counts are never
// compared, nor hotkeys, nor worn extras when bThrown is set; with abCompareScriptAndOwnerData clear, any script
// or owner extra in apCompare counts as a difference.
bool ExtraDataList::CompareListForContainer(ExtraDataList* apCompare, bool abCompareScriptAndOwnerData, bool bThrown) {
    ExtraCritSection.Lock("ExtraDataList::CompareListForContainer()");
    bool bDifferent = false;
    BSExtraData* pExtra = apCompare->pHead;
    while (pExtra) {
        if (!abCompareScriptAndOwnerData) {
            if (pExtra->GetExtraType() == EXTRA_SCRIPT) {
                bDifferent = true;
                break;
            }
            if (pExtra->GetExtraType() == EXTRA_OWNERSHIP) {
                bDifferent = true;
                break;
            }
        }
        if (pExtra->GetExtraType() == EXTRA_WORN && bThrown) {
            pExtra = pExtra->GetNext();
            continue;
        }
        if (pExtra->GetExtraType() != EXTRA_COUNT && pExtra->GetExtraType() != EXTRA_HOT_KEY) {
            BSExtraData* pMine = GetExtraData(static_cast<EXTRA_DATA_TYPE>(pExtra->GetExtraType()));
            if (!pMine) {
                bDifferent = true;
                break;
            }
            if (pMine->Compare(pExtra)) {
                bDifferent = true;
                break;
            }
        }
        pExtra = pExtra->GetNext();
    }
    ExtraCritSection.Unlock();
    return bDifferent;
}

bool ExtraDataList::CompareList(ExtraDataList* apCompare) {
    BaseExtraList::ExtraCritSection.Lock("ExtraDataList::CompareList()");
    bool v10 = false;
    for (BSExtraData* i = apCompare->pHead; i; i = i->GetNext()) {
        if (IsCopyableExtra(i)) {
            uint8_t ExtraType = i->GetExtraType();
            BSExtraData* ExtraData = GetExtraData(ExtraType);
            if (!ExtraData) {
                v10 = true;
                break;
            }
            if (ExtraData->Compare(i)) {
                v10 = true;
                break;
            }
        }
    }
    if (!v10) {
        for (BSExtraData* j = pHead; j; j = j->GetNext()) {
            if (IsCopyableExtra(j)) {
                uint8_t v3 = j->GetExtraType();
                BSExtraData* v8 = apCompare->GetExtraData(v3);
                if (!v8) {
                    v10 = true;
                    break;
                }
                if (v8->Compare(j)) {
                    v10 = true;
                    break;
                }
            }
        }
    }
    uint32_t v4 = ItemsInList();
    if (v4 != apCompare->ItemsInList()) {
        for (BSExtraData* k = pHead; k; k = k->GetNext()) {
            if (IsCopyableExtra(k)) {
                uint8_t v5 = k->GetExtraType();
                if (!apCompare->GetExtraData(v5)) {
                    v10 = true;
                    break;
                }
            }
        }
    }
    BaseExtraList::ExtraCritSection.Unlock();
    return v10;
}

void ExtraDataList::SetCount(int16_t aiCount) {
    const int16_t sCount = aiCount;
    auto* pExtra = static_cast<ExtraCount*>(GetExtraData(EXTRA_COUNT));
    if (sCount == 0 || sCount == 1) {
        if (pExtra) {
            RemoveExtra(pExtra, true);
        }
    }
    else if (pExtra) {
        pExtra->iCount = sCount;
    }
    else {
        void* pMemory = ourMalloc(sizeof(ExtraCount));
        pExtra = pMemory ? ::new (pMemory) ExtraCount(static_cast<uint16_t>(sCount)) : nullptr;
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetCanNotWear(bool abCantWear) {
    bool bHasCannotWear = HasExtra(EXTRA_CANNOTWEAR);
    if (abCantWear && !bHasCannotWear) {
        void* pMemory = ourMalloc(sizeof(ExtraCannotWear));
        AddExtra(pMemory ? ::new (pMemory) ExtraCannotWear : nullptr);
    }
    else if (!abCantWear && bHasCannotWear) {
        RemoveExtra(EXTRA_CANNOTWEAR);
    }
}

void ExtraDataList::RemoveOwnership() {
    BaseExtraList::RemoveExtra(EXTRA_OWNERSHIP);
}

// Only persistent references are kept, except while a save loads.
void ExtraDataList::SetReferencePointer(TESObjectREFR* apRef) {
    if (pSaveLoadGameOLD->GetLoading() || !apRef || apRef->GetRefPersists()) {
        auto* pExtra = static_cast<ExtraReferencePointer*>(GetExtraData(EXTRA_REFERENCE_POINTER));
        if (pExtra) {
            pExtra->pRef = apRef;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraReferencePointer));
            pExtra = pMemory ? ::new (pMemory) ExtraReferencePointer(apRef) : nullptr;
            AddExtra(pExtra);
        }
    }
}

char ExtraDataList::GetHotKey() {
    auto* pExtra = static_cast<ExtraHotKey*>(GetExtraData(EXTRA_HOT_KEY));
    if (!pExtra) {
        return -1;
    }
    return pExtra->chotkey;
}

void ExtraDataList::RemoveHotKey() {
    BaseExtraList::RemoveExtra(EXTRA_HOT_KEY);
}

TESClimate* ExtraDataList::GetClimate() {
    auto* pExtra = static_cast<ExtraCellClimate*>(GetExtraData(EXTRA_CLIMATE));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pClimate;
}

TESRegionList* ExtraDataList::GetRegionList() {
    ExtraRegionList* extraData = static_cast<ExtraRegionList*>(BaseExtraList::GetExtraData(EXTRA_REGIONLIST));
    if (extraData) {
        return extraData->pList;
    }

    return nullptr;
}

// A different old list is destroyed through its virtual deleting destructor.
void ExtraDataList::SetRegionList(TESRegionList* apList) {
    auto* pExtra = static_cast<ExtraRegionList*>(GetExtraData(EXTRA_REGIONLIST));
    if (apList) {
        if (pExtra) {
            if (pExtra->pList && pExtra->pList != apList && pExtra->pList) {
                delete pExtra->pList;
            }
            pExtra->pList = apList;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraRegionList));
            pExtra = pMemory ? ::new (pMemory) ExtraRegionList(apList) : nullptr;
            AddExtra(pExtra);
        }
    }
    else if (pExtra) {
        RemoveExtra(pExtra, true);
    }
}

void ExtraDataList::SetScript(Script* apScript) {
    auto* pExtra = static_cast<ExtraScript*>(GetExtraData(EXTRA_SCRIPT));
    if (pExtra) {
        pExtra->pScript = apScript;
    }
    else {
        void* pMemory = ourMalloc(sizeof(ExtraScript));
        pExtra = pMemory ? ::new (pMemory) ExtraScript(apScript) : nullptr;
        AddExtra(pExtra);
    }
}

// Unlinks every copyable extra (IsCopyableExtra), destroying it when abdelete is set. The presence bits are left
// as they are.
void ExtraDataList::RemoveAllCopyableExtra(bool abdelete) {
    ExtraCritSection.Lock("ExtraDataList::RemoveAllCopyableExtra()");
    BSExtraData* pExtra = pHead;
    BSExtraData* pPrev = nullptr;
    ClearLastExtraAll(this);
    while (pExtra) {
        BSExtraData* pCurrent = pExtra;
        pExtra = pCurrent->GetNext();
        if (IsCopyableExtra(pCurrent)) {
            if (pCurrent == pHead) {
                pHead = pExtra;
            }
            if (pPrev) {
                pPrev->SetNext(pExtra);
            }
            if (abdelete) {
                delete pCurrent;
            }
        }
        else {
            pPrev = pCurrent;
        }
    }
    ExtraCritSection.Unlock();
}

NiPoint3 ExtraDataList::SetStartingAngle(TESObjectREFR* apObject, NiPoint3 newAngle) {
    auto* pExtra = static_cast<ExtraStartingPosition*>(GetExtraData(EXTRA_STARTINGPOSITION));
    if (!pExtra) {
        pExtra = SetStartingPositionPtr(apObject);
    }
    pExtra->startRotation = newAngle;
    return newAngle;
}

NiPoint3 ExtraDataList::SetStartingLocation(TESObjectREFR* apObject, NiPoint3 newLocation) {
    auto* pExtra = static_cast<ExtraStartingPosition*>(GetExtraData(EXTRA_STARTINGPOSITION));
    if (!pExtra) {
        pExtra = SetStartingPositionPtr(apObject);
    }
    pExtra->startPosition = newLocation;
    return newLocation;
}

int32_t ExtraDataList::GetExtraLevelItemIndex() {
    auto* pExtra = static_cast<ExtraLeveledItem*>(GetExtraData(EXTRA_LEVELITEM));
    if (!pExtra) {
        return -1;
    }
    return pExtra->iIndex;
}

void ExtraDataList::SetLevelItemExtraFlag(bool bflag) {
    auto* pExtra = static_cast<ExtraLeveledItem*>(GetExtraData(EXTRA_LEVELITEM));
    if (pExtra) {
        pExtra->bdefault = bflag;
    }
}

bool ExtraDataList::IsEnableStateInverse() {
    auto* pExtra = static_cast<ExtraEnableStateParent*>(GetExtraData(EXTRA_ENABLESTATEPARENT));
    if (!pExtra) {
        return false;
    }
    return pExtra->IsInverse();
}

void ExtraDataList::SetEnableStateParentFlags(uint8_t acFlags) {
    auto* pExtra = static_cast<ExtraEnableStateParent*>(GetExtraData(EXTRA_ENABLESTATEPARENT));
    if (pExtra) {
        pExtra->cFlags = acFlags;
    }
}

float ExtraDataList::GetHealthPerc() {
    auto* pExtra = static_cast<ExtraHealthPerc*>(GetExtraData(EXTRA_HEALTH_PERC));
    if (!pExtra) {
        return 1.0f;
    }
    return pExtra->fHealthPerc;
}

float ExtraDataList::GetHealth() {
    auto* pExtra = static_cast<ExtraHealth*>(GetExtraData(EXTRA_HEALTH));
    if (!pExtra) {
        return -1.0f;
    }
    return pExtra->fHealth;
}

float ExtraDataList::GetRadius() {
    auto* pExtra = static_cast<ExtraRadius*>(GetExtraData(EXTRA_RADIUS));
    if (!pExtra) {
        return 0.0f;
    }
    return pExtra->fRadius;
}

float ExtraDataList::GetActivateChildrenTimer() {
    auto* pExtra = static_cast<ExtraActivateRefChildren*>(GetExtraData(EXTRA_ACTIVATE_REF_CHILDREN));
    if (!pExtra) {
        return -1.0f;
    }
    return pExtra->fActivateChildrenTimer;
}

int32_t ExtraDataList::GetRank() {
    auto* pExtra = static_cast<ExtraRank*>(GetExtraData(EXTRA_RANK));
    if (!pExtra) {
        return -1;
    }
    return pExtra->iRank;
}

int16_t ExtraDataList::GetCount() {
    auto* pExtra = static_cast<ExtraCount*>(GetExtraData(EXTRA_COUNT));
    if (!pExtra) {
        return 1;
    }
    return pExtra->iCount;
}

void ExtraDataList::SetPersistentCell(TESObjectCELL* apCell) {
    auto* pExtra = static_cast<ExtraPersistentCell*>(GetExtraData(EXTRA_PERSISTENT_CELL));
    if (pExtra) {
        if (!apCell) {
            RemoveExtra(pExtra, true);
        }
        else {
            pExtra->pPersistentCell = apCell;
        }
    }
    else if (apCell) {
        void* pMemory = ourMalloc(sizeof(ExtraPersistentCell));
        pExtra = pMemory ? ::new (pMemory) ExtraPersistentCell(apCell) : nullptr;
        AddExtra(pExtra);
    }
}

// Stores the swap through the constructor's result even when the allocation failed, as the game does.
void ExtraDataList::SetImpactSwap(ImpactSwap* apImpactSwap) {
    auto* pExtra = static_cast<ExtraCellImpactSwap*>(GetExtraData(EXTRA_IMPACTSWAP));
    if (apImpactSwap) {
        if (pExtra) {
            pExtra->pImpactSwap = apImpactSwap;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraCellImpactSwap));
            pExtra = pMemory ? ::new (pMemory) ExtraCellImpactSwap : nullptr;
            pExtra->pImpactSwap = apImpactSwap;
            AddExtra(pExtra);
        }
    }
    else if (pExtra) {
        RemoveExtra(pExtra, true);
    }
}

// The schedule's GetStartTime is called before GetDuration. A dropped package is freed with ourFree, as the game does,
// and the walk restarts at the list head.
void ExtraDataList::CheckRemoveRunOnceDayPackage(char acday) {
    auto* pExtra = static_cast<ExtraRunOncePacks*>(GetExtraData(EXTRA_RUN_ONCE_PACKAGES));
    if (!pExtra) {
        return;
    }
    BSSimpleList<RunOncePackage*>* pNode = pExtra->pPackageList;
    while (pNode && pNode->GetItem()) {
        RunOncePackage* pRunOnce = pNode->GetItem();
        bool bPastEndOfDay = false;
        if (pRunOnce && pRunOnce->pPackage) {
            PackageSchedule* pSchedule = pRunOnce->pPackage->GetPackageSchedule();
            int32_t iStartTime = pSchedule->GetStartTime();
            if (iStartTime + pSchedule->GetDuration() >= 21) {
                bPastEndOfDay = true;
            }
        }
        if (pRunOnce->cday != acday || bPastEndOfDay) {
            pExtra->pPackageList->Remove(pRunOnce);
            ourFree(pRunOnce);
            pNode = pExtra->pPackageList;
        }
        else {
            pNode = pNode->GetNext();
        }
    }
}

bool ExtraDataList::IsInRunOnceDayPackageList(TESPackage* apPackage) {
    auto* pExtra = static_cast<ExtraRunOncePacks*>(GetExtraData(EXTRA_RUN_ONCE_PACKAGES));
    if (!pExtra) {
        return false;
    }
    for (BSSimpleList<RunOncePackage*>* pNode = pExtra->pPackageList; pNode && pNode->GetItem();
         pNode = pNode->GetNext()) {
        if (pNode->GetItem()->pPackage == apPackage) {
            return true;
        }
    }
    return false;
}

void ExtraDataList::SetEnableStateParent(TESObjectREFR* apParent) {
    if (apParent) {
        auto* pExtra = static_cast<ExtraEnableStateParent*>(GetExtraData(EXTRA_ENABLESTATEPARENT));
        if (pExtra) {
            pExtra->pParent = apParent;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraEnableStateParent));
            pExtra = pMemory ? ::new (pMemory) ExtraEnableStateParent : nullptr;
            pExtra->pParent = apParent;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_ENABLESTATEPARENT);
    }
}

void ExtraDataList::AddEnableStateChild(TESObjectREFR* apChild) {
    if (!apChild) {
        return;
    }
    auto* pExtra = static_cast<ExtraEnableStateChildren*>(GetExtraData(EXTRA_ENABLESTATECHILDREN));
    if (!pExtra) {
        void* pMemory = ourMalloc(sizeof(ExtraEnableStateChildren));
        pExtra = pMemory ? ::new (pMemory) ExtraEnableStateChildren : nullptr;
        AddExtra(pExtra);
    }
    if (!pExtra->ChildList.IsInList(apChild)) {
        pExtra->ChildList.AddHead(apChild);
    }
}

void ExtraDataList::SetWaterType(TESWaterForm* apWater) {
    if (apWater) {
        auto* pExtra = static_cast<ExtraCellWaterType*>(GetExtraData(EXTRA_WATERTYPE));
        if (pExtra) {
            pExtra->pWater = apWater;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraCellWaterType));
            pExtra = pMemory ? ::new (pMemory) ExtraCellWaterType : nullptr;
            pExtra->pWater = apWater;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_WATERTYPE);
    }
}

void ExtraDataList::SetLinkedRef(TESObjectREFR* apRef) {
    if (apRef) {
        auto* pExtra = static_cast<ExtraLinkedRef*>(GetExtraData(EXTRA_LINKED_REF));
        if (pExtra) {
            pExtra->pLinkedRef = apRef;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraLinkedRef));
            pExtra = pMemory ? ::new (pMemory) ExtraLinkedRef : nullptr;
            pExtra->pLinkedRef = apRef;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_LINKED_REF);
    }
}

void ExtraDataList::AddLinkedRefChild(TESObjectREFR* apChild) {
    if (!apChild) {
        return;
    }
    auto* pExtra = static_cast<ExtraLinkedRefChildren*>(GetExtraData(EXTRA_LINKED_REF_CHILDREN));
    if (!pExtra) {
        void* pMemory = ourMalloc(sizeof(ExtraLinkedRefChildren));
        pExtra = pMemory ? ::new (pMemory) ExtraLinkedRefChildren : nullptr;
        AddExtra(pExtra);
    }
    if (!pExtra->ChildList.IsInList(apChild)) {
        pExtra->ChildList.AddHead(apChild);
    }
}

// A missing extra is created only for a non-null text; an existing one takes a null text too.
void ExtraDataList::SetActivateTextOverride(const char* pTextOverride) {
    auto* pExtra = static_cast<ExtraActivateRef*>(GetExtraData(EXTRA_ACTIVATE_REF));
    if (!pExtra && pTextOverride) {
        void* pMemory = ourMalloc(sizeof(ExtraActivateRef));
        pExtra = pMemory ? ::new (pMemory) ExtraActivateRef : nullptr;
        AddExtra(pExtra);
    }
    if (pExtra) {
        pExtra->ActivateTextOverride.Set(pTextOverride, 0);
    }
}

// The walk stops at the first match.
void ExtraDataList::AddActivateRefChild(TESObjectREFR* apChild) {
    if (!apChild) {
        return;
    }
    auto* pExtra = static_cast<ExtraActivateRefChildren*>(GetExtraData(EXTRA_ACTIVATE_REF_CHILDREN));
    if (!pExtra) {
        void* pMemory = ourMalloc(sizeof(ExtraActivateRefChildren));
        pExtra = pMemory ? ::new (pMemory) ExtraActivateRefChildren : nullptr;
        AddExtra(pExtra);
    }
    bool bFound = false;
    for (BSSimpleList<REF_ACTIVATE_DATA*>* pNode = &pExtra->ChildList; pNode && !pNode->IsEmpty() && !bFound;
         pNode = pNode->GetNext()) {
        if (pNode->GetItem()->pActivateRef == apChild) {
            bFound = true;
        }
    }
    if (!bFound) {
        void* pMemory = ourMalloc(sizeof(REF_ACTIVATE_DATA));
        REF_ACTIVATE_DATA* pData = pMemory ? ::new (pMemory) REF_ACTIVATE_DATA : nullptr;
        pData->pActivateRef = apChild;
        pExtra->ChildList.AddHead(pData);
    }
}

// An entry's refraction flag is the value 2 in iEffectFlags.
void ExtraDataList::SetRefractedByRef(TESObjectREFR* apRef, bool bValue) {
    auto* pExtra = static_cast<ExtraReflectorRefs*>(GetExtraData(EXTRA_REFLECTOR_REFS));
    if (pExtra) {
        for (BSSimpleList<REF_REFLECTOR_DATA*>* pNode = &pExtra->RefList; pNode && pNode->GetItem();
             pNode = pNode->GetNext()) {
            if (pNode->GetItem()->pRef != apRef) {
                continue;
            }
            if (bValue) {
                pNode->GetItem()->iEffectFlags |= 2;
            }
            else {
                pNode->GetItem()->iEffectFlags &= ~2u;
                if (pNode->GetItem()->iEffectFlags == 0) {
                    pExtra->RemoveReference(apRef);
                    if (pExtra->RefList.IsEmpty()) {
                        RemoveExtra(pExtra, true);
                    }
                }
            }
            return;
        }
    }
    if (bValue) {
        if (!pExtra) {
            void* pMemory = ourMalloc(sizeof(ExtraReflectorRefs));
            pExtra = pMemory ? ::new (pMemory) ExtraReflectorRefs : nullptr;
            AddExtra(pExtra);
        }
        pExtra->SetReference(apRef, 2);
    }
}

// An entry's refraction flag is the value 2 in iEffectFlags.
void ExtraDataList::SetRefractsRef(TESObjectREFR* apRef, bool bValue) {
    auto* pExtra = static_cast<ExtraReflectedRefs*>(GetExtraData(EXTRA_REFLECTED_REFS));
    if (pExtra) {
        for (BSSimpleList<REF_REFLECTED_DATA*>* pNode = &pExtra->RefList; pNode && pNode->GetItem();
             pNode = pNode->GetNext()) {
            if (pNode->GetItem()->pRef != apRef) {
                continue;
            }
            if (bValue) {
                pNode->GetItem()->iEffectFlags |= 2;
            }
            else {
                pNode->GetItem()->iEffectFlags &= ~2u;
                if (pNode->GetItem()->iEffectFlags == 0) {
                    pExtra->RemoveReference(apRef);
                    if (pExtra->RefList.IsEmpty()) {
                        RemoveExtra(pExtra, true);
                    }
                }
            }
            return;
        }
    }
    if (bValue) {
        if (!pExtra) {
            void* pMemory = ourMalloc(sizeof(ExtraReflectedRefs));
            pExtra = pMemory ? ::new (pMemory) ExtraReflectedRefs : nullptr;
            AddExtra(pExtra);
        }
        pExtra->SetReference(apRef, 2);
    }
}

void ExtraDataList::SetReflectsRef(TESObjectREFR* apRef, bool bValue) {
    ExtraReflectedRefs* pextra = static_cast<ExtraReflectedRefs*>(GetExtraData(101));
    if (pextra) {
        for (REF_REFLECTED_DATA*& rpItem : pextra->RefList.ItemsUntilNull()) {
            if (rpItem->pRef == apRef) {
                if (bValue) {
                    uint32_t* p_iEffectFlags = &rpItem->iEffectFlags;
                    *p_iEffectFlags |= 1u;
                }
                else {
                    uint32_t* v4 = &rpItem->iEffectFlags;
                    *v4 &= ~1u;
                    if (!rpItem->iEffectFlags) {
                        pextra->RemoveReference(apRef);
                        if (pextra->RefList.IsEmpty()) {
                            RemoveExtra(pextra, true);
                        }
                    }
                }
                return;
            }
        }
    }
    if (bValue) {
        if (!pextra) {
            ExtraReflectedRefs* pextra_2;
            ExtraReflectedRefs* apMemory = static_cast<ExtraReflectedRefs*>(ourMalloc(sizeof(ExtraReflectedRefs)));
            if (apMemory) {
                pextra_2 = ::new (apMemory) ExtraReflectedRefs;
            }
            else {
                pextra_2 = nullptr;
            }
            pextra = pextra_2;
            AddExtra(pextra_2);
        }
        pextra->SetReference(apRef, 1u);
    }
}

void ExtraDataList::SetWaterLightRef(TESObjectREFR* apRef, bool bValue) {
    ExtraWaterLightRefs* pExtra = static_cast<ExtraWaterLightRefs*>(GetExtraData(EXTRA_WATER_LIGHT_REFS));
    if (pExtra) {
        BSSimpleList<TESObjectREFR*>* pRefList = &pExtra->RefList;
        if (bValue) {
            if (!pRefList->IsInList(apRef)) {
                pRefList->AddHead(apRef);
            }
        }
        else {
            pRefList->Remove(apRef);
        }
    }
    else {
        if (bValue) {
            ExtraWaterLightRefs* pNewExtra;
            void* pMemory = ourMalloc(sizeof(ExtraWaterLightRefs));
            if (pMemory) {
                pNewExtra = ::new (pMemory) ExtraWaterLightRefs;
            }
            else {
                pNewExtra = nullptr;
            }
            AddExtra(pNewExtra);
            pNewExtra->RefList.AddHead(apRef);
        }
    }
}

void ExtraDataList::AddPrimitive(BGSPrimitive* apPrimitive) {
    if (!apPrimitive) {
        return;
    }
    void* pMemory = ourMalloc(sizeof(ExtraPrimitive));
    ExtraPrimitive* pExtra = pMemory ? ::new (pMemory) ExtraPrimitive(apPrimitive) : nullptr;
    AddExtra(pExtra);
}

void ExtraDataList::SetPatrolRefData(PatrolRefData* apPatrolRefData) {
    auto* pExtra = static_cast<ExtraPatrolRefData*>(GetExtraData(EXTRA_PATROL_REF_DATA));
    if (pExtra) {
        RemoveExtra(pExtra, true);
    }
    if (apPatrolRefData) {
        void* pMemory = ourMalloc(sizeof(ExtraPatrolRefData));
        pExtra = pMemory ? ::new (pMemory) ExtraPatrolRefData : nullptr;
        pExtra->SetPatrolRefData(apPatrolRefData);
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetMerchantContainer(TESObjectREFR* apRef) {
    if (apRef) {
        auto* pExtra = static_cast<ExtraMerchantContainer*>(GetExtraData(EXTRA_MERCHANTCONTAINER));
        if (pExtra) {
            pExtra->pContainer = apRef;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraMerchantContainer));
            pExtra = pMemory ? ::new (pMemory) ExtraMerchantContainer : nullptr;
            pExtra->pContainer = apRef;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_MERCHANTCONTAINER);
    }
}

void ExtraDataList::SetLevCreaModifier(int32_t aiVal) {
    if (aiVal != LCM_NONE) {
        auto* pExtra = static_cast<ExtraLevCreaModifier*>(GetExtraData(EXTRA_LEVCREA_MOD));
        if (pExtra) {
            pExtra->eModifier = static_cast<LEV_CREA_MODIFIER>(aiVal);
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraLevCreaModifier));
            pExtra = pMemory ? ::new (pMemory) ExtraLevCreaModifier : nullptr;
            pExtra->eModifier = static_cast<LEV_CREA_MODIFIER>(aiVal);
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_LEVCREA_MOD);
    }
}

void ExtraDataList::SetLeveledCreature(TESActorBase* apOriginalBase, TESActorBase* apTemplate) {
    if (apOriginalBase && apTemplate) {
        auto* pExtra = static_cast<ExtraLeveledCreature*>(GetExtraData(EXTRA_LEVELEDCREATURE));
        if (!pExtra) {
            void* pMemory = ourMalloc(sizeof(ExtraLeveledCreature));
            pExtra = pMemory ? ::new (pMemory) ExtraLeveledCreature : nullptr;
            AddExtra(pExtra);
        }
        pExtra->pOriginalBase = apOriginalBase;
        pExtra->pTemplate = apTemplate;
    }
    else {
        RemoveExtra(EXTRA_LEVELEDCREATURE);
    }
}

// Compares the float against the double 0.0; NaN keeps the extra.
void ExtraDataList::SetNorthRotation(float afVal) {
    if (afVal != 0.0) {
        auto* pExtra = static_cast<ExtraNorthRotation*>(GetExtraData(EXTRA_NORTHROTATION));
        if (pExtra) {
            pExtra->fNorthRot = afVal;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraNorthRotation));
            pExtra = pMemory ? ::new (pMemory) ExtraNorthRotation : nullptr;
            pExtra->fNorthRot = afVal;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_NORTHROTATION);
    }
}

void ExtraDataList::SetXTarget(TESObjectREFR* apTarget) {
    if (apTarget) {
        auto* pExtra = static_cast<ExtraXTarget*>(GetExtraData(EXTRA_XTARGET));
        if (pExtra) {
            pExtra->pTarget = apTarget;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraXTarget));
            pExtra = pMemory ? ::new (pMemory) ExtraXTarget : nullptr;
            pExtra->pTarget = apTarget;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_XTARGET);
    }
}

void ExtraDataList::SetEmittanceSource(TESForm* apSource) {
    if (apSource) {
        auto* pExtra = static_cast<ExtraEmittanceSource*>(GetExtraData(EXTRA_EMITTANCE_SOURCE));
        if (pExtra) {
            pExtra->pSource = apSource;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraEmittanceSource));
            pExtra = pMemory ? ::new (pMemory) ExtraEmittanceSource : nullptr;
            pExtra->pSource = apSource;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_EMITTANCE_SOURCE);
    }
}

void ExtraDataList::SetMultiBoundRef(TESObjectREFR* apRef) {
    if (apRef) {
        auto* pExtra = static_cast<ExtraMultiBoundRef*>(GetExtraData(EXTRA_MULTIBOUND_REF));
        if (pExtra) {
            pExtra->pBoundRef = apRef;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraMultiBoundRef));
            pExtra = pMemory ? ::new (pMemory) ExtraMultiBoundRef : nullptr;
            pExtra->pBoundRef = apRef;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_MULTIBOUND_REF);
    }
}

// The replaced marker data is freed with ourFree, as the game does.
void ExtraDataList::SetMultiBoundData(MultiBoundMarkerData* apData) {
    if (apData) {
        auto* pExtra = static_cast<ExtraMultiBoundData*>(GetExtraData(EXTRA_MULTIBOUND_DATA));
        if (pExtra) {
            if (pExtra->pBound) {
                ourFree(pExtra->pBound);
            }
            pExtra->pBound = apData;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraMultiBoundData));
            pExtra = pMemory ? ::new (pMemory) ExtraMultiBoundData : nullptr;
            pExtra->pBound = apData;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_MULTIBOUND_DATA);
    }
}

// Compares the float against the double 0.0; NaN keeps the extra.
void ExtraDataList::SetRadius(float afRadius) {
    if (afRadius == 0.0) {
        RemoveExtra(EXTRA_RADIUS);
        return;
    }
    auto* pExtra = static_cast<ExtraRadius*>(GetExtraData(EXTRA_RADIUS));
    if (pExtra) {
        pExtra->fRadius = afRadius;
    }
    else {
        void* pMemory = ourMalloc(sizeof(ExtraRadius));
        pExtra = pMemory ? ::new (pMemory) ExtraRadius(afRadius) : nullptr;
        AddExtra(pExtra);
    }
}

// Compares the float against the double 0.0; NaN keeps the extra.
void ExtraDataList::SetRadiation(float afRadiation) {
    if (afRadiation == 0.0) {
        RemoveExtra(EXTRA_RADIATION);
        return;
    }
    auto* pExtra = static_cast<ExtraRadiation*>(GetExtraData(EXTRA_RADIATION));
    if (pExtra) {
        pExtra->fRadiation = afRadiation;
    }
    else {
        void* pMemory = ourMalloc(sizeof(ExtraRadiation));
        pExtra = pMemory ? ::new (pMemory) ExtraRadiation(afRadiation) : nullptr;
        AddExtra(pExtra);
    }
}

void ExtraDataList::AddFollower(Actor* apActor) {
    if (apActor == pPlayer) {
        return;
    }
    auto* pExtra = static_cast<ExtraFollower*>(GetExtraData(EXTRA_FOLLOWER));
    if (!pExtra) {
        void* pMemory = ourMalloc(sizeof(ExtraFollower));
        pExtra = pMemory ? ::new (pMemory) ExtraFollower : nullptr;
        AddExtra(pExtra);
    }
    if (!pExtra->pActorlist->IsInList(apActor)) {
        pExtra->pActorlist->AddHead(apActor);
    }
}

void ExtraDataList::SetLastFinishedSequence(const char* apSequenceName) {
    RemoveLastFinishedSequence();
    if (!apSequenceName) {
        return;
    }
    void* pMemory = ourMalloc(sizeof(ExtraLastFinishedSequence));
    ExtraLastFinishedSequence* pExtra = pMemory ? ::new (pMemory) ExtraLastFinishedSequence(apSequenceName) : nullptr;
    AddExtra(pExtra);
}

const char* ExtraDataList::GetLastFinishedSequence() {
    auto* pExtra = static_cast<ExtraLastFinishedSequence*>(GetExtraData(EXTRA_LASTFINISHEDSEQUENCE));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pLastSequenceName;
}

// KEEP: the exe builds ExtraNavMeshPortal's by-value NAVMESH_PORTAL argument in the
// outgoing argument slot with NAVMESH_PORTAL's out-of-line copy constructor (0x69A690, which copies 6 of its
// 8 bytes); a by-value argument through xfnv::original cannot reproduce that call.
// The exe's cleanup-only C++ EH frame is left out.
void ExtraDataList::SetNavMeshPortal(ExtraNavMeshPortal* apPortal) {
    NAVMESH_PORTAL v2;
    BSExtraData* apExtra = GetExtraData(90u);
    if (!apExtra) {
        ExtraNavMeshPortal* v3;
        void* apMemory = ourMalloc(sizeof(ExtraNavMeshPortal));
        if (apMemory) {
            NAVMESH_PORTAL* v5 = &v2;
            v2.~NAVMESH_PORTAL();
            ::new (&v2) NAVMESH_PORTAL(apPortal->Portal);
            v3 = ::new (apMemory) ExtraNavMeshPortal(v2);
        }
        else {
            v3 = nullptr;
        }
        apExtra = v3;
        this->AddExtra(v3);
    }
}

void ExtraDataList::AddFactionChangesExtra() {
    auto* pExtra = static_cast<ExtraFactionChanges*>(GetExtraData(EXTRA_FACTION_CHANGES));
    if (!pExtra) {
        void* pMemory = ourMalloc(sizeof(ExtraFactionChanges));
        pExtra = pMemory ? ::new (pMemory) ExtraFactionChanges : nullptr;
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetIgnoredBySandbox(bool abVal) {
    auto* pExtra = static_cast<ExtraIgnoredBySandbox*>(GetExtraData(EXTRA_IGNORED_BY_SANDBOX));
    if (!pExtra && abVal) {
        void* pMemory = ourMalloc(sizeof(ExtraIgnoredBySandbox));
        pExtra = pMemory ? ::new (pMemory) ExtraIgnoredBySandbox : nullptr;
        AddExtra(pExtra);
    }
    else if (pExtra && !abVal) {
        RemoveExtra(pExtra, true);
    }
}

void ExtraDataList::SetRadioData(RADIO_DATA* apData) {
    if (apData) {
        auto* pExtra = static_cast<ExtraRadioData*>(GetExtraData(EXTRA_RADIO_DATA));
        if (pExtra) {
            pExtra->Data.Copy(apData);
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraRadioData));
            pExtra = pMemory ? ::new (pMemory) ExtraRadioData : nullptr;
            pExtra->Data.Copy(apData);
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_RADIO_DATA);
    }
}

// The light being replaced is deleted even when it is apLight itself.
ExtraLight* ExtraDataList::SetLightPtr(LightRefData* apLight) {
    auto* pExtra = static_cast<ExtraLight*>(GetExtraData(EXTRA_LIGHT));
    if (pExtra) {
        LightRefData* pOld = pExtra->pLight;
        if (pOld) {
            delete pOld;
        }
        pExtra->pLight = apLight;
        return pExtra;
    }
    void* pMemory = ourMalloc(sizeof(ExtraLight));
    pExtra = pMemory ? ::new (pMemory) ExtraLight(apLight) : nullptr;
    AddExtra(pExtra);
    return pExtra;
}

ExtraTeleport* ExtraDataList::SetTeleportPtr(DoorTeleportData* apTeleport) {
    auto* pExtra = static_cast<ExtraTeleport*>(GetExtraData(EXTRA_TELEPORT));
    if (pExtra) {
        if (!apTeleport) {
            RemoveExtra(pExtra, true);
        }
        else {
            DoorTeleportData* pOld = pExtra->pTeleportData;
            if (pOld) {
                delete pOld;
            }
            pExtra->pTeleportData = apTeleport;
        }
    }
    else if (apTeleport) {
        void* pMemory = ourMalloc(sizeof(ExtraTeleport));
        pExtra = pMemory ? ::new (pMemory) ExtraTeleport(apTeleport) : nullptr;
        AddExtra(pExtra);
    }
    return pExtra;
}

ExtraMapMarker* ExtraDataList::SetMapMarkerPtr(MapMarkerData* apMapData) {
    auto* pExtra = static_cast<ExtraMapMarker*>(GetExtraData(EXTRA_MAPMARKER));
    if (pExtra) {
        if (!apMapData) {
            RemoveExtra(pExtra, true);
            pExtra = nullptr;
        }
        else {
            MapMarkerData* pOld = pExtra->pMapData;
            if (pOld) {
                delete pOld;
            }
            pExtra->pMapData = apMapData;
        }
    }
    else if (apMapData) {
        void* pMemory = ourMalloc(sizeof(ExtraMapMarker));
        pExtra = pMemory ? ::new (pMemory) ExtraMapMarker(apMapData) : nullptr;
        AddExtra(pExtra);
    }
    return pExtra;
}

ExtraAudioMarker* ExtraDataList::SetAudioMarkerPtr(AudioMarker* apAudioData) {
    auto* pExtra = static_cast<ExtraAudioMarker*>(GetExtraData(EXTRA_AUDIOMARKER));
    if (pExtra) {
        if (!apAudioData) {
            RemoveExtra(pExtra, true);
            pExtra = nullptr;
        }
        else {
            AudioMarker* pOld = pExtra->pAudioData;
            if (pOld) {
                delete pOld;
            }
            pExtra->pAudioData = apAudioData;
        }
    }
    else if (apAudioData) {
        void* pMemory = ourMalloc(sizeof(ExtraAudioMarker));
        pExtra = pMemory ? ::new (pMemory) ExtraAudioMarker(apAudioData) : nullptr;
        AddExtra(pExtra);
    }
    return pExtra;
}

void ExtraDataList::SetOwnership(TESForm* apOwner) {
    auto* pExtra = static_cast<ExtraOwnership*>(GetExtraData(EXTRA_OWNERSHIP));
    if (pExtra) {
        if (!apOwner) {
            RemoveExtra(pExtra, true);
        }
        else {
            pExtra->pOwner = apOwner;
        }
    }
    else if (apOwner) {
        void* pMemory = ourMalloc(sizeof(ExtraOwnership));
        pExtra = pMemory ? ::new (pMemory) ExtraOwnership(apOwner) : nullptr;
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetGlobal(TESGlobal* apGlobal) {
    ExtraGlobal* v2;
    void* v4;
    ExtraGlobal* extraData = static_cast<ExtraGlobal*>(BaseExtraList::GetExtraData(EXTRA_GLOBAL));
    if (extraData) {
        if (apGlobal) {
            extraData->pGlobal = apGlobal;
        }
        else {
            BaseExtraList::RemoveExtra(extraData, true);
        }
    }
    else {
        if (apGlobal) {
            v4 = ourMalloc(sizeof(ExtraGlobal));
            if (v4) {
                v2 = ::new (v4) ExtraGlobal(apGlobal);
            }
            else {
                v2 = nullptr;
            }
            BaseExtraList::AddExtra(v2);
        }
    }
}

void ExtraDataList::SetRank(int32_t aiRank) {
    ExtraRank* pRank = static_cast<ExtraRank*>(GetExtraData(35));
    if (pRank) {
        if (aiRank == -1) {
            RemoveExtra(pRank, true);
        }
        else {
            pRank->iRank = aiRank;
        }
    }
    else {
        if (aiRank != -1) {
            ExtraRank* pRank_2;
            ExtraRank* apMemory = static_cast<ExtraRank*>(ourMalloc(sizeof(ExtraRank)));
            if (apMemory) {
                pRank_2 = ::new (apMemory) ExtraRank(aiRank);
            }
            else {
                pRank_2 = nullptr;
            }
            AddExtra(pRank_2);
        }
    }
}

void ExtraDataList::SetCell3D(NiNode* apObjectRoot) {
    ExtraCell3D* pExtra = static_cast<ExtraCell3D*>(GetExtraData(2));
    if (apObjectRoot) {
        goto LABEL_6;
    }
    if ((pExtra) && (pExtra->spCellNode == nullptr)) {
        RemoveExtra(pExtra, true);
        pExtra = nullptr;
    }

    if (pExtra) {
    LABEL_6:;
        if (!pExtra) {
            ExtraCell3D* pExtra_2;
            ExtraCell3D* apMemory = static_cast<ExtraCell3D*>(ourMalloc(sizeof(ExtraCell3D)));
            if (apMemory) {
                pExtra_2 = ::new (apMemory) ExtraCell3D;
            }
            else {
                pExtra_2 = nullptr;
            }
            pExtra = pExtra_2;
            AddExtra(pExtra_2);
        }
        pExtra->spCellNode = apObjectRoot;
    }
}

void ExtraDataList::SetCanopyShadowMask(int32_t aeState, NiRenderedTexture* apTex, _D3DLOCKED_RECT*& apOutRect) {
    ExtraCellCanopyShadowMask* pExtra = static_cast<ExtraCellCanopyShadowMask*>(GetExtraData(10));
    if (aeState) {
        if (pExtra) {
            pExtra->eState = static_cast<ExtraCellCanopyShadowMask::CELL_STATE>(aeState);
            pExtra->spMask = apTex;
        }
        else {
            ExtraCellCanopyShadowMask* pExtra_2;
            ExtraCellCanopyShadowMask* apMemory =
                static_cast<ExtraCellCanopyShadowMask*>(ourMalloc(sizeof(ExtraCellCanopyShadowMask)));
            if (apMemory) {
                pExtra_2 = ::new (apMemory)
                    ExtraCellCanopyShadowMask(static_cast<ExtraCellCanopyShadowMask::CELL_STATE>(aeState), apTex);
            }
            else {
                pExtra_2 = nullptr;
            }
            pExtra = pExtra_2;
            AddExtra(pExtra_2);
        }
        apOutRect = &pExtra->Rect;
    }
    else {
        if (pExtra) {
            RemoveExtra(pExtra, true);
        }
    }
}

void ExtraDataList::SetTrespassPackageExtra(TrespassPackage* apPack) {
    ExtraTresPassPackage* pExtra = static_cast<ExtraTresPassPackage*>(GetExtraData(26));
    if (pExtra) {
        if (pExtra->pPack) {
            delete pExtra->pPack;
        }
        pExtra->pPack = apPack;
    }
    else {
        ExtraTresPassPackage* pExtra_2;
        ExtraTresPassPackage* apMemory = static_cast<ExtraTresPassPackage*>(ourMalloc(sizeof(ExtraTresPassPackage)));
        if (apMemory) {
            pExtra_2 = ::new (apMemory) ExtraTresPassPackage(apPack);
        }
        else {
            pExtra_2 = nullptr;
        }
        AddExtra(pExtra_2);
    }
}

void ExtraDataList::AddRoomLinkedRoomRef(TESObjectREFR* apRef) {
    if ((!GetRoomLinkedRefData()) && (apRef)) {
        RoomRefData* v2;
        RoomRefData* apMemory = static_cast<RoomRefData*>(ourMalloc(sizeof(RoomRefData)));
        if (apMemory) {
            v2 = ::new (apMemory) RoomRefData;
        }
        else {
            v2 = nullptr;
        }
        SetRoomLinkedRefData(v2);
    }

    RoomRefData* RoomLinkedRefData = GetRoomLinkedRefData();
    if (!RoomLinkedRefData->kRoomList.IsInList(apRef)) {
        RoomLinkedRefData->kRoomList.AddHead(apRef);
    }
}

void ExtraDataList::SetCollisionData(CollisionData* apCollisionData) {
    BSExtraData* ExtraData = GetExtraData(114u);
    if (ExtraData) {
        RemoveExtra(ExtraData, true);
    }
    if (apCollisionData) {
        ExtraCollisionData* v2;
        ExtraCollisionData* apMemory = static_cast<ExtraCollisionData*>(ourMalloc(sizeof(ExtraCollisionData)));
        if (apMemory) {
            v2 = ::new (apMemory) ExtraCollisionData;
        }
        else {
            v2 = nullptr;
        }
        v2->pCollisionData = apCollisionData;
        AddExtra(v2);
    }
}

void ExtraDataList::SetExtraSayTopicInfoOnceADayExtra(SayOnceTopicInfos* apInfo) {
    ExtraSayTopicInfoOnceADay* v2;
    void* apMemory;
    ExtraSayTopicInfoOnceADay* extraData;
    if (apInfo) {
        extraData =
            static_cast<ExtraSayTopicInfoOnceADay*>(BaseExtraList::GetExtraData(EXTRA_SAY_ONCE_A_DAY_TOPIC_INFO));
        if (extraData) {
            extraData->pListofSaidOnceTopicInfos->AddHead(apInfo);
        }
        else {
            apMemory = ourMalloc(sizeof(ExtraSayTopicInfoOnceADay));
            if (apMemory) {
                v2 = ::new (apMemory) ExtraSayTopicInfoOnceADay(apInfo);
            }
            else {
                v2 = nullptr;
            }
            BaseExtraList::AddExtra(v2);
        }
    }
    else {
        BaseExtraList::RemoveExtra();
    }
}

ExtraAudioBuoyMarker* ExtraDataList::SetAudioBuoyMarkerPtr(AudioBuoyMarkerData* apAudioBuoyData) {
    ExtraAudioBuoyMarker* pExtra = static_cast<ExtraAudioBuoyMarker*>(GetExtraData(145));
    if (pExtra) {
        if (apAudioBuoyData) {
            AudioBuoyMarkerData* apThis = pExtra->pAudioBuoyData;
            if (apThis) {
                AudioBuoyMarkerData* pAudioBuoyData = pExtra->pAudioBuoyData;
                delete apThis;
            }
            pExtra->pAudioBuoyData = apAudioBuoyData;
        }
        else {
            RemoveExtra(pExtra, true);
            return nullptr;
        }
    }
    else {
        if (apAudioBuoyData) {
            ExtraAudioBuoyMarker* pExtra_2;
            ExtraAudioBuoyMarker* apMemory =
                static_cast<ExtraAudioBuoyMarker*>(ourMalloc(sizeof(ExtraAudioBuoyMarker)));
            if (apMemory) {
                pExtra_2 = ::new (apMemory) ExtraAudioBuoyMarker(apAudioBuoyData);
            }
            else {
                pExtra_2 = nullptr;
            }
            pExtra = pExtra_2;
            AddExtra(pExtra_2);
        }
    }
    return pExtra;
}

void ExtraDataList::SetHealth(float afHealth) {
    auto* pExtra = static_cast<ExtraHealth*>(GetExtraData(EXTRA_HEALTH));
    if (pExtra) {
        pExtra->fHealth = afHealth;
    }
    else {
        void* pMemory = ourMalloc(sizeof(ExtraHealth));
        pExtra = pMemory ? ::new (pMemory) ExtraHealth(afHealth) : nullptr;
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetUses(uint8_t acUses) {
    auto* pExtra = static_cast<ExtraUses*>(GetExtraData(EXTRA_USES));
    if (pExtra) {
        pExtra->cUses = acUses;
        return;
    }
    void* pMemory = ourMalloc(sizeof(ExtraUses));
    pExtra = pMemory ? ::new (pMemory) ExtraUses(acUses) : nullptr;
    AddExtra(pExtra);
}

void ExtraDataList::SetTimeLeft(float afTimeLeft) {
    auto* pExtra = static_cast<ExtraTimeLeft*>(GetExtraData(EXTRA_TIMELEFT));
    if (pExtra) {
        pExtra->fTime = afTimeLeft;
        return;
    }
    void* pMemory = ourMalloc(sizeof(ExtraTimeLeft));
    pExtra = pMemory ? ::new (pMemory) ExtraTimeLeft(afTimeLeft) : nullptr;
    AddExtra(pExtra);
}

void ExtraDataList::SetCharge(float afCharge) {
    auto* pExtra = static_cast<ExtraCharge*>(GetExtraData(EXTRA_CHARGE));
    if (pExtra) {
        pExtra->fCharge = afCharge;
        return;
    }
    void* pMemory = ourMalloc(sizeof(ExtraCharge));
    pExtra = pMemory ? ::new (pMemory) ExtraCharge(afCharge) : nullptr;
    AddExtra(pExtra);
}

void ExtraDataList::SetSeedIndex(uint8_t aiSeed) {
    auto* pExtra = static_cast<ExtraSeed*>(GetExtraData(EXTRA_SEED));
    if (aiSeed == 255) {
        if (pExtra) {
            RemoveExtra(pExtra, true);
        }
    }
    else if (pExtra) {
        pExtra->iSeed = aiSeed;
    }
    else {
        void* pMemory = ourMalloc(sizeof(ExtraSeed));
        pExtra = pMemory ? ::new (pMemory) ExtraSeed(aiSeed) : nullptr;
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetObjectHealth(float afHealth) {
    auto* pExtra = static_cast<ExtraObjectHealth*>(GetExtraData(EXTRA_OBJECT_HEALTH));
    if (pExtra) {
        pExtra->fHealth = afHealth;
        return;
    }
    void* pMemory = ourMalloc(sizeof(ExtraObjectHealth));
    pExtra = pMemory ? ::new (pMemory) ExtraObjectHealth(afHealth) : nullptr;
    AddExtra(pExtra);
}

void ExtraDataList::SetClimate(TESClimate* apClimate) {
    auto* pExtra = static_cast<ExtraCellClimate*>(GetExtraData(EXTRA_CLIMATE));
    if (apClimate) {
        if (pExtra) {
            pExtra->pClimate = apClimate;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraCellClimate));
            pExtra = pMemory ? ::new (pMemory) ExtraCellClimate(apClimate) : nullptr;
            AddExtra(pExtra);
        }
    }
    else if (pExtra) {
        RemoveExtra(pExtra, true);
    }
}

void ExtraDataList::SetNoRumors(bool abVal) {
    auto* pExtra = static_cast<ExtraHasNoRumors*>(GetExtraData(EXTRA_NO_RUMORS));
    if (pExtra) {
        pExtra->bNoRumors = abVal;
        return;
    }
    void* pMemory = ourMalloc(sizeof(ExtraHasNoRumors));
    pExtra = pMemory ? ::new (pMemory) ExtraHasNoRumors(abVal) : nullptr;
    AddExtra(pExtra);
}

// Stores the time through the constructor's result even when the allocation failed, as the game does.
void ExtraDataList::SetDetachTime(uint32_t aiTime) {
    if (aiTime) {
        auto* pExtra = static_cast<ExtraDetachTime*>(GetExtraData(EXTRA_CELLDETACHTIME));
        if (pExtra) {
            pExtra->iTime = aiTime;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraDetachTime));
            pExtra = pMemory ? ::new (pMemory) ExtraDetachTime : nullptr;
            pExtra->iTime = aiTime;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_CELLDETACHTIME);
    }
}

// Updates an existing extra and still adds a new one, as the game does.
void ExtraDataList::SetOriginalReferenceExtra(TESObjectREFR* apRef) {
    auto* pExtra = static_cast<ExtraOriginalReference*>(GetExtraData(EXTRA_ORIGINAL_REFERENCE));
    if (pExtra) {
        pExtra->pReference = apRef;
    }
    void* pMemory = ourMalloc(sizeof(ExtraOriginalReference));
    pExtra = pMemory ? ::new (pMemory) ExtraOriginalReference(apRef) : nullptr;
    AddExtra(pExtra);
}

void ExtraDataList::SetInfoGeneralTopic(MenuTopic* apItem) {
    auto* pExtra = static_cast<ExtraInfoGeneralTopic*>(GetExtraData(EXTRA_INFO_GENERAL_TOPIC));
    if (pExtra) {
        pExtra->pInfoGen = apItem;
        return;
    }
    void* pMemory = ourMalloc(sizeof(ExtraInfoGeneralTopic));
    pExtra = pMemory ? ::new (pMemory) ExtraInfoGeneralTopic(apItem) : nullptr;
    AddExtra(pExtra);
}

void ExtraDataList::SetModelSwap(TESModel* apModelSwap, TESForm* apModelSwapForm) {
    auto* pExtra = static_cast<ExtraModelSwap*>(GetExtraData(EXTRA_MODEL_SWAP));
    if (pExtra) {
        pExtra->pModelSwap = apModelSwap;
        pExtra->pModelSwapForm = apModelSwapForm;
    }
    else {
        void* pMemory = ourMalloc(sizeof(ExtraModelSwap));
        pExtra = pMemory ? ::new (pMemory) ExtraModelSwap(apModelSwap, apModelSwapForm) : nullptr;
        AddExtra(pExtra);
    }
}

// Compares against the double 1.0; NaN keeps or adds the extra.
void ExtraDataList::SetScale(float afScale) {
    auto* pExtra = static_cast<ExtraScale*>(GetExtraData(EXTRA_SCALE));
    if (pExtra) {
        if (afScale == 1.0) {
            RemoveExtra(pExtra, true);
        }
        else {
            pExtra->fScale = afScale;
        }
    }
    else if (afScale != 1.0) {
        void* pMemory = ourMalloc(sizeof(ExtraScale));
        pExtra = pMemory ? ::new (pMemory) ExtraScale(afScale) : nullptr;
        AddExtra(pExtra);
    }
}

// Stores the power through the constructor's result even when the allocation failed, as the game does.
void ExtraDataList::SetRefractionPropertyExtra(bool abflag, float afRefractionPower) {
    auto* pExtra = static_cast<ExtraRefractionProperty*>(GetExtraData(EXTRA_REFRACTION_PROPERTY));
    if (abflag) {
        if (!pExtra) {
            void* pMemory = ourMalloc(sizeof(ExtraRefractionProperty));
            pExtra = pMemory ? ::new (pMemory) ExtraRefractionProperty(afRefractionPower) : nullptr;
            AddExtra(pExtra);
        }
        pExtra->fRefractionPower = afRefractionPower;
    }
    else if (pExtra) {
        RemoveExtra(pExtra, true);
    }
}

// An existing extra, or a zero slot without one, gets SetWeaponModActive; it is called on the null extra too,
// deliberately, as the game does.
void ExtraDataList::SetWeaponModSlot(uint8_t slot) {
    auto* pExtra = static_cast<ExtraWeaponModFlags*>(GetExtraData(EXTRA_WEAPON_MOD_SLOTS));
    if (pExtra || !slot) {
        pExtra->SetWeaponModActive(slot);
    }
    else {
        void* pMemory = ourMalloc(sizeof(ExtraWeaponModFlags));
        pExtra = pMemory ? ::new (pMemory) ExtraWeaponModFlags(slot) : nullptr;
        pExtra->SetWeaponModActive(slot);
        AddExtra(pExtra);
    }
}

// Sets the flags through the constructor's result even when the allocation failed, as the game does.
void ExtraDataList::SetWeaponModFlags(uint8_t flags) {
    auto* pExtra = static_cast<ExtraWeaponModFlags*>(GetExtraData(EXTRA_WEAPON_MOD_SLOTS));
    if (pExtra) {
        pExtra->SetWeaponModFlags(flags);
    }
    else {
        void* pMemory = ourMalloc(sizeof(ExtraWeaponModFlags));
        pExtra = pMemory ? ::new (pMemory) ExtraWeaponModFlags : nullptr;
        pExtra->SetWeaponModFlags(flags);
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetHeadTrackTargetExtra(TESObjectREFR* apRef) {
    auto* pExtra = static_cast<ExtraHeadingTarget*>(GetExtraData(EXTRA_HEAD_TRACK_TARGET));
    if (apRef || !pExtra) {
        if (pExtra) {
            pExtra->pHeadingTarget = apRef;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraHeadingTarget));
            pExtra = pMemory ? ::new (pMemory) ExtraHeadingTarget(apRef) : nullptr;
            AddExtra(pExtra);
        }
        if (apRef) {
            apRef->SetTargeted(true);
        }
    }
    else {
        RemoveExtra(pExtra, true);
    }
}

// Bit 0 of the activate flags marks activation by the parent only.
void ExtraDataList::SetActivateParentOnly(bool abVal) {
    auto* pExtra = static_cast<ExtraActivateRef*>(GetExtraData(EXTRA_ACTIVATE_REF));
    if (!pExtra && abVal) {
        void* pMemory = ourMalloc(sizeof(ExtraActivateRef));
        pExtra = pMemory ? ::new (pMemory) ExtraActivateRef : nullptr;
        AddExtra(pExtra);
    }
    if (pExtra) {
        if (abVal) {
            pExtra->cActivateFlags |= 1;
        }
        else {
            pExtra->cActivateFlags &= ~1;
        }
    }
}

void ExtraDataList::SetIsModding(bool bModding) {
    auto* pExtra = static_cast<ExtraModdingItem*>(GetExtraData(EXTRA_WEAPON_IS_MODDING));
    if (pExtra) {
        pExtra->SetItemModding(bModding);
    }
    else {
        void* pMemory = ourMalloc(sizeof(ExtraModdingItem));
        pExtra = pMemory ? ::new (pMemory) ExtraModdingItem(bModding) : nullptr;
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetAmmoExtra(TESAmmo* apAmmoType, int32_t aiCount) {
    if (apAmmoType && aiCount) {
        auto* pExtra = static_cast<ExtraAmmo*>(GetExtraData(EXTRA_AMMO));
        if (pExtra) {
            pExtra->pAmmo = apAmmoType;
            pExtra->iCount = aiCount;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraAmmo));
            pExtra = pMemory ? ::new (pMemory) ExtraAmmo(apAmmoType, aiCount) : nullptr;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_AMMO);
    }
}

AudioBuoyMarkerData* ExtraDataList::GetAudioBuoyMarkerData() {
    auto* pExtra = static_cast<ExtraAudioBuoyMarker*>(GetExtraData(EXTRA_AUDIOBUOYMARKER));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pAudioBuoyData;
}

float ExtraDataList::GetCharge() {
    auto* pExtra = static_cast<ExtraCharge*>(GetExtraData(EXTRA_CHARGE));
    if (!pExtra) {
        return -1.0f;
    }
    return pExtra->fCharge;
}

float ExtraDataList::GetScale() {
    auto* pExtra = static_cast<ExtraScale*>(GetExtraData(EXTRA_SCALE));
    if (!pExtra) {
        return 1.0f;
    }
    return pExtra->fScale;
}

float ExtraDataList::GetRadiation() {
    auto* pExtra = static_cast<ExtraRadiation*>(GetExtraData(EXTRA_RADIATION));
    if (!pExtra) {
        return 0.0f;
    }
    return pExtra->fRadiation;
}

TESObjectREFR* ExtraDataList::GetHeadTrackTargetExtra() {
    auto* pExtra = static_cast<ExtraHeadingTarget*>(GetExtraData(EXTRA_HEAD_TRACK_TARGET));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pHeadingTarget;
}

bool ExtraDataList::GetPackageExtraActivate() {
    auto* pExtra = static_cast<ExtraPackage*>(GetExtraData(EXTRA_PACKAGE));
    if (!pExtra) {
        return false;
    }
    return pExtra->bActivated;
}

bool ExtraDataList::GetPackageExtraDoneOnce() {
    auto* pExtra = static_cast<ExtraPackage*>(GetExtraData(EXTRA_PACKAGE));
    if (!pExtra) {
        return false;
    }
    return pExtra->bDoneOnce;
}

TrespassPackage* ExtraDataList::GetTrespassPackage() {
    auto* pExtra = static_cast<ExtraTresPassPackage*>(GetExtraData(EXTRA_TRESPASS_PACKAGE));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pPack;
}

TESObjectREFR* ExtraDataList::GetOpenCloseActivateRef() {
    auto* pExtra = static_cast<ExtraOpenCloseActivateRef*>(GetExtraData(EXTRA_OPENCLOSEACTIVATE_REF));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pActivateRef;
}

void ExtraDataList::SetActivateChildrenTimer(float afTimer) {
    auto* pExtra = static_cast<ExtraActivateRefChildren*>(GetExtraData(EXTRA_ACTIVATE_REF_CHILDREN));
    if (pExtra) {
        pExtra->fActivateChildrenTimer = afTimer;
    }
}

BSSimpleList<REF_REFLECTED_DATA*>* ExtraDataList::GetReflectedRefs() {
    auto* pExtra = static_cast<ExtraReflectedRefs*>(GetExtraData(EXTRA_REFLECTED_REFS));
    if (!pExtra) {
        return nullptr;
    }
    return &pExtra->RefList;
}

BSSimpleList<TESObjectREFR*>* ExtraDataList::GetWaterLightRefs() {
    auto* pExtra = static_cast<ExtraWaterLightRefs*>(GetExtraData(EXTRA_WATER_LIGHT_REFS));
    if (!pExtra) {
        return nullptr;
    }
    return reinterpret_cast<BSSimpleList<TESObjectREFR*>*>(&pExtra->RefList);
}

char* ExtraDataList::GetSavedAnimation() {
    auto* pExtra = static_cast<ExtraSavedAnimation*>(GetExtraData(EXTRA_SAVED_ANIMATION));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pAnimationBuffer;
}

// Bit 0 of the activate flags marks activation by the parent only.
bool ExtraDataList::IsActivateParentOnly() {
    auto* pExtra = static_cast<ExtraActivateRef*>(GetExtraData(EXTRA_ACTIVATE_REF));
    return pExtra && (pExtra->cActivateFlags & 1) != 0;
}

void ExtraDataList::SetRoomIsMaster(bool abValue) {
    auto* pExtra = static_cast<ExtraRoomRefData*>(GetExtraData(EXTRA_ROOM_REF_DATA));
    if (pExtra) {
        pExtra->pData->ucRoomCount = static_cast<uint8_t>(abValue) != 0;
    }
}

BSSimpleList<TESObjectREFR*>* ExtraDataList::GetRoomLinkedRoomRefList() {
    RoomRefData* pData = GetRoomLinkedRefData();
    if (!pData) {
        return nullptr;
    }
    return &pData->kRoomList;
}

void ExtraDataList::RemoveRoomLinkedPortalRef(TESObjectREFR* apRef) {
    RoomRefData* pData = GetRoomLinkedRefData();
    if (pData) {
        pData->kPortalList.Remove(apRef);
    }
}

TESObjectREFR* ExtraDataList::GetOriginalReference() {
    auto* pExtra = static_cast<ExtraOriginalReference*>(GetExtraData(EXTRA_ORIGINAL_REFERENCE));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pReference;
}

uint8_t ExtraDataList::GetSeedIndex() {
    auto* pExtra = static_cast<ExtraSeed*>(GetExtraData(EXTRA_SEED));
    if (!pExtra) {
        return 255;
    }
    return pExtra->iSeed;
}

uint8_t ExtraDataList::GetTerminalState() {
    auto* pExtra = static_cast<ExtraTerminalState*>(GetExtraData(EXTRA_TERMINALSTATE));
    if (!pExtra) {
        return 0;
    }
    return pExtra->cState;
}

void ExtraDataList::SetCreatureAwakeSound(const BSSoundHandle& afSound) {
    auto* pExtra = static_cast<ExtraCreatureAwakeSound*>(GetExtraData(EXTRA_CREATURE_AWAKE_SOUND));
    if (pExtra) {
        if (afSound == BSSoundHandle()) {
            RemoveExtra(pExtra, true);
        }
        else {
            pExtra->pHandle = afSound;
        }
    }
    else if (afSound != BSSoundHandle()) {
        void* pMemory = ourMalloc(sizeof(ExtraCreatureAwakeSound));
        if (pMemory) {
            pExtra = ::new (pMemory) ExtraCreatureAwakeSound(afSound);
        }
        else {
            pExtra = nullptr;
        }
        AddExtra(pExtra);
    }
}

// As SetCreatureAwakeSound, for the sound extra.
void ExtraDataList::SetSound(const BSSoundHandle& afSound) {
    auto* pExtra = static_cast<ExtraSound*>(GetExtraData(EXTRA_SOUND));
    if (pExtra) {
        if (afSound == BSSoundHandle()) {
            RemoveExtra(pExtra, true);
        }
        else {
            pExtra->pHandle = afSound;
        }
    }
    else if (afSound != BSSoundHandle()) {
        void* pMemory = ourMalloc(sizeof(ExtraSound));
        if (pMemory) {
            pExtra = ::new (pMemory) ExtraSound(afSound);
        }
        else {
            pExtra = nullptr;
        }
        AddExtra(pExtra);
    }
}

// An existing extra keeps its rotation.
void ExtraDataList::SetPackageStartLocation(
    TESWorldSpace* apWorld,
    TESObjectCELL* apIntCell,
    const NiPoint3& aPt,
    float afzrotation
) {
    auto* pExtra = static_cast<ExtraPackageStartLocation*>(GetExtraData(EXTRA_PACKAGESTARTLOC));
    if (!pExtra) {
        void* pMemory = ourMalloc(sizeof(ExtraPackageStartLocation));
        pExtra = pMemory ? ::new (pMemory) ExtraPackageStartLocation(apWorld, apIntCell, aPt, afzrotation) : nullptr;
        AddExtra(pExtra);
    }
    else {
        if (apWorld) {
            pExtra->worldLoc.pLocationForm = apWorld;
        }
        else {
            pExtra->worldLoc.pLocationForm = apIntCell;
        }
        pExtra->worldLoc.locPt = aPt;
    }
}

// The float is compared widened to a double; 1.0 removes the extra, even a null one.
void ExtraDataList::SetHealthPerc(float afHealthPerc) {
    auto* pExtra = static_cast<ExtraHealthPerc*>(GetExtraData(EXTRA_HEALTH_PERC));
    if (afHealthPerc != 1.0) {
        if (!pExtra) {
            void* pMemory = ourMalloc(sizeof(ExtraHealthPerc));
            pExtra = pMemory ? ::new (pMemory) ExtraHealthPerc : nullptr;
            AddExtra(pExtra);
        }
        pExtra->fHealthPerc = afHealthPerc;
    }
    else {
        RemoveExtra(pExtra, true);
    }
}

void ExtraDataList::SetbhkWorld(bhkWorld* apWorld) {
    auto* pExtra = static_cast<ExtraHavok*>(GetExtraData(EXTRA_HAVOK));
    if (apWorld) {
        if (!pExtra) {
            void* pMemory = ourMalloc(sizeof(ExtraHavok));
            pExtra = pMemory ? ::new (pMemory) ExtraHavok(apWorld) : nullptr;
            AddExtra(pExtra);
        }
        else {
            pExtra->spWorld = apWorld;
        }
    }
    else if (pExtra) {
        RemoveExtra(pExtra, true);
    }
}

bhkWorld* ExtraDataList::GetbhkWorld() {
    auto* pExtra = static_cast<ExtraHavok*>(GetExtraData(EXTRA_HAVOK));
    return pExtra ? pExtra->spWorld.m_pObject : nullptr;
}

// A null apMopp with no havok extra is stored through a null extra, deliberately, as the game does.
void ExtraDataList::SetCellMopp(CellMopp* apMopp) {
    auto* pExtra = static_cast<ExtraHavok*>(GetExtraData(EXTRA_HAVOK));
    if (apMopp && !pExtra) {
        void* pMemory = ourMalloc(sizeof(ExtraHavok));
        pExtra = pMemory ? ::new (pMemory) ExtraHavok(nullptr) : nullptr;
        AddExtra(pExtra);
    }
    pExtra->spCellMopp = apMopp;
}

// As GetbhkWorld, for the cell MOPP.
CellMopp* ExtraDataList::GetCellMopp() {
    auto* pExtra = static_cast<ExtraHavok*>(GetExtraData(EXTRA_HAVOK));
    return pExtra ? pExtra->spCellMopp.m_pObject : nullptr;
}

void ExtraDataList::SetMusicType(BGSMusicType* apType) {
    auto* pExtra = static_cast<ExtraCellMusicType*>(GetExtraData(EXTRA_CELLMUSICTYPE));
    if (!apType) {
        if (pExtra) {
            RemoveExtra(pExtra, true);
        }
    }
    else if (!pExtra) {
        void* pMemory = ourMalloc(sizeof(ExtraCellMusicType));
        pExtra = pMemory ? ::new (pMemory) ExtraCellMusicType(apType) : nullptr;
        AddExtra(pExtra);
    }
    else {
        pExtra->pType = apType;
    }
}

void ExtraDataList::SetAcousticSpace(BGSAcousticSpace* apSpace) {
    auto* pExtra = static_cast<ExtraCellAcousticSpace*>(GetExtraData(EXTRA_CELL_ACOUSTIC_SPACE));
    if (!apSpace) {
        if (pExtra) {
            RemoveExtra(pExtra, true);
        }
    }
    else if (!pExtra) {
        void* pMemory = ourMalloc(sizeof(ExtraCellAcousticSpace));
        pExtra = pMemory ? ::new (pMemory) ExtraCellAcousticSpace(apSpace) : nullptr;
        AddExtra(pExtra);
    }
    else {
        pExtra->pSpace = apSpace;
    }
}

void ExtraDataList::SetImageSpace(TESImageSpace* apImageSpace) {
    auto* pExtra = static_cast<ExtraCellImageSpace*>(GetExtraData(EXTRA_IMAGESPACE));
    if (!apImageSpace) {
        if (pExtra) {
            RemoveExtra(pExtra, true);
        }
    }
    else if (!pExtra) {
        void* pMemory = ourMalloc(sizeof(ExtraCellImageSpace));
        pExtra = pMemory ? ::new (pMemory) ExtraCellImageSpace(apImageSpace) : nullptr;
        AddExtra(pExtra);
    }
    else {
        pExtra->pImageSpace = apImageSpace;
    }
}

void ExtraDataList::SetEditorID(const char* apEditorID) {
    auto* pExtra = static_cast<ExtraEditorID*>(GetExtraData(EXTRA_EDITOR_ID));
    if (!apEditorID) {
        if (pExtra) {
            RemoveExtra(pExtra, true);
        }
    }
    else if (!pExtra) {
        void* pMemory = ourMalloc(sizeof(ExtraEditorID));
        pExtra = pMemory ? ::new (pMemory) ExtraEditorID(apEditorID) : nullptr;
        AddExtra(pExtra);
    }
    else {
        pExtra->cEditorID.Set(apEditorID, 0);
    }
}

void ExtraDataList::SetLevelItemExtra(int32_t aiIndex) {
    auto* pExtra = static_cast<ExtraLeveledItem*>(GetExtraData(EXTRA_LEVELITEM));
    if (!pExtra) {
        void* pMemory = ourMalloc(sizeof(ExtraLeveledItem));
        pExtra = pMemory ? ::new (pMemory) ExtraLeveledItem(aiIndex) : nullptr;
        AddExtra(pExtra);
    }
    else {
        pExtra->iIndex = aiIndex;
    }
    pExtra->bdefault = 0;
}

// The new rag doll data is stored before it is filled in, and both are read back through the extra.
void ExtraDataList::SetRagDollData(TESObjectREFR* apRef) {
    auto* pExtra = static_cast<ExtraRagDollData*>(GetExtraData(EXTRA_RAGDOLLDATA));
    if (pExtra) {
        if (!apRef) {
            RemoveExtra(pExtra, true);
        }
        else {
            pExtra->pRagDollData->UpdateDataFromReference(apRef);
        }
    }
    else if (apRef) {
        void* pMemory = ourMalloc(sizeof(ExtraRagDollData));
        pExtra = pMemory ? ::new (pMemory) ExtraRagDollData : nullptr;
        void* pDataMemory = ourMalloc(sizeof(RagDollData));
        RagDollData* pData = pDataMemory ? ::new (pDataMemory) RagDollData : nullptr;
        pExtra->pRagDollData = pData;
        pExtra->pRagDollData->UpdateDataFromReference(apRef);
        AddExtra(pExtra);
    }
}

// Until InitItem runs, the form pointers of a loaded list hold form IDs; each case below turns them into forms. Warning
// is an empty stub. The next extra is read before the case runs, so a removed extra does not end the walk; the portal
// case goes on reading its extra after removing it, as the game does.
void ExtraDataList::InitItem(TESForm* apOwner) {
    ExtraCritSection.Lock(nullptr);
    BSExtraData* pExtra = pHead;
    TESFile* pFile = apOwner->GetFile(-1);
    while (pExtra) {
        BSExtraData* pNext = pExtra->GetNext();
        switch (pExtra->GetExtraType()) {
            case EXTRA_RADIO_DATA: {
                RADIO_DATA* pData = &static_cast<ExtraRadioData*>(pExtra)->Data;
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pData->pPositionRef));
                if (iID) {
                    TESForm::AddCompileIndex(iID, apOwner->GetFile(-1));
                    pData->pPositionRef = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(iID));
                    if (!pData->pPositionRef) {
                        Warning("MASTERFILE: Could not find radio range position ref (%08X).", iID);
                    }
                    else if (pData->pPositionRef->GetInterior()) {
                        Warning("MASTERFILE: Invalid radio range position ref (%08X). Exteriors only.", iID);
                    }
                }
                break;
            }
            case EXTRA_LOCK: {
                LockData* pLock = static_cast<ExtraLock*>(pExtra)->pLock;
                bool bRemove = false;
                if (!bRemove && pLock->pKey) {
                    uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pLock->pKey));
                    TESForm::AddCompileIndex(iID, pFile);
                    pLock->pKey = dynamic_cast<TESKey*>(TESForm::GetFormByNumericID(iID));
                    if (!pLock->pKey) {
                        Warning("MASTERFILE: Unable to find key %08X for lock data. Lock will be removed.", iID);
                        bRemove = true;
                    }
                }
                if (bRemove) {
                    RemoveExtra(pExtra, true);
                }
                break;
            }
            case EXTRA_TELEPORT: {
                bool bRemove = false;
                DoorTeleportData* pData = static_cast<ExtraTeleport*>(pExtra)->pTeleportData;
                if (bRemove || !pData->InitItem(apOwner)) {
                    RemoveExtra(pExtra, true);
                }
                break;
            }
            case EXTRA_MAPMARKER: {
                MapMarkerData* pData = static_cast<ExtraMapMarker*>(pExtra)->pMapData;
                if (pData->GetTravelLoc() && !pData->GetVisible()) {
                    pData->SetTravelLoc(false);
                }
                // the reputation is still a form ID; it is not given the file's compile index
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pData->GetReputation()));
                if (iID) {
                    pData->SetReputation(pDataHandler->GetReputation(iID));
                }
                break;
            }
            case EXTRA_AUDIOMARKER: {
                AudioMarker* pData = static_cast<ExtraAudioMarker*>(pExtra)->pAudioData;
                // The field still holds the controller's form ID; it is not given the file's compile index yet.
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pData->GetLocationControllerID()));
                TESForm::AddCompileIndex(iID, apOwner->GetFile(-1));
                if (dynamic_cast<MediaLocationController*>(TESForm::GetFormByNumericID(iID))) {
                    // The field holds the controller's form ID until the controller is looked up.
                    pData->pMediaLocationController =
                        reinterpret_cast<MediaLocationController*>(static_cast<uintptr_t>(iID));
                }
                else {
                    Warning(
                        "MASTERFILE: Could not find MediaLocationController (%08X) for a AudioMarker in extra data "
                        "list.",
                        iID
                    );
                }
                break;
            }
            case EXTRA_AUDIOBUOYMARKER: break;
            case EXTRA_OWNERSHIP: {
                auto* pX = static_cast<ExtraOwnership*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pOwner));
                TESForm::AddCompileIndex(iID, pFile);
                pX->pOwner = TESForm::GetFormByNumericID(iID);
                if (!pX->pOwner) {
                    Warning("MASTERFILE: Unable to find ownership owner form %08X. Ownership will be removed.", iID);
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_GLOBAL: {
                auto* pX = static_cast<ExtraGlobal*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pGlobal));
                TESForm::AddCompileIndex(iID, pFile);
                pX->pGlobal = dynamic_cast<TESGlobal*>(TESForm::GetFormByNumericID(iID));
                if (!pX->pGlobal) {
                    Warning(
                        "MASTERFILE: Unable to find ownership condition global %08X. Ownership will be removed.", iID
                    );
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_POISON: {
                auto* pX = static_cast<ExtraPoison*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pPoison));
                TESForm::AddCompileIndex(iID, pFile);
                pX->pPoison = dynamic_cast<AlchemyItem*>(TESForm::GetFormByNumericID(iID));
                if (!pX->pPoison) {
                    Warning("MASTERFILE: Unable to find poison %08X. Poison data will be removed.", iID);
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_CLIMATE: {
                auto* pX = static_cast<ExtraCellClimate*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pClimate));
                TESForm::AddCompileIndex(iID, pFile);
                pX->pClimate = dynamic_cast<TESClimate*>(TESForm::GetFormByNumericID(iID));
                if (!pX->pClimate) {
                    Warning("MASTERFILE: Unable to find cell climate %08X. Climate data will be removed.", iID);
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_IMAGESPACE: {
                auto* pX = static_cast<ExtraCellImageSpace*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pImageSpace));
                TESForm::AddCompileIndex(iID, pFile);
                pX->pImageSpace = dynamic_cast<TESImageSpace*>(TESForm::GetFormByNumericID(iID));
                if (!pX->pImageSpace) {
                    Warning("MASTERFILE: Unable to find cell imagespace %08X. ImageSpace data will be removed.", iID);
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_IMPACTSWAP: {
                auto* pX = static_cast<ExtraCellImpactSwap*>(pExtra);
                if (pX->pImpactSwap) {
                    pX->pImpactSwap->InitItem(apOwner);
                }
                break;
            }
            case EXTRA_CELL_ACOUSTIC_SPACE: {
                auto* pX = static_cast<ExtraCellAcousticSpace*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pSpace));
                TESForm::AddCompileIndex(iID, pFile);
                pX->pSpace = dynamic_cast<BGSAcousticSpace*>(TESForm::GetFormByNumericID(iID));
                if (!pX->pSpace) {
                    Warning(
                        "MASTERFILE: Unable to find cell acoustic space %08X. AcousticSpace data will be removed.", iID
                    );
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_WATERTYPE: {
                auto* pX = static_cast<ExtraCellWaterType*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pWater));
                TESForm::AddCompileIndex(iID, pFile);
                pX->pWater = dynamic_cast<TESWaterForm*>(TESForm::GetFormByNumericID(iID));
                if (!pX->pWater) {
                    Warning("MASTERFILE: Unable to find cell water type %08X. Water data will be removed.", iID);
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_CELLMUSICTYPE: {
                auto* pX = static_cast<ExtraCellMusicType*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pType));
                TESForm::AddCompileIndex(iID, pFile);
                pX->pType = dynamic_cast<BGSMusicType*>(TESForm::GetFormByNumericID(iID));
                if (!pX->pType) {
                    Warning("MASTERFILE: Unable to find cell music type %08X. Music data will be removed.", iID);
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_PACKAGESTARTLOC: {
                auto* pX = static_cast<ExtraPackageStartLocation*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->worldLoc.pLocationForm));
                TESForm::AddCompileIndex(iID, pFile);
                pX->worldLoc.pLocationForm = TESForm::GetFormByNumericID(iID);
                if (!pX->worldLoc.pLocationForm) {
                    Warning(
                        "MASTERFILE: Unable to find package start location cell %08X. Package start location extra "
                        "data will be removed.",
                        iID
                    );
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_ENABLESTATEPARENT: {
                auto* pX = static_cast<ExtraEnableStateParent*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pParent));
                TESForm::AddCompileIndex(iID, pFile);
                pX->pParent = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(iID));
                if (!pX->pParent) {
                    Warning(
                        "MASTERFILE: Unable to find enable state parent %08X. Enable state parent data will be "
                        "removed.",
                        iID
                    );
                    RemoveExtra(pX, true);
                }
                else {
                    auto* pOwnerRef = dynamic_cast<TESObjectREFR*>(apOwner);
                    if (pOwnerRef && pOwnerRef->CheckEnableParentLoop()) {
                        pX->pParent->GetExtra()->AddEnableStateChild(pOwnerRef);
                    }
                    else {
                        Warning("MASTERFILE: Enable state parent loop detected. Parent removed.");
                        RemoveExtra(pX, true);
                    }
                }
                break;
            }
            case EXTRA_TELEPORTMARKER: {
                auto* pX = static_cast<ExtraRandomTeleportMarker*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pMarker));
                TESForm::AddCompileIndex(iID, pFile);
                pX->pMarker = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(iID));
                if (!pX->pMarker) {
                    Warning(
                        "MASTERFILE: Unable to find random door teleport marker %08X. Random door teleport marker "
                        "data will be removed.",
                        iID
                    );
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_RAGDOLLDATA:
                // only reported; the rag doll data stays
                if (apOwner->IsActor()) {
                    Actor* pActor = static_cast<Actor*>(apOwner);
                    if (pActor->GetActorBase() && pActor->GetActorBase()->GetHealth() > 0) {
                        Warning(
                            "MASTERFILE: Ragdoll data found on alive actor %08X. Ragdoll data will be removed.",
                            apOwner->iFormID
                        );
                    }
                }
                break;
            case EXTRA_LINKED_REF: {
                auto* pX = static_cast<ExtraLinkedRef*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pLinkedRef));
                TESForm::AddCompileIndex(iID, pFile);
                pX->pLinkedRef = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(iID));
                if (!pX->pLinkedRef) {
                    Warning(
                        "MASTERFILE: Unable to find linked reference %08X. Linked reference data will be removed.", iID
                    );
                    RemoveExtra(pX, true);
                }
                else {
                    pX->pLinkedRef->GetExtra()->AddLinkedRefChild(static_cast<TESObjectREFR*>(apOwner));
                }
                break;
            }
            case EXTRA_OCCLUSION_PLANE_REF_DATA: {
                auto* pX = static_cast<ExtraOcclusionPlaneRefData*>(pExtra);
                for (int32_t i = 0; i < BSOcclusionPlane::BSOP_LINK_COUNT; ++i) {
                    // The stored ID is discarded and 0 is what gets resolved, as the game does.
                    uint32_t iID = 0;
                    TESForm::AddCompileIndex(iID, pFile);
                    pX->pData->pLinkedRefs[i] = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(iID));
                    if (iID && !pX->pData->pLinkedRefs[i]) {
                        Warning(
                            "MASTERFILE: Unable to find occlusion plane reference %08X. Occlusion plane reference "
                            "data will be removed.",
                            iID
                        );
                        RemoveExtra(pX, true);
                        break;
                    }
                    BSOcclusionPlane* pPlane = GetOcclusionPlane();
                    if (pPlane) {
                        if (pX->pData->pLinkedRefs[i]) {
                            pPlane->SetLinkedPlane(
                                static_cast<BSOcclusionPlane::BSOP_LINK>(i),
                                pX->pData->pLinkedRefs[i]->GetExtra()->GetOcclusionPlane()
                            );
                        }
                    }
                    else {
                        Warning("No occlusion plane for occlusion plane ref extra.");
                    }
                }
                break;
            }
            case EXTRA_PORTAL_REF_DATA: {
                auto* pX = static_cast<ExtraPortalRefData*>(pExtra);
                for (int32_t i = 0; i < 2; ++i) {
                    uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pData->pLinkedRefs[i]));
                    TESForm::AddCompileIndex(iID, pFile);
                    pX->pData->pLinkedRefs[i] = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(iID));
                    if (iID && !pX->pData->pLinkedRefs[i]) {
                        Warning(
                            "MASTERFILE: Unable to find portal linked reference %08X. Portal linked reference data "
                            "will be removed.",
                            iID
                        );
                        RemoveExtra(pX, true);
                        break;
                    }
                }
                if (pX->pData->pLinkedRefs[0] == pX->pData->pLinkedRefs[1]) {
                    if (pX->pData->pLinkedRefs[0]) {
                        Warning(
                            "MASTERFILE: Both of portal %08X's linked rooms are the same (%08X). Attempting to fix.",
                            apOwner->iFormID, pX->pData->pLinkedRefs[0]->iFormID
                        );
                        pX->pData->pLinkedRefs[1] = nullptr;
                    }
                    else {
                        Warning("MASTERFILE: Both of portal %08X's linked rooms are NULL.", apOwner->iFormID);
                    }
                }
                if (pX->pData->pLinkedRefs[0]) {
                    pX->pData->pLinkedRefs[0]->GetExtra()->AddRoomLinkedPortalRef(static_cast<TESObjectREFR*>(apOwner));
                }
                if (pX->pData->pLinkedRefs[1]) {
                    pX->pData->pLinkedRefs[1]->GetExtra()->AddRoomLinkedPortalRef(static_cast<TESObjectREFR*>(apOwner));
                }
                break;
            }
            case EXTRA_ROOM_REF_DATA: {
                auto* pX = static_cast<ExtraRoomRefData*>(pExtra);
                for (BSSimpleList<TESObjectREFR*>* pNode = &pX->pData->kRoomList; pNode && pNode->GetItem();
                     pNode = pNode->GetNext()) {
                    uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pNode->GetItem()));
                    TESForm::AddCompileIndex(iID, pFile);
                    void* pRef = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(iID));
                    if (iID && !pRef) {
                        Warning(
                            "MASTERFILE: Unable to find room linked reference %08X. Room linked reference data will "
                            "be removed.",
                            iID
                        );
                        RemoveExtra(pX, true);
                        break;
                    }
                    if (iID) {
                        pNode->SetItem(static_cast<TESObjectREFR*>(pRef));
                    }
                }
                break;
            }
            case EXTRA_ACTIVATE_REF: {
                auto* pX = static_cast<ExtraActivateRef*>(pExtra);
                BSSimpleList<REF_ACTIVATE_DATA*>* pNode = &pX->ParentList;
                BSSimpleList<REF_ACTIVATE_DATA*>* pPrev = nullptr;
                while (pNode && !pNode->IsEmpty()) {
                    REF_ACTIVATE_DATA* pParent = pNode->GetItem();
                    uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pParent->pActivateRef));
                    TESForm::AddCompileIndex(iID, pFile);
                    pParent->pActivateRef = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(iID));
                    if (!pParent->pActivateRef) {
                        Warning("MASTERFILE: Unable to find activate reference %08X.", iID);
                        if (pPrev) {
                            pPrev->Remove(pParent);
                            pNode = pPrev->GetNext();
                        }
                        else {
                            pNode->RemoveHead();
                        }
                        ourFree(pParent);
                    }
                    else {
                        pParent->pActivateRef->GetExtra()->AddActivateRefChild(static_cast<TESObjectREFR*>(apOwner));
                        pPrev = pNode;
                        pNode = pNode->GetNext();
                    }
                }
                if (pX->ParentList.IsEmpty() && !pX->cActivateFlags && !pX->ActivateTextOverride.GetLength()) {
                    Warning("MASTERFILE: Removing empty activate parent extra.");
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_DECAL_REFS: {
                auto* pX = static_cast<ExtraDecalRefs*>(pExtra);
                pX->InitItem(apOwner);
                if (pX->DecalRefList.IsEmpty()) {
                    Warning("MASTERFILE: Removing empty decal extra.");
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_MERCHANTCONTAINER: {
                auto* pX = static_cast<ExtraMerchantContainer*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pContainer));
                TESForm::AddCompileIndex(iID, pFile);
                pX->pContainer = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(iID));
                if (!pX->pContainer) {
                    Warning(
                        "MASTERFILE: Unable to find merchant container %08X. Merchant container data will be "
                        "removed.",
                        iID
                    );
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_XTARGET: {
                auto* pX = static_cast<ExtraXTarget*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pTarget));
                TESForm::AddCompileIndex(iID, pFile);
                pX->pTarget = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(iID));
                if (!pX->pTarget) {
                    Warning(
                        "MASTERFILE: Unable to find XMarker target %08X. XMarker target data will be removed.", iID
                    );
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_ENCOUNTERZONE: {
                auto* pX = static_cast<ExtraEncounterZone*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pZone));
                TESForm::AddCompileIndex(iID, pFile);
                pX->pZone = dynamic_cast<BGSEncounterZone*>(TESForm::GetFormByNumericID(iID));
                if (!pX->pZone) {
                    Warning("Unable to find encounter zone %08X. Encounter zone data will be removed.", iID);
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_EMITTANCE_SOURCE: {
                // a light or a region; the form is not cast, its type is checked
                auto* pX = static_cast<ExtraEmittanceSource*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pSource));
                TESForm::AddCompileIndex(iID, pFile);
                pX->pSource = TESForm::GetFormByNumericID(iID);
                if (pX->pSource) {
                    uint8_t eType = pX->pSource->GetFormType();
                    if (eType != LIGH_ID && eType != REGN_ID) {
                        pX->pSource = nullptr;
                    }
                }
                if (!pX->pSource) {
                    Warning(
                        "MASTERFILE: Unable to find emittance source %08X. Emittance source data will be removed.", iID
                    );
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_MULTIBOUND_REF: {
                auto* pX = static_cast<ExtraMultiBoundRef*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pBoundRef));
                TESForm::AddCompileIndex(iID, pFile);
                pX->pBoundRef = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(iID));
                // a reference cannot be its own multibound
                if (pX->pBoundRef == apOwner) {
                    pX->pBoundRef = nullptr;
                }
                if (!pX->pBoundRef) {
                    Warning(
                        "MASTERFILE: Unable to find multibound ref %08X used by reference %08X. Multibound ref data "
                        "will be removed.",
                        iID, apOwner->iFormID
                    );
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_REFLECTOR_REFS: {
                auto* pX = static_cast<ExtraReflectorRefs*>(pExtra);
                pX->InitItem(apOwner);
                if (pX->RefList.IsEmpty()) {
                    Warning("MASTERFILE: Removing empty reflector water extra.");
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_LIT_WATER_REFS: {
                auto* pX = static_cast<ExtraLitWaterRefs*>(pExtra);
                pX->InitItem(apOwner);
                if (pX->RefList.IsEmpty()) {
                    Warning("MASTERFILE: Removing empty lit water extra.");
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_AMMO: {
                auto* pX = static_cast<ExtraAmmo*>(pExtra);
                uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pX->pAmmo));
                TESForm::AddCompileIndex(iID, pFile);
                pX->pAmmo = dynamic_cast<TESAmmo*>(TESForm::GetFormByNumericID(iID));
                if (!pX->pAmmo) {
                    Warning("Unable to find ammo %08X. Ammo data will be removed.", iID);
                    RemoveExtra(pX, true);
                }
                break;
            }
            case EXTRA_PATROL_REF_DATA: {
                PatrolRefData* pData = GetPatrolRefData();
                if (pData) {
                    pData->InitItem(apOwner);
                }
                break;
            }
            default: break;
        }
        pExtra = pNext;
    }
    ExtraCritSection.Unlock();
}

void ExtraDataList::GetSound(BSSoundHandle& result) {
    auto* pExtra = static_cast<ExtraSound*>(GetExtraData(EXTRA_SOUND));
    if (pExtra) {
        result = pExtra->pHandle;
    }
    else {
        result.Clear();
    }
}

void ExtraDataList::GetCreatureAwakeSound(BSSoundHandle& result) {
    auto* pExtra = static_cast<ExtraCreatureAwakeSound*>(GetExtraData(EXTRA_CREATURE_AWAKE_SOUND));
    if (pExtra) {
        result = pExtra->pHandle;
    }
    else {
        result.Clear();
    }
}

void ExtraDataList::GetWeaponAttackSound(BSSoundHandle& result) {
    ExtraWeaponAttackSound* pSound = static_cast<ExtraWeaponAttackSound*>(GetExtraData(134));
    if (pSound) {
        result = pSound->pHandle;
    }
    else {
        result.Clear();
    }
}

// As SetCreatureAwakeSound, for the weapon attack sound extra.
void ExtraDataList::SetWeaponAttackSound(const BSSoundHandle& afSound) {
    auto* pSound = static_cast<ExtraWeaponAttackSound*>(GetExtraData(EXTRA_WEAPON_ATTACK_SOUND));
    if (pSound) {
        if (afSound == BSSoundHandle()) {
            RemoveExtra(pSound, true);
        }
        else {
            pSound->pHandle = afSound;
        }
    }
    else if (afSound != BSSoundHandle()) {
        void* pMemory = ourMalloc(sizeof(ExtraWeaponAttackSound));
        if (pMemory) {
            pSound = ::new (pMemory) ExtraWeaponAttackSound(afSound);
        }
        else {
            pSound = nullptr;
        }
        AddExtra(pSound);
    }
}

LightRefData* ExtraDataList::GetSpellEffectLight() {
    // the magic light extra is an ExtraLight retyped
    auto* pExtra = static_cast<ExtraLight*>(GetExtraData(EXTRA_MAGIC_LIGHT));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pLight;
}

float ExtraDataList::GetNorthRotation() {
    auto* pExtra = static_cast<ExtraNorthRotation*>(GetExtraData(EXTRA_NORTHROTATION));
    if (!pExtra) {
        return 0.0f;
    }
    return pExtra->fNorthRot;
}

OcclusionPlaneLinkedRefData* ExtraDataList::GetOcclusionPlaneLinkedRefData() {
    ExtraOcclusionPlaneRefData* extraData =
        static_cast<ExtraOcclusionPlaneRefData*>(BaseExtraList::GetExtraData(EXTRA_OCCLUSION_PLANE_REF_DATA));
    if (extraData) {
        return extraData->pData;
    }

    return nullptr;
}

// The sub-buffer is a stack copy of the extra's buffer pointer; the extra forgets the buffer before it is removed.
bool ExtraDataList::RestoreSavedAnimation(TESObjectREFR* apRef) {
    auto* pExtra = static_cast<ExtraSavedAnimation*>(GetExtraData(EXTRA_SAVED_ANIMATION));
    if (!pExtra) {
        return false;
    }
    BGSLoadGameSubBuffer(pExtra->pAnimationBuffer).LoadAnimation(apRef);
    pExtra->pAnimationBuffer = nullptr;
    RemoveExtra(EXTRA_SAVED_ANIMATION);
    return true;
}

// As RestoreSavedAnimation, for the saved Havok state.
bool ExtraDataList::RestoreSavedHavokData(TESObjectREFR* apRef) {
    auto* pExtra = static_cast<ExtraSavedHavokData*>(GetExtraData(EXTRA_SAVED_HAVOK_DATA));
    if (!pExtra) {
        return false;
    }
    BGSLoadGameSubBuffer(pExtra->pHavokBuffer).LoadHavokData(apRef);
    pExtra->pHavokBuffer = nullptr;
    RemoveExtra(EXTRA_SAVED_HAVOK_DATA);
    return true;
}

void ExtraDataList::RemoveSavedHavokData() {
    BaseExtraList::RemoveExtra(EXTRA_SAVED_HAVOK_DATA);
}

TESForm* ExtraDataList::GetStartingWorldOrCell() {
    auto* pExtra = static_cast<ExtraStartingWorldOrCell*>(GetExtraData(EXTRA_STARTINGWORLDORCELL));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pStartingWorldOrCell;
}

void ExtraDataList::SetStartingWorldOrCellForRef(TESObjectREFR* apRef) {
    TESObjectCELL* pcell = apRef->GetSaveParentCell();
    if (pcell) {
        TESWorldSpace* pworld = pcell->GetWorldSpace();
        if (pworld) {
            SetStartingWorldOrCell(pworld);
        }
        else {
            SetStartingWorldOrCell(pcell);
        }
    }
    else {
        int32_t CastingType = apRef->iFormID;
        Warning("FORMS: Reference %s %08X has no parent save cell", apRef->GetFormEditorID(), CastingType);
    }
}

void ExtraDataList::AddRoomLinkedPortalRef(TESObjectREFR* apRef) {
    if ((!GetRoomLinkedRefData()) && (apRef)) {
        RoomRefData* pNewData;
        void* pMemory = ourMalloc(sizeof(RoomRefData));
        if (pMemory) {
            pNewData = ::new (pMemory) RoomRefData;
        }
        else {
            pNewData = nullptr;
        }
        SetRoomLinkedRefData(pNewData);
    }

    RoomRefData* pData = GetRoomLinkedRefData();
    if (!pData->kPortalList.IsInList(apRef)) {
        pData->kPortalList.AddHead(apRef);
    }
}

NiTMap<WaterZone*, int32_t>* ExtraDataList::QWaterZoneMap() {
    auto* pExtra = static_cast<ExtraWaterZoneMap*>(GetExtraData(EXTRA_WATER_ZONE_MAP));
    if (!pExtra) {
        return nullptr;
    }
    return &pExtra->WaterZoneMap;
}

void ExtraDataList::SetPortal(BSPortal* apPortal) {
    if (apPortal) {
        ExtraPortal* pextra = static_cast<ExtraPortal*>(GetExtraData(120));
        if (pextra) {
            pextra->spPortal = apPortal;
        }
        else {
            ExtraPortal* pextra_2;
            ExtraPortal* v4 = static_cast<ExtraPortal*>(ourMalloc(sizeof(ExtraPortal)));
            if (v4) {
                pextra_2 = ::new (v4) ExtraPortal;
            }
            else {
                pextra_2 = nullptr;
            }
            pextra_2->spPortal = apPortal;
            AddExtra(pextra_2);
        }
    }
    else {
        RemoveExtra(120);
    }
}

void ExtraDataList::SetRoom(BSMultiBoundRoom* apRoom) {
    if (apRoom) {
        auto* pExtra = static_cast<ExtraRoom*>(GetExtraData(EXTRA_ROOM));
        if (pExtra) {
            pExtra->spRoom = apRoom;
        }
        else {
            void* pMemory = ourMalloc(sizeof(ExtraRoom));
            pExtra = pMemory ? ::new (pMemory) ExtraRoom : nullptr;
            pExtra->spRoom = apRoom;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_ROOM);
    }
}

BSMultiBoundRoom* ExtraDataList::GetRoom() {
    auto* pExtra = static_cast<ExtraRoom*>(GetExtraData(EXTRA_ROOM));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->spRoom;
}

BSPortal* ExtraDataList::GetPortal() {
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_PORTAL);
    if (extraData) {
        return static_cast<ExtraPortal*>(extraData)->spPortal.m_pObject;
    }

    return nullptr;
}

PortalLinkedRefData* ExtraDataList::GetPortalLinkedRefData() {
    auto* pExtra = static_cast<ExtraPortalRefData*>(GetExtraData(EXTRA_PORTAL_REF_DATA));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pData;
}

TESObjectREFR* ExtraDataList::GetMasterRoom() {
    if (!GetExtraData(123)) {
        return nullptr;
    }
    if (GetRoomIsMaster()) {
        return nullptr;
    }
    using apVisited_t = NiTMap<TESObjectREFR*, bool>;
    apVisited_t apVisited(37u);
    TESObjectREFR* MasterRoomRecurse = GetMasterRoomRecurse(&apVisited);
    return MasterRoomRecurse;
}

void ExtraDataList::SetOcclusionPlane(BSOcclusionPlane* apPlane) {
    if (apPlane) {
        ExtraOcclusionPlane* pextra = static_cast<ExtraOcclusionPlane*>(GetExtraData(113));
        if (pextra) {
            pextra->spPlane = apPlane;
        }
        else {
            ExtraOcclusionPlane* pextra_2;
            ExtraOcclusionPlane* apMemory = static_cast<ExtraOcclusionPlane*>(ourMalloc(sizeof(ExtraOcclusionPlane)));
            if (apMemory) {
                pextra_2 = ::new (apMemory) ExtraOcclusionPlane;
            }
            else {
                pextra_2 = nullptr;
            }
            pextra_2->spPlane = apPlane;
            AddExtra(pextra_2);
        }
    }
    else {
        RemoveExtra(113);
    }
}

void ExtraDataList::RemoveLightPtr() {
    BaseExtraList::RemoveExtra(EXTRA_LIGHT);
}

void ExtraDataList::RemoveSpellEffectLightPtr() {
    BaseExtraList::RemoveExtra(EXTRA_MAGIC_LIGHT);
}

void ExtraDataList::GetCreatureMovementSound(BSSoundHandle& result) {
    ExtraCreatureMovementSound* pSound = static_cast<ExtraCreatureMovementSound*>(GetExtraData(138));
    if (pSound) {
        result = pSound->pHandle;
    }
    else {
        result.Clear();
    }
}

// As SetCreatureAwakeSound, for the creature movement sound extra.
void ExtraDataList::SetCreatureMovementSound(const BSSoundHandle& afSound) {
    auto* pSound = static_cast<ExtraCreatureMovementSound*>(GetExtraData(EXTRA_CREATURE_MOVEMENT_SOUND));
    if (pSound) {
        if (afSound == BSSoundHandle()) {
            RemoveExtra(pSound, true);
        }
        else {
            pSound->pHandle = afSound;
        }
    }
    else if (afSound != BSSoundHandle()) {
        void* pMemory = ourMalloc(sizeof(ExtraCreatureMovementSound));
        if (pMemory) {
            pSound = ::new (pMemory) ExtraCreatureMovementSound(afSound);
        }
        else {
            pSound = nullptr;
        }
        AddExtra(pSound);
    }
}

void ExtraDataList::RemoveAnimSave() {
    BaseExtraList::RemoveExtra(EXTRA_ANIM_SAVE);
}

void ExtraDataList::RemoveLockPtr() {
    BaseExtraList::RemoveExtra(EXTRA_LOCK);
}

void ExtraDataList::RemoveTeleportPtr() {
    BaseExtraList::RemoveExtra(EXTRA_TELEPORT);
}

void ExtraDataList::RemoveHealth() {
    BaseExtraList::RemoveExtra(EXTRA_HEALTH);
}

void ExtraDataList::RemovePoison() {
    BaseExtraList::RemoveExtra(EXTRA_POISON);
}

void ExtraDataList::RemoveScale() {
    BaseExtraList::RemoveExtra(EXTRA_SCALE);
}

void ExtraDataList::RemoveGhost() {
    BaseExtraList::RemoveExtra(EXTRA_GHOST);
}

void ExtraDataList::RemoveCannotWearExtra() {
    BaseExtraList::RemoveExtra(EXTRA_CANNOTWEAR);
}

void ExtraDataList::RemovePackageStartLocation() {
    BaseExtraList::RemoveExtra(EXTRA_PACKAGESTARTLOC);
}

void ExtraDataList::RemoveStartingWorldOrCell() {
    BaseExtraList::RemoveExtra(EXTRA_STARTINGWORLDORCELL);
}

void ExtraDataList::RemoveObjectHealth() {
    BaseExtraList::RemoveExtra(EXTRA_OBJECT_HEALTH);
}

void ExtraDataList::RemoveExtraRunOncePackageList() {
    BaseExtraList::RemoveExtra(EXTRA_RUN_ONCE_PACKAGES);
}

void ExtraDataList::ClearRumorsExtra() {
    BaseExtraList::RemoveExtra(EXTRA_NO_RUMORS);
}

void ExtraDataList::RemoveFriendHitsExtra() {
    BaseExtraList::RemoveExtra(EXTRA_FRIEND_HITS);
}

void ExtraDataList::RemoveSavedAnimation() {
    BaseExtraList::RemoveExtra(EXTRA_SAVED_ANIMATION);
}

void ExtraDataList::RemoveInfoGeneral() {
    BaseExtraList::RemoveExtra(EXTRA_INFO_GENERAL_TOPIC);
}

void ExtraDataList::RemoveModelSwap() {
    BaseExtraList::RemoveExtra(EXTRA_MODEL_SWAP);
}

TESObjectREFR* ExtraDataList::GetLinkedPlaneRef(int32_t aiEdge) {
    OcclusionPlaneLinkedRefData* occlusionPlaneLinkedRefData = GetOcclusionPlaneLinkedRefData();
    if (occlusionPlaneLinkedRefData) {
        return occlusionPlaneLinkedRefData->pLinkedRefs[aiEdge];
    }

    return nullptr;
}

void ExtraDataList::RemoveAllSaveableData(uint32_t aiFlags, TESObjectREFR* apRef) {
    TESObjectCELL* persistentCell;
    uint8_t v5 = 0;
    if (apRef) {
        v5 = (uint8_t)apRef->IsActor();
    }
    if (v5) {
        persistentCell = GetPersistentCell();
        if ((persistentCell) && (apRef->TESObjectREFR::GetParentCell())) {
            SetPersistentCell(nullptr);
            persistentCell->RemoveReference(apRef);
        }
    }
}

void ExtraDataList::RemoveOriginalReferenceExtra() {
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_ORIGINAL_REFERENCE);
    if (extraData) {
        BaseExtraList::RemoveExtra(extraData, true);
    }
}

void ExtraDataList::RemoveWorn(bool ableftworn) {
    if (ableftworn) {
        BaseExtraList::RemoveExtra(EXTRA_WORN_LEFT);
    }
    else {
        BaseExtraList::RemoveExtra(EXTRA_WORN);
    }
}

void ExtraDataList::RemoveReferencePointer() {
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_REFERENCE_POINTER);
    if (extraData) {
        BaseExtraList::RemoveExtra(extraData, true);
    }
}

REF_ACTIVATE_DATA* ExtraDataList::GetActivateData(TESObjectREFR* apRef) {
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_ACTIVATE_REF);
    if (extraData) {
        return static_cast<ExtraActivateRef*>(extraData)->GetData(apRef);
    }

    return nullptr;
}

float ExtraDataList::GetActivateDelay(TESObjectREFR* apRef) {
    REF_ACTIVATE_DATA* data;
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_ACTIVATE_REF);
    if (extraData && ((data = static_cast<ExtraActivateRef*>(extraData)->GetData(apRef)) != nullptr)) {
        return data->fActivateDelay;
    }

    return 0.0;
}

void ExtraDataList::RemoveFollowerExtra() {
    if (BaseExtraList::GetExtraData(EXTRA_FOLLOWER)) {
        BaseExtraList::RemoveExtra(EXTRA_FOLLOWER);
    }
}

void ExtraDataList::RemoveTalkingActorExtra() {
    BaseExtraList::RemoveExtra(EXTRA_TALKING_ACTOR);
}

void ExtraDataList::RemoveIsModding() {
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_WEAPON_IS_MODDING);
    if (extraData) {
        BaseExtraList::RemoveExtra(extraData, true);
    }
}

AlchemyItem* ExtraDataList::GetPoison() {
    ExtraPoison* extraData = static_cast<ExtraPoison*>(BaseExtraList::GetExtraData(EXTRA_POISON));
    if (extraData) {
        return extraData->pPoison;
    }

    return nullptr;
}

bool ExtraDataList::HasOnlyOwnershipNonDefault() {
    BaseExtraList::ExtraCritSection.Lock(nullptr);
    bool bOwnerShip = false;
    BSExtraData* pHead = this->pHead;
    while (true) {
        if (pHead) {
            switch (pHead->GetExtraType()) {
                case 13:
                case 28:
                case 32:
                case 36:
                case 39:
                case 47:
                case 48:
                case 74: {
                    goto LABEL_8;
                }
                case 33: {
                    bOwnerShip = true;
                LABEL_8:;
                    pHead = pHead->GetNext();
                    continue;
                }
                default: {
                    if (bOwnerShip) {
                        bOwnerShip = false;
                    }
                    goto LABEL_9;
                }
            }
        }
        break;
    }
LABEL_9:;
    BaseExtraList::ExtraCritSection.Unlock();
    return bOwnerShip;
}

uint8_t ExtraDataList::HasWeaponMods() {
    ExtraWeaponModFlags* extraData =
        static_cast<ExtraWeaponModFlags*>(BaseExtraList::GetExtraData(EXTRA_WEAPON_MOD_SLOTS));
    return (uint8_t)(extraData && extraData->HasWeaponMods());
}

void ExtraDataList::SetPoison(AlchemyItem* apPoison) {
    auto* pExtra = static_cast<ExtraPoison*>(GetExtraData(EXTRA_POISON));
    if (pExtra) {
        pExtra->pPoison = apPoison;
        return;
    }
    void* pMemory = ourMalloc(sizeof(ExtraPoison));
    pExtra = pMemory ? ::new (pMemory) ExtraPoison(apPoison) : nullptr;
    AddExtra(pExtra);
}

void ExtraDataList::SetPackageExtraTarget(TESObjectREFR* apRef) {
    ExtraPackage* extraData = static_cast<ExtraPackage*>(BaseExtraList::GetExtraData(EXTRA_PACKAGE));
    if (extraData) {
        extraData->pTarg = apRef;
    }
}

void ExtraDataList::SetPackageExtraIndex(int32_t aiIndex) {
    ExtraPackage* extraData = static_cast<ExtraPackage*>(BaseExtraList::GetExtraData(EXTRA_PACKAGE));
    if (extraData) {
        extraData->iindex = aiIndex;
    }
}

int32_t ExtraDataList::GetPackageExtraIndex() {
    ExtraPackage* extraData = static_cast<ExtraPackage*>(BaseExtraList::GetExtraData(EXTRA_PACKAGE));
    if (extraData) {
        return extraData->iindex;
    }

    return 0;
}

TESObjectREFR* ExtraDataList::GetPackageExtraTarget() {
    ExtraPackage* extraData = static_cast<ExtraPackage*>(BaseExtraList::GetExtraData(EXTRA_PACKAGE));
    if (extraData) {
        return extraData->pTarg;
    }

    return nullptr;
}

void ExtraDataList::SetPackageExtraActionComplete(bool abFlag) {
    ExtraPackage* extraData = static_cast<ExtraPackage*>(BaseExtraList::GetExtraData(EXTRA_PACKAGE));
    if (extraData) {
        extraData->bActionComplete = (bool)((uint8_t)abFlag);
    }
}

ActorPackageData* ExtraDataList::GetActorPackageData() {
    ExtraPackageData* extraData = static_cast<ExtraPackageData*>(BaseExtraList::GetExtraData(EXTRA_PACKAGE_DATA));
    if (extraData) {
        return extraData->pActorPackageData;
    }

    return nullptr;
}

void ExtraDataList::SetExtraSayToResponseIDOnExtra(uint32_t anResponseID) {
    ExtraSayToTopicInfo* extraData =
        static_cast<ExtraSayToTopicInfo*>(BaseExtraList::GetExtraData(EXTRA_SAY_TO_TOPIC_INFO));
    if (extraData) {
        extraData->nResponseID = anResponseID;
    }
}

void ExtraDataList::SetExtraSayToInfoExtra(TESTopicInfo* apInfo) {
    ExtraSayToTopicInfo* v2;
    void* v4;
    ExtraSayToTopicInfo* extraData;
    if (apInfo) {
        extraData = static_cast<ExtraSayToTopicInfo*>(BaseExtraList::GetExtraData(EXTRA_SAY_TO_TOPIC_INFO));
        if (extraData) {
            extraData->pInfo = apInfo;
        }
        else {
            v4 = ourMalloc(sizeof(ExtraSayToTopicInfo));
            if (v4) {
                v2 = ::new (v4) ExtraSayToTopicInfo(apInfo);
            }
            else {
                v2 = nullptr;
            }
            BaseExtraList::AddExtra(v2);
        }
    }
    else {
        RemoveSayToInfoExtra();
    }
}

void ExtraDataList::SetExtraSayToTopicOnExtra(TESTopic* apTopic) {
    ExtraSayToTopicInfo* pExtra;
    void* v4;
    if (apTopic) {
        pExtra = static_cast<ExtraSayToTopicInfo*>(BaseExtraList::GetExtraData(EXTRA_SAY_TO_TOPIC_INFO));
        if (!pExtra) {
            v4 = ourMalloc(sizeof(ExtraSayToTopicInfo));
            if (v4) {
                pExtra = ::new (v4) ExtraSayToTopicInfo;
            }
            else {
                pExtra = nullptr;
            }
            pExtra->pInfo = nullptr;
            BaseExtraList::AddExtra(pExtra);
        }
        pExtra->SetTopic(apTopic);
    }
    else {
        RemoveSayToInfoExtra();
    }
}

bool ExtraDataList::ShouldPopInWhenEnabledByParent() {
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_ENABLESTATEPARENT);
    return (bool)((uint8_t)(extraData && static_cast<ExtraEnableStateParent*>(extraData)->ShouldPopIn()));
}

bool ExtraDataList::ShouldPopOutWhenDisabledByParent() {
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_ENABLESTATEPARENT);
    return (bool)((uint8_t)(extraData && static_cast<ExtraEnableStateParent*>(extraData)->ShouldPopOut()));
}

void ExtraDataList::RemoveActorPackageData() {
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_PACKAGE_DATA);
    if (extraData) {
        BaseExtraList::RemoveExtra(extraData, true);
    }
}

void ExtraDataList::NotifyGuardsAboutActivation(TESObjectREFR* apObject, TESObjectREFR* apActionRef) {
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_GUARDED_REF_DATA);
    if (extraData) {
        static_cast<ExtraGuardedRefData*>(extraData)->NotifyGuardsAboutActivation(apObject, apActionRef);
    }
}

void ExtraDataList::RemoveNavMeshPortal() {
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_NAVMESH_PORTAL);
    if (extraData) {
        BaseExtraList::RemoveExtra(extraData, true);
    }
}

void ExtraDataList::RemoveDismembermentExtra() {
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_DISMEMBERED_LIMBS);
    if (extraData) {
        BaseExtraList::RemoveExtra(extraData, true);
    }
}

void ExtraDataList::SetTalkingActorExtra(BGSTalkingActivator* apTalkAct, TESObjectREFR* apRef) {
    ExtraTalkingActor* v3;
    ExtraTalkingActor* extraData = static_cast<ExtraTalkingActor*>(BaseExtraList::GetExtraData(EXTRA_TALKING_ACTOR));
    if (extraData) {
        BaseExtraList::RemoveExtra(extraData, true);
    }
    void* v5 = ourMalloc(sizeof(ExtraTalkingActor));
    if (v5) {
        v3 = ::new (v5) ExtraTalkingActor(apTalkAct, static_cast<PlayerCharacter*>(apRef));
    }
    else {
        v3 = nullptr;
    }
    BaseExtraList::AddExtra(v3);
}

void ExtraDataList::SetPackageExtra(
    TESPackage* apPack,
    int32_t aiIndex,
    TESObjectREFR* apRef,
    bool abcomplete,
    bool abactivated,
    bool abDoneOnce
) {
    ExtraPackage* v7;
    void* v9;
    ExtraPackage* extraData;
    if ((!apPack) ||
        (((!apPack->GetIsCreated()) || (((int32_t)apPack->GetPackType()) == 1)) &&
         ((!apPack->IsNeverToRun()) &&
          ((((int32_t)apPack->GetPackType()) != 24) && (((int32_t)apPack->GetPackType()) != 23))))) {
        extraData = static_cast<ExtraPackage*>(BaseExtraList::GetExtraData(EXTRA_PACKAGE));
        if (apPack) {
            if (extraData) {
                extraData->pPack = apPack;
                extraData->iindex = aiIndex;
                extraData->pTarg = apRef;
                extraData->bActionComplete = (bool)((uint8_t)abcomplete);
                extraData->bActivated = (bool)((uint8_t)abactivated);
                extraData->bDoneOnce = (bool)((uint8_t)abDoneOnce);
            }
            else {
                v9 = ourMalloc(sizeof(ExtraPackage));
                if (v9) {
                    v7 = ::new (v9) ExtraPackage(apPack, (uint32_t)aiIndex, apRef, abcomplete, abactivated, abDoneOnce);
                }
                else {
                    v7 = nullptr;
                }
                BaseExtraList::AddExtra(v7);
            }
        }
        else {
            if (extraData) {
                RemovePackageExtra();
            }
        }
    }
}

void ExtraDataList::SetDistantDataNormal(const NiPoint3& aNormal) {
    ExtraDistantData* extraDistantData;
    void* v4;
    ExtraDistantData* extraData = static_cast<ExtraDistantData*>(BaseExtraList::GetExtraData(EXTRA_DISTANTDATA));
    if (!extraData) {
        v4 = ourMalloc(sizeof(ExtraDistantData));
        if (v4) {
            extraDistantData = ::new (v4) ExtraDistantData;
        }
        else {
            extraDistantData = nullptr;
        }
        extraData = extraDistantData;
        BaseExtraList::AddExtra(extraDistantData);
    }
    extraData->kNormal = aNormal;
}

void ExtraDataList::SetLinkedPlaneRef(TESObjectREFR* apRef, int32_t aiEdge) {
    OcclusionPlaneLinkedRefData* v3;
    void* v5;
    if ((!GetOcclusionPlaneLinkedRefData()) && (apRef)) {
        v5 = ourMalloc(sizeof(OcclusionPlaneLinkedRefData));
        if (v5) {
            v3 = ::new (v5) OcclusionPlaneLinkedRefData;
        }
        else {
            v3 = nullptr;
        }
        SetOcclusionPlaneLinkedRefData(v3);
    }

    OcclusionPlaneLinkedRefData* occlusionPlaneLinkedRefData = GetOcclusionPlaneLinkedRefData();
    if (occlusionPlaneLinkedRefData) {
        occlusionPlaneLinkedRefData->pLinkedRefs[aiEdge] = apRef;
    }
}

void ExtraDataList::SetPortalLinkedRoomRef(int32_t aiWhich, TESObjectREFR* apRef) {
    ExtraDataList* extra;
    ExtraDataList* v4;
    BSMultiBoundRoom* room;
    TESObjectREFR* masterRoom;
    PortalLinkedRefData* v7;
    void* v9;
    if ((!GetPortalLinkedRefData()) && (apRef)) {
        v9 = ourMalloc(sizeof(PortalLinkedRefData));
        if (v9) {
            v7 = ::new (v9) PortalLinkedRefData;
        }
        else {
            v7 = nullptr;
        }
        SetPortalLinkedRefData(v7);
    }

    PortalLinkedRefData* portalLinkedRefData = GetPortalLinkedRefData();
    if (portalLinkedRefData) {
        portalLinkedRefData->pLinkedRefs[aiWhich] = apRef;
    }
    BSPortal* portal = GetPortal();
    if (apRef) {
        extra = apRef->TESObjectREFR::GetExtra();
        masterRoom = extra->GetMasterRoom();
    }
    else {
        masterRoom = nullptr;
    }
    TESObjectREFR* v10 = masterRoom;
    if (!masterRoom) {
        v10 = apRef;
    }
    if (v10) {
        v4 = v10->TESObjectREFR::GetExtra();
        room = v4->GetRoom();
    }
    else {
        room = nullptr;
    }
    if (portal) {
        if (!aiWhich) {
            portal->SetMultiBoundRoomLeft(room);
        }
        if (aiWhich == 1) {
            portal->SetMultiBoundRoomRight(room);
        }
    }
}

void ExtraDataList::SetActorPackageData(ActorPackageData* apData) {
    ExtraPackageData* v2;
    void* v4;
    ExtraPackageData* extraData = static_cast<ExtraPackageData*>(BaseExtraList::GetExtraData(EXTRA_PACKAGE_DATA));
    if (extraData) {
        extraData->pActorPackageData = apData;
    }
    else {
        v4 = ourMalloc(sizeof(ExtraPackageData));
        if (v4) {
            v2 = ::new (v4) ExtraPackageData(apData);
        }
        else {
            v2 = nullptr;
        }
        BaseExtraList::AddExtra(v2);
    }
}

int32_t ExtraDataList::AddFriendHit() {
    ExtraFriendHits* v2;
    void* v4;
    ExtraFriendHits* extraData = static_cast<ExtraFriendHits*>(BaseExtraList::GetExtraData(EXTRA_FRIEND_HITS));
    if (!extraData) {
        v4 = ourMalloc(sizeof(ExtraFriendHits));
        if (v4) {
            v2 = ::new (v4) ExtraFriendHits;
        }
        else {
            v2 = nullptr;
        }
        extraData = v2;
        BaseExtraList::AddExtra(v2);
    }
    extraData->AddHit();
    return extraData->GetHitCount();
}

void ExtraDataList::RevertUnsavedExtra(BGSLoadFormBuffer* apLoadGameBuffer) {
    BGSChangeFlags v4;
    RemoveSavedAnimation();
    RemoveSavedHavokData();
    RemoveLastFinishedSequence();
    BGSChangeFlags* v2 = &(v4 = apLoadGameBuffer->GetOldChangeFlags());
    if (v2->CheckFlags(0x80000000u)) {
        BaseExtraList::RemoveExtra(EXTRA_USEDMARKERS);
    }
}

void ExtraDataList::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* pref = apLoadGameBuffer->GetReference();
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[117])) {
        ExtraSayToTopicInfo* pextraSayToTopicInfo = static_cast<ExtraSayToTopicInfo*>(GetExtraData(117));
        if ((pextraSayToTopicInfo) && (pref)) {
            if (pref->IsMobileObject()) {
                if (pextraSayToTopicInfo->pTopic && pextraSayToTopicInfo->pInfo) {
                    // The callback's context is the reference's form ID.
                    MobileObject::SayToCallBack(reinterpret_cast<void*>(static_cast<uintptr_t>(pref->iFormID)), true);
                }
                else {
                    MobileObject::SayToCallBack(reinterpret_cast<void*>(static_cast<uintptr_t>(pref->iFormID)), false);
                }
            }
            else {
                pextraSayToTopicInfo->bVoiceDone = true;
            }
        }
    }
    if ((pref) && (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[51]))) {
        NonActorMagicTarget* pextraNonActorMagicTarget = static_cast<NonActorMagicTarget*>(GetExtraData(51));
        if (pextraNonActorMagicTarget) {
            ActiveEffect::FinishLoadActiveEffectList(apLoadGameBuffer, &pextraNonActorMagicTarget->lActiveEffectList);
        }
    }
}

ExtraDismemberedLimbs* ExtraDataList::AddDismembermentExtra() {
    ExtraDismemberedLimbs* v2;
    void* v4;
    ExtraDismemberedLimbs* extraData =
        static_cast<ExtraDismemberedLimbs*>(BaseExtraList::GetExtraData(EXTRA_DISMEMBERED_LIMBS));
    if (!extraData) {
        v4 = ourMalloc(sizeof(ExtraDismemberedLimbs));
        if (v4) {
            v2 = ::new (v4) ExtraDismemberedLimbs;
        }
        else {
            v2 = nullptr;
        }
        extraData = v2;
        BaseExtraList::AddExtra(v2);
    }
    return extraData;
}

void ExtraDataList::SetActorCauseExtra(ActorCause* apActorCause) {
    ExtraActorCause* v2;
    void* v4;
    ExtraActorCause* extraData;
    if (apActorCause) {
        extraData = static_cast<ExtraActorCause*>(BaseExtraList::GetExtraData(EXTRA_ACTOR_CAUSE));
        if (extraData) {
            extraData->SetActorCause(apActorCause);
        }
        else {
            v4 = ourMalloc(sizeof(ExtraActorCause));
            if (v4) {
                v2 = ::new (v4) ExtraActorCause;
            }
            else {
                v2 = nullptr;
            }
            v2->SetActorCause(apActorCause);
            BaseExtraList::AddExtra(v2);
        }
    }
    else {
        BaseExtraList::RemoveExtra(EXTRA_ACTOR_CAUSE);
    }
}

ExtraFollowerSwimBreadcrumbs* ExtraDataList::GetOrCreateExtraFollowerSwimBreadcrumbs() {
    ExtraFollowerSwimBreadcrumbs* v2;
    ExtraFollowerSwimBreadcrumbs* extraData =
        static_cast<ExtraFollowerSwimBreadcrumbs*>(BaseExtraList::GetExtraData(EXTRA_FOLLOWER_SWIM_BREADCRUMBS));
    if (extraData) {
        return extraData;
    }
    void* v4 = ourMalloc(sizeof(ExtraFollowerSwimBreadcrumbs));
    if (v4) {
        v2 = ::new (v4) ExtraFollowerSwimBreadcrumbs;
    }
    else {
        v2 = nullptr;
    }
    extraData = v2;
    BaseExtraList::AddExtra(v2);
    return extraData;
}

void ExtraDataList::SetActionFlag(OBJECT_ACTION aeType) {
    auto* pExtra = static_cast<ExtraAction*>(GetExtraData(EXTRA_ACTION));
    if (!pExtra) {
        if (aeType != OBJECT_ACTION_USE_DEFAULT) {
            pExtra = SetActionPtr();
        }
    }
    else if (aeType == OBJECT_ACTION_USE_DEFAULT && !pExtra->pActionRef) {
        RemoveExtra(pExtra, true);
        pExtra = nullptr;
    }
    if (pExtra) {
        pExtra->eAction = static_cast<uint8_t>(aeType);
    }
}

ExtraAction* ExtraDataList::SetActionPtr() {
    auto* pExtra = static_cast<ExtraAction*>(GetExtraData(EXTRA_ACTION));
    if (pExtra) {
        return pExtra;
    }
    void* pMemory = ourMalloc(sizeof(ExtraAction));
    pExtra = pMemory ? ::new (pMemory) ExtraAction : nullptr;
    AddExtra(pExtra);
    return pExtra;
}

ExtraStartingPosition* ExtraDataList::SetStartingPositionPtr(TESObjectREFR* apObject) {
    auto* pExtra = static_cast<ExtraStartingPosition*>(GetExtraData(EXTRA_STARTINGPOSITION));
    if (pExtra) {
        return pExtra;
    }
    void* pMemory = ourMalloc(sizeof(ExtraStartingPosition));
    pExtra = pMemory ? ::new (pMemory) ExtraStartingPosition(apObject) : nullptr;
    AddExtra(pExtra);
    return pExtra;
}

void ExtraDataList::RemovePlayerCrimeListExtra(Actor* apActor) {
    ExtraPlayerCrimeList* pExtra = static_cast<ExtraPlayerCrimeList*>(GetExtraData(53));
    if (pExtra) {
        if (pExtra->pCrime) {
            for (Crime* pcrime : pExtra->pCrime->ItemsUntilNull()) {
                pcrime->RemoveActorKnowList(apActor);
            }
            pExtra->pCrime->RemoveAll();
            if (pExtra->pCrime) {
                delete pExtra->pCrime;
            }
            pExtra->pCrime = nullptr;
        }
        RemoveExtra(pExtra, true);
    }
}

void ExtraDataList::CopyListForContainer(ExtraDataList* apCopy, bool abDontRemove) {
    int8_t v4;
    BSExtraData* pHead;
    (&BaseExtraList::ExtraCritSection)->Lock("ExtraDataList::CopyListForContainer()");
    pHead = apCopy->pHead;
    while (pHead) {
        v4 = 1;
        switch (pHead->GetExtraType()) {
            case 0xDu: {
                AddExtraCopy(pHead);
                if (!abDontRemove) {
                    apCopy->RemoveExtra(pHead, false);
                    pHead = apCopy->pHead;
                    v4 = 0;
                }
                break;
            }
            case 0x16u:
            case 0x17u:
            case 0x1Cu:
            case 0x21u:
            case 0x22u:
            case 0x23u:
            case 0x25u:
            case 0x26u:
            case 0x27u:
            case 0x28u:
            case 0x2Fu:
            case 0x30u:
            case 0x3Fu:
            case 0x47u:
            case 0x4Au:
            case 0x8Du: {
                AddExtraCopy(pHead);
                if ((pHead->GetExtraType() != 48) && (!abDontRemove)) {
                    apCopy->RemoveExtra(pHead, true);
                    pHead = apCopy->pHead;
                    v4 = 0;
                }
                break;
            }
            default: {
                break;
            }
        }
        if ((pHead) && (v4)) {
            pHead = pHead->pNext;
        }
    }
    (&BaseExtraList::ExtraCritSection)->Unlock();
}

void ExtraDataList::ClearTrespassPackage(TESPackage* apPackage) {
    ExtraTresPassPackage* result =
        static_cast<ExtraTresPassPackage*>(BaseExtraList::GetExtraData(EXTRA_TRESPASS_PACKAGE));
    if ((result) && (result->pPack == apPackage)) {
        result->pPack = nullptr;
    }
}

void ExtraDataList::AddGuard(Actor* apActor) {
    ExtraGuardedRefData* v2;
    void* apMemory;
    ExtraGuardedRefData* apExtra =
        static_cast<ExtraGuardedRefData*>(BaseExtraList::GetExtraData(EXTRA_GUARDED_REF_DATA));
    if (!apExtra) {
        apMemory = ourMalloc(sizeof(ExtraGuardedRefData));
        if (apMemory) {
            v2 = ::new (apMemory) ExtraGuardedRefData;
        }
        else {
            v2 = nullptr;
        }
        apExtra = v2;
        BaseExtraList::AddExtra(v2);
    }
    apExtra->AddGuard(apActor);
}

ExtraNavMeshPortal* ExtraDataList::GetNavMeshPortal() {
    return static_cast<ExtraNavMeshPortal*>(BaseExtraList::GetExtraData(EXTRA_NAVMESH_PORTAL));
}

void ExtraDataList::DuplicateExtraListForContainer(ExtraDataList* apCopy) {
    BSExtraData* i;
    (&BaseExtraList::ExtraCritSection)->Lock("ExtraDataList::DuplicateExtraListForContainer()");
    for (i = apCopy->pHead; i; i = i->pNext) {
        switch (i->GetExtraType()) {
            case 0xDu:
            case 0x16u:
            case 0x17u:
            case 0x1Cu:
            case 0x21u:
            case 0x22u:
            case 0x23u:
            case 0x24u:
            case 0x25u:
            case 0x26u:
            case 0x27u:
            case 0x28u:
            case 0x2Fu:
            case 0x30u:
            case 0x3Fu:
            case 0x47u:
            case 0x4Au:
            case 0x8Du: {
                AddExtraCopy(i);
                break;
            }
            default: {
                continue;
            }
        }
    }
    (&BaseExtraList::ExtraCritSection)->Unlock();
}

void ExtraDataList::LoadGameBGS(BGSLoadFormBuffer* a1) {
    ExtraFollowerSwimBreadcrumb* pcrumb_2;
    int32_t iactorBaseChangeFlags;
    uint8_t cflags;
    char cpackDataType;
    FACTION_RANK* pfactionRank_2;
    uint32_t icrimeNumber;
    PackageNS::CRIME_TYPE ecrimeType;
    Crime* pcrime;
    SayOnceTopicInfos* psayOnceTopicInfo_2;
    Actor* pForm;
    RunOncePackage* prunOncePackage_2;
    int32_t iindex;
    uint8_t cextraType_2;
    bool apDest[156];
    TESObjectREFR* pref__pactor = a1->GetReference();
    Actor* pactor = a1->GetActor();
    ourMemset(apDest, 0, 147u);
    for (BSExtraData* pextra = this->pHead; pextra; pextra = pextra->pNext) {
        uint8_t cextraType = pextra->cEtype;
        if ((xDataTypeToChangeFlag[cextraType]) &&
            (a1->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[cextraType]))) {
            apDest[cextraType] = true;
        }
    }
    uint32_t iextraCount = a1->LoadVariableSizedValue();
    for (uint32_t i = 0; i < iextraCount; ++i) {
        int32_t iactorCauseID;
        uint32_t ioldFormID;
        cextraType_2 = 0;
        a1->LoadData(&cextraType_2, 1u);
        int32_t v184 = cextraType_2 - 13;
        switch (cextraType_2) {
            case 13: {
                ExtraScript* pextraScript;
                if (apDest[cextraType_2]) {
                    pextraScript = static_cast<ExtraScript*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraScript* v162;
                    ExtraScript* v297 = static_cast<ExtraScript*>(ourMalloc(sizeof(ExtraScript)));
                    if (v297) {
                        v162 = ::new (v297) ExtraScript;
                    }
                    else {
                        v162 = nullptr;
                    }
                    BSExtraData* v298 = (BSExtraData*)v162;
                    pextraScript = static_cast<ExtraScript*>(this->AddExtra(v162));
                }
                ExtraScript* pextraScript_2 = pextraScript;
                uint32_t FormID_0 = a1->LoadFormID();
                TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
                Script* v12 = dynamic_cast<Script*>(FormByNumericID);
                pextraScript_2->pScript = v12;
                if (!pextraScript_2->pScriptVars) {
                    ScriptLocals* v161;
                    if (pextraScript_2->pScript) {
                        v161 = pextraScript_2->pScript->CloneLocals();
                    }
                    else {
                        v161 = nullptr;
                    }
                    pextraScript_2->pScriptVars = v161;
                }
                if (!pref__pactor) {
                    a1->SetForm(InventoryChanges::GetTempRef());
                }
                if (pextraScript_2->pScriptVars) {
                    pextraScript_2->pScriptVars->LoadGame(a1);
                }
                else {
                    ScriptLocals tempLocals;
                    tempLocals.LoadGame(a1);
                }
                if (!pref__pactor) {
                    a1->pForm = nullptr;
                }
                break;
            }
            case 22: {
                this->SetWorn(true, false);
                break;
            }
            case 24: {
                ExtraPackageStartLocation* pextraPackageStartLoc;
                if (apDest[cextraType_2]) {
                    pextraPackageStartLoc = static_cast<ExtraPackageStartLocation*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraPackageStartLocation* started;
                    ExtraPackageStartLocation* v291 =
                        static_cast<ExtraPackageStartLocation*>(ourMalloc(sizeof(ExtraPackageStartLocation)));
                    if (v291) {
                        started = ::new (v291) ExtraPackageStartLocation;
                    }
                    else {
                        started = nullptr;
                    }
                    BSExtraData* v292 = (BSExtraData*)started;
                    pextraPackageStartLoc = static_cast<ExtraPackageStartLocation*>(this->AddExtra(started));
                }
                ExtraPackageStartLocation* v423 = pextraPackageStartLoc;
                uint32_t v16 = a1->LoadFormID();
                TESForm* v17 = TESForm::GetFormByNumericID(v16);
                v423->worldLoc.pLocationForm = v17;
                a1->LoadData(&v423->worldLoc.locPt, 12u);
                a1->LoadData(&v423->worldLoc.fZRot, 4u);
                break;
            }
            case 25: {
                ExtraPackage* pextraPackage;
                if (apDest[cextraType_2]) {
                    pextraPackage = static_cast<ExtraPackage*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraPackage* v157;
                    ExtraPackage* v293 = static_cast<ExtraPackage*>(ourMalloc(sizeof(ExtraPackage)));
                    if (v293) {
                        v157 = ::new (v293) ExtraPackage;
                    }
                    else {
                        v157 = nullptr;
                    }
                    BSExtraData* v294 = (BSExtraData*)v157;
                    pextraPackage = static_cast<ExtraPackage*>(this->AddExtra(v157));
                }
                ExtraPackage* pextraPackage_2 = pextraPackage;
                a1->LoadFormID(pextraPackage->pPack);
                a1->LoadFormID(pextraPackage_2->pTarg);
                a1->LoadData(&pextraPackage_2->iindex, 4u);
                a1->LoadData(&pextraPackage_2->bActionComplete, 1u);
                a1->LoadData(&pextraPackage_2->bActivated, 1u);
                a1->LoadData(&pextraPackage_2->bDoneOnce, 1u);
                break;
            }
            case 26: {
                ExtraTresPassPackage* pextraTrespassPackage;
                if (apDest[cextraType_2]) {
                    pextraTrespassPackage = static_cast<ExtraTresPassPackage*>(this->GetExtraData(cextraType_2));
                }
                else {
                    BSExtraData* v117;
                    ExtraTresPassPackage* v252 =
                        static_cast<ExtraTresPassPackage*>(ourMalloc(sizeof(ExtraTresPassPackage)));
                    if (v252) {
                        v117 = ::new (v252) ExtraTresPassPackage;
                    }
                    else {
                        v117 = nullptr;
                    }
                    BSExtraData* v253 = v117;
                    pextraTrespassPackage = static_cast<ExtraTresPassPackage*>(this->AddExtra(v117));
                }
                ExtraTresPassPackage* pextraTrespassPackage_2 = pextraTrespassPackage;
                ioldFormID = 0;
                a1->LoadFormID(ioldFormID);
                if (ioldFormID) {
                    pextraTrespassPackage_2->pPack =
                        static_cast<TrespassPackage*>(TESPackage::CreatePackage(PACKAGE_TYPE_TRESPASS));
                    uint32_t CastingType = pextraTrespassPackage_2->pPack->iFormID;
                    pSaveLoadGame->AddChangedFormID(ioldFormID, CastingType);
                    pextraTrespassPackage_2->pPack->LoadGame(a1);
                }
                break;
            }
            case 27: {
                ExtraRunOncePacks* pextraRunOncePackages;
                if (apDest[cextraType_2]) {
                    pextraRunOncePackages = static_cast<ExtraRunOncePacks*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraRunOncePacks* v153;
                    ExtraRunOncePacks* v289 = static_cast<ExtraRunOncePacks*>(ourMalloc(sizeof(ExtraRunOncePacks)));
                    if (v289) {
                        v153 = ::new (v289) ExtraRunOncePacks;
                    }
                    else {
                        v153 = nullptr;
                    }
                    BSExtraData* v290 = (BSExtraData*)v153;
                    pextraRunOncePackages = static_cast<ExtraRunOncePacks*>(this->AddExtra(v153));
                }
                ExtraRunOncePacks* pextraRunOncePackages_2 = pextraRunOncePackages;
                uint32_t isaveGameCount = a1->LoadVariableSizedValue();
                for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
                    RunOncePackage* prunOncePackage = static_cast<RunOncePackage*>(ourMalloc(sizeof(RunOncePackage)));
                    prunOncePackage_2 = prunOncePackage;
                    uint32_t v18 = a1->LoadFormID();
                    TESForm* v19 = TESForm::GetFormByNumericID(v18);
                    TESPackage* v20 = dynamic_cast<TESPackage*>(v19);
                    prunOncePackage_2->pPackage = v20;
                    a1->LoadData(&prunOncePackage_2->cday, 1u);
                    pextraRunOncePackages_2->pPackageList->AddHead(prunOncePackage_2);
                }
                break;
            }
            case 28: {
                ExtraReferencePointer* pextraReferencePointer;
                if (apDest[cextraType_2]) {
                    pextraReferencePointer = static_cast<ExtraReferencePointer*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraReferencePointer* v168;
                    ExtraReferencePointer* v303 =
                        static_cast<ExtraReferencePointer*>(ourMalloc(sizeof(ExtraReferencePointer)));
                    if (v303) {
                        v168 = ::new (v303) ExtraReferencePointer;
                    }
                    else {
                        v168 = nullptr;
                    }
                    BSExtraData* v304 = (BSExtraData*)v168;
                    pextraReferencePointer = static_cast<ExtraReferencePointer*>(this->AddExtra(v168));
                }
                ExtraReferencePointer* v432 = pextraReferencePointer;
                a1->LoadFormID(pextraReferencePointer->pRef);
                break;
            }
            case 29: {
                ExtraFollower* pextraFollower;
                if (apDest[cextraType_2]) {
                    pextraFollower = static_cast<ExtraFollower*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraFollower* v151;
                    ExtraFollower* v286 = static_cast<ExtraFollower*>(ourMalloc(sizeof(ExtraFollower)));
                    if (v286) {
                        v151 = ::new (v286) ExtraFollower;
                    }
                    else {
                        v151 = nullptr;
                    }
                    BSExtraData* v287 = (BSExtraData*)v151;
                    pextraFollower = static_cast<ExtraFollower*>(this->AddExtra(v151));
                }
                ExtraFollower* pextraFollower_2 = pextraFollower;
                uint32_t isaveGameCount_2 = a1->LoadVariableSizedValue();
                for (uint32_t iloop_2 = 0; iloop_2 < isaveGameCount_2; ++iloop_2) {
                    pForm = nullptr;
                    a1->LoadFormID(pForm);
                    pextraFollower_2->pActorlist->AddHead(pForm);
                }
                break;
            }
            case 30: {
                ExtraLevCreaModifier* pextraLevCreaModifier;
                if (apDest[cextraType_2]) {
                    pextraLevCreaModifier = static_cast<ExtraLevCreaModifier*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraLevCreaModifier* v119;
                    ExtraLevCreaModifier* v254 =
                        static_cast<ExtraLevCreaModifier*>(ourMalloc(sizeof(ExtraLevCreaModifier)));
                    if (v254) {
                        v119 = ::new (v254) ExtraLevCreaModifier;
                    }
                    else {
                        v119 = nullptr;
                    }
                    BSExtraData* v255 = (BSExtraData*)v119;
                    pextraLevCreaModifier = static_cast<ExtraLevCreaModifier*>(this->AddExtra(v119));
                }
                a1->LoadData(&pextraLevCreaModifier->eModifier, 4u);
                break;
            }
            case 31: {
                this->SetGhost(true);
                break;
            }
            case 33: {
                ExtraOwnership* pextraOwner;
                if (apDest[cextraType_2]) {
                    pextraOwner = static_cast<ExtraOwnership*>(this->GetExtraData(cextraType_2));
                }
                else {
                    BSExtraData* v174;
                    ExtraOwnership* v309 = static_cast<ExtraOwnership*>(ourMalloc(sizeof(ExtraOwnership)));
                    if (v309) {
                        v174 = ::new (v309) ExtraOwnership;
                    }
                    else {
                        v174 = nullptr;
                    }
                    BSExtraData* v310 = v174;
                    pextraOwner = static_cast<ExtraOwnership*>(this->AddExtra(v174));
                }
                ExtraOwnership* v435 = pextraOwner;
                uint32_t v2 = a1->LoadFormID();
                TESForm* v3 = TESForm::GetFormByNumericID(v2);
                v435->pOwner = v3;
                break;
            }
            case 34: {
                ExtraGlobal* pextraGlobal;
                if (apDest[cextraType_2]) {
                    pextraGlobal = static_cast<ExtraGlobal*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraGlobal* v170;
                    BSExtraData* v305 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraGlobal)));
                    if (v305) {
                        v170 = ::new (v305) ExtraGlobal;
                    }
                    else {
                        v170 = nullptr;
                    }
                    BSExtraData* v306 = (BSExtraData*)v170;
                    pextraGlobal = static_cast<ExtraGlobal*>(this->AddExtra(v170));
                }
                ExtraGlobal* v433 = pextraGlobal;
                uint32_t v4 = a1->LoadFormID();
                TESForm* v5 = TESForm::GetFormByNumericID(v4);
                TESGlobal* v6 = dynamic_cast<TESGlobal*>(v5);
                v433->pGlobal = v6;
                break;
            }
            case 35: {
                ExtraRank* pextraRank;
                if (apDest[cextraType_2]) {
                    pextraRank = static_cast<ExtraRank*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraRank* v172;
                    BSExtraData* v307 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraRank)));
                    if (v307) {
                        v172 = ::new (v307) ExtraRank;
                    }
                    else {
                        v172 = nullptr;
                    }
                    BSExtraData* v308 = (BSExtraData*)v172;
                    pextraRank = static_cast<ExtraRank*>(this->AddExtra(v172));
                }
                ExtraRank* v434 = pextraRank;
                a1->LoadData(&pextraRank->iRank, 4u);
                break;
            }
            case 36: {
                ExtraCount* pextraCount;
                if (apDest[cextraType_2]) {
                    pextraCount = static_cast<ExtraCount*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraCount* v182;
                    BSExtraData* v317 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraCount)));
                    if (v317) {
                        v182 = ::new (v317) ExtraCount;
                    }
                    else {
                        v182 = nullptr;
                    }
                    BSExtraData* apExtra = (BSExtraData*)v182;
                    pextraCount = static_cast<ExtraCount*>(this->AddExtra(v182));
                }
                ExtraCount* v439 = pextraCount;
                a1->LoadData(&pextraCount->iCount, 2u);
                break;
            }
            case 37: {
                ExtraHealth* pextraHealth;
                if (apDest[cextraType_2]) {
                    pextraHealth = static_cast<ExtraHealth*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraHealth* v180;
                    ExtraHealth* v315 = static_cast<ExtraHealth*>(ourMalloc(sizeof(ExtraHealth)));
                    if (v315) {
                        v180 = ::new (v315) ExtraHealth;
                    }
                    else {
                        v180 = nullptr;
                    }
                    BSExtraData* v316 = (BSExtraData*)v180;
                    pextraHealth = static_cast<ExtraHealth*>(this->AddExtra(v180));
                }
                ExtraHealth* v438 = pextraHealth;
                a1->LoadData(&pextraHealth->fHealth, 4u);
                break;
            }
            case 38: {
                ExtraUses* pextraUses;
                if (apDest[cextraType_2]) {
                    pextraUses = static_cast<ExtraUses*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraUses* v115;
                    BSExtraData* v250 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraUses)));
                    if (v250) {
                        v115 = ::new (v250) ExtraUses;
                    }
                    else {
                        v115 = nullptr;
                    }
                    BSExtraData* v251 = (BSExtraData*)v115;
                    pextraUses = static_cast<ExtraUses*>(this->AddExtra(v115));
                }
                ExtraUses* v375 = pextraUses;
                a1->LoadData(&pextraUses->cUses, 1u);
                break;
            }
            case 39: {
                ExtraTimeLeft* pextraTimeLeft;
                if (apDest[cextraType_2]) {
                    pextraTimeLeft = static_cast<ExtraTimeLeft*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraTimeLeft* v113;
                    ExtraTimeLeft* v248 = static_cast<ExtraTimeLeft*>(ourMalloc(sizeof(ExtraTimeLeft)));
                    if (v248) {
                        v113 = ::new (v248) ExtraTimeLeft;
                    }
                    else {
                        v113 = nullptr;
                    }
                    BSExtraData* v249 = (BSExtraData*)v113;
                    pextraTimeLeft = static_cast<ExtraTimeLeft*>(this->AddExtra(v113));
                }
                ExtraTimeLeft* v374 = pextraTimeLeft;
                a1->LoadData(&pextraTimeLeft->fTime, 4u);
                break;
            }
            case 40: {
                ExtraCharge* pextraCharge;
                if (apDest[cextraType_2]) {
                    pextraCharge = static_cast<ExtraCharge*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraCharge* v111;
                    BSExtraData* v246 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraCharge)));
                    if (v246) {
                        v111 = ::new (v246) ExtraCharge;
                    }
                    else {
                        v111 = nullptr;
                    }
                    BSExtraData* v247 = (BSExtraData*)v111;
                    pextraCharge = static_cast<ExtraCharge*>(this->AddExtra(v111));
                }
                ExtraCharge* v373 = pextraCharge;
                a1->LoadData(&pextraCharge->fCharge, 4u);
                break;
            }
            case 42: {
                ExtraLock* pextraLock;
                if (apDest[cextraType_2]) {
                    pextraLock = static_cast<ExtraLock*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraLock* v109;
                    ExtraLock* v244 = static_cast<ExtraLock*>(ourMalloc(sizeof(ExtraLock)));
                    if (v244) {
                        v109 = ::new (v244) ExtraLock;
                    }
                    else {
                        v109 = nullptr;
                    }
                    BSExtraData* v245 = (BSExtraData*)v109;
                    pextraLock = static_cast<ExtraLock*>(this->AddExtra(v109));
                }
                ExtraLock* pextraLock_2 = pextraLock;
                a1->LoadData(pextraLock->pLock, 1u);
                a1->LoadData(&pextraLock_2->pLock->cFlags, 1u);
                uint32_t v40 = a1->LoadFormID();
                TESForm* v41 = TESForm::GetFormByNumericID(v40);
                TESKey* v42 = dynamic_cast<TESKey*>(v41);
                pextraLock_2->pLock->pKey = v42;
                if (a1->GetVersion() >= 16u) {
                    a1->LoadData(&pextraLock_2->pLock->iNumTries, 4u);
                    a1->LoadData(&pextraLock_2->pLock->iTimesUnlocked, 4u);
                }
                break;
            }
            case 43: {
                ExtraTeleport* pextraTeleport;
                if (apDest[cextraType_2]) {
                    pextraTeleport = static_cast<ExtraTeleport*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraTeleport* v82;
                    ExtraTeleport* v215 = static_cast<ExtraTeleport*>(ourMalloc(sizeof(ExtraTeleport)));
                    if (v215) {
                        v82 = ::new (v215) ExtraTeleport;
                    }
                    else {
                        v82 = nullptr;
                    }
                    BSExtraData* v216 = (BSExtraData*)v82;
                    pextraTeleport = static_cast<ExtraTeleport*>(this->AddExtra(v82));
                }
                ExtraTeleport* pextraTeleport_2 = pextraTeleport;
                if (!pextraTeleport->pTeleportData) {
                    DoorTeleportData* v81;
                    DoorTeleportData* v213 = static_cast<DoorTeleportData*>(ourMalloc(sizeof(DoorTeleportData)));
                    if (v213) {
                        v81 = ::new (v213) DoorTeleportData;
                    }
                    else {
                        v81 = nullptr;
                    }
                    DoorTeleportData* v214 = v81;
                    pextraTeleport_2->pTeleportData = v81;
                }
                pextraTeleport_2->pTeleportData->LoadGame(a1);
                break;
            }
            case 44: {
                BSExtraData* ExtraData;
                cflags = 0;
                a1->LoadData(&cflags, 1u);
                if (apDest[cextraType_2]) {
                    ExtraData = this->GetExtraData(cextraType_2);
                }
                else {
                    ExtraData = nullptr;
                }
                ExtraMapMarker* pextraMapMarker = static_cast<ExtraMapMarker*>(ExtraData);
                if ((ExtraData) && (pextraMapMarker->pMapData)) {
                    pextraMapMarker->pMapData->SetFlags(cflags);
                }

                break;
            }
            case 46: {
                ExtraLeveledCreature* pleveledCreature;
                if (apDest[cextraType_2]) {
                    pleveledCreature = static_cast<ExtraLeveledCreature*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraLeveledCreature* v77;
                    ExtraLeveledCreature* v209 =
                        static_cast<ExtraLeveledCreature*>(ourMalloc(sizeof(ExtraLeveledCreature)));
                    if (v209) {
                        v77 = ::new (v209) ExtraLeveledCreature;
                    }
                    else {
                        v77 = nullptr;
                    }
                    BSExtraData* v210 = (BSExtraData*)v77;
                    pleveledCreature = static_cast<ExtraLeveledCreature*>(this->AddExtra(v77));
                }
                ExtraLeveledCreature* pleveledCreature_2 = pleveledCreature;
                TESActorBase* pcurrentOriginalBase = pleveledCreature->pOriginalBase;
                TESActorBase* pcurrentTemplate = pleveledCreature->pTemplate;
                uint32_t v48 = a1->LoadFormID();
                TESForm* v49 = TESForm::GetFormByNumericID(v48);
                TESActorBase* v50 = dynamic_cast<TESActorBase*>(v49);
                pleveledCreature_2->pOriginalBase = v50;
                uint32_t v51 = a1->LoadFormID();
                TESForm* v52 = TESForm::GetFormByNumericID(v51);
                TESActorBase* v53 = dynamic_cast<TESActorBase*>(v52);
                pleveledCreature_2->pTemplate = v53;
                iactorBaseChangeFlags = 0;
                a1->LoadData(&iactorBaseChangeFlags, 4u);
                if ((pactor &&
                     (pactor->Get3D() && (pcurrentTemplate && pleveledCreature_2->pTemplate != pcurrentTemplate))) ||
                    !pSaveLoadGame->GetSaveGameLoading()) {
                    pactor->Set3D(nullptr, false);
                }
                TESActorBase* pnewActorBase_2 = nullptr;
                bool bcreatedTempActorBase = false;
                bool bresetInventory = false;
                if ((pcurrentTemplate != pleveledCreature_2->pTemplate) &&
                    (!a1->GetChangeFlags().CheckFlags(CHANGE_REFR_LEVELED_INVENTORY | CHANGE_REFR_INVENTORY))) {
                    bresetInventory = true;
                }

                if (bresetInventory) {
                    (&pactor->m_Extra)->RemoveContainerChanges();
                    pactor->ClearProcessItems();
                }
                if (pactor &&
                    (pleveledCreature_2->pOriginalBase &&
                     (pleveledCreature_2->pTemplate && pleveledCreature_2->pOriginalBase->GetUsesLeveledTemplate()))) {
                    pnewActorBase_2 = TESActorBaseData::CreateTemplateActorBase(
                        pleveledCreature_2->pOriginalBase, pleveledCreature_2->pTemplate
                    );
                    TESActorBase* pcurrentActorBase = pactor->GetActorBase();
                    uint32_t v55 = pcurrentActorBase->iFormID;
                    if (pDataHandler->IsFormIDCreated(v55)) {
                        delete pcurrentActorBase;
                    }
                    pactor->SetObjectReference(pnewActorBase_2);
                }
                else {
                    if (pcurrentOriginalBase && pcurrentTemplate) {
                        pleveledCreature_2->pOriginalBase = pcurrentOriginalBase;
                        pleveledCreature_2->pTemplate = pcurrentTemplate;
                    }
                    else {
                        this->RemoveExtra(46);
                    }
                }
                if (bresetInventory) {
                    InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(pactor);
                    pinventory->InitLeveledItems();
                    pinventory->InitFromContainerExtra();
                    pinventory->InitScripts();
                    if (pinventory->IsEmpty()) {
                        (&pactor->m_Extra)->RemoveContainerChanges();
                    }
                }
                if (!pnewActorBase_2) {
                    TESActorBase* pnewActorBase;
                    if (pactor->IsNpc()) {
                        TESActorBase* pnewActorBase_3;
                        TESNPC* v204 = static_cast<TESNPC*>(ourMalloc(sizeof(TESNPC)));
                        if (v204) {
                            pnewActorBase_3 = (TESActorBase*)::new (v204) TESNPC;
                        }
                        else {
                            pnewActorBase_3 = nullptr;
                        }
                        TESActorBase* v205 = pnewActorBase_3;
                        pnewActorBase = pnewActorBase_3;
                    }
                    else {
                        TESActorBase* pnewActorBase_4;
                        TESCreature* v202 = static_cast<TESCreature*>(ourMalloc(sizeof(TESCreature)));
                        if (v202) {
                            pnewActorBase_4 = (TESActorBase*)::new (v202) TESCreature;
                        }
                        else {
                            pnewActorBase_4 = nullptr;
                        }
                        TESActorBase* v203 = pnewActorBase_4;
                        pnewActorBase = pnewActorBase_4;
                    }
                    pnewActorBase_2 = pnewActorBase;
                    bcreatedTempActorBase = true;
                }
                BGSChangeFlags ichangeFlags_3 = a1->GetChangeFlags();
                BGSChangeFlags ioldChangeFlags_2 = a1->GetOldChangeFlags();
                bool breconstructedForm = a1->GetReconstructedForm();
                // Each by-value argument is built in place with BGSChangeFlags's constructor.
                BGSChangeFlags kactorBaseChangeFlags;
                kactorBaseChangeFlags.~BGSChangeFlags();
                ::new (&kactorBaseChangeFlags) BGSChangeFlags(iactorBaseChangeFlags);
                a1->SetChangeFlags(kactorBaseChangeFlags);
                BGSChangeFlags knoChangeFlags;
                knoChangeFlags.~BGSChangeFlags();
                ::new (&knoChangeFlags) BGSChangeFlags(0);
                a1->SetOldChangeFlags(knoChangeFlags);
                a1->SetReconstructedForm(false);
                pnewActorBase_2->LoadGame(a1);
                pnewActorBase_2->AddChangeFlags(iactorBaseChangeFlags);
                a1->SetChangeFlags(ichangeFlags_3);
                a1->SetOldChangeFlags(ioldChangeFlags_2);
                a1->SetReconstructedForm(breconstructedForm);
                if (bcreatedTempActorBase) {
                    delete pnewActorBase_2;
                }
                break;
            }
            case 47: {
                ExtraLeveledItem* pextraLeveledItem;
                if (apDest[cextraType_2]) {
                    pextraLeveledItem = static_cast<ExtraLeveledItem*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraLeveledItem* v107;
                    ExtraLeveledItem* v242 = static_cast<ExtraLeveledItem*>(ourMalloc(sizeof(ExtraLeveledItem)));
                    if (v242) {
                        v107 = ::new (v242) ExtraLeveledItem;
                    }
                    else {
                        v107 = nullptr;
                    }
                    BSExtraData* v243 = (BSExtraData*)v107;
                    pextraLeveledItem = static_cast<ExtraLeveledItem*>(this->AddExtra(v107));
                }
                ExtraLeveledItem* pextraLeveledItem_2 = pextraLeveledItem;
                a1->LoadData(&pextraLeveledItem->iIndex, 4u);
                a1->LoadData(&pextraLeveledItem_2->bdefault, 1u);
                break;
            }
            case 48: {
                ExtraScale* pextraScale;
                if (apDest[cextraType_2]) {
                    pextraScale = static_cast<ExtraScale*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraScale* v176;
                    ExtraScale* v311 = static_cast<ExtraScale*>(ourMalloc(sizeof(ExtraScale)));
                    if (v311) {
                        v176 = ::new (v311) ExtraScale;
                    }
                    else {
                        v176 = nullptr;
                    }
                    BSExtraData* v312 = (BSExtraData*)v176;
                    pextraScale = static_cast<ExtraScale*>(this->AddExtra(v176));
                }
                ExtraScale* v436 = pextraScale;
                a1->LoadData(&pextraScale->fScale, 4u);
                break;
            }
            case 50: {
                NonActorMagicCaster* pextraNonActorMagicCaster;
                if (apDest[cextraType_2]) {
                    pextraNonActorMagicCaster = static_cast<NonActorMagicCaster*>(this->GetExtraData(cextraType_2));
                }
                else {
                    NonActorMagicCaster* v105;
                    NonActorMagicCaster* v240 =
                        static_cast<NonActorMagicCaster*>(ourMalloc(sizeof(NonActorMagicCaster)));
                    if (v240) {
                        v105 = ::new (v240) NonActorMagicCaster;
                    }
                    else {
                        v105 = nullptr;
                    }
                    BSExtraData* v241 = (BSExtraData*)v105;
                    pextraNonActorMagicCaster = static_cast<NonActorMagicCaster*>(this->AddExtra(v105));
                }
                NonActorMagicCaster* pextraNonActorMagicCaster_2 = pextraNonActorMagicCaster;
                a1->LoadFormID(pextraNonActorMagicCaster->pCurrentSpell);
                a1->LoadFormID(pextraNonActorMagicCaster_2->pDesiredTarget);
                a1->LoadFormID(pextraNonActorMagicCaster_2->pCasterObject);
                break;
            }
            case 51: {
                NonActorMagicTarget* pextraNonActorMagicTarget;
                if (apDest[cextraType_2]) {
                    pextraNonActorMagicTarget = static_cast<NonActorMagicTarget*>(this->GetExtraData(cextraType_2));
                }
                else {
                    NonActorMagicTarget* v103;
                    NonActorMagicTarget* v238 =
                        static_cast<NonActorMagicTarget*>(ourMalloc(sizeof(NonActorMagicTarget)));
                    if (v238) {
                        v103 = ::new (v238) NonActorMagicTarget;
                    }
                    else {
                        v103 = nullptr;
                    }
                    BSExtraData* v239 = (BSExtraData*)v103;
                    pextraNonActorMagicTarget = static_cast<NonActorMagicTarget*>(this->AddExtra(v103));
                }
                NonActorMagicTarget* pextraNonActorMagicTarget_2 = pextraNonActorMagicTarget;
                a1->LoadFormID(pextraNonActorMagicTarget->pTargetObject);
                ActiveEffect::LoadActiveEffectList(a1, &pextraNonActorMagicTarget_2->lActiveEffectList);
                break;
            }
            case 53: {
                ExtraPlayerCrimeList* pextraPlayerCrimeList;
                if (apDest[cextraType_2]) {
                    pextraPlayerCrimeList = static_cast<ExtraPlayerCrimeList*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraPlayerCrimeList* v140;
                    BSExtraData* v274 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraPlayerCrimeList)));
                    if (v274) {
                        v140 = ::new (v274) ExtraPlayerCrimeList;
                    }
                    else {
                        v140 = nullptr;
                    }
                    BSExtraData* v275 = (BSExtraData*)v140;
                    pextraPlayerCrimeList = static_cast<ExtraPlayerCrimeList*>(this->AddExtra(v140));
                }
                ExtraPlayerCrimeList* pextraPlayerCrimeList_2 = pextraPlayerCrimeList;
                uint32_t isaveGameCount_4 = a1->LoadVariableSizedValue();
                for (uint32_t iloop_4 = 0; iloop_4 < isaveGameCount_4; ++iloop_4) {
                    icrimeNumber = 0;
                    ecrimeType = PackageNS::CRIME_TYPE_NONE;
                    a1->LoadData(&icrimeNumber, 4u);
                    a1->LoadData(&ecrimeType, 4u);
                    pcrime = ReferenceProcessLists.GetCrimeinGlobalCrimeList(ecrimeType, icrimeNumber);
                    if (pcrime) {
                        pextraPlayerCrimeList_2->pCrime->AddHead(pcrime);
                    }
                    else {
                        const char* v138;
                        int32_t v139;
                        if (pref__pactor) {
                            v139 = pref__pactor->iFormID;
                        }
                        else {
                            v139 = 0;
                        }
                        if (pref__pactor) {
                            v138 = pref__pactor->GetFormEditorID();
                        }
                        else {
                            v138 = "UNKNOWN";
                        }
                        SaveGameWarning(
                            "Could not find crime %i with crime type %i in the global crime list for ref %s "
                            "(%08X)",
                            icrimeNumber, ecrimeType, v138, v139
                        );
                    }
                }
                if (!pextraPlayerCrimeList_2->pCrime->m_item) {
                    this->RemoveExtra(53);
                }
                break;
            }
            case 57: {
                ExtraItemDropper* pextraItemDropper;
                if (apDest[cextraType_2]) {
                    pextraItemDropper = static_cast<ExtraItemDropper*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraItemDropper* v101;
                    ExtraItemDropper* v236 = static_cast<ExtraItemDropper*>(ourMalloc(sizeof(ExtraItemDropper)));
                    if (v236) {
                        v101 = ::new (v236) ExtraItemDropper;
                    }
                    else {
                        v101 = nullptr;
                    }
                    BSExtraData* v237 = (BSExtraData*)v101;
                    pextraItemDropper = static_cast<ExtraItemDropper*>(this->AddExtra(v101));
                }
                ExtraItemDropper* v368 = pextraItemDropper;
                a1->LoadFormID(pextraItemDropper->pDropper);
                break;
            }
            case 60: {
                ExtraMerchantContainer* pextraMerchantContainer;
                if (apDest[cextraType_2]) {
                    pextraMerchantContainer = static_cast<ExtraMerchantContainer*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraMerchantContainer* v79;
                    ExtraMerchantContainer* v211 =
                        static_cast<ExtraMerchantContainer*>(ourMalloc(sizeof(ExtraMerchantContainer)));
                    if (v211) {
                        v79 = ::new (v211) ExtraMerchantContainer;
                    }
                    else {
                        v79 = nullptr;
                    }
                    BSExtraData* v212 = (BSExtraData*)v79;
                    pextraMerchantContainer = static_cast<ExtraMerchantContainer*>(this->AddExtra(v79));
                }
                ExtraMerchantContainer* v347 = pextraMerchantContainer;
                a1->LoadFormID(pextraMerchantContainer->pContainer);
                break;
            }
            case 62: {
                this->SetCanNotWear(true);
                break;
            }
            case 63: {
                ExtraPoison* pextraPoison;
                if (apDest[cextraType_2]) {
                    pextraPoison = static_cast<ExtraPoison*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraPoison* v99;
                    ExtraPoison* v234 = static_cast<ExtraPoison*>(ourMalloc(sizeof(ExtraPoison)));
                    if (v234) {
                        v99 = ::new (v234) ExtraPoison;
                    }
                    else {
                        v99 = nullptr;
                    }
                    BSExtraData* v235 = (BSExtraData*)v99;
                    pextraPoison = static_cast<ExtraPoison*>(this->AddExtra(v99));
                }
                ExtraPoison* pextraPoison_2 = pextraPoison;
                a1->LoadFormID(pextraPoison->pPoison);
                MagicItem* MagicItemByNumericID = MagicItem::GetMagicItemByNumericID(
                    static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraPoison_2->pPoison))
                );
                AlchemyItem* v44 = dynamic_cast<AlchemyItem*>(MagicItemByNumericID);
                pextraPoison_2->pPoison = v44;
                break;
            }
            case 69: {
                ExtraFriendHits* pextraFriendHits;
                if (apDest[cextraType_2]) {
                    pextraFriendHits = static_cast<ExtraFriendHits*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraFriendHits* v136;
                    ExtraFriendHits* v272 = static_cast<ExtraFriendHits*>(ourMalloc(sizeof(ExtraFriendHits)));
                    if (v272) {
                        v136 = ::new (v272) ExtraFriendHits;
                    }
                    else {
                        v136 = nullptr;
                    }
                    BSExtraData* v273 = (BSExtraData*)v136;
                    pextraFriendHits = static_cast<ExtraFriendHits*>(this->AddExtra(v136));
                }
                ExtraFriendHits* pextraFriendHits_2 = pextraFriendHits;
                uint32_t isaveGameCount_5 = a1->LoadVariableSizedValue();
                pextraFriendHits_2->Hits.SetSize(isaveGameCount_5, true);
                for (uint32_t i_2 = 0; i_2 < isaveGameCount_5; ++i_2) {
                    CombatTimeStamp* ftimeStamp = &pextraFriendHits_2->Hits[i_2];
                    ftimeStamp->LoadGame(a1);
                }
                if (!pextraFriendHits_2->Hits.iSize) {
                    this->RemoveFriendHitsExtra();
                }
                break;
            }
            case 70: {
                ExtraHeadingTarget* pextraHeadingTarget;
                if (apDest[cextraType_2]) {
                    pextraHeadingTarget = static_cast<ExtraHeadingTarget*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraHeadingTarget* v97;
                    BSExtraData* v232 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraHeadingTarget)));
                    if (v232) {
                        v97 = ::new (v232) ExtraHeadingTarget;
                    }
                    else {
                        v97 = nullptr;
                    }
                    BSExtraData* v233 = (BSExtraData*)v97;
                    pextraHeadingTarget = static_cast<ExtraHeadingTarget*>(this->AddExtra(v97));
                }
                ExtraHeadingTarget* v366 = pextraHeadingTarget;
                a1->LoadFormID(pextraHeadingTarget->pHeadingTarget);
                break;
            }
            case 73: {
                ExtraStartingWorldOrCell* pextraStartingWorldOrCell;
                if (apDest[cextraType_2]) {
                    pextraStartingWorldOrCell =
                        static_cast<ExtraStartingWorldOrCell*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraStartingWorldOrCell* v166;
                    BSExtraData* v301 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraStartingWorldOrCell)));
                    if (v301) {
                        v166 = ::new (v301) ExtraStartingWorldOrCell;
                    }
                    else {
                        v166 = nullptr;
                    }
                    BSExtraData* v302 = (BSExtraData*)v166;
                    pextraStartingWorldOrCell = static_cast<ExtraStartingWorldOrCell*>(this->AddExtra(v166));
                }
                ExtraStartingWorldOrCell* v431 = pextraStartingWorldOrCell;
                uint32_t v7 = a1->LoadFormID();
                TESForm* v8 = TESForm::GetFormByNumericID(v7);
                v431->pStartingWorldOrCell = v8;
                break;
            }
            case 74: {
                ExtraHotKey* pextraHotKey;
                if (apDest[cextraType_2]) {
                    pextraHotKey = static_cast<ExtraHotKey*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraHotKey* v142;
                    ExtraHotKey* v276 = static_cast<ExtraHotKey*>(ourMalloc(sizeof(ExtraHotKey)));
                    if (v276) {
                        v142 = ::new (v276) ExtraHotKey;
                    }
                    else {
                        v142 = nullptr;
                    }
                    BSExtraData* v277 = (BSExtraData*)v142;
                    pextraHotKey = static_cast<ExtraHotKey*>(this->AddExtra(v142));
                }
                ExtraHotKey* v408 = pextraHotKey;
                a1->LoadData(&pextraHotKey->chotkey, 1u);
                break;
            }
            case 77: {
                ExtraInfoGeneralTopic* pextraInfoGeneralTopic;
                if (apDest[cextraType_2]) {
                    pextraInfoGeneralTopic = static_cast<ExtraInfoGeneralTopic*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraInfoGeneralTopic* v95;
                    ExtraInfoGeneralTopic* v230 =
                        static_cast<ExtraInfoGeneralTopic*>(ourMalloc(sizeof(ExtraInfoGeneralTopic)));
                    if (v230) {
                        v95 = ::new (v230) ExtraInfoGeneralTopic;
                    }
                    else {
                        v95 = nullptr;
                    }
                    BSExtraData* v231 = (BSExtraData*)v95;
                    pextraInfoGeneralTopic = static_cast<ExtraInfoGeneralTopic*>(this->AddExtra(v95));
                }
                ExtraInfoGeneralTopic* v365 = pextraInfoGeneralTopic;
                pextraInfoGeneralTopic->pInfoGen->LoadGame_0083FBE0(a1);
                break;
            }
            case 78: {
                ExtraHasNoRumors* pextraHasNoRumors;
                if (apDest[cextraType_2]) {
                    pextraHasNoRumors = static_cast<ExtraHasNoRumors*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraHasNoRumors* v93;
                    BSExtraData* v228 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraHasNoRumors)));
                    if (v228) {
                        v93 = ::new (v228) ExtraHasNoRumors;
                    }
                    else {
                        v93 = nullptr;
                    }
                    BSExtraData* v229 = (BSExtraData*)v93;
                    pextraHasNoRumors = static_cast<ExtraHasNoRumors*>(this->AddExtra(v93));
                }
                ExtraHasNoRumors* v364 = pextraHasNoRumors;
                a1->LoadData(&pextraHasNoRumors->bNoRumors, 1u);
                break;
            }
            case 80: {
                ExtraTerminalState* pextraTerminalState;
                if (apDest[cextraType_2]) {
                    pextraTerminalState = static_cast<ExtraTerminalState*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraTerminalState* v69;
                    ExtraTerminalState* v196 = static_cast<ExtraTerminalState*>(ourMalloc(sizeof(ExtraTerminalState)));
                    if (v196) {
                        v69 = ::new (v196) ExtraTerminalState;
                    }
                    else {
                        v69 = nullptr;
                    }
                    BSExtraData* v197 = (BSExtraData*)v69;
                    pextraTerminalState = static_cast<ExtraTerminalState*>(this->AddExtra(v69));
                }
                ExtraTerminalState* pextraTerminalState_2 = pextraTerminalState;
                a1->LoadData(&pextraTerminalState->cState, 1u);
                a1->LoadData(&pextraTerminalState_2->cLockOverride, 1u);
                break;
            }
            case 84: {
                ExtraActivateRefChildren* pextraActivateRefChildren;
                if (apDest[cextraType_2]) {
                    pextraActivateRefChildren =
                        static_cast<ExtraActivateRefChildren*>(this->GetExtraData(cextraType_2));
                }
                else {
                    pextraActivateRefChildren = nullptr;
                }
                ExtraActivateRefChildren* pextraActivateRefChildren_2 = pextraActivateRefChildren;
                if (pextraActivateRefChildren) {
                    a1->LoadData(&pextraActivateRefChildren_2->fActivateChildrenTimer, 4u);
                }
                else {
                    char ptempData[4];
                    a1->LoadData(ptempData, 4u);
                }
                break;
            }
            case 85: {
                ExtraTalkingActor* pextraTalkingActor;
                if (apDest[cextraType_2]) {
                    pextraTalkingActor = static_cast<ExtraTalkingActor*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraTalkingActor* v86;
                    BSExtraData* v219 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraTalkingActor)));
                    if (v219) {
                        v86 = ::new (v219) ExtraTalkingActor;
                    }
                    else {
                        v86 = nullptr;
                    }
                    BSExtraData* v220 = (BSExtraData*)v86;
                    pextraTalkingActor = static_cast<ExtraTalkingActor*>(this->AddExtra(v86));
                }
                a1->LoadFormID(pextraTalkingActor->pTalkObject);
                break;
            }
            case 86: {
                ExtraObjectHealth* pextraObjectHealth;
                if (apDest[cextraType_2]) {
                    pextraObjectHealth = static_cast<ExtraObjectHealth*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraObjectHealth* v164;
                    BSExtraData* v299 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraObjectHealth)));
                    if (v299) {
                        v164 = ::new (v299) ExtraObjectHealth;
                    }
                    else {
                        v164 = nullptr;
                    }
                    BSExtraData* v300 = (BSExtraData*)v164;
                    pextraObjectHealth = static_cast<ExtraObjectHealth*>(this->AddExtra(v164));
                }
                ExtraObjectHealth* pextraObjectHealth_2 = pextraObjectHealth;
                a1->LoadData(&pextraObjectHealth->fHealth, 4u);
                uint32_t iselfDamage =
                    BGSDestructibleObjectForm::GetSelfDamage(pref__pactor, pextraObjectHealth_2->fHealth);
                if (iselfDamage) {
                    BGSDestructibleObjectForm::SetSelfDamage(pref__pactor, iselfDamage);
                }
                break;
            }
            case 91: {
                ExtraModelSwap* pextraModelSwap;
                if (apDest[cextraType_2]) {
                    pextraModelSwap = static_cast<ExtraModelSwap*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraModelSwap* v159;
                    BSExtraData* v295 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraModelSwap)));
                    if (v295) {
                        v159 = ::new (v295) ExtraModelSwap;
                    }
                    else {
                        v159 = nullptr;
                    }
                    BSExtraData* v296 = (BSExtraData*)v159;
                    pextraModelSwap = static_cast<ExtraModelSwap*>(this->AddExtra(v159));
                }
                ExtraModelSwap* pextraModelSwap_2 = pextraModelSwap;
                uint32_t v13 = a1->LoadFormID();
                TESForm* v14 = TESForm::GetFormByNumericID(v13);
                pextraModelSwap_2->pModelSwapForm = v14;
                iindex = -1;
                a1->LoadData(&iindex, 4u);
                if (pextraModelSwap_2->pModelSwapForm) {
                    TESModel* ModelSwap =
                        BGSDestructibleObjectForm::GetModelSwap(pextraModelSwap_2->pModelSwapForm, iindex);
                    pextraModelSwap_2->pModelSwap = ModelSwap;
                }
                if (pextraModelSwap_2->pModelSwapForm && pextraModelSwap_2->pModelSwap) {
                    if (pref__pactor) {
                        pref__pactor->Set3D(nullptr, true);
                    }
                }
                else {
                    this->RemoveExtra(91);
                }
                break;
            }
            case 92: {
                ExtraRadius* pextraRadius;
                if (apDest[cextraType_2]) {
                    pextraRadius = static_cast<ExtraRadius*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraRadius* v125;
                    BSExtraData* v260 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraRadius)));
                    if (v260) {
                        v125 = ::new (v260) ExtraRadius;
                    }
                    else {
                        v125 = nullptr;
                    }
                    BSExtraData* v261 = (BSExtraData*)v125;
                    pextraRadius = static_cast<ExtraRadius*>(this->AddExtra(v125));
                }
                ExtraRadius* v380 = pextraRadius;
                a1->LoadData(&pextraRadius->fRadius, 4u);
                break;
            }
            case 93: {
                ExtraRadiation* pextraRadiation;
                if (apDest[cextraType_2]) {
                    pextraRadiation = static_cast<ExtraRadiation*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraRadiation* v123;
                    BSExtraData* v258 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraRadiation)));
                    if (v258) {
                        v123 = ::new (v258) ExtraRadiation;
                    }
                    else {
                        v123 = nullptr;
                    }
                    BSExtraData* v259 = (BSExtraData*)v123;
                    pextraRadiation = static_cast<ExtraRadiation*>(this->AddExtra(v123));
                }
                ExtraRadiation* v379 = pextraRadiation;
                a1->LoadData(&pextraRadiation->fRadiation, 4u);
                break;
            }
            case 94: {
                ExtraFactionChanges* pextraFactionChanges;
                if (apDest[cextraType_2]) {
                    pextraFactionChanges = static_cast<ExtraFactionChanges*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraFactionChanges* v134;
                    ExtraFactionChanges* v270 =
                        static_cast<ExtraFactionChanges*>(ourMalloc(sizeof(ExtraFactionChanges)));
                    if (v270) {
                        v134 = ::new (v270) ExtraFactionChanges;
                    }
                    else {
                        v134 = nullptr;
                    }
                    BSExtraData* v271 = (BSExtraData*)v134;
                    pextraFactionChanges = static_cast<ExtraFactionChanges*>(this->AddExtra(v134));
                }
                ExtraFactionChanges* pextraFactionChanges_2 = pextraFactionChanges;
                uint32_t isaveGameCount_6 = a1->LoadVariableSizedValue();
                for (uint32_t iloop_5 = 0; iloop_5 < isaveGameCount_6; ++iloop_5) {
                    FACTION_RANK* pfactionRank = static_cast<FACTION_RANK*>(ourMalloc(sizeof(FACTION_RANK)));
                    pfactionRank_2 = pfactionRank;
                    uint32_t v30 = a1->LoadFormID();
                    TESForm* v31 = TESForm::GetFormByNumericID(v30);
                    TESFaction* v32 = dynamic_cast<TESFaction*>(v31);
                    pfactionRank_2->pFaction = v32;
                    a1->LoadData(&pfactionRank_2->cRank, 1u);
                    pextraFactionChanges_2->pFactionChanges->AddHead(pfactionRank_2);
                }
                BSSimpleList<FACTION_RANK*>* plist = pextraFactionChanges_2->pFactionChanges;
                BSSimpleList<FACTION_RANK*>* plastList = nullptr;
                while (plist && !plist->IsEmpty()) {
                    FACTION_RANK* pfactionRank_3 = plist->m_item;
                    bool bremove = false;
                    if (pfactionRank_3) {
                        if (!pfactionRank_3->pFaction) {
                            void* Block = (void*)pfactionRank_3;
                            ourFree(pfactionRank_3);
                            bremove = true;
                        }
                    }
                    else {
                        bremove = true;
                    }
                    if (bremove) {
                        if (plastList) {
                            plastList->Remove(plist->m_item);
                            plist = plastList->m_pkNext;
                        }
                        else {
                            plist->RemoveHead();
                        }
                    }
                    else {
                        plastList = plist;
                        plist = plist->m_pkNext;
                    }
                }
                break;
            }
            case 95: {
                ExtraDismemberedLimbs* pextraDismemberedLimbs;
                ExtraDismemberedLimbs* pcurrentDismemberedLimbs;
                if (apDest[cextraType_2]) {
                    pcurrentDismemberedLimbs = static_cast<ExtraDismemberedLimbs*>(this->GetExtraData(cextraType_2));
                }
                else {
                    pcurrentDismemberedLimbs = nullptr;
                }
                ExtraDismemberedLimbs* pcurrentDismemberedLimbs_2 = pcurrentDismemberedLimbs;
                if (pcurrentDismemberedLimbs) {
                    this->RemoveExtra(pcurrentDismemberedLimbs_2, false);
                }
                else {
                    if ((a1->GetOldChangeFlags().CheckFlags(CHANGE_ACTOR_EXTRA_DISMEMBERED_LIMBS)) &&
                        (pactor->Get3D())) {
                        pactor->ClearDismemberedLimbs();
                    }
                }
                ExtraDismemberedLimbs* v225 =
                    static_cast<ExtraDismemberedLimbs*>(ourMalloc(sizeof(ExtraDismemberedLimbs)));
                if (v225) {
                    pextraDismemberedLimbs = ::new (v225) ExtraDismemberedLimbs;
                }
                else {
                    pextraDismemberedLimbs = nullptr;
                }
                ExtraDismemberedLimbs* v226 = pextraDismemberedLimbs;
                ExtraDismemberedLimbs* pextraDismemberedLimbs_2 = pextraDismemberedLimbs;
                a1->LoadData(&pextraDismemberedLimbs->sLimbs, 2u);
                a1->LoadData(&pextraDismemberedLimbs_2->eCauseofDeath, 4u);
                a1->LoadData(&pextraDismemberedLimbs_2->eLastHitLimb, 4u);
                a1->LoadData(&pextraDismemberedLimbs_2->bEaten, 1u);
                a1->LoadFormID(pextraDismemberedLimbs_2->pDeathObject);
                uint32_t isaveGameCount_8 = a1->LoadVariableSizedValue();
                pextraDismemberedLimbs_2->DismemberedLimbs.SetSize(isaveGameCount_8, true);
                for (uint32_t i_4 = 0; i_4 < isaveGameCount_8; ++i_4) {
                    DismemberedLimb* v90;
                    DismemberedLimb** pdismemberedLimb = &pextraDismemberedLimbs_2->DismemberedLimbs[i_4];
                    DismemberedLimb* v223 = static_cast<DismemberedLimb*>(ourMalloc(sizeof(DismemberedLimb)));
                    if (v223) {
                        v90 = ::new (v223) DismemberedLimb;
                    }
                    else {
                        v90 = nullptr;
                    }
                    DismemberedLimb* v224 = v90;
                    *pdismemberedLimb = v90;
                    a1->LoadData(*pdismemberedLimb, 1u);
                    a1->LoadData(&(*pdismemberedLimb)->bLimbExploded, 1u);
                    a1->LoadData(&(*pdismemberedLimb)->bObjectArrayIdentical, 1u);
                    if (a1->GetVersion() >= 16u) {
                        a1->LoadData(&(*pdismemberedLimb)->bLimbRemoved, 1u);
                    }
                    uint32_t isaveGameCount_9 = a1->LoadVariableSizedValue();
                    (*pdismemberedLimb)->ObjectArray.SetSize(isaveGameCount_9, true);
                    for (uint32_t i_5 = 0; i_5 < isaveGameCount_9; ++i_5) {
                        TESBoundObject** pForm_2 = &(*pdismemberedLimb)->ObjectArray[i_5];
                        a1->LoadFormID(*pForm_2);
                    }
                    uint32_t isaveGameCount_10 = (*pdismemberedLimb)->ObjectArray.iSize;
                    for (uint32_t i_6 = 0; i_6 < isaveGameCount_10; ++i_6) {
                        TESBoundObject* v89;
                        bool bRemove = false;
                        TESBoundObject** pForm_3 = &(*pdismemberedLimb)->ObjectArray[i_6];
                        if (*pForm_3) {
                            TESForm* v46 = TESForm::GetFormByNumericID(
                                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(*pForm_3))
                            );
                            v89 = dynamic_cast<TESBoundObject*>(v46);
                        }
                        else {
                            v89 = nullptr;
                        }
                        *pForm_3 = v89;
                        if (!*pForm_3) {
                            bRemove = true;
                        }
                        if (bRemove) {
                            (*pdismemberedLimb)->ObjectArray.RemoveFast(i_6--, 1u);
                            --isaveGameCount_10;
                        }
                    }
                }
                bool blimbsIdentical = false;
                if (pcurrentDismemberedLimbs_2) {
                    if (pcurrentDismemberedLimbs_2->CheckDismemberedLimbsIdentical(pextraDismemberedLimbs_2)) {
                        blimbsIdentical = true;
                    }
                    else {
                        pactor->ClearDismemberedLimbs();
                    }
                    delete pcurrentDismemberedLimbs_2;
                }
                this->AddExtra(pextraDismemberedLimbs_2);
                if (!blimbsIdentical) {
                    pactor->CreateDismemberedLimbs();
                }
                break;
            }
            case 96: {
                ExtraActorCause* pextraActorCause;
                if (apDest[cextraType_2]) {
                    pextraActorCause = static_cast<ExtraActorCause*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraActorCause* v84;
                    ExtraActorCause* v217 = static_cast<ExtraActorCause*>(ourMalloc(sizeof(ExtraActorCause)));
                    if (v217) {
                        v84 = ::new (v217) ExtraActorCause;
                    }
                    else {
                        v84 = nullptr;
                    }
                    BSExtraData* v218 = (BSExtraData*)v84;
                    pextraActorCause = static_cast<ExtraActorCause*>(this->AddExtra(v84));
                }
                ExtraActorCause* pextraActorCause_2 = pextraActorCause;
                iactorCauseID = -1;
                a1->LoadData(&iactorCauseID, 4u);
                ActorCause* ActorCauseByID = ActorCause::GetActorCauseByID(iactorCauseID);
                pextraActorCause_2->spActorCause = ActorCauseByID;
                if (!pextraActorCause_2->spActorCause.m_pObject) {
                    this->RemoveExtra(96);
                }
                break;
            }
            case 108: {
                ExtraOpenCloseActivateRef* pextraActivateRef;
                if (apDest[cextraType_2]) {
                    pextraActivateRef = static_cast<ExtraOpenCloseActivateRef*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraOpenCloseActivateRef* v149;
                    BSExtraData* v284 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraOpenCloseActivateRef)));
                    if (v284) {
                        v149 = ::new (v284) ExtraOpenCloseActivateRef;
                    }
                    else {
                        v149 = nullptr;
                    }
                    BSExtraData* v285 = (BSExtraData*)v149;
                    pextraActivateRef = static_cast<ExtraOpenCloseActivateRef*>(this->AddExtra(v149));
                }
                ExtraOpenCloseActivateRef* v414 = pextraActivateRef;
                a1->LoadFormID(pextraActivateRef->pActivateRef);
                break;
            }
            case 110: {
                ExtraAmmo* pextraAmmo;
                if (apDest[cextraType_2]) {
                    pextraAmmo = static_cast<ExtraAmmo*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraAmmo* v121;
                    BSExtraData* v256 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraAmmo)));
                    if (v256) {
                        v121 = ::new (v256) ExtraAmmo;
                    }
                    else {
                        v121 = nullptr;
                    }
                    BSExtraData* v257 = (BSExtraData*)v121;
                    pextraAmmo = static_cast<ExtraAmmo*>(this->AddExtra(v121));
                }
                ExtraAmmo* v378 = pextraAmmo;
                uint32_t v36 = a1->LoadFormID();
                TESForm* v37 = TESForm::GetFormByNumericID(v36);
                TESAmmo* v38 = dynamic_cast<TESAmmo*>(v37);
                v378->pAmmo = v38;
                a1->LoadData(&v378->iCount, 4u);
                break;
            }
            case 112: {
                ExtraPackageData* pextraPackageData;
                if (apDest[cextraType_2]) {
                    pextraPackageData = static_cast<ExtraPackageData*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraPackageData* v132;
                    ExtraPackageData* v266 = static_cast<ExtraPackageData*>(ourMalloc(sizeof(ExtraPackageData)));
                    if (v266) {
                        v132 = ::new (v266) ExtraPackageData;
                    }
                    else {
                        v132 = nullptr;
                    }
                    BSExtraData* v267 = (BSExtraData*)v132;
                    pextraPackageData = static_cast<ExtraPackageData*>(this->AddExtra(v132));
                }
                ExtraPackageData* pextraPackageData_2 = pextraPackageData;
                cpackDataType = -1;
                a1->LoadData(&cpackDataType, 1u);
                if (cpackDataType != -1) {
                    pextraPackageData_2->pActorPackageData =
                        ActorPackageData::CreateActorPackageData(static_cast<PACKAGE_TYPE>(cpackDataType));
                    pextraPackageData_2->pActorPackageData->LoadGame(a1);
                }
                break;
            }
            case 115: {
                ExtraSayTopicInfoOnceADay* pextraTopicInfo;
                if (apDest[cextraType_2]) {
                    pextraTopicInfo = static_cast<ExtraSayTopicInfoOnceADay*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraSayTopicInfoOnceADay* v147;
                    ExtraSayTopicInfoOnceADay* v282 =
                        static_cast<ExtraSayTopicInfoOnceADay*>(ourMalloc(sizeof(ExtraSayTopicInfoOnceADay)));
                    if (v282) {
                        v147 = ::new (v282) ExtraSayTopicInfoOnceADay;
                    }
                    else {
                        v147 = nullptr;
                    }
                    BSExtraData* v283 = (BSExtraData*)v147;
                    pextraTopicInfo = static_cast<ExtraSayTopicInfoOnceADay*>(this->AddExtra(v147));
                }
                ExtraSayTopicInfoOnceADay* pextraTopicInfo_2 = pextraTopicInfo;
                uint32_t isaveGameCount_3 = a1->LoadVariableSizedValue();
                for (uint32_t iloop_3 = 0; iloop_3 < isaveGameCount_3; ++iloop_3) {
                    SayOnceTopicInfos* psayOnceTopicInfo;
                    SayOnceTopicInfos* v280 = static_cast<SayOnceTopicInfos*>(ourMalloc(sizeof(SayOnceTopicInfos)));
                    if (v280) {
                        psayOnceTopicInfo = ::new (v280) SayOnceTopicInfos;
                    }
                    else {
                        psayOnceTopicInfo = nullptr;
                    }
                    SayOnceTopicInfos* v281 = psayOnceTopicInfo;
                    psayOnceTopicInfo_2 = psayOnceTopicInfo;
                    uint32_t v21 = a1->LoadFormID();
                    TESForm* v22 = TESForm::GetFormByNumericID(v21);
                    TESTopicInfo* v23 = dynamic_cast<TESTopicInfo*>(v22);
                    psayOnceTopicInfo_2->pInfo = v23;
                    a1->LoadData(&psayOnceTopicInfo_2->iDateStamp, 4u);
                    a1->LoadData(&psayOnceTopicInfo_2->fHourStamp, 4u);
                    pextraTopicInfo_2->pListofSaidOnceTopicInfos->AddHead(psayOnceTopicInfo_2);
                }
                break;
            }
            case 116: {
                ExtraEncounterZone* pextraEncounterZone;
                if (apDest[cextraType_2]) {
                    pextraEncounterZone = static_cast<ExtraEncounterZone*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraEncounterZone* v127;
                    ExtraEncounterZone* v262 = static_cast<ExtraEncounterZone*>(ourMalloc(sizeof(ExtraEncounterZone)));
                    if (v262) {
                        v127 = ::new (v262) ExtraEncounterZone;
                    }
                    else {
                        v127 = nullptr;
                    }
                    BSExtraData* v263 = (BSExtraData*)v127;
                    pextraEncounterZone = static_cast<ExtraEncounterZone*>(this->AddExtra(v127));
                }
                ExtraEncounterZone* v381 = pextraEncounterZone;
                uint32_t v33 = a1->LoadFormID();
                TESForm* v34 = TESForm::GetFormByNumericID(v33);
                BGSEncounterZone* v35 = dynamic_cast<BGSEncounterZone*>(v34);
                v381->pZone = v35;
                break;
            }
            case 117: {
                ExtraSayToTopicInfo* pextraSayToTopicInfo;
                if (apDest[cextraType_2]) {
                    pextraSayToTopicInfo = static_cast<ExtraSayToTopicInfo*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraSayToTopicInfo* v144;
                    ExtraSayToTopicInfo* v278 =
                        static_cast<ExtraSayToTopicInfo*>(ourMalloc(sizeof(ExtraSayToTopicInfo)));
                    if (v278) {
                        v144 = ::new (v278) ExtraSayToTopicInfo;
                    }
                    else {
                        v144 = nullptr;
                    }
                    BSExtraData* v279 = (BSExtraData*)v144;
                    pextraSayToTopicInfo = static_cast<ExtraSayToTopicInfo*>(this->AddExtra(v144));
                }
                ExtraSayToTopicInfo* v409 = pextraSayToTopicInfo;
                uint32_t v24 = a1->LoadFormID();
                TESForm* v25 = TESForm::GetFormByNumericID(v24);
                TESTopic* v26 = dynamic_cast<TESTopic*>(v25);
                v409->pTopic = v26;
                uint32_t v27 = a1->LoadFormID();
                TESForm* v28 = TESForm::GetFormByNumericID(v27);
                TESTopicInfo* v29 = dynamic_cast<TESTopicInfo*>(v28);
                v409->pInfo = v29;
                a1->LoadData(&v409->bVoiceDone, 1u);
                break;
            }
            case 124: {
                ExtraGuardedRefData* pextraGuardedRefData;
                if (apDest[cextraType_2]) {
                    pextraGuardedRefData = static_cast<ExtraGuardedRefData*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraGuardedRefData* v129;
                    ExtraGuardedRefData* v264 =
                        static_cast<ExtraGuardedRefData*>(ourMalloc(sizeof(ExtraGuardedRefData)));
                    if (v264) {
                        v129 = ::new (v264) ExtraGuardedRefData;
                    }
                    else {
                        v129 = nullptr;
                    }
                    BSExtraData* v265 = (BSExtraData*)v129;
                    pextraGuardedRefData = static_cast<ExtraGuardedRefData*>(this->AddExtra(v129));
                }
                ExtraGuardedRefData* pextraGuardedRefData_2 = pextraGuardedRefData;
                uint32_t isaveGameCount_7 = a1->LoadVariableSizedValue();
                pextraGuardedRefData_2->Guards.SetSize(isaveGameCount_7, true);
                for (uint32_t i_3 = 0; i_3 < isaveGameCount_7; ++i_3) {
                    uint32_t* iguardID = (uint32_t*)&pextraGuardedRefData_2->Guards[i_3];
                    a1->LoadFormID(*iguardID);
                }
                break;
            }
            case 137: {
                ExtraAshPileRef* pextraAshPileRef;
                if (apDest[cextraType_2]) {
                    pextraAshPileRef = static_cast<ExtraAshPileRef*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraAshPileRef* v67;
                    BSExtraData* v194 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraAshPileRef)));
                    if (v194) {
                        v67 = ::new (v194) ExtraAshPileRef;
                    }
                    else {
                        v67 = nullptr;
                    }
                    BSExtraData* v195 = (BSExtraData*)v67;
                    pextraAshPileRef = static_cast<ExtraAshPileRef*>(this->AddExtra(v67));
                }
                ExtraAshPileRef* v331 = pextraAshPileRef;
                a1->LoadFormID(pextraAshPileRef->pAshPileRef);
                break;
            }
            case 139: {
                ExtraFollowerSwimBreadcrumbs* pextraCrumbs;
                if (apDest[cextraType_2]) {
                    pextraCrumbs = static_cast<ExtraFollowerSwimBreadcrumbs*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraFollowerSwimBreadcrumbs* ExtraFollowerSwimBreadcrumbs_;
                    ExtraFollowerSwimBreadcrumbs* v192 =
                        static_cast<ExtraFollowerSwimBreadcrumbs*>(ourMalloc(sizeof(ExtraFollowerSwimBreadcrumbs)));
                    if (v192) {
                        ExtraFollowerSwimBreadcrumbs_ = ::new (v192) ExtraFollowerSwimBreadcrumbs;
                    }
                    else {
                        ExtraFollowerSwimBreadcrumbs_ = nullptr;
                    }
                    BSExtraData* v193 = (BSExtraData*)ExtraFollowerSwimBreadcrumbs_;
                    pextraCrumbs =
                        static_cast<ExtraFollowerSwimBreadcrumbs*>(this->AddExtra(ExtraFollowerSwimBreadcrumbs_));
                }
                ExtraFollowerSwimBreadcrumbs* pextraCrumbs_2 = pextraCrumbs;
                a1->LoadData(&pextraCrumbs->LeaderLocation, 12u);
                if (a1->GetVersion() < 18u) {
                    a1->LoadData(&pextraCrumbs_2->LeaderNavMeshID, 4u);
                }
                else {
                    a1->LoadFormID(pextraCrumbs_2->LeaderNavMeshID);
                }
                a1->LoadData(&pextraCrumbs_2->eLeaderState, 4u);
                uint32_t isaveGameCount_11 = a1->LoadVariableSizedValue();
                for (uint32_t iloop_6 = 0; iloop_6 < isaveGameCount_11; ++iloop_6) {
                    ExtraFollowerSwimBreadcrumb* pcrumb;
                    ExtraFollowerSwimBreadcrumb* v190 =
                        static_cast<ExtraFollowerSwimBreadcrumb*>(ourMalloc(sizeof(ExtraFollowerSwimBreadcrumb)));
                    if (v190) {
                        pcrumb = ::new (v190) ExtraFollowerSwimBreadcrumb;
                    }
                    else {
                        pcrumb = nullptr;
                    }
                    pcrumb_2 = pcrumb;
                    a1->LoadData(pcrumb, 12u);
                    if (a1->GetVersion() < 18u) {
                        a1->LoadData(&pcrumb_2->StartNavMeshID, 4u);
                    }
                    else {
                        a1->LoadFormID(pcrumb_2->StartNavMeshID);
                    }
                    a1->LoadData(&pcrumb_2->EndLocation, 12u);
                    if (a1->GetVersion() < 18u) {
                        a1->LoadData(&pcrumb_2->EndNavMeshID, 4u);
                    }
                    else {
                        a1->LoadFormID(pcrumb_2->EndNavMeshID);
                    }
                    a1->LoadData(&pcrumb_2->bEnteringWater, 1u);
                    pextraCrumbs_2->CrumbList.AddHead(pcrumb_2);
                }
                break;
            }
            case 141: {
                ExtraWeaponModFlags* pextraWMF;
                if (apDest[cextraType_2]) {
                    pextraWMF = static_cast<ExtraWeaponModFlags*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraWeaponModFlags* v178;
                    BSExtraData* v313 = static_cast<BSExtraData*>(ourMalloc(sizeof(ExtraWeaponModFlags)));
                    if (v313) {
                        v178 = ::new (v313) ExtraWeaponModFlags;
                    }
                    else {
                        v178 = nullptr;
                    }
                    BSExtraData* v314 = (BSExtraData*)v178;
                    pextraWMF = static_cast<ExtraWeaponModFlags*>(this->AddExtra(v178));
                }
                ExtraWeaponModFlags* v437 = pextraWMF;
                a1->LoadData(&pextraWMF->cWeaponModsActive, 1u);
                break;
            }
            case 143: {
                ExtraSecuritronFace* pFaceExtra;
                char workstring[256];
                if (apDest[cextraType_2]) {
                    pFaceExtra = static_cast<ExtraSecuritronFace*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraSecuritronFace* v62;
                    ExtraSecuritronFace* v188 =
                        static_cast<ExtraSecuritronFace*>(ourMalloc(sizeof(ExtraSecuritronFace)));
                    if (v188) {
                        v62 = ::new (v188) ExtraSecuritronFace;
                    }
                    else {
                        v62 = nullptr;
                    }
                    BSExtraData* v189 = (BSExtraData*)v62;
                    pFaceExtra = static_cast<ExtraSecuritronFace*>(this->AddExtra(v62));
                }
                ExtraSecuritronFace* pFaceExtra_2 = pFaceExtra;
                a1->LoadString(workstring);
                pFaceExtra_2->SetPersonality(workstring);
                a1->LoadString(workstring);
                pFaceExtra_2->SetMood(workstring);
                break;
            }
            case 144:
            case 145: {
                break;
            }
            case 146: {
                ExtraSpecialRenderFlags* pOEIExtra;
                if (apDest[cextraType_2]) {
                    pOEIExtra = static_cast<ExtraSpecialRenderFlags*>(this->GetExtraData(cextraType_2));
                }
                else {
                    ExtraSpecialRenderFlags* v60;
                    ExtraSpecialRenderFlags* v186 =
                        static_cast<ExtraSpecialRenderFlags*>(ourMalloc(sizeof(ExtraSpecialRenderFlags)));
                    if (v186) {
                        v60 = ::new (v186) ExtraSpecialRenderFlags;
                    }
                    else {
                        v60 = nullptr;
                    }
                    BSExtraData* v187 = (BSExtraData*)v60;
                    pOEIExtra = static_cast<ExtraSpecialRenderFlags*>(this->AddExtra(v60));
                }
                uint32_t uiRenderFlags;
                a1->LoadData(&uiRenderFlags, 4u);
                a1->LoadData(&pOEIExtra->m_fNodeMaxFadeDistance, 4u);
                pOEIExtra->m_flags = uiRenderFlags;
                break;
            }
            default: {
                SaveGameWarning("Unknown extra data type %i found in ExtraDataList::LoadGame", cextraType_2);
                break;
            }
        }
        apDest[cextraType_2] = false;
    }
    for (uint32_t i_7 = 0; i_7 < 147; ++i_7) {
        bool bremoveExtra = apDest[i_7];
        if (bremoveExtra) {
            uint32_t v59 = i_7;
            if (i_7 == 46) {
                bremoveExtra = false;
            }
            else {
                if (v59 == 146) {
                    BSExtraData* v57 = this->GetExtraData(146);
                    ExtraSpecialRenderFlags* pspecialRenderFlags = dynamic_cast<ExtraSpecialRenderFlags*>(v57);
                    if (pspecialRenderFlags) {
                        pspecialRenderFlags->m_flags = pspecialRenderFlags->m_flags & 7;
                    }
                    bremoveExtra = false;
                }
            }
        }
        if (bremoveExtra) {
            this->RemoveExtra(i_7);
        }
    }
}

void ExtraDataList::GetWeaponIdleSound(BSSoundHandle& result) {
    ExtraWeaponIdleSound* pSound = static_cast<ExtraWeaponIdleSound*>(GetExtraData(131));
    if (pSound) {
        result = pSound->pHandle;
    }
    else {
        result.Clear();
    }
}

// As SetCreatureAwakeSound, for the weapon idle sound extra.
void ExtraDataList::SetWeaponIdleSound(const BSSoundHandle& afSound) {
    auto* pSound = static_cast<ExtraWeaponIdleSound*>(GetExtraData(EXTRA_WEAPON_IDLE_SOUND));
    if (pSound) {
        if (afSound == BSSoundHandle()) {
            RemoveExtra(pSound, true);
        }
        else {
            pSound->pHandle = afSound;
        }
    }
    else if (afSound != BSSoundHandle()) {
        void* pMemory = ourMalloc(sizeof(ExtraWeaponIdleSound));
        if (pMemory) {
            pSound = ::new (pMemory) ExtraWeaponIdleSound(afSound);
        }
        else {
            pSound = nullptr;
        }
        AddExtra(pSound);
    }
}

void ExtraDataList::GetActivateLoopSound(BSSoundHandle& result) {
    ExtraActivateLoopSound* pSound = static_cast<ExtraActivateLoopSound*>(GetExtraData(135));
    if (pSound) {
        result = pSound->pHandle;
    }
    else {
        result.Clear();
    }
}

// As SetSound, for the activate loop sound extra.
void ExtraDataList::SetActivateLoopSound(const BSSoundHandle& afSound) {
    auto* pExtra = static_cast<ExtraActivateLoopSound*>(GetExtraData(EXTRA_ACTIVATE_LOOP_SOUND));
    if (pExtra) {
        if (afSound == BSSoundHandle()) {
            RemoveExtra(pExtra, true);
        }
        else {
            pExtra->pHandle = afSound;
        }
    }
    else if (afSound != BSSoundHandle()) {
        void* pMemory = ourMalloc(sizeof(ExtraActivateLoopSound));
        if (pMemory) {
            pExtra = ::new (pMemory) ExtraActivateLoopSound(afSound);
        }
        else {
            pExtra = nullptr;
        }
        AddExtra(pExtra);
    }
}

void ExtraDataList::ClearScriptLocals() {
    ExtraScript* pscriptextra = static_cast<ExtraScript*>(GetExtraData(13));
    if (pscriptextra) {
        pscriptextra->pScriptVars = nullptr;
    }
}

void ExtraDataList::AddToPlayerCrimeList(Crime* apCrime, float afTimeStamp) {
    ExtraPlayerCrimeList* pExtra = static_cast<ExtraPlayerCrimeList*>(GetExtraData(53));
    if (pExtra) {
        pExtra->pCrime->AddHead(apCrime);
    }
    else {
        ExtraPlayerCrimeList* pExtra_2;
        ExtraPlayerCrimeList* apMemory = static_cast<ExtraPlayerCrimeList*>(ourMalloc(sizeof(ExtraPlayerCrimeList)));
        if (apMemory) {
            pExtra_2 = ::new (apMemory) ExtraPlayerCrimeList(apCrime);
        }
        else {
            pExtra_2 = nullptr;
        }
        AddExtra(pExtra_2);
    }
}

void ExtraDataList::AddActivateParent(TESObjectREFR* apRef) {
    if (apRef) {
        ExtraActivateRef* ExtraData = static_cast<ExtraActivateRef*>(GetExtraData(83u));
        if (ExtraData) {
            if (!ExtraData->GetData(apRef)) {
                REF_ACTIVATE_DATA* v4;
                REF_ACTIVATE_DATA* apMemory = static_cast<REF_ACTIVATE_DATA*>(ourMalloc(sizeof(REF_ACTIVATE_DATA)));
                if (apMemory) {
                    v4 = ::new (apMemory) REF_ACTIVATE_DATA;
                }
                else {
                    v4 = nullptr;
                }
                REF_ACTIVATE_DATA* a2 = v4;
                v4->pActivateRef = apRef;
                ExtraData->ParentList.AddHead(a2);
            }
        }
        else {
            REF_ACTIVATE_DATA* v2;
            ExtraActivateRef* v3;
            ExtraActivateRef* v7 = static_cast<ExtraActivateRef*>(ourMalloc(sizeof(ExtraActivateRef)));
            if (v7) {
                v3 = ::new (v7) ExtraActivateRef;
            }
            else {
                v3 = nullptr;
            }
            ExtraData = v3;
            AddExtra(v3);
            REF_ACTIVATE_DATA* v6 = static_cast<REF_ACTIVATE_DATA*>(ourMalloc(sizeof(REF_ACTIVATE_DATA)));
            if (v6) {
                v2 = ::new (v6) REF_ACTIVATE_DATA;
            }
            else {
                v2 = nullptr;
            }
            REF_ACTIVATE_DATA* item = v2;
            v2->pActivateRef = apRef;
            ExtraData->ParentList.AddHead(item);
        }
    }
}

void ExtraDataList::SetActivateDelay(TESObjectREFR* apRef, float afVal) {
    if (apRef) {
        ExtraActivateRef* apExtra = static_cast<ExtraActivateRef*>(GetExtraData(83u));
        REF_ACTIVATE_DATA* a2 = nullptr;
        if (apExtra) {
            a2 = apExtra->GetData(apRef);
        }
        else {
            ExtraActivateRef* v4;
            ExtraActivateRef* apMemory = static_cast<ExtraActivateRef*>(ourMalloc(sizeof(ExtraActivateRef)));
            if (apMemory) {
                v4 = ::new (apMemory) ExtraActivateRef;
            }
            else {
                v4 = nullptr;
            }
            apExtra = v4;
            AddExtra(v4);
        }
        if (!a2) {
            REF_ACTIVATE_DATA* v3;
            REF_ACTIVATE_DATA* v6 = static_cast<REF_ACTIVATE_DATA*>(ourMalloc(sizeof(REF_ACTIVATE_DATA)));
            if (v6) {
                v3 = ::new (v6) REF_ACTIVATE_DATA;
            }
            else {
                v3 = nullptr;
            }
            a2 = v3;
            apExtra->ParentList.AddHead(a2);
            a2->pActivateRef = apRef;
        }
        a2->fActivateDelay = afVal;
    }
}

BSStringT<char> ExtraDataList::GetActivateTextOverride() {
    ExtraActivateRef* pextra = static_cast<ExtraActivateRef*>(GetExtraData(83));
    if (pextra) {
        return pextra->ActivateTextOverride;
    }
    return "";
}

void ExtraDataList::Revert_0042DAE0(BGSLoadFormBuffer* apLoadGameBuffer) {
    bool IsFormIDCreated;
    TESObjectREFR* apItem = apLoadGameBuffer->GetReference();
    if (apItem) {
        Actor* v16 = apLoadGameBuffer->GetActor();
        if (GetModelSwap()) {
            apItem->Set3D(nullptr, true);
        }
        if (GetExtraData(86)) {
            BGSDestructibleObjectForm::SetSelfDamage(apItem, 0);
        }
        TESObjectREFR* ItemDropper = GetItemDropper();
        if (ItemDropper) {
            ExtraDataList* Extra = ItemDropper->GetExtra();
            Extra->RemoveDroppedItem(apItem);
        }
        MapMarkerData* MapMarkerData_ = GetMapMarkerData();
        if (MapMarkerData_) {
            MapMarkerData_->ResetFlags();
        }
        if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN)) {
            SetActivateChildrenTimer(0.0f);
        }
        ScriptLocals* ScriptLocals_ = GetScriptLocals();
        if (ScriptLocals_) {
            ScriptLocals_->Revert(apLoadGameBuffer);
        }
    }
    if (apItem) {
        uint32_t CastingType = apItem->iFormID;
        IsFormIDCreated = pDataHandler->IsFormIDCreated(CastingType);
    }
    else {
        IsFormIDCreated = false;
    }
    for (uint32_t i = 0; i < 147; ++i) {
        char v11 = 0;
        if (xDataTypeToChangeFlag[i]) {
            int32_t afTimer = xDataTypeToChangeFlag[i];
            if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(afTimer)) {
                v11 = 1;
            }
        }
        if ((!IsFormIDCreated) && (v11) && ((xDataTypeToChangeFlag[i] & CHANGE_REFR_EXTRA_CREATED_ONLY) != 0)) {
            v11 = 0;
        }

        if (v11) {
            switch (i) {
                case 13u:
                case 44u:
                case 46u:
                case 73u:
                case 84u:
                case 95u:
                case 144u:
                case 145u: {
                    v11 = 0;
                    break;
                }
                default: {
                    break;
                }
            }
        }
        if (v11) {
            RemoveExtra(i);
        }
    }
}

MenuTopic* ExtraDataList::GetInfoGeneralTopic(TESObjectREFR* apSpeaker) {
    MenuTopic* pItem = nullptr;
    ExtraInfoGeneralTopic* pextra = static_cast<ExtraInfoGeneralTopic*>(GetExtraData(77));
    if (pextra) {
        pItem = pextra->pInfoGen;
        if ((pItem) && (pItem->GetResponseListEmpty())) {
            pItem->FillResponseList(apSpeaker);
        }
    }
    return pItem;
}

void ExtraDataList::RemoveExtraSayTopicInfoOnceADayExtra() {
    BSExtraData* pExtraData = GetExtraData(EXTRA_SAY_ONCE_A_DAY_TOPIC_INFO);
    if (pExtraData) {
        RemoveExtra(pExtraData, true);
    }
}

BSSoundHandle::BSSoundHandle(uint32_t aiID)
  : iSoundID(static_cast<int32_t>(aiID)), bAssumeSuccess(false), eState(ASSUMED_STATE_INITIALIZED) {}

uint32_t BSSoundHandle::GetID() const {
    return iSoundID;
}

// Releases one level of the lock and clears the owner when the count reaches 0. The owner is cleared with a plain
// store, not an interlocked compare-exchange.
void BSSpinLock::Unlock() {
    InitializationFlow();
    if (--uiLockCount == 0) {
        uiOwningThread = 0;
    }
}

void* ourRealloc(void* apPtr, uint32_t aSize) {
    MemoryManager* singleton = &MemoryManager::Instance();
    return singleton->Reallocate(apPtr, aSize);
}

const NiMatrix3 BSOcclusionPlane::QRotation() const {
    return kRotation;
}

void ActorCause::IncRefCount() {
    NiAtomicIncrement(iRefCount);
}

NiColorA BGSPrimitive::GetColor() {
    NiColorA kResult;
    kResult.r = Color.r;
    kResult.g = Color.g;
    kResult.b = Color.b;
    kResult.a = Color.a;
    return kResult;
}

NiPoint3 BGSPrimitive::GetRadii() {
    return Radii;
}

BGSPrimitive::PRIMITIVE_TYPE BGSPrimitive::GetType() {
    return eType;
}

RADIO_DATA::RADIO_DATA()
  : fRadius((float)0.0), eRangeType(RADIO_RANGE_EVERYWHERE), fStaticPct((float)0.0), pPositionRef(nullptr) {}

ExtraAmmo::ExtraAmmo() : BSExtraData(EXTRA_AMMO), pAmmo(nullptr), iCount(0) {}

CollisionData::CollisionData() : eLayer(L_ACTORZONE) {}

COL_LAYER CollisionData::QLayer() {
    return eLayer;
}

bool ExtraCollisionData::Compare(BSExtraData* apCompare) {
    ExtraCollisionData* pCompare = static_cast<ExtraCollisionData*>(apCompare);
    if (!pCollisionData && !pCompare->pCollisionData) {
        return false;
    }
    if (!pCollisionData || !pCompare->pCollisionData) {
        return true;
    }
    return pCollisionData->eLayer != pCompare->pCollisionData->eLayer;
}

ExtraCollisionData::~ExtraCollisionData() {
    ourFree(pCollisionData);
}

bool ExtraEnableStateParent::IsInverse() const {
    return (cFlags & 1) != 0;
}

void ExtraEnableStateParent::SetInverse(bool abVal) {
    if (abVal) {
        this->cFlags |= 1;
        uint8_t cFlags = this->cFlags;
    }
    else {
        this->cFlags &= ~1;
        uint8_t v3 = this->cFlags;
    }
    uint8_t v4 = this->cFlags;
}

ExtraGlobal::ExtraGlobal() : BSExtraData(EXTRA_GLOBAL), pGlobal(nullptr) {}

ExtraLitWaterRefs::~ExtraLitWaterRefs() {
    RefList.RemoveAll();
}

ExtraOwnership::ExtraOwnership() : BSExtraData(EXTRA_OWNERSHIP), pOwner(nullptr) {}

bool ExtraPatrolRefData::Compare(BSExtraData* apCompare) {
    if (!pPatrolData && !static_cast<ExtraPatrolRefData*>(apCompare)->pPatrolData) {
        return false;
    }
    if (pPatrolData && static_cast<ExtraPatrolRefData*>(apCompare)->pPatrolData) {
        return pPatrolData->Compare(static_cast<ExtraPatrolRefData*>(apCompare)->pPatrolData);
    }
    return true;
}

ExtraPatrolRefData::~ExtraPatrolRefData() {
    Clear();
}

void ExtraPatrolRefData::Clear() {
    if (pPatrolData) {
        delete pPatrolData;
    }
    pPatrolData = nullptr;
}

ExtraRank::ExtraRank() : BSExtraData(EXTRA_RANK), iRank(0) {}

ExtraReferencePointer::ExtraReferencePointer() : BSExtraData(EXTRA_REFERENCE_POINTER), pRef(nullptr) {}

ExtraReferencePointer::ExtraReferencePointer(TESObjectREFR* apRef) : BSExtraData(28u), pRef(apRef) {}

int32_t ExtraSpecialRenderFlags::GetFlags() {
    return m_flags;
}

void ExtraSpecialRenderFlags::SetFlags(int32_t aflags) {
    m_flags = aflags;
}

bool ExtraRoomRefData::Compare(BSExtraData* apCompare) {
    ExtraRoomRefData* pCompare = static_cast<ExtraRoomRefData*>(apCompare);
    if (!pData && !pCompare->pData) {
        return false;
    }
    if (!pData || !pCompare->pData) {
        return true;
    }
    if (pData->ucRoomCount != pCompare->pData->ucRoomCount) {
        return true;
    }
    BSSimpleList<TESObjectREFR*>* pList1 = &pData->kRoomList;
    BSSimpleList<TESObjectREFR*>* pList2 = &pCompare->pData->kRoomList;
    while (pList1 && pList2) {
        if (pList1->GetItem() != pList2->GetItem()) {
            return true;
        }
        pList1 = pList1->GetNext();
        pList2 = pList2->GetNext();
    }
    return pList1 || pList2;
}

void OcclusionPlaneLinkedRefData::Endian() {
    for (int32_t i = 0; i < 4; ++i) {
        ENDIAN<TESObjectREFR*>(pLinkedRefs[i], false);
    }
}

bool ExtraOcclusionPlaneRefData::Compare(BSExtraData* apCompare) {
    if (!pData && !static_cast<ExtraOcclusionPlaneRefData*>(apCompare)->pData) {
        return false;
    }
    if (!pData || !static_cast<ExtraOcclusionPlaneRefData*>(apCompare)->pData) {
        return true;
    }
    for (int32_t i = 0; i < 4; ++i) {
        if (pData->pLinkedRefs[i] != static_cast<ExtraOcclusionPlaneRefData*>(apCompare)->pData->pLinkedRefs[i]) {
            return true;
        }
    }
    return false;
}

ExtraOcclusionPlaneRefData::~ExtraOcclusionPlaneRefData() {
    Clear();
}

bool ExtraPortalRefData::Compare(BSExtraData* apCompare) {
    if (!pData && !static_cast<ExtraPortalRefData*>(apCompare)->pData) {
        return false;
    }
    if (!pData || !static_cast<ExtraPortalRefData*>(apCompare)->pData) {
        return true;
    }
    for (int32_t i = 0; i < 2; ++i) {
        if (pData->pLinkedRefs[i] != static_cast<ExtraPortalRefData*>(apCompare)->pData->pLinkedRefs[i]) {
            return true;
        }
    }
    return false;
}

ExtraPortalRefData::~ExtraPortalRefData() {
    Clear();
}

DismemberedLimb::DismemberedLimb()
  : cLimb(0), bLimbExploded(false), bObjectArrayIdentical(false), bLimbRemoved(false), ObjectArray() {}

ExtraActorCause::~ExtraActorCause() {}

ExtraHotKey::ExtraHotKey() : BSExtraData(EXTRA_HOT_KEY), chotkey(0) {}

ExtraHotKey::~ExtraHotKey() {}

ExtraModdingItem::~ExtraModdingItem() {}

void ExtraModdingItem::SetItemModding(bool bSet) {
    bIsModding = bSet;
}

ExtraScale::ExtraScale() : BSExtraData(EXTRA_SCALE), fScale((float)0.0) {}

ExtraSecuritronFace::ExtraSecuritronFace() : BSExtraData(143) {}

ExtraSecuritronFace::~ExtraSecuritronFace() {}

BSStringT<char> ExtraSecuritronFace::GetPersonality() {
    return msPersonality;
}

BSStringT<char> ExtraSecuritronFace::GetMood() {
    return msMood;
}

ExtraTimeLeft::ExtraTimeLeft() : BSExtraData(EXTRA_TIMELEFT), fTime((float)0.0) {}

ExtraTimeLeft::~ExtraTimeLeft() {}

ExtraTresPassPackage::ExtraTresPassPackage() : BSExtraData(EXTRA_TRESPASS_PACKAGE), pPack(nullptr) {}

NAVMESH_PORTAL::NAVMESH_PORTAL() : pNavMesh(nullptr), iTriIndex(0) {}

int32_t BSAtomicCompareExchange(volatile int32_t* apAddress, int32_t aiCompare, int32_t aiValue) {
    return InterlockedCompareExchange32(apAddress, aiValue, aiCompare);
}

// InterlockedCompareExchange takes a LONG, so the cast reads the same unsigned word as signed.
uint32_t BSAtomicCompareExchange(volatile uint32_t* apAddress, uint32_t aiCompare, uint32_t aiValue) {
    return static_cast<uint32_t>(InterlockedCompareExchange32(
        reinterpret_cast<volatile int32_t*>(apAddress), static_cast<int32_t>(aiValue), static_cast<int32_t>(aiCompare)
    ));
}

void* BSAtomicCompareExchange(void* volatile* apAddress, void* apCompare, void* apValue) {
    return InterlockedCompareExchangePtr(apAddress, apValue, apCompare);
}

NavMeshPtr::NavMeshPtr(const NavMeshPtr& arPtr) : NiPointer<NavMesh>(arPtr) {}

NavMeshPtr& NavMeshPtr::operator=(const NavMeshPtr& ptr) {
    return *static_cast<NavMeshPtr*>(SetPointer_(&ptr));
}

LoadFormHeader* TESSaveLoadGame::GetCurrentlyLoadingFormHeader() {
    return m_pCurrentlyLoadingFormHeader;
}

void* TESSaveLoadGame::GetBufferPointer() {
    return m_pBuffer;
}

Script* TESScriptableForm::GetFormScript() {
    return pFormScript;
}

// The shipped debug info places Actor::GetActorBase in this file.
TESActorBase* Actor::GetActorBase() {
    return static_cast<TESActorBase*>(GetObjectReference());
}

void MenuTopic::FillResponseList(TESObjectREFR* apSpeaker) {
    FillResponseList(pQuest, pTopic, pInfo, apSpeaker);
}

TESTopicInfo* MenuTopic::GetInfo() {
    return pInfo;
}

BGSChangeFlags BGSLoadFormBuffer::GetChangeFlags() {
    return Header.iChangeFlags;
}

BGSChangeFlags BGSLoadFormBuffer::GetOldChangeFlags() {
    return iOldChangeFlags;
}

void BGSLoadFormBuffer::SetForm(TESForm* apForm) {
    pForm = apForm;
}

void BGSLoadFormBuffer::SetOldChangeFlags(BGSChangeFlags aiChangeFlags) {
    iOldChangeFlags = aiChangeFlags;
}

// Whether the form was reconstructed (LOAD_FORM_RECONSTRUCTED_FORM).
bool BGSLoadFormBuffer::GetReconstructedForm() {
    return (iFlags & LOAD_FORM_RECONSTRUCTED_FORM) != 0;
}

BGSChangeFlags BGSSaveFormBuffer::GetChangeFlags() {
    return Header.iChangeFlags;
}

void BGSSaveFormBuffer::SetChangeFlags(BGSChangeFlags aiChangeFlags) {
    Header.iChangeFlags = aiChangeFlags;
}

bool BGSSaveLoadGame::GetLoadingMovedRefs() {
    return (GetThreadSpecificFlags() & 2u) != 0;
}

bool BGSSaveLoadGame::GetReconstructingForms() {
    return (GetThreadSpecificFlags() & 4u) != 0;
}

float NiCos(float fRadians) {
    return std::cos(fRadians);
}

float NiSin(float fRadians) {
    return std::sin(fRadians);
}

void* ourMalloc(uint32_t aSize) {
    MemoryManager* v1 = &MemoryManager::Instance();
    return v1->Allocate(aSize);
}

void ourFree(void* apPtr) {
    MemoryManager* v1 = &MemoryManager::Instance();
    v1->Deallocate(apPtr);
}
