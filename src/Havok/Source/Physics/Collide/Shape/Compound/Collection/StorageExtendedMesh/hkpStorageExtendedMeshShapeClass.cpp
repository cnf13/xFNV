#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/StorageExtendedMesh/hkpStorageExtendedMeshShapeClass.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShapeClass.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/Mesh/hkpNamedMeshMaterial.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/StorageExtendedMesh/hkpStorageExtendedMeshShape.h"

static void cleanupLoadedObjecthkpStorageExtendedMeshShapeMaterial(void* p);
// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
static const void* getVtablehkpStorageExtendedMeshShape();

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpStorageExtendedMeshShapeMaterialTypeInfo = {"hkpStorageExtendedMeshShapeMaterial", nullptr,
	cleanupLoadedObjecthkpStorageExtendedMeshShapeMaterial, nullptr, &typeid(hkpStorageExtendedMeshShape::Material),
	sizeof(hkpStorageExtendedMeshShape::Material)};

// The reflection's member tables (file-local data, and hkpStorageExtendedMeshShape::Members).
using hkpStorageExtendedMeshShape_MaterialClass_MembersArray = const hkClassMember[3];
static hkpStorageExtendedMeshShape_MaterialClass_MembersArray hkpStorageExtendedMeshShape_MaterialClass_Members{
	{"restitution", nullptr, nullptr, {hkClassMember::TYPE_HALF}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4, nullptr},
	{"friction", nullptr, nullptr, {hkClassMember::TYPE_HALF}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x6, nullptr},
	{"userData", nullptr, nullptr, {hkClassMember::TYPE_ULONG}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr}};
using hkpStorageExtendedMeshShape_MeshSubpartStorageClass_MembersArray = const hkClassMember[8];
static hkpStorageExtendedMeshShape_MeshSubpartStorageClass_MembersArray
	hkpStorageExtendedMeshShape_MeshSubpartStorageClass_Members{
		{"vertices", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_VECTOR4}, 0, {0}, 0x8,
			nullptr},
		{"indices8", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT8}, 0, {0}, 0x14, nullptr},
		{"indices16", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT16}, 0, {0}, 0x20,
			nullptr},
		{"indices32", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT32}, 0, {0}, 0x2C,
			nullptr},
		{"materialIndices", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT8}, 0, {0}, 0x38,
			nullptr},
		{"materials", &hkpStorageExtendedMeshShapeMaterialClass, nullptr, {hkClassMember::TYPE_ARRAY},
			{hkClassMember::TYPE_STRUCT}, 0, {0}, 0x44, nullptr},
		{"namedMaterials", &hkpNamedMeshMaterialClass, nullptr, {hkClassMember::TYPE_ARRAY},
			{hkClassMember::TYPE_STRUCT}, 0, {0}, 0x50, nullptr},
		{"materialIndices16", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT16}, 0, {0}, 0x5C,
			nullptr}};
using hkpStorageExtendedMeshShape_ShapeSubpartStorageClass_MembersArray = const hkClassMember[3];
static hkpStorageExtendedMeshShape_ShapeSubpartStorageClass_MembersArray
	hkpStorageExtendedMeshShape_ShapeSubpartStorageClass_Members{{"materialIndices", nullptr, nullptr,
																	 {hkClassMember::TYPE_ARRAY},
																	 {hkClassMember::TYPE_UINT8}, 0, {0}, 0x8, nullptr},
		{"materials", &hkpStorageExtendedMeshShapeMaterialClass, nullptr, {hkClassMember::TYPE_ARRAY},
			{hkClassMember::TYPE_STRUCT}, 0, {0}, 0x14, nullptr},
		{"materialIndices16", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT16}, 0, {0}, 0x20,
			nullptr}};
hkpStorageExtendedMeshShape::MembersArray hkpStorageExtendedMeshShape::Members{
	{"meshstorage", &hkpStorageExtendedMeshShapeMeshSubpartStorageClass, nullptr, {hkClassMember::TYPE_ARRAY},
		{hkClassMember::TYPE_POINTER}, 0, {0}, 0xF0, nullptr},
	{"shapestorage", &hkpStorageExtendedMeshShapeShapeSubpartStorageClass, nullptr, {hkClassMember::TYPE_ARRAY},
		{hkClassMember::TYPE_POINTER}, 0, {0}, 0xFC, nullptr}};
hkClass hkpStorageExtendedMeshShapeMaterialClass("hkpStorageExtendedMeshShapeMaterial", &hkpMeshMaterialClass, 0xC,
	nullptr, 0, nullptr, 0, hkpStorageExtendedMeshShape_MaterialClass_Members, 3, nullptr, nullptr, 0, 1);
hkClass hkpStorageExtendedMeshShapeMeshSubpartStorageClass("hkpStorageExtendedMeshShapeMeshSubpartStorage",
	&hkReferencedObjectClass, 0x68, nullptr, 0, nullptr, 0, hkpStorageExtendedMeshShape_MeshSubpartStorageClass_Members,
	8, nullptr, nullptr, 0, 3);
