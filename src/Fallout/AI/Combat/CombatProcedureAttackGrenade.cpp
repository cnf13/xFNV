#include "Fallout/AI/Combat/CombatProcedureAttackGrenade.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/IDebugText.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatDialogueManager.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

CombatProcedureAttackGrenade::CombatProcedureAttackGrenade()
	: eState(ATTACK_STATE_IDLE), bTargetLocation(false), TargetLocation(NiPoint3_ZERO), TargetedPoint(NiPoint3_ZERO),
	  bHighArc(false) {}

CombatProcedureAttackGrenade::CombatProcedureAttackGrenade(const NiPoint3& arTargetLocation, bool abHighArc)
	: eState(ATTACK_STATE_IDLE), bTargetLocation(true), TargetLocation(arTargetLocation), TargetedPoint(NiPoint3_ZERO),
	  bHighArc(abHighArc) {}

CombatProcedureAttackGrenade::~CombatProcedureAttackGrenade() {}

void CombatProcedureAttackGrenade::Update() {
	NiPoint3 a1;
	Actor* pattacker = GetAttacker();
	Actor* ptarget = GetTarget();
	CombatState* pcombatState = pCombatController->pCombatState;
	TESObjectWEAP* pweapon = pCombatController->GetCurrentWeapon();
	Animation* panim = pattacker->GetAnimation();
	if (pweapon && (panim && pweapon->GetProjectile(pattacker))) {
		if (pattacker->IsAttacking()) {
			eState = CombatProcedureAttackGrenade::ATTACK_STATE_ATTACKING;
		} else {
			if (pattacker->GetAnimAction() == ANIM_ACTION_NONE) {
				eState = CombatProcedureAttackGrenade::ATTACK_STATE_IDLE;
			}
		}
		if (!bTargetLocation && eState == CombatProcedureAttackGrenade::ATTACK_STATE_IDLE) {
			bHighArc = 0;
		}
		if (pcombatState->GetHasBufferedDetection() && !pcombatState->bTargetSpaceDifferent) {
			pCombatDialogueManager->StartDialogue(pCombatController, DT_COMBAT, 0, 0);
			TargetedPoint = *CalculateTargetedPoint(&a1);
			float fminRange = 0.0f;
			float fmaxRange = 0.0f;
			float fabsoluteMaxRange = 0.0f;
			CombatUtilities::ComputeOptimalWeaponRanges(pCombatController, pweapon, fminRange, fmaxRange,
				fabsoluteMaxRange);
			const NiPoint3* v2 = &pattacker->GetLocationOnReference();
			NiPoint3 targetVector = TargetedPoint - *v2;
			if (!(fabsoluteMaxRange * fabsoluteMaxRange < targetVector.SqrXYLength())) {
				if (TargetedPoint.operator!=(NiPoint3_ZERO)) {
					SetLookAtTarget(TargetedPoint, 4);
				}
				if (eState != CombatProcedureAttackGrenade::ATTACK_STATE_ATTACKING ||
					(pattacker->GetAnimAction() == ANIM_ACTION_ATTACK_FOLLOW_THROUGH &&
						panim->GetGroupAction(AGS_WEAPON) <= AGA_SPECIAL_IDLE_LOOP_STOP)) {
					BaseProcess* CurrentAIProcess = pattacker->pCurrentProcess;
					if (CurrentAIProcess->CanAttack() && !pCombatController->IsSwitchingWeapons()) {
						const NiPoint3* p_TargetLocation;
						NiPoint3 hitLocation;
						BGSExplosion* pexplosion = pweapon->GetProjectile(pattacker)->GetExplosion();
						if (!pexplosion ||
							(!bTargetLocation ? p_TargetLocation = &ptarget->GetLocationOnReference()
											  : p_TargetLocation = &TargetLocation,
								(hitLocation = *p_TargetLocation, CheckExplosion(pexplosion, hitLocation, 0.0f)))) {
							BGSProjectile* pprojectile_2 = pweapon->GetProjectile(pattacker);
							if (bTargetLocation ||
								CombatUtilities::CalculateGrenadeThrow_(pattacker, ptarget, pprojectile_2, 0, &bHighArc,
									&TargetedPoint, -1.0f)) {
								if (pexplosion) {
									pcombatState->pExplosiveProjectileBlocked = 0;
									ExplosiveProjectileBlockedTimer.Clear();
								}
								BaseProcess* v3 = pattacker->pCurrentProcess;
								v3->SetShotsToFire(1);
								pattacker->QueueAttack(ANIM_GROUP_ATTACK_THROW);
								eState = CombatProcedureAttackGrenade::ATTACK_STATE_ATTACKING;
								if (pexplosion) {
									if (CombatUtilities::IsDangerousExplosion(pexplosion)) {
										pCombatDialogueManager->StartDialogue(pCombatController, DT_COMBAT, 4, 1);
									}
								}
							}
						}
					}
				}
			} else {
				ClearLookAtTarget();
				eState = CombatProcedureAttackGrenade::ATTACK_STATE_IDLE;
			}
		} else {
			if (eState) {
				ClearLookAtTarget();
				eState = CombatProcedureAttackGrenade::ATTACK_STATE_IDLE;
			}
		}
	} else {
		SetFailed("Missing either weapon, animation, or weapon projectile");
	}
}

