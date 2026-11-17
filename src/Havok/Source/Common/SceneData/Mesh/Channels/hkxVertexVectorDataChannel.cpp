#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxVertexVectorDataChannel.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"

// The reflection data of hkxVertexVectorDataChannel.

using hkxVertexVectorDataChannelClass_MembersArray = const hkClassMember[1];
static hkxVertexVectorDataChannelClass_MembersArray hkxVertexVectorDataChannelClass_Members{
    {"perVertexVectors",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_VECTOR4},
     0,
     0,
     0x8,
     nullptr}
};

hkClass hkxVertexVectorDataChannelClass(
    "hkxVertexVectorDataChannel",
    &hkReferencedObjectClass,
    0x14,
    nullptr,
    0,
    nullptr,
    0,
    hkxVertexVectorDataChannelClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    1
);

hkTypeInfo hkxVertexVectorDataChannelTypeInfo = {
    "hkxVertexVectorDataChannel",
    finishLoadedObjecthkxVertexVectorDataChannel,
    cleanupLoadedObjecthkxVertexVectorDataChannel,
    getVtablehkxVertexVectorDataChannel(),
    &typeid(hkxVertexVectorDataChannel),
    sizeof(hkxVertexVectorDataChannel)
};
