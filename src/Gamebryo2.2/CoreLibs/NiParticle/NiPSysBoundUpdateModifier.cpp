#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysBoundUpdateModifier.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/BSStripPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiMeshPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysBoundUpdateModifier::ms_RTTI("NiPSysBoundUpdateModifier", &NiPSysModifier::ms_RTTI);

NiPSysBoundUpdateModifier::NiPSysBoundUpdateModifier()
  : NiPSysModifier(), m_sUpdateSkip(0), m_usUpdateCount(0), m_pkSkipBounds(nullptr) {}

NiPSysBoundUpdateModifier::~NiPSysBoundUpdateModifier() {
    MemoryManager_Singleton.Deallocate(m_pkSkipBounds);
}

// The clone starts with no skip bounds; SetUpdateSkip then allocates them for the original's skip.
NiObject* NiPSysBoundUpdateModifier::CreateClone(NiCloningProcess& kCloning) {
    NiPSysBoundUpdateModifier* pkClone = new NiPSysBoundUpdateModifier;
    NiPSysModifier::CopyMembers(pkClone, kCloning);
    pkClone->SetUpdateSkip(static_cast<uint16_t>(m_sUpdateSkip));
    return pkClone;
}

// Both branches reset the per-pass bounds through SetUpdateSkip; files up to 10.1.0.100 read into a local first.
// KEEP: the old-version branch reads the 2-byte skip into a stack slot nothing initialised ([esp+4] at 0xC2E3A5) and
// passes it on, so a short read passes stack garbage (and the slot's upper word is never written).
void NiPSysBoundUpdateModifier::LoadBinary(NiStream& kStream) {
    NiPSysModifier::LoadBinary(kStream);

    if (kStream.GetFileVersion() > NiStream::GetVersion(10, 1, 0, 100)) {
        NiStreamLoadBinary(kStream, m_sUpdateSkip);
        SetUpdateSkip(static_cast<uint16_t>(m_sUpdateSkip));
    }
    else {
        uint16_t usUpdateSkip;
        NiStreamLoadBinary(kStream, usUpdateSkip);
        SetUpdateSkip(usUpdateSkip);
    }
}

void NiPSysBoundUpdateModifier::LinkObject(NiStream& kStream) {
    NiPSysModifier::LinkObject(kStream);
}

bool NiPSysBoundUpdateModifier::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifier::RegisterStreamables(kStream);
}

void NiPSysBoundUpdateModifier::SaveBinary(NiStream& kStream) {
    NiPSysModifier::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_sUpdateSkip);
}

bool NiPSysBoundUpdateModifier::IsEqual(NiObject* pkObject) {
    if (!NiPSysModifier::IsEqual(pkObject)) {
        return false;
    }

    NiPSysBoundUpdateModifier* pkOther = static_cast<NiPSysBoundUpdateModifier*>(pkObject);
    return pkOther->m_sUpdateSkip == m_sUpdateSkip;
}

void NiPSysBoundUpdateModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifier::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSysBoundUpdateModifier::ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("Update Skip", m_sUpdateSkip));
}

