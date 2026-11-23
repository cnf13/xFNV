#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiBinaryExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <cstring>

NiRTTI NiExtraData::ms_RTTI("NiExtraData", &NiObject::ms_RTTI);

NiExtraData::NiExtraData() {}

NiExtraData::NiExtraData(const NiFixedString& kName) {
    m_kName = kName;
}

// m_kName's own destructor drops the name's reference.
NiExtraData::~NiExtraData() {}

void NiExtraData::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);

    if (kStream.GetFileVersion() < NiStream::GetVersion(5, 0, 0, 11)) {
        // Older files: a link to the name (resolved in LinkObject), then an unnamed blob. A plain NiExtraData with a
        // blob is replaced by an NiBinaryExtraData holding it.
        kStream.ReadLinkID();
        uint32_t uiSize;
        NiStreamLoadBinary(kStream, uiSize);
        if (uiSize > 0 && GetRTTI() == &NiExtraData::ms_RTTI) {
            char* pcData = static_cast<char*>(_NiMalloc(uiSize));
            NiStreamLoadBinary(kStream, pcData, uiSize);
            NiBinaryExtraData* pkBinaryData = new NiBinaryExtraData(uiSize, pcData);
            if (pkBinaryData) {
                kStream.ChangeObject(pkBinaryData);
            }
        }
    }
    else if (kStream.GetFileVersion() < NiStream::GetVersion(20, 1, 0, 1)) {
        kStream.LoadCStringAsFixedString(m_kName);
    }
    else {
        kStream.LoadFixedString(m_kName);
    }
}

// KEEP: niread: before 5.0.0.11 the blob size is read into the stream argument's own stack slot, so a short read
// (malformed data) leaves the stream's address there as the size it allocates and reads; src's size is
// an unwritten local and src cannot express the slot's reuse. porting.md's unwritten-slot exception covers
// GetChunkData only.
void NiExtraData::LinkObject(NiStream& kStream) {
    if (kStream.GetFileVersion() < NiStream::GetVersion(5, 0, 0, 11)) {
        // The linked object's pointer is stored into the name's handle as is, with no string reference taken;
        // NiFixedString offers no such assignment, so the raw store is spelled out. The old handle is deliberately
        // dropped unreleased; only NIFs older than 5.0.0.11 reach this.
        NiObject* pkObject = kStream.GetObjectFromLinkID();
        std::memcpy(static_cast<void*>(&m_kName), &pkObject, sizeof(pkObject));
    }
}

bool NiExtraData::RegisterStreamables(NiStream& kStream) {
    if (!NiObject::RegisterStreamables(kStream)) {
        return false;
    }

    return kStream.RegisterFixedString(m_kName);
}

void NiExtraData::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
    kStream.SaveFixedString(m_kName);
}

bool NiExtraData::IsEqual(NiObject* pObject) {
    if (!NiObject::IsEqual(pObject)) {
        return false;
    }

    return m_kName == static_cast<NiExtraData*>(pObject)->m_kName;
}

// Unlike most classes, the lines start with this class' heading: NiObject's lines are not added.
void NiExtraData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    pkStrings->Add(NiGetViewerString(NiExtraData::ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_pcName", static_cast<const char*>(m_kName)));
}

void NiExtraData::CopyMembers(NiExtraData* pDest, NiCloningProcess& kCloning) {
    NiObject::CopyMembers(pDest, kCloning);
    pDest->m_kName = m_kName;
}

const NiFixedString& NiExtraData::GetName() const {
    return m_kName;
}

void NiExtraData::SetName(const NiFixedString& kName) {
    m_kName.Assign(kName);
}

void NiExtraData::ClearName() {
    m_kName.SetHandle(nullptr);
}

NiObject* NiExtraData::CreateObject() {
    return new NiExtraData;
}
