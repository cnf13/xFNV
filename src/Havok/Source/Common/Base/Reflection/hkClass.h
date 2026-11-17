#pragma once

#include "Reconstruction.hpp"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

class hkClassEnum;
class hkClassMember;
struct hkCustomAttributes;
class hkStreamWriter;
class hkTypeInfo;
class hkTypedUnion;

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"

// Havok's reflection record of one class: its name, its parent's record and its members. src models only the
// queries its bodies call; the exe calls the accessors out of line.
class hkClass {
public:
	enum SignatureFlags : int32_t {
		SIGNATURE_LOCAL = 0x1,
	};

	enum FlagValues : uint32_t {
		FLAGS_NONE = 0,
		FLAGS_NOT_SERIALIZABLE = 1,
	};

	// The PDB's hkClass::Members: the member table hkClassClass describes hkClass with (.data, 10 members).
	using MemberArray = const hkClassMember[10];

	hkClass(const char* className, const hkClass* parentClass, int32_t objectSizeInBytes,
		const hkClass** implementedInterfaces, int32_t numImplementedInterfaces, const hkClassEnum* declaredEnums,
		int32_t numDeclaredEnums, const hkClassMember* members, int32_t numMembers, const void* defaults,
		const hkCustomAttributes* attrs, uint32_t flags, uint32_t version);

	// The class name.
	const char* getName() const;

	// The parent class's record, or null.
	const hkClass* getParent() const;
	// The parent class's record, or null: the non-const overload.
	hkClass* getParent();

	// Whether this class is k or one of k's ancestors (thiscall, ret 8, 0xD46D80). The exe returns the hkBool
	// through a hidden pointer, so isSuperClass takes it as its first argument; k is a pointer because the exe
	// tests it for null before the first comparison.
	hkBool* isSuperClass(hkBool* arResult, const hkClass& k) const;

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

	// Whether the class or an ancestor is virtual (thiscall, ret 4, 0xD470F0). The exe returns the hkBool through a
	// hidden pointer, which hasVtable takes as its argument.
	hkBool* hasVtable(hkBool* arResult) const;

	// The rest of the SDK's hkClass members (declared only: not reconstructed yet). hkResult is an int (0 is
	// success) and the hkBool32 results are plain ints.
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
	// The exe's out-of-line copy (thiscall, ret, 0xD470D0) is a trivial accessor: the inline body stays and is tied
	// with FNVD_ADDRESS.
	int32_t getObjectSize() const { return m_objectSize; }
	// The exe's out-of-line copy of setObjectSize (thiscall, ret 4, 0xD470E0) is a trivial accessor: the inline body
	// stays and is tied with FNVD_ADDRESS.
	void setObjectSize(int32_t size) { m_objectSize = size; }
	// The flag bits.
	const hkFlags<FlagValues, uint32_t>& getFlags() const { return m_flags; }
	hkFlags<FlagValues, uint32_t>& getFlags() { return m_flags; }

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

// The exe's out-of-line copies of the inline getFlags pair (lea eax, [ecx+0x28]; ret): trivial accessors, so the
// inline bodies stay and are tied, not registered (const 0xD47280, non-const 0xD47290).
using hkClassGetFlagsConst = const hkFlags<hkClass::FlagValues, uint32_t>& (hkClass::*)() const;
using hkClassGetFlags = hkFlags<hkClass::FlagValues, uint32_t>& (hkClass::*)();

// hkClassClass: the class record of hkClass itself.
extern hkClass hkClassClass;

// The class records of the other reflection types, all declared here because hkClass must be complete: the exe's
// 0xFC3B50, 0xFC3BC0 and 0xFC3B80 register them as "hkClassMember", "hkClassEnum" and "hkClassEnumItem".
extern hkClass hkClassMemberClass;
extern hkClass hkClassEnumClass;
extern hkClass hkClassEnumItemClass;
inline char* hkDataWorldDict_s_hkDataObjectTupeAttributeID = const_cast<char*>("hk.DataObjectType");

// The class records of the reflection types' older layouts (hkClass 0x24 bytes, hkClassMember 0x14), the PDB's
// hkClassVersion1Class.obj (defined in Common/Base/Reflection/hkClassVersion1Class.cpp). The PDB has
// hkClassMemberVersion1Class and hkClassEnumItemVersion1Class as that file's statics; they are declared here with the
// others because the member tables there name each other's records before their definitions.
extern hkClass hkClassVersion1Class;
extern hkClass hkClassVersion2Class;
extern hkClass hkClassMemberVersion1Class;
extern hkClass hkClassEnumItemVersion1Class;
extern hkClass hkClassEnumVersion1Class;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkClassTypeInfo;
