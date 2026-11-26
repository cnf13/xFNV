#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// The command bytes of MOPP code, as the code generator writes them and the virtual machines read them. Values from
// Havok's release hkpcollide.pdb.
enum hkpMoppCommands : int32_t {
    HK_MOPP_RETURN = 0x0,
    HK_MOPP_SCALE1 = 0x1,
    HK_MOPP_SCALE2 = 0x2,
    HK_MOPP_SCALE3 = 0x3,
    HK_MOPP_SCALE4 = 0x4,
    HK_MOPP_JUMP8 = 0x5,
    HK_MOPP_JUMP16 = 0x6,
    HK_MOPP_JUMP24 = 0x7,
    HK_MOPP_JUMP32 = 0x8,
    HK_MOPP_TERM_REOFFSET8 = 0x9,
    HK_MOPP_TERM_REOFFSET16 = 0xA,
    HK_MOPP_TERM_REOFFSET32 = 0xB,
    HK_MOPP_JUMP_CHUNK = 0xC,
    HK_MOPP_DATA_OFFSET = 0xD,
    HK_MOPP_SPLIT_X = 0x10,
    HK_MOPP_SPLIT_Y = 0x11,
    HK_MOPP_SPLIT_Z = 0x12,
    HK_MOPP_SPLIT_YZ = 0x13,
    HK_MOPP_SPLIT_YMZ = 0x14,
    HK_MOPP_SPLIT_XZ = 0x15,
    HK_MOPP_SPLIT_XMZ = 0x16,
    HK_MOPP_SPLIT_XY = 0x17,
    HK_MOPP_SPLIT_XMY = 0x18,
    HK_MOPP_SPLIT_XYZ = 0x19,
    HK_MOPP_SPLIT_XYMZ = 0x1A,
    HK_MOPP_SPLIT_XMYZ = 0x1B,
    HK_MOPP_SPLIT_XMYMZ = 0x1C,
    HK_MOPP_SINGLE_SPLIT_X = 0x20,
    HK_MOPP_SINGLE_SPLIT_Y = 0x21,
    HK_MOPP_SINGLE_SPLIT_Z = 0x22,
    HK_MOPP_SPLIT_JUMP_X = 0x23,
    HK_MOPP_SPLIT_JUMP_Y = 0x24,
    HK_MOPP_SPLIT_JUMP_Z = 0x25,
    HK_MOPP_DOUBLE_CUT_X = 0x26,
    HK_MOPP_DOUBLE_CUT_Y = 0x27,
    HK_MOPP_DOUBLE_CUT_Z = 0x28,
    HK_MOPP_DOUBLE_CUT24_X = 0x29,
    HK_MOPP_DOUBLE_CUT24_Y = 0x2A,
    HK_MOPP_DOUBLE_CUT24_Z = 0x2B,
    HK_MOPP_TERM4_0 = 0x30,
    HK_MOPP_TERM4_1 = 0x31,
    HK_MOPP_TERM4_2 = 0x32,
    HK_MOPP_TERM4_3 = 0x33,
    HK_MOPP_TERM4_4 = 0x34,
    HK_MOPP_TERM4_5 = 0x35,
    HK_MOPP_TERM4_6 = 0x36,
    HK_MOPP_TERM4_7 = 0x37,
    HK_MOPP_TERM4_8 = 0x38,
    HK_MOPP_TERM4_9 = 0x39,
    HK_MOPP_TERM4_A = 0x3A,
    HK_MOPP_TERM4_B = 0x3B,
    HK_MOPP_TERM4_C = 0x3C,
    HK_MOPP_TERM4_D = 0x3D,
    HK_MOPP_TERM4_E = 0x3E,
    HK_MOPP_TERM4_F = 0x3F,
    HK_MOPP_TERM4_10 = 0x40,
    HK_MOPP_TERM4_11 = 0x41,
    HK_MOPP_TERM4_12 = 0x42,
    HK_MOPP_TERM4_13 = 0x43,
    HK_MOPP_TERM4_14 = 0x44,
    HK_MOPP_TERM4_15 = 0x45,
    HK_MOPP_TERM4_16 = 0x46,
    HK_MOPP_TERM4_17 = 0x47,
    HK_MOPP_TERM4_18 = 0x48,
    HK_MOPP_TERM4_19 = 0x49,
    HK_MOPP_TERM4_1A = 0x4A,
    HK_MOPP_TERM4_1B = 0x4B,
    HK_MOPP_TERM4_1C = 0x4C,
    HK_MOPP_TERM4_1D = 0x4D,
    HK_MOPP_TERM4_1E = 0x4E,
    HK_MOPP_TERM4_1F = 0x4F,
    HK_MOPP_TERM8 = 0x50,
    HK_MOPP_TERM16 = 0x51,
    HK_MOPP_TERM24 = 0x52,
    HK_MOPP_TERM32 = 0x53,
    HK_MOPP_NTERM8 = 0x54,
    HK_MOPP_NTERM16 = 0x55,
    HK_MOPP_NTERM24 = 0x56,
    HK_MOPP_NTERM32 = 0x57,
    HK_MOPP_PROPERTY8_0 = 0x60,
    HK_MOPP_PROPERTY8_1 = 0x61,
    HK_MOPP_PROPERTY8_2 = 0x62,
    HK_MOPP_PROPERTY8_3 = 0x63,
    HK_MOPP_PROPERTY16_0 = 0x64,
    HK_MOPP_PROPERTY16_1 = 0x65,
    HK_MOPP_PROPERTY16_2 = 0x66,
    HK_MOPP_PROPERTY16_3 = 0x67,
    HK_MOPP_PROPERTY32_0 = 0x68,
    HK_MOPP_PROPERTY32_1 = 0x69,
    HK_MOPP_PROPERTY32_2 = 0x6A,
    HK_MOPP_PROPERTY32_3 = 0x6B,
    HK_MOPP_JUMP_CHUNK32 = 0x70,
};

