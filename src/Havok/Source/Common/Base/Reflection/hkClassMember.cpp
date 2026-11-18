#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMemberAccessor.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

static void cleanupLoadedObjecthkClassMember(void* p);
static int32_t isSimpleType(int32_t r_eax);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkClassMemberTypeInfo = {
    "hkClassMember", nullptr, cleanupLoadedObjecthkClassMember, nullptr, &typeid(hkClassMember), sizeof(hkClassMember)
};

// hkClassMemberClass's enums, Type, FlagValues and DeprecatedFlagValues, and their items.
using hkClassMemberTypeEnumItemArray = const hkClassEnum::Item[35];
static hkClassMemberTypeEnumItemArray hkClassMemberTypeEnumItems{
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
    {32, "TYPE_HALF"},        {33, "TYPE_STRINGPTR"},
    {34, "TYPE_MAX"}
};
using hkClassMemberFlagValuesEnumItemArray = const hkClassEnum::Item[5];
static hkClassMemberFlagValuesEnumItemArray hkClassMemberFlagValuesEnumItems{
    {0, "FLAGS_NONE"},
    {128, "ALIGN_8"},
    {256, "ALIGN_16"},
    {512, "NOT_OWNED"},
    {1024, "SERIALIZE_IGNORED"}
};
using hkClassMemberDeprecatedFlagValuesEnumItemArray = const hkClassEnum::Item[6];
static hkClassMemberDeprecatedFlagValuesEnumItemArray hkClassMemberDeprecatedFlagValuesEnumItems{
    {8, "DEPRECATED_SIZE_8"},   {8, "DEPRECATED_ENUM_8"},   {16, "DEPRECATED_SIZE_16"},
    {16, "DEPRECATED_ENUM_16"}, {32, "DEPRECATED_SIZE_32"}, {32, "DEPRECATED_ENUM_32"}
};
using hkClassMemberEnumArray = const hkClassEnum[3];
static hkClassMemberEnumArray hkClassMemberEnums{
    {"Type", hkClassMemberTypeEnumItems, 35, nullptr, 0},
    {"FlagValues", hkClassMemberFlagValuesEnumItems, 5, nullptr, 0},
    {"DeprecatedFlagValues", hkClassMemberDeprecatedFlagValuesEnumItems, 6, nullptr, 0}
};

