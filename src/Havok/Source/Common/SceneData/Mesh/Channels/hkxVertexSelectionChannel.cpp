#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxVertexSelectionChannel.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"

// The reflection data of hkxVertexSelectionChannel (the PDB's module hkxVertexSelectionChannelClass.cpp; the SDK has
// no .cpp for it, so this file takes the header's stem).

// The PDB's name. The reflected member, as the exe's .rdata holds it.
using hkxVertexSelectionChannelClass_MembersArray = const hkClassMember[1];
static hkxVertexSelectionChannelClass_MembersArray hkxVertexSelectionChannelClass_Members{{"selectedVertices", nullptr,
	nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_INT32}, 0, 0, 0x8, nullptr}};

hkClass hkxVertexSelectionChannelClass("hkxVertexSelectionChannel", &hkReferencedObjectClass, 0x14, nullptr, 0, nullptr,
	0, hkxVertexSelectionChannelClass_Members, 1, nullptr, nullptr, 0, 1);

hkTypeInfo hkxVertexSelectionChannelTypeInfo = {"hkxVertexSelectionChannel",
	finishLoadedObjecthkxVertexSelectionChannel, cleanupLoadedObjecthkxVertexSelectionChannel,
	getVtablehkxVertexSelectionChannel(), &typeid(hkxVertexSelectionChannel), sizeof(hkxVertexSelectionChannel)};
