#include "Fallout/AI/Actor_Magic.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSDefaultObjectManager.h"
#include "Fallout Shared/BGSDehydration.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSHunger.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/BGSRadiation.h"
#include "Fallout Shared/BGSSleepDeprevation.h"
#include "Fallout Shared/BGSTextureUseMap.h"
#include "Fallout Shared/BGSTouchSpellForm.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/EnchantmentItem.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicItemForm.h"
#include "Fallout Shared/Magic/MagicItemObject.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESEffectShader.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectBOOK.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESSpellList.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicLight.h"
#include "Fallout/Magic/MagicProjectile.h"
#include "Fallout/Magic/MagicShaderHitEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDefaultAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/rtti.h"

#include <cstdint>
#include <limits>

WornBodyParts_t WornBodyParts{BO_HEAD, BO_HAIR, BO_UPPER_BODY, BO_LEFT_HAND, BO_RIGHT_HAND};

TESEffectShader* Get_EffectReflect() {
    return MagicSystem::pReflectShaderEffect;
}

// Hands the ability to the process; reached through the MagicCaster vtable (this = subobject at +0x88).
void Actor::CastAbility(SpellItem* apSpell, bool abLoadCast) {
    if (GetCurrentAIProcess()) {
        GetCurrentAIProcess()->CastAbility(static_cast<MagicCaster*>(this), apSpell, abLoadCast);
    }
}

// Hands the disease to the process; reached through the MagicCaster vtable (this = subobject at +0x88).
void Actor::TransferDisease(SpellItem* apSpell, MagicTarget* apDesiredTarget, bool abLoadCast) {
    if (GetCurrentAIProcess()) {
        GetCurrentAIProcess()->TransferDisease(static_cast<MagicCaster*>(this), apSpell, apDesiredTarget, abLoadCast);
    }
}

// CheckCast and GetCurrentSpell are MagicCaster virtuals, called through the MagicCaster subobject.
void Actor::StartCast() {
    MagicItem::CannotCastReason eReason;
    MagicCaster* pCaster = static_cast<MagicCaster*>(this);
    if (pCaster->CheckCast(pCaster->GetCurrentSpell(), nullptr, &eReason, false)) {
        SetShouldCheckMagicNode(true);
    }
    else {
        if (this == pPlayer) {
            MagicItem* pSpell = pCaster->GetCurrentSpell();
            ActorValue::Index eSkill = pSpell->GetAssociatedSkill();
            MagicSystem::School eSchool = MagicSystem::MagicSkillToSchool(eSkill);
            pPlayer->PlayMagicFailureSound(eSchool);
            Interface::ShowMessage(
                pCaster->GetCurrentSpell()->GetCannotCastString(eReason).pString,
                0,
                nullptr,
                nullptr,
                2.0f,
                false
            );
        }
    }
}

// The node spells are cast from: the "magic node" in the palette of the animation's controller manager. For a player
// whose third-person 3D is culled (first person) the first-person animation is used. Called through the MagicCaster
// subobject (this = +0x88).
NiNode* Actor::GetMagicNode() {
    if (!Get3D()) {
        return nullptr;
    }
    Animation* pAnimation = nullptr;
    if (this == pPlayer && pPlayer->Get3D(false) && pPlayer->Get3D(false)->GetAppCulled()) {
        pAnimation = pPlayer->GetAnimation(true);
    }
    else {
        pAnimation = GetAnimation();
    }
    NiControllerManager* pControllerManager = pAnimation ? pAnimation->GetControllerManager() : nullptr;
    NiDefaultAVObjectPalette* pObjectPalette = pControllerManager
        ? static_cast<NiDefaultAVObjectPalette*>(pControllerManager->GetObjectPalette())
        : nullptr;
    NiAVObject* pMagicNode = pObjectPalette ? pObjectPalette->GetAVObject(FixedStrings::QMagicNode()) : nullptr;
    return pMagicNode ? pMagicNode->IsNode() : nullptr;
}

MagicItem* Actor::GetCurrentSpell() {
    if (!GetCurrentAIProcess()) {
        return nullptr;
    }
    return GetCurrentAIProcess()->GetCurrentSpell();
}

float Actor::GetMagicEffectivenessModifier(bool abAdjust, float afModifier) {
    return MagicCaster::GetMagicEffectivenessModifier(abAdjust, afModifier);
}

void Actor::SetCurrentSpell(MagicItem* apCurrentSpell) {
    if (GetCurrentAIProcess()) {
        GetCurrentAIProcess()->SetCurrentSpell(apCurrentSpell);
    }
}

MagicTarget* Actor::GetDesiredTarget() {
    if (!pCurrentProcess) {
        return nullptr;
    }
    return pCurrentProcess->GetDesiredTarget();
}

void Actor::SetDesiredTarget(MagicTarget* apDesiredTarget) {
    if (GetCurrentAIProcess()) {
        GetCurrentAIProcess()->SetDesiredTarget(apDesiredTarget);
    }
}

// A ghost only takes abilities. Otherwise the target is added (never as an area target), and a hostile, non-permanent
// spell cast by another actor of a different base fires the OnHit block and makes this actor attacked by the caster,
// unless it is a potion or a weapon enchantment not delivered by ammo's area hit. When the player cast it, this
// actor becomes the HUD's enemy. Reached through the MagicTarget vtable (this = subobject at +0x94).
bool Actor::AddTarget(MagicCaster* apCaster, MagicItem* apSpell, ActiveEffect* apActiveEffect, bool abAreaTarget) {
    if (GetGhost() && apSpell->GetSpellType() != MagicSystem::ABILITY) {
        return false;
    }
    bool bAdded = MagicTarget::AddTarget(apCaster, apSpell, apActiveEffect, false);
    if (apSpell->IsHostile() && !MagicSystem::IsSpellTypePermanent(apSpell->GetSpellType())) {
        Actor* pattacker = dynamic_cast<Actor*>(apCaster);
        if (pattacker && pattacker->GetObjectReference() != GetObjectReference()) {
            // 0x80 is the OnHit action flag.
            Script::SetActionFlag(pattacker, GetExtra(), SCRIPT_ACTION_ONHIT);
            bool battack = true;
            if (apSpell->GetSpellType() == MagicSystem::ENCHANTMENT) {
                EnchantmentItem* penchantment = dynamic_cast<EnchantmentItem*>(apSpell);
                if (penchantment && penchantment->GetCastingType() == MagicSystem::WHEN_STRIKES) {
                    TESBoundObject* psource = apActiveEffect->GetEnchantmentSource();
                    if (!psource ||
                        psource->GetFormType() != AMMO_ID ||
                        (psource->GetFormType() == AMMO_ID && !abAreaTarget)) {
                        battack = false;
                    }
                }
            }
            if (apSpell->GetSpellType() == MagicSystem::POTION) {
                battack = false;
            }
            if (battack) {
                AttackedBy(pattacker, apActiveEffect);
            }
            if (pattacker == pPlayer) {
                Interface::SetEnemyActor(this);
            }
        }
    }
    return bAdded;
}

