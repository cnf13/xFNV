#include "Gamebryo2.2/CoreLibs/NiMain/NiStringExtraData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiStringExtraData::ms_RTTI("NiStringExtraData", &NiExtraData::ms_RTTI);

NiStringExtraData::NiStringExtraData() {}

NiStringExtraData::~NiStringExtraData() {}

// The new object's string starts as NULL_STRING; NiExtraData's CopyMembers runs, then the string is assigned. The
// assignment is written out with the reference counts taken directly rather than through NiFixedString's operator=.
NiObject* NiStringExtraData::CreateClone(NiCloningProcess& kCloning) {
    NiStringExtraData* pkClone = static_cast<NiStringExtraData*>(NiMemObject::operator new(sizeof(NiStringExtraData)));
    if (pkClone) {
        ::new (pkClone) NiStringExtraData;
    }
    NiExtraData::CopyMembers(pkClone, kCloning);
    pkClone->m_kString = m_kString;
    return pkClone;
}

void NiStringExtraData::LoadBinary(NiStream& stream) {
    NiExtraData::LoadBinary(stream);

    if (stream.GetFileVersion() < NiStream::GetVersion(20, 1, 0, 1)) {
        stream.LoadCStringAsFixedString(m_kString);
    }
    else {
        stream.LoadFixedString(m_kString);
    }
}

void NiStringExtraData::LinkObject(NiStream& stream) {
    NiExtraData::LinkObject(stream);
}

bool NiStringExtraData::RegisterStreamables(NiStream& stream) {
    if (!NiExtraData::RegisterStreamables(stream)) {
        return false;
    }

    stream.RegisterFixedString(m_kString);
    return true;
}

void NiStringExtraData::SaveBinary(NiStream& stream) {
    NiExtraData::SaveBinary(stream);
    stream.SaveFixedString(m_kString);
}

// Neither the base class nor the type is checked: only the string handles are compared.
bool NiStringExtraData::IsEqual(NiObject* pObject) {
    if (!pObject) {
        return false;
    }

    return m_kString == static_cast<NiStringExtraData*>(pObject)->m_kString;
}

void NiStringExtraData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiExtraData::GetViewerStrings(pStrings);

    pStrings->Add(NiGetViewerString(NiStringExtraData::ms_RTTI.GetName()));
    pStrings->Add(NiGetViewerString("m_kString", static_cast<const char*>(m_kString)));
}

NiObject* NiStringExtraData::CreateObject() {
    NiStringExtraData* pkObject = static_cast<NiStringExtraData*>(NiMemObject::operator new(sizeof(NiStringExtraData)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiStringExtraData;
}
