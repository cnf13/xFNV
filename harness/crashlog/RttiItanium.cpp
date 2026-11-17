// The crash log's RTTI on the Itanium C++ ABI (the POSIX programs): an object's class from the offset-to-top and
// type_info words before its vtable's address point, its bases from the type_info's class kind (abi::
// __class_type_info, __si_class_type_info, __vmi_class_type_info). Every record is read through readMemory and trusted
// only inside the image.

#include "crashlog/CrashLog.hpp"

#include <cstring>
#include <typeinfo>

namespace fnvd::crashlog {
namespace {

// The ABI's type_info records of classes (cxxabi.h): the vtable pointer and the mangled name first; a single public
// non-virtual base at offset 0 follows in __si_class_type_info; __vmi_class_type_info has flags, a count and the
// bases with their offsets (offset << 8 | flags, bit 0: virtual).
struct ClassTypeInfo {
	uintptr_t uiVtable;
	uintptr_t uiName;
};
struct VmiClassTypeInfo {
	uintptr_t uiVtable;
	uintptr_t uiName;
	uint32_t uiFlags;
	uint32_t uiBaseCount;
};
struct BaseClassInfo {
	uintptr_t uiType;
	intptr_t iOffsetFlags;
};
constexpr intptr_t kVirtualBase = 1;

enum class Kind { None, Class, Single, Multiple };

// One class of each kind, for the vtables of their type_info objects (the runtime's classes of the three kinds).
struct KindRoot {
	virtual ~KindRoot() = default;
};
struct KindSingle : KindRoot {};
struct KindOther {
	virtual ~KindOther() = default;
};
struct KindMultiple : KindRoot, KindOther {};

uintptr_t typeInfoVtable(const std::type_info& arType)
{
	uintptr_t uiVtable;
	std::memcpy(&uiVtable, static_cast<const void*>(&arType), sizeof(uiVtable));
	return uiVtable;
}

// The type_info at auiType: its kind and its mangled name into acName.
Kind typeInfo(uintptr_t auiType, char (&acName)[160])
{
	ClassTypeInfo kType;
	if (!kImage.Contains(auiType, sizeof(kType)) || !readMemory(&kType, auiType, sizeof(kType))) {
		return Kind::None;
	}
	const Kind eKind = kType.uiVtable == typeInfoVtable(typeid(KindRoot))   ? Kind::Class
		: kType.uiVtable == typeInfoVtable(typeid(KindSingle))               ? Kind::Single
		: kType.uiVtable == typeInfoVtable(typeid(KindMultiple))             ? Kind::Multiple
																			 : Kind::None;
	if (eKind == Kind::None || !kImage.Contains(kType.uiName, 2)) {
		return Kind::None;
	}
	size_t uiSize = kImage.uiEnd - kType.uiName;
	uiSize = uiSize < sizeof(acName) ? uiSize : sizeof(acName);
	if (!readMemory(acName, kType.uiName, uiSize)) {
		return Kind::None;
	}
	acName[sizeof(acName) - 1] = '\0';
	return (acName[0] >= '1' && acName[0] <= '9') || acName[0] == 'N' ? eKind : Kind::None;
}

// The vtable at auiVtable's offset to its complete object and its class.
bool vtable(uintptr_t auiVtable, intptr_t& arOffsetToTop, Dynamic& arOut)
{
	intptr_t aiHeader[2];
	if (!kImage.Contains(auiVtable - sizeof(aiHeader), sizeof(aiHeader)) ||
		!readMemory(aiHeader, auiVtable - sizeof(aiHeader), sizeof(aiHeader)) || aiHeader[0] > 0 ||
		aiHeader[0] < -(intptr_t(1) << 20) || typeInfo(static_cast<uintptr_t>(aiHeader[1]), arOut.acName) == Kind::None) {
		return false;
	}
	arOffsetToTop = aiHeader[0];
	arOut.uiType = static_cast<uintptr_t>(aiHeader[1]);
	return true;
}

// Whether the mangled name is the namespace-scope class apClass ("7TESForm").
bool isClass(const char* apMangled, const char* apClass)
{
	size_t uiLength = 0;
	while (*apMangled >= '0' && *apMangled <= '9') {
		uiLength = uiLength * 10 + static_cast<size_t>(*apMangled++ - '0');
	}
	return uiLength == std::strlen(apClass) && std::strcmp(apMangled, apClass) == 0;
}

// The offset of the base apClass inside the class auiType, searched depth first; false when it is not there, or
// only behind a virtual base.
bool baseOffset(uintptr_t auiType, const char* apClass, int32_t aiDepth, intptr_t& arOffset)
{
	char acName[160];
	const Kind eKind = typeInfo(auiType, acName);
	if (eKind == Kind::None || aiDepth > 32) {
		return false;
	}
	if (isClass(acName, apClass)) {
		arOffset = 0;
		return true;
	}
	if (eKind == Kind::Single) {
		uintptr_t uiBase = 0;
		return readMemory(&uiBase, auiType + sizeof(ClassTypeInfo), sizeof(uiBase)) &&
			baseOffset(uiBase, apClass, aiDepth + 1, arOffset);
	}
	if (eKind == Kind::Multiple) {
		VmiClassTypeInfo kType;
		if (!readMemory(&kType, auiType, sizeof(kType)) || kType.uiBaseCount > 64) {
			return false;
		}
		for (uint32_t i = 0; i < kType.uiBaseCount; ++i) {
			BaseClassInfo kBase;
			intptr_t iInner = 0;
			if (!readMemory(&kBase, auiType + sizeof(kType) + i * sizeof(kBase), sizeof(kBase))) {
				return false;
			}
			if (baseOffset(kBase.uiType, apClass, aiDepth + 1, iInner)) {
				if (kBase.iOffsetFlags & kVirtualBase) {
					return false;
				}
				arOffset = (kBase.iOffsetFlags >> 8) + iInner;
				return true;
			}
		}
	}
	return false;
}

} // namespace

bool vtableType(uintptr_t auiVtable, Dynamic& arOut)
{
	intptr_t iOffsetToTop = 0;
	return vtable(auiVtable, iOffsetToTop, arOut);
}

bool dynamicType(uintptr_t auiValue, Dynamic& arOut)
{
	uintptr_t uiVtable = 0;
	intptr_t iOffsetToTop = 0;
	if (!readMemory(&uiVtable, auiValue, sizeof(uiVtable)) || !vtable(uiVtable, iOffsetToTop, arOut)) {
		return false;
	}
	arOut.uiComplete = auiValue + iOffsetToTop;
	return true;
}

bool findBase(const Dynamic& arObject, const char* apClass, uintptr_t& arBase)
{
	intptr_t iOffset = 0;
	if (!baseOffset(arObject.uiType, apClass, 0, iOffset)) {
		return false;
	}
	arBase = arObject.uiComplete + iOffset;
	return true;
}

// "7TESForm" is TESForm, "N4Tile5ExtraE" Tile::Extra; any other form (templates, substitutions) is printed as it is.
void className(Out& arOut, const Dynamic& arObject)
{
	const char* pName = arObject.acName;
	const bool bNested = *pName == 'N';
	const char* p = pName + (bNested ? 1 : 0);
	char acPlain[160];
	size_t uiPlain = 0;
	while (*p >= '1' && *p <= '9') {
		size_t uiLength = 0;
		while (*p >= '0' && *p <= '9') {
			uiLength = uiLength * 10 + static_cast<size_t>(*p++ - '0');
		}
		if (std::strlen(p) < uiLength || uiPlain + uiLength + 3 > sizeof(acPlain)) {
			break;
		}
		if (uiPlain) {
			acPlain[uiPlain++] = ':';
			acPlain[uiPlain++] = ':';
		}
		std::memcpy(acPlain + uiPlain, p, uiLength);
		uiPlain += uiLength;
		p += uiLength;
		if (!bNested) {
			break;
		}
	}
	if (uiPlain && ((bNested && p[0] == 'E' && !p[1]) || (!bNested && !*p))) {
		acPlain[uiPlain] = '\0';
		arOut.str(acPlain);
		return;
	}
	arOut.str(pName);
}

} // namespace fnvd::crashlog
