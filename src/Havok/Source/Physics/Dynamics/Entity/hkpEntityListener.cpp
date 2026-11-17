#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"

#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

// Nothing of this class is reconstructed yet: it is a vtable-only interface, its destructor is never replaced, and
// its two callbacks are reached through the vtable. This translation unit exists so the header is compiled by the
// syntax checker and so a future function of the class has a home.

void hkpEntityListener::entityShapeSetCallback(hkpEntity* entity) {
	if (entity->hkpWorldObject::getWorld()) {
		entityRemovedCallback(entity);
		entityAddedCallback(entity);
	}
}
