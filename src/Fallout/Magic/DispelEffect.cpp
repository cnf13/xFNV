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
    DispelEffect* pCopy = new DispelEffect(pCaster, pSpell, pEffect);
    static_cast<ActiveEffect*>(this)->CopyData(pCopy);
    return pCopy;
}

void DispelEffect::CopyData(ActiveEffect* apDestination) {
    ActiveEffect::CopyData(apDestination);
}

bool DispelEffect::CheckTarget(MagicTarget* apTarget) {
    return dynamic_cast<Character*>(apTarget) != nullptr;
}

void DispelEffect::Start() {
    BSSimpleList<ActiveEffect*>* pList = pTarget->GetActiveEffectList();
    bool bSelfCast = pEffect->data.iRange == 0;
    Actor* pCasterActor = pCaster ? pCaster->GetCasterAsActor() : nullptr;
    while (pList && !pList->IsEmpty()) {
        ActiveEffect* pActiveEffect = pList->m_item;
        BSSimpleList<ActiveEffect*>* pNext = pList->m_pkNext;
        if (pActiveEffect && pActiveEffect != this) {
            MagicItem* pEffectSpell = pActiveEffect->pSpell;
            if (pEffectSpell->GetSpellType() == MagicSystem::SPELL) {
                double dCost = pEffectSpell->GetCost(bSelfCast ? pCasterActor : nullptr);
                double dMagnitude = fMagnitude;
                if (fMagicDispelMagnitudeMult.Float() * dMagnitude >= dCost) {
                    pActiveEffect->Dispel(false);
                }
            }
        }

        pList = pNext;
    }
}

ActiveEffect* DispelEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new DispelEffect(apCaster, apSpell, apEffect);
}
