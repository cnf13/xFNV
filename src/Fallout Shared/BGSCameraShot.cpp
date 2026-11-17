#include "Fallout Shared/BGSCameraShot.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESImageSpaceModifiableForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPathInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSingleInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

// The game settings this file defines, with the game's IDs and values, in initialization order.
NiPoint3 currentCameraPos(NiPoint3_ZERO);
NiPoint3 dollyStartingPos(NiPoint3_ZERO);
NiPoint3 dollyDelta(NiPoint3_ZERO);
SettingT<GameSettingCollection> fVATSCameraDollyTime("fVATSCameraDollyTime", 1.0f);
SettingT<GameSettingCollection> fVATSCameraDollyMin("fVATSCameraDollyMin", 256.0f);
SettingT<GameSettingCollection> fVATSCameraCutAwayDistance("fVATSCameraCutAwayDistance", 25.0f);
SettingT<GameSettingCollection> fVATSCameraEffectVolume("fVATSCameraEffectVolume", 1.0f);

// The time multipliers first, then the enums and flags, then the times and the target percentage; the order is the
// game's.
void BGSCameraShot::CAMERA_SHOT_DATA::Endian() {
	ENDIAN(fPlayerTimeMult, false);
	ENDIAN(fTargetTimeMult, false);
	ENDIAN(fGlobalTimeMult, false);
	ENDIAN(eCameraAction, false);
	ENDIAN(eLocation, false);
	ENDIAN(eTarget, false);
	ENDIAN(iFlags, false);
	ENDIAN(fMaxTime, false);
	ENDIAN(fMinTime, false);
	ENDIAN(fTargetPercentBetweenActors, false);
}

// The time multipliers first, then the enums and flags, then the times; the order is the game's.
void BGSCameraShot::OLD_CAMERA_SHOT_DATA::Endian() {
	ENDIAN(fPlayerTimeMult, false);
	ENDIAN(fGlobalTimeMult, false);
	ENDIAN(eCameraAction, false);
	ENDIAN(eLocation, false);
	ENDIAN(eTarget, false);
	ENDIAN(iFlags, false);
	ENDIAN(fMaxTime, false);
	ENDIAN(fMinTime, false);
}

BGSCameraShot::BGSCameraShot()
	: pTargetRef(nullptr), cCameraHasRotation(-1), bCameraTooClose(false), bCameraIsKillCam(false) {
	cFormType = CAMS_ID;
	BGSCameraShot::InitializeData();
}

BGSCameraShot::~BGSCameraShot() {
	FreeCamera();
}

void BGSCameraShot::InitializeData() {
	Data.eCameraAction = CA_HIT;
	Data.eLocation = CO_TARGET;
	Data.eTarget = CO_ATTACKER;
	Data.fGlobalTimeMult = (float)1.0;
	Data.fPlayerTimeMult = (float)0.0;
	Data.fTargetTimeMult = (float)0.0;
	Data.iFlags = (uint32_t)3;
	Data.fMaxTime = (float)0.0;
	Data.fMinTime = (float)0.0;
	BGSCameraShot* result = this;
	Data.fTargetPercentBetweenActors = (float)0.0;
}

