#include "Havok/Source/Physics/Dynamics/Phantom/hkpCachingShapePhantomClass.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpCachingShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"

hkpCachingShapePhantom::MembersArray hkpCachingShapePhantom::Members{
	{"collisionDetails", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, 0x160, nullptr},
	{"orderDirty", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, 0x16C, nullptr}};

hkClass hkpCachingShapePhantomClass("hkpCachingShapePhantom", &hkpShapePhantomClass, 368, nullptr, 0, nullptr, 0,
	hkpCachingShapePhantom::Members, 2, nullptr, nullptr, 0, 0);
hkTypeInfo hkpCachingShapePhantomTypeInfo = {"hkpCachingShapePhantom", finishLoadedObjecthkpCachingShapePhantom,
	cleanupLoadedObjecthkpCachingShapePhantom, getVtablehkpCachingShapePhantom(), &typeid(hkpCachingShapePhantom),
	sizeof(hkpCachingShapePhantom)};

// The reflection's getVtable for hkpCachingShapePhantom: runs its finish constructor (flag 0) on a 16-aligned stack
// object and returns the vtable that constructor stores (the exe reads it back from the object's first word). The
// vtable is the exe's untyped data at its address, so the integer address is cast to a pointer.
const void* getVtablehkpCachingShapePhantom() {
	return hkGetVtableOf<hkpCachingShapePhantom>();
}

// The class registry's finish-loaded-object callback for a caching shape phantom, which the PDB places in
// .\Phantom\hkpCachingShapePhantomClass.cpp: the null check, then the class's finish constructor out of line
// (cdecl, 2 stack arguments, 0xE00390).
void finishLoadedObjecthkpCachingShapePhantom(void* p, int32_t finishing) {
	if (!p) {
		return;
	}
	hkFinishLoadedObjectFlag flag;
	flag.m_finishing = finishing;
	::new (p) hkpCachingShapePhantom(flag);
}

void cleanupLoadedObjecthkpCachingShapePhantom(void* p) {
	static_cast<hkpCachingShapePhantom*>(p)->~hkpCachingShapePhantom();
}

const hkClass& hkpCachingShapePhantom::staticClass() {
	return hkpCachingShapePhantomClass;
}

const hkClass& hkpCachingShapePhantom::staticClass_00DFD250() {
	return hkpConvexVerticesShapeClass;
}
