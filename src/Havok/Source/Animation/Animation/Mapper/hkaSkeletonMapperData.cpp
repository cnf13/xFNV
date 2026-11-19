#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapperData.h"

#include "Havok/Source/Animation/Animation/Animation/SplineCompressed/hkaSplineCompressedAnimation.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"

static void cleanupLoadedObjecthkaSkeletonMapperDataSimpleMapping(void* p);
static void cleanupLoadedObjecthkaSkeletonMapperDataChainMapping(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkaSkeletonMapperDataSimpleMappingTypeInfo = {
    "hkaSkeletonMapperDataSimpleMapping",
    nullptr,
    cleanupLoadedObjecthkaSkeletonMapperDataSimpleMapping,
    nullptr,
    &typeid(hkaSkeletonMapperData::SimpleMapping),
    sizeof(hkaSkeletonMapperData::SimpleMapping)
};
hkTypeInfo hkaSkeletonMapperDataChainMappingTypeInfo = {
    "hkaSkeletonMapperDataChainMapping",
    nullptr,
    cleanupLoadedObjecthkaSkeletonMapperDataChainMapping,
    nullptr,
    &typeid(hkaSkeletonMapperData::ChainMapping),
    sizeof(hkaSkeletonMapperData::ChainMapping)
};
hkTypeInfo hkaSkeletonMapperDataTypeInfo = {
    "hkaSkeletonMapperData",
    finishLoadedObjecthkaSkeletonMapperData,
    cleanupLoadedObjecthkaSkeletonMapperData,
    nullptr,
    &typeid(hkaSkeletonMapperData),
    sizeof(hkaSkeletonMapperData)
};

// The reflection's enum records: the names of MappingType's values, and the enum; then the member tables (file-local
// data).
using hkaSkeletonMapperDataMappingTypeEnumItemsArray = const hkClassEnum::Item[2];
static hkaSkeletonMapperDataMappingTypeEnumItemsArray hkaSkeletonMapperDataMappingTypeEnumItems{
    {0, "HK_RAGDOLL_MAPPING"},
    {1, "HK_RETARGETING_MAPPING"}
};
using hkaSkeletonMapperDataEnumsArray = const hkClassEnum[1];
static hkaSkeletonMapperDataEnumsArray hkaSkeletonMapperDataEnums{
    {"MappingType", hkaSkeletonMapperDataMappingTypeEnumItems, 2, nullptr, {0}}
};
using hkaSkeletonMapperData_SimpleMappingClass_MembersArray = const hkClassMember[3];
static hkaSkeletonMapperData_SimpleMappingClass_MembersArray hkaSkeletonMapperData_SimpleMappingClass_Members{
    {"boneA", nullptr, nullptr, {hkClassMember::TYPE_INT16}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
    {"boneB", nullptr, nullptr, {hkClassMember::TYPE_INT16}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x2, nullptr},
    {"aFromBTransform",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_QSTRANSFORM},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x10,
     nullptr}
};
using hkaSkeletonMapperData_ChainMappingClass_MembersArray = const hkClassMember[6];
static hkaSkeletonMapperData_ChainMappingClass_MembersArray hkaSkeletonMapperData_ChainMappingClass_Members{
    {"startBoneA", nullptr, nullptr, {hkClassMember::TYPE_INT16}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
    {"endBoneA", nullptr, nullptr, {hkClassMember::TYPE_INT16}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x2, nullptr},
    {"startBoneB", nullptr, nullptr, {hkClassMember::TYPE_INT16}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4, nullptr},
    {"endBoneB", nullptr, nullptr, {hkClassMember::TYPE_INT16}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x6, nullptr},
    {"startAFromBTransform",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_QSTRANSFORM},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x10,
     nullptr},
    {"endAFromBTransform",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_QSTRANSFORM},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x40,
     nullptr}
};

static const hkClassEnum* hkaSkeletonMapperDataMappingTypeEnum = &hkaSkeletonMapperDataEnums[0];

