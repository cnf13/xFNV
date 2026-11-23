#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysMeshEmitter.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkWorld.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/Sky/Precipitation.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "SDK/MSVC/stdlib.h"

#include <bit>
#include <cmath>
#include <cstdint>

NiRTTI NiPSysMeshEmitter::ms_RTTI("NiPSysMeshEmitter", &NiPSysEmitter::ms_RTTI);

float FastSqrt(float afValue) {
    double v1 = afValue;
    float v4 = 0.5 * afValue;
    float afValuea = (1.5 -
                      v4 * std::bit_cast<float>((0x5F3759DF - (std::bit_cast<int32_t>(afValue) >> 1))) *
                          std::bit_cast<float>((0x5F3759DF - (std::bit_cast<int32_t>(afValue) >> 1)))) *
        std::bit_cast<float>((0x5F3759DF - (std::bit_cast<int32_t>(afValue) >> 1)));
    return v1 * afValuea;
}

// The NiPointer<NiGeometry> arrays' new[]: the count cookie, then each element through NiPointer's default constructor.
// The size saturates on overflow, so the allocation fails rather than comes back short. The block is raw MemoryManager
// memory: the cookie (one size_t), then the elements.
NiPointer<NiGeometry>* NewGeometryPointerArray(uint32_t numObjs) {
    size_t stSize = numObjs > SIZE_MAX / sizeof(NiPointer<NiGeometry>) ? SIZE_MAX
                                                                       : sizeof(NiPointer<NiGeometry>) * numObjs;
    stSize = stSize + sizeof(size_t) < stSize ? SIZE_MAX : stSize + sizeof(size_t);
    size_t* puiCookie = static_cast<size_t*>(MemoryManager::Instance().Allocate(stSize));
    if (!puiCookie) {
        return nullptr;
    }
    NiPointer<NiGeometry>* pArray = reinterpret_cast<NiPointer<NiGeometry>*>(puiCookie + 1);
    *puiCookie = numObjs;
    for (int32_t iItem = 0; iItem < static_cast<int32_t>(numObjs); ++iItem) {
        ::new (static_cast<void*>(pArray + iItem)) NiPointer<NiGeometry>;
    }
    return pArray;
}

// The new[] shared by the other NiPointer arrays: the same code as NewGeometryPointerArray above, with each element
// default-constructed.
NiPointer<NiObject>* NewObjectPointerArray(uint32_t numObjs) {
    size_t stSize = numObjs > SIZE_MAX / sizeof(NiPointer<NiObject>) ? SIZE_MAX : sizeof(NiPointer<NiObject>) * numObjs;
    stSize = stSize + sizeof(size_t) < stSize ? SIZE_MAX : stSize + sizeof(size_t);
    size_t* puiCookie = static_cast<size_t*>(MemoryManager::Instance().Allocate(stSize));
    if (!puiCookie) {
        return nullptr;
    }
    NiPointer<NiObject>* pArray = reinterpret_cast<NiPointer<NiObject>*>(puiCookie + 1);
    *puiCookie = numObjs;
    for (int32_t iItem = 0; iItem < static_cast<int32_t>(numObjs); ++iItem) {
        ::new (static_cast<void*>(pArray + iItem)) NiPointer<NiObject>;
    }
    return pArray;
}

NiPSysMeshEmitter::NiSkinnedEmitterData::NiSkinnedEmitterData() : m_spSkinPartition(nullptr) {}

// The skin partition is released by NiPointer's destructor.
NiPSysMeshEmitter::NiSkinnedEmitterData::~NiSkinnedEmitterData() {}

bool NiPSysMeshEmitter::NiSkinnedEmitterData::CreatePartition(NiGeometry* pkGeom) {
    if (!pkGeom || !pkGeom->IsTriBasedGeom()) {
        return false;
    }
    NiSkinInstance* m_pObject = pkGeom->m_spSkinInstance.m_pObject;
    if (!m_pObject) {
        return false;
    }
    NiSkinPartition* pkPartition = m_pObject->m_spSkinPartition.m_pObject;
    if (!pkPartition) {
        NiSkinData* v4 = m_pObject->m_spSkinData.m_pObject;
        if (!v4) {
            return false;
        }
        // Passes the whole m_uiBones value on, at least 4.
        uint32_t uiMaxNumMatrices = v4->m_uiBones;
        if (uiMaxNumMatrices < 4) {
            uiMaxNumMatrices = 4;
        }
        pkPartition = new NiSkinPartition;
        if (!pkPartition->MakePartitions(
                static_cast<NiTriBasedGeomData*>(pkGeom->m_spModelData.m_pObject),
                m_pObject->m_spSkinData.m_pObject,
                uiMaxNumMatrices,
                4,
                0,
                nullptr,
                nullptr,
                nullptr
            )) {
            return false;
        }
    }
    m_spSkinPartition = pkPartition;
    return true;
}

NiPSysMeshEmitter::NiPSysMeshEmitter()
  : m_kGeomEmitterArray(1, 2), m_kSkinnedEmitterData(0, 1), m_eInitVelocityType(NI_VELOCITY_USE_NORMALS),
    m_eEmissionType(NI_EMIT_FROM_VERTICES), m_kEmitAxis(NiPoint3::UNIT_X) {}

// The two arrays' buffers are freed by their destructors.
NiPSysMeshEmitter::~NiPSysMeshEmitter() {}

NiObject* NiPSysMeshEmitter::CreateClone(NiCloningProcess& kCloning) {
    NiPSysMeshEmitter* pkObject = new NiPSysMeshEmitter;
    CopyMembers(pkObject, kCloning);
    pkObject->m_kGeomEmitterArray.SetSize(m_kGeomEmitterArray.m_usSize);
    pkObject->m_eInitVelocityType = m_eInitVelocityType;
    pkObject->m_eEmissionType = m_eEmissionType;
    pkObject->m_kEmitAxis = m_kEmitAxis;
    return pkObject;
}

// The mesh link IDs are queued here and resolved in LinkObject.
void NiPSysMeshEmitter::LoadBinary(NiStream& kStream) {
    NiPSysEmitter::LoadBinary(kStream);

    kStream.ReadMultipleLinkIDs();
    uint32_t uiInitVelocityType;
    NiStreamLoadBinary(kStream, uiInitVelocityType);
    m_eInitVelocityType = static_cast<InitialVelocityType>(uiInitVelocityType);
    uint32_t uiEmissionType;
    NiStreamLoadBinary(kStream, uiEmissionType);
    m_eEmissionType = static_cast<MeshEmissionType>(uiEmissionType);
    m_kEmitAxis.LoadBinary(kStream);
}

