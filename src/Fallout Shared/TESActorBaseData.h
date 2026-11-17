#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSImpactDataSet.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class BGSVoiceType;
class TESActorBase;
class TESFaction;
class TESLevItem;

// A faction an actor belongs to and the actor's rank in it.
struct FACTION_RANK {
	// Swaps the faction's form ID.
	void Endian();

	TESFaction* pFaction;
	uint8_t cRank;
	uint8_t padding05[3];
};

// The ACBS record of an actor base.
struct ACTOR_BASE_DATA {
	// Byte-swaps the record read from a big-endian file.
	void Endian();

	uint32_t iActorBaseFlags;
	uint16_t usFatigue;
	uint16_t usBartergold;
	int16_t sLevel;
	uint16_t usCalcLevelMin;
	uint16_t usCalcLevelMax;
	uint16_t usSpeedMult;
	float fKarma;
	uint16_t usBaseDisposition;
	uint16_t sTemplateUseFlags;
};

// The flags the player's crimes set on a faction (SetPCFactionFlags).
enum PC_FACTION_FLAG : int32_t {
	PC_FACTIONFLAG_NONE = -1,
	PC_FACTIONFLAG_MURDER = 0,
	PC_FACTIONFLAG_ATTACK = 1,
	PC_FACTIONFLAG_PLAYER_IS_ENEMY = 2,
};

// The stats, flags, template and factions of an NPC or creature.
class TESActorBaseData : public BaseFormComponent {
public:
	enum BaseFlags : uint32_t {
		// the sex of an NPC; whether a creature is biped
		BASE_FEMALE = 0x1,
		BASE_AUTO_CALC_STATS = 0x10,
		BASE_CREATURE_FLIES = 0x20,
		BASE_CREATURE_NO_RIGHT_ARM = 0x10000,
		BASE_CREATURE_NO_LEFT_ARM = 0x20000,
		// Also the NPC no-persuasion flag.
		BASE_CREATURE_NO_HEAD = 0x8000,
		BASE_CREATURE_WALKS = 0x40,
		BASE_ESSENTIAL = 0x2,
		BASE_RESPAWN = 0x8,
		BASE_PC_LEVEL_MULT = 0x80,
		BASE_NO_LOWLEVEL_PROCESSING = 0x200,
		BASE_CAN_BE_ALL_RACES = 0x400000,
		BASE_CREATURE_IMMOBILE = 0x800000,
		BASE_NOT_PUSHABLE = 0x8000000,
		BASE_CREATURE_INVULNERABLE = 0x80000000,
		// On a creature: it has its own sounds (CreatureSounds) instead of a sound
		// creature.
		BASE_USE_TEMPLATE = 0x100,
		// IDA names; not in the PDB. Creature flags sharing bits with the NPC ones.
		BASE_CREATURE_BIPED = 0x1,
		BASE_CREATURE_SWIMS = 0x10,
		BASE_CREATURE_NO_COMBAT_IN_WATER = 0x40000,
		BASE_CREATURE_CANT_OPEN_DOORS = 0x400000,
		// An NPC whose face is a character-generation preset.
		BASE_HAS_CHARGEN_FACE = 0x4,
		BASE_NO_BLOOD_SPRAY = 0x800,
		BASE_NO_BLOOD_DECAL = 0x1000,
	};

	// The template use flags: which parts of an actor base come from its template.
	enum TEMPLATE_USE_FLAG : int32_t {
		TU_TRAITS = 0,
		TU_STATS = 1,
		TU_FACTIONS = 2,
		TU_SPELLS = 3,
		TU_AIDATA = 4,
		TU_AIPACKAGES = 5,
		TU_MODEL = 6,
		TU_BASEDATA = 7,
		TU_INVENTORY = 8,
		TU_SCRIPT = 9,
		TU_COUNT = 10,
		TU_COPIED = 0xF,
	};

	enum ALIGNMENT : int32_t {
		ALIGNMENT_GOOD = 0,
		ALIGNMENT_NEUTRAL = 1,
		ALIGNMENT_EVIL = 2,
		ALIGNMENT_VERY_GOOD = 3,
		ALIGNMENT_VERY_EVIL = 4,
		ALIGNMENT_COUNT = 5,
	};

	// The title settings by alignment row (good, neutral, evil) and level.
	using KarmicTitleTable = Setting* [3][50];
	// The alignment names, indexed by ALIGNMENT (ALIGNMENT_COUNT entries).
	using AlignmentNameTable = const char* [5];

	TESActorBaseData();
	~TESActorBaseData();

