#include "Fallout Shared/Magic/MagicSystem.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEffectShader.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESSound.h"
#include "SDK/MSVC/rtti.h"

// The game settings this file defines, with their IDs and default values, in initialization order.
static SettingT<GameSettingCollection> sMagicSchoolAlteration("sMagicSchoolAlteration", "Alteration");
static SettingT<GameSettingCollection> sMagicSchoolConjuration("sMagicSchoolConjuration", "Conjuration");
static SettingT<GameSettingCollection> sMagicSchoolDestruction("sMagicSchoolDestruction", "Destruction");
static SettingT<GameSettingCollection> sMagicSchoolIllusion("sMagicSchoolIllusion", "Illusion");
static SettingT<GameSettingCollection> sMagicSchoolMysticism("sMagicSchoolMysticism", "Mysticism");
static SettingT<GameSettingCollection> sMagicSchoolRestoration("sMagicSchoolRestoration", "Restoration");
static SettingT<GameSettingCollection> sMagicTypeSpell("sMagicTypeSpell", "Actor Effect");
static SettingT<GameSettingCollection> sMagicTypeAbility("sMagicTypeAbility", "Ability");
static SettingT<GameSettingCollection> sMagicTypeDisease("sMagicTypeDisease", "Disease");
static SettingT<GameSettingCollection> sMagicTypeLesserPower("sMagicTypeLesserPower", "Lesser Power");
static SettingT<GameSettingCollection> sMagicTypePower("sMagicTypePower", "Power");
static SettingT<GameSettingCollection> sMagicTypePoison("sMagicTypePoison", "Poison");
static SettingT<GameSettingCollection> sMagicTypeEnchantment("sMagicTypeEnchantment", "Object Effect");
static SettingT<GameSettingCollection> sMagicTypePotion("sMagicTypePotion", "Potion");
static SettingT<GameSettingCollection> sMagicTypeWortcraft("sMagicTypeWortcraft", "Wortcraft");
static SettingT<GameSettingCollection> sMagicTypeLevelled("sMagicTypeLevelled", "Leveled Actor Effect");
static SettingT<GameSettingCollection> sMagicTypeAddiction("sMagicTypeAddiction", "Addiction");
static SettingT<GameSettingCollection> sMagicCastOnce("sMagicCastOnce", "(unused)");
static SettingT<GameSettingCollection> sMagicCastWhenStrikes("sMagicCastWhenStrikes", "Weapon");
static SettingT<GameSettingCollection> sMagicCastWhenUsed("sMagicCastWhenUsed", "(unused)");
static SettingT<GameSettingCollection> sMagicCastConstant("sMagicCastConstant", "Apparel");
static SettingT<GameSettingCollection> sMagicRangeSelf("sMagicRangeSelf", "Self");
static SettingT<GameSettingCollection> sMagicRangeTouch("sMagicRangeTouch", "Touch");
static SettingT<GameSettingCollection> sMagicRangeTarget("sMagicRangeTarget", "Target");
static SettingT<GameSettingCollection> sMagicProjectileTypeBall("sMagicProjectileTypeBall", "Ball");
static SettingT<GameSettingCollection> sMagicProjectileTypeBolt("sMagicProjectileTypeBolt", "Bolt");
static SettingT<GameSettingCollection> sMagicProjectileTypeSpray("sMagicProjectileTypeSpray", "Spray");
static SettingT<GameSettingCollection> sMagicProjectileTypeFog("sMagicProjectileTypeFog", "Fog");

// The name tables point to the settings above, so they follow them.
MagicSystem::CastingTypeNameTable
    MagicSystem::pCastingTypeNames{&sMagicCastOnce, &sMagicCastWhenUsed, &sMagicCastWhenStrikes, &sMagicCastConstant};
