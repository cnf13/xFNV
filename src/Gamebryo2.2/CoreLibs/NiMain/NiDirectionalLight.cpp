#include "Gamebryo2.2/CoreLibs/NiMain/NiDirectionalLight.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiDirectionalLight::ms_RTTI("NiDirectionalLight", &NiLight::ms_RTTI);

NiDirectionalLight::NiDirectionalLight() : m_kWorldDir(1.0f, 0.0f, 0.0f) {
    m_ucEffectType = DIR_LIGHT;
}

NiDirectionalLight::~NiDirectionalLight() {}

const NiRTTI* NiDirectionalLight::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiDirectionalLight::CreateClone(NiCloningProcess& kCloning) {
    NiDirectionalLight* v4;
    void* v3 = NiMemObject::operator new(sizeof(NiDirectionalLight));
    if (v3) {
        v4 = ::new (v3) NiDirectionalLight;
    }
    else {
        v4 = nullptr;
    }
    NiLight::CopyMembers(v4, kCloning);
    v4->m_kWorldDir.x = m_kWorldDir.x;
    v4->m_kWorldDir.y = m_kWorldDir.y;
    v4->m_kWorldDir.z = m_kWorldDir.z;
    return v4;
}

void NiDirectionalLight::LoadBinary(NiStream& stream) {
    NiLight::LoadBinary(stream);
}

void NiDirectionalLight::LinkObject(NiStream& stream) {
    NiLight::LinkObject(stream);
}

bool NiDirectionalLight::RegisterStreamables(NiStream& stream) {
    return NiLight::RegisterStreamables(stream);
}

void NiDirectionalLight::SaveBinary(NiStream& stream) {
    NiLight::SaveBinary(stream);
}

bool NiDirectionalLight::IsEqual(NiObject* pObject) {
    return NiLight::IsEqual(pObject);
}

void NiDirectionalLight::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiLight::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// The light shines along the world rotation's first column.
void NiDirectionalLight::UpdateWorldData(NiUpdateData& UpdateData) {
    NiAVObject::UpdateWorldData(UpdateData);
    m_kWorldDir.x = m_kWorld.m_Rotate.m_pEntry[0][0];
    m_kWorldDir.y = m_kWorld.m_Rotate.m_pEntry[1][0];
    m_kWorldDir.z = m_kWorld.m_Rotate.m_pEntry[2][0];
    m_uiRevID++;
}

NiObject* NiDirectionalLight::CreateObject() {
    NiDirectionalLight* v0 = static_cast<NiDirectionalLight*>(NiMemObject::operator new(sizeof(NiDirectionalLight)));
    if (v0) {
        return ::new (v0) NiDirectionalLight;
    }

    return nullptr;
}
