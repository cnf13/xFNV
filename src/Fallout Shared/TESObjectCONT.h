#pragma once

#include "BSHavok/bhkShape.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESMagicCasterForm.h"
#include "Fallout Shared/TESMagicTargetForm.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Reconstruction.hpp"

class MobileObject;
class TESObjectREFR;
class TESSound;

// A container's DATA record.
struct CONT_DATA {
	char cContFlags;
};

// A container base form (CONT). Its own members are not declared yet.
class TESObjectCONT : public TESBoundAnimObject,
					  public TESContainer,
					  public TESFullName,
					  public TESModelTextureSwap,
					  public TESScriptableForm,
					  public TESWeightForm,
					  public TESMagicCasterForm,
					  public TESMagicTargetForm,
					  public BGSDestructibleObjectForm,
					  public BGSOpenCloseForm {
public:
	TESObjectCONT();
	~TESObjectCONT() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	bool Activate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abIdFlag,
		TESBoundObject* apObjecttoGet, int32_t aicount) override;
	void HandleOpenFinish(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef) override;
	bool GetBlockActivatesWhileOpening() override { return true; }

	using TESForm::Save;

	// The looping sound (pLoopingSound); the exe's +0x94 getter (thiscall, ret 0, 0x9185E0).
	TESSound* GetLoopingSound();
	// Whether the container's contents respawn (flag 2 of Data.cContFlags).
	bool GetRespawn();
	TESSound* GetCloseSound() { return pCloseSound; }
	TESSound* GetOpenSound() { return pOpenSound; }
	// Stores pOpenSound; the exe's shared one-field setter (thiscall, ret 4, 0x8CC6D0, the same
	// body as ArrowProjectile::SetState).
	void SetOpenSound(TESSound* apSound);
	// Stores pCloseSound; the exe's shared setter of the field at +0x90 (thiscall, ret 4,
	// 0x4FB0B0, the same body as BSCullingProcess::SetCullMode).
	void SetCloseSound(TESSound* apSound);
	// Stores pLoopingSound; the exe's shared setter of the field at +0x94 (thiscall, ret 4,
	// 0x4FEBF0, the same body as BGSTalkingActivator::SetVoiceType).
	void SetLoopingSound(TESSound* apSound);
	bool CanActivate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abAllowLockpicking);

	TESSound* pOpenSound;
	TESSound* pCloseSound;
	TESSound* pLoopingSound;
	CONT_DATA Data;
};

inline MobileObject* pstaticcontainer;
inline constexpr fnvd::Vtable TESObjectCONT_vftable{};
inline constexpr fnvd::Vtable TESObjectCONT_vftable_BGSDestructibleObjectForm{};
inline constexpr fnvd::Vtable TESObjectCONT_vftable_BGSOpenCloseForm{};
inline constexpr fnvd::Vtable TESObjectCONT_vftable_TESContainer{};
inline constexpr fnvd::Vtable TESObjectCONT_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESObjectCONT_vftable_TESModelTextureSwap{};
inline constexpr fnvd::Vtable TESObjectCONT_vftable_TESScriptableForm{};
inline constexpr fnvd::Vtable TESObjectCONT_vftable_TESWeightForm{};