// The SDK's pointers to the class's enums (hkClassMemberClass.cpp; .data): the type, subtype and flags members'
// m_enum below read them at start-up.
static const hkClassEnum* hkClassMemberTypeEnum = &hkClassMemberEnums[0];
static const hkClassEnum* hkClassMemberFlagValuesEnum = &hkClassMemberEnums[1];
static const hkClassEnum* hkClassMemberDeprecatedFlagValuesEnum = &hkClassMemberEnums[2];
hkClassMember::MemberArray hkClassMember::Members{
    {"name", nullptr, nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
    {"class", &hkClassClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, 0, 0x4, nullptr},
    {"enum",
     &hkClassEnumClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     0,
     0x8,
     nullptr},
    {"type",
     nullptr,
     hkClassMemberTypeEnum,
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     0,
     0xC,
     nullptr},
    {"subtype",
     nullptr,
     hkClassMemberTypeEnum,
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     0,
     0xD,
     nullptr},
    {"cArraySize", nullptr, nullptr, {hkClassMember::TYPE_INT16}, {hkClassMember::TYPE_VOID}, 0, 0, 0xE, nullptr},
    {"flags",
     nullptr,
     hkClassMemberFlagValuesEnum,
     {hkClassMember::TYPE_FLAGS},
     {hkClassMember::TYPE_UINT16},
     0,
     0,
     0x10,
     nullptr},
    {"offset", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0, 0, 0x12, nullptr},
    {"attributes",
     &hkCustomAttributesClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     hkClassMember::SERIALIZE_IGNORED,
     0x14,
     nullptr}
};
hkClass hkClassMemberClass(
    "hkClassMember",
    nullptr,
    24,
    nullptr,
    0,
    hkClassMemberEnums,
    3,
    hkClassMember::Members,
    9,
    nullptr,
    nullptr,
    0,
    0
);

static void cleanupLoadedObjecthkClassMember(void* p) {
    ;
}

static int32_t isSimpleType(int32_t r_eax) {
    return r_eax >= 0 &&
        (r_eax < 20 || (r_eax == 28 || (r_eax == 30 || (r_eax == 29 || (r_eax == 32 || r_eax == 33)))));
}

const hkClass& hkClassMember::getStructClass() const {
    return *m_class;
}

const hkClass* hkClassMember::getClass() const {
    return m_class;
}

// KEEP: the exe's caller hkClassMember::getSizeInBytes (0xD86980) keeps the type index in edx across its calls of
// this function (lea eax, [edx + edx*2] at 0xD869B2), which the exe body leaves intact; clang does not guarantee it.
int32_t hkClassMember::getCstyleArraySize() const {
    return m_cArraySize;
}

// The member's size in bytes: its type's size times its C-style array size, which counts as 1 when the member is not a
// C-style array. A struct's size is taken from its class record's object size.
int32_t hkClassMember::getSizeInBytes() const {
    switch (m_type.m_storage) {
        case TYPE_BOOL:
        case TYPE_CHAR:
        case TYPE_INT8:
        case TYPE_UINT8:
        case TYPE_INT16:
        case TYPE_UINT16:
        case TYPE_INT32:
        case TYPE_UINT32:
        case TYPE_INT64:
        case TYPE_UINT64:
        case TYPE_REAL:
        case TYPE_VECTOR4:
        case TYPE_QUATERNION:
        case TYPE_MATRIX3:
        case TYPE_ROTATION:
        case TYPE_QSTRANSFORM:
        case TYPE_MATRIX4:
        case TYPE_TRANSFORM:
        case TYPE_POINTER:
        case TYPE_FUNCTIONPOINTER:
        case TYPE_ARRAY:
        case TYPE_SIMPLEARRAY:
        case TYPE_HOMOGENEOUSARRAY:
        case TYPE_VARIANT:
        case TYPE_CSTRING:
        case TYPE_ULONG:
        case TYPE_HALF:
        case TYPE_STRINGPTR:
            if (getCstyleArraySize() != 0) {
                return ClassMemberProperties[m_type.m_storage].m_size * getCstyleArraySize();
            }
            return ClassMemberProperties[m_type.m_storage].m_size * 1;
        // An enum or flags member's size is its underlying type's, which the subtype holds.
        case TYPE_ENUM:
        case TYPE_FLAGS:
            if (getCstyleArraySize() != 0) {
                return ClassMemberProperties[m_subtype.m_storage].m_size * getCstyleArraySize();
            }
            return ClassMemberProperties[m_subtype.m_storage].m_size * 1;
        case TYPE_STRUCT:
            if (getCstyleArraySize() != 0) {
                int32_t numElements = getCstyleArraySize();
                return getStructClass().getObjectSize() * numElements;
            }
            return getStructClass().getObjectSize() * 1;
        default: return -1;
    }
}

void hkClassMember::setEnumValue(void* memberAddress, int32_t value) const {
    int32_t v4;
    int32_t v3 = getSizeInBytes() - 1;
    if (v3) {
        v4 = v3 - 1;
        if (v4) {
            if (v4 == 2) {
                *static_cast<uint32_t*>(memberAddress) = (uint32_t)value;
            }
        }
        else {
            *static_cast<uint16_t*>(memberAddress) = (uint16_t)value;
        }
    }
    else {
        *static_cast<uint8_t*>(memberAddress) = (uint8_t)value;
    }
}

// The member's C++ type as text, copied into buf (at most bufLen characters); returns the text's length. Array element
// types are written with XML-escaped angle brackets, as the game does.
int32_t hkClassMember::getTypeName(char* buf, int32_t bufLen) const {
    Type eType = static_cast<Type>(m_type.m_storage);
    const char* pClassName = m_class ? m_class->getName() : "unknown";
    const char* pEnumName = m_enum ? m_enum->m_name : "unknown";
    XFNV_INLINED_LOCAL(hkStringBuf, s, std::nullopt, std::nullopt);
    ::new (&s) hkStringBuf;
    // EXE-INLINED 0xD868B0
    if ((eType >= TYPE_VOID &&
         (eType < TYPE_POINTER || eType == TYPE_VARIANT || eType == TYPE_ULONG || eType == TYPE_CSTRING ||
          eType == TYPE_HALF || eType == TYPE_STRINGPTR)) ||
        eType == TYPE_HOMOGENEOUSARRAY) {
        getSimpleTypeName(s, m_cArraySize, eType);
    }
    else if (eType == TYPE_POINTER) {
        if (m_class) {
            s.printf("struct %s*", m_class->getName());
        }
        else if (m_subtype.m_storage == TYPE_CHAR) {
            s = "char*";
        }
        else {
            s = "void*";
        }
    }
    else if (eType == TYPE_ARRAY || eType == TYPE_SIMPLEARRAY) {
        Type eSubType = getSubType();
        const char* pArrayName = eType == TYPE_ARRAY ? "hkArray" : "hkSimpleArray";
        if (static_cast<uint8_t>(isSimpleType(eSubType))) {
            s.printf("%s&lt;%s&gt;", pArrayName, ClassMemberProperties[eSubType].m_name);
        }
        else if (eSubType == TYPE_POINTER) {
            if (m_class) {
                s.printf("%s&lt;%s*&gt;", pArrayName, pClassName);
            }
            else {
                s.printf("%s&lt;void*&gt;", pArrayName);
            }
        }
        else if (eSubType == TYPE_STRUCT) {
            s.printf("%s&lt;struct %s&gt;", pArrayName, pClassName);
        }
    }
    else if (eType == TYPE_ENUM) {
        s.printf("enum %s", pEnumName);
    }
    else if (eType == TYPE_FLAGS) {
        s.printf("flags %s", pEnumName);
    }
    else if (eType == TYPE_STRUCT) {
        if (m_cArraySize) {
            s.printf("struct %s[%i]", pClassName, static_cast<int32_t>(m_cArraySize));
        }
        else {
            s.printf("struct %s", pClassName);
        }
    }
    hkString::strNcpy(buf, s.m_string.begin(), bufLen);
    int32_t iLength = s.m_string.m_size - 1;
    // EXE-INLINED 0xCBBD40
    hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
    s.m_string.m_size = 0;
    if (s.m_string.m_capacityAndFlags >= 0) {
        pTemp->bufFree(s.m_string.begin(), s.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
    }
    return iLength;
}

const hkClassEnum& hkClassMember::getEnumClass() const {
    return *m_enum;
}

const hkClassEnum& hkClassMember::getEnumType() {
    return *m_enum;
}

hkBool hkClassMember::isNotOwner() {
    return m_flags.allAreSet(NOT_OWNED);
}

int32_t hkClassMember::getAlignment() const {
    int32_t result;
    int32_t m_storage = m_type.m_storage;
    if (m_storage == 24 || m_storage == 31) {
        m_storage = m_subtype.m_storage;
    }
    if (m_storage == 25) {
        int32_t v3 = 1;
        for (int32_t i = 0; i < m_class->getNumMembers(); ++i) {
            const hkClassMember* Member = &m_class->getMember(i);
            if (Member->getAlignment() > v3) {
                const hkClassMember* v6 = &m_class->getMember(i);
                v3 = v6->getAlignment();
            }
        }
        result = v3;
    }
    else {
        result = ClassMemberProperties[m_storage].m_align;
    }
    uint16_t v8 = m_flags.get();
    if ((v8 & 384) != 0) {
        int32_t v9 = 8 * ((v8 & 256) != 0) + 8;
        if (v9 > result) {
            return v9;
        }
    }
    return result;
}

int32_t hkClassMember::getEnumValue(const void* memberAddress) const {
    int32_t v2 = getSizeInBytes() - 1;
    if (!v2) {
        return *static_cast<const int8_t*>(memberAddress);
    }
    int32_t n2 = v2 - 1;
    if (!n2) {
        return *static_cast<const int16_t*>(memberAddress);
    }
    if (n2 == 2) {
        return *static_cast<const uint32_t*>(memberAddress);
    }
    return 0;
}

int32_t hkClassMember::getCstyleArraySizeOrOne() {
    int32_t result = getCstyleArraySize();
    if (!result) {
        return 1;
    }
    return result;
}

const hkVariant* hkClassMember::getAttribute(const char* id) const {
    const hkCustomAttributes* m_attributes = this->m_attributes;
    if (m_attributes) {
        return m_attributes->getAttribute(id);
    }

    return nullptr;
}

const hkClassMember::TypeProperties& hkClassMember::getClassMemberTypeProperties(Type type) {
    return ClassMemberProperties[type];
}

// The SDK's getSubtypeOf: the element type of an "hkArray<...>" or "hkSimpleArray<...>" type name (getTypeOf of the
// text up to the closing '>'), else the number between '[' and ']', else TYPE_VOID.
hkClassMember::Type hkClassMember::getSubtypeOf(const char* name) {
    if (!hkString::strNcmp(name, "hkArray<", 8)) {
        const char* pInner = name + 8;
        XFNV_INLINED_LOCAL(hkStringBuf, s, std::nullopt, std::nullopt);
        ::new (&s) hkStringBuf(pInner, hkString::strLen(pInner) - 1);
        Type eType = getTypeOf(s.m_string.begin());
        // EXE-INLINED 0xCBBD40
        hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
        s.m_string.m_size = 0;
        if (s.m_string.m_capacityAndFlags >= 0) {
            pTemp->bufFree(s.m_string.begin(), s.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
        }
        return eType;
    }
    if (!hkString::strNcmp(name, "hkSimpleArray<", 14)) {
        const char* pInner = name + 14;
        XFNV_INLINED_LOCAL(hkStringBuf, s, std::nullopt, std::nullopt);
        ::new (&s) hkStringBuf(pInner, hkString::strLen(pInner) - 1);
        Type eType = getTypeOf(s.m_string.begin());
        // EXE-INLINED 0xCBBD40
        hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
        s.m_string.m_size = 0;
        if (s.m_string.m_capacityAndFlags >= 0) {
            pTemp->bufFree(s.m_string.begin(), s.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
        }
        return eType;
    }
    const char* pOpen = hkString::strChr(name, '[');
    if (!pOpen) {
        return TYPE_VOID;
    }
    const char* pClose = hkString::strChr(name, ']');
    hkStringBuf s(pOpen + 1, static_cast<int32_t>(pClose - pOpen) - 1);
    Type eType = static_cast<Type>(hkString::atoi(s.m_string.begin(), 0));
    return eType;
}

// The SDK's getTypeOf: the member type a C++ type name stands for, by its prefix, a trailing '*', or else the name
// (up to any '[') compared with each row of the type table; TYPE_VOID when none matches.
hkClassMember::Type hkClassMember::getTypeOf(const char* name) {
    if (!hkString::strNcmp(name, "enum ", 5)) {
        return TYPE_ENUM;
    }
    if (!hkString::strNcmp(name, "flags ", 6)) {
        return TYPE_FLAGS;
    }
    if (!hkString::strNcmp(name, "hkArray<", 8)) {
        return TYPE_ARRAY;
    }
    if (!hkString::strNcmp(name, "hkSimpleArray<", 14)) {
        return TYPE_SIMPLEARRAY;
    }
    if (!hkString::strNcmp(name, "char*", 5)) {
        return TYPE_CSTRING;
    }
    if (!hkString::strNcmp(name, "hkStringPtr", 11)) {
        return TYPE_STRINGPTR;
    }
    const char* pStar = hkString::strRchr(name, '*');
    if (pStar && !pStar[1]) {
        return TYPE_POINTER;
    }
    hkStringBuf s(name);
    const char* pBracket = hkString::strChr(name, '[');
    if (pBracket) {
        s.slice(0, static_cast<int32_t>(pBracket - name));
    }
    for (int32_t i = 0; i < TYPE_MAX; ++i) {
        if (s == ClassMemberProperties[i].m_name) {
            return static_cast<Type>(i);
        }
    }
    return TYPE_VOID;
}

void getSimpleTypeName(hkStringBuf& ret, int32_t nelem, hkClassMember::Type type) {
    if (nelem) {
        ret.printf("%s[%i]", ClassMemberProperties[type].m_name, nelem);
    }
    else {
        ret.operator=(ClassMemberProperties[type].m_name);
    }
}

hkClass* getClassMemberClass() {
    return &hkClassMemberClass;
}