MagicSystem::RangeNameTable MagicSystem::pRangeNames{&sMagicRangeSelf, &sMagicRangeTouch, &sMagicRangeTarget};
MagicSystem::ProjectileTypeNameTable MagicSystem::pProjectileTypeNames{
    &sMagicProjectileTypeBall,
    &sMagicProjectileTypeBolt,
    &sMagicProjectileTypeSpray,
    &sMagicProjectileTypeFog
};
static SettingT<GameSettingCollection> fMagicBallOptimalDistance("fMagicBallOptimalDistance", 1000.0f);
static SettingT<GameSettingCollection> fMagicBallMaximumDistance("fMagicBallMaximumDistance", 2000.0f);
static SettingT<GameSettingCollection> fMagicBoltOptimalDistance("fMagicBoltOptimalDistance", 1000.0f);
static SettingT<GameSettingCollection> fMagicBoltMaximumDistance("fMagicBoltMaximumDistance", 2000.0f);
static SettingT<GameSettingCollection> fMagicSprayOptimalDistance("fMagicSprayOptimalDistance", 500.0f);
static SettingT<GameSettingCollection> fMagicSprayMaximumDistance("fMagicSprayMaximumDistance", 1000.0f);
static SettingT<GameSettingCollection> fMagicFogOptimalDistance("fMagicFogOptimalDistance", 300.0f);
static SettingT<GameSettingCollection> fMagicFogMaximumDistance("fMagicFogMaximumDistance", 500.0f);
static SettingT<GameSettingCollection> iWortcraftMaxEffectsNovice("iWortcraftMaxEffectsNovice", 1);
static SettingT<GameSettingCollection> iWortcraftMaxEffectsApprentice("iWortcraftMaxEffectsApprentice", 2);
static SettingT<GameSettingCollection> iWortcraftMaxEffectsJourneyman("iWortcraftMaxEffectsJourneyman", 3);
static SettingT<GameSettingCollection> iWortcraftMaxEffectsExpert("iWortcraftMaxEffectsExpert", 4);
static SettingT<GameSettingCollection> iWortcraftMaxEffectsMaster("iWortcraftMaxEffectsMaster", 5);
SettingT<GameSettingCollection> fChameleonMaxRefraction("fChameleonMaxRefraction", 1.0f);
SettingT<GameSettingCollection> fChameleonMinRefraction("fChameleonMinRefraction", 0.01f);
SettingT<GameSettingCollection> fMagicDefaultCEEnchantFactor("fMagicDefaultCEEnchantFactor", 1.0f);
SettingT<GameSettingCollection> fMagicDefaultCEBarterFactor("fMagicDefaultCEBarterFactor", 1.0f);

SpellItem* MagicSystem::GetPipBoyLightSpell() {
    return MagicSystem::pPipBoyLightSpell;
}

