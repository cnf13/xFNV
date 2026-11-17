#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysMeshUpdateModifier.h"

#include "BSMain/BSParentVelocityModifier.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiMeshPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

#include <cmath>

NiRTTI NiPSysMeshUpdateModifier::ms_RTTI("NiPSysMeshUpdateModifier", &NiPSysModifier::ms_RTTI);

// NiTObjectArray<NiPointer<NiAVObject>>::RemoveEnd. Constructs the removed element in the caller's apResult storage and
// returns apResult.
NiPointer<NiAVObject>* NiAVObjectArray_RemoveEnd(NiTObjectArray<NiPointer<NiAVObject>>* apArray, void* apResult) {
    if (apArray->m_usSize == 0) {
        return ::new (apResult) NiPointer<NiAVObject>(nullptr);
    }

    apArray->m_usSize--;
    NiPointer<NiAVObject> spElement = apArray->m_pBase[apArray->m_usSize];
    apArray->m_pBase[apArray->m_usSize] = nullptr;
    if (spElement != nullptr) {
        apArray->m_usESize--;
    }
    return ::new (apResult) NiPointer<NiAVObject>(spElement);
}

// m_kMeshes starts with no slots, growing by 1.
NiPSysMeshUpdateModifier::NiPSysMeshUpdateModifier() {}

NiPSysMeshUpdateModifier::~NiPSysMeshUpdateModifier() {}

NiObject* NiPSysMeshUpdateModifier::CreateClone(NiCloningProcess& kCloning) {
    NiPSysMeshUpdateModifier* pkObject =
        static_cast<NiPSysMeshUpdateModifier*>(NiMemObject::operator new(sizeof(NiPSysMeshUpdateModifier)));
    if (pkObject) {
        ::new (pkObject) NiPSysMeshUpdateModifier;
        CopyMembers(pkObject, kCloning);
        return pkObject;
    }

    CopyMembers(nullptr, kCloning);
    return nullptr;
}

// The stored order is overridden; the mesh links are read here and resolved in LinkObject.
void NiPSysMeshUpdateModifier::LoadBinary(NiStream& kStream) {
    NiPSysModifier::LoadBinary(kStream);
    m_uiOrder = 6500;
    kStream.ReadMultipleLinkIDs();
}

// The meshes are linked without a type check.
void NiPSysMeshUpdateModifier::LinkObject(NiStream& kStream) {
    NiPSysModifier::LinkObject(kStream);

    uint32_t uiNumMeshes = kStream.GetNumberOfLinkIDs();
    NiAVObjectArray_SetSize(&m_kMeshes, uiNumMeshes);
    for (uint32_t i = 0; i < uiNumMeshes; i++) {
        NiPointer<NiAVObject> spMesh = static_cast<NiAVObject*>(kStream.GetObjectFromLinkID());
        NiAVObjectArray_SetAt(&m_kMeshes, i, &spMesh);
    }
}

bool NiPSysMeshUpdateModifier::RegisterStreamables(NiStream& kStream) {
    if (!NiPSysModifier::RegisterStreamables(kStream)) {
        return false;
    }
    for (uint32_t i = 0; i < m_kMeshes.GetSize(); i++) {
        NiAVObject* pkMesh = m_kMeshes.m_pBase[i];
        if (pkMesh) {
            pkMesh->RegisterStreamables(kStream);
        }
    }
    return true;
}

void NiPSysMeshUpdateModifier::SaveBinary(NiStream& kStream) {
    NiPSysModifier::SaveBinary(kStream);

    uint32_t uiNumMeshes = m_kMeshes.GetSize();
    NiStreamSaveBinary(kStream, uiNumMeshes);
    for (uint32_t i = 0; i < uiNumMeshes; i++) {
        kStream.SaveLinkID(m_kMeshes.m_pBase[i]);
    }
}

bool NiPSysMeshUpdateModifier::IsEqual(NiObject* pkObject) {
    if (!NiPSysModifier::IsEqual(pkObject)) {
        return false;
    }

    NiPSysMeshUpdateModifier* pkOther = static_cast<NiPSysMeshUpdateModifier*>(pkObject);
    if (m_kMeshes.GetSize() != pkOther->m_kMeshes.GetSize()) {
        return false;
    }
    for (uint32_t i = 0; i < m_kMeshes.GetSize(); i++) {
        NiAVObject* pkMesh = m_kMeshes.m_pBase[i];
        NiAVObject* pkOtherMesh = nullptr;
        if (i < pkOther->m_kMeshes.GetSize()) {
            pkOtherMesh = pkOther->m_kMeshes.m_pBase[i];
        }
        if (pkMesh) {
            if (!pkOtherMesh || !pkMesh->IsEqual(pkOtherMesh)) {
                return false;
            }
        }
        else if (pkOtherMesh) {
            return false;
        }
    }
    return true;
}

void NiPSysMeshUpdateModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifier::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

void NiPSysMeshUpdateModifier::ProcessClone(NiCloningProcess& kCloning) {
    NiPSysModifier::ProcessClone(kCloning);
    for (uint32_t i = 0; i < m_kMeshes.GetSize(); i++) {
        NiAVObject* pkMesh = m_kMeshes.m_pBase[i];
        if (pkMesh) {
            pkMesh->ProcessClone(kCloning);
        }
    }
}

// Files up to 10.1.0.0 build one pool per mesh; newer ones fill the pools already loaded, when the data asks for it.
void NiPSysMeshUpdateModifier::PostLinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);

    if (kStream.GetFileVersion() <= NiStream::GetVersion(10, 1, 0, 0)) {
        static_cast<NiMeshPSysData*>(m_pkTarget->m_spModelData.m_pObject)->SetNumGenerations(m_kMeshes.GetSize());
        return;
    }

    NiMeshPSysData* pkData = static_cast<NiMeshPSysData*>(m_pkTarget->m_spModelData.m_pObject);
    if (pkData->m_bFillPoolsOnLoad) {
        for (uint32_t i = 0; i < pkData->m_kPools.GetSize(); i++) {
            if (i < m_kMeshes.GetAllocatedSize() && m_kMeshes.m_pBase[i]) {
                pkData->FillPool(i, m_kMeshes.m_pBase[i]);
            }
        }
    }
}

// Each active particle's mesh takes the particle's position, rotation and size, then updates with the particle's age as
// its time.
void NiPSysMeshUpdateModifier::Update(float fTime, NiPSysData* apData) {
    NiMeshPSysData* pkData = static_cast<NiMeshPSysData*>(apData);
    float* pfRotationAngles = pkData->m_pfRotationAngles;
    NiPoint3* pkRotationAxes = pkData->m_pkRotationAxes;
    for (uint16_t i = 0; i < pkData->m_usActive; i++) {
        NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
        NiAVObject* pkMesh = pkData->m_spParticleMeshes->m_kChildren.m_pBase[i];
        pkMesh->m_kLocal.m_Translate = pkData->m_pkVertex[i];

        if (pfRotationAngles && pkRotationAxes) {
            NiMatrix3 kRotate;
            kRotate.MakeRotation(pfRotationAngles[i], pkRotationAxes[i].x, pkRotationAxes[i].y, pkRotationAxes[i].z);
            pkMesh->m_kLocal.m_Rotate = kRotate;
        }

        float fScale = pkData->m_pfSizes[i] * pkData->m_pfRadii[i];
        pkMesh->m_kLocal.m_fScale = std::fabs(fScale);

        NiUpdateData kUpdateData;
        kUpdateData.fTime = pkInfo->m_fAge;
        kUpdateData.bUpdateControllers = true;
        kUpdateData.bParallelUpdate = false;
        kUpdateData.bFoundParticles = false;
        kUpdateData.bFoundMorphController = false;
        kUpdateData.bSceneGraphChange = false;
        pkMesh->UpdateDownwardPass(kUpdateData, 0);
    }
}

// The new particle's mesh comes from its generation's pool, or is cloned from the generation's mesh; generations past
// the last mesh use the last one. It starts hidden until ResolveAddedParticles.
void NiPSysMeshUpdateModifier::Initialize(NiPSysData* apData, uint16_t ausNewParticle) {
    NiMeshPSysData* pkData = static_cast<NiMeshPSysData*>(apData);
    uint16_t usGeneration = pkData->m_pkParticleInfo[ausNewParticle].m_usGeneration;
    if (usGeneration >= m_kMeshes.GetSize()) {
        usGeneration = static_cast<uint16_t>(m_kMeshes.GetSize() - 1);
    }

    NiPointer<NiAVObject> spMesh;
    pkData->RemoveFromPool(usGeneration, spMesh);
    if (!spMesh) {
        spMesh = static_cast<NiAVObject*>(m_kMeshes.m_pBase[usGeneration]->Clone());
    }

    uint32_t& ruFlags = spMesh->m_uFlags;
    ruFlags = NiAVObject::NiFlags(ruFlags | NiAVObject::APP_CULLED);
    NiTimeController::StartAnimations(spMesh, 0.0f);
    pkData->m_spParticleMeshes->SetAt(ausNewParticle, spMesh);
    spMesh->UpdatePropertiesDownward(&m_pkTarget->m_PropertyState);
}

