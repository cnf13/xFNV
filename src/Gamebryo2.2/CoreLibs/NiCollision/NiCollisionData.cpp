#include "Gamebryo2.2/CoreLibs/NiCollision/NiCollisionData.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoundingVolume.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiOBBRoot.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProcessorSpecificCode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstring>

namespace {

    // The block Initialize receives: the scene object, the propagation mode and an optional volume to stream in.
    struct InitializeData {
        NiAVObject* pkSceneObject;
        NiCollisionData::PropagationMode ePropagationMode;
        int32_t bHasABV;
        NiStream* pkStream;
    };

} // namespace

NiRTTI NiCollisionData::ms_RTTI("NiCollisionData", &NiCollisionObject::ms_RTTI);

// KEEP: UpdateWorldData (0xEAD0F0) keeps the parent in edx across its call of this function and reads it after;
// clang does not keep edx intact.
NiAVObject* FindCollisionAncestor(NiAVObject* a1) {
    NiAVObject* result = a1;
    if (a1) {
        while (true) {
            result = result->m_pkParent;
            if (!result) {
                break;
            }
            if (result->m_spCollisionObject.m_pObject) {
                return result;
            }
        }
    }
    return nullptr;
}

NiCollisionData::NiCollisionData()
  : NiCollisionObject(nullptr), m_kLocalVelocity(NiPoint3_ZERO), m_kWorldVelocity(NiPoint3_ZERO),
    m_ePropagationMode(PROPAGATE_ALWAYS), m_eCollisionMode(NOTEST), m_pkModelABV(nullptr), m_pkWorldABV(nullptr),
    m_pkOBBRoot(nullptr), m_pfnCollideCallback(nullptr), m_pvCollideCallbackData(nullptr), m_pkWorldVertex(nullptr),
    m_pkWorldNormal(nullptr), m_bWorldVerticesNeedUpdate(false), m_bWorldNormalsNeedUpdate(false), m_usNumTriangles(0),
    m_bAuxCallbacks(false) {}

NiCollisionData::~NiCollisionData() {
    MemoryManager_Singleton.Deallocate(m_pkWorldVertex);
    MemoryManager_Singleton.Deallocate(m_pkWorldNormal);
    delete m_pkModelABV;
    delete m_pkWorldABV;
    delete m_pkOBBRoot;
    m_pkOBBRoot = nullptr;
}

const NiRTTI* NiCollisionData::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiCollisionData::CreateClone(NiCloningProcess& kCloning) {
    NiCollisionData* v3 = static_cast<NiCollisionData*>(NiMemObject::operator new(sizeof(NiCollisionData)));
    if (v3) {
        NiCollisionData* v4 = ::new (v3) NiCollisionData;
        CopyMembers(v4, kCloning);
        return v4;
    }

    CopyMembers(nullptr, kCloning);
    return nullptr;
}

// KEEP (niread:): the exe reads each mode and the volume flag into a stack slot, then stores or tests it; a short
// read leaves the slot unwritten and the exe uses it all the same.
void NiCollisionData::LoadBinary(NiStream& kStream) {
    NiCollisionObject::LoadBinary(kStream);
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 0, 1, 6)) {
        PropagationMode ePropagationMode;
        NiStreamLoadBinary(kStream, ePropagationMode);
        m_ePropagationMode = ePropagationMode;
    }
    else {
        PropagationMode ePropagationMode;
        NiStreamLoadBinary(kStream, ePropagationMode);
        m_ePropagationMode = ePropagationMode;
        CollisionMode eCollisionMode;
        NiStreamLoadBinary(kStream, eCollisionMode);
        m_eCollisionMode = eCollisionMode;
    }
    bool bHasABV;
    NiStreamLoadBinary(kStream, bHasABV);
    if (bHasABV) {
        m_pkModelABV = NiBoundingVolume::CreateFromStream(kStream);
        m_pkWorldABV = m_pkModelABV->Create();
    }
}