void MagicSystem::GenerateDefaultObjects() {
    AutoMemContext kMemContext(
        MEM_CONTEXT_TES_DEFAULT_OBJECTS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Magic\\MagicSystem.cpp",
        302
    );
    MagicSystem::pAlterationFailureSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(300u));
    if (!MagicSystem::pAlterationFailureSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pAlterationFailureSound = pSound;
        pSound->SetFormID(300u, true);
        MagicSystem::pAlterationFailureSound->SetFormEditorID("MagicFailureSoundAlteration");
        MagicSystem::pAlterationFailureSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pAlterationFailureSound);
    }
    MagicSystem::pConjurationFailureSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(301u));
    if (!MagicSystem::pConjurationFailureSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pConjurationFailureSound = pSound;
        pSound->SetFormID(301u, true);
        MagicSystem::pConjurationFailureSound->SetFormEditorID("MagicFailureSoundConjuration");
        MagicSystem::pConjurationFailureSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pConjurationFailureSound);
    }
    MagicSystem::pDestructionFailureSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(302u));
    if (!MagicSystem::pDestructionFailureSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pDestructionFailureSound = pSound;
        pSound->SetFormID(302u, true);
        MagicSystem::pDestructionFailureSound->SetFormEditorID("MagicFailureSoundDestruction");
        MagicSystem::pDestructionFailureSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pDestructionFailureSound);
    }
    MagicSystem::pIllusionFailureSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(303u));
    if (!MagicSystem::pIllusionFailureSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pIllusionFailureSound = pSound;
        pSound->SetFormID(303u, true);
        MagicSystem::pIllusionFailureSound->SetFormEditorID("MagicFailureSoundIllusion");
        MagicSystem::pIllusionFailureSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pIllusionFailureSound);
    }
    MagicSystem::pMysticismFailureSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(304u));
    if (!MagicSystem::pMysticismFailureSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pMysticismFailureSound = pSound;
        pSound->SetFormID(304u, true);
        MagicSystem::pMysticismFailureSound->SetFormEditorID("MagicFailureSoundMysticism");
        MagicSystem::pMysticismFailureSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pMysticismFailureSound);
    }
    MagicSystem::pRestorationFailureSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(305u));
    if (!MagicSystem::pRestorationFailureSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pRestorationFailureSound = pSound;
        pSound->SetFormID(305u, true);
        MagicSystem::pRestorationFailureSound->SetFormEditorID("MagicFailureSoundRestoration");
        MagicSystem::pRestorationFailureSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pRestorationFailureSound);
    }
    MagicSystem::pAlterationEnchantDrawSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(312u));
    if (!MagicSystem::pAlterationEnchantDrawSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pAlterationEnchantDrawSound = pSound;
        pSound->SetFormID(312u, true);
        MagicSystem::pAlterationEnchantDrawSound->SetFormEditorID("MagicEnchantDrawSoundAlteration");
        MagicSystem::pAlterationEnchantDrawSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pAlterationEnchantDrawSound);
    }
    MagicSystem::pConjurationEnchantDrawSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(313u));
    if (!MagicSystem::pConjurationEnchantDrawSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pConjurationEnchantDrawSound = pSound;
        pSound->SetFormID(313u, true);
        MagicSystem::pConjurationEnchantDrawSound->SetFormEditorID("MagicEnchantDrawSoundConjuration");
        MagicSystem::pConjurationEnchantDrawSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pConjurationEnchantDrawSound);
    }
    MagicSystem::pDestructionEnchantDrawSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(314u));
    if (!MagicSystem::pDestructionEnchantDrawSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pDestructionEnchantDrawSound = pSound;
        pSound->SetFormID(314u, true);
        MagicSystem::pDestructionEnchantDrawSound->SetFormEditorID("MagicEnchantDrawSoundDestruction");
        MagicSystem::pDestructionEnchantDrawSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pDestructionEnchantDrawSound);
    }
    MagicSystem::pIllusionEnchantDrawSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(315u));
    if (!MagicSystem::pIllusionEnchantDrawSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pIllusionEnchantDrawSound = pSound;
        pSound->SetFormID(315u, true);
        MagicSystem::pIllusionEnchantDrawSound->SetFormEditorID("MagicEnchantDrawSoundIllusion");
        MagicSystem::pIllusionEnchantDrawSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pIllusionEnchantDrawSound);
    }
    MagicSystem::pMysticismEnchantDrawSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(316u));
    if (!MagicSystem::pMysticismEnchantDrawSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pMysticismEnchantDrawSound = pSound;
        pSound->SetFormID(316u, true);
        MagicSystem::pMysticismEnchantDrawSound->SetFormEditorID("MagicEnchantDrawSoundMysticism");
        MagicSystem::pMysticismEnchantDrawSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pMysticismEnchantDrawSound);
    }
    MagicSystem::pRestorationEnchantDrawSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(317u));
    if (!MagicSystem::pRestorationEnchantDrawSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pRestorationEnchantDrawSound = pSound;
        pSound->SetFormID(317u, true);
        MagicSystem::pRestorationEnchantDrawSound->SetFormEditorID("MagicEnchantDrawSoundRestoration");
        MagicSystem::pRestorationEnchantDrawSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pRestorationEnchantDrawSound);
    }
    MagicSystem::pAlterationEnchantHitSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(318u));
    if (!MagicSystem::pAlterationEnchantHitSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pAlterationEnchantHitSound = pSound;
        pSound->SetFormID(318u, true);
        MagicSystem::pAlterationEnchantHitSound->SetFormEditorID("MagicEnchantHitSoundAlteration");
        MagicSystem::pAlterationEnchantHitSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pAlterationEnchantHitSound);
    }
    MagicSystem::pConjurationEnchantHitSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(319u));
    if (!MagicSystem::pConjurationEnchantHitSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pConjurationEnchantHitSound = pSound;
        pSound->SetFormID(319u, true);
        MagicSystem::pConjurationEnchantHitSound->SetFormEditorID("MagicEnchantHitSoundConjuration");
        MagicSystem::pConjurationEnchantHitSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pConjurationEnchantHitSound);
    }
    MagicSystem::pDestructionEnchantHitSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(320u));
    if (!MagicSystem::pDestructionEnchantHitSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pDestructionEnchantHitSound = pSound;
        pSound->SetFormID(320u, true);
        MagicSystem::pDestructionEnchantHitSound->SetFormEditorID("MagicEnchantHitSoundDestruction");
        MagicSystem::pDestructionEnchantHitSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pDestructionEnchantHitSound);
    }
    MagicSystem::pIllusionEnchantHitSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(321u));
    if (!MagicSystem::pIllusionEnchantHitSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pIllusionEnchantHitSound = pSound;
        pSound->SetFormID(321u, true);
        MagicSystem::pIllusionEnchantHitSound->SetFormEditorID("MagicEnchantHitSoundIllusion");
        MagicSystem::pIllusionEnchantHitSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pIllusionEnchantHitSound);
    }
    MagicSystem::pMysticismEnchantHitSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(322u));
    if (!MagicSystem::pMysticismEnchantHitSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pMysticismEnchantHitSound = pSound;
        pSound->SetFormID(322u, true);
        MagicSystem::pMysticismEnchantHitSound->SetFormEditorID("MagicEnchantHitSoundMysticism");
        MagicSystem::pMysticismEnchantHitSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pMysticismEnchantHitSound);
    }
    MagicSystem::pRestorationEnchantHitSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(323u));
    if (!MagicSystem::pRestorationEnchantHitSound) {
        TESSound* pSound = new TESSound;
        MagicSystem::pRestorationEnchantHitSound = pSound;
        pSound->SetFormID(323u, true);
        MagicSystem::pRestorationEnchantHitSound->SetFormEditorID("MagicEnchantHitSoundRestoration");
        MagicSystem::pRestorationEnchantHitSound->SetAltered(false);
        pDataHandler->GetSoundList()->AddHead(MagicSystem::pRestorationEnchantHitSound);
    }
    MagicSystem::pAbsorbShaderEffect = dynamic_cast<TESEffectShader*>(TESForm::GetFormByNumericID(324u));
    if (!MagicSystem::pAbsorbShaderEffect) {
        TESEffectShader* pShader = new TESEffectShader;
        MagicSystem::pAbsorbShaderEffect = pShader;
        if (pShader) {
            MagicSystem::pAbsorbShaderEffect->SetFormID(324u, true);
            MagicSystem::pAbsorbShaderEffect->SetFormEditorID("effectAbsorb");
            MagicSystem::pAbsorbShaderEffect->SetAltered(false);
            pDataHandler->GetEffectShaderList()->AddHead(MagicSystem::pAbsorbShaderEffect);
        }
    }
    MagicSystem::pReflectShaderEffect = dynamic_cast<TESEffectShader*>(TESForm::GetFormByNumericID(325u));
    if (!MagicSystem::pReflectShaderEffect) {
        TESEffectShader* pShader = new TESEffectShader;
        MagicSystem::pReflectShaderEffect = pShader;
        if (pShader) {
            MagicSystem::pReflectShaderEffect->SetFormID(325u, true);
            MagicSystem::pReflectShaderEffect->SetFormEditorID("effectReflect");
            MagicSystem::pReflectShaderEffect->SetAltered(false);
            pDataHandler->GetEffectShaderList()->AddHead(MagicSystem::pReflectShaderEffect);
        }
    }
    MagicSystem::pDetectLifeShaderEffect = dynamic_cast<TESEffectShader*>(TESForm::GetFormByNumericID(326u));
    if (!MagicSystem::pDetectLifeShaderEffect) {
        TESEffectShader* pShader = new TESEffectShader;
        MagicSystem::pDetectLifeShaderEffect = pShader;
        if (pShader) {
            MagicSystem::pDetectLifeShaderEffect->SetFormID(326u, true);
            MagicSystem::pDetectLifeShaderEffect->SetFormEditorID("LifeDetected");
            MagicSystem::pDetectLifeShaderEffect->SetAltered(false);
            pDataHandler->GetEffectShaderList()->AddHead(MagicSystem::pDetectLifeShaderEffect);
        }
    }
    MagicSystem::pPipBoyLightSpell = dynamic_cast<SpellItem*>(TESForm::GetFormByNumericID(327u));
    if (!MagicSystem::pPipBoyLightSpell) {
        SpellItem* pSpell = new SpellItem;
        MagicSystem::pPipBoyLightSpell = pSpell;
        if (pSpell) {
            MagicSystem::pPipBoyLightSpell->SetFormID(327u, true);
            MagicSystem::pPipBoyLightSpell->SetFormEditorID("spPipBoyLight");
            MagicSystem::pPipBoyLightSpell->SetAltered(false);
            pDataHandler->GetSpellItemList()->AddHead(MagicSystem::pPipBoyLightSpell);
        }
    }
    EffectSettingCollection::CreateDefaultEffects();
}

