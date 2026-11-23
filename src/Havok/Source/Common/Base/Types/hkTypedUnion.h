#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

class hkClassEnum;

// Havok's reflection value: the type of a member's default value together with the value itself, in the widest storage
// the reflection holds.
class hkTypedUnion {
public:
    // One enum value: the value and the enum's record.
    struct EnumVariant {
        int32_t m_value;
        const hkClassEnum* m_enum;
    };

    // The value's storage: the widest form of every simple type, a variant and an enum value. 64 bytes, 16-aligned;
    // setSimple rejects a value wider than 64 bytes.
    union Storage {
        bool m_bool;
        char m_char;
        int8_t m_int8;
        uint8_t m_uint8;
        int16_t m_int16;
        uint16_t m_uint16;
        int32_t m_int32;
        uint32_t m_uint32;
        int64_t m_int64;
        uint64_t m_uint64;
        float m_float;
        float m_float4[4];
        float m_float12[12];
        float m_float16[16];
        hkVariant m_variant;
        EnumVariant m_enumVariant;
        hkUlong m_ulong;
        int32_t m_maxSize[16];
    };

    // Stores type and copies its size of bytes from addr into m_elem; HK_FAILURE when the
    // type is wider than m_elem.
    hkResult setSimple(const void* orig, hkClassMember::Type type);

    // Both of these are public, like the other reflection records' data, so the bodies that write them
    // (hkClass::getDefault and getDeclaredDefault) can.
    hkEnum<hkClassMember::Type, int32_t> m_type;
    alignas(16) Storage m_elem;
};