// The process keeps the list untyped; it holds the actor's active effects.
BSSimpleList<ActiveEffect*>* Actor::GetActiveEffectList() {
    if (!GetCurrentAIProcess()) {
        return nullptr;
    }
    return reinterpret_cast<BSSimpleList<ActiveEffect*>*>(GetCurrentAIProcess()->GetActiveEffectList());
}

// Refreshes the process's flares for the effect's flare type and plays the flare hit effect it triggers, and has a
// hostile effect that is not an ability, painless or from food make the actor show pain. Reached through the
// MagicTarget vtable (this = subobject at +0x94), which also carries the GetActiveEffectList calls.
void Actor::EffectAdded(ActiveEffect* apEffect) {
    if (!apEffect || !pCurrentProcess) {
        return;
    }
    MagicSystem::FlareType eflareType = MagicSystem::CheckEffectFlares(apEffect->GetEffectSetting());
    if (eflareType &&
        !pCurrentProcess->ShouldCheckFlare(static_cast<MagicTarget*>(this)->GetActiveEffectList(), eflareType)) {
        pCurrentProcess->SetRefreshFlareFlags();
    }
    eflareType = MagicSystem::CheckEffectTriggersFlare(apEffect->GetEffectSetting());
    if (eflareType &&
        pCurrentProcess->ShouldCheckFlare(static_cast<MagicTarget*>(this)->GetActiveEffectList(), eflareType) &&
        ReferenceProcessLists.FlareHitEffect(this, eflareType)) {
        apEffect->ForceNoHitEffect();
    }
    bool bfood = false;
    // Always true: tests "not a potion or not wortcraft", deliberately kept.
    if (apEffect->GetSpellType() != MagicSystem::POTION || apEffect->GetSpellType() != MagicSystem::WORTCRAFT) {
        IngredientItem* pingredient = dynamic_cast<IngredientItem*>(apEffect->GetSpell());
        if (pingredient) {
            bfood = pingredient->IsFood();
        }
        else {
            AlchemyItem* palchemy = dynamic_cast<AlchemyItem*>(apEffect->GetSpell());
            if (palchemy) {
                bfood = palchemy->IsFood();
            }
        }
    }
    if (apEffect->GetEffectItem()->IsHostile() &&
        apEffect->GetSpellType() != MagicSystem::ABILITY &&
        !apEffect->GetEffectSetting()->IsFlagSet(EffectSetting::PAINLESS) &&
        !bfood) {
        TriggerPain(true, true);
    }
}

void Actor::EffectRemoved(ActiveEffect* apEffect) {
    if (apEffect && pCurrentProcess) {
        apEffect->GetEffectSetting();
        if (NiAlphaProperty::GetType()) {
            pCurrentProcess->SetRefreshFlareFlags();
        }
    }
}

void Actor::EffectReflected(MagicCaster* apCaster, ActiveEffect* apEffect) {
    TESEffectShader* pabsorbEffect = MagicSystem::GetAbsorbShaderEffect();
    MagicShaderHitEffect* peffect = new MagicShaderHitEffect(this, pabsorbEffect, 0.0f);
    if (peffect->Init()) {
        ReferenceProcessLists.AddTempEffect(peffect);
    }
    else if (peffect) {
        delete peffect;
    }
}

void Actor::EffectAbsorbed(MagicCaster* apCaster, ActiveEffect* apEffect) {
    TESEffectShader* preflectEffect = Get_EffectReflect();
    MagicShaderHitEffect* peffect = new MagicShaderHitEffect(this, preflectEffect, 0.0f);
    if (peffect->Init()) {
        ReferenceProcessLists.AddTempEffect(peffect);
    }
    else if (peffect) {
        delete peffect;
    }
}

// A hostile effect, not from a worn enchantment, is resisted by the target's poison resistance (for poisons and alchemy
// that can be a poison) and by the effect's own resistance actor value, capped for the player. Alchemy that cannot be a
// poison resists only hostile effects, and food ingredients are never resisted.
float Actor::CheckResistance(MagicCaster* apCaster, MagicItem* apSpell, ActiveEffect* apActiveEffect) {
    if (!apSpell->IsHostile() || apActiveEffect->FromWornEnchantment()) {
        return 1.0f;
    }
    AlchemyItem* pAlchemy = nullptr;
    if (apSpell->GetSpellType() == MagicSystem::POTION) {
        pAlchemy = dynamic_cast<AlchemyItem*>(apSpell);
    }
    bool bPoison = pAlchemy && pAlchemy->CanBePoison();
    if (pAlchemy && !bPoison && !apActiveEffect->GetEffectItem()->IsHostile()) {
        return 1.0f;
    }
    if (apSpell->GetSpellType() == MagicSystem::WORTCRAFT) {
        IngredientItem* pIngredient = dynamic_cast<IngredientItem*>(apSpell);
        if (pIngredient && static_cast<EffectItemList*>(pIngredient)->IsFood()) {
            return 1.0f;
        }
    }
    Actor* pActor = this;
    float fMagicTypeResistance = 0.0f;
    if (bPoison || apSpell->GetSpellType() == MagicSystem::POISON) {
        fMagicTypeResistance = static_cast<ActorValueOwner*>(this)->GetActorFloatValue(ActorValue::POISON_RESISTANCE);
    }
    float fEffectResistance = 0.0f;
    if (pActor) {
        int32_t iResistance = apActiveEffect->GetEffectItem()->GetEffectSetting()->GetResistanceVariable();
        if (iResistance != -1 && fMagicTypeResistance == 0.0) {
            fEffectResistance = static_cast<ActorValueOwner*>(pActor)->GetActorFloatValue(
                static_cast<ActorValue::Index>(iResistance)
            );
        }
        if (IsPc() && fPlayerMaxResistance.Float() < fEffectResistance) {
            fEffectResistance = fPlayerMaxResistance.Float();
        }
    }
    return MagicFormulas::GetMagicTargetResistance(fMagicTypeResistance, fEffectResistance);
}