// KEEP: both enums are read into the stack slot the prologue filled with `this`, so a short read (malformed data)
// leaves this's bits in m_eInitVelocityType and m_eEmissionType, which src cannot express; porting.md's
// unwritten-slot exception covers GetChunkData only.
// Each linked object is added as a mesh without a type check.
void NiPSysMeshEmitter::LinkObject(NiStream& kStream) {
    NiPSysEmitter::LinkObject(kStream);

    uint32_t uiNumMeshes = kStream.GetNumberOfLinkIDs();
    if (uiNumMeshes) {
        m_kGeomEmitterArray.SetSize(uiNumMeshes);
        for (uint32_t i = 0; i < uiNumMeshes; i++) {
            AddGeometryEmitter(static_cast<NiGeometry*>(kStream.GetObjectFromLinkID()));
        }
    }
}

bool NiPSysMeshEmitter::RegisterStreamables(NiStream& kStream) {
    return NiPSysEmitter::RegisterStreamables(kStream);
}

void NiPSysMeshEmitter::SaveBinary(NiStream& kStream) {
    NiPSysEmitter::SaveBinary(kStream);

    uint32_t uiNumMeshes = m_kGeomEmitterArray.GetSize();
    NiStreamSaveBinary(kStream, uiNumMeshes);
    for (uint32_t i = 0; i < uiNumMeshes; i++) {
        kStream.SaveLinkID(m_kGeomEmitterArray.m_pBase[i].m_pObject);
    }
    NiStreamSaveBinary(kStream, m_eInitVelocityType);
    NiStreamSaveBinary(kStream, m_eEmissionType);
    m_kEmitAxis.SaveBinary(kStream);
}

// The meshes are compared slot by slot; a slot must be empty in both or hold equal meshes.
bool NiPSysMeshEmitter::IsEqual(NiObject* pkObject) {
    if (!NiPSysEmitter::IsEqual(pkObject)) {
        return false;
    }

    NiPSysMeshEmitter* pkOther = static_cast<NiPSysMeshEmitter*>(pkObject);
    uint32_t uiNumMeshes = m_kGeomEmitterArray.GetSize();
    if (uiNumMeshes != pkOther->m_kGeomEmitterArray.GetSize()) {
        return false;
    }
    for (uint32_t i = 0; i < uiNumMeshes; i++) {
        NiGeometry* pkGeometry = m_kGeomEmitterArray.m_pBase[i].m_pObject;
        NiGeometry* pkOtherGeometry = pkOther->m_kGeomEmitterArray.m_pBase[i].m_pObject;
        if (pkGeometry) {
            if (!pkOtherGeometry || !pkGeometry->IsEqual(pkOtherGeometry)) {
                return false;
            }
        }
        else if (pkOtherGeometry) {
            return false;
        }
    }

    if (m_eInitVelocityType != pkOther->m_eInitVelocityType) {
        return false;
    }
    if (m_eEmissionType != pkOther->m_eEmissionType) {
        return false;
    }
    return !(m_kEmitAxis != pkOther->m_kEmitAxis);
}

void NiPSysMeshEmitter::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysEmitter::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    for (uint32_t i = 0; i < m_kGeomEmitterArray.GetEffectiveSize(); i++) {
        NiGeometry* pkGeometry = m_kGeomEmitterArray.m_pBase[i].m_pObject;
        if (pkGeometry) {
            pkStrings->Add(NiGetViewerString("Emitter Object", static_cast<const char*>(pkGeometry->m_kName)));
        }
    }

    const char* pcEmissionType;
    switch (m_eEmissionType) {
        case NI_EMIT_FROM_VERTICES: pcEmissionType = "NI_EMIT_FROM_VERTICES"; break;
        case NI_EMIT_FROM_FACE_CENTER: pcEmissionType = "NI_EMIT_FROM_FACE_CENTER"; break;
        case NI_EMIT_FROM_EDGE_CENTER: pcEmissionType = "NI_EMIT_FROM_EDGE_CENTER"; break;
        case NI_EMIT_FROM_FACE_SURFACE: pcEmissionType = "NI_EMIT_FROM_FACE_SURFACE"; break;
        case NI_EMIT_FROM_EDGE_SURFACE: pcEmissionType = "NI_EMIT_FROM_EDGE_SURFACE"; break;
        default: pcEmissionType = "UNKNOWN!!!"; break;
    }
    pkStrings->Add(NiGetViewerString("m_eEmissionType", pcEmissionType));

    const char* pcInitVelocityType;
    switch (m_eInitVelocityType) {
        case NI_VELOCITY_USE_NORMALS: pcInitVelocityType = "NI_VELOCITY_USE_NORMALS"; break;
        case NI_VELOCITY_USE_RANDOM: pcInitVelocityType = "NI_VELOCITY_USE_RANDOM"; break;
        case NI_VELOCITY_USE_DIRECTION: pcInitVelocityType = "NI_VELOCITY_USE_DIRECTION"; break;
        default: pcInitVelocityType = "UNKNOWN!!!"; break;
    }
    pkStrings->Add(NiGetViewerString("m_eInitVelocityType", pcInitVelocityType));

    pkStrings->Add(NiGetViewerString("m_kEmissionAxis.x", m_kEmitAxis.x));
    pkStrings->Add(NiGetViewerString("m_kEmissionAxis.y", m_kEmitAxis.y));
    pkStrings->Add(NiGetViewerString("m_kEmissionAxis.z", m_kEmitAxis.z));
}

// The clone gets the cloned meshes, or the original ones when they were not cloned.
void NiPSysMeshEmitter::ProcessClone(NiCloningProcess& kCloning) {
    NiPSysEmitter::ProcessClone(kCloning);

    NiObject* pkObject = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkObject);
    NiPSysMeshEmitter* pkClone = static_cast<NiPSysMeshEmitter*>(pkObject);
    for (uint32_t i = 0; i < m_kGeomEmitterArray.GetEffectiveSize(); i++) {
        NiGeometry* pkGeometry = m_kGeomEmitterArray.m_pBase[i].m_pObject;
        if (pkGeometry) {
            if (kCloning.m_pkCloneMap->GetAt(pkGeometry, pkObject)) {
                pkClone->AddGeometryEmitter(static_cast<NiGeometry*>(pkObject));
            }
            else {
                pkClone->AddGeometryEmitter(pkGeometry);
            }
        }
    }
}

