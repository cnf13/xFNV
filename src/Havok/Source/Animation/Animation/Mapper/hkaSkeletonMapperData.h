#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"

class hkaSkeleton;

// What an hkaSkeletonMapper holds: the two skeletons it maps between, the bones mapped one to one and the chains
// mapped as a whole, the bones left out and the mapping of the extracted motion. Not polymorphic.
class hkaSkeletonMapperData {
public:
    // What the mapping is for (held in m_mappingType): a ragdoll's rigid bodies or another skeleton.
    enum MappingType : int32_t {
        HK_RAGDOLL_MAPPING = 0,
        HK_RETARGETING_MAPPING = 1,
    };

    // One bone of skeleton A mapped to one bone of skeleton B.
    struct SimpleMapping {
        // Havok SDK signature (hkaAnimation.lib, tied by target).
        static const hkClass& staticClass();

        int16_t m_boneA;
        int16_t m_boneB;
        hkQsTransform m_aFromBTransform;
    };

    // A chain of bones of skeleton A mapped to a chain of skeleton B.
    struct ChainMapping {
        ChainMapping();

        // Copies the four bones and both transforms.
        hkaSkeletonMapperData::ChainMapping& operator=(const hkaSkeletonMapperData::ChainMapping& other);

        // Havok SDK signature (hkaAnimation.lib, tied by target).
        static const hkClass& staticClass();

        int16_t m_startBoneA;
        int16_t m_endBoneA;
        int16_t m_startBoneB;
        int16_t m_endBoneB;
        hkQsTransform m_startAFromBTransform;
        hkQsTransform m_endAFromBTransform;
    };

    hkaSkeletonMapperData();
    hkaSkeletonMapperData(hkFinishLoadedObjectFlag flag);
    ~hkaSkeletonMapperData();

    // The implicit copy assignment: both skeleton references, the three arrays, the extracted motion mapping, the
    // flag and the mapping type.
    hkaSkeletonMapperData& operator=(const hkaSkeletonMapperData& __that);

    // The reflection class hkaSkeletonMapperDataClass.
    static const hkClass& staticClass();

    hkaSkeleton* m_skeletonA;
    hkaSkeleton* m_skeletonB;
    hkArray<SimpleMapping> m_simpleMappings;
    hkArray<ChainMapping> m_chainMappings;
    hkArray<uint16_t> m_unmappedBones;
    hkQsTransform m_extractedMotionMapping;
    hkBool m_keepUnmappedLocal;
    hkEnum<MappingType, int32_t> m_mappingType;
};

extern hkClass hkaSkeletonMapperDataClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkaSkeletonMapperDataSimpleMappingTypeInfo;
extern hkTypeInfo hkaSkeletonMapperDataChainMappingTypeInfo;
extern hkTypeInfo hkaSkeletonMapperDataTypeInfo;
extern hkClass hkaSkeletonMapperDataSimpleMappingClass;
extern hkClass hkaSkeletonMapperDataChainMappingClass;
