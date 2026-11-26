#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleMeshesData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticlesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cmath>

NiRTTI NiParticleMeshesData::ms_RTTI("NiParticleMeshesData", &NiParticlesData::ms_RTTI);

NiParticleMeshesData::NiParticleMeshesData() : m_spParticleMeshes(new NiNode(0)), m_bNeedsPropertyEffectUpdate(false) {}

NiParticleMeshesData::~NiParticleMeshesData() {
    m_spParticleMeshes = nullptr;
}

void NiParticleMeshesData::LoadBinary(NiStream& kStream) {
    NiParticlesData::LoadBinary(kStream);
    kStream.ReadLinkID();
}

void NiParticleMeshesData::LinkObject(NiStream& kStream) {
    NiParticlesData::LinkObject(kStream);
    m_spParticleMeshes = static_cast<NiNode*>(kStream.GetObjectFromLinkID());
}

bool NiParticleMeshesData::RegisterStreamables(NiStream& kStream) {
    if (!NiParticlesData::RegisterStreamables(kStream)) {
        return false;
    }
    m_spParticleMeshes->RegisterStreamables(kStream);
    return true;
}

void NiParticleMeshesData::SaveBinary(NiStream& kStream) {
    NiParticlesData::SaveBinary(kStream);
    kStream.SaveLinkID(m_spParticleMeshes.m_pObject);
}

// The mesh node's IsEqual decides the result.
bool NiParticleMeshesData::IsEqual(NiObject* pkObject) {
    if (!NiParticlesData::IsEqual(pkObject)) {
        return false;
    }
    return m_spParticleMeshes->IsEqual(static_cast<NiParticleMeshesData*>(pkObject)->m_spParticleMeshes.m_pObject);
}

void NiParticleMeshesData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiParticlesData::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.m_pcName));
}

// Detaches the meshes of the particles that go inactive, then clamps the count to the vertex count. The meshes node is
// reloaded on every pass and the 16-bit counts are compared unsigned.
void NiParticleMeshesData::SetActiveVertexCount(uint16_t usActive) {
    for (uint16_t usIndex = usActive; usIndex < m_usActive; ++usIndex) {
        NiNode* pkMeshes = m_spParticleMeshes.m_pObject;
        pkMeshes->DetachChildAt(usIndex);
    }
    uint16_t usVertices = m_usVertices;
    if (usActive > usVertices) {
        m_usActive = usVertices;
    }
    else {
        m_usActive = usActive;
    }
}

// The removed particle's mesh is detached; the last active one's mesh moves into its place.
void NiParticleMeshesData::RemoveParticle(int32_t aiParticle) {
    uint16_t usParticle = static_cast<uint16_t>(aiParticle);
    uint16_t usLast = m_usActive - 1;
    if (usParticle == usLast) {
        m_spParticleMeshes->DetachChildAt(usLast);
        NiParticlesData::RemoveParticle(aiParticle);
    }
    else {
        {
            NiPointer<NiAVObject> spLast;
            m_spParticleMeshes->DetachChildAt(usLast, spLast);
            m_spParticleMeshes->DetachChildAt(usParticle);
            m_spParticleMeshes->SetAt(usParticle, spLast.m_pObject);
        }
        NiParticlesData::RemoveParticle(aiParticle);
    }
}

void NiParticleMeshesData::UpdateParticles() {
    int32_t v2 = 0;
    uint32_t v3 = 0;
    if (m_usActive) {
        int32_t v9 = 0;
        do {
            NiAVObject* m_pObject = m_spParticleMeshes->m_kChildren.m_pBase[v3].m_pObject;
            if (m_pObject) {
                NiPoint3* m_pkVertex = this->m_pkVertex;
                float x = m_pkVertex[v2].x;
                NiPoint3* v7 = &m_pkVertex[v2];
                m_pObject->m_kLocal.m_Translate.x = x;
                m_pObject->m_kLocal.m_Translate.y = v7->y;
                m_pObject->m_kLocal.m_Translate.z = v7->z;
                NiQuaternion* m_pkRotations = this->m_pkRotations;
                if (m_pkRotations) {
                    m_pkRotations[v9].ToRotation(m_pObject->m_kLocal.m_Rotate);
                }
                float v10 = m_pfSizes[v3] * m_pfRadii[v3];
                float v11 = std::fabs(static_cast<double>(v10));
                m_pObject->m_kLocal.m_fScale = v11;
            }
            ++v9;
            ++v3;
            ++v2;
        }
        while (v3 < m_usActive);
    }
}

// Stores the old-format flag through the result even when the allocation failed.
NiObject* NiParticleMeshesData::CreateObject() {
    NiParticleMeshesData* pkData = new NiParticleMeshesData;
    pkData->m_bIsOldRotatingParticlesObject = true;
    return pkData;
}
