#include "Fallout Shared/MiscStats.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESChallengeManager.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Dialogue/MenuTopic.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Interface/Menus/TraitMenu.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"

#include <cstdint>

// The statistics manager constructed at start-up.
static MiscStatManager msm;

MiscStat::MiscStat(const char* apStatName, uint32_t aiIndex, int32_t aiVal) : iVal(aiVal) {
    char* pkey = static_cast<char*>(ourMalloc(13u));
    BSsprintf(pkey, 13, "sMiscStat%03d", aiIndex);
    // The setting's string value is the name pointer; Setting::Info's string is a char*.
    pDisplayName = new SettingT<GameSettingCollection>(pkey, Setting::Info{.str = const_cast<char*>(apStatName)});
}

MiscStat::~MiscStat() {
    if (pDisplayName) {
        delete pDisplayName;
    }
}

void MiscStat::operator delete(void* apMemory) {
    ourFree(apMemory);
}

MiscStatManager::MiscStatManager() {
    if (!MiscStatManager::StatList[0]) {
        for (uint32_t aiIndex = 0; aiIndex < 43; ++aiIndex) {
            MiscStatManager::StatList[aiIndex] = new MiscStat(MiscStatManager::StatNames[aiIndex], aiIndex, 0);
        }
    }
}

const char* MiscStatManager::GetDisplayText(MiscStatID aeID) {
    return MiscStatManager::StatList[(int32_t)aeID]->pDisplayName->String();
}

int32_t MiscStatManager::GetVal(MiscStatID aeID) {
    return MiscStatManager::StatList[(int32_t)aeID]->iVal;
}

void MiscStatManager::Increment(MiscStatID aeID) {
    MiscStatManager::ModVal(aeID, 1);
}

void MiscStatManager::ModVal(MiscStatID aeID, int32_t aiVal) {
    int32_t classStatic;
    TileMenu* menuByClass;
    StatsMenu* menu;
    if (aiVal) {
        if ((((int32_t)aeID) < 43) && MiscStatManager::StatList[(int32_t)aeID]) {
            MiscStatManager::StatList[(int32_t)aeID]->iVal += aiVal;
            TESChallengeManager::UpdateChallenge(CHALLENGE_TYPE_MISC_STAT, aiVal, nullptr, nullptr, aeID, 0, 0);
            classStatic = StatsMenu::GetClassStatic();
            menuByClass = static_cast<TileMenu*>(Menu::GetMenuByClass(classStatic));
            menu = static_cast<StatsMenu*>(menuByClass->GetMenu());
            if (menu) {
                menu->UpdateMiscStat(aeID);
            }
        }
        else {
            Warning("MENUS: Unable to modify miscellaneous stat (id=%d) by %d.", aeID, aiVal);
        }
    }
}

void MiscStatManager::Decrement(MiscStatManager::MiscStatID aeID) {
    MiscStatManager::ModVal(aeID, (-1));
}

// The saved statistic count is 43, one less than MS_COUNT, as the game does.
void MiscStatManager::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    uint32_t uiCount = 43;
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&uiCount), 4, 0);
    for (uint32_t i = 0; i < uiCount; ++i) {
        int32_t iValue;
        if (StatList[i]) {
            iValue = StatList[i]->iVal;
        }
        else {
            iValue = 0;
        }
        int32_t iSavedValue = iValue;
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iSavedValue), 4, 0);
    }
}

// A count above 0x1000000 is taken as corrupt and nothing is read; values past the game's 43 statistics are skipped.
void MiscStatManager::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    uint32_t uiCount = 21;
    if (apLoadGameBuffer->GetVersion() >= 0xB) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&uiCount), 4);
    }
    if (uiCount > 0x1000000) {
        return;
    }
    for (uint32_t i = 0; i < uiCount; ++i) {
        int32_t iValue = 0;
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iValue), 4);
        if (i < 43 && StatList[i]) {
            StatList[i]->iVal = iValue;
        }
    }
}

void MiscStatManager::Revert(BGSLoadGameBuffer* apLoadGameBuffer) {
    for (uint32_t i = 0; i < 43; ++i) {
        if (StatList[i]) {
            StatList[i]->iVal = 0;
        }
    }
}

// Releases every statistic.
void MiscStatManager::Destruct([[maybe_unused]] MiscStatManager* apThis) {
    MiscStatManager::ReleaseMemory();
}

void MiscStatManager::ReleaseMemory() {
    for (int32_t i = 0; i < 43; ++i) {
        if (MiscStatManager::StatList[i]) {
            MiscStat* v0 = MiscStatManager::StatList[i];
            if (v0) {
                delete v0;
            }
        }
    }
    ourMemset(MiscStatManager::StatList, 0, sizeof(MiscStatManager::StatList));
}

void MiscStatManager::SetAll(const int32_t* apStats) {
    for (int32_t i = 0; i < 43; ++i) {
        MiscStatManager::StatList[i]->iVal = apStats[i];
    }
}

void MiscStatManager::GetAll(int32_t* apStats) {
    for (int32_t i = 0; i < 43; ++i) {
        apStats[i] = MiscStatManager::StatList[i]->iVal;
    }
}

MiscStatManager::MiscStatID& operator++(MiscStatManager::MiscStatID& arID) {
    arID = static_cast<MiscStatManager::MiscStatID>(arID + 1);
    return arID;
}

int32_t GetMiscStatIDFromName(char* a1) {
    MiscStatManager::MiscStatID eID;
    // The compare is signed and the loop steps with operator++.
    for (eID = MiscStatManager::MS_QUESTS_COMPLETED; eID < 43; ++eID) {
        if (MiscStatManager::StatNames[eID] && !BSstricmp(MiscStatManager::StatNames[eID], a1)) {
            return eID;
        }
    }
    Warning("MENUS: Miscellaneous stat %s could not be found.  Returning MS_COUNT.", a1);
    return 43;
}
