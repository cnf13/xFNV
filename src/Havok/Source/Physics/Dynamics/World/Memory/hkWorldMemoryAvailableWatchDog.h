#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"

class hkpSimulationIsland;
class hkpWorld;

// Watches the heap for a world and frees memory (removing objects) when the world runs short.
class hkWorldMemoryAvailableWatchDog : public hkReferencedObject {
public:
    // The constructor the derived classes' own use, and the SDK's finish constructor of a loaded object (the derived
    // classes' finish constructors call it): it keeps the fields the load wrote.
    hkWorldMemoryAvailableWatchDog() = default;
    hkWorldMemoryAvailableWatchDog(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag) {}
    ~hkWorldMemoryAvailableWatchDog() override;

    // The heap memory the watch dog wants free.
    virtual int32_t getAmountOfFreeHeapMemoryRequested() = 0;
    // Frees heap memory in world until the requested amount is available.
    virtual void freeHeapMemoryTillRequestedAmountIsAvailable(hkpWorld* world) = 0;
    // Removes constraints and objects of island to reduce its memory use.
    virtual void reduceConstraintsInIsland(hkpSimulationIsland* island, int32_t numIterations) = 0;
};

// The class's reflection record.
extern hkClass hkWorldMemoryAvailableWatchDogClass;
