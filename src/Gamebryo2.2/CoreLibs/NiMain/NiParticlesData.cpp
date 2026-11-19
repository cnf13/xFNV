#include "Gamebryo2.2/CoreLibs/NiMain/NiParticlesData.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleMeshesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <cstring>

NiRTTI NiParticlesData::ms_RTTI("NiParticlesData", &NiGeometryData::ms_RTTI);

NiParticlesData::NiParticlesData()
  : m_bIsOldRotatingParticlesObject(false), m_pfRadii(nullptr), m_usActive(0), m_pfSizes(nullptr),
    m_pkRotations(nullptr), m_pfRotationAngles(nullptr), m_pkRotationAxes(nullptr), m_pucTextureIndices(nullptr),
    m_pkSubTextureOffsets(nullptr), m_ucSubTextureOffsetCount(0) {
    SetConsistency(NiGeometryData::MUTABLE);
    m_bSaveVertexData = false;
}

NiParticlesData::~NiParticlesData() {
    _NiFree(m_pfRadii);
    _NiFree(m_pfSizes);
    (&MemoryManager_Singleton)->Deallocate(m_pkRotations);
    _NiFree(m_pfRotationAngles);
    (&MemoryManager_Singleton)->Deallocate(m_pkRotationAxes);
    if (m_ausRenderOrder) {
        (&MemoryManager_Singleton)->Deallocate(m_ausRenderOrder);
    }
    if (m_afDepths) {
        (&MemoryManager_Singleton)->Deallocate(m_afDepths);
    }
    if (m_pucTextureIndices) {
        _NiFree(m_pucTextureIndices);
    }
    if (m_pkSubTextureOffsets) {
        _NiFree(m_pkSubTextureOffsets);
    }
}