// Reads the shot's model, its data (the older 0x20-byte layout or the 0x28-byte one) and its image space modifier.
bool BGSCameraShot::Load(TESFile* apFile) {
	if (apFile->GetTESForm() != CAMS_ID) {
		return false;
	}
	LoadForm(apFile);
	do {
		uint32_t uiChunk = apFile->GetTESChunk();
		if (uiChunk == NO_CHUNK) {
			break;
		}
		if (uiChunk > MODL_ID) {
			if (uiChunk == MNAM_ID) {
				// The form ID is stored straight into the component's pointer field, as the game does.
				uint32_t uiFormID = 0;
				apFile->GetChunkData(uiFormID);
				SetFormImageSpaceModifying(reinterpret_cast<TESImageSpaceModifier*>(static_cast<uintptr_t>(uiFormID)));
			} else if (uiChunk == MODT_ID) {
				TESModel::LoadModelChunk(this, apFile);
			}
		} else {
			switch (uiChunk) {
			case MODL_ID:
				TESModel::LoadModelChunk(this, apFile);
				break;
			case DATA_ID:
				if (apFile->GetChunkSize() == 0x20) {
					// the older files' 0x20-byte layout, expanded into the current one
					OLD_CAMERA_SHOT_DATA kOldData;
					ourMemset(&kOldData, 0, sizeof(kOldData));
					LoadData(apFile, reinterpret_cast<char*>(&kOldData), 0x20);
					if (apFile->QEndian()) {
						kOldData.Endian();
					}
					Data.eCameraAction = kOldData.eCameraAction;
					Data.eLocation = kOldData.eLocation;
					Data.eTarget = kOldData.eTarget;
					Data.iFlags = kOldData.iFlags;
					Data.fPlayerTimeMult = kOldData.fPlayerTimeMult;
					Data.fGlobalTimeMult = kOldData.fGlobalTimeMult;
					Data.fMaxTime = kOldData.fMaxTime;
					Data.fMinTime = kOldData.fMinTime;
				} else {
					ourMemset(&Data, 0, sizeof(Data));
					apFile->GetChunkData(&Data, 0x28);
					if (apFile->QEndian()) {
						Data.Endian();
					}
				}
				break;
			case EDID_ID: {
				// A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
				char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
				apFile->GetChunkData(pBuffer, 0x200);
				SetFormEditorID(pBuffer);
				break;
			}
			case OBND_ID:
				LoadObjectBound(apFile);
				break;
			}
		}
	} while (apFile->NextChunk());
	return true;
}

void BGSCameraShot::Save() {
	TESForm::StartForm();
	static_cast<TESModel*>(this)->Save(MODL_ID, MODT_ID);
	if (TESForm::QEndianSwapOnSave()) {
		(&Data)->Endian();
	}
	TESForm::__SaveData(&Data, 0x28u);
	if (TESForm::QEndianSwapOnSave()) {
		(&Data)->Endian();
	}
	(static_cast<TESImageSpaceModifiableForm*>(this))->Save();
	TESForm::CloseForm();
}

void BGSCameraShot::InitItem() {
	if (!GetInitialized()) {
		TESImageSpaceModifiableForm::InitItem(this);
		if (Data.fGlobalTimeMult < 9.999999747378752e-05) {
			const char* v1 = GetFormEditorID();
			Warning("Bad VATS CameraShot Data: '%s' has a Global Time Multiplier that is too small.", v1);
			Data.fGlobalTimeMult = 0.1f;
		}
		SetInitialized(1);
	}
}

void BGSCameraShot::Copy(TESForm* apCopy) {
	TESForm* v3 =
		static_cast<TESForm*>(FNVD_DYNAMIC_CAST_VOID(BGSCameraShot)(apCopy, RTTI_TESForm, RTTI_BGSCameraShot));
	if (v3) {
		CopyAllComponents(v3);
		ourMemcpy(&Data, &v3[2].iFormFlags, sizeof(Data));
	}
}

bool BGSCameraShot::Compare(TESForm* apCompare) {
	TESForm* v4 =
		static_cast<TESForm*>(FNVD_DYNAMIC_CAST_VOID(BGSCameraShot)(apCompare, RTTI_TESForm, RTTI_BGSCameraShot));
	if (!v4) {
		return 1;
	}
	if (CompareAllComponents(v4)) {
		return 1;
	}
	return std::memcmp(&Data, &v4[2].iFormFlags, sizeof(Data)) != 0;
}

float BGSCameraShot::GetTargetTimeMult() {
	return Data.fTargetTimeMult;
}

BGSCameraShot::CAM_ACTION BGSCameraShot::GetCameraAction() {
	return Data.eCameraAction;
}

bool BGSCameraShot::GetPosFollowsTarget() {
	return ((Data.iFlags & 1) != 0);
}

bool BGSCameraShot::GetRotFollowsTarget() {
	return ((Data.iFlags & 2) != 0);
}

bool BGSCameraShot::GetIsFirstPersonCamera() {
	return ((Data.iFlags & 8) != 0);
}

