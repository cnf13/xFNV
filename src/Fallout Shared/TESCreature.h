#pragma once

#include "Fallout Shared/BGSImpactDataSet.h"
#include "Fallout Shared/CreatureSounds.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESAttackDamageForm.h"
#include "Fallout Shared/TESModelList.h"

class BGSBodyPartData;
class BGSListForm;
class TESCombatStyle;
class TESSound;

// How loud an actor's movement is.
enum SOUND_LEVEL : int32_t {
	SOUND_LEVEL_LOUD = 0,
	SOUND_LEVEL_NORMAL = 1,
	SOUND_LEVEL_SILENT = 2,
	SOUND_LEVEL_COUNT = 3,
};
// A creature's type (CREATURE_DATA::eType).
enum ENUM_CREATURE_TYPE : int8_t {
	CREATURE_TYPE_ANIMAL = 0,
	CREATURE_TYPE_MUTATED_ANIMAL = 1,
	CREATURE_TYPE_MUTATED_INSECT = 2,
	CREATURE_TYPE_ABOMINATION = 3,
	CREATURE_TYPE_SUPERMUTANT = 4,
	CREATURE_TYPE_FERAL_GHOUL = 5,
	CREATURE_TYPE_ROBOT = 6,
	CREATURE_TYPE_GIANT = 7,
	CREATURE_TYPE_COUNT = 8,
};

// The DATA record of a creature: its type and skills.
struct CREATURE_DATA {
	char eType;
	uint8_t iCombat;
	uint8_t iOther;
	uint8_t iStealth;
};

// A creature's base form (CREA).
class TESCreature : public TESActorBase, public TESAttackDamageForm, public TESModelList {
public:
	using TESActorBase::InitDefaultWorn;

	TESCreature();
	~TESCreature() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	uint16_t GetSaveSize(uint32_t aiFlags) override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void SaveGame(uint32_t aiFlags) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	bool Activate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abIdFlag,
		TESBoundObject* apObjecttoGet, int32_t aicount) override;
	void UnClone3D(TESObjectREFR* apRequester) override;
	NiAVObject* Clone3D(TESObjectREFR* apRequester) override;
	BGSBodyPartData* GetBodyPartData() override;
	void SetBodyPartData(BGSBodyPartData* apData) override;
	TESCombatStyle* GetCombatStyle() const override { return pCombatStyle; }
	void SetCombatStyle(TESCombatStyle* apCombatStyle) override;
	void SetActorValue(ActorValue::Index aiWhich, int32_t aiValue) override;
	// TESActorBaseData
	void CopyFromTemplateForm(TESActorBase* apForceTemplate) override;
	bool GetNoLeftArm() const override;
	bool GetNoRightArm() const override;
	bool GetNoHead() const override;
	bool GetNoShadow() const override;
	bool GetNoBloodSpray() const override;
	void SetNoBloodSpray(bool abVal) override;
	bool GetNoBloodDecal() const override;
	void SetNoBloodDecal(bool abVal) override;
	BGSImpactDataSet::IMPACT_MATERIAL_TYPE GetBloodImpactMaterial() const override { return eBloodImpactMaterial; }
	void SetBloodImpactMaterial(BGSImpactDataSet::IMPACT_MATERIAL_TYPE aeMat) override;
	uint16_t GetFatigue() const override;
	// TESHealthForm
	uint32_t GetFormHealth() const override;
	// ActorValueOwner
	int32_t GetActorValue(ActorValue::Index aiWhich) override;
	// TESAttackDamageForm
	uint16_t GetAttackDamage() const override;
	using TESActorBase::Clone3D;
	using TESActorBase::Revert;
	using TESActorBase::SetActorValue;
	using TESForm::Save;

	// Whether the creature has a humanoid body: an abomination, super mutant, feral ghoul or giant.
	bool IsHumanoidCreature();
	bool DoesFly() const;
	void InitWorn(TESObjectREFR* apRef);
	uint8_t GetCombatSkill() const;
	// Equips the creature's default items on apRef. abArmor is unused.
	void InitDefaultWorn(TESObjectREFR* apRef, bool abArmor, bool abWeapon, bool abAllowChanges);
	// Whether a creature may wear apForm: a light, a form type in (furniture, ammo], or an
	// embedded weapon.
	bool CanWearItem(TESForm* apItem);
	// The list of weapons the creature may use, or null.
	BGSListForm* GetWeaponList();
	char GetCreatureType() const;
	void SetCombatSkill(uint8_t aiVal);
	// Same code as ShadowSceneNode::GetDisableLightUpdate.
	uint8_t GetHandReach() const;
	// Same code as ShadowSceneNode::SetDisableLightUpdate.
	void SetHandReach(uint8_t acVal);
	float GetTurnSpeed() const;
	void SetTurnSpeed(float afVal);
	float GetFootWeight() const;
	void SetFootWeight(float afVal);
	float GetBaseScale() const;
	void SetBaseScale(float afVal);
	SOUND_LEVEL GetSoundLevel() const;
	// The sound level getter that Actor::MeleeAttack calls.
	SOUND_LEVEL GetSoundLevelCopy();
	BGSImpactDataSet* GetImpactDataSet() const;
	void SetImpactDataSet(BGSImpactDataSet* apSet);
	// Same code as MiddleHighProcess::SetCommandingActor.
	void SetWeaponList(BGSListForm* apSet);
	// The sound creature whose sounds this one uses; null when it has its own sounds.
	TESCreature* GetSoundCreature();
	void SetSoundCreature(TESCreature* apCreature);
	// The creature's own sounds; null when it uses a sound creature's.
	CreatureSounds* GetCreatureSounds();
	CreatureSounds* CreateCreatureSounds();
	TESSound* PickCreatureSound(CREATURE_SOUND_TYPE aeType);
	// Drops container weapons whose embedded node ap3D lacks; done once per creature.
	void CheckEmbeddedWeaponNodes(NiAVObject* ap3D);
	bool GetTiltFrontBack();
	bool GetTiltLeftRight();
	void SetCreatureType(char aeType);

protected:
	CREATURE_DATA* GetData();

public:
	union {
		TESCreature* pSoundCreature;
		CreatureSounds* pCreatureSounds;
	};

	CREATURE_DATA Data;
	uint8_t cHandReach;
	uint8_t padding131[3];
	float fTurnSpeed;
	float fFootWeight;
	float fBaseScale;
	TESCombatStyle* pCombatStyle;
	BGSBodyPartData* pBodyPartData;
	BGSImpactDataSet::IMPACT_MATERIAL_TYPE eBloodImpactMaterial;
	BGSImpactDataSet* pImpactDataSet;
	bool bEmbeddedWeaponChecked;
	uint8_t padding151[3];
	SOUND_LEVEL eSoundLevel;
	BGSListForm* pWeaponList;
	char cCanUseHandToHand;
	uint8_t padding15D[3];
};
