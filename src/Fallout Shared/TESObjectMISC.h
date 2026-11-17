#pragma once

#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Reconstruction.hpp"

class TESSound;

// A misc item base form (MISC).
class TESObjectMISC : public TESBoundObject,
					  public TESFullName,
					  public TESModelTextureSwap,
					  public TESIcon,
					  public TESScriptableForm,
					  public TESValueForm,
					  public TESWeightForm,
					  public BGSDestructibleObjectForm,
					  public BGSMessageIcon,
					  public BGSPickupPutdownSounds {
public:
	TESObjectMISC();
	~TESObjectMISC() override;

	void InitializeData() override;
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

	using TESForm::Save;

	// The looping sound (pLoopingSound); the exe's +0xA8 getter (thiscall, ret 0, 0x8CDD90).
	TESSound* GetLoopingSound();
	// Stores pLoopingSound; the exe's shared one-field setter (thiscall, ret 4, 0x9DB930).
	void SetLoopingSound(TESSound* apSound);

	TESSound* pLoopingSound;
};
inline constexpr fnvd::Vtable TESObjectMISC_vftable_BGSPickupPutdownSounds{};
inline constexpr fnvd::Vtable TESObjectMISC_vftable_BGSMessageIcon{};
inline constexpr fnvd::Vtable TESObjectMISC_vftable_BGSDestructibleObjectForm{};
inline constexpr fnvd::Vtable TESObjectMISC_vftable_TESWeightForm{};
inline constexpr fnvd::Vtable TESObjectMISC_vftable_TESValueForm{};
inline constexpr fnvd::Vtable TESObjectMISC_vftable_TESScriptableForm{};
inline constexpr fnvd::Vtable TESObjectMISC_vftable_TESIcon{};
inline constexpr fnvd::Vtable TESObjectMISC_vftable_TESModelTextureSwap{};
inline constexpr fnvd::Vtable TESObjectMISC_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESObjectMISC_vftable{};
