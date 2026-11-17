#include "Fallout/Magic/DetectLifeEffect.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicShaderHitEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/ValueModifierEffect.h"

// Not named in the PDB or IDA; name chosen here. Whether the detect-life archetype took Create: the exe stores
// RegisterActiveEffect's whole return byte.
static char bRegistered =
	EffectArchetypes::RegisterActiveEffect(EffectArchetypes::DETECT_LIFE, &DetectLifeEffect::Create);

DetectLifeEffect::DetectLifeEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
	: ValueModifierEffect(apCaster, apItem, apEffect) {}

DetectLifeEffect::~DetectLifeEffect() {}

// Copies the effect: allocates a new one, constructs it from this effect's caster, spell and effect item, then copies
// the shared data into it. The exe calls CopyData through the vtable (slot 17, +0x44) with this effect as `this`.
ActiveEffect* DetectLifeEffect::Copy() {
	DetectLifeEffect* pCopy = static_cast<DetectLifeEffect*>(ourMalloc(sizeof(DetectLifeEffect)));
	if (pCopy) {
		pCopy = ::new (pCopy) DetectLifeEffect(pCaster, pSpell, pEffect);
	} else {
		pCopy = nullptr;
	}
	static_cast<ActiveEffect*>(this)->CopyData(pCopy);
	return pCopy;
}

// Starts the value modifier; on the player it also clears the detect-life flag.
void DetectLifeEffect::Start() {
	ValueModifierEffect::Start();
	if (GetTarget()->GetTargetStatsObject() == pPlayer) {
		CheckPlayerRunningDetectLife();
	}
}

void DetectLifeEffect::Update(float afSecondsElapsed) {
	ValueModifierEffect::Update(afSecondsElapsed);
}

void DetectLifeEffect::Finish() {
	ValueModifierEffect::Finish();
	MagicTarget* Target = pTarget;
	if (Target->GetTargetStatsObject() == pPlayer) {
		DetectLifeEffect::CheckPlayerRunningDetectLife();
	}
}

void DetectLifeEffect::CheckPlayerRunningDetectLife() {
	DetectLifeEffect::bPlayerDetectingLife = (bool)0;
}

char DetectLifeEffect::GetPlayerDetectingLife() {
	return (int8_t)DetectLifeEffect::bPlayerDetectingLife;
}

ActiveEffect* DetectLifeEffect::Create(MagicCaster* a1, MagicItem* a2, EffectItem* a3) {
	void* v5 = ourMalloc(sizeof(DetectLifeEffect));
	if (v5) {
		return ::new (v5) DetectLifeEffect(a1, a2, a3);
	} else {
		return nullptr;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
bool DetectLifeEffect::CheckActorDetected(Actor* apTarget, bool abCurrentlyDetected) {
	TESEffectShader* pEffectData = MagicSystem::GetDetectLifeShaderEffect();
	if (!pEffectData || !apTarget || apTarget == pPlayer || !apTarget->GetCurrentAIProcess()) {
		return false;
	}
	// The exe scales the range by a magnitude of 0.0f, rounding the product and its square to floats.
	float fmagnitude = 0.0f;
	float fdetectRange = fmagnitude * fMagicUnitsPerFoot.Float();
	fdetectRange = fdetectRange * fdetectRange;
	NiPoint3 location = pPlayer->GetLocationOnReference();
	// fcompp, test ah,0x41, jne: detected only where the squared range is above the squared distance (not on NaN).
	if (fdetectRange > (location - apTarget->GetLocationOnReference()).SqrLength() && !apTarget->IsDead(false) &&
		apTarget->GetCurrentAIProcess()->GetProcessLevel() == PROCESS_TYPE_HIGH && !apTarget->GetDestroyed()) {
		if (!abCurrentlyDetected) {
			MagicShaderHitEffect* pMemory =
				static_cast<MagicShaderHitEffect*>(NiMemObject::operator new(sizeof(MagicShaderHitEffect)));
			MagicHitEffect* peffect;
			if (pMemory) {
				// The duration is the float -1.0f at 0x1012054, stored to the argument slot with fstp dword.
				peffect = ::new (pMemory) MagicShaderHitEffect(apTarget, pEffectData, -1.0f);
			} else {
				peffect = nullptr;
			}
			if (peffect->Init()) {
				ReferenceProcessLists.AddTempEffect(peffect);
			} else if (peffect) {
				delete peffect;
			}
		}
		return true;
	}
	if (abCurrentlyDetected) {
		ReferenceProcessLists.FinishMagicShaderHitEffect(apTarget, pEffectData);
	}
	return false;
}
