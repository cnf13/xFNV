#pragma once

#include "Reconstruction.hpp"

class TESForm;

// A form's save-game change flags, passed by value to the change queries. Only what the reconstructed code uses is
// declared.
class BGSChangeFlags {
public:
	BGSChangeFlags();
	BGSChangeFlags(int32_t aiFlags);

	void RemoveFlags(int32_t aiFlags);
	// Whether any of aiFlags is set.
	bool CheckFlags(int32_t aiFlags);
	void AddFlags(int32_t aiFlags);
	// iFlags.
	operator int32_t() const { return iFlags; }
	// Whether the flags are exactly aiFlags (thiscall, ret 4; CleanupExpiredData calls it). Not named in the PDB or
	// IDA; name chosen here.
	bool Equals(int32_t aiFlags) const;
	// The flags of this that aFlags lacks, written to apResult (the exe's hidden result;
	// thiscall, ret 8, 0x847CD0).
	BGSChangeFlags* GetDifference(BGSChangeFlags* apResult, int32_t aFlags) const;
	bool ReduceToForcedChanges(TESForm* apForm);

	// The changes of aiOldChangeFlags that loading aiNewChangeFlags for a form of type
	// aiFormType cannot revert, written to apResult (the exe's hidden result; cdecl, 0x841120).
	static BGSChangeFlags* GetUnrevertibleChanges(BGSChangeFlags* apResult, BGSChangeFlags aiOldChangeFlags,
		BGSChangeFlags aiNewChangeFlags, uint32_t aiFormType, bool abWillLoad);
	// The name of change flag aiFlag of a form of type aiFormType.
	static const char* GetFlagName(int32_t aiFlag, uint32_t aiFormType, bool abShortName);

	// Protected in the PDB; public so the change queries that still take the flags as an int can read it.
	int32_t iFlags;
};
