#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Reconstruction.hpp"

class ActorValueOwner;

// A form describing one actor value: its script name, flags, callbacks and dependent values.
class ActorValueInfo : public TESForm, public TESFullName, public TESDescription, public TESIcon {
public:
	ActorValueInfo(ActorValue::Index aiWhich, char* apScriptName);
	~ActorValueInfo() override;

	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	// TESIcon
	const char* GetDefaultPath() const override;

	using TESForm::Save;

	// Sets the enumerated count (at most 10) and takes that many name strings from the variadic arguments
	// (cdecl variadic member, this at [esp+4]).
	void SetEnumerated(int32_t aiEnumCount, ...);

	// The form ID the actor value aiWhich uses in compiled script data.
	static uint32_t GetActorValueFormID(ActorValue::Index aiWhich);

	// the name scripts refer to the value by
	char* sScriptName;
	// the short display name
	BSStringT<char> sAbbreviation;
	uint32_t uFlags;
	// the kind of value this is (attribute, skill, ...)
	ActorValue::Section eAVType;
	// derives this value from others (cdecl, float in ST0)
	float (*pDerivationFunction)(ActorValueOwner* apOwner, ActorValue::Index aeActorValue);
	// maps this value onto another
	ActorValue::Index (*pMappingFunction)(ActorValueOwner* apOwner, ActorValue::Index aeActorValue);
	// called when the value changes
	void (*pModifiedCallback)(ActorValueOwner* apOwner, ActorValue::Index aeActorValue, float afPreviousValue,
		float afNewValue, ActorValueOwner* apCause);
	int32_t iNumDependentActorValues;
	// the actor values this one derives from
	int32_t DependentActorValues[15];
	// number of enumerated values, for values split into named parts
	uint32_t iEnumCount;
	// the names of the enumerated values
	const char* sEnumNames[10];
};
inline constexpr fnvd::Vtable ActorValueInfo_vftable_TESIcon{};
inline constexpr fnvd::Vtable ActorValueInfo_vftable_TESDescription{};
inline constexpr fnvd::Vtable ActorValueInfo_vftable_TESFullName{};
inline constexpr fnvd::Vtable ActorValueInfo_vftable{};