	void InitializeDataComponent() override;
	void ClearDataComponent() override;
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;
	virtual void CopyFromTemplateForm(TESActorBase*) {}
	virtual bool GetNoVATSMelee() const { return GetFlag(0x100000); }
	virtual bool GetAllowPCDialogue() const { return GetFlag(0x200000); }
	virtual bool GetAllowPickPocket() const { return GetFlag(0x10000000); }
	virtual bool GetIsGhost() const { return GetFlag(0x20000000); }
	virtual bool GetInvulnerable() const;
	virtual bool GetCantOpenDoors() const;
	virtual bool GetCanBeAllRaces() const;
	virtual bool GetAutoCalcServiceFlags() const;
	virtual bool GetNoPersuasion() const { return true; }
	virtual bool GetNoLeftArm() const { return false; }
	virtual bool GetNoRightArm() const { return false; }
	virtual bool GetNoHead() const { return false; }
	virtual bool GetNoShadow() const { return false; }
	virtual bool GetNoBloodSpray() const { return false; }
	virtual void SetNoBloodSpray(bool) {}
	virtual bool GetNoBloodDecal() const { return false; }
	virtual void SetNoBloodDecal(bool) {}

	// the MATERIAL_TYPE of the blood impacts
	virtual BGSImpactDataSet::IMPACT_MATERIAL_TYPE GetBloodImpactMaterial() const {
		return BGSImpactDataSet::IMPACT_MATERIAL_TYPE(6);
	}

	virtual void SetBloodImpactMaterial(BGSImpactDataSet::IMPACT_MATERIAL_TYPE) {}
	virtual uint16_t GetFatigue() const;
	virtual float GetKarma() const { return actorData.fKarma; }
	// folded with TESObjectREFR::GetObjectReference at 0x7AF430
	virtual BGSVoiceType* GetVoiceType() const { return pVoiceType; }

	// Whether the actor is only in evil factions.
	bool IsInEvilFactionsOnly();
	// Resolves the forms loaded for apOwner.
	void InitItem(TESForm* apOwner);
	// The rank in apFaction, or -1 when not a member; abPC applies the player's faction
	// changes.
	int32_t GetFactionRank(TESFaction* apFaction, bool abPC);
	// The faction list (listFactions); the exe's shared field-address getter 0x5D8A70
	// (thiscall, ret 0).
	BSSimpleList<FACTION_RANK*>* GetFactionList();
	// Whether the actor base can be knocked down.
	bool GetKnockedDowns();
	// Frees every faction entry and empties the faction list.
	void ClearFactionList();
	// Sets the rank in apFaction, adding the faction when missing; rank -1 drops the faction's node.
	void SetFactionRank(TESFaction* apFaction, char acRank);
	bool IsInFaction(TESFaction* apFaction);
	uint16_t GetBarterGold() const;
	void SetBarterGold(uint16_t ausNewgold);
	// Clears change flags of the owning actor base.
	void RemoveFormChange(int32_t aiChangeFlags);
	bool GetFlag(uint32_t aiFlag) const;
	uint16_t GetCalcMin() const;
	uint16_t GetSpeedMultiplier() const;
	bool GetEssential() const;
	bool GetRespawn() const;
	bool GetMatchesPCLevel() const;
	bool GetNotPushable() const;
	bool GetTemplateUseFlag(TEMPLATE_USE_FLAG aeFlag) const;
	void SetTemplateUseFlag(TEMPLATE_USE_FLAG aeFlag, bool abVal);
	bool QCopiedTemplate() const;
	void SetCopiedTemplate(bool abVal);
	void SetTemplateUseFlags(uint16_t asVal);
	bool IsTemplateLeveled() const;
	bool GetUsesLeveledTemplate() const;
	// Every caller of the exe reads only al (movzx) and passes it on unnormalised: a bool, 0 or 1 in eax.
	bool HasLowLevelProcessing() const;
	// Same function as BaseProcess::GetActorPackageThatIsRunning (0x717E50): the address of the actor flags.
	int32_t* GetFlags();
	// The template actor base (thiscall, ret 0; the shared one-field getter 0x59BB30).
	TESForm* QTemplateForm() const;
	uint16_t GetLevel() const;
	void CalcTemplateForRef(TESObjectREFR* apRef);

