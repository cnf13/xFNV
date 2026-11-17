#include "Fallout/AI/Character.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/BSFaceGenNiNode.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESChildCell.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectCLOT.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/SpectatorPackage.h"
#include "Fallout/AI/TrespassPackage.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/GarbageCollector.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiGeomMorpherController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMorphData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/rtti.h"

Character::Character() : Character(true) {}

Character::Character(bool abadd)
	: Actor(abadd), pBiped(nullptr), bTrespassing(false), bGuard(false), fequippedweight(-1.0f) {
	SetFormType(59u);
	Character::ResetArmorRating();
	bAttackOnNextTheft = false;
	iThiefCrimeStamp = 0;
	iMinorCrimes = 0;
	iMajorCrimes = 0;
	bIgnoreCrime = false;
}

Character::~Character() {
	if (!GetTemporary()) {
		TESObjectREFR::Set3D(nullptr, false);
	}
}

uint16_t Character::GetSaveSize(uint32_t aiFlags) {
	return Actor::GetSaveSize(aiFlags);
}

void Character::SaveGame(uint32_t aiFlags) {
	Actor::SaveGame(aiFlags);
}

void Character::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	Actor::SaveGame(apSaveGameBuffer);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bTrespassing), 1, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bGuard), 1, 0);
}

void Character::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	Actor::LoadGame(aiFlags, aiCurrentFlags);
}

void Character::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	char v5;
	char apData;
	Actor::LoadGame(apLoadGameBuffer);
	if (apLoadGameBuffer->GetVersion() < 9u) {
		char v6[7];
		apLoadGameBuffer->LoadData(&apData, 1u);
		apLoadGameBuffer->LoadData(v6, 4u);
	}
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bTrespassing), 1u);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bGuard), 1u);
	if (apLoadGameBuffer->GetVersion() < 9u) {
		char v3[4];
		char v4[7];
		apLoadGameBuffer->LoadData(&v5, 1u);
		apLoadGameBuffer->LoadData(v4, 4u);
		apLoadGameBuffer->LoadData(v3, 4u);
	}
}

void Character::InitLoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	Actor::InitLoadGame(aiFlags, aiCurrentFlags);
}

void Character::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	Actor::InitLoadGame(apLoadGameBuffer);
}

// Reverts the actor's TES data, then forgets the cached armor rating and threshold unless the game is loading a save
// (the exe stores -1.0f into both).
void Character::Revert(uint32_t aiFlags) {
	Actor::Revert(aiFlags);
	if (pSaveLoadGameOLD->IsUnk048Zero()) {
		fArmorRating = (-0x1p+0f);
		fArmorThreshold = (-0x1p+0f);
	}
}

void Character::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
	Actor::Revert(apLoadGameBuffer);
	fArmorRating = (-0x1p+0f);
	fArmorThreshold = (-0x1p+0f);
	fequippedweight = (-0x1p+0f);
	bTrespassing = (bool)0;
	bGuard = (bool)0;
}

void Character::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	Actor::FinishLoadGame(apLoadGameBuffer);
}

// A child race, a race whose first three body-part models have "child" in their path, or (unless abIgnoreHeight)
// a loaded actor less than 110 units tall at scale 1. The exe stores the height as a double, divides by the scale
// and rounds the quotient to a float before comparing it with the double 110.0.
bool Character::IsChild(bool abIgnoreHeight) {
	bool bIsChild = false;
	TESRace* pRace = GetRace();
	if (pRace) {
		bIsChild = pRace->GetChildRace();
		if (!bIsChild) {
			bool bChildModel = false;
			for (int32_t i = 0; !bChildModel && i < 3; ++i) {
				TESModel* pModel = pRace->GetBodyPartModel(GetSex(), static_cast<RACE_BODY_PART>(i));
				if (pModel)
					bChildModel = BSstristr(pModel->GetModel(), "child") != nullptr;
			}
			bIsChild = bChildModel;
		}
	}
	if (!abIgnoreHeight && !bIsChild && Get3D()) {
		double dHeight = GetHeight();
		float fScaledHeight = static_cast<float>(dHeight / GetScale());
		if (fScaledHeight < 110.0)
			bIsChild = true;
	}
	return bIsChild;
}

BipedAnim* Character::GetBiped() {
	return pBiped;
}