bool BGSCameraShot::IsInitialized() {
	if (Data.eCameraAction == 3) {
		return (bool)0;
	}
	if (static_cast<TESModel*>(this)->GetModelLength()) {
		return ((&spCameraNode)->operator->() != 0);
	}
	return (bool)1;
}

bool BGSCameraShot::IsStartAtTimeZero() {
	return ((Data.iFlags & 32) != 0);
}

void BGSCameraShot::FreeCamera() {
	FNVD_INLINED_LOCAL(BSSoundHandle, soundHand, std::nullopt, std::nullopt);
	spLocationNode.operator=(static_cast<NiAVObject*>(0));
	spTargetNode.operator=(static_cast<NiAVObject*>(0));
	pTargetRef = 0;
	spImagespaceTarget.operator=(static_cast<NiAVObject*>(0));
	bCameraTooClose = 0;
	if (spCameraNode.m_pObject) {
		spCameraNode.operator=(static_cast<NiNode*>(0));
		const char* v1 = GetModel();
		pModelLoader->ReleaseFile(v1);
	}
	if (bPlayCameraZoomSounds && !pVATS.GetCount()) {
		bPlayCameraZoomSounds = 0;
		BSAudio* v2 = &BSAudio::QInstance();
		v2->GetSoundHandleByName(&soundHand, "UIVATSCameraOut", 289u);
		soundHand.Play(0);
	}
}

bool BGSCameraShot::GetFollowsBone() {
	return (((int32_t)(Data.iFlags & 4)) == 0);
}

bool BGSCameraShot::GetNoTracer() {
	return (((int32_t)(Data.iFlags & 16)) != 0);
}

NiAVObject* BGSCameraShot::GetLocationNode() {
	return spLocationNode;
}

NiAVObject* BGSCameraShot::GetTargetNode() {
	return spTargetNode;
}

NiAVObject* BGSCameraShot::GetImagespaceTarget() {
	return spImagespaceTarget;
}

void BGSCameraShot::SetCameraLocation(NiAVObject* apLocation) {
	NiAVObject* v2;
	NiAVObject* v3;
	NiMatrix3* orientation;
	const NiPoint3* worldTranslate;
	NiNode* v6;
	NiMatrix3 result;
	void* apMemory;
	NiRefObject* a2;
	int32_t v11;
	if (GetPosFollowsTarget()) {
		(&spLocationNode)->operator=(apLocation);
	} else {
		if (apLocation) {
			apMemory = NiMemObject::operator new(sizeof(NiNode));
			v11 = 0;
			if (apMemory) {
				v6 = ::new (apMemory) NiNode(0);
			} else {
				v6 = nullptr;
			}
			a2 = v6;
			v11 = (-1);
			(&spLocationNode)->operator=(v6);
			if (pTargetRef) {
				orientation = &(result = pTargetRef->GetOrientation());
				v2 = (&spLocationNode)->m_pObject;
				v2->SetRotate(*orientation);
			}
			worldTranslate = &apLocation->GetWorldTranslate();
			v3 = (&spLocationNode)->m_pObject;
			v3->SetWorldTranslate(*worldTranslate);
		}
	}
	PlayerCharacter::bSnapChaseCamera = (bool)1;
}