// A terminal of the MOPP code whose shape key was renumbered: the key it had and the key it has now.
class hkpMoppCodeReindexedTerminal {
public:
    // Havok SDK signature (hkpInternal.lib, tied by target).
    static const hkClass& staticClass();

    uint32_t m_origShapeKey;
    uint32_t m_reindexedShapeKey;
};

// The compiled MOPP code of a shape (a bounding volume tree over its child shapes): the info the code was built with,
// its bytes and how it was built.
class hkpMoppCode : public hkReferencedObject {
public:
    // The info a MOPP code is built with.
    struct CodeInfo {
        // Havok SDK signature (hkpInternal.lib, tied by target).
        static const hkClass& staticClass();

        // The code's offset; the fourth component holds the scale.
        hkVector4 m_offset;
    };

    // How the code was built: hkpMoppUtility::buildCode stores 1 without chunk subdivision, 0 with it; the constructor
    // and bhkStream's loader store BUILD_NOT_SET.
    enum BuildType : int32_t {
        BUILT_WITH_CHUNK_SUBDIVISION = 0,
        BUILT_WITHOUT_CHUNK_SUBDIVISION = 1,
        BUILD_NOT_SET = 2,
    };

    hkpMoppCode();
    hkpMoppCode(const CodeInfo& arInfo, const uint8_t* apMoppData, int32_t aiMoppBytes, BuildType aeBuildType);
    // The SDK's finish constructor: a loaded code keeps the fields the load wrote.
    hkpMoppCode(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_data(aFlag) {}
    ~hkpMoppCode() override;

    // Havok SDK signature (hkpInternal.lib, tied by target).
    static const hkClass& staticClass();

    CodeInfo m_info;
    hkArray<uint8_t> m_data;
    hkEnum<BuildType, int8_t> m_buildType;
};

// The code's bytes go with m_data.
inline hkpMoppCode::~hkpMoppCode() {}

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpMoppCodeReindexedTerminalTypeInfo;
extern hkTypeInfo hkpMoppCodeCodeInfoTypeInfo;

// An empty code at a zero offset and scale, not built yet.
inline hkpMoppCode::hkpMoppCode() {
    m_info.m_offset.setZero4();
    m_buildType.m_storage = BUILD_NOT_SET;
}

inline hkpMoppCode::hkpMoppCode(
    const CodeInfo& arInfo,
    const uint8_t* apMoppData,
    int32_t aiMoppBytes,
    BuildType aeBuildType
) {
    m_info.m_offset = arInfo.m_offset;
    m_data.m_size = aiMoppBytes;
    m_data.m_data = const_cast<uint8_t*>(apMoppData);
    m_data.m_capacityAndFlags = aiMoppBytes | hkArray<uint8_t>::DONT_DEALLOCATE_FLAG;
    m_buildType.m_storage = aeBuildType;
}
