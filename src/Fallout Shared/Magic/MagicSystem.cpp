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
MagicSystem::CastingTypeNameTable MagicSystem::pCastingTypeNames{&sMagicCastOnce, &sMagicCastWhenUsed,
	&sMagicCastWhenStrikes, &sMagicCastConstant};
MagicSystem::RangeNameTable MagicSystem::pRangeNames{&sMagicRangeSelf, &sMagicRangeTouch, &sMagicRangeTarget};
MagicSystem::ProjectileTypeNameTable MagicSystem::pProjectileTypeNames{&sMagicProjectileTypeBall,
	&sMagicProjectileTypeBolt, &sMagicProjectileTypeSpray, &sMagicProjectileTypeFog};
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
	AutoMemContext apMemory(MEM_CONTEXT_TES_DEFAULT_OBJECTS, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Magic\\MagicSystem.cpp", 302);
	TESForm* FormByNumericID = TESForm::GetFormByNumericID(300u);
	MagicSystem::pAlterationFailureSound = dynamic_cast<TESSound*>(FormByNumericID);
	if (!MagicSystem::pAlterationFailureSound) {
		TESSound* v65;
		TESSound* v87 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v87) {
			v65 = ::new (v87) TESSound;
		} else {
			v65 = 0;
		}
		MagicSystem::pAlterationFailureSound = v65;
		v65->SetFormID(300u, 1);
		MagicSystem::pAlterationFailureSound->SetFormEditorID("MagicFailureSoundAlteration");
		MagicSystem::pAlterationFailureSound->SetAltered(0);
		BSSimpleList<TESSound*>* SoundList = pDataHandler->GetSoundList();
		SoundList->AddHead(MagicSystem::pAlterationFailureSound);
	}
	TESForm* v2 = TESForm::GetFormByNumericID(301u);
	MagicSystem::pConjurationFailureSound = dynamic_cast<TESSound*>(v2);
	if (!MagicSystem::pConjurationFailureSound) {
		TESSound* v64;
		TESSound* v86 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v86) {
			v64 = ::new (v86) TESSound;
		} else {
			v64 = 0;
		}
		MagicSystem::pConjurationFailureSound = v64;
		v64->SetFormID(301u, 1);
		MagicSystem::pConjurationFailureSound->SetFormEditorID("MagicFailureSoundConjuration");
		MagicSystem::pConjurationFailureSound->SetAltered(0);
		BSSimpleList<TESSound*>* v3 = pDataHandler->GetSoundList();
		v3->AddHead(MagicSystem::pConjurationFailureSound);
	}
	TESForm* v4 = TESForm::GetFormByNumericID(302u);
	MagicSystem::pDestructionFailureSound = dynamic_cast<TESSound*>(v4);
	if (!MagicSystem::pDestructionFailureSound) {
		TESSound* v63;
		TESSound* v85 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v85) {
			v63 = ::new (v85) TESSound;
		} else {
			v63 = 0;
		}
		MagicSystem::pDestructionFailureSound = v63;
		v63->SetFormID(302u, 1);
		MagicSystem::pDestructionFailureSound->SetFormEditorID("MagicFailureSoundDestruction");
		MagicSystem::pDestructionFailureSound->SetAltered(0);
		BSSimpleList<TESSound*>* v5 = pDataHandler->GetSoundList();
		v5->AddHead(MagicSystem::pDestructionFailureSound);
	}
	TESForm* v6 = TESForm::GetFormByNumericID(303u);
	MagicSystem::pIllusionFailureSound = dynamic_cast<TESSound*>(v6);
	if (!MagicSystem::pIllusionFailureSound) {
		TESSound* v62;
		TESSound* v84 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v84) {
			v62 = ::new (v84) TESSound;
		} else {
			v62 = 0;
		}
		MagicSystem::pIllusionFailureSound = v62;
		v62->SetFormID(303u, 1);
		MagicSystem::pIllusionFailureSound->SetFormEditorID("MagicFailureSoundIllusion");
		MagicSystem::pIllusionFailureSound->SetAltered(0);
		BSSimpleList<TESSound*>* v7 = pDataHandler->GetSoundList();
		v7->AddHead(MagicSystem::pIllusionFailureSound);
	}
	TESForm* v8 = TESForm::GetFormByNumericID(304u);
	MagicSystem::pMysticismFailureSound = dynamic_cast<TESSound*>(v8);
	if (!MagicSystem::pMysticismFailureSound) {
		TESSound* v61;
		TESSound* v83 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v83) {
			v61 = ::new (v83) TESSound;
		} else {
			v61 = 0;
		}
		MagicSystem::pMysticismFailureSound = v61;
		v61->SetFormID(304u, 1);
		MagicSystem::pMysticismFailureSound->SetFormEditorID("MagicFailureSoundMysticism");
		MagicSystem::pMysticismFailureSound->SetAltered(0);
		BSSimpleList<TESSound*>* v9 = pDataHandler->GetSoundList();
		v9->AddHead(MagicSystem::pMysticismFailureSound);
	}
	TESForm* v10 = TESForm::GetFormByNumericID(305u);
	MagicSystem::pRestorationFailureSound = dynamic_cast<TESSound*>(v10);
	if (!MagicSystem::pRestorationFailureSound) {
		TESSound* v60;
		TESSound* v82 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v82) {
			v60 = ::new (v82) TESSound;
		} else {
			v60 = 0;
		}
		MagicSystem::pRestorationFailureSound = v60;
		v60->SetFormID(305u, 1);
		MagicSystem::pRestorationFailureSound->SetFormEditorID("MagicFailureSoundRestoration");
		MagicSystem::pRestorationFailureSound->SetAltered(0);
		BSSimpleList<TESSound*>* v11 = pDataHandler->GetSoundList();
		v11->AddHead(MagicSystem::pRestorationFailureSound);
	}
	TESForm* v12 = TESForm::GetFormByNumericID(312u);
	MagicSystem::pAlterationEnchantDrawSound = dynamic_cast<TESSound*>(v12);
	if (!MagicSystem::pAlterationEnchantDrawSound) {
		TESSound* v59;
		TESSound* v81 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v81) {
			v59 = ::new (v81) TESSound;
		} else {
			v59 = 0;
		}
		MagicSystem::pAlterationEnchantDrawSound = v59;
		v59->SetFormID(312u, 1);
		MagicSystem::pAlterationEnchantDrawSound->SetFormEditorID("MagicEnchantDrawSoundAlteration");
		MagicSystem::pAlterationEnchantDrawSound->SetAltered(0);
		BSSimpleList<TESSound*>* v13 = pDataHandler->GetSoundList();
		v13->AddHead(MagicSystem::pAlterationEnchantDrawSound);
	}
	TESForm* v14 = TESForm::GetFormByNumericID(313u);
	MagicSystem::pConjurationEnchantDrawSound = dynamic_cast<TESSound*>(v14);
	if (!MagicSystem::pConjurationEnchantDrawSound) {
		TESSound* v58;
		TESSound* v80 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v80) {
			v58 = ::new (v80) TESSound;
		} else {
			v58 = 0;
		}
		MagicSystem::pConjurationEnchantDrawSound = v58;
		v58->SetFormID(313u, 1);
		MagicSystem::pConjurationEnchantDrawSound->SetFormEditorID("MagicEnchantDrawSoundConjuration");
		MagicSystem::pConjurationEnchantDrawSound->SetAltered(0);
		BSSimpleList<TESSound*>* v15 = pDataHandler->GetSoundList();
		v15->AddHead(MagicSystem::pConjurationEnchantDrawSound);
	}
	TESForm* v16 = TESForm::GetFormByNumericID(314u);
	MagicSystem::pDestructionEnchantDrawSound = dynamic_cast<TESSound*>(v16);
	if (!MagicSystem::pDestructionEnchantDrawSound) {
		TESSound* v57;
		TESSound* v79 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v79) {
			v57 = ::new (v79) TESSound;
		} else {
			v57 = 0;
		}
		MagicSystem::pDestructionEnchantDrawSound = v57;
		v57->SetFormID(314u, 1);
		MagicSystem::pDestructionEnchantDrawSound->SetFormEditorID("MagicEnchantDrawSoundDestruction");
		MagicSystem::pDestructionEnchantDrawSound->SetAltered(0);
		BSSimpleList<TESSound*>* v17 = pDataHandler->GetSoundList();
		v17->AddHead(MagicSystem::pDestructionEnchantDrawSound);
	}
	TESForm* v18 = TESForm::GetFormByNumericID(315u);
	MagicSystem::pIllusionEnchantDrawSound = dynamic_cast<TESSound*>(v18);
	if (!MagicSystem::pIllusionEnchantDrawSound) {
		TESSound* v56;
		TESSound* v78 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v78) {
			v56 = ::new (v78) TESSound;
		} else {
			v56 = 0;
		}
		MagicSystem::pIllusionEnchantDrawSound = v56;
		v56->SetFormID(315u, 1);
		MagicSystem::pIllusionEnchantDrawSound->SetFormEditorID("MagicEnchantDrawSoundIllusion");
		MagicSystem::pIllusionEnchantDrawSound->SetAltered(0);
		BSSimpleList<TESSound*>* v19 = pDataHandler->GetSoundList();
		v19->AddHead(MagicSystem::pIllusionEnchantDrawSound);
	}
	TESForm* v20 = TESForm::GetFormByNumericID(316u);
	MagicSystem::pMysticismEnchantDrawSound = dynamic_cast<TESSound*>(v20);
	if (!MagicSystem::pMysticismEnchantDrawSound) {
		TESSound* v55;
		TESSound* v77 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v77) {
			v55 = ::new (v77) TESSound;
		} else {
			v55 = 0;
		}
		MagicSystem::pMysticismEnchantDrawSound = v55;
		v55->SetFormID(316u, 1);
		MagicSystem::pMysticismEnchantDrawSound->SetFormEditorID("MagicEnchantDrawSoundMysticism");
		MagicSystem::pMysticismEnchantDrawSound->SetAltered(0);
		BSSimpleList<TESSound*>* v21 = pDataHandler->GetSoundList();
		v21->AddHead(MagicSystem::pMysticismEnchantDrawSound);
	}
	TESForm* v22 = TESForm::GetFormByNumericID(317u);
	MagicSystem::pRestorationEnchantDrawSound = dynamic_cast<TESSound*>(v22);
	if (!MagicSystem::pRestorationEnchantDrawSound) {
		TESSound* v54;
		TESSound* v76 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v76) {
			v54 = ::new (v76) TESSound;
		} else {
			v54 = 0;
		}
		MagicSystem::pRestorationEnchantDrawSound = v54;
		v54->SetFormID(317u, 1);
		MagicSystem::pRestorationEnchantDrawSound->SetFormEditorID("MagicEnchantDrawSoundRestoration");
		MagicSystem::pRestorationEnchantDrawSound->SetAltered(0);
		BSSimpleList<TESSound*>* v23 = pDataHandler->GetSoundList();
		v23->AddHead(MagicSystem::pRestorationEnchantDrawSound);
	}
	TESForm* v24 = TESForm::GetFormByNumericID(318u);
	MagicSystem::pAlterationEnchantHitSound = dynamic_cast<TESSound*>(v24);
	if (!MagicSystem::pAlterationEnchantHitSound) {
		TESSound* v53;
		TESSound* v75 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v75) {
			v53 = ::new (v75) TESSound;
		} else {
			v53 = 0;
		}
		MagicSystem::pAlterationEnchantHitSound = v53;
		v53->SetFormID(318u, 1);
		MagicSystem::pAlterationEnchantHitSound->SetFormEditorID("MagicEnchantHitSoundAlteration");
		MagicSystem::pAlterationEnchantHitSound->SetAltered(0);
		BSSimpleList<TESSound*>* v25 = pDataHandler->GetSoundList();
		v25->AddHead(MagicSystem::pAlterationEnchantHitSound);
	}
	TESForm* v26 = TESForm::GetFormByNumericID(319u);
	MagicSystem::pConjurationEnchantHitSound = dynamic_cast<TESSound*>(v26);
	if (!MagicSystem::pConjurationEnchantHitSound) {
		TESSound* v52;
		TESSound* v74 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v74) {
			v52 = ::new (v74) TESSound;
		} else {
			v52 = 0;
		}
		MagicSystem::pConjurationEnchantHitSound = v52;
		v52->SetFormID(319u, 1);
		MagicSystem::pConjurationEnchantHitSound->SetFormEditorID("MagicEnchantHitSoundConjuration");
		MagicSystem::pConjurationEnchantHitSound->SetAltered(0);
		BSSimpleList<TESSound*>* v27 = pDataHandler->GetSoundList();
		v27->AddHead(MagicSystem::pConjurationEnchantHitSound);
	}
	TESForm* v28 = TESForm::GetFormByNumericID(320u);
	MagicSystem::pDestructionEnchantHitSound = dynamic_cast<TESSound*>(v28);
	if (!MagicSystem::pDestructionEnchantHitSound) {
		TESSound* v51;
		TESSound* v73 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v73) {
			v51 = ::new (v73) TESSound;
		} else {
			v51 = 0;
		}
		MagicSystem::pDestructionEnchantHitSound = v51;
		v51->SetFormID(320u, 1);
		MagicSystem::pDestructionEnchantHitSound->SetFormEditorID("MagicEnchantHitSoundDestruction");
		MagicSystem::pDestructionEnchantHitSound->SetAltered(0);
		BSSimpleList<TESSound*>* v29 = pDataHandler->GetSoundList();
		v29->AddHead(MagicSystem::pDestructionEnchantHitSound);
	}
	TESForm* v30 = TESForm::GetFormByNumericID(321u);
	MagicSystem::pIllusionEnchantHitSound = dynamic_cast<TESSound*>(v30);
	if (!MagicSystem::pIllusionEnchantHitSound) {
		TESSound* v50;
		TESSound* v72 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v72) {
			v50 = ::new (v72) TESSound;
		} else {
			v50 = 0;
		}
		MagicSystem::pIllusionEnchantHitSound = v50;
		v50->SetFormID(321u, 1);
		MagicSystem::pIllusionEnchantHitSound->SetFormEditorID("MagicEnchantHitSoundIllusion");
		MagicSystem::pIllusionEnchantHitSound->SetAltered(0);
		BSSimpleList<TESSound*>* v31 = pDataHandler->GetSoundList();
		v31->AddHead(MagicSystem::pIllusionEnchantHitSound);
	}
	TESForm* v32 = TESForm::GetFormByNumericID(322u);
	MagicSystem::pMysticismEnchantHitSound = dynamic_cast<TESSound*>(v32);
	if (!MagicSystem::pMysticismEnchantHitSound) {
		TESSound* v49;
		TESSound* v71 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v71) {
			v49 = ::new (v71) TESSound;
		} else {
			v49 = 0;
		}
		MagicSystem::pMysticismEnchantHitSound = v49;
		v49->SetFormID(322u, 1);
		MagicSystem::pMysticismEnchantHitSound->SetFormEditorID("MagicEnchantHitSoundMysticism");
		MagicSystem::pMysticismEnchantHitSound->SetAltered(0);
		BSSimpleList<TESSound*>* v33 = pDataHandler->GetSoundList();
		v33->AddHead(MagicSystem::pMysticismEnchantHitSound);
	}
	TESForm* v34 = TESForm::GetFormByNumericID(323u);
	MagicSystem::pRestorationEnchantHitSound = dynamic_cast<TESSound*>(v34);
	if (!MagicSystem::pRestorationEnchantHitSound) {
		TESSound* v48;
		TESSound* v70 = static_cast<TESSound*>(ourMalloc(sizeof(TESSound)));
		if (v70) {
			v48 = ::new (v70) TESSound;
		} else {
			v48 = 0;
		}
		MagicSystem::pRestorationEnchantHitSound = v48;
		v48->SetFormID(323u, 1);
		MagicSystem::pRestorationEnchantHitSound->SetFormEditorID("MagicEnchantHitSoundRestoration");
		MagicSystem::pRestorationEnchantHitSound->SetAltered(0);
		BSSimpleList<TESSound*>* v35 = pDataHandler->GetSoundList();
		v35->AddHead(MagicSystem::pRestorationEnchantHitSound);
	}
	TESForm* v36 = TESForm::GetFormByNumericID(324u);
	MagicSystem::pAbsorbShaderEffect = dynamic_cast<TESEffectShader*>(v36);
	if (!MagicSystem::pAbsorbShaderEffect) {
		TESEffectShader* v47;
		TESEffectShader* v69 = static_cast<TESEffectShader*>(ourMalloc(sizeof(TESEffectShader)));
		if (v69) {
			v47 = ::new (v69) TESEffectShader;
		} else {
			v47 = 0;
		}
		MagicSystem::pAbsorbShaderEffect = v47;
		if (v47) {
			MagicSystem::pAbsorbShaderEffect->SetFormID(324u, 1);
			MagicSystem::pAbsorbShaderEffect->SetFormEditorID("effectAbsorb");
			MagicSystem::pAbsorbShaderEffect->SetAltered(0);
			BSSimpleList<TESEffectShader*>* EffectShaderList = pDataHandler->GetEffectShaderList();
			EffectShaderList->AddHead(MagicSystem::pAbsorbShaderEffect);
		}
	}
	TESForm* v38 = TESForm::GetFormByNumericID(325u);
	MagicSystem::pReflectShaderEffect = dynamic_cast<TESEffectShader*>(v38);
	if (!MagicSystem::pReflectShaderEffect) {
		TESEffectShader* v46;
		TESEffectShader* v68 = static_cast<TESEffectShader*>(ourMalloc(sizeof(TESEffectShader)));
		if (v68) {
			v46 = ::new (v68) TESEffectShader;
		} else {
			v46 = 0;
		}
		MagicSystem::pReflectShaderEffect = v46;
		if (v46) {
			MagicSystem::pReflectShaderEffect->SetFormID(325u, 1);
			MagicSystem::pReflectShaderEffect->SetFormEditorID("effectReflect");
			MagicSystem::pReflectShaderEffect->SetAltered(0);
			BSSimpleList<TESEffectShader*>* v39 = pDataHandler->GetEffectShaderList();
			v39->AddHead(MagicSystem::pReflectShaderEffect);
		}
	}
	TESForm* v40 = TESForm::GetFormByNumericID(326u);
	MagicSystem::pDetectLifeShaderEffect = dynamic_cast<TESEffectShader*>(v40);
	if (!MagicSystem::pDetectLifeShaderEffect) {
		TESEffectShader* v45;
		TESEffectShader* v67 = static_cast<TESEffectShader*>(ourMalloc(sizeof(TESEffectShader)));
		if (v67) {
			v45 = ::new (v67) TESEffectShader;
		} else {
			v45 = 0;
		}
		MagicSystem::pDetectLifeShaderEffect = v45;
		if (v45) {
			MagicSystem::pDetectLifeShaderEffect->SetFormID(326u, 1);
			MagicSystem::pDetectLifeShaderEffect->SetFormEditorID("LifeDetected");
			MagicSystem::pDetectLifeShaderEffect->SetAltered(0);
			BSSimpleList<TESEffectShader*>* v41 = pDataHandler->GetEffectShaderList();
			v41->AddHead(MagicSystem::pDetectLifeShaderEffect);
		}
	}
	TESForm* v42 = TESForm::GetFormByNumericID(327u);
	MagicSystem::pPipBoyLightSpell = dynamic_cast<SpellItem*>(v42);
	if (!MagicSystem::pPipBoyLightSpell) {
		SpellItem* v44;
		SpellItem* v66 = static_cast<SpellItem*>(ourMalloc(sizeof(SpellItem)));
		if (v66) {
			v44 = ::new (v66) SpellItem;
		} else {
			v44 = 0;
		}
		MagicSystem::pPipBoyLightSpell = v44;
		if (v44) {
			MagicSystem::pPipBoyLightSpell->SetFormID(327u, 1);
			MagicSystem::pPipBoyLightSpell->SetFormEditorID("spPipBoyLight");
			MagicSystem::pPipBoyLightSpell->SetAltered(0);
			TESDataHandler::FormList* Conditions = pDataHandler->GetSpellItemList();
			Conditions->AddHead(MagicSystem::pPipBoyLightSpell);
		}
	}
	EffectSettingCollection::CreateDefaultEffects();
}

