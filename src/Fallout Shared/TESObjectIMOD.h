#pragma once

#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Reconstruction.hpp"

// A weapon mod base form. Its members are not declared yet.
class TESObjectIMOD : public TESBoundObject,
					  public TESFullName,
					  public TESModelTextureSwap,
					  public TESIcon,
					  public TESScriptableForm,
					  public TESDescription,
					  public TESValueForm,
					  public TESWeightForm,
					  public BGSDestructibleObjectForm,
					  public BGSMessageIcon,
					  public BGSPickupPutdownSounds {
public:
	TESObjectIMOD();
	~TESObjectIMOD() override;

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
};
inline constexpr fnvd::Vtable TESObjectIMOD_vftable_BGSPickupPutdownSounds{};
inline constexpr fnvd::Vtable TESObjectIMOD_vftable_BGSMessageIcon{};
inline constexpr fnvd::Vtable TESObjectIMOD_vftable_BGSDestructibleObjectForm{};
inline constexpr fnvd::Vtable TESObjectIMOD_vftable_TESWeightForm{};
inline constexpr fnvd::Vtable TESObjectIMOD_vftable_TESValueForm{};
inline constexpr fnvd::Vtable TESObjectIMOD_vftable_TESDescription{};
inline constexpr fnvd::Vtable TESObjectIMOD_vftable_TESScriptableForm{};
inline constexpr fnvd::Vtable TESObjectIMOD_vftable_TESIcon{};
inline constexpr fnvd::Vtable TESObjectIMOD_vftable_TESModelTextureSwap{};
inline constexpr fnvd::Vtable TESObjectIMOD_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESObjectIMOD_vftable{};
