#include "Havok/Source/Common/SceneData/Attributes/hkxSparselyAnimatedBool.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"

// The class record's member table.
using hkxSparselyAnimatedBoolClassMemberArray = const hkClassMember[2];
static hkxSparselyAnimatedBoolClassMemberArray hkxSparselyAnimatedBoolClass_Members{
	{"bools", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_BOOL}, 0, 0, 0x8, nullptr},
	{"times", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_REAL}, 0, 0, 0x14, nullptr}};
hkClass hkxSparselyAnimatedBoolClass("hkxSparselyAnimatedBool", &hkReferencedObjectClass, 32, nullptr, 0, nullptr, 0,
	hkxSparselyAnimatedBoolClass_Members, 2, nullptr, nullptr, 0, 1);
hkTypeInfo hkxSparselyAnimatedBoolTypeInfo = {"hkxSparselyAnimatedBool", finishLoadedObjecthkxSparselyAnimatedBool,
	cleanupLoadedObjecthkxSparselyAnimatedBool, getVtablehkxSparselyAnimatedBool(), &typeid(hkxSparselyAnimatedBool),
	sizeof(hkxSparselyAnimatedBool)};

// The members free the times and the bools.
hkxSparselyAnimatedBool::~hkxSparselyAnimatedBool() {}

const hkClass& hkxSparselyAnimatedBool::staticClass() {
	return hkxSparselyAnimatedBoolClass;
}
