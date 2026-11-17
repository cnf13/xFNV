#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysPositionModifier.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

NiRTTI NiPSysPositionModifier::ms_RTTI("NiPSysPositionModifier", &NiPSysModifier::ms_RTTI);

NiPSysPositionModifier::NiPSysPositionModifier() : NiPSysModifier() {}

NiPSysPositionModifier::~NiPSysPositionModifier() {}

NiObject* NiPSysPositionModifier::CreateClone(NiCloningProcess& kCloning) {
    void* v3 = NiMemObject::operator new(sizeof(NiPSysPositionModifier));
    if (v3) {
        NiPSysPositionModifier* v4 = ::new (v3) NiPSysPositionModifier;
        NiPSysModifier::CopyMembers(v4, kCloning);
        return v4;
    }

    NiPSysModifier::CopyMembers(nullptr, kCloning);
    return nullptr;
}

void NiPSysPositionModifier::LoadBinary(NiStream& kStream) {
    NiPSysModifier::LoadBinary(kStream);
}

void NiPSysPositionModifier::LinkObject(NiStream& kStream) {
    NiPSysModifier::LinkObject(kStream);
}

bool NiPSysPositionModifier::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifier::RegisterStreamables(kStream);
}

void NiPSysPositionModifier::SaveBinary(NiStream& kStream) {
    NiPSysModifier::SaveBinary(kStream);
}

bool NiPSysPositionModifier::IsEqual(NiObject* pkObject) {
    return NiPSysModifier::IsEqual(pkObject);
}

void NiPSysPositionModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifier::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSysPositionModifier::ms_RTTI.GetName()));
}

// Added particles are resolved first. The particle info pointer is read once; the vertex pointer and the active
// count are read on every step.
void NiPSysPositionModifier::Update(float fTime, NiPSysData* pkData) {
    pkData->ResolveAddedParticles();

    NiParticleInfo* pkInfos = pkData->m_pkParticleInfo;
    for (uint16_t i = 0; i < pkData->m_usActive; i++) {
        NiParticleInfo* pkInfo = &pkInfos[i];
        float fDeltaT = fTime - pkInfo->m_fLastUpdate;
        float fMoveX = pkInfo->m_kVelocity.x * fDeltaT;
        float fMoveY = pkInfo->m_kVelocity.y * fDeltaT;
        float fMoveZ = pkInfo->m_kVelocity.z * fDeltaT;
        NiPoint3* pkVertex = &pkData->m_pkVertex[i];
        pkVertex->x += fMoveX;
        pkVertex->y += fMoveY;
        pkVertex->z += fMoveZ;
        pkInfo->m_fLastUpdate = fTime;
    }
}

NiObject* NiPSysPositionModifier::CreateObject() {
    NiPSysPositionModifier* v0 =
        static_cast<NiPSysPositionModifier*>(NiMemObject::operator new(sizeof(NiPSysPositionModifier)));
    if (!v0) {
        return nullptr;
    }
    return ::new (v0) NiPSysPositionModifier;
}