void BGSCameraShot::InitCamera(TESObjectREFR* a1, int32_t a2) {
	NiCamera* pcamera;
	FNVD_INLINED_LOCAL(BSSoundHandle, soundHand_2, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, soundHand, std::nullopt, std::nullopt);
	uint32_t uiNumKeys;
	uint8_t ucSize;
	NiFrustum vatsFrustum;
	if (GetModelLength()) {
		const char* v3 = GetModel();
		spCameraNode.operator=(pModelLoader->LoadFile(v3, LOD_MULT_NONE, 1, 1, 0, 0));
	}
	if (spCameraNode.m_pObject) {
		NiAVObject* v4 = spCameraNode.m_pObject->GetAtChecked(0);
		pcamera = static_cast<NiCamera*>(NiObject::DynamicCastStatic(&NiCamera::ms_RTTI, v4));
	} else {
		pcamera = 0;
	}
	if (!pcamera) {
		Warning("DEFAULT: VATS Camera '%s' doesn't have an NiCamera as the first child. This camera will not be used.",
			GetModel());
		spCameraNode.operator=(static_cast<NiNode*>(0));
		const char* v5 = GetModel();
		pModelLoader->ReleaseFile(v5);
	}
	if (spCameraNode.m_pObject) {
		NiUpdateData UpdateData(fVATSPlaybackTimer, 1, 0);
		if (IsStartAtTimeZero()) {
			float afPhase = -fVATSPlaybackTimer;
			SetPhase(spCameraNode.m_pObject, afPhase);
		}
		spCameraNode.m_pObject->Update(UpdateData);
		if (cCameraHasRotation == -1 && !bCameraIsKillCam) {
			NiSingleInterpController* pinterpController = static_cast<NiSingleInterpController*>(spCameraNode.m_pObject
					->GetController(&NiSingleInterpController::ms_RTTI));
			if (pinterpController) {
				NiTimeController* ptransController = (NiTimeController*)pinterpController;
				NiInterpolator* pinterp = pinterpController->GetInterpolator(0);
				NiTransformInterpolator* ptrans =
					static_cast<NiTransformInterpolator*>(NiObject::DynamicCastStatic(&NiTransformInterpolator::ms_RTTI,
						pinterp));
				NiPathInterpolator* ppath = 0;
				if (ptrans) {
					NiAnimationKey::KeyType eType;
					NiRotKey* pkRotKeys = ptrans->GetRotData(uiNumKeys, eType, ucSize);
					if (uiNumKeys) {
						cCameraHasRotation = 1;
					}
				} else {
					ppath = static_cast<NiPathInterpolator*>(NiObject::DynamicCastStatic(&NiPathInterpolator::ms_RTTI,
						pinterp));
				}
				if (ppath) {
					if (ppath->GetFollow()) {
						cCameraHasRotation = 1;
					}
				}
				if (cCameraHasRotation == -1) {
					cCameraHasRotation = 0;
				}
			}
		}
		if ((&spCameraNode.m_pObject->GetTranslate())->operator==(NiPoint3_ZERO)) {
			Warning("DEFAULT: VATS Camera '%s' doesn't have any position data.", GetModel());
		}
		vatsFrustum = pcamera->GetViewFrustum();
		if (vatsFrustum.m_fTop / vatsFrustum.m_fRight < 0.7400000095367432) {
			Warning("DEFAULT: VATS Camera '%s' has a bad FOV that was not created for a 4x3 aspect ratio.", GetModel());
		}
	}
	pTargetRef = a1;
	PlayerCharacter::bSnapChaseCamera = 1;
	if (a2) {
		if (a2 == 1) {
			bDollyToNewPos = 0;
		} else {
			if (a2 == 2 && Data.eCameraAction == BGSCameraShot::CA_ZOOM) {
				bDollyToNewPos = 1;
				if (!(Data.fMinTime > 0.0)) {
					g_fVATSCameraDollyTime = fVATSCameraDollyTime.Float();
				} else {
					g_fVATSCameraDollyTime = Data.fMinTime;
				}
			}
		}
	} else {
		bDollyToNewPos = 1;
		g_fVATSCameraDollyTime = fVATSCameraDollyTime.Float();
	}
	fDollyStartTime = 0.0f;
	fDollyPercentage = 0.0f;
	dollyDelta = NiPoint3_ZERO;
	dollyStartingPos = NiPoint3_ZERO;
	VATS_COMMAND* pcurrentCommand = pVATS.GetCurrentAction();
	if (pcurrentCommand &&
		(!bPlayCameraZoomSounds &&
			(pcurrentCommand->pHitData->IsFlagSet(HitData::HD_CRITICAL) ||
				(pcurrentCommand->pHitData->IsFlagSet(HitData::HD_CRITICAL_ON_DEATH) ||
					pcurrentCommand->pHitData->IsFlagSet(HitData::HD_FATAL))))) {
		if (Data.eCameraAction == BGSCameraShot::CA_HIT) {
			bPlayCameraZoomSounds = 1;
			BSAudio* v6 = &BSAudio::QInstance();
			v6->GetSoundHandleByName(&soundHand, "UIVATSCameraIn", 289u);
			soundHand.Play(0);
		}
	} else {
		if (bPlayCameraZoomSounds && Data.eCameraAction != BGSCameraShot::CA_HIT) {
			bPlayCameraZoomSounds = 0;
			BSAudio* v7 = &BSAudio::QInstance();
			v7->GetSoundHandleByName(&soundHand_2, "UIVATSCameraOut", 289u);
			soundHand_2.Play(0);
		}
	}
	pPlayer->Setup3rdPersonCameraCaster();
}

