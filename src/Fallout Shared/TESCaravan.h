#pragma once

#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESValueForm.h"
#include "Reconstruction.hpp"

// A Caravan card's DATA chunk: its face value and suit.
struct CARAVANCARDDATA {
	int32_t iFaceValue;
	uint32_t iCardSuit;
};

// A Caravan deck's DATA chunk: one dword, which the exe byte-swaps through its shared one-dword swap.
struct CARAVANDECKDATA {
	void Endian();

	uint32_t uiValue;
};

// A Caravan card base form (CCRD).
class TESCaravanCard : public TESBoundObject,
					   public TESFullName,
					   public TESModelTextureSwap,
					   public TESIcon,
					   public BGSMessageIcon,
					   public TESValueForm,
					   public TESScriptableForm,
					   public BGSPickupPutdownSounds {
public:
	TESCaravanCard();
	~TESCaravanCard() override;

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

	// The exe's copy is a trivial address getter (this + 0xB4).
	CARAVANCARDDATA* GetData() { return &data; }

	static int32_t SortCardsFunc(const void* t1, const void* t2);

	bool bIsActive;
	uint8_t padding99[3];
	TESTexture faceTexture;
	TESTexture backTexture;
	CARAVANCARDDATA data;
};

// A Caravan money base form (CMNY).
class TESCaravanMoney : public TESBoundObject,
						public TESFullName,
						public TESModelTextureSwap,
						public TESIcon,
						public BGSMessageIcon,
						public TESValueForm,
						public BGSPickupPutdownSounds {
public:
	TESCaravanMoney();
	~TESCaravanMoney() override;

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

	TESModelTextureSwap* GetAnteModel(int32_t index);

	TESModelTextureSwap anteModels[2];
};

// A Caravan deck form: its cards and its data.
class TESCaravanDeck : public TESForm, public TESFullName {
public:
	TESCaravanDeck();
	~TESCaravanDeck() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESForm::Save;

	BSSimpleList<TESCaravanCard*>* pDeck;
	CARAVANDECKDATA data;
};

inline constexpr fnvd::Vtable TESCaravanCard_vftable_BGSPickupPutdownSounds{};
inline constexpr fnvd::Vtable TESCaravanCard_vftable_TESScriptableForm{};
inline constexpr fnvd::Vtable TESCaravanCard_vftable_TESValueForm{};
inline constexpr fnvd::Vtable TESCaravanCard_vftable_BGSMessageIcon{};
inline constexpr fnvd::Vtable TESCaravanCard_vftable_TESIcon{};
inline constexpr fnvd::Vtable TESCaravanCard_vftable_TESModelTextureSwap{};
inline constexpr fnvd::Vtable TESCaravanCard_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESCaravanCard_vftable{};
inline constexpr fnvd::Vtable TESCaravanMoney_vftable_BGSPickupPutdownSounds{};
inline constexpr fnvd::Vtable TESCaravanMoney_vftable_TESValueForm{};
inline constexpr fnvd::Vtable TESCaravanMoney_vftable_BGSMessageIcon{};
inline constexpr fnvd::Vtable TESCaravanMoney_vftable_TESIcon{};
inline constexpr fnvd::Vtable TESCaravanMoney_vftable_TESModelTextureSwap{};
inline constexpr fnvd::Vtable TESCaravanMoney_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESCaravanMoney_vftable{};
inline constexpr fnvd::Vtable TESCaravanDeck_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESCaravanDeck_vftable{};
