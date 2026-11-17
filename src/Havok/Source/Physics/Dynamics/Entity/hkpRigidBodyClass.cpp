#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityClass.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"

// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
static const void* getVtablehkpRigidBody();

hkClass hkpRigidBodyClass("hkpRigidBody", &hkpEntityClass, 544, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, nullptr, 0,
	0);
hkTypeInfo hkpRigidBodyTypeInfo = {"hkpRigidBody", finishLoadedObjecthkpRigidBody, cleanupLoadedObjecthkpRigidBody,
	getVtablehkpRigidBody(), &typeid(hkpRigidBody), sizeof(hkpRigidBody)};

// The SDK's getVtable: hkpRigidBody's finish constructor (flag 0), inlined, on a 16-aligned stack object: its base
// hkpEntity's finish constructor; the exe drops the rigid body's own vptr store and returns its vtable's address.
// The local is an hkpEntity (the same 0x220 bytes): what the exe constructs in it is only that base.
static const void* getVtablehkpRigidBody() {
	return hkGetVtableOf<hkpRigidBody>();
}

void cleanupLoadedObjecthkpRigidBody(void* p) {
	static_cast<hkpRigidBody*>(p)->~hkpRigidBody();
}

// The type info's finish-loaded-object callback for hkpRigidBody: the null check, then the rigid body's finish
// constructor, which the exe inlines (cdecl, 2 stack arguments, 0xD49530).
void finishLoadedObjecthkpRigidBody(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkpRigidBody(flag);
	}
}

const hkClass& hkpRigidBody::staticClass() {
	return hkpRigidBodyClass;
}
