#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <stdint.h>

class hkClass;
class hkClassEnum;
struct hkCustomAttributes;
class hkStringBuf;
class hkTypeInfo;

// Havok's reflection record of one member of a class: its name, type, array size, flags and offset. Type's values count
// up from TYPE_VOID.
class hkClassMember {
public:
    enum Type : int32_t {
        TYPE_VOID = 0,
        TYPE_BOOL,
        TYPE_CHAR,
        TYPE_INT8,
        TYPE_UINT8,
        TYPE_INT16,
        TYPE_UINT16,
        TYPE_INT32,
        TYPE_UINT32,
        TYPE_INT64,
        TYPE_UINT64,
        TYPE_REAL,
        TYPE_VECTOR4,
        TYPE_QUATERNION,
        TYPE_MATRIX3,
        TYPE_ROTATION,
        TYPE_QSTRANSFORM,
        TYPE_MATRIX4,
        TYPE_TRANSFORM,
        TYPE_ZERO,
        TYPE_POINTER,
        TYPE_FUNCTIONPOINTER,
        TYPE_ARRAY,
        TYPE_INPLACEARRAY,
        TYPE_ENUM,
        TYPE_STRUCT,
        TYPE_SIMPLEARRAY,
        TYPE_HOMOGENEOUSARRAY,
        TYPE_VARIANT,
        TYPE_CSTRING,
        TYPE_ULONG,
        TYPE_FLAGS,
        TYPE_HALF,
        TYPE_STRINGPTR,
        TYPE_MAX,
    };

    enum FlagValues : int32_t {
        FLAGS_NONE = 0x0,
        ALIGN_8 = 0x80,
        ALIGN_16 = 0x100,
        NOT_OWNED = 0x200,
        SERIALIZE_IGNORED = 0x400,
    };

    // The size and enum-size flags of older class records, which share their values.
    enum DeprecatedFlagValues : int32_t {
        DEPRECATED_SIZE_8 = 8,
        DEPRECATED_ENUM_8 = 8,
        DEPRECATED_SIZE_16 = 16,
        DEPRECATED_ENUM_16 = 16,
        DEPRECATED_SIZE_32 = 32,
        DEPRECATED_ENUM_32 = 32,
    };

    // One row of the member type table: the type, its name, its size and its alignment.
    struct TypeProperties {
        hkEnum<Type, uint8_t> m_type;
        const char* m_name;
        int16_t m_size;
        int16_t m_align;
    };

    // hkClassMember::Members: the member table hkClassMemberClass describes hkClassMember with.
    using MemberArray = const hkClassMember[9];

    // The class record of a struct member, *m_class.
    const hkClass& getStructClass() const;
    const hkClass* getClass() const;
    // The member's C-style array size, m_cArraySize.
    int32_t getCstyleArraySize() const;
    Type getSubType() const {
        return static_cast<Type>(m_subtype.m_storage);
    }
    // The member's size in bytes: its type's size times its C-style array size.
    int32_t getSizeInBytes() const;
    // The subtype's size in bytes.
    int32_t getArrayMemberSize() const;
    // Stores value at memberAddress in the enum's storage size.
    void setEnumValue(void* memberAddress, int32_t value) const;
    // The member's C++ type name into ret; returns its length.
    int32_t getTypeName(char* buf, int32_t bufLen) const;
    // Havok SDK signature (hkBase.lib, tied by reloc).
    const hkClassEnum& getEnumClass() const;
    const hkClassEnum& getEnumType();
    hkBool isNotOwner();
    // The member's alignment: its type's (a struct's: its widest member's), raised to 8 or 16 by the align flags.
    int32_t getAlignment() const;
    // Havok SDK signature (hkBase.lib, tied by bytes,reloc).
    int32_t getEnumValue(const void* memberAddress) const;
    int32_t getCstyleArraySizeOrOne();
    const hkVariant* getAttribute(const char* id) const;

    // The row of type in the member type table.
    static const TypeProperties& getClassMemberTypeProperties(Type type);
    // The element type of an array type name.
    static hkClassMember::Type getSubtypeOf(const char* name);
    // Havok SDK signature (hkBase.lib, tied by bytes,reloc,string).
    static hkClassMember::Type getTypeOf(const char* name);

