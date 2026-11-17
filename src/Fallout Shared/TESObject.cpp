#include "Fallout Shared/TESObject.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSBound.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/SpeedTree/BSTreeModel.h"
#include "Fallout Shared/SpeedTree/BSTreeNode.h"
#include "Fallout Shared/TES.h"
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
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectSTAT.h"
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
#include "Fallout Shared/TESWaterSystem.h"
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
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/FORenderedTerminal.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehdata.h"
#include "SDK/MSVC/iosfwd.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdexcept.h"
#include "SDK/MSVC/string.h"

#include <memory>
#include <stdint.h>
#include <string>

void DestroyRange(float* _First, float* _Last, [[maybe_unused]] std::allocator<float>& _Al) {
	(void)_First;
	(void)_Last;
}

NiNPShortPoint3::NiNPShortPoint3(int32_t aiX, int32_t aiY, int32_t aiZ)
	: x(static_cast<int16_t>(aiX)), y(static_cast<int16_t>(aiY)), z(static_cast<int16_t>(aiZ)) {}

float NiNPShortPoint3::GetFloatX() const {
	return (float)x;
}

float NiNPShortPoint3::GetFloatY() const {
	return (float)y;
}

float NiNPShortPoint3::GetFloatZ() const {
	return (float)z;
}

float NiNPShortPoint3::Length() const {
	float v2 = (float)(double)((z * z) + ((y * y) + (x * x)));
	return NiSqrt(v2);
}

// The component-wise difference this - arPoint, constructed in apResult.
NiNPShortPoint3* NiNPShortPoint3::CalculateRelativeBounds(NiNPShortPoint3* apResult,
	const NiNPShortPoint3& arPoint) const {
	::new (apResult) NiNPShortPoint3(x - arPoint.x, y - arPoint.y, z - arPoint.z);
	return apResult;
}

NiNPShortPoint3& NiNPShortPoint3::operator=(const NiNPShortPoint3& arPt) {
	x = arPt.x;
	y = arPt.y;
	z = arPt.z;
	return *this;
}

const NiNPShortPoint3& NiNPShortPoint3::GetZERO() {
	if ((ZERO_guard & 1) == 0) {
		ZERO_guard |= 1u;
		NiNPShortPoint3::ZERO.~NiNPShortPoint3();
		::new (&NiNPShortPoint3::ZERO) NiNPShortPoint3(0, 0, 0);
	}
	return NiNPShortPoint3::ZERO;
}

TESObject::TESObject() : m_pParentList(nullptr), nod_lpPrev(nullptr), nod_lpNext(nullptr) {}

TESObject::~TESObject() {
	if (m_pParentList) {
		m_pParentList->lnk_Remove(this);
	}
}

NiAVObject* TESObject::Clone3D(TESObjectREFR* pRequester, bool deepCopy) {
	return nullptr;
}

void TESObject::UnClone3D(TESObjectREFR* apRequester) {
	FNVD_INLINED_LOCAL(NiPointer<Model>, spmodel, std::nullopt, std::nullopt);
	if (this == pDoorMarker || this == pTravelMarker) {
		apRequester = 0;
	}
	const char* pworldFilename = GetModelFileName(apRequester);
	if (pworldFilename) {
		if (*pworldFilename) {
			if (!apRequester || apRequester->GetScale() == 1.0) {
				pModelLoader->ReleaseFile(pworldFilename);
			} else {
				char cPath[264];
				GetScaledModelFileName(apRequester, cPath);
				pModelLoader->ReleaseFile(cPath);
				bool blastCopy = 1;
				::new (&spmodel) NiPointer<Model>(static_cast<Model*>(0));
				if (pModelLoader->FindModel(cPath, spmodel) && spmodel.m_pObject->iManualRefCount > 0) {
					blastCopy = 0;
				}
				if (blastCopy) {
					pModelLoader->ReleaseFile(pworldFilename);
					--iNumScaledModelsAdded;
				} else {
					--iNumScaledModelsCloned;
				}
				NiPointer<Model>::Destroy(&spmodel);
			}
		}
	}
	if (GetFormType() == NPC__ID) {
		static_cast<TESNPC*>(this)->TESNPC::ClearFaceGenModels();
	}
}

