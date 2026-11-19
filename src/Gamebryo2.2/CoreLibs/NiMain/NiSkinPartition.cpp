#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "Fallout/AI/BaseProcess.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTCBRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

NiRTTI NiSkinPartition::ms_RTTI("NiSkinPartition", &NiObject::ms_RTTI);

using NiTSet_Interaction_NiTMallocInterface_Interaction =
    NiTSet<NiSkinPartition::Interaction, NiTMallocInterface<NiSkinPartition::Interaction>>;

NiSkinPartition::VertexInteractions::VertexInteractions() : NiTPrimitiveSet<Interaction>(0) {}

void NiSkinPartition::VertexInteractions::Unitize() {
    float v12 = 0.0;
    int32_t m_uiUsed = this->m_uiUsed;
    uint32_t v2 = 0;
    if (m_uiUsed >= 4) {
        float* p_m_fWeight = &m_pBase[1].m_fWeight;
        uint32_t v4 = (static_cast<uint32_t>(m_uiUsed - 4) >> 2) + 1;
        v2 = 4 * v4;
        do {
            double v5 = *(p_m_fWeight - 2);
            p_m_fWeight += 8;
            --v4;
            float v13 = v5 + v12;
            float v14 = v13 + *(p_m_fWeight - 8);
            float v15 = v14 + *(p_m_fWeight - 6);
            v12 = v15 + *(p_m_fWeight - 4);
        }
        while (v4);
    }
    if (v2 < m_uiUsed) {
        float* v6 = &m_pBase[v2].m_fWeight;
        uint32_t v7 = m_uiUsed - v2;
        do {
            double v8 = *v6;
            v6 += 2;
            --v7;
            v12 = v8 + v12;
        }
        while (v7);
    }
    uint32_t v9 = 0;
    float v16 = 1.0 / v12;
    if (m_uiUsed) {
        do {
            double v10 = v16 * m_pBase[v9].m_fWeight;
            float* v11 = &m_pBase[v9++].m_fWeight;
            *v11 = v10;
        }
        while (v9 < this->m_uiUsed);
    }
}

// Sorts by descending weight.
int32_t NiSkinPartition::VertexInteractions::CompareWeights(const void* pv0, const void* pv1) {
    float fWeight0 = static_cast<const Interaction*>(pv0)->m_fWeight;
    float fWeight1 = static_cast<const Interaction*>(pv1)->m_fWeight;
    if (fWeight1 < fWeight0) {
        return -1;
    }
    return fWeight1 > fWeight0;
}

bool NiSkinPartition::BoneSet::Contains(const NiSkinPartition::BoneSet& kThat) const {
    uint32_t m_uiUsed;
    uint8_t v5;
    uint16_t* v6;
    uint32_t v2 = 0;
    uint32_t v3 = 0;
    if (!(&kThat)->m_uiUsed) {
        return true;
    }
    m_uiUsed = this->m_uiUsed;
    while (true) {
        v5 = (uint8_t)(v2 == m_uiUsed);
        if (v2 < m_uiUsed) {
            v6 = &m_pBase[v2];
            do {
                if (((uint32_t)((int32_t)(*v6))) >= ((uint32_t)((int32_t)(&kThat)->m_pBase[v3]))) {
                    break;
                }
                ++v2;
                ++v6;
            }
            while (v2 < m_uiUsed);
            v5 = (uint8_t)(v2 == m_uiUsed);
        }
        if (v5 || (m_pBase[v2] != (&kThat)->m_pBase[v3])) {
            break;
        }
        ++v3;
        ++v2;
        if (v3 >= (&kThat)->m_uiUsed) {
            return true;
        }
    }
    return false;
}

uint32_t NiSkinPartition::BoneSet::MergedSize(const NiSkinPartition::BoneSet& kThat) const {
    NiSkinPartition::BoneSetMerger v6;
    v6.m_kSet0 = this;
    uint16_t v2 = *m_pBase;
    v6.m_kSet1 = &kThat;
    uint16_t v3 = *kThat.m_pBase;
    uint32_t uiMergeSize = 0;
    v6.m_uiIndex[1] = 0;
    v6.m_uiIndex[0] = 0;
    v6.m_usEl[0] = v2;
    v6.m_usEl[1] = v3;
    while (v6.m_usEl[0] != 0xFFFF || v6.m_usEl[1] != 0xFFFF) {
        v6.GetNext();
        ++uiMergeSize;
    }
    return uiMergeSize;
}

void NiSkinPartition::BoneSet::Merge(const NiSkinPartition::BoneSet& kThat) {
    NiSkinPartition::BoneSetMerger v9;
    uint16_t v3 = *this->m_pBase;
    uint16_t* m_pBase = kThat.m_pBase;
    uint32_t uiNewAlloced = m_uiUsed + kThat.m_uiUsed;
    v9.m_usEl[0] = v3;
    uint16_t v6 = *m_pBase;
    v9.m_kSet1 = &kThat;
    uint32_t uiMergeSize = 0;
    v9.m_kSet0 = this;
    v9.m_uiIndex[1] = 0;
    v9.m_uiIndex[0] = 0;
    v9.m_usEl[1] = v6;
    uint16_t* pusNewBase = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * uiNewAlloced));
    while (v9.m_usEl[0] != 0xFFFF || v9.m_usEl[1] != 0xFFFF) {
        pusNewBase[uiMergeSize++] = v9.GetNext();
    }
    _NiFree(this->m_pBase);
    m_uiAlloced = uiNewAlloced;
    this->m_pBase = pusNewBase;
    m_uiUsed = uiMergeSize;
}

// Whether every bone kVertex interacts with is in this sorted set.
bool NiSkinPartition::BoneSet::ContainsVertex(const NiSkinPartition::VertexInteractions& kVertex) const {
    uint32_t uiVertexBones = kVertex.m_uiUsed;
    for (uint32_t b = 0; b < uiVertexBones; b++) {
        uint32_t usBone = static_cast<uint16_t>(kVertex.m_pBase[b].m_uiBone);
        if (!std::bsearch(&usBone, m_pBase, m_uiUsed, sizeof(uint16_t), &NiSkinPartition::BoneSet::CompareBones)) {
            return false;
        }
    }
    return true;
}

bool NiSkinPartition::BoneSet::ContainsTriangle(const uint16_t* pusTriangle, const VertexInteractions* pkVertex) const {
    uint32_t v = 0;
    while (ContainsVertex(pkVertex[pusTriangle[v]])) {
        if (++v >= 3) {
            return true;
        }
    }
    return false;
}

// The qsort and bsearch compare of two bone indices: ascending.
int32_t NiSkinPartition::BoneSet::CompareBones(const void* pv0, const void* pv1) {
    uint16_t us0 = *static_cast<const uint16_t*>(pv0);
    uint16_t us1 = *static_cast<const uint16_t*>(pv1);
    if (us0 < us1) {
        return -1;
    }
    return us1 < us0;
}

uint16_t NiSkinPartition::BoneSetMerger::GetNext() {
    uint16_t result;
    uint16_t v1 = m_usEl[1];
    uint16_t v2 = m_usEl[0];
    if (v1 >= v2) {
        result = m_usEl[0];
        if (v1 == v2) {
            uint16_t v8;
            uint32_t v6 = ++m_uiIndex[1];
            const NiSkinPartition::BoneSet* m_pkSet1 = this->m_kSet1;
            if (v6 >= m_pkSet1->m_uiUsed) {
                v8 = -1;
            }
            else {
                v8 = m_pkSet1->m_pBase[v6];
            }
            m_usEl[1] = v8;
        }
        ++m_uiIndex[0];
        const NiSkinPartition::BoneSet* m_pkSet0 = this->m_kSet0;
        if (m_uiIndex[0] >= m_pkSet0->m_uiUsed) {
            m_usEl[0] = -1;
        }
        else {
            m_usEl[0] = m_pkSet0->m_pBase[m_uiIndex[0]];
        }
    }
    else {
        ++m_uiIndex[1];
        const NiSkinPartition::BoneSet* v3 = this->m_kSet1;
        result = v1;
        uint32_t v5 = m_uiIndex[1];
        if (v5 >= v3->m_uiUsed) {
            m_usEl[1] = -1;
        }
        else {
            m_usEl[1] = v3->m_pBase[v5];
        }
    }
    return result;
}

void NiSkinPartition::TriangleSet::AddTriangles(
    uint32_t uiPartition,
    const uint32_t* puiTriangleToPartition,
    uint32_t uiTriangles
) {
    for (uint32_t i = 0; i < uiTriangles; i++) {
        if (puiTriangleToPartition[i] == uiPartition) {
            if (m_uiUsed == m_uiAlloced) {
                Realloc(m_uiAlloced ? m_uiAlloced * 2 : 1);
            }
            m_pBase[m_uiUsed++] = uint16_t(i);
        }
    }
}