    const char* m_name;
    const hkClass* m_class;
    const hkClassEnum* m_enum;
    hkEnum<Type, uint8_t> m_type;
    hkEnum<Type, uint8_t> m_subtype;
    int16_t m_cArraySize;
    hkFlags<FlagValues, uint16_t> m_flags;
    uint16_t m_offset;
    const hkCustomAttributes* m_attributes;

    static MemberArray Members;
};

// Writes type's name (from the type-name table) into ret, formatted with nelem when it is not 0.
void getSimpleTypeName(hkStringBuf& ret, int32_t nelem, hkClassMember::Type type);
hkClass* getClassMemberClass();

// ClassMemberProperties is a file-local table of hkClassMember.cpp with TYPE_MAX + 1 rows, the last one "hkTypeMax".
// Its rows' names, sizes and alignments are read as fields of the rows.
using ClassMemberProperties_t = hkClassMember::TypeProperties[hkClassMember::TYPE_MAX + 1];
inline ClassMemberProperties_t ClassMemberProperties{
    {{hkClassMember::TYPE_VOID}, "void", -1, -1},
    {{hkClassMember::TYPE_BOOL}, "hkBool", 1, 1},
    {{hkClassMember::TYPE_CHAR}, "hkChar", 1, 1},
    {{hkClassMember::TYPE_INT8}, "hkInt8", 1, 1},
    {{hkClassMember::TYPE_UINT8}, "hkUint8", 1, 1},
    {{hkClassMember::TYPE_INT16}, "hkInt16", 2, 2},
    {{hkClassMember::TYPE_UINT16}, "hkUint16", 2, 2},
    {{hkClassMember::TYPE_INT32}, "hkInt32", 4, 4},
    {{hkClassMember::TYPE_UINT32}, "hkUint32", 4, 4},
    {{hkClassMember::TYPE_INT64}, "hkInt64", 8, 8},
    {{hkClassMember::TYPE_UINT64}, "hkUint64", 8, 8},
    {{hkClassMember::TYPE_REAL}, "hkReal", 4, 4},
    {{hkClassMember::TYPE_VECTOR4}, "hkVector4", 16, 16},
    {{hkClassMember::TYPE_QUATERNION}, "hkQuaternion", 16, 16},
    {{hkClassMember::TYPE_MATRIX3}, "hkMatrix3", 48, 16},
    {{hkClassMember::TYPE_ROTATION}, "hkRotation", 48, 16},
    {{hkClassMember::TYPE_QSTRANSFORM}, "hkQsTransform", 48, 16},
    {{hkClassMember::TYPE_MATRIX4}, "hkMatrix4", 64, 16},
    {{hkClassMember::TYPE_TRANSFORM}, "hkTransform", 64, 16},
    {{hkClassMember::TYPE_ZERO}, "hkZero", -1, -1},
    {{hkClassMember::TYPE_POINTER}, "hkPointer", 4, 4},
    {{hkClassMember::TYPE_FUNCTIONPOINTER}, "hkFunctionPointer", 4, 4},
    {{hkClassMember::TYPE_ARRAY}, "hkArray", 12, 4},
    {{hkClassMember::TYPE_INPLACEARRAY}, "hkInplaceArray", -1, -1},
    {{hkClassMember::TYPE_ENUM}, "hkEnum", -1, -1},
    {{hkClassMember::TYPE_STRUCT}, "hkStruct", -1, -1},
    {{hkClassMember::TYPE_SIMPLEARRAY}, "hkSimpleArray", 8, 4},
    {{hkClassMember::TYPE_HOMOGENEOUSARRAY}, "hkHomogeneousArray", 12, 4},
    {{hkClassMember::TYPE_VARIANT}, "hkVariant", 8, 4},
    {{hkClassMember::TYPE_CSTRING}, "char*", 4, 4},
    {{hkClassMember::TYPE_ULONG}, "hkUlong", 4, 4},
    {{hkClassMember::TYPE_FLAGS}, "hkFlags", -1, -1},
    {{hkClassMember::TYPE_HALF}, "hkHalf", 2, 2},
    {{hkClassMember::TYPE_STRINGPTR}, "hkStringPtr", 4, 4},
    {{hkClassMember::TYPE_MAX}, "hkTypeMax", -1, -1}
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkClassMemberTypeInfo;