bool TESObject::IsMarker() {
	if (!this) {
		return false;
	}
	switch (GetFormType()) {
	case TXST_ID:
	case IDLM_ID:
		return true;
	case ACTI_ID:
		if (!BSstricmp(TESModel::GetModel(this), "EditorMarker.NIF")) {
			return true;
		}
		break;
	case DOOR_ID:
		if (pJailMarker == this) {
			return true;
		}
		break;
	case LIGH_ID:
		if (!TESModel::GetModelLength_00489540(this)) {
			return true;
		}
		break;
	case STAT_ID:
		if (pDoorMarker == this || pTravelMarker == this || pTempleMarker == this || pHolyMarker == this ||
			pNorthMarker == this || pMapMarker == this || pHorseMarker == this || pXMarker == this ||
			pXMarkerHeading == this || pCOCMarkerHeading == this || pBoundMarker == this || pRadiationMarker == this ||
			pAudioMarker == this || pAudioBuoyMarker == this) {
			return true;
		}
		break;
	default:
		break;
	}
	// The form ID is tested unsigned.
	uint32_t uiFormID = iFormID;
	return uiFormID >= 100 && uiFormID <= 119;
}

bool TESObject::IsOcclusionMarker() {
	if (!this) {
		return 0;
	}
	return GetFormType() == 32 && (pRoomMarker == this || (pPlaneMarker == this || pPortalMarker == this));
}

bool TESObject::ReplaceModel() {
	NiUpdateData a2;
	NiNode* v3 = static_cast<NiNode*>(LoadGraphics(nullptr));
	v3->UpdateProperties();
	a2.~NiUpdateData();
	::new (&a2) NiUpdateData(0.0, 0, 0);
	v3->Update(a2);
	return (bool)1;
}

NiAVObject* TESObject::LoadGraphics(TESObjectREFR* apRef) {
	FNVD_INLINED_LOCAL(NiPointer<Model>, spmodel_2, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiPointer<Model>, spmodel, std::nullopt, std::nullopt);
	const char* pworldFilename = GetModelFileName(apRef);
	NiAVObject* pData3D = 0;
	if (apRef && (apRef->IsMarker() && apRef->GetObjectReference()->GetFormType() != LIGH_ID)) {
		NiAVObject* pData3D_2;
		::new (&spmodel) NiPointer<Model>(static_cast<Model*>(0));
		pModelLoader->FindModel(pworldFilename, spmodel);
		if (spmodel.m_pObject) {
			spmodel.m_pObject->IncManualRefCount();
		}
		BSFadeNode* apMemory = static_cast<BSFadeNode*>(NiMemObject::operator new(sizeof(BSFadeNode)));
		if (apMemory) {
			pData3D_2 = ::new (apMemory) BSFadeNode;
		} else {
			pData3D_2 = 0;
		}
		pData3D = pData3D_2;
		NiPointer<Model>::Destroy(&spmodel);
	} else {
		ENUM_LOD_MULT elod = TES::GetLODMult(this);
		if (apRef) {
			if (apRef->GetScale() != 1.0) {
				char cScaledPath[264];
				GetScaledModelFileName(apRef, cScaledPath);
				::new (&spmodel_2) NiPointer<Model>(static_cast<Model*>(0));
				pModelLoader->FindModel(cScaledPath, spmodel_2);
				if (spmodel_2.m_pObject) {
					spmodel_2.m_pObject->IncManualRefCount();
					pData3D = spmodel_2.m_pObject->GetModel();
					++iNumScaledModelsCloned;
				}
				NiPointer<Model>::Destroy(&spmodel_2);
			}
		}
		if (!pData3D) {
			return pModelLoader->LoadFile(pworldFilename, elod, 1, 0, 0, 0);
		}
	}
	return pData3D;
}

NiAVObject* TESObject::Get3D() {
	return nullptr;
}

