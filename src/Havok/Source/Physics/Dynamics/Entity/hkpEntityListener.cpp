#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"

#include "BSCore/NewOverloads.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

void hkpEntityListener::entityShapeSetCallback(hkpEntity* entity) {
    if (entity->hkpWorldObject::getWorld()) {
        entityRemovedCallback(entity);
        entityAddedCallback(entity);
    }
}

hkpEntityListener::hkpEntityListener() {}

void hkpEntityListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}