hkTypeInfo hkpStorageExtendedMeshShapeMeshSubpartStorageTypeInfo{"hkpStorageExtendedMeshShapeMeshSubpartStorage",
	finishLoadedObjecthkpStorageExtendedMeshShapeMeshSubpartStorage,
	cleanupLoadedObjecthkpStorageExtendedMeshShapeMeshSubpartStorage,
	getVtablehkpStorageExtendedMeshShapeMeshSubpartStorage(), &typeid(hkpStorageExtendedMeshShape::MeshSubpartStorage),
	sizeof(hkpStorageExtendedMeshShape::MeshSubpartStorage)};
hkClass hkpStorageExtendedMeshShapeShapeSubpartStorageClass("hkpStorageExtendedMeshShapeShapeSubpartStorage",
	&hkReferencedObjectClass, 0x2C, nullptr, 0, nullptr, 0,
	hkpStorageExtendedMeshShape_ShapeSubpartStorageClass_Members, 3, nullptr, nullptr, 0, 2);
hkTypeInfo hkpStorageExtendedMeshShapeShapeSubpartStorageTypeInfo{"hkpStorageExtendedMeshShapeShapeSubpartStorage",
	finishLoadedObjecthkpStorageExtendedMeshShapeShapeSubpartStorage,
	cleanupLoadedObjecthkpStorageExtendedMeshShapeShapeSubpartStorage,
	getVtablehkpStorageExtendedMeshShapeShapeSubpartStorage(),
	&typeid(hkpStorageExtendedMeshShape::ShapeSubpartStorage),
	sizeof(hkpStorageExtendedMeshShape::ShapeSubpartStorage)};
hkClass hkpStorageExtendedMeshShapeClass("hkpStorageExtendedMeshShape", &hkpExtendedMeshShapeClass, 0x110, nullptr, 0,
	nullptr, 0, hkpStorageExtendedMeshShape::Members, 2, nullptr, nullptr, 0, 0);
hkTypeInfo hkpStorageExtendedMeshShapeTypeInfo{"hkpStorageExtendedMeshShape",
	finishLoadedObjecthkpStorageExtendedMeshShape, cleanupLoadedObjecthkpStorageExtendedMeshShape,
	getVtablehkpStorageExtendedMeshShape(), &typeid(hkpStorageExtendedMeshShape), sizeof(hkpStorageExtendedMeshShape)};

static void cleanupLoadedObjecthkpStorageExtendedMeshShapeMaterial(void* p) {
	;
}

static const void* getVtablehkpStorageExtendedMeshShape() {
	return hkGetVtableOf<hkpStorageExtendedMeshShape>();
}

// The type info's finish-loaded-object callback for a mesh subpart's storage: the null check, then the storage's finish
// constructor.
void finishLoadedObjecthkpStorageExtendedMeshShapeMeshSubpartStorage(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkpStorageExtendedMeshShape::MeshSubpartStorage(flag);
	}
}

void finishLoadedObjecthkpStorageExtendedMeshShapeShapeSubpartStorage(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkpStorageExtendedMeshShape::ShapeSubpartStorage(flag);
	}
}

void finishLoadedObjecthkpStorageExtendedMeshShape(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkpStorageExtendedMeshShape(flag);
	}
}

void cleanupLoadedObjecthkpStorageExtendedMeshShapeMeshSubpartStorage(void* p) {
	static_cast<hkpStorageExtendedMeshShape::MeshSubpartStorage*>(p)->~MeshSubpartStorage();
}

// The type info's vtable getter for a mesh subpart's storage: finishes a stack object (not finishing) with the finish
// constructor and returns the object's first word, the vptr that constructor stored.
const void* getVtablehkpStorageExtendedMeshShapeMeshSubpartStorage() {
	return hkGetVtableOf<hkpStorageExtendedMeshShape::MeshSubpartStorage>();
}

void cleanupLoadedObjecthkpStorageExtendedMeshShapeShapeSubpartStorage(void* p) {
	static_cast<hkpStorageExtendedMeshShape::ShapeSubpartStorage*>(p)->~ShapeSubpartStorage();
}

// As above, for a shape subpart's storage.
const void* getVtablehkpStorageExtendedMeshShapeShapeSubpartStorage() {
	return hkGetVtableOf<hkpStorageExtendedMeshShape::ShapeSubpartStorage>();
}

void cleanupLoadedObjecthkpStorageExtendedMeshShape(void* p) {
	static_cast<hkpStorageExtendedMeshShape*>(p)->~hkpStorageExtendedMeshShape();
}

const hkClass& hkpStorageExtendedMeshShape::Material::staticClass() {
	return hkpStorageExtendedMeshShapeMaterialClass;
}

const hkClass& hkpStorageExtendedMeshShape::MeshSubpartStorage::staticClass() {
	return hkpStorageExtendedMeshShapeMeshSubpartStorageClass;
}

const hkClass& hkpStorageExtendedMeshShape::ShapeSubpartStorage::staticClass() {
	return hkpStorageExtendedMeshShapeShapeSubpartStorageClass;
}

const hkClass& hkpStorageExtendedMeshShape::staticClass() {
	return hkpStorageExtendedMeshShapeClass;
}
