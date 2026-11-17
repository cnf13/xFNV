#include "Fallout/Magic/DispelEffect.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Character.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/CureEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "SDK/MSVC/rtti.h"

// Whether the dispel archetype took DispelEffect::Instantiate. Stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(EffectArchetypes::DISPEL, &DispelEffect::Instantiate);

DispelEffect::DispelEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ActiveEffect(apCaster, apSpell, apEffect) {}

DispelEffect::~DispelEffect() {}

// Copies the effect: allocates a new one, constructs it from this effect's caster, spell and effect item, then copies
// the shared data into it. CopyData is called virtually with this effect as `this`.
ActiveEffect* DispelEffect::Copy() {
    DispelEffect* pCopy = static_cast<DispelEffect*>(ourMalloc(sizeof(DispelEffect)));
    if (pCopy) {
        pCopy = ::new (pCopy) DispelEffect(pCaster, pSpell, pEffect);
    }
    else {
        pCopy = nullptr;
    }
    static_cast<ActiveEffect*>(this)->CopyData(pCopy);
    return pCopy;
}

void DispelEffect::CopyData(ActiveEffect* apDestination) {
    ActiveEffect::CopyData(apDestination);
}

bool DispelEffect::CheckTarget(MagicTarget* apTarget) {
    return (dynamic_cast<Character*>(apTarget) != nullptr);
}

void DispelEffect::Start() {
    Actor* pcasterActor;
    MagicTarget* Target = pTarget;
    BSSimpleList<ActiveEffect*>* plist = Target->GetActiveEffectList();
    bool bselfCast = pEffect->data.iRange == 0;
    if (pCaster) {
        pcasterActor = pCaster->GetCasterAsActor();
    }
    else {
        pcasterActor = nullptr;
    }
    while (plist && !plist->IsEmpty()) {
        ActiveEffect* peffect = plist->m_item;
        BSSimpleList<ActiveEffect*>* pnext = plist->m_pkNext;
        if ((peffect) && (peffect != this)) {
            MagicItem* pspell = peffect->pSpell;
            if (pspell->GetSpellType() == MagicSystem::SPELL) {
                double v2 = pspell->GetCost(bselfCast ? pcasterActor : nullptr);
                double SequenceWeight = fMagnitude;
                if (fMagicDispelMagnitudeMult.Float() * SequenceWeight >= v2) {
                    peffect->Dispel(false);
                }
            }
        }

        plist = pnext;
    }
}

ActiveEffect* DispelEffect::Instantiate(MagicCaster* a1, MagicItem* a2, EffectItem* a3) {
    void* v5 = ourMalloc(sizeof(DispelEffect));
    if (v5) {
        return ::new (v5) DispelEffect(a1, a2, a3);
    }

    return nullptr;
}