void Actor::SetRefraction(bool abEnable, float afRefractionPower) {
    float fRefractionPower = afRefractionPower;
    if (abEnable && afRefractionPower <= 0.0 && MobileObject::GetCurrentAIProcess()) {
        fRefractionPower = MobileObject::GetCurrentAIProcess()->GetScriptRefractionPower();
    }

    uint8_t ucEnable = abEnable;
    if (abEnable && fRefractionPower <= 0.0) {
        ucEnable = 0;
    }
    BSShaderPPLightingProperty::SetRefractionRecurse(Get3D(), ucEnable, fRefractionPower, 0, 0.0, 0);
    ExtraDataList* pExtra = TESObjectREFR::GetExtra();
    if (pExtra) {
        pExtra->SetRefractionPropertyExtra(ucEnable, afRefractionPower);
    }
}

// A scroll is used up as it casts: removes one from the actor, casts the enchantment's magic item and clears the
// player's selected scroll. The enchantment is fetched again from the book's TESEnchantableForm after the removal, and
// that result is cast.
void Actor::CastScroll(TESObjectBOOK* apScroll, MagicTarget* apTarget) {
    if (apScroll && apScroll->IsScroll() && apScroll->TESEnchantableForm::GetFormEnchanting()) {
        static_cast<TESObjectREFR*>(this)
            ->RemoveItem(apScroll, nullptr, 1, false, false, nullptr, nullptr, nullptr, true, false);
        EnchantmentItem* pEnchantment = apScroll->TESEnchantableForm::GetFormEnchanting();
        static_cast<MagicCaster*>(this)->CastSpell(static_cast<MagicItem*>(pEnchantment), apTarget, false);
        if (this == pPlayer) {
            pPlayer->SetSelectedScroll(nullptr);
        }
    }
}

bool Actor::CheckCastWhenStrikesEnchantment(
    ItemChange* apWeapon,
    Actor* apTarget,
    Projectile* apProjectile,
    bool& abInsufficientCharge
) {
    TESBoundObject* penchantmentSource;
    abInsufficientCharge = false;
    MagicItem* pTouchSpell = nullptr;
    EnchantmentItem* pEnchantment = nullptr;
    AlchemyItem* ppoison = nullptr;
    if (apProjectile && apProjectile->GetProjectileType() == Projectile::PROJECTILE_TYPE_GRENADE) {
        return false;
    }
    if (!apWeapon) {
        pTouchSpell = static_cast<MagicItem*>(GetActorBase()->pFormTouchSpell);
    }
    if (!pTouchSpell && !apWeapon && !apProjectile) {
        return false;
    }
    if (!pTouchSpell) {
        TESObjectWEAP* pWeapon = nullptr;
        if (apWeapon && apWeapon->pContainerObj && apWeapon->pContainerObj->GetFormType() == WEAP_ID) {
            pWeapon = static_cast<TESObjectWEAP*>(apWeapon->pContainerObj);
        }

        if (!pWeapon && apProjectile && apProjectile->pWeaponSource) {
            pWeapon = apProjectile->pWeaponSource;
        }

        if (apWeapon) {
            ppoison = apWeapon->GetPoison();
        }
        if (pWeapon) {
            pEnchantment = pWeapon->pFormEnchanting;
            if (!pEnchantment && apProjectile) {
                BGSExplosion* pExplosion;
                if (apProjectile->GetProjectileBase()) {
                    pExplosion = apProjectile->GetProjectileBase()->GetExplosion();
                }
                else {
                    pExplosion = nullptr;
                }
                if (pExplosion) {
                    pEnchantment = pExplosion->pFormEnchanting;
                }
            }
        }
    }
    if (!pTouchSpell && !pEnchantment && !ppoison) {
        return false;
    }
    if (pTouchSpell) {
        penchantmentSource = GetActorBase();
    }
    else if (apProjectile) {
        penchantmentSource = apProjectile->pWeaponSource;
    }
    else {
        penchantmentSource = apWeapon->pContainerObj;
    }
    if (pTouchSpell) {
        bool bCast = false;
        ANIM_GROUP_ENUM egroup = static_cast<ANIM_GROUP_ENUM>(GetActorBase()->iTouchSpellAnimation);
        if (egroup == ANIM_GROUP_NONE) {
            bCast = true;
        }
        else if (pCurrentProcess) {
            if (pCurrentProcess->GetAnimActionAnimSeq()) {
                if (pCurrentProcess->GetAnimActionAnimSeq()->GetAnimGroup()) {
                    bCast = pCurrentProcess->GetAnimActionAnimSeq()->GetAnimGroup()->GetType() == egroup;
                }
            }
        }
        if (bCast) {
            SetCurrentSpell(pTouchSpell);
            if (apTarget) {
                SetDesiredTarget(apTarget);
            }
            else {
                SetDesiredTarget(nullptr);
            }
            ReleaseCast(penchantmentSource);
        }
    }
    if (pEnchantment) {
        SetCurrentSpell(pEnchantment);
        if (apTarget) {
            SetDesiredTarget(apTarget);
        }
        else {
            SetDesiredTarget(nullptr);
        }
        ReleaseCast(penchantmentSource);
    }
    if (ppoison) {
        SetCurrentSpell(ppoison);
        if (apTarget) {
            SetDesiredTarget(apTarget);
        }
        else {
            SetDesiredTarget(nullptr);
        }
        ReleaseCast(penchantmentSource);
        if (apWeapon) {
            apWeapon->RemovePoison();
        }
    }
    return true;
}

bool Actor::RemoveSpell(SpellItem* apSpellIn) {
    bool bremoved = false;
    if (GetActorBase()->HasSpell(apSpellIn)) {
        GetActorBase()->RemoveSpell(apSpellIn);
        GetActorBase()->AddChange(CHANGE_ACTOR_BASE_SPELLLIST);
        bremoved = true;
    }
    if (MagicSystem::IsSpellTypePermanent(apSpellIn->GetSpellType())) {
        if (IsSpellTarget(static_cast<MagicItem*>(apSpellIn), true)) {
            Dispel(
                static_cast<MagicItem*>(apSpellIn),
                static_cast<MagicCaster*>(nullptr),
                static_cast<ActiveEffect*>(nullptr)
            );
            bremoved = true;
        }
    }
    if (apSpellIn->GetSpellType() == MagicSystem::POWER && IsInCastPowerList(apSpellIn)) {
        RemoveCastPower(apSpellIn);
        return true;
    }
    return bremoved;
}

