#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxVertexSelectionChannel.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"

// The reflection data of hkxVertexSelectionChannel.

using hkxVertexSelectionChannelClass_MembersArray = const hkClassMember[1];
static hkxVertexSelectionChannelClass_MembersArray hkxVertexSelectionChannelClass_Members{{"selectedVertices", nullptr,
	nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_INT32}, 0, 0, 0x8, nullptr}};

hkClass hkxVertexSelectionChannelClass("hkxVertexSelectionChannel", &hkReferencedObjectClass, 0x14, nullptr, 0, nullptr,
	0, hkxVertexSelectionChannelClass_Members, 1, nullptr, nullptr, 0, 1);

hkTypeInfo hkxVertexSelectionChannelTypeInfo = {"hkxVertexSelectionChannel",
	finishLoadedObjecthkxVertexSelectionChannel, cleanupLoadedObjecthkxVertexSelectionChannel,
	getVtablehkxVertexSelectionChannel(), &typeid(hkxVertexSelectionChannel), sizeof(hkxVertexSelectionChannel)};
