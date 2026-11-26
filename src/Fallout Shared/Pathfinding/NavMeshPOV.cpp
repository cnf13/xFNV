#include "Fallout Shared/Pathfinding/NavMeshPOV.h"

#include "BSCore/NewOverloads.h"

void NavMeshPOV::operator delete(void* apMemory) {
    ourFree(apMemory);
}
