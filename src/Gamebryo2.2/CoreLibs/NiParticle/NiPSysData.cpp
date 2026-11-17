#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticlesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

#include <cstring>

static const uint32_t data_10BE24C = 0;
NiRTTI NiPSysData::ms_RTTI("NiPSysData", &NiParticlesData::ms_RTTI);

NiPSysData::NiPSysData()
  : m_pkParticleInfo(nullptr), m_pfRotationSpeeds(nullptr), m_usNumAddedParticles(0), m_usAddedParticlesBase(0) {}

// The particle info array and the rotation speeds are freed; NiParticlesData's destructor runs after.
NiPSysData::~NiPSysData() {
    MemoryManager_Singleton.Deallocate(m_pkParticleInfo);
    _NiFree(m_pfRotationSpeeds);
}

NiObject* NiPSysData::CreateClone(NiCloningProcess& kCloning) {
    NiPSysData* pkObject = static_cast<NiPSysData*>(NiMemObject::operator new(sizeof(NiPSysData)));
    if (pkObject) {
        ::new (pkObject) NiPSysData;
        CopyMembers(pkObject, kCloning);
        return pkObject;
    }
    else {
        CopyMembers(0, kCloning);
        return 0;
    }
}

// Files before 10.3.0.5 store rotations as quaternions: they become angle/axis pairs. Every particle gets a
// NiParticleInfo; Bethesda's files (stream version 0x13 and up) store neither the infos nor the speeds nor the
// added-particle counts.
// KEEP: the exe reads the rotation-speeds flag into the frame slot that holds the new info array's address, so a
// short read (malformed data) leaves that address's low byte there and the exe tests it; src cannot express it.
// porting.md's unwritten-slot exception covers GetChunkData only.
void NiPSysData::LoadBinary(NiStream& kStream) {
    NiParticlesData::LoadBinary(kStream);

    if (kStream.GetFileVersion() < 0x0A030005 && m_pkRotations) {
        m_pfRotationAngles = static_cast<float*>(_NiMalloc(m_usVertices * sizeof(float)));
        // new NiPoint3[m_usVertices]: Bethesda's operator new[] on the heap singleton, no constructors run
        m_pkRotationAxes = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(m_usVertices * sizeof(NiPoint3)));
        for (uint16_t i = 0; i < m_usVertices; i++) {
            m_pkRotations[i].ToAngleAxis(m_pfRotationAngles[i], m_pkRotationAxes[i]);
        }
        ReplaceRotations(nullptr);
    }

    // new NiParticleInfo[m_usVertices] on the engine heap, with no count header.
    uint16_t usVertices = m_usVertices;
    NiParticleInfo* pkInfo =
        static_cast<NiParticleInfo*>(MemoryManager_Singleton.Allocate(usVertices * sizeof(NiParticleInfo)));
    if (pkInfo) {
        for (uint32_t i = 0; i < usVertices; i++) {
            new (&pkInfo[i]) NiParticleInfo;
        }
    }
    m_pkParticleInfo = pkInfo;

    if (kStream.m_BSStreamHeader.uiVersion < 0x13) {
        for (uint16_t i = 0; i < m_usVertices; i++) {
            m_pkParticleInfo[i].LoadBinary(kStream);
        }
    }

    if (kStream.GetFileVersion() >= 0x14000002) {
        bool bHasRotationSpeeds;
        NiStreamLoadBinary(kStream, bHasRotationSpeeds);
        if (bHasRotationSpeeds) {
            m_pfRotationSpeeds = static_cast<float*>(_NiMalloc(m_usVertices * sizeof(float)));
            if (kStream.m_BSStreamHeader.uiVersion < 0x13) {
                NiStreamLoadBinary(kStream, m_pfRotationSpeeds, m_usVertices);
            }
        }
    }
    else if (m_pfRotationAngles) {
        m_pfRotationSpeeds = static_cast<float*>(_NiMalloc(m_usVertices * sizeof(float)));
        std::memset(m_pfRotationSpeeds, 0, m_usVertices * sizeof(float));
    }

    if (kStream.m_BSStreamHeader.uiVersion < 0x13) {
        NiStreamLoadBinary(kStream, m_usNumAddedParticles);
        NiStreamLoadBinary(kStream, m_usAddedParticlesBase);
    }
}

void NiPSysData::LinkObject(NiStream& kStream) {
    NiParticlesData::LinkObject(kStream);
}

bool NiPSysData::RegisterStreamables(NiStream& kStream) {
    return NiParticlesData::RegisterStreamables(kStream);
}