// Older files did not save the geometry's keep flags, so the collision mode sets them again.
void NiCollisionData::LinkObject(NiStream& kStream) {
    NiCollisionObject::LinkObject(kStream);
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 0, 1, 16)) {
        SetCollisionMode(m_eCollisionMode);
    }
}

bool NiCollisionData::RegisterStreamables(NiStream& kStream) {
    return NiCollisionObject::RegisterStreamables(kStream);
}

void NiCollisionData::SaveBinary(NiStream& kStream) {
    NiCollisionObject::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_ePropagationMode);
    NiStreamSaveBinary(kStream, m_eCollisionMode);
    bool bHasABV = m_pkModelABV != nullptr;
    NiStreamSaveBinary(kStream, bHasABV);
    if (m_pkModelABV) {
        m_pkModelABV->SaveBinary(kStream);
    }
}

// Same modes; the volumes match when both are missing or both have the same type.
bool NiCollisionData::IsEqual(NiObject* pkObject) {
    if (!NiCollisionObject::IsEqual(pkObject)) {
        return false;
    }
    if (!NiObject::IsKindOf(&ms_RTTI, pkObject)) {
        return false;
    }
    NiCollisionData* pkOther = static_cast<NiCollisionData*>(pkObject);
    if (pkOther->m_eCollisionMode != m_eCollisionMode || pkOther->m_ePropagationMode != m_ePropagationMode) {
        return false;
    }
    if (!pkOther->m_pkModelABV || !m_pkModelABV) {
        return pkOther->m_pkModelABV == m_pkModelABV;
    }
    return pkOther->m_pkModelABV->Type() == m_pkModelABV->Type();
}

void NiCollisionData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiCollisionObject::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    AddViewerStrings(pkStrings);
}

void NiCollisionData::AddViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiCollisionObject::AddViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(GetViewerString("m_ePropagationMode", m_ePropagationMode));
    pkStrings->Add(GetViewerString("m_eCollisionMode", m_eCollisionMode));
    if (m_pkModelABV) {
        m_pkModelABV->AddViewerStrings("m_pkModelABV->Type", pkStrings);
    }
    pkStrings->Add(m_kLocalVelocity.GetViewerString("m_kLocalVelocity"));
    pkStrings->Add(m_kWorldVelocity.GetViewerString("m_kWorldVelocity"));
}

void NiCollisionData::SetSceneGraphObject(NiAVObject* pkSceneObject) {
    NiCollisionObject::SetSceneGraphObject(pkSceneObject);
    SetCollisionMode(m_eCollisionMode);
}

// Moves the world volume onto the scene object; optionally carries velocity down from the nearest ancestor
// with collision data. Either way the cached world vertices and normals go stale.
void NiCollisionData::UpdateWorldData(NiUpdateData&) {
    if (m_pkModelABV) {
        m_pkWorldABV->UpdateWorldData(*m_pkModelABV, m_pkSceneObject->m_kWorld);
    }

    if (ms_bComputeWorldVelocity) {
        NiNode* pkParent = m_pkSceneObject->m_pkParent;
        if (pkParent) {
            NiAVObject* pkAncestor = pkParent->m_spCollisionObject ? pkParent : FindCollisionAncestor(pkParent);
            // The slot holds any NiCollisionObject; here it is taken to be collision data.
            NiCollisionData* pkAncestorData = pkAncestor
                ? reinterpret_cast<NiCollisionData*>(
                      static_cast<bhkNiCollisionObject*>(pkAncestor->m_spCollisionObject)
                  )
                : nullptr;
            if (pkAncestorData) {
                NiPoint3 kVelocity = pkParent->m_kWorld.m_Rotate * m_kLocalVelocity;
                m_kWorldVelocity = pkAncestorData->m_kWorldVelocity + kVelocity;
            }
            else {
                m_kWorldVelocity = pkParent->m_kWorld.m_Rotate * m_kLocalVelocity;
            }
        }
        else {
            m_kWorldVelocity = m_kLocalVelocity;
        }
    }

    m_bWorldNormalsNeedUpdate = true;
    m_bWorldVerticesNeedUpdate = true;
}