NiSkinPartition::Partition::Partition()
  : m_pusBones(nullptr), m_pfWeights(nullptr), m_pusVertexMap(nullptr), m_pucBonePalette(nullptr),
    m_pusTriList(nullptr), m_pusStripLengths(nullptr), m_usStrips(0), m_pkBuffData(nullptr) {}

NiSkinPartition::Partition::~Partition() {
    _NiFree(m_pusBones);
    _NiFree(m_pfWeights);
    _NiFree(m_pusVertexMap);
    _NiFree(m_pucBonePalette);
    _NiFree(m_pusTriList);
    _NiFree(m_pusStripLengths);
}

uint32_t NiSkinPartition::Partition::GetStripLengthSum() const {
    uint32_t uiSum = 0;
    for (uint16_t i = 0; i < m_usStrips; i++) {
        uiSum += m_pusStripLengths[i];
    }
    return uiSum;
}

// Fills this partition from one bone set: the bones (padded to ucBonesPerVertex), the vertex map, the triangles, then
// the weights, either sorted per vertex with a bone palette or one weight per partition bone. Only the low byte of each
// of the three int32_t parameters is read.
void NiSkinPartition::Partition::Create(
    const NiSkinPartition::TriangleSet& kTriangles,
    const NiSkinPartition::BoneSet& kBones,
    const NiTriBasedGeomData* pkTriData,
    const NiSkinPartition::VertexInteractions* pkVertexInteraction,
    int32_t bNeedPalette,
    int32_t bForcePalette,
    int32_t ucBonesPerVertex,
    uint32_t uiMaxBones
) {
    uint8_t ucBones = static_cast<uint8_t>(ucBonesPerVertex);
    uint16_t v13;
    const NiSkinPartition::BoneSet* v9 = &kBones;
    int32_t m_uiUsed = kBones.m_uiUsed;
    if (m_uiUsed <= ucBones) {
        m_uiUsed = ucBones;
    }
    this->m_usBones = m_uiUsed;
    m_usTriangles = kTriangles.m_uiUsed;
    m_pusBones = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * static_cast<uint16_t>(m_uiUsed)));
    for (uint32_t i = 0; i < this->m_usBones; m_pusBones[i++] = v13) {
        if (i >= kBones.m_uiUsed) {
            v13 = 0;
        }
        else {
            v13 = kBones.m_pBase[i];
        }
    }
    uint16_t* pusVertexPartitionIndex = CreateVertexMap(kTriangles, pkTriData);
    CreateTriangleList(kTriangles, pkTriData, pusVertexPartitionIndex);
    _NiFree(pusVertexPartitionIndex);
    if (static_cast<uint8_t>(bNeedPalette)) {
        uint32_t* puiBonePartitionIndex;
        this->m_usBonesPerVertex = ucBones;
        uint32_t uiInteractions_2 = m_usVertices * ucBones;
        m_pfWeights = static_cast<float*>(_NiMalloc(sizeof(float) * uiInteractions_2));
        m_pucBonePalette = static_cast<uint8_t*>(_NiMalloc(uiInteractions_2));
        uint32_t* puiBonePartitionIndex_2 = static_cast<uint32_t*>(_NiMalloc(sizeof(uint32_t) * uiMaxBones));
        uint32_t v17 = 0;
        for (puiBonePartitionIndex = puiBonePartitionIndex_2; v17 < kBones.m_uiUsed; ++v17) {
            puiBonePartitionIndex_2[m_pusBones[v17]] = v17;
        }
        float* arBones = m_pfWeights;
        uint8_t* pucBonePalette = m_pucBonePalette;
        float* pfTempWeightArray = static_cast<float*>(_NiMalloc(sizeof(float) * this->m_usBonesPerVertex));
        uint8_t* pucTempIndexArray = static_cast<uint8_t*>(_NiMalloc(this->m_usBonesPerVertex));
        for (uint32_t aucBonesPerVertex = 0; aucBonesPerVertex < m_usVertices; ++aucBonesPerVertex) {
            float* j;
            uint8_t* pucTempIndex = pucTempIndexArray;
            const NiSkinPartition::VertexInteractions* v20 = &pkVertexInteraction[m_pusVertexMap[aucBonesPerVertex]];
            uint32_t v21 = 0;
            const NiSkinPartition::VertexInteractions* bForcePalettea = v20;
            for (j = pfTempWeightArray; v21 < bForcePalettea->m_uiUsed; ++pucTempIndex) {
                NiSkinPartition::Interaction* m_pBase = v20->m_pBase;
                *j = m_pBase[v21].m_fWeight;
                *pucTempIndex = puiBonePartitionIndex_2[m_pBase[v21].m_uiBone];
                v20 = bForcePalettea;
                ++v21;
                ++j;
            }
            for (; v21 < this->m_usBonesPerVertex; ++pucTempIndex) {
                ++v21;
                *j = 0.0f;
                *pucTempIndex = 0;
                ++j;
            }
            uint32_t v24 = 0;
            if (this->m_usBonesPerVertex) {
                uint32_t m_usBonesPerVertex = this->m_usBonesPerVertex;
                do {
                    uint32_t j_2 = 1;
                    float fTemp = *pfTempWeightArray;
                    uint32_t uiIndex = 0;
                    if (this->m_usBonesPerVertex > 1u) {
                        do {
                            if (fTemp < static_cast<double>(pfTempWeightArray[j_2])) {
                                uiIndex = j_2;
                                fTemp = pfTempWeightArray[j_2];
                            }
                            ++j_2;
                        }
                        while (j_2 < m_usBonesPerVertex);
                    }
                    *arBones++ = pfTempWeightArray[uiIndex];
                    *pucBonePalette = pucTempIndexArray[uiIndex];
                    pfTempWeightArray[uiIndex] = -3.4028235e+38f;
                    ++pucBonePalette;
                    m_usBonesPerVertex = this->m_usBonesPerVertex;
                    ++v24;
                }
                while (v24 < m_usBonesPerVertex);
            }
            puiBonePartitionIndex_2 = puiBonePartitionIndex;
        }
        _NiFree(pfTempWeightArray);
        _NiFree(pucTempIndexArray);
        _NiFree(puiBonePartitionIndex_2);
    }
    else {
        uint16_t m_usBones = this->m_usBones;
        this->m_usBonesPerVertex = m_usBones;
        uint32_t uiInteractions_3 = m_usVertices * m_usBones;
        uint32_t uiInteractions = uiInteractions_3;
        float* v30 = static_cast<float*>(_NiMalloc(sizeof(float) * uiInteractions_3));
        m_pfWeights = v30;
        uint32_t v31 = 0;
        float* bNeedPalettea = v30;
        for (uint32_t aucBonesPerVertexa = 0; v31 < m_usVertices; aucBonesPerVertexa = ++v31) {
            const NiSkinPartition::VertexInteractions* v32 = &pkVertexInteraction[m_pusVertexMap[v31]];
            uint32_t v33 = 0;
            const NiSkinPartition::VertexInteractions* apTriData = v32;
            if (v9->m_uiUsed) {
                while (true) {
                    float arTriangles;
                    uint32_t v34 = v32->m_uiUsed;
                    uint32_t v35 = 0;
                    if (v34) {
                        NiSkinPartition::Interaction* v36 = v32->m_pBase;
                        int32_t v37 = v9->m_pBase[v33];
                        NiSkinPartition::Interaction* v38 = v36;
                        while (v38->m_uiBone != v37) {
                            ++v35;
                            ++v38;
                            if (v35 >= v34) {
                                v30 = bNeedPalettea;
                                v9 = &kBones;
                                goto LABEL_34;
                            }
                        }
                        v30 = bNeedPalettea;
                        v9 = &kBones;
                        arTriangles = v36[v35].m_fWeight;
                    }
                    else {
                    LABEL_34:;
                        arTriangles = 0.0f;
                    }
                    *v30++ = arTriangles;
                    ++v33;
                    bNeedPalettea = v30;
                    if (v33 >= v9->m_uiUsed) {
                        break;
                    }
                    v32 = apTriData;
                }
                v31 = aucBonesPerVertexa;
                uiInteractions_3 = uiInteractions;
            }
            if (v33 < this->m_usBones) {
                do {
                    *v30 = 0.0f;
                    ++v33;
                    ++v30;
                }
                while (v33 < this->m_usBones);
                bNeedPalettea = v30;
            }
        }
        if (static_cast<uint8_t>(bForcePalette)) {
            uint8_t* v39 = static_cast<uint8_t*>(_NiMalloc(uiInteractions_3));
            m_pucBonePalette = v39;
            uint8_t* v40 = v39;
            for (uint32_t k = 0; k < m_usVertices; ++k) {
                for (uint32_t m = 0; m < this->m_usBones; *v40++ = m++) {
                    ;
                }
            }
        }
    }
}