// Only whether rotation speeds exist is saved.
void NiPSysData::SaveBinary(NiStream& kStream) {
    NiParticlesData::SaveBinary(kStream);
    bool bHasRotationSpeeds = m_pfRotationSpeeds != nullptr;
    NiStreamSaveBinary(kStream, bHasRotationSpeeds);
}

// Rotation speeds must agree in presence; the infos and speeds of the active particles and the added-particle
// counts must match.
bool NiPSysData::IsEqual(NiObject* pkObject) {
    if (!NiParticlesData::IsEqual(pkObject)) {
        return false;
    }

    NiPSysData* pkData = static_cast<NiPSysData*>(pkObject);
    if ((m_pfRotationSpeeds && !pkData->m_pfRotationSpeeds) || (!m_pfRotationSpeeds && pkData->m_pfRotationSpeeds)) {
        return false;
    }

    for (uint16_t i = 0; i < m_usActive; i++) {
        if (!m_pkParticleInfo[i].IsEqual(pkData->m_pkParticleInfo[i])) {
            return false;
        }
        if (m_pfRotationSpeeds && m_pfRotationSpeeds[i] != pkData->m_pfRotationSpeeds[i]) {
            return false;
        }
    }

    if (m_usNumAddedParticles != pkData->m_usNumAddedParticles ||
        m_usAddedParticlesBase != pkData->m_usAddedParticlesBase) {
        return false;
    }

    return true;
}

void NiPSysData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiParticlesData::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_pkParticleInfo", static_cast<const void*>(m_pkParticleInfo)));
    pkStrings->Add(NiGetViewerString("m_pfRotationSpeeds", static_cast<const void*>(m_pfRotationSpeeds)));
    pkStrings->Add(NiGetViewerString("NumAddedParticles", m_usNumAddedParticles));
    pkStrings->Add(NiGetViewerString("AddedParticlesBase", m_usAddedParticlesBase));
}

// The base moves the last active particle into the removed one's place; its info and rotation speed follow. The base
// gets the caller's whole int32_t value, not the truncated index.
void NiPSysData::RemoveParticle(int32_t aiParticle) {
    uint16_t usLast = m_usActive - 1;
    NiParticlesData::RemoveParticle(aiParticle);
    uint16_t usParticle = static_cast<uint16_t>(aiParticle);
    if (usParticle != usLast) {
        m_pkParticleInfo[usParticle] = m_pkParticleInfo[usLast];
        if (m_pfRotationSpeeds) {
            m_pfRotationSpeeds[usParticle] = m_pfRotationSpeeds[usLast];
        }
    }
}

// New particles are appended after the active ones and stay inactive until ResolveAddedParticles; the first one of a
// batch fixes the batch's base. Returns INVALID_PARTICLE when the data is full.
uint16_t NiPSysData::AddParticle() {
    if (m_usNumAddedParticles == 0) {
        m_usAddedParticlesBase = m_usActive;
        if (m_usAddedParticlesBase >= m_usVertices) {
            return 0xFFFF;
        }
        m_usNumAddedParticles = 1;
        return m_usAddedParticlesBase;
    }

    if (m_usAddedParticlesBase + m_usNumAddedParticles >= m_usVertices) {
        return 0xFFFF;
    }
    return m_usAddedParticlesBase + m_usNumAddedParticles++;
}

// Activates the added particles. When particles were removed since the batch started (the active count fell below
// the base), the batch is moved down by removing the gap's slots while the count still exceeds the base.
void NiPSysData::ResolveAddedParticles() {
    uint16_t usActive = m_usActive;
    if (usActive < m_usAddedParticlesBase) {
        m_usActive = m_usNumAddedParticles + m_usAddedParticlesBase;
        uint16_t i = usActive;
        for (; i < m_usAddedParticlesBase && m_usActive > m_usAddedParticlesBase; i++) {
            RemoveParticle(i);
        }
        if (i < m_usAddedParticlesBase) {
            m_usActive = i;
        }
    }
    else {
        m_usActive = m_usNumAddedParticles + usActive;
    }

    m_usNumAddedParticles = 0;
    m_usAddedParticlesBase = m_usActive;
}

