#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/Mesh/hkpNamedMeshMaterial.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

static void finishLoadedObjecthkpNamedMeshMaterial(void* p, int32_t finishing);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpNamedMeshMaterialTypeInfo = {"hkpNamedMeshMaterial", finishLoadedObjecthkpNamedMeshMaterial,
	cleanupLoadedObjecthkpNamedMeshMaterial, nullptr, &typeid(hkpNamedMeshMaterial), sizeof(hkpNamedMeshMaterial)};

// The reflection's member table (the PDB's file-local data).
using hkpNamedMeshMaterialClass_MembersArray = const hkClassMember[1];
static hkpNamedMeshMaterialClass_MembersArray hkpNamedMeshMaterialClass_Members{
	{"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4, nullptr}};
hkClass hkpNamedMeshMaterialClass("hkpNamedMeshMaterial", &hkpMeshMaterialClass, 8, nullptr, 0, nullptr, 0,
	hkpNamedMeshMaterialClass_Members, 1, nullptr, nullptr, 0, 0);

static void finishLoadedObjecthkpNamedMeshMaterial(void* p, int32_t finishing) {
	if (p) {
		::new (static_cast<hkStringPtr*>(p) + 1) hkStringPtr(static_cast<hkFinishLoadedObjectFlag>(finishing));
	}
}

// The exe inlines ~hkpNamedMeshMaterial: only m_name's destructor (0xCB0C00) runs.
void cleanupLoadedObjecthkpNamedMeshMaterial(void* p) {
	static_cast<hkpNamedMeshMaterial*>(p)->m_name.~hkStringPtr();
}
