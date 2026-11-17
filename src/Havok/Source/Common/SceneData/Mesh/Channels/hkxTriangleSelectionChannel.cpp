#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxTriangleSelectionChannel.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"

// The reflection data of hkxTriangleSelectionChannel (the PDB's module hkxTriangleSelectionChannelClass.cpp; the SDK
// has no .cpp for it, so this file takes the header's stem).

// The PDB's name. The reflected member, as the exe's .rdata holds it.
using hkxTriangleSelectionChannelClass_MembersArray = const hkClassMember[1];
static hkxTriangleSelectionChannelClass_MembersArray hkxTriangleSelectionChannelClass_Members{{"selectedTriangles",
	nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_INT32}, 0, 0, 0x8, nullptr}};

hkClass hkxTriangleSelectionChannelClass("hkxTriangleSelectionChannel", &hkReferencedObjectClass, 0x14, nullptr, 0,
	nullptr, 0, hkxTriangleSelectionChannelClass_Members, 1, nullptr, nullptr, 0, 1);

hkTypeInfo hkxTriangleSelectionChannelTypeInfo = {"hkxTriangleSelectionChannel",
	finishLoadedObjecthkxTriangleSelectionChannel, cleanupLoadedObjecthkxTriangleSelectionChannel,
	getVtablehkxTriangleSelectionChannel(), &typeid(hkxTriangleSelectionChannel), sizeof(hkxTriangleSelectionChannel)};