void NiPSysData::AlphaSortVertices(NiCamera* apCamera) {
    if (!m_ausRenderOrder) {
        m_ausRenderOrder = static_cast<uint16_t*>(MemoryManager_Singleton.Allocate(
            m_usVertices <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * m_usVertices : SIZE_MAX
        ));
        m_afDepths = static_cast<float*>(MemoryManager_Singleton.Allocate(
            m_usVertices <= SIZE_MAX / sizeof(float) ? sizeof(float) * m_usVertices : SIZE_MAX
        ));
    }
    for (int32_t i = 0; static_cast<uint16_t>(i) < m_usActive; ++i) {
        m_ausRenderOrder[static_cast<uint16_t>(i)] = i;
    }
    if (m_usActive > 1u) {
        int32_t v4 = 0;
        float kWorldDir = apCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
        float kWorldDir_4 = apCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
        float kWorldDir_8 = apCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
        do {
            int32_t v5 = static_cast<uint16_t>(v4);
            NiPoint3* v6 = &m_pkVertex[static_cast<uint16_t>(v4++)];
            float apCameraa = v6->y * kWorldDir_4 + v6->x * kWorldDir + v6->z * kWorldDir_8;
            m_afDepths[v5] = apCameraa;
        }
        while (static_cast<uint16_t>(v4) < m_usActive);
        SortVertices(0, m_usActive - 1);
    }
}

// The clone gets its own copies of the geometry and particle arrays and the particle state. The arrays are allocated
// from the heap singleton (sizes from the 16-bit vertex count never overflow).
void NiPSysData::CopyMembers(NiPSysData* pkDest, NiCloningProcess& kCloning) {
    NiObject::CopyMembers(pkDest, kCloning);
    NiPoint3* pkVertex = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(m_usVertices * sizeof(NiPoint3)));
    size_t stPointBytes = m_usVertices * sizeof(NiPoint3);
    rt::memcpy(pkVertex, m_pkVertex, stPointBytes);
    NiPoint3* pkNormal = 0;
    if (m_pkNormal) {
        pkNormal = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(m_usVertices * sizeof(NiPoint3)));
        rt::memcpy(pkNormal, m_pkNormal, stPointBytes);
    }
    NiColorA* pkColor = 0;
    if (m_pkColor) {
        int32_t iNumObjs = m_usVertices;
        pkColor = static_cast<NiColorA*>(MemoryManager_Singleton.Allocate(m_usVertices * sizeof(NiColorA)));
        if (pkColor) {
            rt::eh_vector_constructor_iterator(pkColor, sizeof(NiColorA), iNumObjs, &fnvd::constructElement<NiColorA>);
        }
        else {
            pkColor = 0;
        }
        rt::memcpy(pkColor, m_pkColor, m_usVertices * sizeof(NiColorA));
    }
    pkDest->Replace(m_usVertices, pkVertex, pkNormal, pkColor, 0, 0, NiGeometryData::NBT_METHOD_NONE);
    NiQuaternion* pkRotations = 0;
    if (m_pkRotations) {
        pkRotations = static_cast<NiQuaternion*>(MemoryManager_Singleton.Allocate(m_usVertices * sizeof(NiQuaternion)));
        rt::memcpy(pkRotations, m_pkRotations, m_usVertices * sizeof(NiQuaternion));
    }
    pkDest->ReplaceRotations(pkRotations);
    float* pfSizes = static_cast<float*>(_NiMalloc(sizeof(float) * m_usVertices));
    size_t stFloatBytes = sizeof(float) * m_usVertices;
    rt::memcpy(pfSizes, m_pfSizes, stFloatBytes);
    pkDest->ReplaceSizes(pfSizes);
    float* pfRadii = static_cast<float*>(_NiMalloc(sizeof(float) * m_usVertices));
    rt::memcpy(pfRadii, m_pfRadii, stFloatBytes);
    pkDest->ReplaceRadii(pfRadii);
    pkDest->m_usActive = m_usActive;
    if (m_pfRotationAngles) {
        float* pfAngles = static_cast<float*>(_NiMalloc(sizeof(float) * m_usVertices));
        rt::memcpy(pfAngles, m_pfRotationAngles, stFloatBytes);
        pkDest->ReplaceRotationAngles(pfAngles);
    }
    if (m_pkRotationAxes) {
        NiPoint3* pkAxes = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(m_usVertices * sizeof(NiPoint3)));
        rt::memcpy(pkAxes, m_pkRotationAxes, m_usVertices * sizeof(NiPoint3));
        pkDest->ReplaceRotationAxes(pkAxes);
    }
    if (m_pucTextureIndices) {
        uint8_t* pucTextureIndices = static_cast<uint8_t*>(_NiMalloc(m_usVertices));
        rt::memcpy(pucTextureIndices, m_pucTextureIndices, m_usVertices);
        pkDest->ReplaceTextureIndices(pucTextureIndices);
    }
    if (m_pkSubTextureOffsets) {
        NiColorA* pkSubTextureOffsets = static_cast<NiColorA*>(_NiMalloc(sizeof(NiColorA) * m_ucSubTextureOffsetCount));
        rt::memcpy(pkSubTextureOffsets, m_pkSubTextureOffsets, sizeof(NiColorA) * m_ucSubTextureOffsetCount);
        pkDest->ReplaceSubTextureOffsets(pkSubTextureOffsets, m_ucSubTextureOffsetCount);
    }
    NiParticleInfo* pkParticleInfo = 0;
    if (m_pkParticleInfo) {
        int32_t iNumInfos = m_usVertices;
        pkParticleInfo =
            static_cast<NiParticleInfo*>(MemoryManager_Singleton.Allocate(m_usVertices * sizeof(NiParticleInfo)));
        if (pkParticleInfo) {
            rt::eh_vector_constructor_iterator(
                pkParticleInfo, sizeof(NiParticleInfo), iNumInfos, &fnvd::constructElement<NiParticleInfo>
            );
        }
        else {
            pkParticleInfo = 0;
        }
        rt::memcpy(pkParticleInfo, m_pkParticleInfo, m_usVertices * sizeof(NiParticleInfo));
    }
    MemoryManager_Singleton.Deallocate(pkDest->m_pkParticleInfo);
    pkDest->m_pkParticleInfo = pkParticleInfo;
    float* pfRotationSpeeds = 0;
    if (m_pfRotationSpeeds) {
        pfRotationSpeeds = static_cast<float*>(_NiMalloc(sizeof(float) * m_usVertices));
        rt::memcpy(pfRotationSpeeds, m_pfRotationSpeeds, sizeof(float) * m_usVertices);
    }
    _NiFree(pkDest->m_pfRotationSpeeds);
    pkDest->m_pfRotationSpeeds = pfRotationSpeeds;
    pkDest->m_usNumAddedParticles = m_usNumAddedParticles;
    pkDest->m_usAddedParticlesBase = m_usAddedParticlesBase;
    float fRadius = m_kBound.m_fRadius;
    pkDest->m_kBound.m_kCenter.x = m_kBound.m_kCenter.x;
    pkDest->m_kBound.m_kCenter.y = m_kBound.m_kCenter.y;
    float z = m_kBound.m_kCenter.z;
    pkDest->m_kBound.m_fRadius = fRadius;
    pkDest->m_kBound.m_kCenter.z = z;
}

