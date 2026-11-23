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
                pLock = new LockData;
                pExtra = new ExtraLock(pLock);
                AddExtra(pExtra);
            }
            else {
                pLock = pExtra->pLock;
            }
            apFile->GetChunkData(pLock, 0x14);
            // The file's record is 0x14 bytes, the key's 4-byte form ID at 4.
            TESFile::WidenFormIDRecords(pLock, 1, 0x14, sizeof(LockData), {{4, offsetof(LockData, pKey)}});
            if (apFile->QEndian()) {
                pLock->Endian();
            }
            pLock->SetLocked(true);
            break;
        }
        case XRDO_ID: {
            RADIO_DATA kRadioData;
            apFile->GetChunkData(&kRadioData, 0x10);
            // The file's record is 0x10 bytes, the position reference's 4-byte form ID at 0xC.
            TESFile::WidenFormIDRecords(
                &kRadioData,
                1,
                0x10,
                sizeof(kRadioData),
                {{0xC, offsetof(RADIO_DATA, pPositionRef)}}
            );
            if (apFile->QEndian()) {
                kRadioData.Endian();
            }
            SetRadioData(&kRadioData);
            break;
        }
        case XTEL_ID: {
            DoorTeleportData* pTeleport = GetTeleport();
            if (!pTeleport) {
                pTeleport = new DoorTeleportData;
                SetTeleportPtr(pTeleport);
            }
            pTeleport->Load(apFile);
            break;
        }
        case XMRK_ID: {
            MapMarkerData* pMapMarker = GetMapMarkerData();
            if (!pMapMarker) {
                pMapMarker = new MapMarkerData;
                SetMapMarkerPtr(pMapMarker);
            }
            pMapMarker->Load(apFile);
            break;
        }
        case MMRK_ID: {
            AudioMarker* pAudioMarker = GetAudioMarkerData();
            if (!pAudioMarker) {
                pAudioMarker = new AudioMarker;
                SetAudioMarkerPtr(pAudioMarker);
            }
            pAudioMarker->Load(apFile);
            break;
        }
        case AMRK_ID: {
            AudioBuoyMarkerData* pAudioBuoyData = GetAudioBuoyMarkerData();
            if (!pAudioBuoyData) {
                pAudioBuoyData = new AudioBuoyMarkerData;
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
            TESRegionList* pRegionList = new TESRegionList(false);
            uint32_t* pRegionIDs = new uint32_t[uiCount];
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
            delete[] pRegionIDs;
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
            ourMemset(&kLocation, 0, sizeof(WORLD_LOCATION));
            apFile->GetChunkData(&kLocation, 0x14);
            // The file's record is 0x14 bytes, the location's 4-byte form ID first.
            TESFile::WidenFormIDRecords(
                &kLocation,
                1,
                0x14,
                sizeof(kLocation),
                {{0, offsetof(WORLD_LOCATION, pLocationForm)}}
            );
            if (apFile->QEndian()) {
                kLocation.Endian();
            }
            SetPackageStartLocation(
                static_cast<TESWorldSpace*>(kLocation.pLocationForm),
                nullptr,
                kLocation.locPt,
                kLocation.fZRot
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
                pExtra = new ExtraRagDollData;
                pExtra->pRagDollData = new RagDollData;
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
            // The file's record is 8 bytes, the parent's 4-byte form ID first.
            TESFile::WidenFormIDRecords(
                &kParentData,
                1,
                8,
                sizeof(kParentData),
                {{0, offsetof(REF_ENABLE_PARENT_DATA, pParent)}}
            );
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
                pExtra = new ExtraActivateRef;
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
                pExtra = new ExtraActivateRef;
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
                pExtra = new ExtraActivateRef;
                AddExtra(pExtra);
            }
            REF_ACTIVATE_DATA* pData = new REF_ACTIVATE_DATA;
            apFile->GetChunkData(pData, 8);
            // The file's record is 8 bytes, the parent's 4-byte form ID first.
            TESFile::WidenFormIDRecords(
                pData,
                1,
                8,
                sizeof(REF_ACTIVATE_DATA),
                {{0, offsetof(REF_ACTIVATE_DATA, pActivateRef)}}
            );
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
            };

            auto* pExtra = static_cast<ExtraActivateRef*>(GetExtraData(EXTRA_ACTIVATE_REF));
            if (!pExtra) {
                pExtra = new ExtraActivateRef;
                AddExtra(pExtra);
            }
            OLD_ACTIVATE_DATA kOldData;
            ourMemset(&kOldData, 0, sizeof(OLD_ACTIVATE_DATA));
            apFile->GetChunkData(&kOldData, 0xC);
            // The file's record is 0xC bytes, the parent's 4-byte form ID first.
            TESFile::WidenFormIDRecords(
                &kOldData,
                1,
                0xC,
                sizeof(kOldData),
                {{0, offsetof(OLD_ACTIVATE_DATA, pActivateRef)}}
            );
            if (apFile->QEndian()) {
                // Swaps the form ID slot as a raw 32-bit value.
                ENDIAN(reinterpret_cast<uint32_t&>(kOldData.pActivateRef), false);
                ENDIAN(kOldData.fActivateDelay, false);
            }
            REF_ACTIVATE_DATA* pData = new REF_ACTIVATE_DATA;
            pData->pActivateRef = kOldData.pActivateRef;
            pData->fActivateDelay = kOldData.fActivateDelay;
            pExtra->ParentList.AddHead(pData);
            pExtra->cActivateFlags = kOldData.cActivateFlags;
            break;
        }
        case XDCR_ID: {
            REF_DECAL_DATA kDecal;
            ourMemset(&kDecal, 0, sizeof(REF_DECAL_DATA));
            apFile->GetChunkData(&kDecal, 0x1C);
            // The file's record is 0x1C bytes, the reference's 4-byte form ID first.
            TESFile::WidenFormIDRecords(&kDecal, 1, 0x1C, sizeof(kDecal), {{0, offsetof(REF_DECAL_DATA, pDecalRef)}});
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
            MultiBoundMarkerData* pData = new MultiBoundMarkerData;
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
            uint32_t uiSize = apFile->GetChunkSize();
            auto* pBuffer = static_cast<char*>(ourMalloc(uiSize));
            apFile->GetChunkData(pBuffer, uiSize);
            kPortal.Load(apOwner, apFile, pBuffer);
            ourFree(pBuffer);
            break;
        }
        case XNDP_ID: {
            NAVMESH_PORTAL kHandle;
            apFile->GetChunkData(&kHandle, 8);
            // The file's record is 8 bytes, the navmesh's 4-byte form ID first.
            TESFile::WidenFormIDRecords(&kHandle, 1, 8, sizeof(kHandle), {{0, offsetof(NAVMESH_PORTAL, pNavMesh)}});
            if (apFile->QEndian()) {
                kHandle.Endian();
            }
            NAVMESH_PORTAL kArgument(kHandle);
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
            ourMemset(&kReflected, 0, sizeof(REF_REFLECTED_DATA));
            apFile->GetChunkData(&kReflected, 8);
            // The file's record is 8 bytes, the reference's 4-byte form ID first.
            TESFile::WidenFormIDRecords(
                &kReflected,
                1,
                8,
                sizeof(kReflected),
                {{0, offsetof(REF_REFLECTED_DATA, pRef)}}
            );
            if (apFile->QEndian()) {
                // Swaps the form ID slot as a raw 32-bit value.
                ENDIAN(reinterpret_cast<uint32_t&>(kReflected.pRef), false);
                ENDIAN(kReflected.iEffectFlags, false);
            }
            if (kReflected.iEffectFlags & REF_REFLECTED_DATA::FLAG_REFLECTION) {
                SetReflectedByRef(kReflected.pRef, true);
            }
            if (kReflected.iEffectFlags & REF_REFLECTED_DATA::FLAG_REFRACTION) {
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
            BGSPrimitive* pPrimitive = BGSPrimitive::CreateAbstract(
                pPrimitiveData->eType,
                pPrimitiveData->aRadii,
                pPrimitiveData->aColor
            );
            if (pPrimitive) {
                AddPrimitive(pPrimitive);
            }
            break;
        }
        case XOCP_ID: {
            OCCLUSION_PLANE_DATA kPlaneData;
            ourMemset(&kPlaneData, 0, sizeof(OCCLUSION_PLANE_DATA));
            apFile->GetChunkData(&kPlaneData, 0x24);
            if (apFile->QEndian()) {
                kPlaneData.Endian();
            }
            BSOcclusionPlane* pPlane = new BSOcclusionPlane;
            pPlane->SetHalfExtents(kPlaneData.rHalfExtents);
            pPlane->SetCenter(kPlaneData.rCenter);
            NiMatrix3 kRotation;
            kRotation.MakeRotation(kPlaneData.fAngle, kPlaneData.rAxis.x, kPlaneData.rAxis.y, kPlaneData.rAxis.z);
            pPlane->SetRotation(kRotation);
            SetOcclusionPlane(pPlane);
            break;
        }
        case XPTL_ID: {
            // The portal's shape is read and dropped.
            OCCLUSION_PLANE_DATA kPortalData;
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
            kPatrolData.fTimeAtRef = 0.0f;
            apFile->GetChunkData(&kPatrolData, 4);
            if (apFile->QEndian()) {
                kPatrolData.Endian();
            }
            PatrolRefData* pPatrolRefData = new PatrolRefData(kPatrolData);
            SetPatrolRefData(pPatrolRefData);
            break;
        }
        case XORD_ID: {
            OcclusionPlaneLinkedRefData* pLinkedRefData = new OcclusionPlaneLinkedRefData;
            apFile->GetChunkData(pLinkedRefData, 0x10);
            // The file's record is four 4-byte form IDs.
            TESFile::WidenFormIDRecords(pLinkedRefData->pLinkedRefs, 4, 4, sizeof(TESObjectREFR*), {{0, 0}});
            if (apFile->QEndian()) {
                pLinkedRefData->Endian();
            }
            SetOcclusionPlaneLinkedRefData(pLinkedRefData);
            break;
        }
        case XPOD_ID: {
            PortalLinkedRefData* pLinkedRefData = new PortalLinkedRefData;
            apFile->GetChunkData(pLinkedRefData, 8);
            // The file's record is two 4-byte form IDs.
            TESFile::WidenFormIDRecords(pLinkedRefData->pLinkedRefs, 2, 4, sizeof(TESObjectREFR*), {{0, 0}});
            if (apFile->QEndian()) {
                pLinkedRefData->Endian();
            }
            SetPortalLinkedRefData(pLinkedRefData);
            break;
        }
        case XRMR_ID: {
            RoomRefData* pRoomRefData = new RoomRefData;
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
            CollisionData* pCollisionData = new CollisionData;
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
                BSExtraData* pNew = new ExtraSpecialRenderFlags;
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
                BSExtraData* pNew = new ExtraSpecialRenderFlags;
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

// Every field is deliberately left unset.
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

// Every field is deliberately left unset.
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
    int32_t extraType = GetExtraType();
    return extraType != apCompare->GetExtraType();
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
        ++iThreadDirty;
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
// the list compare; the list is deliberately not compared again.
BSExtraData* BaseExtraList::GetLastExtra(BaseExtraList* apList, int32_t aeType) {
    BSExtraData* pExtra = nullptr;
    if (apList == pLastExtraList) {
        CheckDirty();
        if (aeType >= 0 && aeType < EXTRA_DATATYPECOUNT) {
            pExtra = ppLastExtraData[aeType];
        }
    }
    return pExtra;
}

// Switching lists drops the whole cache. The type indexes the cache unchecked.
void BaseExtraList::SetLastExtra(BaseExtraList* apList, BSExtraData* apExtra) {
    if (apList && apExtra) {
        if (apList != pLastExtraList) {
            ourMemset(ppLastExtraData, 0, sizeof(BSExtraData*) * EXTRA_DATATYPECOUNT);
            pLastExtraList = apList;
        }
        ppLastExtraData[apExtra->GetExtraType()] = apExtra;
    }
}

// Invalidates every other thread's cache (iDirty) but keeps this thread's in step, then forgets this list's item
// of aeType.
void BaseExtraList::ClearLastExtra(BaseExtraList* apList, int32_t aeType) {
    ++iDirty;
    ++iThreadDirty;
    if (apList == pLastExtraList && aeType >= 0 && aeType < EXTRA_DATATYPECOUNT) {
        ppLastExtraData[aeType] = nullptr;
    }
}

void BaseExtraList::ClearLastExtraAll(BaseExtraList* apList) {
    ++iDirty;
    ++iThreadDirty;
    if (apList == pLastExtraList) {
        ourMemset(ppLastExtraData, 0, sizeof(BSExtraData*) * EXTRA_DATATYPECOUNT);
        pLastExtraList = nullptr;
    }
}

void BaseExtraList::CheckDirty() {
    if (iThreadDirty != iDirty) {
        pLastExtraList = nullptr;
        ourMemset(ppLastExtraData, 0, sizeof(BSExtraData*) * EXTRA_DATATYPECOUNT);
        iThreadDirty = iDirty;
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

ExtraDataList::ExtraDataList() {}

ExtraDataList::~ExtraDataList() {}

void ExtraDataList::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer, TESForm* apBaseObject) {
    TESObjectREFR* pref = apLoadGameBuffer->GetReference();
    Actor* pactor = apLoadGameBuffer->GetActor();
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[28])) {
        ExtraReferencePointer* pextraReferencePointer = static_cast<ExtraReferencePointer*>(GetExtraData(28));
        if (pextraReferencePointer) {
            TESObjectREFR* pReference = nullptr;
            if (pextraReferencePointer->pRef) {
                TESForm* pForm = TESForm::GetFormByNumericID(
                    static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraReferencePointer->pRef))
                );
                pReference = dynamic_cast<TESObjectREFR*>(pForm);
            }
            pextraReferencePointer->pRef = pReference;
        }
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[13])) {
        ExtraScript* pscriptExtra = static_cast<ExtraScript*>(GetExtraData(13));
        if (pscriptExtra && apBaseObject) {
            TESScriptableForm* pscriptableForm = nullptr;
            if (pactor) {
                pscriptableForm = static_cast<TESScriptableForm*>(pactor->GetActorBase());
            }
            else {
                pscriptableForm = dynamic_cast<TESScriptableForm*>(apBaseObject);
            }
            if (!pscriptableForm || !pscriptExtra->pScript || pscriptableForm->pFormScript != pscriptExtra->pScript) {
                RemoveExtra(13);
            }
        }
    }
    if (pref) {
        if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[108])) {
            ExtraOpenCloseActivateRef* pextraActivateRef = static_cast<ExtraOpenCloseActivateRef*>(GetExtraData(108));
            if (pextraActivateRef) {
                TESObjectREFR* pActivateRef = nullptr;
                if (pextraActivateRef->pActivateRef) {
                    TESForm* pForm = TESForm::GetFormByNumericID(
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraActivateRef->pActivateRef))
                    );
                    pActivateRef = dynamic_cast<TESObjectREFR*>(pForm);
                }
                pextraActivateRef->pActivateRef = pActivateRef;
                if (!pextraActivateRef->pActivateRef) {
                    RemoveExtra(108);
                }
            }
        }
        if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[50])) {
            NonActorMagicCaster* pextraNonActorMagicCaster = static_cast<NonActorMagicCaster*>(GetExtraData(50));
            if (pextraNonActorMagicCaster) {
                MagicItem* pCurrentSpell = nullptr;
                if (pextraNonActorMagicCaster->pCurrentSpell) {
                    pCurrentSpell = MagicItem::GetMagicItemByNumericID(
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraNonActorMagicCaster->pCurrentSpell))
                    );
                }
                pextraNonActorMagicCaster->pCurrentSpell = pCurrentSpell;
                MagicTarget* pDesiredTarget = nullptr;
                if (pextraNonActorMagicCaster->pDesiredTarget) {
                    pDesiredTarget = MagicTarget::GetMagicTargetByNumericID(
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraNonActorMagicCaster->pDesiredTarget))
                    );
                }
                pextraNonActorMagicCaster->pDesiredTarget = pDesiredTarget;
                TESObjectREFR* pCasterObject = nullptr;
                if (pextraNonActorMagicCaster->pCasterObject) {
                    TESForm* pForm = TESForm::GetFormByNumericID(
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraNonActorMagicCaster->pCasterObject))
                    );
                    pCasterObject = dynamic_cast<TESObjectREFR*>(pForm);
                }
                pextraNonActorMagicCaster->pCasterObject = pCasterObject;
            }
        }
        if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[51])) {
            NonActorMagicTarget* pextraNonActorMagicTarget = static_cast<NonActorMagicTarget*>(GetExtraData(51));
            if (pextraNonActorMagicTarget) {
                TESObjectREFR* pTargetObject = nullptr;
                if (pextraNonActorMagicTarget->pTargetObject) {
                    TESForm* pForm = TESForm::GetFormByNumericID(
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraNonActorMagicTarget->pTargetObject))
                    );
                    pTargetObject = dynamic_cast<TESObjectREFR*>(pForm);
                }
                pextraNonActorMagicTarget->pTargetObject = pTargetObject;
                ActiveEffect::InitLoadActiveEffectList(apLoadGameBuffer, &pextraNonActorMagicTarget->lActiveEffectList);
            }
        }
        if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[57])) {
            ExtraItemDropper* pextraItemDropper = static_cast<ExtraItemDropper*>(GetExtraData(57));
            if (pextraItemDropper) {
                TESObjectREFR* pDropper = nullptr;
                if (pextraItemDropper->pDropper) {
                    TESForm* pForm = TESForm::GetFormByNumericID(
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraItemDropper->pDropper))
                    );
                    pDropper = dynamic_cast<TESObjectREFR*>(pForm);
                }
                pextraItemDropper->pDropper = pDropper;
                if (pextraItemDropper->pDropper) {
                    pextraItemDropper->pDropper->m_Extra.AddDroppedItem(pref);
                }
                else {
                    RemoveExtra(57);
                }
            }
        }
        if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[85])) {
            ExtraTalkingActor* pextraTalkingActor = static_cast<ExtraTalkingActor*>(GetExtraData(85));
            if (pextraTalkingActor) {
                MobileObject* pTalkObject = nullptr;
                if (pextraTalkingActor->pTalkObject) {
                    TESForm* pForm = TESForm::GetFormByNumericID(
                        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraTalkingActor->pTalkObject))
                    );
                    pTalkObject = dynamic_cast<MobileObject*>(pForm);
                }
                pextraTalkingActor->pTalkObject = pTalkObject;
            }
        }
        if (pactor) {
            if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[25])) {
                ExtraPackage* pextraPackage = static_cast<ExtraPackage*>(GetExtraData(25));
                if (pextraPackage) {
                    TESPackage* pPack = nullptr;
                    if (pextraPackage->pPack) {
                        TESForm* pForm = TESForm::GetFormByNumericID(
                            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraPackage->pPack))
                        );
                        pPack = dynamic_cast<TESPackage*>(pForm);
                    }
                    pextraPackage->pPack = pPack;
                    TESObjectREFR* pTarg = nullptr;
                    if (pextraPackage->pTarg) {
                        TESForm* pForm = TESForm::GetFormByNumericID(
                            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraPackage->pTarg))
                        );
                        pTarg = dynamic_cast<TESObjectREFR*>(pForm);
                    }
                    pextraPackage->pTarg = pTarg;
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
                            pForm = dynamic_cast<Actor*>(
                                TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm)))
                            );
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
                    if (apLoadGameBuffer->GetVersion() < 14u) {
                        RemoveFollower(pPlayer);
                    }
                }
            }
            if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[112])) {
                ExtraPackageData* pextraPackageData = static_cast<ExtraPackageData*>(GetExtraData(112));
                if (pextraPackageData && pextraPackageData->pActorPackageData) {
                    GetPackageExtra();
                    pextraPackageData->pActorPackageData->InitLoadGame(apLoadGameBuffer);
                }
            }
            if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[60])) {
                ExtraMerchantContainer* pextraMerchantContainer = static_cast<ExtraMerchantContainer*>(
                    GetExtraData(60)
                );
                if (pextraMerchantContainer) {
                    TESObjectREFR* pContainer = nullptr;
                    if (pextraMerchantContainer->pContainer) {
                        TESForm* pForm = TESForm::GetFormByNumericID(
                            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraMerchantContainer->pContainer))
                        );
                        pContainer = dynamic_cast<TESObjectREFR*>(pForm);
                    }
                    pextraMerchantContainer->pContainer = pContainer;
                }
            }
            if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[26])) {
                ExtraTresPassPackage* pextraTresPassPackage = static_cast<ExtraTresPassPackage*>(GetExtraData(26));
                if (pextraTresPassPackage && pextraTresPassPackage->pPack) {
                    pextraTresPassPackage->pPack->InitLoadGame(apLoadGameBuffer);
                }
            }
            if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[70])) {
                ExtraHeadingTarget* pextraHeadingTarget = static_cast<ExtraHeadingTarget*>(GetExtraData(70));
                if (pextraHeadingTarget) {
                    TESObjectREFR* pHeadingTarget = nullptr;
                    if (pextraHeadingTarget->pHeadingTarget) {
                        TESForm* pForm = TESForm::GetFormByNumericID(
                            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraHeadingTarget->pHeadingTarget))
                        );
                        pHeadingTarget = dynamic_cast<TESObjectREFR*>(pForm);
                    }
                    pextraHeadingTarget->pHeadingTarget = pHeadingTarget;
                }
            }
            if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[95])) {
                ExtraDismemberedLimbs* pextraDismemberedLimbs = static_cast<ExtraDismemberedLimbs*>(GetExtraData(95));
                if (pextraDismemberedLimbs) {
                    TESForm* pDeathObject = nullptr;
                    if (pextraDismemberedLimbs->pDeathObject) {
                        pDeathObject = TESForm::GetFormByNumericID(
                            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraDismemberedLimbs->pDeathObject))
                        );
                    }
                    pextraDismemberedLimbs->pDeathObject = pDeathObject;
                }
            }
            if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[137])) {
                ExtraAshPileRef* pextraAshPileRef = static_cast<ExtraAshPileRef*>(GetExtraData(137));
                if (pextraAshPileRef) {
                    TESObjectREFR* pAshPileRef = nullptr;
                    if (pextraAshPileRef->pAshPileRef) {
                        TESForm* pForm = TESForm::GetFormByNumericID(
                            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraAshPileRef->pAshPileRef))
                        );
                        pAshPileRef = dynamic_cast<TESObjectREFR*>(pForm);
                    }
                    pextraAshPileRef->pAshPileRef = pAshPileRef;
                    if (pextraAshPileRef->pAshPileRef) {
                        pextraAshPileRef->pAshPileRef->m_Extra.SetAshPileRef(pactor);
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
        else if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[43])) {
            ExtraTeleport* pextraTeleport = static_cast<ExtraTeleport*>(GetExtraData(43));
            if (pextraTeleport) {
                pextraTeleport->pTeleportData->InitLoadGame(apLoadGameBuffer);
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
        if (xDataTypeToChangeFlag[cextraType] &&
            apSaveGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[cextraType])) {
            bsaveExtra = true;
        }

        // Extras with a created-only change flag are saved only for created forms.
        if (!bcreated && bsaveExtra && (xDataTypeToChangeFlag[cextraType] & CHANGE_REFR_EXTRA_CREATED_ONLY) != 0) {
            bsaveExtra = false;
        }
        if (!pactor && cextraType == EXTRA_ASHPILE_REF) {
            bsaveExtra = false;
        }
        if (bsaveExtra) {
            apSaveGameBuffer->SaveDataEndian(&cextraType, sizeof(cextraType), 0);
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
                    apSaveGameBuffer
                        ->SaveDataEndian(&pextraStartLoc->worldLoc.locPt, sizeof(pextraStartLoc->worldLoc.locPt), 0);
                    apSaveGameBuffer
                        ->SaveDataEndian(&pextraStartLoc->worldLoc.fZRot, sizeof(pextraStartLoc->worldLoc.fZRot), 0);
                    break;
                }
                case EXTRA_PACKAGE: {
                    auto* pextraPackage = static_cast<ExtraPackage*>(pExtra);
                    apSaveGameBuffer->SaveFormID(pextraPackage->pPack, 0);
                    apSaveGameBuffer->SaveFormID(pextraPackage->pTarg, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraPackage->iindex, sizeof(pextraPackage->iindex), 0);
                    apSaveGameBuffer
                        ->SaveDataEndian(&pextraPackage->bActionComplete, sizeof(pextraPackage->bActionComplete), 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraPackage->bActivated, sizeof(pextraPackage->bActivated), 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraPackage->bDoneOnce, sizeof(pextraPackage->bDoneOnce), 0);
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
                            apSaveGameBuffer->SaveDataEndian(&prunOncePackage->cday, sizeof(prunOncePackage->cday), 0);
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
                    apSaveGameBuffer->SaveDataEndian(
                        &static_cast<ExtraLevCreaModifier*>(pExtra)->eModifier,
                        sizeof(static_cast<ExtraLevCreaModifier*>(pExtra)->eModifier),
                        0
                    );
                    break;
                case EXTRA_OWNERSHIP:
                    apSaveGameBuffer->SaveFormID(static_cast<ExtraOwnership*>(pExtra)->pOwner, 0);
                    break;
                case EXTRA_GLOBAL: apSaveGameBuffer->SaveFormID(static_cast<ExtraGlobal*>(pExtra)->pGlobal, 0); break;
                case EXTRA_RANK:
                    apSaveGameBuffer->SaveDataEndian(
                        &static_cast<ExtraRank*>(pExtra)->iRank,
                        sizeof(static_cast<ExtraRank*>(pExtra)->iRank),
                        0
                    );
                    break;
                case EXTRA_COUNT:
                    apSaveGameBuffer->SaveDataEndian(
                        &static_cast<ExtraCount*>(pExtra)->iCount,
                        sizeof(static_cast<ExtraCount*>(pExtra)->iCount),
                        0
                    );
                    break;
                case EXTRA_HEALTH:
                    apSaveGameBuffer->SaveDataEndian(
                        &static_cast<ExtraHealth*>(pExtra)->fHealth,
                        sizeof(static_cast<ExtraHealth*>(pExtra)->fHealth),
                        0
                    );
                    break;
                case EXTRA_USES:
                    apSaveGameBuffer->SaveDataEndian(
                        &static_cast<ExtraUses*>(pExtra)->cUses,
                        sizeof(static_cast<ExtraUses*>(pExtra)->cUses),
                        0
                    );
                    break;
                case EXTRA_TIMELEFT:
                    apSaveGameBuffer->SaveDataEndian(
                        &static_cast<ExtraTimeLeft*>(pExtra)->fTime,
                        sizeof(static_cast<ExtraTimeLeft*>(pExtra)->fTime),
                        0
                    );
                    break;
                case EXTRA_CHARGE:
                    apSaveGameBuffer->SaveDataEndian(
                        &static_cast<ExtraCharge*>(pExtra)->fCharge,
                        sizeof(static_cast<ExtraCharge*>(pExtra)->fCharge),
                        0
                    );
                    break;
                case EXTRA_LOCK: {
                    auto* pextraLock = static_cast<ExtraLock*>(pExtra);
                    apSaveGameBuffer
                        ->SaveDataEndian(&pextraLock->pLock->cLockLevel, sizeof(pextraLock->pLock->cLockLevel), 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraLock->pLock->cFlags, sizeof(pextraLock->pLock->cFlags), 0);
                    apSaveGameBuffer->SaveFormID(pextraLock->pLock->pKey, 0);
                    apSaveGameBuffer
                        ->SaveDataEndian(&pextraLock->pLock->iNumTries, sizeof(pextraLock->pLock->iNumTries), 0);
                    apSaveGameBuffer->SaveDataEndian(
                        &pextraLock->pLock->iTimesUnlocked,
                        sizeof(pextraLock->pLock->iTimesUnlocked),
                        0
                    );
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
                    apSaveGameBuffer->SaveDataEndian(&cFlags, sizeof(cFlags), 0);
                    break;
                }
                case EXTRA_LEVELEDCREATURE: {
                    auto* pleveledCreature = static_cast<ExtraLeveledCreature*>(pExtra);
                    apSaveGameBuffer->SaveFormID(pleveledCreature->pOriginalBase, 0);
                    apSaveGameBuffer->SaveFormID(pleveledCreature->pTemplate, 0);
                    TESActorBase* pactorBase = pactor->GetActorBase();
                    int32_t iactorBaseChangeFlags = static_cast<int32_t>(pactorBase->iChangeFlags);
                    apSaveGameBuffer->SaveDataEndian(&iactorBaseChangeFlags, sizeof(iactorBaseChangeFlags), 0);
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
                    apSaveGameBuffer->SaveDataEndian(&pextraLeveledItem->iIndex, sizeof(pextraLeveledItem->iIndex), 0);
                    apSaveGameBuffer
                        ->SaveDataEndian(&pextraLeveledItem->bdefault, sizeof(pextraLeveledItem->bdefault), 0);
                    break;
                }
                case EXTRA_SCALE:
                    apSaveGameBuffer->SaveDataEndian(
                        &static_cast<ExtraScale*>(pExtra)->fScale,
                        sizeof(static_cast<ExtraScale*>(pExtra)->fScale),
                        0
                    );
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
                            apSaveGameBuffer->SaveDataEndian(&iCrimeNumber, sizeof(iCrimeNumber), 0);
                            apSaveGameBuffer->SaveDataEndian(&ecrimeType, sizeof(ecrimeType), 0);
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
                        static_cast<ExtraStartingWorldOrCell*>(pExtra)->pStartingWorldOrCell,
                        0
                    );
                    break;
                case EXTRA_HOT_KEY:
                    apSaveGameBuffer->SaveDataEndian(
                        &static_cast<ExtraHotKey*>(pExtra)->chotkey,
                        sizeof(static_cast<ExtraHotKey*>(pExtra)->chotkey),
                        0
                    );
                    break;
                case EXTRA_INFO_GENERAL_TOPIC:
                    static_cast<ExtraInfoGeneralTopic*>(pExtra)->pInfoGen->SaveGame_0(apSaveGameBuffer);
                    break;
                case EXTRA_NO_RUMORS:
                    apSaveGameBuffer->SaveDataEndian(
                        &static_cast<ExtraHasNoRumors*>(pExtra)->bNoRumors,
                        sizeof(static_cast<ExtraHasNoRumors*>(pExtra)->bNoRumors),
                        0
                    );
                    break;
                case EXTRA_TERMINALSTATE: {
                    auto* pextraTerminalState = static_cast<ExtraTerminalState*>(pExtra);
                    apSaveGameBuffer
                        ->SaveDataEndian(&pextraTerminalState->cState, sizeof(pextraTerminalState->cState), 0);
                    apSaveGameBuffer->SaveDataEndian(
                        &pextraTerminalState->cLockOverride,
                        sizeof(pextraTerminalState->cLockOverride),
                        0
                    );
                    break;
                }
                case EXTRA_ACTIVATE_REF_CHILDREN:
                    apSaveGameBuffer->SaveDataEndian(
                        &static_cast<ExtraActivateRefChildren*>(pExtra)->fActivateChildrenTimer,
                        sizeof(static_cast<ExtraActivateRefChildren*>(pExtra)->fActivateChildrenTimer),
                        0
                    );
                    break;
                case EXTRA_TALKING_ACTOR:
                    apSaveGameBuffer->SaveFormID(static_cast<ExtraTalkingActor*>(pExtra)->pTalkObject, 0);
                    break;
                case EXTRA_OBJECT_HEALTH:
                    apSaveGameBuffer->SaveDataEndian(
                        &static_cast<ExtraObjectHealth*>(pExtra)->fHealth,
                        sizeof(static_cast<ExtraObjectHealth*>(pExtra)->fHealth),
                        0
                    );
                    break;
                case EXTRA_MODEL_SWAP: {
                    auto* pextraModelSwap = static_cast<ExtraModelSwap*>(pExtra);
                    apSaveGameBuffer->SaveFormID(pextraModelSwap->pModelSwapForm, 0);
                    int32_t ModelSwapIndex = BGSDestructibleObjectForm::GetModelSwapIndex(
                        pextraModelSwap->pModelSwapForm,
                        pextraModelSwap->pModelSwap
                    );
                    apSaveGameBuffer->SaveDataEndian(&ModelSwapIndex, sizeof(ModelSwapIndex), 0);
                    break;
                }
                case EXTRA_RADIUS:
                    apSaveGameBuffer->SaveDataEndian(
                        &static_cast<ExtraRadius*>(pExtra)->fRadius,
                        sizeof(static_cast<ExtraRadius*>(pExtra)->fRadius),
                        0
                    );
                    break;
                case EXTRA_RADIATION:
                    apSaveGameBuffer->SaveDataEndian(
                        &static_cast<ExtraRadiation*>(pExtra)->fRadiation,
                        sizeof(static_cast<ExtraRadiation*>(pExtra)->fRadiation),
                        0
                    );
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
                            apSaveGameBuffer->SaveDataEndian(&pfactionRank->cRank, sizeof(pfactionRank->cRank), 0);
                            ++isaveGameCount;
                        }
                    }
                    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
                    break;
                }
                case EXTRA_DISMEMBERED_LIMBS: {
                    auto* pextraDismemberedLimbs = static_cast<ExtraDismemberedLimbs*>(pExtra);
                    apSaveGameBuffer
                        ->SaveDataEndian(&pextraDismemberedLimbs->sLimbs, sizeof(pextraDismemberedLimbs->sLimbs), 0);
                    apSaveGameBuffer->SaveDataEndian(
                        &pextraDismemberedLimbs->eCauseofDeath,
                        sizeof(pextraDismemberedLimbs->eCauseofDeath),
                        0
                    );
                    apSaveGameBuffer->SaveDataEndian(
                        &pextraDismemberedLimbs->eLastHitLimb,
                        sizeof(pextraDismemberedLimbs->eLastHitLimb),
                        0
                    );
                    apSaveGameBuffer
                        ->SaveDataEndian(&pextraDismemberedLimbs->bEaten, sizeof(pextraDismemberedLimbs->bEaten), 0);
                    apSaveGameBuffer->SaveFormID(pextraDismemberedLimbs->pDeathObject, 0);
                    BSSimpleArray<DismemberedLimb*>* pLimbs = &pextraDismemberedLimbs->DismemberedLimbs;
                    uint32_t isaveGameCount = pLimbs ? pLimbs->GetSize() : 0;
                    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
                    for (uint32_t j = 0; j < isaveGameCount; ++j) {
                        DismemberedLimb** ppLimb = &pextraDismemberedLimbs->DismemberedLimbs[j];
                        apSaveGameBuffer->SaveDataEndian(&(*ppLimb)->cLimb, sizeof((*ppLimb)->cLimb), 0);
                        apSaveGameBuffer
                            ->SaveDataEndian(&(*ppLimb)->bLimbExploded, sizeof((*ppLimb)->bLimbExploded), 0);
                        apSaveGameBuffer->SaveDataEndian(
                            &(*ppLimb)->bObjectArrayIdentical,
                            sizeof((*ppLimb)->bObjectArrayIdentical),
                            0
                        );
                        apSaveGameBuffer->SaveDataEndian(&(*ppLimb)->bLimbRemoved, sizeof((*ppLimb)->bLimbRemoved), 0);
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
                    apSaveGameBuffer->SaveDataEndian(&iactorCauseID, sizeof(iactorCauseID), 0);
                    break;
                }
                case EXTRA_OPENCLOSEACTIVATE_REF:
                    apSaveGameBuffer->SaveFormID(static_cast<ExtraOpenCloseActivateRef*>(pExtra)->pActivateRef, 0);
                    break;
                case EXTRA_AMMO: {
                    auto* pextraAmmo = static_cast<ExtraAmmo*>(pExtra);
                    apSaveGameBuffer->SaveFormID(pextraAmmo->pAmmo, 0);
                    apSaveGameBuffer->SaveDataEndian(&pextraAmmo->iCount, sizeof(pextraAmmo->iCount), 0);
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
                    apSaveGameBuffer->SaveDataEndian(&cpackDataType, sizeof(cpackDataType), 0);
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
                            apSaveGameBuffer->SaveDataEndian(
                                &psayOnceTopicInfo->iDateStamp,
                                sizeof(psayOnceTopicInfo->iDateStamp),
                                0
                            );
                            apSaveGameBuffer->SaveDataEndian(
                                &psayOnceTopicInfo->fHourStamp,
                                sizeof(psayOnceTopicInfo->fHourStamp),
                                0
                            );
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
                    apSaveGameBuffer->SaveDataEndian(
                        &pextraSayToTopicInfo->bVoiceDone,
                        sizeof(pextraSayToTopicInfo->bVoiceDone),
                        0
                    );
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
                    apSaveGameBuffer
                        ->SaveDataEndian(&pextraCrumbs->LeaderLocation, sizeof(pextraCrumbs->LeaderLocation), 0);
                    apSaveGameBuffer->SaveFormID(pextraCrumbs->LeaderNavMeshID, 0);
                    apSaveGameBuffer
                        ->SaveDataEndian(&pextraCrumbs->eLeaderState, sizeof(pextraCrumbs->eLeaderState), 0);
                    uint32_t isaveGameCount = 0;
                    uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
                    for (BSSimpleList<ExtraFollowerSwimBreadcrumb*>* plist = &pextraCrumbs->CrumbList; plist;
                         plist = plist->m_pkNext) {
                        ExtraFollowerSwimBreadcrumb* pcrumb = plist->m_item;
                        if (pcrumb) {
                            apSaveGameBuffer->SaveDataEndian(&pcrumb->StartLocation, sizeof(pcrumb->StartLocation), 0);
                            apSaveGameBuffer->SaveFormID(pcrumb->StartNavMeshID, 0);
                            apSaveGameBuffer->SaveDataEndian(&pcrumb->EndLocation, sizeof(pcrumb->EndLocation), 0);
                            apSaveGameBuffer->SaveFormID(pcrumb->EndNavMeshID, 0);
                            apSaveGameBuffer
                                ->SaveDataEndian(&pcrumb->bEnteringWater, sizeof(pcrumb->bEnteringWater), 0);
                            ++isaveGameCount;
                        }
                    }
                    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
                    break;
                }
                case EXTRA_WEAPON_MOD_SLOTS:
                    apSaveGameBuffer->SaveDataEndian(
                        &static_cast<ExtraWeaponModFlags*>(pExtra)->cWeaponModsActive,
                        sizeof(static_cast<ExtraWeaponModFlags*>(pExtra)->cWeaponModsActive),
                        0
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
                    apSaveGameBuffer->SaveDataEndian(&uiFlags, sizeof(uiFlags), 0);
                    apSaveGameBuffer->SaveDataEndian(
                        &pextraRenderFlags->m_fNodeMaxFadeDistance,
                        sizeof(pextraRenderFlags->m_fNodeMaxFadeDistance),
                        0
                    );
                    break;
                }
                default:
                    SaveGameWarning(
                        "SAVELOAD: Unknown extra data type %i found in ExtraDataList::SaveGame",
                        cextraType
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
            if (pDataHandler->IsFormIDCreated(pextraPackage->pPack->iFormID)) {
                pextraPackage->pPack->InitLoadGame();
            }
            if (pextraPackage->pTarg) {
                uint32_t itargetID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraPackage->pTarg));
                pextraPackage->pTarg = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(itargetID));
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
                    pactor = dynamic_cast<Actor*>(TESForm::GetFormByNumericID(iactorID));
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
    else if ((aiFlags & CHANGE_DOOR_EXTRA_TELEPORT) != 0) {
        DoorTeleportData* pteleport = GetTeleport();
        pteleport->FinishInitLoadGame();
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 70u) {
        BSSimpleList<TESObjectREFR*>* plist = GetDroppedItemList();
        BSSimpleList<TESObjectREFR*>* plastList = nullptr;
        while (plist && !plist->IsEmpty()) {
            TESObjectREFR* pref = plist->m_item;
            uint32_t irefID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pref));
            if (pref) {
                pref = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(irefID));
            }
            if (pref) {
                plist->SetItem(pref);
                pref->m_Extra.SetItemDropper(apRef);
                plastList = plist;
                plist = plist->m_pkNext;
            }
            else if (plastList) {
                TESObjectREFR* pstaleItem = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(irefID));
                plastList->Remove(pstaleItem);
                plist = plastList->m_pkNext;
            }
            else {
                plist->RemoveHead();
                if (plist->IsEmpty()) {
                    plist = nullptr;
                }
            }
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 70u) {
        uint32_t idropperID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetItemDropper()));
        if (idropperID) {
            TESObjectREFR* pDropper = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(idropperID));
            SetItemDropper(pDropper);
            if (pDropper) {
                pDropper->m_Extra.AddDroppedItem(apRef);
            }
        }
    }
    TESObjectREFR* ptarget = GetHeadTrackTargetExtra();
    if (ptarget) {
        uint32_t itargetID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ptarget));
        ptarget = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(itargetID));
        SetHeadTrackTargetExtra(ptarget);
    }
}