// Picks a random mesh, builds its skin partition on first use if it is skinned, and emits from it as the emission
// type says.
void NiPSysMeshEmitter::ComputeInitialPositionAndVelocity(NiPoint3& kPosition, NiPoint3& kVelocity) {
    uint32_t uiNumMeshes = m_kGeomEmitterArray.GetEffectiveSize();
    if (!uiNumMeshes) {
        return;
    }

    // The product is truncated at the x87's precision, which a double holds exactly here.
    float fRandom = NiUnitRandom();
    uint32_t uiMesh = static_cast<uint32_t>(static_cast<double>(fRandom) * uiNumMeshes);
    if (uiMesh == uiNumMeshes) {
        uiMesh = uiNumMeshes - 1;
    }
    if (uiMesh >= m_kGeomEmitterArray.GetEffectiveSize()) {
        return;
    }
    NiGeometry* pkGeometry = m_kGeomEmitterArray.m_pBase[uiMesh].m_pObject;
    if (!pkGeometry) {
        return;
    }

    NiSkinInstance* pkSkin = pkGeometry->m_spSkinInstance.m_pObject;
    NiSkinnedEmitterData* pkSkinData = uiMesh < m_kSkinnedEmitterData.GetEffectiveSize()
        ? m_kSkinnedEmitterData.m_pBase[uiMesh].m_pObject
        : nullptr;
    if (pkSkin && pkSkinData && !pkSkinData->m_spSkinPartition.m_pObject) {
        pkSkinData->CreatePartition(pkGeometry);
    }

    switch (m_eEmissionType) {
        case NI_EMIT_FROM_FACE_CENTER:
        case NI_EMIT_FROM_FACE_SURFACE:
            if (pkSkin) {
                EmitFromSkinnedFace(pkSkinData, pkGeometry, kPosition, kVelocity);
            }
            else {
                EmitFromFace(pkGeometry, kPosition, kVelocity);
            }
            break;
        case NI_EMIT_FROM_EDGE_CENTER:
        case NI_EMIT_FROM_EDGE_SURFACE:
            if (pkSkin) {
                EmitFromSkinnedEdge(pkSkinData, pkGeometry, kPosition, kVelocity);
            }
            else {
                EmitFromEdge(pkGeometry, kPosition, kVelocity);
            }
            break;
        default:
            if (pkSkin) {
                EmitFromSkinnedVertex(pkSkinData, pkGeometry, kPosition, kVelocity);
            }
            else {
                EmitFromVertex(pkGeometry, kPosition, kVelocity);
            }
            break;
    }
}

// The partition is shared only when the mesh has a skin with one. A failed allocation is deliberately not checked
// before the partition is stored.
void NiPSysMeshEmitter::AddGeometryEmitter(NiGeometry* pkGeomEmitter) {
    if (!pkGeomEmitter) {
        return;
    }

    m_kGeomEmitterArray.AddFirstEmpty(pkGeomEmitter);

    NiSkinInstance* pkSkin = pkGeomEmitter->m_spSkinInstance.m_pObject;
    NiSkinPartition* pkPartition = pkSkin ? pkSkin->m_spSkinPartition.m_pObject : nullptr;
    NiSkinnedEmitterData* pkSkinData = new NiSkinnedEmitterData;
    if (pkPartition) {
        pkSkinData->m_spSkinPartition = pkPartition;
    }
    m_kSkinnedEmitterData.AddFirstEmpty(pkSkinData);

    if (pkGeomEmitter->m_spModelData.m_pObject) {
        // Keep the positions, normals, indices and bone data, which emission reads.
        pkGeomEmitter->m_spModelData->m_ucKeepFlags |= NiGeometryData::KEEP_XYZ | NiGeometryData::KEEP_NORM |
            NiGeometryData::KEEP_INDICES | NiGeometryData::KEEP_BONEDATA;
    }
}

bool NiPSysMeshEmitter::EmitFromVertex(NiGeometry* pkGeomEmitter, NiPoint3& kPosition, NiPoint3& kVelocity) {
    int32_t iRandom = rt::rand();
    NiGeometryData* m_pObject = pkGeomEmitter->m_spModelData.m_pObject;
    int32_t iVertex = iRandom % m_pObject->m_usVertices;
    NiPoint3* m_pkVertex = m_pObject->m_pkVertex;
    NiPoint3* m_pkNormal = m_pObject->m_pkNormal;
    if (!m_pkVertex) {
        return false;
    }
    kPosition = m_pkVertex[iVertex];
    if ((m_eInitVelocityType == NiPSysMeshEmitter::NI_VELOCITY_USE_NORMALS) &&
        pkGeomEmitter->m_spModelData->m_pkNormal) {
        NiPoint3 kNormalVelocity;
        float fSqrSpeed = kVelocity.y * kVelocity.y + kVelocity.x * kVelocity.x + kVelocity.z * kVelocity.z;
        float fSpeed = std::sqrt(static_cast<double>(fSqrSpeed));
        kNormalVelocity.x = m_pkNormal[iVertex].x * fSpeed;
        kNormalVelocity.y = m_pkNormal[iVertex].y * fSpeed;
        kNormalVelocity.z = fSpeed * m_pkNormal[iVertex].z;
        kVelocity = kNormalVelocity;
    }

    TransformIntoCoordinateSpace(pkGeomEmitter, kPosition, kVelocity);
    return true;
}

