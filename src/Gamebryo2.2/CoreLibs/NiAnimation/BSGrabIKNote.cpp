#include "Gamebryo2.2/CoreLibs/NiAnimation/BSGrabIKNote.h"

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

NiRTTI BSGrabIKNote::ms_RTTI("BSGrabIKNote", &BSAnimNote::ms_RTTI);

BSGrabIKNote::BSGrabIKNote() : BSAnimNote(BSAnimNote::BS_ANT_GRABIK, 0.0f), fGain(0.0f), eState(BS_GIK_NONE) {}

BSGrabIKNote::~BSGrabIKNote() {}

const NiRTTI* BSGrabIKNote::GetRTTI() const {
	return &ms_RTTI;
}

NiObject* BSGrabIKNote::CreateClone(NiCloningProcess& arCloneData) {
	BSGrabIKNote* pkCtlr = static_cast<BSGrabIKNote*>(NiMemObject::operator new(sizeof(BSGrabIKNote)));
	BSGrabIKNote* pkCtlr_2 = pkCtlr;
	if (pkCtlr) {
		::new (pkCtlr) BSGrabIKNote;
	} else {
		pkCtlr_2 = 0;
	}
	CopyMembers(pkCtlr_2, arCloneData);
	pkCtlr_2->eArm = eArm;
	pkCtlr_2->fGain = fGain;
	pkCtlr_2->eState = eState;
	return pkCtlr_2;
}

void BSGrabIKNote::LoadBinary(NiStream& arStream) {
	BSAnimNote::LoadBinary(arStream);
	NiStreamLoadBinary(arStream, eArm);
	NiStreamLoadBinary(arStream, fGain);
	NiStreamLoadBinary(arStream, eState);
}

void BSGrabIKNote::LinkObject(NiStream& arStream) {
	BSAnimNote::LinkObject(arStream);
}

bool BSGrabIKNote::RegisterStreamables(NiStream& arStream) {
	return BSAnimNote::RegisterStreamables(arStream);
}

void BSGrabIKNote::SaveBinary(NiStream& arStream) {
	BSAnimNote::SaveBinary(arStream);
	NiStreamSaveBinary(arStream, eArm);
	NiStreamSaveBinary(arStream, fGain);
	NiStreamSaveBinary(arStream, eState);
}

bool BSGrabIKNote::IsEqual(NiObject* apObject) {
	if (!BSAnimNote::IsEqual(apObject))
		return false;

	BSGrabIKNote* pNote = static_cast<BSGrabIKNote*>(apObject);
	return eArm == pNote->eArm && fGain == pNote->fGain && eState == pNote->eState;
}

// The switches have no breaks: each case also adds the strings of the cases below it.
void BSGrabIKNote::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	BSAnimNote::GetViewerStrings(apStrings);
	switch (eArm) {
	case BS_GIK_LEFT_ARM:
		apStrings->Add(NiGetViewerString("Arm", "Left"));
		[[fallthrough]];
	case BS_GIK_RIGHT_ARM:
		apStrings->Add(NiGetViewerString("Arm", "Right"));
		[[fallthrough]];
	default:
		apStrings->Add(NiGetViewerString("Arm", ""));
		break;
	}

	apStrings->Add(NiGetViewerString("Gain", fGain));
	switch (eState) {
	case BS_GIK_START:
		apStrings->Add(NiGetViewerString("State", "Start"));
		[[fallthrough]];
	case BS_GIK_STOP:
		apStrings->Add(NiGetViewerString("State", "Stop"));
		[[fallthrough]];
	default:
		apStrings->Add(NiGetViewerString("State", "None"));
		break;
	}
}

// Parses "GrabIK <R|L> <gain> <Start|Stop>" after the base note's fields.
// Unregistered (KEEP, src cannot express it exactly): strncpy copies four characters of the gain with no
// terminator (0xA44499), and the NiString constructor (0xA444AA) reads on past the 4-byte buffer at [esp+0xC] into
// the return address above it (its high byte 0 ends the string), so the exe's NiString holds up to three return
// address bytes after the gain; src's stack has other bytes after the buffer.
bool BSGrabIKNote::ParseTextKey(const NiTextKey* apTextKey) {
	char acGain[4];
	bool bResult = BSAnimNote::ParseTextKey(apTextKey);
	const char* pcText = apTextKey->m_kText.GetHandle();
	if (std::strncmp(pcText, "GrabIK", 6u)) {
		return bResult;
	}
	pcText += 6;
	if (*pcText == ' ') {
		char cArm = pcText[1];
		if (cArm == 'R' || cArm == 'L') {
			bResult = true;
			eArm = static_cast<BSGrabIKNote::GRABIK_ARM>(cArm == 'R');
		}
	}
	pcText += 2;
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
		pcText += 4;
	}
	if (!bResult || *pcText != ' ') {
		return bResult;
	}
	++pcText;
	if (!std::strncmp(pcText, "Start", 5u)) {
		eState = BSGrabIKNote::BS_GIK_START;
	}
	if (!std::strncmp(pcText, "Stop", 4u)) {
		eState = BSGrabIKNote::BS_GIK_STOP;
	}
	return bResult;
}

NiObject* BSGrabIKNote::CreateObject() {
	BSGrabIKNote* pkObject = static_cast<BSGrabIKNote*>(NiMemObject::operator new(sizeof(BSGrabIKNote)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) BSGrabIKNote;
}
