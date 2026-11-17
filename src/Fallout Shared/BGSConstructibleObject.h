#pragma once

#include "Fallout Shared/TESObjectMISC.h"
#include "Reconstruction.hpp"

class BGSListForm;

// A constructible object base form, derived from the misc item.
class BGSConstructibleObject : public TESObjectMISC {
public:
	BGSConstructibleObject();
	~BGSConstructibleObject() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESObjectMISC::Save;

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	// the items a build needs, and the item it makes
	BGSListForm* pRequiredItems;
	TESBoundObject* pCreatedItem;
};
inline constexpr fnvd::Vtable BGSConstructibleObject_vftable{};
inline constexpr fnvd::Vtable BGSConstructibleObject_vftable_BGSDestructibleObjectForm{};
inline constexpr fnvd::Vtable BGSConstructibleObject_vftable_BGSMessageIcon{};
inline constexpr fnvd::Vtable BGSConstructibleObject_vftable_BGSPickupPutdownSounds{};
inline constexpr fnvd::Vtable BGSConstructibleObject_vftable_TESFullName{};
inline constexpr fnvd::Vtable BGSConstructibleObject_vftable_TESIcon{};
inline constexpr fnvd::Vtable BGSConstructibleObject_vftable_TESModelTextureSwap{};
inline constexpr fnvd::Vtable BGSConstructibleObject_vftable_TESScriptableForm{};
inline constexpr fnvd::Vtable BGSConstructibleObject_vftable_TESValueForm{};
inline constexpr fnvd::Vtable BGSConstructibleObject_vftable_TESWeightForm{};
