#include "Havok/Source/Common/GeometryUtilities/Mesh/hkMeshVertexBuffer.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"

// The reflection data of hkMeshVertexBuffer.

// The reflected enums of hkMeshVertexBuffer.
using hkMeshVertexBufferFlagsEnumItemsArray = const hkClassEnum::Item[5];
static hkMeshVertexBufferFlagsEnumItemsArray hkMeshVertexBufferFlagsEnumItems{
    {hkMeshVertexBuffer::ACCESS_READ, "ACCESS_READ"},
    {hkMeshVertexBuffer::ACCESS_WRITE, "ACCESS_WRITE"},
    {hkMeshVertexBuffer::ACCESS_READ_WRITE, "ACCESS_READ_WRITE"},
    {hkMeshVertexBuffer::ACCESS_WRITE_DISCARD, "ACCESS_WRITE_DISCARD"},
    {hkMeshVertexBuffer::ACCESS_ELEMENT_ARRAY, "ACCESS_ELEMENT_ARRAY"}
};
using hkMeshVertexBufferLockResultEnumItemsArray = const hkClassEnum::Item[3];
static hkMeshVertexBufferLockResultEnumItemsArray hkMeshVertexBufferLockResultEnumItems{
    {hkMeshVertexBuffer::RESULT_FAILURE, "RESULT_FAILURE"},
    {hkMeshVertexBuffer::RESULT_SUCCESS, "RESULT_SUCCESS"},
    {hkMeshVertexBuffer::RESULT_IN_USE, "RESULT_IN_USE"}
};
using hkMeshVertexBufferEnumsArray = const hkClassEnum[2];
static hkMeshVertexBufferEnumsArray hkMeshVertexBufferEnums{
    {"Flags", hkMeshVertexBufferFlagsEnumItems, 5, nullptr, 0},
    {"LockResult", hkMeshVertexBufferLockResultEnumItems, 3, nullptr, 0}
};

const hkClassEnum* hkMeshVertexBufferFlagsEnum = &hkMeshVertexBufferEnums[0];
const hkClassEnum* hkMeshVertexBufferLockResultEnum = &hkMeshVertexBufferEnums[1];

hkClass hkMeshVertexBufferClass(
    "hkMeshVertexBuffer",
    &hkReferencedObjectClass,
    8,
    nullptr,
    0,
    hkMeshVertexBufferEnums,
    2,
    nullptr,
    0,
    nullptr,
    nullptr,
    0,
    0
);
