#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstdlib>
#include <cstring>

NiRTTI NiSkinInstance::ms_RTTI("NiSkinInstance", &NiObject::ms_RTTI);

namespace {

    // Orders bone names for the viewer; a null entry sorts after the other.
    int CompareBoneNames(const void* pvName0, const void* pvName1) {
        if (!pvName0) {
            return 1;
        }
        if (!pvName1) {
            return -1;
        }
        return strcmp(*static_cast<const char* const*>(pvName0), *static_cast<const char* const*>(pvName1));
    }

} // namespace

NiSkinInstance::NiSkinInstance()
  : m_spSkinData(nullptr), m_spSkinPartition(nullptr), m_pkRootParent(nullptr), m_ppkBones(nullptr),
    m_uiFrameID(0xFFFFFFFF), m_uiNumMatrices(0), m_uiNumRegisters(0), m_uiAllocatedSize(0), m_pvBoneMatrices(nullptr),
    m_pvSkinToWorldWorldToSkinMatrix(nullptr) {}

NiSkinInstance::NiSkinInstance(NiSkinData* apSkinData, NiAVObject* apRootParent, NiAVObject** appBones)
  : m_spSkinData(apSkinData), m_spSkinPartition(nullptr), m_pkRootParent(apRootParent), m_ppkBones(appBones),
    m_uiFrameID(0xFFFFFFFF), m_uiNumMatrices(0), m_uiNumRegisters(0), m_uiAllocatedSize(0), m_pvBoneMatrices(nullptr),
    m_pvSkinToWorldWorldToSkinMatrix(nullptr) {}

// The renderer drops whatever it built for this instance; the smart pointers release the partition and skin data.
NiSkinInstance::~NiSkinInstance() {
    _NiFree(m_ppkBones);
    if (NiRenderer::ms_pkRenderer) {
        NiRenderer::ms_pkRenderer->PurgeSkinInstance(this);
    }
}

// Only an instance whose root parent or a bone is being cloned gets a clone; the others are shared. The clone keeps the
// original's root and bones where those are not cloned. Goes on with a null clone when the allocation fails.
NiObject* NiSkinInstance::CreateClone(NiCloningProcess& kCloning) {
    if (!NeedClone(kCloning)) {
        return this;
    }

    // The default constructor, as in CreateObject.
    NiSkinInstance* pkClone = new NiSkinInstance;
    CopyMembers(pkClone, kCloning);

    NiObject* pkClonedObject = nullptr;
    if (kCloning.m_pkCloneMap->GetAt(m_pkRootParent, pkClonedObject)) {
        pkClone->m_pkRootParent = static_cast<NiAVObject*>(pkClonedObject);
    }
    else {
        pkClone->m_pkRootParent = m_pkRootParent;
    }

    uint32_t uiBones = m_spSkinData->m_uiBones;
    pkClone->m_ppkBones = static_cast<NiAVObject**>(_NiMalloc(sizeof(NiAVObject*) * uiBones));
    for (uint32_t i = 0; i < uiBones; i++) {
        if (kCloning.m_pkCloneMap->GetAt(m_ppkBones[i], pkClonedObject)) {
            pkClone->m_ppkBones[i] = static_cast<NiAVObject*>(pkClonedObject);
        }
        else {
            pkClone->m_ppkBones[i] = m_ppkBones[i];
        }
    }

    return pkClone;
}

void NiSkinInstance::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);

    // Skin data, skin partition (10.1.0.101 on; older files keep it in the skin data), root parent, bones.
    kStream.ReadLinkID();
    if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 101)) {
        kStream.ReadLinkID();
    }
    kStream.ReadLinkID();
    kStream.ReadMultipleLinkIDs();
}

void NiSkinInstance::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);

    m_spSkinData = static_cast<NiSkinData*>(kStream.GetObjectFromLinkID());
    if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 101)) {
        m_spSkinPartition = static_cast<NiSkinPartition*>(kStream.GetObjectFromLinkID());
    }
    m_pkRootParent = static_cast<NiAVObject*>(kStream.GetObjectFromLinkID());

    uint32_t uiBones = kStream.GetNumberOfLinkIDs();
    m_ppkBones = static_cast<NiAVObject**>(_NiMalloc(sizeof(NiAVObject*) * uiBones));
    for (uint32_t i = 0; i < uiBones; i++) {
        m_ppkBones[i] = static_cast<NiAVObject*>(kStream.GetObjectFromLinkID());
    }
}