void NiPSysData::SortVertices(int32_t a2, int32_t a3) {
    while (1) {
        int32_t v6 = a2;
        int32_t a3a = a3;
        uint16_t v18 = this->m_ausRenderOrder[a2];
        if (a2 < a3) {
            double v7 = this->m_afDepths[this->m_ausRenderOrder[a2]];
            do {
                uint16_t* m_ausRenderOrder = this->m_ausRenderOrder;
                int32_t v9 = m_ausRenderOrder[a3];
                float* m_afDepths = this->m_afDepths;
                uint16_t* v11 = &m_ausRenderOrder[a3];
                if (m_afDepths[v9] <= v7) {
                    int32_t v12;
                    do {
                        if (a2 >= a3) {
                            break;
                        }
                        v12 = *--v11;
                        --a3;
                    }
                    while (m_afDepths[v12] <= v7);
                }
                if (a2 != a3) {
                    this->m_ausRenderOrder[a2++] = this->m_ausRenderOrder[a3];
                }
                uint16_t* v13 = this->m_ausRenderOrder;
                int32_t v14 = v13[a2];
                uint16_t* v15 = &v13[a2];
                float* v16 = this->m_afDepths;
                if (v16[v14] >= v7) {
                    int32_t v17;
                    do {
                        if (a2 >= a3) {
                            break;
                        }
                        v17 = v15[1];
                        ++v15;
                        ++a2;
                    }
                    while (v16[v17] >= v7);
                }
                if (a2 == a3) {
                    break;
                }
                this->m_ausRenderOrder[a3--] = this->m_ausRenderOrder[a2];
            }
            while (a2 < a3);
        }
        a3 = a3a;
        this->m_ausRenderOrder[a2] = v18;
        if (v6 < a2) {
            SortVertices(v6, a2 - 1);
        }
        if (a3a <= a2) {
            break;
        }
        ++a2;
    }
}

NiObject* NiPSysData::CreateObject() {
    NiPSysData* pkObject = static_cast<NiPSysData*>(NiMemObject::operator new(sizeof(NiPSysData)));
    if (!pkObject) {
        return 0;
    }
    return ::new (pkObject) NiPSysData;
}
