#include "BSCore/RTTI.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpBoxMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"

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

// The SDK's finishLoadedObject for hkpBoxMotion: its finish constructor (0xC9D1C0) run in place, when there is an
// object; the exe inlines that constructor here (cdecl, 0xE00450).
void finishLoadedObjecthkpBoxMotion(void* p, int32_t finishing) {
	if (p) {
		::new (p) hkpBoxMotion(finishing);
	}
}