void Actor::SetAlpha(float afNewAlpha) {
    if (pCurrentProcess && pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
        static_cast<HighProcess*>(pCurrentProcess)->SetMaxAlpha(afNewAlpha);
    }
}

float Actor::GetAlpha() {
    if (!pCurrentProcess || pCurrentProcess->GetProcessLevel()) {
        return 1.0;
    }

    return static_cast<HighProcess*>(pCurrentProcess)->QMaxAlpha();
}

bool Actor::GetShouldCheckMagicNode() {
    if (!GetCurrentAIProcess()) {
        return false;
    }
    return GetCurrentAIProcess()->GetShouldCheckMagicNode();
}

// AutoMemContext's constructor and destructor ignore the file, line and flag, and are called on this stack object. The
// 0.3 is the double 0.30000001192092896.
void Actor::UpdateMagic(bool abSleeping, bool abCompanionMenu) {
    if (IsStillLoading()) {
        return;
    }
    alignas(AutoMemContext) uint8_t acMemContext[sizeof(AutoMemContext)];
    AutoMemContext* pMemContext = ::new (acMemContext) AutoMemContext(
        MEM_CONTEXT_TES_ACTOR,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Actor_Magic.cpp",
        1253
    );
    float afDelta = 0.0f;
    float fSystemTimeClock = ReferenceProcessLists.GetSystemTimeClock();
    if (abSleeping) {
        afDelta = 3600.0 / appCalendar.GetTimeScale();
    }
    else if (abCompanionMenu) {
        afDelta = 0.001f;
    }
    else {
        afDelta = fSystemTimeClock - fLastUpdate;
        if (fSystemTimeClock < fLastUpdate || fLastUpdate < 0.0) {
            fLastUpdate = fSystemTimeClock;
            afDelta = 0.0f;
            if (fSystemTimeClock > 0.0 && fSystemTimeClock < 0.30000001192092896) {
                fLastUpdate = 0.0f;
                afDelta = fSystemTimeClock;
            }
        }
    }
    if (pCurrentProcess) {
        pCurrentProcess->UpdateRadiation(this, afDelta);
    }
    if (static_cast<MagicCaster*>(this)->GetCurrentSpell()) {
        bool bClearSpell = true;
        BSAnimGroupSequence* pSequence = GetCurrentAIProcess()->GetAnimActionAnimSeq();
        if (pSequence) {
            switch (pSequence->GetAnimGroup()->GetType()) {
                case ANIM_GROUP_ATTACK_LEFT:
                case ANIM_GROUP_ATTACK_LEFT_IS:
                case ANIM_GROUP_ATTACK_RIGHT:
                case ANIM_GROUP_ATTACK_RIGHT_IS:
                case ANIM_GROUP_ATTACK_3:
                case ANIM_GROUP_ATTACK_3_IS:
                case ANIM_GROUP_ATTACK_4:
                case ANIM_GROUP_ATTACK_4_IS:
                case ANIM_GROUP_ATTACK_5:
                case ANIM_GROUP_ATTACK_5_IS:
                case ANIM_GROUP_ATTACK_6:
                case ANIM_GROUP_ATTACK_6_IS:
                case ANIM_GROUP_ATTACK_7:
                case ANIM_GROUP_ATTACK_7_IS:
                case ANIM_GROUP_ATTACK_8:
                case ANIM_GROUP_ATTACK_8_IS:
                case ANIM_GROUP_ATTACK_LOOP:
                case ANIM_GROUP_ATTACK_LOOP_IS:
                case ANIM_GROUP_ATTACK_NORMAL_POWER: bClearSpell = false; break;
                default: break;
            }
        }
        if (bClearSpell) {
            SetCurrentSpell(nullptr);
        }
    }
    if (GetShouldCheckMagicNode()) {
        CheckMagicNode();
    }
    if (MagicCaster::pLight) {
        MagicCaster::UpdateLight(afDelta);
    }
    MagicTarget::UpdateTarget(afDelta);
    if (!IsDead(false)) {
        if (IsPc()) {
            RestoreActionPoints(afDelta);
        }
        if (pCurrentProcess && pCurrentProcess->GetCachedFlag(CachedValues::CVF_HEALTH_DAMAGED)) {
            RestoreHealth(afDelta);
        }
        if ((pCurrentProcess && pCurrentProcess->GetCachedFlag(CachedValues::CVF_FATIGUE_DAMAGED)) ||
            GetFatigue() < 0.0) {
            RestoreFatigue(afDelta);
        }
        UpdateWornEnchantments(afDelta);
    }
    fLastUpdate = fSystemTimeClock;
    pMemContext->~AutoMemContext();
}

void Actor::CheckMagicNode() {
    bool battach;
    MagicItem* pspell;
    Animation* panim;
    NiNode* pnode = GetMagicNode();
    if (pnode) {
        pspell = GetCurrentSpell();
        battach = false;
        panim = GetAnimation();
        if (panim) {
            if (pspell) {
                BSAnimGroupSequence* pseq = panim->GetCurrentSequence(AGS_LEFT_ARM);
                if (pseq) {
                    battach = pseq->GetState() == NiControllerSequence::kAnimating;
                }
            }
            else {
                BSAnimGroupSequence* pseq = panim->GetCurrentSequence(AGS_LEFT_ARM);
                if (pseq) {
                    battach = pseq->GetState() == NiControllerSequence::kAnimating;
                }
            }
        }
        if (pspell) {
            if (!pnode->GetChildCount() && battach) {
                AttachCastingArt(pspell);
            }
            return;
        }
        if (pnode->GetChildCount()) {
            if (!battach) {
                pnode->RemoveAllChildren();
            }
            for (uint32_t i = 0; i < pnode->GetArrayCount(); ++i) {
                NiControllerSequence* pseq;
                NiControllerManager* pmanager;
                NiAVObject* pav = pnode->GetAt(i);
                NiTimeController* po = pav ? pav->GetControllers() : nullptr;
                if (po) {
                    pmanager = static_cast<NiControllerManager*>(
                        NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, po)
                    );
                }
                else {
                    pmanager = nullptr;
                }
                if (pmanager) {
                    pseq = pmanager->GetSequenceByName(*get_g_SpecialIdle_Cast());
                }
                else {
                    pseq = nullptr;
                }
                if (pseq) {
                    double dLastTime = pseq->m_fLastTime;
                    if (pseq->m_fEndKeyTime < dLastTime) {
                        pnode->DetachChildAt(i);
                    }
                }
            }
        }
        if (pnode->GetChildCount()) {
            return;
        }
    }
    SetShouldCheckMagicNode(false);
}