// Each update grows the bound over every iSkip-th particle, starting at m_usUpdateCount, into that pass's slot of
// m_pkSkipBounds; the system's bound is the merge of the filled slots. Mesh and strip data have their own passes.
void NiPSysBoundUpdateModifier::Update(float fTime, NiPSysData* pkData) {
    uint16_t usNumParticles = pkData->m_usActive;
    if (usNumParticles == 0) {
        pkData->m_kBound.m_kCenter.x = NiPoint3_ZERO.x;
        pkData->m_kBound.m_kCenter.y = NiPoint3_ZERO.y;
        pkData->m_kBound.m_kCenter.z = NiPoint3_ZERO.z;
        pkData->m_kBound.m_fRadius = 0.0f;
        m_usUpdateCount = 0;
        return;
    }

    if (pkData->IsKindOf(&NiMeshPSysData::ms_RTTI)) {
        UpdateMeshBounds(fTime, static_cast<NiMeshPSysData*>(pkData));
        return;
    }
    if (pkData->IsKindOf(&BSStripPSysData::ms_RTTI)) {
        UpdateStripBounds(fTime, static_cast<BSStripPSysData*>(pkData));
        return;
    }

    if (m_sUpdateSkip == -1) {
        SetUpdateSkip(static_cast<int16_t>(pkData->m_usVertices / 50 + 1));
    }

    int32_t iNumParticles = usNumParticles;
    int32_t iMaxSkip = iNumParticles / 50 + 1;
    int32_t iSkip = m_sUpdateSkip < iMaxSkip ? m_sUpdateSkip : iMaxSkip;
    iSkip = iSkip > 1 ? iSkip : 1;

    NiPoint3* pkVertex = pkData->m_pkVertex;
    float* pfRadii = pkData->m_pfRadii;
    float* pfSizes = pkData->m_pfSizes;
    float fMinX = pkVertex[0].x;
    float fMinY = pkVertex[0].y;
    float fMinZ = pkVertex[0].z;
    float fMaxX = fMinX;
    float fMaxY = fMinY;
    float fMaxZ = fMinZ;
    for (int32_t i = m_usUpdateCount; i < iNumParticles; i += iSkip) {
        float fRadius = pfSizes[i] * pfRadii[i];

        float fMin = pkVertex[i].x - fRadius;
        fMinX = fMin > fMinX ? fMinX : fMin;
        float fMax = pkVertex[i].x + fRadius;
        fMaxX = fMax < fMaxX ? fMaxX : fMax;

        fMin = pkVertex[i].y - fRadius;
        fMinY = fMin > fMinY ? fMinY : fMin;
        fMax = pkVertex[i].y + fRadius;
        fMaxY = fMax < fMaxY ? fMaxY : fMax;

        fMin = pkVertex[i].z - fRadius;
        fMinZ = fMin > fMinZ ? fMinZ : fMin;
        fMax = pkVertex[i].z + fRadius;
        fMaxZ = fMax < fMaxZ ? fMaxZ : fMax;
    }

    NiBound* pkPassBound = &m_pkSkipBounds[m_usUpdateCount];
    float fSumX = fMaxX + fMinX;
    float fSumY = fMaxY + fMinY;
    float fSumZ = fMaxZ + fMinZ;
    pkPassBound->m_kCenter.x = static_cast<float>(fSumX * 0.5);
    pkPassBound->m_kCenter.y = static_cast<float>(fSumY * 0.5);
    pkPassBound->m_kCenter.z = static_cast<float>(fSumZ * 0.5);
    float fDiffX = fMaxX - fMinX;
    float fDiffY = fMaxY - fMinY;
    float fDiffZ = fMaxZ - fMinZ;
    float fSqrLength = fDiffX * fDiffX + fDiffY * fDiffY + fDiffZ * fDiffZ;
    float fLength = std::sqrt(fSqrLength);
    float fPassRadius = static_cast<float>(fLength * 0.5);
    m_pkSkipBounds[m_usUpdateCount].m_fRadius = fPassRadius;

    for (int32_t i = iSkip; i < m_sUpdateSkip; i++) {
        m_pkSkipBounds[i].m_kCenter.x = NiPoint3_ZERO.x;
        m_pkSkipBounds[i].m_kCenter.y = NiPoint3_ZERO.y;
        m_pkSkipBounds[i].m_kCenter.z = NiPoint3_ZERO.z;
        m_pkSkipBounds[i].m_fRadius = 0.0f;
    }

    NiBound kBound = m_pkSkipBounds[m_usUpdateCount];
    for (int32_t i = 1; i < iSkip; i++) {
        if (m_pkSkipBounds[i].m_fRadius != 0.0f) {
            kBound.Merge(&m_pkSkipBounds[i]);
        }
    }

    pkData->m_kBound = kBound;
    if (kBound.m_fRadius == 0.0f) {
        pkData->m_kBound.m_fRadius = pkData->m_pfRadii[0] * pkData->m_pfSizes[0];
    }

    m_usUpdateCount++;
    if (m_usUpdateCount >= iSkip) {
        m_usUpdateCount = 0;
    }
}

// The engine's flag that the system's bound is recomputed by a modifier moves with this modifier.
void NiPSysBoundUpdateModifier::SetSystemPointer(NiParticleSystem* pkTarget) {
    if (m_pkTarget) {
        m_pkTarget->m_bDynamicBounds = false;
    }
    if (pkTarget) {
        pkTarget->m_bDynamicBounds = true;
    }
    NiPSysModifier::SetSystemPointer(pkTarget);
}

