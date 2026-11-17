#include "Fallout/Magic/VampirismEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Character.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"

// Not named in the PDB or IDA; name chosen here. Whether the vampirism archetype took Instantiate: the exe stores
// RegisterActiveEffect's whole return byte.
static char bRegistered =
	EffectArchetypes::RegisterActiveEffect(EffectArchetypes::VAMPIRISM, &VampirismEffect::Instantiate);

VampirismEffect::VampirismEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
	: ActiveEffect(apCaster, apSpell, apEffect) {}

VampirismEffect::~VampirismEffect() {}

// Allocates a copy, constructs it from this effect's caster, spell and effect item, then copies the effect data.
// The exe's cleanup-only EH frame is left out.
ActiveEffect* VampirismEffect::Copy() {
	void* pMemory = ourMalloc(sizeof(VampirismEffect));
	VampirismEffect* pCopy;
	if (pMemory) {
		pCopy = ::new (pMemory) VampirismEffect(pCaster, pSpell, pEffect);
	} else {
		pCopy = nullptr;
	}
	CopyData(pCopy);
	return pCopy;
}

void VampirismEffect::InitLoadGame(Actor* apActor) {
	ActiveEffect::InitLoadGame(apActor);
	dynamic_cast<Character*>(apActor);
}

void VampirismEffect::Revert(Actor* apActor) {
	ActiveEffect::Revert(apActor);
}

void VampirismEffect::CopyData(ActiveEffect* apDestination) {
	ActiveEffect::CopyData(apDestination);
}

// Only characters can be drained.
bool VampirismEffect::CheckTarget(MagicTarget* apTarget) {
	return dynamic_cast<Character*>(apTarget) != nullptr;
}

// The exe's cleanup-only C++ EH frame is left out.
ActiveEffect* VampirismEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
	VampirismEffect* v5 = static_cast<VampirismEffect*>(ourMalloc(sizeof(VampirismEffect)));
	if (v5) {
		return ::new (v5) VampirismEffect(apCaster, apSpell, apEffect);
	} else {
		return 0;
	}
}