void NiSkinPartition::Partition::LoadBinary(NiStream& kStream) {
    NiStreamLoadBinary(kStream, m_usVertices);
    NiStreamLoadBinary(kStream, m_usTriangles);
    NiStreamLoadBinary(kStream, m_usBones);
    NiStreamLoadBinary(kStream, m_usStrips);
    NiStreamLoadBinary(kStream, m_usBonesPerVertex);

    m_pusBones = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * m_usBones));
    NiStreamLoadBinary(kStream, m_pusBones, m_usBones);

    // From 10.0.1.18 on, each array but the bone list is preceded by whether it is present.
    bool bHasVertexMap;
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 0, 1, 18)) {
        bHasVertexMap = true;
    }
    else {
        NiStreamLoadBinary(kStream, bHasVertexMap);
    }
    if (bHasVertexMap) {
        m_pusVertexMap = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * m_usVertices));
        NiStreamLoadBinary(kStream, m_pusVertexMap, m_usVertices);
    }

    uint32_t uiInteractions = m_usVertices * m_usBonesPerVertex;

    bool bHasWeights;
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 0, 1, 18)) {
        bHasWeights = true;
    }
    else {
        NiStreamLoadBinary(kStream, bHasWeights);
    }
    if (bHasWeights) {
        m_pfWeights = static_cast<float*>(_NiMalloc(sizeof(float) * uiInteractions));
        NiStreamLoadBinary(kStream, m_pfWeights, uiInteractions);
    }

    uint32_t uiTriListLength;
    if (m_usStrips) {
        m_pusStripLengths = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * m_usStrips));
        NiStreamLoadBinary(kStream, m_pusStripLengths, m_usStrips);
        uiTriListLength = GetStripLengthSum();
    }
    else {
        uiTriListLength = 3 * m_usTriangles;
    }

    bool bHasTriList;
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 0, 1, 18)) {
        bHasTriList = true;
    }
    else {
        NiStreamLoadBinary(kStream, bHasTriList);
    }
    if (bHasTriList) {
        m_pusTriList = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * uiTriListLength));
        NiStreamLoadBinary(kStream, m_pusTriList, uiTriListLength);
    }

    // The bone palette's flag is in every version.
    bool bHasBonePalette;
    NiStreamLoadBinary(kStream, bHasBonePalette);
    if (bHasBonePalette) {
        m_pucBonePalette = static_cast<uint8_t*>(_NiMalloc(uiInteractions));
        NiStreamLoadBinary(kStream, m_pucBonePalette, uiInteractions);
    }
}

// KEEP: niread: from 10.0.1.18 on a short read leaves a presence flag unwritten (bHasVertexMap is read into the
// stream argument's slot, the others into unwritten locals), and the exe tests it.
void NiSkinPartition::Partition::SaveBinary(NiStream& kStream) {
    NiStreamSaveBinary(kStream, m_usVertices);
    NiStreamSaveBinary(kStream, m_usTriangles);
    NiStreamSaveBinary(kStream, m_usBones);
    NiStreamSaveBinary(kStream, m_usStrips);
    NiStreamSaveBinary(kStream, m_usBonesPerVertex);

    NiStreamSaveBinary(kStream, m_pusBones, m_usBones);

    bool bHasVertexMap = m_pusVertexMap != nullptr;
    NiStreamSaveBinary(kStream, bHasVertexMap);
    if (bHasVertexMap) {
        NiStreamSaveBinary(kStream, m_pusVertexMap, m_usVertices);
    }

    uint32_t uiInteractions = m_usVertices * m_usBonesPerVertex;

    bool bHasWeights = m_pfWeights != nullptr;
    NiStreamSaveBinary(kStream, bHasWeights);
    if (bHasWeights) {
        NiStreamSaveBinary(kStream, m_pfWeights, uiInteractions);
    }

    uint32_t uiTriListLength;
    if (m_usStrips) {
        NiStreamSaveBinary(kStream, m_pusStripLengths, m_usStrips);
        uiTriListLength = GetStripLengthSum();
    }
    else {
        uiTriListLength = 3 * m_usTriangles;
    }

    bool bHasTriList = m_pusTriList != nullptr;
    NiStreamSaveBinary(kStream, bHasTriList);
    if (bHasTriList) {
        NiStreamSaveBinary(kStream, m_pusTriList, uiTriListLength);
    }

    bool bHasBonePalette = m_pucBonePalette != nullptr;
    NiStreamSaveBinary(kStream, bHasBonePalette);
    if (bHasBonePalette) {
        NiStreamSaveBinary(kStream, m_pucBonePalette, uiInteractions);
    }
}

// The arrays are compared byte for byte, the weights included. Only the bone palette may be missing on one side.
bool NiSkinPartition::Partition::IsEqual(const Partition& kPartition) {
    if (m_usVertices != kPartition.m_usVertices || m_usTriangles != kPartition.m_usTriangles ||
        m_usBones != kPartition.m_usBones || m_usStrips != kPartition.m_usStrips ||
        m_usBonesPerVertex != kPartition.m_usBonesPerVertex) {
        return false;
    }

    if (std::memcmp(m_pusBones, kPartition.m_pusBones, sizeof(uint16_t) * m_usBones)) {
        return false;
    }

    if (std::memcmp(m_pusVertexMap, kPartition.m_pusVertexMap, sizeof(uint16_t) * m_usVertices)) {
        return false;
    }

    uint32_t uiInteractions = m_usVertices * m_usBonesPerVertex;
    if (std::memcmp(m_pfWeights, kPartition.m_pfWeights, sizeof(float) * uiInteractions)) {
        return false;
    }

    if (m_usStrips) {
        if (std::memcmp(m_pusStripLengths, kPartition.m_pusStripLengths, sizeof(uint16_t) * m_usStrips)) {
            return false;
        }

        if (std::memcmp(m_pusTriList, kPartition.m_pusTriList, sizeof(uint16_t) * GetStripLengthSum())) {
            return false;
        }
    }
    else {
        if (std::memcmp(m_pusTriList, kPartition.m_pusTriList, sizeof(uint16_t) * 3 * m_usTriangles)) {
            return false;
        }
    }

    if ((m_pucBonePalette == nullptr) != (kPartition.m_pucBonePalette == nullptr)) {
        return false;
    }

    if (m_pucBonePalette && std::memcmp(m_pucBonePalette, kPartition.m_pucBonePalette, uiInteractions)) {
        return false;
    }

    return true;
}

// Maps each model vertex the triangles use to a partition vertex (USHRT_MAX for none) and builds the reverse map.
uint16_t* NiSkinPartition::Partition::CreateVertexMap(
    const NiSkinPartition::TriangleSet& kTriangles,
    const NiTriBasedGeomData* pkTriData
) {
    uint32_t uiVertices = pkTriData->m_usVertices;
    uint16_t* pusVertexPartitionIndex = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * uiVertices));
    if (uiVertices) {
        char cOdd = uiVertices & 1;
        std::memset(pusVertexPartitionIndex, 255u, 4 * (uiVertices >> 1));
        uint16_t* pusTail = &pusVertexPartitionIndex[2 * (uiVertices >> 1)];
        for (int32_t i = cOdd; i; --i) {
            *pusTail++ = 0xFFFF;
        }
    }
    uint32_t uiPartitionVertices = 0;
    for (uint32_t t = 0; t < m_usTriangles; ++t) {
        uint16_t ausTriangle[3];
        pkTriData->GetTriangleIndices(kTriangles.m_pBase[t], ausTriangle[0], ausTriangle[1], ausTriangle[2]);
        for (uint32_t k = 0; k < 3; ++k) {
            uint32_t uiVert = ausTriangle[k];
            if (pusVertexPartitionIndex[uiVert] == 0xFFFF) {
                pusVertexPartitionIndex[uiVert] = uiPartitionVertices++;
            }
        }
    }
    uint16_t* pusVertexMap = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * uiPartitionVertices));
    m_usVertices = uiPartitionVertices;
    m_pusVertexMap = pusVertexMap;
    for (uint32_t v = 0; v < uiVertices; ++v) {
        uint16_t usPartitionVert = pusVertexPartitionIndex[v];
        if (usPartitionVert != 0xFFFF) {
            m_pusVertexMap[usPartitionVert] = v;
        }
    }
    return pusVertexPartitionIndex;
}

