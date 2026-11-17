#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkLiquidAction.h"
#include "BSHavok/bhkPoseArray.h"
#include "BSHavok/bhkRagdollShareData.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShaderBuffer.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SkyShader.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AnimSequenceBase.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/Havok/TESWaterListener.h"
#include "Fallout Shared/IDLE_ANIM_ROOT.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/LockFreeMap.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoMap.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleTask.h"
#include "Fallout Shared/Pathfinding/NavMeshPOVSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/NavMesh_Util.h"
#include "Fallout Shared/Pathfinding/ObstacleTaskNavMeshOperation.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLOSGrid.h"
#include "Fallout Shared/Pathfinding/PathingLOSGridMap.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/SpeedTree/BSTreeModel.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESGrassAreaParam.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectTREE.h"
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
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TEX_USER_DATA.h"
#include "Fallout Shared/WadingWaterData.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Debug/DebugTextPages/IDebugTextPage.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSReconstructFormsInAllFilesMap.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadChangesMap.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGlobalData.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadReferencesMap.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadStructs.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSequence.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9DynamicTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderedCubeMapData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderedTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDevImageConverter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedCubeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderLibrary.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreator.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramFactory.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBChip.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBDynamicSet.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBSet.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/string.h"

#include <stdint.h>

// The exe folds the SetValue of every map with a 4-byte key and a bool value (vtable slot 3; IDA's
// NiTMapBase<DWORD,bool>); this instantiation stands for them.
using NiTMapBaseUInt32Bool = NiTMapBase<DFALL<NiTMapItem<uint32_t, bool>>, uint32_t, bool>;

// The save loader's map of form headers by a 1-byte key: the key's zero-extended value modulo the bucket count
// (slot 1) and a byte compare of the two keys (slot 2).
using NiTMapBaseUInt8LoadFormHeaderList =
	NiTMapBase<NiTPointerAllocator<size_t>, uint8_t, BSSimpleList<LoadFormHeader*>*>;

// The exe folds the pointer-key KeyToHashIndex (slot 1, 0xA63840) and IsKeysEqual (slot 2, 0xE92590) of every map
// with a 4-byte key (NiTPointerMap<const char*, NiObject* (*)()>, NiTMap<NiAVObject*, short>, ...); this
// instantiation, the first the exe's vtables name, stands for them.
using NiTMapBaseCreateFunctions = NiTMapBase<NiTPointerAllocator<size_t>, const char*, NiObject* (*)()>;
// IsKeysEqual (0xE92590) stays unregistered: the exe zeroes all of eax before its sete, but the template's bool
// return sets only al and leaves the key's upper bytes in eax (strict: exe 0x1, src 0xffffff01), which src cannot
// express.

// DeleteItem (slot 6) of the maps from a pointer key to a 2-byte value (IDA's NiTMap<NiAVObject*, short>): the exe
// clears only the value before it frees the item.
using NiTMapAVObjectShort = NiTMap<NiAVObject*, int16_t>;
// DeleteItem (slot 6) folded across the maps with a 4-byte value (NiTMap<NiAVObject*,
// BSSimpleList<bhkLiquidAction*>*>, NiTMap<_D3DFORMAT, NiPixelFormat*>, ...); the first instantiation the exe's
// vtables name stands for them.
using NiTMapAVObjectLiquidActions = NiTMap<NiAVObject*, BSSimpleList<bhkLiquidAction*>*>;
// DeleteItem (slot 6) of the string-to-string maps (NiTMap<const char*, char*>; NiTStringMap<char*> and
// BSTCaseInsensitiveStringMap<char*> share the slot): the exe clears the value before it frees the item.
using NiTMapStringString = NiTMap<const char*, char*>;
// The map from an 8-byte key to a 4-byte value (IDA's NiTMap<long long, int>): the hash is the key's low dword
// modulo the bucket count (slot 1, div).
using NiTMapBaseInt64Int = NiTMapBase<DFALL<NiTMapItem<int64_t, int32_t>>, int64_t, int32_t>;

// Its NewItem (slot 5) calls the node allocation out of line (0x84BB30, a forwarder in NiTMap.h).
using NiTMapInt64Int = NiTMap<int64_t, int32_t>;
// SetValue (slot 3, 0x84B990) stays unregistered: the exe returns pkItem in eax (stdcall-style, ret 0x10), but the
// template's SetValue returns void, which src cannot express (strict check: return differs).
// SetValue (slot 3, 0x993B30) of the maps from a 4-byte key to an 8-byte value (NiTMap<unsigned int,
// ReconstructFormData>, NiTMap<Actor*, long long>, NiTMap<LowProcess*, LP_LOCK_DATA>) stays unregistered for the same
// reason: the exe leaves the value's low dword in eax, which the template's void SetValue does not reproduce.

// NewItem (slot 5) of every NiTPointerMap (BGSSaveLoadChangesMap, NiTPointerMap<NiObject*, NiObject*>,
// NiTPointerMap<TESObjectREFR*, bool>, ...): the exe folds them into one body; this instantiation, the first the
// exe's vtables name, stands for them.
using NiTPointerMapObjectObject = NiTPointerMap<NiObject*, NiObject*>;

// IsKeysEqual (slot 2) and NewItem (slot 5) of the default-allocator maps with a 4-byte key and a 4-byte value
// (NiTMap<const char*, Setting*>, NiTMap<TESObjectREFR*, bool>, NiTMap<WaterZone*, int>, BSMap<TESFaction*, bool>,
// ...): the exe folds each into one body; this instantiation, the first the exe's vtables name, stands for them.
using NiTMapBaseStringSetting = NiTMapBase<DFALL<NiTMapItem<const char*, Setting*>>, const char*, Setting*>;
using NiTMapStringSetting = NiTMap<const char*, Setting*>;

using NiTMapBaseStringForm = NiTMapBase<DFALL<NiTMapItem<const char*, TESForm*>>, const char*, TESForm*>;

using NiTMapBaseIntInt = NiTMapBase<DFALL<NiTMapItem<int32_t, int32_t>>, int32_t, int32_t>;

using NiTMapBase_lNiTPointerAllocator_lunsigned_sint_g_cunsigned_sshort_cAnimSequenceBase_p =
	NiTMapBase<NiTPointerAllocator<uint32_t>, uint16_t, AnimSequenceBase*>;

using NiTMapBase_DFALL_NiTMapItem_NiAVObject_short_NiAVObject_short =
	NiTMapBase<DFALL<NiTMapItem<NiAVObject*, int16_t>>, NiAVObject*, int16_t>;

// Its out-of-line constructor (IDA's ??0?$NiTMapBase@V?$DFALL@V?$NiTMapItem@_JH@@@@_JH@@6A@_0; the PDB has no
// name for it): the vtable, the bucket count, an empty item count and the zeroed bucket table.
inline constexpr fnvd::Vtable NiTMapBaseInt64Int_vftable{};

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
char* ConstructWorldLocation(void* apThis, [[maybe_unused]] void* apEdx) {
	::new (apThis) WORLD_LOCATION;
	return static_cast<char*>(static_cast<void*>(static_cast<int8_t*>(apThis)));
}

// Constructs a NiTMapItem<TESForm *, BGSLoadGameSubBuffer> in apThis (thiscall): the memory comes first, then the
// unused edx.
void* ConstructMapItem(void* apThis, [[maybe_unused]] void* apEdx) {
	::new (apThis) NiTMapItem<TESForm*, BGSLoadGameSubBuffer>;
	return apThis;
}