// The player's old biped is deleted at once; any other character's goes to the garbage collector.
void Character::SetBiped(BipedAnim* apBiped) {
	if (pBiped) {
		if (pBiped == apBiped)
			return;
		if (this == pPlayer) {
			BipedAnim* pOldBiped = pBiped;
			if (pOldBiped)
				delete pOldBiped;
		} else {
			GarbageCollector::Add(pBiped);
		}
	}
	pBiped = apBiped;
}

// The exe calls it directly as well as through the vtable.
// The exe's cleanup-only C++ EH frame is left out.
void Character::Update(float afDelta) {
	Actor::Update(afDelta);
	if (this != pPlayer || pPlayer->Is3rdPerson()) {
		NiAVObject* proot = Get3D();
		if (proot) {
			if (pCurrentProcess) {
				BSFaceGenNiNode* pfacenode = pCurrentProcess->GetFaceNode();
				BSFaceGenNiNode* pfacenodeSkinned = pCurrentProcess->GetFaceSkinnedNode();
				NiAVObject* panimFace = pCurrentProcess->GetHeadAnims();
				if (!pfacenode) {
					NiFixedString* v2 = &FixedStrings::QRaceHeadBiped();
					NiAVObject* ObjectByName_1 = BSUtilities::GetObjectByName(proot, *v2);
					pfacenode = static_cast<BSFaceGenNiNode*>(NiObject::DynamicCastStatic(&BSFaceGenNiNode::ms_RTTI,
						ObjectByName_1));
					pCurrentProcess->SetFaceNode(pfacenode);
				}
				if (!pfacenodeSkinned) {
					NiFixedString* v4 = &FixedStrings::QRaceHeadSkinned();
					NiAVObject* v5 = BSUtilities::GetObjectByName(proot, *v4);
					pfacenodeSkinned =
						static_cast<BSFaceGenNiNode*>(NiObject::DynamicCastStatic(&BSFaceGenNiNode::ms_RTTI, v5));
					pCurrentProcess->SetFaceSkinnedNode(pfacenodeSkinned);
				}
				if (!panimFace) {
					NiFixedString* v6 = &FixedStrings::QHeadAnims();
					panimFace = BSUtilities::GetObjectByName(proot, *v6);
					pCurrentProcess->SetHeadAnims(panimFace);
				}
				if ((!pfacenode || pfacenode->GetSelectiveUpdate()) &&
					(!pfacenodeSkinned || !pfacenodeSkinned->GetAppCulled())) {
					if (panimFace) {
						if (pfacenode) {
							if (pfacenodeSkinned) {
								bool bheadAnimsCulled = panimFace->m_pkParent->GetAppCulled();
								BSFaceGenAnimationData* v7 = pfacenode->GetAnimationData();
								v7->SetAnimHeadCulled(bheadAnimsCulled);
								BSFaceGenAnimationData* v8 = pfacenodeSkinned->GetAnimationData();
								v8->SetAnimHeadCulled(bheadAnimsCulled);
								if (!bheadAnimsCulled) {
									NiGeomMorpherController* panimMorph =
										static_cast<NiGeomMorpherController*>(panimFace
												->GetController(&NiGeomMorpherController::ms_RTTI));
									uint32_t icount = panimMorph->GetNumTargets();
									for (uint32_t n = 0; n < icount; ++n) {
										NiMorphData::MorphTarget* ptarget = panimMorph->GetTarget(n);
										NiInterpolator* pinterp = panimMorph->GetInterpolator(n);
										NiBlendFloatInterpolator* pfloatInterp = 0;
										if (NiObject::IsExactKindOf(&NiBlendFloatInterpolator::ms_RTTI, pinterp)) {
											pfloatInterp = static_cast<NiBlendFloatInterpolator*>(pinterp);
										}
										if (pfloatInterp) {
											float fvalue = pfloatInterp->m_fFloatValue;
											const NiFixedString* Name = &ptarget->GetName();
											FNVD_CONSTRUCTED_LOCAL(NiFixedString, pname, &NiFixedString::Destruct,
												*Name);
											if (fvalue >= 0.0) {
												if (*pname.GetHandleString()) {
													BS_FACEGEN_EXPRESSION eexp = BS_FG_EXPRESSION_NONE;
													BS_FACEGEN_MODIFIER emod = BS_FG_MODIFIER_NONE;
													BS_FACEGEN_PHONEME ephon = BS_FG_PHONEME_NONE;
													switch (*pname.GetHandleString()) {
													case 65: {
														if (pname.GetHandleString()[1] == 110) {
															eexp = BS_FG_EXPRESSION_ANGER;
														} else {
															ephon = BS_FG_PHONEME_AAH;
														}
														break;
													}
													case 66: {
														switch (pname.GetHandleString()[1]) {
														case 77: {
															ephon = BS_FG_PHONEME_B_M_P;
															break;
														}
														case 105: {
															ephon = BS_FG_PHONEME_BIG_AAH;
															break;
														}
														case 108: {
															emod = static_cast<
																BS_FACEGEN_MODIFIER>(pname.GetHandleString()[5] != 76);
															break;
														}
														case 114: {
															char v17 = pname.GetHandleString()[4];
															switch (v17) {
															case 68: {
																if (pname.GetHandleString()[8] == 76) {
																	emod = BS_FG_MODIFIER_BROW_DOWN_LEFT;
																} else {
																	emod = BS_FG_MODIFIER_BROW_DOWN_RIGHT;
																}
																break;
															}
															case 73: {
																if (pname.GetHandleString()[6] == 76) {
																	emod = BS_FG_MODIFIER_BROW_IN_LEFT;
																} else {
																	emod = BS_FG_MODIFIER_BROW_IN_RIGHT;
																}
																break;
															}
															case 85: {
																if (pname.GetHandleString()[6] == 76) {
																	emod = BS_FG_MODIFIER_BROW_UP_LEFT;
																} else {
																	emod = BS_FG_MODIFIER_BROW_UP_RIGHT;
																}
																break;
															}
															}
															break;
														}
														default: {
															goto LABEL_101;
														}
														}
														break;
													}
													case 67: {
														if (pname.GetHandleString()[1] == 111) {
															eexp = BS_FG_EXPRESSION_COMBAT_ANGRY;
														} else {
															ephon = BS_FG_PHONEME_CH_J_SH;
														}
														break;
													}
													case 68: {
														ephon = BS_FG_PHONEME_D_S_T;
														break;
													}
													case 69: {
														if (pname.GetHandleString()[1] == 101) {
															ephon = BS_FG_PHONEME_EEE;
														} else {
															ephon = BS_FG_PHONEME_EH;
														}
														break;
													}
													case 70: {
														if (pname.GetHandleString()[1] == 101) {
															eexp = BS_FG_EXPRESSION_FEAR;
														} else {
															ephon = BS_FG_PHONEME_F_V;
														}
														break;
													}
													case 72: {
														switch (pname.GetHandleString()[4]) {
														case 80: {
															emod = BS_FG_MODIFIER_HEAD_PITCH;
															break;
														}
														case 82: {
															emod = BS_FG_MODIFIER_HEAD_ROLL;
															break;
														}
														case 89: {
															emod = BS_FG_MODIFIER_HEAD_YAW;
															break;
														}
														case 121: {
															eexp = BS_FG_EXPRESSION_HAPPY;
															break;
														}
														default: {
															goto LABEL_101;
														}
														}
														break;
													}
													case 73: {
														ephon = BS_FG_PHONEME_I;
														break;
													}
													case 75: {
														ephon = BS_FG_PHONEME_K;
														break;
													}
													case 76: {
														switch (pname.GetHandleString()[4]) {
														case 68: {
															emod = BS_FG_MODIFIER_LOOK_DOWN;
															break;
														}
														case 76: {
															emod = BS_FG_MODIFIER_LOOK_LEFT;
															break;
														}
														case 82: {
															emod = BS_FG_MODIFIER_LOOK_RIGHT;
															break;
														}
														case 85: {
															emod = BS_FG_MODIFIER_LOOK_UP;
															break;
														}
														default: {
															goto LABEL_101;
														}
														}
														break;
													}
													case 77: {
														switch (pname.GetHandleString()[5]) {
														case 97: {
															eexp = BS_FG_EXPRESSION_MOOD_SAD;
															break;
														}
														case 101: {
															eexp = BS_FG_EXPRESSION_MOOD_NEUTRAL;
															break;
														}
														case 102: {
															eexp = BS_FG_EXPRESSION_MOOD_AFRAID;
															break;
														}
														case 108: {
															eexp = BS_FG_EXPRESSION_MOOD_PLEASANT;
															break;
														}
														case 110: {
															if (pname.GetHandleString()[6] == 110) {
																eexp = BS_FG_EXPRESSION_MOOD_ANNOYED;
															} else {
																eexp = BS_FG_EXPRESSION_MOOD_ANGRY;
															}
															break;
														}
														case 111: {
															eexp = BS_FG_EXPRESSION_MOOD_COCKY;
															break;
														}
														case 114: {
															eexp = BS_FG_EXPRESSION_MOOD_DRUGGED;
															break;
														}
														default: {
															goto LABEL_101;
														}
														}
														break;
													}
													case 78: {
														ephon = BS_FG_PHONEME_N;
														break;
													}
													case 79: {
														if (pname.GetHandleString()[1] == 104) {
															ephon = BS_FG_PHONEME_OH;
														} else {
															ephon = BS_FG_PHONEME_OOH_Q;
														}
														break;
													}
													case 80: {
														eexp = BS_FG_EXPRESSION_MOOD_PAINED;
														break;
													}
													case 82: {
														ephon = BS_FG_PHONEME_R;
														break;
													}
													case 83: {
														char v16 = pname.GetHandleString()[1];
														switch (v16) {
														case 97: {
															eexp = BS_FG_EXPRESSION_SAD;
															break;
														}
														case 113: {
															if (pname.GetHandleString()[6] == 76) {
																emod = BS_FG_MODIFIER_SQUINT_LEFT;
															} else {
																emod = BS_FG_MODIFIER_SQUINT_RIGHT;
															}
															break;
														}
														case 117: {
															eexp = BS_FG_EXPRESSION_SURPRISE;
															break;
														}
														}
														break;
													}
													case 84: {
														ephon = BS_FG_PHONEME_TH;
														break;
													}
													case 87: {
														ephon = BS_FG_PHONEME_W;
														break;
													}
													default: {
														break;
													}
													}
												LABEL_101:;
													if (eexp == BS_FG_EXPRESSION_NONE) {
														if (emod == BS_FG_MODIFIER_NONE) {
															if (ephon != BS_FG_PHONEME_NONE) {
																BSFaceGenAnimationData* v11 =
																	pfacenode->GetAnimationData();
																v11->SetAnimPhonemeValue(ephon, fvalue);
																BSFaceGenAnimationData* v10 =
																	pfacenodeSkinned->GetAnimationData();
																v10->SetAnimPhonemeValue(ephon, fvalue);
															}
														} else {
															BSFaceGenAnimationData* v13 = pfacenode->GetAnimationData();
															v13->SetAnimModifierValue(emod, fvalue);
															BSFaceGenAnimationData* v12 =
																pfacenodeSkinned->GetAnimationData();
															v12->SetAnimModifierValue(emod, fvalue);
														}
													} else {
														BSFaceGenAnimationData* v15 = pfacenode->GetAnimationData();
														v15->SetAnimExpressionValue(eexp, fvalue);
														BSFaceGenAnimationData* v14 =
															pfacenodeSkinned->GetAnimationData();
														v14->SetAnimExpressionValue(eexp, fvalue);
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

bool Character::IsActoraGuard() {
	return bGuard;
}

void Character::SetActorGuard(bool abGuard) {
	bGuard = abGuard;
}

void Character::SetEquippedWeight(float afvalue) {
	fequippedweight = afvalue;
}

float Character::GetTurningSpeed() {
	return (&fCharacterDefaultTurningSpeed)->Float();
}

// The exe's cleanup-only C++ EH frame is left out.
void Character::InitiateSandmanPackage(Actor* apTarget, TESObjectREFR* apfurniture, FurnitureMark* aMarker,
	uint8_t auiIndex) {
	if (!IsInCombat()) {
		PackageTarget* pnew;
		PackageLocation* ploc;
		pCurrentProcess->SetRunOncePackage(0, this);
		BaseProcess* CurrentAIProcess = pCurrentProcess;
		CurrentAIProcess->ClearActionHeadTrackTarget(0);
		BaseProcess* v15 = pCurrentProcess;
		v15->GetCurrentPackage();
		if (GetCurrentPackage()) {
			TESPackage* CurrentPackage = GetCurrentPackage();
			if (CurrentPackage->IsInterruptPackage()) {
				EndDialogue();
			}
		}
		TESPackage* ppack = TESPackage::CreatePackage(PACKAGE_TYPE_SANDMAN);
		ppack->SetMustReachLocation(1);
		ppack->SetMustComplete(1);
		PackageLocation* v19 = static_cast<PackageLocation*>(ourMalloc(sizeof(PackageLocation)));
		if (v19) {
			ploc = ::new (v19) PackageLocation;
		} else {
			ploc = 0;
		}
		ploc->SetLocType(PLT_REFERENCE);
		ploc->SetLocReference(this);
		ppack->SetPackageLocation(ploc);
		if (ploc) {
			delete ploc;
		}
		PackageTarget* v18 = static_cast<PackageTarget*>(ourMalloc(sizeof(PackageTarget)));
		if (v18) {
			pnew = ::new (v18) PackageTarget;
		} else {
			pnew = 0;
		}
		ppack->SetPackageTarget(pnew);
		if (pnew) {
			delete pnew;
		}
		ppack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_SANDMAN;
		ppack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
		ppack->GetPackageTarget()->SetTargReference(apTarget);
		ppack->GetPackageTarget()->iValue = 0;
		BaseProcess* v12 = pCurrentProcess;
		v12->ComputeLastTimeProcessed();
		if (pCurrentProcess->GetCurrentPackage()) {
			bool v11 = pCurrentProcess->IsPackageDoneOnce();
			bool v10 = pCurrentProcess->IsTargetActivated();
			bool v9 = pCurrentProcess->GetCurrentActionComplete();
			TESObjectREFR* v8 = pCurrentProcess->GetTarget();
			int32_t v7 = pCurrentProcess->GetCurrentProcedureIndex();
			TESPackage* v6 = pCurrentProcess->GetCurrentPackage();
			(&m_Extra)->SetPackageExtra(v6, v7, v8, v9, v10, v11);
		}
		pCurrentProcess->SetFurniture(apfurniture, auiIndex, aMarker);
		PutCreatedPackage(ppack, 0, 1);
		apTarget->InitiateDoNothingPackage();
	}
}

TESRace* Character::GetRace() {
	TESNPC* pBaseForm = static_cast<TESNPC*>(GetObjectReference());
	return pBaseForm->GetFormRace();
}

void Character::ResetArmorRating() {
	fArmorRating = -1.0f;
	fArmorThreshold = -1.0f;
}

bool Character::InitiateTresPassPackage(TrespassPackage* apPack) {
	TESPackage* currentPackage;
	if (Actor::IsInCombat() ||
		(IsDead((bool)0) ||
			(Actor::IsAlarmed() ||
				(Actor::IsFleeing(0) ||
					(IsTresPassing() || (apPack && (((int32_t)apPack->GetNumberActors()) > 1))))))) {
		return (bool)0;
	}
	if (GetSitSleepState() && ((((int32_t)GetSitSleepState()) != 4) && (((int32_t)GetSitSleepState()) != 9))) {
		return (bool)0;
	}
	if (!apPack) {
		return (bool)0;
	}
	pCurrentProcess->ClearRunOncePackage();
	BaseProcess* currentAIProcess = MobileObject::GetCurrentAIProcess();
	currentAIProcess->ClearActionHeadTrackTarget((bool)1);
	if (MobileObject::GetCurrentPackage()) {
		currentPackage = MobileObject::GetCurrentPackage();
		if (currentPackage->IsInterruptPackage()) {
			Actor::EndInterruptPackage(0);
		}
	}
	BaseProcess* v5 = MobileObject::GetCurrentAIProcess();
	v5->ComputeLastTimeProcessed();
	BaseProcess* v4 = MobileObject::GetCurrentAIProcess();
	v4->SavePackageToExtraData(this);
	PutCreatedPackage(apPack, (bool)0, (bool)0);
	apPack->ModNumberActors(1);
	pCurrentProcess->SetProcedureIndexRunning((uint32_t)0);
	return (bool)1;
}

// The exe's cleanup-only C++ EH frame is left out.
void Character::InitiateSpectator() {
	TESPackage* v1;
	if (!GetCurrentPackage() || (v1 = GetCurrentPackage(), v1->GetPackType() != PACKAGE_TYPE_FLEE)) {
		if (!IsInCombat() && (!IsinDialogue() || GetCurrentTarget() != pPlayer) &&
			(GetSitSleepState() == SIT_SLEEP_NORMAL ||
				(GetSitSleepState() == SIT_SLEEP_IS_SITTING || GetSitSleepState() == SIT_SLEEP_IS_SLEEPING)) &&
			!ShouldSkipFallOutBehavior(FALLOUT_BEHAVIORS_OBSERVE_COMBAT_BEHAVIOR)) {
			BaseProcess* pproc = pCurrentProcess;
			if (pproc) {
				TESPackage* ppack = pproc->GetRunOncePackage();
				if (!ppack ||
					(ppack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_OBSERVE_COMBAT &&
						(ppack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_FLEE &&
							ppack->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_FLEE_NON_COMBAT))) {
					TESPackage* ppack_2 = pproc->GetCurrentPackage();
					if (!ppack_2 ||
						(ppack_2->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_OBSERVE_COMBAT &&
							(ppack_2->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_FLEE &&
								ppack_2->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_FLEE_NON_COMBAT))) {
						SpectatorPackage* pspecPack;
						pCurrentProcess->ClearRunOncePackage();
						BaseProcess* CurrentAIProcess = pCurrentProcess;
						CurrentAIProcess->ClearActionHeadTrackTarget(1);
						if (pCurrentProcess->GetSitSleepState() == SIT_SLEEP_IS_SITTING || IsSleeping()) {
							InitiateGetUpPackage();
						}
						SpectatorPackage* apMemory =
							static_cast<SpectatorPackage*>(ourMalloc(sizeof(SpectatorPackage)));
						if (apMemory) {
							pspecPack = ::new (apMemory) SpectatorPackage;
						} else {
							pspecPack = 0;
						}
						BaseProcess* v7 = pCurrentProcess;
						v7->ComputeLastTimeProcessed();
						if (GetCurrentPackage()) {
							TESPackage* CurrentPackage = GetCurrentPackage();
							pspecPack->CopyAllFlags(CurrentPackage);
						}
						if (pCurrentProcess) {
							BaseProcess* v6 = pCurrentProcess;
							if (v6->GetPackageThatIsRunning()) {
								BaseProcess* v5 = pCurrentProcess;
								TESPackage* v3 = v5->GetPackageThatIsRunning();
								if (v3->IsInterruptPackage()) {
									EndInterruptPackage(0);
								}
							}
						}
						BaseProcess* v4 = pCurrentProcess;
						v4->SavePackageToExtraData(this);
						PutCreatedPackage(pspecPack, 0, 1);
					}
				}
			}
		}
	}
}

float Character::GetArmorRating() {
	TESObjectARMO* parmorFound[20];
	int32_t iarmorFoundCount = 0;
	parmorFound[0] = 0;
	if (fArmorRating < 0.0) {
		fArmorRating = 0.0f;
		InventoryChanges* pinventory = (&m_Extra)->GetContainerChanges();
		if (pinventory) {
			for (BIPED_OBJECT i = BO_HEAD; i < BIPED_OBJECT_COUNT; i = static_cast<BIPED_OBJECT>(i + 1)) {
				ItemChange* pchange = pinventory->GetWornItem(i, 0);
				if (pchange) {
					TESObjectARMO* parmo = dynamic_cast<TESObjectARMO*>(pchange->pContainerObj);
					if (parmo) {
						bool balreadyCounted = 0;
						if (iarmorFoundCount) {
							for (int32_t j = 0; j < iarmorFoundCount && !balreadyCounted; ++j) {
								balreadyCounted = parmorFound[j] == parmo;
							}
						}
						if (!balreadyCounted) {
							ActorValueOwner* a2;
							if (this) {
								a2 = &static_cast<ActorValueOwner&>(static_cast<Actor&>(*this));
							} else {
								a2 = 0;
							}
							fArmorRating = pchange->GetConditionScaledDR(a2) + fArmorRating;
							parmorFound[iarmorFoundCount++] = parmo;
						}
					}
					delete pchange;
				}
			}
		}
	}
	if (!(fMaxArmorRating.Float() > 0.0)) {
		return fArmorRating;
	} else {
		float* v2 = &fMaxArmorRating.Float();
		return BSMIN(fArmorRating, *v2);
	}
}

float Character::GetArmorThreshold() {
	TESObjectARMO* parmorFound[20];
	int32_t iarmorFoundCount = 0;
	parmorFound[0] = 0;
	if (fArmorThreshold < 0.0) {
		fArmorThreshold = 0.0f;
		InventoryChanges* pinventory = (&m_Extra)->GetContainerChanges();
		if (pinventory) {
			for (int32_t i = 0; i < 20; ++i) {
				ItemChange* pchange = pinventory->GetWornItem(static_cast<BIPED_OBJECT>(i), 0);
				if (pchange) {
					TESObjectARMO* parmo = dynamic_cast<TESObjectARMO*>(pchange->pContainerObj);
					if (parmo) {
						bool balreadyCounted = 0;
						if (iarmorFoundCount) {
							for (int32_t j = 0; j < iarmorFoundCount && !balreadyCounted; ++j) {
								balreadyCounted = parmorFound[j] == parmo;
							}
						}
						if (!balreadyCounted) {
							ActorValueOwner* a2;
							if (this) {
								a2 = &static_cast<ActorValueOwner&>(static_cast<Actor&>(*this));
							} else {
								a2 = 0;
							}
							fArmorThreshold = pchange->GetModifiedDamageThreshold(a2) + fArmorThreshold;
							parmorFound[iarmorFoundCount++] = parmo;
						}
					}
					delete pchange;
				}
			}
		}
	}
	return fArmorThreshold;
}

int32_t Character::GetAttackDamage() {
	float v3 = (float)0.0;
	float v2 = (float)0.0;
	if (this) {
		CombatFormulas::CalcHandToHandDamage(static_cast<ActorValueOwner*>(this), 0, v3, v2);
	} else {
		CombatFormulas::CalcHandToHandDamage(nullptr, 0, v3, v2);
	}
	return (int32_t)v3;
}

void Character::SetTresPassing() {
	TESObjectCELL* lpParentCell = GetParentCell();
	bool lbTrespassing = false;
	if (lpParentCell) {
		lbTrespassing = lpParentCell->CheckifTrespassing(this);
	}
	bTrespassing = lbTrespassing;
}

void Character::PrecacheData() {
	Actor::PrecacheData();
	GetArmorThreshold();
	GetArmorRating();
	NiNode* apScene = Get3D();
	if (apScene) {
		if (pCurrentProcess) {
			if (!pCurrentProcess->GetFaceNode()) {
				NiFixedString* v1 = &FixedStrings::QRaceHeadBiped();
				NiAVObject* ObjectByName = BSUtilities::GetObjectByName(apScene, *v1);
				BSFaceGenNiNode* v3 =
					static_cast<BSFaceGenNiNode*>(NiObject::DynamicCastStatic(&BSFaceGenNiNode::ms_RTTI, ObjectByName));
				pCurrentProcess->SetFaceNode(v3);
			}
			if (!pCurrentProcess->GetFaceSkinnedNode()) {
				NiFixedString* v4 = &FixedStrings::QRaceHeadSkinned();
				NiAVObject* v5 = BSUtilities::GetObjectByName(apScene, *v4);
				BSFaceGenNiNode* v6 =
					static_cast<BSFaceGenNiNode*>(NiObject::DynamicCastStatic(&BSFaceGenNiNode::ms_RTTI, v5));
				pCurrentProcess->SetFaceSkinnedNode(v6);
			}
			if (!pCurrentProcess->GetHeadAnims()) {
				NiFixedString* v7 = &FixedStrings::QHeadAnims();
				NiAVObject* v8 = BSUtilities::GetObjectByName(apScene, *v7);
				pCurrentProcess->SetHeadAnims(v8);
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void Character::InitiateCannibal(Actor* apTarget) {
	if (!IsInCombat()) {
		PackageTarget* pnew;
		PackageLocation* ploc;
		pCurrentProcess->SetRunOncePackage(0, this);
		BaseProcess* CurrentAIProcess = pCurrentProcess;
		CurrentAIProcess->ClearActionHeadTrackTarget(0);
		BaseProcess* v12 = pCurrentProcess;
		v12->GetCurrentPackage();
		if (GetCurrentPackage()) {
			TESPackage* CurrentPackage = GetCurrentPackage();
			if (CurrentPackage->IsInterruptPackage()) {
				EndDialogue();
			}
		}
		TESPackage* ppack = TESPackage::CreatePackage(PACKAGE_TYPE_CANNIBAL);
		ppack->SetMustReachLocation(1);
		ppack->SetMustComplete(1);
		PackageLocation* v16 = static_cast<PackageLocation*>(ourMalloc(sizeof(PackageLocation)));
		if (v16) {
			ploc = ::new (v16) PackageLocation;
		} else {
			ploc = 0;
		}
		ploc->SetLocType(PLT_REFERENCE);
		ploc->SetLocReference(this);
		ppack->SetPackageLocation(ploc);
		if (ploc) {
			delete ploc;
		}
		PackageTarget* v15 = static_cast<PackageTarget*>(ourMalloc(sizeof(PackageTarget)));
		if (v15) {
			pnew = ::new (v15) PackageTarget;
		} else {
			pnew = 0;
		}
		ppack->SetPackageTarget(pnew);
		if (pnew) {
			delete pnew;
		}
		ppack->ePROCEDURE_TYPE = PACKAGE_PROCEDURE_CANNIBAL;
		ppack->GetPackageTarget()->SetTargType(PTT_REFERENCE);
		ppack->GetPackageTarget()->SetTargReference(apTarget);
		ppack->GetPackageTarget()->iValue = 0;
		BaseProcess* v9 = pCurrentProcess;
		v9->ComputeLastTimeProcessed();
		if (pCurrentProcess->GetCurrentPackage()) {
			bool v8 = pCurrentProcess->IsPackageDoneOnce();
			bool v7 = pCurrentProcess->IsTargetActivated();
			bool v6 = pCurrentProcess->GetCurrentActionComplete();
			TESObjectREFR* v5 = pCurrentProcess->GetTarget();
			int32_t v4 = pCurrentProcess->GetCurrentProcedureIndex();
			TESPackage* v3 = pCurrentProcess->GetCurrentPackage();
			(&m_Extra)->SetPackageExtra(v3, v4, v5, v6, v7, v8);
		}
		PutCreatedPackage(ppack, 0, 1);
		apTarget->InitiateDoNothingPackage();
	}
}

void Character::Reset3D() {
	TaskQueueInterface* instance;
	FOPipboyManager* v2;
	BaseProcess* v3;
	BaseProcess* v4;
	BaseProcess* v5;
	BaseProcess* v6;
	BaseProcess* currentAIProcess;
	FOPipboyManager* pipboy;
	if (TESObjectREFR::Get3D()) {
		if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
			instance = TaskQueueInterface::GetInstance();
			instance->QueueCharacterReset3D(this);
		} else {
			if (static_cast<const void*>(this) == static_cast<const void*>(pPlayer)) {
				pipboy = Interface::GetPipboy();
				pipboy->Shutdown();
			}
			currentAIProcess = MobileObject::GetCurrentAIProcess();
			currentAIProcess->Set3DUpdateFlag(RESET_MODEL);
			v6 = MobileObject::GetCurrentAIProcess();
			v6->Set3DUpdateFlag(RESET_SKIN);
			v5 = MobileObject::GetCurrentAIProcess();
			v5->Set3DUpdateFlag(RESET_HEAD);
			v4 = MobileObject::GetCurrentAIProcess();
			v4->Set3DUpdateFlag(RESET_SCALE);
			v3 = MobileObject::GetCurrentAIProcess();
			v3->Update3dModel(this);
			if (static_cast<const void*>(this) == static_cast<const void*>(pPlayer)) {
				v2 = Interface::GetPipboy();
				v2->Initialize();
			}
			if (((int32_t)pCurrentProcess->GetProcessLevel()) == 0) {
				static_cast<HighProcess*>(pCurrentProcess)->SetActorHeight(0.0f);
			}
		}
	}
}

float Character::GetClothingValue() {
	float v11 = 0.0f;
	InventoryChanges* ContainerChanges = (&m_Extra)->GetContainerChanges();
	if (ContainerChanges) {
		for (BIPED_OBJECT part = BO_HEAD; part < BIPED_OBJECT_COUNT; part = static_cast<BIPED_OBJECT>(part + 1)) {
			ItemChange* WornItem = ContainerChanges->GetWornItem(part, 0);
			if (WornItem) {
				char* v8 = static_cast<char*>(FNVD_DYNAMIC_CAST_VOID(TESObjectARMO)(WornItem->pContainerObj,
					RTTI_TESBoundObject, RTTI_TESObjectARMO));
				char* v7 = static_cast<char*>(FNVD_DYNAMIC_CAST_VOID(TESObjectCLOT)(WornItem->pContainerObj,
					RTTI_TESBoundObject, RTTI_TESObjectCLOT));
				if (v8) {
					double ItemValue = WornItem->GetItemValue();
					double v5 = fClothingArmorScale.Float() * ItemValue;
					v11 = fClothingArmorBase.Float() + v5 + v11;
				} else {
					if (v7) {
						double v4 = WornItem->GetItemValue();
						double v3 = fClothingClassScale.Float() * v4;
						v11 = fClothingBase.Float() + v3 + v11;
					}
				}
				delete WornItem;
			}
		}
	}
	if (v11 > 100.0) {
		return 100.0f;
	}
	return v11;
}

bool TESRace::GetChildRace() const {
	return (Data.iFlags & RACE_DATA::RACE_FLAG_CHILD) != 0;
}

uint32_t TrespassPackage::GetNumberActors() {
	return iNumberActors;
}

NiMorphData::MorphTarget* NiGeomMorpherController::GetTarget(uint32_t uiIndex) {
	return m_spMorphData.m_pObject->GetTarget(uiIndex);
}
