#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Reconstruction.hpp"

class EffectSetting;
class SpellItem;
class TESEffectShader;
class TESSound;

// The magic system's static tables and default objects. Only what reconstructed code calls is declared.
class MagicSystem {
public:
	// The schools of magic. Values from the Xbox PDB.
	enum School : int32_t {
		ALTERATION = 0,
		CONJURATION = 1,
		DESTRUCTION = 2,
		ILLUSION = 3,
		MYSTICISM = 4,
		RESTORATION = 5,
		SCHOOL_COUNT = 6,
	};

	enum SpellType : int32_t {
		SPELL = 0,
		DISEASE = 1,
		POWER = 2,
		LESSER_POWER = 3,
		ABILITY = 4,
		POISON = 5,
		ENCHANTMENT = 6,
		POTION = 7,
		WORTCRAFT = 8,
		LEVELED_SPELL = 9,
		ADDICTION = 10,
		SPELL_TYPE_COUNT = 11,
	};

	enum CastingType : int32_t {
		CAST_ONCE = 0,
		WHEN_USED = 1,
		WHEN_STRIKES = 2,
		CONSTANT_EFFECT = 3,
		CASTING_COUNT = 4,
	};

	using CastingTypeNameTable = Setting* [CASTING_COUNT];

	enum FlareType : int32_t {
		FLARE_NONE = 0,
		FLARE_FIRE = 1,
		FLARE_FROST = 2,
		FLARE_SHOCK = 4,
		FLARE_WEAPON = 8,
	};

	enum ProjectileType : int32_t {
		BALL = 0,
		BOLT = 1,
		SPRAY = 2,
		FOG = 3,
		PROJECTILE_TYPE_COUNT = 4,
	};

	// The ranges of an effect. Values from the Xbox PDB.
	enum Range : int32_t {
		SELF = 0,
		TOUCH = 1,
		TARGET = 2,
		RANGE_COUNT = 3,
	};

	// The names of Range's and ProjectileType's values (PDB names), after pCastingTypeNames; no PC code reads them.
	using RangeNameTable = Setting* [RANGE_COUNT];
	using ProjectileTypeNameTable = Setting* [PROJECTILE_TYPE_COUNT];

	// The pip-boy light spell.
	static SpellItem* GetPipBoyLightSpell();
	// Creates the built-in magic forms; cdecl at 0x40CA50.
	static void GenerateDefaultObjects();
	// Whether spells of type aeType are permanent (disease, ability, addiction); cdecl at
	// 0x804B30.
	static bool IsSpellTypePermanent(SpellType aeType);
	// The flares the effect setting shows; the exe's stub always returns FLARE_NONE (cdecl,
	// 0x40DF90, folded with CheckEffectTriggersFlare).
	static FlareType CheckEffectFlares(const EffectSetting* apSetting);
	// The flare a hit by the effect setting triggers; the exe's stub always returns
	// FLARE_NONE (cdecl, 0x40DF90, folded with CheckEffectFlares).
	static FlareType CheckEffectTriggersFlare(const EffectSetting* apSetting);
	// Whether the flare flags hold the flare type (cdecl, 0x929390; defined in MiddleHighProcess.cpp, its PDB file).
	static bool CheckFlareFlag(FlareType aeFlareType, uint32_t aiFlareFlags);
	static float GetChameleonMinRefraction();
	static TESEffectShader* GetDetectLifeShaderEffect();
	static TESEffectShader* GetAbsorbShaderEffect();
	static float GetChameleonMaxRefraction();
	static MagicSystem::School MagicSkillToSchool(ActorValue::Index aeSkill);
	// Sets bShowMagicStats.
	static void SetShowMagicStats(bool abShow);
	// Reads bShowMagicStats.
	static bool GetShowMagicStats();
	static const char* GetMagicItemCastName(MagicSystem::CastingType aiIndex);
	static TESSound* GetMagicFailureSound(MagicSystem::School aeSchool);

	static inline TESSound* pAlterationFailureSound;
	static inline TESSound* pConjurationFailureSound;
	static inline TESSound* pDestructionFailureSound;
	static inline TESSound* pIllusionFailureSound;
	static inline TESSound* pMysticismFailureSound;
	static inline TESEffectShader* pReflectShaderEffect;
	static inline TESEffectShader* pDetectLifeShaderEffect;
	static inline TESEffectShader* pAbsorbShaderEffect;
	static inline SpellItem* pPipBoyLightSpell;
	static CastingTypeNameTable pCastingTypeNames;
	static RangeNameTable pRangeNames;
	static ProjectileTypeNameTable pProjectileTypeNames;
	static inline TESSound* pRestorationFailureSound;
	static inline TESSound* pAlterationEnchantDrawSound;
	static inline TESSound* pConjurationEnchantDrawSound;
	static inline TESSound* pDestructionEnchantDrawSound;
	static inline TESSound* pIllusionEnchantDrawSound;
	static inline TESSound* pMysticismEnchantDrawSound;
	static inline TESSound* pRestorationEnchantDrawSound;
	static inline TESSound* pAlterationEnchantHitSound;
	static inline TESSound* pConjurationEnchantHitSound;
	static inline TESSound* pDestructionEnchantHitSound;
	static inline TESSound* pIllusionEnchantHitSound;
	static inline TESSound* pMysticismEnchantHitSound;
	static inline TESSound* pRestorationEnchantHitSound;
};

// Game settings: the refraction range of chameleon effects.
extern SettingT<GameSettingCollection> fChameleonMaxRefraction;
extern SettingT<GameSettingCollection> fChameleonMinRefraction;

// The game settings MagicSystem.cpp defines (PDB), at their exe addresses.
extern SettingT<GameSettingCollection> fMagicDefaultCEEnchantFactor;
extern SettingT<GameSettingCollection> fMagicDefaultCEBarterFactor;
