// The crash log's RTTI on the MSVC ABI (the Windows programs): an object's class from the complete object locator
// before its vtable, its bases from the class hierarchy descriptor. Every record is read through readMemory and
// trusted only inside the image.

#include "xfnv/crashlog/CrashLog.hpp"

namespace fnvd::crashlog {
namespace {

// The MSVC RTTI records (rttidata.h). On x86 their links are 32-bit addresses; on x64 they are offsets from the
// image base and the locator's signature is 1 (its last field, its own offset, is not read).
constexpr bool k64 = sizeof(void*) == 8;
struct CompleteObjectLocator {
	uint32_t uiSignature;
	uint32_t uiOffset;
	uint32_t uiCdOffset;
	uint32_t uiTypeDescriptor;
	uint32_t uiClassDescriptor;
};
struct ClassHierarchyDescriptor {
	uint32_t uiSignature;
	uint32_t uiAttributes;
	uint32_t uiNumBaseClasses;
	uint32_t uiBaseClassArray;
};
struct BaseClassDescriptor {
	uint32_t uiTypeDescriptor;
	uint32_t uiNumContainedBases;
	int32_t iMdisp;
	int32_t iPdisp;
	int32_t iVdisp;
	uint32_t uiAttributes;
};
// A type descriptor's decorated name starts after its vtable pointer and spare word.
constexpr uint32_t kTypeNameOffset = 2 * sizeof(void*);
constexpr uint32_t kLocatorSignature = k64 ? 1 : 0;

// The address an RTTI link names: the link itself on x86, the image base plus it on x64.
uintptr_t rttiLink(uint32_t auiLink)
{
	return k64 ? kImage.uiBegin + auiLink : auiLink;
}

// The decorated name of the type descriptor at auiDescriptor, into acName; false unless it reads as one.
bool typeName(uint32_t auiDescriptor, char (&acName)[160])
{
	const uintptr_t uiName = rttiLink(auiDescriptor) + kTypeNameOffset;
	if (!kImage.Contains(uiName, 4)) {
		return false;
	}
	size_t uiSize = kImage.uiEnd - uiName;
	uiSize = uiSize < sizeof(acName) ? uiSize : sizeof(acName);
	if (!readMemory(acName, uiName, uiSize)) {
		return false;
	}
	acName[sizeof(acName) - 1] = '\0';
	return acName[0] == '.' && acName[1] == '?' && acName[2] == 'A';
}

// The locator of the vtable at auiVtable, when there is one.
bool locator(uintptr_t auiVtable, CompleteObjectLocator& arLocator, char (&acName)[160])
{
	uintptr_t uiLocator = 0;
	if (!kImage.Contains(auiVtable - sizeof(uiLocator), sizeof(uiLocator)) ||
		!readMemory(&uiLocator, auiVtable - sizeof(uiLocator), sizeof(uiLocator)) ||
		!kImage.Contains(uiLocator, sizeof(arLocator)) || !readMemory(&arLocator, uiLocator, sizeof(arLocator))) {
		return false;
	}
	return arLocator.uiSignature == kLocatorSignature && typeName(arLocator.uiTypeDescriptor, acName);
}

} // namespace

bool vtableType(uintptr_t auiVtable, Dynamic& arOut)
{
	CompleteObjectLocator kLocator;
	if (!locator(auiVtable, kLocator, arOut.acName)) {
		return false;
	}
	arOut.uiType = rttiLink(kLocator.uiClassDescriptor);
	return true;
}

bool dynamicType(uintptr_t auiValue, Dynamic& arOut)
{
	uintptr_t uiVtable = 0;
	CompleteObjectLocator kLocator;
	if (!readMemory(&uiVtable, auiValue, sizeof(uiVtable)) || !locator(uiVtable, kLocator, arOut.acName)) {
		return false;
	}
	arOut.uiComplete = auiValue - kLocator.uiOffset;
	arOut.uiType = rttiLink(kLocator.uiClassDescriptor);
	return true;
}

bool findBase(const Dynamic& arObject, const char* apClass, uintptr_t& arBase)
{
	ClassHierarchyDescriptor kHierarchy;
	if (!kImage.Contains(arObject.uiType, sizeof(kHierarchy)) ||
		!readMemory(&kHierarchy, arObject.uiType, sizeof(kHierarchy)) || kHierarchy.uiNumBaseClasses > 512) {
		return false;
	}
	for (uint32_t i = 0; i < kHierarchy.uiNumBaseClasses; ++i) {
		uint32_t uiEntry = 0;
		BaseClassDescriptor kBase;
		char acName[160];
		const uintptr_t uiSlot = rttiLink(kHierarchy.uiBaseClassArray) + 4 * i;
		if (!kImage.Contains(uiSlot, 4) || !readMemory(&uiEntry, uiSlot, 4) ||
			!kImage.Contains(rttiLink(uiEntry), sizeof(kBase)) ||
			!readMemory(&kBase, rttiLink(uiEntry), sizeof(kBase)) || !typeName(kBase.uiTypeDescriptor, acName)) {
			return false;
		}
		// ".?AV" (a class) or ".?AU" (a struct), the name, "@@"
		const char* pWanted = apClass;
		const char* pName = acName + 4;
		while (*pWanted && *pWanted == *pName) {
			++pWanted;
			++pName;
		}
		if (!*pWanted && pName[0] == '@' && pName[1] == '@' && !pName[2]) {
			if (kBase.iPdisp != -1) {
				return false;
			}
			arBase = arObject.uiComplete + kBase.iMdisp;
			return true;
		}
	}
	return false;
}

// "NiTPointerList@PAVTile@@" style names are printed as they are; plain nested names ("Extra@Tile@@") become
// "Tile::Extra".
void className(Out& arOut, const Dynamic& arObject)
{
	const char* pRest = arObject.acName + 4;
	for (const char* p = pRest; *p; ++p) {
		if (*p == '?' || *p == '$') {
			arOut.str(pRest);
			return;
		}
	}
	// the parts, innermost first, each ending at '@'; the list ends at "@@"
	const char* apParts[16];
	int32_t iParts = 0;
	for (const char* p = pRest; *p && *p != '@' && iParts < 16;) {
		apParts[iParts++] = p;
		while (*p && *p != '@') {
			++p;
		}
		if (*p) {
			++p;
		}
	}
	for (int32_t i = iParts - 1; i >= 0; --i) {
		for (const char* p = apParts[i]; *p && *p != '@'; ++p) {
			arOut.chr(*p);
		}
		if (i) {
			arOut.str("::");
		}
	}
}

} // namespace fnvd::crashlog
