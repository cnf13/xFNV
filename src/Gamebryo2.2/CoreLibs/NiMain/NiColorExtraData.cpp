#include "Gamebryo2.2/CoreLibs/NiMain/NiColorExtraData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiColorExtraData::ms_RTTI("NiColorExtraData", &NiExtraData::ms_RTTI);

NiColorExtraData::NiColorExtraData() {
    m_kColorA = NiColorA_BLACK;
}

NiColorExtraData::~NiColorExtraData() {}

const NiRTTI* NiColorExtraData::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiColorExtraData::CreateClone(NiCloningProcess& kCloning) {
    NiColorExtraData* pkObject_2 = new NiColorExtraData;
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_kColorA = m_kColorA;
    return pkObject_2;
}

// Files before 5.0.0.14 store an opaque colour without alpha; it is read through NiPoint3::LoadBinary (the same
// three 4-byte reads as NiColor's).
void NiColorExtraData::LoadBinary(NiStream& kStream) {
    NiExtraData::LoadBinary(kStream);
    if (kStream.GetFileVersion() < NiStream::GetVersion(5, 0, 0, 14)) {
        NiPoint3 kColor;
        kColor.x = 0.0f;
        kColor.y = 0.0f;
        kColor.z = 0.0f;
        kColor.LoadBinary(kStream);
        m_kColorA = NiColorA(kColor.x, kColor.y, kColor.z, 1.0f);
    }
    else {
        m_kColorA.LoadBinary(kStream);
    }
}

void NiColorExtraData::LinkObject(NiStream& kStream) {
    NiExtraData::LinkObject(kStream);
}

bool NiColorExtraData::RegisterStreamables(NiStream& kStream) {
    return NiExtraData::RegisterStreamables(kStream);
}

void NiColorExtraData::SaveBinary(NiStream& kStream) {
    NiExtraData::SaveBinary(kStream);
    m_kColorA.SaveBinary(kStream);
}

// Only the colours are compared: neither the class nor the name is checked.
bool NiColorExtraData::IsEqual(NiObject* pObject) {
    if (!pObject) {
        return false;
    }

    return !(m_kColorA != static_cast<NiColorExtraData*>(pObject)->m_kColorA);
}

void NiColorExtraData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiExtraData::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pStrings->Add(m_kColorA.GetViewerString("Color = "));
}

void NiColorExtraData::SetValue(NiColorA akColorA) {
    m_kColorA = akColorA;
}

NiObject* NiColorExtraData::CreateObject() {
    return new NiColorExtraData;
}