bool NiSkinInstance::RegisterStreamables(NiStream& kStream) {
    if (!NiObject::RegisterStreamables(kStream)) {
        return false;
    }

    m_spSkinData->RegisterStreamables(kStream);
    if (m_spSkinPartition) {
        m_spSkinPartition->RegisterStreamables(kStream);
    }
    m_pkRootParent->RegisterStreamables(kStream);

    uint32_t uiBones = m_spSkinData->m_uiBones;
    for (uint32_t i = 0; i < uiBones; i++) {
        m_ppkBones[i]->RegisterStreamables(kStream);
    }

    return true;
}

void NiSkinInstance::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);

    kStream.SaveLinkID(m_spSkinData);
    kStream.SaveLinkID(m_spSkinPartition);
    kStream.SaveLinkID(m_pkRootParent);

    uint32_t uiBones = m_spSkinData->m_uiBones;
    NiStreamSaveBinary(kStream, uiBones);
    for (uint32_t i = 0; i < uiBones; i++) {
        kStream.SaveLinkID(m_ppkBones[i]);
    }
}

bool NiSkinInstance::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiSkinInstance* pkSkinInstance = static_cast<NiSkinInstance*>(pkObject);

    if (!m_spSkinData->IsEqual(pkSkinInstance->m_spSkinData)) {
        return false;
    }

    if (m_spSkinPartition && !m_spSkinPartition->IsEqual(pkSkinInstance->m_spSkinPartition)) {
        return false;
    }

    if (!m_spSkinPartition && pkSkinInstance->m_spSkinPartition) {
        return false;
    }

    return true;
}

void NiSkinInstance::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiObject::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(ms_RTTI.m_pcName));

    if (m_pkRootParent) {
        pkStrings->Add(NiGetViewerString("Root Parent", static_cast<const char*>(m_pkRootParent->m_kName)));
    }

    if (m_spSkinData) {
        pkStrings->Add(
            NiGetViewerString("Hardware partitions", m_spSkinPartition ? m_spSkinPartition->m_uiPartitions : 0u)
        );

        uint32_t uiBones = m_spSkinData->m_uiBones;
        pkStrings->Add(NiGetViewerString("Bone Count", uiBones));

        // The bones by name (with their address), sorted.
        char** ppcNames = static_cast<char**>(_NiMalloc(sizeof(char*) * uiBones));
        for (uint32_t i = 0; i < uiBones; i++) {
            char acName[1024];
            std::sprintf(acName, "%s (%08X)", static_cast<const char*>(m_ppkBones[i]->m_kName), m_ppkBones[i]);
            char* pcCopy = static_cast<char*>(_NiMalloc(std::strlen(acName) + 1));
            ppcNames[i] = pcCopy;
            std::strcpy(pcCopy, acName);
        }

        std::qsort(ppcNames, uiBones, sizeof(char*), &CompareBoneNames);

        for (uint32_t i = 0; i < uiBones; i++) {
            char acPrefix[10];
            NiSprintf(acPrefix, sizeof(acPrefix), "  %3d", i);
            pkStrings->Add(NiGetViewerString(acPrefix, ppcNames[i] ? ppcNames[i] : "<noname>"));
        }

        for (uint32_t i = 0; i < uiBones; i++) {
            _NiFree(ppcNames[i]);
        }
        _NiFree(ppcNames);

        m_spSkinPartition->GetViewerStrings(pkStrings);
    }
}

void NiSkinInstance::PostLinkObject(NiStream& kStream) {
    // Older files keep the partition in the skin data; the instance takes it over.
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 101) && m_spSkinData) {
        m_spSkinPartition = m_spSkinData->m_spSkinPartition.m_pObject;
        m_spSkinData->m_spSkinPartition = nullptr;
    }
}