// Emits from the centre of a random triangle of a triangle mesh (after up to six tries for a triangle with three
// distinct indices); for NI_EMIT_FROM_FACE_SURFACE from a random point on it. An NiLines emits from an edge instead.
// ComputeInitialPositionAndVelocity calls it for the face emission types.
bool NiPSysMeshEmitter::EmitFromFace(NiGeometry* pkGeomEmitter, NiPoint3& kPosition, NiPoint3& kVelocity) {
    if (!pkGeomEmitter) {
        return false;
    }
    if (!pkGeomEmitter->IsTriBasedGeom()) {
        if (!pkGeomEmitter->IsKindOfMember(&NiLines::ms_RTTI)) {
            return false;
        }
        return EmitFromEdge(pkGeomEmitter, kPosition, kVelocity);
    }
    uint32_t uiTriangles = static_cast<NiTriBasedGeomData*>(pkGeomEmitter->m_spModelData.m_pObject)
                               ->GetActiveTriangleCount();
    uint32_t uiTri = static_cast<uint32_t>(rt::rand()) % uiTriangles;
    if (uiTri == uiTriangles) {
        uiTri = uiTriangles - 1;
    }
    uint16_t usIndex0 = 0;
    uint16_t usIndex1 = 0;
    uint16_t usIndex2 = 0;
    for (int32_t iTry = 0; iTry < 6; ++iTry) {
        static_cast<NiTriBasedGeomData*>(pkGeomEmitter->m_spModelData.m_pObject)
            ->GetTriangleIndices(static_cast<uint16_t>(uiTri), usIndex0, usIndex1, usIndex2);
        if (usIndex0 != usIndex1 && usIndex0 != usIndex2 && usIndex1 != usIndex2) {
            break;
        }
        if (++uiTri >= uiTriangles) {
            uiTri = 0;
        }
    }
    NiGeometryData* pkData = pkGeomEmitter->m_spModelData.m_pObject;
    NiPoint3* pkVertex = pkData->m_pkVertex;
    NiPoint3* pkNormal = pkData->m_pkNormal;
    if (!pkVertex) {
        return false;
    }
    NiPoint3 kVert0 = pkVertex[usIndex0];
    NiPoint3 kVert1 = pkVertex[usIndex1];
    NiPoint3 kVert2 = pkVertex[usIndex2];
    // The sum is computed component by component, each component rounded to float.
    NiPoint3 kSum;
    kSum.x = kVert1.x + kVert0.x;
    kSum.y = kVert0.y + kVert1.y;
    kSum.z = kVert0.z + kVert1.z;
    NiPoint3 kSum2;
    kSum2.x = kSum.x + kVert2.x;
    kSum2.y = kSum.y + kVert2.y;
    kSum2.z = kSum.z + kVert2.z;
    // Multiplies by the double 0.3333333432674408 (float 1/3) and rounds each product to float.
    kPosition.x = static_cast<float>(kSum2.x * 0.3333333432674408);
    kPosition.y = static_cast<float>(kSum2.y * 0.3333333432674408);
    kPosition.z = static_cast<float>(kSum2.z * 0.3333333432674408);

    if (m_eInitVelocityType == NI_VELOCITY_USE_NORMALS && pkNormal) {
        NiPoint3 kNormal0 = pkNormal[usIndex0];
        NiPoint3 kNormal1 = pkNormal[usIndex1];
        NiPoint3 kNormal2 = pkNormal[usIndex2];
        kSum.x = kNormal1.x + kNormal0.x;
        kSum.y = kNormal0.y + kNormal1.y;
        kSum.z = kNormal0.z + kNormal1.z;
        kSum2.x = kSum.x + kNormal2.x;
        kSum2.y = kSum.y + kNormal2.y;
        kSum2.z = kSum.z + kNormal2.z;
        NiPoint3 kNormal;
        kNormal.x = static_cast<float>(kSum2.x * 0.3333333432674408);
        kNormal.y = static_cast<float>(kSum2.y * 0.3333333432674408);
        kNormal.z = static_cast<float>(0.3333333432674408 * kSum2.z);
        NiPoint3::UnitizeVector(kNormal);
        float fSpeed = kVelocity.Length();
        kVelocity.x = kNormal.x * fSpeed;
        kVelocity.y = kNormal.y * fSpeed;
        kVelocity.z = fSpeed * kNormal.z;
    }

    if (m_eEmissionType == NI_EMIT_FROM_FACE_SURFACE) {
        // Computes NiPoint3::operator-, operator* and operator+ by hand, each component rounded to float: e1 + ((e2 *
        // fT2 - e1) * fT1 + v0) with e1 = v1 - v0, e2 = v2 - v0 and fT1 shaped by FastSqrt.
        NiPoint3 kEdge1;
        kEdge1.x = kVert1.x - kVert0.x;
        kEdge1.y = kVert1.y - kVert0.y;
        kEdge1.z = kVert1.z - kVert0.z;
        NiPoint3 kEdge2;
        kEdge2.x = kVert2.x - kVert0.x;
        kEdge2.y = kVert2.y - kVert0.y;
        kEdge2.z = kVert2.z - kVert0.z;
        float fRand1 = NiUnitRandom();
        float fT1 = FastSqrt(fRand1);
        float fT2 = NiUnitRandom();
        NiPoint3 kTemp;
        kTemp.x = kEdge2.x * fT2;
        kTemp.y = kEdge2.y * fT2;
        kTemp.z = fT2 * kEdge2.z;
        NiPoint3 kDiff;
        kDiff.x = kTemp.x - kEdge1.x;
        kDiff.y = kTemp.y - kEdge1.y;
        kDiff.z = kTemp.z - kEdge1.z;
        kTemp.x = kDiff.x * fT1;
        kTemp.y = kDiff.y * fT1;
        kTemp.z = fT1 * kDiff.z;
        NiPoint3 kOffset;
        kOffset.x = kTemp.x + kVert0.x;
        kOffset.y = kTemp.y + kVert0.y;
        kOffset.z = kTemp.z + kVert0.z;
        kPosition.x = kEdge1.x + kOffset.x;
        kPosition.y = kEdge1.y + kOffset.y;
        kPosition.z = kEdge1.z + kOffset.z;
    }
    TransformIntoCoordinateSpace(pkGeomEmitter, kPosition, kVelocity);
    return true;
}

