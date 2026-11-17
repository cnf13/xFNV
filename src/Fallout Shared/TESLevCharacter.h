#pragma once

#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Reconstruction.hpp"

// The leveled NPC form (LVLN): its leveled list picks the NPC it stands for.
class TESLevCharacter : public TESBoundAnimObject, public TESLeveledList, public TESModelTextureSwap {
public:
	TESLevCharacter();
	~TESLevCharacter() override;

	bool Load(TESFile* apFile) override;
	void Save() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	void UnClone3D(TESObjectREFR*) override {}
	NiAVObject* Clone3D(TESObjectREFR*) override { return nullptr; }
	// TESLeveledList
	int32_t GetMaxLevelDifference() override;

	using TESBoundAnimObject::Clone3D;
	using TESBoundAnimObject::LoadGame;
	using TESBoundAnimObject::Revert;
	using TESBoundAnimObject::Save;
	using TESBoundAnimObject::SaveGame;

	// IDA's struct ends with this dword; the exe allocates 0x70 bytes.
	uint32_t unk6C;
};
inline constexpr fnvd::Vtable TESLevCharacter_vftable_TESModelTextureSwap{};
inline constexpr fnvd::Vtable TESLevCharacter_vftable_TESLeveledList{};
inline constexpr fnvd::Vtable TESLevCharacter_vftable{};