void NiCollisionData::Initialize(void* apData) {
    InitializeData* pkData = static_cast<InitializeData*>(apData);
    SetSceneGraphObject(pkData->pkSceneObject);
    m_ePropagationMode = pkData->ePropagationMode;
    if (pkData->bHasABV) {
        SetModelSpaceABV(NiBoundingVolume::CreateFromStream(*pkData->pkStream));
    }
}

// Files before 10.1.0.6 used other propagation numbers and had no collision mode: renumber the mode and pick
// the volume test when a volume was loaded, the OBB test otherwise.
void NiCollisionData::Convert(uint32_t uiVersion, void*) {
    if (uiVersion >= NiStream::GetVersion(10, 0, 1, 6)) {
        return;
    }
    switch (m_ePropagationMode) {
        case PROPAGATE_ON_SUCCESS: m_ePropagationMode = PROPAGATE_NEVER; break;
        case PROPAGATE_ON_FAILURE: m_ePropagationMode = PROPAGATE_NEVER; return;
        case PROPAGATE_ALWAYS: m_ePropagationMode = PROPAGATE_ON_SUCCESS; break;
        case PROPAGATE_NEVER: m_ePropagationMode = PROPAGATE_ALWAYS; break;
        case PROPAGATIONMODE_POS: m_ePropagationMode = PROPAGATE_ALWAYS; return;
        default: return;
    }
    SetCollisionMode(m_pkModelABV ? USE_ABV : USE_OBB);
}

// The base class tracks no velocity; this always answers zero.
NiPoint3 NiCollisionData::GetVelocity() {
    return NiPoint3_ZERO;
}

void NiCollisionData::SetModelSpaceABV(NiBoundingVolume* pkBV) {
    delete m_pkModelABV;
    delete m_pkWorldABV;
    m_pkModelABV = pkBV;
    if (pkBV) {
        m_pkWorldABV = pkBV->Create();
        if (m_pkSceneObject) {
            m_pkWorldABV->UpdateWorldData(*m_pkModelABV, m_pkSceneObject->m_kWorld);
        }
    }
    else {
        m_pkWorldABV = nullptr;
    }
}

void NiCollisionData::SetCollisionMode(CollisionMode eCollision) {
    m_eCollisionMode = eCollision;
    if (!m_pkSceneObject || !m_pkSceneObject->IsGeometry()) {
        return;
    }
    NiGeometryData* pkData = static_cast<NiGeometry*>(m_pkSceneObject)->m_spModelData;
    if (pkData && (eCollision == USE_OBB || eCollision == USE_TRI)) {
        // What the vertex and triangle tests need kept on the CPU.
        pkData->m_ucKeepFlags |= NiGeometryData::KEEP_XYZ | NiGeometryData::KEEP_NORM | NiGeometryData::KEEP_INDICES |
            NiGeometryData::KEEP_BONEDATA;
    }
}

void NiCollisionData::CreateWorldVertices() {
    NiAVObject* m_pkSceneObject = this->m_pkSceneObject;
    if (m_pkSceneObject) {
        NiTriBasedGeom* pkGeom = m_pkSceneObject->IsTriBasedGeom();
        if ((pkGeom) && (!m_pkWorldVertex)) {
            uint16_t m_usVertices = pkGeom->m_spModelData.m_pObject->m_usVertices;
            if (m_usVertices) {
                m_usNumVertices = m_usVertices;
                m_pkWorldVertex = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(
                    m_usVertices <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * m_usVertices : SIZE_MAX
                ));
                m_bWorldVerticesNeedUpdate = true;
                // The source's pkGeom->GetTriangleCount(): a tri-based geometry's model data is NiTriBasedGeomData.
                m_usNumTriangles = static_cast<NiTriBasedGeomData*>(pkGeom->m_spModelData.m_pObject)->m_usTriangles;
            }
        }
    }
}