// The former mesh is returned through aspAVObject, which the caller has already constructed (its old object is
// released). The new mesh is held by a temporary pointer that is released right after the array's SetAt.
void NiPSysMeshUpdateModifier::SetMeshAt(uint32_t uiIndex, NiAVObject* pkMesh, NiPointer<NiAVObject>& aspAVObject) {
    if (uiIndex < m_kMeshes.GetSize()) {
        NiPointer<NiAVObject> spFormerMesh = m_kMeshes.GetAt(uiIndex);
        {
            NiPointer<NiAVObject> spMesh = pkMesh;
            NiAVObjectArray_SetAt(&m_kMeshes, uiIndex, &spMesh);
        }
        aspAVObject = spFormerMesh;
    }
    else {
        {
            // SetAtGrow.
            NiPointer<NiAVObject> spMesh = pkMesh;
            if (uiIndex >= m_kMeshes.GetAllocatedSize()) {
                NiAVObjectArray_SetSize(&m_kMeshes, uiIndex + m_kMeshes.GetGrowBy());
            }
            NiAVObjectArray_SetAt(&m_kMeshes, uiIndex, &spMesh);
        }
        aspAVObject = nullptr;
    }
}

// Each mesh's clone goes to the same index of pkDest; the former mesh SetMeshAt hands back is released at once.
void NiPSysMeshUpdateModifier::CopyMembers(NiPSysMeshUpdateModifier* pkDest, NiCloningProcess& kCloning) {
    NiPSysModifier::CopyMembers(pkDest, kCloning);
    for (uint32_t i = 0; i < m_kMeshes.GetSize(); i++) {
        NiAVObject* pkMesh = m_kMeshes.m_pBase[i];
        if (pkMesh) {
            NiAVObject* pkClone = static_cast<NiAVObject*>(pkMesh->CreateClone(kCloning));
            NiPointer<NiAVObject> spFormerMesh(nullptr);
            pkDest->SetMeshAt(i, pkClone, spFormerMesh);
        }
    }
}

NiObject* NiPSysMeshUpdateModifier::CreateObject() {
    NiPSysMeshUpdateModifier* pkObject =
        static_cast<NiPSysMeshUpdateModifier*>(NiMemObject::operator new(sizeof(NiPSysMeshUpdateModifier)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiPSysMeshUpdateModifier;
}

void NiMeshPSysData::FillPool(uint32_t uiGeneration, NiAVObject* pkOriginal) {
    if (uiGeneration >= m_kPools.GetSize()) {
        return;
    }
    NiTObjectArray<NiPointer<NiAVObject>>* pkPool = m_kPools.GetAt(uiGeneration);
    if (!pkPool) {
        return;
    }
    for (uint32_t i = 0; i < pkPool->GetAllocatedSize(); i++) {
        NiPointer<NiAVObject> spClone = static_cast<NiAVObject*>(pkOriginal->Clone());
        NiAVObjectArray_SetAt(pkPool, i, &spClone);
    }
}

// The null paths release the old object and store null; the pool path assigns from the removed element and then
// releases the element.
void NiMeshPSysData::RemoveFromPool(uint32_t uiGeneration, NiPointer<NiAVObject>& aspAVObject) {
    if (uiGeneration >= m_kPools.GetSize() || !m_kPools.GetAt(uiGeneration)) {
        aspAVObject = nullptr;
        return;
    }
    NiTObjectArray<NiPointer<NiAVObject>>* pkPool = m_kPools.GetAt(uiGeneration);
    // The removed element is built in uiGeneration's storage, which is dead by then.
    NiPointer<NiAVObject>* pspRemoved = reinterpret_cast<NiPointer<NiAVObject>*>(&uiGeneration);
    NiPointer<NiAVObject>* pspRemovedResult = NiAVObjectArray_RemoveEnd(pkPool, pspRemoved);
    aspAVObject = *pspRemovedResult;
    NiAVObject* pkRemoved = pspRemoved->m_pObject;
    if (pkRemoved) {
        pkRemoved->DecRefCount();
    }
}