// Emits from the middle of an edge: a random triangle's edge (0-1, 1-2 or 0-2 by rand() % 3, after up to six tries for
// a triangle with three distinct indices) of a triangle mesh, or a random segment of an NiLines; for
// NI_EMIT_FROM_EDGE_SURFACE from a random point on it. ComputeInitialPositionAndVelocity calls it for the edge emission
// types.
bool NiPSysMeshEmitter::EmitFromEdge(NiGeometry* pkGeomEmitter, NiPoint3& kPosition, NiPoint3& kVelocity) {
    if (!pkGeomEmitter) {
        return false;
    }
    uint16_t usIndex0 = 0;
    uint16_t usIndex1 = 0;
    if (pkGeomEmitter->IsTriBasedGeom()) {
        uint32_t uiTriangles = static_cast<NiTriBasedGeomData*>(pkGeomEmitter->m_spModelData.m_pObject)
                                   ->GetActiveTriangleCount();
        uint32_t uiTri = static_cast<uint32_t>(rt::rand()) % uiTriangles;
        uint16_t usIndex2 = 0;
        for (int32_t iTry = 0; iTry < 6; ++iTry) {
            static_cast<NiTriBasedGeomData*>(pkGeomEmitter->m_spModelData.m_pObject)
                ->GetTriangleIndices(static_cast<uint16_t>(uiTri), usIndex0, usIndex1, usIndex2);
            if (usIndex0 != usIndex1 && usIndex0 != usIndex2 && usIndex1 != usIndex2) {
                break;
            }
            if (++uiTri >= uiTriangles) {
                uiTri = 0;
            }
        }
        switch (rt::rand() % 3) {
            case 1:
                usIndex0 = usIndex1;
                usIndex1 = usIndex2;
                break;
            case 2: usIndex1 = usIndex2; break;
            default: break;
        }
    }
    else {
        if (!pkGeomEmitter->IsKindOf(&NiLines::ms_RTTI)) {
            return false;
        }
        float fRand = NiUnitRandom();
        int32_t iLast = pkGeomEmitter->m_spModelData->m_usVertices - 1;
        // The product is computed exactly in double and truncates toward zero.
        int32_t iIndex = static_cast<int32_t>(static_cast<int64_t>(static_cast<double>(iLast) * fRand));
        usIndex0 = static_cast<uint16_t>(iIndex);
        usIndex1 = static_cast<uint16_t>(iIndex + 1);
    }
    NiGeometryData* pkData = pkGeomEmitter->m_spModelData.m_pObject;
    NiPoint3* pkVertex = pkData->m_pkVertex;
    NiPoint3* pkNormal = pkData->m_pkNormal;
    if (!pkVertex) {
        return false;
    }
    NiPoint3 kVert0 = pkVertex[usIndex0];
    NiPoint3 kVert1 = pkVertex[usIndex1];
    NiPoint3 kSum = kVert0 + kVert1;
    // Multiplies by the double 0.5 and rounds each product to float.
    kPosition.x = static_cast<float>(kSum.x * 0.5);
    kPosition.y = static_cast<float>(kSum.y * 0.5);
    kPosition.z = static_cast<float>(kSum.z * 0.5);

    if (m_eInitVelocityType == NI_VELOCITY_USE_NORMALS && pkNormal) {
        NiPoint3 kNormal0 = pkNormal[usIndex0];
        NiPoint3 kNormal1 = pkNormal[usIndex1];
        kSum = kNormal0 + kNormal1;
        NiPoint3 kNormal;
        kNormal.x = static_cast<float>(kSum.x * 0.5);
        kNormal.y = static_cast<float>(kSum.y * 0.5);
        kNormal.z = static_cast<float>(0.5 * kSum.z);
        NiPoint3::UnitizeVector(kNormal);
        float fSpeed = kVelocity.Length();
        kVelocity.x = kNormal.x * fSpeed;
        kVelocity.y = kNormal.y * fSpeed;
        kVelocity.z = fSpeed * kNormal.z;
    }

    if (m_eEmissionType == NI_EMIT_FROM_EDGE_SURFACE) {
        float fT = NiUnitRandom();
        // NiPoint3::operator-, operator* and operator+ written out per component, each component rounded to float.
        NiPoint3 kEdge;
        kEdge.x = kVert1.x - kVert0.x;
        kEdge.y = kVert1.y - kVert0.y;
        kEdge.z = kVert1.z - kVert0.z;
        NiPoint3 kOffset;
        kOffset.x = kEdge.x * fT;
        kOffset.y = kEdge.y * fT;
        kOffset.z = fT * kEdge.z;
        kPosition.x = kVert0.x + kOffset.x;
        kPosition.y = kVert0.y + kOffset.y;
        kPosition.z = kVert0.z + kOffset.z;
    }
    TransformIntoCoordinateSpace(pkGeomEmitter, kPosition, kVelocity);
    return true;
}

// Picks a random partition, then a random vertex of it (the products are computed exactly in double and truncate toward
// zero).
bool NiPSysMeshEmitter::EmitFromSkinnedVertex(
    NiSkinnedEmitterData* pkSkinEmitterData,
    NiGeometry* pkGeomEmitter,
    NiPoint3& kPosition,
    NiPoint3& kVelocity
) {
    if (!pkGeomEmitter || !pkGeomEmitter->IsTriBasedGeom()) {
        return false;
    }
    NiSkinPartition* pkSkinPartition = pkSkinEmitterData->m_spSkinPartition.m_pObject;
    if (!pkSkinPartition) {
        return false;
    }
    int32_t iPartitions = static_cast<uint16_t>(pkSkinPartition->m_uiPartitions);
    float fRand = NiUnitRandom();
    int32_t iPartition = static_cast<int32_t>(static_cast<double>(fRand) * iPartitions);
    if (iPartition >= iPartitions - 1) {
        iPartition = iPartitions - 1;
    }
    NiSkinPartition::Partition* pkPart = &pkSkinEmitterData->m_spSkinPartition->m_pkPartitions[iPartition];
    float fVertRand = NiUnitRandom();
    NiSkinInstance* pkSkin = pkGeomEmitter->m_spSkinInstance.m_pObject;
    if (!pkSkin) {
        return false;
    }
    NiPoint3 kVert = NiPoint3::ZERO;
    NiPoint3 kNormal = NiPoint3::ZERO;
    int32_t iLast = pkPart->m_usVertices - 1;
    uint32_t uiIndex = static_cast<uint32_t>(static_cast<int32_t>(iLast * static_cast<double>(fVertRand)));
    // DeformBySkin takes the geometry as a NiTriBasedGeom.
    DeformBySkin(kVert, kNormal, static_cast<NiTriBasedGeom*>(pkGeomEmitter), pkSkin, pkPart, uiIndex);
    kPosition = kVert;
    if (m_eInitVelocityType == NI_VELOCITY_USE_NORMALS && pkGeomEmitter->m_spModelData->m_pkNormal) {
        float fSpeed = kVelocity.Length();
        kVelocity = fSpeed * kNormal;
    }
    TransformIntoCoordinateSpace(pkGeomEmitter, kPosition, kVelocity);
    return true;
}