void CombatProcedureAttackGrenade::Shutdown() {
	ClearLookAtTarget();
}

void CombatProcedureAttackGrenade::DisplayDebugText(IDebugText* apDebugText, DEBUGTEXT_LOCATION aeLocation,
	DEBUGTEXT_JUSTIFY aeJustify) {
	CombatProcedureAttackGrenade::ATTACK_STATE eState = this->eState;
	if (eState) {
		if (eState == CombatProcedureAttackGrenade::ATTACK_STATE_ATTACKING) {
			apDebugText->PrintLine(aeLocation, aeJustify, "ATTACKING");
		}
	} else {
		apDebugText->PrintLine(aeLocation, aeJustify, "IDLE");
	}
	apDebugText->PrintLine(aeLocation, aeJustify, "Targeted Point: %.02f, %.02f, %.02f", TargetedPoint.x,
		TargetedPoint.y, TargetedPoint.z);
	if (bHighArc) {
		apDebugText->PrintLine(aeLocation, aeJustify, "HIGH ARC");
	}
	if (ExplosiveProjectileBlockedTimer.IsSet()) {
		// Reads the target time first, then the elapsed time, and passes each on as a double.
		double dTargetTime = ExplosiveProjectileBlockedTimer.GetTargetTime();
		double dElapsedTime = ExplosiveProjectileBlockedTimer.ElapsedTime();
		apDebugText->PrintLine(aeLocation, aeJustify, "Explosive Projectile Blocked Timer: %.02f/%.02f", dElapsedTime,
			dTargetTime);
	}
}

bool CombatProcedureAttackGrenade::GetTargetedPoint(NiPoint3& arTargetedPoint, NiPoint3& arReleasePoint) {
	Actor* pattacker = GetAttacker();
	Actor* ptarget = GetTarget();
	TESObjectWEAP* pweapon = pCombatController->GetCurrentWeapon();
	BGSProjectile* pprojectile = pweapon->GetProjectile(pattacker);
	BGSExplosion* pexplosion = pprojectile->GetExplosion();
	bool barcValid = 0;
	if (bTargetLocation) {
		if (CombatUtilities::CheckProjectileArc(pattacker, ptarget, pprojectile, arReleasePoint, TargetLocation,
				NiPoint3_ZERO, bHighArc, TargetedPoint, -1.0f)) {
			arTargetedPoint = TargetedPoint;
			barcValid = 1;
		}
	} else {
		BGSProjectile* Projectile_ = pweapon->GetProjectile(pattacker);
		Actor* Target = GetTarget();
		Actor* pattacker_2 = GetAttacker();
		if (CombatUtilities::CalculateGrenadeThrow_(pattacker_2, Target, Projectile_, &arReleasePoint, &bHighArc,
				&TargetedPoint, -1.0f)) {
			arTargetedPoint = TargetedPoint;
			barcValid = 1;
		}
	}
	if (barcValid) {
		if (pexplosion) {
			NiPoint3 hitLocation;
			if (bTargetLocation) {
				hitLocation = TargetLocation;
			} else {
				hitLocation = TargetedPoint;
				hitLocation.z = (&ptarget->GetLocationOnReference())->z;
			}
			float ftime =
				CombatUtilities::ComputeProjectileTime_(pprojectile, arReleasePoint, hitLocation, bHighArc, 0.0f);
			if (!CheckExplosion(pexplosion, hitLocation, ftime)) {
				barcValid = 0;
			}
		}
	}
	if (!barcValid && bTargetLocation) {
		pCombatController->pCombatState->SetTargetFlushed(1);
		SetFinished();
	}
	return barcValid;
}