// A refractive actor gets its refraction; any other its process's alpha times the fade (and, for the player, the
// camera's alpha).
void Actor::UpdateAlpha() {
    if (BSShaderManager::IsRefractionEnabled() && IsRefractive()) {
        SetRefraction(true, GetExtra()->GetRefractionPropertyExtra()->fRefractionPower);
        return;
    }
    float fFade = 1.0f;
    float fFadeAlpha = 1.0f;
    float fAlphaMult = 1.0f;
    if (pCurrentProcess && pCurrentProcess->GetProcessLevel() == PROCESS_TYPE_HIGH) {
        fAlphaMult = pCurrentProcess->GetAlphaMult();
        fFadeAlpha = static_cast<HighProcess*>(pCurrentProcess)->GetFadeAlpha();
        if (pCurrentProcess->GetFadeState() == HighProcess::FADE_OUT) {
            fFade = 1.0f;
        }
    }
    float fAlpha = 1.0f * fFade * fFadeAlpha * fAlphaMult;
    if (this == pPlayer) {
        fAlpha = fAlpha * fPlayerCameraAlpha;
    }
    SetAlpha(fAlpha);
}

CastPowerItem* Actor::GetCastPowerItem(SpellItem* apPower) {
    CastPowerItem* pitem = nullptr;
    if (apPower && CastPowers.m_item) {
        for (BSSimpleList<void*>* plist = &CastPowers; plist && !pitem; plist = plist->m_pkNext) {
            if (plist->m_item && *static_cast<SpellItem**>(plist->m_item) == apPower) {
                pitem = static_cast<CastPowerItem*>(plist->m_item);
            }
        }
    }

    return pitem;
}

void Actor::SetAlphaMult(float afAlpha) {
    if (pCurrentProcess) {
        pCurrentProcess->SetAlphaMult(afAlpha);
    }
}

// Without abWornEnchantmentsOnly, casts the abilities and diseases of the base's and the race's spell lists; with it,
// the enchantment of each worn item whose object no earlier slot already cast. Either way the every-actor ability is
// then cast unless the actor already has it.
void Actor::CastPermanentMagic(bool abWornEnchantmentsOnly) {
    if (!abWornEnchantmentsOnly) {
        for (BSSimpleList<SpellItem*>* pspells = GetActorBase()->GetSpellList(); pspells;
             pspells = pspells->GetNext()) {
            SpellItem* pspell = pspells->GetItem();
            if (pspell) {
                static_cast<MagicCaster*>(this)->CastAbility(pspell, true);
                static_cast<MagicCaster*>(this)->TransferDisease(pspell, static_cast<MagicTarget*>(this), true);
            }
        }
        for (BSSimpleList<SpellItem*>* pspells = GetRace() ? GetRace()->GetSpellList() : nullptr; pspells;
             pspells = pspells->GetNext()) {
            SpellItem* pspell = pspells->GetItem();
            if (pspell) {
                static_cast<MagicCaster*>(this)->CastAbility(pspell, true);
                static_cast<MagicCaster*>(this)->TransferDisease(pspell, static_cast<MagicTarget*>(this), true);
            }
        }
    }
    else {
        InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(this);
        TESBoundObject* pusedSources[BIPED_OBJECT_COUNT];
        for (TESBoundObject*& pSource : pusedSources) {
            pSource = nullptr;
        }
        for (int32_t i = BO_HEAD; i < BIPED_OBJECT_COUNT; ++i) {
            ItemChange* pitem = pinventory->GetWornItem(static_cast<BIPED_OBJECT>(i), false);
            TESEnchantableForm* penchantable = pitem ? dynamic_cast<TESEnchantableForm*>(pitem->GetContainerObject())
                                                     : nullptr;
            EnchantmentItem* penchantment = penchantable ? penchantable->GetFormEnchanting() : nullptr;
            if (penchantment) {
                bool bfound = false;
                for (int32_t j = BO_HEAD; j < i && !bfound; ++j) {
                    bfound = pusedSources[j] && pusedSources[j] == pitem->GetContainerObject();
                }
                if (!bfound && CanCastWornEnchantment(pitem)) {
                    static_cast<MagicCaster*>(this)
                        ->CastWornEnchantment(penchantment, pitem->GetContainerObject(), true);
                    pusedSources[i] = pitem->GetContainerObject();
                }
            }
            if (pitem) {
                delete pitem;
            }
        }
    }
    TESForm* peveryActorAbility = BGSDefaultObjectManager::GetDefaultObject(DO_EVERY_ACTOR_ABILITY);
    if (peveryActorAbility) {
        SpellItem* pspell = dynamic_cast<SpellItem*>(peveryActorAbility);
        if (pspell && !static_cast<MagicTarget*>(this)->IsSpellTarget(pspell, true)) {
            static_cast<MagicCaster*>(this)->CastAbility(pspell, true);
        }
    }
}

// Whether the worn item's enchantment may be cast: asks the overload with the item's object and its first instance's
// extra data.
bool Actor::CanCastWornEnchantment(ItemChange* apEnchantedItem) {
    ExtraDataList* pextra;
    if (apEnchantedItem->GetExtraDataList()) {
        pextra = apEnchantedItem->GetExtraDataList()->GetItem();
    }
    else {
        pextra = nullptr;
    }
    return CanCastWornEnchantment(apEnchantedItem->GetContainerObject(), pextra);
}

bool Actor::CanCastWornEnchantment(TESBoundObject* apObject, ExtraDataList* apExtra) {
    return true;
}