// GetTriangleIndices is passed three adjacent words of one local array.
void NiSkinPartition::Partition::CreateTriangleList(
    const NiSkinPartition::TriangleSet& kTriangles,
    const NiTriBasedGeomData* pkTriData,
    const uint16_t* pusVertexPartitionIndex
) {
    m_pusTriList = static_cast<uint16_t*>(_NiMalloc(3 * sizeof(uint16_t) * m_usTriangles));
    for (uint32_t t = 0; t < m_usTriangles; ++t) {
        uint16_t ausTriangle[3];
        pkTriData->GetTriangleIndices(kTriangles.m_pBase[t], ausTriangle[0], ausTriangle[1], ausTriangle[2]);
        uint16_t* pusPartitionTriangle = &m_pusTriList[3 * t];
        pusPartitionTriangle[0] = pusVertexPartitionIndex[ausTriangle[0]];
        pusPartitionTriangle[1] = pusVertexPartitionIndex[ausTriangle[1]];
        pusPartitionTriangle[2] = pusVertexPartitionIndex[ausTriangle[2]];
    }
}

void NiSkinPartition::Partition::operator delete(void* apMemory) {
    ourFree(apMemory);
}

NiSkinPartition::NiSkinPartition() : m_pkPartitions(nullptr) {}

NiSkinPartition::~NiSkinPartition() {
    if (NiRenderer::ms_pkRenderer) {
        NiRenderer::ms_pkRenderer->PurgeSkinPartitionRendererData(this);
    }
    delete[] m_pkPartitions;
}

void NiSkinPartition::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_uiPartitions);

    // A real new[]: clang gives the class a vector deleting destructor only where its new[] is seen, and the
    // destructor's delete[] goes through it.
    Partition* pkPartitions = new Partition[m_uiPartitions];
    m_pkPartitions = pkPartitions;

    for (uint32_t i = 0; i < m_uiPartitions; i++) {
        m_pkPartitions[i].LoadBinary(kStream);
    }
}

void NiSkinPartition::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
}

bool NiSkinPartition::RegisterStreamables(NiStream& kStream) {
    return NiObject::RegisterStreamables(kStream);
}

void NiSkinPartition::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_uiPartitions);
    for (uint32_t i = 0; i < m_uiPartitions; i++) {
        m_pkPartitions[i].SaveBinary(kStream);
    }
}

bool NiSkinPartition::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiSkinPartition* pkSkinPartition = static_cast<NiSkinPartition*>(pkObject);

    if (m_uiPartitions != pkSkinPartition->m_uiPartitions) {
        return false;
    }

    for (uint32_t i = 0; i < m_uiPartitions; i++) {
        if (!m_pkPartitions[i].IsEqual(pkSkinPartition->m_pkPartitions[i])) {
            return false;
        }
    }

    return true;
}

void NiSkinPartition::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiObject::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(ms_RTTI.m_pcName));

    for (uint32_t i = 0; i < m_uiPartitions; i++) {
        char acPartition[32];
        BSsprintf(acPartition, sizeof(acPartition), "Partition %d = %d bones", i, m_pkPartitions[i].m_usBones);
        pkStrings->Add(NiGetViewerString(acPartition));
    }
}

void NiSkinPartition::ReplacePartitions(NiSkinPartition::Partition* pkPartitions, uint32_t uiPartitions) {
    m_pkPartitions = pkPartitions;
    m_uiPartitions = uiPartitions;
}

// Splits the mesh into hardware partitions: one vertex interaction set per vertex, the bone sets merged into groups of
// at most ucBonesPerPartition bones, one partition per bone set, then the partitions left without vertices removed.
// The three trailing arrays are only written when all of them are given.
bool NiSkinPartition::MakePartitions(
    const NiTriBasedGeomData* pkTriData,
    const NiSkinData* pkSkinData,
    int32_t ucBonesPerPartition,
    int32_t ucBonesPerVertex,
    int32_t bForcePalette,
    int32_t* apWArray,
    uint16_t* apusPartNumA,
    bool* apbStateChangeA
) {
    NiBoneSetCollection kBoneSets;
    // MakeBoneSets always writes it.
    uint32_t* puiTriangleToPartition;

    VertexInteractions* pkVertexInteraction = CreateVertexInteractions(pkTriData->m_usVertices, pkSkinData);
    if (!pkVertexInteraction) {
        return false;
    }

    bool bPartition = RemoveExcessInteractions(pkTriData, ucBonesPerPartition, ucBonesPerVertex, pkVertexInteraction);
    if (bPartition) {
        bPartition = MakeBoneSets(
            pkTriData, pkSkinData, ucBonesPerPartition, pkVertexInteraction, kBoneSets, puiTriangleToPartition, apWArray
        );

        if (apWArray && apusPartNumA && apbStateChangeA) {
            for (uint32_t i = 0; i < kBoneSets.m_uiUsed; ++i) {
                BoneSet* pkBoneSet = kBoneSets.m_pBase[i];
                apusPartNumA[i] = static_cast<uint16_t>(pkBoneSet->iWValue);
                if (i == 0) {
                    apbStateChangeA[0] = true;
                }
                else {
                    BoneSet* pkPrevious = kBoneSets.m_pBase[i - 1];
                    apbStateChangeA[i] =
                        !(pkBoneSet->Contains(*pkPrevious) && pkBoneSet->GetSize() == pkPrevious->GetSize());
                }
            }
        }

        // A real new[], as in LoadBinary.
        Partition* pkPartitions = new Partition[m_uiPartitions];

        TriangleSet kTriangles;

        // The flag is stored over the low byte of ucBonesPerPartition, keeping its upper bits, and that whole value is
        // passed to Create, as the game does.
        int32_t bNeedPalette = static_cast<int32_t>(
            (static_cast<uint32_t>(ucBonesPerPartition) & 0xFFFFFF00u) |
            (static_cast<uint8_t>(ucBonesPerPartition) != static_cast<uint8_t>(ucBonesPerVertex) ? 1u : 0u)
        );
        for (uint32_t p = 0; p < m_uiPartitions; ++p) {
            kTriangles.AddTriangles(p, puiTriangleToPartition, pkTriData->m_usTriangles);
            BoneSet* pkBoneSet = kBoneSets.GetAt(p);
            pkPartitions[p].Create(
                kTriangles, *pkBoneSet, pkTriData, pkVertexInteraction, bNeedPalette, bForcePalette, ucBonesPerVertex,
                pkSkinData->m_uiBones
            );
            kTriangles.RemoveAll();
            // NiDelete of the bone set now that its partition holds a copy of its bones.
            if (pkBoneSet) {
                _NiFree(pkBoneSet->m_pBase);
                MemoryManager_Singleton.Deallocate(pkBoneSet);
            }
        }

        _NiFree(puiTriangleToPartition);

        // Drop the partitions that got no vertices, shifting the ones after each down.
        for (;;) {
            if (m_uiPartitions == 0) {
                break;
            }
            uint32_t uiEmpty = 0;
            while (uiEmpty < m_uiPartitions && pkPartitions[uiEmpty].m_usVertices != 0) {
                ++uiEmpty;
            }
            if (uiEmpty == m_uiPartitions) {
                break;
            }
            for (uint32_t s = uiEmpty; s < m_uiPartitions - 1; ++s) {
                rt::memcpy(&pkPartitions[s], &pkPartitions[s + 1], sizeof(Partition));
                apWArray[s] = apWArray[s + 1];
                apusPartNumA[s] = apusPartNumA[s + 1];
                apbStateChangeA[s] = apbStateChangeA[s + 1];
            }
            --m_uiPartitions;
        }

        m_pkPartitions = pkPartitions;
    }

    // NiDelete[] of the interaction array: each set's buffer last to first, then the block (its count sits before it;
    // the raw NiNew[] block, untyped memory).
    int32_t iInteraction = static_cast<int32_t>(reinterpret_cast<size_t*>(pkVertexInteraction)[-1]) - 1;
    for (; iInteraction >= 0; --iInteraction) {
        _NiFree(pkVertexInteraction[iInteraction].m_pBase);
    }
    MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(pkVertexInteraction) - 1);
    return bPartition;
}

