#include "Havok/Source/Common/SceneData/Selection/hkxNodeSelectionSet.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeHolder.h"
#include "Havok/Source/Common/SceneData/Graph/hkxNode.h"

static void cleanupLoadedObjecthkxNodeSelectionSet(void* p);

hkxNodeSelectionSet::MembersArray hkxNodeSelectionSet::Members{
	{"selectedNodes", &hkxNodeClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0, {0}, 0x14,
		nullptr},
	{"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr}};
hkClass hkxNodeSelectionSetClass("hkxNodeSelectionSet", &hkxAttributeHolderClass, 0x24, nullptr, 0, nullptr, 0,
	hkxNodeSelectionSet::Members, 2, nullptr, nullptr, 0, 0);
hkTypeInfo hkxNodeSelectionSetTypeInfo{"hkxNodeSelectionSet", finishLoadedObjecthkxNodeSelectionSet,
	cleanupLoadedObjecthkxNodeSelectionSet, getVtablehkxNodeSelectionSet(), &typeid(hkxNodeSelectionSet),
	sizeof(hkxNodeSelectionSet)};

static void cleanupLoadedObjecthkxNodeSelectionSet(void* p) {
	static_cast<hkxNodeSelectionSet*>(p)->~hkxNodeSelectionSet();
}

// The members' destructors release m_name and the node references and free their array; the base's destroys the
// attribute groups.
hkxNodeSelectionSet::~hkxNodeSelectionSet() {}

const hkClass& hkxNodeSelectionSet::staticClass() {
	return hkxNodeSelectionSetClass;
}