void CombatProcedureAttackGrenade::FiredProjectile(Projectile* apProjectile) {
	if (bHighArc) {
		float* v2 = &gs_fGrenadeHighArcSpeedPercentage.Float();
		apProjectile->SetSpeedMult(*v2);
	}
	bHighArc = 0;
	if (bTargetLocation) {
		pCombatController->pCombatState->SetTargetFlushed(1);
		SetFinished();
	}
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureAttackGrenade::GetType() {
	return ATTACK_GRENADE;
}

// The procedure's data, the state, the target location and its flag, the targeted point, the arc flag, then the
// blocked-projectile timer.
void CombatProcedureAttackGrenade::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	CombatProcedure::SaveGame(apSaveGameBuffer);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eState), 4, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bTargetLocation), 1, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&TargetLocation), 0xC, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&TargetedPoint), 0xC, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bHighArc), 1, 0);
	ExplosiveProjectileBlockedTimer.SaveGame(apSaveGameBuffer);
}

void CombatProcedureAttackGrenade::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	CombatProcedure::LoadGame(apLoadGameBuffer);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eState), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bTargetLocation), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&TargetLocation), 0xC);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&TargetedPoint), 0xC);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHighArc), 1);
	ExplosiveProjectileBlockedTimer.LoadGame(apLoadGameBuffer);
}

void CombatProcedureAttackGrenade::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	CombatProcedure::InitLoadGame(apLoadGameBuffer);
}

bool CombatProcedureAttackGrenade::CheckExplosion(BGSExplosion* apExplosion, const NiPoint3& arLocation, float afTime) {
	Actor* pattacker = pCombatController->pAttacker;
	CombatState* pcombatState = pCombatController->pCombatState;
	if (pCombatManager->CheckExplosionAttack(pattacker, apExplosion, arLocation, afTime)) {
		return 1;
	}
	if (ExplosiveProjectileBlockedTimer.IsSet()) {
		if (ExplosiveProjectileBlockedTimer.Check()) {
			pcombatState->pExplosiveProjectileBlocked = apExplosion;
		}
	} else {
		float* v4 = &fExplosiveProjectileBlockedWaitTime.Float();
		ExplosiveProjectileBlockedTimer.Reset(*v4);
	}
	if (bTargetLocation) {
		pcombatState->SetTargetFlushed(1);
		SetFinished();
	}
	return 0;
}

NiPoint3* CombatProcedureAttackGrenade::CalculateTargetedPoint(NiPoint3* a1) {
	NiPoint3 v11;
	NiPoint3 arResult;
	if (bTargetLocation || GetTarget()) {
		NiPoint3* ProjectedPointForTarget;
		float v9;
		TESObjectWEAP* CurrentWeapon = pCombatController->GetCurrentWeapon();
		Actor* Attacker = GetAttacker();
		BGSProjectile* apProjectile = CurrentWeapon->GetProjectile(Attacker);
		if (this->bHighArc) {
			v9 = gs_fGrenadeHighArcSpeedPercentage.Float();
		} else {
			v9 = 1.0f;
		}
		float afProjectileSpeed = apProjectile->GetSpeed() * v9;
		bool bHighArc = this->bHighArc;
		if (bTargetLocation) {
			Actor* v6 = GetAttacker();
			ProjectedPointForTarget = CombatUtilities::GetProjectedPointForTarget(&v11, v6, TargetLocation, 0,
				apProjectile, afProjectileSpeed, bHighArc);
		} else {
			Actor* Target = GetTarget();
			Actor* v4 = GetAttacker();
			ProjectedPointForTarget = &(arResult = CombatUtilities::GetProjectedPointForTarget(v4, Target, 0,
											apProjectile, afProjectileSpeed, bHighArc));
		}
		NiPoint3 ecx0 = *ProjectedPointForTarget;
		*a1 = ecx0;
		return a1;
	} else {
		*a1 = NiPoint3_ZERO;
		return a1;
	}
}