void ExtraDataList::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags, TESObjectREFR* apRef) {
    bool pActor = false;
    if (apRef) {
        pActor = apRef->IsActor();
    }
    if (!pActor && (aiFlags & CHANGE_DOOR_EXTRA_TELEPORT) != 0) {
        GetTeleport();
    }
}

// KEEP: some chunks copy stack bytes nothing wrote into the form buffer: XNDP's last two bytes and XLOC's padding (and,
// in the 32-bit layout, XESP's padding after cFlags). An uninitialised read: unregistered. Saves each extra's chunks,
// under the extra data lock. Form pointers are saved as their form IDs. The exe's cleanup-only C++ EH frame (the XATO
// strings') is left out.
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
                saveData.pLocationForm = reinterpret_cast<TESForm*>(
                    static_cast<uintptr_t>(pStart->worldLoc.pLocationForm->iFormID)
                );
                saveData.locPt = pStart->worldLoc.locPt;
                // Saved from the extra like the form and point (a departure): the exe saves an unset rotation, which
                // the loader passes back as the package start rotation.
                saveData.fZRot = pStart->worldLoc.fZRot;
                if (TESForm::QEndianSwapOnSave()) {
                    saveData.Endian();
                }
                // The file's record is 0x14 bytes, the location's 4-byte form ID first.
                TESFile::NarrowFormIDRecords(
                    &saveData,
                    1,
                    0x14,
                    sizeof(saveData),
                    {{0, offsetof(WORLD_LOCATION, pLocationForm)}}
                );
                TESForm::__AddChunkData(XPSL_ID, &saveData, 20u);
                if (TESForm::QEndianSwapOnSave()) {
                    saveData.Endian();
                }
                break;
            }
            case EXTRA_LEVCREA_MOD:
                TESForm::AddChunk(
                    XLCM_ID,
                    static_cast<uint32_t>(static_cast<ExtraLevCreaModifier*>(pExtra)->eModifier)
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
                // The file's record is 0x14 bytes, the key's 4-byte form ID at 4.
                TESFile::NarrowFormIDRecords(&data, 1, 0x14, sizeof(LockData), {{4, offsetof(LockData, pKey)}});
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
                    data.pParent = reinterpret_cast<TESObjectREFR*>(
                        static_cast<uintptr_t>(pParentExtra->pParent->iFormID)
                    );
                    data.cFlags = pParentExtra->cFlags;
                    if (TESForm::QEndianSwapOnSave()) {
                        data.Endian();
                    }
                    // The file's record is 8 bytes, the parent's 4-byte form ID first.
                    TESFile::NarrowFormIDRecords(
                        &data,
                        1,
                        8,
                        sizeof(data),
                        {{0, offsetof(REF_ENABLE_PARENT_DATA, pParent)}}
                    );
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
                    saveData.pActivateRef = reinterpret_cast<TESObjectREFR*>(
                        static_cast<uintptr_t>(saveData.pActivateRef->iFormID)
                    );
                    if (TESForm::QEndianSwapOnSave()) {
                        saveData.Endian();
                    }
                    // The file's record is 8 bytes, the parent's 4-byte form ID first.
                    TESFile::NarrowFormIDRecords(
                        &saveData,
                        1,
                        8,
                        sizeof(REF_ACTIVATE_DATA),
                        {{0, offsetof(REF_ACTIVATE_DATA, pActivateRef)}}
                    );
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
                    saveData.pDecalRef = reinterpret_cast<TESObjectREFR*>(
                        static_cast<uintptr_t>(pData->pDecalRef->iFormID)
                    );
                    if (TESForm::QEndianSwapOnSave()) {
                        saveData.Endian();
                    }
                    // The file's record is 0x1C bytes, the reference's 4-byte form ID first.
                    TESFile::NarrowFormIDRecords(
                        &saveData,
                        1,
                        0x1C,
                        sizeof(saveData),
                        {{0, offsetof(REF_DECAL_DATA, pDecalRef)}}
                    );
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
                    // The file's record is 8 bytes, the navmesh's 4-byte form ID first.
                    TESFile::NarrowFormIDRecords(
                        &saveData,
                        1,
                        8,
                        sizeof(saveData),
                        {{0, offsetof(NAVMESH_PORTAL, pNavMesh)}}
                    );
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
                    // The file's record is 8 bytes, the reference's 4-byte form ID first.
                    TESFile::NarrowFormIDRecords(
                        &saveData,
                        1,
                        8,
                        sizeof(saveData),
                        {{0, offsetof(REF_REFLECTOR_DATA, pRef)}}
                    );
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
                // The file's record is 0x10 bytes, the position reference's 4-byte form ID at 0xC. The fields before
                // it keep their offsets, so narrowing the extra's own data in place changes only its padding at 0xC.
                TESFile::NarrowFormIDRecords(
                    pData,
                    1,
                    0x10,
                    sizeof(RADIO_DATA),
                    {{0xC, offsetof(RADIO_DATA, pPositionRef)}}
                );
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
                pPatrolExtra->pPatrolData->OnPatrol.Save(XPPA_ID);
                break;
            }
            case EXTRA_OCCLUSION_PLANE: {
                OCCLUSION_PLANE_DATA saveData;
                BSOcclusionPlane* pPlane = static_cast<ExtraOcclusionPlane*>(pExtra)->spPlane;
                saveData.rHalfExtents = pPlane->kHalfExtents;
                saveData.rCenter = pPlane->kCenter;
                pPlane->QRotation()
                    .ExtractAngleAndAxis(saveData.fAngle, saveData.rAxis.x, saveData.rAxis.y, saveData.rAxis.z);
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
                // The file's record is four 4-byte form IDs.
                TESFile::NarrowFormIDRecords(saveData.pLinkedRefs, 4, 4, sizeof(TESObjectREFR*), {{0, 0}});
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
                // The file's record is two 4-byte form IDs.
                TESFile::NarrowFormIDRecords(saveData.pLinkedRefs, 2, 4, sizeof(TESObjectREFR*), {{0, 0}});
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
                pPortal->QRotation()
                    .ExtractAngleAndAxis(saveData.fAngle, saveData.rAxis.x, saveData.rAxis.y, saveData.rAxis.z);
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
    uint32_t iheadingTargetID;
    uint32_t idropperID;
    uint32_t iworldSpaceID;
    uint8_t clength;
    uint32_t iactorID;
    uint32_t ilocationID;
    uint32_t ipackageID;
    uint8_t cpackType;
    uint32_t itargetID;
    uint32_t ipackID;
    uint32_t ikeyID;
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
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        pblockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    BaseExtraList::ExtraCritSection.Lock(nullptr);
    icount = 0;
    uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&icount, sizeof(icount));
    for (BSExtraData* work = pHead; work; work = work->pNext) {
        cextratype = work->cEtype;
        void* ptempBuffer = pSaveLoadGameOLD->m_pBuffer;
        switch (work->cEtype) {
            case EXTRA_PERSISTENT_CELL: {
                if (apRef && apRef->IsActor()) {
                    pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, sizeof(cextratype));
                    ExtraPersistentCell* ppersistentCell = dynamic_cast<ExtraPersistentCell*>(work);
                    iworldSpaceID = 0;
                    if (ppersistentCell->pPersistentCell) {
                        iworldSpaceID = ppersistentCell->pPersistentCell->GetWorldSpace()->iFormID;
                    }
                    pSaveLoadGameOLD->SaveNumericID(&iworldSpaceID, 4u);
                }

                break;
            }
            case EXTRA_PACKAGESTARTLOC: {
                pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, sizeof(cextratype));
                ExtraPackageStartLocation* pstartLocation = dynamic_cast<ExtraPackageStartLocation*>(work);
                ilocationID = 0;
                if (pstartLocation->worldLoc.pLocationForm) {
                    ilocationID = pstartLocation->worldLoc.pLocationForm->iFormID;
                }
                else {
                    const char* pEditorID = apRef->GetFormEditorID();
                    Warning(
                        "SAVELOAD: Saving package start location on %08X %s, but the location is NULL.",
                        apRef->iFormID,
                        pEditorID
                    );
                }
                pSaveLoadGameOLD->SaveNumericID(&ilocationID, 4u);
                pSaveLoadGameOLD->SaveGameDataOLD(
                    &pstartLocation->worldLoc.locPt,
                    sizeof(pstartLocation->worldLoc.locPt)
                );
                pSaveLoadGameOLD->SaveGameDataOLD(
                    &pstartLocation->worldLoc.fZRot,
                    sizeof(pstartLocation->worldLoc.fZRot)
                );
                break;
            }
            case EXTRA_PACKAGE: {
                pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, sizeof(cextratype));
                ExtraPackage* ppackage = dynamic_cast<ExtraPackage*>(work);
                ipackID = ppackage->pPack->iFormID;
                itargetID = 0;
                if (ppackage->pTarg) {
                    itargetID = ppackage->pTarg->iFormID;
                }
                pSaveLoadGameOLD->SaveNumericID(&ipackID, 4u);
                pSaveLoadGameOLD->SaveGameDataOLD(&ppackage->iindex, sizeof(ppackage->iindex));
                pSaveLoadGameOLD->SaveNumericID(&itargetID, 4u);
                pSaveLoadGameOLD->SaveGameDataOLD(&ppackage->bActionComplete, sizeof(ppackage->bActionComplete));
                pSaveLoadGameOLD->SaveGameDataOLD(&ppackage->bActivated, sizeof(ppackage->bActivated));
                if (pSaveLoadGameOLD->GetCurrentVersion() >= 64u && pDataHandler->IsFormIDCreated(ipackID)) {
                    cpackType = ppackage->pPack->GetPackType();
                    pSaveLoadGameOLD->SaveGameDataOLD(&cpackType, sizeof(cpackType));
                    ppackage->pPack->SaveGame();
                }
                break;
            }
            case EXTRA_RUN_ONCE_PACKAGES: {
                uint16_t ipackageCount;
                pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, sizeof(cextratype));
                ipackageCount = 0;
                uint16_t* ppackageCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
                pSaveLoadGameOLD->SaveGameDataOLD(&ipackageCount, sizeof(ipackageCount));
                ExtraRunOncePacks* ppackages = dynamic_cast<ExtraRunOncePacks*>(work);
                for (BSSimpleList<RunOncePackage*>* plist = ppackages->pPackageList; plist && !plist->IsEmpty();
                     plist = plist->m_pkNext) {
                    RunOncePackage* prunOncePackage = plist->m_item;
                    if (prunOncePackage) {
                        ipackageID = prunOncePackage->pPackage->iFormID;
                        pSaveLoadGameOLD->SaveNumericID(&ipackageID, 4u);
                        pSaveLoadGameOLD->SaveGameDataOLD(&prunOncePackage->cday, sizeof(prunOncePackage->cday));
                        ++ipackageCount;
                    }
                }
                *ppackageCount = ipackageCount;
                break;
            }
            case EXTRA_FOLLOWER: {
                uint16_t ifollowerCount;
                pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, sizeof(cextratype));
                ExtraFollower* pfollow = dynamic_cast<ExtraFollower*>(work);
                ifollowerCount = 0;
                uint16_t* pfollowerCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
                pSaveLoadGameOLD->SaveGameDataOLD(&ifollowerCount, sizeof(ifollowerCount));
                for (BSSimpleList<Actor*>* plist = pfollow->pActorlist; plist && !plist->IsEmpty();
                     plist = plist->m_pkNext) {
                    Actor* pactor = plist->m_item;
                    if (pactor) {
                        iactorID = pactor->iFormID;
                        pSaveLoadGameOLD->SaveNumericID(&iactorID, 4u);
                        ++ifollowerCount;
                    }
                }
                *pfollowerCount = ifollowerCount;
                break;
            }
            case EXTRA_GHOST: {
                pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, sizeof(cextratype));
                break;
            }
            case EXTRA_LOCK: {
                if ((aiFlags & CHANGE_OBJECT_EXTRA_LOCK) != 0) {
                    pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, sizeof(cextratype));
                    // Reads the lock without a dynamic cast: type 42 is ExtraLock.
                    LockData* plock = static_cast<ExtraLock*>(work)->pLock;
                    ikeyID = 0;
                    if (plock->pKey) {
                        ikeyID = plock->pKey->iFormID;
                    }
                    pSaveLoadGameOLD->SaveGameDataOLD(&plock->cLockLevel, sizeof(plock->cLockLevel));
                    pSaveLoadGameOLD->SaveNumericID(&ikeyID, 4u);
                    pSaveLoadGameOLD->SaveGameDataOLD(&plock->cFlags, sizeof(plock->cFlags));
                }
                break;
            }
            case EXTRA_TELEPORT: {
                if ((aiFlags & CHANGE_DOOR_EXTRA_TELEPORT) != 0) {
                    pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, sizeof(cextratype));
                    ExtraTeleport* pteleport = dynamic_cast<ExtraTeleport*>(work);
                    pteleport->pTeleportData->SaveGame();
                }
                break;
            }
            case EXTRA_ITEMDROPPER: {
                pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, sizeof(cextratype));
                ExtraItemDropper* pdropper = dynamic_cast<ExtraItemDropper*>(work);
                idropperID = 0;
                if (pdropper->pDropper) {
                    idropperID = pdropper->pDropper->iFormID;
                }
                pSaveLoadGameOLD->SaveNumericID(&idropperID, 4u);
                break;
            }
            case EXTRA_LASTFINISHEDSEQUENCE: {
                if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
                    pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, sizeof(cextratype));
                    ExtraLastFinishedSequence* plast = dynamic_cast<ExtraLastFinishedSequence*>(work);
                    clength = BSstrlen(plast->pLastSequenceName);
                    pSaveLoadGameOLD->SaveGameDataOLD(&clength, sizeof(clength));
                    pSaveLoadGameOLD->SaveGameDataOLD(plast->pLastSequenceName, clength);
                }
                break;
            }
            case EXTRA_HEAD_TRACK_TARGET: {
                pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, sizeof(cextratype));
                ExtraHeadingTarget* pheading = dynamic_cast<ExtraHeadingTarget*>(work);
                iheadingTargetID = 0;
                if (pheading->pHeadingTarget) {
                    iheadingTargetID = pheading->pHeadingTarget->iFormID;
                }
                pSaveLoadGameOLD->SaveNumericID(&iheadingTargetID, 4u);
                break;
            }
            case EXTRA_INFO_GENERAL_TOPIC: {
                ExtraInfoGeneralTopic* pinfoGeneralTopic = dynamic_cast<ExtraInfoGeneralTopic*>(work);
                if (pinfoGeneralTopic->pInfoGen) {
                    pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, sizeof(cextratype));
                    pinfoGeneralTopic->pInfoGen->SaveGame();
                }
                break;
            }
            case EXTRA_NO_RUMORS: {
                pSaveLoadGameOLD->SaveGameDataOLD(&cextratype, sizeof(cextratype));
                ExtraHasNoRumors* phasNoRumors = dynamic_cast<ExtraHasNoRumors*>(work);
                pSaveLoadGameOLD->SaveGameDataOLD(&phasNoRumors->bNoRumors, sizeof(phasNoRumors->bNoRumors));
                break;
            }
            default: {
                break;
            }
        }
        if (ptempBuffer != pSaveLoadGameOLD->m_pBuffer) {
            ++icount;
        }
    }
    *pcount = icount;
    // 11295 is the source line number this check reports.
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* pendPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* pEditorID = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                pendPosition - itempStartPosition,
                pheader->iFormID,
                pEditorID,
                iFlags,
                11295,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                pendPosition - itempStartPosition,
                11295,
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
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp",
                11295
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
        pSaveLoadGameOLD->LoadGameDataOLD(&uiMarker, sizeof(uiMarker));
        if (uiMarker != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                uint32_t uiFlags = pHeader->iFlags;
                uint8_t ucVersion = pHeader->cVersion;
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp",
                    11308,
                    pHeader->iFormID,
                    pForm->GetFormEditorID(),
                    ucVersion,
                    uiFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp",
                    11308,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->LoadGameDataOLD(&usSize, sizeof(usSize));
    }
    bool bScriptLoaded = false;
    uint16_t usCount;
    pSaveLoadGameOLD->LoadGameDataOLD(&usCount, sizeof(usCount));
    for (int32_t i = 0; i < usCount; ++i) {
        uint8_t ucType;
        pSaveLoadGameOLD->LoadGameDataOLD(&ucType, sizeof(ucType));
        switch (ucType) {
            case EXTRA_COUNT:
                if ((aiFlags & 0) != 0) {
                    uint16_t usItemCount;
                    pSaveLoadGameOLD->LoadGameDataOLD(&usItemCount, sizeof(usItemCount));
                    SetCount(usItemCount);
                }
                break;
            case EXTRA_HEALTH:
                if ((aiFlags & 0) != 0) {
                    float fHealth;
                    pSaveLoadGameOLD->LoadGameDataOLD(&fHealth, sizeof(fHealth));
                    SetHealth(fHealth);
                }
                break;
            case EXTRA_USES:
                if ((aiFlags & 0) != 0) {
                    uint8_t ucUses;
                    pSaveLoadGameOLD->LoadGameDataOLD(&ucUses, sizeof(ucUses));
                    SetUses(ucUses);
                }
                break;
            case EXTRA_WEAPON_MOD_SLOTS:
                if ((aiFlags & 0) != 0) {
                    uint8_t ucModFlags;
                    pSaveLoadGameOLD->LoadGameDataOLD(&ucModFlags, sizeof(ucModFlags));
                    SetWeaponModFlags(ucModFlags);
                }
                break;
            case EXTRA_TIMELEFT:
                if ((aiFlags & 0) != 0) {
                    float fTimeLeft;
                    pSaveLoadGameOLD->LoadGameDataOLD(&fTimeLeft, sizeof(fTimeLeft));
                    SetTimeLeft(fTimeLeft);
                }
                break;
            case EXTRA_CHARGE:
                if ((aiFlags & 0) != 0) {
                    float fCharge;
                    pSaveLoadGameOLD->LoadGameDataOLD(&fCharge, sizeof(fCharge));
                    SetCharge(fCharge);
                }
                break;
            case EXTRA_LOCK:
                if ((aiFlags & CHANGE_OBJECT_EXTRA_LOCK) != 0) {
                    LockData* pLock = new LockData;
                    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x15) {
                        pSaveLoadGameOLD->LoadGameDataOLD(&pLock->cLockLevel, sizeof(pLock->cLockLevel));
                        uint32_t uiKeyID;
                        pSaveLoadGameOLD->LoadNumericID(&uiKeyID, 4);
                        if (pSaveLoadGameOLD->GetCurrentVersion() < 0x1A) {
                            // Older saves hold a second form ID that is read and dropped.
                            uint32_t uiUnusedID;
                            pSaveLoadGameOLD->LoadNumericID(&uiUnusedID, 4);
                        }
                        pSaveLoadGameOLD->LoadGameDataOLD(&pLock->cFlags, sizeof(pLock->cFlags));
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
                    pSaveLoadGameOLD->LoadGameDataOLD(&fScale, sizeof(fScale));
                    if (apRef) {
                        apRef->SetScale(fScale);
                    }
                    else {
                        SetScale(fScale);
                    }
                }
                if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x43 && (aiFlags & 0) != 0) {
                    float fScale;
                    pSaveLoadGameOLD->LoadGameDataOLD(&fScale, sizeof(fScale));
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
                    pSaveLoadGameOLD->LoadGameDataOLD(&iRank, sizeof(iRank));
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
                        ScriptLocals* pScratch = new ScriptLocals;
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
                    pSaveLoadGameOLD->LoadGameDataOLD(&iIndex, sizeof(iIndex));
                    uint8_t ucLevelFlag;
                    pSaveLoadGameOLD->LoadGameDataOLD(&ucLevelFlag, sizeof(ucLevelFlag));
                    SetLevelItemExtra(iIndex);
                    SetLevelItemExtraFlag(ucLevelFlag);
                }
                break;
            case EXTRA_PACKAGE: {
                uint32_t uiPackageID;
                pSaveLoadGameOLD->LoadNumericID(&uiPackageID, 4);
                int32_t iIndex;
                pSaveLoadGameOLD->LoadGameDataOLD(&iIndex, sizeof(iIndex));
                uint32_t uiTargetID;
                pSaveLoadGameOLD->LoadNumericID(&uiTargetID, 4);
                // Bytes are passed on to bool parameters as read.
                bool bComplete;
                pSaveLoadGameOLD->LoadGameDataOLD(&bComplete, sizeof(bComplete));
                bool bActivated;
                pSaveLoadGameOLD->LoadGameDataOLD(&bActivated, sizeof(bActivated));
                TESPackage* pPackage = nullptr;
                if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x40 && pDataHandler->IsFormIDCreated(uiPackageID)) {
                    uint8_t ucPackType;
                    pSaveLoadGameOLD->LoadGameDataOLD(&ucPackType, sizeof(ucPackType));
                    pPackage = pSaveLoadGameOLD->CreatePackage(uiPackageID, ucPackType);
                    pPackage->LoadGame();
                }
                if (!pPackage) {
                    pPackage = dynamic_cast<TESPackage*>(TESForm::GetFormByNumericID(uiPackageID));
                }
                if (pPackage) {
                    SetPackageExtra(
                        pPackage,
                        iIndex,
                        reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiTargetID)),
                        bComplete,
                        bActivated,
                        false
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
                    pSaveLoadGameOLD->LoadGameDataOLD(&uiUnused, sizeof(uiUnused));
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
                pSaveLoadGameOLD->LoadGameDataOLD(&usPackages, sizeof(usPackages));
                for (int32_t j = 0; j < usPackages; ++j) {
                    uint32_t uiPackageID;
                    pSaveLoadGameOLD->LoadNumericID(&uiPackageID, 4);
                    uint8_t ucDay;
                    pSaveLoadGameOLD->LoadGameDataOLD(&ucDay, sizeof(ucDay));
                    TESPackage* pPackage = dynamic_cast<TESPackage*>(TESForm::GetFormByNumericID(uiPackageID));
                    if (pPackage) {
                        SetRunOnceExtraPackage(pPackage, ucDay);
                    }
                }
                break;
            }
            case EXTRA_PACKAGESTARTLOC: {
                WORLD_LOCATION kLocation;
                uint32_t uiLocationID;
                pSaveLoadGameOLD->LoadNumericID(&uiLocationID, 4);
                pSaveLoadGameOLD->LoadGameDataOLD(&kLocation.locPt, sizeof(kLocation.locPt));
                pSaveLoadGameOLD->LoadGameDataOLD(&kLocation.fZRot, sizeof(kLocation.fZRot));
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
                pSaveLoadGameOLD->LoadGameDataOLD(&usFollowers, sizeof(usFollowers));
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
                    NonActorMagicCaster* pCaster = new NonActorMagicCaster(
                        reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiCasterID))
                    );
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
                    NonActorMagicTarget* pTarget = new NonActorMagicTarget(
                        reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiTargetID))
                    );
                    MagicTarget* pMagicTarget = pTarget;
                    ActiveEffect::LoadActiveEffectListTES(pMagicTarget->GetActiveEffectList(), nullptr);
                    AddExtra(pTarget);
                }
                break;
            case EXTRA_TELEPORT:
                if ((aiFlags & CHANGE_DOOR_EXTRA_TELEPORT) != 0) {
                    DoorTeleportData* pTeleport = new DoorTeleportData;
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
                        pSaveLoadGameOLD->LoadGameDataOLD(&iGroup, sizeof(iGroup));
                        if (iGroup < 245) {
                            SetLastFinishedSequence(animGroupInfo[iGroup].pName);
                        }
                    }
                    if (pSaveLoadGameOLD->GetCurrentVersion() < 0x15 || pSaveLoadGameOLD->GetCurrentVersion() >= 0x17) {
                        uint8_t ucLength;
                        pSaveLoadGameOLD->LoadGameDataOLD(&ucLength, sizeof(ucLength));
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
                    pSaveLoadGameOLD->LoadGameDataOLD(&ucMarkerFlags, sizeof(ucMarkerFlags));
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
                pSaveLoadGameOLD->LoadGameDataOLD(&ucItems, sizeof(ucItems));
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
                    pSaveLoadGameOLD->LoadGameDataOLD(&ucHotKey, sizeof(ucHotKey));
                    SetHotKey(ucHotKey);
                }
                break;
            case EXTRA_INFO_GENERAL_TOPIC: {
                MenuTopic* pTopic = new MenuTopic;
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
                // The byte goes on to the bool parameter as read.
                bool bNoRumors;
                pSaveLoadGameOLD->LoadGameDataOLD(&bNoRumors, sizeof(bNoRumors));
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
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp",
                12009,
                pHeader->iFormID,
                pForm->GetFormEditorID(),
                ucVersion,
                uiFlags
            );
        }
        else if (pNow < pStart + usSize) {
            uint32_t uiFlags = pHeader->iFlags;
            uint8_t ucVersion = pHeader->cVersion;
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading form is "
                "%08X %s with version %i and flags %08X",
                static_cast<int32_t>((pStart + usSize) - pNow),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp",
                12009,
                pHeader->iFormID,
                pForm->GetFormEditorID(),
                ucVersion,
                uiFlags
            );
        }
    }
    else if (pNow > pStart + usSize) {
        Warning(
            "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
            static_cast<int32_t>(pNow - (pStart + usSize)),
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp",
            12009,
            pSaveLoadGameOLD->GetCurrentVersion()
        );
    }
    else if (pNow < pStart + usSize) {
        Warning(
            "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
            static_cast<int32_t>((pStart + usSize) - pNow),
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp",
            12009,
            pSaveLoadGameOLD->GetCurrentVersion()
        );
    }
}

