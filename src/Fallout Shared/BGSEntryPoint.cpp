#include "Fallout Shared/BGSEntryPoint.h"

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/BGSEntryPointFunction.h"
#include "Fallout Shared/BGSPerkEntry.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout/AI/PlayerCharacter.h"

#include <cstdarg>

// The filter forms of the entry points, named after them (the perk owner first). Not named in the PDB or IDA; names
// chosen here.
using EntryPointParameterArray1 = BGSEntryPoint::EntryPointParameter[1];
using EntryPointParameterArray2 = BGSEntryPoint::EntryPointParameter[2];
using EntryPointParameterArray3 = BGSEntryPoint::EntryPointParameter[3];
static EntryPointParameterArray1 kOwnerNames{{"Perk Owner"}};
static EntryPointParameterArray2 kOwnerWeaponNames{{"Perk Owner"}, {"Weapon"}};
static EntryPointParameterArray3 kOwnerWeaponTargetNames{{"Perk Owner"}, {"Weapon"}, {"Target"}};
static EntryPointParameterArray2 kOwnerTargetNames{{"Perk Owner"}, {"Target"}};
static EntryPointParameterArray2 kOwnerAttackerNames{{"Perk Owner"}, {"Attacker"}};
static EntryPointParameterArray3 kOwnerAttackerWeaponNames{{"Perk Owner"}, {"Attacker"}, {"Attacker Weapon"}};
static EntryPointParameterArray3 kOwnerAttackerAttackeeNames{{"Perk Owner"}, {"Attacker"}, {"Attackee"}};
static EntryPointParameterArray2 kOwnerItemNames{{"Perk Owner"}, {"Item"}};
// No parameters in the exe point here: the mine entry point's take kOwnerItemNames.
static EntryPointParameterArray2 kOwnerMineNames{{"Perk Owner"}, {"Mine"}};
static BGSEntryPoint::EntryPointParameters kOwnerParameters{1, kOwnerNames};
static BGSEntryPoint::EntryPointParameters kOwnerWeaponParameters{2, kOwnerWeaponNames};
static BGSEntryPoint::EntryPointParameters kOwnerWeaponTargetParameters{3, kOwnerWeaponTargetNames};
static BGSEntryPoint::EntryPointParameters kOwnerTargetParameters{2, kOwnerTargetNames};
static BGSEntryPoint::EntryPointParameters kOwnerAttackerParameters{2, kOwnerAttackerNames};
static BGSEntryPoint::EntryPointParameters kOwnerAttackerWeaponParameters{3, kOwnerAttackerWeaponNames};
static BGSEntryPoint::EntryPointParameters kOwnerAttackerAttackeeParameters{3, kOwnerAttackerAttackeeNames};
static BGSEntryPoint::EntryPointParameters kOwnerItemParameters{2, kOwnerItemNames};
// The exe's data: the mine's filter names are the item's (kOwnerItemNames), not kOwnerMineNames.
static BGSEntryPoint::EntryPointParameters kOwnerMineParameters{2, kOwnerItemNames};

