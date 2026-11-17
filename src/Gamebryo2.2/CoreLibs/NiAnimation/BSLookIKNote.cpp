#include "Gamebryo2.2/CoreLibs/NiAnimation/BSLookIKNote.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/BSAnimNote.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSLookIKNote::ms_RTTI("BSLookIKNote", &BSAnimNote::ms_RTTI);

BSLookIKNote::BSLookIKNote() : BSAnimNote(BSAnimNote::BS_ANT_LOOKIK, 0.0f), fGain(0.0f), eState(BS_LIK_NONE) {}

BSLookIKNote::~BSLookIKNote() {}

const NiRTTI* BSLookIKNote::GetRTTI() const {
	return &ms_RTTI;
}

NiObject* BSLookIKNote::CreateClone(NiCloningProcess& arCloneData) {
	NiObject* pkCtlr = static_cast<NiObject*>(NiMemObject::operator new(sizeof(BSLookIKNote)));
	BSLookIKNote* pkCtlr_2 = static_cast<BSLookIKNote*>(pkCtlr);
	if (pkCtlr) {
		::new (pkCtlr) BSLookIKNote;
	} else {
		pkCtlr_2 = 0;
	}
	CopyMembers(pkCtlr_2, arCloneData);
	pkCtlr_2->fGain = fGain;
	pkCtlr_2->eState = eState;
	return pkCtlr_2;
}

void BSLookIKNote::LoadBinary(NiStream& arStream) {
	BSAnimNote::LoadBinary(arStream);
	NiStreamLoadBinary(arStream, fGain);
	NiStreamLoadBinary(arStream, eState);
}

void BSLookIKNote::LinkObject(NiStream& arStream) {
	BSAnimNote::LinkObject(arStream);
}

// A jump to the NiAVObjectPalette body; BSGrabIKNote shares it.
bool BSLookIKNote::RegisterStreamables(NiStream& arStream) {
	return BSAnimNote::RegisterStreamables(arStream);
}

void BSLookIKNote::SaveBinary(NiStream& arStream) {
	BSAnimNote::SaveBinary(arStream);
	NiStreamSaveBinary(arStream, fGain);
	NiStreamSaveBinary(arStream, eState);
}

bool BSLookIKNote::IsEqual(NiObject* apObject) {
	if (!BSAnimNote::IsEqual(apObject))
		return false;

	BSLookIKNote* pNote = static_cast<BSLookIKNote*>(apObject);
	return pNote->fGain == fGain && eState == pNote->eState;
}

// The switch has no breaks: each case also adds the strings of the cases below it.
void BSLookIKNote::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	BSAnimNote::GetViewerStrings(apStrings);
	apStrings->Add(NiGetViewerString("Gain", fGain));
	switch (eState) {
	case BS_LIK_START:
		apStrings->Add(NiGetViewerString("State", "Start"));
		[[fallthrough]];
	case BS_LIK_STOP:
		apStrings->Add(NiGetViewerString("State", "Stop"));
		[[fallthrough]];
	default:
		apStrings->Add(NiGetViewerString("State", "None"));
		break;
	}
}

// Parses "LookIK <gain> <Start|Stop>" after the base note's fields.
// Unregistered (KEEP, src cannot express it exactly): strncpy copies four characters of the gain with no
// terminator (0xA4409A), and the NiString constructor (0xA440AB) reads on past the 4-byte buffer at [esp+0xC] into
// the return address above it (its high byte 0 ends the string), so the exe's NiString holds up to three return
// address bytes after the gain; src's stack has other bytes after the buffer. The exe builds the NiString in
// apTextKey's argument slot.
bool BSLookIKNote::ParseTextKey(const NiTextKey* apTextKey) {
	char acGain[4];
	bool bResult = BSAnimNote::ParseTextKey(apTextKey);
	const char* pcText = apTextKey->m_kText.GetHandle();
	if (std::strncmp(pcText, "LookIK", 6u)) {
		return bResult;
	}
	pcText += 6;
	if (!bResult || *pcText != ' ') {
		return false;
	}
	++pcText;
	std::strncpy(acGain, pcText, 4u);
	{
		FNVD_CONSTRUCTED_LOCAL(NiString, kGain, &NiString::Destruct, acGain);
		if (!kGain.ToFloat(fGain)) {
			bResult = false;
		}
	}
	pcText += 4;
	if (!bResult || *pcText != ' ') {
		return bResult;
	}
	++pcText;
	if (!std::strncmp(pcText, "Start", 5u)) {
		eState = BSLookIKNote::BS_LIK_START;
	}
	if (!std::strncmp(pcText, "Stop", 4u)) {
		eState = BSLookIKNote::BS_LIK_STOP;
	}
	return bResult;
}

NiObject* BSLookIKNote::CreateObject() {
	BSLookIKNote* pkObject = static_cast<BSLookIKNote*>(NiMemObject::operator new(sizeof(BSLookIKNote)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) BSLookIKNote;
}
