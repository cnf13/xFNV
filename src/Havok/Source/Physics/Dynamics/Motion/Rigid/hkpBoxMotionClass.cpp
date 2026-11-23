#include "BSCore/RTTI.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpBoxMotion.h"

// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
static const void* getVtablehkpBoxMotion();
static void cleanupLoadedObjecthkpBoxMotion(void* p);

hkClass hkpBoxMotionClass("hkpBoxMotion", &hkpMotionClass, 288, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, nullptr, 0,
	0);
hkTypeInfo hkpBoxMotionTypeInfo = {"hkpBoxMotion", finishLoadedObjecthkpBoxMotion, cleanupLoadedObjecthkpBoxMotion,
	getVtablehkpBoxMotion(), &typeid(hkpBoxMotion), sizeof(hkpBoxMotion)};

static const void* getVtablehkpBoxMotion() {
	return hkGetVtableOf<hkpBoxMotion>();
}

static void cleanupLoadedObjecthkpBoxMotion(void* p) {
	static_cast<hkpBoxMotion*>(p)->~hkpBoxMotion();
}

// finishLoadedObject for hkpBoxMotion: runs its finish constructor in place, when there is an object.
void finishLoadedObjecthkpBoxMotion(void* p, int32_t finishing) {
	if (p) {
		::new (p) hkpBoxMotion(finishing);
	}
}