// The filter parameters are spelled as the values of the k...Parameters globals above, which the exe's initialiser
// copies in at start-up (below).
BGSEntryPoint::EntryPointTable BGSEntryPoint::EntryPoints{
	{"Calculate Weapon Damage", {3, kOwnerWeaponTargetNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Calculate My Critical Hit Chance", {3, kOwnerWeaponTargetNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Calculate My Critical Hit Damage", {3, kOwnerWeaponTargetNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Calculate Weapon Attack AP Cost", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Calculate Mine Explode Chance", {2, kOwnerItemNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Adjust Range Penalty", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Adjust Limb Damage", {3, kOwnerAttackerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Calculate Weapon Range", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Calculate To Hit Chance", {3, kOwnerWeaponTargetNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Adjust Experience Points", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Adjust Gained Skill Points", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Adjust Book Skill Points", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Recovered Health", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Calculate Inventory AP Cost", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Get Disposition", {2, kOwnerTargetNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Get Should Attack", {2, kOwnerAttackerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Get Should Assist", {3, kOwnerAttackerAttackeeNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Calculate Buy Price", {2, kOwnerItemNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Get Bad Karma", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Get Good Karma", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Ignore Locked Terminal", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Add Leveled List On Death", {2, kOwnerTargetNames}, BGSEntryPointFunction::LEVELED_LIST_FUNCTION},
	{"Get Max Carry Weight", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Addiction Chance", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Addiction Duration", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Positive Chem Duration", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Adjust Drinking Radiation", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Activate", {2, kOwnerTargetNames}, BGSEntryPointFunction::ACTIVATE_FUNCTION},
	{"Mysterious Stranger", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Has Paralyzing Palm", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Hacking Science Bonus", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Ignore Running During Detection", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Ignore Broken Lock", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Has Concentrated Fire", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Calculate Gun Spread", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Player Kill AP Reward", {3, kOwnerWeaponTargetNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Enemy Critical Hit Chance", {3, kOwnerWeaponTargetNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Reload Speed", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Equip Speed", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Action Point Regen", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Action Point Cost", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Miss Fortune", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Run Speed", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Attack Speed", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Radiation Consumed", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Has Pip Hacker", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Has Meltdown", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"See Enemy Health", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Has Jury Rigging", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Threat Range", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Threat", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Has Fast Travel Always", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Knockdown Chance", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Weapon Strength Req", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Aiming Move Speed", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Light Items", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Damage Threshold (defender)", {3, kOwnerAttackerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Chance for Ammo Item", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Damage Threshold (attacker)", {3, kOwnerWeaponTargetNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Throwing Velocity", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Chance for Item On Fire", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Has Unarmed Forward Power Attack", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Has Unarmed Back Power Attack", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Has Unarmed Crouched Power Attack", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Has Unarmed Counter Attack", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Has Unarmed Left Power Attack", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Has Unarmed Right Power Attack", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"VATS HelperChance", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Modify Item Damage", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Has Improved Detection", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Has Improved Spotting", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Has Improved Item Detection", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Adjust Explosion Radius", {2, kOwnerWeaponNames}, BGSEntryPointFunction::VALUE_FUNCTION},
	{"Adjust Heavy Weapon Weight", {1, kOwnerNames}, BGSEntryPointFunction::VALUE_FUNCTION}};

// Runs the perk owner's perks for the entry point. The variadic arguments are the entry point's filter forms after
// the owner (FilterParameters.cCount counts the owner), then the function type's arguments; a null among them stops
// the run with a warning. A player teammate runs the player's companion perk entries (bIsTeammate).
// The decomp shows the player as the owner from the teammate swap on; the asm saves the original owner (ebp-4) and
// stores it back into apPerkOwner once the entry list is found, so the filters and the functions get the original
// actor. Each argument array holds ten pointers, as the exe's frame does; the exe does not bound the counts.
void BGSEntryPoint::HandleEntryPoint(ENTRY_POINT aeEntryPoint, Actor* apPerkOwner, ...) {
	// The exe compares the entry point signed (jl).
	if (!apPerkOwner || aeEntryPoint >= ENTRY_POINT_COUNT) {
		return;
	}
	bool bIsTeammate = false;
	Actor* pOwner = apPerkOwner;
	if (apPerkOwner != pPlayer && apPerkOwner->IsPlayerTeammate()) {
		bIsTeammate = true;
		apPerkOwner = pPlayer;
	}
	BSSimpleList<BGSPerkEntry*>* pPerkEntries =
		apPerkOwner->GetPerkEntryList(static_cast<uint8_t>(aeEntryPoint), bIsTeammate);
	if (!pPerkEntries) {
		return;
	}
	if (!pPerkEntries->GetItem()) {
		return;
	}
	apPerkOwner = pOwner;
	uint8_t ucNumArgs = EntryPoints[aeEntryPoint].FilterParameters.cCount;
	int32_t iFunctionType = EntryPoints[aeEntryPoint].cFunctionType;
	uint8_t ucNumParams = BGSEntryPointFunction::GetArgumentCount(iFunctionType);
	TESForm* apFilterForms[10];
	void* apFunctionArguments[10];
	apFilterForms[0] = apPerkOwner;
	va_list plist;
	va_start(plist, apPerkOwner);
	bool bEnoughArgs = true;
	for (uint32_t i = 1; i < ucNumArgs; ++i) {
		apFilterForms[i] = va_arg(plist, TESForm*);
		if (!apFilterForms[i]) {
			bEnoughArgs = false;
		}
	}
	bool bEnoughParams = true;
	for (uint32_t j = 0; j < ucNumParams; ++j) {
		apFunctionArguments[j] = va_arg(plist, void*);
		if (!apFunctionArguments[j]) {
			bEnoughParams = false;
		}
	}
	va_end(plist);
	if (bEnoughArgs && bEnoughParams) {
		while (pPerkEntries) {
			BGSPerkEntry* pPerkEntry = pPerkEntries->GetItem();
			if (pPerkEntry && pPerkEntry->CheckConditionFilters(ucNumArgs, apFilterForms)) {
				// The exe gets the data (slot 2) before the function (slot 1).
				BGSEntryPointFunctionData* pFunctionData =
					static_cast<BGSEntryPointFunctionData*>(pPerkEntry->GetFunctionData());
				uint8_t ucFunction = pPerkEntry->GetFunction();
				BGSEntryPointFunction::ExecuteFunction(static_cast<
														   BGSEntryPointFunction::ENTRY_POINT_FUNCTION>(ucFunction),
					apPerkOwner, static_cast<BGSEntryPointFunction::ENTRY_POINT_FUNCTION_TYPE>(iFunctionType),
					ucNumParams, apFunctionArguments, pFunctionData);
			}
			pPerkEntries = pPerkEntries->GetNext();
		}
	} else {
		// The exe names "parameters" and the filter count whenever the filter forms were all given, even when a
		// function argument is the one missing.
		uint8_t ucExpected = bEnoughArgs ? ucNumArgs : ucNumParams;
		const char* pKind = bEnoughArgs ? "parameters" : "arguments";
		Warning("DEFAULT: Invalid %s to entry point function %s, expected %i.", pKind,
			EntryPoints[aeEntryPoint].pEntryPointName, ucExpected);
	}
}