// Emits from the centre of a random triangle of a random partition, or from a random point on it for
// NI_EMIT_FROM_FACE_SURFACE.
bool NiPSysMeshEmitter::EmitFromSkinnedFace(
    NiSkinnedEmitterData* pkSkinEmitterData,
    NiGeometry* pkGeomEmitter,
    NiPoint3& kPosition,
    NiPoint3& kVelocity
) {
    if (!pkGeomEmitter || !pkGeomEmitter->IsTriBasedGeom()) {
        return false;
    }
    NiSkinPartition* pkSkinPartition = pkSkinEmitterData->m_spSkinPartition.m_pObject;
    if (!pkSkinPartition) {
        return false;
    }
    int32_t iPartitions = static_cast<uint16_t>(pkSkinPartition->m_uiPartitions);
    float fRand = NiUnitRandom();
    int32_t iPartition = static_cast<int32_t>(static_cast<double>(fRand) * iPartitions);
    if (iPartition >= iPartitions - 1) {
        iPartition = iPartitions - 1;
    }
    NiSkinPartition::Partition* pkPart = &pkSkinEmitterData->m_spSkinPartition->m_pkPartitions[iPartition];
    if (pkPart->m_usTriangles == 0) {
        return EmitFromSkinnedVertex(pkSkinEmitterData, pkGeomEmitter, kPosition, kVelocity);
    }
    int32_t iTriangles = pkPart->m_usTriangles;
    int32_t iTri = static_cast<int32_t>(static_cast<double>(NiUnitRandom()) * iTriangles);
    if (iTri >= iTriangles - 1) {
        iTri = iTriangles - 1;
    }
    NiSkinInstance* pkSkin = pkGeomEmitter->m_spSkinInstance.m_pObject;
    if (!pkSkin) {
        return false;
    }
    uint32_t uiIndex0;
    uint32_t uiIndex1;
    uint32_t uiIndex2;
    if (pkPart->m_usStrips) {
        const uint16_t* pusTriList = pkPart->m_pusTriList;
        uiIndex0 = pusTriList[iTri];
        uiIndex1 = pusTriList[iTri + 1];
        uiIndex2 = pusTriList[iTri + 2];
    }
    else {
        const uint16_t* pusTri = &pkPart->m_pusTriList[3 * iTri];
        uiIndex0 = pusTri[0];
        uiIndex1 = pusTri[1];
        uiIndex2 = pusTri[2];
    }
    NiPoint3 kVert0 = NiPoint3::ZERO;
    NiPoint3 kNormal0 = NiPoint3::ZERO;
    NiPoint3 kVert1 = NiPoint3::ZERO;
    NiPoint3 kNormal1 = NiPoint3::ZERO;
    NiPoint3 kVert2 = NiPoint3::ZERO;
    NiPoint3 kNormal2 = NiPoint3::ZERO;
    NiTriBasedGeom* pkTriGeom = static_cast<NiTriBasedGeom*>(pkGeomEmitter);
    DeformBySkin(kVert0, kNormal0, pkTriGeom, pkSkin, pkPart, uiIndex0);
    DeformBySkin(kVert1, kNormal1, pkTriGeom, pkSkin, pkPart, uiIndex1);
    DeformBySkin(kVert2, kNormal2, pkTriGeom, pkSkin, pkPart, uiIndex2);

    NiPoint3 kSum = kVert0 + kVert1;
    NiPoint3 kTotal = kSum + kVert2;
    kPosition = kTotal / 3.0f;

    if (m_eInitVelocityType == NI_VELOCITY_USE_NORMALS) {
        kSum = kNormal0 + kNormal1;
        kTotal = kSum + kNormal2;
        NiPoint3 kNormal = kTotal / 3.0f;
        NiPoint3::UnitizeVector(kNormal);
        float fSpeed = kVelocity.Length();
        kVelocity = fSpeed * kNormal;
    }

    if (m_eEmissionType == NI_EMIT_FROM_FACE_SURFACE) {
        NiPoint3 kEdge1;
        kEdge1.x = kVert1.x - kVert0.x;
        kEdge1.y = kVert1.y - kVert0.y;
        kEdge1.z = kVert1.z - kVert0.z;
        NiPoint3 kEdge2;
        kEdge2.x = kVert2.x - kVert0.x;
        kEdge2.y = kVert2.y - kVert0.y;
        kEdge2.z = kVert2.z - kVert0.z;
        float fScale = FastSqrt(NiUnitRandom());
        NiPoint3 kScaled = NiUnitRandom() * kEdge2;
        NiPoint3 kDiff;
        kDiff.x = kScaled.x - kEdge1.x;
        kDiff.y = kScaled.y - kEdge1.y;
        kDiff.z = kScaled.z - kEdge1.z;
        NiPoint3 kOffset = fScale * kDiff;
        NiPoint3 kPoint = kOffset + kVert0;
        kPosition = kPoint + kEdge1;
    }
    TransformIntoCoordinateSpace(pkGeomEmitter, kPosition, kVelocity);
    return true;
}

