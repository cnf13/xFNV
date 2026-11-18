#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

class hkClassEnum;
class hkClassMember;
struct hkCustomAttributes;
class hkStreamWriter;
class hkTypedUnion;
class hkTypeInfo;

// Havok's reflection record of one class: its name, its parent's record and its members. Only the queries used by this
// code are declared.
class hkClass {
public:
    enum SignatureFlags : int32_t {
        SIGNATURE_LOCAL = 0x1,
    };

    enum FlagValues : uint32_t {
        FLAGS_NONE = 0,
        FLAGS_NOT_SERIALIZABLE = 1,
    };

    // hkClass::Members: the member table hkClassClass describes hkClass with (10 members).
    using MemberArray = const hkClassMember[10];

    hkClass(
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
    );

    // The class name.
    const char* getName() const;

    // The parent class's record, or null.
    const hkClass* getParent() const;
    // The parent class's record, or null: the non-const overload.
    hkClass* getParent();

    // Whether this class is k or one of k's ancestors.
    hkBool isSuperClass(const hkClass& k) const;

    // The number of members, the parents' included.
    int32_t getNumMembers() const;

    // Member i, counting the parents' members first.
    const hkClassMember& getMember(int32_t i) const;

    // Member i, the non-const overload: a five-byte jump to the const getMember, which
    // callers in other translation units go through.
    hkClassMember& getMember(int32_t i);

    // The member called name, the parents' included, or null.
    const hkClassMember* getMemberByName(const char* name) const;

    // The version the record describes.
    int32_t getDescribedVersion() const;

    // The number of this class's own members.
    int32_t getNumDeclaredMembers() const;

    // This class's own member i.
    const hkClassMember& getDeclaredMember(int32_t i) const;

    // The index of the own member called name, or -1.
    int32_t getDeclaredMemberIndexByName(const char* name) const;

    // The index of the member called name, the parents' included, or -1.
    int32_t getMemberIndexByName(const char* name) const;

    // Whether the class or an ancestor is virtual. The hkBool is returned through arResult, which hasVtable takes as
    // its argument.
    hkBool hasVtable() const;

    // The remaining hkClass members are declared only. hkResult is an int (0 is success) and the hkBool32 results are
    // plain ints.
    int32_t getInheritanceDepth() const;
    const hkClass* getInterface(int32_t i) const;
    int32_t getNumInterfaces() const;
    const hkClass* getDeclaredInterface(int32_t i) const;
    int32_t getNumDeclaredInterfaces() const;
    const hkClassEnum& getEnum(int32_t enumIndex) const;
    const hkClassEnum* getEnumByName(const char* name) const;
    int32_t getNumEnums() const;
    const hkClassEnum& getDeclaredEnum(int32_t enumIndex) const;
    const hkClassEnum* getDeclaredEnumByName(const char* name) const;
    // The SDK's getNumDeclaredEnums: m_numDeclaredEnums (+0x14).
    int32_t getNumDeclaredEnums() const;
    const hkClassMember* getDeclaredMemberByName(const char* name) const;
    int32_t hasDefault(int32_t memberIndex) const;
    int32_t hasDeclaredDefault(int32_t declaredIndex) const;
    hkResult getDefault(int32_t aiMemberIndex, hkStreamWriter* apWriter) const;
    hkResult getDeclaredDefault(int32_t aiDeclaredIndex, hkStreamWriter* apWriter) const;
    hkResult getDefault(int32_t aiMemberIndex, hkTypedUnion& arValue) const;
    hkResult getDeclaredDefault(int32_t aiDeclaredIndex, hkTypedUnion& arValue) const;
    // Where the member's default value sits in m_defaults, and the member itself, by the index getMember takes:
    // HK_SUCCESS, or HK_FAILURE when the member has no default (neither out parameter
    // is written then).
    hkResult retrieveMember(int32_t memberIndex, const void*& defaultOut, const hkClassMember*& memberOut) const;
    uint32_t getSignature(int32_t signatureFlags = 0) const;
    // The SDK's writeSignature: the interface count, the declared
    // enums' signatures and count, then per declared member its class's and enum's signatures, name, types and
    // flags, through an hkOArchive on apWriter. getSignature calls it.
    void writeSignature(hkStreamWriter* w) const;
    const hkVariant* getAttribute(const char* id) const;
    int32_t getObjectSize() const {
        return m_objectSize;
    }
    void setObjectSize(int32_t size) {
        m_objectSize = size;
    }
    // The flag bits.
    const hkFlags<FlagValues, uint32_t>& getFlags() const {
        return m_flags;
    }
    hkFlags<FlagValues, uint32_t>& getFlags() {
        return m_flags;
    }

    const char* m_name;
    const hkClass* m_parent;
    int32_t m_objectSize;
    int32_t m_numImplementedInterfaces;
    const hkClassEnum* m_declaredEnums;
    int32_t m_numDeclaredEnums;
    const hkClassMember* m_declaredMembers;
    int32_t m_numDeclaredMembers;
    // The default values of the members, laid out as the object is.
    const void* m_defaults;
    const hkCustomAttributes* m_attributes;
    hkFlags<FlagValues, uint32_t> m_flags;
    int32_t m_describedVersion;

    static MemberArray Members;
};

using hkClassGetFlagsConst = const hkFlags<hkClass::FlagValues, uint32_t>& (hkClass::*)() const;
using hkClassGetFlags = hkFlags<hkClass::FlagValues, uint32_t>& (hkClass::*)();

// hkClassClass: the class record of hkClass itself.
extern hkClass hkClassClass;

// The class records of the other reflection types, all declared here because hkClass must be complete. They are named
// "hkClassMember", "hkClassEnum" and "hkClassEnumItem".
extern hkClass hkClassMemberClass;
extern hkClass hkClassEnumClass;
extern hkClass hkClassEnumItemClass;
inline char* hkDataWorldDict_s_hkDataObjectTupeAttributeID = const_cast<char*>("hk.DataObjectType");

// The class records of the reflection types' older layouts, defined in Common/Base/Reflection/hkClassVersion1Class.cpp.
// hkClassMemberVersion1Class and hkClassEnumItemVersion1Class are declared here with the others because the member
// tables there name each other's records before their definitions.
extern hkClass hkClassVersion1Class;
extern hkClass hkClassVersion2Class;
extern hkClass hkClassMemberVersion1Class;
extern hkClass hkClassEnumItemVersion1Class;
extern hkClass hkClassEnumVersion1Class;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkClassTypeInfo;