// Every per-particle array is allocated for m_usVertices. Older files store one radius for all particles and pointer
// values as "has" flags; Bethesda's files (stream version 0x13 and up) keep only the flags, so the arrays get their
// defaults. Rotations of files up to 10.0.1.0 are dropped unless this is a NiParticleMeshesData. The radii, rotations,
// rotation-angles, rotation-axes, texture-index and sizes flags are read into uninitialised locals; a short read leaves
// them unwritten and the body branches on them, as the game does.
void NiParticlesData::LoadBinary(NiStream& kStream) {
    NiGeometryData::LoadBinary(kStream);

    if (kStream.GetFileVersion() < 0x04010007) {
        uint16_t usDiscard;
        NiStreamLoadBinary(kStream, usDiscard);
    }

    m_pfRadii = static_cast<float*>(_NiMalloc(m_usVertices * sizeof(float)));
    if (kStream.GetFileVersion() < 0x0A00010D) {
        float fRadius;
        NiStreamLoadBinary(kStream, fRadius);
        for (uint16_t i = 0; i < m_usVertices; i++) {
            m_pfRadii[i] = fRadius;
        }
    }
    else {
        bool bHasRadii;
        NiStreamLoadBinary(kStream, bHasRadii);
        if (bHasRadii && kStream.m_BSStreamHeader.uiVersion < 0x13) {
            NiStreamLoadBinary(kStream, m_pfRadii, m_usVertices);
        }
        else {
            for (uint16_t i = 0; i < m_usVertices; i++) {
                m_pfRadii[i] = 1.0f;
            }
        }
    }

    NiStreamLoadBinary(kStream, m_usActive);

    bool bHasSizes;
    if (kStream.GetFileVersion() < 0x04010007) {
        NiStreamLoadBinary(kStream, m_pfSizes);
        bHasSizes = m_pfSizes != nullptr;
    }
    else {
        NiStreamLoadBinary(kStream, bHasSizes);
    }
    m_pfSizes = static_cast<float*>(_NiMalloc(m_usVertices * sizeof(float)));
    if (bHasSizes && kStream.m_BSStreamHeader.uiVersion < 0x13) {
        NiStreamLoadBinary(kStream, m_pfSizes, m_usVertices);
    }
    else {
        for (uint16_t i = 0; i < m_usVertices; i++) {
            m_pfSizes[i] = 1.0f;
        }
    }

    bool bHasRotations;
    if (m_bIsOldRotatingParticlesObject) {
        if (kStream.GetFileVersion() < 0x04010000) {
            NiStreamLoadBinary(kStream, m_pkRotations);
            bHasRotations = m_pkRotations != nullptr;
        }
        else {
            NiStreamLoadBinary(kStream, bHasRotations);
        }
        // new NiQuaternion[m_usVertices]: Bethesda's operator new[], no constructors run
        m_pkRotations =
            static_cast<NiQuaternion*>((&MemoryManager_Singleton)->Allocate(m_usVertices * sizeof(NiQuaternion)));
        if (bHasRotations) {
            for (uint32_t i = 0; i < m_usVertices; i++) {
                m_pkRotations[i].LoadBinary(kStream);
            }
        }
        else {
            for (uint32_t i = 0; i < m_usVertices; i++) {
                m_pkRotations[i] = NiQuaternion_IDENTITY;
            }
        }
    }
    else if (kStream.GetFileVersion() >= 0x05000008) {
        NiStreamLoadBinary(kStream, bHasRotations);
        if (bHasRotations) {
            m_pkRotations =
                static_cast<NiQuaternion*>((&MemoryManager_Singleton)->Allocate(m_usVertices * sizeof(NiQuaternion)));
            if (kStream.m_BSStreamHeader.uiVersion < 0x13) {
                for (uint16_t i = 0; i < m_usVertices; i++) {
                    m_pkRotations[i].LoadBinary(kStream);
                }
            }
            if (kStream.GetFileVersion() <= 0x0A000100 && !IsKindOf(&NiParticleMeshesData::ms_RTTI)) {
                (&MemoryManager_Singleton)->Deallocate(m_pkRotations);
                m_pkRotations = nullptr;
            }
        }
    }

    if (kStream.GetFileVersion() >= 0x0A030005) {
        bool bHasRotationAngles;
        NiStreamLoadBinary(kStream, bHasRotationAngles);
        if (bHasRotationAngles) {
            m_pfRotationAngles = static_cast<float*>(_NiMalloc(m_usVertices * sizeof(float)));
            if (kStream.m_BSStreamHeader.uiVersion < 0x13) {
                for (uint16_t i = 0; i < m_usVertices; i++) {
                    NiStreamLoadBinary(kStream, m_pfRotationAngles[i]);
                }
            }
        }

        bool bHasRotationAxes;
        NiStreamLoadBinary(kStream, bHasRotationAxes);
        if (bHasRotationAxes) {
            // new NiPoint3[m_usVertices]: Bethesda's operator new[], no constructors run
            m_pkRotationAxes =
                static_cast<NiPoint3*>((&MemoryManager_Singleton)->Allocate(m_usVertices * sizeof(NiPoint3)));
            if (kStream.m_BSStreamHeader.uiVersion < 0x13) {
                for (uint16_t i = 0; i < m_usVertices; i++) {
                    m_pkRotationAxes[i].LoadBinary(kStream);
                }
            }
        }
    }

    if (kStream.m_BSStreamHeader.uiVersion > 0x11) {
        // Only the flag is stored; the indices start at zero.
        bool bHasTextureIndices;
        NiStreamLoadBinary(kStream, bHasTextureIndices);
        if (bHasTextureIndices) {
            m_pucTextureIndices = static_cast<uint8_t*>(_NiMalloc(m_usVertices));
            memset(m_pucTextureIndices, 0, m_usVertices);
        }

        NiStreamLoadBinary(kStream, m_ucSubTextureOffsetCount);
        if (m_ucSubTextureOffsetCount > 0) {
            m_pkSubTextureOffsets = static_cast<NiColorA*>(_NiMalloc(m_ucSubTextureOffsetCount * sizeof(NiColorA)));
            for (uint16_t i = 0; i < m_ucSubTextureOffsetCount; i++) {
                m_pkSubTextureOffsets[i].LoadBinary(kStream);
            }
        }
    }
}

void NiParticlesData::LinkObject(NiStream& kStream) {
    NiGeometryData::LinkObject(kStream);
}

bool NiParticlesData::RegisterStreamables(NiStream& kStream) {
    return NiGeometryData::RegisterStreamables(kStream);
}

// Only the "has" flags of the per-particle arrays are saved, then the sub-texture offsets.
void NiParticlesData::SaveBinary(NiStream& kStream) {
    NiGeometryData::SaveBinary(kStream);

    bool bHasRadii = m_pfRadii != nullptr;
    NiStreamSaveBinary(kStream, bHasRadii);
    NiStreamSaveBinary(kStream, m_usActive);
    bool bHasSizes = m_pfSizes != nullptr;
    NiStreamSaveBinary(kStream, bHasSizes);
    bool bHasRotations = m_pkRotations != nullptr;
    NiStreamSaveBinary(kStream, bHasRotations);
    bool bHasRotationAngles = m_pfRotationAngles != nullptr;
    NiStreamSaveBinary(kStream, bHasRotationAngles);
    bool bHasRotationAxes = m_pkRotationAxes != nullptr;
    NiStreamSaveBinary(kStream, bHasRotationAxes);
    bool bHasTextureIndices = m_pucTextureIndices != nullptr;
    NiStreamSaveBinary(kStream, bHasTextureIndices);

    NiStreamSaveBinary(kStream, m_ucSubTextureOffsetCount);
    for (uint32_t i = 0; i < m_ucSubTextureOffsetCount; i++) {
        m_pkSubTextureOffsets[i].SaveBinary(kStream);
    }
}