const char* TESObject::GetModelFileName(TESObjectREFR* apRef) {
	const char* pPath = 0;
	if (apRef) {
		pPath = apRef->GetModel();
	}
	if (!pPath) {
		pPath = TESBipedModelForm::GetWorldModel(this, apRef);
	}
	if (!pPath) {
		return TESModel::GetModel(this);
	}
	return pPath;
}

void TESObject::GetScaledModelFileName(TESObjectREFR* apRef, char* apScaledFileName) {
	const char* pworldFilename = GetModelFileName(apRef);
	float fHeight = apRef->GetScale() * 100.0;
	BSsprintf(apScaledFileName, 260u, "%s%i", pworldFilename, FloatToInt(fHeight));
}

// The constructor deliberately leaves both points unset, as the game does.
TESBoundObject::BOUND_DATA::BOUND_DATA() {}

void TESBoundObject::BOUND_DATA::Endian() {
	ENDIAN(BoundMax.x, false);
	ENDIAN(BoundMax.y, false);
	ENDIAN(BoundMax.z, false);
	ENDIAN(BoundMin.x, false);
	ENDIAN(BoundMin.y, false);
	ENDIAN(BoundMin.z, false);
}

TESBoundObject::TESBoundObject() {
	BoundData.BoundMin = NiNPShortPoint3::GetZERO();
	BoundData.BoundMax = NiNPShortPoint3::GetZERO();
}

TESBoundObject::~TESBoundObject() {}

void TESBoundObject::SaveObjectBound() {
	if (TESForm::QEndianSwapOnSave()) {
		(&BoundData)->Endian();
	}
	TESForm::__AddChunkData(OBND_ID, &BoundData, 0xCu);
	if (TESForm::QEndianSwapOnSave()) {
		(&BoundData)->Endian();
	}
}

void TESBoundObject::LoadObjectBound(TESFile* apFile) {
	if (apFile) {
		if (((int32_t)apFile->GetTESChunk()) == 1145979471) {
			apFile->GetChunkData(&BoundData, 0xCu);
			if (apFile->QEndian()) {
				(&BoundData)->Endian();
			}
		}
	}
}

bool TESBoundObject::Activate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abIdFlag,
	TESBoundObject* apObjecttoGet, int32_t aicount) {
	Actor* v7 = dynamic_cast<Actor*>(apActionRef);
	if ((!v7) || (!apItemActivated)) {
		return (bool)0;
	}
	v7->PickUpObject(apItemActivated, aicount, abIdFlag);
	return (bool)1;
}

