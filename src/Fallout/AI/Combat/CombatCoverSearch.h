#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/PathingCoverLocation.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class BGSLoadGameBuffer;
class BGSSaveFormBuffer;

// Cover search state of a combat state: the search location, its time stamp and the cover locations. CombatState embeds
// two.
class CombatCoverSearchInfo {
public:
    CombatCoverSearchInfo();

    // Clears the search radius, sets the time stamp to the most negative float and empties the cover locations.
    void Clear();
    // Copies apCopy, or resets when it is null.
    void Copy(const CombatCoverSearchInfo* apCopy);
    void LoadGameBGS(BGSLoadGameBuffer* apLoadGameBuffer);
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    NiPoint3 kSearchLocation;
    NiPoint3 kCenter;
    float fRadius;
    CombatTimeStamp kLastSearch;
    BSSimpleArray<PathingCoverLocation> kCoverLocations;
    uint32_t uiPreviousCoverKey;
    uint32_t uiPreviousState;
    bool bSearching;
};