// The geometry's model vertices through the processor-specific point transform into the world vertex cache.
void NiCollisionData::UpdateWorldVertices() {
    NiAVObject* pkSceneObject = m_pkSceneObject;
    if (pkSceneObject) {
        NiGeometry* pkGeom = pkSceneObject->IsGeometry();
        if ((pkGeom) && (m_pkWorldVertex) && (m_bWorldVerticesNeedUpdate)) {
            NiGeometryData* pkData = pkGeom->m_spModelData.m_pObject;
            NiPoint3* pkVertex = pkData->m_pkVertex;
            uint16_t usVertices = pkData->GetActiveVertexCount();
            NiProcessorSpecificCode::ms_pPointsTransform(
                usVertices, &pkVertex->x, &m_pkWorldVertex->x, &pkGeom->m_kWorld
            );
            m_bWorldVerticesNeedUpdate = false;
        }
    }
}

// Builds the OBB tree of a tri shape from its triangle list, or of tri strips from their non-degenerate triangles.
void NiCollisionData::CreateOBB(int32_t iBinSize) {
    const NiRTTI* pkRTTI;
    NiAVObject* pkSceneObject = m_pkSceneObject;
    if (pkSceneObject && (pkRTTI = pkSceneObject->GetRTTI()) != nullptr) {
        while (pkRTTI != &NiTriShape::ms_RTTI) {
            pkRTTI = pkRTTI->GetBaseRTTI();
            if (!pkRTTI) {
                goto LABEL_5;
            }
        }
        if (!m_pkOBBRoot) {
            if (!m_pkWorldVertex) {
                CreateWorldVertices();
                UpdateWorldVertices();
            }
            NiTriShape* pkShape = static_cast<NiTriShape*>(m_pkSceneObject);
            NiOBBRoot* pkMemory = static_cast<NiOBBRoot*>(MemoryManager_Singleton.Allocate(sizeof(NiOBBRoot)));
            if (pkMemory) {
                NiTriShapeData* pkData = static_cast<NiTriShapeData*>(pkShape->m_spModelData.m_pObject);
                NiPoint3* pkVertex = pkData->m_pkVertex;
                uint16_t* pusTriList = pkData->GetTriList();
                m_pkOBBRoot = ::new (pkMemory) NiOBBRoot(
                    static_cast<NiTriShapeData*>(pkShape->m_spModelData.m_pObject)->m_usTriangles, pusTriList, pkVertex,
                    m_pkWorldVertex, iBinSize
                );
            }
            else {
                m_pkOBBRoot = nullptr;
            }
        }
    }
    else {
    LABEL_5:;
        NiAVObject* pkObject = m_pkSceneObject;
        if (pkObject) {
            const NiRTTI* pkStripsRTTI = pkObject->GetRTTI();
            if (pkStripsRTTI) {
                while (pkStripsRTTI != &NiTriStrips::ms_RTTI) {
                    pkStripsRTTI = pkStripsRTTI->GetBaseRTTI();
                    if (!pkStripsRTTI) {
                        return;
                    }
                }
                if (!m_pkOBBRoot) {
                    if (!m_pkWorldVertex) {
                        CreateWorldVertices();
                        UpdateWorldVertices();
                    }
                    NiTriStrips* pkStrips = static_cast<NiTriStrips*>(m_pkSceneObject);
                    uint32_t uiTriangles =
                        static_cast<NiTriBasedGeomData*>(pkStrips->m_spModelData.m_pObject)->m_usTriangles;
                    uint32_t uiKept = 0;
                    uint16_t* pusTriList = static_cast<uint16_t*>(_NiMalloc(3 * sizeof(uint16_t) * uiTriangles));
                    uint16_t* pusTri = pusTriList;
                    for (uint32_t i = 0; i < uiTriangles; ++i) {
                        NiTriBasedGeomData* pkData =
                            static_cast<NiTriBasedGeomData*>(pkStrips->m_spModelData.m_pObject);
                        pkData->GetTriangleIndices(static_cast<uint16_t>(i), pusTri[0], pusTri[1], pusTri[2]);
                        // A degenerate triangle is overwritten by the next one.
                        if (pusTri[0] != pusTri[1] && pusTri[0] != pusTri[2] && pusTri[1] != pusTri[2]) {
                            pusTri += 3;
                            ++uiKept;
                        }
                    }
                    NiOBBRoot* pkRoot;
                    NiOBBRoot* pkMemory = static_cast<NiOBBRoot*>(MemoryManager_Singleton.Allocate(sizeof(NiOBBRoot)));
                    if (pkMemory) {
                        // Passes the whole count.
                        pkRoot = ::new (pkMemory) NiOBBRoot(
                            static_cast<int32_t>(uiKept), pusTriList, pkStrips->m_spModelData.m_pObject->m_pkVertex,
                            m_pkWorldVertex, iBinSize
                        );
                    }
                    else {
                        pkRoot = nullptr;
                    }
                    m_pkOBBRoot = pkRoot;
                    MemoryManager_Singleton.Deallocate(pusTriList);
                }
            }
        }
    }
}

