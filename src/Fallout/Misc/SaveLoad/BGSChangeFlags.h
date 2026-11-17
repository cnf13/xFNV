#pragma once

#include <cstdint>

class TESForm;

// A form's save-game change flags, passed by value to the change queries. Only part of the class is declared.
class BGSChangeFlags {
public:
    BGSChangeFlags();
    BGSChangeFlags(int32_t aiFlags);

    void RemoveFlags(int32_t aiFlags);
    // Whether any of aiFlags is set.
    bool CheckFlags(int32_t aiFlags);
    void AddFlags(int32_t aiFlags);
    // iFlags.
    operator int32_t() const {
        return iFlags;
    }
    // Whether the flags are exactly aiFlags. CleanupExpiredData calls it.
    bool Equals(int32_t aiFlags) const;
    // The flags of this that aFlags lacks, written to apResult.
    BGSChangeFlags* GetDifference(BGSChangeFlags* apResult, int32_t aFlags) const;
    bool ReduceToForcedChanges(TESForm* apForm);

    // The changes of aiOldChangeFlags that loading aiNewChangeFlags for a form of type aiFormType cannot revert,
    // written to apResult.
    static BGSChangeFlags* GetUnrevertibleChanges(
        BGSChangeFlags* apResult,
        BGSChangeFlags aiOldChangeFlags,
        BGSChangeFlags aiNewChangeFlags,
        uint32_t aiFormType,
        bool abWillLoad
    );
    // The name of change flag aiFlag of a form of type aiFormType.
    static const char* GetFlagName(int32_t aiFlag, uint32_t aiFormType, bool abShortName);

    // Public so the change queries that take the flags as an int can read it.
    int32_t iFlags;
};