void NiSkinInstance::Deform(
    const NiGeometryData* apModelData,
    NiPoint3* apDstVertex,
    NiPoint3* apDstNormal,
    NiPoint3* apDstBinormal,
    NiPoint3* apDstTangent,
    uint32_t auiStride
) {
    uint32_t fatigue = apModelData->NiGeometryData::GetVertexCount();
    NiPoint3* unk24 = apModelData->GetNormals();
    NiPoint3* objectReference = apModelData->NiGeometryData::GetVertices();
    Deform(objectReference, unk24, fatigue, apDstVertex, apDstNormal, apDstBinormal, apDstTangent, auiStride);
}

void NiSkinInstance::CopyMembers(NiSkinInstance* pkDest, NiCloningProcess& kCloning) {
    NiObject::CopyMembers(pkDest, kCloning);

    pkDest->m_spSkinData = m_spSkinData;
    pkDest->m_spSkinPartition = m_spSkinPartition;
}

// Whether the root parent or any bone is in the clone map, that is, is being cloned.
bool NiSkinInstance::NeedClone(NiCloningProcess& kCloning) {
    NiObject* pkClonedObject;
    if (kCloning.m_pkCloneMap->GetAt(m_pkRootParent, pkClonedObject)) {
        return true;
    }

    uint32_t uiBones = m_spSkinData->m_uiBones;
    for (uint32_t i = 0; i < uiBones; i++) {
        if (kCloning.m_pkCloneMap->GetAt(m_ppkBones[i], pkClonedObject)) {
            return true;
        }
    }
    return false;
}

// NiSkinInstance::UpdateModelBound is in Xenon/NiSkinInstance_Xenon.cpp. The default constructor: no skin data,
// partition, root or bones; no bone matrices yet (frame ID -1).
NiObject* NiSkinInstance::CreateObject() {
    return new NiSkinInstance;
}