// The old array is freed even when null. The new one has a slot per pass, at least one, each an empty bound. The size
// (at most 32767 bounds) cannot saturate.
void NiPSysBoundUpdateModifier::SetUpdateSkip(int32_t aiUpdateSkip) {
    int16_t sUpdateSkip = static_cast<int16_t>(aiUpdateSkip);
    NiBound* pkOldBounds = m_pkSkipBounds;
    m_sUpdateSkip = sUpdateSkip;
    MemoryManager_Singleton.Deallocate(pkOldBounds);

    int32_t iNumBounds = static_cast<int16_t>(sUpdateSkip > 1 ? sUpdateSkip : 1);
    m_pkSkipBounds = static_cast<NiBound*>(MemoryManager_Singleton.Allocate(iNumBounds * sizeof(NiBound)));
    for (int32_t i = 0; i < iNumBounds; i++) {
        m_pkSkipBounds[i].m_kCenter.x = NiPoint3_ZERO.x;
        m_pkSkipBounds[i].m_kCenter.y = NiPoint3_ZERO.y;
        m_pkSkipBounds[i].m_kCenter.z = NiPoint3_ZERO.z;
        m_pkSkipBounds[i].m_fRadius = 0.0f;
    }
}

// Mesh particles: the pass merges the meshes' world bounds and the result goes to the mesh node's world bound, not
// to the data's bound.
void NiPSysBoundUpdateModifier::UpdateMeshBounds(float fTime, NiMeshPSysData* pkMeshData) {
    uint16_t usNumParticles = pkMeshData->m_usActive;
    NiNode* pkMeshes = pkMeshData->m_spParticleMeshes.m_pObject;
    if (m_sUpdateSkip == -1) {
        SetUpdateSkip(static_cast<int16_t>(pkMeshData->m_usVertices / 20 + 1));
    }

    int32_t iNumParticles = usNumParticles;
    int32_t iMaxSkip = iNumParticles / 20 + 1;
    int32_t iSkip = m_sUpdateSkip < iMaxSkip ? m_sUpdateSkip : iMaxSkip;
    iSkip = iSkip > 1 ? iSkip : 1;

    NiAVObject* pkChild = pkMeshes->m_kChildren.m_pBase[0].m_pObject;
    NiBound kBound = *(pkChild->m_kWorldBound ? pkChild->m_kWorldBound : &NiAVObject::NullBoundS);
    for (int32_t i = m_usUpdateCount; i < iNumParticles; i += iSkip) {
        pkChild = pkMeshes->m_kChildren.m_pBase[i].m_pObject;
        kBound.Merge(pkChild->m_kWorldBound ? pkChild->m_kWorldBound : &NiAVObject::NullBoundS);
    }

    m_pkSkipBounds[m_usUpdateCount].m_kCenter = kBound.m_kCenter;
    m_pkSkipBounds[m_usUpdateCount].m_fRadius = kBound.m_fRadius;

    for (int32_t i = iSkip; i < m_sUpdateSkip; i++) {
        m_pkSkipBounds[i].m_kCenter.x = NiPoint3_ZERO.x;
        m_pkSkipBounds[i].m_kCenter.y = NiPoint3_ZERO.y;
        m_pkSkipBounds[i].m_kCenter.z = NiPoint3_ZERO.z;
        m_pkSkipBounds[i].m_fRadius = 0.0f;
    }

    for (int32_t i = 1; i < iSkip; i++) {
        if (m_pkSkipBounds[i].m_fRadius != 0.0f) {
            kBound.Merge(&m_pkSkipBounds[i]);
        }
    }

    if (kBound.m_fRadius == 0.0f) {
        kBound.m_fRadius = pkMeshData->m_pfSizes[0] * pkMeshData->m_pfRadii[0];
    }
    *pkMeshes->m_kWorldBound = kBound;

    m_usUpdateCount++;
    if (m_usUpdateCount >= iSkip) {
        m_usUpdateCount = 0;
    }
}

