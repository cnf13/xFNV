#include "Havok/Source/Common/Base/Reflection/hkClass.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/System/Io/OArchive/hkOArchive.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Crc32/hkCrc32StreamWriter.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Base/Types/hkTypedUnion.h"

static void cleanupLoadedObjecthkClass(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkClassTypeInfo = {"hkClass", nullptr,          cleanupLoadedObjecthkClass,
                              nullptr,   &typeid(hkClass), sizeof(hkClass)};

// hkClassClass's enums, SignatureFlags and FlagValues, and their items.
using hkClassSignatureFlagsEnumItemArray = const hkClassEnum::Item[1];
static hkClassSignatureFlagsEnumItemArray hkClassSignatureFlagsEnumItems{{1, "SIGNATURE_LOCAL"}};
using hkClassFlagValuesEnumItemArray = const hkClassEnum::Item[2];
static hkClassFlagValuesEnumItemArray hkClassFlagValuesEnumItems{{0, "FLAGS_NONE"}, {1, "FLAGS_NOT_SERIALIZABLE"}};
using hkClassEnumArray = const hkClassEnum[2];
static hkClassEnumArray hkClassEnums{
    {"SignatureFlags", hkClassSignatureFlagsEnumItems, 1, nullptr, 0},
    {"FlagValues", hkClassFlagValuesEnumItems, 2, nullptr, 0}
};

// The SDK's pointers to the class's enums (hkClassClass.cpp; .data): the flags member's m_enum below reads the second
// at start-up.
static const hkClassEnum* hkClassSignatureFlagsEnum = &hkClassEnums[0];
static const hkClassEnum* hkClassFlagValuesEnum = &hkClassEnums[1];
hkClass::MemberArray hkClass::Members{
    {"name", nullptr, nullptr, {hkClassMember::TYPE_CSTRING}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
    {"parent", &hkClassClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, 0, 0x4, nullptr},
    {"objectSize", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x8, nullptr},
    {"numImplementedInterfaces",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     0,
     0xC,
     nullptr},
    {"declaredEnums",
     &hkClassEnumClass,
     nullptr,
     {hkClassMember::TYPE_SIMPLEARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     0,
     0x10,
     nullptr},
    {"declaredMembers",
     &hkClassMemberClass,
     nullptr,
     {hkClassMember::TYPE_SIMPLEARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     0,
     0x18,
     nullptr},
    {"defaults",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     hkClassMember::SERIALIZE_IGNORED,
     0x20,
     nullptr},
    {"attributes",
     &hkCustomAttributesClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     hkClassMember::SERIALIZE_IGNORED,
     0x24,
     nullptr},
    {"flags",
     nullptr,
     hkClassFlagValuesEnum,
     {hkClassMember::TYPE_FLAGS},
     {hkClassMember::TYPE_UINT32},
     0,
     0,
     0x28,
     nullptr},
    {"describedVersion", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x2C, nullptr}
};
hkClass hkClassClass("hkClass", nullptr, 48, nullptr, 0, hkClassEnums, 2, hkClass::Members, 10, nullptr, nullptr, 0, 0);

static void cleanupLoadedObjecthkClass(void* p) {
    ;
}

using hkClassSetObjectSize = void (hkClass::*)(int32_t);

// No pointer to the implemented interfaces is kept, only their count.
hkClass::hkClass(
    const char* className,
    const hkClass* parentClass,
    int32_t objectSizeInBytes,
    const hkClass** implementedInterfaces,
    int32_t numImplementedInterfaces,
    const hkClassEnum* declaredEnums,
    int32_t numDeclaredEnums,
    const hkClassMember* members,
    int32_t numMembers,
    const void* defaults,
    const hkCustomAttributes* attrs,
    uint32_t flags,
    uint32_t version
)
  : m_name(className), m_parent(parentClass), m_objectSize(objectSizeInBytes),
    m_numImplementedInterfaces(numImplementedInterfaces), m_declaredEnums(declaredEnums),
    m_numDeclaredEnums(numDeclaredEnums), m_declaredMembers(members), m_numDeclaredMembers(numMembers),
    m_defaults(defaults), m_attributes(attrs), m_flags(flags), m_describedVersion(version) {
    (void)implementedInterfaces;
}

const char* hkClass::getName() const {
    return m_name;
}

const hkClass* hkClass::getParent() const {
    return m_parent;
}

hkClass* hkClass::getParent() {
    return const_cast<hkClass*>(m_parent);
}

// Walks up from k comparing class names, so that records loaded from different files still match.
hkBool* hkClass::isSuperClass(hkBool* arResult, const hkClass& k) const {
    const hkClass* c = &k;
    while (c) {
        if (hkString::strCmp(c->getName(), getName()) == 0) {
            arResult->m_bool = true;
            return arResult;
        }
        c = c->getParent();
    }
    arResult->m_bool = false;
    return arResult;
}

int32_t hkClass::getNumMembers() const {
    const hkClass* m_parent;
    int32_t result;
    m_parent = this->m_parent;
    for (result = m_numDeclaredMembers; m_parent; m_parent = m_parent->m_parent) {
        result += m_parent->m_numDeclaredMembers;
    }
    return result;
}

// Member i, counting the parents' members first: the index runs from getNumMembers() downwards until it lands inside
// one class's declared members. Falls through to member 0 when the index is out of range, as the game does.
const hkClassMember& hkClass::getMember(int32_t i) const {
    int32_t lIndex = i - getNumMembers();
    const hkClass* c = this;
    while (c) {
        lIndex += c->m_numDeclaredMembers;
        if (lIndex >= 0) {
            return c->m_declaredMembers[lIndex];
        }
        c = c->m_parent;
    }
    return this->m_declaredMembers[0];
}

hkClassMember& hkClass::getMember(int32_t i) {
    return const_cast<hkClassMember&>(static_cast<const hkClass*>(this)->getMember(i));
}

const hkClassMember* hkClass::getMemberByName(const char* name) const {
    const hkClassMember* Member;
    int32_t v3 = 0;
    if (getNumMembers() <= 0) {
        return nullptr;
    }
    while (true) {
        Member = &getMember(v3);
        if (!hkString::strCmp(Member->m_name, name)) {
            break;
        }
        if (++v3 >= getNumMembers()) {
            return nullptr;
        }
    }
    return Member;
}

int32_t hkClass::getDescribedVersion() const {
    return (int32_t)((uint32_t)m_describedVersion);
}

int32_t hkClass::getNumDeclaredMembers() const {
    return m_numDeclaredMembers;
}

const hkClassMember& hkClass::getDeclaredMember(int32_t i) const {
    return m_declaredMembers[i];
}

int32_t hkClass::getDeclaredMemberIndexByName(const char* name) const {
    const hkClassMember* v4;
    int32_t v3 = 0;
    if (getNumDeclaredMembers() <= 0) {
        return (-1);
    }
    while (true) {
        v4 = &getDeclaredMember(v3);
        if (!hkString::strCmp(v4->m_name, name)) {
            break;
        }
        if ((++v3) >= getNumDeclaredMembers()) {
            return (-1);
        }
    }
    return v3;
}

int32_t hkClass::getMemberIndexByName(const char* name) const {
    const hkClassMember* v4;
    int32_t v3 = 0;
    if (getNumMembers() <= 0) {
        return (-1);
    }
    while (true) {
        v4 = &getMember(v3);
        if (!hkString::strCmp(v4->m_name, name)) {
            break;
        }
        if ((++v3) >= getNumMembers()) {
            return (-1);
        }
    }
    return v3;
}

// Whether the class or an ancestor is virtual: the root class's field decides.
hkBool* hkClass::hasVtable(hkBool* arResult) const {
    const hkClass* c = this;
    while (const hkClass* pParent = c->getParent()) {
        c = pParent;
    }
    arResult->m_bool = c->m_numImplementedInterfaces != 0;
    return arResult;
}

int32_t hkClass::getInheritanceDepth() const {
    const hkClass* pThis = this;
    int32_t result;
    for (result = 0; pThis; ++result) {
        pThis = pThis->m_parent;
    }
    return result;
}

// The interface lists are not kept: no interface is ever found.
const hkClass* hkClass::getInterface(int32_t i) const {
    return nullptr;
}

int32_t hkClass::getNumInterfaces() const {
    const hkClass* m_parent;
    int32_t result;
    m_parent = this->m_parent;
    for (result = m_numImplementedInterfaces; m_parent; m_parent = m_parent->m_parent) {
        result += m_parent->m_numImplementedInterfaces;
    }
    return result;
}

const hkClass* hkClass::getDeclaredInterface(int32_t i) const {
    return nullptr;
}

// Returns m_numImplementedInterfaces.
int32_t hkClass::getNumDeclaredInterfaces() const {
    return m_numImplementedInterfaces;
}

// Enum i, counting the parents' enums first: the index runs from getNumEnums() downwards until it lands inside one
// class's declared enums. Falls through to enum 0 when the index is out of range, as the game does.
const hkClassEnum& hkClass::getEnum(int32_t enumIndex) const {
    int32_t lIndex = enumIndex - getNumEnums();
    const hkClass* c = this;
    while (c) {
        lIndex += c->m_numDeclaredEnums;
        if (lIndex >= 0) {
            return c->m_declaredEnums[lIndex];
        }
        c = c->m_parent;
    }
    return this->m_declaredEnums[0];
}

const hkClassEnum* hkClass::getEnumByName(const char* name) const {
    const hkClassEnum* Enum;
    int32_t v3 = 0;
    if (getNumEnums() <= 0) {
        return nullptr;
    }
    while (true) {
        Enum = &getEnum(v3);
        if (!hkString::strCmp(Enum->m_name, name)) {
            break;
        }
        if (++v3 >= getNumEnums()) {
            return nullptr;
        }
    }
    return Enum;
}

// The number of enums, the parents' included.
int32_t hkClass::getNumEnums() const {
    const hkClass* m_parent;
    int32_t result;
    m_parent = this->m_parent;
    for (result = m_numDeclaredEnums; m_parent; m_parent = m_parent->m_parent) {
        result += m_parent->m_numDeclaredEnums;
    }
    return result;
}

const hkClassEnum& hkClass::getDeclaredEnum(int32_t enumIndex) const {
    return m_declaredEnums[enumIndex];
}

const hkClassEnum* hkClass::getDeclaredEnumByName(const char* name) const {
    const hkClassEnum* DeclaredEnum;
    int32_t v3 = 0;
    if (getNumDeclaredEnums() <= 0) {
        return nullptr;
    }
    while (true) {
        DeclaredEnum = &getDeclaredEnum(v3);
        if (!hkString::strCmp(DeclaredEnum->m_name, name)) {
            break;
        }
        if (++v3 >= getNumDeclaredEnums()) {
            return nullptr;
        }
    }
    return DeclaredEnum;
}

int32_t hkClass::getNumDeclaredEnums() const {
    return m_numDeclaredEnums;
}

const hkClassMember* hkClass::getDeclaredMemberByName(const char* name) const {
    const hkClassMember* v4;
    int32_t v3 = 0;
    if (getNumDeclaredMembers() <= 0) {
        return nullptr;
    }
    while (true) {
        v4 = &getDeclaredMember(v3);
        if (!hkString::strCmp(v4->m_name, name)) {
            break;
        }
        if ((++v3) >= getNumDeclaredMembers()) {
            return nullptr;
        }
    }
    return v4;
}

// Whether retrieveMember finds a default for the member. pDefault deliberately holds this until retrieveMember writes
// it, as the game does.
int32_t hkClass::hasDefault(int32_t memberIndex) const {
    const void* pDefault = this;
    const hkClassMember* pMember;
    return retrieveMember(memberIndex, pDefault, pMember) == HK_SUCCESS;
}

// The SDK reads m_defaults as the table of int offsets, one per declared member.
int32_t hkClass::hasDeclaredDefault(int32_t declaredIndex) const {
    const int32_t* piDefaults = static_cast<const int32_t*>(m_defaults);
    return piDefaults && piDefaults[declaredIndex] >= 0;
}

// Writes member aiMemberIndex's default value to apWriter; HK_FAILURE, with nothing written, when it has none.
hkResult hkClass::getDefault(int32_t aiMemberIndex, hkStreamWriter* apWriter) const {
    const void* pDefault;
    const hkClassMember* pMember;
    hkResult result = retrieveMember(aiMemberIndex, pDefault, pMember);
    if (result == HK_SUCCESS) {
        apWriter->write(pDefault, pMember->getSizeInBytes());
    }
    return result;
}

// The SDK's getDeclaredDefault into a stream: m_defaults is the table of int offsets, one per declared member, from
// the table's own address to the default's bytes; a negative offset is no default.
hkResult hkClass::getDeclaredDefault(int32_t aiDeclaredIndex, hkStreamWriter* apWriter) const {
    const int32_t* piDefaults = static_cast<const int32_t*>(m_defaults);
    if (!piDefaults) {
        return HK_FAILURE;
    }
    int32_t iOffset = piDefaults[aiDeclaredIndex];
    if (iOffset < 0) {
        return HK_FAILURE;
    }
    const char* pData = static_cast<const char*>(m_defaults) + iOffset;
    apWriter->write(pData, m_declaredMembers[aiDeclaredIndex].getSizeInBytes());
    return HK_SUCCESS;
}

hkResult hkClass::getDefault(int32_t aiMemberIndex, hkTypedUnion& arValue) const {
    const void* pData;
    const hkClassMember* pMember;
    hkResult eResult = retrieveMember(aiMemberIndex, pData, pMember);
    if (eResult != HK_SUCCESS) {
        return eResult;
    }
    uint8_t ucType = pMember->m_type.m_storage;
    if (ucType == hkClassMember::TYPE_POINTER) {
        arValue.m_elem.m_variant.m_class = pMember->getClass();
        arValue.m_type.m_storage = hkClassMember::TYPE_POINTER;
        arValue.m_elem.m_variant.m_object = const_cast<void*>(pData);
        return eResult;
    }
    if (ucType == hkClassMember::TYPE_ENUM) {
        const hkClassEnum* pEnum = &pMember->getEnumClass();
        int32_t iValue = pMember->getEnumValue(pData);
        arValue.m_elem.m_enumVariant.m_enum = pEnum;
        arValue.m_elem.m_enumVariant.m_value = iValue;
        arValue.m_type.m_storage = hkClassMember::TYPE_ENUM;
        return eResult;
    }
    return arValue.setSimple(pData, static_cast<hkClassMember::Type>(ucType));
}

// The SDK's getDeclaredDefault into a typed union, as getDefault: a pointer default is the object it points to with
// the member's class, an enum default its value with the enum, anything else copied by setSimple.
hkResult hkClass::getDeclaredDefault(int32_t aiDeclaredIndex, hkTypedUnion& arValue) const {
    const int32_t* piDefaults = static_cast<const int32_t*>(m_defaults);
    if (!piDefaults) {
        return HK_FAILURE;
    }
    int32_t iOffset = piDefaults[aiDeclaredIndex];
    if (iOffset < 0) {
        return HK_FAILURE;
    }
    const void* pData = static_cast<const char*>(m_defaults) + iOffset;
    const hkClassMember* pMember = &m_declaredMembers[aiDeclaredIndex];
    uint8_t ucType = pMember->m_type.m_storage;
    if (ucType == hkClassMember::TYPE_POINTER) {
        const hkClass* pClass = pMember->getClass();
        arValue.m_elem.m_variant.m_object = *static_cast<void* const*>(pData);
        arValue.m_elem.m_variant.m_class = pClass;
        arValue.m_type.m_storage = hkClassMember::TYPE_POINTER;
        return HK_SUCCESS;
    }
    if (ucType == hkClassMember::TYPE_ENUM) {
        const hkClassEnum* pEnum = &pMember->getEnumClass();
        int32_t iValue = pMember->getEnumValue(pData);
        arValue.m_elem.m_enumVariant.m_enum = pEnum;
        arValue.m_elem.m_enumVariant.m_value = iValue;
        arValue.m_type.m_storage = hkClassMember::TYPE_ENUM;
        return HK_SUCCESS;
    }
    return arValue.setSimple(pData, static_cast<hkClassMember::Type>(ucType));
}

// Where member aiMemberIndex's default value sits in the class's defaults buffer, and the member itself: the buffer
// holds one offset per member, negative when the member has no default. Neither out parameter is written on failure.
hkResult hkClass::retrieveMember(int32_t memberIndex, const void*& defaultOut, const hkClassMember*& memberOut) const {
    int32_t aiMemberIndex = memberIndex;
    int32_t lIndex = aiMemberIndex - getNumMembers();
    const hkClass* c = this;
    while (c) {
        lIndex += c->m_numDeclaredMembers;
        if (lIndex >= 0) {
            break;
        }
        c = c->m_parent;
    }
    if (!c) {
        return HK_FAILURE;
    }
    const int32_t* lOffsets = static_cast<const int32_t*>(c->m_defaults);
    if (!lOffsets) {
        return HK_FAILURE;
    }
    int32_t lOffset = lOffsets[lIndex];
    if (lOffset < 0) {
        return HK_FAILURE;
    }
    defaultOut = static_cast<const uint8_t*>(c->m_defaults) + lOffset;
    memberOut = c->m_declaredMembers + lIndex;
    return HK_SUCCESS;
}

uint32_t hkClass::getSignature(int32_t signatureFlags) const {
    const hkClass* pThis = this;
    FNVD_INLINED_LOCAL(hkCrc32StreamWriter, v4, std::nullopt, std::nullopt);
    ::new (&v4) hkCrc32StreamWriter(0);
    for (; pThis; pThis = pThis->getParent()) {
        pThis->writeSignature(&v4);
        if ((signatureFlags & 1) != 0) {
            break;
        }
    }
    return v4.getCrc32();
}

// Writes, through an archive on apWriter, the interface count, each declared enum's signature and their count, then per
// declared member the signatures of its class chain and enum, its name and its type words, and the member count. Works
// on a copy of each member's fields: an enum or flags member's ENUM_n size flag is toggled off and its subtype written
// as 0; a serialization-ignored member is written as TYPE_ZERO, with its type as the subtype and its old subtype, when
// set, as an extra word. The archive is created without byte-swapping.
void hkClass::writeSignature(hkStreamWriter* w) const {
    hkOArchive oa(w, hkBool(false));
    oa.write32(m_numImplementedInterfaces);
    for (int32_t i = 0; i < m_numDeclaredEnums; ++i) {
        m_declaredEnums[i].writeSignature(w);
    }
    oa.write32(m_numDeclaredEnums);
    for (int32_t i = 0; i < m_numDeclaredMembers; ++i) {
        const hkClassMember& member = m_declaredMembers[i];
        const char* pName = member.m_name;
        const hkClass* pClass = member.m_class;
        const hkClassEnum* pEnum = member.m_enum;
        uint8_t ucType = member.m_type.m_storage;
        uint8_t ucSubType;
        int16_t sArraySize = member.m_cArraySize;
        uint16_t usFlags = member.m_flags.m_storage;
        if (ucType == hkClassMember::TYPE_ENUM || ucType == hkClassMember::TYPE_FLAGS) {
            const hkClassMember::TypeProperties& props = hkClassMember::getClassMemberTypeProperties(
                static_cast<hkClassMember::Type>(member.m_subtype.m_storage)
            );
            usFlags ^= static_cast<uint16_t>(props.m_size * 8);
            ucSubType = 0;
        }
        else {
            ucSubType = member.m_subtype.m_storage;
        }
        uint16_t usOldSubType = 0;
        if (usFlags & hkClassMember::SERIALIZE_IGNORED) {
            usFlags ^= hkClassMember::SERIALIZE_IGNORED;
            if (ucSubType) {
                usOldSubType = ucSubType;
            }
            ucSubType = ucType;
            ucType = hkClassMember::TYPE_ZERO;
        }
        // Tests the member's own type and subtype here, not the copy's.
        if (pClass && member.m_type.m_storage != hkClassMember::TYPE_POINTER &&
            member.m_subtype.m_storage != hkClassMember::TYPE_POINTER) {
            for (const hkClass* c = member.getClass(); c; c = c->getParent()) {
                c->writeSignature(w);
            }
        }
        if (pEnum) {
            member.getEnumClass().writeSignature(w);
        }
        oa.writeRaw(pName, hkString::strLen(pName));
        oa.write16(static_cast<int16_t>(ucType));
        oa.write16(static_cast<int16_t>(ucSubType));
        if (usOldSubType) {
            oa.write16(static_cast<int16_t>(usOldSubType));
        }
        oa.write16(sArraySize);
        oa.write16(static_cast<int16_t>(usFlags));
    }
    oa.write32(m_numDeclaredMembers);
}

const hkVariant* hkClass::getAttribute(const char* id) const {
    const hkCustomAttributes* m_attributes = this->m_attributes;
    if (m_attributes) {
        return m_attributes->getAttribute(id);
    }

    return nullptr;
}

const hkVariant* hkCustomAttributes::getAttribute(const char* id) const {
    int32_t v3 = 0;
    if (m_numAttributes <= 0) {
        return nullptr;
    }
    for (int32_t i = 0; hkString::strCmp(m_attributes[i].m_name, id); ++i) {
        if (++v3 >= m_numAttributes) {
            return nullptr;
        }
    }
    return &m_attributes[v3].m_value;
}

// Stores type and copies its size of bytes from addr into m_elem; HK_FAILURE, nothing written, when the type is wider
// than m_elem. The size compare is unsigned.
hkResult hkTypedUnion::setSimple(const void* orig, hkClassMember::Type type) {
    const hkClassMember::TypeProperties& props = hkClassMember::getClassMemberTypeProperties(type);
    if (static_cast<uint16_t>(props.m_size) > sizeof(Storage)) {
        return HK_FAILURE;
    }
    m_type.m_storage = type;
    hkString::memCpy(&m_elem, orig, props.m_size);
    return HK_SUCCESS;
}
