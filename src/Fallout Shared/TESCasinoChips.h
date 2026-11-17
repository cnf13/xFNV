#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESValueForm.h"

class TESQuest;

// A casino's DATA chunk: the blackjack shuffle and payout, the slot machine's reel stops and the limits.
struct CASINO_DATA {
	void Endian();

	float fBJ_ShufflePercent;
	float fBJ_BlackjackPayout;
	int32_t iSlotStop_A;
	int32_t iSlotStop_B;
	int32_t iSlotStop_C;
	int32_t iSlotStop_D;
	int32_t iSlotStop_E;
	int32_t iSlotStop_F;
	int32_t iSlotStop_W;
	int32_t iBJNumDecks;
	int32_t iMaxWinnings;
	uint32_t casinoChipID;
	uint32_t questID;
	bool bBJ_DealerHoleCard;
	uint8_t padding35[3];
};

// A casino chips base form (CHIP).
class TESCasinoChips : public TESBoundObject,
					   public TESFullName,
					   public TESModelTextureSwap,
					   public TESIcon,
					   public BGSMessageIcon,
					   public TESValueForm,
					   public BGSDestructibleObjectForm,
					   public BGSPickupPutdownSounds {
public:
	TESCasinoChips();
	~TESCasinoChips() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	// also override TESValueForm's slots of the same signatures
	uint16_t GetSaveSize(uint32_t aiFlags) override;
	void SaveGame(uint32_t aiFlags) override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	NiAVObject* Clone3D(TESObjectREFR* pRequester, bool deepCopy) override;

	using TESBoundObject::Clone3D;
	using TESForm::Save;

	BSStringT<char> cDesc;
};

// A casino form (CSNO).
class TESCasino : public TESForm, public TESFullName {
public:
	TESCasino();
	~TESCasino() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESForm::Save;

	// The texture of slot reel index.
	TESTexture* GetTexture(int32_t index);
	// The winnings quest, data.questID.
	TESQuest* GetQuest();
	// The casino's chips, data.casinoChipID.
	TESCasinoChips* GetChipType();
	int32_t GetMaxWinnings();
	CASINO_DATA* GetData() { return &data; }
	TESModelTextureSwap* GetModel(int32_t a2);

	TESModelTextureSwap modelArrayList[10];
	TESTexture textureArrayList[11];
	CASINO_DATA data;
};