void BGSCameraShot::SetCameraTarget(NiAVObject* apTarget) {
	if (apTarget) {
		NiNode* i;
		if (GetRotFollowsTarget()) {
			spTargetNode.operator=(apTarget);
		} else {
			NiNode* v2;
			NiNode* apMemory = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
			if (apMemory) {
				v2 = ::new (apMemory) NiNode(0);
			} else {
				v2 = 0;
			}
			spTargetNode.operator=(v2);
			spTargetNode.m_pObject->SetWorldTranslate(apTarget->GetWorldTranslate());
		}
		for (i = static_cast<NiNode*>(apTarget); i && !i->IsFadeNode(); i = i->m_pkParent) {
			;
		}
		if (i) {
			spImagespaceTarget.operator=(i);
		} else {
			spImagespaceTarget.operator=(apTarget);
		}
		PlayerCharacter::bSnapChaseCamera = 1;
	} else {
		spTargetNode.operator=(static_cast<NiAVObject*>(0));
		spImagespaceTarget.operator=(static_cast<NiAVObject*>(0));
	}
}

void BGSCameraShot::Update(float afTime) {
	NiPoint3 apResult;
	NiFrustum vatsFrustum;
	NiFrustum gameFrustum;
	FNVD_INLINED_LOCAL(BSSoundHandle, soundHand, std::nullopt, std::nullopt);
	NiPoint3 targetPos;
	NiPoint3 cameraLocalPos_3;
	NiPoint3 cameraLocalPos_2;
	NiPoint3 cameraLocalPos;
	NiPoint3 cameraPos;
	if (spCameraNode.m_pObject) {
		NiUpdateData UpdateData(afTime, 1, 0);
		spCameraNode.m_pObject->Update(UpdateData);
		currentCameraPos = spCameraNode.m_pObject->GetTranslate();
		cameraPos = currentCameraPos;
		if (!bCameraTooClose || pPlayer->IsDead(0)) {
			if (GetPosFollowsTarget()) {
				if (GetFollowsBone() && (pTargetRef && Data.eLocation == 2)) {
					cameraLocalPos_2 = cameraPos;
					NiMatrix3::TransformVertices(pTargetRef->GetOrientation(), NiPoint3_ZERO, 1u, &cameraLocalPos_2,
						&cameraPos);
				} else {
					cameraLocalPos = cameraPos;
					NiMatrix3::TransformVertices(pVATSAttacker->GetOrientation(), NiPoint3_ZERO, 1u, &cameraLocalPos,
						&cameraPos);
				}
			} else {
				if (spLocationNode.m_pObject) {
					cameraLocalPos_3 = cameraPos;
					NiMatrix3::TransformVertices((&spLocationNode.m_pObject->m_kLocal)->m_Rotate, NiPoint3_ZERO, 1u,
						&cameraLocalPos_3, &cameraPos);
				}
			}
			if (spLocationNode.m_pObject) {
				bool bplayZoomSound = 0;
				if (dollyStartingPos.operator==(NiPoint3_ZERO)) {
					dollyStartingPos = chaseCameraPos;
					bplayZoomSound = 1;
				}
				targetPos = spLocationNode.m_pObject->GetWorldTranslate();
				cameraPos.operator+=(targetPos);
				pPlayer->UpdateChaseCamera(&cameraPos, &targetPos, 0);
				chaseCameraPos = cameraPos;
				if (bDollyToNewPos) {
					dollyDelta = chaseCameraPos - dollyStartingPos;
					double v12 = dollyDelta.Length();
					if (!(fVATSCameraDollyMin.Float() >= v12)) {
						if (Data.eCameraAction) {
							if (bplayZoomSound && !bPlayCameraZoomSounds) {
								BSAudio* v4 = &BSAudio::QInstance();
								v4->GetSoundHandleByName(&soundHand, "UIVATSCameraIn", 289u);
								soundHand.Play(0);
							}
							fDollyPercentage = 1.0 / g_fVATSCameraDollyTime * (afTime - fDollyStartTime);
							if (!(fDollyPercentage >= 1.0)) {
								chaseCameraPos = dollyStartingPos + dollyDelta * fDollyPercentage;
							} else {
								bDollyToNewPos = 0;
								fDollyStartTime = 0.0f;
							}
						} else {
							bDollyToNewPos = 0;
							fDollyStartTime = 0.0f;
						}
					} else {
						bDollyToNewPos = 0;
						fDollyStartTime = 0.0f;
					}
				}
				NiCamera* pcamera = static_cast<NiCamera*>(spCameraNode.m_pObject->GetAtChecked(0));
				if (pcamera) {
					vatsFrustum = pcamera->GetViewFrustum();
					NiCamera* pgameCamera = Main::GetWorldSceneGraph()->GetCamera();
					gameFrustum = pgameCamera->GetViewFrustum();
					NiRenderer* Renderer_ = NiRenderer::GetRenderer();
					double ScreenWidth = static_cast<double>(Renderer_->GetScreenWidth());
					NiRenderer* v7 = NiRenderer::GetRenderer();
					float fwideMult = ScreenWidth / static_cast<double>(v7->GetScreenHeight()) / 1.3333333730697632;
					gameFrustum.m_fLeft = vatsFrustum.m_fLeft * fwideMult;
					gameFrustum.m_fRight = vatsFrustum.m_fRight * fwideMult;
					gameFrustum.m_fTop = vatsFrustum.m_fTop;
					gameFrustum.m_fBottom = vatsFrustum.m_fBottom;
					pgameCamera->SetViewFrustum(gameFrustum);
				}
			}
		} else {
			pPlayer->SetFirstPerson(1);
		}
		NiPoint3* v8 = GetTargetPos(&apResult);
		double v10 = (*v8 - cameraPos).Length();
		if (fVATSCameraCutAwayDistance.Float() >= v10) {
			bCameraTooClose = 1;
		}
	}
}