// Skins auiVertices model vertices (and normals, binormals and tangents when given) into the destination arrays: each
// bone's bone-to-skin transform, weighted per vertex. The binormals and tangents follow the normals in the source
// array. The destination arrays are raw vertex buffers auiStride bytes apart, walked as bytes (hence the char casts).
// The rounding is deliberately the game's: the scaled rotation, every transformed point and, with normals, every
// weighted point are stored as floats; without normals the weighted vertex is added unrounded.
void NiSkinInstance::Deform(
    const NiPoint3* apSrcVertex,
    const NiPoint3* apSrcNormal,
    uint32_t auiVertices,
    NiPoint3* apDstVertex,
    NiPoint3* apDstNormal,
    NiPoint3* apDstBinormal,
    NiPoint3* apDstTangent,
    uint32_t auiStride
) {
    NiTransform kWorldToRootParent;
    m_pkRootParent->m_kWorld.Invert(kWorldToRootParent);
    NiTransform kWorldToSkin = m_spSkinData->m_kRootParentToSkin * kWorldToRootParent;
    NiSkinData* pkSkinData = m_spSkinData.m_pObject;
    NiSkinData::BoneData* pkBoneData = pkSkinData->m_pkBoneData;
    uint32_t uiBones = pkSkinData->m_uiBones;

    if (apSrcNormal && apDstNormal) {
        if (apDstBinormal && apDstTangent) {
            const NiPoint3* pkSrcBinormal = apSrcNormal + auiVertices;
            const NiPoint3* pkSrcTangent = pkSrcBinormal + auiVertices;
            if (auiStride == sizeof(NiPoint3)) {
                std::memset(apDstVertex, 0, sizeof(NiPoint3) * auiVertices);
                std::memset(apDstNormal, 0, sizeof(NiPoint3) * auiVertices);
                std::memset(apDstBinormal, 0, sizeof(NiPoint3) * auiVertices);
                std::memset(apDstTangent, 0, sizeof(NiPoint3) * auiVertices);
            }
            else {
                NiPoint3* pkV = apDstVertex;
                NiPoint3* pkN = apDstNormal;
                NiPoint3* pkB = apDstBinormal;
                NiPoint3* pkT = apDstTangent;
                for (uint32_t i = auiVertices; i; --i) {
                    *pkV = NiPoint3::ZERO;
                    *pkN = NiPoint3::ZERO;
                    *pkB = NiPoint3::ZERO;
                    *pkT = NiPoint3::ZERO;
                    pkV = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(pkV) + auiStride);
                    pkN = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(pkN) + auiStride);
                    pkB = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(pkB) + auiStride);
                    pkT = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(pkT) + auiStride);
                }
            }

            for (uint32_t i = 0; i < uiBones; ++i) {
                NiSkinData::BoneData* pkBone = &pkBoneData[i];
                uint32_t uiVerts = pkBone->m_usVerts;
                NiTransform kBoneToSkin = kWorldToSkin * m_ppkBones[i]->m_kWorld * pkBone->m_kSkinToBone;
                const NiMatrix3& kRot = kBoneToSkin.m_Rotate;
                float fScale = kBoneToSkin.m_fScale;
                float s00 = kRot.m_pEntry[0][0] * fScale;
                float s01 = kRot.m_pEntry[0][1] * fScale;
                float s02 = kRot.m_pEntry[0][2] * fScale;
                float s10 = kRot.m_pEntry[1][0] * fScale;
                float s11 = kRot.m_pEntry[1][1] * fScale;
                float s12 = kRot.m_pEntry[1][2] * fScale;
                float s20 = kRot.m_pEntry[2][0] * fScale;
                float s21 = kRot.m_pEntry[2][1] * fScale;
                float s22 = kRot.m_pEntry[2][2] * fScale;
                NiSkinData::BoneVertData* pkVertData = pkBone->m_pkBoneVertData;
                for (uint32_t j = 0; j < uiVerts; ++j) {
                    uint32_t uiVert = pkVertData[j].m_usVert;
                    float fWeight = pkVertData[j].m_fWeight;
                    uint32_t uiOffset = uiVert * auiStride;
                    const NiPoint3& kSrcV = apSrcVertex[uiVert];
                    const NiPoint3& kSrcN = apSrcNormal[uiVert];
                    const NiPoint3& kSrcB = pkSrcBinormal[uiVert];
                    const NiPoint3& kSrcT = pkSrcTangent[uiVert];
                    NiPoint3 kV;
                    kV.x = s00 * kSrcV.x + s01 * kSrcV.y + s02 * kSrcV.z + kBoneToSkin.m_Translate.x;
                    kV.y = s10 * kSrcV.x + s11 * kSrcV.y + s12 * kSrcV.z + kBoneToSkin.m_Translate.y;
                    kV.z = s20 * kSrcV.x + s21 * kSrcV.y + s22 * kSrcV.z + kBoneToSkin.m_Translate.z;
                    NiPoint3 kN;
                    kN.x = kRot.m_pEntry[0][0] * kSrcN.x + kRot.m_pEntry[0][1] * kSrcN.y +
                        kRot.m_pEntry[0][2] * kSrcN.z;
                    kN.y = kRot.m_pEntry[1][0] * kSrcN.x + kRot.m_pEntry[1][1] * kSrcN.y +
                        kRot.m_pEntry[1][2] * kSrcN.z;
                    kN.z = kRot.m_pEntry[2][0] * kSrcN.x + kRot.m_pEntry[2][1] * kSrcN.y +
                        kRot.m_pEntry[2][2] * kSrcN.z;
                    NiPoint3 kB;
                    kB.x = kRot.m_pEntry[0][0] * kSrcB.x + kRot.m_pEntry[0][1] * kSrcB.y +
                        kRot.m_pEntry[0][2] * kSrcB.z;
                    kB.y = kRot.m_pEntry[1][0] * kSrcB.x + kRot.m_pEntry[1][1] * kSrcB.y +
                        kRot.m_pEntry[1][2] * kSrcB.z;
                    kB.z = kRot.m_pEntry[2][0] * kSrcB.x + kRot.m_pEntry[2][1] * kSrcB.y +
                        kRot.m_pEntry[2][2] * kSrcB.z;
                    NiPoint3 kT;
                    kT.x = kRot.m_pEntry[0][0] * kSrcT.x + kRot.m_pEntry[0][1] * kSrcT.y +
                        kRot.m_pEntry[0][2] * kSrcT.z;
                    kT.y = kRot.m_pEntry[1][0] * kSrcT.x + kRot.m_pEntry[1][1] * kSrcT.y +
                        kRot.m_pEntry[1][2] * kSrcT.z;
                    kT.z = kRot.m_pEntry[2][0] * kSrcT.x + kRot.m_pEntry[2][1] * kSrcT.y +
                        kRot.m_pEntry[2][2] * kSrcT.z;

                    NiPoint3* pkDst = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(apDstVertex) + uiOffset);
                    NiPoint3 kW;
                    kW.x = kV.x * fWeight;
                    kW.y = kV.y * fWeight;
                    kW.z = kV.z * fWeight;
                    pkDst->x += kW.x;
                    pkDst->y += kW.y;
                    pkDst->z += kW.z;
                    pkDst = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(apDstNormal) + uiOffset);
                    kW.x = kN.x * fWeight;
                    kW.y = kN.y * fWeight;
                    kW.z = kN.z * fWeight;
                    pkDst->x += kW.x;
                    pkDst->y += kW.y;
                    pkDst->z += kW.z;
                    pkDst = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(apDstBinormal) + uiOffset);
                    kW.x = kB.x * fWeight;
                    kW.y = kB.y * fWeight;
                    kW.z = kB.z * fWeight;
                    pkDst->x += kW.x;
                    pkDst->y += kW.y;
                    pkDst->z += kW.z;
                    pkDst = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(apDstTangent) + uiOffset);
                    kW.x = kT.x * fWeight;
                    kW.y = kT.y * fWeight;
                    kW.z = kT.z * fWeight;
                    pkDst->x += kW.x;
                    pkDst->y += kW.y;
                    pkDst->z += kW.z;
                }
            }
            NiPoint3::UnitizeVectors(apDstNormal, auiVertices, auiStride);
            NiPoint3::UnitizeVectors(apDstBinormal, auiVertices, auiStride);
            NiPoint3::UnitizeVectors(apDstTangent, auiVertices, auiStride);
        }
        else {
            if (auiStride == sizeof(NiPoint3)) {
                std::memset(apDstVertex, 0, sizeof(NiPoint3) * auiVertices);
                std::memset(apDstNormal, 0, sizeof(NiPoint3) * auiVertices);
            }
            else {
                NiPoint3* pkV = apDstVertex;
                NiPoint3* pkN = apDstNormal;
                for (uint32_t i = auiVertices; i; --i) {
                    *pkV = NiPoint3::ZERO;
                    *pkN = NiPoint3::ZERO;
                    pkV = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(pkV) + auiStride);
                    pkN = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(pkN) + auiStride);
                }
            }

            for (uint32_t i = 0; i < uiBones; ++i) {
                NiSkinData::BoneData* pkBone = &pkBoneData[i];
                uint32_t uiVerts = pkBone->m_usVerts;
                NiTransform kBoneToSkin = kWorldToSkin * m_ppkBones[i]->m_kWorld * pkBone->m_kSkinToBone;
                const NiMatrix3& kRot = kBoneToSkin.m_Rotate;
                float fScale = kBoneToSkin.m_fScale;
                float s00 = kRot.m_pEntry[0][0] * fScale;
                float s01 = kRot.m_pEntry[0][1] * fScale;
                float s02 = kRot.m_pEntry[0][2] * fScale;
                float s10 = kRot.m_pEntry[1][0] * fScale;
                float s11 = kRot.m_pEntry[1][1] * fScale;
                float s12 = kRot.m_pEntry[1][2] * fScale;
                float s20 = kRot.m_pEntry[2][0] * fScale;
                float s21 = kRot.m_pEntry[2][1] * fScale;
                float s22 = kRot.m_pEntry[2][2] * fScale;
                NiSkinData::BoneVertData* pkVertData = pkBone->m_pkBoneVertData;
                for (uint32_t j = 0; j < uiVerts; ++j) {
                    uint32_t uiVert = pkVertData[j].m_usVert;
                    float fWeight = pkVertData[j].m_fWeight;
                    uint32_t uiOffset = uiVert * auiStride;
                    const NiPoint3& kSrcV = apSrcVertex[uiVert];
                    const NiPoint3& kSrcN = apSrcNormal[uiVert];
                    NiPoint3 kV;
                    kV.x = s00 * kSrcV.x + s01 * kSrcV.y + s02 * kSrcV.z + kBoneToSkin.m_Translate.x;
                    kV.y = s10 * kSrcV.x + s11 * kSrcV.y + s12 * kSrcV.z + kBoneToSkin.m_Translate.y;
                    kV.z = s20 * kSrcV.x + s21 * kSrcV.y + s22 * kSrcV.z + kBoneToSkin.m_Translate.z;
                    NiPoint3 kN;
                    kN.x = kRot.m_pEntry[0][0] * kSrcN.x + kRot.m_pEntry[0][1] * kSrcN.y +
                        kRot.m_pEntry[0][2] * kSrcN.z;
                    kN.y = kRot.m_pEntry[1][0] * kSrcN.x + kRot.m_pEntry[1][1] * kSrcN.y +
                        kRot.m_pEntry[1][2] * kSrcN.z;
                    kN.z = kRot.m_pEntry[2][0] * kSrcN.x + kRot.m_pEntry[2][1] * kSrcN.y +
                        kRot.m_pEntry[2][2] * kSrcN.z;

                    NiPoint3* pkDst = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(apDstVertex) + uiOffset);
                    NiPoint3 kW;
                    kW.x = kV.x * fWeight;
                    kW.y = kV.y * fWeight;
                    kW.z = kV.z * fWeight;
                    pkDst->x += kW.x;
                    pkDst->y += kW.y;
                    pkDst->z += kW.z;
                    pkDst = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(apDstNormal) + uiOffset);
                    kW.x = kN.x * fWeight;
                    kW.y = kN.y * fWeight;
                    kW.z = kN.z * fWeight;
                    pkDst->x += kW.x;
                    pkDst->y += kW.y;
                    pkDst->z += kW.z;
                }
            }
            NiPoint3::UnitizeVectors(apDstNormal, auiVertices, auiStride);
        }
    }
    else {
        if (auiStride == sizeof(NiPoint3)) {
            std::memset(apDstVertex, 0, sizeof(NiPoint3) * auiVertices);
        }
        else {
            NiPoint3* pkV = apDstVertex;
            for (uint32_t i = auiVertices; i; --i) {
                *pkV = NiPoint3::ZERO;
                pkV = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(pkV) + auiStride);
            }
        }

        for (uint32_t i = 0; i < uiBones; ++i) {
            NiSkinData::BoneData* pkBone = &pkBoneData[i];
            uint32_t uiVerts = pkBone->m_usVerts;
            NiTransform kBoneToSkin = kWorldToSkin * m_ppkBones[i]->m_kWorld * pkBone->m_kSkinToBone;
            const NiMatrix3& kRot = kBoneToSkin.m_Rotate;
            float fScale = kBoneToSkin.m_fScale;
            float s00 = kRot.m_pEntry[0][0] * fScale;
            float s01 = kRot.m_pEntry[0][1] * fScale;
            float s02 = kRot.m_pEntry[0][2] * fScale;
            float s10 = kRot.m_pEntry[1][0] * fScale;
            float s11 = kRot.m_pEntry[1][1] * fScale;
            float s12 = kRot.m_pEntry[1][2] * fScale;
            float s20 = kRot.m_pEntry[2][0] * fScale;
            float s21 = kRot.m_pEntry[2][1] * fScale;
            float s22 = kRot.m_pEntry[2][2] * fScale;
            NiSkinData::BoneVertData* pkVertData = pkBone->m_pkBoneVertData;
            for (uint32_t j = 0; j < uiVerts; ++j) {
                uint32_t uiVert = pkVertData[j].m_usVert;
                float fWeight = pkVertData[j].m_fWeight;
                NiPoint3* pkDst = reinterpret_cast<NiPoint3*>(
                    reinterpret_cast<char*>(apDstVertex) + uiVert * auiStride
                );
                const NiPoint3& kSrcV = apSrcVertex[uiVert];
                NiPoint3 kV;
                kV.x = s00 * kSrcV.x + s01 * kSrcV.y + s02 * kSrcV.z + kBoneToSkin.m_Translate.x;
                kV.y = s10 * kSrcV.x + s11 * kSrcV.y + s12 * kSrcV.z + kBoneToSkin.m_Translate.y;
                kV.z = s20 * kSrcV.x + s21 * kSrcV.y + s22 * kSrcV.z + kBoneToSkin.m_Translate.z;
                // The weighted vertex is not rounded to float before it is added here.
                pkDst->x = kV.x * fWeight + pkDst->x;
                pkDst->y = kV.y * fWeight + pkDst->y;
                pkDst->z = kV.z * fWeight + pkDst->z;
            }
        }
    }
}