// Merges the bone sets that fit into one partition until no pair does: the pair with the largest share wins, and when
// the merged set and the other one carry different weight values both are merged and the search restarts. The sets that
// then became identical are grouped, and the collection is selection-sorted by group so equal sets are adjacent (the
// group array is a 50-entry stack array with no bound check, as the game does).
void NiSkinPartition::MergeBoneSets(int32_t ucBonesPerPartition, NiBoneSetCollection& kBoneSets) {
    for (;;) {
        uint32_t auiMergeSets[2] = {0, 0};
        int32_t iMaxShare = -1;

        uint32_t p0 = 0;
        while (p0 < kBoneSets.GetSize()) {
            BoneSet* pkBoneSet0 = kBoneSets.GetAt(p0);
            for (uint32_t p1 = p0 + 1; p1 < kBoneSets.GetSize(); ++p1) {
                BoneSet* pkBoneSet1 = kBoneSets.GetAt(p1);
                uint32_t uiMergeSize = pkBoneSet0->MergedSize(*pkBoneSet1);
                if (uiMergeSize <= static_cast<uint8_t>(ucBonesPerPartition)) {
                    int32_t iShare = static_cast<int32_t>(pkBoneSet0->GetSize()) +
                        static_cast<int32_t>(pkBoneSet1->GetSize()) - static_cast<int32_t>(uiMergeSize);
                    // Drops the share of two equally large sets to -1, as the game does.
                    if (pkBoneSet0->GetSize() == pkBoneSet1->GetSize()) {
                        iShare = -1;
                    }
                    if (iShare > iMaxShare) {
                        auiMergeSets[0] = p0;
                        auiMergeSets[1] = p1;
                        iMaxShare = iShare;
                    }
                }
            }
            ++p0;
        }

        if (iMaxShare < 0) {
            break;
        }

        kBoneSets.GetAt(auiMergeSets[0])->Merge(*kBoneSets.GetAt(auiMergeSets[1]));
        BoneSet* pkOtherSet = kBoneSets.GetAt(auiMergeSets[1]);
        if (kBoneSets.GetAt(auiMergeSets[0])->iWValue != pkOtherSet->iWValue) {
            pkOtherSet->Merge(*kBoneSets.GetAt(auiMergeSets[0]));
            continue;
        }

        // NiDelete of the merged-in set, then its removal from the collection.
        _NiFree(pkOtherSet->m_pBase);
        MemoryManager_Singleton.Deallocate(pkOtherSet);
        kBoneSets.RemoveAt(auiMergeSets[1]);

        BoneSet* pkNewSet = kBoneSets.GetAt(auiMergeSets[0]);
        uint32_t p = 0;
        while (p < kBoneSets.GetSize()) {
            BoneSet* pkSet = kBoneSets.GetAt(p);
            if (pkSet != pkNewSet && pkNewSet->Contains(*pkSet)) {
                _NiFree(pkSet->m_pBase);
                MemoryManager_Singleton.Deallocate(pkSet);
                kBoneSets.RemoveAt(p);
            }
            else {
                ++p;
            }
        }
    }

    // Group the sets that hold the same bones; a set takes the group of the first equal set before it.
    int32_t aiGroups[50];
    int32_t iGroupCount = 0;
    for (uint32_t p = 0; p < kBoneSets.GetSize(); ++p) {
        if (p != 0) {
            BoneSet* pkSet = kBoneSets.GetAt(p);
            uint32_t q = 0;
            while (q < p) {
                BoneSet* pkOther = kBoneSets.GetAt(q);
                if (pkSet->Contains(*pkOther) && pkOther->Contains(*pkSet)) {
                    break;
                }
                ++q;
            }
            if (q != p) {
                aiGroups[p] = aiGroups[q];
                continue;
            }
        }
        aiGroups[p] = iGroupCount;
        ++iGroupCount;
    }

    uint32_t uiCount = kBoneSets.GetSize();
    for (uint32_t i = 0; i < uiCount; ++i) {
        for (uint32_t j = 0; j < i; ++j) {
            if (aiGroups[i] > aiGroups[j]) {
                int32_t iTemp = aiGroups[i];
                aiGroups[i] = aiGroups[j];
                aiGroups[j] = iTemp;
                BoneSet* pkTemp = kBoneSets.GetAt(i);
                kBoneSets.ReplaceAt(i, kBoneSets.GetAt(j));
                kBoneSets.ReplaceAt(j, pkTemp);
            }
        }
    }
}

// One interaction set per vertex (NiNew[]: the count before the array), filled from each bone's vertex weights, then
// each set unitized (twice, deliberately, as the game does). The first vertex no bone weights frees the array and fails
// the call. The reinterpret_casts are on the raw NiNew[] block, which is untyped memory: the count, then the array
// after it.
NiSkinPartition::VertexInteractions*
NiSkinPartition::CreateVertexInteractions(uint32_t uiVertices, const NiSkinData* pkSkinData) {
    uint64_t ullSize = uint64_t{sizeof(VertexInteractions)} * uiVertices;
    size_t stSize = ullSize >> 32 ? SIZE_MAX : static_cast<size_t>(ullSize);
    stSize = stSize + sizeof(size_t) < stSize ? SIZE_MAX : stSize + sizeof(size_t);
    void* pvMemory = MemoryManager_Singleton.Allocate(stSize);
    VertexInteractions* pkVertexInteraction = nullptr;
    if (pvMemory) {
        *static_cast<size_t*>(pvMemory) = uiVertices;
        pkVertexInteraction = reinterpret_cast<VertexInteractions*>(static_cast<size_t*>(pvMemory) + 1);
        for (int32_t i = 0; i < static_cast<int32_t>(uiVertices); ++i) {
            ::new (static_cast<void*>(pkVertexInteraction + i)) VertexInteractions;
        }
    }

    uint32_t uiBones = pkSkinData->m_uiBones;
    for (uint32_t uiBone = 0; uiBone < uiBones; ++uiBone) {
        NiSkinData::BoneData* pkBoneData = &pkSkinData->m_pkBoneData[uiBone];
        for (uint32_t i = 0; i < pkBoneData->m_usVerts; ++i) {
            NiSkinData::BoneVertData* pkBoneVertData = pkBoneData->m_pkBoneVertData;
            float fWeight = pkBoneVertData[i].m_fWeight;
            VertexInteractions* pkSet = &pkVertexInteraction[pkBoneVertData[i].m_usVert];
            uint32_t uiAlloced = pkSet->m_uiAlloced;
            if (pkSet->m_uiUsed == uiAlloced) {
                pkSet->Realloc(uiAlloced ? 2 * uiAlloced : 1);
            }
            Interaction* pkBase = pkSet->m_pBase;
            uint32_t uiUsed = pkSet->m_uiUsed;
            pkBase[uiUsed].m_uiBone = uiBone;
            pkBase[uiUsed].m_fWeight = fWeight;
            ++pkSet->m_uiUsed;
        }
    }

    for (uint32_t v = 0; v < uiVertices; ++v) {
        VertexInteractions* pkSet = &pkVertexInteraction[v];
        if (!pkSet->m_uiUsed) {
            // NiDelete[] of the interaction array: each set's buffer last to first, then the block.
            if (pkVertexInteraction) {
                for (int32_t j = static_cast<int32_t>(reinterpret_cast<size_t*>(pkVertexInteraction)[-1]) - 1; j >= 0;
                     --j) {
                    _NiFree(pkVertexInteraction[j].m_pBase);
                }
                MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(pkVertexInteraction) - 1);
            }
            return nullptr;
        }
        pkSet->Unitize();
        pkSet->Unitize();
    }
    return pkVertexInteraction;
}