// Emits from the middle of a random edge of a random triangle, or from a random point on it for
// NI_EMIT_FROM_EDGE_SURFACE.
bool NiPSysMeshEmitter::EmitFromSkinnedEdge(
    NiSkinnedEmitterData* pkSkinEmitterData,
    NiGeometry* pkGeomEmitter,
    NiPoint3& kPosition,
    NiPoint3& kVelocity
) {
    if (!pkGeomEmitter || !pkGeomEmitter->IsTriBasedGeom()) {
        return false;
    }
    NiSkinPartition* pkSkinPartition = pkSkinEmitterData->m_spSkinPartition.m_pObject;
    if (!pkSkinPartition) {
        return false;
    }
    int32_t iPartitions = static_cast<uint16_t>(pkSkinPartition->m_uiPartitions);
    float fRand = NiUnitRandom();
    int32_t iPartition = static_cast<int32_t>(static_cast<double>(fRand) * iPartitions);
    if (iPartition >= iPartitions - 1) {
        iPartition = iPartitions - 1;
    }
    NiSkinPartition::Partition* pkPart = &pkSkinEmitterData->m_spSkinPartition->m_pkPartitions[iPartition];
    if (pkPart->m_usTriangles == 0) {
        return EmitFromSkinnedVertex(pkSkinEmitterData, pkGeomEmitter, kPosition, kVelocity);
    }
    int32_t iTriangles = pkPart->m_usTriangles;
    int32_t iTri = static_cast<int32_t>(static_cast<double>(NiUnitRandom()) * iTriangles);
    if (iTri >= iTriangles - 1) {
        iTri = iTriangles - 1;
    }
    NiSkinInstance* pkSkin = pkGeomEmitter->m_spSkinInstance.m_pObject;
    if (!pkSkin) {
        return false;
    }
    const uint16_t* pusTri = pkPart->m_usStrips ? &pkPart->m_pusTriList[iTri] : &pkPart->m_pusTriList[3 * iTri];
    uint32_t uiIndex0 = pusTri[0];
    uint32_t uiIndex1 = pusTri[1];
    uint32_t uiIndex2 = pusTri[2];
    // The edge: 0-1, 1-2 or 0-2 by rand() % 3.
    switch (rt::rand() % 3) {
        case 1:
            uiIndex0 = uiIndex1;
            uiIndex1 = uiIndex2;
            break;
        case 2: uiIndex1 = uiIndex2; break;
        default: break;
    }
    NiPoint3 kVert0 = NiPoint3::ZERO;
    NiPoint3 kNormal0 = NiPoint3::ZERO;
    NiPoint3 kVert1 = NiPoint3::ZERO;
    NiPoint3 kNormal1 = NiPoint3::ZERO;
    NiTriBasedGeom* pkTriGeom = static_cast<NiTriBasedGeom*>(pkGeomEmitter);
    DeformBySkin(kVert0, kNormal0, pkTriGeom, pkSkin, pkPart, uiIndex0);
    DeformBySkin(kVert1, kNormal1, pkTriGeom, pkSkin, pkPart, uiIndex1);

    NiPoint3 kSum = kVert0 + kVert1;
    kPosition = kSum / 2.0f;

    if (m_eInitVelocityType == NI_VELOCITY_USE_NORMALS) {
        kSum = kNormal0 + kNormal1;
        NiPoint3 kNormal = kSum / 2.0f;
        NiPoint3::UnitizeVector(kNormal);
        float fSpeed = kVelocity.Length();
        kVelocity = fSpeed * kNormal;
    }

    if (m_eEmissionType == NI_EMIT_FROM_EDGE_SURFACE) {
        NiPoint3 kEdge;
        kEdge.x = kVert1.x - kVert0.x;
        kEdge.y = kVert1.y - kVert0.y;
        kEdge.z = kVert1.z - kVert0.z;
        float fT = NiUnitRandom();
        NiPoint3 kOffset;
        kOffset.x = kEdge.x * fT;
        kOffset.y = kEdge.y * fT;
        kOffset.z = fT * kEdge.z;
        kPosition = kOffset + kVert0;
    }
    TransformIntoCoordinateSpace(pkGeomEmitter, kPosition, kVelocity);
    return true;
}

