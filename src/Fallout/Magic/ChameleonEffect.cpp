#include "Fallout/Magic/ChameleonEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/ValueModifierEffect.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"

static char bChameleonEffectInitialized =
	EffectArchetypes::RegisterActiveEffect(EffectArchetypes::CHAMELEON, &ChameleonEffect::Instantiate);

ChameleonEffect::ChameleonEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
	: ValueModifierEffect(apCaster, apItem, apEffect) {}

ChameleonEffect::~ChameleonEffect() {}

// Copies the effect: a new instance built from the same caster, spell and effect item, then the field copy.
// The exe's C++ EH (unwind) frame is left out: it only matters when a call in here throws.
ActiveEffect* ChameleonEffect::Copy() {
	ActiveEffect* v2;
	void* v4 = ourMalloc(sizeof(ChameleonEffect));
	if (v4) {
		v2 = ::new (v4) ChameleonEffect(pCaster, pSpell, pEffect);
	} else {
		v2 = nullptr;
	}
	// The exe calls the override through vtable slot 17 (+0x44), with this in ecx.
	ActiveEffect* pThis = this;
	pThis->CopyData(v2);
	return v2;
}

void ChameleonEffect::FinishInitLoadGame(Actor* apActor) {
	ValueModifierEffect::FinishInitLoadGame(apActor);
	if (BSShaderManager::IsRefractionEnabled()) {
		apActor->SetAlpha(1.0f);
		float v6 = apActor->GetActorFloatValue(eWhichActorValue) / 100.0;
		float a2 = BSMAX(0.0f, v6);
		float v9 = BSMIN(1.0f, a2);
		float v5 = 1.0 - v9;
		float x2 = MagicSystem::GetChameleonMaxRefraction();
		float x1 = MagicSystem::GetChameleonMinRefraction();
		float fRefractionPower = FLerp(x1, x2, 0.0f, 1.0f, v5);
		apActor->SetRefraction(1, fRefractionPower);
	} else {
		apActor->UpdateAlpha();
	}
}

void ChameleonEffect::Revert(Actor* apActor) {
	ActiveEffect::Revert(apActor);
	if (BSShaderManager::IsRefractionEnabled()) {
		apActor->SetRefraction(1, 0.0f);
	} else {
		apActor->UpdateAlpha();
	}
}

// IDA: ChameleonEffect::Revert_0.
void ChameleonEffect::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
	Actor* pactor = apLoadGameBuffer->GetActor();
	// The exe calls the empty base body 0x4534F0.
	ActiveEffect::Revert(apLoadGameBuffer);
	if (BSShaderManager::IsRefractionEnabled()) {
		pactor->SetRefraction(1, 0.0f);
	} else {
		pactor->UpdateAlpha();
	}
}

void ChameleonEffect::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	Actor* pactor = apLoadGameBuffer->GetActor();
	ActiveEffect::FinishLoadGame(apLoadGameBuffer);
	if (bActive) {
		if (BSShaderManager::IsRefractionEnabled()) {
			pactor->SetAlpha(1.0f);
			float v6 = pactor->GetActorFloatValue(eWhichActorValue) / 100.0;
			float a2 = BSMAX(0.0f, v6);
			float fChameleon = BSMIN(1.0f, a2);
			float v5 = 1.0 - fChameleon;
			float x2 = MagicSystem::GetChameleonMaxRefraction();
			float x1 = MagicSystem::GetChameleonMinRefraction();
			float fRefractionPower = FLerp(x1, x2, 0.0f, 1.0f, v5);
			pactor->SetRefraction(1, fRefractionPower);
		} else {
			pactor->UpdateAlpha();
		}
	}
}

void ChameleonEffect::CopyData(ActiveEffect* apDestination) {
	ValueModifierEffect::CopyData(apDestination);
}

bool ChameleonEffect::CheckTarget(MagicTarget* apTarget) {
	Actor* pTargetActor;
	if (apTarget) {
		pTargetActor = apTarget->GetTargetAsActor();
	} else {
		pTargetActor = nullptr;
	}
	return pTargetActor != nullptr;
}

void ChameleonEffect::Start() {
	Actor* ptargetActor;
	if (pTarget) {
		ptargetActor = pTarget->GetTargetAsActor();
	} else {
		ptargetActor = 0;
	}
	ValueModifierEffect::Start();
	if (ptargetActor) {
		if (BSShaderManager::IsRefractionEnabled()) {
			ptargetActor->SetAlpha(1.0f);
			float v5 = ptargetActor->GetActorFloatValue(eWhichActorValue) / 100.0;
			float a2 = BSMAX(0.0f, v5);
			float v9 = BSMIN(1.0f, a2);
			float v4 = 1.0 - v9;
			float x2 = MagicSystem::GetChameleonMaxRefraction();
			float x1 = MagicSystem::GetChameleonMinRefraction();
			float fRefractionPower = FLerp(x1, x2, 0.0f, 1.0f, v4);
			ptargetActor->SetRefraction(1, fRefractionPower);
		} else {
			ptargetActor->UpdateAlpha();
		}
	}
}

void ChameleonEffect::Finish() {
	Actor* ptargetActor;
	ValueModifierEffect::Finish();
	if (pTarget) {
		ptargetActor = pTarget->GetTargetAsActor();
	} else {
		ptargetActor = 0;
	}
	if (ptargetActor) {
		if (BSShaderManager::IsRefractionEnabled()) {
			float v5 = ptargetActor->GetActorFloatValue(eWhichActorValue) / 100.0;
			float a2 = BSMAX(0.0, v5);
			float v9 = BSMIN(1.0f, a2);
			if (!(v9 > 0.0)) {
				ptargetActor->SetRefraction(1, 0.0f);
				ptargetActor->UpdateAlpha();
			} else {
				float v4 = 1.0 - v9 / 100.0;
				float x2 = MagicSystem::GetChameleonMaxRefraction();
				float x1 = MagicSystem::GetChameleonMinRefraction();
				float fRefractionPower = FLerp(x1, x2, 0.0f, 1.0f, v4);
				ptargetActor->SetRefraction(1, fRefractionPower);
			}
		} else {
			ptargetActor->UpdateAlpha();
		}
	}
}

// The detect life, invisibility and night eye effects share this entry.
void ChameleonEffect::Update(float afSecondsElapsed) {
	ValueModifierEffect::Update(afSecondsElapsed);
}

ActiveEffect* ChameleonEffect::Instantiate(MagicCaster* a1, MagicItem* a2, EffectItem* a3) {
	void* v5 = ourMalloc(sizeof(ChameleonEffect));
	if (v5) {
		return ::new (v5) ChameleonEffect(a1, a2, a3);
	} else {
		return nullptr;
	}
}
