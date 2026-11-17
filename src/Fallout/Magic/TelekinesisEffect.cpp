#include "Fallout/Magic/TelekinesisEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSDevices/Controls.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkMouseSpringAction.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/NiObjectGarbage.h"
#include "BSMain/Setting.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESEffectShader.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/bhkTelekinesisListener.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicShaderHitEffect.h"
#include "Fallout/Magic/ValueModifierEffect.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"

// Whether the telekinesis archetype took Instantiate: stores RegisterActiveEffect's whole return byte.
static char bRegistered =
	EffectArchetypes::RegisterActiveEffect(EffectArchetypes::TELEKINESIS, &TelekinesisEffect::Instantiate);

TelekinesisEffect::TelekinesisEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
	: ValueModifierEffect(apCaster, apSpell, apEffect), spSpring(nullptr), fHoldDistance(0.0f), fMoveVelocity(0.0f),
	  pRef(nullptr), bThrow(false), bFirstUpdate(false) {}

TelekinesisEffect::~TelekinesisEffect() {}

// Allocates a copy, constructs it from this effect's caster, spell and effect item, then copies the effect data.
ActiveEffect* TelekinesisEffect::Copy() {
	void* pMemory = ourMalloc(sizeof(TelekinesisEffect));
	TelekinesisEffect* pCopy;
	if (pMemory) {
		pCopy = ::new (pMemory) TelekinesisEffect(pCaster, pSpell, pEffect);
	} else {
		pCopy = nullptr;
	}
	CopyData(pCopy);
	return pCopy;
}

void TelekinesisEffect::Update(float a1) {
	TESObjectREFR* v2;
	TESObjectREFR* GrabObject;
	bhkWorld* pobjectWorld;
	bhkWorld* pcasterWorld;
	MagicCaster* grabSpring;
	ValueModifierEffect::Update(a1);
	bhkMouseSpringAction* BhkMouseSpringAction = static_cast<bhkMouseSpringAction*>(pPlayer->GetBhkMouseSpringAction());
	if (BhkMouseSpringAction->GetWorldObject() &&
		(!pCaster || !pPlayer->GetGrabObject()->GetParentCell()
				? pcasterWorld = 0
				: (grabSpring = pCaster,
					  (v2 = grabSpring->GetCasterStatsObject(), pcasterWorld = v2->GetParentCell()->GetbhkWorld())),
			(!pPlayer->GetGrabObject() || !pPlayer->GetGrabObject()->GetParentCell()
					? pobjectWorld = 0
					: (GrabObject = pPlayer->GetGrabObject(),
						  pobjectWorld = GrabObject->GetParentCell()->GetbhkWorld()),
				BhkMouseSpringAction &&
					(pRef == pPlayer->GetGrabObject() &&
						(pPlayer->GetGrabMode() == PlayerCharacter::GRAB_TELEKINESIS &&
							pcasterWorld == pobjectWorld))))) {
		Controls* v5;
		Controls* Controls_ = appMain->GetControls();
		if (Controls_->GetUserAction(Controls::UACTN_BLOCK, 0) &&
			(v5 = appMain->GetControls(), v5->GetUserAction(Controls::UACTN_USE, 0))) {
			float* v6 = &fMagicTelekinesisMoveBase.Float();
			if (fMoveVelocity > fMagicTelekinesisMoveAccelerate.Float() * 0.10000000149011612 + *v6) {
				bThrow = 1;
			}
			Dispel(0);
		} else {
			Controls* v7 = appMain->GetControls();
			if (v7->GetUserAction(Controls::UACTN_USE, 0)) {
				if (fMagicTelekinesisMoveBase.Float() > static_cast<double>(fMoveVelocity)) {
					fMoveVelocity = fMagicTelekinesisMoveBase.Float();
				}
				fMoveVelocity = fMagicTelekinesisMoveAccelerate.Float() * a1 + fMoveVelocity;
				if (fMagicTelekinesisMoveMax.Float() < static_cast<double>(fMoveVelocity)) {
					fMoveVelocity = fMagicTelekinesisMoveMax.Float();
				}
			} else {
				Controls* v8 = appMain->GetControls();
				if (v8->GetUserAction(Controls::UACTN_BLOCK, 0)) {
					if (fMoveVelocity > -fMagicTelekinesisMoveBase.Float()) {
						fMoveVelocity = -fMagicTelekinesisMoveBase.Float();
					}
					fMoveVelocity = fMoveVelocity - fMagicTelekinesisMoveAccelerate.Float() * a1;
					if (fMoveVelocity < -fMagicTelekinesisMoveMax.Float()) {
						fMoveVelocity = -fMagicTelekinesisMoveMax.Float();
					}
				} else {
					fMoveVelocity = 0.0f;
				}
			}
		}
		fHoldDistance = pPlayer->GetGrabDistance();
		fHoldDistance = fMoveVelocity * a1 + fHoldDistance;
		if (fMagicTelekinesisDistanceMin.Float() > static_cast<double>(fHoldDistance)) {
			fHoldDistance = fMagicTelekinesisDistanceMin.Float();
		}
		float* v9 = &fMagicUnitsPerFoot.Float();
		double v12 = fMagnitude * *v9;
		if (fHoldDistance > fMagicTelekinesiDistanceMult.Float() * v12) {
			float* v10 = &fMagicUnitsPerFoot.Float();
			double v11 = fMagnitude * *v10;
			fHoldDistance = fMagicTelekinesiDistanceMult.Float() * v11;
		}
		pPlayer->SetTelekinesisDistance(fHoldDistance);
		if (!bFirstUpdate) {
			pPlayer->UpdateMouseSpring(a1);
		}
		bFirstUpdate = 0;
	} else {
		Dispel(0);
	}
}