bool MagicSystem::IsSpellTypePermanent(MagicSystem::SpellType aeType) {
    return aeType == DISEASE || aeType == ABILITY || aeType == ADDICTION;
}

MagicSystem::FlareType MagicSystem::CheckEffectFlares(const EffectSetting* apSetting) {
    return FLARE_NONE;
}

MagicSystem::FlareType MagicSystem::CheckEffectTriggersFlare(const EffectSetting* apSetting) {
    return FLARE_NONE;
}

float MagicSystem::GetChameleonMinRefraction() {
    return fChameleonMinRefraction.Float();
}

TESEffectShader* MagicSystem::GetDetectLifeShaderEffect() {
    return MagicSystem::pDetectLifeShaderEffect;
}

TESEffectShader* MagicSystem::GetAbsorbShaderEffect() {
    return MagicSystem::pAbsorbShaderEffect;
}

float MagicSystem::GetChameleonMaxRefraction() {
    return fChameleonMaxRefraction.Float();
}

MagicSystem::School MagicSystem::MagicSkillToSchool(ActorValue::Index aeSkill) {
    return MagicSystem::SCHOOL_COUNT;
}

void MagicSystem::SetShowMagicStats(bool abShow) {
    bIsMagicStats = abShow;
}

bool MagicSystem::GetShowMagicStats() {
    return bIsMagicStats;
}

const char* MagicSystem::GetMagicItemCastName(MagicSystem::CastingType aiIndex) {
    return MagicSystem::pCastingTypeNames[aiIndex]->String();
}

TESSound* MagicSystem::GetMagicFailureSound(MagicSystem::School aeSchool) {
    TESSound* pSound;
    switch (aeSchool) {
        case MagicSystem::ALTERATION: {
            pSound = MagicSystem::pAlterationFailureSound;
            break;
        }
        case MagicSystem::CONJURATION: {
            pSound = MagicSystem::pConjurationFailureSound;
            break;
        }
        case MagicSystem::DESTRUCTION: {
            pSound = MagicSystem::pDestructionFailureSound;
            break;
        }
        case MagicSystem::ILLUSION: {
            pSound = MagicSystem::pIllusionFailureSound;
            break;
        }
        case MagicSystem::MYSTICISM: {
            pSound = MagicSystem::pMysticismFailureSound;
            break;
        }
        case MagicSystem::RESTORATION: {
            pSound = MagicSystem::pRestorationFailureSound;
            break;
        }
        default: {
            pSound = nullptr;
            break;
        }
    }
    return pSound;
}
