#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

// A string interned in the global string table; equal strings share one handle.
class NiFixedString {
public:
	// The empty string: the shared null handle, no reference taken (inlined, e.g. NiTextKey's 0xA46B50).
	NiFixedString() : m_kHandle(NiGlobalStringTable::NULL_STRING) {}
	// noexcept: the exe's constructor (0x438170) never throws, so callers need no EH frame for it (as NiMemObject's
	// operator new).
	NiFixedString(const char* pcString) noexcept;
	// Takes a reference on arString's handle.
	NiFixedString(const NiFixedString& arString);
	// Drops the handle's reference.
	~NiFixedString();

	// equal by handle, else by contents
	bool Equals(const char* pcStr) const;

	// The interned characters, or null; interned strings with equal contents share this pointer.
	const char* GetHandle() const { return m_kHandle; }

	// Stores kHandle without touching any reference count: what an exe constructor writes for a default name.
	void SetHandle(NiGlobalStringTable::GlobalStringHandle kHandle) { m_kHandle = kHandle; }

	operator const char*() const { return m_kHandle; }
	// Not named in the PDB or IDA; name chosen here. The exe's out-of-line copy of the PDB's
	// NiFixedString::operator const char*() const (thiscall, ret 0, 0x43B1B0); the inline operator above stays for the
	// sites the exe inlines it at.
	const char* GetHandleString() const;

	bool Exists() const { return m_kHandle != nullptr; }

	// equal by handle only
	bool operator==(const NiFixedString& kString) const { return m_kHandle == kString.m_kHandle; }

	// Takes a reference on the new handle, then drops the old one's (inlined in the exe). Copy construction takes no
	// reference here (there is no destructor to give it back), so a copy must not be assigned to afterwards.
	NiFixedString& operator=(const NiFixedString& kString) {
		if (m_kHandle != kString.m_kHandle) {
			NiGlobalStringTable::GlobalStringHandle kHandle = kString.m_kHandle;
			NiGlobalStringTable::IncRefCount(kHandle);
			NiGlobalStringTable::DecRefCount(m_kHandle);
			m_kHandle = kHandle;
		}
		return *this;
	}

	// Not named in the PDB or IDA; name chosen here. The exe's out-of-line copy of the PDB's
	// NiFixedString::operator=(const NiFixedString&) (thiscall, ret 4, 0x43BA10; defined in ModelLoader.cpp), which
	// calls IncRefCount and DecRefCount out of line; the inline operator above stays for the sites the exe inlines
	// it at.
	// Inferred type: kString: NiFixedString* from 0x43ba01 (parameter 0 of NiControllerSequence::SetName), its members
	// used at 0x43ba21
	NiFixedString& Assign(const NiFixedString& kString);

	// Interns pcString (AddString's reference is the one kept), then drops the old handle's (inlined in the exe,
	// e.g. GetCtlrID 0xA4EBD0).
	NiFixedString& operator=(const char* pcString) {
		if (m_kHandle != pcString) {
			NiGlobalStringTable::GlobalStringHandle kHandle = m_kHandle;
			m_kHandle = NiGlobalStringTable::AddString(pcString);
			NiGlobalStringTable::DecRefCount(kHandle);
		}
		return *this;
	}

	bool Bool() const;
	// Inferred type: pcString: char* from 0xa2e75e (passed to NiGlobalStringTable::AddString 0)
	void* Set(char* pcString);
	bool EqualsNoCase(const char* pcStr);

	// Not named in the PDB or IDA; name chosen here. The exe's out-of-line handle compare of two strings (Gamebryo's
	// operator==(const NiFixedString&, const NiFixedString&)), cdecl, ret 0.
	static bool HandlesEqual(const NiFixedString& arString1, const NiFixedString& arString2);
	// Freed through ourFree (the exe's deleting destructor 0x4B9900).
	static void operator delete(void* apMemory);

	NiGlobalStringTable::GlobalStringHandle m_kHandle;
};

// Replaces kString with its standardized path (NiPath::Standardize on a 260-byte copy).
void NiStandardizeFilePath(NiFixedString& kString);

// An empty fixed string the exe reads in many places (IDA's NiFixedString::Empty; the PDB has no name for it). A
// global here because the class is incomplete inside its own body.
extern NiFixedString EmptyFS;