// Preloads the permanent spells (diseases, abilities, addictions) of the base's and the race's spell lists, the
// enchantments of the items worn in WornBodyParts, and the every-actor ability. The slot loop compares unsigned against
// the table's size.
void Actor::PreloadPermanentMagic() {
    for (BSSimpleList<SpellItem*>* pspells = GetActorBase()->GetSpellList(); pspells; pspells = pspells->GetNext()) {
        SpellItem* pspell = pspells->GetItem();
        if (pspell && MagicSystem::IsSpellTypePermanent(pspell->GetSpellType())) {
            pspell->Preload(false);
        }
    }
    for (BSSimpleList<SpellItem*>* pspells = GetRace() ? GetRace()->GetSpellList() : nullptr; pspells;
         pspells = pspells->GetNext()) {
        SpellItem* pspell = pspells->GetItem();
        if (pspell && MagicSystem::IsSpellTypePermanent(pspell->GetSpellType())) {
            pspell->Preload(false);
        }
    }
    InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(this);
    for (BIPED_OBJECT eBodyPart : WornBodyParts) {
        ItemChange* pitem = pinventory->GetWornItem(eBodyPart, false);
        TESEnchantableForm* penchantable = pitem ? dynamic_cast<TESEnchantableForm*>(pitem->GetContainerObject())
                                                 : nullptr;
        EnchantmentItem* penchantment = penchantable ? penchantable->GetFormEnchanting() : nullptr;
        if (penchantment) {
            penchantment->Preload(false);
        }
        if (pitem) {
            delete pitem;
        }
    }
    TESForm* peveryActorAbility = BGSDefaultObjectManager::GetDefaultObject(DO_EVERY_ACTOR_ABILITY);
    if (peveryActorAbility) {
        SpellItem* pspell = dynamic_cast<SpellItem*>(peveryActorAbility);
        if (pspell && MagicSystem::IsSpellTypePermanent(pspell->GetSpellType())) {
            pspell->Preload(false);
        }
    }
}

void Actor::UnloadPermanentMagic() {
    for (SpellItem* pspell : *&GetActorBase()->spellList) {
        if (pspell) {
            if (MagicSystem::IsSpellTypePermanent(pspell->GetSpellType())) {
                pspell->Unload(true);
            }
        }
    }
    BSSimpleList<SpellItem*>* pspells = GetRace() ? &GetRace()->spellList : nullptr;
    for (SpellItem* pspell : *pspells) {
        if (pspell) {
            if (MagicSystem::IsSpellTypePermanent(pspell->GetSpellType())) {
                pspell->Unload(true);
            }
        }
    }
    InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(this);
    for (BIPED_OBJECT eBodyPart : WornBodyParts) {
        TESEnchantableForm* penchantable;
        ItemChange* pitem = pinventory->GetWornItem(eBodyPart, false);
        if (pitem) {
            penchantable = dynamic_cast<TESEnchantableForm*>(pitem->pContainerObj);
        }
        else {
            penchantable = nullptr;
        }
        EnchantmentItem* penchantment = penchantable ? penchantable->pFormEnchanting : nullptr;
        if (penchantment) {
            penchantment->Unload(true);
        }
        if (pitem) {
            delete pitem;
        }
    }
    TESForm* peveryActorAbility = BGSDefaultObjectManager::GetDefaultObject(DO_EVERY_ACTOR_ABILITY);
    if (peveryActorAbility) {
        SpellItem* pspell = dynamic_cast<SpellItem*>(peveryActorAbility);
        if (pspell) {
            if (MagicSystem::IsSpellTypePermanent(pspell->GetSpellType())) {
                pspell->Unload(false);
            }
        }
    }
}

bool Actor::IsInCastPowerList(SpellItem* apPower) {
    return GetCastPowerItem(apPower) != nullptr;
}

void Actor::DeleteCastPowers() {
    for (BSSimpleList<void*>* plist = &CastPowers; plist; plist = plist->m_pkNext) {
        void* pitem = plist->m_item;
        if (pitem) {
            ourFree(pitem);
        }
    }
    CastPowers.RemoveAll();
}

void Actor::Eat(TESBoundObject* apfood, ExtraDataList* pextra, bool abNow) {
    if (apfood) {
        if (apfood->GetQuestObject()) {
            if (this == pPlayer) {
                Interface::ShowMessage(
                    sNoEatQuestItem.String(),
                    0,
                    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                    nullptr,
                    2.0f,
                    false
                );
            }
        }
        else {
            if (!pSaveLoadGameOLD->GetLoading()) {
                Script::SetActionFlag(this, pextra, SCRIPT_ACTION_ONEQUIP);
                Script::SetActionFlag(apfood, &m_Extra, SCRIPT_ACTION_ONEQUIP);
            }
            MagicItem* peatspell = nullptr;
            if (apfood->IsMagicItem()) {
                peatspell = static_cast<MagicItemObject*>(apfood);
            }
            if (this == pPlayer && !abNow) {
                if (pPlayer->AddEatDrinkItem(peatspell)) {
                    RemoveItem(apfood, pextra, 1, false, false, nullptr, nullptr, nullptr, true, false);
                }
            }
            else {
                SetCurrentSpell(peatspell);
                ReleaseCast(nullptr);
                if (this != pPlayer) {
                    RemoveItem(apfood, pextra, 1, false, false, nullptr, nullptr, nullptr, true, false);
                }
            }
        }
    }
}

void Actor::SetShouldCheckMagicNode(bool abCheck) {
    if (GetCurrentAIProcess()) {
        GetCurrentAIProcess()->SetShouldCheckMagicNode(abCheck);
    }
}

bool Actor::DrinkPotion(AlchemyItem* apPotion, ExtraDataList* pextra, bool abNow) {
    if (!apPotion) {
        return false;
    }
    bool bconsumed = true;
    if (this == pPlayer && !abNow) {
        bconsumed = pPlayer->AddEatDrinkItem(&static_cast<MagicItem&>(static_cast<MagicItemObject&>(*apPotion)));
        if (bconsumed) {
            RemoveItem(apPotion, pextra, 1, false, false, nullptr, nullptr, nullptr, true, false);
        }
    }
    else {
        SetCurrentSpell(&static_cast<MagicItem&>(static_cast<MagicItemObject&>(*apPotion)));
        ReleaseCast(nullptr);
        if (this != pPlayer) {
            RemoveItem(apPotion, pextra, 1, false, false, nullptr, nullptr, nullptr, true, false);
        }
    }
    return bconsumed;
}

void Actor::CastAlchemy(MagicItem* apPotionIngredient, bool abLast) {
    if (apPotionIngredient->GetSpellType() == MagicSystem::POTION ||
        apPotionIngredient->GetSpellType() == MagicSystem::WORTCRAFT) {
        TESBoundObject* pobject = dynamic_cast<TESBoundObject*>(apPotionIngredient);
        if (abLast) {
            if (apPotionIngredient->GetSpellType() == MagicSystem::POTION) {
                AlchemyItem* ppotion = dynamic_cast<AlchemyItem*>(apPotionIngredient);
                DrinkPotion(ppotion, nullptr, true);
            }
            else {
                Eat(pobject, nullptr, true);
            }
        }
        else {
            SetCurrentSpell(apPotionIngredient);
            ReleaseCast(nullptr);
            RemoveItem(pobject, nullptr, 1, false, false, nullptr, nullptr, nullptr, true, false);
        }
    }
}