// The mappingType entry holds its enum itself.
using hkaSkeletonMapperDataClass_MembersArray = const hkClassMember[8];
static hkaSkeletonMapperDataClass_MembersArray hkaSkeletonMapperDataClass_Members{
    {"skeletonA",
     &hkaSkeletonClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x0,
     nullptr},
    {"skeletonB",
     &hkaSkeletonClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x4,
     nullptr},
    {"simpleMappings",
     &hkaSkeletonMapperDataSimpleMappingClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x8,
     nullptr},
    {"chainMappings",
     &hkaSkeletonMapperDataChainMappingClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x14,
     nullptr},
    {"unmappedBones",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_INT16},
     0,
     {0},
     0x20,
     nullptr},
    {"extractedMotionMapping",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_QSTRANSFORM},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x30,
     nullptr},
    {"keepUnmappedLocal",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_BOOL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x60,
     nullptr},
    {"mappingType",
     nullptr,
     &hkaSkeletonMapperDataEnums[0],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_INT32},
     0,
     {0},
     0x64,
     nullptr}
};
hkClass hkaSkeletonMapperDataSimpleMappingClass(
    "hkaSkeletonMapperDataSimpleMapping",
    nullptr,
    0x40,
    nullptr,
    0,
    nullptr,
    0,
    hkaSkeletonMapperData_SimpleMappingClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkaSkeletonMapperDataChainMappingClass(
    "hkaSkeletonMapperDataChainMapping",
    nullptr,
    0x70,
    nullptr,
    0,
    nullptr,
    0,
    hkaSkeletonMapperData_ChainMappingClass_Members,
    6,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkaSkeletonMapperDataClass(
    "hkaSkeletonMapperData",
    nullptr,
    0x70,
    nullptr,
    0,
    hkaSkeletonMapperDataEnums,
    1,
    hkaSkeletonMapperDataClass_Members,
    8,
    nullptr,
    nullptr,
    0,
    1
);

static void cleanupLoadedObjecthkaSkeletonMapperDataSimpleMapping(void* p) {
    ;
}

static void cleanupLoadedObjecthkaSkeletonMapperDataChainMapping(void* p) {
    ;
}

const hkClass& hkaSkeletonMapperData::SimpleMapping::staticClass() {
    return hkaSkeletonMapperDataSimpleMappingClass;
}

hkaSkeletonMapperData::hkaSkeletonMapperData()
  : m_skeletonA(nullptr), m_skeletonB(nullptr), m_simpleMappings(), m_chainMappings(), m_unmappedBones(),
    m_keepUnmappedLocal(true) {
    m_extractedMotionMapping.m_translation.setZero4();
    m_extractedMotionMapping.m_rotation.m_vec.m_quad = hkQuadReal0001;
    m_extractedMotionMapping.m_scale.setAll(1.0f);
}

// The loaded data stay as loaded: each array is built with its own finish constructor.
hkaSkeletonMapperData::hkaSkeletonMapperData(hkFinishLoadedObjectFlag flag)
  : m_simpleMappings(flag), m_chainMappings(flag), m_unmappedBones(flag) {}

// The three arrays go with their members; the skeletons are released here.
hkaSkeletonMapperData::~hkaSkeletonMapperData() {
    if (m_skeletonB) {
        m_skeletonB->removeReference();
    }
    m_skeletonB = nullptr;
    if (m_skeletonA) {
        m_skeletonA->removeReference();
    }
    m_skeletonA = nullptr;
}

// The implicit copy assignment of the class. Each skeleton is an hkRefPtr: a reference on the new skeleton, the old one
// released, then the pointer stored. Each array goes through hkArray::operator= (copyFromArray): the mapping arrays
// through the class version, which grows the storage, assigns the elements both arrays hold and copy-constructs the
// rest (the placement new's null test included); the unmapped bones through the plain-data version, which reallocates
// only when the storage is too small.
hkaSkeletonMapperData& hkaSkeletonMapperData::operator=(const hkaSkeletonMapperData& __that) {
    if (__that.m_skeletonA) {
        __that.m_skeletonA->addReference();
    }
    if (m_skeletonA) {
        m_skeletonA->removeReference();
    }
    m_skeletonA = __that.m_skeletonA;
    if (__that.m_skeletonB) {
        __that.m_skeletonB->addReference();
    }
    if (m_skeletonB) {
        m_skeletonB->removeReference();
    }
    m_skeletonB = __that.m_skeletonB;

    m_simpleMappings = __that.m_simpleMappings;
    m_chainMappings = __that.m_chainMappings;
    m_unmappedBones = __that.m_unmappedBones;

    m_extractedMotionMapping = __that.m_extractedMotionMapping;
    m_keepUnmappedLocal.m_bool = __that.m_keepUnmappedLocal.m_bool;
    m_mappingType.m_storage = __that.m_mappingType.m_storage;
    return *this;
}

const hkClass& hkaSkeletonMapperData::staticClass() {
    return hkaSkeletonMapperDataClass;
}