// The requester's copy of this object's 3D. A loaded model that only this caller holds is shared as is, unless the
// object needs its own copy (morphers, a door without an Unequip sequence, a ragdoll, add-ons, texture swaps,
// emittance, a scale other than the model's); otherwise it is cloned (deep, with the same textures, when deepCopy), and
// a scaled copy is registered with the model loader under the scaled file name. The scaled clone is built in a second
// cloning process, separate from kCloneProc, which stays live until the end. The requester's scale and the scaled
// model's 3D scale are stored as doubles before the compares, so those locals are doubles.
NiAVObject* TESBoundObject::Clone3D(TESObjectREFR* pRequester, bool deepCopy) {
	NiAVObject* pNode = nullptr;
	if (this == pDoorMarker || this == pTravelMarker) {
		pRequester = nullptr;
	}
	NiAVObject* pLoadedNode = LoadGraphics(pRequester);
	if (pRequester && pRequester->IsMarker() && pRequester->GetObjectReference()->GetFormType() != LIGH_ID) {
		return pLoadedNode;
	}
	if (!pLoadedNode) {
		return pNode;
	}
	if (GetNoCollision() && pRequester) {
		pRequester->SetNoCollision(true);
	}
	if (!deepCopy && pLoadedNode->GetRefCount() == 1) {
		bool bShare = false;
		switch (GetFormType()) {
		case ACTI_ID:
		case CREA_ID:
			// Only sets the flag; it was clear before this point.
			if (HasMorpherController(static_cast<NiNode*>(pLoadedNode))) {
				deepCopy = true;
			}
			break;
		case DOOR_ID: {
			bShare = true;
			NiNode* pDoorNode = pLoadedNode->IsNode();
			if (pDoorNode && pDoorNode->GetAtChecked(0) && pDoorNode->GetAt(0)->GetControllers()) {
				NiTimeController* pControllers = pDoorNode->GetAt(0)->GetControllers();
				NiControllerManager* pControllerMgr =
					static_cast<NiControllerManager*>(NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI,
						pControllers));
				if (pControllerMgr && pControllerMgr->GetSequenceByName(FixedStrings::QUnequip())) {
					bShare = false;
				}
			}
			break;
		}
		case PWAT_ID:
		case WEAP_ID:
		case AMMO_ID:
		case NPC__ID:
		case PROJ_ID:
		case EXPL_ID:
			break;
		default:
			bShare = !TESHavokUtilities::Should3DRagDoll(pRequester, pLoadedNode);
			break;
		}
		if (bShare) {
			BSXFlags* pBSXFlags = static_cast<BSXFlags*>(pLoadedNode->GetExtraData(BSXFlags::Tag()));
			// Tests only the low byte of QAddon's result.
			if (pBSXFlags && static_cast<uint8_t>(pBSXFlags->QAddon())) {
				bShare = false;
			}
		}
		if (bShare && TESModelTextureSwap::HasTextureSwap(this, pRequester)) {
			bShare = false;
		}
		if (bShare && BSUtilities::GetExternalEmittanceRecurse(pLoadedNode)) {
			bShare = false;
		}
		if (bShare) {
			if (!pRequester) {
				pNode = pLoadedNode;
			} else {
				double dScaledHeight = pRequester->GetScale();
				if (dScaledHeight == pLoadedNode->GetScale()) {
					pNode = pLoadedNode;
				}
			}
		}
	}
	if (!pNode) {
		float fScaledHeight = 1.0f;
		if (pRequester) {
			fScaledHeight = pRequester->GetScale();
		}
		if (fScaledHeight == pLoadedNode->GetScale()) {
			fScaledHeight = 1.0f;
		}
		if (fScaledHeight == 0.0) {
			Warning("MODELS: %s has a scale of 0.  Change it in the editor.", pRequester->GetFullName());
			fScaledHeight = 1.0f;
		}
		BSCloningProcess kCloneProc;
		kCloneProc.~BSCloningProcess();
		::new (&kCloneProc) BSCloningProcess(fScaledHeight);
		if (deepCopy) {
			NiObject* pClonedObject = TES::pInstance->CreateDeepCopySameTextures(pLoadedNode, kCloneProc);
			if (pClonedObject && pClonedObject->IsNode()) {
				pNode = pClonedObject->IsNode();
			} else {
				void* pMemory = NiMemObject::operator new(sizeof(NiNode));
				NiNode* pNewNode = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
				pNewNode->AttachChild(static_cast<NiAVObject*>(pClonedObject), true);
				pNode = pNewNode;
			}
		} else {
			pNode = static_cast<NiAVObject*>(pLoadedNode->Clone(kCloneProc));
		}
		if (fScaledHeight != 1.0 && pLoadedNode->GetScale() == 1.0) {
			pNode->SetScale(fScaledHeight);
			char cScaledPath[0x104];
			GetScaledModelFileName(pRequester, cScaledPath);
			alignas(NiPointer<Model>) uint8_t aScaledModelStorage[sizeof(NiPointer<Model>)];
			NiPointer<Model>& spScaledModel = *::new (aScaledModelStorage) NiPointer<Model>(nullptr);
			pModelLoader->FindModel(cScaledPath, spScaledModel);
			if (spScaledModel.operator->()) {
				if (spScaledModel.operator->()->GetModel()) {
					double dLocalScale = spScaledModel.operator->()->GetModel()->GetScale();
					if (dLocalScale == pRequester->GetScale()) {
						NiAVObject* pResult = Clone3D(pRequester, deepCopy);
						NiPointer<Model>::Destroy(&spScaledModel);
						return pResult;
					}
				}
				Warning("Invalid scale found on ModelLoader copy of file '%s'", cScaledPath);
			} else {
				void* pMemory = ourMalloc(sizeof(Model));
				Model* pScaledModel =
					pMemory ? ::new (pMemory) Model(cScaledPath, static_cast<NiNode*>(pNode)) : nullptr;
				spScaledModel = pScaledModel;
				spScaledModel.operator->()->IncManualRefCount();
				if (pModelLoader->AddModel(cScaledPath, spScaledModel.operator->())) {
					++iNumScaledModelsAdded;
				} else {
					Model* pFailedModel = spScaledModel.operator->();
					spScaledModel = static_cast<Model*>(nullptr);
					if (pFailedModel) {
						delete pFailedModel;
					}
				}
				BSCloningProcess kScaledCloneProc;
				kScaledCloneProc.~BSCloningProcess();
				::new (&kScaledCloneProc) BSCloningProcess(1.0f);
				pNode = static_cast<NiAVObject*>(pNode->Clone(kScaledCloneProc));
			}
			NiPointer<Model>::Destroy(&spScaledModel);
		}
	}
	if (!pNode) {
		return pNode;
	}
	pNode->SetScale(1.0f);
	pNode->SetAppCulled(false);
	pNode->SetRotate(NiMatrix3_IDENTITY);
	pNode->SetTranslate(NiPoint3_ZERO);
	if (pNode->GetParent()) {
		pNode->DetachParent();
	}
	return pNode;
}

