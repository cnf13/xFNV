#include "Fallout/Magic/CureEffect.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Reconstruction.hpp"

static char bCureDiseaseEffectInitialized =
	EffectArchetypes::RegisterActiveEffect(EffectArchetypes::CURE_DISEASE, &CureEffect::InstantiateCureDisease);
static char bCureParalysisEffectInitialized =
	EffectArchetypes::RegisterActiveEffect(EffectArchetypes::CURE_PARALYSIS, &CureEffect::InstantiateCureParalysis);
static char bCureAddictionEffectInitialized =
	EffectArchetypes::RegisterActiveEffect(EffectArchetypes::CURE_ADDICTION, &CureEffect::InstantiateCureAddiction);

// Not named in the PDB or IDA; name chosen here. Whether the cure-poison archetype took CurePoision: the exe stores
// RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(EffectArchetypes::CURE_POISON, &CurePoision);

// The exe's cleanup-only C++ EH frame is left out.
ActiveEffect* CurePoision(MagicCaster* a1, MagicItem* a2, EffectItem* a3) {
	CureEffect* apMemory = static_cast<CureEffect*>(ourMalloc(sizeof(CureEffect)));
	if (apMemory) {
		return ::new (apMemory) CureEffect(a1, a2, a3, 5);
	} else {
		return 0;
	}
}

CureEffect::CureEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect, int32_t aiActorValue)
	: ActiveEffect(apCaster, apItem, apEffect), eSpellType(static_cast<MagicSystem::SpellType>(aiActorValue)),
	  eArchetypeID(EffectArchetypes::ARCHETYPE_NONE) {}

CureEffect::CureEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect,
	EffectArchetypes::ArchetypeID aeArchetype)
	: ActiveEffect(apCaster, apItem, apEffect), eSpellType(MagicSystem::SPELL), eArchetypeID(aeArchetype) {}

CureEffect::~CureEffect() {}

// eArchetypeID -1 means the copy cures a spell type, built with the spell-type constructor; otherwise it cures an
// archetype, built with the archetype constructor (0x80C0B0). Either way the base data is then copied through the
// vtable (slot 17, ecx = this).
ActiveEffect* CureEffect::Copy() {
	ActiveEffect* pCopy = nullptr;
	if (eArchetypeID == -1) {
		void* pMemory = ourMalloc(sizeof(CureEffect));
		if (pMemory) {
			pCopy = ::new (pMemory) CureEffect(pCaster, pSpell, pEffect, (int32_t)eSpellType);
		}
	} else {
		void* pMemory = ourMalloc(sizeof(CureEffect));
		if (pMemory) {
			pCopy = ::new (pMemory) CureEffect(pCaster, pSpell, pEffect, eArchetypeID);
		}
	}
	CopyData(pCopy);
	return pCopy;
}

bool CureEffect::CheckTarget(MagicTarget* apTarget) {
	Actor* pTargetActor;
	if (apTarget) {
		pTargetActor = apTarget->GetTargetAsActor();
	} else {
		pTargetActor = nullptr;
	}
	return pTargetActor != nullptr;
}

void CureEffect::Start() {
	MagicTarget* unk24;
	MagicTarget* v2;
	int32_t v3;
	int32_t v4;
	if (((uint32_t)eArchetypeID) == ((-1))) {
		v4 = (int32_t)eSpellType;
		unk24 = ActiveEffect::GetTarget();
		unk24->Dispel_4(v4);
	} else {
		v3 = eArchetypeID;
		v2 = ActiveEffect::GetTarget();
		v2->Dispel(static_cast<EffectArchetypes::ArchetypeID>(v3), nullptr);
	}
}

ActiveEffect* CureEffect::InstantiateCureDisease(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
	void* apMemory = ourMalloc(sizeof(CureEffect));
	if (apMemory) {
		return ::new (apMemory) CureEffect(apCaster, apSpell, apEffect, 1);
	} else {
		return nullptr;
	}
}

ActiveEffect* CureEffect::InstantiateCureParalysis(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
	void* apMemory = ourMalloc(sizeof(CureEffect));
	if (apMemory) {
		return ::new (apMemory) CureEffect(apCaster, apSpell, apEffect, 24);
	} else {
		return nullptr;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
ActiveEffect* CureEffect::InstantiateCureAddiction(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
	CureEffect* apMemory = static_cast<CureEffect*>(ourMalloc(sizeof(CureEffect)));
	if (apMemory) {
		return ::new (apMemory) CureEffect(apCaster, apSpell, apEffect, 10);
	} else {
		return 0;
	}
}