	void AddFormChange(int32_t aiChangeFlags);
	void AddChangeFlags(int32_t aiChangeFlags);
	void SetFlags(int32_t aiNewFlags);
	void SetFlagBit(int32_t aiFlag, bool abVal, bool abAddChange);
	// The bytes the base data adds to a save.
	uint16_t GetSaveSize(uint32_t aiFlags);
	// Saves the base data when the save changed it (CHANGE_ACTOR_BASE_DATA).
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
	// Loads the base data when the save changed it (CHANGE_ACTOR_BASE_DATA), keeping the current sex flag.
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	// Saves the base data to the old-format save.
	void SaveGameTES(uint32_t aiFlags);
	// Loads the base data from the old-format save.
	void LoadGameTES(uint32_t aiFlags, uint32_t aiCurrentFlags);
	void SetMatchesPCLevel(bool abVal);
	void SetRunsInLow(bool abValue, bool abAddChange);
	void SetLevel(uint16_t asValue);
	void SetCalcLevelMin(uint16_t asVal);
	void SetCalcLevelMax(uint16_t asVal);
	void SetBaseDisposition(uint16_t asVal);
	void SetFatigue(uint16_t ausFatigue);
	void SetSpeedMult(uint16_t asSpeedMult);
	// The exe's shared one-field getters and setters: the level (0x47D370), the calc level
	// maximum (0x47D3B0), the base disposition (0x47D3D0), the death item (0x441110) and the voice type (set
	// 0x50F9C0); all thiscall.
	uint16_t GetLevelExact() const;
	uint16_t GetCalcLevelMax() const;
	uint16_t GetBaseDisposition() const;
	TESLevItem* GetDeathItem();
	void SetDeathItem(TESLevItem* apItem);
	void SetVoiceType(BGSVoiceType* apVoiceType);
	void SetKarma(float afKarma);
	// Replaces the faction list with a copy of apCopy's.
	void CopyFactionList(TESActorBaseData* apCopy);
	// The exe's shared one-field setter 0x7037C0 (thiscall, ret 4).
	void SetTemplateForm(TESForm* apTemplate);
	bool GetNoRumors();
	bool GetNoRotatingToHeadtrack();
	void RemoveChangeFlags(int32_t aiChangeFlags);
	void SetEssential(bool abVal);
	void Save();
	void SetPCFactionFlags(PC_FACTION_FLAG aeflag);

	// The alignment band a karma value falls in.
	static ALIGNMENT GetAlignmentForKarma(float afKarma);
	// The karmic title of apActor's level and alignment, or "".
	static const char* GetKarmicTitle(class Actor* apActor);
	// A new form of apOriginalBase's type, copied from it and then from the template apTemplateBase.
	static TESActorBase* CreateTemplateActorBase(TESActorBase* apOriginalBase, TESActorBase* apTemplateBase);
	// The first leveled template in apBase's template chain; aiOutCompositeTemplateUseFlags
	// receives the use flags combined along the way.
	static TESForm* GetLeveledTemplateInHierarchy(TESActorBase* apBase, uint16_t& aiOutCompositeTemplateUseFlags);

	ACTOR_BASE_DATA actorData;
	TESLevItem* pDeathItem;
	BGSVoiceType* pVoiceType;
	TESForm* pTemplateForm;
	uint32_t iChangeFlags;
	BSSimpleList<FACTION_RANK*> listFactions;

	static KarmicTitleTable pKarmicTitles;
	static inline AlignmentNameTable pAlignmentNames{"Good", "Neutral", "Evil", "VeryGood", "VeryEvil"};
	static SettingT<GameSettingCollection> fDefaultFatigue;
	static SettingT<GameSettingCollection> fDefaultKarma;
	static SettingT<GameSettingCollection> iBaseDisposition;
	static SettingT<GameSettingCollection> fAlignGoodMinKarma;
	static SettingT<GameSettingCollection> fAlignVeryEvilMaxKarma;
	static SettingT<GameSettingCollection> fAlignVeryGoodMinKarma;
	static SettingT<GameSettingCollection> fAlignEvilMaxKarma;
	static SettingT<GameSettingCollection> fKarmaDeathPenalty;
	static inline float pfDefaultAlignmentValues[5]{500.0f, 0.0f, -500.0f, 900.0f, -900.0f};
};
inline constexpr fnvd::Vtable TESActorBaseData_vftable{};

