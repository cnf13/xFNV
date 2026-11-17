#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

// The class records of the reflection types' older layouts: hkClass version 1 and 2, hkClassMember, hkClassEnumItem and
// hkClassEnum. hkClass::updateMetadataInplace reads hkClassVersion1Class's member offsets. The member tables belong as
// static members of hkClassVersion1, hkClassVersion2 and hkClassEnumVersion1, which are not declared, so they are
// file-local here under those names with `_` for `::`.

// The older hkClassMember's enums: its Type items are plain numbers, the older enum's values (its TYPE_MAX is 32, the
// current hkClassMember::TYPE_HALF).
using hkClassMemberVersion1TypeEnumItemsArray = const hkClassEnum::Item[33];
static hkClassMemberVersion1TypeEnumItemsArray hkClassMemberVersion1TypeEnumItems{
    {0, "TYPE_VOID"},         {1, "TYPE_BOOL"},
    {2, "TYPE_CHAR"},         {3, "TYPE_INT8"},
    {4, "TYPE_UINT8"},        {5, "TYPE_INT16"},
    {6, "TYPE_UINT16"},       {7, "TYPE_INT32"},
    {8, "TYPE_UINT32"},       {9, "TYPE_INT64"},
    {10, "TYPE_UINT64"},      {11, "TYPE_REAL"},
    {12, "TYPE_VECTOR4"},     {13, "TYPE_QUATERNION"},
    {14, "TYPE_MATRIX3"},     {15, "TYPE_ROTATION"},
    {16, "TYPE_QSTRANSFORM"}, {17, "TYPE_MATRIX4"},
    {18, "TYPE_TRANSFORM"},   {19, "TYPE_ZERO"},
    {20, "TYPE_POINTER"},     {21, "TYPE_FUNCTIONPOINTER"},
    {22, "TYPE_ARRAY"},       {23, "TYPE_INPLACEARRAY"},
    {24, "TYPE_ENUM"},        {25, "TYPE_STRUCT"},
    {26, "TYPE_SIMPLEARRAY"}, {27, "TYPE_HOMOGENEOUSARRAY"},
    {28, "TYPE_VARIANT"},     {29, "TYPE_CSTRING"},
    {30, "TYPE_ULONG"},       {31, "TYPE_FLAGS"},
    {32, "TYPE_MAX"}
};
using hkClassMemberVersion1FlagsEnumItemsArray = const hkClassEnum::Item[6];
static hkClassMemberVersion1FlagsEnumItemsArray hkClassMemberVersion1FlagsEnumItems{
    {1, "POINTER_OPTIONAL"}, {2, "POINTER_VOIDSTAR"}, {8, "ENUM_8"},
    {16, "ENUM_16"},         {32, "ENUM_32"},         {64, "ARRAY_RAWDATA"}
};
using hkClassMemberVersion1RangeEnumItemsArray = const hkClassEnum::Item[7];
static hkClassMemberVersion1RangeEnumItemsArray hkClassMemberVersion1RangeEnumItems{{0, "INVALID"},   {1, "DEFAULT"},
                                                                                    {2, "ABS_MIN"},   {4, "ABS_MAX"},
                                                                                    {8, "SOFT_MIN"},  {16, "SOFT_MAX"},
                                                                                    {32, "RANGE_MAX"}};
using hkClassMemberVersion1EnumsArray = const hkClassEnum[3];
static hkClassMemberVersion1EnumsArray hkClassMemberVersion1Enums{
    {"Type", hkClassMemberVersion1TypeEnumItems, 33, nullptr, {0}},
    {"Flags", hkClassMemberVersion1FlagsEnumItems, 6, nullptr, {0}},
    {"Range", hkClassMemberVersion1RangeEnumItems, 7, nullptr, {0}}
};
// The enums by pointer.
static const hkClassEnum* hkClassMemberVersion1TypeEnum = &hkClassMemberVersion1Enums[0];
static const hkClassEnum* hkClassMemberVersion1FlagsEnum = &hkClassMemberVersion1Enums[1];
static const hkClassEnum* hkClassMemberVersion1RangeEnum = &hkClassMemberVersion1Enums[2];

