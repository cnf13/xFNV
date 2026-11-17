#pragma once

#include "Fallout Shared/TESObjectMISC.h"
#include "Reconstruction.hpp"

// A key base form: a misc item with the same members (both allocate 0xAC bytes).
class TESKey : public TESObjectMISC {
public:
	TESKey();
	~TESKey() override;
};
inline constexpr fnvd::Vtable TESKey_vftable_BGSPickupPutdownSounds{};
inline constexpr fnvd::Vtable TESKey_vftable_BGSMessageIcon{};
inline constexpr fnvd::Vtable TESKey_vftable_BGSDestructibleObjectForm{};
inline constexpr fnvd::Vtable TESKey_vftable_TESWeightForm{};
inline constexpr fnvd::Vtable TESKey_vftable_TESValueForm{};
inline constexpr fnvd::Vtable TESKey_vftable_TESScriptableForm{};
inline constexpr fnvd::Vtable TESKey_vftable_TESIcon{};
inline constexpr fnvd::Vtable TESKey_vftable_TESModelTextureSwap{};
inline constexpr fnvd::Vtable TESKey_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESKey_vftable{};