// The game settings TESActorBaseData.cpp defines (PDB), at their exe addresses.
extern SettingT<GameSettingCollection> sKarmicTitleNeutral03;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral28;
extern SettingT<GameSettingCollection> sKarmicTitleGood15;
extern SettingT<GameSettingCollection> sKarmicTitleEvil21;
extern SettingT<GameSettingCollection> sKarmicTitleGood08;
extern SettingT<GameSettingCollection> sKarmicTitleEvil18;
extern SettingT<GameSettingCollection> sKarmicTitleEvil06;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral30;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral24;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral19;
extern SettingT<GameSettingCollection> sKarmicTitleGood27;
extern SettingT<GameSettingCollection> sKarmicTitleGood25;
extern SettingT<GameSettingCollection> sKarmicTitleEvil24;
extern SettingT<GameSettingCollection> sKarmicTitleGood23;
extern SettingT<GameSettingCollection> sKarmicTitleEvil10;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral04;
extern SettingT<GameSettingCollection> sKarmicTitleEvil23;
extern SettingT<GameSettingCollection> sKarmicTitleGood01;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral02;
extern SettingT<GameSettingCollection> sKarmicTitleEvil13;
extern SettingT<GameSettingCollection> sKarmicTitleGood05;
extern SettingT<GameSettingCollection> sKarmicTitleGood12;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral17;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral13;
extern SettingT<GameSettingCollection> sKarmicTitleEvil26;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral11;
extern SettingT<GameSettingCollection> sKarmicTitleGood02;
extern SettingT<GameSettingCollection> sKarmicTitleGood07;
extern SettingT<GameSettingCollection> sKarmicTitleEvil05;
extern SettingT<GameSettingCollection> sKarmicTitleEvil27;
extern SettingT<GameSettingCollection> sKarmicTitleEvil19;
extern SettingT<GameSettingCollection> sKarmicTitleGood11;
extern SettingT<GameSettingCollection> sKarmicTitleGood17;
extern SettingT<GameSettingCollection> sKarmicTitleGood06;
extern SettingT<GameSettingCollection> sKarmicTitleGood13;
extern SettingT<GameSettingCollection> sKarmicTitleEvil30;
extern SettingT<GameSettingCollection> sKarmicTitleGood16;
extern SettingT<GameSettingCollection> sKarmicTitleGood03;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral20;
extern SettingT<GameSettingCollection> sKarmicTitleGood26;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral05;
extern SettingT<GameSettingCollection> sKarmicTitleGood10;
extern SettingT<GameSettingCollection> sKarmicTitleEvil16;
extern SettingT<GameSettingCollection> sKarmicTitleEvil14;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral26;
extern SettingT<GameSettingCollection> sKarmicTitleGood24;
extern SettingT<GameSettingCollection> sKarmicTitleEvil07;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral18;
extern SettingT<GameSettingCollection> sKarmicTitleEvil15;
extern SettingT<GameSettingCollection> sKarmicTitleEvil01;
extern SettingT<GameSettingCollection> sKarmicTitleGood19;
extern SettingT<GameSettingCollection> sKarmicTitleEvil29;
extern SettingT<GameSettingCollection> sKarmicTitleGood18;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral23;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral09;
extern SettingT<GameSettingCollection> sKarmicTitleEvil04;
extern SettingT<GameSettingCollection> sKarmicTitleEvil17;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral12;
extern SettingT<GameSettingCollection> sKarmicTitleGood20;
extern SettingT<GameSettingCollection> sKarmicTitleEvil20;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral01;
extern SettingT<GameSettingCollection> sKarmicTitleGood21;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral10;
extern SettingT<GameSettingCollection> sKarmicTitleEvil11;
extern SettingT<GameSettingCollection> fAlignMaxKarma;
extern SettingT<GameSettingCollection> sKarmicTitleEvil03;
extern SettingT<GameSettingCollection> sKarmicTitleGood04;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral22;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral07;
extern SettingT<GameSettingCollection> fAlignMinKarma;
extern SettingT<GameSettingCollection> sKarmicTitleEvil28;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral15;
extern SettingT<GameSettingCollection> sKarmicTitleGood28;
extern SettingT<GameSettingCollection> sKarmicTitleEvil08;
extern SettingT<GameSettingCollection> sKarmicTitleGood09;
extern SettingT<GameSettingCollection> sKarmicTitleEvil25;
extern SettingT<GameSettingCollection> sKarmicTitleGood29;
extern SettingT<GameSettingCollection> sKarmicTitleEvil22;
extern SettingT<GameSettingCollection> sKarmicTitleEvil12;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral29;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral25;
extern SettingT<GameSettingCollection> sKarmicTitleEvil09;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral14;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral27;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral21;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral16;
extern SettingT<GameSettingCollection> sKarmicTitleGood30;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral08;
extern SettingT<GameSettingCollection> sKarmicTitleGood14;
extern SettingT<GameSettingCollection> sKarmicTitleGood22;
extern SettingT<GameSettingCollection> sKarmicTitleEvil02;
extern SettingT<GameSettingCollection> sKarmicTitleNeutral06;
inline int32_t ibaddefaults = -1;
inline float fbadverybaddiff;
inline float fgoodbaddiff;
inline float fgoodverygooddiff;