using hkClassVersion1_MembersArray = const hkClassMember[7];
static hkClassVersion1_MembersArray hkClassVersion1_Members{
    {"name", nullptr, nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
    {"parent",
     &hkClassVersion1Class,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x4,
     nullptr},
    {"objectSize", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr},
    {"numImplementedInterfaces",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xC,
     nullptr},
    {"declaredEnums",
     &hkClassEnumVersion1Class,
     nullptr,
     {hkClassMember::TYPE_SIMPLEARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x10,
     nullptr},
    {"declaredMembers",
     &hkClassMemberVersion1Class,
     nullptr,
     {hkClassMember::TYPE_SIMPLEARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x18,
     nullptr},
    {"hasVtable", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr}
};
using hkClassVersion2_MembersArray = const hkClassMember[7];
static hkClassVersion2_MembersArray hkClassVersion2_Members{
    {"name", nullptr, nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
    {"parent",
     &hkClassVersion2Class,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x4,
     nullptr},
    {"objectSize", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x8, nullptr},
    {"numImplementedInterfaces",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0xC,
     nullptr},
    {"declaredEnums",
     &hkClassEnumVersion1Class,
     nullptr,
     {hkClassMember::TYPE_SIMPLEARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x10,
     nullptr},
    {"declaredMembers",
     &hkClassMemberVersion1Class,
     nullptr,
     {hkClassMember::TYPE_SIMPLEARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x18,
     nullptr},
    {"defaults", nullptr, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr}
};
using hkClassMemberVersion1Class_MembersArray = hkClassMember[8];
static hkClassMemberVersion1Class_MembersArray hkClassMemberVersion1Class_Members{
    {"name", nullptr, nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
    {"class",
     &hkClassVersion1Class,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x4,
     nullptr},
    {"enum",
     &hkClassEnumVersion1Class,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x8,
     nullptr},
    {"type",
     nullptr,
     &hkClassMemberVersion1Enums[0],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_INT8},
     0,
     {0},
     0xC,
     nullptr},
    {"subtype",
     nullptr,
     &hkClassMemberVersion1Enums[0],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_INT8},
     0,
     {0},
     0xD,
     nullptr},
    {"cArraySize", nullptr, nullptr, {hkClassMember::TYPE_INT16}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xE, nullptr},
    {"flags", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr},
    {"offset", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x12, nullptr}
};
using hkClassEnumVersion1_Item_MembersArray = const hkClassMember[2];
static hkClassEnumVersion1_Item_MembersArray hkClassEnumVersion1_Item_Members{
    {"value", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
    {"name", nullptr, nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4, nullptr}
};
using hkClassEnumVersion1_MembersArray = const hkClassMember[2];
static hkClassEnumVersion1_MembersArray hkClassEnumVersion1_Members{
    {"name", nullptr, nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
    {"items",
     &hkClassEnumItemVersion1Class,
     nullptr,
     {hkClassMember::TYPE_SIMPLEARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x4,
     nullptr}
};

hkClass hkClassVersion1Class(
    "hkClass",
    nullptr,
    36,
    nullptr,
    0,
    nullptr,
    0,
    hkClassVersion1_Members,
    7,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkClassVersion2Class(
    "hkClass",
    nullptr,
    36,
    nullptr,
    0,
    nullptr,
    0,
    hkClassVersion2_Members,
    7,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkClassMemberVersion1Class(
    "hkClassMember",
    nullptr,
    20,
    nullptr,
    0,
    hkClassMemberVersion1Enums,
    3,
    hkClassMemberVersion1Class_Members,
    8,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkClassEnumItemVersion1Class(
    "hkClassEnumItem",
    nullptr,
    8,
    nullptr,
    0,
    nullptr,
    0,
    hkClassEnumVersion1_Item_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkClassEnumVersion1Class(
    "hkClassEnum",
    nullptr,
    12,
    nullptr,
    0,
    nullptr,
    0,
    hkClassEnumVersion1_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
