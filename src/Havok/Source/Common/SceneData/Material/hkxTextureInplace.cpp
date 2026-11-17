#include "Havok/Source/Common/SceneData/Material/hkxTextureInplace.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"

static void cleanupLoadedObjecthkxTextureInplace(void* p);

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkxTextureInplace, as the exe's .rdata holds them.
using hkxTextureInplaceClass_MembersArray = const hkClassMember[4];
static hkxTextureInplaceClass_MembersArray hkxTextureInplaceClass_Members{
	{"fileType", nullptr, nullptr, hkClassMember::TYPE_CHAR, hkClassMember::TYPE_VOID, 4, 0, 0x8, nullptr},
	{"data", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT8, 0, 0, 0xC, nullptr},
	{"name", nullptr, nullptr, hkClassMember::TYPE_STRINGPTR, hkClassMember::TYPE_VOID, 0, 0, 0x18, nullptr},
	{"originalFilename", nullptr, nullptr, hkClassMember::TYPE_STRINGPTR, hkClassMember::TYPE_VOID, 0, 0, 0x1C,
		nullptr}};
hkClass hkxTextureInplaceClass("hkxTextureInplace", &hkReferencedObjectClass, 0x20, nullptr, 0, nullptr, 0,
	hkxTextureInplaceClass_Members, 4, nullptr, nullptr, 0, 1);
hkTypeInfo hkxTextureInplaceTypeInfo = {"hkxTextureInplace", finishLoadedObjecthkxTextureInplace,
	cleanupLoadedObjecthkxTextureInplace, getVtablehkxTextureInplace(), &typeid(hkxTextureInplace),
	sizeof(hkxTextureInplace)};

static void cleanupLoadedObjecthkxTextureInplace(void* p) {
	static_cast<hkxTextureInplace*>(p)->~hkxTextureInplace();
}

// The members' destructors release the strings and free m_data.
hkxTextureInplace::~hkxTextureInplace() {}

const hkClass& hkxTextureInplace::staticClass() {
	return hkxTextureInplaceClass;
}