// Casts the enchantment of an equipped armour or clothing item at once (EquipObject, unless the player's cast is
// queued). apExtra is unused.
void Actor::CheckEquipWornEnchantment(TESBoundObject* apObject, ExtraDataList* apExtra) {
    if (apObject->GetFormType() == ARMO_ID || apObject->GetFormType() == CLOT_ID) {
        TESEnchantableForm* penchantable = dynamic_cast<TESEnchantableForm*>(apObject);
        EnchantmentItem* penchantment = penchantable ? penchantable->pFormEnchanting : nullptr;
        if (penchantment) {
            static_cast<MagicCaster*>(this)->CastWornEnchantment(penchantment, apObject, false);
        }
    }
}

void Actor::AttachCastingArt(MagicItem* apSpell) {
    NiAVObject* pdata3D = nullptr;
    EffectSetting* pavEffect = nullptr;
    if (apSpell) {
        pavEffect = apSpell->GetAVEffect(MagicSystem::SELF);
        if (pavEffect && pavEffect->GetModelLength()) {
            pdata3D = MagicProjectile::Clone3D_0(pavEffect);
        }
    }
    NiNode* pnode = GetMagicNode();
    if (pnode && pdata3D) {
        NiControllerSequence* pseq;
        pnode->RemoveAllChildren();
        pnode->AttachChild(pdata3D, 1);
        if (pavEffect->data.pLight && !Main::GetSceneIsComplex()) {
            if (pLight) {
                MagicLight* pOldLight = pLight;
                if (pOldLight) {
                    delete pOldLight;
                }
            }
            pLight = nullptr;
            pLight = new MagicLight(pavEffect->data.pLight->iFormID, pnode);
        }

        NiControllerManager* pmanager = static_cast<NiControllerManager*>(
            NiObject::DynamicCastStatic(&NiControllerManager::ms_RTTI, pdata3D->GetControllers())
        );
        if (pmanager) {
            pseq = pmanager->GetSequenceByName(*get_g_SpecialIdle_Cast());
        }
        else {
            pseq = nullptr;
        }
        if (pseq) {
            pmanager->DeactivateAll(0.0f);
            pmanager->ActivateSequence(pseq, 0, false, 1.0f, 0.0f, nullptr);
            pmanager->SetActive(true);
            pseq->SetOffset(-std::numeric_limits<float>::max());
            if (pLight) {
                float fFadeTime = pseq->m_fEndKeyTime * 0.75;
                pLight->Fade(true, fFadeTime);
            }
        }
        if (pLight) {
            pLight->Update(0.0f);
        }
    }
}

float Actor::GetAlphaMult() {
    if (pCurrentProcess) {
        return pCurrentProcess->GetAlphaMult();
    }

    return 1.0;
}

void Actor::CheckDeletedCaster(MagicCaster* apMagicCaster) {
    if (apMagicCaster) {
        for (BSSimpleList<ActiveEffect*>* plist = GetActiveEffectList(); plist && plist->ItemsInList();
             plist = plist->m_pkNext) {
            if (plist->m_item) {
                plist->m_item->CheckDeletedCaster(apMagicCaster);
            }
        }
    }
}

void Actor::RemoveCastPower(SpellItem* apPower) {
    CastPowerItem* pitem = GetCastPowerItem(apPower);
    if (pitem) {
        CastPowers.Remove(*reinterpret_cast<void**>(&pitem));
        ourFree(pitem);
    }
}

// The stage of afOriginalValue loses its sickness and the stage of the new value casts its own; the player is told
// whether the rads rose (afDelta > 0; a NaN afDelta gets the decrease message) and which sickness applies. The update
// sound runs when afDelta is not 0 (NaN counts as a change).
void Actor::CheckRadiationLevelChange(
    ActorValueOwner* apActor,
    ActorValue::Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    char acMessage[260];
    Actor* pActor = apActor ? static_cast<Actor*>(apActor) : nullptr;
    if (!pActor || !pActor->IsPc()) {
        return;
    }
    float fNewValue = static_cast<ActorValueOwner*>(pActor)->GetActorFloatValue(aiWhich);
    if (pActor->IsPc() && afDelta != 0.0) {
        pPlayer->ReportRadiationChange();
    }
    BGSRadiationStage* pOldStage = BGSRadiationStage::FindStageForRadLevel(afOriginalValue);
    BGSRadiationStage* pNewStage = BGSRadiationStage::FindStageForRadLevel(fNewValue);
    if (pOldStage == pNewStage) {
        return;
    }
    if (pOldStage) {
        SpellItem* pOldSickness = pOldStage->GetSickness();
        static_cast<MagicTarget*>(pActor)->Dispel(static_cast<MagicItem*>(pOldSickness), nullptr, nullptr);
    }
    if (pNewStage) {
        static_cast<MagicCaster*>(pActor)->CastRadiation(pNewStage->GetSickness(), false);
    }
    if (pActor != pPlayer) {
        return;
    }
    if (afDelta > 0.0) {
        std::sprintf(acMessage, sRadiationIncrease.String());
        Interface::ShowMessage(
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds",
            nullptr,
            2.0f,
            false
        );
    }
    else {
        std::sprintf(acMessage, sRadiationDecrease.String());
        Interface::ShowMessage(
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds",
            nullptr,
            2.0f,
            false
        );
    }
    if (pNewStage) {
        // The sickness is converted to TESFullName without a null test.
        const char* pSicknessName = static_cast<TESFullName&>(*pNewStage->GetSickness()).GetFullName();
        std::sprintf(acMessage, "%s %s", sRadiationSick.String(), pSicknessName);
        Interface::ShowMessage(
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds",
            nullptr,
            2.0f,
            false
        );
    }
    else {
        std::sprintf(acMessage, sRadiationNotSick.String());
        Interface::ShowMessage(
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds",
            nullptr,
            2.0f,
            false
        );
    }
    static_cast<MagicTarget*>(pActor)->UpdateTarget(0.0f);
}