// Four bytes of its own; save version 0x40 added two more four-byte values.
uint16_t TelekinesisEffect::GetSaveSize(Actor* apActor) {
	uint16_t usSize = ValueModifierEffect::GetSaveSize(apActor);
	usSize += 4;
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x40) {
		usSize += 4;
		usSize += 4;
	}
	return usSize;
}

// The target is saved as its form ID; save version 0x40 added the hold distance and move velocity.
void TelekinesisEffect::SaveGame(Actor* apActor) {
	ValueModifierEffect::SaveGame(apActor);
	uint32_t uiRefID = 0;
	if (pRef)
		uiRefID = pRef->GetFormID();
	pSaveLoadGameOLD->SaveNumericID(&uiRefID, 4);
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x40) {
		pSaveLoadGameOLD->SaveGameDataOLD(&fHoldDistance, 4);
		pSaveLoadGameOLD->SaveGameDataOLD(&fMoveVelocity, 4);
	}
}

// The target field holds the loaded form ID until the load game resolves it.
void TelekinesisEffect::LoadGame(Actor* apActor) {
	ValueModifierEffect::LoadGame(apActor);
	uint32_t uiRefID;
	pSaveLoadGameOLD->LoadNumericID(&uiRefID, 4);
	pRef = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiRefID));
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x40) {
		pSaveLoadGameOLD->LoadGameDataOLD(&fHoldDistance, 4);
		pSaveLoadGameOLD->LoadGameDataOLD(&fMoveVelocity, 4);
	}
}

void TelekinesisEffect::FinishInitLoadGame(Actor* apActor) {
	TESForm* formByNumericID;
	TESObjectREFR* pRef;
	ValueModifierEffect::FinishInitLoadGame(apActor);
	pRef = this->pRef;
	if (pRef) {
		formByNumericID = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pRef));
		this->pRef = dynamic_cast<TESObjectREFR*>(formByNumericID);
	}
	if (this->pRef) {
		CreateSpring();
	}
	bFirstUpdate = (bool)1;
}

void TelekinesisEffect::Revert(Actor* apActor) {
	ValueModifierEffect::Revert(apActor);
}

// A deleted caster dispels the effect at once.
bool TelekinesisEffect::CheckDeletedCaster(MagicCaster* apCaster) {
	bool bDeleted = ValueModifierEffect::CheckDeletedCaster(apCaster);
	if (bDeleted) {
		Dispel(true);
	}
	return bDeleted;
}

void TelekinesisEffect::CopyData(ActiveEffect* apSource) {
	ValueModifierEffect::CopyData(apSource);
}

void TelekinesisEffect::Start() {
	ValueModifierEffect::Start();
	pRef = InterfaceManager::GetTelekinesisTarget();
	CreateSpring();
	bFirstUpdate = (bool)0;
}

void TelekinesisEffect::Finish() {
	DoThrow();
	EffectSetting* EffectSetting_ = GetEffectSetting();
	TESEffectShader* FormTwo = EffectSetting_->data.pEffectShader;
	ReferenceProcessLists.FinishMagicShaderHitEffect(pRef, FormTwo);
	ValueModifierEffect::Finish();
}

void TelekinesisEffect::DoThrow() {
	if (!pPlayer->GetBhkMouseSpringAction()) {
		return;
	}
	hkpEntity* pEntity = static_cast<bhkMouseSpringAction*>(pPlayer->GetBhkMouseSpringAction())->GetEntity();
	pPlayer->DestroyMouseSpring();
	if (!pEntity) {
		return;
	}
	bhkRigidBody* pBody = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(pEntity));
	if (bThrow) {
		NiPoint3 kDirection;
		alignas(16) NiMatrix3 kRotation;
		NiPoint3 kAngle = pPlayer->GetAngleOnReference();
		kRotation.FromEulerAnglesZXY(kAngle.z, kAngle.x, kAngle.y);
		kRotation.GetCol(1, kDirection);
		float fSpeed = fMagicTelekinesisThrow.Float() + fMoveVelocity;
		NiPoint3 kVelocity = kDirection * fSpeed;
		alignas(16) hkVector4 kImpulse;
		NI2HK(kImpulse, kVelocity);
		hkSimdReal kMass(pBody->GetMass());
		kImpulse.mul4(kMass);
		pBody->operator*()->applyLinearImpulse(kImpulse);
	} else {
		pBody->SetLinearVelocity(NiPoint3_ZERO);
	}
}

