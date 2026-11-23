#include "Gamebryo2.2/CoreLibs/NiAnimation/BSAnimNote.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSAnimNote::ms_RTTI("BSAnimNote", &NiObject::ms_RTTI);
NiRTTI BSAnimNotes::ms_RTTI("BSAnimNotes", &NiObject::ms_RTTI);

BSAnimNote::~BSAnimNote() {}

const NiRTTI* BSAnimNote::GetRTTI() const {
    return &ms_RTTI;
}

void BSAnimNote::LoadBinary(NiStream& arStream) {
    NiObject::LoadBinary(arStream);
    NiStreamLoadBinary(arStream, eType);
    NiStreamLoadBinary(arStream, fTime);
}

void BSAnimNote::LinkObject(NiStream& arStream) {
    NiObject::LinkObject(arStream);
}

bool BSAnimNote::RegisterStreamables(NiStream& arStream) {
    return NiObject::RegisterStreamables(arStream);
}

void BSAnimNote::SaveBinary(NiStream& arStream) {
    NiObject::SaveBinary(arStream);
    NiStreamSaveBinary(arStream, eType);
    NiStreamSaveBinary(arStream, fTime);
}

bool BSAnimNote::IsEqual(NiObject* apObject) {
    if (!NiObject::IsEqual(apObject)) {
        return false;
    }

    BSAnimNote* pNote = static_cast<BSAnimNote*>(apObject);
    return eType == pNote->eType && fTime == pNote->fTime;
}

void BSAnimNote::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiObject::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString("Time", fTime));
    // a grab IK note lists both its own type and the generic one
    switch (eType) {
        case BS_ANT_GRABIK: apStrings->Add(NiGetViewerString("Type", "GrabIK")); [[fallthrough]];
        default: apStrings->Add(NiGetViewerString("Type", "AnimNote")); break;
    }
}

bool BSAnimNote::ParseTextKey(const NiTextKey* apTextKey) {
    fTime = apTextKey->m_fTime;
    return true;
}

void BSAnimNote::CopyMembers(BSAnimNote* pDest, NiCloningProcess& arCloneData) {
    NiObject::CopyMembers(pDest, arCloneData);
    pDest->eType = eType;
    pDest->fTime = fTime;
}

BSAnimNotes::BSAnimNotes() {}

BSAnimNotes::~BSAnimNotes() {
    for (uint32_t i = 0; i < usNumNotes; ++i) {
        BSAnimNote* pNote = NoteA[i];
        if (pNote) {
            delete pNote;
        }
    }
    MemoryManager::Instance().Deallocate(NoteA);
}

const NiRTTI* BSAnimNotes::GetRTTI() const {
    return &ms_RTTI;
}

// The note pointers are left unset here; LinkObject fills them in. A zero count keeps the old array pointer.
void BSAnimNotes::LoadBinary(NiStream& arStream) {
    NiObject::LoadBinary(arStream);
    NiStreamLoadBinary(arStream, usNumNotes);
    if (usNumNotes) {
        NoteA = static_cast<BSAnimNote**>(MemoryManager::Instance().Allocate(usNumNotes * sizeof(BSAnimNote*)));
    }

    for (uint32_t ui = 0; ui < usNumNotes; ++ui) {
        arStream.ReadLinkID();
    }
}

// The linked objects are taken as notes without a type check; each takes a reference.
void BSAnimNotes::LinkObject(NiStream& arStream) {
    NiObject::LinkObject(arStream);
    for (uint32_t ui = 0; ui < usNumNotes; ++ui) {
        NoteA[ui] = static_cast<BSAnimNote*>(arStream.GetObjectFromLinkID());
        NoteA[ui]->IncRefCount();
    }
}

bool BSAnimNotes::RegisterStreamables(NiStream& arStream) {
    bool bResult = NiObject::RegisterStreamables(arStream);
    for (uint32_t ui = 0; ui < usNumNotes && bResult; ++ui) {
        bResult = NoteA[ui]->RegisterStreamables(arStream);
    }

    return bResult;
}

void BSAnimNotes::SaveBinary(NiStream& arStream) {
    NiObject::SaveBinary(arStream);
    NiStreamSaveBinary(arStream, usNumNotes);
    for (uint32_t ui = 0; ui < usNumNotes; ++ui) {
        arStream.SaveLinkID(NoteA[ui]);
    }
}

// A jump to the NiObject body, shared by many classes.
bool BSAnimNotes::IsEqual(NiObject* apObject) {
    return NiObject::IsEqual(apObject);
}

void BSAnimNotes::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiObject::GetViewerStrings(apStrings);
}

NiObject* BSAnimNotes::CreateObject() {
    return new BSAnimNotes;
}