NiPoint3* BGSCameraShot::GetTargetPos(NiPoint3* apResult) {
	NiPoint3 apResulta;
	NiPoint3 targetPos = NiPoint3_ZERO;
	if (!(Data.fTargetPercentBetweenActors > 0.0)) {
		if (spTargetNode.m_pObject) {
			targetPos = spTargetNode.m_pObject->GetWorldTranslate();
		}
	} else {
		float afPercentage = Data.fTargetPercentBetweenActors;
		targetPos = *pVATS.GetTargetPercentagePos(&apResulta, afPercentage);
	}
	if (cCameraHasRotation == 1 && spCameraNode.m_pObject) {
		(&spCameraNode.m_pObject->m_kLocal)
			->m_Rotate.ToEulerAnglesXYZ(kVATSCameraAngle.x, kVATSCameraAngle.y, kVATSCameraAngle.z);
	} else {
		kVATSCameraAngle.x = 0.0f;
		kVATSCameraAngle.y = 0.0f;
		kVATSCameraAngle.z = 0.0f;
	}
	*apResult = targetPos;
	return apResult;
}

bool HitData::IsFlagSet(Flag aeFlag) {
	return (uiFlags & aeFlag) != 0;
}

NiRotKey* NiTransformData::GetRotAnim(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) {
	uiNumKeys = m_usNumRotKeys;
	eType = m_eRotType;
	ucSize = m_ucRotSize;
	return m_pkRotKeys;
}