void TelekinesisEffect::CreateSpring() {
	CFilter v14;
	CFilter v17;
	CFilter apResult;
	NiPoint3 location;
	if (pRef && (pCaster && pRef->GetObjectReference()->GetFormType() != CONT_ID)) {
		CFilter* v1;
		CFilter* v2;
		bhkRigidBody* pbody;
		bhkCollisionObject* pcol;
		NiAVObject* pav = (NiAVObject*)pRef->Get3D();
		if (pav) {
			pcol = FindFirstCollisionObject(pav);
		} else {
			pcol = 0;
		}
		if (pcol) {
			pbody = pcol->GetRigidBody();
		} else {
			pbody = 0;
		}
		if (pbody) {
			bhkTelekinesisListener::AllowToFinish(pbody);
		}
		if (pbody &&
			((v1 = pbody->GetCollisionFilter(&apResult), v1->GetLayer() == L_CLUTTER) ||
				(v2 = pbody->GetCollisionFilter(&v17), v2->GetLayer() == L_WEAPON))) {
			bhkTelekinesisListener* v9;
			NiPoint3 dir;
			NiPoint3 origin;
			hkpRigidBody* v28 = pbody->operator*();
			void* apMemory = ourMalloc(sizeof(bhkTelekinesisListener));
			if (apMemory) {
				CFilter* CollisionFilter = pbody->GetCollisionFilter(&v14);
				v9 = ::new (apMemory) bhkTelekinesisListener(pRef, pbody, CollisionFilter->iFilter);
			} else {
				v9 = 0;
			}
			BSFadeNode* pavlod = static_cast<BSFadeNode*>(NiObject::DynamicCastStatic(&BSFadeNode::ms_RTTI, pav));
			if (pavlod) {
				pavlod->SetLODMultType(LOD_MULT_DISTANTLOD);
			}
			if (pPlayer->GetBhkMouseSpringAction()) {
				pPlayer->DestroyMouseSpring();
			}
			if (!pSaveLoadGameOLD->GetLoading()) {
				fHoldDistance = 10000.0f;
			}
			pPlayer->CreateMouseSpring(pRef, PlayerCharacter::GRAB_TELEKINESIS, fHoldDistance);
			pPlayer->GetEyeVector(origin, dir, 1);
			if (!pSaveLoadGameOLD->GetLoading()) {
				bhkMouseSpringAction* BhkMouseSpringAction =
					static_cast<bhkMouseSpringAction*>(pPlayer->GetBhkMouseSpringAction());
				BhkMouseSpringAction->GetMousePositionInWorld(location);
				NiPoint3 originToRef = location - origin;
				fHoldDistance = dir.Dot(originToRef);
				pPlayer->SetTelekinesisDistance(fHoldDistance);
				dir.operator*=(fHoldDistance);
			}
			EffectSetting* EffectSetting_ = GetEffectSetting();
			if (EffectSetting_->data.pEffectShader) {
				MagicShaderHitEffect* peffect;
				MagicShaderHitEffect* v13 =
					static_cast<MagicShaderHitEffect*>(NiMemObject::operator new(sizeof(MagicShaderHitEffect)));
				if (v13) {
					EffectSetting* v6 = GetEffectSetting();
					TESEffectShader* FormTwo = v6->data.pEffectShader;
					peffect = ::new (v13) MagicShaderHitEffect(pRef, FormTwo, -1.0f);
				} else {
					peffect = 0;
				}
				if (peffect->Init()) {
					ReferenceProcessLists.AddTempEffect(peffect);
				} else {
					if (peffect) {
						delete peffect;
					}
				}
			}
		} else {
			Dispel(0);
		}
	} else {
		Dispel(0);
	}
}

ActiveEffect* TelekinesisEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
	void* v5 = ourMalloc(sizeof(TelekinesisEffect));
	if (v5) {
		return ::new (v5) TelekinesisEffect(apCaster, apSpell, apEffect);
	} else {
		return nullptr;
	}
}

hkpMouseSpringAction* bhkMouseSpringAction::operator*() const {
	if (this) {
		return static_cast<hkpMouseSpringAction*>(GethkObject());
	} else {
		return nullptr;
	}
}

// The game-unit overload.
void bhkMouseSpringAction::GetMousePositionInWorld(NiPoint3& arPos) {
	hkpMouseSpringAction* pm = **this;
	if (pm) {
		HK2NI(arPos, pm->m_mousePositionInWorld);
	}
}
