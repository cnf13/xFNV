#include "Gamebryo2.2/CoreLibs/NiParticle/NiMeshPSysData.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticlesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI NiMeshPSysData::ms_RTTI("NiMeshPSysData", &NiPSysData::ms_RTTI);

NiMeshPSysData::NiMeshPSysData() : m_spParticleMeshes(nullptr), m_kPools(0, 1) {}

// The pools are emptied and freed; the compiler then frees the pool array, releases the mesh node and runs
// NiPSysData's destructor.
NiMeshPSysData::~NiMeshPSysData() {
    while (m_kPools.GetSize()) {
        NiTObjectArray<NiPointer<NiAVObject>>* pkPool = m_kPools.RemoveEnd();
        delete pkPool;
    }
}

NiObject* NiMeshPSysData::CreateClone(NiCloningProcess& kCloning) {
    NiMeshPSysData* v4 = new NiMeshPSysData;
    CopyMembers(v4, kCloning);
    return v4;
}

// KEEP: niread: the stream reads fill locals nothing initialised: a short read (malformed data) leaves the pool count
// unwritten, which the exe loops on (0xC31162), or a generation's pool size, which it tests (0xC31198); the
// fill-pools flag is read into the low byte of the stream argument's own slot (0xC3111D).
void NiMeshPSysData::LoadBinary(NiStream& kStream) {
    NiPSysData::LoadBinary(kStream);
    if (kStream.GetFileVersion() > NiStream::GetVersion(10, 1, 0, 0)) {
        NiStreamLoadBinary(kStream, m_uiDefaultPoolSize);
        uint8_t ucFillPoolsOnLoad;
        NiStreamLoadBinary(kStream, ucFillPoolsOnLoad);
        m_bFillPoolsOnLoad = ucFillPoolsOnLoad != 0;
        uint32_t uiMaxSize;
        NiStreamLoadBinary(kStream, uiMaxSize);
        m_kPools.SetSize(uiMaxSize);
        for (uint32_t uiLoop = 0; uiLoop < uiMaxSize; ++uiLoop) {
            uint32_t uiGenerationPoolSize;
            NiStreamLoadBinary(kStream, uiGenerationPoolSize);
            if (uiGenerationPoolSize) {
                NiTObjectArray<NiPointer<NiAVObject>>* pkPool = static_cast<NiTObjectArray<NiPointer<NiAVObject>>*>(
                    MemoryManager_Singleton.Allocate(sizeof(NiTObjectArray<NiPointer<NiAVObject>>))
                );
                if (pkPool) {
                    ::new (pkPool) NiTObjectArray<NiPointer<NiAVObject>>;
                }
                else {
                    pkPool = nullptr;
                }
                NiAVObjectArray_SetSize(pkPool, uiGenerationPoolSize);
                m_kPools.SetAt(uiLoop, pkPool);
            }
        }
        kStream.ReadLinkID();
    }
    else {
        m_uiDefaultPoolSize = m_usVertices;
        SetNumGenerations(1);
        kStream.ReadLinkID();
    }
}

// The mesh node is linked without a type check.
void NiMeshPSysData::LinkObject(NiStream& kStream) {
    NiPSysData::LinkObject(kStream);
    m_spParticleMeshes = static_cast<NiNode*>(kStream.GetObjectFromLinkID());
}

bool NiMeshPSysData::RegisterStreamables(NiStream& kStream) {
    if (!NiPSysData::RegisterStreamables(kStream)) {
        return false;
    }
    m_spParticleMeshes->RegisterStreamables(kStream);
    return true;
}

// Only each pool's capacity is saved; a missing pool saves 0.
void NiMeshPSysData::SaveBinary(NiStream& kStream) {
    NiPSysData::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uiDefaultPoolSize);
    uint8_t ucFillPoolsOnLoad = m_bFillPoolsOnLoad;
    NiStreamSaveBinary(kStream, ucFillPoolsOnLoad);

    uint32_t uiNumGenerations = m_kPools.GetSize();
    NiStreamSaveBinary(kStream, uiNumGenerations);
    for (uint32_t i = 0; i < m_kPools.GetSize(); i++) {
        NiTObjectArray<NiPointer<NiAVObject>>* pkPool = m_kPools.GetAt(i);
        uint32_t uiPoolSize = pkPool ? pkPool->GetAllocatedSize() : 0;
        NiStreamSaveBinary(kStream, uiPoolSize);
    }

    kStream.SaveLinkID(m_spParticleMeshes);
}

bool NiMeshPSysData::IsEqual(NiObject* pkObject) {
    if (!NiPSysData::IsEqual(pkObject)) {
        return false;
    }
    NiMeshPSysData* pkOther = static_cast<NiMeshPSysData*>(pkObject);
    return m_spParticleMeshes->IsEqual(pkOther->m_spParticleMeshes);
}

void NiMeshPSysData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysData::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

void NiMeshPSysData::ProcessClone(NiCloningProcess& kCloning) {
    NiPSysData::ProcessClone(kCloning);
    m_spParticleMeshes->ProcessClone(kCloning);
}

