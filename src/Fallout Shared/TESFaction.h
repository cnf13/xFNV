#pragma once

#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESReactionForm.h"
#include "Fallout Shared/TESTexture.h"
#include "Reconstruction.hpp"

class TESReputation;

bool Cmd_GetPCFactionAttack(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);
bool Cmd_GetPCEnemyofFaction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue);

// A faction's rank data: the male and female rank titles and the insignia texture.
class RANK_DATA {
public:
	RANK_DATA();
	~RANK_DATA();

	// The exe's deleting destructor (0x5FC8D0) frees through ourFree.
	static void operator delete(void* apMemory);

	BSStringT<char> cMaleRankTitle;
	BSStringT<char> cFemaleRankTitle;
	TESTexture TextureInsignia;
};

// The faction's flags (the PDB's TESFaction.cpp).
struct FACTION_DATA {
	// Swaps the flags.
	void Endian();

	uint32_t iFlags;
};

// A faction form (FACT). Only the members the reconstructed code uses are declared.
class TESFaction : public TESForm, public TESFullName, public TESReactionForm {
public:
	TESFaction();

	~TESFaction() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	uint16_t GetSaveSize(uint32_t aiFlags) override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void SaveGame(uint32_t aiFlags) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) override;
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	using TESForm::Revert;
	using TESForm::Save;

	// Whether the faction is flagged evil.
	bool GetIsEvil();
	// Appends a new, empty rank data to the faction's rank list and returns its index.
	int32_t AddRankData();
	// &m_rankDataList. The PDB calls it GetData.
	BSSimpleList<RANK_DATA*>* QRankDataList() { return &m_rankDataList; }

	// Whether the player is an enemy of the faction (flag 0x10).
	bool GetPCAnEnemy();

	// Whether faction flag 0x200 is set; IDA calls it TracksCrime. Named after the PDB's
	// GetAllowSell: the PDB lists GetTrackCrime then GetAllowSell after the flags 0x1-0x40, and the FACT record's
	// second flag byte holds Track Crime (0x100) then Allow Sell (0x200) (thiscall, ret 0, 0x578770).
	bool GetAllowSell();
	void SetPCAnEnemy(bool abVal) { SetFlag(0x10, abVal); }
	int32_t GetFactionReaction(TESFaction* pFaction);
	// Adds reaction to the reaction towards pFaction (TESReactionForm, 0x48C0B0) and marks the reactions changed.
	void ModFactionReaction(TESFaction* pFaction, int32_t reaction);
	// Adds reaction to the reaction towards pFaction and marks the reactions changed.
	void SetFactionReaction(TESFaction* pFaction, int32_t reaction);
	bool GetPCExpelled();
	bool GetPCHasMurdereredMember();
	bool GetPCHasAttackedMember();
	bool IsTrackCrime();
	void SetPCMurder(bool abVal);
	void SetPCAttack(bool abVal);
	bool GetSpecialCombat();
	void SetPCExpelled(bool abVal);
	void AddMinorCrime(int32_t a2, uint8_t a3);

protected:
	void ClearRankDataList();
	void SetFlag(uint32_t aiFlag, bool abVal);

public:
	void HandleMajorCrimeReputation(int32_t a2, char a3);
	void SetFactionReaction_0(TESFaction* pFaction, int32_t reaction);

	FACTION_DATA data;
	TESReputation* pReputation;
	BSSimpleList<RANK_DATA*> m_rankDataList;
	uint32_t iMajorCrime;
	uint32_t iMinorCrime;
};
inline constexpr fnvd::Vtable TESFaction_vftable{};
inline constexpr fnvd::Vtable TESFaction_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESFaction_vftable_TESReactionForm{};
