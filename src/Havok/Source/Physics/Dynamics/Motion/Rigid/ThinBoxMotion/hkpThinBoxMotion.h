#pragma once

#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpBoxMotion.h"

void cleanupLoadedObjecthkpThinBoxMotion(void* p);
// The type info's finish-loaded-object callback for hkpThinBoxMotion.
void finishLoadedObjecthkpThinBoxMotion(void* p, int32_t finishing);
// Havok SDK signature (hkpDynamics.lib, tied by reloc,target).
const void* getVtablehkpThinBoxMotion();

// The motion of a thin box: a box motion whose constructor sets the thin box motion type. It overrides no virtual
// (its vtable repeats hkpBoxMotion's entries) and adds no fields.
class hkpThinBoxMotion : public hkpBoxMotion {
public:
	hkpThinBoxMotion(hkFinishLoadedObjectFlag flag);
	hkpThinBoxMotion(const hkVector4& position, const hkQuaternion& rotation);

	// Havok SDK signature (hkpDynamics.lib, tied by target).
	static const hkClass& staticClass();
};

extern hkClass hkpThinBoxMotionClass;
extern hkTypeInfo hkpThinBoxMotionTypeInfo;