// The weights are read four per vertex (16 * uiWhichIndex bytes in), whatever m_usBonesPerVertex.
void NiPSysMeshEmitter::DeformBySkin(
    NiPoint3& kVert,
    NiPoint3& kNormal,
    NiTriBasedGeom* pkGeom,
    NiSkinInstance* pkSkin,
    NiSkinPartition::Partition* pkPart,
    uint32_t uiWhichIndex
) {
    float v16;
    float v17;
    NiPoint3 kDstNormal;
    float v37;
    float v38;
    float v39;
    float v40;
    NiTransform kWorldToRootParent;
    NiTransform kWorldToRootParent_2;
    NiTransform kWorldToSkin;
    NiTransform v59;
    NiSkinData* m_pObject = pkSkin->m_spSkinData.m_pObject;
    pkSkin->m_pkRootParent->m_kWorld.Invert(kWorldToRootParent);
    kWorldToSkin = *::new (&kWorldToRootParent_2) NiTransform(m_pObject->m_kRootParentToSkin * kWorldToRootParent);
    NiSkinData::BoneData* m_pkBoneData = m_pObject->m_pkBoneData;
    uint32_t v9 = uiWhichIndex;
    float x = NiPoint3::ZERO.x;
    kDstNormal.x = NiPoint3::ZERO.x;
    float z = NiPoint3::ZERO.z;
    kDstNormal.z = NiPoint3::ZERO.z;
    NiGeometryData* v10 = pkGeom->m_spModelData.m_pObject;
    NiPoint3* m_pkNormal = v10->m_pkNormal;
    float y = NiPoint3::ZERO.y;
    kDstNormal.y = NiPoint3::ZERO.y;
    NiPoint3* m_pkVertex = v10->m_pkVertex;
    int32_t v13 = pkPart->m_pusVertexMap[uiWhichIndex];
    float v50 = m_pkVertex[v13].x;
    float v14 = m_pkVertex[v13].y;
    float v15 = m_pkVertex[v13].z;
    if (m_pkNormal) {
        v40 = m_pkNormal[v13].x;
        v16 = m_pkNormal[v13].y;
        v17 = m_pkNormal[v13].z;
    }
    else {
        v37 = 1.0f;
        v38 = 0.0f;
        v40 = 1.0f;
        v39 = 0.0f;
        v16 = 0.0f;
        v17 = 0.0f;
    }
    uint32_t m_usBonesPerVertex = pkPart->m_usBonesPerVertex;
    uint32_t v19 = 0;
    if (m_usBonesPerVertex) {
        while (true) {
            uint8_t* m_pucBonePalette = pkPart->m_pucBonePalette;
            uint16_t v21 = m_pucBonePalette ? pkPart->m_pusBones[m_pucBonePalette[v9 * m_usBonesPerVertex + v19]]
                                            : pkPart->m_pusBones[v19];
            NiTransform* p_m_kSkinToBone = &m_pkBoneData[v21].m_kSkinToBone;
            kWorldToRootParent = *::new (&v59)
                                     NiTransform(kWorldToSkin * pkSkin->m_ppkBones[v21]->m_kWorld * *p_m_kSkinToBone);
            kWorldToRootParent_2.m_Rotate.m_pEntry[0][0] = kWorldToRootParent.m_Rotate.m_pEntry[0][0] *
                kWorldToRootParent.m_fScale;
            float* m_pfWeights = pkPart->m_pfWeights;
            kWorldToRootParent_2.m_Rotate.m_pEntry[0][1] = kWorldToRootParent.m_Rotate.m_pEntry[0][1] *
                kWorldToRootParent.m_fScale;
            kWorldToRootParent_2.m_Rotate.m_pEntry[0][2] = kWorldToRootParent.m_Rotate.m_pEntry[0][2] *
                kWorldToRootParent.m_fScale;
            kWorldToRootParent_2.m_Rotate.m_pEntry[1][0] = kWorldToRootParent.m_Rotate.m_pEntry[1][0] *
                kWorldToRootParent.m_fScale;
            kWorldToRootParent_2.m_Rotate.m_pEntry[1][1] = kWorldToRootParent.m_Rotate.m_pEntry[1][1] *
                kWorldToRootParent.m_fScale;
            kWorldToRootParent_2.m_Rotate.m_pEntry[1][2] = kWorldToRootParent.m_Rotate.m_pEntry[1][2] *
                kWorldToRootParent.m_fScale;
            kWorldToRootParent_2.m_Rotate.m_pEntry[2][0] = kWorldToRootParent.m_Rotate.m_pEntry[2][0] *
                kWorldToRootParent.m_fScale;
            kWorldToRootParent_2.m_Rotate.m_pEntry[2][1] = kWorldToRootParent.m_Rotate.m_pEntry[2][1] *
                kWorldToRootParent.m_fScale;
            kWorldToRootParent_2.m_Rotate.m_pEntry[2][2] = kWorldToRootParent.m_fScale *
                kWorldToRootParent.m_Rotate.m_pEntry[2][2];
            float v32 = m_pfWeights[4 * uiWhichIndex + v19];
            v37 = kWorldToRootParent_2.m_Rotate.m_pEntry[0][2] * v15 +
                (kWorldToRootParent_2.m_Rotate.m_pEntry[0][0] * v50 +
                 kWorldToRootParent_2.m_Rotate.m_pEntry[0][1] * v14) +
                kWorldToRootParent.m_Translate.x;
            v38 = kWorldToRootParent_2.m_Rotate.m_pEntry[1][1] * v14 +
                kWorldToRootParent_2.m_Rotate.m_pEntry[1][0] * v50 +
                kWorldToRootParent_2.m_Rotate.m_pEntry[1][2] * v15 + kWorldToRootParent.m_Translate.y;
            v39 = v50 * kWorldToRootParent_2.m_Rotate.m_pEntry[2][0] +
                v14 * kWorldToRootParent_2.m_Rotate.m_pEntry[2][1] +
                v15 * kWorldToRootParent_2.m_Rotate.m_pEntry[2][2] + kWorldToRootParent.m_Translate.z;
            float v46 = kWorldToRootParent.m_Rotate.m_pEntry[0][2] * v17 +
                (kWorldToRootParent.m_Rotate.m_pEntry[0][0] * v40 + kWorldToRootParent.m_Rotate.m_pEntry[0][1] * v16);
            float v47 = kWorldToRootParent.m_Rotate.m_pEntry[1][1] * v16 +
                kWorldToRootParent.m_Rotate.m_pEntry[1][0] * v40 + kWorldToRootParent.m_Rotate.m_pEntry[1][2] * v17;
            float v48 = v17 * kWorldToRootParent.m_Rotate.m_pEntry[2][2] +
                (v40 * kWorldToRootParent.m_Rotate.m_pEntry[2][0] + v16 * kWorldToRootParent.m_Rotate.m_pEntry[2][1]);
            float v43 = v37 * v32;
            float v44 = v38 * v32;
            float v45 = v39 * v32;
            x = v43 + x;
            y = y + v44;
            z = z + v45;
            float v53 = v46 * v32;
            float v54 = v47 * v32;
            float v55 = v32 * v48;
            kDstNormal.x = v53 + kDstNormal.x;
            kDstNormal.y = kDstNormal.y + v54;
            m_usBonesPerVertex = pkPart->m_usBonesPerVertex;
            ++v19;
            kDstNormal.z = kDstNormal.z + v55;
            if (v19 >= m_usBonesPerVertex) {
                break;
            }
            v9 = uiWhichIndex;
        }
    }
    kDstNormal.Unitize();
    kVert.x = x;
    kVert.y = y;
    float v27 = kDstNormal.x;
    kVert.z = z;
    float v28 = kDstNormal.y;
    kNormal.x = v27;
    float v29 = kDstNormal.z;
    kNormal.y = v28;
    kNormal.z = v29;
}

void NiPSysMeshEmitter::TransformIntoCoordinateSpace(NiAVObject* pkObj, NiPoint3& kPosition, NiPoint3& kVelocity) {
    NiTransform kDest;
    NiTransform arXform;
    if (pkObj) {
        NiPoint3 v15;
        NiPoint3 result;
        NiTransform kInvDest;
        arXform = pkObj->m_kWorld;
        kDest = m_pkTarget->m_kWorld;
        kDest.Invert(kInvDest);
        kDest = kInvDest * arXform;
        kPosition = (result = kDest * kPosition);
        NiPSysMeshEmitter::InitialVelocityType m_eInitVelocityType = this->m_eInitVelocityType;
        if (m_eInitVelocityType) {
            int32_t v6 = m_eInitVelocityType - 1;
            if (v6) {
                if (v6 == 1) {
                    result = kDest.m_Rotate * m_kEmitAxis;
                    float v11 = NiUnitRandom();
                    float v12 = (v11 - 0.5) * m_fSpeedVar + m_fSpeed;
                    result.Unitize();
                    v15.x = result.x * v12;
                    double y = result.y;
                    kVelocity.x = v15.x;
                    v15.y = y * v12;
                    kVelocity.y = v15.y;
                    v15.z = v12 * result.z;
                    kVelocity.z = v15.z;
                }
            }
            else {
                kVelocity.x = NiSymmetricRandom();
                kVelocity.y = NiSymmetricRandom();
                kVelocity.z = NiSymmetricRandom();
                kVelocity.Unitize();
                float v13 = NiUnitRandom();
                float v14 = (v13 - 0.5) * m_fSpeedVar + m_fSpeed;
                kVelocity.x = kVelocity.x * v14;
                kVelocity.y = v14 * kVelocity.y;
                kVelocity.z = v14 * kVelocity.z;
            }
        }
        else {
            kVelocity = (v15 = kDest.m_Rotate * kVelocity);
        }
    }
}

NiObject* NiPSysMeshEmitter::CreateObject() {
    return new NiPSysMeshEmitter;
}