// Drops each over-full triangle's lightest bone from its vertices and from the coincident vertices with the same bones
// until the triangle fits in ucBonesPerPartition bones, then keeps the ucBonesPerVertex heaviest weights of each
// vertex; false when a vertex is left without bones. Only the low byte of the two int32_t parameters is used.
bool NiSkinPartition::RemoveExcessInteractions(
    const NiTriBasedGeomData* pkTriData,
    int32_t ucBonesPerPartition,
    int32_t ucBonesPerVertex,
    NiSkinPartition::VertexInteractions* pkVertexInteraction
) {
    uint32_t uiTriangles = pkTriData->m_usTriangles;
    uint32_t uiVertices = pkTriData->m_usVertices;

    using NiTPointerMapUIntFloat = NiTPointerMap<uint32_t, float>;
    NiTPointerMapUIntFloat kMap(37);

    NiTPrimitiveSet<uint32_t> kCoincidentVerts;

    uint16_t* pusIndex0;
    uint16_t* pusIndex1;
    pkTriData->IndexSort(pusIndex0, pusIndex1, NiGeometryData::VERTEX_MASK);

    for (uint32_t t = 0; t < uiTriangles; t++) {
        uint16_t ausTriangle[3];
        pkTriData->GetTriangleIndices(t, ausTriangle[0], ausTriangle[1], ausTriangle[2]);
        if (ausTriangle[0] == ausTriangle[1] || ausTriangle[1] == ausTriangle[2] || ausTriangle[2] == ausTriangle[0]) {
            continue;
        }

        MakeBoneMap(kMap, ausTriangle, pkVertexInteraction);
        uint32_t uiBones = kMap.m_kAllocator.m_uiCount;
        uint8_t ucMaxBones = static_cast<uint8_t>(ucBonesPerPartition);
        if (uiBones > ucMaxBones) {
            while (true) {
                // The map holds uiBones bones, so the first position is read without testing it.
                NiTMapItem<uint32_t, float>* pos = kMap.GetFirstPos();
                float fMinWeight = 3.4028235e+38f;
                uint32_t uiMinBone = 0;
                do {
                    uint32_t uiBone;
                    float fWeight;
                    kMap.GetNext(pos, uiBone, fWeight);
                    if (fWeight < fMinWeight) {
                        fMinWeight = fWeight;
                        uiMinBone = uiBone;
                    }
                }
                while (pos);

                for (uint32_t v = 0; v < 3; v++) {
                    uint32_t uiVert = ausTriangle[v];

                    // kCoincidentVerts.RemoveAll(), then Add(uiVert).
                    kCoincidentVerts.m_uiUsed = 0;
                    if (kCoincidentVerts.m_uiUsed == kCoincidentVerts.m_uiAlloced) {
                        uint32_t uiNewSize = kCoincidentVerts.m_uiAlloced ? 2 * kCoincidentVerts.m_uiAlloced : 1;
                        if (uiNewSize != kCoincidentVerts.m_uiAlloced) {
                            uint32_t* puiNewBase = static_cast<uint32_t*>(_NiMalloc(sizeof(uint32_t) * uiNewSize));
                            _NiFree(kCoincidentVerts.m_pBase);
                            kCoincidentVerts.m_pBase = puiNewBase;
                            kCoincidentVerts.m_uiAlloced = uiNewSize;
                        }
                    }
                    kCoincidentVerts.m_pBase[kCoincidentVerts.m_uiUsed++] = uiVert;

                    uint32_t uiEnd = pusIndex1[uiVert];
                    NiSkinPartition::VertexInteractions* pkI = &pkVertexInteraction[uiVert];

                    for (uint32_t uiStart = uiEnd; uiStart > 0; uiStart--) {
                        uint32_t uiCoincidentVert = pusIndex0[uiStart - 1];
                        if (pkTriData->VertexCompare(uiCoincidentVert, uiVert, NiGeometryData::VERTEX_MASK)) {
                            break;
                        }

                        // EXE-INLINED none: NiSkinPartition::VertexInteractions::SameBones
                        const NiSkinPartition::VertexInteractions& kI = pkVertexInteraction[uiCoincidentVert];
                        uint32_t uiUsed = pkI->m_uiUsed;
                        if (uiUsed != kI.m_uiUsed) {
                            continue;
                        }
                        uint32_t b = 0;
                        while (b < uiUsed && pkI->m_pBase[b].m_uiBone == kI.m_pBase[b].m_uiBone) {
                            b++;
                        }
                        if (b < uiUsed) {
                            continue;
                        }

                        if (kCoincidentVerts.m_uiUsed == kCoincidentVerts.m_uiAlloced) {
                            uint32_t uiNewSize = kCoincidentVerts.m_uiAlloced ? 2 * kCoincidentVerts.m_uiAlloced : 1;
                            kCoincidentVerts.Realloc(uiNewSize);
                        }
                        kCoincidentVerts.m_pBase[kCoincidentVerts.m_uiUsed++] = uiCoincidentVert;
                    }

                    for (; uiEnd < uiVertices - 1; uiEnd++) {
                        uint32_t uiCoincidentVert = pusIndex0[uiEnd + 1];
                        if (pkTriData->VertexCompare(uiCoincidentVert, uiVert, NiGeometryData::VERTEX_MASK)) {
                            break;
                        }

                        // EXE-INLINED none: NiSkinPartition::VertexInteractions::SameBones
                        const NiSkinPartition::VertexInteractions& kI = pkVertexInteraction[uiCoincidentVert];
                        uint32_t uiUsed = pkI->m_uiUsed;
                        if (uiUsed != kI.m_uiUsed) {
                            continue;
                        }
                        uint32_t b = 0;
                        while (b < uiUsed && pkI->m_pBase[b].m_uiBone == kI.m_pBase[b].m_uiBone) {
                            b++;
                        }
                        if (b < uiUsed) {
                            continue;
                        }

                        if (kCoincidentVerts.m_uiUsed == kCoincidentVerts.m_uiAlloced) {
                            uint32_t uiNewSize = kCoincidentVerts.m_uiAlloced ? 2 * kCoincidentVerts.m_uiAlloced : 1;
                            kCoincidentVerts.Realloc(uiNewSize);
                        }
                        kCoincidentVerts.m_pBase[kCoincidentVerts.m_uiUsed++] = uiCoincidentVert;
                    }

                    for (uint32_t j = 0; j < kCoincidentVerts.m_uiUsed; j++) {
                        NiSkinPartition::VertexInteractions* pkJ = &pkVertexInteraction[kCoincidentVerts.m_pBase[j]];

                        // EXE-INLINED none: NiSkinPartition::VertexInteractions::RemoveBone
                        uint32_t uiUsed = pkJ->m_uiUsed;
                        uint32_t b = 0;
                        while (b < uiUsed && pkJ->m_pBase[b].m_uiBone != uiMinBone) {
                            b++;
                        }
                        if (b >= uiUsed) {
                            continue;
                        }
                        for (; b < pkJ->m_uiUsed - 1; b++) {
                            pkJ->m_pBase[b] = pkJ->m_pBase[b + 1];
                        }
                        if (--pkJ->m_uiUsed == 0) {
                            _NiFree(pusIndex0);
                            _NiFree(pusIndex1);
                            // kMap's destructor runs on return.
                            return false;
                        }
                        pkJ->Unitize();
                    }
                }

                if (--uiBones == ucMaxBones) {
                    break;
                }
                kMap.RemoveAll();
                MakeBoneMap(kMap, ausTriangle, pkVertexInteraction);
            }
        }
        kMap.RemoveAll();
    }

    uint8_t ucMaxVertexBones = static_cast<uint8_t>(ucBonesPerVertex);
    for (uint32_t v = 0; v < uiVertices; v++) {
        NiSkinPartition::VertexInteractions* pkI = &pkVertexInteraction[v];
        if (pkI->m_uiUsed > ucMaxVertexBones) {
            // EXE-INLINED none: NiSkinPartition::VertexInteractions::SortByWeight
            std::qsort(
                pkI->m_pBase, pkI->m_uiUsed, sizeof(NiSkinPartition::Interaction),
                &NiSkinPartition::VertexInteractions::CompareWeights
            );
            // EXE-INLINED none: NiSkinPartition::VertexInteractions::Truncate
            pkI->m_uiUsed = ucMaxVertexBones;
            pkI->Unitize();
        }
    }

    _NiFree(pusIndex0);
    _NiFree(pusIndex1);
    // kMap's destructor runs on return.
    return true;
}