char* NiCollisionData::GetViewerString(const char* apcPrefix, PropagationMode aeMode) const {
    size_t stLen = strlen(apcPrefix) + 32;
    char* pcString = static_cast<char*>(_NiMalloc(stLen));
    switch (aeMode) {
        case PROPAGATE_ON_SUCCESS: NiSprintf(pcString, stLen, "%s = PROPAGATE_ON_SUCCESS", apcPrefix); break;
        case PROPAGATE_ON_FAILURE: NiSprintf(pcString, stLen, "%s = PROPAGATE_ON_FAILURE", apcPrefix); break;
        case PROPAGATE_ALWAYS: NiSprintf(pcString, stLen, "%s = PROPAGATE_ALWAYS", apcPrefix); break;
        case PROPAGATE_NEVER: NiSprintf(pcString, stLen, "%s = PROPAGATE_NEVER", apcPrefix); break;
        default: NiSprintf(pcString, stLen, "%s = UNKNOWN!!!", apcPrefix); break;
    }
    return pcString;
}

char* NiCollisionData::GetViewerString(const char* apcPrefix, CollisionMode aeMode) const {
    size_t stLen = strlen(apcPrefix) + 17;
    char* pcString = static_cast<char*>(_NiMalloc(stLen));
    switch (aeMode) {
        case USE_OBB: NiSprintf(pcString, stLen, "%s = USE_OBB", apcPrefix); break;
        case USE_TRI: NiSprintf(pcString, stLen, "%s = USE_TRI", apcPrefix); break;
        case USE_ABV: NiSprintf(pcString, stLen, "%s = USE_ABV", apcPrefix); break;
        case NOTEST: NiSprintf(pcString, stLen, "%s = NOTEST", apcPrefix); break;
        case USE_NIBOUND: NiSprintf(pcString, stLen, "%s = USE_NIBOUND", apcPrefix); break;
        default: NiSprintf(pcString, stLen, "%s = UNKNOWN!!!", apcPrefix); break;
    }
    return pcString;
}

void NiCollisionData::CopyMembers(NiCollisionData* pkDest, NiCloningProcess& kCloning) {
    NiObject::CopyMembers(pkDest, kCloning);
    pkDest->m_ePropagationMode = m_ePropagationMode;
    pkDest->m_eCollisionMode = m_eCollisionMode;
    if (m_pkModelABV) {
        pkDest->SetModelSpaceABV(m_pkModelABV->Clone());
    }
    if (m_pkOBBRoot) {
        if (!pkDest->m_pkWorldVertex) {
            pkDest->CreateWorldVertices();
            pkDest->UpdateWorldVertices();
        }
        pkDest->CreateOBB(1);
    }
}

NiObject* NiCollisionData::CreateObject() {
    void* v0 = NiMemObject::operator new(sizeof(NiCollisionData));
    if (v0) {
        return ::new (v0) NiCollisionData;
    }

    return nullptr;
}
