#include "Fallout Shared/Pathfinding/NavMeshPOV.h"

#include "BSCore/MemoryManager.h"

void NavMeshPOV::operator delete(void* apMemory) {
    ourFree(apMemory);
}