bool TESBoundObject::ReplaceModel() {
	const char* Model_ = TESModel::GetModel(this);
	return ReplaceModel(Model_);
}

NiAVObject* TESBoundObject::Clone3D(TESObjectREFR* pRequester) {
	return Clone3D(pRequester, 0);
}

bool TESBoundObject::ReplaceModel(const char* pfilename) {
	const char* Model_ = TESModel::GetModel(this);
	pModelLoader->ReleaseFile(Model_);
	NiNode* v6 = static_cast<NiNode*>(LoadGraphics(0));
	v6->UpdateProperties();
	NiUpdateData a2(0.0, 0, 0);
	v6->Update(a2);
	return 1;
}

// Half the length of the bound box's diagonal.
float TESBoundObject::GetBoundSize() {
	NiNPShortPoint3 kDiff;
	return BoundData.BoundMin.CalculateRelativeBounds(&kDiff, BoundData.BoundMax)->Length() * 0.5;
}

// Reads the coordinates z first, then y, then x, as the game does.
NiPoint3 TESBoundObject::GetBoundMin() {
	float fZ = BoundData.BoundMin.GetFloatZ();
	float fY = BoundData.BoundMin.GetFloatY();
	float fX = BoundData.BoundMin.GetFloatX();
	FNVD_INLINED_LOCAL(NiPoint3, kPoint, std::nullopt, std::nullopt);
	return *::new (&kPoint) NiPoint3(fX, fY, fZ);
}

NiPoint3 TESBoundObject::GetBoundMax() {
	float fZ = BoundData.BoundMax.GetFloatZ();
	float fY = BoundData.BoundMax.GetFloatY();
	float fX = BoundData.BoundMax.GetFloatX();
	// As in GetBoundMin, the point is built in a frame local and copied into the result.
	FNVD_INLINED_LOCAL(NiPoint3, kPoint, std::nullopt, std::nullopt);
	return *::new (&kPoint) NiPoint3(fX, fY, fZ);
}

void TESBoundObject::CopyBoundData(TESBoundObject* apCopy) {
	ourMemcpy(&BoundData, &apCopy->BoundData, sizeof(BoundData));
}

TESBoundAnimObject::TESBoundAnimObject() {}

TESBoundAnimObject::~TESBoundAnimObject() {}

// Every derived class whose slot holds this address inherits it.
void TESBoundAnimObject::Set3D(NiAVObject* apObject, bool abTestProcessLevel) {
	TESObject::Set3D(apObject, abTestProcessLevel);
}

bool TESBoundAnimObject::ReplaceModel(const char* pfilename) {
	return (bool)((uint8_t)TESBoundObject::ReplaceModel(pfilename));
}