// Builds the bone sets, merges them into sets of at most ucBonesPerPartition bones and gives each triangle its set.
// Without apWArray a triangle goes to the first set holding its bones (degenerate ones to -1) and the sets no triangle
// took are dropped. With it, a map from each set's weight value to its index gives a triangle the set of its vertices'
// common weight value, or of their smallest one (the largest above 100); a triangle whose three weight values all
// differ fails the call, deliberately leaving pbAssigned allocated as the game does.
bool NiSkinPartition::MakeBoneSets(
    const NiTriBasedGeomData* pkTriData,
    const NiSkinData* pkSkinData,
    int32_t ucBonesPerPartition,
    NiSkinPartition::VertexInteractions* pkVertexInteraction,
    NiSkinPartition::NiBoneSetCollection& kBoneSets,
    uint32_t*& puiTriangleToPartition,
    const int32_t* apWArray
) {
    uint32_t uiTriangles = pkTriData->m_usTriangles;
    CreateInitialBoneSets(pkTriData, pkVertexInteraction, kBoneSets, apWArray);
    MergeBoneSets(ucBonesPerPartition, kBoneSets);
    puiTriangleToPartition = static_cast<uint32_t*>(_NiMalloc(sizeof(uint32_t) * uiTriangles));
    bool* pbAssigned = static_cast<bool*>(_NiMalloc(kBoneSets.m_uiUsed));
    uint16_t ausTriangle[3];
    if (!apWArray) {
        for (uint32_t p = 0; p < kBoneSets.m_uiUsed; ++p) {
            pbAssigned[p] = false;
        }
        for (uint32_t t = 0; t < uiTriangles; ++t) {
            pkTriData->GetTriangleIndices(t, ausTriangle[0], ausTriangle[1], ausTriangle[2]);
            if (ausTriangle[0] == ausTriangle[1] || ausTriangle[1] == ausTriangle[2] ||
                ausTriangle[2] == ausTriangle[0]) {
                puiTriangleToPartition[t] = UINT32_MAX;
            }
            else {
                int32_t iPart =
                    NiSkinPartition::AssignTriangle(kBoneSets, ausTriangle, pkVertexInteraction, pbAssigned, true);
                if (iPart < 0) {
                    iPart =
                        NiSkinPartition::AssignTriangle(kBoneSets, ausTriangle, pkVertexInteraction, pbAssigned, false);
                }
                puiTriangleToPartition[t] = static_cast<uint32_t>(iPart);
            }
        }
        // Each set no triangle took is replaced by the last one, whose triangles are renumbered.
        uint32_t p = 0;
        while (p < kBoneSets.m_uiUsed) {
            if (pbAssigned[p]) {
                ++p;
                continue;
            }
            --kBoneSets.m_uiUsed;
            kBoneSets.m_pBase[p] = kBoneSets.m_pBase[kBoneSets.m_uiUsed];
            uint32_t uiMoved = kBoneSets.m_uiUsed;
            for (uint32_t i = 0; i < uiTriangles; ++i) {
                if (puiTriangleToPartition[i] == uiMoved) {
                    puiTriangleToPartition[i] = p;
                }
            }
        }
    }
    else {
        using WMap = NiTMap<int32_t, int32_t>;
        WMap kWMap(37);

        for (uint32_t p = 0; p < kBoneSets.m_uiUsed; ++p) {
            kWMap.SetAt_B57EA0(kBoneSets.m_pBase[p]->iWValue, static_cast<int32_t>(p));
        }

        bool bFailed = false;
        for (uint32_t t = 0; t < uiTriangles; ++t) {
            int32_t iBoneSetW = -1;
            pkTriData->GetTriangleIndices(t, ausTriangle[0], ausTriangle[1], ausTriangle[2]);
            int32_t iW0 = apWArray[ausTriangle[0]];
            int32_t iW1 = apWArray[ausTriangle[1]];
            if (iW0 == iW1 && iW1 == apWArray[ausTriangle[2]] && iW0 == apWArray[ausTriangle[2]]) {
                kWMap.GetAt_A53C30(iW0, iBoneSetW);
                if (iBoneSetW != -1) {
                    puiTriangleToPartition[t] = static_cast<uint32_t>(iBoneSetW);
                }
            }
            else {
                int32_t iW2 = apWArray[ausTriangle[2]];
                int32_t iWSmall = iW1 < iW2 ? iW1 : iW2;
                iWSmall = iW0 < iWSmall ? iW0 : iWSmall;
                int32_t iWLarge = iW1 > iW2 ? iW1 : iW2;
                iWLarge = iW0 > iWLarge ? iW0 : iWLarge;
                if (iWLarge > 100) {
                    iWSmall = iWLarge;
                }
                kWMap.GetAt_A53C30(iWSmall, iBoneSetW);
                if (iBoneSetW != -1) {
                    puiTriangleToPartition[t] = static_cast<uint32_t>(iBoneSetW);
                }
            }
            int32_t iCheck1 = apWArray[ausTriangle[1]];
            int32_t iCheck0 = apWArray[ausTriangle[0]];
            if (iCheck0 != iCheck1) {
                int32_t iCheck2 = apWArray[ausTriangle[2]];
                if (iCheck1 != iCheck2 && iCheck0 != iCheck2) {
                    bFailed = true;
                    break;
                }
            }
        }

        if (bFailed) {
            return false;
        }
    }
    m_uiPartitions = kBoneSets.m_uiUsed;
    _NiFree(pbAssigned);
    return true;
}

void NiSkinPartition::MakeBoneMap(
    NiTPointerMap<uint32_t, float>& kMap,
    const uint16_t* pusTriangle,
    const NiSkinPartition::VertexInteractions* pkVertexInteraction
) {
    uint32_t v4 = 0;
    uint32_t v10 = 0;
    do {
        const NiSkinPartition::VertexInteractions* v6 = &pkVertexInteraction[pusTriangle[v4]];
        int32_t v7 = 0;
        const NiSkinPartition::VertexInteractions* kMapa = v6;
        uint32_t m_uiUsed = v6->m_uiUsed;
        if (m_uiUsed) {
            while (true) {
                uint32_t uiBone = v6->m_pBase[v7].m_uiBone;
                NiTMapItem<uint32_t, float>* v9 = kMap.m_ppkHashTable[kMap.KeyToHashIndex(uiBone)];
                if (!v9) {
                    goto LABEL_10;
                }
                while (!kMap.IsKeysEqual(uiBone, v9->m_key)) {
                    v9 = v9->m_pkNext;
                    if (!v9) {
                        goto LABEL_10;
                    }
                }
                if (kMapa->m_pBase[v7].m_fWeight > static_cast<double>(v9->m_val)) {
                LABEL_10:;
                    kMap.SetAt(uiBone, kMapa->m_pBase[v7].m_fWeight);
                }
                if (++v7 >= m_uiUsed) {
                    break;
                }
                v6 = kMapa;
            }
            v4 = v10;
        }
        v10 = ++v4;
    }
    while (v4 < 3);
}