void ExtraDataList::Revert(uint32_t aiFlags, TESObjectREFR* apRef) {
    uint8_t bActor = 0;
    if (apRef) {
        bActor = apRef->IsActor();
    }
    if ((aiFlags & CHANGE_DOOR_EXTRA_TELEPORT) != 0 && !bActor && GetTeleport()) {
        RemoveTeleportPtr();
    }

    if ((aiFlags & CHANGE_REFR_ANIMATION) != 0) {
        RemoveLastFinishedSequence();
    }
    if (pSaveLoadGameOLD->IsUnk048Zero()) {
        if (bActor) {
            RemovePackageExtra();
            RemoveExtraRunOncePackageList();
            RemovePackageStartLocation();
            RemoveFollowerExtra();
            RemoveFriendHitsExtra();
            BaseExtraList::RemoveExtra(EXTRA_DROPPEDITEMLIST);
            SetHeadTrackTargetExtra(nullptr);
            RemoveGhost();
            RemoveInfoGeneral();
            ClearRumorsExtra();
        }
        else {
            TESObjectREFR* pDropper = GetItemDropper();
            if (pDropper) {
                pDropper->TESObjectREFR::GetExtra()->RemoveDroppedItem(apRef);
                SetItemDropper(nullptr);
            }
        }
    }
}

