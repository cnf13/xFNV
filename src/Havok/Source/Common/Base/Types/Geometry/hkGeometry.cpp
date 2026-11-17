#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

static void cleanupLoadedObjecthkGeometryTriangle(void* p);
static void finishLoadedObjecthkGeometry(void* p, int32_t finishing);
static void cleanupLoadedObjecthkGeometry(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkGeometryTriangleTypeInfo = {"hkGeometryTriangle", nullptr, cleanupLoadedObjecthkGeometryTriangle, nullptr,
	&typeid(hkGeometry::Triangle), sizeof(hkGeometry::Triangle)};
hkTypeInfo hkGeometryTypeInfo = {"hkGeometry", finishLoadedObjecthkGeometry, cleanupLoadedObjecthkGeometry, nullptr,
	&typeid(hkGeometry), sizeof(hkGeometry)};

// The member tables of the two class records (the SDK's file-local hk<Class>Class_Members of hkGeometryClass.cpp).
using hkGeometryTriangleClassMemberTable = const hkClassMember[4];
static hkGeometryTriangleClassMemberTable hkGeometryTriangleClass_Members{
	{"a", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
	{"b", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x4, nullptr},
	{"c", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x8, nullptr},
	{"material", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0xC, nullptr}};
using hkGeometryClassMemberTable = const hkClassMember[2];
static hkGeometryClassMemberTable hkGeometryClass_Members{
	{"vertices", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_VECTOR4}, 0, 0, 0x0, nullptr},
	{"triangles", &hkGeometryTriangleClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_STRUCT}, 0, 0,
		0xC, nullptr}};
hkClass hkGeometryTriangleClass("hkGeometryTriangle", nullptr, 16, nullptr, 0, nullptr, 0,
	hkGeometryTriangleClass_Members, 4, nullptr, nullptr, 0, 0);
hkClass hkGeometryClass("hkGeometry", nullptr, 24, nullptr, 0, nullptr, 0, hkGeometryClass_Members, 2, nullptr, nullptr,
	0, 0);

static void cleanupLoadedObjecthkGeometryTriangle(void* p) {
	;
}

static void finishLoadedObjecthkGeometry(void* p, int32_t finishing) {
	;
}

static void cleanupLoadedObjecthkGeometry(void* p) {
	static_cast<hkGeometry*>(p)->~hkGeometry();
}

const hkClass& hkGeometry::Triangle::staticClass() {
	return hkGeometryTriangleClass;
}

const hkClass& hkGeometry::Triangle::staticClass_00DFE1E0() {
	return hkClassClass;
}

// The vertex and triangle arrays start empty and free themselves.
hkGeometry::hkGeometry() {}

hkGeometry::~hkGeometry() {}

const hkClass& hkGeometry::staticClass() {
	return hkGeometryClass;
}

const hkClass& hkGeometry::staticClass_00DFE1F0() {
	return hkBitFieldClass;
}
