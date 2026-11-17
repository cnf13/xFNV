#include "Fallout/Magic/MagicHitEffect.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicModelHitEffect.h"
#include "Fallout/Magic/MagicShaderHitEffect.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "SDK/MSVC/rtti.h"

NiRTTI MagicHitEffect::ms_RTTI("MagicHitEffect", &BSTempEffect::ms_RTTI);

MagicHitEffect::MagicHitEffect()
	: BSTempEffect(nullptr, 3.4028235e+38f), pEffect(nullptr), pTarget(nullptr), fAge(0.0f), bFinished(false) {}

MagicHitEffect::MagicHitEffect(TESObjectREFR* a1, ActiveEffect* a2)
	: BSTempEffect(a1 ? a1->TESObjectREFR::GetParentCell() : nullptr, 3.4028235e+38f), pEffect(a2), pTarget(a1),
	  fAge(0.0f), bFinished(false) {}

MagicHitEffect::~MagicHitEffect() {
	pTarget = nullptr;
	if (pEffect) {
		pEffect->RemoveHitEffect(this);
	}
	pEffect = nullptr;
}

bool MagicHitEffect::Update(float afTimeDelta) {
	fAge = (float)(fAge + afTimeDelta);
	if ((!pTarget) || ((!pTarget->Get3D()) || pTarget->TESForm::GetDelete())) {
		return (bool)0;
	}
	if (fLifetime < ((double)fAge)) {
		bFinished = (bool)1;
	}
	return (bool)1;
}

// fAge (4) and bFinished (1), then fLifetime (4) from save version 0x72 on.
uint16_t MagicHitEffect::GetSaveSize(ActiveEffect* apEffect, TESObjectREFR* apTarget) {
	uint16_t usSize = 0;
	usSize += 4;
	usSize += 1;
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x72)
		usSize += 4;
	return usSize;
}

// The temp effect's data, the age and finished flag, then the target's form ID.
void MagicHitEffect::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
	BSTempEffect::SaveGame(apSaveGameBuffer);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fAge), 4, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bFinished), 1, 0);
	apSaveGameBuffer->SaveFormID(pTarget, 0);
}

void MagicHitEffect::SaveGame(ActiveEffect* apEffect, TESObjectREFR* apTarget) {
	pSaveLoadGameOLD->SaveGameDataOLD(&fAge, 4);
	pSaveLoadGameOLD->SaveGameDataOLD(&bFinished, 1);
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x72)
		pSaveLoadGameOLD->SaveGameDataOLD(&fLifetime, 4);
}

// The target is loaded as its form ID; InitLoadGame resolves it.
void MagicHitEffect::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
	BSTempEffect::LoadGame(apLoadGameBuffer);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fAge), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bFinished), 1);
	apLoadGameBuffer->LoadFormID(pTarget);
}

void MagicHitEffect::LoadGame(ActiveEffect* apEffect, TESObjectREFR* apTarget) {
	pSaveLoadGameOLD->LoadGameDataOLD(&fAge, 4);
	pSaveLoadGameOLD->LoadGameDataOLD(&bFinished, 1);
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x72)
		pSaveLoadGameOLD->LoadGameDataOLD(&fLifetime, 4);
	pEffect = apEffect;
	pTarget = apTarget;
}

void MagicHitEffect::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
	TESObjectREFR* v3;
	if (pTarget) {
		TESForm* FormByNumericID =
			TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTarget)));
		v3 = dynamic_cast<TESObjectREFR*>(FormByNumericID);
	} else {
		v3 = 0;
	}
	pTarget = v3;
}

void MagicHitEffect::InitLoadGame(ActiveEffect* apEffect, TESObjectREFR* apTarget) {
	if (apTarget) {
		pCell = apTarget->TESObjectREFR::GetParentCell();
	}
}

void MagicHitEffect::FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
	BSTempEffect::FinishLoadGame(apLoadGameBuffer);
}