// Radii, active count and sizes must agree in presence and value (radii over all vertices, sizes over the active
// particles); rotations must agree in presence. The remaining arrays are compared whenever this has them, against
// the other's without a null test, and the sub-texture offsets over this object's count.
bool NiParticlesData::IsEqual(NiObject* pkObject) {
    if (!NiGeometryData::IsEqual(pkObject)) {
        return false;
    }

    NiParticlesData* pkData = static_cast<NiParticlesData*>(pkObject);

    if ((m_pfRadii && !pkData->m_pfRadii) || (!m_pfRadii && pkData->m_pfRadii)) {
        return false;
    }
    if (m_pfRadii) {
        for (uint16_t i = 0; i < m_usVertices; i++) {
            if (m_pfRadii[i] != pkData->m_pfRadii[i]) {
                return false;
            }
        }
    }

    if (m_usActive != pkData->m_usActive) {
        return false;
    }

    if ((m_pfSizes && !pkData->m_pfSizes) || (!m_pfSizes && pkData->m_pfSizes)) {
        return false;
    }
    if (m_pfSizes) {
        for (uint16_t i = 0; i < m_usActive; i++) {
            if (m_pfSizes[i] != pkData->m_pfSizes[i]) {
                return false;
            }
        }
    }

    if ((m_pkRotations && !pkData->m_pkRotations) || (!m_pkRotations && pkData->m_pkRotations)) {
        return false;
    }
    if (m_pkRotations) {
        for (uint32_t i = 0; i < m_usActive; i++) {
            const NiQuaternion& kRotation = m_pkRotations[i];
            const NiQuaternion& kOther = pkData->m_pkRotations[i];
            if (kRotation.m_fW != kOther.m_fW || kRotation.m_fX != kOther.m_fX || kRotation.m_fY != kOther.m_fY ||
                kRotation.m_fZ != kOther.m_fZ) {
                return false;
            }
        }
    }

    if (m_pfRotationAngles) {
        for (uint32_t i = 0; i < m_usActive; i++) {
            if (m_pfRotationAngles[i] != pkData->m_pfRotationAngles[i]) {
                return false;
            }
        }
    }

    if (m_pkRotationAxes) {
        for (uint32_t i = 0; i < m_usActive; i++) {
            const NiPoint3& kAxis = m_pkRotationAxes[i];
            const NiPoint3& kOther = pkData->m_pkRotationAxes[i];
            if (kAxis.x != kOther.x || kAxis.y != kOther.y || kAxis.z != kOther.z) {
                return false;
            }
        }
    }

    if (m_pucTextureIndices) {
        for (uint32_t i = 0; i < m_usActive; i++) {
            if (m_pucTextureIndices[i] != pkData->m_pucTextureIndices[i]) {
                return false;
            }
        }
    }

    if (m_pkSubTextureOffsets) {
        for (uint32_t i = 0; i < m_ucSubTextureOffsetCount; i++) {
            const NiColorA& kOffset = m_pkSubTextureOffsets[i];
            const NiColorA& kOther = pkData->m_pkSubTextureOffsets[i];
            if (kOffset.r != kOther.r || kOffset.g != kOther.g || kOffset.b != kOther.b || kOffset.a != kOther.a) {
                return false;
            }
        }
    }

    return true;
}

void NiParticlesData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiGeometryData::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_pfRadii", static_cast<const void*>(m_pfRadii)));
    pkStrings->Add(NiGetViewerString("m_usActive", m_usActive));
    pkStrings->Add(NiGetViewerString("m_pfSizes", static_cast<const void*>(m_pfSizes)));
    pkStrings->Add(NiGetViewerString("m_pkRotations", static_cast<const void*>(m_pkRotations)));
    pkStrings->Add(NiGetViewerString("m_pfRotationAngles", static_cast<const void*>(m_pfRotationAngles)));
    pkStrings->Add(NiGetViewerString("m_pkRotationAxes", static_cast<const void*>(m_pkRotationAxes)));
    pkStrings->Add(NiGetViewerString("m_pucTextureIndices", static_cast<const void*>(m_pucTextureIndices)));
    pkStrings->Add(NiGetViewerString("m_ucSubTextureOffsetCount", m_ucSubTextureOffsetCount));
    pkStrings->Add(NiGetViewerString("m_pkSubTextureOffsets", static_cast<const void*>(m_pkSubTextureOffsets)));
}

