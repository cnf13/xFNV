// An object's identity by src's own types: a node's name, a form's id, type and name, a reference's base and cell, a
// tile's name; and the game's thread objects. Every object is read as a copy through readMemory.

#include "xfnv/crashlog/CrashLog.hpp"

#include "BSCore/BSThread.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"

namespace fnvd::crashlog {
namespace {

// A copy of the T at auiAddress, read as the exe lays it out (src's type); null when unreadable. One buffer per
// type: a caller takes what it needs before copying another object of the same type.
template <typename T>
const T* copyOf(uintptr_t auiAddress)
{
	alignas(16) static unsigned char acCopy[sizeof(T)];
	return readMemory(acCopy, auiAddress, sizeof(T)) ? reinterpret_cast<const T*>(acCopy) : nullptr;
}

void quoted(Out& arOut, const void* apText)
{
	if (apText) {
		arOut.chr('"').guardedStr(reinterpret_cast<uintptr_t>(apText), 80).chr('"');
	}
}

// " form 0001A2B3 type 42 "Doc Mitchell"" for the TESForm at auiForm (any pointer to a form object).
void formIdentity(Out& arOut, uintptr_t auiObject)
{
	Dynamic kObject;
	uintptr_t uiForm = 0;
	if (!dynamicType(auiObject, kObject) || !findBase(kObject, "TESForm", uiForm)) {
		return;
	}
	if (const TESForm* pForm = copyOf<TESForm>(uiForm)) {
		arOut.str(" form ").hex(pForm->iFormID).str(" type ").dec(pForm->cFormType);
	}
	uintptr_t uiFullName = 0;
	if (findBase(kObject, "TESFullName", uiFullName)) {
		if (const TESFullName* pName = copyOf<TESFullName>(uiFullName); pName && pName->cFullName.pString) {
			arOut.chr(' ');
			quoted(arOut, pName->cFullName.pString);
		}
	}
}

void tileName(Out& arOut, uintptr_t auiObject)
{
	Dynamic kObject;
	uintptr_t uiTile = 0;
	if (dynamicType(auiObject, kObject) && findBase(kObject, "Tile", uiTile)) {
		if (const Tile* pTile = copyOf<Tile>(uiTile); pTile && pTile->xName.pString) {
			arOut.str(" tile ");
			quoted(arOut, pTile->xName.pString);
		}
	}
}

} // namespace

void identity(Out& arOut, const Dynamic& arObject)
{
	uintptr_t uiBase = 0;
	if (findBase(arObject, "NiObjectNET", uiBase)) {
		if (const NiObjectNET* pObject = copyOf<NiObjectNET>(uiBase); pObject && pObject->m_kName.GetHandle()) {
			arOut.str(" name ");
			quoted(arOut, pObject->m_kName.GetHandle());
		}
	}
	formIdentity(arOut, arObject.uiComplete);
	if (findBase(arObject, "TESObjectREFR", uiBase)) {
		if (const TESObjectREFR* pRef = copyOf<TESObjectREFR>(uiBase)) {
			const uintptr_t uiObject = reinterpret_cast<uintptr_t>(pRef->data.pObjectReference);
			const uintptr_t uiCell = reinterpret_cast<uintptr_t>(pRef->pParentCell);
			arOut.str(" | base");
			formIdentity(arOut, uiObject);
			arOut.str(" | cell");
			formIdentity(arOut, uiCell);
		}
	}
	tileName(arOut, arObject.uiComplete);
	if (findBase(arObject, "Menu", uiBase)) {
		if (const Menu* pMenu = copyOf<Menu>(uiBase)) {
			tileName(arOut, reinterpret_cast<uintptr_t>(pMenu->pMenu));
		}
	}
}

bool threadObject(Out& arOut, uintptr_t auiValue, uint32_t auiThreadId)
{
	Dynamic kObject;
	uintptr_t uiThread = 0;
	if (!dynamicType(auiValue, kObject) || !findBase(kObject, "BSThread", uiThread)) {
		return false;
	}
	const BSThread* pThread = copyOf<BSThread>(uiThread);
	if (!pThread || pThread->m_ThreadID != auiThreadId) {
		return false;
	}
	arOut.str(" ");
	className(arOut, kObject);
	arOut.str(" at ").hex(kObject.uiComplete);
	return true;
}

} // namespace fnvd::crashlog