// Strips: the pass grows the bound over every point of every iSkip-th strip.
void NiPSysBoundUpdateModifier::UpdateStripBounds(float fTime, BSStripPSysData* apData) {
    BSStripPSysData* pkStripData = apData;
    uint16_t usNumParticles = apData->m_usActive;
    if (m_sUpdateSkip == -1) {
        SetUpdateSkip(static_cast<int16_t>(apData->m_usVertices / 10 + 1));
    }

    int32_t iNumParticles = usNumParticles;
    int32_t iMaxSkip = iNumParticles / 10 + 1;
    int32_t iSkip = m_sUpdateSkip < iMaxSkip ? m_sUpdateSkip : iMaxSkip;
    iSkip = iSkip > 1 ? iSkip : 1;

    NiPoint3* pkVertex = apData->m_pkVertex;
    float fMinX = pkVertex[0].x;
    float fMinY = pkVertex[0].y;
    float fMinZ = pkVertex[0].z;
    float fMaxX = fMinX;
    float fMaxY = fMinY;
    float fMaxZ = fMinZ;
    for (int32_t i = m_usUpdateCount; i < iNumParticles; i += iSkip) {
        uint16_t usStrip = static_cast<uint16_t>(i);
        uint16_t usNumPoints = pkStripData->GetPointCount(usStrip);
        for (uint16_t j = 0; j < usNumPoints; j++) {
            const NiPoint3* pkPoint = pkStripData->GetPointPoint(usStrip, j);
            float fRadii = pkStripData->GetPointRadii(usStrip, j);
            float fRadius = pkStripData->GetPointSize(usStrip, j) * fRadii;

            float fMin = pkPoint->x - fRadius;
            fMinX = fMin > fMinX ? fMinX : fMin;
            float fMax = pkPoint->x + fRadius;
            fMaxX = fMax < fMaxX ? fMaxX : fMax;

            fMin = pkPoint->y - fRadius;
            fMinY = fMin > fMinY ? fMinY : fMin;
            fMax = pkPoint->y + fRadius;
            fMaxY = fMax < fMaxY ? fMaxY : fMax;

            fMin = pkPoint->z - fRadius;
            fMinZ = fMin > fMinZ ? fMinZ : fMin;
            fMax = pkPoint->z + fRadius;
            fMaxZ = fMax < fMaxZ ? fMaxZ : fMax;
        }
    }

    NiBound* pkPassBound = &m_pkSkipBounds[m_usUpdateCount];
    float fSumX = fMaxX + fMinX;
    float fSumY = fMaxY + fMinY;
    float fSumZ = fMaxZ + fMinZ;
    pkPassBound->m_kCenter.x = static_cast<float>(fSumX * 0.5);
    pkPassBound->m_kCenter.y = static_cast<float>(fSumY * 0.5);
    pkPassBound->m_kCenter.z = static_cast<float>(fSumZ * 0.5);
    float fDiffX = fMaxX - fMinX;
    float fDiffY = fMaxY - fMinY;
    float fDiffZ = fMaxZ - fMinZ;
    float fSqrLength = fDiffX * fDiffX + fDiffY * fDiffY + fDiffZ * fDiffZ;
    float fLength = std::sqrt(fSqrLength);
    float fPassRadius = static_cast<float>(fLength * 0.5);
    m_pkSkipBounds[m_usUpdateCount].m_fRadius = fPassRadius;

    for (int32_t i = iSkip; i < m_sUpdateSkip; i++) {
        m_pkSkipBounds[i].m_kCenter.x = NiPoint3_ZERO.x;
        m_pkSkipBounds[i].m_kCenter.y = NiPoint3_ZERO.y;
        m_pkSkipBounds[i].m_kCenter.z = NiPoint3_ZERO.z;
        m_pkSkipBounds[i].m_fRadius = 0.0f;
    }

    alignas(8) NiBound kBound = m_pkSkipBounds[m_usUpdateCount];
    for (int32_t i = 1; i < iSkip; i++) {
        if (m_pkSkipBounds[i].m_fRadius != 0.0f) {
            kBound.Merge(&m_pkSkipBounds[i]);
        }
    }

    apData->m_kBound = kBound;
    if (kBound.m_fRadius == 0.0f) {
        apData->m_kBound.m_fRadius = apData->m_pfSizes[0] * apData->m_pfRadii[0];
    }

    m_usUpdateCount++;
    if (m_usUpdateCount >= iSkip) {
        m_usUpdateCount = 0;
    }
}

NiObject* NiPSysBoundUpdateModifier::CreateObject() {
    return new NiPSysBoundUpdateModifier;
}