void ExtraDataList::SetRoomLinkedRefData(RoomRefData* apData) {
    BSExtraData* pOldExtra = GetExtraData(123u);
    if (pOldExtra) {
        RemoveExtra(pOldExtra, true);
    }
    if (apData) {
        ExtraRoomRefData* pExtra = new ExtraRoomRefData;
        pExtra->pData = apData;
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetStartingWorldOrCell(TESForm* apWorldOrCell) {
    if (apWorldOrCell) {
        auto* pExtra = static_cast<ExtraStartingWorldOrCell*>(GetExtraData(EXTRA_STARTINGWORLDORCELL));
        if (!pExtra) {
            pExtra = new ExtraStartingWorldOrCell;
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
    BSExtraData* pOldExtra = GetExtraData(118u);
    if (pOldExtra) {
        RemoveExtra(pOldExtra, true);
    }
    if (apData) {
        ExtraOcclusionPlaneRefData* pExtra = new ExtraOcclusionPlaneRefData;
        pExtra->pData = apData;
        AddExtra(pExtra);
    }
}

TESObjectREFR* ExtraDataList::GetMasterRoomRecurse(NiTMap<TESObjectREFR*, bool>* apRoomsTouchedMap) {
    ExtraRoomRefData* pextra = static_cast<ExtraRoomRefData*>(GetExtraData(123));
    if (!pextra) {
        return nullptr;
    }
    for (TESObjectREFR* pref : pextra->pData->kRoomList.ItemsUntilNull()) {
        if (pref->m_Extra.GetRoomIsMaster()) {
            return pref;
        }
    }
    for (TESObjectREFR* pref : pextra->pData->kRoomList.ItemsUntilNull()) {
        bool btest;
        if (!apRoomsTouchedMap->GetAt(pref, btest)) {
            apRoomsTouchedMap->SetAt(pref, btest);
            TESObjectREFR* pmaster = pref->m_Extra.GetMasterRoomRecurse(apRoomsTouchedMap);
            if (pmaster) {
                return pmaster;
            }
        }
    }
    return nullptr;
}

void ExtraDataList::SetPortalLinkedRefData(PortalLinkedRefData* apLinkedRefData) {
    BSExtraData* pOldExtra = GetExtraData(119u);
    if (pOldExtra) {
        RemoveExtra(pOldExtra, true);
    }
    if (apLinkedRefData) {
        ExtraPortalRefData* pExtra = new ExtraPortalRefData;
        pExtra->pData = apLinkedRefData;
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetReflectedByRef(TESObjectREFR* apRef, bool bValue) {
    ExtraReflectorRefs* pextra = static_cast<ExtraReflectorRefs*>(GetExtraData(102));
    if (pextra) {
        for (REF_REFLECTOR_DATA*& rpItem : pextra->RefList.ItemsUntilNull()) {
            if (rpItem->pRef == apRef) {
                if (bValue) {
                    rpItem->iEffectFlags |= REF_REFLECTOR_DATA::FLAG_REFLECTION;
                }
                else {
                    rpItem->iEffectFlags &= ~REF_REFLECTOR_DATA::FLAG_REFLECTION;
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
            pextra = new ExtraReflectorRefs;
            AddExtra(pextra);
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
    else if (bValue) {
        pextra = new ExtraLitWaterRefs;
        AddExtra(pextra);
        pextra->RefList.AddHead(apRef);
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
    pExtra = new ExtraLock(apLock);
    AddExtra(pExtra);
    return pExtra;
}

void ExtraDataList::AddDecalRef(TESObjectREFR* apRef, const NiPoint3& aIntersect, const NiPoint3& aNormal) {
    if (apRef) {
        ExtraDecalRefs* pExtra = static_cast<ExtraDecalRefs*>(BaseExtraList::GetExtraData(EXTRA_DECAL_REFS));
        if (!pExtra) {
            pExtra = new ExtraDecalRefs;
            BaseExtraList::AddExtra(pExtra);
        }
        pExtra->SetRefDecalData(apRef, aIntersect, aNormal);
    }
}

ExtraLight* ExtraDataList::SetSpellEffectLightPtr(LightRefData* apLight) {
    ExtraLight* pExtra = static_cast<ExtraLight*>(GetExtraData(64));
    if (pExtra) {
        if (pExtra->pLight) {
            delete pExtra->pLight;
        }
        pExtra->pLight = apLight;
        return pExtra;
    }

    pExtra = new ExtraLight(apLight);
    pExtra->cEtype = 64;
    AddExtra(pExtra);
    return pExtra;
}

NiPoint3 ExtraDataList::GetStartingAngle(TESObjectREFR* apObject) {
    ExtraStartingPosition* pStart = static_cast<ExtraStartingPosition*>(GetExtraData(15u));
    if (!pStart) {
        pStart = SetStartingPositionPtr(apObject);
    }
    return pStart->startRotation;
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
        ExtraSeenData* pExtra = static_cast<ExtraSeenData*>(GetExtraData(5));
        if (pExtra) {
            if (pExtra->pSeenData) {
                delete pExtra->pSeenData;
            }
            pExtra->pSeenData = apData;
        }
        else {
            pExtra = new ExtraSeenData;
            pExtra->pSeenData = apData;
            AddExtra(pExtra);
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
    return !extraData || static_cast<ExtraPatrolRefInUseData*>(extraData)->GetCanBeUsedBy(apUser);
}

char ExtraDataList::GetTerminalLockState() {
    auto* pExtra = static_cast<ExtraTerminalState*>(GetExtraData(EXTRA_TERMINALSTATE));
    if (!pExtra) {
        return -1;
    }
    return static_cast<int8_t>(pExtra->cLockOverride);
}

void ExtraDataList::SetTerminalLockState(char acLockState) {
    auto* pExtra = static_cast<ExtraTerminalState*>(GetExtraData(EXTRA_TERMINALSTATE));
    if (acLockState) {
        if (!pExtra) {
            pExtra = new ExtraTerminalState;
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

void ExtraDataList::SetTerminalState(uint8_t acState) {
    auto* pExtra = static_cast<ExtraTerminalState*>(GetExtraData(EXTRA_TERMINALSTATE));
    if (acState) {
        if (!pExtra) {
            pExtra = new ExtraTerminalState;
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
            pExtra = new ExtraCombatStyle(apCombatStyle);
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
        pExtra = new ExtraPatrolRefInUseData(apUser);
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetGhost(bool abGhost) {
    bool bHasExtra = HasExtra(EXTRA_GHOST);
    if (abGhost && !bHasExtra) {
        ExtraGhost* pExtra = new ExtraGhost;
        AddExtra(pExtra);
    }
    else if (!abGhost && bHasExtra) {
        RemoveExtra(EXTRA_GHOST);
    }
}

void ExtraDataList::SetOpenCloseActivateRef(TESObjectREFR* apRef) {
    if (apRef) {
        auto* pExtra = static_cast<ExtraOpenCloseActivateRef*>(GetExtraData(EXTRA_OPENCLOSEACTIVATE_REF));
        if (pExtra) {
            pExtra->pActivateRef = apRef;
        }
        else {
            pExtra = new ExtraOpenCloseActivateRef;
            pExtra->pActivateRef = apRef;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_OPENCLOSEACTIVATE_REF);
    }
}

void ExtraDataList::SetAshPileRef(TESObjectREFR* apRef) {
    auto* pExtra = static_cast<ExtraAshPileRef*>(GetExtraData(EXTRA_ASHPILE_REF));
    if (apRef) {
        if (!pExtra) {
            pExtra = new ExtraAshPileRef;
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
    pExtra = new ExtraHotKey(static_cast<uint8_t>(acValue));
    AddExtra(pExtra);
}

void ExtraDataList::SetRunOnceExtraPackage(TESPackage* apPack, char acDay) {
    auto* pExtra = static_cast<ExtraRunOncePacks*>(GetExtraData(EXTRA_RUN_ONCE_PACKAGES));
    if (!pExtra) {
        pExtra = new ExtraRunOncePacks;
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
    ExtraSayToTopicInfo* extraData = static_cast<ExtraSayToTopicInfo*>(
        BaseExtraList::GetExtraData(EXTRA_SAY_TO_TOPIC_INFO)
    );
    if (extraData) {
        return extraData->pInfo;
    }
    return nullptr;
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
    ExtraAnim* pExtra = static_cast<ExtraAnim*>(GetExtraData(16u));
    if (pExtra) {
        if (pExtra->pAnimation) {
            delete pExtra->pAnimation;
        }
        pExtra->pAnimation = apAnim;
        return pExtra;
    }

    pExtra = new ExtraAnim(apAnim);
    AddExtra(pExtra);
    return pExtra;
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
    ExtraPortalRefData* pExtraData = static_cast<ExtraPortalRefData*>(
        BaseExtraList::GetExtraData(EXTRA_PORTAL_REF_DATA)
    );
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
                if (apRef && apRef->IsActor()) {
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
                    pDataHandler->IsFormIDCreated(pPackage->pPack->iFormID)) {
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
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                isize,
                pheader->iFormID,
                pEditorID,
                iFlags,
                10677,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\ExtraDataList.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                isize,
                10677,
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
    else if (apRagData) {
        pextra = new ExtraRagDollData;
        pextra->pRagDollData = new RagDollData;
        pextra->pRagDollData->Copy(apRagData);
        AddExtra(pextra);
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
        ExtraSavedHavokData* pNew = new ExtraSavedHavokData;
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
        ExtraSavedAnimation* pNew = new ExtraSavedAnimation;
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

void ExtraDataList::SetItemDropper(TESObjectREFR* apDropper) {
    if (apDropper) {
        auto* pExtra = static_cast<ExtraItemDropper*>(GetExtraData(EXTRA_ITEMDROPPER));
        if (!pExtra) {
            pExtra = new ExtraItemDropper;
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
            pextra = new ExtraDroppedItemList;
            AddExtra(pextra);
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
        uint8_t cextraType = ptemp->cEtype;
        if (cextraType && (cextraType <= 2u ? pSaveLoadGame->GetReconstructingForms() : cextraType == 4)) {
            plast = ptemp;
        }
        else {
            if (ptemp == pHead) {
                pHead = pwork;
            }
            if (plast) {
                plast->pNext = pwork;
            }
            delete ptemp;
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
            pExtra = new ExtraMultiBound;
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

void ExtraDataList::SetRandomTeleportMarker(TESObjectREFR* apMarker) {
    if (apMarker) {
        auto* pExtra = static_cast<ExtraRandomTeleportMarker*>(GetExtraData(EXTRA_TELEPORTMARKER));
        if (pExtra) {
            pExtra->pMarker = apMarker;
        }
        else {
            pExtra = new ExtraRandomTeleportMarker;
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
        pextra = new ExtraWaterZoneMap;
        AddExtra(pextra);
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
    ExtraSayToTopicInfo* extraData = static_cast<ExtraSayToTopicInfo*>(
        BaseExtraList::GetExtraData(EXTRA_SAY_TO_TOPIC_INFO)
    );
    if (extraData) {
        return extraData->pTopic;
    }
    return nullptr;
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
            pExtra = new ExtraEncounterZone;
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
// when the copy's eState is set; with a zero eState it writes through a never-initialised pointer.
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
                pExtra = new ExtraEditorRefMoveData(nullptr);
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
            LockData* pLock = new LockData;
            ourMemcpy(pLock, static_cast<ExtraLock*>(apCopy)->pLock, sizeof(LockData));
            SetLockPtr(pLock);
            break;
        }
        case EXTRA_RADIO_DATA: SetRadioData(&static_cast<ExtraRadioData*>(apCopy)->Data); break;
        case EXTRA_TELEPORT: {
            DoorTeleportData* pTeleport = new DoorTeleportData;
            pTeleport->Copy(static_cast<ExtraTeleport*>(apCopy)->pTeleportData);
            SetTeleportPtr(pTeleport);
            break;
        }
        case EXTRA_MAPMARKER: {
            MapMarkerData* pMapData = new MapMarkerData;
            pMapData->Copy(static_cast<ExtraMapMarker*>(apCopy)->pMapData);
            SetMapMarkerPtr(pMapData);
            break;
        }
        case EXTRA_AUDIOMARKER: {
            AudioMarker* pAudioData = new AudioMarker;
            pAudioData->Copy(static_cast<ExtraAudioMarker*>(apCopy)->pAudioData);
            SetAudioMarkerPtr(pAudioData);
            break;
        }
        case EXTRA_AUDIOBUOYMARKER: {
            AudioBuoyMarkerData* pBuoyData = new AudioBuoyMarkerData;
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
                 pNode && pNode->GetItem();
                 pNode = pNode->GetNext()) {
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
            // Deliberately left unset: SetCanopyShadowMask writes it only for a nonzero state.
            D3DLOCKED_RECT* pRect;
            SetCanopyShadowMask(pCopy->eState, pCopy->spMask, pRect);
            pRect->pBits = pCopy->Rect.pBits;
            pRect->Pitch = pCopy->Rect.Pitch;
            break;
        }
        case EXTRA_CLIMATE: SetClimate(static_cast<ExtraCellClimate*>(apCopy)->pClimate); break;
        case EXTRA_IMAGESPACE: SetImageSpace(static_cast<ExtraCellImageSpace*>(apCopy)->pImageSpace); break;
        case EXTRA_IMPACTSWAP: {
            ImpactSwap* pSwap = new ImpactSwap;
            pSwap->Copy(static_cast<ExtraCellImpactSwap*>(apCopy)->pImpactSwap);
            SetImpactSwap(pSwap);
            break;
        }
        case EXTRA_REFERENCE_POINTER: SetReferencePointer(static_cast<ExtraReferencePointer*>(apCopy)->pRef); break;
        case EXTRA_PACKAGE: {
            auto* pCopy = static_cast<ExtraPackage*>(apCopy);
            SetPackageExtra(
                pCopy->pPack,
                pCopy->iindex,
                pCopy->pTarg,
                pCopy->bActionComplete,
                pCopy->bActivated,
                pCopy->bDoneOnce
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
                pExtra = new ExtraActivateRef;
                AddExtra(pExtra);
            }
            pExtra->Copy(apCopy);
            break;
        }
        case EXTRA_DECAL_REFS: {
            auto* pExtra = static_cast<ExtraDecalRefs*>(GetExtraData(EXTRA_DECAL_REFS));
            if (!pExtra) {
                pExtra = new ExtraDecalRefs;
                AddExtra(pExtra);
            }
            pExtra->Copy(apCopy);
            break;
        }
        case EXTRA_REFLECTED_REFS: {
            auto* pExtra = static_cast<ExtraReflectedRefs*>(GetExtraData(EXTRA_REFLECTED_REFS));
            if (!pExtra) {
                pExtra = new ExtraReflectedRefs;
                AddExtra(pExtra);
            }
            pExtra->Copy(apCopy);
            break;
        }
        case EXTRA_REFLECTOR_REFS: {
            auto* pExtra = static_cast<ExtraReflectorRefs*>(GetExtraData(EXTRA_REFLECTOR_REFS));
            if (!pExtra) {
                pExtra = new ExtraReflectorRefs;
                AddExtra(pExtra);
            }
            pExtra->Copy(apCopy);
            break;
        }
        case EXTRA_LIT_WATER_REFS: {
            auto* pExtra = static_cast<ExtraLitWaterRefs*>(GetExtraData(EXTRA_LIT_WATER_REFS));
            if (!pExtra) {
                pExtra = new ExtraLitWaterRefs;
                AddExtra(pExtra);
            }
            pExtra->Copy(apCopy);
            break;
        }
        case EXTRA_WATER_LIGHT_REFS: {
            auto* pExtra = static_cast<ExtraWaterLightRefs*>(GetExtraData(EXTRA_WATER_LIGHT_REFS));
            if (!pExtra) {
                pExtra = new ExtraWaterLightRefs;
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
                pData = new MultiBoundMarkerData;
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
                 pNode && pNode->GetItem();
                 pNode = pNode->GetNext()) {
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
            PatrolRefData* pData = new PatrolRefData;
            pData->Copy(static_cast<ExtraPatrolRefData*>(apCopy)->pPatrolData);
            SetPatrolRefData(pData);
            break;
        }
        case EXTRA_OCCLUSION_PLANE: SetOcclusionPlane(static_cast<ExtraOcclusionPlane*>(apCopy)->spPlane); break;
        case EXTRA_OCCLUSION_PLANE_REF_DATA: {
            OcclusionPlaneLinkedRefData* pData = new OcclusionPlaneLinkedRefData;
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
            CollisionData* pData = new CollisionData;
            pData->SetLayer(static_cast<ExtraCollisionData*>(apCopy)->pCollisionData->QLayer());
            SetCollisionData(pData);
            break;
        }
        case EXTRA_SAY_ONCE_A_DAY_TOPIC_INFO:
            for (BSSimpleList<SayOnceTopicInfos*>* pNode =
                     static_cast<ExtraSayTopicInfoOnceADay*>(apCopy)->pListofSaidOnceTopicInfos;
                 pNode && pNode->GetItem();) {
                SetExtraSayTopicInfoOnceADayExtra(pNode->GetItem());
                // The result of GetNext is deliberately dropped, so the walk never advances past a
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
            if (pCopy->GetWeaponModActive(TESObjectWEAP::MOD_ONE)) {
                SetWeaponModSlot(1);
            }
            if (pCopy->GetWeaponModActive(TESObjectWEAP::MOD_TWO)) {
                SetWeaponModSlot(2);
            }
            if (pCopy->GetWeaponModActive(TESObjectWEAP::MOD_THREE)) {
                SetWeaponModSlot(4);
            }
            break;
        }
        case EXTRA_SPECIAL_RENDER_FLAGS: {
            auto* pExtra = static_cast<ExtraSpecialRenderFlags*>(GetExtraData(EXTRA_SPECIAL_RENDER_FLAGS));
            if (!pExtra) {
                pExtra = new ExtraSpecialRenderFlags;
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
                    ExtraScript* pExtraScript = new ExtraScript(pScript);
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
        pExtra = new ExtraContainerChanges(apChanges);
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
            AddExtra(new ExtraWornLeft);
        }
        else if (!abWorn && bHasWorn) {
            RemoveExtra(EXTRA_WORN_LEFT);
        }
    }
    else {
        bool bHasWorn = HasExtra(EXTRA_WORN);
        if (abWorn && !bHasWorn) {
            AddExtra(new ExtraWorn);
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
    bool bDifferent = false;
    for (BSExtraData* pExtra = apCompare->pHead; pExtra; pExtra = pExtra->GetNext()) {
        if (IsCopyableExtra(pExtra)) {
            BSExtraData* pMine = GetExtraData(pExtra->GetExtraType());
            if (!pMine) {
                bDifferent = true;
                break;
            }
            if (pMine->Compare(pExtra)) {
                bDifferent = true;
                break;
            }
        }
    }
    if (!bDifferent) {
        for (BSExtraData* pExtra = pHead; pExtra; pExtra = pExtra->GetNext()) {
            if (IsCopyableExtra(pExtra)) {
                BSExtraData* pOther = apCompare->GetExtraData(pExtra->GetExtraType());
                if (!pOther) {
                    bDifferent = true;
                    break;
                }
                if (pOther->Compare(pExtra)) {
                    bDifferent = true;
                    break;
                }
            }
        }
    }
    uint32_t uiItemCount = ItemsInList();
    if (uiItemCount != apCompare->ItemsInList()) {
        for (BSExtraData* pExtra = pHead; pExtra; pExtra = pExtra->GetNext()) {
            if (IsCopyableExtra(pExtra)) {
                if (!apCompare->GetExtraData(pExtra->GetExtraType())) {
                    bDifferent = true;
                    break;
                }
            }
        }
    }
    BaseExtraList::ExtraCritSection.Unlock();
    return bDifferent;
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
        pExtra = new ExtraCount(static_cast<uint16_t>(sCount));
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetCanNotWear(bool abCantWear) {
    bool bHasCannotWear = HasExtra(EXTRA_CANNOTWEAR);
    if (abCantWear && !bHasCannotWear) {
        AddExtra(new ExtraCannotWear);
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
            pExtra = new ExtraReferencePointer(apRef);
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
            pExtra = new ExtraRegionList(apList);
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
        pExtra = new ExtraScript(apScript);
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
        pExtra = new ExtraPersistentCell(apCell);
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetImpactSwap(ImpactSwap* apImpactSwap) {
    auto* pExtra = static_cast<ExtraCellImpactSwap*>(GetExtraData(EXTRA_IMPACTSWAP));
    if (apImpactSwap) {
        if (pExtra) {
            pExtra->pImpactSwap = apImpactSwap;
        }
        else {
            pExtra = new ExtraCellImpactSwap;
            pExtra->pImpactSwap = apImpactSwap;
            AddExtra(pExtra);
        }
    }
    else if (pExtra) {
        RemoveExtra(pExtra, true);
    }
}

// The schedule's GetStartTime is called before GetDuration. A dropped package is freed with ourFree,
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
            pExtra = new ExtraEnableStateParent;
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
        pExtra = new ExtraEnableStateChildren;
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
            pExtra = new ExtraCellWaterType;
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
            pExtra = new ExtraLinkedRef;
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
        pExtra = new ExtraLinkedRefChildren;
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
        pExtra = new ExtraActivateRef;
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
        pExtra = new ExtraActivateRefChildren;
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
        REF_ACTIVATE_DATA* pData = new REF_ACTIVATE_DATA;
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
                pNode->GetItem()->iEffectFlags |= REF_REFLECTOR_DATA::FLAG_REFRACTION;
            }
            else {
                pNode->GetItem()->iEffectFlags &= ~REF_REFLECTOR_DATA::FLAG_REFRACTION;
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
            pExtra = new ExtraReflectorRefs;
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
                pNode->GetItem()->iEffectFlags |= REF_REFLECTED_DATA::FLAG_REFRACTION;
            }
            else {
                pNode->GetItem()->iEffectFlags &= ~REF_REFLECTED_DATA::FLAG_REFRACTION;
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
            pExtra = new ExtraReflectedRefs;
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
                    rpItem->iEffectFlags |= REF_REFLECTED_DATA::FLAG_REFLECTION;
                }
                else {
                    rpItem->iEffectFlags &= ~REF_REFLECTED_DATA::FLAG_REFLECTION;
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
            pextra = new ExtraReflectedRefs;
            AddExtra(pextra);
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
    else if (bValue) {
        ExtraWaterLightRefs* pNewExtra;
        pNewExtra = new ExtraWaterLightRefs;
        AddExtra(pNewExtra);
        pNewExtra->RefList.AddHead(apRef);
    }
}

void ExtraDataList::AddPrimitive(BGSPrimitive* apPrimitive) {
    if (!apPrimitive) {
        return;
    }
    ExtraPrimitive* pExtra = new ExtraPrimitive(apPrimitive);
    AddExtra(pExtra);
}

void ExtraDataList::SetPatrolRefData(PatrolRefData* apPatrolRefData) {
    auto* pExtra = static_cast<ExtraPatrolRefData*>(GetExtraData(EXTRA_PATROL_REF_DATA));
    if (pExtra) {
        RemoveExtra(pExtra, true);
    }
    if (apPatrolRefData) {
        pExtra = new ExtraPatrolRefData;
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
            pExtra = new ExtraMerchantContainer;
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
            pExtra = new ExtraLevCreaModifier;
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
            pExtra = new ExtraLeveledCreature;
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
            pExtra = new ExtraNorthRotation;
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
            pExtra = new ExtraXTarget;
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
            pExtra = new ExtraEmittanceSource;
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
            pExtra = new ExtraMultiBoundRef;
            pExtra->pBoundRef = apRef;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_MULTIBOUND_REF);
    }
}

// The replaced marker data is freed with ourFree.
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
            pExtra = new ExtraMultiBoundData;
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
        pExtra = new ExtraRadius(afRadius);
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
        pExtra = new ExtraRadiation(afRadiation);
        AddExtra(pExtra);
    }
}

void ExtraDataList::AddFollower(Actor* apActor) {
    if (apActor == pPlayer) {
        return;
    }
    auto* pExtra = static_cast<ExtraFollower*>(GetExtraData(EXTRA_FOLLOWER));
    if (!pExtra) {
        pExtra = new ExtraFollower;
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
    ExtraLastFinishedSequence* pExtra = new ExtraLastFinishedSequence(apSequenceName);
    AddExtra(pExtra);
}

const char* ExtraDataList::GetLastFinishedSequence() {
    auto* pExtra = static_cast<ExtraLastFinishedSequence*>(GetExtraData(EXTRA_LASTFINISHEDSEQUENCE));
    if (!pExtra) {
        return nullptr;
    }
    return pExtra->pLastSequenceName;
}

void ExtraDataList::SetNavMeshPortal(ExtraNavMeshPortal* apPortal) {
    if (!GetExtraData(90u)) {
        AddExtra(new ExtraNavMeshPortal(apPortal->Portal));
    }
}

void ExtraDataList::AddFactionChangesExtra() {
    auto* pExtra = static_cast<ExtraFactionChanges*>(GetExtraData(EXTRA_FACTION_CHANGES));
    if (!pExtra) {
        pExtra = new ExtraFactionChanges;
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetIgnoredBySandbox(bool abVal) {
    auto* pExtra = static_cast<ExtraIgnoredBySandbox*>(GetExtraData(EXTRA_IGNORED_BY_SANDBOX));
    if (!pExtra && abVal) {
        pExtra = new ExtraIgnoredBySandbox;
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
            pExtra = new ExtraRadioData;
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
    pExtra = new ExtraLight(apLight);
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
        pExtra = new ExtraTeleport(apTeleport);
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
        pExtra = new ExtraMapMarker(apMapData);
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
        pExtra = new ExtraAudioMarker(apAudioData);
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
        pExtra = new ExtraOwnership(apOwner);
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetGlobal(TESGlobal* apGlobal) {
    ExtraGlobal* extraData = static_cast<ExtraGlobal*>(BaseExtraList::GetExtraData(EXTRA_GLOBAL));
    if (extraData) {
        if (apGlobal) {
            extraData->pGlobal = apGlobal;
        }
        else {
            BaseExtraList::RemoveExtra(extraData, true);
        }
    }
    else if (apGlobal) {
        extraData = new ExtraGlobal(apGlobal);
        BaseExtraList::AddExtra(extraData);
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
    else if (aiRank != -1) {
        pRank = new ExtraRank(aiRank);
        AddExtra(pRank);
    }
}

void ExtraDataList::SetCell3D(NiNode* apObjectRoot) {
    ExtraCell3D* pExtra = static_cast<ExtraCell3D*>(GetExtraData(2));
    if (!apObjectRoot) {
        if (pExtra && pExtra->spCellNode == nullptr) {
            RemoveExtra(pExtra, true);
            pExtra = nullptr;
        }
        if (!pExtra) {
            return;
        }
    }
    if (!pExtra) {
        pExtra = new ExtraCell3D;
        AddExtra(pExtra);
    }
    pExtra->spCellNode = apObjectRoot;
}

void ExtraDataList::SetCanopyShadowMask(int32_t aeState, NiRenderedTexture* apTex, _D3DLOCKED_RECT*& apOutRect) {
    ExtraCellCanopyShadowMask* pExtra = static_cast<ExtraCellCanopyShadowMask*>(GetExtraData(10));
    if (aeState) {
        if (pExtra) {
            pExtra->eState = static_cast<ExtraCellCanopyShadowMask::CELL_STATE>(aeState);
            pExtra->spMask = apTex;
        }
        else {
            pExtra = new ExtraCellCanopyShadowMask(static_cast<ExtraCellCanopyShadowMask::CELL_STATE>(aeState), apTex);
            AddExtra(pExtra);
        }
        apOutRect = &pExtra->Rect;
    }
    else if (pExtra) {
        RemoveExtra(pExtra, true);
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
        pExtra = new ExtraTresPassPackage(apPack);
        AddExtra(pExtra);
    }
}

void ExtraDataList::AddRoomLinkedRoomRef(TESObjectREFR* apRef) {
    if (!GetRoomLinkedRefData() && apRef) {
        RoomRefData* pNewData = new RoomRefData;
        SetRoomLinkedRefData(pNewData);
    }

    RoomRefData* pData = GetRoomLinkedRefData();
    if (!pData->kRoomList.IsInList(apRef)) {
        pData->kRoomList.AddHead(apRef);
    }
}

void ExtraDataList::SetCollisionData(CollisionData* apCollisionData) {
    BSExtraData* pOldExtra = GetExtraData(114u);
    if (pOldExtra) {
        RemoveExtra(pOldExtra, true);
    }
    if (apCollisionData) {
        ExtraCollisionData* pExtra = new ExtraCollisionData;
        pExtra->pCollisionData = apCollisionData;
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetExtraSayTopicInfoOnceADayExtra(SayOnceTopicInfos* apInfo) {
    if (apInfo) {
        ExtraSayTopicInfoOnceADay* extraData = static_cast<ExtraSayTopicInfoOnceADay*>(
            BaseExtraList::GetExtraData(EXTRA_SAY_ONCE_A_DAY_TOPIC_INFO)
        );
        if (extraData) {
            extraData->pListofSaidOnceTopicInfos->AddHead(apInfo);
        }
        else {
            extraData = new ExtraSayTopicInfoOnceADay(apInfo);
            BaseExtraList::AddExtra(extraData);
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
            AudioBuoyMarkerData* pOldData = pExtra->pAudioBuoyData;
            if (pOldData) {
                delete pOldData;
            }
            pExtra->pAudioBuoyData = apAudioBuoyData;
        }
        else {
            RemoveExtra(pExtra, true);
            return nullptr;
        }
    }
    else if (apAudioBuoyData) {
        pExtra = new ExtraAudioBuoyMarker(apAudioBuoyData);
        AddExtra(pExtra);
    }
    return pExtra;
}

void ExtraDataList::SetHealth(float afHealth) {
    auto* pExtra = static_cast<ExtraHealth*>(GetExtraData(EXTRA_HEALTH));
    if (pExtra) {
        pExtra->fHealth = afHealth;
    }
    else {
        pExtra = new ExtraHealth(afHealth);
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetUses(uint8_t acUses) {
    auto* pExtra = static_cast<ExtraUses*>(GetExtraData(EXTRA_USES));
    if (pExtra) {
        pExtra->cUses = acUses;
        return;
    }
    pExtra = new ExtraUses(acUses);
    AddExtra(pExtra);
}

void ExtraDataList::SetTimeLeft(float afTimeLeft) {
    auto* pExtra = static_cast<ExtraTimeLeft*>(GetExtraData(EXTRA_TIMELEFT));
    if (pExtra) {
        pExtra->fTime = afTimeLeft;
        return;
    }
    pExtra = new ExtraTimeLeft(afTimeLeft);
    AddExtra(pExtra);
}

void ExtraDataList::SetCharge(float afCharge) {
    auto* pExtra = static_cast<ExtraCharge*>(GetExtraData(EXTRA_CHARGE));
    if (pExtra) {
        pExtra->fCharge = afCharge;
        return;
    }
    pExtra = new ExtraCharge(afCharge);
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
        pExtra = new ExtraSeed(aiSeed);
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetObjectHealth(float afHealth) {
    auto* pExtra = static_cast<ExtraObjectHealth*>(GetExtraData(EXTRA_OBJECT_HEALTH));
    if (pExtra) {
        pExtra->fHealth = afHealth;
        return;
    }
    pExtra = new ExtraObjectHealth(afHealth);
    AddExtra(pExtra);
}

void ExtraDataList::SetClimate(TESClimate* apClimate) {
    auto* pExtra = static_cast<ExtraCellClimate*>(GetExtraData(EXTRA_CLIMATE));
    if (apClimate) {
        if (pExtra) {
            pExtra->pClimate = apClimate;
        }
        else {
            pExtra = new ExtraCellClimate(apClimate);
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
    pExtra = new ExtraHasNoRumors(abVal);
    AddExtra(pExtra);
}

void ExtraDataList::SetDetachTime(uint32_t aiTime) {
    if (aiTime) {
        auto* pExtra = static_cast<ExtraDetachTime*>(GetExtraData(EXTRA_CELLDETACHTIME));
        if (pExtra) {
            pExtra->iTime = aiTime;
        }
        else {
            pExtra = new ExtraDetachTime;
            pExtra->iTime = aiTime;
            AddExtra(pExtra);
        }
    }
    else {
        RemoveExtra(EXTRA_CELLDETACHTIME);
    }
}

// Updates an existing extra and still adds a new one.
void ExtraDataList::SetOriginalReferenceExtra(TESObjectREFR* apRef) {
    auto* pExtra = static_cast<ExtraOriginalReference*>(GetExtraData(EXTRA_ORIGINAL_REFERENCE));
    if (pExtra) {
        pExtra->pReference = apRef;
    }
    pExtra = new ExtraOriginalReference(apRef);
    AddExtra(pExtra);
}

void ExtraDataList::SetInfoGeneralTopic(MenuTopic* apItem) {
    auto* pExtra = static_cast<ExtraInfoGeneralTopic*>(GetExtraData(EXTRA_INFO_GENERAL_TOPIC));
    if (pExtra) {
        pExtra->pInfoGen = apItem;
        return;
    }
    pExtra = new ExtraInfoGeneralTopic(apItem);
    AddExtra(pExtra);
}

void ExtraDataList::SetModelSwap(TESModel* apModelSwap, TESForm* apModelSwapForm) {
    auto* pExtra = static_cast<ExtraModelSwap*>(GetExtraData(EXTRA_MODEL_SWAP));
    if (pExtra) {
        pExtra->pModelSwap = apModelSwap;
        pExtra->pModelSwapForm = apModelSwapForm;
    }
    else {
        pExtra = new ExtraModelSwap(apModelSwap, apModelSwapForm);
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
        pExtra = new ExtraScale(afScale);
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetRefractionPropertyExtra(bool abflag, float afRefractionPower) {
    auto* pExtra = static_cast<ExtraRefractionProperty*>(GetExtraData(EXTRA_REFRACTION_PROPERTY));
    if (abflag) {
        if (!pExtra) {
            pExtra = new ExtraRefractionProperty(afRefractionPower);
            AddExtra(pExtra);
        }
        pExtra->fRefractionPower = afRefractionPower;
    }
    else if (pExtra) {
        RemoveExtra(pExtra, true);
    }
}

// An existing extra, or a zero slot without one, gets SetWeaponModActive; it is called on the null extra too,
// deliberately.
void ExtraDataList::SetWeaponModSlot(uint8_t slot) {
    auto* pExtra = static_cast<ExtraWeaponModFlags*>(GetExtraData(EXTRA_WEAPON_MOD_SLOTS));
    if (pExtra || !slot) {
        pExtra->SetWeaponModActive(slot);
    }
    else {
        pExtra = new ExtraWeaponModFlags(slot);
        pExtra->SetWeaponModActive(slot);
        AddExtra(pExtra);
    }
}

void ExtraDataList::SetWeaponModFlags(uint8_t flags) {
    auto* pExtra = static_cast<ExtraWeaponModFlags*>(GetExtraData(EXTRA_WEAPON_MOD_SLOTS));
    if (pExtra) {
        pExtra->SetWeaponModFlags(flags);
    }
    else {
        pExtra = new ExtraWeaponModFlags;
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
            pExtra = new ExtraHeadingTarget(apRef);
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

void ExtraDataList::SetActivateParentOnly(bool abVal) {
    auto* pExtra = static_cast<ExtraActivateRef*>(GetExtraData(EXTRA_ACTIVATE_REF));
    if (!pExtra && abVal) {
        pExtra = new ExtraActivateRef;
        AddExtra(pExtra);
    }
    if (pExtra) {
        if (abVal) {
            pExtra->cActivateFlags |= ExtraActivateRef::FLAG_PARENT_ACTIVATE_ONLY;
        }
        else {
            pExtra->cActivateFlags &= ~ExtraActivateRef::FLAG_PARENT_ACTIVATE_ONLY;
        }
    }
}

void ExtraDataList::SetIsModding(bool bModding) {
    auto* pExtra = static_cast<ExtraModdingItem*>(GetExtraData(EXTRA_WEAPON_IS_MODDING));
    if (pExtra) {
        pExtra->SetItemModding(bModding);
    }
    else {
        pExtra = new ExtraModdingItem(bModding);
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
            pExtra = new ExtraAmmo(apAmmoType, aiCount);
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

bool ExtraDataList::IsActivateParentOnly() {
    auto* pExtra = static_cast<ExtraActivateRef*>(GetExtraData(EXTRA_ACTIVATE_REF));
    return pExtra && (pExtra->cActivateFlags & ExtraActivateRef::FLAG_PARENT_ACTIVATE_ONLY) != 0;
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
        pExtra = new ExtraCreatureAwakeSound(afSound);
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
        pExtra = new ExtraSound(afSound);
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
        pExtra = new ExtraPackageStartLocation(apWorld, apIntCell, aPt, afzrotation);
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
            pExtra = new ExtraHealthPerc;
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
            pExtra = new ExtraHavok(apWorld);
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

// A null apMopp with no havok extra is stored through a null extra, deliberately.
void ExtraDataList::SetCellMopp(CellMopp* apMopp) {
    auto* pExtra = static_cast<ExtraHavok*>(GetExtraData(EXTRA_HAVOK));
    if (apMopp && !pExtra) {
        pExtra = new ExtraHavok(nullptr);
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
        pExtra = new ExtraCellMusicType(apType);
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
        pExtra = new ExtraCellAcousticSpace(apSpace);
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
        pExtra = new ExtraCellImageSpace(apImageSpace);
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
        pExtra = new ExtraEditorID(apEditorID);
        AddExtra(pExtra);
    }
    else {
        pExtra->cEditorID.Set(apEditorID, 0);
    }
}

void ExtraDataList::SetLevelItemExtra(int32_t aiIndex) {
    auto* pExtra = static_cast<ExtraLeveledItem*>(GetExtraData(EXTRA_LEVELITEM));
    if (!pExtra) {
        pExtra = new ExtraLeveledItem(aiIndex);
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
        pExtra = new ExtraRagDollData;
        RagDollData* pData = new RagDollData;
        pExtra->pRagDollData = pData;
        pExtra->pRagDollData->UpdateDataFromReference(apRef);
        AddExtra(pExtra);
    }
}

// Until InitItem runs, the form pointers of a loaded list hold form IDs; each case below turns them into forms. Warning
// is an empty stub. The next extra is read before the case runs, so a removed extra does not end the walk; the portal
// case goes on reading its extra after removing it.
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
                    pData->pMediaLocationController = reinterpret_cast<MediaLocationController*>(
                        static_cast<uintptr_t>(iID)
                    );
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
                        "MASTERFILE: Unable to find ownership condition global %08X. Ownership will be removed.",
                        iID
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
                        "MASTERFILE: Unable to find cell acoustic space %08X. AcousticSpace data will be removed.",
                        iID
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
                        "MASTERFILE: Unable to find linked reference %08X. Linked reference data will be removed.",
                        iID
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
                    // The stored ID is discarded and 0 is what gets resolved.
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
                            apOwner->iFormID,
                            pX->pData->pLinkedRefs[0]->iFormID
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
                    TESObjectREFR* pRef = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(iID));
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
                        pNode->SetItem(pRef);
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
                        "MASTERFILE: Unable to find XMarker target %08X. XMarker target data will be removed.",
                        iID
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
                        "MASTERFILE: Unable to find emittance source %08X. Emittance source data will be removed.",
                        iID
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
                        iID,
                        apOwner->iFormID
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
        pSound = new ExtraWeaponAttackSound(afSound);
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
    ExtraOcclusionPlaneRefData* extraData = static_cast<ExtraOcclusionPlaneRefData*>(
        BaseExtraList::GetExtraData(EXTRA_OCCLUSION_PLANE_REF_DATA)
    );
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
        int32_t irefID = apRef->iFormID;
        Warning("FORMS: Reference %s %08X has no parent save cell", apRef->GetFormEditorID(), irefID);
    }
}

void ExtraDataList::AddRoomLinkedPortalRef(TESObjectREFR* apRef) {
    if (!GetRoomLinkedRefData() && apRef) {
        RoomRefData* pNewData;
        pNewData = new RoomRefData;
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
            pextra = new ExtraPortal;
            pextra->spPortal = apPortal;
            AddExtra(pextra);
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
            pExtra = new ExtraRoom;
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
    apVisited_t kVisited(37u);
    return GetMasterRoomRecurse(&kVisited);
}

void ExtraDataList::SetOcclusionPlane(BSOcclusionPlane* apPlane) {
    if (apPlane) {
        ExtraOcclusionPlane* pextra = static_cast<ExtraOcclusionPlane*>(GetExtraData(113));
        if (pextra) {
            pextra->spPlane = apPlane;
        }
        else {
            pextra = new ExtraOcclusionPlane;
            pextra->spPlane = apPlane;
            AddExtra(pextra);
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
        pSound = new ExtraCreatureMovementSound(afSound);
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
    uint8_t bIsActor = 0;
    if (apRef) {
        bIsActor = apRef->IsActor();
    }
    if (bIsActor) {
        TESObjectCELL* pPersistentCell = GetPersistentCell();
        if (pPersistentCell && apRef->TESObjectREFR::GetParentCell()) {
            SetPersistentCell(nullptr);
            pPersistentCell->RemoveReference(apRef);
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
                case 33: {
                    bOwnerShip = true;
                    [[fallthrough]];
                }
                case 13:
                case 28:
                case 32:
                case 36:
                case 39:
                case 47:
                case 48:
                case 74: {
                    pHead = pHead->GetNext();
                    continue;
                }
                default: {
                    if (bOwnerShip) {
                        bOwnerShip = false;
                    }
                    break;
                }
            }
        }
        break;
    }
    BaseExtraList::ExtraCritSection.Unlock();
    return bOwnerShip;
}

uint8_t ExtraDataList::HasWeaponMods() {
    ExtraWeaponModFlags* extraData = static_cast<ExtraWeaponModFlags*>(
        BaseExtraList::GetExtraData(EXTRA_WEAPON_MOD_SLOTS)
    );
    return extraData && extraData->HasWeaponMods();
}

void ExtraDataList::SetPoison(AlchemyItem* apPoison) {
    auto* pExtra = static_cast<ExtraPoison*>(GetExtraData(EXTRA_POISON));
    if (pExtra) {
        pExtra->pPoison = apPoison;
        return;
    }
    pExtra = new ExtraPoison(apPoison);
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
        extraData->bActionComplete = abFlag;
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
    ExtraSayToTopicInfo* extraData = static_cast<ExtraSayToTopicInfo*>(
        BaseExtraList::GetExtraData(EXTRA_SAY_TO_TOPIC_INFO)
    );
    if (extraData) {
        extraData->nResponseID = anResponseID;
    }
}

void ExtraDataList::SetExtraSayToInfoExtra(TESTopicInfo* apInfo) {
    if (apInfo) {
        ExtraSayToTopicInfo* extraData = static_cast<ExtraSayToTopicInfo*>(
            BaseExtraList::GetExtraData(EXTRA_SAY_TO_TOPIC_INFO)
        );
        if (extraData) {
            extraData->pInfo = apInfo;
        }
        else {
            extraData = new ExtraSayToTopicInfo(apInfo);
            BaseExtraList::AddExtra(extraData);
        }
    }
    else {
        RemoveSayToInfoExtra();
    }
}

void ExtraDataList::SetExtraSayToTopicOnExtra(TESTopic* apTopic) {
    if (apTopic) {
        ExtraSayToTopicInfo* pExtra = static_cast<ExtraSayToTopicInfo*>(
            BaseExtraList::GetExtraData(EXTRA_SAY_TO_TOPIC_INFO)
        );
        if (!pExtra) {
            pExtra = new ExtraSayToTopicInfo;
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
    return extraData && static_cast<ExtraEnableStateParent*>(extraData)->ShouldPopIn();
}

bool ExtraDataList::ShouldPopOutWhenDisabledByParent() {
    BSExtraData* extraData = BaseExtraList::GetExtraData(EXTRA_ENABLESTATEPARENT);
    return extraData && static_cast<ExtraEnableStateParent*>(extraData)->ShouldPopOut();
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
    ExtraTalkingActor* extraData = static_cast<ExtraTalkingActor*>(BaseExtraList::GetExtraData(EXTRA_TALKING_ACTOR));
    if (extraData) {
        BaseExtraList::RemoveExtra(extraData, true);
    }
    ExtraTalkingActor* pExtra = new ExtraTalkingActor(apTalkAct, static_cast<PlayerCharacter*>(apRef));
    BaseExtraList::AddExtra(pExtra);
}

void ExtraDataList::SetPackageExtra(
    TESPackage* apPack,
    int32_t aiIndex,
    TESObjectREFR* apRef,
    bool abcomplete,
    bool abactivated,
    bool abDoneOnce
) {
    if (!apPack ||
        ((!apPack->GetIsCreated() || apPack->GetPackType() == PACKAGE_TYPE_FOLLOW) &&
         !apPack->IsNeverToRun() &&
         apPack->GetPackType() != PACKAGE_TYPE_SPECTATOR &&
         apPack->GetPackType() != PACKAGE_TYPE_TRESPASS)) {
        ExtraPackage* extraData = static_cast<ExtraPackage*>(BaseExtraList::GetExtraData(EXTRA_PACKAGE));
        if (apPack) {
            if (extraData) {
                extraData->pPack = apPack;
                extraData->iindex = aiIndex;
                extraData->pTarg = apRef;
                extraData->bActionComplete = abcomplete;
                extraData->bActivated = abactivated;
                extraData->bDoneOnce = abDoneOnce;
            }
            else {
                extraData = new ExtraPackage(apPack, aiIndex, apRef, abcomplete, abactivated, abDoneOnce);
                BaseExtraList::AddExtra(extraData);
            }
        }
        else if (extraData) {
            RemovePackageExtra();
        }
    }
}

void ExtraDataList::SetDistantDataNormal(const NiPoint3& aNormal) {
    ExtraDistantData* extraData = static_cast<ExtraDistantData*>(BaseExtraList::GetExtraData(EXTRA_DISTANTDATA));
    if (!extraData) {
        extraData = new ExtraDistantData;
        BaseExtraList::AddExtra(extraData);
    }
    extraData->kNormal = aNormal;
}

void ExtraDataList::SetLinkedPlaneRef(TESObjectREFR* apRef, int32_t aiEdge) {
    if (!GetOcclusionPlaneLinkedRefData() && apRef) {
        OcclusionPlaneLinkedRefData* pNewData = new OcclusionPlaneLinkedRefData;
        SetOcclusionPlaneLinkedRefData(pNewData);
    }

    OcclusionPlaneLinkedRefData* pData = GetOcclusionPlaneLinkedRefData();
    if (pData) {
        pData->pLinkedRefs[aiEdge] = apRef;
    }
}

void ExtraDataList::SetPortalLinkedRoomRef(int32_t aiWhich, TESObjectREFR* apRef) {
    if (!GetPortalLinkedRefData() && apRef) {
        PortalLinkedRefData* pNewData = new PortalLinkedRefData;
        SetPortalLinkedRefData(pNewData);
    }

    PortalLinkedRefData* pData = GetPortalLinkedRefData();
    if (pData) {
        pData->pLinkedRefs[aiWhich] = apRef;
    }
    BSPortal* pPortal = GetPortal();
    TESObjectREFR* pMasterRoom = nullptr;
    if (apRef) {
        pMasterRoom = apRef->TESObjectREFR::GetExtra()->GetMasterRoom();
    }
    TESObjectREFR* pRoomRef = pMasterRoom;
    if (!pMasterRoom) {
        pRoomRef = apRef;
    }
    BSMultiBoundRoom* pRoom = nullptr;
    if (pRoomRef) {
        pRoom = pRoomRef->TESObjectREFR::GetExtra()->GetRoom();
    }
    if (pPortal) {
        if (!aiWhich) {
            pPortal->SetMultiBoundRoomLeft(pRoom);
        }
        if (aiWhich == 1) {
            pPortal->SetMultiBoundRoomRight(pRoom);
        }
    }
}

void ExtraDataList::SetActorPackageData(ActorPackageData* apData) {
    ExtraPackageData* extraData = static_cast<ExtraPackageData*>(BaseExtraList::GetExtraData(EXTRA_PACKAGE_DATA));
    if (extraData) {
        extraData->pActorPackageData = apData;
    }
    else {
        extraData = new ExtraPackageData(apData);
        BaseExtraList::AddExtra(extraData);
    }
}

int32_t ExtraDataList::AddFriendHit() {
    ExtraFriendHits* extraData = static_cast<ExtraFriendHits*>(BaseExtraList::GetExtraData(EXTRA_FRIEND_HITS));
    if (!extraData) {
        extraData = new ExtraFriendHits;
        BaseExtraList::AddExtra(extraData);
    }
    extraData->AddHit();
    return extraData->GetHitCount();
}

void ExtraDataList::RevertUnsavedExtra(BGSLoadFormBuffer* apLoadGameBuffer) {
    BGSChangeFlags kOldFlags;
    RemoveSavedAnimation();
    RemoveSavedHavokData();
    RemoveLastFinishedSequence();
    kOldFlags = apLoadGameBuffer->GetOldChangeFlags();
    if (kOldFlags.CheckFlags(CHANGE_REFR_EXTRA_GAME_ONLY)) {
        BaseExtraList::RemoveExtra(EXTRA_USEDMARKERS);
    }
}

void ExtraDataList::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* pref = apLoadGameBuffer->GetReference();
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[117])) {
        ExtraSayToTopicInfo* pextraSayToTopicInfo = static_cast<ExtraSayToTopicInfo*>(GetExtraData(117));
        if (pextraSayToTopicInfo && pref) {
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
    if (pref && apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[51])) {
        NonActorMagicTarget* pextraNonActorMagicTarget = static_cast<NonActorMagicTarget*>(GetExtraData(51));
        if (pextraNonActorMagicTarget) {
            ActiveEffect::FinishLoadActiveEffectList(apLoadGameBuffer, &pextraNonActorMagicTarget->lActiveEffectList);
        }
    }
}

ExtraDismemberedLimbs* ExtraDataList::AddDismembermentExtra() {
    ExtraDismemberedLimbs* extraData = static_cast<ExtraDismemberedLimbs*>(
        BaseExtraList::GetExtraData(EXTRA_DISMEMBERED_LIMBS)
    );
    if (!extraData) {
        extraData = new ExtraDismemberedLimbs;
        BaseExtraList::AddExtra(extraData);
    }
    return extraData;
}

void ExtraDataList::SetActorCauseExtra(ActorCause* apActorCause) {
    if (apActorCause) {
        ExtraActorCause* extraData = static_cast<ExtraActorCause*>(BaseExtraList::GetExtraData(EXTRA_ACTOR_CAUSE));
        if (extraData) {
            extraData->SetActorCause(apActorCause);
        }
        else {
            extraData = new ExtraActorCause;
            extraData->SetActorCause(apActorCause);
            BaseExtraList::AddExtra(extraData);
        }
    }
    else {
        BaseExtraList::RemoveExtra(EXTRA_ACTOR_CAUSE);
    }
}

ExtraFollowerSwimBreadcrumbs* ExtraDataList::GetOrCreateExtraFollowerSwimBreadcrumbs() {
    ExtraFollowerSwimBreadcrumbs* extraData = static_cast<ExtraFollowerSwimBreadcrumbs*>(
        BaseExtraList::GetExtraData(EXTRA_FOLLOWER_SWIM_BREADCRUMBS)
    );
    if (extraData) {
        return extraData;
    }
    extraData = new ExtraFollowerSwimBreadcrumbs;
    BaseExtraList::AddExtra(extraData);
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
    pExtra = new ExtraAction;
    AddExtra(pExtra);
    return pExtra;
}

ExtraStartingPosition* ExtraDataList::SetStartingPositionPtr(TESObjectREFR* apObject) {
    auto* pExtra = static_cast<ExtraStartingPosition*>(GetExtraData(EXTRA_STARTINGPOSITION));
    if (pExtra) {
        return pExtra;
    }
    pExtra = new ExtraStartingPosition(apObject);
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
    BaseExtraList::ExtraCritSection.Lock("ExtraDataList::CopyListForContainer()");
    BSExtraData* pExtra = apCopy->pHead;
    while (pExtra) {
        int8_t bAdvance = 1;
        switch (pExtra->GetExtraType()) {
            case 0xDu: {
                AddExtraCopy(pExtra);
                if (!abDontRemove) {
                    apCopy->RemoveExtra(pExtra, false);
                    pExtra = apCopy->pHead;
                    bAdvance = 0;
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
                AddExtraCopy(pExtra);
                if (pExtra->GetExtraType() != 48 && !abDontRemove) {
                    apCopy->RemoveExtra(pExtra, true);
                    pExtra = apCopy->pHead;
                    bAdvance = 0;
                }
                break;
            }
            default: {
                break;
            }
        }
        if (pExtra && bAdvance) {
            pExtra = pExtra->pNext;
        }
    }
    BaseExtraList::ExtraCritSection.Unlock();
}

void ExtraDataList::ClearTrespassPackage(TESPackage* apPackage) {
    ExtraTresPassPackage* pExtra = static_cast<ExtraTresPassPackage*>(
        BaseExtraList::GetExtraData(EXTRA_TRESPASS_PACKAGE)
    );
    if (pExtra && pExtra->pPack == apPackage) {
        pExtra->pPack = nullptr;
    }
}

void ExtraDataList::AddGuard(Actor* apActor) {
    ExtraGuardedRefData* pExtra = static_cast<ExtraGuardedRefData*>(
        BaseExtraList::GetExtraData(EXTRA_GUARDED_REF_DATA)
    );
    if (!pExtra) {
        pExtra = new ExtraGuardedRefData;
        BaseExtraList::AddExtra(pExtra);
    }
    pExtra->AddGuard(apActor);
}

ExtraNavMeshPortal* ExtraDataList::GetNavMeshPortal() {
    return static_cast<ExtraNavMeshPortal*>(BaseExtraList::GetExtraData(EXTRA_NAVMESH_PORTAL));
}

void ExtraDataList::DuplicateExtraListForContainer(ExtraDataList* apCopy) {
    BSExtraData* i;
    BaseExtraList::ExtraCritSection.Lock("ExtraDataList::DuplicateExtraListForContainer()");
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
    BaseExtraList::ExtraCritSection.Unlock();
}

void ExtraDataList::LoadGameBGS(BGSLoadFormBuffer* apLoadGameBuffer) {
    int32_t iactorBaseChangeFlags;
    uint8_t cflags;
    char cpackDataType;
    uint32_t icrimeNumber;
    PackageNS::CRIME_TYPE ecrimeType;
    Crime* pcrime;
    Actor* pForm;
    int32_t iindex;
    uint8_t csavedType;
    bool bexistingExtra[156];
    TESObjectREFR* pref__pactor = apLoadGameBuffer->GetReference();
    Actor* pactor = apLoadGameBuffer->GetActor();
    ourMemset(bexistingExtra, 0, 147u);
    for (BSExtraData* pextra = this->pHead; pextra; pextra = pextra->pNext) {
        uint8_t cextraType = pextra->cEtype;
        if (xDataTypeToChangeFlag[cextraType] &&
            apLoadGameBuffer->GetChangeFlags().CheckFlags(xDataTypeToChangeFlag[cextraType])) {
            bexistingExtra[cextraType] = true;
        }
    }
    uint32_t iextraCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < iextraCount; ++i) {
        int32_t iactorCauseID;
        uint32_t ioldFormID;
        csavedType = 0;
        apLoadGameBuffer->LoadData(&csavedType, sizeof(csavedType));
        switch (csavedType) {
            case 13: {
                ExtraScript* pextraScript;
                if (bexistingExtra[csavedType]) {
                    pextraScript = static_cast<ExtraScript*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraScript* pNewExtra = new ExtraScript;
                    pextraScript = static_cast<ExtraScript*>(this->AddExtra(pNewExtra));
                }
                TESForm* pLoadedForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
                pextraScript->pScript = dynamic_cast<Script*>(pLoadedForm);
                if (!pextraScript->pScriptVars) {
                    ScriptLocals* pScriptVars = nullptr;
                    if (pextraScript->pScript) {
                        pScriptVars = pextraScript->pScript->CloneLocals();
                    }
                    pextraScript->pScriptVars = pScriptVars;
                }
                if (!pref__pactor) {
                    apLoadGameBuffer->SetForm(InventoryChanges::GetTempRef());
                }
                if (pextraScript->pScriptVars) {
                    pextraScript->pScriptVars->LoadGame(apLoadGameBuffer);
                }
                else {
                    ScriptLocals tempLocals;
                    tempLocals.LoadGame(apLoadGameBuffer);
                }
                if (!pref__pactor) {
                    apLoadGameBuffer->pForm = nullptr;
                }
                break;
            }
            case 22: {
                this->SetWorn(true, false);
                break;
            }
            case 24: {
                ExtraPackageStartLocation* pextraPackageStartLoc;
                if (bexistingExtra[csavedType]) {
                    pextraPackageStartLoc = static_cast<ExtraPackageStartLocation*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraPackageStartLocation* pNewExtra = new ExtraPackageStartLocation;
                    pextraPackageStartLoc = static_cast<ExtraPackageStartLocation*>(this->AddExtra(pNewExtra));
                }
                TESForm* pLoadedForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
                pextraPackageStartLoc->worldLoc.pLocationForm = pLoadedForm;
                apLoadGameBuffer->LoadData(
                    &pextraPackageStartLoc->worldLoc.locPt,
                    sizeof(pextraPackageStartLoc->worldLoc.locPt)
                );
                apLoadGameBuffer->LoadData(
                    &pextraPackageStartLoc->worldLoc.fZRot,
                    sizeof(pextraPackageStartLoc->worldLoc.fZRot)
                );
                break;
            }
            case 25: {
                ExtraPackage* pextraPackage;
                if (bexistingExtra[csavedType]) {
                    pextraPackage = static_cast<ExtraPackage*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraPackage* pNewExtra = new ExtraPackage;
                    pextraPackage = static_cast<ExtraPackage*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadFormID(pextraPackage->pPack);
                apLoadGameBuffer->LoadFormID(pextraPackage->pTarg);
                apLoadGameBuffer->LoadData(&pextraPackage->iindex, sizeof(pextraPackage->iindex));
                apLoadGameBuffer->LoadData(&pextraPackage->bActionComplete, sizeof(pextraPackage->bActionComplete));
                apLoadGameBuffer->LoadData(&pextraPackage->bActivated, sizeof(pextraPackage->bActivated));
                apLoadGameBuffer->LoadData(&pextraPackage->bDoneOnce, sizeof(pextraPackage->bDoneOnce));
                break;
            }
            case 26: {
                ExtraTresPassPackage* pextraTrespassPackage;
                if (bexistingExtra[csavedType]) {
                    pextraTrespassPackage = static_cast<ExtraTresPassPackage*>(this->GetExtraData(csavedType));
                }
                else {
                    BSExtraData* pNewExtra = new ExtraTresPassPackage;
                    pextraTrespassPackage = static_cast<ExtraTresPassPackage*>(this->AddExtra(pNewExtra));
                }
                ioldFormID = 0;
                apLoadGameBuffer->LoadFormID(ioldFormID);
                if (ioldFormID) {
                    pextraTrespassPackage->pPack = static_cast<TrespassPackage*>(
                        TESPackage::CreatePackage(PACKAGE_TYPE_TRESPASS)
                    );
                    pSaveLoadGame->AddChangedFormID(ioldFormID, pextraTrespassPackage->pPack->iFormID);
                    pextraTrespassPackage->pPack->LoadGame(apLoadGameBuffer);
                }
                break;
            }
            case 27: {
                ExtraRunOncePacks* pextraRunOncePackages;
                if (bexistingExtra[csavedType]) {
                    pextraRunOncePackages = static_cast<ExtraRunOncePacks*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraRunOncePacks* pNewExtra = new ExtraRunOncePacks;
                    pextraRunOncePackages = static_cast<ExtraRunOncePacks*>(this->AddExtra(pNewExtra));
                }
                uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
                for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
                    RunOncePackage* prunOncePackage = static_cast<RunOncePackage*>(ourMalloc(sizeof(RunOncePackage)));
                    TESForm* pLoadedForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
                    prunOncePackage->pPackage = dynamic_cast<TESPackage*>(pLoadedForm);
                    apLoadGameBuffer->LoadData(&prunOncePackage->cday, sizeof(prunOncePackage->cday));
                    pextraRunOncePackages->pPackageList->AddHead(prunOncePackage);
                }
                break;
            }
            case 28: {
                ExtraReferencePointer* pextraReferencePointer;
                if (bexistingExtra[csavedType]) {
                    pextraReferencePointer = static_cast<ExtraReferencePointer*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraReferencePointer* pNewExtra = new ExtraReferencePointer;
                    pextraReferencePointer = static_cast<ExtraReferencePointer*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadFormID(pextraReferencePointer->pRef);
                break;
            }
            case 29: {
                ExtraFollower* pextraFollower;
                if (bexistingExtra[csavedType]) {
                    pextraFollower = static_cast<ExtraFollower*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraFollower* pNewExtra = new ExtraFollower;
                    pextraFollower = static_cast<ExtraFollower*>(this->AddExtra(pNewExtra));
                }
                uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
                for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
                    pForm = nullptr;
                    apLoadGameBuffer->LoadFormID(pForm);
                    pextraFollower->pActorlist->AddHead(pForm);
                }
                break;
            }
            case 30: {
                ExtraLevCreaModifier* pextraLevCreaModifier;
                if (bexistingExtra[csavedType]) {
                    pextraLevCreaModifier = static_cast<ExtraLevCreaModifier*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraLevCreaModifier* pNewExtra = new ExtraLevCreaModifier;
                    pextraLevCreaModifier = static_cast<ExtraLevCreaModifier*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(&pextraLevCreaModifier->eModifier, sizeof(pextraLevCreaModifier->eModifier));
                break;
            }
            case 31: {
                this->SetGhost(true);
                break;
            }
            case 33: {
                ExtraOwnership* pextraOwner;
                if (bexistingExtra[csavedType]) {
                    pextraOwner = static_cast<ExtraOwnership*>(this->GetExtraData(csavedType));
                }
                else {
                    BSExtraData* pNewExtra = new ExtraOwnership;
                    pextraOwner = static_cast<ExtraOwnership*>(this->AddExtra(pNewExtra));
                }
                TESForm* pLoadedForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
                pextraOwner->pOwner = pLoadedForm;
                break;
            }
            case 34: {
                ExtraGlobal* pextraGlobal;
                if (bexistingExtra[csavedType]) {
                    pextraGlobal = static_cast<ExtraGlobal*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraGlobal* pNewExtra = new ExtraGlobal;
                    pextraGlobal = static_cast<ExtraGlobal*>(this->AddExtra(pNewExtra));
                }
                TESForm* pLoadedForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
                pextraGlobal->pGlobal = dynamic_cast<TESGlobal*>(pLoadedForm);
                break;
            }
            case 35: {
                ExtraRank* pextraRank;
                if (bexistingExtra[csavedType]) {
                    pextraRank = static_cast<ExtraRank*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraRank* pNewExtra = new ExtraRank;
                    pextraRank = static_cast<ExtraRank*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(&pextraRank->iRank, sizeof(pextraRank->iRank));
                break;
            }
            case 36: {
                ExtraCount* pextraCount;
                if (bexistingExtra[csavedType]) {
                    pextraCount = static_cast<ExtraCount*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraCount* pNewExtra = new ExtraCount;
                    pextraCount = static_cast<ExtraCount*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(&pextraCount->iCount, sizeof(pextraCount->iCount));
                break;
            }
            case 37: {
                ExtraHealth* pextraHealth;
                if (bexistingExtra[csavedType]) {
                    pextraHealth = static_cast<ExtraHealth*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraHealth* pNewExtra = new ExtraHealth;
                    pextraHealth = static_cast<ExtraHealth*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(&pextraHealth->fHealth, sizeof(pextraHealth->fHealth));
                break;
            }
            case 38: {
                ExtraUses* pextraUses;
                if (bexistingExtra[csavedType]) {
                    pextraUses = static_cast<ExtraUses*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraUses* pNewExtra = new ExtraUses;
                    pextraUses = static_cast<ExtraUses*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(&pextraUses->cUses, sizeof(pextraUses->cUses));
                break;
            }
            case 39: {
                ExtraTimeLeft* pextraTimeLeft;
                if (bexistingExtra[csavedType]) {
                    pextraTimeLeft = static_cast<ExtraTimeLeft*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraTimeLeft* pNewExtra = new ExtraTimeLeft;
                    pextraTimeLeft = static_cast<ExtraTimeLeft*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(&pextraTimeLeft->fTime, sizeof(pextraTimeLeft->fTime));
                break;
            }
            case 40: {
                ExtraCharge* pextraCharge;
                if (bexistingExtra[csavedType]) {
                    pextraCharge = static_cast<ExtraCharge*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraCharge* pNewExtra = new ExtraCharge;
                    pextraCharge = static_cast<ExtraCharge*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(&pextraCharge->fCharge, sizeof(pextraCharge->fCharge));
                break;
            }
            case 42: {
                ExtraLock* pextraLock;
                if (bexistingExtra[csavedType]) {
                    pextraLock = static_cast<ExtraLock*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraLock* pNewExtra = new ExtraLock;
                    pextraLock = static_cast<ExtraLock*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(pextraLock->pLock, 1u);
                apLoadGameBuffer->LoadData(&pextraLock->pLock->cFlags, sizeof(pextraLock->pLock->cFlags));
                TESForm* pLoadedForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
                pextraLock->pLock->pKey = dynamic_cast<TESKey*>(pLoadedForm);
                if (apLoadGameBuffer->GetVersion() >= 16u) {
                    apLoadGameBuffer->LoadData(&pextraLock->pLock->iNumTries, sizeof(pextraLock->pLock->iNumTries));
                    apLoadGameBuffer->LoadData(
                        &pextraLock->pLock->iTimesUnlocked,
                        sizeof(pextraLock->pLock->iTimesUnlocked)
                    );
                }
                break;
            }
            case 43: {
                ExtraTeleport* pextraTeleport;
                if (bexistingExtra[csavedType]) {
                    pextraTeleport = static_cast<ExtraTeleport*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraTeleport* pNewExtra = new ExtraTeleport;
                    pextraTeleport = static_cast<ExtraTeleport*>(this->AddExtra(pNewExtra));
                }
                if (!pextraTeleport->pTeleportData) {
                    pextraTeleport->pTeleportData = new DoorTeleportData;
                }
                pextraTeleport->pTeleportData->LoadGame(apLoadGameBuffer);
                break;
            }
            case 44: {
                cflags = 0;
                apLoadGameBuffer->LoadData(&cflags, sizeof(cflags));
                BSExtraData* pExtra = bexistingExtra[csavedType] ? this->GetExtraData(csavedType) : nullptr;
                ExtraMapMarker* pextraMapMarker = static_cast<ExtraMapMarker*>(pExtra);
                if (pExtra && pextraMapMarker->pMapData) {
                    pextraMapMarker->pMapData->SetFlags(cflags);
                }

                break;
            }
            case 46: {
                ExtraLeveledCreature* pleveledCreature;
                if (bexistingExtra[csavedType]) {
                    pleveledCreature = static_cast<ExtraLeveledCreature*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraLeveledCreature* pNewExtra = new ExtraLeveledCreature;
                    pleveledCreature = static_cast<ExtraLeveledCreature*>(this->AddExtra(pNewExtra));
                }
                TESActorBase* pcurrentOriginalBase = pleveledCreature->pOriginalBase;
                TESActorBase* pcurrentTemplate = pleveledCreature->pTemplate;
                TESForm* pLoadedForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
                pleveledCreature->pOriginalBase = dynamic_cast<TESActorBase*>(pLoadedForm);
                pLoadedForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
                pleveledCreature->pTemplate = dynamic_cast<TESActorBase*>(pLoadedForm);
                iactorBaseChangeFlags = 0;
                apLoadGameBuffer->LoadData(&iactorBaseChangeFlags, sizeof(iactorBaseChangeFlags));
                if ((
                        pactor && pactor->Get3D() && pcurrentTemplate && pleveledCreature->pTemplate != pcurrentTemplate
                    ) ||
                    !pSaveLoadGame->GetSaveGameLoading()) {
                    pactor->Set3D(nullptr, false);
                }
                TESActorBase* pnewActorBase = nullptr;
                bool bcreatedTempActorBase = false;
                bool bresetInventory = false;
                if (pcurrentTemplate != pleveledCreature->pTemplate &&
                    !apLoadGameBuffer->GetChangeFlags().CheckFlags(
                        CHANGE_REFR_LEVELED_INVENTORY | CHANGE_REFR_INVENTORY
                    )) {
                    bresetInventory = true;
                }

                if (bresetInventory) {
                    pactor->m_Extra.RemoveContainerChanges();
                    pactor->ClearProcessItems();
                }
                if (pactor &&
                    pleveledCreature->pOriginalBase &&
                    pleveledCreature->pTemplate &&
                    pleveledCreature->pOriginalBase->GetUsesLeveledTemplate()) {
                    pnewActorBase = TESActorBaseData::CreateTemplateActorBase(
                        pleveledCreature->pOriginalBase,
                        pleveledCreature->pTemplate
                    );
                    TESActorBase* pcurrentActorBase = pactor->GetActorBase();
                    if (pDataHandler->IsFormIDCreated(pcurrentActorBase->iFormID)) {
                        delete pcurrentActorBase;
                    }
                    pactor->SetObjectReference(pnewActorBase);
                }
                else if (pcurrentOriginalBase && pcurrentTemplate) {
                    pleveledCreature->pOriginalBase = pcurrentOriginalBase;
                    pleveledCreature->pTemplate = pcurrentTemplate;
                }
                else {
                    this->RemoveExtra(46);
                }
                if (bresetInventory) {
                    InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(pactor);
                    pinventory->InitLeveledItems();
                    pinventory->InitFromContainerExtra();
                    pinventory->InitScripts();
                    if (pinventory->IsEmpty()) {
                        pactor->m_Extra.RemoveContainerChanges();
                    }
                }
                if (!pnewActorBase) {
                    if (pactor->IsNpc()) {
                        pnewActorBase = new TESNPC;
                    }
                    else {
                        pnewActorBase = new TESCreature;
                    }
                    bcreatedTempActorBase = true;
                }
                BGSChangeFlags kchangeFlags = apLoadGameBuffer->GetChangeFlags();
                BGSChangeFlags koldChangeFlags = apLoadGameBuffer->GetOldChangeFlags();
                bool breconstructedForm = apLoadGameBuffer->GetReconstructedForm();
                // Each by-value argument is built in place with BGSChangeFlags's constructor.
                BGSChangeFlags kactorBaseChangeFlags(iactorBaseChangeFlags);
                apLoadGameBuffer->SetChangeFlags(kactorBaseChangeFlags);
                BGSChangeFlags knoChangeFlags(0);
                apLoadGameBuffer->SetOldChangeFlags(knoChangeFlags);
                apLoadGameBuffer->SetReconstructedForm(false);
                pnewActorBase->LoadGame(apLoadGameBuffer);
                pnewActorBase->AddChangeFlags(iactorBaseChangeFlags);
                apLoadGameBuffer->SetChangeFlags(kchangeFlags);
                apLoadGameBuffer->SetOldChangeFlags(koldChangeFlags);
                apLoadGameBuffer->SetReconstructedForm(breconstructedForm);
                if (bcreatedTempActorBase) {
                    delete pnewActorBase;
                }
                break;
            }
            case 47: {
                ExtraLeveledItem* pextraLeveledItem;
                if (bexistingExtra[csavedType]) {
                    pextraLeveledItem = static_cast<ExtraLeveledItem*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraLeveledItem* pNewExtra = new ExtraLeveledItem;
                    pextraLeveledItem = static_cast<ExtraLeveledItem*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(&pextraLeveledItem->iIndex, sizeof(pextraLeveledItem->iIndex));
                apLoadGameBuffer->LoadData(&pextraLeveledItem->bdefault, sizeof(pextraLeveledItem->bdefault));
                break;
            }
            case 48: {
                ExtraScale* pextraScale;
                if (bexistingExtra[csavedType]) {
                    pextraScale = static_cast<ExtraScale*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraScale* pNewExtra = new ExtraScale;
                    pextraScale = static_cast<ExtraScale*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(&pextraScale->fScale, sizeof(pextraScale->fScale));
                break;
            }
            case 50: {
                NonActorMagicCaster* pextraNonActorMagicCaster;
                if (bexistingExtra[csavedType]) {
                    pextraNonActorMagicCaster = static_cast<NonActorMagicCaster*>(this->GetExtraData(csavedType));
                }
                else {
                    NonActorMagicCaster* pNewExtra = new NonActorMagicCaster;
                    pextraNonActorMagicCaster = static_cast<NonActorMagicCaster*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadFormID(pextraNonActorMagicCaster->pCurrentSpell);
                apLoadGameBuffer->LoadFormID(pextraNonActorMagicCaster->pDesiredTarget);
                apLoadGameBuffer->LoadFormID(pextraNonActorMagicCaster->pCasterObject);
                break;
            }
            case 51: {
                NonActorMagicTarget* pextraNonActorMagicTarget;
                if (bexistingExtra[csavedType]) {
                    pextraNonActorMagicTarget = static_cast<NonActorMagicTarget*>(this->GetExtraData(csavedType));
                }
                else {
                    NonActorMagicTarget* pNewExtra = new NonActorMagicTarget;
                    pextraNonActorMagicTarget = static_cast<NonActorMagicTarget*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadFormID(pextraNonActorMagicTarget->pTargetObject);
                ActiveEffect::LoadActiveEffectList(apLoadGameBuffer, &pextraNonActorMagicTarget->lActiveEffectList);
                break;
            }
            case 53: {
                ExtraPlayerCrimeList* pextraPlayerCrimeList;
                if (bexistingExtra[csavedType]) {
                    pextraPlayerCrimeList = static_cast<ExtraPlayerCrimeList*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraPlayerCrimeList* pNewExtra = new ExtraPlayerCrimeList;
                    pextraPlayerCrimeList = static_cast<ExtraPlayerCrimeList*>(this->AddExtra(pNewExtra));
                }
                uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
                for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
                    icrimeNumber = 0;
                    ecrimeType = PackageNS::CRIME_TYPE_NONE;
                    apLoadGameBuffer->LoadData(&icrimeNumber, sizeof(icrimeNumber));
                    apLoadGameBuffer->LoadData(&ecrimeType, sizeof(ecrimeType));
                    pcrime = ReferenceProcessLists.GetCrimeinGlobalCrimeList(ecrimeType, icrimeNumber);
                    if (pcrime) {
                        pextraPlayerCrimeList->pCrime->AddHead(pcrime);
                    }
                    else {
                        const char* pEditorID;
                        int32_t irefID;
                        if (pref__pactor) {
                            irefID = pref__pactor->iFormID;
                        }
                        else {
                            irefID = 0;
                        }
                        if (pref__pactor) {
                            pEditorID = pref__pactor->GetFormEditorID();
                        }
                        else {
                            pEditorID = "UNKNOWN";
                        }
                        SaveGameWarning(
                            "Could not find crime %i with crime type %i in the global crime list for ref %s "
                            "(%08X)",
                            icrimeNumber,
                            ecrimeType,
                            pEditorID,
                            irefID
                        );
                    }
                }
                if (!pextraPlayerCrimeList->pCrime->m_item) {
                    this->RemoveExtra(53);
                }
                break;
            }
            case 57: {
                ExtraItemDropper* pextraItemDropper;
                if (bexistingExtra[csavedType]) {
                    pextraItemDropper = static_cast<ExtraItemDropper*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraItemDropper* pNewExtra = new ExtraItemDropper;
                    pextraItemDropper = static_cast<ExtraItemDropper*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadFormID(pextraItemDropper->pDropper);
                break;
            }
            case 60: {
                ExtraMerchantContainer* pextraMerchantContainer;
                if (bexistingExtra[csavedType]) {
                    pextraMerchantContainer = static_cast<ExtraMerchantContainer*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraMerchantContainer* pNewExtra = new ExtraMerchantContainer;
                    pextraMerchantContainer = static_cast<ExtraMerchantContainer*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadFormID(pextraMerchantContainer->pContainer);
                break;
            }
            case 62: {
                this->SetCanNotWear(true);
                break;
            }
            case 63: {
                ExtraPoison* pextraPoison;
                if (bexistingExtra[csavedType]) {
                    pextraPoison = static_cast<ExtraPoison*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraPoison* pNewExtra = new ExtraPoison;
                    pextraPoison = static_cast<ExtraPoison*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadFormID(pextraPoison->pPoison);
                MagicItem* pMagicItem = MagicItem::GetMagicItemByNumericID(
                    static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pextraPoison->pPoison))
                );
                pextraPoison->pPoison = dynamic_cast<AlchemyItem*>(pMagicItem);
                break;
            }
            case 69: {
                ExtraFriendHits* pextraFriendHits;
                if (bexistingExtra[csavedType]) {
                    pextraFriendHits = static_cast<ExtraFriendHits*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraFriendHits* pNewExtra = new ExtraFriendHits;
                    pextraFriendHits = static_cast<ExtraFriendHits*>(this->AddExtra(pNewExtra));
                }
                uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
                pextraFriendHits->Hits.SetSize(isaveGameCount, true);
                for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
                    CombatTimeStamp* ftimeStamp = &pextraFriendHits->Hits[iloop];
                    ftimeStamp->LoadGame(apLoadGameBuffer);
                }
                if (!pextraFriendHits->Hits.iSize) {
                    this->RemoveFriendHitsExtra();
                }
                break;
            }
            case 70: {
                ExtraHeadingTarget* pextraHeadingTarget;
                if (bexistingExtra[csavedType]) {
                    pextraHeadingTarget = static_cast<ExtraHeadingTarget*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraHeadingTarget* pNewExtra = new ExtraHeadingTarget;
                    pextraHeadingTarget = static_cast<ExtraHeadingTarget*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadFormID(pextraHeadingTarget->pHeadingTarget);
                break;
            }
            case 73: {
                ExtraStartingWorldOrCell* pextraStartingWorldOrCell;
                if (bexistingExtra[csavedType]) {
                    pextraStartingWorldOrCell = static_cast<ExtraStartingWorldOrCell*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraStartingWorldOrCell* pNewExtra = new ExtraStartingWorldOrCell;
                    pextraStartingWorldOrCell = static_cast<ExtraStartingWorldOrCell*>(this->AddExtra(pNewExtra));
                }
                TESForm* pLoadedForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
                pextraStartingWorldOrCell->pStartingWorldOrCell = pLoadedForm;
                break;
            }
            case 74: {
                ExtraHotKey* pextraHotKey;
                if (bexistingExtra[csavedType]) {
                    pextraHotKey = static_cast<ExtraHotKey*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraHotKey* pNewExtra = new ExtraHotKey;
                    pextraHotKey = static_cast<ExtraHotKey*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(&pextraHotKey->chotkey, sizeof(pextraHotKey->chotkey));
                break;
            }
            case 77: {
                ExtraInfoGeneralTopic* pextraInfoGeneralTopic;
                if (bexistingExtra[csavedType]) {
                    pextraInfoGeneralTopic = static_cast<ExtraInfoGeneralTopic*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraInfoGeneralTopic* pNewExtra = new ExtraInfoGeneralTopic;
                    pextraInfoGeneralTopic = static_cast<ExtraInfoGeneralTopic*>(this->AddExtra(pNewExtra));
                }
                pextraInfoGeneralTopic->pInfoGen->LoadGame(apLoadGameBuffer);
                break;
            }
            case 78: {
                ExtraHasNoRumors* pextraHasNoRumors;
                if (bexistingExtra[csavedType]) {
                    pextraHasNoRumors = static_cast<ExtraHasNoRumors*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraHasNoRumors* pNewExtra = new ExtraHasNoRumors;
                    pextraHasNoRumors = static_cast<ExtraHasNoRumors*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(&pextraHasNoRumors->bNoRumors, sizeof(pextraHasNoRumors->bNoRumors));
                break;
            }
            case 80: {
                ExtraTerminalState* pextraTerminalState;
                if (bexistingExtra[csavedType]) {
                    pextraTerminalState = static_cast<ExtraTerminalState*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraTerminalState* pNewExtra = new ExtraTerminalState;
                    pextraTerminalState = static_cast<ExtraTerminalState*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(&pextraTerminalState->cState, sizeof(pextraTerminalState->cState));
                apLoadGameBuffer->LoadData(
                    &pextraTerminalState->cLockOverride,
                    sizeof(pextraTerminalState->cLockOverride)
                );
                break;
            }
            case 84: {
                ExtraActivateRefChildren* pextraActivateRefChildren;
                if (bexistingExtra[csavedType]) {
                    pextraActivateRefChildren = static_cast<ExtraActivateRefChildren*>(this->GetExtraData(csavedType));
                }
                else {
                    pextraActivateRefChildren = nullptr;
                }
                if (pextraActivateRefChildren) {
                    apLoadGameBuffer->LoadData(
                        &pextraActivateRefChildren->fActivateChildrenTimer,
                        sizeof(pextraActivateRefChildren->fActivateChildrenTimer)
                    );
                }
                else {
                    char ptempData[4];
                    apLoadGameBuffer->LoadData(ptempData, sizeof(ptempData));
                }
                break;
            }
            case 85: {
                ExtraTalkingActor* pextraTalkingActor;
                if (bexistingExtra[csavedType]) {
                    pextraTalkingActor = static_cast<ExtraTalkingActor*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraTalkingActor* pNewExtra = new ExtraTalkingActor;
                    pextraTalkingActor = static_cast<ExtraTalkingActor*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadFormID(pextraTalkingActor->pTalkObject);
                break;
            }
            case 86: {
                ExtraObjectHealth* pextraObjectHealth;
                if (bexistingExtra[csavedType]) {
                    pextraObjectHealth = static_cast<ExtraObjectHealth*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraObjectHealth* pNewExtra = new ExtraObjectHealth;
                    pextraObjectHealth = static_cast<ExtraObjectHealth*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(&pextraObjectHealth->fHealth, sizeof(pextraObjectHealth->fHealth));
                uint32_t iselfDamage = BGSDestructibleObjectForm::GetSelfDamage(
                    pref__pactor,
                    pextraObjectHealth->fHealth
                );
                if (iselfDamage) {
                    BGSDestructibleObjectForm::SetSelfDamage(pref__pactor, iselfDamage);
                }
                break;
            }
            case 91: {
                ExtraModelSwap* pextraModelSwap;
                if (bexistingExtra[csavedType]) {
                    pextraModelSwap = static_cast<ExtraModelSwap*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraModelSwap* pNewExtra = new ExtraModelSwap;
                    pextraModelSwap = static_cast<ExtraModelSwap*>(this->AddExtra(pNewExtra));
                }
                TESForm* pLoadedForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
                pextraModelSwap->pModelSwapForm = pLoadedForm;
                iindex = -1;
                apLoadGameBuffer->LoadData(&iindex, sizeof(iindex));
                if (pextraModelSwap->pModelSwapForm) {
                    pextraModelSwap->pModelSwap = BGSDestructibleObjectForm::GetModelSwap(
                        pextraModelSwap->pModelSwapForm,
                        iindex
                    );
                }
                if (pextraModelSwap->pModelSwapForm && pextraModelSwap->pModelSwap) {
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
                if (bexistingExtra[csavedType]) {
                    pextraRadius = static_cast<ExtraRadius*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraRadius* pNewExtra = new ExtraRadius;
                    pextraRadius = static_cast<ExtraRadius*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(&pextraRadius->fRadius, sizeof(pextraRadius->fRadius));
                break;
            }
            case 93: {
                ExtraRadiation* pextraRadiation;
                if (bexistingExtra[csavedType]) {
                    pextraRadiation = static_cast<ExtraRadiation*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraRadiation* pNewExtra = new ExtraRadiation;
                    pextraRadiation = static_cast<ExtraRadiation*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(&pextraRadiation->fRadiation, sizeof(pextraRadiation->fRadiation));
                break;
            }
            case 94: {
                ExtraFactionChanges* pextraFactionChanges;
                if (bexistingExtra[csavedType]) {
                    pextraFactionChanges = static_cast<ExtraFactionChanges*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraFactionChanges* pNewExtra = new ExtraFactionChanges;
                    pextraFactionChanges = static_cast<ExtraFactionChanges*>(this->AddExtra(pNewExtra));
                }
                uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
                for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
                    FACTION_RANK* pfactionRank = static_cast<FACTION_RANK*>(ourMalloc(sizeof(FACTION_RANK)));
                    TESForm* pLoadedForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
                    pfactionRank->pFaction = dynamic_cast<TESFaction*>(pLoadedForm);
                    apLoadGameBuffer->LoadData(&pfactionRank->cRank, sizeof(pfactionRank->cRank));
                    pextraFactionChanges->pFactionChanges->AddHead(pfactionRank);
                }
                BSSimpleList<FACTION_RANK*>* plist = pextraFactionChanges->pFactionChanges;
                BSSimpleList<FACTION_RANK*>* plastList = nullptr;
                while (plist && !plist->IsEmpty()) {
                    FACTION_RANK* pfactionRank = plist->m_item;
                    bool bremove = false;
                    if (pfactionRank) {
                        if (!pfactionRank->pFaction) {
                            ourFree(pfactionRank);
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
                if (bexistingExtra[csavedType]) {
                    pcurrentDismemberedLimbs = static_cast<ExtraDismemberedLimbs*>(this->GetExtraData(csavedType));
                }
                else {
                    pcurrentDismemberedLimbs = nullptr;
                }
                if (pcurrentDismemberedLimbs) {
                    this->RemoveExtra(pcurrentDismemberedLimbs, false);
                }
                else if (
                    apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_ACTOR_EXTRA_DISMEMBERED_LIMBS) &&
                    pactor->Get3D()
                ) {
                    pactor->ClearDismemberedLimbs();
                }
                pextraDismemberedLimbs = new ExtraDismemberedLimbs;
                apLoadGameBuffer->LoadData(&pextraDismemberedLimbs->sLimbs, sizeof(pextraDismemberedLimbs->sLimbs));
                apLoadGameBuffer->LoadData(
                    &pextraDismemberedLimbs->eCauseofDeath,
                    sizeof(pextraDismemberedLimbs->eCauseofDeath)
                );
                apLoadGameBuffer->LoadData(
                    &pextraDismemberedLimbs->eLastHitLimb,
                    sizeof(pextraDismemberedLimbs->eLastHitLimb)
                );
                apLoadGameBuffer->LoadData(&pextraDismemberedLimbs->bEaten, sizeof(pextraDismemberedLimbs->bEaten));
                apLoadGameBuffer->LoadFormID(pextraDismemberedLimbs->pDeathObject);
                uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
                pextraDismemberedLimbs->DismemberedLimbs.SetSize(isaveGameCount, true);
                for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
                    DismemberedLimb** pdismemberedLimb = &pextraDismemberedLimbs->DismemberedLimbs[iloop];
                    *pdismemberedLimb = new DismemberedLimb;
                    apLoadGameBuffer->LoadData(*pdismemberedLimb, 1u);
                    apLoadGameBuffer->LoadData(
                        &(*pdismemberedLimb)->bLimbExploded,
                        sizeof((*pdismemberedLimb)->bLimbExploded)
                    );
                    apLoadGameBuffer->LoadData(
                        &(*pdismemberedLimb)->bObjectArrayIdentical,
                        sizeof((*pdismemberedLimb)->bObjectArrayIdentical)
                    );
                    if (apLoadGameBuffer->GetVersion() >= 16u) {
                        apLoadGameBuffer->LoadData(
                            &(*pdismemberedLimb)->bLimbRemoved,
                            sizeof((*pdismemberedLimb)->bLimbRemoved)
                        );
                    }
                    uint32_t iobjectCount = apLoadGameBuffer->LoadVariableSizedValue();
                    (*pdismemberedLimb)->ObjectArray.SetSize(iobjectCount, true);
                    for (uint32_t iobject = 0; iobject < iobjectCount; ++iobject) {
                        apLoadGameBuffer->LoadFormID((*pdismemberedLimb)->ObjectArray[iobject]);
                    }
                    iobjectCount = (*pdismemberedLimb)->ObjectArray.iSize;
                    for (uint32_t iobject = 0; iobject < iobjectCount; ++iobject) {
                        bool bRemove = false;
                        TESBoundObject** ppObject = &(*pdismemberedLimb)->ObjectArray[iobject];
                        TESBoundObject* pObject = nullptr;
                        if (*ppObject) {
                            TESForm* pFoundForm = TESForm::GetFormByNumericID(
                                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(*ppObject))
                            );
                            pObject = dynamic_cast<TESBoundObject*>(pFoundForm);
                        }
                        *ppObject = pObject;
                        if (!*ppObject) {
                            bRemove = true;
                        }
                        if (bRemove) {
                            (*pdismemberedLimb)->ObjectArray.RemoveFast(iobject--, 1u);
                            --iobjectCount;
                        }
                    }
                }
                bool blimbsIdentical = false;
                if (pcurrentDismemberedLimbs) {
                    if (pcurrentDismemberedLimbs->CheckDismemberedLimbsIdentical(pextraDismemberedLimbs)) {
                        blimbsIdentical = true;
                    }
                    else {
                        pactor->ClearDismemberedLimbs();
                    }
                    delete pcurrentDismemberedLimbs;
                }
                this->AddExtra(pextraDismemberedLimbs);
                if (!blimbsIdentical) {
                    pactor->CreateDismemberedLimbs();
                }
                break;
            }
            case 96: {
                ExtraActorCause* pextraActorCause;
                if (bexistingExtra[csavedType]) {
                    pextraActorCause = static_cast<ExtraActorCause*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraActorCause* pNewExtra = new ExtraActorCause;
                    pextraActorCause = static_cast<ExtraActorCause*>(this->AddExtra(pNewExtra));
                }
                iactorCauseID = -1;
                apLoadGameBuffer->LoadData(&iactorCauseID, sizeof(iactorCauseID));
                pextraActorCause->spActorCause = ActorCause::GetActorCauseByID(iactorCauseID);
                if (!pextraActorCause->spActorCause.m_pObject) {
                    this->RemoveExtra(96);
                }
                break;
            }
            case 108: {
                ExtraOpenCloseActivateRef* pextraActivateRef;
                if (bexistingExtra[csavedType]) {
                    pextraActivateRef = static_cast<ExtraOpenCloseActivateRef*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraOpenCloseActivateRef* pNewExtra = new ExtraOpenCloseActivateRef;
                    pextraActivateRef = static_cast<ExtraOpenCloseActivateRef*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadFormID(pextraActivateRef->pActivateRef);
                break;
            }
            case 110: {
                ExtraAmmo* pextraAmmo;
                if (bexistingExtra[csavedType]) {
                    pextraAmmo = static_cast<ExtraAmmo*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraAmmo* pNewExtra = new ExtraAmmo;
                    pextraAmmo = static_cast<ExtraAmmo*>(this->AddExtra(pNewExtra));
                }
                TESForm* pLoadedForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
                pextraAmmo->pAmmo = dynamic_cast<TESAmmo*>(pLoadedForm);
                apLoadGameBuffer->LoadData(&pextraAmmo->iCount, sizeof(pextraAmmo->iCount));
                break;
            }
            case 112: {
                ExtraPackageData* pextraPackageData;
                if (bexistingExtra[csavedType]) {
                    pextraPackageData = static_cast<ExtraPackageData*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraPackageData* pNewExtra = new ExtraPackageData;
                    pextraPackageData = static_cast<ExtraPackageData*>(this->AddExtra(pNewExtra));
                }
                cpackDataType = -1;
                apLoadGameBuffer->LoadData(&cpackDataType, sizeof(cpackDataType));
                if (cpackDataType != -1) {
                    pextraPackageData->pActorPackageData = ActorPackageData::CreateActorPackageData(
                        static_cast<PACKAGE_TYPE>(cpackDataType)
                    );
                    pextraPackageData->pActorPackageData->LoadGame(apLoadGameBuffer);
                }
                break;
            }
            case 115: {
                ExtraSayTopicInfoOnceADay* pextraTopicInfo;
                if (bexistingExtra[csavedType]) {
                    pextraTopicInfo = static_cast<ExtraSayTopicInfoOnceADay*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraSayTopicInfoOnceADay* pNewExtra = new ExtraSayTopicInfoOnceADay;
                    pextraTopicInfo = static_cast<ExtraSayTopicInfoOnceADay*>(this->AddExtra(pNewExtra));
                }
                uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
                for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
                    SayOnceTopicInfos* psayOnceTopicInfo = new SayOnceTopicInfos;
                    TESForm* pLoadedForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
                    psayOnceTopicInfo->pInfo = dynamic_cast<TESTopicInfo*>(pLoadedForm);
                    apLoadGameBuffer->LoadData(&psayOnceTopicInfo->iDateStamp, sizeof(psayOnceTopicInfo->iDateStamp));
                    apLoadGameBuffer->LoadData(&psayOnceTopicInfo->fHourStamp, sizeof(psayOnceTopicInfo->fHourStamp));
                    pextraTopicInfo->pListofSaidOnceTopicInfos->AddHead(psayOnceTopicInfo);
                }
                break;
            }
            case 116: {
                ExtraEncounterZone* pextraEncounterZone;
                if (bexistingExtra[csavedType]) {
                    pextraEncounterZone = static_cast<ExtraEncounterZone*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraEncounterZone* pNewExtra = new ExtraEncounterZone;
                    pextraEncounterZone = static_cast<ExtraEncounterZone*>(this->AddExtra(pNewExtra));
                }
                TESForm* pLoadedForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
                pextraEncounterZone->pZone = dynamic_cast<BGSEncounterZone*>(pLoadedForm);
                break;
            }
            case 117: {
                ExtraSayToTopicInfo* pextraSayToTopicInfo;
                if (bexistingExtra[csavedType]) {
                    pextraSayToTopicInfo = static_cast<ExtraSayToTopicInfo*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraSayToTopicInfo* pNewExtra = new ExtraSayToTopicInfo;
                    pextraSayToTopicInfo = static_cast<ExtraSayToTopicInfo*>(this->AddExtra(pNewExtra));
                }
                TESForm* pLoadedForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
                pextraSayToTopicInfo->pTopic = dynamic_cast<TESTopic*>(pLoadedForm);
                pLoadedForm = TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID());
                pextraSayToTopicInfo->pInfo = dynamic_cast<TESTopicInfo*>(pLoadedForm);
                apLoadGameBuffer->LoadData(&pextraSayToTopicInfo->bVoiceDone, sizeof(pextraSayToTopicInfo->bVoiceDone));
                break;
            }
            case 124: {
                ExtraGuardedRefData* pextraGuardedRefData;
                if (bexistingExtra[csavedType]) {
                    pextraGuardedRefData = static_cast<ExtraGuardedRefData*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraGuardedRefData* pNewExtra = new ExtraGuardedRefData;
                    pextraGuardedRefData = static_cast<ExtraGuardedRefData*>(this->AddExtra(pNewExtra));
                }
                uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
                pextraGuardedRefData->Guards.SetSize(isaveGameCount, true);
                for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
                    uint32_t* iguardID = &pextraGuardedRefData->Guards[iloop];
                    apLoadGameBuffer->LoadFormID(*iguardID);
                }
                break;
            }
            case 137: {
                ExtraAshPileRef* pextraAshPileRef;
                if (bexistingExtra[csavedType]) {
                    pextraAshPileRef = static_cast<ExtraAshPileRef*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraAshPileRef* pNewExtra = new ExtraAshPileRef;
                    pextraAshPileRef = static_cast<ExtraAshPileRef*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadFormID(pextraAshPileRef->pAshPileRef);
                break;
            }
            case 139: {
                ExtraFollowerSwimBreadcrumbs* pextraCrumbs;
                if (bexistingExtra[csavedType]) {
                    pextraCrumbs = static_cast<ExtraFollowerSwimBreadcrumbs*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraFollowerSwimBreadcrumbs* pNewExtra = new ExtraFollowerSwimBreadcrumbs;
                    pextraCrumbs = static_cast<ExtraFollowerSwimBreadcrumbs*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(&pextraCrumbs->LeaderLocation, sizeof(pextraCrumbs->LeaderLocation));
                if (apLoadGameBuffer->GetVersion() < 18u) {
                    apLoadGameBuffer->LoadData(&pextraCrumbs->LeaderNavMeshID, sizeof(pextraCrumbs->LeaderNavMeshID));
                }
                else {
                    apLoadGameBuffer->LoadFormID(pextraCrumbs->LeaderNavMeshID);
                }
                apLoadGameBuffer->LoadData(&pextraCrumbs->eLeaderState, sizeof(pextraCrumbs->eLeaderState));
                uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
                for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
                    ExtraFollowerSwimBreadcrumb* pcrumb = new ExtraFollowerSwimBreadcrumb;
                    apLoadGameBuffer->LoadData(pcrumb, 12u);
                    if (apLoadGameBuffer->GetVersion() < 18u) {
                        apLoadGameBuffer->LoadData(&pcrumb->StartNavMeshID, sizeof(pcrumb->StartNavMeshID));
                    }
                    else {
                        apLoadGameBuffer->LoadFormID(pcrumb->StartNavMeshID);
                    }
                    apLoadGameBuffer->LoadData(&pcrumb->EndLocation, sizeof(pcrumb->EndLocation));
                    if (apLoadGameBuffer->GetVersion() < 18u) {
                        apLoadGameBuffer->LoadData(&pcrumb->EndNavMeshID, sizeof(pcrumb->EndNavMeshID));
                    }
                    else {
                        apLoadGameBuffer->LoadFormID(pcrumb->EndNavMeshID);
                    }
                    apLoadGameBuffer->LoadData(&pcrumb->bEnteringWater, sizeof(pcrumb->bEnteringWater));
                    pextraCrumbs->CrumbList.AddHead(pcrumb);
                }
                break;
            }
            case 141: {
                ExtraWeaponModFlags* pextraWMF;
                if (bexistingExtra[csavedType]) {
                    pextraWMF = static_cast<ExtraWeaponModFlags*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraWeaponModFlags* pNewExtra = new ExtraWeaponModFlags;
                    pextraWMF = static_cast<ExtraWeaponModFlags*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadData(&pextraWMF->cWeaponModsActive, sizeof(pextraWMF->cWeaponModsActive));
                break;
            }
            case 143: {
                ExtraSecuritronFace* pFaceExtra;
                char workstring[256];
                if (bexistingExtra[csavedType]) {
                    pFaceExtra = static_cast<ExtraSecuritronFace*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraSecuritronFace* pNewExtra = new ExtraSecuritronFace;
                    pFaceExtra = static_cast<ExtraSecuritronFace*>(this->AddExtra(pNewExtra));
                }
                apLoadGameBuffer->LoadString(workstring);
                pFaceExtra->SetPersonality(workstring);
                apLoadGameBuffer->LoadString(workstring);
                pFaceExtra->SetMood(workstring);
                break;
            }
            case 144:
            case 145: {
                break;
            }
            case 146: {
                ExtraSpecialRenderFlags* pOEIExtra;
                if (bexistingExtra[csavedType]) {
                    pOEIExtra = static_cast<ExtraSpecialRenderFlags*>(this->GetExtraData(csavedType));
                }
                else {
                    ExtraSpecialRenderFlags* pNewExtra = new ExtraSpecialRenderFlags;
                    pOEIExtra = static_cast<ExtraSpecialRenderFlags*>(this->AddExtra(pNewExtra));
                }
                uint32_t uiRenderFlags;
                apLoadGameBuffer->LoadData(&uiRenderFlags, sizeof(uiRenderFlags));
                apLoadGameBuffer->LoadData(
                    &pOEIExtra->m_fNodeMaxFadeDistance,
                    sizeof(pOEIExtra->m_fNodeMaxFadeDistance)
                );
                pOEIExtra->m_flags = uiRenderFlags;
                break;
            }
            default: {
                SaveGameWarning("Unknown extra data type %i found in ExtraDataList::LoadGame", csavedType);
                break;
            }
        }
        bexistingExtra[csavedType] = false;
    }
    for (uint32_t i = 0; i < 147; ++i) {
        bool bremoveExtra = bexistingExtra[i];
        if (bremoveExtra) {
            if (i == 46) {
                bremoveExtra = false;
            }
            else if (i == 146) {
                ExtraSpecialRenderFlags* pspecialRenderFlags = dynamic_cast<ExtraSpecialRenderFlags*>(
                    this->GetExtraData(146)
                );
                if (pspecialRenderFlags) {
                    pspecialRenderFlags->m_flags &= 7;
                }
                bremoveExtra = false;
            }
        }
        if (bremoveExtra) {
            this->RemoveExtra(i);
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
        pSound = new ExtraWeaponIdleSound(afSound);
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
        pExtra = new ExtraActivateLoopSound(afSound);
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
        pExtra = new ExtraPlayerCrimeList(apCrime);
        AddExtra(pExtra);
    }
}

void ExtraDataList::AddActivateParent(TESObjectREFR* apRef) {
    if (apRef) {
        ExtraActivateRef* pExtra = static_cast<ExtraActivateRef*>(GetExtraData(83u));
        if (pExtra) {
            if (!pExtra->GetData(apRef)) {
                REF_ACTIVATE_DATA* pData = new REF_ACTIVATE_DATA;
                pData->pActivateRef = apRef;
                pExtra->ParentList.AddHead(pData);
            }
        }
        else {
            pExtra = new ExtraActivateRef;
            AddExtra(pExtra);
            REF_ACTIVATE_DATA* pData = new REF_ACTIVATE_DATA;
            pData->pActivateRef = apRef;
            pExtra->ParentList.AddHead(pData);
        }
    }
}

void ExtraDataList::SetActivateDelay(TESObjectREFR* apRef, float afVal) {
    if (apRef) {
        ExtraActivateRef* pExtra = static_cast<ExtraActivateRef*>(GetExtraData(83u));
        REF_ACTIVATE_DATA* pData = nullptr;
        if (pExtra) {
            pData = pExtra->GetData(apRef);
        }
        else {
            pExtra = new ExtraActivateRef;
            AddExtra(pExtra);
        }
        if (!pData) {
            pData = new REF_ACTIVATE_DATA;
            pExtra->ParentList.AddHead(pData);
            pData->pActivateRef = apRef;
        }
        pData->fActivateDelay = afVal;
    }
}

BSStringT<char> ExtraDataList::GetActivateTextOverride() {
    ExtraActivateRef* pextra = static_cast<ExtraActivateRef*>(GetExtraData(83));
    if (pextra) {
        return pextra->ActivateTextOverride;
    }
    return "";
}

void ExtraDataList::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* pRef = apLoadGameBuffer->GetReference();
    if (pRef) {
        if (GetModelSwap()) {
            pRef->Set3D(nullptr, true);
        }
        if (GetExtraData(86)) {
            BGSDestructibleObjectForm::SetSelfDamage(pRef, 0);
        }
        TESObjectREFR* pDropper = GetItemDropper();
        if (pDropper) {
            pDropper->GetExtra()->RemoveDroppedItem(pRef);
        }
        MapMarkerData* pMapData = GetMapMarkerData();
        if (pMapData) {
            pMapData->ResetFlags();
        }
        if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN)) {
            SetActivateChildrenTimer(0.0f);
        }
        ScriptLocals* pScriptLocals = GetScriptLocals();
        if (pScriptLocals) {
            pScriptLocals->Revert(apLoadGameBuffer);
        }
    }
    bool bisFormIDCreated = pRef ? pDataHandler->IsFormIDCreated(pRef->iFormID) : false;
    for (uint32_t i = 0; i < 147; ++i) {
        char cremove = 0;
        if (xDataTypeToChangeFlag[i]) {
            int32_t iflag = xDataTypeToChangeFlag[i];
            if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(iflag)) {
                cremove = 1;
            }
        }
        if (!bisFormIDCreated && cremove && (xDataTypeToChangeFlag[i] & CHANGE_REFR_EXTRA_CREATED_ONLY) != 0) {
            cremove = 0;
        }

        if (cremove) {
            switch (i) {
                case 13u:
                case 44u:
                case 46u:
                case 73u:
                case 84u:
                case 95u:
                case 144u:
                case 145u: {
                    cremove = 0;
                    break;
                }
                default: {
                    break;
                }
            }
        }
        if (cremove) {
            RemoveExtra(i);
        }
    }
}

MenuTopic* ExtraDataList::GetInfoGeneralTopic(TESObjectREFR* apSpeaker) {
    MenuTopic* pItem = nullptr;
    ExtraInfoGeneralTopic* pextra = static_cast<ExtraInfoGeneralTopic*>(GetExtraData(77));
    if (pextra) {
        pItem = pextra->pInfoGen;
        if (pItem && pItem->GetResponseListEmpty()) {
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
    return MemoryManager::Instance().Reallocate(apPtr, aSize);
}

const NiMatrix3 BSOcclusionPlane::QRotation() const {
    return kRotation;
}

void ActorCause::IncRefCount() {
    NiAtomicIncrement(iRefCount);
}

NiColorA BGSPrimitive::GetColor() {
    return Color;
}

NiPoint3 BGSPrimitive::GetRadii() {
    return Radii;
}

BGSPrimitive::PRIMITIVE_TYPE BGSPrimitive::GetType() {
    return eType;
}

RADIO_DATA::RADIO_DATA() : fRadius(0.0f), eRangeType(RADIO_RANGE_EVERYWHERE), fStaticPct(0.0f), pPositionRef(nullptr) {}

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
    return (cFlags & FLAG_INVERSE) != 0;
}

void ExtraEnableStateParent::SetInverse(bool abVal) {
    if (abVal) {
        this->cFlags |= FLAG_INVERSE;
    }
    else {
        this->cFlags &= ~FLAG_INVERSE;
    }
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
    for (TESObjectREFR*& pLinkedRef : pLinkedRefs) {
        ENDIAN<TESObjectREFR*>(pLinkedRef, false);
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

ExtraScale::ExtraScale() : BSExtraData(EXTRA_SCALE), fScale(0.0f) {}

ExtraSecuritronFace::ExtraSecuritronFace() : BSExtraData(143) {}

ExtraSecuritronFace::~ExtraSecuritronFace() {}

BSStringT<char> ExtraSecuritronFace::GetPersonality() {
    return msPersonality;
}

BSStringT<char> ExtraSecuritronFace::GetMood() {
    return msMood;
}

ExtraTimeLeft::ExtraTimeLeft() : BSExtraData(EXTRA_TIMELEFT), fTime(0.0f) {}

ExtraTimeLeft::~ExtraTimeLeft() {}

ExtraTresPassPackage::ExtraTresPassPackage() : BSExtraData(EXTRA_TRESPASS_PACKAGE), pPack(nullptr) {}

NAVMESH_PORTAL::NAVMESH_PORTAL() : pNavMesh(nullptr), iTriIndex(0) {}

int32_t BSAtomicCompareExchange(volatile int32_t* apAddress, int32_t aiCompare, int32_t aiValue) {
    return InterlockedCompareExchange32(apAddress, aiValue, aiCompare);
}

// InterlockedCompareExchange takes a LONG, so the cast reads the same unsigned word as signed.
uint32_t BSAtomicCompareExchange(volatile uint32_t* apAddress, uint32_t aiCompare, uint32_t aiValue) {
    return static_cast<uint32_t>(InterlockedCompareExchange32(
        reinterpret_cast<volatile int32_t*>(apAddress),
        static_cast<int32_t>(aiValue),
        static_cast<int32_t>(aiCompare)
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
    return (uiThreadSpecificFlags & THREAD_FLAG_LOADING_MOVED_REFS) != 0;
}

bool BGSSaveLoadGame::GetReconstructingForms() {
    return (uiThreadSpecificFlags & THREAD_FLAG_RECONSTRUCTING_FORMS) != 0;
}

float NiCos(float fRadians) {
    return std::cos(fRadians);
}

float NiSin(float fRadians) {
    return std::sin(fRadians);
}

void* ourMalloc(uint32_t aSize) {
    return MemoryManager::Instance().Allocate(aSize);
}

void ourFree(void* apPtr) {
    MemoryManager::Instance().Deallocate(apPtr);
}