// The active count is clamped to the vertex count.
void NiParticlesData::SetActiveVertexCount(uint16_t usActive) {
    m_usActive = (usActive > m_usVertices) ? m_usVertices : usActive;
}

// The last active particle moves into the removed one's place in every array this data has (vertices always),
// then the active count drops by one.
void NiParticlesData::RemoveParticle(int32_t aiParticle) {
    uint16_t usParticle = static_cast<uint16_t>(aiParticle);
    uint16_t usLast = m_usActive - 1;
    if (usParticle != usLast) {
        m_pkVertex[usParticle] = m_pkVertex[usLast];
        if (m_pkColor) {
            m_pkColor[usParticle] = m_pkColor[usLast];
        }
        if (m_pkNormal) {
            m_pkNormal[usParticle] = m_pkNormal[usLast];
        }
        if (m_pfRadii) {
            m_pfRadii[usParticle] = m_pfRadii[usLast];
        }
        if (m_pfSizes) {
            m_pfSizes[usParticle] = m_pfSizes[usLast];
        }
        if (m_pkRotations) {
            m_pkRotations[usParticle] = m_pkRotations[usLast];
        }
        if (m_pfRotationAngles) {
            m_pfRotationAngles[usParticle] = m_pfRotationAngles[usLast];
        }
        if (m_pkRotationAxes) {
            m_pkRotationAxes[usParticle] = m_pkRotationAxes[usLast];
        }
        if (m_pucTextureIndices) {
            m_pucTextureIndices[usParticle] = m_pucTextureIndices[usLast];
        }
    }
    m_usActive--;
}

void NiParticlesData::ReplaceRadii(float* pfRadii) {
    _NiFree(m_pfRadii);
    m_pfRadii = pfRadii;
}

void NiParticlesData::ReplaceSizes(float* pfSizes) {
    _NiFree(m_pfSizes);
    m_pfSizes = pfSizes;
}

void NiParticlesData::ReplaceRotationAngles(float* pfRotationAngles) {
    _NiFree(m_pfRotationAngles);
    m_pfRotationAngles = pfRotationAngles;
}

void NiParticlesData::ReplaceRotations(NiQuaternion* pkRotations) {
    (&MemoryManager_Singleton)->Deallocate(m_pkRotations);
    m_pkRotations = pkRotations;
}

void NiParticlesData::ReplaceSubTextureOffsets(NiColorA* pkSubTextureOffsets, uint8_t ucSubTextureOffsetCount) {
    uint8_t ucClamped =
        (ucSubTextureOffsetCount < MAX_SUBTEXTURE_COUNT) ? ucSubTextureOffsetCount : MAX_SUBTEXTURE_COUNT;
    _NiFree(m_pkSubTextureOffsets);
    m_ucSubTextureOffsetCount = ucClamped;
    m_pkSubTextureOffsets = pkSubTextureOffsets;
}

void NiParticlesData::ReplaceTextureIndices(uint8_t* pucTextureIndices) {
    _NiFree(m_pucTextureIndices);
    m_pucTextureIndices = pucTextureIndices;
}

void NiParticlesData::ReplaceRotationAxes(NiPoint3* pkRotationAxes) {
    (&MemoryManager_Singleton)->Deallocate(m_pkRotationAxes);
    m_pkRotationAxes = pkRotationAxes;
}

NiObject* NiParticlesData::CreateObject() {
    void* v0 = NiMemObject::operator new(sizeof(NiParticlesData));
    if (!v0) {
        return nullptr;
    }
    return ::new (v0) NiParticlesData;
}

// The loader of "NiRotatingParticlesData": a NiParticlesData flagged as an old rotating-particles object. The flag is
// set after the null test, so a failed allocation writes through null, deliberately, as the game does.
NiObject* NiParticlesData::CreateObject_00A96660() {
    NiParticlesData* pData = static_cast<NiParticlesData*>(NiMemObject::operator new(sizeof(NiParticlesData)));
    if (pData) {
        // EXE-INLINED 0xA963F0
        ::new (pData) NiParticlesData;
    }
    pData->m_bIsOldRotatingParticlesObject = true;
    return pData;
}