TESBoundTreeObject::TESBoundTreeObject() {}

TESBoundTreeObject::~TESBoundTreeObject() {}

NiAVObject* TESBoundTreeObject::Clone3D(TESObjectREFR* pRequester, bool deepCopy) {
	BSTreeNode* pnode = 0;
	if (GetFormType() != TREE_ID) {
		return 0;
	}
	if (BSTreeManager::GetInstance(1)) {
		BSTreeManager* Singleton = BSTreeManager::GetInstance(1);
		pnode = Singleton->CreateTreeForReference(pRequester, static_cast<TESObjectTREE*>(this));
	}
	RefreshBound(pnode);
	if (pnode) {
		if (pnode->GetModel()) {
			if (pRequester) {
				BSTreeModel* v5 = pnode->GetModel();
				pRequester->SetSeed(v5->iSeed);
			}
		}
	}
	return pnode;
}

TESObjectList::TESObjectList(bool modelload)
	: lnk_dwCount(0), lnk_lpRoot(nullptr), lnk_lpTail(nullptr), pOwner(nullptr) {}

TESObjectList::~TESObjectList() {
	lnk_Destroy();
}

// Links lpNew in after the tail.
TESObject* TESObjectList::lnk_Append(TESObject* lpNew) {
	return lnk_Place(lnk_lpTail, lpNew);
}

TESObject* TESObjectList::lnk_Place(TESObject* lpPrev, TESObject* lpNew) {
	lpNew->nod_lpPrev = lpPrev;
	if (lpPrev) {
		lpNew->nod_lpNext = lpPrev->nod_lpNext;
		if (lpPrev->nod_lpNext) {
			lpPrev->nod_lpNext->nod_lpPrev = lpNew;
		}
		lpPrev->nod_lpNext = lpNew;
	} else {
		lpNew->nod_lpNext = 0;
	}
	if (!lnk_GetNext(lpNew)) {
		lnk_lpTail = lpNew;
	}
	if (!lnk_GetPrev(lpNew)) {
		lnk_lpRoot = lpNew;
	}
	++lnk_dwCount;
	lpNew->m_pParentList = this;
	return lpNew;
}

TESObject* TESObjectList::lnk_GetNext(TESObject* lpNode) {
	return lpNode->nod_GetNext();
}

void TESObjectList::lnk_Remove(TESObject* lpNode) {
	if (lpNode == lnk_lpRoot) {
		lnk_lpRoot = lnk_lpRoot->nod_lpNext;
	}
	if (lpNode == lnk_lpTail) {
		lnk_lpTail = lnk_lpTail->nod_lpPrev;
	}
	TESObject* pPrev = lpNode->nod_lpPrev;
	if (pPrev) {
		pPrev->nod_lpNext = lpNode->nod_lpNext;
	}
	TESObject* pNext = lpNode->nod_lpNext;
	if (pNext) {
		pNext->nod_lpPrev = lpNode->nod_lpPrev;
	}
	lpNode->m_pParentList = 0;
	--lnk_dwCount;
}

void TESObjectList::lnk_Destroy() {
	while (lnk_lpTail) {
		TESObject* pPrev = lnk_lpTail->nod_lpPrev;
		if (lnk_lpTail) {
			delete lnk_lpTail;
		}
		lnk_lpTail = pPrev;
	}
	lnk_dwCount = 0;
	lnk_lpRoot = 0;
	lnk_lpTail = 0;
}

TESObject* TESObjectList::lnk_GetPrev(TESObject* lpNode) {
	return lpNode->nod_lpPrev;
}

void BSBound::GetBoundMin(NiPoint3& arPoint) {
	NiPoint3 v2;
	arPoint = *&(v2 = (&Center)->operator-(Extents));
}

void BSBound::GetBoundMax(NiPoint3& arPoint) {
	NiPoint3 v2;
	arPoint = *&(v2 = (&Center)->operator+(Extents));
}

const NiFixedString& BSBound::Tag() {
	return *BSBound::pTag;
}

NiFixedString& FixedStrings::QUnequip() {
	return *pUnequip;
}