void MagicHitEffect::UpdateParentCell(NiAVObject* apRoot) {
	if (pTarget) {
		pCell = pTarget->GetParentCell();
		if (apRoot) {
			if (pCell) {
				if (pCell->Get3D()) {
					NiNode* v2;
					if (!apRoot->m_pkParent || (v2 = pCell->Get3D(), v2 != apRoot->m_pkParent->m_pkParent)) {
						NiNode* DynamicNode = static_cast<NiNode*>(pCell->GetDynamicNode());
						DynamicNode->AttachChild(apRoot, 1);
					}
				}
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
BSSimpleList<MagicHitEffect*>* MagicHitEffect::InstantiateHitEffects(ActiveEffect* apEffect) {
	MagicHitEffect* peffect;
	if (apEffect->pTarget->GetTargetAsActor()) {
		Actor* TargetAsActor = apEffect->pTarget->GetTargetAsActor();
		TESCreature* pcreature = dynamic_cast<TESCreature*>(TargetAsActor->GetActorBase());
		Actor* v2 = apEffect->pTarget->GetTargetAsActor();
		Character* pcharacter = dynamic_cast<Character*>(v2);
		if (!pcreature || !pcreature->IsHumanoidCreature()) {
			if (pcharacter) {
				pcharacter->IsNpc();
			}
		}
	}
	BSSimpleList<MagicHitEffect*>* plist_2 = 0;
	peffect = 0;
	EffectSetting* EffectSetting_ = apEffect->GetEffectSetting();
	if (EffectSetting_->IsFlagSet(EffectSetting::GORY_VISUALS) && bDisableAllGore.Bool()) {
		return 0;
	}
	if (!apEffect->IsFlagSet(ActiveEffect::NO_HITEFFECT_NIF) && !Main::GetSceneIsComplex()) {
		MagicHitEffect* peffect_2;
		MagicModelHitEffect* apMemory =
			static_cast<MagicModelHitEffect*>(NiMemObject::operator new(sizeof(MagicModelHitEffect)));
		if (apMemory) {
			MagicTarget* Target = apEffect->pTarget;
			TESObjectREFR* v5 = Target->GetTargetStatsObject();
			peffect_2 = (MagicHitEffect*)::new (apMemory) MagicModelHitEffect(v5, apEffect);
		} else {
			peffect_2 = 0;
		}
		peffect = peffect_2;
		if (peffect_2->Init()) {
			BSSimpleList<MagicHitEffect*>* plist;
			ReferenceProcessLists.AddTempEffect(peffect);
			void* v18 = ourMalloc(sizeof(BSSimpleList<MagicHitEffect*>));
			if (v18) {
				plist = ::new (v18) BSSimpleList<MagicHitEffect*>(peffect);
			} else {
				plist = 0;
			}
			plist_2 = plist;
		} else {
			if (peffect) {
				delete peffect;
			}
		}
	}
	if (!apEffect->IsFlagSet(ActiveEffect::NO_HITEFFECT_SHADER)) {
		MagicHitEffect* peffect_3;
		MagicShaderHitEffect* v17 =
			static_cast<MagicShaderHitEffect*>(NiMemObject::operator new(sizeof(MagicShaderHitEffect)));
		if (v17) {
			MagicTarget* v11 = apEffect->pTarget;
			TESObjectREFR* v7 = v11->GetTargetStatsObject();
			peffect_3 = (MagicHitEffect*)::new (v17) MagicShaderHitEffect(v7, apEffect);
		} else {
			peffect_3 = 0;
		}
		peffect = peffect_3;
		if (peffect_3->Init()) {
			ReferenceProcessLists.AddTempEffect(peffect);
			if (plist_2) {
				plist_2->AddHead(peffect);
			} else {
				BSSimpleList<MagicHitEffect*>* v9;
				void* v16 = ourMalloc(sizeof(BSSimpleList<MagicHitEffect*>));
				if (v16) {
					v9 = ::new (v16) BSSimpleList<MagicHitEffect*>(peffect);
				} else {
					v9 = 0;
				}
				return v9;
			}
		} else {
			if (peffect) {
				delete peffect;
			}
		}
	}
	return plist_2;
}

void ActiveEffect::RemoveHitEffect(MagicHitEffect* apHitEffect) {
	if (pHitEffects) {
		pHitEffects->Remove(apHitEffect);
	}
}