bool MagicSystem::IsSpellTypePermanent(MagicSystem::SpellType aeType) {
	return (bool)((uint8_t)((((int32_t)aeType) == 1) || ((((int32_t)aeType) == 4) || (((int32_t)aeType) == 10))));
}

MagicSystem::FlareType MagicSystem::CheckEffectFlares(const EffectSetting* apSetting) {
	return FLARE_NONE;
}

MagicSystem::FlareType MagicSystem::CheckEffectTriggersFlare(const EffectSetting* apSetting) {
	return FLARE_NONE;
}

float MagicSystem::GetChameleonMinRefraction() {
	return (&fChameleonMinRefraction)->Float();
}

TESEffectShader* MagicSystem::GetDetectLifeShaderEffect() {
	return MagicSystem::pDetectLifeShaderEffect;
}

TESEffectShader* MagicSystem::GetAbsorbShaderEffect() {
	return MagicSystem::pAbsorbShaderEffect;
}

float MagicSystem::GetChameleonMaxRefraction() {
	return (&fChameleonMaxRefraction)->Float();
}

MagicSystem::School MagicSystem::MagicSkillToSchool(ActorValue::Index aeSkill) {
	return MagicSystem::SCHOOL_COUNT;
}

void MagicSystem::SetShowMagicStats(bool abShow) {
	bIsMagicStats = (bool)((int8_t)abShow);
}

bool MagicSystem::GetShowMagicStats() {
	return (bool)((uint8_t)bIsMagicStats);
}

const char* MagicSystem::GetMagicItemCastName(MagicSystem::CastingType aiIndex) {
	return MagicSystem::pCastingTypeNames[(int32_t)aiIndex]->String();
}

TESSound* MagicSystem::GetMagicFailureSound(MagicSystem::School aeSchool) {
	TESSound* result;
	switch (aeSchool) {
	case MagicSystem::ALTERATION: {
		result = MagicSystem::pAlterationFailureSound;
		break;
	}
	case MagicSystem::CONJURATION: {
		result = MagicSystem::pConjurationFailureSound;
		break;
	}
	case MagicSystem::DESTRUCTION: {
		result = MagicSystem::pDestructionFailureSound;
		break;
	}
	case MagicSystem::ILLUSION: {
		result = MagicSystem::pIllusionFailureSound;
		break;
	}
	case MagicSystem::MYSTICISM: {
		result = MagicSystem::pMysticismFailureSound;
		break;
	}
	case MagicSystem::RESTORATION: {
		result = MagicSystem::pRestorationFailureSound;
		break;
	}
	default: {
		result = 0;
		break;
	}
	}
	return result;
}
