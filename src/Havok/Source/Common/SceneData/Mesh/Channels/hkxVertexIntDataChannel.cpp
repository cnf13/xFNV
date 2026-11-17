#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxVertexIntDataChannel.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"

// The reflection data of hkxVertexIntDataChannel (the PDB's module hkxVertexIntDataChannelClass.cpp; the SDK has no
// .cpp for it, so this file takes the header's stem).

// The PDB's name. The reflected member, as the exe's .rdata holds it.
using hkxVertexIntDataChannelClass_MembersArray = const hkClassMember[1];
static hkxVertexIntDataChannelClass_MembersArray hkxVertexIntDataChannelClass_Members{
	{"perVertexInts", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_INT32}, 0, 0, 0x8, nullptr}};

hkClass hkxVertexIntDataChannelClass("hkxVertexIntDataChannel", &hkReferencedObjectClass, 0x14, nullptr, 0, nullptr, 0,
	hkxVertexIntDataChannelClass_Members, 1, nullptr, nullptr, 0, 1);

hkTypeInfo hkxVertexIntDataChannelTypeInfo = {"hkxVertexIntDataChannel", finishLoadedObjecthkxVertexIntDataChannel,
	cleanupLoadedObjecthkxVertexIntDataChannel, getVtablehkxVertexIntDataChannel(), &typeid(hkxVertexIntDataChannel),
	sizeof(hkxVertexIntDataChannel)};