// Dehydration's form of CheckRadiationLevelChange: the stage of afOriginalValue loses its sickness, the stage of the
// new value casts its own, and the player is told whether the value rose (afDelta > 0; a NaN afDelta gets the decrease
// message) and which sickness applies.
void Actor::CheckDehydrationLevelChange(
    ActorValueOwner* apActor,
    ActorValue::Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    char acMessage[260];
    Actor* pActor = apActor ? static_cast<Actor*>(apActor) : nullptr;
    if (!pActor || !pActor->IsPc()) {
        return;
    }
    float fNewValue = static_cast<ActorValueOwner*>(pActor)->GetActorFloatValue(aiWhich);
    BGSDehydrationStage* poldStage = BGSDehydrationStage::FindStageForRadLevel(afOriginalValue);
    BGSDehydrationStage* pnewStage = BGSDehydrationStage::FindStageForRadLevel(fNewValue);
    if (poldStage == pnewStage) {
        return;
    }
    if (poldStage) {
        static_cast<MagicTarget*>(pActor)->Dispel(static_cast<MagicItem*>(poldStage->GetSickness()), nullptr, nullptr);
    }
    if (pnewStage) {
        static_cast<MagicCaster*>(pActor)->CastRadiation(pnewStage->GetSickness(), false);
    }
    if (pActor != pPlayer) {
        return;
    }
    if (afDelta > 0.0) {
        std::sprintf(acMessage, sDehydrationIncrease.String());
        Interface::ShowMessage(
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds",
            nullptr,
            2.0f,
            false
        );
    }
    else {
        std::sprintf(acMessage, sDehydrationDecrease.String());
        Interface::ShowMessage(
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds",
            nullptr,
            2.0f,
            false
        );
    }
    if (pnewStage) {
        // The sickness is converted to TESFullName without a null test.
        const char* pSicknessName = static_cast<TESFullName&>(*pnewStage->GetSickness()).GetFullName();
        std::sprintf(acMessage, "%s %s", sRadiationSick.String(), pSicknessName);
        Interface::ShowMessage(
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds",
            nullptr,
            2.0f,
            false
        );
    }
    else {
        std::sprintf(acMessage, sDehydrationNotSick.String());
        Interface::ShowMessage(
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds",
            nullptr,
            2.0f,
            false
        );
    }
    static_cast<MagicTarget*>(pActor)->UpdateTarget(0.0f);
}

// Hunger's form of CheckDehydrationLevelChange.
void Actor::CheckHungerLevelChange(
    ActorValueOwner* apActor,
    ActorValue::Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    char acMessage[260];
    Actor* pActor = apActor ? static_cast<Actor*>(apActor) : nullptr;
    if (!pActor || !pActor->IsPc()) {
        return;
    }
    float fNewValue = static_cast<ActorValueOwner*>(pActor)->GetActorFloatValue(aiWhich);
    BGSHungerStage* poldStage = BGSHungerStage::FindStageForRadLevel(afOriginalValue);
    BGSHungerStage* pnewStage = BGSHungerStage::FindStageForRadLevel(fNewValue);
    if (poldStage == pnewStage) {
        return;
    }
    if (poldStage) {
        static_cast<MagicTarget*>(pActor)->Dispel(static_cast<MagicItem*>(poldStage->GetSickness()), nullptr, nullptr);
    }
    if (pnewStage) {
        static_cast<MagicCaster*>(pActor)->CastRadiation(pnewStage->GetSickness(), false);
    }
    if (pActor != pPlayer) {
        return;
    }
    if (afDelta > 0.0) {
        std::sprintf(acMessage, sHungerIncrease.String());
        Interface::ShowMessage(
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds",
            nullptr,
            2.0f,
            false
        );
    }
    else {
        std::sprintf(acMessage, sHungerDecrease.String());
        Interface::ShowMessage(
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds",
            nullptr,
            2.0f,
            false
        );
    }
    if (pnewStage) {
        // The sickness is converted to TESFullName without a null test.
        const char* pSicknessName = static_cast<TESFullName&>(*pnewStage->GetSickness()).GetFullName();
        std::sprintf(acMessage, "%s %s", sRadiationSick.String(), pSicknessName);
        Interface::ShowMessage(
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds",
            nullptr,
            2.0f,
            false
        );
    }
    else {
        std::sprintf(acMessage, sHungerNotSick.String());
        Interface::ShowMessage(
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds",
            nullptr,
            2.0f,
            false
        );
    }
    static_cast<MagicTarget*>(pActor)->UpdateTarget(0.0f);
}

// Sleep deprivation's form of CheckDehydrationLevelChange.
void Actor::CheckSleepDeprevationLevelChange(
    ActorValueOwner* apActor,
    ActorValue::Index aiWhich,
    float afOriginalValue,
    float afDelta,
    ActorValueOwner* apActorSource
) {
    char acMessage[260];
    Actor* pActor = apActor ? static_cast<Actor*>(apActor) : nullptr;
    if (!pActor || !pActor->IsPc()) {
        return;
    }
    float fNewValue = static_cast<ActorValueOwner*>(pActor)->GetActorFloatValue(aiWhich);
    BGSSleepDeprevationStage* poldStage = BGSSleepDeprevationStage::FindStageForRadLevel(afOriginalValue);
    BGSSleepDeprevationStage* pnewStage = BGSSleepDeprevationStage::FindStageForRadLevel(fNewValue);
    if (poldStage == pnewStage) {
        return;
    }
    if (poldStage) {
        static_cast<MagicTarget*>(pActor)->Dispel(static_cast<MagicItem*>(poldStage->GetSickness()), nullptr, nullptr);
    }
    if (pnewStage) {
        static_cast<MagicCaster*>(pActor)->CastRadiation(pnewStage->GetSickness(), false);
    }
    if (pActor != pPlayer) {
        return;
    }
    if (afDelta > 0.0) {
        std::sprintf(acMessage, sSleepDeprevationIncrease.String());
        Interface::ShowMessage(
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds",
            nullptr,
            2.0f,
            false
        );
    }
    else {
        std::sprintf(acMessage, sSleepDeprevationDecrease.String());
        Interface::ShowMessage(
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds",
            nullptr,
            2.0f,
            false
        );
    }
    if (pnewStage) {
        // The sickness is converted to TESFullName without a null test.
        const char* pSicknessName = static_cast<TESFullName&>(*pnewStage->GetSickness()).GetFullName();
        std::sprintf(acMessage, "%s %s", sRadiationSick.String(), pSicknessName);
        Interface::ShowMessage(
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds",
            nullptr,
            2.0f,
            false
        );
    }
    else {
        std::sprintf(acMessage, sSleepDeprevationNotSick.String());
        Interface::ShowMessage(
            acMessage,
            0,
            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds",
            nullptr,
            2.0f,
            false
        );
    }
    static_cast<MagicTarget*>(pActor)->UpdateTarget(0.0f);
}
