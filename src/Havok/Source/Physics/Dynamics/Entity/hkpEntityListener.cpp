#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"

#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

void hkpEntityListener::entityShapeSetCallback(hkpEntity* entity) {
    if (entity->hkpWorldObject::getWorld()) {
        entityRemovedCallback(entity);
        entityAddedCallback(entity);
    }
}