// One bone set per triangle. Without apWArray: the bones of its three vertices, dropped when a set already holds
// them and replacing the sets it holds. With it: for each of the three vertices' weight values, the bones are added to
// every set of that weight value, or to a new set when there is none.
void NiSkinPartition::CreateInitialBoneSets(
    const NiTriBasedGeomData* pkTriData,
    NiSkinPartition::VertexInteractions* pkVertexInteraction,
    NiTPrimitiveSet<NiSkinPartition::BoneSet*>& kBoneSets,
    const int32_t* apWArray
) {
    uint32_t v8 = 0;
    uint16_t ausTriangle[3];
    const NiTriBasedGeomData* v5 = pkTriData;
    const int32_t* v6 = apWArray;
    NiTPrimitiveSet<NiSkinPartition::BoneSet*>* v7 = &kBoneSets;
    uint32_t m_usTriangles = pkTriData->m_usTriangles;
    if (!apWArray) {
        int32_t v63 = 0;
        if (!pkTriData->m_usTriangles) {
            return;
        }
        while (true) {
            v5->GetTriangleIndices(v63, *ausTriangle, ausTriangle[1], ausTriangle[2]);
            if (ausTriangle[0] != ausTriangle[1] &&
                (ausTriangle[1] != ausTriangle[2] && ausTriangle[2] != ausTriangle[0])) {
                NiSkinPartition::BoneSet* v46;
                NiSkinPartition::BoneSet* v45 = static_cast<NiSkinPartition::BoneSet*>(
                    MemoryManager_Singleton.Allocate(sizeof(NiSkinPartition::BoneSet))
                );
                if (v45) {
                    v45->m_pBase = nullptr;
                    v45->m_uiAlloced = 0;
                    v45->m_uiUsed = 0;
                    v46 = v45;
                }
                else {
                    v46 = nullptr;
                }
                for (uint32_t i = 0; i < 3; ++i) {
                    NiSkinPartition::VertexInteractions* v47 = &pkVertexInteraction[ausTriangle[i]];
                    uint32_t v48 = 0;
                    uint32_t m_uiUsed = v47->m_uiUsed;
                    if (m_uiUsed) {
                        do {
                            uint16_t* m_pBase;
                            uint16_t m_uiBone = v47->m_pBase[v48].m_uiBone;
                            uint32_t v50 = v46->m_uiUsed;
                            uint32_t v51 = 0;
                            if (!v50) {
                                goto LABEL_88;
                            }
                            m_pBase = v46->m_pBase;
                            while (*m_pBase != m_uiBone) {
                                ++v51;
                                ++m_pBase;
                                if (v51 >= v50) {
                                    goto LABEL_88;
                                }
                            }
                            if (v51 == -1) {
                            LABEL_88:;
                                uint32_t m_uiAlloced = v46->m_uiAlloced;
                                if (v50 == m_uiAlloced) {
                                    uint32_t v54;
                                    if (m_uiAlloced) {
                                        v54 = 2 * m_uiAlloced;
                                    }
                                    else {
                                        v54 = 1;
                                    }
                                    v46->Realloc(v54);
                                }
                                v46->m_pBase[v46->m_uiUsed++] = m_uiBone;
                            }
                            ++v48;
                        }
                        while (v48 < m_uiUsed);
                    }
                }
                std::qsort(v46->m_pBase, v46->m_uiUsed, sizeof(uint16_t), &NiSkinPartition::BoneSet::CompareBones);
                uint32_t v55 = 0;
                if (kBoneSets.m_uiUsed) {
                    while (true) {
                        NiSkinPartition::BoneSet* v56 = kBoneSets.m_pBase[v55];
                        if (v56->Contains(*v46)) {
                            break;
                        }
                        if (v46->Contains(*v56)) {
                            if (v56) {
                                _NiFree(v56->m_pBase);
                                MemoryManager_Singleton.Deallocate(v56);
                            }
                            kBoneSets.m_pBase[v55] = kBoneSets.m_pBase[--kBoneSets.m_uiUsed];
                        }
                        else {
                            ++v55;
                        }
                        if (v55 >= kBoneSets.m_uiUsed) {
                            goto LABEL_106;
                        }
                    }
                    _NiFree(v46->m_pBase);
                    MemoryManager_Singleton.Deallocate(v46);
                }
            LABEL_106:;
                uint32_t v57 = kBoneSets.m_uiUsed;
                if (v55 == v57) {
                    uint32_t v58 = kBoneSets.m_uiAlloced;
                    if (v57 == v58) {
                        uint32_t v59;
                        if (v58) {
                            v59 = 2 * v58;
                        }
                        else {
                            v59 = 1;
                        }
                        kBoneSets.Realloc(v59);
                    }
                    kBoneSets.m_pBase[kBoneSets.m_uiUsed++] = v46;
                }
            }
            if (++v63 >= m_usTriangles) {
                return;
            }
            v5 = pkTriData;
        }
    }
    uint32_t v74 = 0;
    if (!pkTriData->m_usTriangles) {
        return;
    }
    while (true) {
        uint32_t v75[3];
        v5->GetTriangleIndices(v8, *ausTriangle, ausTriangle[1], ausTriangle[2]);
        int32_t v9 = v6[ausTriangle[1]];
        v75[0] = v6[ausTriangle[0]];
        int32_t v10 = v6[ausTriangle[2]];
        uint32_t v11 = 0;
        v75[1] = v9;
        v75[2] = v10;
        uint32_t v64 = 0;
        do {
            bool v25;
            int32_t v72;
            uint32_t v12 = 0;
            char v60 = 0;
            uint32_t v69 = 0;
            if (!v7->m_uiUsed) {
                goto LABEL_34;
            }
            v72 = v75[v11];
            do {
                NiSkinPartition::BoneSet* v13 = v7->m_pBase[v12];
                if (v13->iWValue == v72) {
                    uint16_t* v14 = ausTriangle;
                    v60 = 1;
                    uint16_t* v61 = ausTriangle;
                    int32_t v73 = 3;
                    do {
                        uint32_t v15 = 0;
                        NiSkinPartition::VertexInteractions* v16 = &pkVertexInteraction[*v14];
                        if (v16->m_uiUsed) {
                            do {
                                uint16_t* v20;
                                uint16_t v17 = v16->m_pBase[v15].m_uiBone;
                                uint32_t v18 = v13->m_uiUsed;
                                uint32_t v19 = 0;
                                uint16_t v66 = v17;
                                if (!v18) {
                                    goto LABEL_17;
                                }
                                v20 = v13->m_pBase;
                                while (*v20 != v17) {
                                    ++v19;
                                    ++v20;
                                    if (v19 >= v18) {
                                        goto LABEL_17;
                                    }
                                }
                                if (v19 == -1) {
                                LABEL_17:;
                                    uint32_t v21 = v13->m_uiAlloced;
                                    if (v18 == v21) {
                                        uint32_t v22 = 2 * v21;
                                        if (!v21) {
                                            v22 = 1;
                                        }
                                        if (v22 != v21) {
                                            uint16_t* v23;
                                            if (v22) {
                                                v23 = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * v22));
                                                for (uint32_t j = 0; j < v13->m_uiUsed; ++j) {
                                                    v23[j] = v13->m_pBase[j];
                                                }
                                            }
                                            else {
                                                v23 = nullptr;
                                            }
                                            _NiFree(v13->m_pBase);
                                            v13->m_pBase = v23;
                                            v17 = v66;
                                            v13->m_uiAlloced = v22;
                                        }
                                    }
                                    v14 = v61;
                                    v13->m_pBase[v13->m_uiUsed++] = v17;
                                }
                                ++v15;
                                v16 = &pkVertexInteraction[*v14];
                            }
                            while (v15 < v16->m_uiUsed);
                        }
                        ++v14;
                        v25 = v73-- == 1;
                        v61 = v14;
                    }
                    while (!v25);
                    std::qsort(v13->m_pBase, v13->m_uiUsed, sizeof(uint16_t), &NiSkinPartition::BoneSet::CompareBones);
                    v12 = v69;
                    v7 = &kBoneSets;
                }
                v69 = ++v12;
            }
            while (v12 < v7->m_uiUsed);
            if (!v60) {
                v11 = v64;
            LABEL_34:;
                NiSkinPartition::BoneSet* v26 = static_cast<NiSkinPartition::BoneSet*>(
                    MemoryManager_Singleton.Allocate(sizeof(NiSkinPartition::BoneSet))
                );
                NiSkinPartition::BoneSet* v27 = nullptr;
                if (v26) {
                    v26->m_pBase = nullptr;
                    v26->m_uiAlloced = 0;
                    v26->m_uiUsed = 0;
                    v27 = v26;
                }
                v27->iWValue = v75[v11];
                uint16_t* v62 = ausTriangle;
                int32_t v70 = 3;
                uint16_t* v28 = ausTriangle;
                do {
                    uint32_t v29 = 0;
                    NiSkinPartition::VertexInteractions* v30 = &pkVertexInteraction[*v28];
                    if (v30->m_uiUsed) {
                        do {
                            uint16_t* v34;
                            uint16_t v31 = v30->m_pBase[v29].m_uiBone;
                            uint32_t v32 = v27->m_uiUsed;
                            uint32_t v33 = 0;
                            uint16_t v67 = v31;
                            if (!v32) {
                                goto LABEL_44;
                            }
                            v34 = v27->m_pBase;
                            while (*v34 != v31) {
                                ++v33;
                                ++v34;
                                if (v33 >= v32) {
                                    goto LABEL_44;
                                }
                            }
                            if (v33 == -1) {
                            LABEL_44:;
                                uint32_t v35 = v27->m_uiAlloced;
                                if (v32 == v35) {
                                    uint32_t v36 = 2 * v35;
                                    if (!v35) {
                                        v36 = 1;
                                    }
                                    if (v36 != v35) {
                                        uint16_t* v37;
                                        if (v36) {
                                            v37 = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * v36));
                                            for (uint32_t k = 0; k < v27->m_uiUsed; ++k) {
                                                v37[k] = v27->m_pBase[k];
                                            }
                                        }
                                        else {
                                            v37 = nullptr;
                                        }
                                        _NiFree(v27->m_pBase);
                                        v27->m_pBase = v37;
                                        v31 = v67;
                                        v27->m_uiAlloced = v36;
                                    }
                                }
                                v28 = v62;
                                v27->m_pBase[v27->m_uiUsed++] = v31;
                            }
                            ++v29;
                            v30 = &pkVertexInteraction[*v28];
                        }
                        while (v29 < v30->m_uiUsed);
                    }
                    ++v28;
                    v25 = v70-- == 1;
                    v62 = v28;
                }
                while (!v25);
                std::qsort(v27->m_pBase, v27->m_uiUsed, sizeof(uint16_t), &NiSkinPartition::BoneSet::CompareBones);
                uint32_t v40 = kBoneSets.m_uiAlloced;
                if (kBoneSets.m_uiUsed == v40) {
                    uint32_t v41 = 2 * v40;
                    if (!v40) {
                        v41 = 1;
                    }
                    kBoneSets.Realloc(v41);
                }
                v7 = &kBoneSets;
                kBoneSets.m_pBase[kBoneSets.m_uiUsed++] = v27;
            }
            v11 = v64 + 1;
            v64 = v11;
        }
        while (v11 < 3);
        v5 = pkTriData;
        v8 = v74 + 1;
        v74 = v8;
        if (v8 < pkTriData->m_usTriangles) {
            v6 = apWArray;
            continue;
        }
        break;
    }
}

NiObject* NiSkinPartition::CreateObject() {
    NiSkinPartition* pkObject = static_cast<NiSkinPartition*>(NiMemObject::operator new(sizeof(NiSkinPartition)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiSkinPartition;
}

int32_t NiSkinPartition::AssignTriangle(
    NiTPrimitiveSet<NiSkinPartition::BoneSet*>& kBoneSets,
    const uint16_t* pusTriangle,
    NiSkinPartition::VertexInteractions* pkVertexInteraction,
    bool* pbAssigned,
    bool bAssign
) {
    uint32_t p = 0;
    if (!kBoneSets.m_uiUsed) {
        return -1;
    }
    while (pbAssigned[p] != bAssign || !kBoneSets.m_pBase[p]->ContainsTriangle(pusTriangle, pkVertexInteraction)) {
        if (++p >= kBoneSets.m_uiUsed) {
            return -1;
        }
    }
    pbAssigned[p] = true;
    return p;
}