// Detaches the meshes of the particles that are no longer active; the base is not called.
void NiMeshPSysData::SetActiveVertexCount(uint16_t usActive) {
    for (uint16_t i = usActive; i < m_usActive; i++) {
        m_spParticleMeshes->DetachChildAt(i);
    }
    m_usActive = usActive > m_usVertices ? m_usVertices : usActive;
}

// The last active particle's mesh moves into the removed one's place, and the removed mesh goes back to its
// generation's pool.
void NiMeshPSysData::RemoveParticle(int32_t aiParticle) {
    uint16_t usParticle = static_cast<uint16_t>(aiParticle);
    uint16_t usLast = m_usActive - 1;
    uint32_t uiGeneration = m_pkParticleInfo[usParticle].m_usGeneration;
    NiPSysData::RemoveParticle(aiParticle);

    NiPointer<NiAVObject> spMesh;
    if (usParticle == usLast) {
        m_spParticleMeshes->DetachChildAt(usLast, spMesh);
    }
    else {
        NiPointer<NiAVObject> spLast;
        m_spParticleMeshes->DetachChildAt(usLast, spLast);
        m_spParticleMeshes->DetachChildAt(usParticle, spMesh);
        m_spParticleMeshes->SetAt(usParticle, spLast);
    }
    AddToPool(uiGeneration, spMesh);
}

// The added particles' meshes become visible.
void NiMeshPSysData::ResolveAddedParticles() {
    for (uint16_t i = m_usAddedParticlesBase; i < m_usAddedParticlesBase + m_usNumAddedParticles; i++) {
        NiAVObject* pkMesh = m_spParticleMeshes->m_kChildren.m_pBase[i];
        if (pkMesh) {
            uint32_t& ruFlags = pkMesh->m_uFlags;
            ruFlags = NiAVObject::NiFlags(ruFlags & ~NiAVObject::APP_CULLED);
        }
    }
    NiPSysData::ResolveAddedParticles();
}

void NiMeshPSysData::SetNumGenerations(uint32_t uiNumGenerations) {
    uint32_t uiLoop = m_kPools.m_usSize;
    if (uiNumGenerations >= uiLoop) {
        if (uiNumGenerations > uiLoop) {
            NiTPrimitiveArray<NiTObjectArray<NiPointer<NiAVObject>>*>* p_m_kPools = &m_kPools;
            m_kPools.SetSize(uiNumGenerations);
            for (; uiLoop < uiNumGenerations; ++uiLoop) {
                NiTObjectArray<NiPointer<NiAVObject>>* pkPool = static_cast<NiTObjectArray<NiPointer<NiAVObject>>*>(
                    MemoryManager_Singleton.Allocate(sizeof(NiTObjectArray<NiPointer<NiAVObject>>))
                );
                if (pkPool) {
                    ::new (pkPool) NiTObjectArray<NiPointer<NiAVObject>>;
                }
                else {
                    pkPool = nullptr;
                }
                uint32_t m_uiDefaultPoolSize = this->m_uiDefaultPoolSize;
                NiTObjectArray<NiPointer<NiAVObject>>* v13 = pkPool;
                NiAVObjectArray_SetSize(pkPool, m_uiDefaultPoolSize);
                p_m_kPools->SetAt(uiLoop, v13);
            }
        }
    }
    else {
        do {
            uint16_t m_usSize = m_kPools.m_usSize;
            if (m_usSize) {
                NiTObjectArray<NiPointer<NiAVObject>>** m_pBase = m_kPools.m_pBase;
                uint16_t v7 = m_usSize - 1;
                m_kPools.m_usSize = v7;
                NiTObjectArray<NiPointer<NiAVObject>>** v8 = &m_pBase[v7];
                NiTObjectArray<NiPointer<NiAVObject>>* v9 = *v8;
                *v8 = nullptr;
                if (v9) {
                    --m_kPools.m_usESize;
                    delete v9;
                }
            }
        }
        while (m_kPools.m_usSize > uiNumGenerations);
    }
}

void NiMeshPSysData::AddToPool(uint32_t uiGeneration, NiPointer<NiAVObject> spObject) {
    if (uiGeneration < m_kPools.GetSize()) {
        NiTObjectArray<NiPointer<NiAVObject>>* pkPool = m_kPools.GetAt(uiGeneration);
        if (pkPool && pkPool->GetSize() < pkPool->GetAllocatedSize()) {
            NiAVObjectArray_Add(pkPool, spObject);
        }
    }
}

// The cloned mesh node is stored without a type check.
void NiMeshPSysData::CopyMembers(NiMeshPSysData* pkDest, NiCloningProcess& kCloning) {
    NiPSysData::CopyMembers(pkDest, kCloning);
    pkDest->m_spParticleMeshes = static_cast<NiNode*>(m_spParticleMeshes->CreateClone(kCloning));
    pkDest->m_bFillPoolsOnLoad = m_bFillPoolsOnLoad;
}

NiObject* NiMeshPSysData::CreateObject() {
    return new NiMeshPSysData;
}
