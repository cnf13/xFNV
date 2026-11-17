#include "Havok/Source/Common/SceneData/Mesh/Channels/hkxVertexFloatDataChannel.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"

// The reflection data of hkxVertexFloatDataChannel (the PDB's module hkxVertexFloatDataChannelClass.cpp; the SDK has
// no .cpp for it, so this file takes the header's stem).

// The PDB's names. The reflected enum of hkxVertexFloatDataChannel, as the exe's .rdata holds it.
using hkxVertexFloatDataChannelVertexFloatDimensionsEnumItemsArray = const hkClassEnum::Item[3];
static hkxVertexFloatDataChannelVertexFloatDimensionsEnumItemsArray
	hkxVertexFloatDataChannelVertexFloatDimensionsEnumItems{{hkxVertexFloatDataChannel::FLOAT, "FLOAT"},
		{hkxVertexFloatDataChannel::DISTANCE, "DISTANCE"}, {hkxVertexFloatDataChannel::ANGLE, "ANGLE"}};
using hkxVertexFloatDataChannelEnumsArray = const hkClassEnum[1];
static hkxVertexFloatDataChannelEnumsArray hkxVertexFloatDataChannelEnums{
	{"VertexFloatDimensions", hkxVertexFloatDataChannelVertexFloatDimensionsEnumItems, 3, nullptr, {0}}};
// The PDB's name. The class's enum, by pointer (.data).
static const hkClassEnum* hkxVertexFloatDataChannelVertexFloatDimensionsEnum = &hkxVertexFloatDataChannelEnums[0];
// The PDB's name. The class record's member table (.data): the dimensions member names the enum record the pointer
// above holds; the exe's image holds null there and the array's dynamic initializer stores the pointer.
using hkxVertexFloatDataChannelClass_MembersArray = hkClassMember[2];
static hkxVertexFloatDataChannelClass_MembersArray hkxVertexFloatDataChannelClass_Members{
	{"perVertexFloats", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_REAL}, 0, {0}, 0x8,
		nullptr},
	{"dimensions", nullptr, &hkxVertexFloatDataChannelEnums[0], {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_UINT8},
		0, {0}, 0x14, nullptr}};

hkClass hkxVertexFloatDataChannelClass("hkxVertexFloatDataChannel", &hkReferencedObjectClass, 24, nullptr, 0,
	hkxVertexFloatDataChannelEnums, 1, hkxVertexFloatDataChannelClass_Members, 2, nullptr, nullptr, 0, 1);

hkTypeInfo hkxVertexFloatDataChannelTypeInfo = {"hkxVertexFloatDataChannel",
	finishLoadedObjecthkxVertexFloatDataChannel, cleanupLoadedObjecthkxVertexFloatDataChannel,
	getVtablehkxVertexFloatDataChannel(), &typeid(hkxVertexFloatDataChannel), sizeof(hkxVertexFloatDataChannel)};
