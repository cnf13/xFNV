#include "BSSystemUtilities/BSSteamAwardsSystemUtility.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "SDK/Steam/isteamuserstats.h"

#include <cstdint>

char return_0_1() {
    return 0;
}

BSSteamAwardsSystemUtility::BSSteamAwardsSystemUtility() {}

// m_list is emptied by BSSimpleList's own destructor.
BSSteamAwardsSystemUtility::~BSSteamAwardsSystemUtility() {}

bool BSSteamAwardsSystemUtility::Unlock(int32_t aiAchievement) {
    int32_t* pItem;
    int32_t* pID = static_cast<int32_t*>(MemoryManager_Singleton.Allocate(sizeof(int32_t)));
    if (pID) {
        *pID = aiAchievement;
        pItem = pID;
    }
    else {
        pItem = nullptr;
    }
    m_list.AddTail(pItem);
    return true;
}

// Each queued ID is freed before the range test, and an out-of-range ID does not advance to the next node, as the game
// does.
void BSSteamAwardsSystemUtility::Idle() {
    if (m_list.GetNext() || m_list.m_item) {
        BSSimpleList<int32_t*>* pNode = &m_list;
        while (pNode) {
            if (!pNode->GetNext() && !pNode->m_item) {
                break;
            }
            int32_t* pID = pNode->m_item;
            int32_t iAchievement = *pID;
            MemoryManager_Singleton.Deallocate(pID);
            if (iAchievement <= 100) {
                if (SteamUserStats()) {
                    char acName[4];
                    BSsprintf(acName, 4, "A%02d", iAchievement);
                    acName[3] = 0;
                    SteamUserStats()->SetAchievement(acName);
                }
                pNode = pNode->GetNext();
            }
            else {
                Warning("Achievement %d is out of range and will not be awarded!", iAchievement);
            }
        }
        if (SteamUserStats()) {
            SteamUserStats()->StoreStats();
        }
        m_list.RemoveAll();
    }
}

BSSteamAwardsSystemUtility* BSSteamAwardsSystemUtility::QInstance() {
    if (!BSSteamAwardsSystemUtility::m_pInstance) {
        BSSteamAwardsSystemUtility* v1;
        BSSteamAwardsSystemUtility* apMemory = static_cast<BSSteamAwardsSystemUtility*>(
            MemoryManager_Singleton.Allocate(sizeof(BSSteamAwardsSystemUtility))
        );
        if (apMemory) {
            v1 = ::new (apMemory) BSSteamAwardsSystemUtility;
        }
        else {
            v1 = nullptr;
        }
        BSSteamAwardsSystemUtility::m_pInstance = v1;
    }
    return BSSteamAwardsSystemUtility::m_pInstance;
}

BSSteamAwardsSystemUtility* BSSteamAwardsSystemUtility::Initialize() {
    return BSSteamAwardsSystemUtility::QInstance();
}

// The utility is freed with MemoryManager::Deallocate after its destructor (DestroySteamAwardsUtility).
void BSSteamAwardsSystemUtility::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}
