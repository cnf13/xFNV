#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineBasisData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiBSplineBasisData::ms_RTTI("NiBSplineBasisData", &NiObject::ms_RTTI);

// m_kBasisDegree3 is built by its own default constructor.
NiBSplineBasisData::NiBSplineBasisData() : NiObject() {}

NiBSplineBasisData::~NiBSplineBasisData() {}

const NiRTTI* NiBSplineBasisData::GetRTTI() const {
    return &ms_RTTI;
}

// The clone gets the whole basis, cached values included.
NiObject* NiBSplineBasisData::CreateClone(NiCloningProcess& kCloning) {
    NiBSplineBasisData* pkClone = new NiBSplineBasisData;
    NiObject::CopyMembers(pkClone, kCloning);
    pkClone->m_kBasisDegree3 = m_kBasisDegree3;
    return pkClone;
}

// Only the quantity of control points is streamed; the values are computed on use.
void NiBSplineBasisData::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_kBasisDegree3.m_iQuantity);
}

void NiBSplineBasisData::LinkObject(NiStream& stream) {
    NiObject::LinkObject(stream);
}

bool NiBSplineBasisData::RegisterStreamables(NiStream& stream) {
    return NiObject::RegisterStreamables(stream);
}

void NiBSplineBasisData::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_kBasisDegree3.m_iQuantity);
}

bool NiBSplineBasisData::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    return m_kBasisDegree3.IsEqual(&static_cast<NiBSplineBasisData*>(pkObject)->m_kBasisDegree3);
}

void NiBSplineBasisData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiObject::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

NiBSplineBasis<float, 3>& NiBSplineBasisData::GetDegree3Basis() {
    return m_kBasisDegree3;
}

NiObject* NiBSplineBasisData::CreateObject() {
    return new NiBSplineBasisData;
}
