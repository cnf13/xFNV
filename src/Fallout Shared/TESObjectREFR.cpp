#include "Fallout Shared/TESObjectREFR.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSAudio/BSWin32Audio.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/ScrapHeap.h"
#include "BSHavok/WeaponObject.h"
#include "BSHavok/bhkAabbPhantom.h"
#include "BSHavok/bhkAcousticSpacePhantomCallbackShape.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkBlendController.h"
#include "BSHavok/bhkBoxShape.h"
#include "BSHavok/bhkBvTreeShape.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkConvexShape.h"
#include "BSHavok/bhkCylinderShape.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkNiTriStripsShape.h"
#include "BSHavok/bhkPackedNiTriStripsShape.h"
#include "BSHavok/bhkPhantom.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkRagdollPenetrationUtil.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkRigidBodyCinfo.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkShapeCollection.h"
#include "BSHavok/bhkShapePhantom.h"
#include "BSHavok/bhkSimpleShapePhantom.h"
#include "BSHavok/bhkSphereRepShape.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkTTNode.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWaterPhantomCallbackShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldNS.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSBound.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundAABB.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSMultiBoundOBB.h"
#include "BSMain/BSMultiBoundRoom.h"
#include "BSMain/BSMultiBoundShape.h"
#include "BSMain/BSOcclusionPlane.h"
#include "BSMain/BSParticleSystemManager.h"
#include "BSMain/BSPortal.h"
#include "BSMain/BSPortalGraph.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/BSValueNode.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/IOManager.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/NiObjectGarbage.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMain/SettingCollectionMap.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderNoLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/WaterShaderProperty.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorCause.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSAddonNode.h"
#include "Fallout Shared/BGSAddonNodeSoundHandleExtra.h"
#include "Fallout Shared/BGSBipedModelList.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSDecalNode.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEncounterZone.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSMovableStatic.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSPlaceableWater.h"
#include "Fallout Shared/BGSPrimitive.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BGSTerminal.h"
#include "Fallout Shared/BGSTextureSet.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BSAnimNoteReceiver.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CheckWithinMultiBoundTask.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/CreatureSounds.h"
#include "Fallout Shared/DecalData.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/FaceGen/BSFaceGenNiNode.h"
#include "Fallout Shared/FaceGen/NiGeometryDataIteratorNiPoint3.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/BGSAcousticSpaceListener.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/Havok/TESTrapListener.h"
#include "Fallout Shared/Havok/TESWaterListener.h"
#include "Fallout Shared/IDLE_ANIM_ROOT.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/LipSynchAnim.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/MultiBoundMarkerData.h"
#include "Fallout Shared/NiFormArray.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionDataImposter.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/SpeedTree/BSTreeNode.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESChallengeManager.h"
#include "Fallout Shared/TESChildCell.h"
#include "Fallout Shared/TESClass.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESFurniture.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESLand.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESMagicTargetForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectACTI.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectCONT.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectMISC.h"
#include "Fallout Shared/TESObjectSTAT.h"
#include "Fallout Shared/TESObjectTREE.h"
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
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TeleportData.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ArrowProjectile.h"
#include "Fallout/AI/BSDoorHavokController.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/AI/Combat/BeamProjectile.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/ContinuousBeamProjectile.h"
#include "Fallout/AI/Combat/FlameProjectile.h"
#include "Fallout/AI/Combat/GrenadeProjectile.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/Creature.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/FORenderedTerminal.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/BarterMenu.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/NonActorMagicCaster.h"
#include "Fallout/Magic/NonActorMagicTarget.h"
#include "Fallout/Misc/GarbageCollector.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormHeader.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMultiTargetTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Box/hkpBoxShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/PhantomCallback/hkpPhantomCallbackShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <xmmintrin.h>

// GetCalcLevel's function-local static (no guard: zero-initialised data) and the settings it reads.
static int32_t ileveladjust;

NiPoint3 TESObjectREFR::boundMin;

// The game settings this file defines, with their IDs and default values, in initialization order.
SettingT<INISettingCollection> bPreventHavokAddClutter("bPreventHavokAddClutter:HAVOK", false);
SettingT<INISettingCollection> bPreventHavokAddAll("bPreventHavokAddAll:HAVOK", false);
SettingT<INISettingCollection> TESObjectREFR::bPrimitivesOn("bPrimitivesOn", false);
SettingT<INISettingCollection> bFileControllerOnRoot("bFileControllerOnRoot:TestAllCells", true);
SettingT<GameSettingCollection> iCalcLevelAdjustDown("iCalcLevelAdjustDown", 1);
SettingT<GameSettingCollection> iCalcLevelAdjustUp("iCalcLevelAdjustUp", 1);
NiPoint3 TESObjectREFR::boundMax;
CheckWithinMultiBoundTask::AttachMapType CheckWithinMultiBoundTask::AttachMap(37);

// The debug colours of the leveled actors' difficulties.
static NiColor ColorLeveledActorEasy(0.0f, 1.0f, 0.0f);
static NiColor ColorLeveledActorMedium(1.0f, 1.0f, 0.0f);
static NiColor ColorLeveledActorHard(1.0f, 0.5f, 0.0f);
static NiColor ColorLeveledActorBoss(1.0f, 0.0f, 0.0f);

LOADED_REF_DATA::LOADED_REF_DATA() : m_spData3D(nullptr), spPhantom(nullptr) {}

LOADED_REF_DATA::~LOADED_REF_DATA() {}

void LOADED_REF_DATA::SetFlag(uint32_t aiFlag, bool abValue) {
    if (abValue) {
        iFlags |= aiFlag;
    }
    else {
        iFlags &= (~aiFlag);
    }
}

bool LOADED_REF_DATA::GetFlag(uint32_t aiFlag) {
    return (aiFlag & iFlags) != 0;
}

TESObjectREFR::TESObjectREFR() : pLoadedData(nullptr) {
    SetFormType(REFR_ID);
    TESObjectREFR::InitializeData();
    data.pObjectReference = nullptr;
    pParentCell = nullptr;
    pRandomSound = nullptr;
}

TESObjectREFR::~TESObjectREFR() {
    BSSimpleList<REF_ACTIVATE_DATA*>* pactlist;
    CheckWithinMultiBoundTask::CancelAttachment(this);
    if (IsFurniture()) {
        TESObjectREFR::kFurnitureLock.Lock(nullptr);
        nFurnitureListDirtied = appTimer.iLastTime;
        for (BSSimpleList<Actor*>* pIter = ReferenceProcessLists.GetAliveActorList(); pIter; pIter = pIter->GetNext()) {
            Actor* pActor = pIter->GetItem();
            if (pActor && pActor->GetCurrentAIProcess()) {
                pActor->GetCurrentAIProcess()->RemoveFurnitureRef(this);
            }
        }
        TESObjectREFR::kFurnitureLock.Unlock();
    }
    if (GetFormType() == FURN_ID) {
        nFurnitureListDirtied = appTimer.iLastTime;
    }
    GarbageCollector::Remove(this);
    if (GetObjectReference() && GetObjectReference()->TESForm::GetHasSpokenFlag()) {
        TESWorldSpace* pworld = GetWorldSpace();
        if (pworld && pworld->GetTerrainManager()) {
            pworld->GetTerrainManager()->ReleaseTree(this);
        }
    }
    if (pLoadedData) {
        bhkWorld* pw = GetParentCell() ? GetParentCell()->GetbhkWorld() : nullptr;
        if (pw) {
            pw->RemoveBody(pLoadedData->spPhantom.m_pObject);
        }
        pLoadedData->spPhantom = nullptr;
    }
    bool ballow = pSaveLoadGameOLD->m_bAllowChanges;
    pSaveLoadGameOLD->m_bAllowChanges = false;
    if (!GetTemporary()) {
        CleanUpTraps();
        BSDoorHavokController::ReleaseDoor(this);
        pSaveLoadGameOLD->RemoveFormFromInitFormArray(this);
        if (!pDataHandler->GetClearingData()) {
            TESObjectREFR* pparent = GetEnableStateParent();
            if (pparent) {
                pparent->m_Extra.RemoveEnableStateChild(this);
            }
            for (BSSimpleList<TESObjectREFR*>* plist = GetEnableStateChildList(); plist && !plist->IsEmpty();
                 plist = GetEnableStateChildList()) {
                plist->m_item->SetEnableStateParent(nullptr);
            }
        }
        TESObjectREFR* pdropper = m_Extra.GetItemDropper();
        if (pdropper) {
            pdropper->m_Extra.RemoveDroppedItem(this);
        }
        for (BSSimpleList<TESObjectREFR*>* pdroplist = m_Extra.GetDroppedItemList(); pdroplist && !pdroplist->IsEmpty();
             pdroplist = pdroplist->m_pkNext) {
            pdroplist->m_item->m_Extra.SetItemDropper(nullptr);
            if (pdroplist->m_item->GetOwner() == this) {
                pdroplist->m_item->RemoveOwner();
            }
        }
        if (!pDataHandler->GetClearingData()) {
            TESObjectREFR* pashpile = m_Extra.GetAshPileRef();
            if (pashpile) {
                pashpile->m_Extra.SetAshPileRef(nullptr);
            }
            BSSimpleList<REF_REFLECTOR_DATA*>* preflist = m_Extra.GetReflectorRefs();
            while (preflist && preflist->m_item) {
                REF_REFLECTOR_DATA* pdata = preflist->m_item;
                preflist = preflist->m_pkNext;
                pdata->pRef->m_Extra.SetReflectsRef(this, false);
                pdata->pRef->m_Extra.SetRefractsRef(this, false);
            }
            Script::RemoveDelayedScriptActionReference(this);
            TESObjectREFR* pSelf = this;
            TESObjectCELL::GetAutoWaterReflectList()->Remove(pSelf);
            TESObjectCELL::GetAutoWaterRefractList()->Remove(pSelf);
            if (GetTargeted()) {
                int32_t iFormType = GetFormType();
                if (iFormType < ACHR_ID || iFormType > ACRE_ID) {
                    ReferenceProcessLists.ReferDestructorNullOut(this);
                    pPlayer->RemoveAllActionsForObject(this);
                    ReferenceProcessLists.NullOutTargetRef(this, true);
                    ReferenceProcessLists.NullOutActorInCrimeList(this);
                    ReferenceProcessLists.RemoveReferenceMuzzleFlash(this);
                }
                else {
                    pPlayer->RemoveAllActionsForObject(this);
                }
            }
            if (!TESObjectREFR::IsProjectile()) {
                ReferenceProcessLists.FinishMagicHitEffect(this);
            }
        }
        if (IsRecentPCDrop()) {
            pPlayer->ClearDroppedReference(this);
        }
        if (!pDataHandler->GetClearingData() && IsFurniture()) {
            TESObjectREFR* pSelf = this;
            TES::pInstance->GetBedChairList()->Remove(pSelf);
        }
        if (GetDestructible()) {
            BGSDestructibleObjectForm::RemoveDestructibleObject(this);
        }
    }
    TESObjectREFR* plinkedref = GetLinkedRef();
    if (plinkedref) {
        plinkedref->m_Extra.RemoveLinkedRefChild(this);
    }
    for (BSSimpleList<TESObjectREFR*>* preflist = m_Extra.GetLinkedRefChildren(); preflist && !preflist->IsEmpty();
         preflist = preflist->m_pkNext) {
        preflist->m_item->SetLinkedRef(nullptr);
    }
    for (pactlist = m_Extra.GetActivateParentList(); pactlist && !pactlist->IsEmpty(); pactlist = pactlist->m_pkNext) {
        pactlist->m_item->pActivateRef->m_Extra.RemoveActivateRefChild(this);
    }
    for (pactlist = m_Extra.GetActivateRefChildren(); pactlist && !pactlist->IsEmpty(); pactlist = pactlist->m_pkNext) {
        pactlist->m_item->pActivateRef->m_Extra.RemoveActivateParent(this);
    }
    TESObjectREFR::SetDelete(true);
    TESObjectREFR::ClearData();
    if (pParentCell) {
        pParentCell->RemoveReference(this);
    }
    if (GetRefPersists() && pDataHandler && !pDataHandler->GetClearingData()) {
        TESObjectCELL* ppersistparent = TESObjectREFR::GetSaveParentCell();
        if (ppersistparent) {
            ppersistparent->RemoveReference(this);
        }
    }
    if (GetObjectReference() && TESObjectREFR::IsObstacle()) {
        NavMeshObstacleManager::GetInstance()->RemoveObstacleForReference(this);
    }
    if (GetObjectReference() && GetObjectReference()->GetFormType() == DOOR_ID) {
        NavMeshObstacleManager::GetInstance()->OnDoorRemove(this);
    }
    Interface::ClearReferencePointers(this);
    if (Main::GetDebugTextRef() == this) {
        Main::ClearDebugTextRef();
    }
    if (!pDataHandler->GetClearingData()) {
        if (Interface::GetEnemyActor() == this) {
            Interface::SetEnemyActor(nullptr);
        }
        if (pPlayer) {
            pPlayer->RemoveConflictingObject(this);
        }
    }
    if (pPlayer->GetGrabObject() == this) {
        pPlayer->ClearGrabObject();
    }
    pSaveLoadGameOLD->m_bAllowChanges = ballow;
}

void TESObjectREFR::InitializeData() {
    TESBoundObject* pObjectReference = data.pObjectReference;
    ourMemset(&data, 0, sizeof(data));
    data.pObjectReference = pObjectReference;
    fRefScale = 1.0f;
    if (pLoadedData) {
        pLoadedData->pCurrentWaterObject = nullptr;
        pLoadedData->fRelevantWaterHeight = -std::numeric_limits<float>::max();
    }
    if (!pSaveLoadGame || !pSaveLoadGame->GetReconstructingForms()) {
        FreeLoadedData();
    }
}

// A model swap extra survives the clear while the reference has 3D: it is unlinked, put back after the list is
// emptied, and destroyed once the 3D is gone.
void TESObjectREFR::ClearData() {
    if (!pSaveLoadGame || !pSaveLoadGame->GetReconstructingForms()) {
        RestoreLeveledCreatureOriginalBase();
    }
    KillLight(false);
    TESObjectCELL* pPersistentCell = m_Extra.GetPersistentCell();
    bool bReconstructing = false;
    if (pSaveLoadGame->GetReconstructingForms()) {
        bReconstructing = true;
    }
    BSExtraData* pModelSwap = nullptr;
    if (Get3D()) {
        pModelSwap = GetExtra()->GetExtraData(EXTRA_MODEL_SWAP);
        if (pModelSwap) {
            GetExtra()->RemoveExtra(pModelSwap, false);
        }
    }
    RemoveFromAllWater(false);
    if (bReconstructing) {
        m_Extra.RemoveAllDefault(true);
    }
    else {
        m_Extra.RemoveAll(true);
    }
    if (pModelSwap) {
        GetExtra()->AddExtra(pModelSwap);
    }
    if (pPersistentCell) {
        m_Extra.SetPersistentCell(pPersistentCell);
    }
    if (!pSaveLoadGame->GetReconstructingForms()) {
        Set3D(nullptr, true);
    }
    if (pModelSwap) {
        GetExtra()->RemoveExtra(pModelSwap, true);
    }
}

// The reference's form record. A deleted record keeps only its persistent cell (restoring a leveled creature's original
// base unless forms are being reconstructed) and returns whether the base object is set. Otherwise: NAME is the base
// object (the current one is kept while reconstructing forms when its form ID is a created one; a missing base warns
// and skips DATA), XSCL the scale, ONAM action 8, DATA the position and rotation, EDID and OBND go to the form's own
// overrides, RCLR is skipped and the extra-data chunks go to the extra list; any other chunk warns. A patrol point
// whose script has text but no compiled data warns. When the chunk is empty (malformed data), the EDID buffer is
// initialised to "" (one zeroed byte, allocated only for an empty chunk) and DATA's placement to zeroes.
bool TESObjectREFR::Load(TESFile* apFile) {
    CHUNK_ID eChunk = NO_CHUNK;
    // Constructs a placement here that is never used.
    REFR_PLACEMENT_DATA kUnusedPlacement;
    SetInitialized(false);
    bool bSkipData = false;
    uint32_t uiBaseID = 0;
    LoadForm(apFile);
    SetInitialized(false);
    if (GetDelete()) {
        TESObjectCELL* pPersistentCell = m_Extra.GetPersistentCell();
        if (!pSaveLoadGame || !pSaveLoadGame->GetReconstructingForms()) {
            RestoreLeveledCreatureOriginalBase();
        }
        m_Extra.RemoveAll(true);
        if (pPersistentCell) {
            m_Extra.SetPersistentCell(pPersistentCell);
        }
        return GetObjectReference() != nullptr;
    }
    while (true) {
        eChunk = static_cast<CHUNK_ID>(apFile->GetTESChunk());
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case EDID_ID: {
                // Allocates the chunk's size on the stack and reads at most 0x200 bytes.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pEditorID = static_cast<char*>(XFNV_ALLOCA(uiChunkSize ? uiChunkSize : 1));
                // An empty chunk leaves the buffer unwritten (see above).
                pEditorID[0] = '\0';
                apFile->GetChunkData(pEditorID, 0x200u);
                SetFormEditorID(pEditorID);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case XSCL_ID: apFile->GetChunkData<float>(fRefScale); break;
            case NAME_ID: {
                apFile->GetChunkData<uint32_t>(uiBaseID);
                TESForm::AddCompileIndex(uiBaseID, apFile);
                TESForm* pForm = TESForm::GetFormByNumericID(uiBaseID);
                TESBoundObject* pBase = dynamic_cast<TESBoundObject*>(pForm);
                bool bSetBase = true;
                if (data.pObjectReference &&
                    pSaveLoadGame->GetReconstructingForms() &&
                    pDataHandler->IsFormIDCreated(data.pObjectReference->GetFormID())) {
                    bSetBase = false;
                }
                if (bSetBase) {
                    data.pObjectReference = pBase;
                    if (!data.pObjectReference) {
                        Warning(
                            "MASTERFILE: Missing/Invalid base object (%08X) for reference (%08X)",
                            uiBaseID,
                            GetFormID()
                        );
                        bSkipData = true;
                    }
                }
                break;
            }
            case ONAM_ID: SetAction(OBJECT_ACTION_OPEN_BY_DEFAULT); break;
            case DATA_ID:
                if (!bSkipData) {
                    REFR_PLACEMENT_DATA kPlacement;
                    // Zeroed here, unlike the game, as an empty chunk leaves it unwritten (see above).
                    kPlacement.kPosition.x = 0.0f;
                    kPlacement.kPosition.y = 0.0f;
                    kPlacement.kPosition.z = 0.0f;
                    kPlacement.kRotation.x = 0.0f;
                    kPlacement.kRotation.y = 0.0f;
                    kPlacement.kRotation.z = 0.0f;
                    SetDelete(false);
                    apFile->GetChunkData(&kPlacement, sizeof(REFR_PLACEMENT_DATA));
                    if (apFile->QEndian()) {
                        kPlacement.Endian();
                    }
                    data.kPosition = kPlacement.kPosition;
                    data.kRotation = kPlacement.kRotation;
                }
                break;
            case RCLR_ID: break;
            case AMRK_ID:
            case MMRK_ID:
            case RNAM_ID:
            case XACR_ID:
            case XACT_ID:
            case XAMC_ID:
            case XAMT_ID:
            case XAPD_ID:
            case XAPR_ID:
            case XATO_ID:
            case XCAS_ID:
            case XCCM_ID:
            case XCET_ID:
            case XCHG_ID:
            case XCIM_ID:
            case XCLM_ID:
            // XLCR_ID holds 0x504C4358, the 'XCLP' chunk.
            case XLCR_ID:
            case XCLR_ID:
            case XCMO_ID:
            case XCMT_ID:
            case XCNT_ID:
            case XCWT_ID:
            case XDCR_ID:
            case XEDL_ID:
            case XEMI_ID:
            case XESP_ID:
            case XEZN_ID:
            case XGLB_ID:
            case XHLP_ID:
            case XHLT_ID:
            case XHRS_ID:
            case XIBS_ID:
            case XLCM_ID:
            case XLKR_ID:
            case XLMB_ID:
            case XLOC_ID:
            case XLOD_ID:
            case XLTW_ID:
            case XMBO_ID:
            case XMBP_ID:
            case XMBR_ID:
            case XMRC_ID:
            case XMRK_ID:
            case XNDP_ID:
            case XNVP_ID:
            case XOCP_ID:
            case XORD_ID:
            case XOWN_ID:
            case XPCI_ID:
            case XPOD_ID:
            case XPPA_ID:
            case XPRD_ID:
            case XPRM_ID:
            case XPSL_ID:
            case XPSN_ID:
            case XPTL_ID:
            case XPWR_ID:
            case XRAD_ID:
            case XRDO_ID:
            case XRDS_ID:
            case XRGB_ID:
            case XRGD_ID:
            case XRMR_ID:
            case XRNK_ID:
            case XROO_ID:
            case XRTM_ID:
            case XSED_ID:
            case XSOL_ID:
            case XSRD_ID:
            case XSRF_ID:
            case XTEL_ID:
            case XTIM_ID:
            case XTRG_ID:
            case XTRI_ID:
            case XUSE_ID:
            case XWLT_ID: GetExtra()->Load(apFile, this); break;
            default:
                Warning("MASTERFILE: TESObjectREFR::Load abnormally terminated.\n\n");
                eChunk = DATA_ID;
                break;
        }
        if (!apFile->NextChunk()) {
            break;
        }
    }
    PatrolRefData* pPatrolData = GetExtra()->GetPatrolRefData();
    if (pPatrolData) {
        Script* pScript = pPatrolData->GetOnPatrolScript();
        if (pScript && !pScript->GetHeader()->dataSize && pScript->GetText() && !pScript->GetIsCompiled()) {
            alignas(BSStringT<char>) uint8_t aucMessage[sizeof(BSStringT<char>)];
            BSStringT<char>* pMessage = ::new (aucMessage) BSStringT<char>;
            pMessage->SPrintF(
                "Patrol point event script for point %s has text but has not been compiled.",
                GetFormEditorID()
            );
            Warning(pMessage->String());
            pMessage->~BSStringT<char>();
        }
    }
    return data.pObjectReference != nullptr;
}

// The reference's form record: the base object's form ID, the extra data, the scale when it is not 1.0 (or is NaN),
// ONAM for action 8, then DATA (position and rotation), byte-swapped around the write when saving byte-swapped.
void TESObjectREFR::Save() {
    StartForm();
    if (GetDelete()) {
        CloseForm();
        return;
    }
    TESForm::AddChunk(NAME_ID, data.pObjectReference->GetFormID());
    GetExtra()->Save();
    if (fRefScale != 1.0) {
        TESForm::AddChunk(XSCL_ID, fRefScale);
    }
    if (GetAction(OBJECT_ACTION_OPEN_BY_DEFAULT)) {
        TESForm::AddChunk(ONAM_ID);
    }
    REFR_PLACEMENT_DATA kPlacement;
    kPlacement.kPosition = data.kPosition;
    kPlacement.kRotation = data.kRotation;
    if (TESForm::QEndianSwapOnSave()) {
        kPlacement.Endian();
    }
    TESForm::__AddChunkData(DATA_ID, &kPlacement, sizeof(REFR_PLACEMENT_DATA));
    if (TESForm::QEndianSwapOnSave()) {
        kPlacement.Endian();
    }
    CloseForm();
}

// Whether this reference is saved before apForm: a cell child in the same save cell orders by type and persistence (a
// persistent reference before a temporary one, land and navmesh after persistent references); otherwise the save cell
// decides, through its SavesBefore(TESForm*).
bool TESObjectREFR::SavesBefore(TESForm* apForm) {
    bool bretval = false;
    uint8_t FormType = apForm->GetFormType();
    if (TESObjectCELL::IsFormTypeCellChild(FormType)) {
        TESChildCell* pcellchild = dynamic_cast<TESChildCell*>(apForm);
        TESObjectCELL* pformcell = pcellchild->GetSaveParentCell();
        if (pformcell == GetSaveParentCell()) {
            switch (apForm->GetFormType()) {
                case REFR_ID:
                case ACHR_ID:
                case ACRE_ID:
                case PMIS_ID:
                case PGRE_ID:
                case PBEA_ID:
                case PFLA_ID:
                case PCBE_ID: {
                    TESObjectREFR* pref = dynamic_cast<TESObjectREFR*>(apForm);
                    if (GetRefPersists()) {
                        if (pref) {
                            bretval = !pref->GetRefPersists();
                        }
                    }
                    else {
                        bretval = false;
                    }
                    break;
                }
                case LAND_ID:
                case NAVM_ID: {
                    bretval = GetRefPersists();
                    break;
                }
                default: {
                    return bretval;
                }
            }
        }
        else {
            bretval = GetSaveParentCell()->SavesBefore(pformcell);
        }
    }
    else {
        bretval = GetSaveParentCell()->SavesBefore(apForm);
    }
    return bretval;
}

bool TESObjectREFR::SavesBefore(FORM* apGroupFORM) {
    if (apGroupFORM && apGroupFORM->form == formEnumString[2].uiFormString) {
        uint32_t iFormID = apGroupFORM->iFormID;
        if (iFormID < 8 || iFormID > 9) {
            TESObjectCELL* pcell = GetSaveParentCell();
            return pcell->SavesBefore(apGroupFORM);
        }

        TESForm* pForm = TESForm::GetFormByNumericID(apGroupFORM->flags);
        TESObjectCELL* pcell = dynamic_cast<TESObjectCELL*>(pForm);
        if (pcell && GetSaveParentCell() == pcell) {
            return GetRefPersists() && apGroupFORM->iFormID == 9;
        }
    }

    return false;
}

TESForm* TESObjectREFR::CreateDuplicateForm(bool abCreateEditorID, NiTPointerMap<TESForm*, TESForm*>* apCopyMap) {
    BSExtraData* pExtraData = m_Extra.GetExtraData(43);
    BSExtraData* pTeleport = dynamic_cast<ExtraTeleport*>(pExtraData);
    if (pTeleport) {
        m_Extra.RemoveExtra(pTeleport, false);
    }
    pExtraData = m_Extra.GetExtraData(55);
    BSExtraData* pEnableStateParent = dynamic_cast<ExtraEnableStateParent*>(pExtraData);
    if (pEnableStateParent) {
        m_Extra.RemoveExtra(pEnableStateParent, false);
    }
    pExtraData = m_Extra.GetExtraData(59);
    BSExtraData* pRandomTeleportMarker = dynamic_cast<ExtraRandomTeleportMarker*>(pExtraData);
    if (pRandomTeleportMarker) {
        m_Extra.RemoveExtra(pRandomTeleportMarker, false);
    }
    pExtraData = m_Extra.GetExtraData(81);
    BSExtraData* pLinkedRef = dynamic_cast<ExtraLinkedRef*>(pExtraData);
    if (pLinkedRef) {
        m_Extra.RemoveExtra(pLinkedRef, false);
    }
    pExtraData = m_Extra.GetExtraData(81);
    BSExtraData* pActivateRef = dynamic_cast<ExtraActivateRef*>(pExtraData);
    if (pActivateRef) {
        m_Extra.RemoveExtra(pActivateRef, false);
    }
    pExtraData = m_Extra.GetExtraData(87);
    BSExtraData* pDecalRefs = dynamic_cast<ExtraDecalRefs*>(pExtraData);
    if (pDecalRefs) {
        m_Extra.RemoveExtra(pDecalRefs, false);
    }
    pExtraData = m_Extra.GetExtraData(60);
    BSExtraData* pMerchantContainer = dynamic_cast<ExtraMerchantContainer*>(pExtraData);
    if (pMerchantContainer) {
        m_Extra.RemoveExtra(pMerchantContainer, false);
    }
    pExtraData = m_Extra.GetExtraData(99);
    BSExtraData* pMultiBoundRef = dynamic_cast<ExtraMultiBoundRef*>(pExtraData);
    if (pMultiBoundRef) {
        m_Extra.RemoveExtra(pMultiBoundRef, false);
    }
    TESObjectCELL* pParentCell = GetParentCell();
    SetParentCell(nullptr);
    TESForm* pDuplicate = TESForm::CreateDuplicateForm(false, apCopyMap);
    TESObjectREFR* pCopy = dynamic_cast<TESObjectREFR*>(pDuplicate);
    if (pTeleport) {
        m_Extra.AddExtra(pTeleport);
    }
    if (pEnableStateParent) {
        m_Extra.AddExtra(pEnableStateParent);
    }
    if (pRandomTeleportMarker) {
        m_Extra.AddExtra(pRandomTeleportMarker);
    }
    if (pLinkedRef) {
        m_Extra.AddExtra(pLinkedRef);
    }
    if (pActivateRef) {
        m_Extra.AddExtra(pActivateRef);
    }
    if (pDecalRefs) {
        m_Extra.AddExtra(pDecalRefs);
    }
    if (pMerchantContainer) {
        m_Extra.AddExtra(pMerchantContainer);
    }
    if (pMultiBoundRef) {
        m_Extra.AddExtra(pMultiBoundRef);
    }
    SetParentCell(pParentCell);
    pCopy->Set3D(nullptr, true);
    pCopy->SetRefPersists(false);
    return pCopy;
}

void TESObjectREFR::PostDuplicateProcess(NiTPointerMap<TESForm*, TESForm*>* apCopyMap) {
    if (apCopyMap) {
        TESForm* pcopy = nullptr;
        if (apCopyMap->GetAt(this, pcopy) && pcopy) {
            TESObjectREFR* pcopyref = dynamic_cast<TESObjectREFR*>(pcopy);
            if (pcopyref) {
                int32_t i;
                TESForm* ptemp;
                TESObjectREFR* pesparent = GetEnableStateParent();
                if (pesparent) {
                    ptemp = nullptr;
                    if (apCopyMap->GetAt(pesparent, ptemp) && ptemp) {
                        TESObjectREFR* pcopyesparent = dynamic_cast<TESObjectREFR*>(ptemp);
                        if (pcopyesparent) {
                            pcopyref->SetEnableStateParent(pcopyesparent);
                            pcopyref->SetEnableStateInverse(GetEnableStateInverse());
                            pcopyesparent->m_Extra.AddEnableStateChild(pcopyref);
                        }
                    }
                }
                TESObjectREFR* ptelmark = GetRandomTeleportMarker();
                if (ptelmark) {
                    ptemp = nullptr;
                    if (apCopyMap->GetAt(ptelmark, ptemp) && ptemp) {
                        TESObjectREFR* pcopytelmark = dynamic_cast<TESObjectREFR*>(ptemp);
                        if (pcopytelmark) {
                            pcopyref->SetRandomTeleportMarker(pcopytelmark);
                        }
                    }
                }
                TESObjectREFR* plinkedref = GetLinkedRef();
                if (plinkedref) {
                    ptemp = nullptr;
                    if (apCopyMap->GetAt(plinkedref, ptemp) && ptemp) {
                        TESObjectREFR* pcopylinkedref = dynamic_cast<TESObjectREFR*>(ptemp);
                        if (pcopylinkedref) {
                            pcopyref->SetLinkedRef(pcopylinkedref);
                            pcopylinkedref->m_Extra.AddLinkedRefChild(pcopyref);
                        }
                    }
                }
                for (i = 0; i < 2; ++i) {
                    TESObjectREFR* proomref = m_Extra.GetPortalLinkedRoomRef(i);
                    if (proomref) {
                        ptemp = nullptr;
                        if (apCopyMap->GetAt(proomref, ptemp) && ptemp) {
                            TESObjectREFR* pcopyroomref = dynamic_cast<TESObjectREFR*>(ptemp);
                            if (pcopyroomref) {
                                pcopyref->m_Extra.SetPortalLinkedRoomRef(i, pcopyroomref);
                            }
                        }
                        else {
                            pcopyref->m_Extra.SetPortalLinkedRoomRef(i, proomref);
                        }
                    }
                }
                for (i = 0; i < 4; ++i) {
                    TESObjectREFR* pplaneref = m_Extra.GetLinkedPlaneRef(i);
                    if (pplaneref) {
                        ptemp = nullptr;
                        if (apCopyMap->GetAt(pplaneref, ptemp) && ptemp) {
                            TESObjectREFR* pcopyPlaneref = dynamic_cast<TESObjectREFR*>(ptemp);
                            if (pcopyPlaneref) {
                                pcopyref->m_Extra.SetLinkedPlaneRef(pcopyPlaneref, i);
                            }
                        }
                    }
                }
                BSSimpleList<REF_ACTIVATE_DATA*>* pactlist = m_Extra.GetActivateParentList();
                bool baddedact = false;
                while (pactlist && !pactlist->IsEmpty()) {
                    REF_ACTIVATE_DATA* pdata = pactlist->m_item;
                    pactlist = pactlist->m_pkNext;
                    ptemp = nullptr;
                    if (apCopyMap->GetAt(pdata->pActivateRef, ptemp) && ptemp) {
                        TESObjectREFR* pcopyactivateref = dynamic_cast<TESObjectREFR*>(ptemp);
                        if (pcopyactivateref) {
                            pcopyref->m_Extra.AddActivateParent(pcopyactivateref);
                            pcopyref->m_Extra.SetActivateDelay(pcopyactivateref, pdata->fActivateDelay);
                            baddedact = true;
                            pcopyactivateref->m_Extra.AddActivateRefChild(pcopyref);
                        }
                    }
                }
                if (baddedact) {
                    pcopyref->m_Extra.SetActivateParentOnly(m_Extra.IsActivateParentOnly());
                }
                for (BSSimpleList<REF_DECAL_DATA*>* pdecallist = m_Extra.GetDecalRefs();
                     pdecallist && !pdecallist->IsEmpty();
                     pdecallist = pdecallist->m_pkNext) {
                    REF_DECAL_DATA* pDecalData = pdecallist->m_item;
                    ptemp = nullptr;
                    if (apCopyMap->GetAt(pDecalData->pDecalRef, ptemp) && ptemp) {
                        TESObjectREFR* pcopydecal = dynamic_cast<TESObjectREFR*>(ptemp);
                        if (pcopydecal) {
                            pcopyref->AddDecalRef(pcopydecal, pDecalData->Intersect, pDecalData->Normal);
                        }
                    }
                }
                TESObjectREFR* pmerchcont = GetMerchantContainer();
                if (pmerchcont) {
                    ptemp = nullptr;
                    if (apCopyMap->GetAt(pmerchcont, ptemp) && ptemp) {
                        TESObjectREFR* pcopymerchcont = dynamic_cast<TESObjectREFR*>(ptemp);
                        if (pcopymerchcont) {
                            pcopyref->SetMerchantContainer(pcopymerchcont);
                        }
                    }
                }
                DoorTeleportData* ptel = GetTeleport();
                if (ptel && ptel->pLinkedDoor) {
                    ptemp = nullptr;
                    if (apCopyMap->GetAt(ptel->pLinkedDoor, ptemp) && ptemp) {
                        TESObjectREFR* pcopylinkeddoor = dynamic_cast<TESObjectREFR*>(ptemp);
                        if (pcopylinkeddoor) {
                            DoorTeleportData* pcopytel = pcopyref->AddTeleport();
                            pcopytel->pLinkedDoor = pcopylinkeddoor;
                            NiPoint3 telangle = *&ptel->rotation;
                            NiPoint3 teloffset = *&ptel->position - ptel->pLinkedDoor->GetLocationOnReference();
                            NiPoint3 telpos = pcopylinkeddoor->GetLocationOnReference() + teloffset;
                            pcopytel->SetTeleportPosition(telpos);
                            pcopytel->SetTeleportRotation(telangle);
                        }
                    }
                }
            }
        }
    }
}

// The bytes the reference takes in a save: the base form's size, the save-game block header, the container changes,
// the extra data list, the attached animation, the Havok data and the scale, logged against the header of the form
// being saved when bDebugSaveBuffer is set.
uint16_t TESObjectREFR::GetSaveSize(uint32_t aiFlags) {
    uint16_t usSize = TESForm::GetSaveSize(aiFlags);
    uint16_t usStartSize = usSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        usSize += 4;
        usSize += 2;
    }
    if ((aiFlags & CHANGE_REFR_INVENTORY) != 0) {
        ExtraDataList* pExtra = TESObjectREFR::GetExtra();
        InventoryChanges* pChanges = pExtra->GetContainerChanges();
        usSize = static_cast<uint16_t>(usSize + pChanges->GetSaveSize());
    }
    if (IsActor()) {
        ExtraDataList* pExtra = TESObjectREFR::GetExtra();
        usSize = static_cast<uint16_t>(usSize + pExtra->GetSaveSize(aiFlags, this));
    }
    if ((aiFlags & CHANGE_REFR_ANIMATION) != 0 && !IsActor()) {
        usSize += 2;
        usSize = static_cast<uint16_t>(usSize + GetAttachedAnimationSaveSize());
    }
    if ((aiFlags & CHANGE_REFR_HAVOK_MOVE) != 0) {
        usSize += 2;
        usSize = static_cast<uint16_t>(usSize + GetHavokDataSaveSize(nullptr));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x43 && (aiFlags & CHANGE_REFR_SCALE) != 0) {
        usSize += 4;
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlySavingFormHeader();
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                usSize - usStartSize,
                pHeader->iFormID,
                pForm->GetFormEditorID(),
                pHeader->iFlags,
                2768,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectREFR.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                usSize - usStartSize,
                2768,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectREFR.cpp"
            );
        }
    }
    return usSize;
}

// The old save of the reference: the base form's data, a 'BLOK' block whose size is patched in at the end, the
// container changes, an actor's extra data, the attached animation, the Havok data and the scale, each by change flag;
// logged when bDebugSaveBuffer is set. The block size is written through the buffer position taken after the 'BLOK' id.
void TESObjectREFR::SaveGame(uint32_t aiFlags) {
    TESForm::SaveGame(aiFlags);
    uint16_t usBlockSize = 0;
    char* pBlockSize = nullptr;
    char* pStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
    if (bDebugSaveBuffer.Bool()) {
        pStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t uiBlockID = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&uiBlockID, sizeof(uiBlockID));
        pBlockSize = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->SaveGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    if ((aiFlags & CHANGE_REFR_INVENTORY) != 0) {
        TESObjectREFR::GetExtra()->GetContainerChanges()->SaveGame();
    }
    if (IsActor()) {
        TESObjectREFR::GetExtra()->SaveGame(aiFlags, this);
    }
    if ((aiFlags & CHANGE_REFR_ANIMATION) != 0 && !IsActor()) {
        uint16_t usAnimationSize = GetAttachedAnimationSaveSize();
        TESForm::SaveGameDataOLD(&usAnimationSize, sizeof(usAnimationSize));
        if (usAnimationSize) {
            SaveAttachedAnimation();
        }
    }
    if ((aiFlags & CHANGE_REFR_HAVOK_MOVE) != 0) {
        HavokSaveData kHavokSaveData;
        uint16_t usHavokSize = GetHavokDataSaveSize(&kHavokSaveData);
        TESForm::SaveGameDataOLD(&usHavokSize, sizeof(usHavokSize));
        if (usHavokSize) {
            SaveHavokData(&kHavokSaveData);
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x43 && (aiFlags & CHANGE_REFR_SCALE) != 0) {
        TESForm::SaveGameDataOLD(&fRefScale, sizeof(fRefScale));
    }
    if (bDebugSaveBuffer.Bool()) {
        char* pEnd = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        SaveFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlySavingFormHeader();
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            uint32_t uiFlags = pHeader->iFlags;
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pStart),
                pHeader->iFormID,
                pEditorID,
                uiFlags,
                2817,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectREFR.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pStart),
                2817,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectREFR.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pEnd = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        if (pEnd > pBlockSize + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectREFR.cpp",
                2817
            );
        }
        // The save buffer is raw bytes: the block's 16-bit size is patched in place.
        *reinterpret_cast<uint16_t*>(pBlockSize) = static_cast<uint16_t>(pEnd - pBlockSize);
    }
}

// The save: the base form's save, the reference scale and the extra data by change flags, the inventory, then the
// animation (into a sub-buffer, or measured around SaveAnimation).
void TESObjectREFR::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint32_t uiFlags;
    TESForm::SaveGame(apSaveGameBuffer);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_SCALE)) {
        // SaveDataEndian is handed the float's 4 raw bytes.
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fRefScale), sizeof(fRefScale), 0);
    }
    uiFlags = IsActor() ? CHANGE_REFR_EXTRA_OWNERSHIP | CHANGE_REFR_EXTRA_GAME_ONLY | CHANGE_REFR_EXTRA_ENCOUNTER_ZONE |
            CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN | CHANGE_ACTOR_EXTRA_PACKAGE_DATA | CHANGE_ACTOR_EXTRA_LEVELED_ACTOR |
            CHANGE_ACTOR_EXTRA_MERCHANT_CONTAINER | CHANGE_ACTOR_EXTRA_DISMEMBERED_LIMBS
                        : CHANGE_REFR_EXTRA_OWNERSHIP | CHANGE_REFR_EXTRA_GAME_ONLY | CHANGE_REFR_EXTRA_ENCOUNTER_ZONE |
            CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN | CHANGE_OBJECT_EXTRA_ITEM_DATA | CHANGE_OBJECT_EXTRA_AMMO |
            CHANGE_OBJECT_EXTRA_LOCK | CHANGE_DOOR_EXTRA_TELEPORT;
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(uiFlags)) {
        TESObjectREFR::GetExtra()->SaveGame(apSaveGameBuffer);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_INVENTORY | CHANGE_REFR_LEVELED_INVENTORY)) {
        InventoryChanges::GetInventoryChanges(this)->SaveGameBGS(apSaveGameBuffer);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_ANIMATION) && !IsActor()) {
        char* pSavedAnimation = TESObjectREFR::GetExtra()->GetSavedAnimation();
        BGSLoadGameSubBuffer kSubBuffer(pSavedAnimation);
        if (TESObjectREFR::Get3D() == nullptr && static_cast<char*>(kSubBuffer) != nullptr) {
            kSubBuffer.SaveGame(apSaveGameBuffer);
        }
        else {
            uint32_t uiStart = apSaveGameBuffer->StartVariableSizedValue();
            uint32_t uiBefore = apSaveGameBuffer->GetBufferPosition();
            SaveAnimation(apSaveGameBuffer);
            uint32_t uiAfter = apSaveGameBuffer->GetBufferPosition();
            apSaveGameBuffer->SaveVariableSizedValue(uiAfter - uiBefore, uiStart);
        }
    }
}

// The old load of the reference: the base form's data, the 'BLOK' block header, the enable state from the enable parent
// while loading, the 3D of a disabled or deleted reference dropped, the inventory, the extra data, the queued animation
// and Havok data, the scale and the open state by change flags; the water state is reset and the block's size checked
// against what was read. The creature branch calls two shared empty stubs (one with 1, the other twice).
void TESObjectREFR::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
    if (!IsActor() && (aiFlags & CHANGE_OBJECT_EMPTY) != 0) {
        SetEmpty(true);
    }
    uint16_t usBlockSize = 0;
    char* pBlockStart = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t uiBlockID;
        pSaveLoadGameOLD->LoadGameDataOLD(&uiBlockID, sizeof(uiBlockID));
        if (uiBlockID != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                uint32_t uiFlags = pHeader->iFlags;
                uint32_t uiVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Currently loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectREFR.cpp",
                    2832,
                    pHeader->iFormID,
                    pEditorID,
                    uiVersion,
                    uiFlags
                );
            }
            else {
                uint32_t uiVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Current version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectREFR.cpp",
                    2832,
                    uiVersion
                );
            }
        }
        pBlockStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->LoadGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    if (pSaveLoadGameOLD->GetLoading()) {
        TESObjectREFR* pParent = GetEnableStateParent();
        if (pParent) {
            if (GetEnableStateInverse()) {
                SetDisabled(!pParent->GetDisabled());
            }
            else {
                SetDisabled(pParent->GetDisabled());
            }
        }
    }
    if ((aiFlags & CHANGE_FORM_FLAGS) != 0 && (GetDisabled() || GetDelete())) {
        Set3D(nullptr, true);
    }
    if ((aiFlags & CHANGE_REFR_INVENTORY) != 0 && HasContainer()) {
        if (IsActor() && IsCreature()) {
            RemoveWeapon();
            TESForm::Revert(1);
            RemoveAmmo();
            RemoveAmmo();
        }
        TESObjectREFR::GetExtra()->RemoveContainerChanges();
        Actor* pActor = nullptr;
        if (IsActor()) {
            pActor = static_cast<Actor*>(this);
        }
        if (pActor) {
            pActor->ClearProcessItems();
        }
        InventoryChanges::GetInventoryChanges(this)->LoadGame();
    }
    TESObjectREFR::GetExtra()->RemoveAllSaveableData(aiFlags | aiCurrentFlags, this);
    uint32_t uiExtraFlags = 0;
    if (pSaveLoadGameOLD->GetCurrentVersion() < 0x43) {
        uiExtraFlags |= CHANGE_REFR_SCALE;
    }
    if ((aiFlags & uiExtraFlags) != 0 || IsActor()) {
        TESObjectREFR::GetExtra()->LoadGame(aiFlags, aiCurrentFlags, this);
    }
    if ((aiFlags & CHANGE_REFR_ANIMATION) != 0 && !IsActor()) {
        uint16_t usAnimationSize;
        TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&usAnimationSize), sizeof(usAnimationSize));
        if (usAnimationSize) {
            pSaveLoadGameOLD->QueueAttachedAnimationLoad(this, usAnimationSize);
        }
    }
    if ((aiFlags & CHANGE_REFR_HAVOK_MOVE) != 0) {
        uint16_t usHavokSize;
        TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&usHavokSize), sizeof(usHavokSize));
        if (usHavokSize) {
            pSaveLoadGameOLD->QueueHavokDataLoad(this, usHavokSize);
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x43 && (aiFlags & CHANGE_REFR_SCALE) != 0) {
        TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&fRefScale), sizeof(fRefScale));
        SetScale(fRefScale);
    }
    if ((aiFlags & CHANGE_OBJECT_OPEN_DEFAULT_STATE) != 0) {
        if (TESObjectREFR::GetExtra()->GetAction(OBJECT_ACTION_OPEN_BY_DEFAULT)) {
            TESObjectREFR::GetExtra()->ClearAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
        }
        else {
            TESObjectREFR::GetExtra()->SetAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
        }
    }
    if ((aiFlags & CHANGE_OBJECT_OPEN_STATE) != 0) {
        TESObjectREFR::GetExtra()->RemoveLastFinishedSequence();
    }
    if (pLoadedData) {
        // -FLT_MAX.
        pLoadedData->fRelevantWaterHeight = -std::numeric_limits<float>::max();
        pLoadedData->iUnderwaterCount = 0;
        pLoadedData->pCurrentWaterObject = nullptr;
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pCurrent = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        char* pBlockEnd = pBlockStart + usBlockSize;
        LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            if (pCurrent > pBlockEnd) {
                uint32_t uiFlags = pHeader->iFlags;
                uint32_t uiVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  "
                    "Currently loading form is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pCurrent - pBlockEnd),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectREFR.cpp",
                    2959,
                    pHeader->iFormID,
                    pEditorID,
                    uiVersion,
                    uiFlags
                );
            }
            else if (pCurrent < pBlockEnd) {
                uint32_t uiFlags = pHeader->iFlags;
                uint32_t uiVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  "
                    "Currently loading form is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pBlockEnd - pCurrent),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectREFR.cpp",
                    2959,
                    pHeader->iFormID,
                    pEditorID,
                    uiVersion,
                    uiFlags
                );
            }
        }
        else if (pCurrent > pBlockEnd) {
            uint32_t uiVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pCurrent - pBlockEnd),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectREFR.cpp",
                2959,
                uiVersion
            );
        }
        else if (pCurrent < pBlockEnd) {
            uint32_t uiVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pBlockEnd - pCurrent),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectREFR.cpp",
                2959,
                uiVersion
            );
        }
    }
}

void TESObjectREFR::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    uint8_t cIsActor = IsActor();
    if (!cIsActor) {
        if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_OBJECT_EMPTY)) {
            TESForm::SetEmpty(true);
        }
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_SCALE)) {
        apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&fRefScale)), sizeof(fRefScale));
        SetScale(fRefScale);
    }
    uint32_t uiExtraFlags = static_cast<uint32_t>(cIsActor ? (-1543104448) : (-1543365568));
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(uiExtraFlags)) {
        TESObjectREFR::GetExtra()->LoadGameBGS(apLoadGameBuffer);
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_INVENTORY | CHANGE_REFR_LEVELED_INVENTORY)) {
        TESObjectREFR::GetExtra()->RemoveContainerChanges();
        InventoryChanges* pInventoryChanges = InventoryChanges::GetInventoryChanges(this);
        pInventoryChanges->LoadGame_ObjectsList(apLoadGameBuffer);
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_ANIMATION) && !IsActor()) {
        pSaveLoadGame->QueueSubBuffer(QUEUED_SUB_BUFFER_ANIMATION, apLoadGameBuffer, nullptr);
    }
    if (!IsActor()) {
        if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_OBJECT_OPEN_DEFAULT_STATE)) {
            if (TESObjectREFR::GetExtra()->GetAction(OBJECT_ACTION_OPEN_BY_DEFAULT)) {
                TESObjectREFR::GetExtra()->ClearAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
            }
            else {
                TESObjectREFR::GetExtra()->SetAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
            }
        }
    }
    if (TESForm::GetDestructible() && pLoadedData && pLoadedData->m_spData3D.m_pObject) {
        if (TESObjectREFR::GetExtra()->GetObjectHealth() != -1.0 && TESObjectREFR::GetObjectReference()) {
            TESBoundObject* pObjectReference = TESObjectREFR::GetObjectReference();
            BGSDestructibleObjectForm* pDestructionForm = BGSDestructibleObjectForm::GetDestructionForm(
                pObjectReference
            );
            if (pDestructionForm->UpdateCurrentDamageStage(this, false)) {
                UpdateAddonNodeSounds(false);
            }
        }
    }

    UpdateLODMultInvisibility(this, Get3D());
}

// The TES-format load: the base form's empty stub, the enable state from the parent, then the container and the extra
// data. A second disabled/deleted block sits behind two flag masks of 0, so it never runs: only the IsMobileObject call
// that feeds its guard is kept.
void TESObjectREFR::InitLoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESObjectREFR* pEnableStateParent;
    TESForm::InitLoadGame(aiFlags, aiCurrentFlags);
    pEnableStateParent = GetEnableStateParent();
    if (pEnableStateParent) {
        if (GetEnableStateInverse()) {
            SetDisabled(!pEnableStateParent->GetDisabled());
        }
        else {
            SetDisabled(pEnableStateParent->GetDisabled());
        }
    }
    if (((aiFlags & CHANGE_FORM_FLAGS) != 0 || pEnableStateParent != nullptr) && (GetDisabled() || GetDelete())) {
        Set3D(nullptr, true);
    }
    if ((aiFlags & CHANGE_REFR_INVENTORY) != 0 && HasContainer()) {
        InventoryChanges::GetInventoryChanges(this)->InitLoadGameTES();
    }

    // The result only guards the disabled/deleted Set3D block below, whose masks are 0; the call stays.
    if (IsMobileObject()) {}
    if (IsActor()) {
        TESBoundObject* pBaseObject = data.pObjectReference;
        TESObjectREFR::GetExtra()->InitLoadGameTES(aiFlags, aiCurrentFlags, this, pBaseObject);
    }
}

// The BGS-format load: the base form's empty stub, the enable state from the parent, the extra data and the inventory
// by change flags, then the tree LOD of a disabled reference whose base object has spoken.
void TESObjectREFR::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* pEnableStateParent;
    uint32_t uiFlags;
    TESForm::Revert(apLoadGameBuffer);
    pEnableStateParent = GetEnableStateParent();
    if (pEnableStateParent) {
        if (GetEnableStateInverse()) {
            SetDisabled(!pEnableStateParent->GetDisabled());
        }
        else {
            SetDisabled(pEnableStateParent->GetDisabled());
        }
    }
    uiFlags = IsActor() ? CHANGE_REFR_EXTRA_OWNERSHIP | CHANGE_REFR_EXTRA_GAME_ONLY | CHANGE_REFR_EXTRA_ENCOUNTER_ZONE |
            CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN | CHANGE_ACTOR_EXTRA_PACKAGE_DATA | CHANGE_ACTOR_EXTRA_LEVELED_ACTOR |
            CHANGE_ACTOR_EXTRA_MERCHANT_CONTAINER | CHANGE_ACTOR_EXTRA_DISMEMBERED_LIMBS
                        : CHANGE_REFR_EXTRA_OWNERSHIP | CHANGE_REFR_EXTRA_GAME_ONLY | CHANGE_REFR_EXTRA_ENCOUNTER_ZONE |
            CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN | CHANGE_OBJECT_EXTRA_ITEM_DATA | CHANGE_OBJECT_EXTRA_AMMO |
            CHANGE_OBJECT_EXTRA_LOCK | CHANGE_DOOR_EXTRA_TELEPORT;
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(uiFlags)) {
        TESBoundObject* pObjectReference = data.pObjectReference;
        TESObjectREFR::GetExtra()->InitLoadGame(apLoadGameBuffer, pObjectReference);
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_INVENTORY | CHANGE_REFR_LEVELED_INVENTORY)) {
        InventoryChanges::GetInventoryChanges(this)->InitLoadGame(apLoadGameBuffer);
    }
    if (GetObjectReference() && (GetObjectReference()->TESForm::GetHasSpokenFlag() && GetDisabled())) {
        TESWorldSpace* pWorldSpace = GetWorldSpace();
        if (pWorldSpace) {
            pWorldSpace->GetTerrainManager()->HideTree(this, true);
        }
    }
}

// Finishes the TES-format load: the base form's empty stub, the container's scripts, the 3D's transform and update,
// then the actor's extra data. The base form's stub is reached through TESForm::InitLoadGame, whose body is empty.
void TESObjectREFR::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    NiNode* pNode;
    TESForm::InitLoadGame(aiFlags, aiOldFlags);
    if ((aiFlags & CHANGE_REFR_INVENTORY) != 0 && HasContainer()) {
        InventoryChanges::GetInventoryChanges(this)->InitScripts();
    }

    pNode = TESObjectREFR::Get3D();
    if (pNode) {
        pNode->SetTranslate(this->GetLocationOnReference());
        NiMatrix3 kOrientation = GetOrientation();
        pNode->SetRotate(kOrientation);
        bhkWorld::UpdatePosition(pNode, true, false);
        NiUpdateData kUpdateData(0.0f, false, false);
        pNode->Update(kUpdateData);
    }
    if (IsActor()) {
        TESObjectREFR::GetExtra()->FinishInitLoadGame(aiFlags, aiOldFlags, this);
    }
    SetStillLoading(false);
}

// The old revert of the reference: the base form's (empty) revert, the animation, an actor's extra data and the
// inventory by change flags, then an open/close form's open state, a dead reference's ragdoll and the sound of a
// disabled, deleted or destroyed sound object. The blocks under the game's dead `aiFlags & 0` tests are left out: the
// re-queueing of the 3D while loading never runs (its test still calls GetLoading first, as here), and the extra data
// is reverted for actors only. The creature branch calls the shared empty stubs (one with 1, the other twice).
// ResetInventory is called directly outside a load and through the vtable during one.
void TESObjectREFR::Revert(uint32_t aiFlags) {
    TESForm::Revert(aiFlags);
    if (!IsActor() && (aiFlags & CHANGE_OBJECT_EMPTY) != 0) {
        SetEmpty(false);
    }
    pSaveLoadGameOLD->GetLoading();
    if ((aiFlags & CHANGE_REFR_ANIMATION) != 0 && !IsActor()) {
        ResetAnimation_Old();
    }
    if (IsActor()) {
        TESObjectREFR::GetExtra()->Revert(aiFlags, this);
    }
    if ((aiFlags & CHANGE_REFR_INVENTORY) != 0) {
        if (IsActor() && IsCreature()) {
            RemoveWeapon();
            TESForm::Revert(1);
            RemoveAmmo();
            RemoveAmmo();
        }
        if (!pSaveLoadGameOLD->GetLoading()) {
            TESObjectREFR::ResetInventory(false);
        }
        else {
            ResetInventory(false);
        }
    }
    if (BGSOpenCloseForm::IsOpenCloseForm(GetObjectReference())) {
        if ((aiFlags & CHANGE_OBJECT_OPEN_DEFAULT_STATE) != 0 && !pSaveLoadGameOLD->GetReconstructedAllObjects()) {
            if (TESObjectREFR::GetExtra()->GetAction(OBJECT_ACTION_OPEN_BY_DEFAULT)) {
                TESObjectREFR::GetExtra()->ClearAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
            }
            else {
                TESObjectREFR::GetExtra()->SetAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
            }
        }
        if (pSaveLoadGameOLD->IsUnk048Zero()) {
            BGSOpenCloseForm::SetOpenState(this, GetAction(OBJECT_ACTION_OPEN_BY_DEFAULT), true);
        }
    }
    if (pSaveLoadGameOLD->IsUnk048Zero() && IsDead(false)) {
        RestoreRagDollData(nullptr);
    }
    if (pSaveLoadGameOLD->IsUnk048Zero() && GetObjectReference()) {
        uint32_t eType = GetObjectReference()->GetFormType();
        if ((eType == ACTI_ID || eType == SOUN_ID || eType == DOOR_ID) &&
            (GetDisabled() || GetDelete() || (eType == DOOR_ID && GetDestroyed()))) {
            AttachSound(false);
        }
    }
}

void TESObjectREFR::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    BGSChangeFlags kOldFlags;
    TESForm::Revert(apLoadGameBuffer);
    if (pLoadedData) {
        pLoadedData->fCachedRadius = -1.0f;
    }
    bool bActor = IsActor();
    if (!bActor && apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_OBJECT_EMPTY)) {
        SetEmpty(false);
    }

    bool bDamaged = m_Extra.GetObjectHealth() != -1.0;
    uint32_t uiExtraFlags;
    if (bActor) {
        uiExtraFlags = CHANGE_REFR_EXTRA_OWNERSHIP | CHANGE_REFR_EXTRA_GAME_ONLY | CHANGE_REFR_EXTRA_ENCOUNTER_ZONE |
            CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN | CHANGE_ACTOR_EXTRA_PACKAGE_DATA | CHANGE_ACTOR_EXTRA_LEVELED_ACTOR |
            CHANGE_ACTOR_EXTRA_MERCHANT_CONTAINER | CHANGE_ACTOR_EXTRA_DISMEMBERED_LIMBS;
    }
    else {
        uiExtraFlags = CHANGE_REFR_EXTRA_OWNERSHIP | CHANGE_REFR_EXTRA_GAME_ONLY | CHANGE_REFR_EXTRA_ENCOUNTER_ZONE |
            CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN | CHANGE_OBJECT_EXTRA_ITEM_DATA | CHANGE_OBJECT_EXTRA_AMMO |
            CHANGE_OBJECT_EXTRA_LOCK | CHANGE_DOOR_EXTRA_TELEPORT;
    }
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(uiExtraFlags)) {
        m_Extra.Revert(apLoadGameBuffer);
    }
    if (bDamaged && GetDestructible() && pLoadedData && pLoadedData->m_spData3D.m_pObject) {
        BGSDestructibleObjectForm* pDestructionForm = BGSDestructibleObjectForm::GetDestructionForm(
            GetObjectReference()
        );
        if (pDestructionForm->UpdateCurrentDamageStage(this, true)) {
            UpdateAddonNodeSounds(false);
        }
    }

    BGSChangeFlags* pOldFlags = ::new (&kOldFlags) BGSChangeFlags(apLoadGameBuffer->GetOldChangeFlags());
    if (pOldFlags->CheckFlags(CHANGE_REFR_ANIMATION) && !IsActor()) {
        ResetAnimation();
    }
    m_Extra.RevertUnsavedExtra(apLoadGameBuffer);
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_REFR_INVENTORY | CHANGE_REFR_LEVELED_INVENTORY)) {
        if (apLoadGameBuffer->GetRevertOnly()) {
            ResetInventory(false);
        }
        else if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_REFR_INVENTORY)) {
            TESObjectREFR::ResetInventory(false);
        }
        else {
            TESObjectREFR::ResetInventory(true);
        }
    }
    if (!IsActor()) {
        if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_OBJECT_OPEN_DEFAULT_STATE)) {
            if (m_Extra.GetAction(OBJECT_ACTION_OPEN_BY_DEFAULT)) {
                m_Extra.ClearAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
            }
            else {
                m_Extra.SetAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
            }
        }
        bool bOpen = GetAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
        BGSOpenCloseForm::SetOpenState(this, bOpen, true);
    }
    if (Get3D()) {
        if (TESHavokUtilities::Should3DRagDoll(this, Get3D()) && !IsActor()) {
            RestoreRagDollData(nullptr);
        }
    }
    if (pParentCell && apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN)) {
        pParentCell->RemoveActivatingRef(this);
    }

    UpdateLODMultInvisibility(this, Get3D());
}

// Drops the animation change flag when the reference has a 3D but no save in progress and no playing animation.
void TESObjectREFR::CheckSaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::CheckSaveGame(apSaveGameBuffer);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_ANIMATION) &&
        !IsActor() &&
        TESObjectREFR::Get3D() &&
        (!pSaveLoadGame->GetSaveGameSaving() && !IsPlayingAnimation())) {
        apSaveGameBuffer->RemoveChange(CHANGE_REFR_ANIMATION);
    }
}

// Restores the door state, the parent cell's activating reference and the queued 3D. The flags it queries depend on
// IsActor.
void TESObjectREFR::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint32_t uiFlags;
    TESForm::FinishLoadGame(apLoadGameBuffer);
    uiFlags = IsActor() ? CHANGE_REFR_EXTRA_OWNERSHIP | CHANGE_REFR_EXTRA_GAME_ONLY | CHANGE_REFR_EXTRA_ENCOUNTER_ZONE |
            CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN | CHANGE_ACTOR_EXTRA_PACKAGE_DATA | CHANGE_ACTOR_EXTRA_LEVELED_ACTOR |
            CHANGE_ACTOR_EXTRA_MERCHANT_CONTAINER | CHANGE_ACTOR_EXTRA_DISMEMBERED_LIMBS
                        : CHANGE_REFR_EXTRA_OWNERSHIP | CHANGE_REFR_EXTRA_GAME_ONLY | CHANGE_REFR_EXTRA_ENCOUNTER_ZONE |
            CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN | CHANGE_OBJECT_EXTRA_ITEM_DATA | CHANGE_OBJECT_EXTRA_AMMO |
            CHANGE_OBJECT_EXTRA_LOCK | CHANGE_DOOR_EXTRA_TELEPORT;
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(uiFlags)) {
        TESObjectREFR::GetExtra()->FinishLoadGame(apLoadGameBuffer);
    }
    if (!IsActor() && apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_OBJECT_OPEN_STATE)) {
        if (TESObjectREFR::GetOriginalOpenByDefaultState()) {
            TESObjectREFR::ClearAction(OBJECT_ACTION_OPEN);
            BGSOpenCloseForm::SnapHavokTo3D(this);
            NavMeshObstacleManager::GetInstance()->OnDoorClose(this);
        }
        else {
            TESObjectREFR::SetAction(OBJECT_ACTION_OPEN);
            BGSOpenCloseForm::SnapHavokTo3D(this);
            NavMeshObstacleManager::GetInstance()->OnDoorOpen(this);
        }
    }

    if (pParentCell && apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN)) {
        pParentCell->AddActivatingRef(this);
    }

    if (!pSaveLoadGame->GetSaveGameLoading()) {
        if (TESForm::GetDisabledOrDeleted()) {
            Set3D(nullptr, false);
        }
        else if (!Get3D() && TESObjectREFR::GetParentCell()) {
            TESObjectCELL* pCell = TESObjectREFR::GetParentCell();
            if (pCell->GetAttached()) {
                TESObjectCELL* pPriorityCell = TESObjectREFR::GetParentCell();
                IO_TASK_PRIORITY ePriority = TES::pInstance->GetCellPriority(pPriorityCell, nullptr);
                pModelLoader->QueueReference(this, ePriority, true);
            }
        }
    }
}

// Finishes a loaded reference once: registers radio stations, warns about leveled-actor bases and resets a corrupt
// position or angle, initialises the extra data (scaling the health to the base's), gives single ammo a full clip,
// moves a scaled multibound marker's scale into its half extents, sets up an actor (start position, death, guard flag,
// leveled template, low processing, the player's run speed) or a movable object's start position, follows the enable
// parent, then initialises the container and the script and hides the LOD tree of a disabled tree. Warnings raised
// meanwhile are summarised in one. Passes 0 as BGSTalkingActivator::IsRadioStation's aiData. Calls the "loading" stub
// TESSaveLoadGame::GetLoading twice in the enable-parent test (`!a || a`). For the reference's own
// editor ID it calls TESForm::GetFormEditorID directly, not virtually.
void TESObjectREFR::InitItem() {
    if (GetInitialized()) {
        return;
    }
    if (BGSTalkingActivator::IsRadioStation(GetObjectReference(), 0)) {
        BGSTalkingActivator::AddRadioStation(this);
    }
    uint32_t uiWarningCount = MessageHandler::QWarningCount();
    MessageHandler::IncDisableWarningCount(true);
    MessageHandler::SetWarningCount(0);
    if (data.pObjectReference) {
        int32_t iFormType = data.pObjectReference->GetFormType();
        if (iFormType >= LVLC_ID && iFormType <= LVLN_ID) {
            Warning(
                "MASTERFILE: Invalid base object. Leveled Character/Creature refs no longer allowed. Create a "
                "template instead."
            );
        }
    }
    if (data.pObjectReference && data.pObjectReference->GetDelete()) {
        SetDelete(true);
    }
    if (!std::isfinite(data.kPosition.x) ||
        !std::isfinite(data.kPosition.y) ||
        !std::isfinite(data.kPosition.z) ||
        std::isnan(data.kPosition.x) ||
        std::isnan(data.kPosition.y) ||
        std::isnan(data.kPosition.z)) {
        Warning("MASTERFILE: Corrupt location found on reference, setting to (0, 0, 0).");
        data.kPosition = NiPoint3::ZERO;
    }
    if (!std::isfinite(data.kRotation.x) ||
        !std::isfinite(data.kRotation.y) ||
        !std::isfinite(data.kRotation.z) ||
        std::isnan(data.kRotation.x) ||
        std::isnan(data.kRotation.y) ||
        std::isnan(data.kRotation.z)) {
        Warning("MASTERFILE: Corrupt angle found on reference, setting to (0, 0, 0).");
        data.kRotation = NiPoint3::ZERO;
    }
    GetExtra()->InitItem(this);
    float fHealthPerc = GetExtra()->GetHealthPerc();
    if (fHealthPerc != 1.0) {
        float fHealth = static_cast<float>(TESHealthForm::GetFormHealth(GetObjectReference()));
        float fNewHealth = fHealthPerc * fHealth;
        GetExtra()->SetHealth(fNewHealth);
    }
    if (GetObjectReference()->GetFormType() == AMMO_ID && GetExtra()->GetCount() == 1) {
        uint8_t ucClipRounds = static_cast<TESAmmo*>(GetObjectReference())->GetFormClipRounds();
        GetExtra()->SetCount(ucClipRounds);
    }
    if (IsMultiBoundRef()) {
        if (GetScale() != 1.0) {
            MultiBoundMarkerData* pMultiBoundData = GetMultiBoundData();
            if (!pMultiBoundData) {
                pMultiBoundData = new MultiBoundMarkerData;
                GetExtra()->SetMultiBoundData(pMultiBoundData);
            }
            NiPoint3 kHalfExtents = pMultiBoundData->GetHalfExtents();
            kHalfExtents *= GetScale();
            pMultiBoundData->SetHalfExtents(kHalfExtents);
            SetScale(1.0f);
        }
        TESWorldSpace* pWorld = GetWorldSpace();
        if (pWorld) {
            pWorld->AddMultiBoundRef(this);
        }
    }
    Actor* pActor = nullptr;
    if (IsActor()) {
        pActor = static_cast<Actor*>(this);
    }
    if (pActor) {
        if (!pActor->HasEditorLocation()) {
            pActor->SetStartingPosition();
        }
        if (!pActor->IsPc() && TESObjectREFR::IsDead(false)) {
            pActor->SetLifeState(ACTOR_LIFE_STATE_DEAD);
            ReferenceProcessLists.RemoveReference(pActor, PROCESS_TYPE_LOW);
        }
        if (pActor->IsNpc()) {
            TESClass* pClass = pActor->GetClass();
            bool bIsGuard = false;
            if (pClass) {
                bIsGuard = pClass->IsGuard();
            }
            pActor->SetActorGuard(bIsGuard);
        }
        TESActorBase* pActorBase = pActor->GetActorBase();
        if (pActorBase->GetUsesLeveledTemplate() && !HasLeveledCreatureCalced()) {
            pActorBase->CalcTemplateForRef(this);
        }
        pActor->SetRunsInLow(pActorBase->HasLowLevelProcessing());
        if (pActor == pPlayer) {
            float fRunSpeed = GamePlayFormulas::GetActorRunSpeed(pPlayer, nullptr, nullptr, false, true, false, false);
            bhkCharacterController::SetGravFrictDampSpeed(fRunSpeed);
        }
    }
    else if (CanBeMoved()) {
        SetStartingPosition(NiPoint3::ZERO);
    }
    bool bAllowChanges = pSaveLoadGame->SetThreadAllowChanges(true);
    if (!pSaveLoadGameOLD->GetLoading() || pSaveLoadGameOLD->GetLoading()) {
        TESObjectREFR* pEnableParent = GetEnableStateParent();
        if (pEnableParent) {
            bool bLoading = pSaveLoadGameOLD->GetLoading();
            if (!pSaveLoadGameOLD->GetLoading()) {
                pSaveLoadGameOLD->SetLoading(false);
            }
            if (GetEnableStateInverse()) {
                SetDisabled(!pEnableParent->GetDisabled());
            }
            else {
                SetDisabled(pEnableParent->GetDisabled());
            }
            if (GetDisabled() && GetVisibleDistant() && GetParentCell()) {
                GetParentCell()->DecRefsWithVisibleDistantCount();
            }
            pSaveLoadGameOLD->SetLoading(bLoading);
        }
    }
    UpdateEnableStateChildrenFlagsRecursive();
    pSaveLoadGame->SetThreadAllowChanges(bAllowChanges);
    SetDestructible(BGSDestructibleObjectForm::IsDestructible(data.pObjectReference));
    data.kRotation.z = ClampAngle(data.kRotation.z);
    SetInitialized(true);
    if (GetObjectReference() &&
        GetObjectReference()->GetFormType() != WEAP_ID &&
        GetExtra()->GetExtraData(EXTRA_AMMO)) {
        GetExtra()->RemoveExtra(EXTRA_AMMO);
    }
    MessageHandler::IncDisableWarningCount(false);
    uint32_t uiNewWarningCount = MessageHandler::QWarningCount();
    MessageHandler::SetWarningCount(uiWarningCount);
    if (uiNewWarningCount) {
        if (GetParentCell() && GetParentCell()->GetInterior()) {
            TESObjectCELL* pCell = GetParentCell();
            TESBoundObject* pBase = GetObjectReference();
            uint32_t uiCellID = GetParentCell()->GetFormID();
            const char* pCellEditorID = pCell->GetFormEditorID();
            uint32_t uiRefID = GetFormID();
            const char* pRefEditorID;
            pRefEditorID = TESForm::GetFormEditorID();
            uint32_t uiBaseID = GetObjectReference()->GetFormID();
            const char* pBaseEditorID = pBase->GetFormEditorID();
            Warning(
                "MASTERFILE: Errors were encountered during InitItem for reference:\n\nBase: '%s' (%08X)\nRef: "
                "'%s' (%08X)\nCell: '%s' (%08X)\n\nSee Warnings file for more information.",
                pBaseEditorID,
                uiBaseID,
                pRefEditorID,
                uiRefID,
                pCellEditorID,
                uiCellID
            );
        }
        else if (GetParentCell()) {
            TESWorldSpace* pWorld = GetWorldSpace();
            TESObjectCELL* pCell = GetParentCell();
            TESBoundObject* pBase = GetObjectReference();
            uint32_t uiWorldID = GetWorldSpace()->GetFormID();
            const char* pWorldEditorID = pWorld->GetFormEditorID();
            int32_t iCellY = GetParentCell()->GetDataY();
            int32_t iCellX = GetParentCell()->GetDataX();
            uint32_t uiCellID = GetParentCell()->GetFormID();
            const char* pCellEditorID = pCell->GetFormEditorID();
            uint32_t uiRefID = GetFormID();
            const char* pRefEditorID;
            pRefEditorID = TESForm::GetFormEditorID();
            uint32_t uiBaseID = GetObjectReference()->GetFormID();
            const char* pBaseEditorID = pBase->GetFormEditorID();
            Warning(
                "MASTERFILE: Errors were encountered during InitItem for reference:\n\nBase: '%s' (%08X)\nRef: "
                "'%s' (%08X)\nCell: '%s' (%08X) (%i, %i) in world '%s' (%08X) \n\nSee Warnings file for more "
                "information.",
                pBaseEditorID,
                uiBaseID,
                pRefEditorID,
                uiRefID,
                pCellEditorID,
                uiCellID,
                iCellX,
                iCellY,
                pWorldEditorID,
                uiWorldID
            );
        }
        else {
            TESBoundObject* pBase = GetObjectReference();
            uint32_t uiRefID = GetFormID();
            const char* pRefEditorID;
            pRefEditorID = TESForm::GetFormEditorID();
            uint32_t uiBaseID = GetObjectReference()->GetFormID();
            const char* pBaseEditorID = pBase->GetFormEditorID();
            Warning(
                "MASTERFILE: Errors were encountered during InitItem for reference:\n\nBase: '%s' (%08X)\nRef: "
                "'%s' (%08X)\nCell: NONE\n\nSee Warnings file for more information.",
                pBaseEditorID,
                uiBaseID,
                pRefEditorID,
                uiRefID
            );
        }
    }
    if (HasContainer()) {
        InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(this);
        pChanges->InitLeveledItems();
        pChanges->InitFromContainerExtra();
        pChanges->InitScripts();
        if (pChanges->IsEmpty()) {
            GetExtra()->RemoveContainerChanges();
        }
    }
    InitScript();
    SetStillLoading(false);
    if (GetObjectReference()->GetLODTree() && GetDisabled()) {
        TESWorldSpace* pWorld = GetWorldSpace();
        if (pWorld) {
            pWorld->GetTerrainManager()->HideTree(this, true);
        }
    }
}

ENUM_FORM_ID TESObjectREFR::GetSavedFormType() {
    int32_t projFlags;
    if (!TESObjectREFR::GetObjectReference()) {
        return REFR_ID;
    }
    int32_t typeID = TESObjectREFR::GetObjectReference()->GetFormType();
    switch (typeID) {
        case NPC__ID: {
            return ACHR_ID;
        }
        case CREA_ID: {
            return ACRE_ID;
        }
        case PROJ_ID: {
            projFlags = static_cast<BGSProjectile*>(TESObjectREFR::GetObjectReference())->GetMotionType();
            if (projFlags <= 262144) {
                switch (projFlags) {
                    case 262144: {
                        return PBEA_ID;
                    }
                    case 65536: {
                        return PMIS_ID;
                    }
                    case 131072: {
                        return PGRE_ID;
                    }
                }
            }
            else {
                if (projFlags == 524288) {
                    return PFLA_ID;
                }

                if (projFlags == 1048576) {
                    return PCBE_ID;
                }
            }
            break;
        }
    }
    return REFR_ID;
}

// The world-space description is computed even though the final format only uses the base and cell descriptions.
// The reference's own editor ID uses the direct TESForm empty-string getter, not its virtual override.
void TESObjectREFR::GetFormDetailedString(BSStringT<char>& aString) {
    char cObject[264];
    char cWorld[264];
    cObject[0] = 0;
    TESForm* pObject = GetObjectReference();
    if (pObject) {
        uint32_t uiID = pObject->GetFormID();
        const char* pEditorID = pObject->GetFormEditorID();
        const char* pType = pObject->GetFormTypeString();
        std::sprintf(cObject, " to %s form '%s' (%08X)", pType, pEditorID, uiID);
    }
    cWorld[0] = 0;
    TESWorldSpace* pWorld = GetWorldSpace();
    if (pWorld) {
        uint32_t uiID = pWorld->GetFormID();
        TESForm* pWorldForm = pWorld;
        const char* pEditorID = pWorldForm->GetFormEditorID();
        std::sprintf(cWorld, " in WorldSpace '%s' (%08X)", pEditorID, uiID);
    }
    BSStringT<char> Cell;
    TESObjectCELL* pCell = GetParentCell();
    if (pCell) {
        pCell->GetFormDetailedString(Cell);
    }
    const char* pCellString = Cell.String();
    uint32_t uiID = GetFormID();
    const char* pEditorID = TESForm::GetFormEditorID();
    const char* pType = GetFormTypeString();
    aString.SPrintF("%s Form '%s' (%08X)%s in Cell %s", pType, pEditorID, uiID, cObject, pCellString);
}

// On the local map when the base object is; the base object is fetched twice.
bool TESObjectREFR::GetOnLocalMap() {
    if (GetObjectReference()) {
        TESBoundObject* pObj = GetObjectReference();
        if (pObj->GetOnLocalMap()) {
            return true;
        }
    }
    return false;
}

void TESObjectREFR::SetDelete(bool abDeleted) {
    if (abDeleted &&
        !pDataHandler->GetClearingData() &&
        pDataHandler->IsFormIDCreated(GetFormID()) &&
        !GetTemporary() &&
        !pSaveLoadGame->GetSaveGameLoading()) {
        pSaveLoadGame->RemoveChanges(this);
    }
    TESForm::SetDelete(abDeleted);
}

void TESObjectREFR::SetAltered(bool altered) {
    if (!GetTemporary() && altered) {
        TESObjectCELL* pCell = GetSaveParentCell();
        if (pCell) {
            pCell->SetAltered(true);
        }
    }
    TESForm::SetAltered(altered);
}

// Copies apCopy's reference: the persistence, the components, the placement data, the extra data and the 3D. The
// reference leaves its enable parent's, linked reference's and activate parents' child lists first and rejoins them
// after. When apCopy is temporary, the reflection, refraction and lit-water links move from this reference's lists to
// apCopy's (the water shaders' light lists follow the reference's light). A different base, scale (unless an actor),
// ammo count, radius or emittance source drops the old 3D. A door's linked door is pointed back at the reference,
// losing a lock or owner it shares with it (with a warning).
void TESObjectREFR::Copy(TESForm* apCopy) {
    TESObjectREFR* pCopy = dynamic_cast<TESObjectREFR*>(apCopy);
    if (!pCopy) {
        return;
    }
    bool bReset3D = false;
    if (!GetTemporary() && GetEnableStateParent()) {
        GetEnableStateParent()->GetExtra()->RemoveEnableStateChild(this);
    }
    SetRefPersists(pCopy->GetRefPersists());
    CopyAllComponents(apCopy);
    if (GetObjectReference()) {
        TESBoundObject* pBase = GetObjectReference();
        if (pBase != pCopy->GetObjectReference()) {
            bReset3D = true;
        }
    }
    ourMemcpy(&data, pCopy->GetData(), sizeof(ReferenceData));
    // A NaN scale compares unequal.
    float fScale = fRefScale;
    if (fScale == pCopy->fRefScale || IsActor()) {
        if (GetObjectReference() &&
            GetObjectReference()->GetFormType() == AMMO_ID &&
            GetExtra()->GetCount() != pCopy->GetExtra()->GetCount()) {
            bReset3D = true;
        }
        else {
            float fCopyRadius = pCopy->GetRadius();
            float fRadius = GetRadius();
            if (!FloatEQ(fRadius, fCopyRadius, 0.001f)) {
                bReset3D = true;
            }
            else {
                TESForm* pEmittance = GetEmittanceSource();
                if (pEmittance != pCopy->GetEmittanceSource()) {
                    bReset3D = true;
                }
            }
        }
    }
    else {
        bReset3D = true;
    }
    if (Get3D()) {
        const NiPoint3* pLocation = &GetLocationOnReference();
        Get3D()->SetTranslate(*pLocation);
        NiMatrix3 kRotate = GetOrientation();
        Get3D()->SetRotate(kRotate);
        bhkNiCollisionObject::ResetSim(Get3D(), true);
        NiUpdateData kUpdateData(0.0f, false, false);
        Get3D()->Update(kUpdateData);
    }
    if (pCopy->GetParentCell()) {
        pCopy->GetParentCell()->AddReference(this, false);
        if (!Get3D()) {
            pIOManager->LoadAllQueuedPriorities();
        }
    }
    else if (GetParentCell()) {
        GetParentCell()->RemoveReference(this);
    }
    DoorTeleportData* pTeleport = GetTeleport();
    if (pTeleport) {
        DoorTeleportData* pCopyTeleport = pCopy->GetTeleport();
        if (!pCopyTeleport || pCopyTeleport->GetLinkedDoor() != pTeleport->GetLinkedDoor()) {
            pTeleport->GetLinkedDoor()->RemoveTeleport();
            pTeleport->SetLinkedDoor(nullptr);
        }
    }
    if (GetLinkedRef()) {
        GetLinkedRef()->GetExtra()->RemoveLinkedRefChild(this);
        SetLinkLine(false);
    }
    if (GetExtra()->GetActivateParentList()) {
        BSSimpleList<REF_ACTIVATE_DATA*>* pParents = GetExtra()->GetActivateParentList();
        while (pParents && !pParents->IsEmpty()) {
            REF_ACTIVATE_DATA* pParent = pParents->GetItem();
            pParents = pParents->GetNext();
            pParent->pActivateRef->GetExtra()->RemoveActivateRefChild(this);
        }
        SetActivateLine(false);
    }
    BSSimpleList<REF_REFLECTOR_DATA*>* pReflectors = nullptr;
    if (pCopy->GetTemporary()) {
        for (pReflectors = GetExtra()->GetReflectorRefs(); pReflectors && pReflectors->GetItem();
             pReflectors = pReflectors->GetNext()) {
            pReflectors->GetItem()->pRef->GetExtra()->SetReflectsRef(this, false);
            pReflectors->GetItem()->pRef->GetExtra()->SetRefractsRef(this, false);
        }
        for (BSSimpleList<TESObjectREFR*>* pWaterRefs = GetExtra()->GetLitWaterRefs();
             pWaterRefs && pWaterRefs->GetItem();
             pWaterRefs = pWaterRefs->GetNext()) {
            LightRefData* pLightData = GetExtra()->GetLight();
            if (pLightData && pLightData->spLight) {
                TESObjectREFR* pWater = pWaterRefs->GetItem();
                if (pWater->Get3D() && TES::pInstance->GetWaterSystem()) {
                    TESObjectREFR* pWaterRef = pWaterRefs->GetItem();
                    NiAVObject* pWaterGeometry = TES::pInstance->GetWaterSystem()->GetWaterGeometry(pWaterRef);
                    NiGeometry* pGeometry = static_cast<NiGeometry*>(pWaterGeometry);
                    BSShader* pShader = pGeometry->GetShader();
                    if (pShader &&
                        pShader->GetShaderType() == static_cast<uint32_t>(BSShaderManager::BSSM_SHADERTYPE_WATER)) {
                        WaterShaderProperty* pProperty = static_cast<WaterShaderProperty*>(
                            pWaterGeometry->GetProperty(NiProperty::SHADE)
                        );
                        NiLight* pLight = pLightData->spLight;
                        pProperty->LightList.Remove(pLight);
                    }
                }
            }
            pWaterRefs->GetItem()->GetExtra()->SetWaterLightRef(this, false);
        }
    }
    // Gets the copy's extra data first, then this reference's.
    ExtraDataList* pCopyExtra = pCopy->GetExtra();
    GetExtra()->CopyList(pCopyExtra);
    if (pCopy->GetTemporary()) {
        for (pReflectors = pCopy->GetExtra()->GetReflectorRefs(); pReflectors && pReflectors->GetItem();
             pReflectors = pReflectors->GetNext()) {
            if (pReflectors->GetItem()->iEffectFlags & REF_REFLECTOR_DATA::FLAG_REFLECTION) {
                pReflectors->GetItem()->pRef->GetExtra()->SetReflectsRef(this, true);
            }
            if (pReflectors->GetItem()->iEffectFlags & REF_REFLECTOR_DATA::FLAG_REFRACTION) {
                pReflectors->GetItem()->pRef->GetExtra()->SetRefractsRef(this, true);
            }
        }
        for (BSSimpleList<TESObjectREFR*>* pWaterRefs = pCopy->GetExtra()->GetLitWaterRefs();
             pWaterRefs && pWaterRefs->GetItem();
             pWaterRefs = pWaterRefs->GetNext()) {
            LightRefData* pLightData = GetExtra()->GetLight();
            if (pLightData && pLightData->spLight) {
                TESObjectREFR* pWater = pWaterRefs->GetItem();
                if (pWater->Get3D() && TES::pInstance->GetWaterSystem()) {
                    TESObjectREFR* pWaterRef = pWaterRefs->GetItem();
                    NiAVObject* pWaterGeometry = TES::pInstance->GetWaterSystem()->GetWaterGeometry(pWaterRef);
                    NiGeometry* pGeometry = static_cast<NiGeometry*>(pWaterGeometry);
                    BSShader* pShader = pGeometry->GetShader();
                    if (pShader &&
                        pShader->GetShaderType() == static_cast<uint32_t>(BSShaderManager::BSSM_SHADERTYPE_WATER)) {
                        WaterShaderProperty* pProperty = static_cast<WaterShaderProperty*>(
                            pWaterGeometry->GetProperty(NiProperty::SHADE)
                        );
                        NiLight* pFindLight = pLightData->spLight;
                        if (!pProperty->LightList.FindPos(pFindLight, nullptr)) {
                            NiLight* pAddLight = pLightData->spLight;
                            pProperty->LightList.AddTail(pAddLight);
                            pProperty->bSpecularLighting = true;
                        }
                        NiLight* pRemoveLight = pLightData->spLight;
                        BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->RemoveLight(pRemoveLight);
                    }
                }
            }
            pWaterRefs->GetItem()->GetExtra()->SetWaterLightRef(this, true);
        }
    }
    if (GetLinkedRef()) {
        GetLinkedRef()->GetExtra()->AddLinkedRefChild(this);
        SetLinkLineRecurse(true, nullptr);
    }
    if (!GetTemporary() && GetExtra()->GetActivateParentList()) {
        BSSimpleList<REF_ACTIVATE_DATA*>* pParents = GetExtra()->GetActivateParentList();
        while (pParents && !pParents->IsEmpty()) {
            REF_ACTIVATE_DATA* pParent = pParents->GetItem();
            pParents = pParents->GetNext();
            pParent->pActivateRef->GetExtra()->AddActivateRefChild(this);
        }
        SetActivateLineRecurse(true);
    }
    SetScale(pCopy->fRefScale);
    if (bReset3D) {
        Set3D(nullptr, true);
    }
    if (pCopy->Get3D()) {
        if (GetTemporary() && data.pObjectReference && data.pObjectReference->GetFormType() == TREE_ID) {
            Set3D(data.pObjectReference->Clone3D(this, false), true);
        }
        if (!Get3D() && GetParentCell() && TES::pInstance->IsCellLoaded(GetParentCell(), false)) {
            TES::pInstance->AttachModel(this, GetParentCell(), nullptr, false);
        }
        else if (!data.pObjectReference || data.pObjectReference->GetFormType() != TREE_ID) {
            NiAVObject* pNew3D = Load3D(false);
            pNew3D->SetTranslate(GetLocationOnReference());
            NiMatrix3 kNewRotate = GetOrientation();
            pNew3D->SetRotate(kNewRotate);
            pNew3D->SetScale(GetScale());
            if (pCopy->Get3D()->GetParent()) {
                NiNode* pParent = pCopy->Get3D()->GetParent();
                pParent->AttachChild(pNew3D, true);
            }
        }
        if (Get3D() && GetObjectReference() && GetObjectReference()->GetFormType() == LIGH_ID) {
            NiNode* p3D = Get3D();
            static_cast<TESObjectLIGH*>(GetObjectReference())->GenDynamic(this, static_cast<BSFadeNode*>(p3D), false);
        }
        if (GetAction(OBJECT_ACTION_OPEN_BY_DEFAULT)) {
            BGSOpenCloseForm::SetOpenState(this, true, true);
        }
        else {
            BGSOpenCloseForm::SetOpenState(this, false, true);
        }
        bhkNiCollisionObject::ResetSim(Get3D(), true);
        NiUpdateData kNewUpdateData(0.0f, false, false);
        Get3D()->Update(kNewUpdateData);
        Get3D()->UpdateProperties();
    }
    else {
        Set3D(nullptr, true);
    }
    if (pCopy->GetTemporary()) {
        FixAttachedBSObjectSizeAndTransform();
    }
    pTeleport = GetTeleport();
    if (pTeleport && !GetTemporary()) {
        TESObjectREFR* pLinkedDoor = pTeleport->GetLinkedDoor();
        if (pLinkedDoor) {
            if (!pLinkedDoor->GetRefPersists()) {
                pLinkedDoor->SetRefPersists(true);
            }
            DoorTeleportData* pLinkedTeleport = pLinkedDoor->GetTeleport();
            if (!pLinkedTeleport) {
                pLinkedTeleport = pLinkedDoor->AddTeleport();
            }
            pLinkedTeleport->SetLinkedDoor(this);
            bool bRemovedShared = false;
            if (GetMyLock() && pLinkedDoor->GetMyLock()) {
                pLinkedDoor->GetExtra()->RemoveLockPtr();
                bRemovedShared = true;
            }
            if (GetMyOwner() && pLinkedDoor->GetMyOwner()) {
                pLinkedDoor->RemoveOwner();
                bRemovedShared = true;
            }
            if (bRemovedShared) {
                Warning("DEFAULT: Conflicting shared data removed from linked door reference.");
            }
        }
        else {
            RemoveTeleport();
            pTeleport = nullptr;
        }
    }
    if (!GetTemporary() && GetEnableStateParent()) {
        GetEnableStateParent()->GetExtra()->AddEnableStateChild(this);
    }
}

bool TESObjectREFR::Compare(TESForm* apCompare) {
    TESObjectREFR* pCompareRef = dynamic_cast<TESObjectREFR*>(apCompare);
    if (!pCompareRef) {
        return true;
    }
    if (TESForm::CompareAllComponents(pCompareRef)) {
        return true;
    }
    ReferenceData* pData = pCompareRef->GetData();
    if (std::memcmp(&data, pData, sizeof(data))) {
        return true;
    }
    TESObjectCELL* pCell = TESObjectREFR::GetParentCell();
    if (pCell != pCompareRef->TESObjectREFR::GetParentCell()) {
        return true;
    }
    if (m_Extra.CompareList(pCompareRef->TESObjectREFR::GetExtra())) {
        return true;
    }
    double dScale = fRefScale;
    return dScale != pCompareRef->fRefScale;
}

bool TESObjectREFR::BelongsInGroup(FORM* apGroupFORM, bool abAllowParentGroups, bool abCurrentOnly) {
    bool bretval = false;
    if (apGroupFORM && apGroupFORM->form == formEnumString[2].uiFormString) {
        TESObjectCELL* pparentcell = GetSaveParentCell();
        uint32_t iFormID = apGroupFORM->iFormID;
        if (iFormID == 6) {
            if (!abAllowParentGroups) {
                return false;
            }
        }
        else {
            if (iFormID <= 7 || iFormID > 9) {
                if (abAllowParentGroups) {
                    return pparentcell->BelongsInGroup(apGroupFORM, abAllowParentGroups, abCurrentOnly);
                }
                return bretval;
            }
        }
        if (pparentcell->FormIDMatchesWithoutIndex(apGroupFORM->flags)) {
            if (abCurrentOnly && apGroupFORM->iFormID != 6) {
                if (GetRefPersists()) {
                    if (apGroupFORM->iFormID == 8) {
                        return true;
                    }
                }
                else {
                    if (apGroupFORM->iFormID == 9) {
                        return true;
                    }
                }
            }
            else {
                return true;
            }
        }
    }
    return bretval;
}

// The group the reference saves into under apParentGroup: its save cell's child group (from a world child, a persistent
// cell's, or the cell's sub-block), or under that child group the persistent or temporary children group. Otherwise
// apOutGroupFORM keeps form 0. The save cell comes through the TESChildCell interface.
void TESObjectREFR::CreateGroupData(FORM* apOutGroupFORM, FORM_GROUP* apParentGroup) {
    if (!apOutGroupFORM) {
        return;
    }
    apOutGroupFORM->form = 0;
    if (!apParentGroup) {
        return;
    }
    TESObjectCELL* pCell = static_cast<TESChildCell*>(this)->GetSaveParentCell();
    uint32_t uiCellID = pCell->GetFormID();
    bool bInGroup = false;
    switch (apParentGroup->GroupData.iFormID) {
        case FGT_WORLDCHILD:
            if (pCell->GetPersistentCell()) {
                bInGroup = true;
            }
            [[fallthrough]];
        case FGT_INTCELLSUBBLOCK:
        case FGT_EXTCELLSUBBLOCK:
            if (apParentGroup->GroupData.flags == pCell->GetGroupSubBlockKey()) {
                bInGroup = true;
            }
            if (bInGroup) {
                apOutGroupFORM->form = formEnumString[GRUP_ID].uiFormString;
                apOutGroupFORM->iFormID = FGT_CELLCHILD;
                apOutGroupFORM->flags = uiCellID;
                apOutGroupFORM->length = 0;
                apOutGroupFORM->iVersionControl = 0;
            }
            break;
        case FGT_CELLCHILD:
            if (apParentGroup->GroupData.flags == uiCellID) {
                apOutGroupFORM->form = formEnumString[GRUP_ID].uiFormString;
                if (GetRefPersists()) {
                    apOutGroupFORM->iFormID = FGT_PERSISTENTCHILD;
                }
                else {
                    apOutGroupFORM->iFormID = FGT_TEMPCHILD;
                }
                apOutGroupFORM->flags = uiCellID;
                apOutGroupFORM->length = 0;
                apOutGroupFORM->iVersionControl = 0;
            }
            break;
        default: break;
    }
}

const char* TESObjectREFR::GetFormEditorID() {
    if (GetFormEditorIDLength()) {
        return TESForm::GetFormEditorID();
    }
    if (!data.pObjectReference || pDataHandler->GetClearingData()) {
        return "";
    }
    const char* pEditorID = data.pObjectReference->GetFormEditorID();
    if (!pEditorID || !*pEditorID) {
        TESCreature* pOriginalBase = static_cast<TESCreature*>(m_Extra.GetLevCreaOriginalBase());
        if (pOriginalBase) {
            return pOriginalBase->GetFormEditorID();
        }
    }
    return pEditorID;
}

TESObjectCELL* TESObjectREFR::GetSaveParentCell() {
    TESObjectCELL* pretcell = GetParentCell();
    if ((GetRefPersists() || GetTemporary()) && (!pretcell || !pretcell->GetInterior())) {
        return m_Extra.GetPersistentCell();
    }
    return pretcell;
}

bool TESObjectREFR::GetEditorLocation(
    NiPoint3& arOriginalLocation,
    NiPoint3& arOriginalAngle,
    TESForm*& arpLocationForm,
    TESForm* apLocationForm
) {
    ExtraStartingPosition* pstartingPosition = static_cast<ExtraStartingPosition*>(m_Extra.GetExtraData(15));
    if (!pstartingPosition) {
        return false;
    }
    arpLocationForm = m_Extra.GetStartingWorldOrCell();
    if (!arpLocationForm) {
        arpLocationForm = GetWorldSpace();
        if (!arpLocationForm) {
            if (pParentCell) {
                arpLocationForm = pParentCell;
            }
            else {
                if (!apLocationForm) {
                    return false;
                }
                if (apLocationForm->GetFormType() == CELL_ID) {
                    if (static_cast<TESObjectCELL*>(apLocationForm)->TESObjectCELL::GetInterior()) {
                        arpLocationForm = apLocationForm;
                    }
                    else {
                        arpLocationForm = static_cast<TESObjectCELL*>(apLocationForm)->TESObjectCELL::GetWorldSpace();
                    }
                }
                else {
                    arpLocationForm = apLocationForm;
                }
            }
        }
    }
    arOriginalLocation = pstartingPosition->startPosition;
    arOriginalAngle = pstartingPosition->startRotation;
    return true;
}

// abSayTo is never read.
BSSoundHandle TESObjectREFR::VoiceSoundFunction(
    TESTopic* apTopic,
    Actor* apTarget,
    bool abPlayerhead,
    bool abAlwaysPlay,
    bool abForceCorrectDuration,
    bool abSayTo,
    bool abForceSubtitles
) {
    BSSoundHandle aSoundHand;
    DialogueItem* pVoice = apTopic->CreateDialogueItem(this, apTarget, nullptr, nullptr, false);
    // Stores the voice file's duration and never passes it on.
    float fSoundDuration = 0.0f;
    SetSayToTopic(apTopic);
    uint32_t ivoiceEffectFlag = 0;
    if (pVoice) {
        TESTopicInfo* ptopicinfo = pVoice->pInfo;
        SetSayToTopicInfo(ptopicinfo);
        if (ptopicinfo) {
            ptopicinfo->RunResult(TIRS_BEGIN, this);
            ptopicinfo->AddTopicList();
        }
        pVoice->FirstResponse();
        DialogueResponse* pResponse = pVoice->GetCurrentResponse();
        if (pResponse) {
            if (pResponse->cVoice.pString) {
                char cVoicePath[512];
                BSstrcpy(cVoicePath, 512, pResponse->cVoice.pString);
                if (abForceCorrectDuration) {
                    LipSynchAnim* panim = nullptr;
                    BSStringT<char> filename;
                    filename.Set(cVoicePath, 512u);
                    if (LipSynchAnim::ChangeFilenameToLipSynch(filename)) {
                        panim = LipSynchAnim::LoadFromFile(filename);
                        if (panim) {
                            // The unsigned frame count is zero-extended to 64 bits before conversion.
                            fSoundDuration = static_cast<double>(static_cast<int64_t>(panim->iNumFrames)) / 30.0;
                            if (panim) {
                                delete panim;
                            }
                        }
                    }
                }
                if (!abPlayerhead && Get3D()) {
                    if (abAlwaysPlay) {
                        aSoundHand = BSAudio::QInstance().GetSoundHandleByFilename(
                            cVoicePath,
                            ivoiceEffectFlag | (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT),
                            nullptr
                        );
                    }
                    else {
                        aSoundHand = BSAudio::QInstance().GetSoundHandleByFilename(
                            cVoicePath,
                            ivoiceEffectFlag |
                                (BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_VOICE |
                                 BSGameSound::SOUND_FLAG_ONE_SHOT),
                            nullptr
                        );
                    }
                }
                else {
                    if (abAlwaysPlay) {
                        aSoundHand = BSAudio::QInstance().GetSoundHandleByFilename(
                            cVoicePath,
                            ivoiceEffectFlag | (BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_ONE_SHOT),
                            nullptr
                        );
                    }
                    else {
                        aSoundHand = BSAudio::QInstance().GetSoundHandleByFilename(
                            cVoicePath,
                            ivoiceEffectFlag |
                                (BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_VOICE |
                                 BSGameSound::SOUND_FLAG_ONE_SHOT),
                            nullptr
                        );
                    }
                }
                if (!abPlayerhead && Get3D()) {
                    const NiPoint3& rkPos = Get3D()->GetWorldTranslate();
                    float fX = rkPos.x;
                    float fY = rkPos.y;
                    float fZ = rkPos.z;
                    aSoundHand.SetPosition(fX, fY, fZ);
                    float fMax = BSAudio::GetMaxDialogDist();
                    float fMin = BSAudio::GetMinDialogDist();
                    aSoundHand.SetMinMax(fMin, fMax);
                    aSoundHand.SetObjectToFollow(Get3D());
                }
                aSoundHand.Play(false);
                // The callback's context is the reference's form ID.
                aSoundHand.SetCompletionCallback(
                    &TESObjectREFR::SayToCallBack,
                    reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID))
                );
            }
            else {
                if (ptopicinfo) {
                    TESObjectREFR::SayToCallBack(reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID)), false);
                }
            }
            if (gs_bGeneralSubtitles_Gameplay.Bool() ||
                (IsActor() && static_cast<uint8_t>(static_cast<Actor*>(this)->GetShowSubtitles()))) {
                if (fSoundDuration <= 0.0) {
                    fSoundDuration = 2.0f;
                }
                // The position is read before the by-value handle argument is built.
                NiPoint3 kSpeakerLoc = GetLocationOnReference();
                Interface::ShowSubtitle(pResponse->cText.pString, aSoundHand, kSpeakerLoc, apTarget, abForceSubtitles);
            }
        }
    }
    if (fSoundDuration <= 0.0) {
        fSoundDuration = 2.0f;
    }
    // A copy into the caller's result: returning aSoundHand itself would let C++ build it there.
    return BSSoundHandle(aSoundHand);
}

void TESObjectREFR::UpdateSoundCallBack() {
    ExtraSayToTopicInfo* pSayToExtra = TESObjectREFR::GetExtra()->GetSayToExtra();
    if (pSayToExtra && pSayToExtra->bVoiceDone) {
        if (pSayToExtra->pInfo && !pSayToExtra->pInfo->IsRunImmediately()) {
            pSayToExtra->pInfo->RunResult(TIRS_END, this);
        }

        if (pSayToExtra->pTopic) {
            ExtraDataList* pExtra = TESObjectREFR::GetExtra();
            Script::SetActionFlag(pSayToExtra->pTopic, pExtra, SCRIPT_ACTION_ONSAYTODONE);
        }
        TESObjectREFR::GetExtra()->RemoveSayToInfoExtra();
    }
}

void TESObjectREFR::DamageObject(float afDamage, bool abSelfDamage) {
    if (TESForm::GetDestructible() && (!TESForm::GetDestroyed() || abSelfDamage)) {
        TESBoundObject* pBaseForm = TESObjectREFR::GetObjectReference();
        BGSDestructibleObjectForm* pDestructionForm = BGSDestructibleObjectForm::GetDestructionForm(pBaseForm);
        pDestructionForm->DamageObject(static_cast<Actor*>(this), afDamage, abSelfDamage);
    }
}

bool TESObjectREFR::GetCastShadows() {
    bool bCastShadows = (!TESObjectREFR::GetObjectReference() ||
                         TESObjectREFR::GetObjectReference()->TESForm::GetFormType() == LIGH_ID) &&
        (TESForm::GetFormFlags() & kFormFlags_CastShadows) != 0;
    return bCastShadows;
}

void TESObjectREFR::SetCastShadows(bool abVal) {
    if (!GetObjectReference() || GetObjectReference()->GetFormType() == LIGH_ID) {
        int32_t iNewFlags;
        int32_t iflags = iFormFlags;
        if (abVal) {
            iNewFlags = iflags | kFormFlags_CastShadows;
        }
        else {
            iNewFlags = iflags & ~kFormFlags_CastShadows;
        }
        LightRefData* plight = GetLight();
        if (plight) {
            if (!abVal) {
                BSShaderManager::ClearAllShaderPassStages();
            }
            NiLight* pLight = plight->spLight.m_pObject;
            ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(0);
            pShadowSceneNode->SetLightCastShadows(pLight, abVal);
        }
        iFormFlags = iNewFlags;
    }
}

bool TESObjectREFR::GetMotionBlur() {
    if (GetObjectReference() != nullptr && GetObjectReference()->GetFormType() != MSTT_ID) {
        return false;
    }
    return (GetFormFlags() & kFormFlags_MotionBlur) != 0;
}

void TESObjectREFR::SetMotionBlur(bool abVal) {
    if (!GetObjectReference() || GetObjectReference()->GetFormType() == MSTT_ID) {
        int32_t iNewFlags;
        int32_t iflags = iFormFlags;
        if (abVal) {
            iNewFlags = iflags | TESForm::kFormFlags_CastShadows;
        }
        else {
            iNewFlags = iflags & ~TESForm::kFormFlags_CastShadows;
        }
        iFormFlags = iNewFlags;
    }
}

// The header keeps this name to distinguish the reference slot from TESForm's. Tests the whole byte the base form's
// virtual returns and returns 0 or 1; the result goes through its byte in memory so that clang does not pass the
// callee's return on as an already normalised bool.
bool TESObjectREFR::IsDangerous() {
    if (!GetObjectReference()) {
        return false;
    }
    TESForm* pObject = GetObjectReference();
    bool bDangerous = pObject->GetDangerous();
    return static_cast<uint8_t>(bDangerous) != 0;
}

// Tests the whole byte the base form's virtual returns and returns 0 or 1, as IsDangerous does.
bool TESObjectREFR::IsObstacle() {
    TESBoundObject* pBase = GetObjectReference();
    if (pBase == nullptr) {
        return false;
    }
    bool bObstacle = pBase->GetObstacle();
    return static_cast<uint8_t>(bObstacle) != 0;
}

bool TESObjectREFR::IsQuestObject() {
    if (!GetObjectReference()) {
        return false;
    }
    return GetObjectReference()->GetQuestObject();
}

void TESObjectREFR::SetActorCause(ActorCause* apActorCause) {
    TESObjectREFR::GetExtra()->SetActorCauseExtra(apActorCause);
    AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
}

ActorCause* TESObjectREFR::GetActorCause() {
    ExtraActorCause* pextra = m_Extra.GetActorCauseExtra();
    if (pextra) {
        return pextra->GetActorCause();
    }

    return nullptr;
}

NiPoint3 TESObjectREFR::GetStartingAngle() {
    NiPoint3 kAngle;
    kAngle = m_Extra.GetStartingAngle(this);
    return kAngle;
}

NiPoint3 TESObjectREFR::GetStartingLocation() {
    NiPoint3 kLocation;
    kLocation = m_Extra.GetStartingLocation(this);
    return kLocation;
}

// Records the starting location (the current one when aposition is zero) and the current angle. The AutoMemContext
// constructor's result is ignored. It reads the location and the angle before it gets the extra data list.
void TESObjectREFR::SetStartingPosition(NiPoint3 aposition) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_TES_REFERENCES,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectREFR.cpp",
        15105
    );
    if (aposition != NiPoint3::ZERO) {
        GetExtra()->SetStartingLocation(this, aposition);
    }
    else {
        NiPoint3 kLocation = GetLocationOnReference();
        GetExtra()->SetStartingLocation(this, kLocation);
    }
    NiPoint3 kAngle = GetAngleOnReference();
    GetExtra()->SetStartingAngle(this, kAngle);
    pContext->~AutoMemContext();
}

// Updates the reference's light and its spell effect light from the base light.
void TESObjectREFR::UpdateRefLight() {
    LightRefData* pLight = GetExtra()->GetLight();
    TESObjectLIGH* pLightBase = nullptr;
    if (GetObjectReference() && GetObjectReference()->GetFormType() == LIGH_ID) {
        pLightBase = static_cast<TESObjectLIGH*>(GetObjectReference());
    }
    if (pLight && pLightBase) {
        pLightBase->Update(pLight, nullptr);
    }
    pLight = GetExtra()->GetSpellEffectLight();
    if (pLight && pLightBase) {
        pLightBase->Update(pLight, nullptr);
    }
}

TESObjectREFR* TESObjectREFR::RemoveItem(
    TESBoundObject* apObject,
    ExtraDataList* apExtra,
    int32_t aiNumber,
    bool abStealing,
    bool abDropWorld,
    TESObjectREFR* apContainerObj,
    const NiPoint3* apPoint,
    const NiPoint3* apRotate,
    bool abDelete,
    bool abPreferStolen
) {
    TESObjectREFR* pdroppedref = nullptr;
    if (!apObject) {
        return nullptr;
    }
    if (HasContainer()) {
        bool bcheck = false;
        if (!IsActor() && GetParentCell()) {
            bcheck = HasScript();
        }

        Script::SetActionFlag(this, apExtra, SCRIPT_ACTION_ONDROP);
        InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(this);
        pdroppedref = pinventory->RemoveItem(
            this,
            apObject,
            abStealing,
            aiNumber,
            apExtra,
            abDropWorld,
            apContainerObj,
            apPoint,
            apRotate,
            abDelete,
            abPreferStolen,
            nullptr
        );
        if (IsActor()) {
            Actor* pActor = static_cast<Actor*>(this);
            // The player losing its last Rock-It Launcher.
            if (pActor &&
                pActor->IsPc() &&
                apObject->GetFormType() == WEAP_ID &&
                static_cast<TESObjectWEAP*>(apObject)->IsRockItLauncher() &&
                !GetInventoryObjectCount(apObject)) {
                pPlayer->ReturnRockItAmmo();
            }
            if (pActor &&
                pActor->GetCurrentAIProcess() &&
                !pActor->IsDead(false) &&
                this != pPlayer &&
                !Interface::ContainerIsBarter() &&
                !Interface::ContainerIsRecipe()) {
                pActor->GetCurrentAIProcess()->RemoveQueuedItem(apObject, apExtra);
                if (apObject->GetFormType() == WEAP_ID && !pActor->IsInCombat()) {
                    ItemChange* pitem = pActor->GetBestWeapon(COMBAT_WEAPON_TYPE_NONE);
                    if (pitem) {
                        if (Interface::IsInMenuMode() && pActor->IsPlayerTeammate()) {
                            pActor->Actor::EquipObject(pitem->pContainerObj, 1, nullptr, true, false, true);
                        }
                        else {
                            pActor->Actor::QueueEquipObject(pitem->pContainerObj, 1, nullptr, true, false, true);
                        }
                    }
                }
            }
        }
        if (bcheck && !HasScript()) {
            GetParentCell()->RemoveScriptedRef(this);
        }
    }
    return pdroppedref;
}

void TESObjectREFR::RemoveItemType(ENUM_FORM_ID aeType, bool abStealing, int32_t aiNumber) {
    InventoryChanges* inventoryChanges;
    if (HasContainer()) {
        inventoryChanges = InventoryChanges::GetInventoryChanges(this);
        inventoryChanges->RemoveItemType(aeType, abStealing, aiNumber);
    }
}

bool TESObjectREFR::AddWornItem(TESBoundObject* apObject, int32_t aiNumber, ExtraDataList* apExtra, bool abForceEquip) {
    InventoryChanges* inventoryChanges;
    if (HasContainer()) {
        inventoryChanges = InventoryChanges::GetInventoryChanges(this);
        if (static_cast<const void*>(this) == static_cast<const void*>(pPlayer)) {
            TESChallengeManager::UpdateChallenge(CHALLENGE_TYPE_AQUIRE_LIST, aiNumber, apObject, nullptr, 0, 0, 0);
            TESChallengeManager::UpdateChallenge(CHALLENGE_TYPE_AQUIRE, aiNumber, apObject, nullptr, 0, 0, 0);
        }
        Script::SetActionFlag(this, apExtra, SCRIPT_ACTION_ONADD);
        inventoryChanges->AddWorn(apObject, aiNumber, static_cast<Actor*>(this), apExtra, false, false);
        return true;
    }
    return false;
}

bool TESObjectREFR::RemoveWornItem(TESBoundObject* apObject, int32_t aiNumber, ExtraDataList* apExtra) {
    InventoryChanges* pInventoryChanges;
    TESContainer* pContainer = HasContainer();
    uint8_t ucRemoved = 1;
    if (pContainer) {
        pInventoryChanges = InventoryChanges::GetInventoryChanges(this);
        Script::SetActionFlag(this, apExtra, SCRIPT_ACTION_ONUNEQUIP);
        pInventoryChanges->RemoveWorn(
            reinterpret_cast<bool*>(&ucRemoved),
            apObject,
            aiNumber,
            static_cast<Actor*>(this),
            apExtra,
            false,
            false,
            nullptr,
            nullptr
        );
    }
    return ucRemoved;
}

void TESObjectREFR::AddObjecttoContainer(TESBoundObject* apObj, ExtraDataList* apExtra, int32_t aicount) {
    if (apExtra) {
        if (apExtra->GetOwner()) {
            TESForm* pOwner = apExtra->GetOwner();
            if (pOwner == GetOwner()) {
                apExtra->RemoveOwnership();
            }
        }
        TESObjectREFR* pref = apExtra->GetReferencePointer();
        if (pref) {
            uint32_t uiFormID = pref->iFormID;
            BSSimpleList<TESObjectREFR*>* pActorList = ReferenceProcessLists.GetActorTargetinganObject(
                uiFormID,
                static_cast<Actor*>(this)
            );
            BSSimpleList<TESObjectREFR*>* pTemplist = pActorList;
            while (pActorList && pActorList->m_item) {
                Actor* pActor = static_cast<Actor*>(pActorList->m_item);
                if (pActor && pActor->GetCurrentPackageTarget()) {
                    pActor->SetCurrentTarget(this);
                }
                else {
                    pActor->SetCurrentTarget(nullptr);
                }
                pActorList = pActorList->m_pkNext;
            }
            if (pTemplist) {
                pTemplist->RemoveAllPointerList();
                delete pTemplist;
            }
        }
    }
    AddBoundObjectItem(apObj, apExtra, aicount);
}

// The reference's own location plus the centre of its bound box: the sum of the bounds, halved, added to the location.
// All three are virtual calls, read in this order: the max bound first, then the min bound (the min is the left operand
// of the sum), and the location last.
NiPoint3 TESObjectREFR::GetLookingAtLocation() {
    NiPoint3 kMax = GetBoundMax();
    NiPoint3 kMin = GetBoundMin();
    NiPoint3 kCenter = (kMin + kMax) / 2.0f;
    return GetLocationOnReference() + kCenter;
}

MagicCaster* TESObjectREFR::GetMagicCaster() {
    BSExtraData* pExtraData = TESObjectREFR::GetExtra()->GetExtraData(EXTRA_MAGICCASTER);
    NonActorMagicCaster* pCaster = dynamic_cast<NonActorMagicCaster*>(pExtraData);
    if (!pCaster) {
        pCaster = new NonActorMagicCaster(this);
        TESObjectREFR::GetExtra()->AddExtra(pCaster);
        AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
    }
    if (pCaster) {
        return static_cast<MagicCaster*>(pCaster);
    }

    return nullptr;
}

MagicTarget* TESObjectREFR::GetMagicTarget() {
    NonActorMagicTarget* pTarget = nullptr;
    if (TESMagicTargetForm::CanBeMagicTarget(GetObjectReference())) {
        BSExtraData* pExtraData = m_Extra.GetExtraData(51);
        pTarget = dynamic_cast<NonActorMagicTarget*>(pExtraData);
        if (!pTarget) {
            pTarget = new NonActorMagicTarget(this);
            m_Extra.AddExtra(pTarget);
            AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
        }
    }
    if (pTarget) {
        return static_cast<MagicTarget*>(pTarget);
    }

    return nullptr;
}

BSFaceGenNiNode* TESObjectREFR::GetFaceNodeBiped(NiNode* apHeadNode) {
    if (!IsActor()) {
        return nullptr;
    }
    if (!static_cast<Actor*>(this)->GetActorType()) {
        return nullptr;
    }
    NiNode* p3D = TESObjectREFR::Get3D();
    return static_cast<BSFaceGenNiNode*>(BSUtilities::GetObjectByName(p3D, "BSFaceGenNiNodeBiped"));
}

// Actor::GetActorType is called on this only once IsActor holds, so the static_cast to Actor is safe.
BSFaceGenNiNode* TESObjectREFR::GetFaceNodeSkinned(NiNode* apHeadNode) {
    if (!IsActor()) {
        return nullptr;
    }
    if (!static_cast<Actor*>(this)->GetActorType()) {
        return nullptr;
    }
    return reinterpret_cast<BSFaceGenNiNode*>(
        BSUtilities::GetObjectByName(TESObjectREFR::Get3D(), "BSFaceGenNiNodeSkinned")
    );
}

BSFaceGenNiNode* TESObjectREFR::GetFaceNode(NiNode* apHeadNode) {
    return GetFaceNodeSkinned(apHeadNode);
}

BSFaceGenAnimationData* TESObjectREFR::GetFaceAnimationData(NiNode* apHeadNode) {
    BSFaceGenNiNode* pFaceNode = GetFaceNode(apHeadNode);
    if (pFaceNode) {
        return pFaceNode->GetAnimationData();
    }
    return nullptr;
}

bool TESObjectREFR::ClampToGround() {
    float fHeight;
    TESObjectCELL* pCell = TESObjectREFR::GetParentCell();
    if (!pCell) {
        return false;
    }
    NiPoint3 kLocation = GetLocationOnReference();
    if (!pCell->GetLandHeight(kLocation, fHeight)) {
        return false;
    }
    kLocation.z = fHeight;
    SetLocationOnReference(kLocation);
    return true;
}

// Removes the 3D's Havok bodies and the phantom with the Havok update locked; an acoustic space also drops its queued
// removal from the cell's listener.
bool TESObjectREFR::DetachHavok() {
    bool bDetached = false;
    TES::pInstance->LockHavokUpdateMT(true);
    NiNode* p3D = Get3D();
    if (p3D) {
        CleanUpTraps();
        bhkWorld::RemoveObjects(p3D, true, false);
        bDetached = true;
    }
    if (pLoadedData) {
        pLoadedData->spPhantom = nullptr;
    }
    if (GetObjectReference() && GetObjectReference()->GetFormType() == ASPC_ID && GetParentCell()) {
        static_cast<BGSAcousticSpaceListener*>(GetParentCell()->GetbhkWorld()->QAcousticSpaceListener())
            ->DequeueRemoval(this, true);
    }
    SetLoadedDataFlag(LOADED_DATA_HAVOK_INITED, false);
    TES::pInstance->LockHavokUpdateMT(false);
    return bDetached;
}

// Builds the reference's Havok bodies once its cell is attached: the special bodies of placeable water, collision
// markers, acoustic spaces and primitive triggers, then adds the collision objects to the cell's world. Ash piles that
// are loading or at rest (or not dropped) are fixed in place; a dropped dynamic body gets a downward push.
void TESObjectREFR::InitHavok() {
    if (!pParentCell || pParentCell->GetAttached() || pParentCell->IsAttaching()) {
        NiAVObject* proot = Get3D();
        NiAVObject* pav = proot;
        if (proot) {
            TES::pInstance->LockHavokUpdateMT(true);
            if (!GetLoadedDataFlag(LOADED_DATA_HAVOK_INITED)) {
                if (GetObjectReference()->GetFormType() == PWAT_ID) {
                    BGSPlaceableWater* pwater = static_cast<BGSPlaceableWater*>(GetObjectReference());
                    if (!pwater->IsAutomaticWater()) {
                        InitHavokForPlaceableWater();
                    }
                }
                else if (GetObjectReference() == pCollisionMarker) {
                    InitHavokForCollisionObject();
                }
                else if (GetObjectReference()->GetFormType() == ASPC_ID) {
                    InitHavokForAcousticSpace();
                }
                else if (GetExtra()->GetPrimitive() && GetObjectReference()->GetFormType() == ACTI_ID) {
                    InitHavokForPrimitiveTrigger();
                }
            }
            NiNode* pn = proot->IsNode();
            bhkNiCollisionObject* pcol = nullptr;
            if (!IsActor()) {
                pcol = bhkUtilFunctions::FindCollisionObjectBFS(pn, 1);
                pav = pcol ? pcol->GetSceneGraphObject() : pav;
            }
            if (pav) {
                TESObjectCELL* pcell = GetParentCell();
                if (pcell) {
                    bhkWorld* pw = pcell->GetbhkWorld();
                    bhkCollisionObject* pcol = FindFirstCollisionObject(pav);
                    bhkWorldObject* pwo = pcol ? pcol->GetWorldObject() : nullptr;
                    if (pw && (!pwo || pwo->GethkWorld() != pw->GethkWorld())) {
                        pw->AddObjectsAndMoveController(pav, proot);
                        if (GetObjectReference() == pAshPile1 || GetObjectReference() == pAshPile2) {
                            bhkRigidBody* prb = static_cast<bhkRigidBody*>(pcol ? pcol->GetWorldObject() : nullptr);
                            hkVector4 vel = prb->GetLinearVelocity();
                            if (((pSaveLoadGame->GetSaveGameLoading() || IsStillLoading()) &&
                                 prb &&
                                 vel.equals3(hkVector4::getZero(), 0.001f)) ||
                                !GetDroppedRef()) {
                                bhkWorld::SetMotion(proot, hkpMotion::MOTION_FIXED, true, false, true);
                            }
                        }
                        // The form type is kept as a 32-bit int and compared signed.
                        int32_t eType = GetObjectReference()->GetFormType();
                        if (eType < STAT_ID || eType > SCOL_ID) {
                            if (GetDroppedRef()) {
                                bhkRigidBody* prb = static_cast<bhkRigidBody*>(pcol ? pcol->GetWorldObject() : nullptr);
                                hkpRigidBody* prbHK = prb ? prb->Gethk() : nullptr;
                                if (prbHK &&
                                    prbHK->getMotion()->getType() != hkpMotion::MOTION_FIXED &&
                                    prbHK->getMotion()->getType() != hkpMotion::MOTION_KEYFRAMED) {
                                    NiPoint3 velocity = NiPoint3::UNIT_Z * -1.0f;
                                    TESHavokUtilities::AddVelocity(proot, velocity, false);
                                }
                                SetDroppedRef(false);
                            }
                        }
                        else {
                            bhkWorld::SetMotion(proot, hkpMotion::MOTION_FIXED, true, false, true);
                        }
                        CheckAndAddHavokChangeFlag(proot);
                    }
                }
            }
            else if (NiObject::IsKindOf(&BSTreeNode::ms_RTTI, proot)) {
                TESObjectCELL* pcell = GetParentCell();
                if (pcell) {
                    bhkWorld* pw = pcell->GetbhkWorld();
                    if (pw) {
                        pw->AddObjects(proot, false, false, 0, false);
                    }
                }
            }
            TES::pInstance->LockHavokUpdateMT(false);
        }
        SetLoadedDataFlag(LOADED_DATA_HAVOK_INITED, true);
    }
}

// The reference's 3D: the node of its editor primitive or the clone of its base object's model, then everything a
// loaded 3D needs (the addon nodes, the destructible stage, the animation, the water lights, the multibound), and the
// node itself.
NiAVObject* TESObjectREFR::Load3D(bool abBackgroundLoading) {
    bool bDisabled = GetDisabled();
    bool bLoadingSave = pSaveLoadGame->GetSaveGameLoading();
    if (GetDelete() || bDisabled) {
        return nullptr;
    }
    if (GetObjectReference()) {
        TESBoundObject* pObjectReference = GetObjectReference();
        if (pObjectReference->GetFormType() == PWAT_ID) {
            TES::pInstance->IncrementPlaceableWaterCount();
            if (GetParentCell()) {
                bhkWorld* pWorld = GetParentCell()->GetbhkWorld();
                if (pWorld->GetWaterListener()) {
                    // The water listener is a bhkEntityListener.
                    static_cast<bhkEntityListener*>(GetParentCell()->GetbhkWorld()->GetWaterListener())
                        ->SetEnabled(true);
                }
            }
        }
    }
    CreateLoadedData();
    alignas(NiPointer<NiAVObject>) uint8_t aNodeStorage[sizeof(NiPointer<NiAVObject>)];
    NiPointer<NiAVObject>& spNode = *::new (aNodeStorage) NiPointer<NiAVObject>(TESObjectREFR::Get3D());
    bool bNoNode = spNode == nullptr;
    if (!spNode || GetExtra()->GetPrimitive()) {
        TESBoundObject* pObject = data.pObjectReference;
        if (data.pObjectReference) {
            BGSPrimitive* pPrimitive = GetExtra()->GetPrimitive();
            if (pPrimitive) {
                pPrimitive->Build();
                spNode = pPrimitive->GetNode();
                if (!bPrimitivesOn) {
                    NiColorA kColor = pPrimitive->GetColor();
                    kColor.a = 0.0f;
                    pPrimitive->SetColor(kColor);
                }
                if (GetObjectReference() != pCollisionMarker && spNode) {
                    spNode->SetAppCulled(true);
                }

                SetLoadedDataFlag(LOADED_DATA_PRIMITIVE, true);
            }
            else {
                spNode = data.pObjectReference->Clone3D(this);
            }
            if (abBackgroundLoading) {
                SetBackgroundLoadingRef(this, NiSmartPointerCast<NiNode>(spNode));
                if (spNode) {
                    spNode->IncRefCount();
                }
            }
            else {
                Set3DSimple(spNode);
            }
            bool bHaveFlames = false;
            if (spNode) {
                NiNode* pAddonRoot = spNode->IsNode();
                bHaveFlames = HasAddonFlags(pAddonRoot);
                SetLoadedDataFlag(LOADED_DATA_HAS_ADDON_FLAGS, bHaveFlames);
            }
            if (GetDisableFade()) {
                NiSmartPointerCast<BSFadeNode>(spNode)->TurnFadeNodeOn();
                SetDisableFade(false);
            }
            if (GetDestructible() && spNode) {
                if (GetExtra()->GetObjectHealth() != 0.0) {
                    TESBoundObject* pPreloadTarget = GetObjectReference();
                    BGSDestructibleObjectForm::GetDestructionForm(GetObjectReference())
                        ->PreloadReplacementModels(pPreloadTarget);
                }
                if (GetEmpty() && BGSDestructibleObjectForm::HideSmallDebris(NiSmartPointerCast<NiNode>(spNode))) {
                    GetExtra()->RemoveSavedHavokData();
                }

                if (BGSDestructibleObjectForm::GetDestructionForm(GetObjectReference())
                        ->UpdateCurrentDamageStage(this, false)) {
                    UpdateAddonNodeSounds(false);
                }
            }

            TESModelTextureSwap* pModelSwap = dynamic_cast<TESModelTextureSwap*>(GetTESModel());
            if (pModelSwap && spNode) {
                NiNode* pSwapRoot = spNode->IsNode();
                pModelSwap->SwapTextures(pSwapRoot);
            }

            if (GetObjectReference() && GetObjectReference()->GetHasPLSpecTex()) {
                SwapPlatformLanguageTextures(spNode);
            }
            if (spNode) {
                Creature* pCreature = nullptr;
                if (IsActor() && data.pObjectReference->GetFormType() == CREA_ID) {
                    pCreature = static_cast<Creature*>(this);
                    pCreature->SetRagDoll(pCreature->FindRagDoll(spNode));
                }
                if (bHaveFlames) {
                    if (GetFireOff()) {
                        RemoveAddonNodes(NiSmartPointerCast<NiNode>(spNode));
                    }
                    else {
                        AddAddonNodes(NiSmartPointerCast<NiNode>(spNode));
                    }
                }
                if (HasRandomAnim()) {
                    float fPhase = static_cast<float>(static_cast<double>(Random() % 1000) / 100.0);
                    RandomizeAddons(spNode->IsNode(), fPhase);
                }
                if (pObject->GetFormType() == WEAP_ID) {
                    // The weapon's 3D is passed unchecked; RemoveScabard walks it as a node.
                    RemoveScabard(NiSmartPointerCast<NiNode>(spNode));
                }
                bool bMotionBlur = GetMotionBlur();
                BSShaderProperty::SetMotionBlurRecurse(spNode, bMotionBlur);
                spNode->SetTranslate(data.kPosition);
                NiMatrix3 kRotate = GetOrientation();
                spNode->SetRotate(kRotate);
                if (GetCenterOnCreation()) {
                    bhkWorld::Enable(spNode, false, true, true);
                    NiUpdateData kUpdateData(0.0f, false, false);
                    spNode->Update(kUpdateData);
                    NiUpdateData kUpdateData2(0.0f, false, false);
                    spNode->Update(kUpdateData2);
                    bhkWorld::Enable(spNode, true, true, true);
                    NiPoint3 kOffset = data.kPosition - spNode->GetWorldBound().GetCenter();
                    data.kPosition += kOffset;
                    spNode->SetTranslate(data.kPosition);
                }
                bhkNiCollisionObject::ResetSim(NiSmartPointerCast<NiNode>(spNode), true);
                NiNode* pNode = spNode->IsNode();
                if (pNode && pNode->IsFadeNode())
                // Tests IsFadeNode's result on pNode but stores through spNode's object.
                {
                    NiSmartPointerCast<BSFadeNode>(spNode)->SetUserData(this);
                }

                BSShaderManager::SetObjectType(spNode);
                if (data.pObjectReference == pDoorMarker || data.pObjectReference == pTravelMarker) {
                    spNode->SetAppCulled(true);
                }
                // The NPC base object is fetched here and never used; the three getter calls are kept deliberately.
                if (GetObjectReference() && GetObjectReference()->GetFormType() == NPC__ID) {
                    GetObjectReference();
                }
                if (GetTeleport() && GetTeleport()->GetTeleportCell()) {
                    GetTeleport()->GetTeleportCell()->SetLowestProcessToMiddleLow(true);
                }

                float fScale = GetScale();
                spNode->SetScale(fScale);
                InitAnimation();
                if (IsActor() && data.pObjectReference->GetFormType() == NPC__ID) {
                    BSFaceGenAnimationData* pFaceData = GetFaceAnimationData(nullptr);
                    if (pFaceData) {
                        pFaceData->SetDead(IsDead(false), true);
                    }
                    static_cast<TESNPC*>(data.pObjectReference)->FixNPCNormals(this);
                }
                spNode->UpdateProperties();
                if (IsActor()) {
                    Actor* pActor = static_cast<Actor*>(this);
                    HighProcess* pHighProcess = nullptr;
                    pActor->InitLightingPropertyPtr(spNode);
                    if (pActor->GetCurrentAIProcess() &&
                        pActor->GetCurrentAIProcess()->GetProcessLevel() == PROCESS_TYPE_HIGH) {
                        pHighProcess = static_cast<HighProcess*>(pActor->GetCurrentAIProcess());
                    }

                    if (pHighProcess) {
                        pHighProcess->InitBoneLOD(pActor);
                        pHighProcess->ClearFaceAnimationData();
                    }
                    if (pCreature) {
                        pCreature->CheckMissingLimbs();
                    }
                    if (!TES::pInstance->CellTestRunning() &&
                        IsDead(true) &&
                        GetAnimation() &&
                        (GetAnimation()->GroupLoaded(ANIM_GROUP_DEATH) && !bLoadingSave)) {
                        Animation* pAnimation = GetAnimation();
                        pAnimation->ForceSection(AGS_WHOLE_BODY, ANIM_GROUP_DEATH, AGA_NONE, 0.0f, -1);
                        BSAnimGroupSequence* pSequence = pAnimation->GetCurrentSequence(AGS_MOVEMENT);
                        if (pSequence) {
                            if (pActor->HasRagDoll()) {
                                if (!IsDead(false)) {
                                    TESAnimGroup* pGroup = pSequence->GetAnimGroup();
                                    float fTime = pGroup->GetTime(AGA_STOP);
                                    pAnimation->Update(pActor, 0.0f, fTime);
                                    pAnimation->UpdateMovement(this);
                                    NiMatrix3 kRotation;
                                    const NiPoint3& kAngle = GetAngleOnReference();
                                    kRotation.MakeZRotation(kAngle.z);
                                    NiPoint3 kDirection(0.0f, 1.0f, 0.0f);
                                    kDirection = kRotation * kDirection;
                                    if (pActor->pRagdollController) {
                                        pActor->pRagdollController->DisableRagdollAnim(true);
                                    }
                                    bhkBlendController::DoKnockDown(spNode, kDirection, true, 0.0f, true);
                                    AddChange(CHANGE_REFR_HAVOK_MOVE);
                                }
                            }
                            else {
                                float fEndTime = pSequence->GetEndKeyTime();
                                pSequence->SetOffset(fEndTime);
                                float fUpdateTime = pSequence->GetEndKeyTime();
                                pAnimation->Update(pActor, 0.0f, fUpdateTime);
                                pAnimation->UpdateMovement(this);
                            }
                        }
                    }
                }
                if (TESHavokUtilities::Should3DRagDoll(this, spNode)) {
                    RestoreRagDollData(nullptr);
                }
                GetExtra()->RestoreSavedAnimation(this);
                ApplyAllDecals(false);
                PreCalcTriStripsShapeAabb(spNode);
            }
        }
    }
    if (IsObstacle()) {
        NavMeshObstacleManager::GetInstance()->AddObstacleForReference(this);
    }
    if (GetOnLocalMap()) {
        BSShaderProperty::SetFlagRecurse(spNode, BSShaderProperty::BSSP_FLAG_SHOW_IN_LOCAL_MAP, true);
    }
    if (GetIsStingerLOD()) {
        BSShaderProperty::SetFlagRecurse(spNode, BSShaderProperty::BSSP_FLAG_STINGER, true);
    }
    if (GetIsImposter()) {
        BSShaderProperty::SetFlagRecurse(spNode, BSShaderProperty::BSSP_FLAG_LODBUILDING, true);
        if (TESRegionDataImposter::IsImposterInActiveRegionList(this)) {
            TESRegionDataImposter::SetImposterVisible(this, true, false);
        }
        else {
            TESRegionDataImposter::SetImposterVisible(this, false, true);
        }
    }
    if (GetObjectReference() && GetObjectReference()->GetFormType() == LIGH_ID) {
        BSSoundHandle kSoundHandle;
        GetExtra()->GetSound(kSoundHandle);
        if (kSoundHandle.IsValid()) {
            kSoundHandle.SetObjectToFollow(spNode);
        }
        BSSimpleList<TESObjectREFR*>* pLitWaterRefs = GetExtra()->GetLitWaterRefs();
        LightRefData* pLightData = GetLight();
        if (pLightData && pLightData->spLight && !GetTemporary()) {
            for (; pLitWaterRefs && pLitWaterRefs->GetItem(); pLitWaterRefs = pLitWaterRefs->GetNext()) {
                TESObjectREFR* pWaterRef = pLitWaterRefs->GetItem();
                if (pWaterRef->Get3D()) {
                    NiAVObject* pWaterGeometry = TES::pInstance->GetWaterSystem()->GetWaterGeometry(pWaterRef);
                    BSShader* pShader = static_cast<NiGeometry*>(pWaterGeometry)->GetShader();
                    if (pShader &&
                        pShader->GetShaderType() == static_cast<uint32_t>(BSShaderManager::BSSM_SHADERTYPE_WATER)) {
                        WaterShaderProperty* pProperty = static_cast<WaterShaderProperty*>(
                            pWaterGeometry->GetProperty(NiProperty::SHADE)
                        );
                        NiLight* pFindLight = pLightData->spLight;
                        if (!pProperty->LightList.FindPos(pFindLight, nullptr)) {
                            NiLight* pAddLight = pLightData->spLight;
                            pProperty->LightList.AddTail(pAddLight);
                            pProperty->bSpecularLighting = true;
                        }
                        NiLight* pRemoveLight = pLightData->spLight;
                        BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->RemoveLight(pRemoveLight);
                    }
                }
            }
        }
    }

    if (GetObjectReference() && GetObjectReference()->GetFormType() == PWAT_ID) {
        BSSimpleList<TESObjectREFR*>* pWaterLightRefs = GetExtra()->GetWaterLightRefs();
        for (; pWaterLightRefs && pWaterLightRefs->GetItem(); pWaterLightRefs = pWaterLightRefs->GetNext()) {
            LightRefData* pLightData = static_cast<TESObjectREFR*>(pWaterLightRefs->GetItem())->GetLight();
            if (pLightData && pLightData->spLight && (!GetTemporary() && Get3D())) {
                NiAVObject* pWaterGeometry = TES::pInstance->GetWaterSystem()->GetWaterGeometry(this);
                BSShader* pShader = static_cast<NiGeometry*>(pWaterGeometry)->GetShader();
                if (pShader &&
                    pShader->GetShaderType() == static_cast<uint32_t>(BSShaderManager::BSSM_SHADERTYPE_WATER)) {
                    WaterShaderProperty* pProperty = static_cast<WaterShaderProperty*>(
                        pWaterGeometry->GetProperty(NiProperty::SHADE)
                    );
                    NiLight* pFindLight = pLightData->spLight;
                    if (!pProperty->LightList.FindPos(pFindLight, nullptr)) {
                        NiLight* pAddLight = pLightData->spLight;
                        pProperty->LightList.AddTail(pAddLight);
                        pProperty->bSpecularLighting = true;
                    }
                    NiLight* pRemoveLight = pLightData->spLight;
                    BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->RemoveLight(pRemoveLight);
                }
            }
        }
    }

    if (bNoNode && !GetExtra()->GetRoomIsMaster()) {
        TESObjectREFR* pMasterRoom = GetExtra()->GetMasterRoom();
        if (pMasterRoom) {
            alignas(NiPointer<BSMultiBound>) uint8_t aMultiBoundStorage[sizeof(NiPointer<BSMultiBound>)];
            NiPointer<BSMultiBound>& spMultiBound = *::new (aMultiBoundStorage)
                                                        NiPointer<BSMultiBound>(GetMultiBound());
            bool bNotJoined = pMasterRoom->GetMultiBoundRoom()->QJoinedMultiBoundList().FindPos(
                                  spMultiBound,
                                  nullptr
                              ) == nullptr;
            spMultiBound.~NiPointer<BSMultiBound>();
            if (bNotJoined) {
                pMasterRoom->GetMultiBoundRoom()->AddJoinedMultibound(GetMultiBound());
            }
        }
    }

    if (abBackgroundLoading) {
        ClearBackgroundLoadingRef();
    }
    if (bNoNode &&
        GetObjectReference() &&
        GetObjectReference()->GetFormType() == PWAT_ID &&
        !static_cast<BGSPlaceableWater*>(GetObjectReference())->IsAutomaticWater()) {
        bool bUpdated = UpdateMultiboundShapeTransformsRecursive(spNode);
        spNode->UpdateWorldBound();
        if (!bUpdated) {
            uint32_t uiFormID = GetFormID();
            const char* pEditorID = GetFormEditorID();
            char aDestination[256];
            BSsprintf(
                aDestination,
                256,
                "WATER: Trying to load water object '%s' (%08X) without multibound data. Please "
                "have an artist re-export this object ASAP!",
                pEditorID,
                uiFormID
            );
            Warning("%s", aDestination);
        }
    }

    BSExtraData* pSecuritronFace = GetExtraData(EXTRA_SECURITRON_FACE);
    if (pSecuritronFace) {
        static_cast<ExtraSecuritronFace*>(pSecuritronFace)->ApplyFace(spNode);
    }
    // The loaded object is passed as is, without a type check; UpdateLODMultInvisibility takes an NiNode*.
    UpdateLODMultInvisibility(this, NiSmartPointerCast<NiNode>(spNode));
    NiAVObject* p3D = spNode;
    spNode.~NiPointer<NiAVObject>();
    return p3D;
}

// Replaces the reference's 3D with apObject (null unloads it). The old 3D is detached from its parent, keeping its
// local transform for the new one, and leaves the shadow scene; when unloading, the sounds, markers, saved Havok and
// animation state, light, weapon, addon nodes, character controller and ragdoll that hang on it are released and the 3D
// goes to a garbage list. The new 3D then takes the old one's place, and the teleport cell and navmesh obstacles
// follow. The old 3D is held in a NiPointer.
void TESObjectREFR::Set3D(NiAVObject* apObject, bool abTestProcessLevel) {
    if (TESObjectREFR::Get3D() == apObject) {
        if (!apObject) {
            pModelLoader->CancelReference(this);
            AttachSound(false);
        }
        return;
    }
    if (TESObjectREFR::Get3D() && !TESObjectREFR::Get3D()->GetParent()) {
        pModelLoader->CancelReference(this);
    }
    if (pLoadedData && pLoadedData->m_spData3D && !apObject && IsActor()) {
        Actor* pActor = static_cast<Actor*>(this);
        if (pActor->GetCurrentAIProcess() && pActor->GetCurrentProcessType() <= PROCESS_TYPE_MIDDLE_HIGH) {
            if (abTestProcessLevel) {
                NiNode* pNode = pLoadedData->m_spData3D;
                const char* pName = pNode->GetName().GetHandleString();
                Warning("AI: Unloading art for a high or middle high process actor (%s).", pName);
            }
            BaseProcess* pProcess = pActor->GetCurrentAIProcess();
            pProcess->SetFaceSkinnedNode(nullptr);
            pProcess->SetFaceNode(nullptr);
            pProcess->SetHeadAnims(nullptr);
            static_cast<MiddleHighProcess*>(pProcess)->ClearDamageNodes();
        }
    }
    if (pLoadedData) {
        pLoadedData->iUnderwaterCount = 0;
    }
    BGSPrimitive* pPrimitive = GetExtra()->GetPrimitive();
    if (!apObject && pPrimitive) {
        pPrimitive->ClearObjects();
    }
    if (!apObject) {
        Release3DRelatedData();
    }
    // the old 3D's local transform, defaulting to the reference's position and scale
    NiPoint3 kTranslate = GetData()->kPosition;
    NiMatrix3 kRotate;
    float fScale = GetScale();
    NiNode* pParent = nullptr;
    Actor* pActor = nullptr;
    if (IsActor()) {
        pActor = static_cast<Actor*>(this);
    }
    if (TESObjectREFR::Get3D() &&
        apObject &&
        data.pObjectReference &&
        data.pObjectReference->GetFormType() == TREE_ID) {
        return;
    }
    if (GetDestructible() && GetExtra()->GetObjectHealth() != 0.0) {
        if (TESObjectREFR::Get3D() && !apObject) {
            BGSDestructibleObjectForm::GetDestructionForm(GetObjectReference())->UnloadReplacementModels();
        }
        else if (apObject && !TESObjectREFR::Get3D()) {
            TESBoundObject* pObject = GetObjectReference();
            BGSDestructibleObjectForm::GetDestructionForm(GetObjectReference())->PreloadReplacementModels(pObject);
        }
    }
    alignas(NiPointer<NiNode>) uint8_t aOld3DStorage[sizeof(NiPointer<NiNode>)];
    NiPointer<NiNode>& spOld3D = *::new (aOld3DStorage) NiPointer<NiNode>(TESObjectREFR::Get3D());
    if (spOld3D) {
        pParent = spOld3D->GetParent();
        if (pParent) {
            // a temporary tree's 3D stays attached
            if (!GetTemporary() || !data.pObjectReference || data.pObjectReference->GetFormType() != TREE_ID) {
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    NiNode* pOld3D = spOld3D;
                    TaskQueueInterface::GetInstance()->QueueNodeDetach(pOld3D);
                }
                else {
                    pParent->DetachChild(spOld3D);
                }
            }
            kTranslate = spOld3D->GetTranslate();
            kRotate = spOld3D->GetLocalTransform().m_Rotate;
            fScale = spOld3D->GetScale();
        }
        NiNode* pOld3D = spOld3D;
        BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->RemoveObject(pOld3D);
        if (IsMobileObject()) {
            pOld3D = spOld3D;
            BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->RemoveShadowLight(pOld3D);
        }
        if (!apObject) {
            bool bHasMovingSound = false;
            BSFadeNode* pFadeNode = spOld3D->IsFadeNode();
            if (pFadeNode) {
                pFadeNode->SetUserData(nullptr);
                bHasMovingSound = pFadeNode->QHasMovingSound();
            }
            if (spOld3D->QHasMovingSound()) {
                RemoveSounds(spOld3D->IsNode());
            }
            FreeUpAllMarkers();
            pVATS.ClearTarget(this);
            if (!GetDelete() && pSaveLoadGame->GetAllowChanges()) {
                // Each BGSChangeFlags argument is built in place through its constructor.
                BGSChangeFlags kHavokFlags(CHANGE_REFR_HAVOK_MOVE);
                if (pSaveLoadGame->GetChange(this, kHavokFlags)) {
                    BGSLoadGameSubBuffer kHavokBuffer;
                    kHavokBuffer.SaveHavokData(this);
                    char* pBuffer = kHavokBuffer;
                    GetExtra()->SetSavedHavokData(pBuffer);
                }
                BGSChangeFlags kAnimationFlags(CHANGE_REFR_ANIMATION);
                if (pSaveLoadGame->GetChange(this, kAnimationFlags)) {
                    bool bPlayingAnimation = false;
                    if (!IsActor()) {
                        bPlayingAnimation = IsPlayingAnimation();
                    }
                    if (bPlayingAnimation) {
                        BGSLoadGameSubBuffer kAnimationBuffer;
                        kAnimationBuffer.SaveAnimation(this);
                        char* pBuffer = kAnimationBuffer;
                        GetExtra()->SetSavedAnimation(pBuffer);
                    }
                    else {
                        RemoveChange(CHANGE_REFR_ANIMATION);
                    }
                }
            }
            KillLight(false);
            SetAnimation(nullptr);
            if (pActor) {
                RemoveWeapon();
                AttachAmmo(nullptr);
                RemoveAmmo();
                RemoveAmmo();
            }
            SetBiped(nullptr);
            bool bHasAddons = HasAddonFlags(spOld3D->IsNode());
            SetLoadedDataFlag(LOADED_DATA_HAS_ADDON_FLAGS, bHasAddons);
            if (bHasAddons) {
                RemoveAddonNodes(spOld3D);
            }
            if (pActor) {
                MobileObject* pMobile = static_cast<MobileObject*>(this);
                if (pMobile) {
                    bhkCharacterController* pController = static_cast<bhkCharacterController*>(
                        pMobile->GetCharController()
                    );
                    if (pController) {
                        pController->SetNiObject(nullptr);
                    }
                    if (pMobile->GetCurrentAIProcess()) {
                        pMobile->GetCurrentAIProcess()->SetBSBound(nullptr);
                    }
                }
                if (pActor->pRagdollController) {
                    delete pActor->pRagdollController;
                    pActor->pRagdollController = nullptr;
                }
                if (pActor->pPenetrationDetection) {
                    pActor->pPenetrationDetection->Clear();
                }
            }
            if (bHasMovingSound || IsProjectile() || IsMobileObject()) {
                NiNode* p3D = Get3D();
                static_cast<BSWin32Audio&>(BSAudio::QInstance()).StopMovingSounds(p3D, 0.0f, 0);
            }
            UpdateAddonNodeSounds(true);
        }
        if (!pParentCell || !pParentCell->GetUnloading()) {
            ReferenceProcessLists.FinishMagicHitEffect(this);
            if (ArrowProjectile::GetArrowCount() > 0) {
                ArrowProjectile::DeleteAttachedArrows(this, true);
            }
        }
        if (pActor && pActor != pPlayer) {
            BaseProcess* pProcess = pActor->GetCurrentAIProcess();
            if (pProcess && pProcess->GetProcessLevel() <= PROCESS_TYPE_MIDDLE_HIGH) {
                pProcess->SetLightingShaderProperty(nullptr);
            }
            GarbageCollector::Add(static_cast<NiAVObject*>(spOld3D));
        }
        else {
            NiObjectGarbage::Add(spOld3D);
        }
        // When set, the base object's UnClone3D is skipped.
        bool bSharedModel = GetLoadedDataFlag(LOADED_DATA_PRIMITIVE);
        spOld3D = nullptr;
        Set3DSimple(nullptr);
        if (!apObject && data.pObjectReference) {
            if (HasTemp3D()) {
                pModelLoader->CancelReference(this);
            }
            else if (!bSharedModel) {
                data.pObjectReference->UnClone3D(this);
            }
        }
    }
    else if (!apObject) {
        pModelLoader->CancelReference(this);
    }
    if (!apObject) {
        AttachSound(false);
    }
    Set3DSimple(apObject);
    if (apObject && pParent) {
        apObject->SetTranslate(kTranslate);
        apObject->SetRotate(kRotate);
        apObject->SetScale(fScale);
        pParent->AttachChild(apObject, true);
        bhkNiCollisionObject::ResetSim(apObject, true);
        apObject->UpdateProperties();
        NiUpdateData kUpdateData(0.0f, false, false);
        apObject->Update(kUpdateData);
    }
    // Sets the fade node's linked reference on the new 3D without checking its type.
    if (apObject && data.pObjectReference && data.pObjectReference->GetFormType() != TREE_ID) {
        static_cast<BSFadeNode*>(apObject)->SetUserData(this);
    }
    if (!pSaveLoadGameOLD->GetLoading() && !apObject && GetTeleport() && GetTeleport()->GetTeleportCell()) {
        GetTeleport()->GetTeleportCell()->SetLowestProcessToMiddleLow(false);
    }
    if (IsObstacle()) {
        if (apObject) {
            NavMeshObstacleManager::GetInstance()->AddObstacleForReference(this);
        }
        else {
            NavMeshObstacleManager::GetInstance()->RemoveObstacleForReference(this);
        }
    }
    if (!apObject && GetObjectReference() && GetObjectReference()->GetFormType() == DOOR_ID) {
        NavMeshObstacleManager::GetInstance()->OnDoorRemove(this);
    }
    spOld3D.~NiPointer<NiNode>();
}

// Tests the whole byte the base form's virtual returns and returns 0 or 1; the result goes through its byte in memory
// so that clang does not pass the callee's return on as an already normalised bool.
bool TESObjectREFR::HasRandomAnim() {
    if (!GetObjectReference()) {
        return false;
    }
    bool bRandomAnim = GetObjectReference()->GetRandomAnim();
    return static_cast<uint8_t>(bRandomAnim) != 0;
}

// The bound box corner from the 3D's "BBX" extra data, else zero. Calls TESObjectREFR::Get3D directly.
NiPoint3 TESObjectREFR::GetBoundMin() {
    NiAVObject* p3D = TESObjectREFR::Get3D();
    if (p3D) {
        NiExtraData* pExtraData = p3D->GetExtraData(BSBound::Tag());
        if (pExtraData) {
            BSBound* pBound = static_cast<BSBound*>(pExtraData);
            pBound->GetBoundMin(boundMin);
            return boundMin;
        }
    }
    return NiPoint3::ZERO;
}

NiPoint3 TESObjectREFR::GetBoundMax() {
    NiAVObject* p3D = TESObjectREFR::Get3D();
    if (p3D) {
        NiExtraData* pExtraData = p3D->GetExtraData(BSBound::Tag());
        if (pExtraData) {
            BSBound* pBound = static_cast<BSBound*>(pExtraData);
            pBound->GetBoundMax(boundMax);
            return boundMax;
        }
    }
    return NiPoint3::ZERO;
}

// Tests the base's form type: a light updates its light and then, like any non-actor, ends the sequences of its
// controller manager that finished playing; an actor (types 42 and 43) updates its Animation instead, or creates it
// when it has 3D but no Animation yet.
void TESObjectREFR::UpdateAnimation() {
    bool bdisabled = GetDisabled();
    if (GetDelete() || bdisabled) {
        return;
    }
    bool bactor;
    int32_t eTypeID = data.pObjectReference->GetFormType();
    if (eTypeID != 30 && eTypeID > 41 && eTypeID <= 43) {
        bactor = true;
    }
    else {
        if (eTypeID == 30) {
            UpdateRefLight();
        }
        bactor = false;
        NiNode* p3d = TESObjectREFR::Get3D();
        if (p3d) {
            NiTimeController* pc = p3d->GetControllers();
            if (pc) {
                NiControllerManager* pmanager = pc->IsNiControllerManager();
                uint32_t iactive = 0;
                if (pmanager && pmanager->GetActive()) {
                    for (uint32_t n = 0; n < pmanager->GetSequenceCount(); ++n) {
                        NiControllerSequence* pseq = pmanager->GetSequenceAt(n);
                        if (pseq && pseq->GetState()) {
                            if (BSAnimGroupSequence::PlaySounds(pseq, this)) {
                                EndSequence(pseq);
                                pmanager->DeactivateSequence(pseq, 0.0f);
                                const char* pSequenceName = pseq->m_kName.GetHandleString();
                                SetLastFinishedSequence(pSequenceName);
                                BGSOpenCloseForm::HandleAnimEnd(this, pseq);
                            }
                            else {
                                ++iactive;
                            }
                        }
                    }
                    if (!iactive) {
                        pmanager->SetActive(false);
                    }
                }
            }
        }
    }
    if (!bactor) {
        return;
    }
    Animation* animPtr = GetAnimation();
    NiAVObject* p3D = TESObjectREFR::Get3D();
    if (!animPtr) {
        if (p3D) {
            InitAnimation();
        }
        return;
    }
    if (!p3D || !p3D->m_pkParent) {
        return;
    }
    if (pVATS.GetMode() == VATS::VATS_PLAYBACK && this == pVATSTarget) {
        // The seconds are held as a double before the multiply.
        double dSeconds = appTimer.GetSeconds();
        animPtr->Update(this, static_cast<float>(pVATS.GetTargetUpdateMult() * dSeconds), -1.0f);
    }
    else {
        animPtr->Update(this, appTimer.GetSeconds(), -1.0f);
    }
}

Animation* TESObjectREFR::GetAnimation() {
    return TESObjectREFR::GetExtra()->GetAnimation();
}

BipedAnim* TESObjectREFR::GetCurrentBiped() {
    return GetBiped();
}

void TESObjectREFR::MoveHavok(bool abForceRec) {
    bhkNiCollisionObject::ResetSim(Get3D(), abForceRec);
}

// Rebuilds the container inventory from the base container: only the leveled items when abLeveledOnly (forced for a
// reference holding a quest item outside a save load), else everything. Items dropped from the container are then
// unlinked from it, and deleted unless they are quest objects.
void TESObjectREFR::ResetInventory(bool abLeveledOnly) {
    if (!pSaveLoadGame->GetSaveGameLoading() && HasQuestObject()) {
        abLeveledOnly = true;
    }
    if (HasContainer()) {
        InventoryChanges* pChanges = nullptr;
        if (abLeveledOnly) {
            pChanges = InventoryChanges::GetInventoryChanges(this);
            pChanges->RemoveDefaultLeveledItems();
        }
        else {
            GetExtra()->RemoveContainerChanges();
            pChanges = InventoryChanges::GetInventoryChanges(this);
        }
        if (IsActor()) {
            static_cast<Actor*>(this)->ClearProcessItems();
        }
        pChanges->InitLeveledItems();
        if (!abLeveledOnly) {
            pChanges->InitFromContainerExtra();
        }
        pChanges->InitScripts();
        if (pChanges->IsEmpty()) {
            GetExtra()->RemoveContainerChanges();
        }
        BSSimpleList<TESObjectREFR*>* pDropped = GetExtra()->GetDroppedItemList();
        for (; pDropped && !pDropped->IsEmpty(); pDropped->RemoveHead()) {
            TESObjectREFR* pItem = pDropped->GetItem();
            pItem->GetExtra()->SetItemDropper(nullptr);
            if (!pItem->IsQuestObject()) {
                pItem->MarkAsDeleted();
            }
        }
    }
}

bool TESObjectREFR::IsProjectile() {
    uint8_t cType = GetFormType();
    if (cType >= PMIS_ID && cType <= PFLA_ID) {
        return true;
    }
    return cType == PCBE_ID;
}

// Without a water zone map the loaded data takes the new cell's water height (-FLT_MAX without water); an actor's
// process moves its attached arrows, and a high process actor moves to high again.
void TESObjectREFR::SetParentCell(TESObjectCELL* apCell) {
    pParentCell = apCell;
    if (!GetExtra()->QWaterZoneMap()) {
        if (pLoadedData) {
            float fWaterHeight;
            if (apCell && apCell->HasWater()) {
                fWaterHeight = apCell->GetWaterHeight();
            }
            else {
                fWaterHeight = -std::numeric_limits<float>::max();
            }
            pLoadedData->fRelevantWaterHeight = fWaterHeight;
        }
        if (this == pPlayer) {
            TESWaterSystem::SetPlayerForceUnderwater(false);
        }
    }
    if (IsActor()) {
        MobileObject* pMobile = static_cast<MobileObject*>(this);
        if (pMobile->GetCurrentAIProcess()) {
            BaseProcess* pProcess = pMobile->GetCurrentAIProcess();
            pProcess->MoveAttachedArrowsToParentCell(this);
            if (pMobile->GetCurrentProcessType() == PROCESS_TYPE_HIGH) {
                pMobile->MovetoHigh();
            }
        }
    }
}

// An actor other than the player's base is dead when its base form's health is zero or less (compared as a float),
// anything else when it is destroyed.
bool TESObjectREFR::IsDead(bool /*abNotEssential*/) {
    // The form ID of the player's base NPC, whose health does not decide whether it is dead.
    constexpr uint32_t PLAYER_BASE_FORM_ID = 7;
    bool bDead = false;
    if (IsActor() && GetFormID() != PLAYER_BASE_FORM_ID) {
        float fHealth = static_cast<float>(TESHealthForm::GetFormHealth(GetObjectReference()));
        if (fHealth <= 0.0) {
            bDead = true;
        }
    }
    else if (GetDestroyed()) {
        bDead = true;
    }
    return bDead;
}

// The reference's receiver from its extra data; without one, a new receiver for this reference goes into a new
// ExtraAnimNoteReceiver.
BSAnimNoteReceiver* TESObjectREFR::CreateAnimNoteReceiver() {
    ExtraAnimNoteReceiver* pExtra = static_cast<ExtraAnimNoteReceiver*>(GetExtraData(EXTRA_ANIMNOTE_RECEIVER));
    if (!pExtra) {
        BSAnimNoteReceiver* pReceiver = new BSAnimNoteReceiver;
        pReceiver->SetObjectRefr(this);
        pExtra = new ExtraAnimNoteReceiver(pReceiver);
        GetExtra()->AddExtra(pExtra);
    }
    return pExtra->pReceiver;
}

BSAnimNoteReceiver* TESObjectREFR::GetAnimNoteReceiever() {
    ExtraAnimNoteReceiver* extraData = static_cast<ExtraAnimNoteReceiver*>(GetExtraData(EXTRA_ANIMNOTE_RECEIVER));
    if (extraData) {
        return extraData->pReceiver;
    }

    return nullptr;
}

bool TESObjectREFR::CanUseIdle(TESIdleForm* apIdle) {
    TESIdleForm* pidle;
    char cIdleDir[264];
    if (!apIdle) {
        return false;
    }
    for (pidle = apIdle; pidle->GetParentIdle(); pidle = pidle->GetParentIdle()) {
        ;
    }
    const char* pModel = GetModel();
    TESIdleManager::GetIdleDirName(pModel, cIdleDir);
    NiFormArray* parray = &pIdleManager->GetIdleAnimRoot(cIdleDir)->IdleArray;
    if (parray) {
        for (int32_t n = 0; n < parray->m_uiSize; ++n) {
            if (parray->GetFormAt(n) == pidle) {
                return true;
            }
        }
    }
    parray = pIdleManager->GetLooseIdleArray(cIdleDir);
    if (parray) {
        for (int32_t n = 0; n < parray->m_uiSize; ++n) {
            if (parray->GetFormAt(n) == pidle) {
                return true;
            }
        }
    }
    return false;
}

void TESObjectREFR::GatherHavokSaveData(BGSHavokSaveData* apHavokSaveData) {
    NiAVObject* pobj3D = TESObjectREFR::Get3D();
    if (pobj3D) {
        bhkWorldNS::ObjectRecData recData;
        apHavokSaveData->pRef = this;
        apHavokSaveData->pObj3D = pobj3D;
        apHavokSaveData->pCollisionRoot = bhkNiCollisionObject::GetbhkNiCollisionObject(pobj3D);
        recData.eAction = bhkWorldNS::OA_TOTAL;
        recData.bRecurse = true;
        recData.uDataA[0].p = apHavokSaveData;
        bhkWorld::DoObjectRec(pobj3D, recData, &TESObjectREFR::GatherHavokSaveDataForCollisionObject);
        if (apHavokSaveData->iActiveBoneCount && apHavokSaveData->iInactiveBoneCount) {
            apHavokSaveData->cFlags |= HAVOK_SAVE_BODY_FLAGS;
        }
        else {
            if (apHavokSaveData->iActiveBoneCount) {
                apHavokSaveData->cFlags |= HAVOK_SAVE_VELOCITIES;
            }
        }
        if (apHavokSaveData->iKeyFramedBoneCount) {
            apHavokSaveData->cFlags |= HAVOK_SAVE_BODY_FLAGS;
        }
    }
}

int32_t TESObjectREFR::GetInventoryCount(bool abBarter, bool abPlayableCount) {
    InventoryChanges* numCaravanLosses;
    int32_t count = (-1);
    if (!HasContainer()) {
        return count - 1;
    }
    if (abBarter) {
        numCaravanLosses = pDataHandler->GetBarterContainer();
    }
    else {
        numCaravanLosses = InventoryChanges::GetInventoryChanges(this);
    }
    count = numCaravanLosses->GetCount(abPlayableCount);
    return count - 1;
}

void TESObjectREFR::LoadAnimation(BGSLoadGameBuffer* apLoadGameBuffer) {
    char cSequenceName[268];
    NiControllerManager* pManager = nullptr;
    NiObjectNET* p3D = TESObjectREFR::Get3D();
    if (p3D && p3D->GetControllers()) {
        pManager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, p3D->GetControllers())
        );
    }

    apLoadGameBuffer->LoadString(cSequenceName);
    if (BSstrlen(cSequenceName)) {
        m_Extra.SetLastFinishedSequence(cSequenceName);
        char cReversed = 0;
        if (BGSOpenCloseForm::IsOpenCloseForm(GetObjectReference())) {
            if (BSstricmp(cSequenceName, "Open")) {
                if (!BSstricmp(cSequenceName, "Close")) {
                    BSstrcpy(cSequenceName, 260, "Open");
                    cReversed = 1;
                }
            }
            else {
                BSstrcpy(cSequenceName, 260, "Close");
                cReversed = 1;
            }
        }
        if (cReversed) {
            float fPosition = -std::numeric_limits<float>::max();
            SetSequencePosition(pManager, cSequenceName, fPosition);
        }
        else {
            SetSequencePosition(pManager, cSequenceName, std::numeric_limits<float>::max());
        }
    }
    TESObjectREFR::LoadControllerManagerBGS(pManager, apLoadGameBuffer, -1.0f);
}

// The saved flags and body count are read first. With the same body count the flags are restored, the 3D is made
// keyframed (an actor that is not unconscious, dead, dying or knocked out) or dynamic, updated, its bodies loaded, and
// updated again. Otherwise an actor's ragdoll animation is disabled and the 3D knocked down. The form ID is fetched
// before the editor ID.
void TESObjectREFR::LoadHavokDataBGS(BGSLoadGameBuffer* apLoadGameBuffer) {
    NiNode* p3D = TESObjectREFR::Get3D();
    if (!p3D) {
        uint32_t uiFormID = GetFormID();
        SaveGameWarning(
            "HAVOK: Cannot load Havok data for reference %s %08X because it has no 3D.",
            GetFormEditorID(),
            uiFormID
        );
        return;
    }
    uint8_t cFlags = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cFlags), sizeof(cFlags));
    uint32_t uiOrgCount = apLoadGameBuffer->LoadVariableSizedValue();
    BGSHavokSaveData kLoadData;
    kLoadData.pSaveLoadBuffer = apLoadGameBuffer;
    GatherHavokSaveData(&kLoadData);
    uint32_t uiCurrentCount = kLoadData.iActiveBoneCount + kLoadData.iInactiveBoneCount;
    if (uiOrgCount != uiCurrentCount) {
        uint32_t uiFormID = GetFormID();
        SaveGameWarning(
            "HAVOK: Havok data bone count differs on reference %s %08X.  Original count: %i, "
            "Current count: %i.  This should only happen with art changes.",
            GetFormEditorID(),
            uiFormID,
            uiOrgCount,
            uiCurrentCount
        );
        Actor* pActor = nullptr;
        if (IsActor()) {
            pActor = static_cast<Actor*>(this);
        }
        if (pActor) {
            if (pActor->pRagdollController) {
                pActor->pRagdollController->DisableRagdollAnim(true);
            }
            bhkBlendController::DoKnockDown(p3D, NiPoint3::ZERO, true, 0.0f, false);
        }
        return;
    }
    kLoadData.cFlags = cFlags;
    bhkWorldNS::ObjectRecData kRecData;
    kRecData.eAction = bhkWorldNS::OA_TOTAL;
    kRecData.bRecurse = true;
    kRecData.uDataA[0].p = &kLoadData;
    hkpMotion::MotionType eMotion = hkpMotion::MOTION_DYNAMIC;
    if (IsActor()) {
        Actor* pActor = static_cast<Actor*>(this);
        bool bInactive = pActor->IsUnConscious() ||
            pActor->IsDead(false) ||
            pActor->IsDying() ||
            pActor->IsKnockedOut();
        if (!bInactive) {
            eMotion = hkpMotion::MOTION_KEYFRAMED;
        }
    }
    bhkWorld::SetMotion(p3D, eMotion, true, false, false);
    NiUpdateData kUpdateData(0.0f, false, false);
    p3D->Update(kUpdateData);
    bhkWorld::DoObjectRec(p3D, kRecData, &TESObjectREFR::LoadHavokDataForCollisionObject);
    p3D->Update(kUpdateData);
}

// The walk data's action is 0x12 (OA_TOTAL: no table handler); its pWorld and uDataA[1..3] are deliberately left unset.
void TESObjectREFR::SaveHavokData(BGSSaveFormBuffer* apSaveGameBuffer) {
    NiNode* p3D = TESObjectREFR::Get3D();
    if (p3D) {
        BGSHavokSaveData kSaveData;
        kSaveData.pSaveLoadBuffer = apSaveGameBuffer;
        GatherHavokSaveData(&kSaveData);
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&kSaveData.cFlags), sizeof(kSaveData.cFlags), 0);
        uint32_t uiBodyCount = kSaveData.iActiveBoneCount + kSaveData.iInactiveBoneCount;
        apSaveGameBuffer->SaveVariableSizedValue(uiBodyCount);
        bhkWorldNS::ObjectRecData kRecData;
        kRecData.eAction = bhkWorldNS::OA_TOTAL;
        kRecData.bRecurse = true;
        kRecData.uDataA[0].p = &kSaveData;
        bhkWorld::DoObjectRec(p3D, kRecData, &TESObjectREFR::SaveHavokDataForCollisionObject);
    }
}

void TESObjectREFR::SaveHavokData(HavokSaveData* apHavokSaveData) {
    NiAVObject* pobj3D = TESObjectREFR::Get3D();
    if (pobj3D) {
        bhkWorldNS::ObjectRecData recData;
        SaveGameDataOLD(apHavokSaveData, 1);
        uint16_t sboneCount = apHavokSaveData->sActiveBoneCount + apHavokSaveData->sInactiveBoneCount;
        SaveGameDataOLD(&sboneCount, sizeof(sboneCount));
        recData.eAction = bhkWorldNS::OA_TOTAL;
        recData.bRecurse = true;
        recData.uDataA[0].p = apHavokSaveData;
        bhkWorld::DoObjectRec(pobj3D, recData, &TESObjectREFR::SaveHavokDataForCollisionObjectOld);
    }
}

// The exe's cleanup-only C++ EH frame is left out.
void TESObjectREFR::SetLinkLineRecurse(bool abShow, BSMap<TESObjectREFR*, bool>* apVisited) {
    // Starts true, for visited (a departure): when GetAt misses it leaves bVisited unwritten, and SetAt stores it in
    // the map; the exe stores an uninitialised byte. Nothing reads the stored values, so any value would do.
    bool bVisited = true;
    BSMap<TESObjectREFR*, bool> kVisited(37u);
    if (!apVisited) {
        apVisited = &kVisited;
    }
    if (apVisited->GetAt(this, bVisited)) {}
    else {
        apVisited->SetAt(this, bVisited);
        if (GetLinkedRef()) {
            SetLinkLine(abShow);
            GetLinkedRef()->SetLinkLineRecurse(abShow, apVisited);
        }
        for (BSSimpleList<TESObjectREFR*>* pIter = m_Extra.GetLinkedRefChildren(); pIter && !pIter->IsEmpty();
             pIter = pIter->m_pkNext) {
            pIter->m_item->SetLinkLine(abShow);
            pIter->m_item->SetLinkLineRecurse(abShow, apVisited);
        }
        kVisited.RemoveAll_1();
    }
}

// Loads the reference's Havok state (saves 81 on; older ones go to LoadHavokDataPreVersion81): the flags and the
// saved bone count, then each rigid body's state through LoadHavokDataForCollisionObjectOld when the bone count still
// matches the 3D's; otherwise the data is skipped with a warning and an actor is knocked down.
void TESObjectREFR::LoadHavokData(uint16_t aiSaveSize) {
    bhkWorldNS::ObjectRecData recData;
    uint16_t sSavedBoneCount;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 81u) {
        NiAVObject* pObj3D = TESObjectREFR::Get3D();
        if (pObj3D) {
            HavokSaveData havokSaveData;
            LoadGameDataOLD(&havokSaveData, 1);
            LoadGameDataOLD(&sSavedBoneCount, sizeof(sSavedBoneCount));
            Actor* pActor = nullptr;
            if (IsActor()) {
                pActor = static_cast<Actor*>(this);
            }
            if (pActor) {
                if ((havokSaveData.cFlags & HAVOK_SAVE_WEAPON) != 0) {
                    pActor->SetHavokWeapon(true);
                }
                else {
                    pActor->SetHavokWeapon(false);
                }
            }
            HavokSaveData currentHavokSaveData;
            GetHavokDataSaveSize(&currentHavokSaveData);
            uint16_t sCurrentBoneCount = currentHavokSaveData.sInactiveBoneCount +
                currentHavokSaveData.sActiveBoneCount;
            if (sSavedBoneCount == sCurrentBoneCount) {
                havokSaveData.pRef = this;
                havokSaveData.pObj3D = pObj3D;
                havokSaveData.pCollisionRoot = bhkNiCollisionObject::GetbhkNiCollisionObject(pObj3D);
                recData.eAction = bhkWorldNS::OA_TOTAL;
                recData.bRecurse = true;
                recData.uDataA[0].p = &havokSaveData;
                if (IsActor()) {
                    bhkWorld::SetMotion(pObj3D, hkpMotion::MOTION_DYNAMIC, true, false, true);
                    NiUpdateData kUpdateData(0.0f, false, false);
                    pObj3D->Update(kUpdateData);
                }
                bhkWorld::DoObjectRec(pObj3D, recData, &TESObjectREFR::LoadHavokDataForCollisionObjectOld);
                if (pActor && pActor->IsParalyzed()) {
                    bhkRigidBody::SetFixed(pObj3D, 1, 1);
                    bhkWorld::SetMotion(pObj3D, hkpMotion::MOTION_DYNAMIC, true, false, true);
                    Animation* pAnim = pActor->GetAnimation();
                    if (pAnim) {
                        pAnim->ClearGroup(AGS_WHOLE_BODY, 0.0f);
                        pAnim->ClearAccumRootTranslate();
                    }
                }

                if (!IsActor()) {
                    NiUpdateData kUpdateData(0.0f, false, false);
                    pObj3D->Update(kUpdateData);
                }
            }
            else {
                int32_t iCurrentBoneCount = sCurrentBoneCount;
                int32_t iSavedBoneCount = sSavedBoneCount;
                int32_t iRefFormID = iFormID;
                Warning(
                    "HAVOK: Havok data bone count differs on reference %s %08X.  Original count: %i, Current "
                    "count: %i.  This should only happen with art changes.",
                    GetFormEditorID(),
                    iRefFormID,
                    iSavedBoneCount,
                    iCurrentBoneCount
                );
                if ((havokSaveData.cFlags & HAVOK_SAVE_WEAPON) != (currentHavokSaveData.cFlags & HAVOK_SAVE_WEAPON)) {
                    const char* pCurrent = (currentHavokSaveData.cFlags & HAVOK_SAVE_WEAPON) != 0 ? "true" : "false";
                    const char* pSaved = (havokSaveData.cFlags & HAVOK_SAVE_WEAPON) != 0 ? "true" : "false";
                    Warning(
                        "HAVOK: Bone count difference likely due to weapon bone difference.  Saved: %s Current: %s",
                        pSaved,
                        pCurrent
                    );
                }
                pSaveLoadGameOLD->AdvanceBuffer(aiSaveSize - 3);
                if (pActor) {
                    if (pActor->pRagdollController) {
                        pActor->pRagdollController->DisableRagdollAnim(true);
                    }
                    bhkBlendController::DoKnockDown(pObj3D, NiPoint3::ZERO, 1, 0.0f, 0);
                }
            }
        }
        else {
            int32_t iRefFormID = iFormID;
            Warning(
                "HAVOK: Cannot load Havok data for reference %s %08X because it has no 3D.",
                GetFormEditorID(),
                iRefFormID
            );
            pSaveLoadGameOLD->AdvanceBuffer(aiSaveSize);
        }
    }
    else {
        LoadHavokDataPreVersion81(aiSaveSize);
    }
}

// An activated/door reference whose sequence is animation group 25 ends the reference's Havok motion: the cell's world
// activates the 3D, then the motion change is either queued for the 3D task thread or set here, and the reference is
// marked changed.
void TESObjectREFR::EndSequence(NiControllerSequence* apSequence) {
    uint8_t ucFormType = data.pObjectReference->GetFormType();
    if (ucFormType != ACTI_ID && ucFormType != DOOR_ID) {
        return;
    }
    const char* pSequenceName = apSequence->GetName().GetHandleString();
    // Reads animGroupInfo entry 25's name; the group enum has no name for 25.
    if (BSstricmp(pSequenceName, animGroupInfo[25].pName) != 0) {
        return;
    }
    if (!pParentCell) {
        return;
    }
    if (!pParentCell->GetbhkWorld()) {
        return;
    }
    bhkWorld::Activate(Get3D(), true, true, true);
    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
        BSPackedTask::PackedTaskVariantData kMotionChange;
        kMotionChange.iData = 1;
        // Takes the 3D before it fetches the task queue.
        NiAVObject* p3D = Get3D();
        TaskQueueInterface::GetInstance()->QueueHavokMotionChange(p3D, kMotionChange.iData);
    }
    else {
        // Reads the world and drops the result; only the call itself matters.
        pParentCell->GetbhkWorld();
        bhkWorld::SetMotion(Get3D(), hkpMotion::MOTION_DYNAMIC, true, false, true);
    }
    AddChange(CHANGE_REFR_HAVOK_MOVE);
}

bool TESObjectREFR::GetMarkerUsed(uint32_t aiMarkerIndex, bool abIgnoreReservations) {
    bool bMarkerUsed = false;
    ExtraUsedMarkers* pUsedMarkers = static_cast<ExtraUsedMarkers*>(GetExtra()->GetExtraData(18u));
    ExtraUsedMarkers* pReservedMarkers = static_cast<ExtraUsedMarkers*>(GetExtra()->GetExtraData(130u));
    if (pUsedMarkers && GetObjectReference()) {
        if (GetObjectReference()->GetFormType() == 39) {
            bMarkerUsed = pUsedMarkers->GetMarkerUsed(aiMarkerIndex);
        }
    }

    if (pReservedMarkers && !bMarkerUsed && !abIgnoreReservations) {
        return pReservedMarkers->GetMarkerUsed(aiMarkerIndex);
    }

    return bMarkerUsed;
}

// Checks whether the activating actor is a high-process actor fading out to teleport (BaseProcess::GetFadeState).
bool TESObjectREFR::Activate(
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjecttoGet,
    int32_t aiCount
) {
    static int32_t iprocessingscripts;
    bool bActivated;
    int32_t i3rdPerson;
    bool bFadingOut = false;
    TESObjectACTI* pObject = static_cast<TESObjectACTI*>(GetObjectReference());
    if (pObject == pAshPile1 || static_cast<TESObjectACTI*>(GetObjectReference()) == pAshPile2) {
        TESObjectREFR* pAshPileRef = m_Extra.GetAshPileRef();
        if (pAshPileRef) {
            return pAshPileRef->Activate(apActionRef, abIdFlag, apObjecttoGet, aiCount);
        }
    }
    bScriptActivate = false;
    if (apActionRef && apActionRef->IsActor()) {
        Actor* pActionActor = static_cast<Actor*>(apActionRef);
        if (pActionActor->GetCurrentAIProcess()) {
            if (pActionActor->GetCurrentAIProcess()->GetProcessLevel() == PROCESS_TYPE_HIGH) {
                if (pActionActor->GetCurrentAIProcess()->GetFadeState() == BaseProcess::FADE_TELEPORT_OUT) {
                    bFadingOut = true;
                }
            }
        }
    }

    if (apActionRef == pPlayer) {
        if (Interface::IsPipBoyOpening()) {
            return true;
        }
        i3rdPerson = pPlayer->Is3rdPerson();
        if (i3rdPerson != pPlayer->GetWant3rdPerson()) {
            return false;
        }
        if (pPlayer->IsChild(false) && !pPlayer->CanChildUseObject(this)) {
            Interface::ShowMessage(sNoChildUse.String(), 0, nullptr, nullptr, 2.0f, false);
            return true;
        }
    }

    if (!GetDestroyed() || IsActor()) {
        if (GetObjectReference() &&
            GetObjectReference()->GetFormType() == DOOR_ID &&
            apActionRef &&
            apActionRef->IsActor() &&
            !*GetFullName()) {
            return false;
        }

        if (apActionRef != pPlayer ||
            *GetFullName() ||
            GetObjectReference()->GetFormType() == PWAT_ID ||
            GetObjectReference()->GetFormType() == TACT_ID) {
            if (GetInaccessible()) {
                return false;
            }

            if (!m_Extra.IsActivateParentOnly() || m_Extra.GetActivateData(apActionRef)) {
                if (apActionRef && !bFadingOut) {
                    if (Interface::GetConsoleSelection() == this) {
                        char cMessage[268];
                        const char* pFullName = TESFullName::GetFullName(GetObjectReference());
                        const char* pFormTypeString = GetObjectReference()->GetFormTypeString();
                        std::sprintf(
                            cMessage,
                            "'%s' activated %s '%s'",
                            TESFullName::GetFullName(apActionRef->GetObjectReference()),
                            pFormTypeString,
                            pFullName
                        );
                        Interface::PrintLine(static_cast<const char*>(cMessage));
                    }
                    if (this != pPlayer && !GetAction(OBJECT_ACTION_USE_DEFAULT)) {
                        SetAction(OBJECT_ACTION_ACTIVATE);
                        SetActionRef(apActionRef);
                        if (iprocessingscripts < 5) {
                            ++iprocessingscripts;
                            RunScript();
                            --iprocessingscripts;
                        }
                        if (!bScriptActivate && apActionRef->IsActor()) {
                            static_cast<Actor*>(apActionRef)
                                ->Actor::ActivatedReference(this, apObjecttoGet, aiCount, true);
                        }
                        return bScriptActivate;
                    }
                }
                m_Extra.NotifyGuardsAboutActivation(this, apActionRef);
                m_Extra.RemoveExtra(124);
                if (GetObjectReference()) {
                    switch (GetObjectReference()->GetFormType()) {
                        case STAT_ID:
                        case SCOL_ID:
                        case MSTT_ID:
                        case TREE_ID: {
                            bActivated = false;
                            break;
                        }
                        case IDLM_ID: {
                            return true;
                        }
                        case DOOR_ID: {
                            DoorTeleportData* pTel = GetTeleport();
                            if (pTel && apActionRef == pPlayer) {
                                if (pPlayer->pCurrentProcess->GetFadeState() != BaseProcess::FADE_TELEPORT_OUT) {
                                    TESObjectCELL* pcell = pTel->GetTeleportCell();
                                    if (pcell && pcell->GetInterior()) {
                                        pcell->ProcessActors();
                                    }
                                }
                            }
                            [[fallthrough]];
                        }
                        default: {
                            InitChildActivates(apActionRef);
                            if (data.pObjectReference->Activate(this, apActionRef, abIdFlag, apObjecttoGet, aiCount)) {
                                if (apActionRef && apActionRef->IsActor()) {
                                    static_cast<Actor*>(apActionRef)
                                        ->Actor::ActivatedReference(this, apObjecttoGet, aiCount, false);
                                    if (apActionRef != pPlayer) {
                                        pPlayer->CheckForQuestTargetUpdate(apActionRef);
                                    }
                                }

                                bScriptActivate = true;
                                bActivated = true;
                            }
                            else {
                                bActivated = false;
                            }
                            break;
                        }
                    }
                }
                else {
                    return false;
                }
            }
            else {
                if (apActionRef == pPlayer) {
                    Interface::ShowMessage(
                        sRemoteActivation.String(),
                        0,
                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_thinking.dds",
                        nullptr,
                        2.0f,
                        false
                    );
                }
                return false;
            }
        }
        else {
            return false;
        }
    }
    else {
        return true;
    }

    return bActivated;
}

ExtraDataList* TESObjectREFR::GetExtra() {
    return &m_Extra;
}

LightRefData* TESObjectREFR::GetSpellEffectLight() {
    return TESObjectREFR::GetExtra()->GetSpellEffectLight();
}

// Saves the playing sequences of the 3D's controller manager (none without 3D or controllers) at the current time.
void TESObjectREFR::SaveAttachedAnimation() {
    NiControllerManager* pManager = nullptr;
    NiNode* p3D = TESObjectREFR::Get3D();
    if (p3D && p3D->GetControllers()) {
        NiTimeController* pControllers = p3D->GetControllers();
        pManager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pControllers)
        );
    }
    SaveControllerManager(pManager, -1.0f);
}

uint16_t TESObjectREFR::GetAttachedAnimationSaveSize() {
    NiControllerManager* pControllerManager = nullptr;
    NiNode* p3D = TESObjectREFR::Get3D();
    if (p3D && p3D->GetControllers()) {
        NiObject* pControllers = p3D->GetControllers();
        pControllerManager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pControllers)
        );
    }

    return TESObjectREFR::GetControllerManagerSaveSize(pControllerManager);
}

// The size accumulates in a 16-bit local. The walk data's action is OA_TOTAL (no table handler); its
// pWorld and uDataA[1..3] are deliberately left unset.
uint16_t TESObjectREFR::GetHavokDataSaveSize(HavokSaveData* apHavokSaveData) {
    uint16_t usSize = 0;
    NiNode* p3D = TESObjectREFR::Get3D();
    if (p3D) {
        ++usSize;
        usSize += 2;
        HavokSaveData kSaveData;
        if (!apHavokSaveData) {
            apHavokSaveData = &kSaveData;
        }
        apHavokSaveData->pRef = this;
        apHavokSaveData->pObj3D = p3D;
        apHavokSaveData->pCollisionRoot = bhkNiCollisionObject::GetbhkNiCollisionObject(p3D);
        bhkWorldNS::ObjectRecData kRecData;
        kRecData.eAction = bhkWorldNS::OA_TOTAL;
        kRecData.bRecurse = true;
        kRecData.uDataA[0].p = apHavokSaveData;
        bhkWorld::DoObjectRec(p3D, kRecData, &TESObjectREFR::GetCollisionObjectSize);
        uint16_t usBodies = apHavokSaveData->sActiveBoneCount + apHavokSaveData->sInactiveBoneCount;
        if (apHavokSaveData->sActiveBoneCount && apHavokSaveData->sInactiveBoneCount) {
            apHavokSaveData->cFlags |= HAVOK_SAVE_BODY_FLAGS;
            usSize += usBodies;
        }
        else if (apHavokSaveData->sActiveBoneCount) {
            apHavokSaveData->cFlags |= HAVOK_SAVE_VELOCITIES;
        }
        if (apHavokSaveData->sActiveBoneCount) {
            usSize += 24 * apHavokSaveData->sActiveBoneCount;
        }
        uint16_t usPlaced = usBodies;
        if (apHavokSaveData->cFlags & HAVOK_SAVE_ROOT_BODY) {
            --usPlaced;
        }
        usSize += 28 * usPlaced;
    }
    return usSize;
}

// Saves the last finished sequence's name, then the playing sequences of the 3D's controller manager (none without 3D
// or controllers) at the current time, into apSaveGameBuffer.
void TESObjectREFR::SaveAnimation(BGSSaveFormBuffer* apSaveGameBuffer) {
    NiControllerManager* pManager = nullptr;
    NiNode* p3D = TESObjectREFR::Get3D();
    if (p3D && p3D->GetControllers()) {
        NiTimeController* pControllers = p3D->GetControllers();
        pManager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pControllers)
        );
    }
    const char* pLastFinishedSequence = GetExtra()->GetLastFinishedSequence();
    apSaveGameBuffer->SaveString(pLastFinishedSequence, 0);
    SaveControllerManager(pManager, apSaveGameBuffer, -1.0f);
}

void TESObjectREFR::MarkAsDeleted() {
    SetDelete(true);
    SetAltered(true);
    Set3D(nullptr, false);
}

Script* TESObjectREFR::GetScriptPointer() {
    return TESObjectREFR::GetExtra()->GetScript();
}

TESBoundObject* TESObjectREFR::GetObjectReference() {
    return data.pObjectReference;
}

// The reference's health: an actor's Health actor value (100 while essential and down), else the base object's
// health, overridden by the reference's ExtraHealth; -1 when it has none.
float TESObjectREFR::GetHealth() {
    float fHealth = -1.0f;
    if (IsActor()) {
        Actor* pActor = static_cast<Actor*>(this);
        if (pActor->GetLifeState() == ACTOR_LIFE_STATE_ESSENTIAL_DOWN) {
            fHealth = 100.0f;
        }
        else {
            fHealth = static_cast<float>(static_cast<ActorValueOwner*>(pActor)->GetActorValue(ActorValue::HEALTH));
        }
    }
    else {
        TESHealthForm* pHealthForm = dynamic_cast<TESHealthForm*>(GetObjectReference());
        if (pHealthForm) {
            ExtraHealth* pExtraHealth = static_cast<ExtraHealth*>(GetExtra()->GetExtraData(EXTRA_HEALTH));
            if (pExtraHealth) {
                fHealth = pExtraHealth->fHealth;
            }
            else {
                fHealth = static_cast<float>(pHealthForm->GetFormHealth());
            }
        }
    }
    return fHealth;
}

TESObjectCELL* TESObjectREFR::GetParentCell() {
    return pParentCell;
}

const NiPoint3& TESObjectREFR::GetAngleOnReference() {
    return data.kRotation;
}

void TESObjectREFR::GetMapNameForLocation(BSStringT<char>& aOutMapName) {
    TESObjectCELL* pint = GetParentCell();
    if (pint && !pint->GetInterior()) {
        pint = nullptr;
    }

    if (pint) {
        aOutMapName.Set(pint->GetFullName(), 0u);
    }
    else {
        TESWorldSpace* pworld = GetWorldSpace();
        if (pworld) {
            NiPoint3 kLocation = GetLocationOnReference();
            pworld->GetMapNameForLocation(aOutMapName, kLocation);
        }
        else {
            aOutMapName.Set("", 0u);
        }
    }
}

BSExtraData* TESObjectREFR::GetExtraData(uint8_t aType) {
    return GetExtra()->GetExtraData(static_cast<EXTRA_DATA_TYPE>(aType));
}

bool TESObjectREFR::ShouldPlaySound() {
    if (GetDisabledOrDeleted()) {
        return false;
    }
    if (GetObjectReference()->GetFormType() == DOOR_ID && GetDestroyed()) {
        return false;
    }
    return true;
}

void TESObjectREFR::SetTargeted(bool abVal) {
    uint32_t iFlags = GetFormFlags();
    if (abVal) {
        iFlags |= kFormFlags_Targeted;
    }
    else {
        iFlags &= ~kFormFlags_Targeted;
    }
    SetFormFlags(iFlags);
}

void TESObjectREFR::SetHasTemp3D(bool abVal) {
    uint32_t iFlags = GetFormFlags();
    if (abVal) {
        iFlags |= kFormFlag_HasTemp3D;
    }
    else {
        iFlags &= ~kFormFlag_HasTemp3D;
    }
    SetFormFlags(iFlags);
}

bool TESObjectREFR::HasTemp3D() {
    return (GetFormFlags() & kFormFlag_HasTemp3D) != 0;
}

bool TESObjectREFR::GetPersistFlagValue() {
    return (GetFormFlags() & kFormFlags_IsRefPersistent) != 0;
}

bool TESObjectREFR::GetDroppedRef() {
    if (GetObjectReference() == nullptr) {
        return false;
    }
    if (GetObjectReference()->GetFormType() == DOOR_ID) {
        return false;
    }
    return (GetFormFlags() & kFormFlags_DroppedRef) != 0;
}

bool TESObjectREFR::GetVisibleDistant() {
    if ((GetFormFlags() & TESForm::kFormFlag_MustBeVisibleDistant) != 0) {
        return true;
    }
    if (GetObjectReference() != nullptr && GetObjectReference()->GetMustBeVisibleDistant()) {
        return true;
    }
    return false;
}

void TESObjectREFR::Set3DSimple(NiAVObject* ap3D) {
    if (IsObstacle()) {
        if (ap3D) {
            NavMeshObstacleManager::GetInstance()->AddObstacleForReference(this);
        }
        else {
            NavMeshObstacleManager::GetInstance()->RemoveObstacleForReference(this);
        }
    }
    if (!ap3D && GetObjectReference() && GetObjectReference()->GetFormType() == DOOR_ID) {
        NavMeshObstacleManager::GetInstance()->OnDoorRemove(this);
    }
    if (!ap3D && GetObjectReference() == pRoomMarker) {
        GetExtra()->SetRoom(nullptr);
    }
    else if (!ap3D && GetObjectReference() == pPortalMarker) {
        GetExtra()->SetPortal(nullptr);
    }
    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
        TaskQueueInterface::GetInstance()->QueueHavokDetach(this);
    }
    else {
        DetachHavok();
    }
    Set3DVerySimple(ap3D);
}

bool TESObjectREFR::GetCenterOnCreation() {
    if (IsActor()) {
        return false;
    }
    return (GetFormFlags() & kFormFlags_CenterRefOnCreation) != 0;
}

bool TESObjectREFR::IsFurniture() {
    if (data.pObjectReference == nullptr) {
        return false;
    }
    return GetObjectReference()->GetFormType() == FURN_ID;
}

bool TESObjectREFR::Is3DCritical() {
    return !IsActor();
}

void TESObjectREFR::SetAngleOnReferenceX(float afx) {
    data.kRotation.x = afx;
    AddChange(CHANGE_REFR_MOVE);
}

float TESObjectREFR::GetRelevantWaterHeight() {
    // the lowest representable float: no water
    float fHeight = -std::numeric_limits<float>::max();
    if (pLoadedData != nullptr) {
        fHeight = pLoadedData->fRelevantWaterHeight;
    }
    return fHeight;
}

ScriptLocals* TESObjectREFR::GetScriptVariables() {
    return GetExtra()->GetScriptLocals();
}

TESForm* TESObjectREFR::GetMyOwner() {
    return GetExtra()->GetOwner();
}

void TESObjectREFR::SetOwner(TESForm* apOwner) {
    GetExtra()->SetOwnership(apOwner);
    AddChange(CHANGE_REFR_EXTRA_OWNERSHIP);
    DoorTeleportData* pTeleport;
    pTeleport = GetTeleport();
    if (pTeleport && pTeleport->GetLinkedDoor()) {
        pTeleport->GetLinkedDoor()->GetExtra()->SetOwnership(nullptr);
        TESForm* pLinkedDoor = pTeleport->GetLinkedDoor();
        pLinkedDoor->AddChange(CHANGE_REFR_EXTRA_OWNERSHIP);
    }
}

void TESObjectREFR::SetGlobal(TESGlobal* apGlobal) {
    GetExtra()->SetGlobal(apGlobal);
    AddChange(CHANGE_REFR_EXTRA_OWNERSHIP);
    DoorTeleportData* pTeleport;
    pTeleport = GetTeleport();
    if (pTeleport && pTeleport->GetLinkedDoor()) {
        pTeleport->GetLinkedDoor()->GetExtra()->SetGlobal(nullptr);
        TESForm* pLinkedDoor = pTeleport->GetLinkedDoor();
        pLinkedDoor->AddChange(CHANGE_REFR_EXTRA_OWNERSHIP);
    }
}

void TESObjectREFR::SetRank(int32_t aiRank) {
    GetExtra()->SetRank(aiRank);
    AddChange(CHANGE_REFR_EXTRA_OWNERSHIP);
    DoorTeleportData* pTeleport;
    pTeleport = GetTeleport();
    if (pTeleport && pTeleport->GetLinkedDoor()) {
        pTeleport->GetLinkedDoor()->GetExtra()->SetRank(-1);
        TESForm* pLinkedDoor = pTeleport->GetLinkedDoor();
        pLinkedDoor->AddChange(CHANGE_REFR_EXTRA_OWNERSHIP);
    }
}

DoorTeleportData* TESObjectREFR::GetTeleport() {
    return GetExtra()->GetTeleport();
}

DoorTeleportData* TESObjectREFR::AddTeleport() {
    DoorTeleportData* pTeleport = TESObjectREFR::GetExtra()->GetTeleport();
    if (!pTeleport) {
        pTeleport = new DoorTeleportData;
        TESObjectREFR::GetExtra()->SetTeleportPtr(pTeleport);
        AddChange(CHANGE_DOOR_EXTRA_TELEPORT);
    }
    return pTeleport;
}

void TESObjectREFR::RemoveTeleport() {
    TESObjectREFR* pThis = this;
    pThis->m_Extra.RemoveTeleportPtr();
    ExtraNavMeshPortal* pportal = pThis->m_Extra.GetNavMeshPortal();
    if (pportal) {
        NavMesh* pnavmesh = pportal->Portal.pNavMesh;
        if (pnavmesh) {}
        pThis->m_Extra.RemoveNavMeshPortal();
    }
    pThis->RemoveChange(CHANGE_DOOR_EXTRA_TELEPORT);
}

void TESObjectREFR::RemoveOwner() {
    TESObjectREFR::GetExtra()->SetOwnership(nullptr);
    TESObjectREFR::GetExtra()->SetGlobal(nullptr);
    TESObjectREFR::GetExtra()->SetRank(-1);
    AddChange(CHANGE_REFR_EXTRA_OWNERSHIP);
    DoorTeleportData* pTeleport = GetTeleport();
    if (pTeleport && pTeleport->pLinkedDoor) {
        pTeleport->pLinkedDoor->TESObjectREFR::GetExtra()->SetOwnership(nullptr);
        pTeleport->pLinkedDoor->TESObjectREFR::GetExtra()->SetGlobal(nullptr);
        pTeleport->pLinkedDoor->TESObjectREFR::GetExtra()->SetRank(-1);
        pTeleport->pLinkedDoor->AddChange(CHANGE_REFR_EXTRA_OWNERSHIP);
    }
}

void TESObjectREFR::SetLinkLine(bool abShow) {
    NiNode* p3D = Get3D();
    if (p3D) {
        NiNode* pNode = p3D->IsNode();
        if (pNode) {
            NiAVObject* pLinkLine = pNode->GetObjectByName("LinkLine");
            if (pLinkLine) {
                pNode->DetachChild(pLinkLine);
            }
        }
    }
}

void TESObjectREFR::SetActivateLine(bool abShow) {
    NiPoint3 kStart;
    NiMatrix3 kInverse;
    NiPoint3 kEnd;
    NiNode* p3D = Get3D();
    if (p3D) {
        NiNode* pRoot = p3D->IsNode();
        if (pRoot) {
            char cAttached = 0;
            NiAVObject* pObject = pRoot->GetObjectByName("ActivateLine");
            NiNode* pLineNode = pObject ? pObject->IsNode() : nullptr;
            if (pLineNode) {
                pLineNode->RemoveAllChildren();
                if (!abShow) {
                    pRoot->DetachChild(pLineNode);
                    pLineNode = nullptr;
                }
            }
            if (abShow) {
                // The parents' list, walked node by node: the node is tested for null and IsEmpty at the top of each
                // pass, then the item's reference and the next node are taken.
                BSSimpleList<REF_ACTIVATE_DATA*>* pNode = m_Extra.GetActivateParentList();
                while (pNode && !pNode->IsEmpty()) {
                    TESObjectREFR* pParent = pNode->m_item->pActivateRef;
                    pNode = pNode->m_pkNext;
                    kStart = NiPoint3::ZERO;
                    const NiPoint3* pLocation = &GetLocationOnReference();
                    const NiPoint3* pParentLocation = &pParent->GetLocationOnReference();
                    ::new (&kEnd) NiPoint3(*pParentLocation - *pLocation);
                    NiMatrix3 kRotation = GetOrientation();
                    kRotation.Inverse(&kInverse);
                    kEnd = kInverse * kEnd;
                    float fInvScale = 1.0 / GetScale();
                    kEnd = kEnd * fInvScale;
                    NiLines* pLine = MakeLine(
                        kStart,
                        NiColorA(1.0f, 1.0f, 1.0f, 1.0f),
                        kEnd,
                        NiColorA(0.0f, 0.0f, 0.0f, 1.0f),
                        true
                    );
                    if (!pLineNode) {
                        pLineNode = new NiNode(0);
                        pLineNode->SetName("ActivateLine");
                        pRoot->AttachChild(pLineNode, 1);
                    }
                    pLineNode->AttachChild(pLine, 1);
                    cAttached = 1;
                }
            }
            if (cAttached) {
                pRoot->UpdateProperties();
                NiUpdateData kUpdateData(0.0f, false, false);
                pRoot->Update(kUpdateData);
            }
        }
    }
}

void TESObjectREFR::SetActivateLineRecurse(bool abShow) {
    SetActivateLine(abShow);
    for (BSSimpleList<REF_ACTIVATE_DATA*>* pIter = m_Extra.GetActivateRefChildren(); pIter && !pIter->IsEmpty();
         pIter = pIter->m_pkNext) {
        pIter->m_item->pActivateRef->SetActivateLine(abShow);
    }
}

// Fits the occlusion plane, portal, room or multibound attached to a primitive marker reference to the primitive's
// radii and its node's transform, then updates the world's shadow scene node. A multibound's AABB or OBB shape is
// resized in place when it still fits the reference's rotation; otherwise the primitive makes a new shape.
void TESObjectREFR::FixAttachedBSObjectSizeAndTransform() {
    BGSPrimitive* pPrimitive = GetExtra()->GetPrimitive();
    if (!pPrimitive) {
        return;
    }
    if (GetObjectReference() == pPlaneMarker || GetObjectReference() == pPortalMarker) {
        BSOcclusionPlane* pPlane = nullptr;
        if (GetObjectReference() == pPlaneMarker) {
            pPlane = GetExtra()->GetOcclusionPlane();
        }
        else {
            pPlane = GetExtra()->GetPortal();
        }
        if (pPlane) {
            float fHalfHeight = pPrimitive->GetRadii().z;
            float fHalfWidth = pPrimitive->GetRadii().x;
            NiPoint2 kHalfExtents(fHalfWidth, fHalfHeight);
            pPlane->SetHalfExtents(kHalfExtents);
            pPlane->SetCenter(pPrimitive->GetNode()->GetWorldTranslate());
            pPlane->SetRotation(pPrimitive->GetNode()->GetLocalTransform().m_Rotate);
        }
    }
    else if (GetObjectReference() == pRoomMarker || GetObjectReference() == pBoundMarker) {
        BSMultiBound* pBound = nullptr;
        if (GetObjectReference() == pRoomMarker) {
            BSMultiBoundRoom* pRoom = GetExtra()->GetRoom();
            if (!pRoom) {
                return;
            }
            pBound = pRoom->QMultiBound();
        }
        else {
            pBound = GetExtra()->GetMultiBound();
        }
        if (pBound) {
            BSMultiBoundShape* pShape = pBound->QShape();
            if (pShape) {
                NiPoint3 kRadii = pPrimitive->GetRadii();
                NiPoint3 kCenter = pPrimitive->GetNode()->GetWorldTranslate();
                NiPoint3 kAngle = GetAngleOnReference();
                if (kAngle == NiPoint3::ZERO) {
                    if (pShape->QType() == BSMultiBoundShape::BS_SHAPE_AABB) {
                        static_cast<BSMultiBoundAABB*>(pShape)->SetHalfExtents(kRadii);
                        pShape->SetCenter(kCenter);
                    }
                    else {
                        NiPoint3 kRotation = GetAngleOnReference();
                        pBound->SetShape(pPrimitive->MakeMultiBoundShape(kRotation));
                    }
                }
                else if (pShape->QType() == BSMultiBoundShape::BS_SHAPE_AABB) {
                    pBound->SetShape(pPrimitive->MakeMultiBoundShape(kAngle));
                }
                else {
                    static_cast<BSMultiBoundAABB*>(pShape)->SetHalfExtents(kRadii);
                    pShape->SetCenter(kCenter);
                    const NiTransform& rLocal = pPrimitive->GetNode()->GetLocalTransform();
                    static_cast<BSMultiBoundOBB*>(pShape)->SetAxis(rLocal.m_Rotate);
                }
            }
        }
    }
    NiUpdateData kUpdateData(0.0f, false, false);
    BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->Update(kUpdateData);
}

AudioMarker* TESObjectREFR::GetExtraAudioMarker() {
    return GetExtra()->GetAudioMarkerData();
}

LockData* TESObjectREFR::GetMyLock() {
    return GetExtra()->GetLock();
}

TESForm* TESObjectREFR::GetEmittanceSource() {
    return GetExtra()->GetEmittanceSource();
}

TESObjectREFR* TESObjectREFR::GetLinkedRef() {
    return GetExtra()->GetLinkedRef();
}

TESObjectREFR* TESObjectREFR::GetEnableStateParent() {
    return GetExtra()->GetEnableStateParent();
}

bool TESObjectREFR::GetEnableStateInverse() {
    return GetExtra()->IsEnableStateInverse();
}

BSSimpleList<TESObjectREFR*>* TESObjectREFR::GetEnableStateChildList() {
    return GetExtra()->GetEnableStateChildren();
}

LightRefData* TESObjectREFR::GetLight() {
    return GetExtra()->GetLight();
}

// Gives the reference a new light record holding pLight at full dimmer and stores it in the extra data.
void TESObjectREFR::SetLight(NiLight* pLight) {
    LightRefData* pLightData = new LightRefData;
    pLightData->spLight = pLight;
    pLightData->fWantDimmer = 1.0f;
    GetExtra()->SetLightPtr(pLightData);
}

bool TESObjectREFR::GetAction(OBJECT_ACTION type) {
    return GetExtra()->GetAction(type);
}

bool TESObjectREFR::IsActivatingChildren() {
    return GetExtra()->GetActivateChildrenTimer() > 0.0;
}

TESWorldSpace* TESObjectREFR::GetPackageStartLocationWorld() {
    WORLD_LOCATION* pLocation = GetExtra()->GetPackageStartLocation();
    if (pLocation != nullptr &&
        pLocation->pLocationForm != nullptr &&
        pLocation->pLocationForm->GetFormType() == WRLD_ID) {
        return static_cast<TESWorldSpace*>(pLocation->pLocationForm);
    }
    return nullptr;
}

TESObjectCELL* TESObjectREFR::GetPackageStartLocationInteriorCell() {
    WORLD_LOCATION* pLocation = GetExtra()->GetPackageStartLocation();
    if (pLocation != nullptr &&
        pLocation->pLocationForm != nullptr &&
        pLocation->pLocationForm->GetFormType() == CELL_ID) {
        return static_cast<TESObjectCELL*>(pLocation->pLocationForm);
    }
    return nullptr;
}

const NiPoint3& TESObjectREFR::GetPackageStartLocationCoord() {
    WORLD_LOCATION* pLocation = GetExtra()->GetPackageStartLocation();
    if (pLocation) {
        return pLocation->locPt;
    }
    return GetLocationOnReference();
}

TESObjectREFR* TESObjectREFR::GetRadioStationPositionRef() {
    TESObjectREFR* pRef = nullptr;
    RADIO_DATA* pRadio = GetExtra()->GetRadioData();
    if (pRadio != nullptr && pRadio->eRangeType == 0) {
        pRef = pRadio->pPositionRef;
    }
    return pRef;
}

float TESObjectREFR::GetRadioStationRadius() {
    float fRadius = 0.0f;
    RADIO_DATA* pRadio = GetExtra()->GetRadioData();
    if (pRadio != nullptr && pRadio->eRangeType == 0) {
        fRadius = pRadio->fRadius;
    }
    return fRadius;
}

RADIO_RANGE_TYPE TESObjectREFR::GetRadioStationRangeType() {
    uint32_t eRangeType = 0;
    RADIO_DATA* pRadio = GetExtra()->GetRadioData();
    if (pRadio != nullptr) {
        eRangeType = pRadio->eRangeType;
    }
    return static_cast<RADIO_RANGE_TYPE>(eRangeType);
}

const char* TESObjectREFR::GetFullName() {
    return TESFullName::GetFullName(data.pObjectReference);
}

// The prompt kind, chosen by the base object's form type and, for actors, by their state.
int32_t TESObjectREFR::GetRefPromptType() {
    if (!IsActor() && GetDestroyed()) {
        return 0;
    }
    switch (GetObjectReference()->GetFormType()) {
        case PWAT_ID: return 14;
        case BOOK_ID: return 6;
        case ARMO_ID:
        case CLOT_ID:
        case INGR_ID:
        case MISC_ID:
        case WEAP_ID:
        case AMMO_ID:
        case KEYM_ID:
        case ALCH_ID:
        case NOTE_ID:
        case COBJ_ID:
        case LVLI_ID:
        case PGRE_ID:
        case IMOD_ID:
        case CHIP_ID:
        case CCRD_ID:
        case CMNY_ID: return 1;
        case CONT_ID: return 2;
        case DOOR_ID: return BSstrlen(GetFullName()) != 0 ? 8 : 0;
        case FLOR_ID: return 1;
        case ACTI_ID: return TESForm::GetNameLen(data.pObjectReference) != 0 ? 4 : 0;
        case FURN_ID:
            if (static_cast<TESFurniture*>(data.pObjectReference)->CanSitOn()) {
                return 3;
            }
            if (static_cast<TESFurniture*>(data.pObjectReference)->CanSleepOn()) {
                return 5;
            }
            return 0;
        case TACT_ID:
        case NPC__ID: {
            Actor* pActor = nullptr;
            if (IsActor()) {
                pActor = static_cast<Actor*>(this);
            }
            if (pActor == pPlayer) {
                return 0;
            }
            if (IsDead(false) && pActor->GetLifeState() != ACTOR_LIFE_STATE_ESSENTIAL_DOWN) {
                return 2;
            }
            if (pActor && pActor->IsSleeping() && pPlayer->QCanWakeSleepingActor()) {
                return 11;
            }
            if (pActor && pActor->GetEssential()) {
                return 10;
            }
            if (pPlayer->IsSneaking() && GetObjectReference()->GetFormType() != TACT_ID) {
                return 1;
            }
            return 7;
        }
        case CREA_ID: {
            Actor* pCreature = nullptr;
            if (IsActor()) {
                pCreature = static_cast<Actor*>(this);
            }
            if (!pCreature) {
                return 0;
            }
            if (IsDead(false) &&
                (!pCreature->IsPlayerTeammate() || pCreature->GetLifeState() != ACTOR_LIFE_STATE_ESSENTIAL_DOWN)) {
                return 2;
            }
            // The base is offset to its TESActorBaseData without a null test.
            TESActorBaseData& rBaseData = *pCreature->GetActorBase();
            if (!rBaseData.GetAllowPCDialogue()) {
                return 0;
            }
            if (pCreature->GetEssential()) {
                return 10;
            }
            if (pPlayer->IsSneaking()) {
                return 1;
            }
            return 7;
        }
        case LIGH_ID: return static_cast<TESObjectLIGH*>(data.pObjectReference)->GetCanCarry() ? 1 : 4;
        case TERM_ID: return 4;
        default: return 0;
    }
}

const char* TESObjectREFR::GetModel() {
    TESModel* pModel = GetTESModel();
    if (pModel != nullptr) {
        return pModel->GetModel();
    }
    return nullptr;
}

bool TESObjectREFR::IsMarker() {
    TESBoundObject* pBase = data.pObjectReference;
    if (pBase != nullptr) {
        return pBase->IsMarker();
    }
    return false;
}

// Compares the float bound size with a double 700.0; unordered counts as not larger.
bool TESObjectREFR::Is3DHighPriority() {
    bool bResult = false;
    if (GetVisibleDistant()) {
        bResult = true;
    }
    else if (!IsActor() && !IsMarker() && GetObjectReference() && GetObjectReference()->GetBoundSize() > 700.0) {
        bResult = true;
    }

    return bResult;
}

// Allocates the special render extra only when enabling invisibility, and removes an empty flags extra.
void TESObjectREFR::SetIsInvisibleLODMultType(bool abIsInvisibleLODMultType) {
    auto* pExtra = static_cast<ExtraSpecialRenderFlags*>(GetExtraData(EXTRA_SPECIAL_RENDER_FLAGS));
    if (abIsInvisibleLODMultType && !pExtra) {
        ExtraSpecialRenderFlags* pNewExtra = new ExtraSpecialRenderFlags;
        pExtra = static_cast<ExtraSpecialRenderFlags*>(GetExtra()->AddExtra(pNewExtra));
    }
    if (pExtra) {
        uint32_t uiFlags = pExtra->GetFlags();
        if (abIsInvisibleLODMultType) {
            uiFlags |= ExtraSpecialRenderFlags::SRF_INVISIBLE_LOD_MULT_TYPE;
        }
        else {
            uiFlags &= ~ExtraSpecialRenderFlags::SRF_INVISIBLE_LOD_MULT_TYPE;
        }
        pExtra->SetFlags(uiFlags);
        if (!uiFlags) {
            GetExtra()->RemoveExtra(EXTRA_SPECIAL_RENDER_FLAGS);
        }
        TESForm* pForm = this;
        pForm->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
    }
}

bool TESObjectREFR::GetRefPersists() {
    TESFile* pFile = GetFile(-1);
    if (pFile != nullptr && !pDataHandler->GetSaveLoad() && !pDataHandler->GetClearingData() && !pFile->GetMaster()) {
        return true;
    }
    return GetPersistFlagValue();
}

float TESObjectREFR::GetScale() {
    float fScale = fRefScale;
    TESBoundObject* pBase = GetObjectReference();
    if (pBase != nullptr) {
        switch (pBase->GetFormType()) {
            case NPC__ID: fScale = static_cast<TESNPC*>(pBase)->GetHeight() * fScale; break;
            case CREA_ID: fScale = static_cast<TESCreature*>(pBase)->GetBaseScale() * fScale; break;
            default: break;
        }
    }
    return fScale;
}

// The scale is printed with "%.2f" and read back, which keeps two decimals, then clamped to [0.01, 10]; markers other
// than texture sets keep theirs. The scrap buffer is 0x20 bytes, yet 0x200 is passed to BSsprintf.
// The clamps compare against the double constants and store the float ones. An actor's character controller is rebuilt
// at the new scale, keeping its pitch and roll.
void TESObjectREFR::SetScale(float newscale) {
    if (IsMarker() && GetObjectReference() && GetObjectReference()->GetFormType() != TXST_ID) {
        return;
    }
    char* pBuffer = static_cast<char*>(
        MemoryManager::Instance().GetThreadScrapHeap()->Allocate(0x20, ScrapHeap::ALIGNMENT)
    );
    BSsprintf(pBuffer, 0x200, "%.2f", newscale);
    newscale = static_cast<float>(std::atof(pBuffer));
    if (newscale < 0.009999999776482582) {
        newscale = 0.01f;
    }
    if (newscale > 10.0) {
        newscale = 10.0f;
    }
    fRefScale = newscale;
    AddChange(CHANGE_REFR_SCALE);
    newscale = GetScale();
    NiAVObject* p3D;
    if (this == pPlayer) {
        p3D = pPlayer->Get3D(true);
        if (p3D) {
            p3D->SetScale(newscale);
            NiUpdateData kFirstPersonData(0.0f, false, false);
            p3D->Update(kFirstPersonData);
        }
        p3D = pPlayer->Get3D(false);
    }
    else {
        p3D = Get3D();
    }
    if (p3D) {
        p3D->SetScale(newscale);
        NiUpdateData kData(0.0f, false, false);
        p3D->Update(kData);
        if (IsActor()) {
            MobileObject* pMobile = static_cast<MobileObject*>(this);
            bhkCharacterController* pController = static_cast<bhkCharacterController*>(pMobile->GetCharController());
            float fPitchAngle = 0.0f;
            float fRollAngle = 0.0f;
            if (pController) {
                fPitchAngle = pController->QPitchAngle();
                fRollAngle = pController->QRollAngle();
            }
            pMobile->DetachCharController();
            if (pMobile->GetCurrentAIProcess()) {
                BaseProcess* pProcess = pMobile->GetCurrentAIProcess();
                pProcess->SetCharController(NiPointer<bhkCharacterController>(nullptr));
            }
            pMobile->InitHavok();
            pController = static_cast<bhkCharacterController*>(pMobile->GetCharController());
            if (pController) {
                pController->fPitchAngle = fPitchAngle;
                pController->fRollAngle = fRollAngle;
            }
            if (pMobile == pPlayer) {
                pPlayer->ResetShapeCasterInfo();
            }
        }
    }
    MemoryManager::Instance().GetThreadScrapHeap()->Deallocate(pBuffer);
}

float TESObjectREFR::GetModelBoundSize() {
    TESBoundObject* pBase = GetObjectReference();
    if (pBase == nullptr) {
        return 0.0f;
    }
    return pBase->GetBoundSize();
}

float TESObjectREFR::GetDistanceFromPoint(const NiPoint3& aPt) {
    NiPoint3 kDelta = aPt - data.kPosition;
    return kDelta.Length();
}

float TESObjectREFR::GetRadius() {
    if (pLoadedData && pLoadedData->fCachedRadius >= 0.0) {
        return pLoadedData->fCachedRadius;
    }
    float fVal = 0.0f;
    if (!GetObjectReference()) {
        return fVal;
    }
    switch (GetObjectReference()->GetFormType()) {
        case LIGH_ID: {
            TESObjectLIGH* plight = static_cast<TESObjectLIGH*>(GetObjectReference());
            // The light's radius is unsigned.
            fVal = static_cast<float>(plight->GetRadius());
            fVal = GetExtra()->GetRadius() + fVal;
            break;
        }
        case STAT_ID:
            if (GetObjectReference() == pRadiationMarker) {
                fVal = GetExtra()->GetRadius();
            }
            else if (GetObjectReference() == pMapMarker) {
                fVal = GetExtra()->GetRadius();
                if (FloatEQ(fVal, 0.0f, 0.001f)) {
                    fVal = static_cast<float>(static_cast<int32_t>(iMapMarkerRevealDistance.UInt()));
                }
            }
            else if (GetObjectReference() == pAudioMarker) {
                fVal = GetExtra()->GetRadius();
                if (FloatEQ(fVal, 0.0f, 0.001f)) {
                    fVal = 5000.0f;
                }
            }
            break;
    }
    if (pLoadedData) {
        pLoadedData->fCachedRadius = fVal;
    }
    return fVal;
}

float TESObjectREFR::GetDistanceFromReference(TESObjectREFR* apRef, bool abDisabled, bool bSameSpace) {
    // the highest representable float: out of reach
    float fResult = std::numeric_limits<float>::max();
    bool bInInteriors = false;
    if (!apRef) {
        return fResult;
    }
    if ((apRef->GetDisabled() && !abDisabled) || apRef->GetDelete()) {
        return fResult;
    }
    bool bSameCell = true;
    if (!bSameSpace) {
        TESObjectCELL* pTargetParentCell = apRef->GetParentCell();
        TESObjectCELL* pParentCell = GetParentCell();
        if (pTargetParentCell && pParentCell && (pTargetParentCell->GetInterior() || pParentCell->GetInterior())) {
            if (pTargetParentCell != pParentCell) {
                bSameCell = false;
            }
            bInInteriors = true;
        }
        if (bSameCell && !bInInteriors) {
            // The target's cell is paired with this reference's worldspace and the other way round.
            TESWorldSpace* pTargetWorldspace = pTargetParentCell ? pTargetParentCell->GetWorldSpace() : GetWorldSpace();
            TESWorldSpace* pWorldspace = pParentCell ? pParentCell->GetWorldSpace() : apRef->GetWorldSpace();
            if (pTargetWorldspace != pWorldspace || !pTargetWorldspace) {
                bSameCell = false;
            }
        }
    }
    if (!bSameCell && !bSameSpace) {
        return fResult;
    }
    fResult = GetDistanceFromPoint(apRef->GetLocationOnReference());
    return fResult;
}

bool TESObjectREFR::HasLeveledCreatureCalced() {
    TESBoundObject* pBase = GetObjectReference();
    if (pBase != nullptr && pBase->IsActorBase()) {
        return GetExtra()->HasLevCreaCalced();
    }
    return false;
}

bool TESObjectREFR::GetIsImposter() {
    ExtraSpecialRenderFlags* pFlags = static_cast<ExtraSpecialRenderFlags*>(GetExtraData(EXTRA_SPECIAL_RENDER_FLAGS));
    return pFlags != nullptr && (pFlags->m_flags & ExtraSpecialRenderFlags::SRF_IMPOSTER) != 0;
}

bool TESObjectREFR::GetIsStingerLOD() {
    ExtraSpecialRenderFlags* pFlags = static_cast<ExtraSpecialRenderFlags*>(GetExtraData(EXTRA_SPECIAL_RENDER_FLAGS));
    return pFlags != nullptr && (pFlags->m_flags & ExtraSpecialRenderFlags::SRF_STINGER_LOD) != 0;
}

bool TESObjectREFR::GetIsInvisibleLODMultType() {
    ExtraSpecialRenderFlags* pFlags = static_cast<ExtraSpecialRenderFlags*>(GetExtraData(EXTRA_SPECIAL_RENDER_FLAGS));
    return pFlags != nullptr && (pFlags->m_flags & ExtraSpecialRenderFlags::SRF_INVISIBLE_LOD_MULT_TYPE) != 0;
}

BSMultiBoundRoom* TESObjectREFR::GetMultiBoundRoom() {
    if (!IsMultiBoundRef()) {
        return nullptr;
    }
    if (GetDelete()) {
        return nullptr;
    }
    if (!GetExtra()->GetRoomIsMaster()) {
        return nullptr;
    }
    BSMultiBoundRoom* pRoom = GetExtra()->GetRoom();
    if (!pRoom) {
        pRoom = MultiBoundMarkerData::CreateRoomForRef(this);
        GetExtra()->SetRoom(pRoom);
        RoomRefData* pRoomData = GetExtra()->GetRoomLinkedRefData();
        if (pRoomData) {
            for (TESObjectREFR* pRef : pRoomData->kRoomList.ItemsUntilNull()) {
                pRef->GetExtra()->AddRoomLinkedRoomRef(this);
                GetExtra()->AddRoomLinkedRoomRef(pRef);
            }
        }
    }
    return pRoom;
}

NiMatrix3 TESObjectREFR::GetOrientation() {
    NiMatrix3 kRot;
    // Actors turn only about z.
    if (IsActor()) {
        kRot.FromEulerAnglesXYZ(0.0f, 0.0f, data.kRotation.z);
    }
    else {
        kRot.FromEulerAnglesXYZ(data.kRotation.x, data.kRotation.y, data.kRotation.z);
    }
    return static_cast<const NiMatrix3&>(kRot);
}

bool TESObjectREFR::IsMultiBoundRef() {
    return GetObjectReference() == pBoundMarker || GetObjectReference() == pRoomMarker;
}

TESObjectREFR* TESObjectREFR::GetMultiBoundRef() {
    return TESObjectREFR::GetExtra()->GetMultiBoundRef();
}

MultiBoundMarkerData* TESObjectREFR::GetMultiBoundData() {
    MultiBoundMarkerData* pData = nullptr;
    if (IsMultiBoundRef()) {
        pData = GetExtra()->GetMultiBoundData();
    }
    return pData;
}

const NiPoint3& TESObjectREFR::GetMultiBoundHalfExtent() {
    MultiBoundMarkerData* pdata = GetMultiBoundData();
    BGSPrimitive* pprim = GetExtra()->GetPrimitive();
    if (pprim) {
        if (pdata) {
            // Copies the radii before it fetches the half extents.
            NiPoint3 kRadii = pprim->GetRadii();
            if (pdata->GetHalfExtents() != kRadii) {
                pdata->SetHalfExtents(pprim->GetRadii());
            }
        }
        return pprim->QRadii();
    }
    if (pdata) {
        return pdata->GetHalfExtents();
    }
    return MultiBoundMarkerData::GetDefaultHalfExtents();
}

bool TESObjectREFR::QBoundInsideMultibound(BSMultiBound* apBound) {
    NiNode* pNode = Get3D();
    if (!pNode->IsVisualObject()) {
        return false;
    }
    if (!pNode) {
        return false;
    }
    BSMultiBoundAABB* pShape = static_cast<BSMultiBoundAABB*>(apBound->QShape());
    return pShape->CheckBound(pNode->GetWorldBound()) == BSMultiBoundShape::BS_INTERSECT_CONTAINSTARGET;
}

bool TESObjectREFR::QCenterInsideMultibound(BSMultiBound* apBound) {
    NiPoint3 kLocation = GetLocationOnReference();
    return apBound->QPointWithin(kLocation);
}

bool TESObjectREFR::UpdateMultiboundShapeTransformsRecursive(NiAVObject* apObject) {
    bool bfoundmbnode = false;
    if (!apObject) {
        return bfoundmbnode;
    }
    if (NiObject::IsKindOf_0(&BSMultiBoundNode::ms_RTTI, apObject)) {
        NiAVObject* pchild;
        NiPoint3* pHalfExtents;
        float z;
        float x;
        float y;
        float ang;
        BSMultiBoundNode* pmbnode = static_cast<BSMultiBoundNode*>(apObject);
        apObject->GetWorldTransform().m_Rotate.ExtractAngleAndAxis(ang, x, y, z);
        bool brotated = std::fabs(ang) > 1e-05;
        BSMultiBound* pbound = pmbnode->QMultiBound();
        BSMultiBoundShape* pshape = pbound->QShape();
        BSMultiBoundAABB* paabb = dynamic_cast<BSMultiBoundAABB*>(pshape);
        BSMultiBoundOBB* pobb = dynamic_cast<BSMultiBoundOBB*>(pshape);
        if (paabb) {
            pHalfExtents = &paabb->HalfExtents;
        }
        else if (pobb) {
            pHalfExtents = &pobb->HalfExtents;
        }
        else {
            pHalfExtents = &NiPoint3::ZERO;
        }
        NiPoint3 extents = *pHalfExtents;
        if (brotated) {
            if (ang < 0.0) {
                ang = ang + 6.2831854820251465;
            }
            float fFrom90 = ang - 1.5707963705062866;
            float fFrom270 = ang - 4.71238899230957;
            if (std::fabs(fFrom90) < 1e-05 || std::fabs(fFrom270) < 9.999999747378752e-06) {
                float f = extents.x;
                extents.x = extents.y;
                extents.y = f;
                brotated = false;
            }
            float fFrom360 = ang - 6.2831854820251465;
            if (std::fabs(fFrom360) < 0.0001) {
                brotated = false;
            }
        }
        if (brotated) {
            paabb = nullptr;
            pobb = new BSMultiBoundOBB;
            pobb->SetAxis(pmbnode->GetWorldTransform().m_Rotate);
            pbound->SetShape(pobb);
            pshape = pobb;
        }
        NiPoint3 kCenter = pmbnode->GetWorldTranslate();
        if (pmbnode->GetChildCount() == 1) {
            pchild = pmbnode->GetAt(0);
        }
        else {
            pchild = nullptr;
        }
        if (pchild) {
            kCenter = pchild->GetWorldBound().m_kCenter;
        }
        pshape->SetCenter(kCenter);
        float fScale = pmbnode->m_kWorld.m_fScale;
        extents *= fScale;
        if (paabb) {
            paabb->SetHalfExtents(extents);
        }
        if (pobb) {
            pobb->SetHalfExtents(extents);
        }
        pmbnode->UpdateWorldBound();
        bfoundmbnode = true;
    }
    NiNode* pnode = apObject->IsNode();
    if (pnode) {
        for (int32_t i = 0; i < pnode->GetChildCount(); ++i) {
            NiAVObject* pchild = pnode->GetAt(i);
            if (UpdateMultiboundShapeTransformsRecursive(pchild)) {
                return true;
            }
        }
    }
    return bfoundmbnode;
}

void TESObjectREFR::PreCalcTriStripsShapeAabb(NiAVObject* ap3D) {
    NiTransform niwtrans;
    if (ap3D) {
        bhkBvTreeShape* pbhktreeshape;
        bhkRigidBody* pbody;
        NiUpdateData updatedata(0.0f, true, false);
        ap3D->UpdateWorldData(updatedata);
        niwtrans = ap3D->GetWorldTransform();
        hkTransform kTransform;
        kTransform.setIdentity();
        NI2HK(kTransform, niwtrans);
        bhkCollisionObject* pcol = bhkCollisionObject::GetbhkCollisionObject(ap3D);
        pbody = pcol ? pcol->GetRigidBody() : nullptr;
        if (pbody) {
            pbhktreeshape = static_cast<bhkBvTreeShape*>(pbody->GetShape());
        }
        else {
            pbhktreeshape = nullptr;
        }
        if (pbhktreeshape) {
            hkNiTriStripsShape* pstripshape = nullptr;
            hkPackedNiTriStripsShape* ppackedStripShape = nullptr;
            if (NiObject::IsKindOf_0(&bhkNiTriStripsShape::ms_RTTI, pbhktreeshape)) {
                pstripshape = static_cast<hkNiTriStripsShape*>(pbhktreeshape->Gethk());
            }
            else if (NiObject::IsKindOf_0(&bhkPackedNiTriStripsShape::ms_RTTI, pbhktreeshape)) {
                ppackedStripShape = static_cast<hkPackedNiTriStripsShape*>(pbhktreeshape->Gethk());
            }
            else if (NiObject::IsKindOf_0(&bhkBvTreeShape::ms_RTTI, pbhktreeshape)) {
                bhkShapeCollection* pbhkshapecol = pbhktreeshape->GetShapeCollection();
                if (pbhkshapecol && NiObject::IsKindOf_0(&bhkNiTriStripsShape::ms_RTTI, pbhkshapecol)) {
                    pstripshape = static_cast<hkNiTriStripsShape*>(pbhkshapecol->Gethk());
                }
                else if (pbhkshapecol && NiObject::IsKindOf_0(&bhkPackedNiTriStripsShape::ms_RTTI, pbhkshapecol)) {
                    ppackedStripShape = static_cast<hkPackedNiTriStripsShape*>(pbhkshapecol->Gethk());
                }
            }
            if (pstripshape) {
                pstripshape->CalcWorldAabb(kTransform);
            }
            if (ppackedStripShape) {
                ppackedStripShape->CalcWorldAabb(kTransform);
            }
        }
        NiNode* pnode = ap3D->IsNode();
        if (pnode) {
            uint32_t icount = pnode->GetArrayCount();
            for (uint32_t i = 0; i < icount; ++i) {
                this->PreCalcTriStripsShapeAabb(pnode->GetAt(i));
            }
        }
    }
}

void TESObjectREFR::SetMultiBoundRef(TESObjectREFR* apRef) {
    if ((!apRef || apRef->IsMultiBoundRef()) && this != apRef) {
        TESObjectREFR::GetExtra()->SetMultiBoundRef(apRef);
    }
}

TESObjectREFR* TESObjectREFR::GetXTarget() {
    if (GetObjectReference() != pXMarker && GetObjectReference() != pXMarkerHeading) {
        return nullptr;
    }
    return GetExtra()->GetXTarget();
}

LockData* TESObjectREFR::GetLock() {
    LockData* pLock = GetMyLock();
    if (pLock == nullptr) {
        DoorTeleportData* pTeleport = GetTeleport();
        if (pTeleport != nullptr && pTeleport->pLinkedDoor != nullptr) {
            pLock = pTeleport->pLinkedDoor->GetMyLock();
        }
    }
    return pLock;
}

int32_t TESObjectREFR::GetLockLevel() {
    LockData* pLock = GetLock();
    if (pLock) {
        return pLock->GetLevel(this);
    }
    return 0;
}

ExtraLock* TESObjectREFR::GetExtraLock() {
    ExtraLock* pLock = static_cast<ExtraLock*>(GetExtraData(EXTRA_LOCK));
    if (pLock == nullptr) {
        DoorTeleportData* pTeleport = GetTeleport();
        if (pTeleport != nullptr && pTeleport->pLinkedDoor != nullptr) {
            pLock = pTeleport->pLinkedDoor->GetExtraLock();
        }
    }
    return pLock;
}

void TESObjectREFR::Lock() {
    ExtraLock* pLock = GetExtraLock();
    if (pLock != nullptr) {
        pLock->LockDoor();
        AddLockChange();
    }
}

void TESObjectREFR::UnLock() {
    ExtraLock* pLock = GetExtraLock();
    if (pLock != nullptr) {
        pLock->UnLockDoor();
        AddLockChange();
    }
}

TESForm* TESObjectREFR::GetSpace() {
    if (pParentCell != nullptr) {
        TESWorldSpace* pWorldSpace = pParentCell->GetWorldSpace();
        if (pWorldSpace != nullptr) {
            return pWorldSpace;
        }
        return pParentCell;
    }
    TESObjectCELL* pPersistentCell = GetExtra()->GetPersistentCell();
    return pPersistentCell != nullptr ? pPersistentCell->GetWorldSpace() : nullptr;
}

TESWorldSpace* TESObjectREFR::GetWorldSpace() {
    TESWorldSpace* pWorldSpace = nullptr;
    TESObjectCELL* pCell = GetParentCell();
    if (pCell == nullptr) {
        pCell = GetSaveParentCell();
    }
    if (pCell != nullptr) {
        pWorldSpace = pCell->GetWorldSpace();
    }
    return pWorldSpace;
}

MapMarkerData* TESObjectREFR::GetMapMarkerData() {
    return TESObjectREFR::GetExtra()->GetMapMarkerData();
}

bool TESObjectREFR::GetInterior() {
    bool bInterior = true;
    TESObjectCELL* pCell = GetParentCell();
    if (pCell != nullptr) {
        bInterior = pCell->GetInterior();
    }
    else {
        pCell = GetSaveParentCell();
        if (pCell != nullptr && pCell->GetWorldSpace() != nullptr) {
            bInterior = false;
        }
    }
    return bInterior;
}

BGSEncounterZone* TESObjectREFR::GetEncounterZone() {
    BGSEncounterZone* pZone = GetExtra()->GetEncounterZone();
    if (pZone == nullptr && GetParentCell() != nullptr) {
        pZone = GetParentCell()->GetEncounterZone();
    }
    if (pZone == nullptr && GetWorldSpace() != nullptr) {
        pZone = GetWorldSpace()->GetEncounterZone();
    }
    return pZone;
}

void TESObjectREFR::SetEncounterZone(BGSEncounterZone* apZone) {
    GetExtra()->SetEncounterZone(apZone);
    AddChange(CHANGE_REFR_EXTRA_ENCOUNTER_ZONE);
}

void TESObjectREFR::CopyExtraDataList(ExtraDataList* apCopy) {
    GetExtra()->CopyList(apCopy);
}

void TESObjectREFR::SetActionRef(TESObjectREFR* apRefIn) {
    GetExtra()->SetActionRef(apRefIn);
}

void TESObjectREFR::SetAction(OBJECT_ACTION type) {
    if (type == OBJECT_ACTION_OPEN) {
        if (GetOriginalOpenByDefaultState()) {
            RemoveChange(CHANGE_OBJECT_OPEN_STATE);
        }
        else {
            AddChange(CHANGE_OBJECT_OPEN_STATE);
        }
    }
    GetExtra()->SetAction(type);
}

void TESObjectREFR::ClearAction(OBJECT_ACTION type) {
    if (type == OBJECT_ACTION_OPEN) {
        if (GetOriginalOpenByDefaultState()) {
            AddChange(CHANGE_OBJECT_OPEN_STATE);
        }
        else {
            RemoveChange(CHANGE_OBJECT_OPEN_STATE);
        }
    }
    GetExtra()->ClearAction(type);
}

void TESObjectREFR::SetLocationOnReferenceSimple(const NiPoint3& arPoint) {
    data.kPosition = arPoint;
}

// Unlike SetAngleOnReferenceZ, the angle is not clamped for actors.
void TESObjectREFR::SetAngleOnReferenceY(float afy) {
    data.kRotation.y = afy;
    AddChange(CHANGE_REFR_MOVE);
}

void TESObjectREFR::SetAngleOnReferenceZ(float afz) {
    data.kRotation.z = afz;
    if (IsActor()) {
        data.kRotation.z = ClampAngle(data.kRotation.z);
    }
    AddChange(CHANGE_REFR_MOVE);
}

void TESObjectREFR::SetAngleOnReference(NiPoint3 aPoint) {
    data.kRotation = aPoint;
    if (IsActor()) {
        data.kRotation.z = ClampAngle(data.kRotation.z);
    }
    AddChange(CHANGE_REFR_MOVE);
}

void TESObjectREFR::SetObjectReference(TESBoundObject* apObject) {
    data.pObjectReference = apObject;
    SetDestructible(BGSDestructibleObjectForm::IsDestructible(apObject));
    if (IsMobileObject()) {
        SetRunsInLow(static_cast<TESActorBase*>(apObject)->HasLowLevelProcessing());
    }
}

void TESObjectREFR::SetLoadedDataFlag(uint32_t aiFlag, bool abValue) {
    if (pLoadedData != nullptr) {
        pLoadedData->SetFlag(aiFlag, abValue);
    }
}

bool TESObjectREFR::GetLoadedDataFlag(uint32_t aiFlag) {
    if (pLoadedData == nullptr) {
        return false;
    }
    return pLoadedData->GetFlag(aiFlag);
}

bool TESObjectREFR::QUnderwater() {
    return pLoadedData && static_cast<int32_t>(pLoadedData->iUnderwaterCount) > 0;
}

void TESObjectREFR::SetUnderwater(bool abVal) {
    if (pLoadedData) {
        pLoadedData->iUnderwaterCount += abVal ? 1 : -1;
        if ((pLoadedData->iUnderwaterCount & 0x80000000u) != 0) {
            pLoadedData->iUnderwaterCount = 0;
        }
        pLoadedData->pCurrentWaterObject = nullptr;
        uint32_t iUnderwaterCount = pLoadedData->iUnderwaterCount;
        if (iUnderwaterCount) {
            if (iUnderwaterCount == 1) {
                BSSimpleList<TESObjectREFR*>* preflist;
                if (GetParentCell()) {
                    preflist = GetParentCell()->GetWaterRefList();
                }
                else {
                    preflist = nullptr;
                }
                for (TESObjectREFR* pref : preflist->ItemsUntilNull()) {
                    BGSPlaceableWater* pwater = dynamic_cast<BGSPlaceableWater*>(pref->GetObjectReference());
                    if (pwater->IsAutomaticWater()) {
                        pLoadedData->pCurrentWaterObject = pref;
                        break;
                    }
                }
                if (!IsActor() && GetObjectReference()->GetFormType() != DOOR_ID) {
                    SetInPlaceableWater(true);
                }
            }
        }
        else {
            TESObjectREFR* CastingType;
            ExtraWaterZoneMap* pmap = static_cast<ExtraWaterZoneMap*>(m_Extra.GetExtraData(126));
            if (pmap) {
                CastingType = pmap->pHighestWaterZone->pRef;
            }
            else {
                CastingType = nullptr;
            }
            pLoadedData->pCurrentWaterObject = CastingType;
            if (!IsActor() && GetObjectReference()->GetFormType() != DOOR_ID) {
                SetInPlaceableWater(false);
            }
        }
    }
}

int32_t TESObjectREFR::GetInventoryObjectCount(TESBoundObject* apObject) {
    int32_t iCount = 0;
    InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(this);
    if (pChanges != nullptr) {
        iCount = pChanges->GetObjectCount(apObject);
    }
    return iCount;
}

bool TESObjectREFR::HasQuestObject() {
    bool bResult = false;
    if (HasContainer() != nullptr) {
        bResult = InventoryChanges::GetInventoryChanges(this)->HasQuestObject();
    }
    return bResult;
}

bool TESObjectREFR::HasScript() {
    bool bResult = false;
    if (TESScriptableForm::GetFormScript(GetObjectReference()) != nullptr) {
        bResult = true;
    }
    else if (HasContainer() != nullptr) {
        bResult = InventoryChanges::GetInventoryChanges(this)->HasScript();
    }
    return bResult;
}

bool TESObjectREFR::WearingObject(TESBoundObject* apObject) {
    bool bResult = false;
    if (HasContainer() != nullptr &&
        InventoryChanges::GetInventoryChanges(this)->WearingObject(apObject, false) != nullptr) {
        bResult = true;
    }

    return bResult;
}

ItemChange* TESObjectREFR::GetInventoryItem(TESBoundObject* apObject, uint32_t aId) {
    ItemChange* pItem = nullptr;
    InventoryChanges* pChanges = GetExtra()->GetContainerChanges();
    if (pChanges != nullptr) {
        pItem = pChanges->GetInventoryItem(apObject, aId);
    }
    return pItem;
}

// Containers, NPCs and creatures have a container component.
TESContainer* TESObjectREFR::HasContainer() {
    TESContainer* pContainer = nullptr;
    if (!GetObjectReference()) {
        return pContainer;
    }
    int32_t eType = GetObjectReference()->GetFormType();
    if (eType == CONT_ID) {
        pContainer = static_cast<TESObjectCONT*>(GetObjectReference());
    }
    else if (eType > AMMO_ID && eType <= CREA_ID) {
        pContainer = static_cast<TESActorBase*>(GetObjectReference());
    }
    return pContainer;
}

TESModel* TESObjectREFR::GetTESModel() {
    TESModel* pmodel = GetExtra()->GetModelSwap();
    if (!pmodel && GetObjectReference()) {
        if (GetObjectReference()->GetFormType() == WEAP_ID) {
            TESObjectWEAP* pweap = static_cast<TESObjectWEAP*>(GetObjectReference());
            if (GetExtra()->GetWeaponModFlags()) {
                pmodel = pweap->GetModTESModel(GetExtra()->GetWeaponModFlags(), false);
            }
            else {
                pmodel = pweap->GetWorldTESModel();
            }
        }
        else {
            pmodel = dynamic_cast<TESModel*>(GetObjectReference());
        }
    }
    if (!pmodel) {
        TESBipedModelForm* pbipedModel = TESBipedModelForm::GetFormAsBipedModel(GetObjectReference());
        if (pbipedModel) {
            TESActorBase::SEX esex = TESActorBase::SEX_MALE;
            TESForm* powner = GetOwner();
            if (powner && powner->GetFormType() == NPC__ID) {
                esex = static_cast<TESActorBase::SEX>(static_cast<TESActorBase*>(powner)->GetSex());
            }
            pmodel = pbipedModel->GetWorldTESModel(esex);
        }
    }
    return pmodel;
}

// The reference's owner, else its teleport door's, else (not for furniture, doors or activators) its encounter zone's,
// else its cell's.
TESForm* TESObjectREFR::GetOwner() {
    TESForm* pOwnership = GetMyOwner();
    if (IsActor()) {
        return pOwnership;
    }
    if (!pOwnership) {
        DoorTeleportData* pTeleport = GetTeleport();
        if (pTeleport && pTeleport->pLinkedDoor) {
            pOwnership = pTeleport->pLinkedDoor->GetMyOwner();
        }
    }
    if (pOwnership || IsFurniture() || !GetObjectReference()) {
        return pOwnership;
    }
    if (GetObjectReference()->GetFormType() == DOOR_ID) {
        return pOwnership;
    }
    if (GetObjectReference()->GetFormType() == ACTI_ID) {
        return pOwnership;
    }
    BGSEncounterZone* pEncounterZone = GetEncounterZone();
    if (pEncounterZone && pEncounterZone != BGSEncounterZone::GetNoZone()) {
        pOwnership = pEncounterZone->QOwnerForm();
    }
    if (pOwnership || !GetParentCell()) {
        return pOwnership;
    }
    return GetParentCell()->GetOwner();
}

bool TESObjectREFR::GetOriginalOpenByDefaultState() {
    BGSChangeFlags kFlags(CHANGE_OBJECT_OPEN_DEFAULT_STATE);
    if (pSaveLoadGame->GetChange(this, kFlags)) {
        return !GetExtra()->GetAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
    }
    return GetExtra()->GetAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
}

void TESObjectREFR::AddLockChange() {
    if (GetMyLock()) {
        AddChange(CHANGE_OBJECT_EXTRA_LOCK);
    }
    else {
        DoorTeleportData* pTeleport = GetTeleport();
        if (pTeleport && pTeleport->GetLinkedDoor() && pTeleport->GetLinkedDoor()->GetMyLock()) {
            pTeleport->GetLinkedDoor()->AddChange(CHANGE_OBJECT_EXTRA_LOCK);
        }
    }
}

void TESObjectREFR::Release3DRelatedData() {
    if (Get3D()) {
        if (GetObjectReference() && GetObjectReference()->GetFormType() == PWAT_ID) {
            TES::pInstance->DecrementPlaceableWaterCount();
        }

        TESObjectREFR::RemoveRefFromMultiboundTestMap(this);
        RemoveFromAllWater(false);
        for (TESObjectREFR* plrref : m_Extra.GetRoomLinkedRoomRefList()->ItemsUntilNull()) {
            if (GetMultiBoundRoom() && plrref->GetMultiBoundRoom()) {
                NiPointer<BSMultiBound> spMultiBound = GetMultiBoundRoom()->QMultiBound();
                plrref->GetMultiBoundRoom()->QJoinedMultiBoundList().Remove(spMultiBound);
            }
        }
        if (static_cast<TESObjectSTAT*>(GetObjectReference()) == pPortalMarker) {
            m_Extra.GetPortal();
            if (static_cast<TESObjectSTAT*>(GetObjectReference()) == pPortalMarker) {
                TESObjectREFR* plrref = m_Extra.GetPortalLinkedRoomRef(0);
                if (plrref) {
                    plrref->m_Extra.RemoveRoomLinkedPortalRef(this);
                }
                plrref = m_Extra.GetPortalLinkedRoomRef(1);
                if (plrref) {
                    plrref->m_Extra.RemoveRoomLinkedPortalRef(this);
                }
            }
        }
        if (GetObjectReference() && GetObjectReference()->GetFormType() == LIGH_ID) {
            BSSimpleList<TESObjectREFR*>* preflist = m_Extra.GetLitWaterRefs();
            LightRefData* prefrlight = GetLight();
            while (preflist && preflist->m_item) {
                preflist->m_item->m_Extra.SetWaterLightRef(this, false);
                if (prefrlight && prefrlight->spLight.m_pObject) {
                    TESObjectREFR* pWaterRef = preflist->m_item;
                    if (pWaterRef->Get3D() && TES::pInstance->GetWaterSystem()) {
                        pWaterRef = preflist->m_item;
                        NiTriBasedGeom* pgeom = static_cast<NiTriBasedGeom*>(
                            TES::pInstance->GetWaterSystem()->GetWaterGeometry(pWaterRef)
                        );
                        BSShader* pshader = pgeom->GetShader();
                        if (pshader && pshader->iShaderType == BSShaderManager::BSSM_SHADERTYPE_WATER) {
                            WaterShaderProperty* pwaterprop = static_cast<WaterShaderProperty*>(pgeom->GetProperty(3));
                            pwaterprop->LightList.Remove(prefrlight->spLight.m_pObject);
                        }
                    }
                }

                preflist = preflist->m_pkNext;
            }
        }

        if (GetObjectReference() && GetObjectReference()->GetFormType() == ASPC_ID && GetParentCell()) {
            GetParentCell()->GetbhkWorld()->pAcousticSpaceListener->BGSAcousticSpaceListener::DequeueRemoval(
                this,
                true
            );
        }

        if (pParentCell) {
            TESWaterListener* plistener;
            if (GetObjectReference() && GetObjectReference()->GetFormType() == PWAT_ID) {
                TES::pInstance->GetWaterSystem()->RemoveWaterRefFromWaterSystem(this);
            }

            pParentCell->RemoveAnimatedRef(this);
            pParentCell->RemoveEmittanceRef(this);
            if (pParentCell && pParentCell->GetbhkWorld()) {
                plistener = pParentCell->GetbhkWorld()->pWaterListener;
            }
            else {
                plistener = nullptr;
            }
            if (plistener) {
                plistener->HandleQueuedRemovals(this, false, true);
            }
            if (GetObjectReference() && GetObjectReference()->GetFormType() == PWAT_ID) {
                pParentCell->RemoveWaterRef(this);
                plistener->RemoveWaterZoneForReference(this);
            }

            BSPortalGraph* pgraph = pParentCell->GetPortalGraph();
            if (pgraph && GetObjectReference()) {
                if (GetObjectReference() == pRoomMarker) {
                    BSMultiBoundRoom* proom = m_Extra.GetRoom();
                    if (proom) {
                        pgraph->RemoveRoom(proom);
                    }
                }
                else if (static_cast<TESObjectSTAT*>(GetObjectReference()) == pPlaneMarker) {
                    BSOcclusionPlane* pplane = m_Extra.GetOcclusionPlane();
                    if (pplane) {
                        if (m_Extra.GetMultiBoundRef()) {
                            BSMultiBoundRoom* proom = m_Extra.GetMultiBoundRef()->m_Extra.GetRoom();
                            if (proom) {
                                proom->RemoveOccluder(pplane);
                            }
                            else {
                                pgraph->RemoveOccluder(pplane);
                            }
                        }
                        else {
                            pgraph->RemoveOccluder(pplane);
                        }
                    }
                }
                else if (static_cast<TESObjectSTAT*>(GetObjectReference()) == pPortalMarker) {
                    BSPortal* pportal = m_Extra.GetPortal();
                    if (pportal) {
                        for (int32_t i = 0; i < 2; ++i) {
                            PortalLinkedRefData* pdata = m_Extra.GetPortalLinkedRefData();
                            if (pdata && pdata->pLinkedRefs[i]) {
                                if (pdata->pLinkedRefs[i]->GetMultiBoundRoom()) {
                                    (&pdata->pLinkedRefs[i]->GetMultiBoundRoom()->QPortalList())->Remove(pportal);
                                    if (i) {
                                        pportal->SetMultiBoundRoomRight(nullptr);
                                    }
                                    else {
                                        pportal->SetMultiBoundRoomLeft(nullptr);
                                    }
                                }
                            }
                            else {
                                pgraph->RemovePortal(pportal);
                            }
                        }
                    }
                }
            }
        }
    }
}

void TESObjectREFR::FreeUpAllMarkers() {
    uint32_t i;
    if (TESObjectREFR::GetObjectReference()) {
        if (TESObjectREFR::GetObjectReference()->GetFormType() == FURN_ID && TESObjectREFR::GetObjectReference()) {
            for (i = 0; i < 30; ++i) {
                SetMarkerUsed(i, false);
                SetMarkerReserved(i, false);
            }
        }
    }
}

bool TESObjectREFR::IsPlayingAnimation() {
    if (m_Extra.GetLastFinishedSequence()) {
        return true;
    }
    if (!pLoadedData) {
        return false;
    }
    if (!pLoadedData->m_spData3D.m_pObject) {
        return false;
    }
    NiControllerManager* pManager = static_cast<NiControllerManager*>(
        NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pLoadedData->m_spData3D->GetControllers())
    );
    if (!pManager || !pManager->GetActive()) {
        return false;
    }
    uint32_t uiSequenceCount = pManager->GetSequenceCount();
    for (uint32_t uiIndex = 0; uiIndex < uiSequenceCount; ++uiIndex) {
        BSAnimGroupSequence* pSequence = static_cast<BSAnimGroupSequence*>(pManager->GetSequenceAt(uiIndex));
        if (pSequence->GetState()) {
            const char* pName = animGroupInfo[0].pName;
            const char* pSequenceName = pSequence->m_kName.GetHandleString();
            if (BSstricmp(pSequenceName, pName)) {
                pName = animGroupInfo[2].pName;
                pSequenceName = pSequence->m_kName.GetHandleString();
                if (BSstricmp(pSequenceName, pName)) {
                    return true;
                }
            }
        }
    }
    return false;
}

NiNode* TESObjectREFR::Get3D_Body() {
    return TESObjectREFR::Get3D();
}

// Queues the 3D of an enabled reference that has none (or only a temporary one), except markers other than lights; a
// disabled one only gets its OnLoad action. Loaded 3D is updated and its havok restored.
void TESObjectREFR::UpdateReference3D() {
    bool bMarker = false;
    if (IsMarker() && GetObjectReference() && GetObjectReference()->GetFormType() != LIGH_ID) {
        bMarker = true;
    }
    if ((!TESObjectREFR::Get3D() || HasTemp3D()) &&
        !pModelLoader->IsReferenceQueued(this) &&
        !GetDelete() &&
        !bMarker) {
        if (!GetDisabled()) {
            if (IsActor()) {
                Actor* pActor = static_cast<Actor*>(this);
                if (pActor) {
                    pActor->MovetoHigh();
                    ReferenceProcessLists.RemoveActorFromTempChangeList(pActor);
                }
            }
            IO_TASK_PRIORITY ePriority = TES::pInstance->GetCellPriority(GetParentCell(), nullptr);
            pModelLoader->QueueReference(this, ePriority, true);
        }
        else if (!pSaveLoadGame->GetSaveGameLoading()) {
            Script::InitActionList(this, GetExtra());
            Script::SetActionFlag(this, GetExtra(), SCRIPT_ACTION_ONLOAD);
        }
    }
    if (TESObjectREFR::Get3D()) {
        NiUpdateData kUpdateData(0.0f, false, false);
        TESObjectREFR::Get3D()->Update(kUpdateData);
        bool bIsActor = IsActor();
        if (bIsActor) {
            static_cast<Actor*>(this)->CreateDismemberedLimbs();
            GetExtra()->RestoreSavedHavokData(this);
        }
        InitHavok();
        if (bIsActor) {
            static_cast<Actor*>(this)->ApplyCriticalStage();
            static_cast<Actor*>(this)->UpdateDismemberedLimbVel();
        }
        else {
            GetExtra()->RestoreSavedHavokData(this);
        }
    }
    else if (GetExtra()->GetPrimitive()) {
        InitHavok();
    }
}

// Attaches apWeapon to the reference's 3D: to the player's first- and third-person bipeds (with its mod slots and the
// gun scope), to another reference's biped, or as an add-on node; then refreshes the actor's weapon state.
void TESObjectREFR::AttachWeapon(TESObjectWEAP* apWeapon) {
    if (!TESObjectREFR::Get3D()) {
        return;
    }
    if (std::strstr(GetFullName(), "Lily")) {
        Warning("HEYAOOO\n");
    }
    bool bIronSights = false;
    bool iModSlots = false;
    float fValue = 0.0f;
    Actor* pActor = nullptr;
    if (IsActor()) {
        pActor = static_cast<Actor*>(this);
    }
    if (!pSaveLoadGame->GetSaveGameLoading() && pActor->GetCurrentAIProcess()->GetWeaponDrawn()) {
        pActor->GetCurrentAIProcess()->SetWeaponDrawn(pActor, false);
    }

    BipedAnim* pBipedAnim;
    if (this == pPlayer) {
        pBipedAnim = pPlayer->GetBiped(true);
    }
    else {
        pBipedAnim = GetBiped();
    }
    if (this == pPlayer) {
        ItemChange* pWeapon = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon();
        if (pWeapon) {
            iModSlots = pWeapon->GetModSlots();
            bIronSights = pWeapon->HasModEffectActive(WEAPON_MOD_IRON_SITES, fValue);
        }
        if (pBipedAnim) {
            pBipedAnim->AttachBipedWeapon(apWeapon, iModSlots);
        }
        pBipedAnim = pPlayer->GetBiped(false);
        if (apWeapon->HasScope() && this == pPlayer && (!apWeapon->HasModScope() || bIronSights)) {
            Interface::InitGunScope(apWeapon->GetScopeModel());
        }
    }
    else if (GetAnimation() && GetAnimation()->GetControllerManager()) {
        uint16_t usGroup = TESAnimGroup::AnimGroup(
            AGM_NONE,
            weaponToAnimGroupWeapon[apWeapon->GetWeaponType()],
            ANIM_GROUP_WEAPONS_START,
            false
        );
        if (!GetAnimation()->GroupLoaded(usGroup) && !IsDead(true)) {
            BSSimpleList<const char*>* pFileList = pModelLoader->BuildKFFileList(
                GetModel(),
                true,
                false,
                weaponToAnimGroupWeapon[apWeapon->GetWeaponType()]
            );
            GetAnimation()->AddAnimationsFromList(pFileList);
        }
    }
    if (pBipedAnim) {
        pBipedAnim->AttachBipedWeapon(apWeapon, iModSlots);
    }
    else {
        TESModelTextureSwap* pSwap = apWeapon;
        if (pSwap) {
            BipedAnim::LoadAndAttachAddOn(apWeapon, pSwap, BO_WEAPON, this, nullptr);
            if (!pSaveLoadGame->GetSaveGameLoading()) {
                bool bDrawn = pActor->IsWeaponDrawn();
                if (!bDrawn && apWeapon->IsEmbeddedWeapon()) {
                    bDrawn = true;
                }
                BaseProcess* pProcess = pActor->GetCurrentAIProcess();
                Animation* pAnimation = pActor->GetAnimation();
                pProcess->ForceWeaponDrawnSheathed(bDrawn, pBipedAnim, pAnimation, pActor);
            }
        }
    }
    if (pActor) {
        if (!pBipedAnim) {
            pActor->ReloadTargets(true);
        }
        pActor->InitLightingPropertyPtr(TESObjectREFR::Get3D());
        pActor->UpdateAlpha();
        pActor->UpdateWeaponConditionEffects(true);
        BGSBipedModelList* pModelList = BGSBipedModelList::GetFormAsBipedModelList(apWeapon);
        if (pModelList && pModelList->GetBipedModelList()) {
            pActor->GetCurrentAIProcess()->Set3DUpdateFlag(RESET_MODEL);
        }
    }
}

bool TESObjectREFR::IsCrimeToActivate() {
    bool bCrime = false;
    LockData* pLock = GetLock();
    if (pPlayer->IsDead(false)) {
        return false;
    }
    if (GetObjectReference()->GetFormType() == DOOR_ID) {
        if (GetObjectReference() && !IsAnOwner(pPlayer, true) && GetOwner()) {
            if (TESObjectDOOR::CanActorIgnoreLock(this, pPlayer, false, false)) {
                return false;
            }
            if (pLock && pLock->IsLocked()) {
                bCrime = true;
            }
            else {
                DoorTeleportData* pTeleport = GetTeleport();
                TESForm* pOwner = nullptr;
                bool bIsEvil = false;
                if (pTeleport) {
                    TESObjectCELL* pTeleportCell = pTeleport->GetTeleportCell();
                    if (pTeleportCell) {
                        pOwner = pTeleportCell->GetOwner();
                        if (pOwner && pOwner->GetFormType() == FACT_ID) {
                            bIsEvil = static_cast<TESFaction*>(pOwner)->TESFaction::GetIsEvil();
                        }
                        else if (
                            pOwner &&
                            pOwner->GetFormType() == NPC__ID &&
                            static_cast<TESNPC*>(pOwner)->TESActorBaseData::IsInEvilFactionsOnly()
                        ) {
                            bIsEvil = true;
                        }
                    }
                    bCrime = pTeleportCell &&
                        pOwner &&
                        pTeleportCell->GetInterior() &&
                        !pTeleportCell->GetPublicState() &&
                        !bIsEvil;
                }
            }
        }
        else {
            bool bIsEvil = false;
            DoorTeleportData* pTeleport = GetTeleport();
            if (pTeleport) {
                TESObjectCELL* pTeleportCell = pTeleport->GetTeleportCell();
                TESForm* pOwner = nullptr;
                if (pTeleportCell) {
                    pOwner = pTeleportCell->GetOwner();
                    if (pOwner && pOwner->GetFormType() == FACT_ID) {
                        bIsEvil = static_cast<TESFaction*>(pOwner)->TESFaction::GetIsEvil();
                    }
                    else if (
                        pOwner &&
                        pOwner->GetFormType() == NPC__ID &&
                        static_cast<TESNPC*>(pOwner)->TESActorBaseData::IsInEvilFactionsOnly()
                    ) {
                        bIsEvil = true;
                    }
                }
                bCrime = pTeleportCell &&
                    pOwner &&
                    pTeleportCell->GetInterior() &&
                    !pTeleportCell->GetPublicState() &&
                    !bIsEvil &&
                    !pTeleportCell->IsActorCellOwner(pPlayer);
            }
            else {
                bCrime = false;
            }
        }
    }
    else if (GetObjectReference()->GetFormType() == FURN_ID) {
        bCrime = static_cast<TESFurniture*>(GetObjectReference())->TESFurniture::CanSleepOn() &&
            GetOwner() &&
            !IsAnOwner(pPlayer, true);
    }
    else if (GetObjectReference()->GetFormType() == NPC__ID && pPlayer->IsSneaking() && !IsDead(false)) {
        bCrime = true;
    }
    else if (GetObjectReference()->GetFormType() == CREA_ID && pPlayer->IsSneaking()) {
        TESCreature* pCreature = dynamic_cast<TESCreature*>(GetObjectReference());
        if (pCreature && pCreature->GetAllowPCDialogue() && !IsDead(false)) {
            bCrime = true;
        }
    }
    if (!bCrime && GetObjectReference()->GetFormType() != CREA_ID && GetOwner()) {
        return !IsAnOwner(pPlayer, true);
    }

    return bCrime;
}

void TESObjectREFR::AttachAmmo(TESAmmo* apAmmo) {}

void TESObjectREFR::RemoveAmmo() {}

// Takes the worn weapon's 3D off the reference: through the biped when it has one (both of the player's, leaving
// the iron sights, VATS playback and the gun scope), otherwise by dropping the backpack and releasing the weapon
// model when a weapon node holds it. The weapon bones are then emptied, and an actor's targets and lighting
// refreshed.
void TESObjectREFR::RemoveWeapon() {
    if (!TESObjectREFR::Get3D()) {
        return;
    }
    TESBoundObject* pWeapon = nullptr;
    InventoryChanges* pChanges = GetExtra()->GetContainerChanges();
    if (pChanges) {
        ItemChange* pWeaponItem = pChanges->GetWornItem(BO_WEAPON, false);
        if (pWeaponItem) {
            pWeapon = pWeaponItem->GetContainerObject();
            delete pWeaponItem;
        }
    }
    BipedAnim* pAnim;
    if (this == pPlayer) {
        if (pPlayer->GetIronSights() && pPlayer->GetTempPerson()) {
            pPlayer->SetIronSights(false, false, false);
        }
        pAnim = pPlayer->GetBiped(true);
    }
    else {
        pAnim = GetBiped();
    }
    Actor* pActor = nullptr;
    if (IsActor()) {
        pActor = static_cast<Actor*>(this);
    }
    if (this == pPlayer) {
        pAnim = pPlayer->GetBiped(true);
        pVATS.QuitVATSPlayback(false, true);
        if (pAnim) {
            pAnim->RemoveBipedWeapon();
        }
        pAnim = pPlayer->GetBiped(false);
        if (pPlayer->GetIronSights()) {
            Interface::SetGunScopeVisible(false);
        }
        Interface::ClearGunScope();
    }
    // GetCurrentAIProcess is called on pActor even when the reference is not an actor.
    BaseProcess* pCurrProc = pActor->GetCurrentAIProcess();
    if (pAnim) {
        FalloutAudio::LockAcousticSpace(true);
        pAnim->RemoveBipedWeapon();
        FalloutAudio::LockAcousticSpace(false);
    }
    else {
        bool bWeaponNode = false;
        NiAVObject* pBackpack = BSUtilities::GetObjectByName(Get3D(), "Backpack");
        if (pBackpack) {
            pBackpack->GetParent()->DetachChild(pBackpack);
        }
        NiNode* pBone = nullptr;
        if (pCurrProc) {
            pBone = pCurrProc->GetRightWeaponBone(nullptr);
            bWeaponNode = pBone && pBone->GetChildCount();
            if (!bWeaponNode) {
                // a bow hangs off the left weapon bone
                pBone = pCurrProc->GetLeftWeaponBone(nullptr);
                bool bBow = false;
                if (pBone) {
                    for (uint32_t i = 0; i < pBone->GetArrayCount(); ++i) {
                        NiAVObject* pChild = pBone->GetAt(i);
                        if (pChild) {
                            const char* pName;
                            pName = pChild->GetName();
                            if (pName) {
                                pName = pChild->GetName();
                                if (!BSstrcmp(pName, "Bow")) {
                                    bBow = true;
                                    break;
                                }
                            }
                        }
                    }
                }
                if (bBow) {
                    bWeaponNode = pBone && pBone->GetChildCount();
                }
            }
            if (!bWeaponNode) {
                NiNode* p3D = Get3D();
                if (p3D) {
                    NiAVObject* pWeaponNode = BSUtilities::GetObjectByName(p3D, "Weapon");
                    if (pWeaponNode) {
                        bWeaponNode = true;
                        pBone = static_cast<NiNode*>(pWeaponNode);
                    }
                }
            }
        }
        if (pWeapon) {
            TESModelTextureSwap* pModel = static_cast<TESObjectWEAP*>(pWeapon);
            if (pModel && bWeaponNode) {
                pModelLoader->ReleaseFile(pModel->GetModel());
            }
        }
    }
    if (pCurrProc) {
        NiNode* pBone = pCurrProc->GetRightWeaponBone(nullptr);
        if (pBone) {
            BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->RemoveObject(pBone);
            pBone->DetachAllChildren();
        }
        if (IsCreature()) {
            NiNode* p3D = Get3D();
            if (p3D) {
                NiNode* pWeaponNode = static_cast<NiNode*>(BSUtilities::GetObjectByName(p3D, "Weapon"));
                if (pWeaponNode) {
                    BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->RemoveObject(pWeaponNode);
                    pWeaponNode->DetachAllChildren();
                }
            }
        }
        pBone = pCurrProc->GetLeftWeaponBone(nullptr);
        if (pBone) {
            bool bBow = false;
            for (uint32_t i = 0; i < pBone->GetArrayCount(); ++i) {
                NiAVObject* pChild = pBone->GetAt(i);
                if (pChild) {
                    const char* pName;
                    pName = pChild->GetName();
                    if (pName) {
                        pName = pChild->GetName();
                        if (!BSstrcmp(pName, "Bow")) {
                            bBow = true;
                            break;
                        }
                    }
                }
            }
            if (bBow) {
                BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->RemoveObject(pBone);
                pBone->DetachAllChildren();
            }
        }
    }
    if (pActor) {
        pCurrProc = pActor->GetCurrentAIProcess();
        if (!pAnim && pCurrProc && !pDataHandler->GetClearingData() && !pSaveLoadGameOLD->GetLoading()) {
            pActor->ReloadTargets(false);
        }
        pActor->InitLightingPropertyPtr(TESObjectREFR::Get3D());
        if (pActor->GetCurrentAIProcess()) {
            BGSBipedModelList* pModelList = BGSBipedModelList::GetFormAsBipedModelList(pWeapon);
            if (pModelList && pModelList->GetBipedModelList()) {
                pActor->GetCurrentAIProcess()->Set3DUpdateFlag(RESET_MODEL);
            }
        }
    }
}

bool TESObjectREFR::IsAnOwner(Actor* pOwner, bool busefaction) {
    bool bretval = false;
    TESForm* pownership = nullptr;
    TESGlobal* pglobal = nullptr;
    if (GetOwner()) {
        pownership = GetOwner();
        GetOwnershipRank();
        pglobal = GetOwnershipGlobal();
    }
    if (pownership && pOwner) {
        TESBoundObject* pbase = pOwner->GetTemplateActorBase();
        if (!pbase) {
            pbase = pOwner->GetObjectReference();
        }
        TESActorBase* pactorbase = nullptr;
        switch (pbase->GetFormType()) {
            case NPC__ID:
            case CREA_ID: pactorbase = dynamic_cast<TESActorBase*>(pbase); break;
        }
        if (pownership == pbase) {
            bretval = true;
        }
        else {
            TESFaction* pfac = nullptr;
            if (pownership && pownership->GetFormType() == FACT_ID) {
                pfac = static_cast<TESFaction*>(pownership);
            }
            if (pfac && pfac->GetAllowSell()) {
                busefaction = true;
            }
            if (pfac && busefaction) {
                if (pfac && pactorbase && pOwner->IsInFaction(pfac)) {
                    bretval = true;
                }
            }
            else if (!pglobal || pglobal->GetValue() == 0.0) {
                // This store is skipped when the value is nonzero or NaN.
                bretval = false;
            }
        }
    }
    else if (!pownership) {
        bretval = true;
    }
    return bretval;
}

// The reserved-marker extra is read through ExtraUsedMarkers::GetMarkerUsed as well.
int32_t TESObjectREFR::GetFirstFreeMarkerIndex(bool abIgnoreReservations) {
    int32_t iResult = -1;
    ExtraUsedMarkers* pUsedMarkers = static_cast<ExtraUsedMarkers*>(GetExtra()->GetExtraData(EXTRA_USEDMARKERS));
    ExtraUsedMarkers* pReservedMarkers = static_cast<ExtraUsedMarkers*>(
        GetExtra()->GetExtraData(EXTRA_RESERVEDMARKERS)
    );
    if (!GetObjectReference()) {
        return iResult;
    }
    if (GetObjectReference()->GetFormType() == FURN_ID) {
        TESFurniture* pFurniture = static_cast<TESFurniture*>(GetObjectReference());
        if (abIgnoreReservations) {
            for (uint32_t i = 0; i < 30; ++i) {
                if (pFurniture->GetMarkerEnabled(i) && (!pUsedMarkers || !pUsedMarkers->GetMarkerUsed(i))) {
                    iResult = i;
                    break;
                }
            }
        }
        else {
            for (uint32_t i = 0; i < 30; ++i) {
                if (pFurniture->GetMarkerEnabled(i) &&
                    (!pUsedMarkers || !pUsedMarkers->GetMarkerUsed(i)) &&
                    (!pReservedMarkers || !pReservedMarkers->GetMarkerUsed(i))) {
                    iResult = i;
                    break;
                }
            }
        }
    }
    else {
        if (abIgnoreReservations) {
            return 0;
        }
        if (!pUsedMarkers && !pReservedMarkers) {
            return 0;
        }
        uint32_t uiCount = 1;
        for (uint32_t i = 0; i < uiCount; ++i) {
            if ((!pUsedMarkers || !pUsedMarkers->GetMarkerUsed(i)) &&
                (!pReservedMarkers || !pReservedMarkers->GetMarkerUsed(i))) {
                iResult = i;
                break;
            }
        }
    }
    return iResult;
}

bool TESObjectREFR::HasFreeMarker(bool abIgnoreReservations) {
    if (!TESObjectREFR::GetObjectReference()) {
        return false;
    }
    if (TESObjectREFR::GetObjectReference()->TESForm::GetFormType() == FURN_ID || !abIgnoreReservations) {
        return GetFirstFreeMarkerIndex(abIgnoreReservations) != -1;
    }
    return false;
}

// The candidate positions and their marker indices go into arrays sized for every marker; the positions are moved into
// the world by the reference's rotation and position, and the nearest to aLocation wins. The winner's index is kept in
// a byte, 0x7F meaning none.
bool TESObjectREFR::GetClosestFreeMarker(
    const NiPoint3& aLocation,
    bool abSitOK,
    bool abSleepOK,
    FurnitureMark& aOutMarker,
    uint32_t& aiOutMarkerIndex,
    bool abIgnoreReservations
) {
    bool bResult = false;
    if (GetObjectReference() && GetObjectReference()->GetFormType() == FURN_ID) {
        if (Get3D()) {
            TESFurniture* pFurniture = static_cast<TESFurniture*>(GetObjectReference());
            if (pFurniture) {
                BSFurnitureMarker* pMarker = BSFurnitureMarker::Find(Get3D());
                if (pMarker) {
                    uint32_t uiMarks = pMarker->QNumMarks();
                    if (uiMarks) {
                        NiPoint3* pPositions = static_cast<NiPoint3*>(XFNV_ALLOCA16(12 * uiMarks));
                        int32_t* pIndices = static_cast<int32_t*>(XFNV_ALLOCA16(4 * uiMarks));
                        uint32_t uiCount = 0;
                        for (uint32_t i = 0; i < uiMarks; ++i) {
                            if (!pFurniture->GetMarkerEnabled(i) || GetMarkerUsed(i, abIgnoreReservations)) {
                                continue;
                            }
                            int32_t iType = pMarker->QMark(i).GetType();
                            if ((TESFurniture::IsSitMarker(iType) && abSitOK) ||
                                (TESFurniture::IsSleepMarker(iType) && abSleepOK) ||
                                (!TESFurniture::IsSitMarker(iType) && !TESFurniture::IsSleepMarker(iType))) {
                                pPositions[uiCount] = pMarker->QMark(i).QPosition();
                                pIndices[uiCount] = static_cast<int32_t>(i);
                                ++uiCount;
                            }
                        }
                        if (uiCount) {
                            NiPoint3* pWorld = static_cast<NiPoint3*>(XFNV_ALLOCA16(12 * uiMarks));
                            const NiPoint3* pLocation = &GetLocationOnReference();
                            NiMatrix3 kOrientation = GetOrientation();
                            NiMatrix3::TransformVertices(kOrientation, *pLocation, uiCount, pPositions, pWorld);
                            float fClosest = std::numeric_limits<float>::max();
                            uint8_t ucClosest = 0x7F;
                            for (uint32_t j = 0; j < uiCount; ++j) {
                                NiPoint3 kOffset = aLocation - pWorld[j];
                                float fDistance = kOffset.Length();
                                if (fClosest > fDistance) {
                                    fClosest = fDistance;
                                    ucClosest = static_cast<uint8_t>(j);
                                }
                            }
                            if (ucClosest != 0x7F) {
                                aiOutMarkerIndex = pIndices[ucClosest];
                                aOutMarker.SetType(pMarker->QMark(aiOutMarkerIndex).GetType());
                                aOutMarker.SetPosition(pWorld[ucClosest]);
                                // The heading is kept as a double and added to the angle in double precision.
                                double dHeading = pMarker->QMark(aiOutMarkerIndex).GetHeading();
                                float fHeading = static_cast<float>(GetAngleOnReference().z + dHeading);
                                aOutMarker.SetHeading(fHeading);
                                bResult = true;
                            }
                        }
                    }
                }
            }
        }
        else {
            aOutMarker.SetPosition(GetLocationOnReference());
            aOutMarker.SetType(0);
            aOutMarker.SetHeading(0.0f);
            aiOutMarkerIndex = -1;
            bResult = false;
        }
    }
    else if (!abIgnoreReservations) {
        ExtraUsedMarkers* pReserved = static_cast<ExtraUsedMarkers*>(GetExtra()->GetExtraData(EXTRA_RESERVEDMARKERS));
        // This result is stored, then overwritten with true below.
        if (pReserved) {
            bResult = !pReserved->GetMarkerUsed(0);
        }
        aOutMarker.SetPosition(GetLocationOnReference());
        aOutMarker.SetType(0);
        aOutMarker.SetHeading(0.0f);
        aiOutMarkerIndex = -1;
        bResult = true;
    }
    return bResult;
}

// Builds the reference's animation: an NPC or creature gets a new Animation from its KF files; any other 3D with a
// controller manager starts its idle (or open/close) sequences. Actors then re-attach their worn weapon and set the
// weapon's drawn state; a saved animation time is applied last.
void TESObjectREFR::InitAnimation() {
    NiNode* p3D = TESObjectREFR::Get3D();
    NiNode* pNode = p3D ? p3D->IsNode() : nullptr;
    BipedAnim* pBiped = GetBiped();
    BSSimpleList<const char*>* pFileList = nullptr;
    Animation* pAnimation = nullptr;
    SetAnimation(pAnimation);
    if (!pNode) {
        return;
    }
    if (GetDelete()) {
        return;
    }
    TESBoundObject* pObjectReference = data.pObjectReference;
    TESBoundObject* pBase = data.pObjectReference;
    Actor* pActor = IsActor() ? static_cast<Actor*>(this) : nullptr;
    uint32_t eWeaponType = 0;
    if (data.pObjectReference->GetFormType() == NPC__ID || data.pObjectReference->GetFormType() == CREA_ID) {
        const char* pModel = GetModel();
        const char* pFileName = std::strrchr(pModel, '\\');
        if (pFileName && _strnicmp(pFileName + 1, "Skeleton", 8)) {
            const char* pEditorID = data.pObjectReference->GetFormEditorID();
            Warning("MODELS: Actor '%s' needs to have a 'skeleton*.nif' model instead of '%s'.", pEditorID, pModel);
        }
        eWeaponType = pActor->GetCurrentWeaponAnimType();
    }
    uint32_t eFormType = data.pObjectReference->GetFormType();
    if (eFormType == NPC__ID) {
        if (pBiped) {
            pBiped->RemoveAllParts();
            Animation* pNew = new Animation;
            pAnimation = SetAnimation(pNew);
            p3D = TESObjectREFR::Get3D();
            pNode = p3D ? p3D->IsNode() : nullptr;
            if (!pNode) {
                uint32_t uiFormID = pObjectReference->GetFormID();
                const char* pEditorID = pObjectReference->GetFormEditorID();
                Warning("ANIMATION: SetAnimation cleared 3D because %s (%08X) was moved", pEditorID, uiFormID);
                return;
            }
            ReplaceModel();
            pFileList = BuildKFFileList(static_cast<ANIM_GROUP_WEAPON_ENUM>(eWeaponType));
            if (!pAnimation->Init(pFileList, pNode, this, true)) {
                uint32_t uiFormID = pObjectReference->GetFormID();
                const char* pEditorID = pObjectReference->GetFormEditorID();
                Warning(
                    "ANIMATION: Bad InitAnimation for NPC '%s' (%08X). Missing 'Idle' animation.",
                    pEditorID,
                    uiFormID
                );
            }
            TESAnimation* pKFForm = static_cast<TESActorBase*>(pObjectReference);
            if (GetHealth() > 0.0 && pKFForm->HasKFFiles()) {
                char acPath[260];
                BSstrcpy(acPath, 0x104, GetModel());
                char* pSlash = std::strrchr(acPath, '\\');
                if (pSlash) {
                    *pSlash = '\0';
                    pAnimation->AddSpecialAnimations(
                        reinterpret_cast<BSSimpleList<const char*>*>(pKFForm->GetKFFileList()),
                        acPath
                    );
                }
            }
        }
    }
    else if (eFormType == CREA_ID) {
        Animation* pNew = new Animation;
        pAnimation = SetAnimation(pNew);
        p3D = TESObjectREFR::Get3D();
        pNode = p3D ? p3D->IsNode() : nullptr;
        if (!pNode) {
            uint32_t uiFormID = pObjectReference->GetFormID();
            const char* pEditorID = pObjectReference->GetFormEditorID();
            Warning("ANIMATION: SetAnimation cleared 3D because %s (%08X) was moved", pEditorID, uiFormID);
            return;
        }
        ReplaceModel();
        pFileList = BuildKFFileList(static_cast<ANIM_GROUP_WEAPON_ENUM>(eWeaponType));
        if (!pAnimation->Init(pFileList, pNode, this, true)) {
            uint32_t uiFormID = pBase->GetFormID();
            const char* pEditorID = pBase->GetFormEditorID();
            Warning(
                "ANIMATION: Bad InitAnimation for Creature '%s' (%08X). Missing 'Idle' animation.",
                pEditorID,
                uiFormID
            );
        }
        TESAnimation* pKFForm = static_cast<TESActorBase*>(pBase);
        if (GetHealth() > 0.0 && pKFForm->HasKFFiles()) {
            char acPath[260];
            BSstrcpy(acPath, 0x104, GetModel());
            char* pSlash = std::strrchr(acPath, '\\');
            if (pSlash) {
                *pSlash = '\0';
                pAnimation->AddSpecialAnimations(
                    reinterpret_cast<BSSimpleList<const char*>*>(pKFForm->GetKFFileList()),
                    acPath
                );
            }
        }
        static_cast<TESCreature*>(pBase)->CheckEmbeddedWeaponNodes(p3D);
    }
    else {
        if (eFormType == WEAP_ID && static_cast<TESObjectWEAP*>(data.pObjectReference)->IsThrownWeapon()) {
            SetLight(pNode, false);
        }
        if (pNode->GetControllers()) {
            NiControllerManager* pManager = static_cast<NiControllerManager*>(
                NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pNode->GetControllers())
            );
            if (pManager) {
                ForceChange(CHANGE_REFR_ANIMATION);
                if (pManager->GetCumulativeAnimations()) {
                    const char* pModel = GetModel();
                    const char* pFormTypeName = formEnumString[data.pObjectReference->GetFormType()].pFormString;
                    Warning(
                        "ANIMATION: %s '%s' is cumulative. This is not allowed. Re-export to correct this problem.",
                        pFormTypeName,
                        pModel
                    );
                }
                bool bOpenStateSet = false;
                if (BGSOpenCloseForm::IsOpenCloseForm(GetObjectReference())) {
                    bool bOpen = GetAction(OBJECT_ACTION_OPEN_BY_DEFAULT);
                    if (!pSaveLoadGame->GetSaveGameLoading()) {
                        bool bDefaultOpen = GetOriginalOpenByDefaultState();
                        BGSChangeFlags kFlags(CHANGE_OBJECT_OPEN_STATE);
                        bool bChanged = pSaveLoadGame->GetChange(this, kFlags);
                        bOpen = bDefaultOpen != bChanged;
                    }
                    bOpenStateSet = BGSOpenCloseForm::SetOpenState(this, bOpen, true);
                }
                NiFixedString kIdleName(animGroupInfo[0].pName);
                NiControllerSequence* pIdle = pManager->GetSequenceByName(kIdleName);
                NiFixedString kSecondName(animGroupInfo[2].pName);
                NiControllerSequence* pSecond = pManager->GetSequenceByName(kSecondName);
                if (pIdle || pSecond) {
                    float fTime = 0.0f;
                    if (pIdle) {
                        if (pIdle->GetCycleType() != NiControllerSequence::CYCLE_LOOP) {
                            Warning("ANIMATION: Idle animation must be looping '%s'.", GetModel());
                        }
                        else {
                            pManager->ActivateSequence(pIdle, 0, false, 1.0f, 0.0f, nullptr);
                            pIdle->SetOffset(-std::numeric_limits<float>::max());
                            fTime = pIdle->GetBeginKeyTime();
                        }
                    }
                    if (pSecond) {
                        pManager->ActivateSequence(pSecond, 0, false, 1.0f, 0.0f, nullptr);
                        pSecond->SetOffset(-std::numeric_limits<float>::max());
                        fTime = pSecond->GetBeginKeyTime();
                    }
                    NiUpdateData kUpdateData(fTime, true, false);
                    pNode->Update(kUpdateData);
                }
                else {
                    if (!bOpenStateSet) {
                        pManager->SetActive(true);
                        pManager->DeactivateAll(0.0f);
                        NiControllerSequence* pFirst = pManager->GetSequenceAt(0);
                        pManager->ActivateSequence(pFirst, 0, false, 1.0f, 0.0f, nullptr);
                        pFirst->SetOffset(-std::numeric_limits<float>::max());
                        NiUpdateData kFirstData(pFirst->GetBeginKeyTime(), true, false);
                        pNode->Update(kFirstData);
                    }
                    pManager->DeactivateAll(0.0f);
                    pManager->SetActive(false);
                }
            }
            else {
                NiUpdateData kUpdateData(0.0f, false, false);
                pNode->UpdateDownwardPass(kUpdateData, 0);
            }
        }
        else {
            NiUpdateData kUpdateData(0.0f, false, false);
            pNode->UpdateDownwardPass(kUpdateData, 0);
        }
    }
    if (pAnimation) {
        pAnimation->Update(static_cast<Actor*>(this), 0.0f, -1.0f);
    }
    if (pActor) {
        BaseProcess* pProcess = nullptr;
        if (pActor->GetCurrentAIProcess()) {
            // The compare is signed.
            int32_t iLevel = static_cast<int32_t>(pActor->GetCurrentAIProcess()->GetProcessLevel());
            if (iLevel >= PROCESS_TYPE_HIGH && iLevel <= PROCESS_TYPE_MIDDLE_HIGH) {
                pProcess = pActor->GetCurrentAIProcess();
            }
        }
        if (pProcess && this != pPlayer && !pSaveLoadGame->GetSaveGameLoading()) {
            InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(this);
            ItemChange* pWornItem = pChanges ? pChanges->GetWornItem(BO_WEAPON, false) : nullptr;
            TESBoundObject* pWeapon = pWornItem ? pWornItem->GetItemChangeObject() : nullptr;
            ItemChange* pWeaponChange = pChanges ? pChanges->GetWornItem(BO_WEAPON, false) : nullptr;
            pProcess->SetCurrentWeapon(pWeaponChange, Get3D(), false);
            Actor* pSelf = IsActor() ? static_cast<Actor*>(this) : nullptr;
            bool bIsLily = IsActor() && pSelf->IsPlayerTeammate() && std::strstr(pSelf->GetFullName(), "Lily");
            if (IsActor() &&
                (IsNpc() || bIsLily) &&
                pWeapon &&
                (!pProcess->Get3DUpdateFlag(RESET_MODEL) && pWornItem)) {
                if (AILinearTaskThreadManager::ShouldQueue3DTask() &&
                    this != TESObjectREFR::GetBackgroundLoadingRef()) {
                    TESObjectWEAP* pWornWeapon = static_cast<TESObjectWEAP*>(pWornItem->GetItemChangeObject());
                    TaskQueueInterface::GetInstance()->QueueAttachWeapon(static_cast<Actor*>(this), pWornWeapon);
                }
                else if (!bIsLily) {
                    AttachWeapon(static_cast<TESObjectWEAP*>(pWornItem->GetItemChangeObject()));
                }
            }

            if (pAnimation) {
                NiNode* pSceneRoot;
                pSceneRoot = pAnimation->GetSceneRoot();
                if (pSceneRoot) {
                    BaseProcess* pBoneProcess = pActor->GetCurrentAIProcess();
                    NiNode* pRoot;
                    pRoot = pAnimation->GetSceneRoot();
                    pBoneProcess->StoreBonePointers(pAnimation->GetControllerManager(), pRoot);
                }
            }
            if (!pSaveLoadGame->GetSaveGameLoading()) {
                pProcess->GetCurrentPackage();
                if (pActor->GetWantWeaponDrawn()) {
                    pProcess->SetWeaponDrawn(pActor, true);
                }
                if (this == pPlayer && pBiped == pPlayer->GetBiped(true)) {
                    BaseProcess* pDrawProcess = pActor->GetCurrentAIProcess();
                    Animation* pPlayerAnimation = pPlayer->GetAnimation(true);
                    bool bDrawn = pActor->IsWeaponDrawn();
                    pDrawProcess->ForceWeaponDrawnSheathed(bDrawn, pBiped, pPlayerAnimation, pActor);
                }
                else {
                    BaseProcess* pDrawProcess = pActor->GetCurrentAIProcess();
                    Animation* pActorAnimation = pActor->Actor::GetAnimation();
                    bool bDrawn = pActor->IsWeaponDrawn();
                    pDrawProcess->ForceWeaponDrawnSheathed(bDrawn, pBiped, pActorAnimation, pActor);
                }
            }
        }
        if (IsCreature() && pActor && pActor->IsPlayerTeammate() && pSaveLoadGame->GetSaveGameLoading()) {
            BaseProcess* pDrawProcess = pActor->GetCurrentAIProcess();
            Animation* pActorAnimation = pActor->Actor::GetAnimation();
            bool bDrawn = pActor->IsWeaponDrawn();
            pDrawProcess->ForceWeaponDrawnSheathed(bDrawn, pBiped, pActorAnimation, pActor);
        }
        NiTimeController* pController = pNode->GetController(&NiMultiTargetTransformController::ms_RTTI);
        if (pController) {
            pController->SetComputeScaledTime(true);
        }
    }
    ExtraAnimSave* pAnimSave = GetExtra()->GetAnimSave();
    if (pAnimSave && pAnimation) {
        NiUpdateData kSaveData(pAnimSave->data.fTime, false, false);
        TESObjectREFR::Get3D()->Update(kSaveData);
        GetExtra()->RemoveAnimSave();
    }
}

void TESObjectREFR::SetMarkerUsed(uint32_t aiMarkerIndex, bool abVal) {
    if (!GetObjectReference() || GetObjectReference()->GetFormType() != FURN_ID) {
        return;
    }
    AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
    ExtraUsedMarkers* pUsedMarkers = static_cast<ExtraUsedMarkers*>(GetExtra()->GetExtraData(EXTRA_USEDMARKERS));
    if (pUsedMarkers) {
        pUsedMarkers->SetMarkerUsed(aiMarkerIndex, abVal);
        if (!pUsedMarkers->iUsedMarkers) {
            GetExtra()->RemoveExtra(pUsedMarkers, true);
        }
    }
    else if (abVal) {
        pUsedMarkers = new ExtraUsedMarkers;
        GetExtra()->AddExtra(pUsedMarkers);
        pUsedMarkers->SetMarkerUsed(aiMarkerIndex, abVal);
    }
}

void TESObjectREFR::ClearAllDecals() {
    NiNode* p3D = Get3D();
    NiNode* pNode = p3D ? p3D->IsNode() : nullptr;
    if (!pNode) {
        return;
    }
    for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
        NiAVObject* pChild = pNode->GetAt(i);
        if (pChild && NiObject::IsExactKindOf(&BGSDecalNode::ms_RTTI, pChild)) {
            pChild->DetachParent();
            pNode->SetAt(i, nullptr);
        }
    }
}

// Places each decal of the reference's decal refs again, after clearing them, as a permanent simple decal sized from
// its texture set's decal data, or from the diffuse texture's aspect ratio when the set has none. The decal list holds
// REF_DECAL_DATA items. The else branch computes the width and the height separately, the same way.
void TESObjectREFR::ApplyAllDecals(bool /*abSetDecalData*/) {
    ClearAllDecals();
    BSSimpleList<REF_DECAL_DATA*>* pIter = GetExtra()->GetDecalRefs();
    while (pIter && !pIter->IsEmpty()) {
        REF_DECAL_DATA* pRefData = static_cast<REF_DECAL_DATA*>(pIter->GetItem());
        pIter = pIter->GetNext();
        DECAL_CREATION_DATA kData;
        kData.pOrigin = pRefData->Intersect;
        kData.pDirection = pRefData->Normal;
        kData.pAVObj = Get3D();
        kData.pTexSet = static_cast<BGSTextureSet*>(pRefData->pDecalRef->GetObjectReference());
        NiPointer<NiTexture> spTexture(nullptr);
        // The diffuse texture.
        kData.pTexSet->GetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, spTexture);
        NiTexture* pTexture = spTexture;
        if (kData.pTexSet->HasDecalData()) {
            DecalData* pDecalData = kData.pTexSet->GetDecalData();
            // The maximum is read before the minimum, in the game's order.
            float fMaxWidth = pDecalData->GetDecalMaxWidth();
            float fMinWidth = pDecalData->GetDecalMinWidth();
            kData.fDecalWidth = RandomFloat(fMinWidth, fMaxWidth);
            float fMaxHeight = pDecalData->GetDecalMaxHeight();
            float fMinHeight = pDecalData->GetDecalMinHeight();
            kData.fDecalHeight = RandomFloat(fMinHeight, fMaxHeight);
            float fBlue = static_cast<uint8_t>(pDecalData->GetDecalColor() >> 16) / 255.0;
            float fGreen = static_cast<uint8_t>(static_cast<uint16_t>(pDecalData->GetDecalColor()) >> 8) / 255.0;
            float fRed = static_cast<uint8_t>(pDecalData->GetDecalColor()) / 255.0;
            kData.fVertexColor = NiColor(fRed, fGreen, fBlue);
            kData.fDecalDepth = pDecalData->GetDecalDepth();
            kData.fSpecular = pDecalData->GetDecalShininess();
            kData.bAlphaTesting = pDecalData->GetAlphaTesting();
            kData.bAlphaBlending = pDecalData->GetAlphaBlending();
            if (pDecalData->IsParallax()) {
                kData.bUseParallaxOcclusion = true;
                kData.fParallaxOcclusionScale = pDecalData->GetDecalParallaxScale();
                kData.ucParallaxOcclusionMaxPasses = pDecalData->GetDecalParallaxPasses();
            }
        }
        else {
            uint32_t uiHeight = pTexture->GetHeight();
            uint32_t uiWidth = pTexture->GetWidth();
            float fRatio = static_cast<float>(uiHeight / uiWidth);
            float fScale = pRefData->pDecalRef->GetScale();
            kData.fDecalWidth = fRatio * 128.0 * fScale;
            kData.fDecalHeight = fRatio * 128.0 * fScale;
            kData.fDecalDepth = 48.0f;
        }
        kData.fDecalEpsilon = 90.0f;
        kData.cSubTex = -1;
        kData.bUseOcclusionQuery = false;
        GetParentCell()->AddDecal(kData, BGSDecalManager::DECAL_TYPE_PERMENANT_SIMPLE, false);
    }
}

// For an NPC, its base rebuilds the biped's models; for a creature with 3D, each model of the creature's model list (in
// the folder of the reference's model) is loaded, cloned at the reference's scale and attached to the skeleton. Other
// references' base objects replace their own model.
bool TESObjectREFR::ReplaceModel() {
    TESNPC* pNPC = static_cast<TESNPC*>(data.pObjectReference);
    TESCreature* pCreature = static_cast<TESCreature*>(data.pObjectReference);
    BipedAnim* pBiped = GetBiped();
    bool bReplaced = false;
    Animation* pAnimation = GetAnimation();
    if (pAnimation) {
        pAnimation->ZeroGlobalTransform();
    }
    // Calls Get3D directly here.
    NiNode* p3D = TESObjectREFR::Get3D();
    switch (data.pObjectReference->GetFormType()) {
        case NPC__ID:
            if (pBiped) {
                pNPC->ReplaceRefModel(this);
            }
            bReplaced = true;
            break;
        case CREA_ID:
            if (p3D) {
                pCreature->InitWorn(this);
                BSSimpleList<const char*>* pIter = pCreature->GetNIFFileList();
                NiNode* pSkeleton = p3D;
                while (pIter && !pIter->IsEmpty()) {
                    char modelPath[260];
                    BSstrcpy(modelPath, 260, GetModel());
                    char* pFileName = std::strrchr(modelPath, '\\');
                    BSstrcpy(pFileName + 1, 260 - (pFileName - modelPath) - 1, pIter->GetItem());
                    NiNode* pModel = pModelLoader->LoadFile(modelPath, LOD_MULT_ACTORS, true, false, false, false);
                    if (pModel) {
                        alignas(BSCloningProcess) uint8_t kCloningStorage[sizeof(BSCloningProcess)];
                        BSCloningProcess& kCloning = *::new (kCloningStorage) BSCloningProcess(GetScale());
                        {
                            NiPointer<NiObject> spClone(nullptr);
                            NiNode* pClone;
                            if (HasMorpherController(pModel)) {
                                spClone = TES::pInstance->CreateDeepCopySameTextures(pModel, kCloning);
                                pClone = NiSmartPointerCast<NiNode>(spClone);
                            }
                            else {
                                pClone = static_cast<NiNode*>(pModel->Clone(kCloning));
                            }
                            bool bSkinned = Animation::FindSkinnedNode(pClone);
                            NiAVObject* pBip01 = GetActorBone(p3D, FixedStrings::QBip01());
                            if (bSkinned && pBip01) {
                                NiFixedString kName("SkinAttachment");
                                NiAVObject* pAttachment = pBip01->GetObjectByName(kName);
                                if (pAttachment) {
                                    static_cast<NiNode*>(pAttachment)->AttachChild(pClone, true);
                                }
                                else {
                                    pBip01->GetParent()->AttachChild(pClone, true);
                                }
                                BipedAnim::AttachToSkeleton(pSkeleton, pClone, nullptr, true);
                            }
                            else {
                                BipedAnim::AttachToParent(pSkeleton, pClone, nullptr, nullptr, BO_NONE, nullptr);
                            }
                        }
                        kCloning.~BSCloningProcess();
                    }
                    pIter = pIter->GetNext();
                }
            }
            bReplaced = true;
            break;
        default: break;
    }
    if (!bReplaced) {
        bReplaced = data.pObjectReference->ReplaceModel();
    }
    return bReplaced;
}

void TESObjectREFR::SetMarkerReserved(uint32_t aiMarkerIndex, bool abVal) {
    AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
    ExtraUsedMarkers* pextra = static_cast<ExtraUsedMarkers*>(m_Extra.GetExtraData(130));
    if (pextra) {
        pextra->SetMarkerUsed(aiMarkerIndex, abVal);
        if (!pextra->iUsedMarkers) {
            m_Extra.RemoveExtra(pextra, true);
        }
    }
    else if (abVal) {
        BSExtraData* pNewExtra;
        pNewExtra = new ExtraReservedMarkers;
        m_Extra.AddExtra(pNewExtra);
        static_cast<ExtraUsedMarkers*>(pNewExtra)->ExtraUsedMarkers::SetMarkerUsed(aiMarkerIndex, abVal);
    }
}

// Marker aiIndex of the furniture's 3D, in world space: its position rotated and moved by the reference, its heading
// turned by the reference's z angle. The heading sum is done in double and rounded to float.
bool TESObjectREFR::GetMarkerAtIndex(int32_t aiIndex, FurnitureMark& aOutMarker) {
    bool bFound = false;
    if (GetObjectReference() && GetObjectReference()->GetFormType() == FURN_ID && Get3D() && GetObjectReference()) {
        BSFurnitureMarker* pMarkers = BSFurnitureMarker::Find(Get3D());
        if (pMarkers && pMarkers->QNumMarks() > static_cast<uint32_t>(aiIndex)) {
            bFound = true;
            aOutMarker = pMarkers->QMark(aiIndex);
            NiPoint3 kPosition = aOutMarker.QPosition();
            const NiPoint3* pLocation = &GetLocationOnReference();
            NiMatrix3 kOrientation = GetOrientation();
            NiMatrix3::TransformVertices(kOrientation, *pLocation, 1, &kPosition, &aOutMarker.Position);
            double dHeading = aOutMarker.GetHeading();
            float fHeading = static_cast<float>(GetAngleOnReference().z + dHeading);
            aOutMarker.SetHeading(fHeading);
        }
    }
    return bFound;
}

void TESObjectREFR::SetSayToTopic(TESTopic* apTopic) {
    GetExtra()->SetExtraSayToTopicOnExtra(apTopic);
    AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
}

void TESObjectREFR::SetSayToTopicInfo(TESTopicInfo* apInfo) {
    GetExtra()->SetExtraSayToInfoExtra(apInfo);
    AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
}

void TESObjectREFR::SetSayToResponseID(uint32_t anResponseID) {
    GetExtra()->SetExtraSayToResponseIDOnExtra(anResponseID);
    AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
}

// Whether the reference must persist: references to the hardcoded markers (form IDs 4-6, 0x10, 0x12, 0x15, 0x1F,
// 0x23-0x24, 0x34 and 0x3B), texture sets and placeable water always; radio talking activators, random or teleport
// doors, and actors with low-level processing too. Passes 0 as IsRadioStation's aiData and tests only the low byte of
// HasLowLevelProcessing's result.
bool TESObjectREFR::MustRefPersist() {
    TESBoundObject* pBase = GetObjectReference();
    if (!pBase) {
        return false;
    }
    switch (pBase->GetFormID()) {
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x10:
        case 0x12:
        case 0x15:
        case 0x1F:
        case 0x23:
        case 0x24:
        case 0x34:
        case 0x3B: return true;
        default: break;
    }
    switch (pBase->GetFormType()) {
        case TXST_ID: return true;
        case PWAT_ID: return true;
        case TACT_ID: return BGSTalkingActivator::IsRadioStation(pBase, 0);
        case NPC__ID:
        case CREA_ID: return static_cast<uint8_t>(static_cast<TESActorBase*>(pBase)->HasLowLevelProcessing()) != 0;
        case DOOR_ID:
            if (static_cast<TESObjectDOOR*>(pBase)->IsRandomDoor()) {
                return true;
            }
            return GetTeleport() != nullptr;
        default: return false;
    }
}

// Enables the reference: clears its disabled flag, enables its ash pile (unless it is an ash pile itself), resets an
// actor's inventory and gives a mobile object without a process a low one. In a loaded cell its 3D is queued (and a
// light's scene light updated); an actor's package is then re-evaluated. Last, the enable-state children follow
// (disabled when they are inverse).
void TESObjectREFR::Enable() {
    if (!GetDisabled()) {
        return;
    }
    SetAltered(true);
    SetDisabled(false);
    if (GetParentCell() && IsActor()) {
        GetParentCell()->RemoveScriptedRef(this);
    }
    TESObjectREFR* pAshPile = GetExtra()->GetAshPileRef();
    if (pAshPile && GetObjectReference() != pAshPile1 && GetObjectReference() != pAshPile2) {
        pAshPile->Enable();
    }
    if (IsActor()) {
        ResetInventory(true);
    }
    MobileObject* pMobile = nullptr;
    if (IsMobileObject()) {
        pMobile = static_cast<MobileObject*>(this);
    }
    if (pMobile && !pMobile->GetCurrentAIProcess()) {
        LowProcess* pProcess = new LowProcess;
        pMobile->SetCurrentAIProcess(pProcess);
        ReferenceProcessLists.AddReference(pMobile, PROCESS_TYPE_LOW, false, false, nullptr);
    }
    bool bQueued = false;
    if (GetParentCell() &&
        TES::pInstance->IsCellLoaded(GetParentCell(), false) &&
        !Get3D() &&
        !pModelLoader->IsReferenceQueued(this)) {
        bQueued = true;
        if (pMobile && pMobile->IsActor()) {
            ReferenceProcessLists.AddActorToTempChangeList(static_cast<Actor*>(pMobile));
        }
        if (GetParentCell()->GetAttached()) {
            AttachSound(true);
        }
        if (!GetDelete() &&
            GetObjectReference() &&
            (GetObjectReference()->GetFormType() == LIGH_ID ||
             GetObjectReference()->GetFormType() == ACTI_ID ||
             !IsMarker())) {
            IO_TASK_PRIORITY ePriority = TES::pInstance->GetCellPriority(GetParentCell(), nullptr);
            pModelLoader->QueueReference(this, ePriority, false);
        }
        if (GetParentCell()->GetAttached() &&
            Get3D() &&
            GetObjectReference() &&
            GetObjectReference()->GetFormType() == LIGH_ID) {
            LightRefData* pLight = GetLight();
            if (pLight && pLight->spLight) {
                NiLight* pNiLight = pLight->spLight;
                BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->UpdateLight_0(pNiLight);
            }
        }
    }
    if (pMobile) {
        if (!bQueued) {
            pMobile->ChangeProcessLevel();
        }
        Actor* pActor = nullptr;
        if (pMobile->IsActor()) {
            pActor = static_cast<Actor*>(pMobile);
        }
        if (pActor && pActor->GetCurrentAIProcess()) {
            BaseProcess* pProcess = pActor->GetCurrentAIProcess();
            pProcess->UpdateMissedPackages(pActor, true);
            pActor->EvaluatePackage(false, true);
            if (pActor->GetCurrentAIProcess()->GetProcessLevel() != PROCESS_TYPE_HIGH) {
                pActor->Process(0.0f);
            }
        }
    }
    pPlayer->CheckForQuestTargetUpdate(this);
    // A NaN or non-positive radius skips the radio update.
    if (GetRadioStationRadius() > 0.0) {
        FalloutRadio::NewCellEntered();
    }
    BSSimpleList<TESObjectREFR*>* pChildren = GetEnableStateChildList();
    while (pChildren) {
        if (pChildren->IsEmpty()) {
            break;
        }
        TESObjectREFR* pChild = pChildren->GetItem();
        if (pChild->GetEnableStateInverse()) {
            pChild->GetExtra()->ShouldPopOutWhenDisabledByParent();
            pChild->Disable();
        }
        else {
            if (pChild->GetExtra()->ShouldPopInWhenEnabledByParent()) {
                pChild->SetDisableFade(true);
            }
            pChild->Enable();
        }
        pChildren = pChildren->GetNext();
    }
}

// Disables the reference (never the player): drops its 3D, disables its ash pile (unless it is an ash pile itself),
// cleans up its traps and sets the disabled flag. A mobile object with a process loses it, after an actor's magic,
// arrows, combat and perception entries, dialogue, package and furniture are cleaned up. Its LOD tree is hidden, the
// enable-state children follow (enabled when they are inverse), dropped items are deleted and its sounds detached.
void TESObjectREFR::Disable() {
    if (GetDisabled() || this == pPlayer) {
        return;
    }
    Set3D(nullptr, false);
    TESObjectREFR* pAshPile = GetExtra()->GetAshPileRef();
    if (pAshPile && GetObjectReference() != pAshPile1 && GetObjectReference() != pAshPile2) {
        pAshPile->Disable();
    }
    CleanUpTraps();
    SetAltered(true);
    SetDisabled(true);
    MobileObject* pMobile = nullptr;
    if (IsMobileObject()) {
        pMobile = static_cast<MobileObject*>(this);
    }
    if (pMobile && pMobile->GetCurrentAIProcess()) {
        if (pMobile->IsActor()) {
            Actor* pActor = static_cast<Actor*>(pMobile);
            pActor->DispelPermanentMagic();
            static_cast<MagicTarget&>(*pActor).Dispel();
            if (ArrowProjectile::GetArrowCount() > 0) {
                ArrowProjectile::DeleteAttachedArrows(this, true);
            }
            pPlayer->RemoveActorFromPlayercombatList(pActor);
            pPlayer->RemovePerceivedActor(pActor);
            ReferenceProcessLists.RemoveActorCloseToPlayer(pActor);
            if (GetParentCell() && HasScript()) {
                GetParentCell()->AddScriptedRef(this);
            }
            if (pActor->IsDying()) {
                pActor->DoDeathStuff();
            }
        }
        pMobile->CleanUpPointersOnDisable();
        ReferenceProcessLists.RemoveReference(pMobile, pMobile->GetCurrentProcessType());
        if (IsActor() || GetTargeted()) {
            ReferenceProcessLists.NullOutTargetRef(this, false);
        }
        if (pMobile->GetCurrentAIProcess() && pMobile->GetCurrentAIProcess()->GetCurrentPackage()) {
            Actor* pActor = nullptr;
            if (pMobile->IsActor()) {
                pActor = static_cast<Actor*>(pMobile);
            }
            if (pActor) {
                pActor->EndDialogue();
                pActor->EndInterruptPackage(false);
            }
            else if (pMobile->IsTalkingActivator()) {
                pMobile->EndDialogue();
            }
        }
        if (IsActor()) {
            RemoveChange(CHANGE_ACTOR_TEMP_MODIFIERS);
            RemoveChange(CHANGE_ACTOR_DAMAGE_MODIFIERS);
        }
        if (pMobile->GetCurrentAIProcess()) {
            if (pMobile->GetCurrentAIProcess()->GetCurrentFurniture()) {
                if (pMobile->IsActor()) {
                    static_cast<Actor*>(pMobile)->GetOutofFurnitureQuick();
                }
                else {
                    // Fetches both processes, then reads the index before the furniture.
                    BaseProcess* pIndexProcess = pMobile->GetCurrentAIProcess();
                    BaseProcess* pFurnitureProcess = pMobile->GetCurrentAIProcess();
                    int32_t iIndex = pIndexProcess->GetCurrentFurnitureIndex();
                    pFurnitureProcess->GetCurrentFurniture()->SetMarkerUsed(iIndex, false);
                }
            }
            BaseProcess* pProcess = pMobile->GetCurrentAIProcess();
            delete pProcess;
            pMobile->SetCurrentAIProcess(nullptr);
        }
    }
    if (GetObjectReference()->TESForm::GetHasSpokenFlag()) {
        TES::pInstance->GetWorldSpace()->GetTerrainManager()->HideTree(this, true);
    }
    SetLoadedDataFlag(LOADED_DATA_HAVOK_INITED, false);
    pPlayer->CheckForQuestTargetUpdate(this);
    BSSimpleList<TESObjectREFR*>* pChildren = GetEnableStateChildList();
    while (pChildren && !pChildren->IsEmpty()) {
        TESObjectREFR* pChild = pChildren->GetItem();
        if (pChild->GetEnableStateInverse()) {
            if (pChild->GetExtra()->ShouldPopInWhenEnabledByParent()) {
                pChild->SetDisableFade(true);
            }
            pChild->Enable();
        }
        else {
            pChild->Disable();
        }
        pChildren = pChildren->GetNext();
    }
    BSSimpleList<TESObjectREFR*>* pDropped = GetExtra()->GetDroppedItemList();
    while (pDropped && !pDropped->IsEmpty()) {
        TESObjectREFR* pItem = pDropped->GetItem();
        pItem->GetExtra()->SetItemDropper(nullptr);
        pItem->MarkAsDeleted();
        pDropped->RemoveHead();
    }
    AttachSound(false);
    if (IsActor()) {
        FalloutRadio::DisableNPCRadio(this);
    }
}

// A mobile object at high or middle-high process level hands the animation to its process; anything else keeps it in
// the reference's extra data. The checking-models flag is read and dropped.
Animation* TESObjectREFR::SetAnimation(Animation* apAnim) {
    MobileObject* pMobileObject = nullptr;
    if (IsMobileObject()) {
        pMobileObject = static_cast<MobileObject*>(this);
    }
    if (pMobileObject) {
        if (apAnim) {
            ForceChange(CHANGE_REFR_ANIMATION);
        }
        else {
            RemoveChange(CHANGE_REFR_ANIMATION);
        }
    }
    if (pMobileObject) {
        MiddleHighProcess* pProcess = nullptr;
        if (pMobileObject->GetCurrentAIProcess()) {
            PROCESS_TYPE eLevel = pMobileObject->GetCurrentAIProcess()->GetProcessLevel();
            if (eLevel != PROCESS_TYPE_MIDDLE_HIGH && eLevel != PROCESS_TYPE_HIGH && apAnim) {
                static_cast<Actor*>(pMobileObject)->ConditionalFree(false);
            }
            eLevel = pMobileObject->GetCurrentAIProcess()->GetProcessLevel();
            if (eLevel >= PROCESS_TYPE_HIGH && eLevel <= PROCESS_TYPE_MIDDLE_HIGH) {
                pProcess = static_cast<MiddleHighProcess*>(pMobileObject->GetCurrentAIProcess());
            }
        }
        if (pProcess) {
            pProcess->SetAnimation(apAnim);
            return apAnim;
        }
    }
    pDataHandler->GetCheckingModels();
    if (GetExtra()->GetAnimation() == apAnim) {
        return apAnim;
    }
    if (apAnim) {
        GetExtra()->SetAnimPtr(apAnim);
    }
    else {
        GetExtra()->RemoveAnimPtr();
    }
    return apAnim;
}

BSSimpleList<const char*>* TESObjectREFR::BuildKFFileList(ANIM_GROUP_WEAPON_ENUM aeWeapType) {
    BSSimpleList<const char*>* pfileList = nullptr;
    if (!GetObjectReference()) {
        return pfileList;
    }
    int32_t iFormType = GetObjectReference()->GetFormType();
    if (iFormType == 42) {
        TESNPC* pNPC = static_cast<TESNPC*>(GetObjectReference());
        if (pNPC->iFormID != 7 && IsDead(true)) {
            WEAPON_TYPE eWeaponType;
            char cPath[264];
            char cModelPath[268];
            BSstrcpy(cPath, 260, "Data\\Meshes\\Characters\\_Male\\Locomotion\\MTIdle.kf");
            BSstrcpy(cModelPath, 260, "Characters\\_Male\\Locomotion\\MTIdle.kf");
            pfileList = pModelLoader->BuildFileList(cPath, cModelPath, nullptr);
            BSstrcpy(cPath, 260, "Data\\Meshes\\Characters\\_Male\\MTIdle.kf");
            char* pSlash = std::strrchr(cPath, 92);
            TESObjectWEAP* pWeapon = static_cast<Actor*>(this)->Actor::GetCurrentWeapon();
            if (pWeapon) {
                eWeaponType = pWeapon->GetWeaponType();
            }
            else {
                eWeaponType = WEAPON_HAND_TO_HAND;
            }
            const char* pWeaponPrefix = animWeaponInfo[weaponToAnimGroupWeapon[eWeaponType]];
            const char* pModel;
            if (pWeaponPrefix && *pWeaponPrefix) {
                std::sprintf(pSlash, "\\%sHolster.KF", pWeaponPrefix);
                pModel = GetModel();
                pModelLoader->BuildFileList(cPath, pModel, pfileList);
                std::sprintf(pSlash, "\\PA%sHolster.KF", pWeaponPrefix);
                pModel = GetModel();
                pModelLoader->BuildFileList(cPath, pModel, pfileList);
            }

            BSstrcpy(pSlash, 260 - (pSlash - cPath), "\\Death.KF");
            pModel = GetModel();
            pModelLoader->BuildFileList(cPath, pModel, pfileList);
        }
        else {
            char cIdlePath[264];
            const char* pModel = pNPC->GetModel();
            pfileList = pModelLoader->BuildKFFileList(pModel, true, true, aeWeapType);
            BSstrcpy(cIdlePath, 260, pNPC->GetModel());
            char* pSlash = std::strrchr(cIdlePath, 92);
            if (IsChild(true)) {
                BSstrcpy(pSlash, 260 - (pSlash - cIdlePath) - 1, "\\Locomotion\\Child\\IdleAnims");
            }
            else if (pNPC->GetSex() == TESActorBase::SEX_FEMALE) {
                BSstrcpy(pSlash, 260 - (pSlash - cIdlePath) - 1, "\\Locomotion\\Female\\IdleAnims");
            }
            else {
                BSstrcpy(pSlash, 260 - (pSlash - cIdlePath) - 1, "\\Locomotion\\Male\\IdleAnims");
            }
            BSSimpleList<const char*>* pRootList = pIdleManager->GetRootFilenameList(cIdlePath, AGW_NONE);
            pModelLoader->CopyFilenameList(pRootList, pfileList);
        }
    }
    else if (iFormType == 43) {
        Actor* pActor = static_cast<Actor*>(this);
        if (pActor->GetCurrentAIProcess()) {
            PROCESS_TYPE eLevel;
            if (!pActor->IsDead(false) &&
                (eLevel = pActor->GetCurrentAIProcess()->GetProcessLevel()) >= PROCESS_TYPE_HIGH &&
                eLevel <= PROCESS_TYPE_MIDDLE_HIGH) {
                MiddleHighProcess* pProcess = static_cast<MiddleHighProcess*>(pActor->GetCurrentAIProcess());
                if (pProcess->GetCurrentWeapon()) {
                    ItemChange* pWeaponItem = pProcess->GetCurrentWeapon();
                    aeWeapType = weaponToAnimGroupWeapon[static_cast<TESObjectWEAP*>(pWeaponItem->pContainerObj)
                                                             ->TESObjectWEAP::GetWeaponType()];
                }
                else if (pActor->GetWantWeaponDrawn()) {
                    aeWeapType = AGW_HAND_TO_HAND;
                }
            }
        }
        if (IsDead(true)) {
            WEAPON_TYPE eWeaponType;
            char cPath[264];
            BSstrcpy(cPath, 260, "Data\\");
            BSstrcat(cPath, 260, "Meshes");
            BSstrcat(cPath, 260, "\\");
            BSstrcat(cPath, 260, GetModel());
            char* pSlash = std::strrchr(cPath, 92);
            BSstrcpy(pSlash, 260 - (pSlash - cPath), "\\MTIdle.KF");
            const char* pModel = GetModel();
            pfileList = pModelLoader->BuildFileList(cPath, pModel, nullptr);
            TESObjectWEAP* pWeapon = pActor->GetCurrentWeapon();
            if (pWeapon) {
                eWeaponType = pWeapon->GetWeaponType();
            }
            else {
                eWeaponType = WEAPON_HAND_TO_HAND;
            }
            const char* pWeaponPrefix = animWeaponInfo[weaponToAnimGroupWeapon[eWeaponType]];
            if (pWeaponPrefix && *pWeaponPrefix) {
                std::sprintf(pSlash, "\\%sHolster.KF", pWeaponPrefix);
                pModel = GetModel();
                pModelLoader->BuildFileList(cPath, pModel, pfileList);
                std::sprintf(pSlash, "\\PA%sHolster.KF", pWeaponPrefix);
                pModel = GetModel();
                pModelLoader->BuildFileList(cPath, pModel, pfileList);
            }

            BSstrcpy(pSlash, 260 - (pSlash - cPath), "\\Death.KF");
            pModel = GetModel();
            pModelLoader->BuildFileList(cPath, pModel, pfileList);
        }
        else {
            const char* pModel = GetModel();
            return pModelLoader->BuildKFFileList(pModel, true, true, aeWeapType);
        }
    }
    return pfileList;
}

void TESObjectREFR::SetRefPersists(bool abVal) {
    uint32_t uiNewFlags;
    TESObjectCELL* pcell = GetSaveParentCell();
    uint32_t iflags = iFormFlags;
    if (abVal) {
        uiNewFlags = iflags | kFormFlags_IsRefPersistent;
    }
    else {
        uiNewFlags = iflags & ~kFormFlags_IsRefPersistent;
    }
    iFormFlags = uiNewFlags;
    if (abVal) {
        AddChange(CHANGE_FORM_FLAGS);
        if (pcell && !pcell->GetInterior() && !pcell->GetPersistentCell()) {
            TESWorldSpace* pworld = pcell->GetWorldSpace();
            pworld->AddPersistentRef(this);
        }
    }
    else if (pcell && pcell->GetPersistentCell()) {
        TESWorldSpace* pworld = pcell->GetWorldSpace();
        pworld->RemovePersistentRef(this);
        SetAltered(true);
    }
}

bool TESObjectREFR::MustBeVisibleDistant() {
    if (!TESObjectREFR::GetObjectReference()) {
        return false;
    }
    return TESObjectREFR::GetObjectReference()->TESForm::GetMustBeVisibleDistant();
}

void TESObjectREFR::SetVisibleDistant(bool abVal) {
    uint32_t iFlags = TESForm::GetFormFlags();
    if (abVal) {
        iFlags |= kFormFlag_MustBeVisibleDistant;
    }
    else {
        iFlags &= ~kFormFlag_MustBeVisibleDistant;
    }
    TESForm::SetFormFlags(iFlags);
}

// The zone's level, else the player's; abAllowShift steps the shared adjustment up to iCalcLevelAdjustUp and wraps it
// to -iCalcLevelAdjustDown (signed compare). A level of 0 becomes 1 (unsigned compare).
uint32_t TESObjectREFR::GetCalcLevel(bool abAllowShift) {
    uint32_t uiLevel = 0;
    BGSEncounterZone* pEncounterZone = GetEncounterZone();
    if (pEncounterZone && pEncounterZone != BGSEncounterZone::GetNoZone()) {
        uiLevel = pEncounterZone->QZoneLevel();
    }
    else {
        uiLevel = pPlayer->GetLevel();
    }
    if (abAllowShift) {
        ileveladjust = ileveladjust + 1;
        if (ileveladjust > static_cast<int32_t>(iCalcLevelAdjustUp.UInt())) {
            ileveladjust = -static_cast<int32_t>(iCalcLevelAdjustDown.UInt());
        }
        uiLevel += ileveladjust;
    }
    if (uiLevel < 1) {
        uiLevel = 1;
    }
    return uiLevel;
}

// False when following the enable state parents from this reference comes back to it. A parent not yet initialized
// still holds its form ID, which is looked up; a parent already seen ends the walk.
bool TESObjectREFR::CheckEnableParentLoop() {
    bool bsafe = true;
    alignas(NiTPointerMap<TESObjectREFR*, bool>)
        uint8_t aCheckedMapStorage[sizeof(NiTPointerMap<TESObjectREFR*, bool>)];
    NiTPointerMap<TESObjectREFR*, bool>* pcheckedmap = ::new (aCheckedMapStorage)
        NiTPointerMap<TESObjectREFR*, bool>(0x25);
    TESObjectREFR* pcheck = GetEnableStateParent();
    while (pcheck && bsafe) {
        if (pcheck == this) {
            bsafe = false;
        }
        else {
            pcheckedmap->SetAt(pcheck, true);
            if (pcheck->GetInitialized()) {
                pcheck = pcheck->GetEnableStateParent();
            }
            else {
                TESObjectREFR* pParent = pcheck->GetEnableStateParent();
                if (pParent) {
                    uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pParent));
                    pcheck = dynamic_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(iID));
                }
                else {
                    pcheck = nullptr;
                }
            }
            bool bdone = false;
            if (pcheck && pcheckedmap->GetAt(pcheck, bdone) && bdone) {
                pcheck = nullptr;
            }
        }
    }
    pcheckedmap->~NiTPointerMap<TESObjectREFR*, bool>();
    return bsafe;
}

bool TESObjectREFR::CanHaveSound() {
    if (!GetObjectReference()) {
        return false;
    }
    switch (GetObjectReference()->GetFormType()) {
        case SOUN_ID:
        case ACTI_ID:
        case TERM_ID:
        case CONT_ID:
        case DOOR_ID:
        case LIGH_ID:
        case MISC_ID:
        case STAT_ID:
        case MSTT_ID:
        case PWAT_ID:
        case CREA_ID: return true;
        default: return false;
    }
}

// Starts (abAttach) or stops the reference's looping sound. Attaching an enabled, undestroyed reference picks the
// looping sound of its base form (a sound form plays itself, as a beam along its primitive when it has one), stops and
// drops any sound already stored, then plays the new one when it is a beam or loops, else keeps it as the random sound;
// activators with a radio station turn the radio on when open. Detaching fades the stored sound out over a second and
// turns an activator's radio off. Either way the addon nodes' sounds follow when the 3D is loaded. The form ID is read
// into a local before BSAudio::QInstance is called.
void TESObjectREFR::AttachSound(bool abAttach) {
    if (!GetObjectReference()) {
        return;
    }
    if (abAttach) {
        if (!GetDisabled() && !GetDestroyed()) {
            TESSound* pLoopingSound = nullptr;
            BGSPrimitive* pPrimitive = nullptr;
            NiPoint3 kBeamEnd = NiPoint3::ZERO;
            NiPoint3 kBeamStart = NiPoint3::ZERO;
            switch (GetObjectReference()->GetFormType()) {
                case SOUN_ID:
                    pLoopingSound = static_cast<TESSound*>(GetObjectReference());
                    pPrimitive = GetExtra()->GetPrimitive();
                    if (pPrimitive) {
                        NiMatrix3 kRotation = GetOrientation();
                        NiPoint3 kLocation = GetLocationOnReference();
                        kBeamEnd = -1.0f * GetExtra()->GetPrimitive()->GetRadii();
                        kBeamEnd = kBeamEnd * kRotation;
                        kBeamEnd += GetLocationOnReference();
                        kBeamStart = GetExtra()->GetPrimitive()->GetRadii();
                        kBeamStart = kBeamStart * kRotation;
                        kBeamStart += kLocation;
                        // Swaps the two y components through the location copy's y.
                        kLocation.y = kBeamStart.y;
                        kBeamStart.y = kBeamEnd.y;
                        kBeamEnd.y = kLocation.y;
                    }
                    break;
                case ACTI_ID:
                    if (static_cast<TESObjectACTI*>(GetObjectReference())->GetRadioStation() &&
                        BGSOpenCloseForm::GetOpenState(this) == BGSOpenCloseForm::OBJECT_OPEN &&
                        !pSaveLoadGame->GetSaveGameLoading()) {
                        FalloutRadio::ActivateRadio(this, true);
                    }
                    pLoopingSound = static_cast<TESObjectACTI*>(GetObjectReference())->GetLoopingSound();
                    break;
                case TERM_ID: pLoopingSound = static_cast<BGSTerminal*>(GetObjectReference())->GetLoopingSound(); break;
                case CONT_ID:
                    pLoopingSound = static_cast<TESObjectCONT*>(GetObjectReference())->GetLoopingSound();
                    break;
                case DOOR_ID: pLoopingSound = static_cast<TESObjectDOOR*>(GetObjectReference())->GetLoopSound(); break;
                case LIGH_ID: pLoopingSound = static_cast<TESObjectLIGH*>(GetObjectReference())->GetSound(); break;
                case MISC_ID:
                    pLoopingSound = static_cast<TESObjectMISC*>(GetObjectReference())->GetLoopingSound();
                    break;
                case STAT_ID: pLoopingSound = static_cast<TESObjectSTAT*>(GetObjectReference())->pRandomSound; break;
                case MSTT_ID:
                    pLoopingSound = static_cast<BGSMovableStatic*>(GetObjectReference())->GetLoopingSound();
                    break;
                case PWAT_ID: {
                    BGSPlaceableWater* pWater = static_cast<BGSPlaceableWater*>(GetObjectReference());
                    if (pWater && !pWater->IsAutomaticWater() && !pWater->IsLODWater()) {
                        TESWaterForm* pWaterForm = pWater->GetWaterType();
                        if (pWaterForm) {
                            pLoopingSound = pWaterForm->GetWaterSound();
                        }
                    }
                    break;
                }
                case CREA_ID:
                    if (IsActor() && IsCreature()) {
                        pLoopingSound = static_cast<TESCreature*>(GetObjectReference())
                                            ->PickCreatureSound(CREA_SOUND_LOOP);
                    }
                    break;
                default: break;
            }
            BSSoundHandle kSound;
            GetExtra()->GetSound(kSound);
            if (kSound.IsValid()) {
                kSound.Stop();
                kSound.Release();
                GetExtra()->RemoveExtra(EXTRA_SOUND);
            }
            if (pLoopingSound) {
                uint32_t uiBeamFlag = pPrimitive ? BSGameSound::SOUND_FLAG_BEAM_EMITTER : 0;
                uint32_t uiLoopFlag = (pLoopingSound->GetData().iFlags & BSGameSound::SOUND_FLAG_LOOP)
                    ? BSGameSound::SOUND_FLAG_LOOP
                    : 0;
                if (pPrimitive) {
                    uint32_t uiFormID = pLoopingSound->GetFormID();
                    kSound = BSAudio::QInstance().PlaySound(
                        uiFormID,
                        static_cast<BSGameSound::SoundFlags>(
                            uiBeamFlag | BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_LOOP |
                            BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER
                        ),
                        kBeamStart,
                        0
                    );
                    kSound.SetBeamEnd(kBeamEnd.x, kBeamEnd.y, kBeamEnd.z);
                }
                else if (uiLoopFlag) {
                    const NiPoint3* pLocation = &GetLocationOnReference();
                    uint32_t uiFormID = pLoopingSound->GetFormID();
                    kSound = BSAudio::QInstance().PlaySound(
                        uiFormID,
                        static_cast<BSGameSound::SoundFlags>(
                            uiLoopFlag | BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER
                        ),
                        *pLocation,
                        0
                    );
                }
                else {
                    pRandomSound = pLoopingSound;
                }
                GetExtra()->SetSound(kSound);
            }
        }
    }
    else {
        switch (GetObjectReference()->GetFormType()) {
            case SOUN_ID:
            case ACTI_ID:
            case TERM_ID:
            case CONT_ID:
            case DOOR_ID:
            case LIGH_ID:
            case MISC_ID:
            case STAT_ID:
            case MSTT_ID:
            case PWAT_ID:
            case CREA_ID: {
                BSSoundHandle kSound;
                GetExtra()->GetSound(kSound);
                if (kSound.IsValid()) {
                    kSound.FadeOutAndRelease(1000);
                    GetExtra()->RemoveExtra(EXTRA_SOUND);
                }
                break;
            }
            default: break;
        }
        if (GetObjectReference() &&
            GetObjectReference()->GetFormType() == ACTI_ID &&
            static_cast<TESObjectACTI*>(GetObjectReference())->GetRadioStation() &&
            !pSaveLoadGame->GetSaveGameLoading() &&
            !pDataHandler->GetClearingData()) {
            FalloutRadio::ActivateRadio(this, false);
        }
    }
    if (TESObjectREFR::Get3D()) {
        UpdateAddonNodeSounds(abAttach);
    }
}

void TESObjectREFR::UpdateAddonNodeSounds(bool abDetachAll) {
    NiNode* p3D = TESObjectREFR::Get3D();
    if (p3D && GetLoadedDataFlag(LOADED_DATA_HAS_ADDON_FLAGS)) {
        UpdateAddonNodeSoundsRecursive(p3D, abDetachAll);
    }
}

void TESObjectREFR::UpdateAddonNodeSoundsRecursive(NiAVObject* apRoot, bool abDetachAll) {
    if (apRoot) {
        if (apRoot->GetAppCulled()) {
            abDetachAll = true;
        }
        if (NiObject::IsKindOf_0(&BSValueNode::ms_RTTI, apRoot)) {
            BSValueNode* pvalueNode = static_cast<BSValueNode*>(apRoot);
            uint32_t uiValue = pvalueNode->QValue();
            BGSAddonNode* paddonNode = pDataHandler->GetAddonNode(uiValue);
            if (paddonNode && paddonNode->pSound) {
                const NiFixedString* pTag = &BGSAddonNodeSoundHandleExtra::Tag();
                NiPointer<BGSAddonNodeSoundHandleExtra> spsoundHandleExtra(
                    static_cast<BGSAddonNodeSoundHandleExtra*>(apRoot->GetExtraData(*pTag))
                );
                if (abDetachAll) {
                    if (spsoundHandleExtra.m_pObject) {
                        apRoot->RemoveExtraData(BGSAddonNodeSoundHandleExtra::Tag());
                        spsoundHandleExtra->SoundHandle.Stop();
                        spsoundHandleExtra->SoundHandle.Release();
                    }
                }
                else if (!spsoundHandleExtra.m_pObject) {
                    uint32_t isoundFlags;
                    spsoundHandleExtra = new BGSAddonNodeSoundHandleExtra;
                    if (TESSound::ConvertLoopFlag(paddonNode->pSound->GetData().iFlags)) {
                        isoundFlags = TESSound::ConvertLoopFlag(paddonNode->pSound->GetData().iFlags) |
                            BSGameSound::SOUND_FLAG_IS_3D;
                    }
                    else {
                        isoundFlags = BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT;
                    }
                    uint32_t uiSoundID = paddonNode->pSound->iFormID;
                    spsoundHandleExtra->SoundHandle = BSAudio::QInstance().GetSoundHandleByNumericID(
                        uiSoundID,
                        isoundFlags
                    );
                    spsoundHandleExtra->SoundHandle.SetObjectToFollow(apRoot);
                    NiPoint3 kPosition = apRoot->GetWorldTranslate();
                    spsoundHandleExtra->SoundHandle.SetPosition(kPosition.x, kPosition.y, kPosition.z);
                    spsoundHandleExtra->SoundHandle.Play(false);
                    apRoot->AddExtraData(spsoundHandleExtra.m_pObject);
                }
            }
        }
        NiNode* pnode = apRoot->IsNode();
        if (pnode) {
            for (uint32_t n = 0; n < pnode->GetArrayCount(); ++n) {
                NiAVObject* pobj = pnode->GetAt(n);
                if (pobj) {
                    UpdateAddonNodeSoundsRecursive(pobj, abDetachAll);
                }
            }
        }
    }
}

// Adds the light to the world scene unless the reference is deleted or disabled or lights water.
void TESObjectREFR::AddLight(bool abMagic) {
    if (GetDelete() || GetDisabled()) {
        return;
    }
    LightRefData* pLight;
    if (abMagic) {
        pLight = GetExtra()->GetSpellEffectLight();
    }
    else {
        pLight = GetExtra()->GetLight();
    }
    if (!pLight || !pLight->spLight) {
        return;
    }
    if (!GetExtra()->GetLitWaterRefs() || (GetExtra()->GetLitWaterRefs() && GetExtra()->GetLitWaterRefs()->IsEmpty())) {
        bool bDynamic = true;
        if (GetObjectReference() && GetObjectReference()->GetFormType() == LIGH_ID) {
            bDynamic = static_cast<TESObjectLIGH*>(GetObjectReference())->GetDynamic();
        }
        BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->AddLight(pLight->spLight, bDynamic);
    }
}

void TESObjectREFR::RemoveLight(bool abMagic) {
    ExtraDataList* pExtra = TESObjectREFR::GetExtra();
    LightRefData* pLight = abMagic ? pExtra->GetSpellEffectLight() : pExtra->GetLight();
    if (pLight && pLight->spLight.m_pObject) {
        NiLight* pNiLight = pLight->spLight.m_pObject;
        ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD);
        pShadowSceneNode->RemoveLight(pNiLight);
    }
}

// An NPC or creature whose high process has no 3D yet runs nothing.
bool TESObjectREFR::RunScript() {
    bool bRan = false;
    if (GetDelete() || !GetObjectReference()) {
        return bRan;
    }
    int32_t eType = GetObjectReference()->GetFormType();
    if (eType == CONT_ID || (eType > AMMO_ID && eType <= CREA_ID)) {
        if (eType != CONT_ID) {
            MobileObject* pMobile = static_cast<MobileObject*>(this);
            if (pMobile->GetCurrentAIProcess() &&
                pMobile->GetCurrentProcessType() == PROCESS_TYPE_HIGH &&
                !pMobile->Get3D()) {
                return false;
            }
        }
        if (HasContainer()) {
            InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(this);
            if (pChanges && pChanges->RunScripts(this)) {
                bRan = true;
            }
        }
    }
    Script* pScript = GetExtra()->GetScript();
    if (!pScript) {
        return bRan;
    }
    ScriptLocals* pLocals = GetExtra()->GetScriptLocals();
    if (pScript->Run(this, pLocals, nullptr, false)) {
        bRan = true;
    }
    else if (pLocals) {
        AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
    }
    if (!GetExtra()->GetSayToExtra()) {
        return bRan;
    }
    if (!IsMobileObject()) {
        UpdateSoundCallBack();
    }
    return bRan;
}

bool TESObjectREFR::UpdateChildActivates() {
    bool bactive = false;
    float ftime = appTimer.fDelta;
    BSSimpleList<REF_ACTIVATE_DATA*>* plist = m_Extra.GetActivateRefChildren();
    float fcurrentActivateChildrenTimer = m_Extra.GetActivateChildrenTimer();
    float fnewActivateChildrenTimer = fcurrentActivateChildrenTimer + ftime;
    while (plist && !plist->IsEmpty()) {
        REF_ACTIVATE_DATA* pdata = plist->m_item;
        if (fcurrentActivateChildrenTimer < static_cast<double>(pdata->fActivateDelay)) {
            if (fnewActivateChildrenTimer >= static_cast<double>(pdata->fActivateDelay)) {
                pdata->pActivateRef->Activate(this, false, nullptr, 1);
            }
            else {
                bactive = true;
            }
        }
        plist = plist->m_pkNext;
    }
    if (!bactive) {
        RemoveChange(CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN);
        fnewActivateChildrenTimer = 0.0f;
    }
    m_Extra.SetActivateChildrenTimer(fnewActivateChildrenTimer);
    return bactive;
}

NiAVObject* TESObjectREFR::GetActorBone(NiAVObject* apRoot, const char* apSearch) {
    return BSUtilities::GetObjectByName(apRoot, apSearch, true);
}

// The node named apSearch under apRoot, searching the root's own name too.
NiAVObject* TESObjectREFR::GetActorBone(NiAVObject* apRoot, NiFixedString& apSearch) {
    return BSUtilities::GetObjectByName(apRoot, apSearch, true);
}

// The rotation scaled by the base NPC's race; the race scaling is compiled out, so this copies apRotIn.
NiMatrix3 TESObjectREFR::MultipleMatrixByRace(NiMatrix3& apRotIn) {
    return apRotIn;
}

// An AutoMemContext is constructed and destroyed in local storage, and the constructor's result is ignored.
void TESObjectREFR::InitScript() {
    if (GetDelete()) {
        return;
    }
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_SCRIPTS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectREFR.cpp",
        6510
    );
    auto* pScriptExtra = static_cast<ExtraScript*>(GetExtra()->GetExtraData(EXTRA_SCRIPT));
    if (pScriptExtra && pScriptExtra->pScript) {
        pContext->~AutoMemContext();
        return;
    }
    TESForm* pForm = GetTemplateActorBase();
    if (!pForm) {
        pForm = GetObjectReference();
    }
    Script* pScript = TESScriptableForm::GetFormScript(pForm);
    if (pScript) {
        GetExtra()->SetScript(pScript);
        ScriptLocals* pLocals = pScript->CloneLocals();
        GetExtra()->SetScriptLocals(pLocals);
    }
    if (HasContainer()) {
        InventoryChanges::GetInventoryChanges(this)->InitScripts();
    }
    pContext->~AutoMemContext();
}

TESGlobal* TESObjectREFR::GetOwnershipGlobal() {
    TESGlobal* pGlobal = GetExtra()->GetGlobal();
    if (!pGlobal) {
        DoorTeleportData* pTeleport = GetTeleport();
        if (pTeleport && pTeleport->pLinkedDoor) {
            pGlobal = pTeleport->pLinkedDoor->GetExtra()->GetGlobal();
        }
    }
    if (!pGlobal && GetParentCell()) {
        pGlobal = GetParentCell()->GetOwnershipGlobal();
    }
    return pGlobal;
}

// -1 means no rank at that level; none at all gives 0.
int32_t TESObjectREFR::GetOwnershipRank() {
    int32_t iRank = GetExtra()->GetRank();
    if (iRank == -1) {
        DoorTeleportData* pTeleport = GetTeleport();
        if (pTeleport && pTeleport->pLinkedDoor) {
            iRank = pTeleport->pLinkedDoor->GetExtra()->GetRank();
        }
    }
    if (iRank == -1) {
        BGSEncounterZone* pEncounterZone = GetEncounterZone();
        if (pEncounterZone && pEncounterZone != BGSEncounterZone::GetNoZone()) {
            iRank = pEncounterZone->QOwnerFactionRank();
        }
        if (iRank == -1 && GetParentCell()) {
            iRank = GetParentCell()->GetOwnershipRank();
        }
    }
    if (iRank == -1) {
        iRank = 0;
    }
    return iRank;
}

// Warning is an empty stub; the form ID is read before the editor ID.
bool TESObjectREFR::CheckUsesExternalEmittance() {
    bool bResult = false;
    if (!GetObjectReference()) {
        return bResult;
    }
    if (GetObjectReference()->GetFormType() == LIGH_ID && GetEmittanceSource()) {
        bResult = true;
    }
    else if (Get3D()) {
        bResult = BSUtilities::GetExternalEmittanceRecurse(Get3D());
        if (!bResult && GetEmittanceSource()) {
            uint32_t uiFormID = GetFormID();
            const char* pEditorID = GetFormEditorID();
            Warning(
                "3D for Ref '%s' (%08X) does not support external emittance but ref has explicit emittance data",
                pEditorID,
                uiFormID
            );
        }
    }
    return bResult;
}

void TESObjectREFR::UpdateEnableStateChildrenFlagsRecursive() {
    for (BSSimpleList<TESObjectREFR*>* pNode = GetEnableStateChildList(); pNode; pNode = pNode->GetNext()) {
        TESObjectREFR* pChild = pNode->GetItem();
        if (pChild) {
            bool bDisabled = GetDisabled();
            if (pChild->GetEnableStateInverse()) {
                bDisabled = !bDisabled;
            }
            if (pChild->GetDisabled() != bDisabled) {
                pChild->SetDisabled(bDisabled);
                pChild->UpdateEnableStateChildrenFlagsRecursive();
            }
        }
    }
}

void TESObjectREFR::SetPackageStartLocation(
    TESWorldSpace* apWorld,
    TESObjectCELL* apIntCell,
    const NiPoint3& aPt,
    float afzrotation
) {
    GetExtra()->SetPackageStartLocation(apWorld, apIntCell, aPt, afzrotation);
    AddChange(CHANGE_ACTOR_EXTRA_PACKAGE_DATA);
}

void TESObjectREFR::SetLeveledCreature(TESActorBase* apOriginalBase, TESActorBase* apTemplate) {
    if (!GetObjectReference() || !GetObjectReference()->IsActorBase()) {
        return;
    }
    if (apOriginalBase && apTemplate) {
        ForceChange(CHANGE_ACTOR_EXTRA_LEVELED_ACTOR);
    }
    else {
        RemoveChange(CHANGE_ACTOR_EXTRA_LEVELED_ACTOR);
    }
    GetExtra()->SetLeveledCreature(apOriginalBase, apTemplate);
}

// The created base is destroyed through its virtual destructor with flag 1 (`delete`).
void TESObjectREFR::RestoreLeveledCreatureOriginalBase() {
    if (!data.pObjectReference) {
        return;
    }
    if (!pDataHandler->IsFormIDCreated(data.pObjectReference->GetFormID()) || !IsLeveledCreature()) {
        return;
    }
    TESBoundObject* pObject = data.pObjectReference;
    if (pObject) {
        delete pObject;
    }
    data.pObjectReference = static_cast<TESBoundObject*>(GetExtra()->GetLevCreaOriginalBase());
}

// Releases the 3D and the phantom, then deletes the loaded data.
void TESObjectREFR::FreeLoadedData() {
    if (pLoadedData) {
        pLoadedData->m_spData3D = nullptr;
        pLoadedData->spPhantom = nullptr;
        LOADED_REF_DATA* pData = pLoadedData;
        if (pData) {
            delete pData;
        }
        pLoadedData = nullptr;
    }
}

void TESObjectREFR::KillLight(bool abMagic) {
    LightRefData* pLight;
    if (abMagic) {
        pLight = GetExtra()->GetSpellEffectLight();
    }
    else {
        pLight = GetExtra()->GetLight();
    }
    if (!pLight) {
        return;
    }
    if (pLight->spLight) {
        BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_WORLD)->RemoveLight(pLight->spLight);
        pLight->spLight = nullptr;
    }
    if (abMagic) {
        GetExtra()->RemoveSpellEffectLightPtr();
    }
    else {
        GetExtra()->RemoveLightPtr();
    }
}

// Reads the zone's water reference through bhkTTNode<TESObjectREFR>::QReference and the world's water listener through
// bhkWorld::QWaterListener.
void TESObjectREFR::RemoveFromAllWater(bool abTraverseAllZones) {
    TESWaterSystem* pWaterSystem = TES::pInstance->GetWaterSystem();
    BSExtraData* pWaterZoneData = GetExtra()->GetExtraData(EXTRA_WATER_ZONE_MAP);
    if (abTraverseAllZones) {
        if (GetParentCell()) {
            bhkWorld* pWorld = GetParentCell()->GetbhkWorld();
            if (pWorld) {
                TESWaterListener* pListener = static_cast<TESWaterListener*>(pWorld->QWaterListener());
                if (pListener) {
                    pListener->RemoveTarget(this);
                    pListener->HandleQueuedRemovals(this, false, true);
                }
            }
        }
    }
    else if (pWaterZoneData) {
        NiTMap<WaterZone*, int32_t>* pZoneMap = GetExtra()->QWaterZoneMap();
        if (pZoneMap) {
            NiTMapItem<WaterZone*, int32_t>* pPos = pZoneMap->GetFirstPos();
            while (pPos) {
                WaterZone* pZone;
                int32_t iValue;
                pZoneMap->GetNext(pPos, pZone, iValue);
                pZone->RemoveTarget(this);
                pWaterSystem->RemoveTESObjectFromWaterGroup(pZone->QReference(), this, pZone->fWaterHeight);
            }
        }
    }
    if (pWaterZoneData) {
        GetExtra()->RemoveExtra(pWaterZoneData, true);
    }
    if (pParentCell && pParentCell->HasWater()) {
        pWaterSystem->RemoveTESObjectFromWaterGroup(nullptr, this, GetRelevantWaterHeight());
    }
    if (TES::pInstance->GetWaterSystem()) {
        TES::pInstance->GetWaterSystem()->RemoveTESObjectFromAllAutoWaterGroups(this);
    }
    if (IsActor()) {
        bhkCharacterController* pController = static_cast<bhkCharacterController*>(
            static_cast<MobileObject*>(this)->GetCharController()
        );
        if (pController) {
            pController->SetWaterHeight(GetRelevantWaterHeight());
        }
    }
    if (pLoadedData) {
        pLoadedData->iUnderwaterCount = 0;
        pLoadedData->pCurrentWaterObject = nullptr;
    }
}

bool TESObjectREFR::IsLeveledCreature() {
    if (!GetObjectReference()) {
        return false;
    }
    if (!GetObjectReference()->IsActorBase()) {
        return false;
    }
    if (GetExtra()->HasLevCreaCalced()) {
        return true;
    }
    return static_cast<TESActorBase*>(GetObjectReference())->GetUsesLeveledTemplate();
}

bool TESObjectREFR::GetRespawn() {
    bool bretval = false;
    if (!IsLeveledCreature() && GetObjectReference()) {
        switch (GetObjectReference()->GetFormType()) {
            case CONT_ID: {
                TESObjectCONT* pcont = static_cast<TESObjectCONT*>(GetObjectReference());
                if (pcont) {
                    bretval = pcont->GetRespawn();
                }
                break;
            }
            case NPC__ID:
            case CREA_ID: {
                TESActorBase* pbase = static_cast<TESActorBase*>(GetObjectReference());
                if (pbase) {
                    bretval = pbase->GetRespawn();
                }
                break;
            }
        }
    }
    return bretval;
}

void TESObjectREFR::Set3DVerySimple(NiAVObject* ap3D) {
    if (ap3D) {
        CreateLoadedData();
        pLoadedData->m_spData3D = static_cast<NiNode*>(ap3D);
    }
    else {
        FreeLoadedData();
    }
}

bool TESObjectREFR::CanBeMoved() {
    if (pDataHandler->IsFormIDCreated(GetFormID())) {
        return true;
    }
    switch (GetObjectReference()->GetFormType()) {
        case STAT_ID: return IsMarker();
        case SCOL_ID:
        case TREE_ID:
        case FLOR_ID:
        case FURN_ID: return false;
        default: return true;
    }
}

// A position with a coordinate at +-FLT_MAX, or x or y outside +-4096000 (NaN included), while a save loads, or x or
// y beyond +-4096000 otherwise, is replaced by the editor location raised by 10.
void TESObjectREFR::SetLocationOnReference(const NiPoint3& arPoint) {
    constexpr double kFloatMax = 3.4028234663852886e+38;
    constexpr double kFloatMin = -3.4028234663852886e+38;
    constexpr double kWorldMax = 4096000.0;
    constexpr double kWorldMin = -4096000.0;
    constexpr double kRaise = 10.0;

    bool bInvalidPos = false;
    NiPoint3 kStartingPos = NiPoint3::ZERO;
    NiPoint3 kRotation = NiPoint3::ZERO;
    if (pSaveLoadGame && pSaveLoadGame->GetSaveGameLoading()) {
        if (arPoint.x == kFloatMax ||
            arPoint.x == kFloatMin ||
            arPoint.y == kFloatMax ||
            arPoint.y == kFloatMin ||
            arPoint.z == kFloatMax ||
            arPoint.z == kFloatMin ||
            !(arPoint.x < kWorldMax) ||
            !(arPoint.x > kWorldMin) ||
            !(arPoint.y < kWorldMax) ||
            !(arPoint.y > kWorldMin)) {
            TESForm* pLocationForm = nullptr;
            GetEditorLocation(kStartingPos, kRotation, pLocationForm, nullptr);
            kStartingPos.z = kStartingPos.z + kRaise;
            bInvalidPos = true;
        }
    }
    else if (arPoint.x > kWorldMax || arPoint.x < kWorldMin || arPoint.y > kWorldMax || arPoint.y < kWorldMin) {
        NiPoint3 kAngle = NiPoint3::ZERO;
        TESForm* pLocationForm = nullptr;
        GetEditorLocation(kStartingPos, kAngle, pLocationForm, nullptr);
        kStartingPos.z = kStartingPos.z + kRaise;
        bInvalidPos = true;
    }

    TESWorldSpace* pWorldSpace = nullptr;
    if (GetRefPersists() && TESWorldSpace::IsFixedRef(this)) {
        pWorldSpace = GetWorldSpace();
        if (pWorldSpace) {
            pWorldSpace->RemoveFromPersistentRefData(this);
        }
    }
    if (!bInvalidPos) {
        data.kPosition = arPoint;
    }
    else {
        data.kPosition = kStartingPos;
        if (Get3D()) {
            Get3D()->SetTranslate(kStartingPos);
            bhkNiCollisionObject::ResetSim(pLoadedData->m_spData3D, true);
            pLoadedData->m_spData3D->UpdateProperties();
            NiUpdateData kUpdateData(0.0f, false, false);
            pLoadedData->m_spData3D->Update(kUpdateData);
        }
    }
    if (pWorldSpace) {
        pWorldSpace->AddToPersistentRefData(this);
    }
    AddChange(CHANGE_REFR_MOVE);
}

// A nonpersistent reference can only return to a loaded cell.
bool TESObjectREFR::MoveToEditorLocation(TESForm* apLocationForm) {
    NiPoint3 OriginalLocation;
    NiPoint3 OriginalAngle;
    TESForm* pLocation = nullptr;
    TESObjectREFR* pRef = this;
    if (!pRef->GetEditorLocation(OriginalLocation, OriginalAngle, pLocation, apLocationForm)) {
        return false;
    }
    TESObjectCELL* pCell = nullptr;
    TESWorldSpace* pWorld = nullptr;
    GetLocationCellOrWorld(pLocation, pCell, pWorld);
    if (pWorld) {
        pCell = pWorld->GetCellFromWorldCoord(OriginalLocation);
    }
    if (!GetRefPersists() && (!pCell || !TES::pInstance->IsCellLoaded(pCell, false))) {
        return false;
    }
    SetLocationOnReference(OriginalLocation);
    SetAngleOnReference(OriginalAngle);
    if (pCell && pCell->GetInterior()) {
        MoveRefToNewSpace(this, pCell, nullptr);
    }
    else {
        MoveRefToNewSpace(this, nullptr, pWorld);
    }
    Update3DPosition();
    return true;
}

void TESObjectREFR::Update3DPosition() {
    NiMatrix3 kRotation;
    void* pCharController;
    NiNode* p3D = TESObjectREFR::Get3D();
    if (p3D) {
        if (IsMobileObject()) {
            pCharController = static_cast<MobileObject*>(this)->GetCharController();
            if (pCharController) {
                static_cast<bhkCharacterController*>(pCharController)->SetPosition(data.kPosition);
            }
        }
        p3D->SetTranslate(data.kPosition);
        kRotation = GetOrientation();
        p3D->SetRotate(kRotation);
        bhkWorld::UpdatePosition(p3D, true, false);
        NiUpdateData kUpdateData(0.0, false, false);
        p3D->Update(kUpdateData);
    }
}

void TESObjectREFR::CreateLoadedData() {
    if (pLoadedData) {
        return;
    }
    pLoadedData = new LOADED_REF_DATA;
    pLoadedData->pCurrentWaterObject = nullptr;
    pLoadedData->iUnderwaterCount = 0;
    TESObjectCELL* pCell = GetParentCell();
    float fWaterHeight;
    if (pCell && pCell->HasWater()) {
        fWaterHeight = pCell->GetWaterHeight();
    }
    else {
        fWaterHeight = -std::numeric_limits<float>::max();
    }
    pLoadedData->fRelevantWaterHeight = fWaterHeight;
    pLoadedData->fCachedRadius = -1.0f;
    pLoadedData->iFlags = 0;
    pLoadedData->m_spData3D = nullptr;
    pLoadedData->spPhantom = nullptr;
}

// The world's trap listener is the bhkTrapListener base of a TESTrapListener, so the pointer is adjusted back to the
// TESTrapListener after its null test.
void TESObjectREFR::CleanUpTraps() {
    bhkWorld* pw;
    TESObjectCELL* pcell = GetParentCell();
    if (pcell) {
        pw = pcell->GetbhkWorld();
    }
    else {
        pw = nullptr;
    }
    if (pw) {
        TESTrapListener* ptrapListener = static_cast<TESTrapListener*>(pw->pTrapListener);
        if (ptrapListener) {
            ptrapListener->RemoveZone(this);
            ptrapListener->RemoveTarget(this);
            ptrapListener->ClearCurrentRefs(this);
        }
    }
}

void TESObjectREFR::SetDroppedRef(bool abVal) {
    if (GetObjectReference() && GetObjectReference()->GetFormType() != DOOR_ID) {
        int32_t iNewFlags;
        int32_t iflags = iFormFlags;
        if (abVal) {
            iNewFlags = iflags | kFormFlags_DroppedRef;
        }
        else {
            iNewFlags = iflags & ~kFormFlags_DroppedRef;
        }
        iFormFlags = iNewFlags;
    }
}

// Gives a placeable water reference its Havok water volume in the cell's world: automatic water gets a fixed box (2048
// x 2048 x 500 half extents) centred on the cell at the water height, any other the shape of its collision object
// (unless that is already a bounding volume shape); either is wrapped with the water phantom callback shape into a
// rigid body, made dynamic for automatic water, and added to the world. The creation infos and the Havok vectors are
// 16-byte aligned.
void TESObjectREFR::InitHavokForPlaceableWater() {
    bhkWorld* pWorld = GetParentCell() ? GetParentCell()->GetbhkWorld() : nullptr;
    if (!pWorld || !GetObjectReference()) {
        return;
    }
    if (static_cast<BGSPlaceableWater*>(GetObjectReference())->IsAutomaticWater()) {
        // Fetched and never used.
        GetBoundMin();
        GetBoundMax();
        NiPoint3 kExtents;
        ::new (&kExtents) NiPoint3(2048.0f, 2048.0f, 500.0f);
        alignas(16) hkVector4 kHalfExtents;
        NI2HK(kHalfExtents, kExtents);
        void* pMemory = hkReferencedObject::newObject(sizeof(hkpBoxShape));
        // Passes the default convex radius global, not a constant.
        hkpBoxShape* pBox = pMemory ? ::new (pMemory) hkpBoxShape(kHalfExtents, hkConvexShapeDefaultRadius) : nullptr;
        pMemory = hkReferencedObject::newObject(sizeof(bhkWaterPhantomCallbackShape));
        bhkWaterPhantomCallbackShape* pPhantom = pMemory ? ::new (pMemory) bhkWaterPhantomCallbackShape : nullptr;
        pMemory = hkReferencedObject::newObject(sizeof(hkpBvShape));
        hkpBvShape* pBvShape = pMemory ? ::new (pMemory) hkpBvShape(pBox, pPhantom) : nullptr;
        bhkRigidBodyCinfo kInfo;
        kInfo.m_shape = pBvShape;
        kInfo.SetMotionType(hkpMotion::MOTION_FIXED);
        alignas(16) hkVector4 kPosition;
        alignas(16) hkQuaternion kRotation;
        kRotation.setIdentity();
        NiPoint3 kCellCenter;
        kCellCenter.x = static_cast<float>(GetParentCell()->GetDataX() * 4096 + 2048);
        kCellCenter.y = static_cast<float>(GetParentCell()->GetDataY() * 4096 + 2048);
        // Subtracts a double 500.0 and stores the result as a float.
        kCellCenter.z = static_cast<float>(GetParentCell()->GetWaterHeight() - 500.0);
        NI2HK(kPosition, kCellCenter);
        kInfo.SetPosition(kPosition);
        kInfo.SetRotation(kRotation);
        kInfo.m_collisionFilterInfo = 0x50000 | L_WATER;
        bhkRigidBody* pBody = new bhkRigidBody(kInfo);
        bhkCollisionObject* pCollisionObject = new bhkCollisionObject(Get3D());
        const NiFixedString* pTag = &BSXFlags::Tag();
        BSXFlags* pFlags = static_cast<BSXFlags*>(Get3D()->GetExtraData(*pTag));
        if (!pFlags) {
            pFlags = new BSXFlags;
            Get3D()->AddExtraData(pFlags);
        }
        pFlags->SetFlags(BSXFlags::HAVOK, true);
        pCollisionObject->SetWorldObject(pBody);
        pBody->SetAVObject(Get3D(), 0);
        pBody->SetMotionType(hkpMotion::MOTION_DYNAMIC);
        pWorld->bhkWorld::Add(pBody);
        return;
    }
    bhkNiCollisionObject* pCollisionObject = bhkNiCollisionObject::GetbhkNiCollisionObject(Get3D());
    if (!pCollisionObject) {
        Warning("No collision object for placeable water object %08X.", GetFormID());
        return;
    }
    const hkpShape* pShape = pCollisionObject->GetWorldObject()->GetCollidable()->hkpCdBody::getShape();
    if (!pShape) {
        Warning("No shape for placeable water object %08X's collision object.", GetFormID());
        return;
    }
    if (pShape->getType() == HK_SHAPE_BV) {
        return;
    }
    void* pMemory = hkReferencedObject::newObject(sizeof(bhkWaterPhantomCallbackShape));
    bhkWaterPhantomCallbackShape* pPhantom = pMemory ? ::new (pMemory) bhkWaterPhantomCallbackShape : nullptr;
    pMemory = hkReferencedObject::newObject(sizeof(hkpBvShape));
    hkpBvShape* pBvShape = pMemory ? ::new (pMemory) hkpBvShape(pShape, pPhantom) : nullptr;
    bhkRigidBodyCinfo kInfo;
    kInfo.m_shape = pBvShape;
    kInfo.SetMotionType(hkpMotion::MOTION_FIXED);
    alignas(16) hkVector4 kPosition;
    alignas(16) NiQuaternion kNiRotation;
    alignas(16) hkQuaternion kRotation;
    Get3D()->GetRotate(kNiRotation);
    NI2HK(kPosition, GetLocationOnReference());
    NI2HKMIGRATION_QUATERNION(kRotation, kNiRotation);
    kInfo.SetPosition(kPosition);
    kInfo.SetRotation(kRotation);
    kInfo.m_collisionFilterInfo = 0x50000 | L_WATER;
    bhkRigidBody* pBody = new bhkRigidBody(kInfo);
    bhkCollisionObject* pNewCollisionObject = new bhkCollisionObject(Get3D());
    const NiFixedString* pTag = &BSXFlags::Tag();
    BSXFlags* pFlags = static_cast<BSXFlags*>(Get3D()->GetExtraData(*pTag));
    if (!pFlags) {
        pFlags = new BSXFlags;
        Get3D()->AddExtraData(pFlags);
    }
    pFlags->SetFlags(BSXFlags::HAVOK, true);
    pNewCollisionObject->SetWorldObject(pBody);
    pBody->SetAVObject(Get3D(), 0);
    pWorld->bhkWorld::Add(pBody);
}

void TESObjectREFR::InitHavokForCollisionObject() {
    NiPoint3 kRadii;
    NiMatrix3 kRotation;
    hkVector4 hkvec;
    bhkWorld* pw;
    BGSPrimitive* pprim = m_Extra.GetPrimitive();
    if (this->GetParentCell()) {
        pw = this->GetParentCell()->GetbhkWorld();
    }
    else {
        pw = nullptr;
    }
    if (pw && pprim) {
        alignas(16) NiQuaternion nirot;
        hkVector4 hkpos;
        bhkRigidBody* prb;
        BSXFlags* pflags;
        COL_LAYER elayer;
        bhkShape* pshape = nullptr;
        hkTransform transform;
        this->SetScale(1.0f);
        this->Get3D()->SetScale(1.0f);
        NiUpdateData kUpdateData(0.0f, false, false);
        this->Get3D()->Update(kUpdateData);
        alignas(16) NiTransform trn;
        trn.m_Rotate = *::new (&kRotation) NiMatrix3(this->GetOrientation());
        trn.m_Translate = this->GetLocationOnReference();
        trn.m_fScale = 1.0f;
        switch (pprim->eType) {
            case 1u: {
                NiPoint3* pRadii = ::new (&kRadii) NiPoint3(pprim->GetRadii());
                NI2HK(hkvec, *pRadii);
                pshape = new bhkBoxShape(hkvec);
                break;
            }
            case 2u: {
                pshape = new bhkSphereShape(NI2HK(pprim->GetRadii().x), false);
                break;
            }
            case 3u: {
                NiPoint3 halfextents = pprim->GetRadii();
                halfextents.y = 0.01f;
                NI2HK(hkvec, halfextents);
                pshape = new bhkBoxShape(hkvec);
                break;
            }
        }
        bhkRigidBodyCinfo info;
        info.SetShape(pshape);
        uint32_t ifilterset = 0x30000;
        CollisionData* pdata = this->m_Extra.GetCollisionData();
        if (pdata) {
            elayer = pdata->eLayer;
        }
        else {
            elayer = L_TRANSPARENT;
        }
        ifilterset |= elayer;
        info.SetCollisionFilter(ifilterset);
        info.SetMotionType(hkpMotion::MOTION_FIXED);
        pflags = new BSXFlags;
        pflags->SetFlags(BSXFlags::HAVOK, true);
        this->Get3D()->AddExtraData(pflags);
        prb = new bhkRigidBody(info);
        hkQuaternion hkrot;
        this->Get3D()->GetRotate(nirot);
        NI2HK(hkpos, this->GetLocationOnReference());
        NI2HKMIGRATION_QUATERNION(hkrot, nirot);
        prb->SetPosition(hkpos);
        prb->SetRotation(hkrot);
        prb->SetAVObject(this->Get3D(), 0);
        prb->Add(pw);
        bhkCollisionObject* pcoll = new bhkCollisionObject(this->Get3D());
        pcoll->SetWorldObject(prb);
    }
}

void TESObjectREFR::InitHavokForAcousticSpace() {
    NiPoint3 kRadii;
    NiMatrix3 kRotation;
    hkVector4 hkvec;
    bhkWorld* pw;
    BGSPrimitive* pprim = m_Extra.GetPrimitive();
    if (this->GetParentCell()) {
        pw = this->GetParentCell()->GetbhkWorld();
    }
    else {
        pw = nullptr;
    }
    if (pw) {
        BSXFlags* pflags;
        bhkRigidBody* rigidBody;
        alignas(16) NiQuaternion quat;
        hkVector4 hkpos;
        hkpBvShape* bvShape;
        bhkAcousticSpacePhantomCallbackShape* phantomShape;
        hkpShape* pshape = nullptr;
        hkTransform transform;
        this->SetScale(1.0f);
        this->Get3D()->SetScale(1.0f);
        NiUpdateData kUpdateData(0.0f, false, false);
        this->Get3D()->Update(kUpdateData);
        alignas(16) NiTransform trn;
        trn.m_Rotate = (kRotation = this->GetOrientation());
        trn.m_Translate = this->GetLocationOnReference();
        trn.m_fScale = 1.0f;
        NiPoint3* pRadii = &(kRadii = pprim->GetRadii());
        NI2HK(hkvec, *pRadii);
        hkpBoxShape* pBoxMemory = static_cast<hkpBoxShape*>(hkReferencedObject::newObject(sizeof(hkpBoxShape)));
        if (pBoxMemory) {
            pshape = ::new (pBoxMemory) hkpBoxShape(hkvec, hkConvexShapeDefaultRadius);
        }
        else {
            pshape = nullptr;
        }
        bhkAcousticSpacePhantomCallbackShape* pPhantomMemory = static_cast<bhkAcousticSpacePhantomCallbackShape*>(
            hkReferencedObject::newObject(sizeof(bhkAcousticSpacePhantomCallbackShape))
        );
        if (pPhantomMemory) {
            phantomShape = ::new (pPhantomMemory) bhkAcousticSpacePhantomCallbackShape;
        }
        else {
            phantomShape = nullptr;
        }
        hkpShape* pCallbackShape = phantomShape;
        hkpBvShape* pBvMemory = static_cast<hkpBvShape*>(hkReferencedObject::newObject(sizeof(hkpBvShape)));
        if (pBvMemory) {
            bvShape = ::new (pBvMemory) hkpBvShape(pshape, pCallbackShape);
        }
        else {
            bvShape = nullptr;
        }
        hkpShape* pBodyShape = bvShape;
        bhkRigidBodyCinfo info;
        info.m_shape = pBodyShape;
        info.SetMotionType(hkpMotion::MOTION_FIXED);
        hkQuaternion hkquat;
        this->Get3D()->GetRotate(quat);
        NI2HK(hkpos, this->GetLocationOnReference());
        NI2HKMIGRATION_QUATERNION(hkquat, quat);
        info.SetPosition(hkpos);
        info.SetRotation(hkquat);
        info.m_collisionFilterInfo = 0x50015;
        rigidBody = new bhkRigidBody(info);
        bhkNiCollisionObject* pcollobj = new bhkCollisionObject(this->Get3D());
        pflags = new BSXFlags;
        pflags->SetFlags(BSXFlags::HAVOK, true);
        this->Get3D()->AddExtraData(pflags);
        pcollobj->SetWorldObject(rigidBody);
        rigidBody->SetAVObject(this->Get3D(), 0);
        pw->Add(rigidBody);
    }
}

// A box primitive with no rotation outside the projectile zone layer becomes an aabb phantom, any other a simple shape
// phantom; a primitive that is neither box nor sphere hands Gethk a null shape. The aabb phantom goes
// into the reference's bhkSimpleShapePhantom pointer. kTransform is 16-byte aligned.
void TESObjectREFR::InitHavokForPrimitiveTrigger() {
    BGSPrimitive* pPrimitive = GetExtra()->GetPrimitive();
    bhkWorld* pHavokWorld = GetParentCell() ? GetParentCell()->GetbhkWorld() : nullptr;
    if (!pHavokWorld) {
        return;
    }
    bhkShape* pShape = nullptr;
    // Constructed and never used.
    hkTransform kHavokTransform;
    hkVector4 kExtents;
    SetScale(1.0f);
    Get3D()->SetScale(1.0f);
    NiUpdateData kUpdateData(0.0f, false, false);
    Get3D()->Update(kUpdateData);
    alignas(16) NiTransform kTransform;
    kTransform.m_Rotate = GetOrientation();
    kTransform.m_Translate = GetLocationOnReference();
    kTransform.m_fScale = 1.0f;
    CollisionData* pCollisionData = GetExtra()->GetCollisionData();
    COL_LAYER eLayer = pCollisionData ? pCollisionData->QLayer() : L_ACTORZONE;
    if (eLayer != L_PROJECTILEZONE &&
        pPrimitive->GetType() == BGSPrimitive::PRIMITIVE_BOX &&
        kTransform.m_Rotate == NiMatrix3::IDENTITY) {
        // Raw storage: the implicit constructor would call hkAabb's constructor, which the game does not do.
        alignas(hkAabbPhantomCinfo) uint8_t aInfoStorage[sizeof(hkAabbPhantomCinfo)];
        hkAabbPhantomCinfo& kInfo = *reinterpret_cast<hkAabbPhantomCinfo*>(aInfoStorage);
        ::new (aInfoStorage) hkAabbPhantomCinfo;
        hkVector4 kHavokPoint;
        NiPoint3 kPoint;
        kPoint = pPrimitive->GetRadii() + kTransform.m_Translate;
        NI2HK(kHavokPoint, kPoint);
        kInfo.aabb.m_max = kHavokPoint;
        // The second corner's sum is built in a 16-aligned local and then copied.
        alignas(16) NiPoint3 kNegatedPoint = pPrimitive->GetRadii() * -1.0f + kTransform.m_Translate;
        kPoint = kNegatedPoint;
        NI2HK(kHavokPoint, kPoint);
        kInfo.aabb.m_min = kHavokPoint;
        kInfo.m_broadPhaseType = hkpWorldObject::BROAD_PHASE_PHANTOM;
        kInfo.m_collisionFilterInfo = 0x50000 | eLayer;
        bhkAabbPhantom* pPhantom = new bhkAabbPhantom(kInfo);
        pLoadedData->spPhantom = static_cast<bhkSimpleShapePhantom*>(static_cast<bhkPhantom*>(pPhantom));
        kInfo.~hkAabbPhantomCinfo();
    }
    else {
        if (pPrimitive->GetType() == BGSPrimitive::PRIMITIVE_BOX) {
            NI2HK(kExtents, pPrimitive->GetRadii());
            pShape = new bhkBoxShape(kExtents);
        }
        if (pPrimitive->GetType() == BGSPrimitive::PRIMITIVE_SPHERE) {
            pShape = new bhkSphereShape(NI2HK(pPrimitive->GetRadii().x), false);
        }
        hkSimpleShapePhantomCinfo kInfo;
        kInfo.m_shape = pShape->Gethk();
        kInfo.m_broadPhaseType = hkpWorldObject::BROAD_PHASE_PHANTOM;
        kInfo.m_collisionFilterInfo = 0x50000 | eLayer;
        bhkSimpleShapePhantom* pPhantom = new bhkSimpleShapePhantom(kInfo);
        pPhantom->SetTransform(kTransform);
        pLoadedData->spPhantom = pPhantom;
    }
    new bhkCollisionObject(Get3D());
    BSXFlags* pFlags = new BSXFlags;
    pFlags->SetFlags(BSXFlags::HAVOK, true);
    Get3D()->AddExtraData(pFlags);
    NiNode* p3D = Get3D();
    pLoadedData->spPhantom->SetAVObject(p3D, 0);
    pHavokWorld->Add(pLoadedData->spPhantom);
}

// The body is compiled out in the game: it always returns false.
bool TESObjectREFR::CheckAndAddHavokChangeFlag(NiAVObject* apRoot) {
    return false;
}

void TESObjectREFR::RestoreRagDollData(RagDollData* pragdata) {
    // Get3D is called directly here, not through the vtable.
    if (TESObjectREFR::Get3D()) {
        if (!pragdata) {
            pragdata = GetExtra()->GetRagDollData();
        }
        if (pragdata) {
            pragdata->UpdateReferenceFromData(this);
        }
    }
    else if (pragdata) {
        GetExtra()->SetRagDollDataFromCopy(pragdata);
    }
}

void TESObjectREFR::SetLastFinishedSequence(const char* apSequenceName) {
    if (pSaveLoadGameOLD->GetLoading() && GetExtra()->GetLastFinishedSequence()) {
        return;
    }
    GetExtra()->SetLastFinishedSequence(apSequenceName);
}

bool TESObjectREFR::GetIsOfType(TESBoundObject* apObj) {
    TESBoundObject* pLeveledCreature = static_cast<TESBoundObject*>(
        TESObjectREFR::GetExtra()->GetLevCreaOriginalBase()
    );
    TESBoundObject* pLeveledCreatureBase = static_cast<TESBoundObject*>(
        TESObjectREFR::GetExtra()->GetLevCreaTemplate()
    );
    return TESObjectREFR::GetObjectReference() == apObj || pLeveledCreature == apObj || pLeveledCreatureBase == apObj;
}

void TESObjectREFR::SetCenterOnCreation(bool abVal) {
    if (!IsActor()) {
        uint32_t iFlags = TESForm::GetFormFlags();
        if (abVal) {
            iFlags |= kFormFlags_CenterRefOnCreation;
        }
        else {
            iFlags &= ~kFormFlags_CenterRefOnCreation;
        }
        TESForm::SetFormFlags(iFlags);
    }
}

bool TESObjectREFR::GetTargeted() {
    return (TESForm::GetFormFlags() & kFormFlags_Targeted) != 0;
}

BGSEncounterZone* TESObjectREFR::GetLocationEncounterZone() {
    if (pParentCell) {
        return pParentCell->GetEncounterZone();
    }
    TESWorldSpace* worldSpace = GetWorldSpace();
    if (worldSpace) {
        return worldSpace->GetEncounterZone();
    }

    return nullptr;
}

void TESObjectREFR::FreeMultiBound() {
    ExtraDataList* extraDataList = TESObjectREFR::GetExtra();
    extraDataList->SetMultiBound(nullptr);
}

TESObjectREFR* TESObjectREFR::GetRandomTeleportMarker() {
    return TESObjectREFR::GetExtra()->GetRandomTeleportMarker();
}

void TESObjectREFR::SetLinkedRef(TESObjectREFR* apRef) {
    ExtraDataList* extraDataList;
    if (!apRef || apRef->GetRefPersists()) {
        extraDataList = TESObjectREFR::GetExtra();
        extraDataList->SetLinkedRef(apRef);
    }
}

TESObjectREFR* TESObjectREFR::GetMerchantContainer() {
    return TESObjectREFR::GetExtra()->GetMerchantContainer();
}

void TESObjectREFR::SetMerchantContainer(TESObjectREFR* apRef) {
    ExtraDataList* extraDataList;
    if (!apRef || apRef->GetRefPersists()) {
        extraDataList = TESObjectREFR::GetExtra();
        extraDataList->SetMerchantContainer(apRef);
    }
}

void TESObjectREFR::SetEnableStateParent(TESObjectREFR* apParent) {
    TESObjectREFR* pEnableStateParent = GetEnableStateParent();
    if (pEnableStateParent) {
        pEnableStateParent->TESObjectREFR::GetExtra()->RemoveEnableStateChild(this);
    }
    if (!apParent || apParent->GetRefPersists()) {
        TESObjectREFR::GetExtra()->SetEnableStateParent(apParent);
    }
}

void TESObjectREFR::SetEnableStateInverse(bool abVal) {
    TESObjectREFR::GetExtra()->SetEnableStateInverse(abVal);
}

float TESObjectREFR::GetRadioStaticPct() {
    RADIO_DATA* pRadioData = TESObjectREFR::GetExtra()->GetRadioData();
    if (pRadioData && !pRadioData->eRangeType) {
        return pRadioData->fStaticPct;
    }

    return 0.0f;
}

uint32_t TESObjectREFR::GetActionFlag() {
    return TESObjectREFR::GetExtra()->GetActionFlag();
}

TESObjectREFR* TESObjectREFR::GetActionRef() {
    return TESObjectREFR::GetExtra()->GetActionRef();
}

bool TESObjectREFR::HasObjectId(uint32_t aId) {
    if (HasContainer()) {
        return InventoryChanges::GetInventoryChanges(this)->HasObjectId(aId);
    }
    return false;
}

bool TESObjectREFR::GetInaccessible() {
    if (!TESObjectREFR::GetObjectReference()) {
        return false;
    }
    if (TESObjectREFR::GetObjectReference()->TESForm::GetFormType() == DOOR_ID) {
        return (TESForm::GetFormFlags() & 256) != 0;
    }
    return false;
}

bool TESObjectREFR::GetIsHiddenDoor() {
    if (!TESObjectREFR::GetObjectReference()) {
        return false;
    }
    if (TESObjectREFR::GetObjectReference()->TESForm::GetFormType() == DOOR_ID) {
        return (TESForm::GetFormFlags() & 0x40u) != 0;
    }
    return false;
}

bool TESObjectREFR::IsCellLoaded() {
    return pParentCell && TES::pInstance->IsCellLoaded(pParentCell, false);
}

TESTopic* TESObjectREFR::GetSayToTopic() {
    return TESObjectREFR::GetExtra()->GetExtraSayToTopic();
}

BSMultiBound* TESObjectREFR::GetMultiBound() {
    if (!IsMultiBoundRef()) {
        return nullptr;
    }
    if (TESForm::GetDelete()) {
        return nullptr;
    }
    BSMultiBound* pMultiBound = TESObjectREFR::GetExtra()->GetMultiBound();
    if (pMultiBound) {
        return pMultiBound;
    }
    pMultiBound = MultiBoundMarkerData::CreateMultiBoundForRef(this);
    TESObjectREFR::GetExtra()->SetMultiBound(pMultiBound);
    return pMultiBound;
}

void TESObjectREFR::SetRandomTeleportMarker(TESObjectREFR* apMarker) {
    if (!apMarker || apMarker->GetRefPersists()) {
        TESObjectREFR::GetExtra()->SetRandomTeleportMarker(apMarker);
    }
}

void TESObjectREFR::MarkAsPickedUp() {
    TESObjectREFR* pItemDropper = TESObjectREFR::GetExtra()->GetItemDropper();
    if (pItemDropper) {
        pItemDropper->TESObjectREFR::GetExtra()->RemoveDroppedItem(this);
    }
    MarkAsDeleted();
}

void TESObjectREFR::AddReferenceItem(TESObjectREFR* apObject, int32_t aiNumber, bool abFlag, bool abWearit) {
    if (HasContainer()) {
        if ((!IsActor() || GetDisabled()) && GetParentCell() && apObject->HasScript() && !HasScript()) {
            GetParentCell()->AddScriptedRef(this);
        }

        if (this == pPlayer) {
            TESChallengeManager::UpdateChallenge(
                CHALLENGE_TYPE_AQUIRE_LIST,
                aiNumber,
                apObject->GetObjectReference(),
                nullptr,
                0,
                0,
                0
            );
            TESChallengeManager::UpdateChallenge(
                CHALLENGE_TYPE_AQUIRE,
                aiNumber,
                apObject->GetObjectReference(),
                nullptr,
                0,
                0,
                0
            );
        }
        Script::SetActionFlag(this, &apObject->m_Extra, SCRIPT_ACTION_ONADD);
        InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(this);
        char cAddObject = 1;
        if (apObject->IsProjectile()) {
            cAddObject = 0;
            if (static_cast<Projectile*>(apObject)->pSecondaryItemChange) {
                ItemChange* pnewChange;
                pnewChange = new ItemChange(static_cast<Projectile*>(apObject)->pSecondaryItemChange);
                if (pnewChange && pnewChange->pExtraObjectList && pnewChange->pExtraObjectList->m_item) {
                    Script::SetActionFlag(this, pnewChange->pExtraObjectList->m_item, SCRIPT_ACTION_ONADD);
                }

                pinventory->AddChangeItem(pnewChange, true);
            }
            else if (static_cast<Projectile*>(apObject)->pWeaponSource) {
                TESBoundObject* paddObject;
                TESObjectWEAP* pweapSource = static_cast<Projectile*>(apObject)->pWeaponSource;
                if (pweapSource->GetProjectile(nullptr) &&
                    pweapSource->GetProjectile(nullptr)->GetDefaultWeaponSource()) {
                    pweapSource = pweapSource->GetProjectile(nullptr)->GetDefaultWeaponSource();
                }

                if (pweapSource->IsThrownWeapon()) {
                    paddObject = pweapSource;
                }
                else {
                    Actor* pActor = IsActor() ? static_cast<Actor*>(this) : nullptr;
                    paddObject = pweapSource->GetCurrentAmmo(pActor);
                }
                if (paddObject) {
                    Script* pscript = TESScriptableForm::GetFormScript(paddObject);
                    if (pscript) {
                        ExtraDataList* pExtra;
                        pExtra = new ExtraDataList;
                        pExtra->SetCount(1);
                        if (pExtra && !pExtra->GetScript()) {
                            TESObjectREFR* pref;
                            pExtra->SetScript(pscript);
                            pExtra->SetScriptLocals(pscript->CloneLocals());
                            pref = new TESObjectREFR;
                            pref->SetTemporary();
                            ScriptLocals* pLocals = pExtra->GetScriptLocals();
                            pscript->Run(pref, pLocals, nullptr, false);
                            if (pref) {
                                delete pref;
                            }
                        }

                        Script::SetActionFlag(this, pExtra, SCRIPT_ACTION_ONADD);
                        pinventory->AddItem(paddObject, pExtra, 1);
                    }
                    else {
                        pinventory->AddItem(paddObject, nullptr, 1);
                    }
                }
            }
        }
        if (cAddObject) {
            pinventory->AddItem(apObject, aiNumber, abFlag, abWearit);
        }
        if (apObject->GetRefPersists()) {
            apObject->m_Extra.SetReferencePointer(this);
            apObject->AddChange(CHANGE_OBJECT_EXTRA_ITEM_DATA);
        }
    }
}

bool TESObjectREFR::IsOwnerEvil(TESForm* apOwner) {
    bool bretval = false;
    TESForm* powner = apOwner ? apOwner : GetOwner();
    if (powner) {
        if (powner->GetFormType() == FACT_ID) {
            if (static_cast<TESFaction*>(powner)->TESFaction::GetIsEvil()) {
                return true;
            }
        }
        else if (powner->GetFormType() == NPC__ID) {
            Actor* pactor = ReferenceProcessLists.GetActorRefInHigh(static_cast<TESNPC*>(powner), true);
            bretval = true;
            if (pactor) {
                ActorValueOwner* pActorValues = static_cast<ActorValueOwner*>(pactor->GetActorBase());
                float fKarma = pActorValues->GetActorFloatValue(ActorValue::KARMA);
                TESActorBaseData::ALIGNMENT ealign = TESActorBaseData::GetAlignmentForKarma(fKarma);
                if (ealign != TESActorBaseData::ALIGNMENT_EVIL) {
                    return ealign == TESActorBaseData::ALIGNMENT_VERY_EVIL;
                }
            }
        }
    }
    return bretval;
}

void TESObjectREFR::AddBoundObjectItem(TESBoundObject* apObject, ExtraDataList* apExtra, int32_t aiNumber) {
    if (HasContainer()) {
        if ((!IsActor() || TESForm::GetDisabled()) &&
            TESObjectREFR::GetParentCell() &&
            TESScriptableForm::GetFormScript(apObject) &&
            !HasScript()) {
            TESObjectREFR::GetParentCell()->AddScriptedRef(this);
        }

        if (static_cast<const void*>(this) == static_cast<const void*>(pPlayer)) {
            TESChallengeManager::UpdateChallenge(CHALLENGE_TYPE_AQUIRE_LIST, aiNumber, apObject, nullptr, 0, 0, 0);
            TESChallengeManager::UpdateChallenge(CHALLENGE_TYPE_AQUIRE, aiNumber, apObject, nullptr, 0, 0, 0);
        }
        Script::SetActionFlag(this, apExtra, SCRIPT_ACTION_ONADD);
        InventoryChanges::GetInventoryChanges(this)->AddItem(apObject, apExtra, aiNumber);
    }
}

void TESObjectREFR::SetLocationOnReferenceZ(float afZ) {
    NiPoint3 kLocation;
    kLocation.x = data.kPosition.x;
    kLocation.y = data.kPosition.y;
    kLocation.z = afZ;
    SetLocationOnReference(kLocation);
}

int32_t TESObjectREFR::GetGoldCount() {
    int32_t objectCount = 0;
    if (!HasContainer()) {
        return objectCount;
    }
    InventoryChanges* inventoryChanges = InventoryChanges::GetInventoryChanges(this);
    TESForm* formByNumericID = TESForm::GetFormByNumericID(15);
    if (formByNumericID) {
        objectCount = inventoryChanges->GetObjectCount(static_cast<TESBoundObject*>(formByNumericID));
    }
    if (!inventoryChanges->IsEmpty()) {
        return objectCount;
    }
    ExtraDataList* extraDataList = TESObjectREFR::GetExtra();
    extraDataList->RemoveContainerChanges();
    return objectCount;
}

int32_t TESObjectREFR::GetCount() {
    return TESObjectREFR::GetExtra()->GetCount();
}

uint32_t TESObjectREFR::GetSayToResponseID() {
    return TESObjectREFR::GetExtra()->GetExtraSayToResponseID();
}

TESTopicInfo* TESObjectREFR::GetSayToTopicInfo() {
    return TESObjectREFR::GetExtra()->GetExtraSayToTopicInfo();
}

TESObjectREFR* TESObjectREFR::GetRelevantWaterType() {
    if (pLoadedData) {
        return pLoadedData->pCurrentWaterObject;
    }

    return nullptr;
}

void TESObjectREFR::SetRelevantWaterType(TESObjectREFR* apWaterRef) {
    if (pLoadedData) {
        pLoadedData->pCurrentWaterObject = apWaterRef;
    }
}

bool TESObjectREFR::GetDismembered(BGSBodyPart::LIMB_ENUM aeLimb) {
    ExtraDismemberedLimbs* pDismemberedLimbs = GetExtra()->GetDismembermentExtra();
    return pDismemberedLimbs && pDismemberedLimbs->Dismembered(aeLimb);
}

void TESObjectREFR::ResetAnimation_Old() {
    NiAVObject* p3D = TESObjectREFR::Get3D();
    NiControllerManager* pManager = nullptr;
    if (p3D) {
        bool bReset = false;
        const char* pLastSequence = m_Extra.GetLastFinishedSequence();
        if (pLastSequence) {
            bReset = BSstricmp(pLastSequence, animGroupInfo[25].pName) == 0;
        }
        if (!bReset && p3D->GetControllers()) {
            NiObject* pControllers = p3D->GetControllers();
            pManager = static_cast<NiControllerManager*>(
                NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pControllers)
            );
            if (pManager) {
                BSAnimGroupSequence* pSequence = static_cast<BSAnimGroupSequence*>(
                    pManager->GetSequenceByName(animGroupInfo[25].pName)
                );
                if (pSequence && pSequence->GetState()) {
                    bReset = true;
                }
            }
        }

        if (!bReset) {
            bReset = HasMorpherController(p3D);
        }
        if (bReset) {
            Set3D(data.pObjectReference->Clone3D(this), true);
            InitHavok();
        }
    }
    NiNode* pNode = TESObjectREFR::Get3D();
    if (pNode && pNode->GetControllers()) {
        NiObject* pControllers = pNode->GetControllers();
        pManager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pControllers)
        );
        if (pManager) {
            BSAnimGroupSequence* pFirstSequence = static_cast<BSAnimGroupSequence*>(
                pManager->GetSequenceByName(animGroupInfo[0].pName)
            );
            BSAnimGroupSequence* pSecondSequence = static_cast<BSAnimGroupSequence*>(
                pManager->GetSequenceByName(animGroupInfo[2].pName)
            );
            pManager->DeactivateAll(0.0f);
            if (pFirstSequence || pSecondSequence) {
                pManager->SetActive(true);
                if (pFirstSequence) {
                    if (!pFirstSequence->GetState()) {
                        pManager->ActivateSequence(pFirstSequence, 0, false, 1.0f, 0.0f, nullptr);
                    }
                    pFirstSequence->SetOffset(-std::numeric_limits<float>::max());
                    float fBeginTime = pFirstSequence->GetBeginKeyTime();
                    NiUpdateData kUpdateData(fBeginTime, true, false);
                    pNode->Update(kUpdateData);
                }
                if (pSecondSequence) {
                    if (!pSecondSequence->GetState()) {
                        pManager->ActivateSequence(pSecondSequence, 0, false, 1.0f, 0.0f, nullptr);
                    }
                    pSecondSequence->SetOffset(-std::numeric_limits<float>::max());
                    float fBeginTime = pSecondSequence->GetBeginKeyTime();
                    NiUpdateData kUpdateData(fBeginTime, true, false);
                    pNode->Update(kUpdateData);
                }
            }
            else {
                pManager->SetActive(true);
                pFirstSequence = static_cast<BSAnimGroupSequence*>(pManager->GetSequenceAt(0));
                if (pFirstSequence) {
                    pManager->ActivateSequence(pFirstSequence, 0, false, 1.0f, 0.0f, nullptr);
                    pFirstSequence->SetOffset(-std::numeric_limits<float>::max());
                    float fBeginTime = pFirstSequence->GetBeginKeyTime();
                    NiUpdateData kUpdateData(fBeginTime, true, false);
                    pNode->Update(kUpdateData);
                }
                pManager->DeactivateAll(0.0f);
                pManager->SetActive(false);
            }
        }
    }

    NiTimeController* pController = pNode ? pNode->GetControllers() : nullptr;
    pManager = static_cast<NiControllerManager*>(
        NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pController)
    );
    if (pManager) {
        pManager->DeactivateAll(0.0f);
    }
}

BGSWorldLocation* TESObjectREFR::GetWorldLocation(BGSWorldLocation* apLocation) {
    TESObjectCELL* pSpace = static_cast<TESObjectCELL*>(GetSpace());
    ::new (apLocation) BGSWorldLocation(GetLocationOnReference(), pSpace);
    return apLocation;
}

void TESObjectREFR::SetLight(NiAVObject* apObj, bool abStateOn) {
    if (!apObj) {
        return;
    }
    NiAVObject* pObject = BSUtilities::GetObjectByName(apObj, "##LightOff");
    if (pObject) {
        pObject->SetAppCulled(abStateOn);
    }
    pObject = BSUtilities::GetObjectByName(apObj, "##LightOn");
    if (pObject) {
        pObject->SetAppCulled(static_cast<uint8_t>(abStateOn) == 0);
    }
}

const NiPoint3& TESObjectREFR::GetLinkedDoorTeleportPosition() {
    DoorTeleportData* pTeleport = GetTeleport();
    TESObjectREFR* pLinkedDoor = nullptr;
    if (pTeleport) {
        pLinkedDoor = pTeleport->GetLinkedDoor();
        if (pLinkedDoor) {
            pTeleport = pLinkedDoor->GetTeleport();
            if (pTeleport) {
                return pTeleport->GetTeleportPosition();
            }
        }
    }
    return NiPoint3::ZERO;
}

void TESObjectREFR::SetLimbRemoved(BGSBodyPart::LIMB_ENUM aeLimb, bool abRemoved) {
    ExtraDismemberedLimbs* pDismembermentExtra = GetExtra()->GetDismembermentExtra();
    if (pDismembermentExtra) {
        pDismembermentExtra->RemoveLimb(aeLimb, abRemoved);
    }
}

void TESObjectREFR::SetOrientation(NiMatrix3 rot) {
    rot.ToEulerAnglesXYZ(data.kRotation.x, data.kRotation.y, data.kRotation.z);
}

void TESObjectREFR::ClearInterpolators() {
    NiControllerManager* controllerManager;
    NiNode* sceneRoot;
    Animation* pAnimation = GetAnimation();
    if (pAnimation) {
        pAnimation->SpecialIdleFree(true, false);
        pAnimation->ClearGroup(AGS_WHOLE_BODY, 0.0);
        controllerManager = pAnimation->GetControllerManager();
        controllerManager->DeactivateAll(0.0);
        sceneRoot = pAnimation->Animation::GetSceneRoot();
        Animation::ClearControllersInterpolators(sceneRoot);
        pAnimation->ClearBlendInterps();
    }
}

bool TESObjectREFR::GetVATStarget() {
    uint8_t bTargettable = 0;
    if (!TESForm::GetDestructible()) {
        return bTargettable;
    }
    TESBoundObject* pObjectReference = TESObjectREFR::GetObjectReference();
    BGSDestructibleObjectForm* pDestructionForm = BGSDestructibleObjectForm::GetDestructionForm(pObjectReference);
    if (pDestructionForm) {
        bTargettable = pDestructionForm->GetTargetableInVATS();
    }
    if (TESForm::GetVATSTargetOverride()) {
        return !bTargettable;
    }
    return bTargettable;
}

MobileObject* TESObjectREFR::AddTalkingActivatorMobileObjectREFR() {
    TESBoundObject* pObjectReference = TESObjectREFR::GetObjectReference();
    return static_cast<BGSTalkingActivator*>(pObjectReference)
        ->AddMobileObjectExtra(static_cast<PlayerCharacter*>(this));
}

float TESObjectREFR::GetWeightInContainer(bool abIsHardcore) {
    float fInventoryWeight = 0.0f;
    if (HasContainer()) {
        InventoryChanges* pContainerChanges = TESObjectREFR::GetExtra()->GetContainerChanges();
        if (pContainerChanges) {
            fInventoryWeight = pContainerChanges->GetInventoryWeight(abIsHardcore);
        }
        else {
            fInventoryWeight = HasContainer()->GetContainerWeight(abIsHardcore);
        }
    }
    if (!IsActor()) {
        return fInventoryWeight;
    }
    if (static_cast<const void*>(this) == static_cast<const void*>(pPlayer)) {
        return pPlayer->GetRockItLauncherWeight() + fInventoryWeight;
    }

    if (fInventoryWeight > static_cast<Actor*>(this)->GetMaxCarryWeight()) {
        return static_cast<Actor*>(this)->GetMaxCarryWeight();
    }

    return fInventoryWeight;
}

bool TESObjectREFR::HasSaidTopicInfo(TESTopicInfo* apTopicInfo) {
    ExtraDataList* pExtra;
    BSSimpleList<SayOnceTopicInfos*>* pIter;
    if (static_cast<MobileObject*>(this)->bTalkingActivator) {
        pExtra = static_cast<MobileObject*>(this)->pTalkingActivator->TESObjectREFR::GetExtra();
    }
    else {
        pExtra = TESObjectREFR::GetExtra();
    }
    ExtraSayTopicInfoOnceADay* pSaidOnceExtra = pExtra->GetExtraSayTopicInfoOnceADayExtra();
    if (pSaidOnceExtra) {
        for (pIter = pSaidOnceExtra->pListofSaidOnceTopicInfos; pIter && pIter->m_item; pIter = pIter->m_pkNext) {
            if (pIter->m_item->pInfo == apTopicInfo) {
                return true;
            }
        }
    }
    return false;
}

bool TESObjectREFR::SetSequencePosition(NiControllerManager* apManager, const char* apSequence, float afPosition) {
    BSAnimGroupSequence* pseq;
    NiAVObject* pobj3D = TESObjectREFR::Get3D();
    if (!apManager || !pobj3D) {
        return false;
    }
    uint32_t isequenceCount = apManager->GetSequenceCount();
    for (uint32_t n = 0;; ++n) {
        if (n >= isequenceCount) {
            return false;
        }
        pseq = static_cast<BSAnimGroupSequence*>(apManager->GetSequenceAt(n));
        if (pseq) {
            const char* pSequenceName = pseq->m_kName.GetHandleString();
            if (!std::strcmp(pSequenceName, apSequence)) {
                break;
            }
        }
    }
    apManager->DeactivateAll(0.0f);
    apManager->SetActive(true);
    if (!pseq->GetState()) {
        apManager->ActivateSequence(pseq, 0, false, 1.0f, 0.0f, nullptr);
    }
    float fkeyTime = afPosition;
    float foffset = afPosition;
    if (!(static_cast<double>(afPosition) < pseq->GetBeginKeyTime())) {
        if (static_cast<double>(afPosition) > pseq->m_fEndKeyTime) {
            fkeyTime = pseq->m_fEndKeyTime;
            foffset = pseq->m_fEndKeyTime;
        }
        pseq->SetOffset(foffset);
    }
    else {
        fkeyTime = pseq->GetBeginKeyTime();
        foffset = -std::numeric_limits<float>::max();
        pseq->SetOffset(foffset);
    }
    bhkNiCollisionObject::ResetSim(pobj3D, true);
    NiUpdateData UpdateData(fkeyTime, true, false);
    pobj3D->Update(UpdateData);
    EndSequence(pseq);
    apManager->DeactivateSequence(pseq, 0.0f);
    apManager->SetActive(false);
    return true;
}

void TESObjectREFR::SetSeed(uint32_t auiSeed) {
    if (GetExtra()) {
        if (!GetObjectReference() || GetObjectReference()->GetFormType() == 37) {
            TESObjectTREE* pTree = static_cast<TESObjectTREE*>(GetObjectReference());
            if (pTree) {
                uint8_t ucIndex = pTree->GetIndexForSeed(auiSeed);
                GetExtra()->SetSeedIndex(ucIndex);
            }
        }
    }
}

// Loads the Havok state of saves before 81: the flags (one byte, by version), the saved bone count (save 24 on), then
// each rigid body's state through the pre-81 per-object callback when the bone count still matches the 3D's;
// otherwise the data is skipped with a warning and an actor is knocked down. The byte locals are filled by
// LoadGameDataOLD.
void TESObjectREFR::LoadHavokDataPreVersion81(uint16_t aiSaveSize) {
    uint8_t cflags = 0;
    if (pSaveLoadGameOLD->GetCurrentVersion() < 22u) {
        bool bsaveVelocities;
        LoadGameDataOLD(&bsaveVelocities, sizeof(bsaveVelocities));
        if (bsaveVelocities) {
            cflags |= HAVOK_SAVE_VELOCITIES;
        }
    }
    NiAVObject* pobj3D = TESObjectREFR::Get3D();
    if (pobj3D) {
        Actor* pactor = nullptr;
        if (IsActor()) {
            pactor = static_cast<Actor*>(this);
        }
        if (pSaveLoadGameOLD->GetCurrentVersion() >= 43u) {
            LoadGameDataOLD(&cflags, sizeof(cflags));
        }
        if (pSaveLoadGameOLD->GetCurrentVersion() >= 22u && pSaveLoadGameOLD->GetCurrentVersion() < 43u) {
            bool bsaveVelocities;
            LoadGameDataOLD(&bsaveVelocities, sizeof(bsaveVelocities));
            if (bsaveVelocities) {
                cflags |= HAVOK_SAVE_VELOCITIES;
            }
        }
        if (pSaveLoadGameOLD->GetCurrentVersion() >= 24u) {
            uint8_t cboneCount;
            LoadGameDataOLD(&cboneCount, sizeof(cboneCount));
            HavokSaveData havokSaveData;
            GetHavokDataSaveSize(&havokSaveData);
            uint8_t ccurrentBoneCount = static_cast<uint8_t>(
                havokSaveData.sActiveBoneCount + havokSaveData.sInactiveBoneCount
            );
            if (ccurrentBoneCount != cboneCount) {
                Warning(
                    "HAVOK: Havok data bone count differs on reference %s %08X.  Original count: %i, Current "
                    "count: "
                    "%i.  This should only happen with art changes.",
                    GetFormEditorID(),
                    iFormID,
                    cboneCount,
                    ccurrentBoneCount
                );
                pSaveLoadGameOLD->AdvanceBuffer(aiSaveSize - 2);
                if (pactor) {
                    if (pactor->pRagdollController) {
                        pactor->pRagdollController->DisableRagdollAnim(true);
                    }
                    bhkBlendController::DoKnockDown(pobj3D, NiPoint3::ZERO, 1, 0.0f, 0);
                }
                return;
            }
        }
        bhkWorldNS::ObjectRecData recData;
        recData.eAction = bhkWorldNS::OA_TOTAL;
        recData.bRecurse = true;
        recData.uDataA[0].p = bhkNiCollisionObject::GetbhkNiCollisionObject(pobj3D);
        recData.uDataA[1].p = this;
        recData.uDataA[2].i = cflags;
        recData.uDataA[3].i = 0;
        bhkWorld::DoObjectRec(pobj3D, recData, &TESObjectREFR::LoadHavokDataForCollisionObjectPreVersion81);
        if (pactor && pactor->IsParalyzed()) {
            bhkRigidBody::SetFixed(pobj3D, 1, 1);
            bhkWorld::SetMotion(pobj3D, hkpMotion::MOTION_DYNAMIC, true, false, true);
            Animation* panim = pactor->GetAnimation();
            if (panim) {
                panim->ClearGroup(AGS_WHOLE_BODY, 0.0f);
                panim->ClearAccumRootTranslate();
            }
        }

        NiUpdateData kUpdateData(0.0f, false, false);
        pobj3D->Update(kUpdateData);
    }
    else {
        Warning(
            "HAVOK: Cannot load Havok data for reference %s %08X because it has no 3D.",
            GetFormEditorID(),
            iFormID
        );
        pSaveLoadGameOLD->AdvanceBuffer(aiSaveSize);
    }
}

void TESObjectREFR::ResetAnimation() {
    NiAVObject* pobj3D = TESObjectREFR::Get3D();
    NiControllerManager* pmanager = nullptr;
    bool bReset = false;
    if (pobj3D) {
        const char* plastFinishedSequence = m_Extra.GetLastFinishedSequence();
        if (plastFinishedSequence) {
            bReset = BSstricmp(plastFinishedSequence, animGroupInfo[25].pName) == 0;
        }
        if (!bReset && pobj3D->GetControllers()) {
            NiObject* pControllers = pobj3D->GetControllers();
            pmanager = static_cast<NiControllerManager*>(
                NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pControllers)
            );
            if (pmanager) {
                NiControllerSequence* punequipSeq = pmanager->GetSequenceByName(animGroupInfo[25].pName);
                if (punequipSeq && punequipSeq->GetState()) {
                    bReset = true;
                }
            }
        }

        if (!bReset) {
            bReset = HasMorpherController(pobj3D);
        }
        if (bReset) {
            NiAVObject* pnew3D = data.pObjectReference->Clone3D(this);
            Set3D(pnew3D, true);
            InitHavok();
            if (pParentCell && TES::pInstance->IsCellLoaded(pParentCell, false)) {
                pParentCell->AttachReference3D(this, false);
            }
        }
    }
    pobj3D = TESObjectREFR::Get3D();
    if (pobj3D && pobj3D->GetControllers()) {
        NiObject* pControllers = pobj3D->GetControllers();
        pmanager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pControllers)
        );
        if (pmanager) {
            for (uint32_t i = 0; i < pmanager->GetSequenceCount(); ++i) {
                NiControllerSequence* pseq = pmanager->GetSequenceAt(i);
                if (pseq && pseq->GetState()) {
                    BGSOpenCloseForm::HandleAnimEndHavok(this, pseq);
                }
            }
            pmanager->DeactivateAll(0.0f);
            pmanager->SetActive(true);
            NiControllerSequence* pfirstSeq = pmanager->GetSequenceAt(0);
            if (pfirstSeq) {
                pmanager->ActivateSequence(pfirstSeq, 0, false, 1.0f, 0.0f, nullptr);
                pfirstSeq->SetOffset(-std::numeric_limits<float>::max());
                float fBeginTime = pfirstSeq->GetBeginKeyTime();
                NiUpdateData kUpdateData(fBeginTime, true, false);
                pobj3D->Update(kUpdateData);
            }
            pmanager->DeactivateAll(0.0f);
            pmanager->SetActive(false);
            NiControllerSequence* pdefaultSeq1 = pmanager->GetSequenceByName(animGroupInfo[0].pName);
            NiControllerSequence* pdefaultSeq2 = pmanager->GetSequenceByName(animGroupInfo[2].pName);
            if (pdefaultSeq1 || pdefaultSeq2) {
                pmanager->SetActive(true);
                if (pdefaultSeq1) {
                    if (!pdefaultSeq1->GetState()) {
                        pmanager->ActivateSequence(pdefaultSeq1, 0, false, 1.0f, 0.0f, nullptr);
                    }
                    pdefaultSeq1->SetOffset(-std::numeric_limits<float>::max());
                    float fBeginTime = pdefaultSeq1->GetBeginKeyTime();
                    NiUpdateData kUpdateData(fBeginTime, true, false);
                    pobj3D->Update(kUpdateData);
                }
                if (pdefaultSeq2) {
                    if (!pdefaultSeq2->GetState()) {
                        pmanager->ActivateSequence(pdefaultSeq2, 0, false, 1.0f, 0.0f, nullptr);
                    }
                    pdefaultSeq2->SetOffset(-std::numeric_limits<float>::max());
                    float fBeginTime = pdefaultSeq2->GetBeginKeyTime();
                    NiUpdateData kUpdateData(fBeginTime, true, false);
                    pobj3D->Update(kUpdateData);
                }
            }
        }
    }

    NiTimeController* pController = pobj3D ? pobj3D->GetControllers() : nullptr;
    pmanager = static_cast<NiControllerManager*>(
        NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pController)
    );
    if (pmanager) {
        pmanager->DeactivateAll(0.0f);
    }
    if (bReset && pobj3D) {
        BSShaderManager::GetShadowSceneNode(0)->AddObject(pobj3D);
    }
}

void TESObjectREFR::SetIsImposter(bool abIsImposter) {
    ExtraSpecialRenderFlags* pOEIExtra = static_cast<ExtraSpecialRenderFlags*>(GetExtraData(146));
    if (abIsImposter && !pOEIExtra) {
        ExtraSpecialRenderFlags* pNewExtra = new ExtraSpecialRenderFlags;
        pOEIExtra = static_cast<ExtraSpecialRenderFlags*>(m_Extra.AddExtra(pNewExtra));
    }
    if (pOEIExtra) {
        uint32_t uiNewFlags;
        uint32_t iflags = pOEIExtra->m_flags;
        if (abIsImposter) {
            uiNewFlags = iflags | ExtraSpecialRenderFlags::SRF_IMPOSTER;
        }
        else {
            uiNewFlags = iflags & ~ExtraSpecialRenderFlags::SRF_IMPOSTER;
        }
        pOEIExtra->m_flags = uiNewFlags;
        if (!uiNewFlags) {
            m_Extra.RemoveExtra(146);
        }
        AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
    }
}

const NiPoint3& TESObjectREFR::GetLinkedDoorTeleportRotation() {
    TESObjectREFR* pLinkedDoor;
    DoorTeleportData* pLinkedTeleport;
    DoorTeleportData* pTeleport = GetTeleport();
    if (pTeleport &&
        (pLinkedDoor = pTeleport->GetLinkedDoor()) != nullptr &&
        (pLinkedTeleport = pLinkedDoor->GetTeleport()) != nullptr) {
        return *&pLinkedTeleport->rotation;
    }

    return NiPoint3::ZERO;
}

LockData* TESObjectREFR::AddLock() {
    LockData* pLock = GetMyLock();
    if (!pLock) {
        pLock = new LockData;
        GetExtra()->SetLockPtr(pLock);
    }
    return pLock;
}

uint32_t TESObjectREFR::GetSeed() {
    if (GetObjectReference()) {
        if (GetObjectReference()->GetFormType() == 37) {
            TESObjectTREE* pTree = static_cast<TESObjectTREE*>(GetObjectReference());
            if (pTree) {
                uint8_t ucSeedIndex = GetExtra()->GetSeedIndex();
                return pTree->GetSeedAtIndex(ucSeedIndex);
            }
        }
    }
    return 0;
}

void TESObjectREFR::InitChildActivates(TESObjectREFR* apActionRef) {
    bool bactive = false;
    for (BSSimpleList<REF_ACTIVATE_DATA*>* plist = m_Extra.GetActivateRefChildren(); plist && !plist->IsEmpty();
         plist = plist->m_pkNext) {
        REF_ACTIVATE_DATA* pdata = plist->m_item;
        pdata->fActivateDelay = pdata->pActivateRef->m_Extra.GetActivateDelay(this);
        if (pdata->fActivateDelay <= 0.0) {
            MobileObject* pActivator;
            if (static_cast<TESObjectACTI*>(GetObjectReference()) == pAshPile1 ||
                static_cast<TESObjectACTI*>(GetObjectReference()) == pAshPile2) {
                pActivator = static_cast<MobileObject*>(apActionRef);
            }
            else {
                pActivator = static_cast<MobileObject*>(this);
            }
            pdata->pActivateRef->Activate(pActivator, false, nullptr, 1);
        }
        else {
            bactive = true;
        }
    }
    if (bactive) {
        GetParentCell()->AddActivatingRef(this);
    }
}

void TESObjectREFR::AddDecalRef(TESObjectREFR* apRef, const NiPoint3& aIntersect, const NiPoint3& aNormal) {
    if (apRef) {
        m_Extra.AddDecalRef(apRef, aIntersect, aNormal);
    }
}

NiPoint3 TESObjectREFR::GetDeltaForAngle(float afAngle) {
    NiMatrix3 kRotation;
    float fAngle = GetAngleOnReference().z + afAngle;
    fAngle = ClampAngle(fAngle);
    NiPoint3 kDirection(0.0f, 1.0f, 0.0f);
    kRotation.MakeZRotation(fAngle);
    kDirection = kRotation * kDirection;
    kDirection.Unitize();
    return kDirection;
}

void TESObjectREFR::SetSpellEffectLight(NiPointLight* pLight) {
    LightRefData* pLightData;
    pLightData = new LightRefData;
    pLightData->spLight = static_cast<NiLight*>(pLight);
    pLightData->fWantDimmer = 1.0f;
    m_Extra.SetSpellEffectLightPtr(pLightData);
}

void TESObjectREFR::SetDismembered(
    BGSBodyPart::LIMB_ENUM aeLimb,
    TESForm* apDeathObject,
    CAUSE_OF_DEATH aeDeath,
    BGSBodyPart::LIMB_ENUM aeLastHitLimb,
    bool abExplodedLimb
) {
    ExtraDismemberedLimbs* pextraLimbs = m_Extra.AddDismembermentExtra();
    if (aeLimb != BGSBodyPart::LIMB_NONE) {
        pextraLimbs->Dismember(this, aeLimb, abExplodedLimb);
    }
    pextraLimbs->eCauseofDeath = aeDeath;
    pextraLimbs->pDeathObject = apDeathObject;
    if (aeLimb == BGSBodyPart::LIMB_NONE) {
        pextraLimbs->eLastHitLimb = aeLastHitLimb;
    }
    else {
        pextraLimbs->eLastHitLimb = aeLimb;
    }
    AddChange(CHANGE_ACTOR_EXTRA_DISMEMBERED_LIMBS);
}

CAUSE_OF_DEATH TESObjectREFR::GetCauseOfDeath() {
    CAUSE_OF_DEATH eCause = CAUSE_OF_DEATH_NONE;
    ExtraDismemberedLimbs* pextraLimbs = m_Extra.GetDismembermentExtra();
    if (pextraLimbs) {
        eCause = pextraLimbs->eCauseofDeath;
    }
    return eCause;
}

ItemChange* TESObjectREFR::GetInventoryItem(int32_t aiIndex, bool abBarter) {
    if (HasContainer()) {
        InventoryChanges* pContainer;
        if (abBarter) {
            pContainer = pDataHandler->GetBarterContainer();
        }
        else {
            pContainer = InventoryChanges::GetInventoryChanges(this);
        }
        return pContainer->GetItembyIndex(aiIndex);
    }
    return nullptr;
}

// Returns apContainer when there is no container, else the iterator; no caller reads the return value.
void* TESObjectREFR::GetInventoryIterator(
    char acBarter,
    InventoryChanges::IterData** apIterator,
    InventoryChanges** apContainer
) {
    TESContainer* pContainer = HasContainer();
    void* pResult = apContainer;
    *apContainer = nullptr;
    *apIterator = nullptr;
    if (pContainer) {
        if (acBarter) {
            *apContainer = pDataHandler->GetBarterContainer();
        }
        else {
            *apContainer = InventoryChanges::GetInventoryChanges(this);
        }
        InventoryChanges::IterData* pIterator = (*apContainer)->GetIterator();
        *apIterator = pIterator;
        pResult = pIterator;
    }
    return pResult;
}

void TESObjectREFR::ShowPrimitive(bool abShow) {
    BGSPrimitive* pprim = m_Extra.GetPrimitive();
    if (pprim && pprim->GetNode()) {
        float fAlpha;
        NiColorA clr = pprim->GetColor();
        if (abShow) {
            fAlpha = 0.2f;
        }
        else {
            fAlpha = 0.0f;
        }
        clr.a = fAlpha;
        pprim->SetColor(clr);
        if (static_cast<TESObjectSTAT*>(GetObjectReference()) != pCollisionMarker && Get3D()) {
            Get3D()->SetAppCulled(!abShow);
        }
    }
}

MobileObject* TESObjectREFR::GetMobileObjectExtra() {
    if (GetExtra()->GetTalkingActorExtra()) {
        return GetExtra()->GetTalkingActorExtra()->pTalkObject;
    }
    return nullptr;
}

void TESObjectREFR::SetInZone(WaterZone* apZone, bool abValue) {
    if (pLoadedData) {
        pLoadedData->pCurrentWaterObject = m_Extra.SetInWaterZone(apZone, abValue);
        if (pLoadedData->pCurrentWaterObject) {
            pLoadedData->fRelevantWaterHeight = pLoadedData->pCurrentWaterObject->GetLocationOnReference().z;
        }
        else if (GetParentCell() && GetParentCell()->HasWater()) {
            pLoadedData->fRelevantWaterHeight = GetParentCell()->GetWaterHeight();
        }
        else {
            pLoadedData->fRelevantWaterHeight = -std::numeric_limits<float>::max();
        }
    }
    if (IsActor()) {
        bhkCharacterController* pcon = static_cast<MobileObject*>(this)->MobileObject::GetCharController();
        if (pcon) {
            pcon->SetWaterHeight(GetRelevantWaterHeight());
        }
    }
}

bool TESObjectREFR::GetIgnoredBySandbox() {
    return GetExtra()->GetIgnoredBySandbox();
}

void TESObjectREFR::SayForceGoodbyeGreetingToPlayer(
    TESTopic* apGreetTopic,
    bool abForceSubtitle,
    bool abForceHeadTrack
) {
    int32_t ioldLipDistance = iSpeakSoundLipDistance.Int();
    iSpeakSoundLipDistance = std::numeric_limits<int32_t>::max();
    Actor* pactor = nullptr;
    if (IsActor()) {
        pactor = static_cast<Actor*>(this);
    }
    if (pactor) {
        if (pactor->pCurrentProcess && !pactor->IsDead(false)) {
            DialogueItem* pVoice = apGreetTopic->CreateDialogueItem(pactor, pPlayer, nullptr, nullptr, true);
            pactor->bSoundFileDone = true;
            pactor->SetDialogueItemTarget(pPlayer);
            if (pactor->GetSitSleepState() == SIT_SLEEP_NORMAL && pactor->pCurrentProcess->GetGreetingFlag()) {
                pactor->EndInterruptPackage(false);
                pactor->pCurrentProcess->CreateFollownoEscort(pactor, true);
            }

            pactor->pCurrentProcess->ProcessGreet(pactor, apGreetTopic, abForceSubtitle, true, true, false);
            if (pVoice) {
                pVoice->FirstResponse();
                pVoice->GetCurrentResponse();
                pVoice->RunResult(TIRS_BEGIN);
                pVoice->RunResult(TIRS_END);
                Script::SetActionFlag(apGreetTopic, &pactor->m_Extra, SCRIPT_ACTION_ONSAYTODONE);
            }
            if (pactor->pCurrentProcess->GetGreetingFlag()) {
                pactor->GetSayToTopic();
                DialogueItem* pItem = pactor->pCurrentProcess->GetSubtitleItem();
                if (pItem) {
                    DialogueResponse* pResponse = pItem->GetCurrentResponse();
                    if (GetIsFullHelp()) {
                        char acSubtitle[268];
                        std::sprintf(acSubtitle, "'%s' :%s", GetFullName(), pResponse->cText.pString);
                        NiPoint3 kSpeakerLoc = GetLocationOnReference();
                        BSSoundHandle kHandle(0u);
                        Interface::ShowSubtitle(acSubtitle, kHandle, kSpeakerLoc, pPlayer, false);
                    }
                    else {
                        NiPoint3 kSpeakerLoc = GetLocationOnReference();
                        BSSoundHandle kHandle(0u);
                        Interface::ShowSubtitle(pResponse->cText.pString, kHandle, kSpeakerLoc, pPlayer, false);
                    }
                    pactor->pCurrentProcess->SetSubtitleItem(nullptr);
                }
            }
        }
    }
    else {
        NiNode* p3DModel = Get3D();
        if (IsMobileObject()) {
            if (!p3DModel) {
                MobileObject* pmobileObj = static_cast<MobileObject*>(this);
                p3DModel = pmobileObj ? pmobileObj->pTalkingActivator->Get3D() : nullptr;
            }
            else {
                SetSayToTopic(apGreetTopic);
                DialogueItem* pVoice = apGreetTopic->CreateDialogueItem(this, nullptr, nullptr, nullptr, false);
                if (pVoice) {
                    SetSayToTopicInfo(pVoice->pInfo);
                }
                BSSoundHandle aSoundHand = VoiceSoundFunction(apGreetTopic, pPlayer, false, false, false, false, false);
                aSoundHand.SetCompletionCallback(
                    &TESObjectREFR::SayToCallBack,
                    reinterpret_cast<void*>(static_cast<uintptr_t>(iFormID))
                );
            }
        }
    }
    SetHasSpokenFlag(true);
    iSpeakSoundLipDistance = ioldLipDistance;
    ReferenceProcessLists.SortActorsCloseToPlayer();
}

// Sets the reference's health: an ExtraHealth equal to the base object's health is removed, else updated; with no
// ExtraHealth, one is added when the value differs from the base's. The base's unsigned health is converted to double
// for each compare.
void TESObjectREFR::SetHealth(float afVal) {
    TESHealthForm* pHealthForm = dynamic_cast<TESHealthForm*>(GetObjectReference());
    if (pHealthForm) {
        ExtraHealth* pExtraHealth = static_cast<ExtraHealth*>(m_Extra.GetExtraData(EXTRA_HEALTH));
        if (pExtraHealth) {
            if (afVal == static_cast<double>(pHealthForm->GetFormHealth())) {
                m_Extra.RemoveExtra(pExtraHealth, true);
            }
            else {
                pExtraHealth->fHealth = afVal;
            }
        }
        else {
            if (afVal != static_cast<double>(pHealthForm->GetFormHealth())) {
                m_Extra.SetHealth(afVal);
            }
        }
    }
}

bool TESObjectREFR::QVerticesInsideMultiboundRecurse(
    NiAVObject* apObject,
    BSMultiBound* apMulti,
    bool& abFoundGeom,
    bool abAllVertices
) {
    if (NiObject::IsExactKindOf(&BGSDecalNode::ms_RTTI, apObject)) {
        return abFoundGeom;
    }
    NiGeometry* pgeom = static_cast<NiGeometry*>(NiObject::DynamicCastStatic(&NiGeometry::ms_RTTI, apObject));
    if (pgeom) {
        abFoundGeom = true;
        NiGeometryData* pdata = pgeom->GetModelData();
        NiTConvertingStrideIterator<NiPoint3> kGeomDataBlock;
        if (!pdata->LockPackedVertexData(true)) {
            return false;
        }
        pdata->GetVerticesIterator(*static_cast<NiGeometryDataIteratorNiPoint3*>(&kGeomDataBlock));
        for (int32_t iIndex = 0; iIndex < pgeom->GetVertexCount(); ++iIndex) {
            NiPoint3 vertexPosition = NiPoint3::ZERO;
            kGeomDataBlock.Get(iIndex, vertexPosition);
            vertexPosition = pgeom->GetWorldTransform() * vertexPosition;
            if (!apMulti->QPointWithin(vertexPosition) && abAllVertices) {
                pdata->UnlockPackedVertexData();
                return false;
            }
        }
        pdata->UnlockPackedVertexData();
        if (!abAllVertices) {
            return false;
        }
    }
    NiNode* pnode = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, apObject));
    if (pnode) {
        for (int32_t i = 0; i < pnode->GetChildCount(); ++i) {
            NiAVObject* pChild = pnode->GetAt(static_cast<uint32_t>(i));
            if (!TESObjectREFR::QVerticesInsideMultiboundRecurse(pChild, apMulti, abFoundGeom, abAllVertices)) {
                return false;
            }
        }
    }
    return abFoundGeom;
}

void TESObjectREFR::RandomizeAddons(NiAVObject* apObj, float afPhase) {
    NiTimeController* pTextureControllers;
    NiAVObject* pChild;
    uint32_t p;
    NiNode* pNode;
    NiTimeController* i;
    NiTimeController* j;
    NiProperty* pTextureProperty;
    if (apObj) {
        for (i = apObj->NiObjectNET::GetControllers(); i; i = i->GetNext()) {
            i->SetPhase(afPhase);
        }
        pTextureProperty = apObj->GetProperty(5);
        if (pTextureProperty) {
            pTextureControllers = pTextureProperty->NiObjectNET::GetControllers();
        }
        else {
            pTextureControllers = nullptr;
        }
        for (j = pTextureControllers; j; j = j->GetNext()) {
            j->SetPhase(afPhase);
        }
        pNode = apObj->IsNode();
        if (pNode) {
            for (p = 0; p < pNode->GetArrayCount(); ++p) {
                pChild = pNode->GetAt(p);
                TESObjectREFR::RandomizeAddons(pChild, afPhase);
            }
        }
    }
}

void TESObjectREFR::SaveControllerManager(
    NiControllerManager* apManager,
    BGSSaveGameBuffer* apSaveGameBuffer,
    float afTime
) {
    if (afTime == -1.0) {
        afTime = fCellAppTime;
    }
    uint32_t uiCount = 0;
    uint32_t uiPosition = apSaveGameBuffer->StartVariableSizedValue();
    if (apManager && apManager->GetActive()) {
        for (uint32_t uiIndex = 0; uiIndex < apManager->GetSequenceCount(); ++uiIndex) {
            BSAnimGroupSequence* pSequence = static_cast<BSAnimGroupSequence*>(apManager->GetSequenceAt(uiIndex));
            if (pSequence && pSequence->GetState()) {
                const char* pName = pSequence->m_kName.GetHandleString();
                apSaveGameBuffer->SaveString(pName, 0);
                pSequence->SaveGame(static_cast<BGSSaveFormBuffer*>(apSaveGameBuffer), afTime);
                ++uiCount;
            }
        }
    }

    apSaveGameBuffer->SaveVariableSizedValue(uiCount, uiPosition);
}

// Saves the manager's playing sequences: a count, filled in last through the buffer position saved before it, then
// each playing sequence's name (a length byte and the text) and state at afTime (-1: the cell's current time).
void TESObjectREFR::SaveControllerManager(NiControllerManager* apManager, float afTime) {
    uint8_t clength;
    uint16_t icount;
    if (afTime == -1.0) {
        afTime = fCellAppTime;
    }
    icount = 0;
    uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&icount, sizeof(icount));
    if (apManager && apManager->GetActive()) {
        for (uint32_t n = 0; n < apManager->GetSequenceCount(); ++n) {
            BSAnimGroupSequence* pseq = static_cast<BSAnimGroupSequence*>(apManager->GetSequenceAt(n));
            if (pseq && pseq->GetState()) {
                const char* pname = pseq->m_kName.GetHandleString();
                clength = BSstrlen(pname);
                pSaveLoadGameOLD->SaveGameDataOLD(&clength, sizeof(clength));
                pname = pseq->m_kName.GetHandleString();
                pSaveLoadGameOLD->SaveGameDataOLD(pname, clength);
                pseq->SaveGame(afTime);
                ++icount;
            }
        }
    }

    *pcount = icount;
}

// Each saved sequence is a name (saves 21 and 22: an anim group index, its group's name or an empty name past the
// table) found among the manager's sequences: an inactive one is activated, its state loaded at afTime, and the 3D's
// particles caught up from afTime - (offset + afTime), at least 0, to afTime in steps of (offset + afTime) / 20, at
// least 0.0166666. The times are float locals; the step's divide is done in double.
void TESObjectREFR::LoadControllerManager(NiControllerManager* apManager, NiAVObject* apObj3D, float afTime) {
    if (afTime == -1.0) {
        afTime = fCellAppTime;
    }
    uint16_t usCount;
    pSaveLoadGameOLD->LoadGameDataOLD(&usCount, sizeof(usCount));
    if (usCount > 65000) {
        usCount = 0;
    }
    int32_t iSequenceCount = 0;
    if (apManager) {
        iSequenceCount = apManager->GetSequenceCount();
        if (usCount) {
            apManager->SetActive(true);
        }
    }
    bool bAnyLoaded = false;
    char acName[0x104];
    for (int32_t i = 0; i < usCount; ++i) {
        if (pSaveLoadGameOLD->GetCurrentVersion() >= 21 && pSaveLoadGameOLD->GetCurrentVersion() < 23) {
            int32_t iGroup;
            pSaveLoadGameOLD->LoadGameDataOLD(&iGroup, sizeof(iGroup));
            if (iGroup < 245) {
                BSstrcpy(acName, 0x104, animGroupInfo[iGroup].pName);
            }
            else {
                ourMemset(acName, 0, 0x104);
            }
        }
        if (pSaveLoadGameOLD->GetCurrentVersion() < 21 || pSaveLoadGameOLD->GetCurrentVersion() >= 23) {
            uint8_t cLength;
            pSaveLoadGameOLD->LoadGameDataOLD(&cLength, sizeof(cLength));
            ourMemset(acName, 0, 0x104);
            pSaveLoadGameOLD->LoadGameDataOLD(acName, cLength);
        }
        bool bFound = false;
        if (apManager) {
            for (uint32_t j = 0; j < static_cast<uint32_t>(iSequenceCount); ++j) {
                NiControllerSequence* pSequence = apManager->GetSequenceAt(static_cast<int32_t>(j));
                if (!pSequence || BSstrcmp(pSequence->GetName().GetHandleString(), acName)) {
                    continue;
                }
                if (pSequence->GetState() == NiControllerSequence::kInactive) {
                    apManager->ActivateSequence(pSequence, 0, false, 1.0f, 0.0f, nullptr);
                }
                static_cast<BSAnimGroupSequence*>(pSequence)->LoadGame(afTime);
                if (apObj3D && HasParticleSystem(apObj3D)) {
                    float fEnd = pSequence->GetOffset() + afTime;
                    float fStart = afTime - fEnd;
                    if (fStart < 0.0) {
                        fStart = 0.0f;
                    }
                    float fStep = static_cast<float>(fEnd / 20.0);
                    if (fStep < 0.016666600480675697) {
                        fStep = 0.0166666f;
                    }
                    for (float fTime = fStart; afTime > fTime; fTime = fTime + fStep) {
                        NiUpdateData kStepData(fTime, false, false);
                        apObj3D->UpdateSelected(kStepData);
                    }
                }
                bFound = true;
                bAnyLoaded = true;
                break;
            }
        }
        if (!bFound) {
            pSaveLoadGameOLD->AdvanceBuffer(BSAnimGroupSequence::GetSequenceSaveSize());
        }
    }
    if (bAnyLoaded && apObj3D) {
        NiUpdateData kUpdateData(afTime, true, false);
        apObj3D->Update(kUpdateData);
    }
}

// The quest object test is the virtual IsQuestObject (slot 0x160).
void TESObjectREFR::SetDroppedItemData(TESObjectREFR* apDropper, TESObjectREFR* apDroppedItem) {
    if (apDroppedItem &&
        apDropper &&
        apDropper != pPlayer &&
        !apDroppedItem->GetRefPersists() &&
        !apDroppedItem->IsQuestObject()) {
        apDropper->GetExtra()->AddDroppedItem(apDroppedItem);
        apDroppedItem->GetExtra()->SetItemDropper(apDropper);
        apDroppedItem->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
    }
}

void TESObjectREFR::SayToCallBack(void* apRef, bool abSucceeded) {
    TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apRef)));
    if (pForm && pForm->IsReference()) {
        TESObjectREFR* pRef = static_cast<TESObjectREFR*>(pForm);
        if (pRef) {
            ExtraSayToTopicInfo* pSayTo = pRef->GetExtra()->GetSayToExtra();
            if (pSayTo) {
                pSayTo->bVoiceDone = true;
            }
        }
    }
}

// The reference whose 3D holds apAVObjectIn: the player for the player's first-person node, else the linked
// reference of the nearest fade node up the parent chain. An ammo reference's node defers to the reference above it
// when there is one. Null when no fade node up the chain has a linked reference.
TESObjectREFR* TESObjectREFR::FindReferenceFor3D(NiAVObject* apAVObjectIn) {
    NiAVObject* pCurrent = apAVObjectIn;
    TESObjectREFR* pRef = nullptr;
    NiAVObject* pPlayerNode = nullptr;
    if (pPlayer) {
        pPlayerNode = pPlayer->Get3D(true);
    }
    while (pCurrent) {
        if (pCurrent == pPlayerNode) {
            return pPlayer;
        }
        BSFadeNode* pFadeNode = pCurrent->IsFadeNode();
        if (pFadeNode) {
            pRef = pFadeNode->GetLinkedObj();
            if (pRef) {
                if (pRef->GetObjectReference() && pRef->GetObjectReference()->GetFormType() == AMMO_ID) {
                    TESObjectREFR* pParentRef = FindReferenceFor3D(pCurrent->GetParent());
                    if (pParentRef) {
                        pRef = pParentRef;
                    }
                }
                return pRef;
            }
        }
        pCurrent = pCurrent->GetParent();
    }
    return pRef;
}

bool TESObjectREFR::HasAddonFlags(NiNode* apNode) {
    NiExtraData* pextraData = apNode->GetExtraData(BSXFlags::Tag());
    if (!pextraData) {
        return false;
    }
    BSXFlags* pbsx = static_cast<BSXFlags*>(pextraData);
    return pbsx->QAddon() != 0;
}

// Stops the moving sounds attached to apNode and to every node under it. Returns whether apNode itself had one.
bool TESObjectREFR::RemoveSounds(NiNode* apNode) {
    bool bResult = false;
    if (!apNode) {
        return false;
    }
    if (apNode->QHasMovingSound()) {
        static_cast<BSWin32Audio&>(BSAudio::QInstance()).StopMovingSounds(apNode, 0.0f, 0);
        bResult = true;
    }
    // The index compare is unsigned.
    for (uint32_t i = 0; i < apNode->GetArrayCount(); ++i) {
        NiAVObject* pChild = apNode->GetAt(i);
        if (pChild) {
            NiNode* pChildNode = pChild->IsNode();
            if (pChildNode) {
                RemoveSounds(pChildNode);
            }
        }
    }
    return bResult;
}

// A value node whose value names an addon node with a model and children: the addon's model is uncloned and the
// children detached, which the result reports. Then recurses into every child node.
bool TESObjectREFR::RemoveAddonNodes(NiNode* apNode) {
    bool bResult = false;
    if (!apNode) {
        return false;
    }
    if (NiObject::IsKindOf_0(&BSValueNode::ms_RTTI, apNode)) {
        bhkWorld::RemoveObjects(apNode, true, true);
        BGSAddonNode* pAddonNode = pDataHandler->GetAddonNode(static_cast<BSValueNode*>(apNode)->iValue);
        if (pAddonNode && pAddonNode->GetModelLength() && apNode->GetChildCount()) {
            pAddonNode->UnClone3D(nullptr);
            apNode->DetachAllChildren();
            bResult = true;
        }
    }
    for (uint32_t i = 0; i < apNode->GetArrayCount(); ++i) {
        NiAVObject* pChild = apNode->GetAt(i);
        if (pChild) {
            NiNode* pChildNode = pChild->IsNode();
            if (pChildNode) {
                RemoveAddonNodes(pChildNode);
            }
        }
    }
    return bResult;
}

uint32_t TESObjectREFR::GetLightingObjectType(NiAVObject* ap3D) {
    ENUM_LOD_MULT eType = LOD_MULT_NONE;
    if (!ap3D) {
        return eType;
    }
    TESObjectREFR* pRef = FindReferenceFor3D(ap3D);
    if (!pRef || !pRef->GetObjectReference()) {
        return eType;
    }
    switch (pRef->GetObjectReference()->GetFormType()) {
        case ACTI_ID:
        case DOOR_ID:
        case STAT_ID:
        case SCOL_ID: eType = LOD_MULT_OBJECTS; break;
        case ARMO_ID:
        case CLOT_ID:
        case WEAP_ID:
        case AMMO_ID: eType = LOD_MULT_TREES; break;
        case CONT_ID:
        case FURN_ID: eType = LOD_MULT_ITEMS; break;
        case INGR_ID:
        case MISC_ID:
        case KEYM_ID:
        case COBJ_ID:
        case IMOD_ID:
        case CHIP_ID:
        case CCRD_ID:
        case CMNY_ID: eType = LOD_MULT_LANDSCAPE; break;
        case NPC__ID:
        case CREA_ID: eType = LOD_MULT_ACTORS; break;
        default: eType = LOD_MULT_DISTANTLOD; break;
    }
    return eType;
}

TESObjectREFR* TESObjectREFR::GetBackgroundLoadingRef() {
    return pBackgroundLoadingRef;
}

uint16_t TESObjectREFR::GetControllerManagerSaveSize(NiControllerManager* apManager) {
    uint16_t usSize = 2;
    if (apManager && apManager->GetActive()) {
        for (uint32_t uiIndex = 0; uiIndex < apManager->GetSequenceCount(); ++uiIndex) {
            BSAnimGroupSequence* pSequence = static_cast<BSAnimGroupSequence*>(apManager->GetSequenceAt(uiIndex));
            if (pSequence && pSequence->GetState()) {
                const char* pName = pSequence->m_kName.GetHandleString();
                int16_t sNameSize = BSstrlen(pName) + (usSize + 1);
                usSize = pSequence->GetSaveSize() + sNameSize;
            }
        }
    }

    return usSize;
}

void TESObjectREFR::MoveRefToNewSpace(TESObjectREFR* apRef, TESObjectCELL* apInterior, TESWorldSpace* apWorld) {
    TESObjectCELL* pTargetCell = nullptr;
    bool bResetNotify = false;
    bhkNiCollisionObject* pCollisionObject = nullptr;
    NiNode* pNode = apRef->Get3D();
    bhkCharacterController* pCharController = nullptr;
    MobileObject* pMobile = nullptr;
    if (apRef->IsMobileObject()) {
        pMobile = static_cast<MobileObject*>(apRef);
        pCharController = static_cast<bhkCharacterController*>(pMobile->GetCharController());
    }
    if (pCharController) {
        pCharController->SetStartingHeight(pMobile->GetLocationOnReference().z);
    }
    if (pNode) {
        pCollisionObject = static_cast<bhkNiCollisionObject*>(pNode->GetCollisionObject());
        if (pCollisionObject && pCollisionObject->QNotify()) {
            pCollisionObject->SetNotify(false);
            bResetNotify = true;
        }
    }
    if (apInterior && !apInterior->GetInterior()) {
        apInterior = nullptr;
    }
    if (apRef && (apInterior || apWorld)) {
        bool bCellLoaded = false;
        bool bDetachHavok = false;
        TESWorldSpace* pCurrWorld = apRef->GetWorldSpace();
        TESObjectCELL* pParentCell = apRef->GetParentCell();
        if (apRef->IsDead(false)) {
            apRef->GetExtra()->RemoveSavedHavokData();
            apRef->SetHavokDeath(false);
        }
        if (apInterior) {
            if (pParentCell == apInterior) {
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    TaskQueueInterface::GetInstance()->QueueHavokMove(apRef);
                }
                else {
                    apRef->MoveHavok(false);
                }
                if (pMobile && pCharController) {
                    pCharController->SetPosition(pMobile->GetLocationOnReference());
                }
            }
            else {
                bDetachHavok = true;
                if (!apRef->IsActor() && !apRef->GetExtra()->GetStartingWorldOrCell()) {
                    apRef->GetExtra()->SetStartingWorldOrCellForRef(apRef);
                }
                if (pCurrWorld && apRef->GetRefPersists()) {
                    pCurrWorld->RemovePersistentRef(apRef);
                }
                apInterior->AddReference(apRef, false);
            }
            if (TES::pInstance->IsCellLoaded(apInterior, true)) {
                bCellLoaded = true;
            }
            pTargetCell = apInterior;
        }
        else {
            if (pParentCell && pParentCell->GetInterior()) {
                bDetachHavok = true;
            }
            if (pCurrWorld != apWorld && !apRef->IsActor() && !apRef->GetExtra()->GetStartingWorldOrCell()) {
                apRef->GetExtra()->SetStartingWorldOrCellForRef(apRef);
            }
            if (apRef->GetRefPersists() && pCurrWorld != apWorld) {
                if (pCurrWorld) {
                    pCurrWorld->RemovePersistentRef(apRef);
                }
                apWorld->AddPersistentRef(apRef);
            }
            int32_t iCellX = FloatToInt(apRef->GetLocationOnReference().x) >> 12;
            int32_t iCellY = FloatToInt(apRef->GetLocationOnReference().y) >> 12;
            pTargetCell = apWorld->GetCellFromCellCoord(iCellX, iCellY);
            if (pTargetCell) {
                pTargetCell->AddReference(apRef, false);
                if (TES::pInstance->IsCellLoaded(pTargetCell, true)) {
                    if (pCurrWorld != apWorld) {
                        bCellLoaded = true;
                        bDetachHavok = true;
                        if (apRef->IsActor()) {
                            MobileObject* pActor = static_cast<MobileObject*>(apRef);
                            if (pActor->GetCurrentAIProcess()) {
                                pActor->GetCurrentAIProcess()->ComputeLastTimeProcessed();
                            }
                        }
                    }
                    else if (bDetachHavok && pParentCell && TES::pInstance->IsCellLoaded(pParentCell, true)) {
                        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                            TaskQueueInterface::GetInstance()->QueueHavokMove(apRef);
                        }
                        else {
                            apRef->MoveHavok(false);
                        }
                    }
                }
            }
            else {
                if (pParentCell) {
                    pParentCell->RemoveReference(apRef);
                }
                if ((!apRef->IsActor() || !pPlayer->IsSleepingorResting()) && !pSaveLoadGameOLD->GetLoading()) {
                    apRef->Set3D(nullptr, false);
                }
            }
        }
        if (pMobile && !bCellLoaded) {
            ReferenceProcessLists.AddActorToTempChangeList(static_cast<Actor*>(pMobile));
            if (pParentCell &&
                !pTargetCell &&
                pMobile->GetDesiredProcessLevel() == PROCESS_TYPE_LOW &&
                !pMobile->GetRefPersists()) {
                pMobile->SetParentCell(pParentCell);
                pMobile->AddChange(CHANGE_REFR_CELL_CHANGED);
                pSaveLoadGameOLD->UnloadForm(pMobile);
                pSaveLoadGame->UnloadForm(pMobile, false);
                pMobile->SetParentCell(nullptr);
            }
        }
        if (bDetachHavok) {
            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                TaskQueueInterface::GetInstance()->QueueHavokDetach(apRef);
            }
            else {
                apRef->DetachHavok();
            }
        }
        if (bCellLoaded) {
            bool bInitHavok = false;
            if (apRef->IsMobileObject() && apRef->Get3D() && pTargetCell->GetAttached()) {
                BaseProcess* pProcess = pMobile->GetCurrentAIProcess();
                if (pProcess &&
                    (pProcess->GetProcessLevel() == PROCESS_TYPE_MIDDLE_HIGH ||
                     pProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) &&
                    !pProcess->GetCharController()) {
                    bInitHavok = true;
                }
            }
            if (bInitHavok) {
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    TaskQueueInterface::GetInstance()->QueueHavokInit(apRef);
                }
                else {
                    apRef->InitHavok();
                }
            }
            else if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                TaskQueueInterface::GetInstance()->QueueHavokMove(apRef);
            }
            else {
                apRef->MoveHavok(false);
            }
        }
        if (apRef->GetParentCell() != pParentCell && !apRef->GetRefPersists() && apRef != pPlayer) {
            apRef->AddChange(CHANGE_REFR_CELL_CHANGED);
        }
    }
    if (bResetNotify && apRef->Get3D()) {
        pCollisionObject->SetNotify(true);
    }
}

// A new reference of form type aeType, built through the constructor of its class, with a process when abadd; null for
// any other type. AutoMemContext as in InitScript. TACT_ID builds a MobileObject.
TESObjectREFR* TESObjectREFR::CreateReference(uint8_t aeType, bool abadd) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_TES_REFERENCES,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectREFR.cpp",
        5267
    );
    TESObjectREFR* pretref = nullptr;
    switch (aeType) {
        case TACT_ID: {
            pretref = new MobileObject;
            break;
        }
        case REFR_ID: {
            pretref = new TESObjectREFR;
            break;
        }
        case ACHR_ID: {
            pretref = new Character(abadd);
            break;
        }
        case ACRE_ID: {
            pretref = new Creature(abadd);
            break;
        }
        case PMIS_ID: {
            pretref = new MissileProjectile(abadd);
            break;
        }
        case PGRE_ID: {
            pretref = new GrenadeProjectile(abadd);
            break;
        }
        case PBEA_ID: {
            pretref = new BeamProjectile(abadd);
            break;
        }
        case PFLA_ID: {
            pretref = new FlameProjectile(abadd);
            break;
        }
        case PCBE_ID: {
            pretref = new ContinuousBeamProjectile(abadd);
            break;
        }
        default: break;
    }
    pContext->~AutoMemContext();
    return pretref;
}

// Releases the culling of the BSValueNodes under apNode whose addon node is a master particle system and adds them
// to the master particle system manager; true when one was added. Culled nodes are not searched below.
bool TESObjectREFR::AddMasterParticleAddonNodes(NiNode* apNode) {
    if (!apNode) {
        return false;
    }
    bool bAdded = false;
    bool bValueNode;
    bValueNode = NiObject::IsKindOf(&BSValueNode::ms_RTTI, apNode);
    if (bValueNode) {
        BSValueNode* pValueNode = static_cast<BSValueNode*>(apNode);
        BGSAddonNode* pAddonNode = pDataHandler->GetAddonNode(pValueNode->QValue());
        if (pAddonNode && pAddonNode->IsMasterParticleSystem()) {
            pValueNode->SetAppCulled(false);
            uint32_t uiIndex = pAddonNode->GetMasterParticleSystemIndex();
            Main::GetMasterParticleSystemManager()->AddEmitter(uiIndex, pValueNode);
            bAdded = true;
        }
    }
    if (apNode->GetAppCulled()) {
        return bAdded;
    }
    for (uint32_t i = 0; i < apNode->GetArrayCount(); ++i) {
        NiAVObject* pChild = apNode->GetAt(i);
        if (pChild) {
            NiNode* pChildNode = pChild->IsNode();
            if (pChildNode) {
                bAdded |= AddMasterParticleAddonNodes(pChildNode);
            }
        }
    }
    return bAdded;
}

bool TESObjectREFR::RemoveMasterParticleAddonNodes(NiNode* apNode) {
    uint32_t uiIndex;
    NiNode* pChildNode;
    NiAVObject* pChild;
    uint32_t i;
    BGSAddonNode* pAddonNode;
    if (!apNode) {
        return false;
    }
    uint8_t ucRemoved = 0;
    if (NiObject::IsKindOf_0(&BSValueNode::ms_RTTI, apNode)) {
        uiIndex = static_cast<uint32_t>(static_cast<BSValueNode*>(apNode)->iValue);
        pAddonNode = pDataHandler->GetAddonNode(uiIndex);
        if (pAddonNode && pAddonNode->IsMasterParticleSystem()) {
            apNode->SetAppCulled(true);
            ucRemoved = 1;
        }
    }
    for (i = 0; i < apNode->GetArrayCount(); ++i) {
        pChild = apNode->GetAt(i);
        if (pChild) {
            pChildNode = pChild->IsNode();
            if (pChildNode) {
                ucRemoved |= TESObjectREFR::RemoveMasterParticleAddonNodes(pChildNode);
            }
        }
    }
    return ucRemoved;
}

bool TESObjectREFR::HasTimeControllers(NiAVObject* ap3D) {
    bool bResult = false;
    if (!ap3D) {
        return bResult;
    }
    if (ap3D->GetControllers()) {
        bResult = true;
    }
    if (!bResult) {
        NiTListItem<NiPointer<NiProperty>>* kPos = ap3D->GetPropertyList().GetHeadPos();
        while (kPos && !bResult) {
            NiProperty* pkProperty = ap3D->GetPropertyList().GetNext(kPos);
            if (pkProperty) {
                bResult = pkProperty->GetControllers() != nullptr;
            }
        }
    }
    if (!bResult) {
        NiNode* pNode = ap3D->IsNode();
        if (pNode) {
            uint32_t uiCount = pNode->GetArrayCount();
            for (uint32_t i = 0; i < uiCount; ++i) {
                if (HasTimeControllers(pNode->GetAt(i))) {
                    bResult = true;
                    break;
                }
            }
        }
    }
    return bResult;
}

void TESObjectREFR::FilteredAddObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    if (bPreventHavokAddAll.Bool()) {
        NiAVObject* pObject = apCollision->GetSceneGraphObject();
        pObject->SetSelectiveUpdate(false);
        pObject->SetSelectiveUpdateTransforms(false);
        pObject->SetSelectiveUpdateRigid(true);
        arData.bRecurse = false;
        return;
    }
    bool bAdd = true;
    if (bPreventHavokAddClutter.Bool() && apCollision) {
        NiAVObject* pObject = apCollision->GetSceneGraphObject();
        TESObjectREFR* pRef = FindReferenceFor3D(pObject);
        if (pRef && pRef->GetObjectReference()) {
            if (TESContainer::ContainerCanHoldType(pRef->GetObjectReference()->GetFormType())) {
                bAdd = false;
            }
            if (!bAdd && pRef->GetObjectReference()->GetFormType() == LIGH_ID) {
                TESObjectLIGH* pLight = static_cast<TESObjectLIGH*>(pRef->GetObjectReference());
                if (pLight && !pLight->GetCanCarry()) {
                    bAdd = true;
                }
            }
            if (!bAdd) {
                pObject->SetSelectiveUpdate(false);
                pObject->SetSelectiveUpdateTransforms(false);
                pObject->SetSelectiveUpdateRigid(true);
                arData.bRecurse = false;
            }
        }
    }
    if (bAdd) {
        bhkWorld::AddObject(apCollision, arData);
    }
}

// Counts the collision object's rigid body into the HavokSaveData in arData.uDataA[0]: HAVOK_SAVE_WEAPON for a weapon,
// HAVOK_SAVE_ROOT_BODY for the collision root, then the active or inactive count. Objects named "Arrow" (other than the
// root) and fade nodes of an actor are skipped.
void TESObjectREFR::GetCollisionObjectSize(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData) {
    HavokSaveData* pSaveData = static_cast<HavokSaveData*>(arData.uDataA[0].p);
    bool bWeapon;
    bWeapon = NiObject::IsKindOf(&WeaponObject::ms_RTTI, apCollision);
    if (bWeapon) {
        pSaveData->cFlags |= HAVOK_SAVE_WEAPON;
    }
    NiAVObject* pObject = apCollision->GetSceneGraphObject();
    if (apCollision != pSaveData->pCollisionRoot &&
        pObject &&
        pObject->GetName().GetHandleString() &&
        !BSstrcmp(pObject->GetName().GetHandleString(), "Arrow")) {
        return;
    }

    if (pSaveData->pRef->IsActor()) {
        bool bFade;
        bFade = NiObject::IsKindOf(&BSFadeNode::ms_RTTI, pObject);
        if (bFade) {
            return;
        }
    }
    bhkWorldObject* pWorldObject = apCollision->GetWorldObject();
    if (!pWorldObject) {
        return;
    }
    bhkRigidBody* pBody = static_cast<bhkRigidBody*>(NiObject::DynamicCastStatic(&bhkRigidBody::ms_RTTI, pWorldObject));
    if (!pBody) {
        return;
    }
    if (apCollision == pSaveData->pCollisionRoot) {
        pSaveData->cFlags |= HAVOK_SAVE_ROOT_BODY;
    }
    if (pBody->IsActive()) {
        ++pSaveData->sActiveBoneCount;
    }
    else {
        ++pSaveData->sInactiveBoneCount;
    }
}

// Writes the collision object's rigid body to the old save: its position and rotation (not for the collision root), its
// active flag when the HavokSaveData in arData.uDataA[0] has HAVOK_SAVE_BODY_FLAGS, and an active body's velocities.
// The same objects as GetCollisionObjectSize are skipped.
void TESObjectREFR::SaveHavokDataForCollisionObjectOld(
    bhkNiCollisionObject* apCollision,
    bhkWorldNS::ObjectRecData& arData
) {
    HavokSaveData* pSaveData = static_cast<HavokSaveData*>(arData.uDataA[0].p);
    NiAVObject* pObject = apCollision->GetSceneGraphObject();
    if (apCollision != pSaveData->pCollisionRoot &&
        pObject &&
        pObject->GetName().GetHandleString() &&
        !BSstrcmp(pObject->GetName().GetHandleString(), "Arrow")) {
        return;
    }

    if (pSaveData->pRef->IsActor()) {
        bool bFade;
        bFade = NiObject::IsKindOf(&BSFadeNode::ms_RTTI, pObject);
        if (bFade) {
            return;
        }
    }
    bhkWorldObject* pWorldObject = apCollision->GetWorldObject();
    if (!pWorldObject) {
        return;
    }
    bhkRigidBody* pBody = static_cast<bhkRigidBody*>(NiObject::DynamicCastStatic(&bhkRigidBody::ms_RTTI, pWorldObject));
    if (!pBody) {
        return;
    }
    if (apCollision != pSaveData->pCollisionRoot) {
        NiPoint3 kPosition;
        NiQuaternion kRotation;
        pBody->GetPosition(kPosition);
        pBody->GetRotation(kRotation);
        pSaveLoadGameOLD->SaveGameDataOLD(&kPosition, sizeof(kPosition));
        pSaveLoadGameOLD->SaveGameDataOLD(&kRotation, sizeof(kRotation));
    }
    bool bActive = pBody->IsActive();
    if (pSaveData->cFlags & HAVOK_SAVE_BODY_FLAGS) {
        pSaveLoadGameOLD->SaveGameDataOLD(&bActive, sizeof(bActive));
    }
    if (bActive) {
        NiPoint3 kLinearVelocity;
        NiPoint3 kAngularVelocity;
        pBody->GetLinearVelocityNi(&kLinearVelocity);
        pBody->GetAngularVelocity(kAngularVelocity);
        pSaveLoadGameOLD->SaveGameDataOLD(&kLinearVelocity, sizeof(kLinearVelocity));
        pSaveLoadGameOLD->SaveGameDataOLD(&kAngularVelocity, sizeof(kAngularVelocity));
    }
}

// Reads one rigid body back from a save older than version 81. The walk data holds, untyped, the collision root in
// uDataA[0], the reference in uDataA[1] and the load flags in uDataA[2]: bit 2 set (from save version 43 on) or the
// root (before 43) takes the reference's own position and rotation instead of reading them, and clears bit 2; bit 1
// reads the velocities and activates the body, else they are zeroed. An "Arrow" object is skipped, and an actor's fade
// node; an actor's other bodies become dynamic. The getters are called as GetCollisionObjectSize does.
void TESObjectREFR::LoadHavokDataForCollisionObjectPreVersion81(
    bhkNiCollisionObject* apCollision,
    bhkWorldNS::ObjectRecData& arData
) {
    bhkWorldObject* pWorldObject = apCollision->GetWorldObject();
    apCollision->SetReset(false);
    if (apCollision->GetSceneGraphObject() &&
        apCollision->GetSceneGraphObject()->GetName().GetHandleString() &&
        !BSstrcmp(apCollision->GetSceneGraphObject()->GetName().GetHandleString(), "Arrow")) {
        return;
    }

    TESObjectREFR* pRef = static_cast<TESObjectREFR*>(arData.uDataA[1].p);
    if (pRef->IsActor()) {
        bool bFade;
        bFade = NiObject::IsKindOf(&BSFadeNode::ms_RTTI, apCollision->GetSceneGraphObject());
        if (bFade) {
            return;
        }
        apCollision->SetMotionType(hkpMotion::MOTION_DYNAMIC, nullptr, false);
    }
    if (!pWorldObject) {
        return;
    }
    bhkRigidBody* pBody = static_cast<bhkRigidBody*>(NiObject::DynamicCastStatic(&bhkRigidBody::ms_RTTI, pWorldObject));
    if (!pBody) {
        return;
    }
    NiPoint3 kPosition;
    NiPoint3 kLinearVelocity;
    NiPoint3 kAngularVelocity;
    NiQuaternion kRotation;
    bool bReadPlacement = (arData.uDataA[2].i & 2) == 0;
    if (pSaveLoadGameOLD->GetCurrentVersion() < 43) {
        bReadPlacement = apCollision != static_cast<bhkNiCollisionObject*>(arData.uDataA[0].p);
    }
    if (bReadPlacement) {
        pSaveLoadGameOLD->LoadGameDataOLD(&kPosition, sizeof(kPosition));
        pSaveLoadGameOLD->LoadGameDataOLD(&kRotation, sizeof(kRotation));
        pBody->SetPosition(kPosition);
        pBody->SetRotation(kRotation);
    }
    else {
        arData.uDataA[2].i &= ~2u;
        pBody->SetPosition(pRef->GetLocationOnReference());
        NiQuaternion kRefRotation;
        kRefRotation.FromRotation(pRef->GetOrientation());
        pBody->SetRotation(kRefRotation);
    }
    if (arData.uDataA[2].i & 1) {
        pSaveLoadGameOLD->LoadGameDataOLD(&kLinearVelocity, sizeof(kLinearVelocity));
        pSaveLoadGameOLD->LoadGameDataOLD(&kAngularVelocity, sizeof(kAngularVelocity));
        pBody->SetLinearVelocity(kLinearVelocity);
        pBody->SetAngularVelocity(kAngularVelocity);
        pBody->SetActive(true);
    }
    else {
        pBody->SetLinearVelocity(NiPoint3::ZERO);
        pBody->SetAngularVelocity(NiPoint3::ZERO);
    }
}

bool TESObjectREFR::AddAddonNodes(NiNode* apNode) {
    const char* pName;
    uint32_t uiValue;
    BSFadeNode* pFadeNode;
    float fPhase;
    NiAVObject* pAddon3D;
    NiNode* pChildNode;
    NiAVObject* pChild;
    uint32_t i;
    BGSAddonNode* pAddonNode;
    if (!apNode) {
        return false;
    }
    uint8_t bHasAddonNodes = 0;
    if (NiObject::IsKindOf_0(&BSValueNode::ms_RTTI, apNode)) {
        for (i = 0; i < apNode->GetArrayCount(); ++i) {
            pChild = apNode->GetAt(i);
            if (pChild) {
                pChildNode = pChild->IsNode();
                if (pChildNode) {
                    if (pChildNode->GetName().Bool()) {
                        pName = pChildNode->GetName().GetHandleString();
                        if (!std::strncmp(pName, "BASE", 4u)) {
                            pChildNode->DetachChildAt(i);
                        }
                    }
                }
            }
        }
        uiValue = static_cast<uint32_t>(static_cast<BSValueNode*>(apNode)->iValue);
        pAddonNode = pDataHandler->GetAddonNode(uiValue);
        if (pAddonNode &&
            static_cast<TESModelTextureSwap*>(pAddonNode)->GetModelLength() &&
            !pAddonNode->IsMasterParticleSystem()) {
            pAddon3D = pAddonNode->Clone3D(nullptr);
            if (pAddon3D) {
                pFadeNode = pAddon3D->IsFadeNode();
                if (pFadeNode) {
                    pFadeNode->SetUserData(nullptr);
                }
                fPhase = static_cast<float>((Random() % 1000) / 100.0);
                TESObjectREFR::RandomizeAddons(pAddon3D, fPhase);
                apNode->AttachChild(pAddon3D, true);
                bHasAddonNodes = 1;
            }
        }
    }
    for (i = 0; i < apNode->GetArrayCount(); ++i) {
        pChild = apNode->GetAt(i);
        if (pChild) {
            pChildNode = pChild->IsNode();
            if (pChildNode) {
                bHasAddonNodes |= TESObjectREFR::AddAddonNodes(pChildNode);
            }
        }
    }
    if (bHasAddonNodes) {
        apNode->SetSelectiveUpdate(true);
        apNode->SetSelectiveUpdateRigid(false);
    }
    return bHasAddonNodes;
}

bool TESObjectREFR::IsReferenceFormType(int32_t aeFormType) {
    return aeFormType >= REFR_ID && (aeFormType <= PFLA_ID || aeFormType == PCBE_ID);
}

bool TESObjectREFR::HasAddonNodes(NiNode* apNode) {
    NiNode* pChildNode;
    NiAVObject* pChild;
    uint32_t i;
    if (!apNode) {
        return false;
    }
    if (NiObject::IsKindOf_0(&BSValueNode::ms_RTTI, apNode) && apNode->GetChildCount()) {
        return true;
    }
    for (i = 0; i < apNode->GetArrayCount(); ++i) {
        pChild = apNode->GetAt(i);
        if (pChild) {
            pChildNode = pChild->IsNode();
            if (pChildNode && TESObjectREFR::HasAddonNodes(pChildNode)) {
                return true;
            }
        }
    }
    return false;
}

void TESObjectREFR::SetTerminalUnlocked(TESObjectREFR* apTerminal, char acUnlocked) {
    if (apTerminal) {
        uint8_t cTerminalState = apTerminal->TESObjectREFR::GetExtra()->GetTerminalState();
        if (acUnlocked) {
            cTerminalState |= 0x80;
        }
        else {
            cTerminalState &= ~0x80;
        }
        apTerminal->TESObjectREFR::GetExtra()->SetTerminalState(cTerminalState);
        apTerminal->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
    }
}

bool TESObjectREFR::IsHacked(TESObjectREFR* apTerminal) {
    if (apTerminal) {
        return (apTerminal->TESObjectREFR::GetExtra()->GetTerminalState() & 128) != 0;
    }
    return false;
}

bool TESObjectREFR::IsLockedOut(TESObjectREFR* apTerminal) {
    int32_t iEntryPointValue;
    if (!apTerminal) {
        return false;
    }
    uint8_t ucTerminalState = apTerminal->GetExtra()->GetTerminalState();
    if ((ucTerminalState & 127) == 0) {
        return false;
    }
    if ((ucTerminalState & 127) != 1) {
        return true;
    }
    *reinterpret_cast<float*>(&iEntryPointValue) = 0.0;
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::IGNORE_LOCKED_TERMINAL, pPlayer, &iEntryPointValue);
    return *reinterpret_cast<float*>(&iEntryPointValue) == 0.0;
}

void TESObjectREFR::LoadControllerManagerBGS(
    NiControllerManager* apManager,
    BGSLoadGameBuffer* apBuffer,
    float afTime
) {
    NiAVObject* pobj3D;
    if (afTime == -1.0) {
        afTime = fCellAppTime;
    }
    uint32_t icount = apBuffer->LoadVariableSizedValue();
    if (apManager) {
        apManager->DeactivateAll(0.0f);
        if (icount) {
            apManager->SetActive(true);
        }
    }
    bool banyFound = false;
    TESObjectREFR* pref = apBuffer->GetReference();
    if (pref) {
        pobj3D = pref->TESObjectREFR::Get3D();
    }
    else {
        pobj3D = nullptr;
    }
    for (uint32_t i = 0; i < icount; ++i) {
        char cSequenceName[268];
        apBuffer->LoadString(cSequenceName);
        BSAnimGroupSequence* pseq = nullptr;
        bool bfound = false;
        if (apManager) {
            uint32_t isequenceCount = apManager->GetSequenceCount();
            for (uint32_t n = 0; n < isequenceCount; ++n) {
                pseq = static_cast<BSAnimGroupSequence*>(apManager->GetSequenceAt(n));
                if (pseq) {
                    const char* pSequenceName = pseq->m_kName.GetHandleString();
                    if (!BSstrcmp(pSequenceName, cSequenceName)) {
                        if (!pseq->GetState()) {
                            apManager->ActivateSequence(pseq, 0, false, 1.0f, 0.0f, nullptr);
                        }
                        pseq->LoadGame(apBuffer, afTime);
                        if (pobj3D && HasParticleSystem(pobj3D)) {
                            float fsequenceTime = pseq->GetOffset() + afTime;
                            float fstartTime = afTime - fsequenceTime;
                            if (fstartTime < 0.0) {
                                fstartTime = 0.0f;
                            }
                            float fframeTime = fsequenceTime / 20.0;
                            if (fframeTime < 0.016666600480675697) {
                                fframeTime = 0.0166666f;
                            }
                            for (float fStepTime = fstartTime; afTime > static_cast<double>(fStepTime);
                                 fStepTime = fStepTime + fframeTime) {
                                NiUpdateData kUpdateData(fStepTime, false, false);
                                pobj3D->UpdateSelected(kUpdateData);
                            }
                        }

                        bfound = true;
                        banyFound = true;
                        break;
                    }
                }
            }
        }
        if (!bfound) {
            BSAnimGroupSequence tempSequence;
            tempSequence.LoadGame(apBuffer, afTime);
        }
    }
    if (banyFound && pobj3D) {
        bhkNiCollisionObject::ResetSim(pobj3D, true);
        NiUpdateData kUpdateData(afTime, true, false);
        pobj3D->Update(kUpdateData);
    }
}

// DoObjectRec callback of LoadHavokData: arData.uDataA[0] is the load's HavokSaveData. Each rigid body below the
// reference's 3D (not the collision root, not an arrow, not an actor's faded node) gets its position and rotation
// (the reference's own with HAVOK_SAVE_ROOT_BODY, else saved ones) and its velocities (saved, or zero).
void TESObjectREFR::LoadHavokDataForCollisionObjectOld(
    bhkNiCollisionObject* apCollision,
    bhkWorldNS::ObjectRecData& arData
) {
    NiPoint3 kAngularVelocity;
    NiPoint3 kLinearVelocity;
    NiQuaternion kRotation;
    NiPoint3 kPosition;
    bool bHasVelocity;
    HavokSaveData* phavokSaveData = static_cast<HavokSaveData*>(arData.uDataA[0u].p);
    NiObject* pkObject = apCollision->GetWorldObject();
    apCollision->SetReset(false);
    NiObjectNET* pSceneObject = apCollision->m_pkSceneObject;
    if ((apCollision == phavokSaveData->pCollisionRoot ||
         !pSceneObject ||
         !pSceneObject->m_kName.GetHandleString() ||
         BSstrcmp(pSceneObject->m_kName.GetHandleString(), "Arrow")) &&
        (!phavokSaveData->pRef->IsActor() || !NiObject::IsKindOf_0(&BSFadeNode::ms_RTTI, pSceneObject)) &&
        pkObject) {
        bhkRigidBody* prigidBody = static_cast<bhkRigidBody*>(
            NiObject::DynamicCastStatic(&bhkRigidBody::ms_RTTI, pkObject)
        );
        if (prigidBody) {
            const hkpWorld* pWorld = prigidBody->GethkWorld();
            bool bInWorld = pWorld != nullptr;
            if (pWorld) {
                prigidBody->SetActive(false);
            }
            if ((phavokSaveData->cFlags & HAVOK_SAVE_ROOT_BODY) != 0) {
                NiQuaternion kRefRotation;
                phavokSaveData->cFlags &= ~HAVOK_SAVE_ROOT_BODY;
                prigidBody->SetPosition(phavokSaveData->pRef->GetLocationOnReference());
                kRefRotation.FromRotation(phavokSaveData->pRef->GetOrientation());
                prigidBody->SetRotation(kRefRotation);
            }
            else {
                pSaveLoadGameOLD->LoadGameDataOLD(&kPosition, sizeof(kPosition));
                pSaveLoadGameOLD->LoadGameDataOLD(&kRotation, sizeof(kRotation));
                prigidBody->SetPosition(kPosition);
                prigidBody->SetRotation(kRotation);
            }
            bHasVelocity = false;
            if ((phavokSaveData->cFlags & HAVOK_SAVE_BODY_FLAGS) != 0) {
                pSaveLoadGameOLD->LoadGameDataOLD(&bHasVelocity, sizeof(bHasVelocity));
            }
            else {
                bHasVelocity = (phavokSaveData->cFlags & HAVOK_SAVE_VELOCITIES) != 0;
            }
            if (bHasVelocity) {
                pSaveLoadGameOLD->LoadGameDataOLD(&kLinearVelocity, sizeof(kLinearVelocity));
                pSaveLoadGameOLD->LoadGameDataOLD(&kAngularVelocity, sizeof(kAngularVelocity));
                prigidBody->SetLinearVelocity(kLinearVelocity);
                prigidBody->SetAngularVelocity(kAngularVelocity);
                prigidBody->SetActive(true);
            }
            else {
                prigidBody->SetLinearVelocity(NiPoint3::ZERO);
                prigidBody->SetAngularVelocity(NiPoint3::ZERO);
                if (bInWorld) {
                    prigidBody->SetActive(false);
                }
            }
        }
    }
}

void TESObjectREFR::SaveHavokDataForCollisionObject(
    bhkNiCollisionObject* apCollision,
    bhkWorldNS::ObjectRecData& arData
) {
    NiPoint3 kAngVel;
    NiPoint3 kLinVel;
    NiQuaternion kRot;
    NiPoint3 kPos;
    char cFlags;
    BGSHavokSaveData* phavokSaveData = static_cast<BGSHavokSaveData*>(arData.uDataA[0u].p);
    BGSSaveGameBuffer* pSaveGameBuffer = static_cast<BGSSaveGameBuffer*>(phavokSaveData->pSaveLoadBuffer);
    bhkWorldObject* pworldObject = apCollision->GetWorldObject();
    if (pworldObject) {
        bhkRigidBody* prigidBody = static_cast<bhkRigidBody*>(
            NiObject::DynamicCastStatic(&bhkRigidBody::ms_RTTI, pworldObject)
        );
        if (prigidBody) {
            if (apCollision != phavokSaveData->pCollisionRoot) {
                prigidBody->GetPosition(kPos);
                prigidBody->GetRotation(kRot);
                pSaveGameBuffer->SaveDataEndian(&kPos, sizeof(kPos), 0);
                pSaveGameBuffer->SaveDataEndian(&kRot, sizeof(kRot), 0);
            }
            cFlags = 0;
            bool bActive = prigidBody->IsActive();
            if ((phavokSaveData->cFlags & HAVOK_SAVE_BODY_FLAGS) != 0) {
                if (bActive) {
                    cFlags |= HAVOK_BODY_ACTIVE;
                }
                if (prigidBody->GetMotionType() == hkpMotion::MOTION_KEYFRAMED &&
                    !static_cast<uint8_t>(prigidBody->IsFlagSet(bhkWorldObject::KEYFRAMED_BY_LOADEDAREABOUND))) {
                    cFlags |= HAVOK_BODY_KEYFRAMED;
                }
                pSaveGameBuffer->SaveDataEndian(&cFlags, sizeof(cFlags), 0);
            }
            if (bActive) {
                prigidBody->GetLinearVelocityNi(&kLinVel);
                prigidBody->GetAngularVelocity(kAngVel);
                pSaveGameBuffer->SaveDataEndian(&kLinVel, sizeof(kLinVel), 0);
                pSaveGameBuffer->SaveDataEndian(&kAngVel, sizeof(kAngVel), 0);
            }
        }
    }
}

char TESObjectREFR::RemoveRefFromMultiboundTestMap(TESObjectREFR* apRef) {
    return TESObjectCELL::MultiboundTestMap.RemoveAt(apRef);
}

NiPoint3 TESObjectREFR::GetBoundMax(NiAVObject* apObj) {
    if (apObj) {
        const NiFixedString* pTag = &BSBound::Tag();
        BSBound* pBound = static_cast<BSBound*>(apObj->GetExtraData(*pTag));
        if (pBound) {
            pBound->GetBoundMax(boundMax);
            return boundMax;
        }
    }

    return NiPoint3::ZERO;
}

void TESObjectREFR::TransChangeCallback(bhkNiCollisionObject* apCollision, uint32_t aiChanged) {
    NiAVObject* pav = apCollision->m_pkSceneObject;
    TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pav);
    if (pref) {
        TESHavokUtilities::Should3DRagDoll(pref, pref->Get3D());
        if ((aiChanged & 1) != 0) {
            TESObjectCELL* prefcell = pref->GetParentCell();
            if (prefcell) {
                bool bbadpos = false;
                bool bbadStartPos = false;
                NiPoint3 newloc = pav->GetWorldTranslate();
                if (std::isfinite(newloc.x) &&
                    std::isfinite(newloc.y) &&
                    std::isfinite(newloc.z) &&
                    !std::isnan(newloc.x) &&
                    !std::isnan(newloc.y) &&
                    !std::isnan(newloc.z)) {
                    if (prefcell->GetInterior()) {
                        NiNode* pstatic3D = prefcell->GetChildNode(TESObjectCELL::CN_STATIC);
                        if (pstatic3D) {
                            NiBound kBound = pstatic3D->GetWorldBound();
                            if (kBound.m_kCenter != NiPoint3::ZERO) {
                                double fHeightDiff = kBound.m_kCenter.z - pav->GetWorldTranslate().z;
                                if (kBound.m_fRadius < fHeightDiff) {
                                    bbadpos = true;
                                    double fStartingPosDiff = kBound.m_kCenter.z - pref->GetStartingLocation().z;
                                    if (kBound.m_fRadius < fStartingPosDiff) {
                                        bbadStartPos = true;
                                    }
                                }
                            }
                        }
                    }
                    else if (prefcell->GetLand()) {
                        TESObjectLAND* pLand = prefcell->GetLand();
                        float fminHeight = pLand->GetMinMaxLandHeight().x - 128.0;
                        double dZ = pav->GetWorldTranslate().z;
                        if (dZ < fminHeight) {
                            bbadpos = true;
                            if (pref->GetStartingLocation().z < static_cast<double>(fminHeight)) {
                                bbadStartPos = true;
                            }
                        }
                    }
                }
                else {
                    bbadpos = true;
                }
                if (bbadpos) {
                    if (bbadStartPos) {
                        NiPoint3 ignore;
                        NiPoint3 safepos = NiPoint3::ZERO;
                        prefcell->GetCOCPlacementInfo(safepos, ignore);
                        pref->SetStartingPosition(safepos);
                    }
                    NiAVObject* pRef3D = pref->Get3D();
                    if (pRef3D) {
                        pRef3D->SetTranslate(pref->GetStartingLocation());
                        pRef3D->SetRotate(NiMatrix3::IDENTITY);
                    }
                    bhkNiCollisionObject::ResetSim(pRef3D, true);
                    NiUpdateData kUpdateData(0.0f, true, false);
                    pRef3D->Update(kUpdateData);
                    bhkRigidBody* pbody = static_cast<bhkRigidBody*>(apCollision->GetWorldObject());
                    if (pbody) {
                        float fmaxVel = pbody->GethkMaxLinearVelocity() * 0.25;
                        if (fmaxVel > 5.0) {
                            pbody->SethkMaxLinearVelocity(fmaxVel);
                        }
                        else {
                            apCollision->SetMotionType(hkpMotion::MOTION_KEYFRAMED, nullptr, false);
                            apCollision->SetNotify(false);
                            int32_t iRefFormID = pref->iFormID;
                            Warning(
                                "HAVOK: Disabling collision on ref '%s' (%08X).",
                                pref->GetFormEditorID(),
                                iRefFormID
                            );
                        }
                    }
                }
            }
            NiPoint3 oldloc = pref->GetLocationOnReference();
            bool bresetAllow = pSaveLoadGame->SetThreadAllowChanges(false);
            pref->SetLocationOnReference(pav->GetWorldTranslate());
            pSaveLoadGame->SetThreadAllowChanges(bresetAllow);
            pref->AddChange(CHANGE_REFR_HAVOK_MOVE);
            if (pref->GetObjectReference() &&
                pref->GetObjectReference()->GetFormType() == LIGH_ID &&
                !pref->GetFireOff()) {
                const NiPoint3* pLocation = &pref->GetLocationOnReference();
                if (static_cast<double>(pLocation->z) < pref->GetRelevantWaterHeight()) {
                    pref->SetFireOff(true);
                    TESObjectREFR::RemoveAddonNodes(pref->Get3D());
                    pref->RemoveLight(false);
                }
            }

            if (!TES::pInstance->pInteriorCell) {
                TESWorldSpace* pworld = TES::pInstance->pWorldSpace;
                Actor* pac = nullptr;
                if (pref && pref->IsActor()) {
                    pac = static_cast<Actor*>(pref);
                }

                if (pac) {
                    pac->SetForceUpdateQuest(false);
                }
                float fY = pref->GetLocationOnReference().y;
                const NiPoint3* pLocation = &pref->GetLocationOnReference();
                TESObjectCELL* pnewcell = pDataHandler->GetCellFromWorldCoord(pLocation->x, fY, pworld, false);
                if (pnewcell != prefcell) {
                    if (pnewcell) {
                        TESObjectREFR::MoveRefToNewSpace(pref, nullptr, pworld);
                        if (!pref->IsActor() && pref->Get3D()) {
                            NiNode* pdynamic = static_cast<NiNode*>(pnewcell->GetDynamicNode());
                            if (pdynamic) {
                                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                                    TESObjectCELL::QueueAttachChild(pref, pdynamic);
                                }
                                else {
                                    pnewcell->PerformCellNodeAttach(pref, pdynamic);
                                }
                            }
                        }
                    }
                    else {
                        bresetAllow = pSaveLoadGame->SetThreadAllowChanges(false);
                        pref->SetLocationOnReference(oldloc);
                        pSaveLoadGame->SetThreadAllowChanges(bresetAllow);
                        pav->SetTranslate(oldloc);
                        apCollision->SetMotionType(hkpMotion::MOTION_KEYFRAMED, nullptr, false);
                    }
                }
            }
        }
        if ((aiChanged & 2) != 0) {
            NiPoint3 rot;
            pav->GetWorldTransform().m_Rotate.ToEulerAnglesXYZ(rot.x, rot.y, rot.z);
            bool bresetAllow = pSaveLoadGame->SetThreadAllowChanges(false);
            pref->SetAngleOnReference(rot);
            pSaveLoadGame->SetThreadAllowChanges(bresetAllow);
        }
        NiNode* p3D = pref->Get3D();
        ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(0);
        pShadowSceneNode->UpdateObjectLighting(p3D, true);
    }
}

void TESObjectREFR::DebugDisplayCallback(NiGeometry* ap3D, const NiColor& color) {
    if (ap3D) {
        BSShaderNoLightingProperty* pProperty = new BSShaderNoLightingProperty;
        pProperty->SetFlag(BSShaderProperty::BSSP_FLAG_WIREFRAME, 1);
        ap3D->AttachProperty(pProperty);
        BSShaderManager::PrepareObject(ap3D, 0, 0);
    }
}

void TESObjectREFR::GatherHavokSaveDataForCollisionObject(
    bhkNiCollisionObject* apCollision,
    bhkWorldNS::ObjectRecData& arData
) {
    BGSHavokSaveData* phavokSaveData = static_cast<BGSHavokSaveData*>(arData.uDataA[0u].p);
    bhkWorldObject* pworldObject = apCollision->GetWorldObject();
    if (pworldObject) {
        bhkRigidBody* prigidBody = static_cast<bhkRigidBody*>(
            NiObject::DynamicCastStatic(&bhkRigidBody::ms_RTTI, pworldObject)
        );
        if (prigidBody) {
            if (apCollision == phavokSaveData->pCollisionRoot) {
                phavokSaveData->cFlags |= HAVOK_SAVE_ROOT_BODY;
            }
            if (prigidBody->IsActive()) {
                ++phavokSaveData->iActiveBoneCount;
            }
            else {
                ++phavokSaveData->iInactiveBoneCount;
            }
            if (prigidBody->GetMotionType() == 4 &&
                !static_cast<uint8_t>(prigidBody->IsFlagSet(bhkWorldObject::KEYFRAMED_BY_LOADEDAREABOUND))) {
                ++phavokSaveData->iKeyFramedBoneCount;
            }
        }
    }
}

void UpdateLODMultInvisibility(TESObjectREFR* apRef, NiNode* apNode) {
    TESBoundObject* pObjectReference;
    int32_t iLODMult;
    uint8_t bIsInvisible;
    BSFadeNode* pFadeNode;
    if (apNode) {
        pFadeNode = apNode->IsFadeNode();
        if (pFadeNode) {
            bIsInvisible = apRef->GetIsInvisibleLODMultType();
            if (bIsInvisible != (pFadeNode->GetMultType() == LOD_MULT_INVISIBLE)) {
                if (bIsInvisible) {
                    pFadeNode->ForceLODMultType(LOD_MULT_INVISIBLE);
                    pFadeNode->TurnFadeNodeOff();
                }
                else {
                    pObjectReference = apRef->TESObjectREFR::GetObjectReference();
                    iLODMult = static_cast<int32_t>(TES::GetLODMult(pObjectReference));
                    pFadeNode->ForceLODMultType(static_cast<ENUM_LOD_MULT>(iLODMult));
                    pFadeNode->TurnFadeNodeOn();
                }
            }
        }
    }
}

void hkQuaternion_operator_assign(hkQuaternion* apThis, const hkQuaternion& q) {
    apThis->m_vec = q.m_vec;
}

void TESObjectREFR::LoadHavokDataForCollisionObject(bhkNiCollisionObject* apObject, bhkWorld::ObjectRecData& arData) {
    NiPoint3 kAngVel;
    NiPoint3 kLinVel;
    char cFlags;
    NiQuaternion kRot;
    NiPoint3 kPos;
    BGSHavokSaveData* pSaveData = static_cast<BGSHavokSaveData*>(arData.uDataA[0u].p);
    BGSLoadGameBuffer* pSaveLoadBuffer = static_cast<BGSLoadGameBuffer*>(pSaveData->pSaveLoadBuffer);
    NiObject* pkObject = apObject->GetWorldObject();
    apObject->SetReset(false);
    if (pkObject) {
        bhkRigidBody* pBody = static_cast<bhkRigidBody*>(NiObject::DynamicCastStatic(&bhkRigidBody::ms_RTTI, pkObject));
        if (pBody) {
            if ((pSaveData->cFlags & HAVOK_SAVE_ROOT_BODY) != 0) {
                pSaveData->cFlags &= ~HAVOK_SAVE_ROOT_BODY;
                kPos = pSaveData->pRef->GetLocationOnReference();
                kRot.FromRotation(pSaveData->pRef->GetOrientation());
            }
            else {
                pSaveLoadBuffer->LoadData(&kPos, sizeof(kPos));
                pSaveLoadBuffer->LoadData(&kRot, sizeof(kRot));
            }
            pBody->SetRotation(kRot);
            pBody->SetPosition(kPos);
            bool bHasVelocity = (pSaveData->cFlags & HAVOK_SAVE_VELOCITIES) != 0;
            if ((pSaveData->cFlags & HAVOK_SAVE_BODY_FLAGS) != 0) {
                cFlags = 0;
                pSaveLoadBuffer->LoadData(&cFlags, sizeof(cFlags));
                if ((cFlags & HAVOK_BODY_ACTIVE) != 0) {
                    bHasVelocity = true;
                }
                if ((cFlags & HAVOK_BODY_KEYFRAMED) != 0) {
                    pBody->SetMotionType(hkpMotion::MOTION_KEYFRAMED);
                }
            }
            if (bHasVelocity) {
                pSaveLoadBuffer->LoadData(&kLinVel, sizeof(kLinVel));
                pSaveLoadBuffer->LoadData(&kAngVel, sizeof(kAngVel));
                pBody->SetLinearVelocity(kLinVel);
                pBody->SetAngularVelocity(kAngVel);
                pBody->SetActive(true);
            }
        }
    }
}

BGSHavokSaveData::BGSHavokSaveData()
  : cFlags(0), iActiveBoneCount(0), iInactiveBoneCount(0), iKeyFramedBoneCount(0), pRef(nullptr), pObj3D(nullptr),
    pCollisionRoot(nullptr), pSaveLoadBuffer(nullptr) {}

HavokSaveData::HavokSaveData()
  : cFlags(0), sActiveBoneCount(0), sInactiveBoneCount(0), pRef(nullptr), pObj3D(nullptr), pCollisionRoot(nullptr) {}

REFR_PLACEMENT_DATA::REFR_PLACEMENT_DATA() {}

void REFR_PLACEMENT_DATA::Endian() {
    ENDIAN(kPosition.x, false);
    ENDIAN(kPosition.y, false);
    ENDIAN(kPosition.z, false);
    ENDIAN(kRotation.x, false);
    ENDIAN(kRotation.y, false);
    ENDIAN(kRotation.z, false);
}

bhkAcousticSpacePhantomCallbackShape::bhkAcousticSpacePhantomCallbackShape() {}

bhkShapeCollection* bhkBvTreeShape::GetShapeCollection() {
    hkpShape* pShape = GethkShapeCollection();
    if (pShape) {
        return static_cast<bhkShapeCollection*>(bhkShapeCollection::Getbhk(pShape));
    }

    return nullptr;
}

hkpShapeCollection* bhkBvTreeShape::GethkShapeCollection() const {
    // bhkBvTreeShape::operator* is bhkShape's null-safe getter.
    const hkpShape* pShape = **this;
    if (!pShape) {
        return nullptr;
    }
    // The collection's container is its second base; static_cast keeps the null test.
    return const_cast<hkpShapeCollection*>(static_cast<const hkpShapeCollection*>(pShape->getContainer()));
}

bhkCollisionObject::bhkCollisionObject(NiAVObject* apObject) : bhkNiCollisionObject(static_cast<NiNode*>(apObject)) {}

bhkEntity::bhkEntity() {
    ++iCount;
}

void bhkNiCollisionObject::SetNotify(bool abTF) {
    SetBit(abTF, 4);
    if (abTF) {
        SetSetLocal(true);
    }
}

const hkVector4& bhkRigidBody::GetLinearVelocity() {
    hkpRigidBody* pBody = **this;
    return pBody ? pBody->getLinearVelocity() : hkVector4::getZero();
}

const hkVector4& bhkRigidBody::GetAngularVelocity() {
    hkpRigidBody* pBody = **this;
    return pBody ? pBody->getAngularVelocity() : hkVector4::getZero();
}

void bhkRigidBody::GetAngularVelocity(NiPoint3& arPos) {
    HK2NINS(arPos, GetAngularVelocity());
}

void bhkRigidBody::SetLinearVelocity(const hkVector4& arVel) {
    hkpRigidBody* pBody = **this;
    if (pBody) {
        MarkWrite();
        pBody->setLinearVelocity(arVel);
        UnMarkWrite();
    }
}

void bhkRigidBody::SetAngularVelocity(const hkVector4& arVel) {
    hkpRigidBody* pBody = **this;
    if (pBody) {
        MarkWrite();
        pBody->setAngularVelocity(arVel);
        UnMarkWrite();
    }
}

// Sets the body's linear velocity from a Gamebryo vector (converted by NI2HK, with the world scale).
void bhkRigidBody::SetLinearVelocity(const NiPoint3& arVel) {
    hkVector4 hkv;
    this->SetLinearVelocity(NI2HK(hkv, arVel));
}

// Sets the body's angular velocity from a Gamebryo vector (converted by NI2HKNS, without the world scale).
void bhkRigidBody::SetAngularVelocity(const NiPoint3& arVel) {
    hkVector4 hkv;
    this->SetAngularVelocity(NI2HKNS(hkv, arVel));
}

bool bhkRigidBody::IsActive() {
    hkpEntity* pEntity = **this;
    if (!pEntity) {
        return false;
    }
    return pEntity->isActive();
}

float bhkRigidBody::GethkMaxLinearVelocity() {
    hkUFloat8 apMemory;
    hkpRigidBody* pb = **this;
    if (pb) {
        hkUFloat8* p_m_maxLinearVelocity;
        hkpMotion* pdm = pb->getStoredDynamicMotion();
        if (pdm) {
            p_m_maxLinearVelocity = &pdm->m_motionState.m_maxLinearVelocity;
        }
        else {
            float f = pb->getMaxLinearVelocity();
            apMemory = hkUFloat8(f);
            p_m_maxLinearVelocity = &apMemory;
        }
        return *p_m_maxLinearVelocity;
    }

    return 0.0f;
}

void bhkRigidBodyCinfo::SetMotionType(hkpMotion::MotionType aeType) {
    hk.m_motionType = aeType;
}

void bhkRigidBodyCinfo::SetRotation(const hkQuaternion& arRot) {
    hkQuaternion_operator_assign(&hk.m_rotation, arRot);
}

void bhkRigidBodyCinfo::SetShape(const bhkShape* apShape) {
    m_shape = **apShape;
    hk.m_shape = m_shape;
}

hkpShape* bhkShape::Gethk() {
    return static_cast<hkpShape*>(phkObject);
}

bhkShape* bhkShapeCollection::Getbhk(hkpShape* apShape) {
    return bhkShape::Getbhk(apShape);
}

hkShapePhantomCinfo::~hkShapePhantomCinfo() {}

bhkShapePhantom::bhkShapePhantom() {
    ++iCount;
    ResetPhantomFlags();
}

bhkShapePhantom::~bhkShapePhantom() {
    bhkSerializable::Kill();
    --iCount;
}

void bhkShapePhantom::SetTransform(const NiTransform& arTrans) {
    hkTransform hkt;
    hkTransform* pTransform = &NI2HK(hkt, arTrans);
    bhkShapePhantom::setTransform(pTransform);
}

hkSimpleShapePhantomCinfo::hkSimpleShapePhantomCinfo() {}

hkSimpleShapePhantomCinfo::~hkSimpleShapePhantomCinfo() {}

bhkSimpleShapePhantom::bhkSimpleShapePhantom(const hkSimpleShapePhantomCinfo& arInfo) {
    bhkSimpleShapePhantom::Init(arInfo);
    ++iCount;
    ResetPhantomFlags();
}

bhkSphereRepShape::bhkSphereRepShape() {
    ++iCount;
}

bhkSphereShape::bhkSphereShape(float afRadius, bool abBS) {
    ++iCount;
    if (abBS) {
        Init(NI2HK(afRadius));
    }
    else {
        Init(afRadius);
    }
}

bhkTrapListener* bhkWorld::QTrapListener() {
    return pTrapListener;
}

bool bhkWorldObject::ForceAdd(bhkWorld* apWorld) {
    return Add(apWorld);
}

hkWorldObjectCinfo::hkWorldObjectCinfo() : m_collisionFilterInfo(0), m_shape(nullptr) {
    m_broadPhaseType = hkpWorldObject::BROAD_PHASE_ENTITY;
}

hkWorldObjectCinfo::~hkWorldObjectCinfo() {}

const NiPoint3& FurnitureMark::QPosition() {
    return Position;
}

NiPoint3& BGSPrimitive::QRadii() {
    return Radii;
}

BSFadeNode* BGSPrimitive::GetNode() {
    return spNode;
}

IBSAnimNoteReceiver::IBSAnimNoteReceiver() {}

IBSAnimNoteReceiver::~IBSAnimNoteReceiver() {}

void BSAnimNoteReceiver::SetObjectRefr(TESObjectREFR* apObjRefr) {
    pObjRefr = apObjRefr;
}

void CheckWithinMultiBoundTask::Run() {
    if (sp3D.m_pObject) {
        BSFadeNode* pfade = sp3D->IsFadeNode();
        if (pfade && pfade->GetLinkedObj() && pBoundRef->Get3D()) {
            bool bignore = true;
            bWithin = TESObjectREFR::QVerticesInsideMultiboundRecurse(sp3D.m_pObject, spBound.m_pObject, bignore, true);
        }
    }
}

void CheckWithinMultiBoundTask::Finish() {
    IOTask::AddToPostProcessQueue();
}

// When the test found the 3D within the multibound, the reference linked to its fade node goes into AttachMap against
// the bound reference, unless the bound reference's cell is still testing it. GetParentCell is called again only to
// reach the static IsRefInMultiboundTestMap.
void CheckWithinMultiBoundTask::PostProcess() {
    if (bWithin) {
        BSFadeNode* pFadeNode;
        if (sp3D) {
            NiAVObject* p3D = sp3D;
            pFadeNode = p3D->IsFadeNode();
        }
        else {
            pFadeNode = nullptr;
        }
        TESObjectREFR* pLinkedObj;
        if (pFadeNode) {
            pLinkedObj = pFadeNode->GetLinkedObj();
        }
        else {
            pLinkedObj = nullptr;
        }
        if (pLinkedObj &&
            pBoundRef->GetParentCell() &&
            pBoundRef->GetParentCell()->IsRefInMultiboundTestMap(pLinkedObj)) {
            AttachMap.SetAt(pLinkedObj, pBoundRef);
        }
    }
}

void CheckWithinMultiBoundTask::CancelAttachment(TESObjectREFR* apMoribundRef) {
    AttachMap.RemoveAt(apMoribundRef);
}

void CheckWithinMultiBoundTask::DoAttachments() {
    NiTMapItem<TESObjectREFR*, TESObjectREFR*>* pPos = AttachMap.GetFirstPos();
    while (pPos) {
        TESObjectREFR* pRefr;
        TESObjectREFR* pBoundRef;
        AttachMap.GetNext(pPos, pRefr, pBoundRef);
        if (!pRefr || !pBoundRef) {
            continue;
        }
        bool bAttached = pRefr->Get3D() && pRefr->GetParentCell()->GetDynamicNode() == pRefr->Get3D()->GetParent();
        if (bAttached) {
            continue;
        }
        NiNode* pMultiBoundNode = pRefr->GetParentCell()->GetMultiBoundNode(pBoundRef);
        if (pMultiBoundNode) {
            NiNode* pParent = pRefr->Get3D()->GetParent();
            pParent->DetachChild(pRefr->Get3D());
            pMultiBoundNode->AttachChild(pRefr->Get3D(), true);
            pRefr->SetMultiBoundRef(pBoundRef);
        }
    }
    AttachMap.RemoveAll_1();
}

ExtraAnimNoteReceiver::~ExtraAnimNoteReceiver() {
    delete pReceiver;
}

void ZoneEntry::RemoveTarget(TESObjectREFR* apTarget) {
    TargetEntry* pEntry = GetTargetEntry(apTarget);
    RemoveTargetEntry(pEntry);
}

// The node base sits at +0x10: the entry of the node found (null stays null).
TargetEntry* TargetEntry::GetTargetEntry(TESObjectREFR* apTarget) {
    return static_cast<TargetEntry*>(GetEntry(apTarget));
}

void BGSZoneTargetListener::RemoveZone(TESObjectREFR* apZone) {
    ZoneEntry* pZoneEntry = GetZoneEntry(apZone);
    RemoveZoneEntry(pZoneEntry);
}

void MultiBoundMarkerData::SetHalfExtents(const NiPoint3& aExt) {
    HalfExtents = aExt;
}

const NiPoint3& MultiBoundMarkerData::GetDefaultHalfExtents() {
    return MultiBoundMarkerData::DefHalfExtents;
}

Script* PackageEventAction::QScript() {
    return pResultScript;
}

char* Script::GetText() {
    return m_text;
}

float TESCreature::GetBaseScale() const {
    return fBaseScale;
}

InventoryChanges* TESDataHandler::GetBarterContainer() {
    return pBarterContainer;
}

uint32_t TESObjectLIGH::GetRadius() {
    return data.radius;
}

bool TESMagicTargetForm::CanBeMagicTarget(TESForm* apForm) {
    return dynamic_cast<TESMagicTargetForm*>(apForm) != nullptr;
}

void TESObjectCELL::DecRefsWithVisibleDistantCount() {
    --sNumRefsWithVisibleDistant;
}

// The found value goes to a local that is never read.
bool TESObjectCELL::IsRefInMultiboundTestMap(TESObjectREFR* apRef) {
    TESObjectREFR* pValue;
    return MultiboundTestMap.GetAt(apRef, pValue);
}

bool TESObjectCONT::GetRespawn() {
    return (Data.cContFlags & CONT_DATA::FLAG_RESPAWN) != 0;
}

NiFixedString& FixedStrings::QBip01() {
    return *pBip01;
}

bool PlayerCharacter::GetTempPerson() {
    return bTemp1stPerson || bTemp3rdPerson;
}

void PlayerCharacter::ClearDroppedReference(TESObjectREFR* apRef) {
    if (apRef) {
        DroppedRefList.Remove(apRef);
    }
}

void FalloutAudio::LockAcousticSpace(bool abIn) {
    FalloutAudio::bLockAcousticSpace = abIn;
}

BSStringT<char>* DialogueResponse::GetText() {
    return &cText;
}

void BGSSaveFormBuffer::RemoveChange(int32_t aiFlags) {
    Header.iChangeFlags.RemoveFlags(aiFlags);
}

bool NiControllerManager::GetCumulativeAnimations() {
    return m_bCumulative;
}

void NiNode::DetachAllChildren() {
    uint32_t icount = m_kChildren.m_usSize;
    for (uint32_t i = 0; i < icount; ++i) {
        NiAVObject* pchild = m_kChildren[i].m_pObject;
        if (pchild) {
            pchild->DetachParent();
        }
    }
    m_kChildren.RemoveAll();
}

// KEEP: unregistered. With a null apNode the return value is whatever the frame setup left in the return register:
// the address of the function's exception registration record on the stack. src cannot express that return (the
// strict check DIFFs on it, checkbody 29d4f0babe86).
void NiNode::SetWireframe(NiNode* apNode) {
    if (apNode) {
        BSShaderNoLightingProperty* pProperty = new BSShaderNoLightingProperty;
        pProperty->SetFlag(BSShaderProperty::BSSP_FLAG_WIREFRAME, true);
        apNode->AttachProperty(pProperty);
        BSShaderManager::PrepareObject(apNode, false, false);
    }
}

NiVBChip* NiGeometryBufferData::GetVBChip(uint32_t uiIndex) {
    if (uiIndex >= m_uiStreamCount) {
        return nullptr;
    }
    return m_ppkVBChip[uiIndex];
}

hkUFloat8::hkUFloat8(const float f) {
    *this = f;
}

hkpPhantomCallbackShape::hkpPhantomCallbackShape() : hkpShape(HK_SHAPE_PHANTOM_CALLBACK) {}
