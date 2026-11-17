#include "Havok/Source/Physics/Dynamics/World/Memory/Default/hkpDefaultWorldMemoryWatchDog.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

hkpDefaultWorldMemoryWatchDog::MembersArray hkpDefaultWorldMemoryWatchDog::Members{
    {"freeHeapMemoryRequested",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x8,
     nullptr}
};

hkClass hkpDefaultWorldMemoryWatchDogClass(
    "hkpDefaultWorldMemoryWatchDog",
    &hkWorldMemoryAvailableWatchDogClass,
    12,
    nullptr,
    0,
    nullptr,
    0,
    hkpDefaultWorldMemoryWatchDog::Members,
    1,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkpDefaultWorldMemoryWatchDogTypeInfo = {
    "hkpDefaultWorldMemoryWatchDog",
    finishLoadedObjecthkpDefaultWorldMemoryWatchDog,
    cleanupLoadedObjecthkpDefaultWorldMemoryWatchDog,
    getVtablehkpDefaultWorldMemoryWatchDog(),
    &typeid(hkpDefaultWorldMemoryWatchDog),
    sizeof(hkpDefaultWorldMemoryWatchDog)
};

char getMoppInstruction(char a1) {
    return a1;
}

// Returns r_eax shifted right by 8 (a logical shift).
uint32_t getMoppOffset(uint32_t r_eax) {
    return r_eax >> 8;
}

// Replaces *r_eax's low byte with a1 and returns r_eax.
int32_t* setLowByte(uint8_t a1, int32_t* r_eax) {
    *r_eax = (*r_eax & ~0xFF) | a1;
    return r_eax;
}

// Keeps *a1's low byte and stores r_eax above it.
int32_t setUpperBits(int32_t* a1, int32_t r_eax) {
    int32_t result = r_eax << 8;
    *a1 = result | (*a1 & 0xFF);
    return result;
}

hkpDefaultWorldMemoryWatchDog::hkpDefaultWorldMemoryWatchDog(int32_t minMemoryAvailable)
  : m_freeHeapMemoryRequested(minMemoryAvailable) {}

hkpDefaultWorldMemoryWatchDog::~hkpDefaultWorldMemoryWatchDog() {}

int32_t hkpDefaultWorldMemoryWatchDog::getAmountOfFreeHeapMemoryRequested() {
    return m_freeHeapMemoryRequested;
}

// Walks the world's islands, active ones from the last down, then the inactive ones, removing the entities whose
// auto-remove level is at least the current minimum, until the heap can give the requested amount. Each round's
// minimum is the highest level the previous round found below its minimum; a round that finds none ends the search.
void hkpDefaultWorldMemoryWatchDog::freeHeapMemoryTillRequestedAmountIsAvailable(hkpWorld* world) {
    int32_t freeHeapMemoryRequested = m_freeHeapMemoryRequested;
    int32_t minAutoRemoveLevel = 127;
    int32_t maxFoundAutoRemoveLevel;
    do {
        maxFoundAutoRemoveLevel = 0;
        // A negative index i selects inactive island -i - 1.
        for (int32_t i = world->m_activeSimulationIslands.m_size - 1; i >= -world->m_inactiveSimulationIslands.m_size;
             --i) {
            if (i >= world->m_activeSimulationIslands.m_size) {
                i = world->m_activeSimulationIslands.m_size - 1;
                if (i < -world->m_inactiveSimulationIslands.m_size) {
                    return;
                }
            }
            hkpSimulationIsland* island =
                i >= 0 ? world->m_activeSimulationIslands.m_data[i] : world->m_inactiveSimulationIslands.m_data[-i - 1];
            removeObjectsFromIsland(island, minAutoRemoveLevel, freeHeapMemoryRequested, maxFoundAutoRemoveLevel);
            if (hkMemoryRouter::getInstance().m_heap->canAllocTotal(freeHeapMemoryRequested)) {
                return;
            }
        }
        minAutoRemoveLevel = maxFoundAutoRemoveLevel;
    }
    while (maxFoundAutoRemoveLevel > 0);
}

// Removes the island's first entity from its world. The result of removeEntity is ignored.
void hkpDefaultWorldMemoryWatchDog::reduceConstraintsInIsland(hkpSimulationIsland* island, int32_t solverBufferSize) {
    hkBool result;
    island->getWorld()->removeEntity(&result, *island->m_entities.begin());
}

const hkClass& hkpDefaultWorldMemoryWatchDog::staticClass() {
    return hkpDefaultWorldMemoryWatchDogClass;
}

void hkpDefaultWorldMemoryWatchDog::removeObjectsFromIsland(
    hkpSimulationIsland* island,
    int32_t currentAutoRemoveLevel,
    int32_t freeHeapMemoryRequested,
    int32_t& maxFoundAutoRemoveLevelOut
) {
    hkpSimulationIsland* v4 = island;
    int32_t v5 = island->m_entities.m_size - 1;
    if (v5 >= 0) {
        int32_t* v6 = &maxFoundAutoRemoveLevelOut;
        do {
            int32_t m_autoRemoveLevel = v4->m_entities.m_data[v5]->m_autoRemoveLevel;
            if (m_autoRemoveLevel < currentAutoRemoveLevel) {
                int32_t v10 = *v6;
                if (*v6 <= m_autoRemoveLevel) {
                    v10 = v4->m_entities.m_data[v5]->m_autoRemoveLevel;
                }
                *v6 = v10;
            }
            else {
                hkBool removed;
                v4->getWorld()->removeEntity(&removed, v4->m_entities.m_data[v5]);
                hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
                if (!Instance->m_heap->canAllocTotal(freeHeapMemoryRequested)) {
                    return;
                }
            }
            --v5;
        }
        while (v5 >= 0);
    }
}
