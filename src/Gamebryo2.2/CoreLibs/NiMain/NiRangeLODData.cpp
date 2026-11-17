#include "Gamebryo2.2/CoreLibs/NiMain/NiRangeLODData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLODData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLODNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/string.h"

#include <cfloat>
#include <cmath>
#include <cstring>

NiRTTI NiRangeLODData::ms_RTTI("NiRangeLODData", &NiLODData::ms_RTTI);

NiRangeLODData::NiRangeLODData() : m_kCenter(NiPoint3_ZERO), m_uiNumRanges(0), m_pkRanges(nullptr) {}

NiRangeLODData::~NiRangeLODData() {
    _NiFree(m_pkRanges);
}

NiObject* NiRangeLODData::CreateClone(NiCloningProcess& kCloning) {
    void* pMemory = NiMemObject::operator new(sizeof(NiRangeLODData));
    NiRangeLODData* pkClone = nullptr;
    if (pMemory) {
        // EXE-INLINED 0xA97700
        pkClone = ::new (pMemory) NiRangeLODData;
    }
    CopyMembers(pkClone, kCloning);
    return pkClone;
}

// The ranges are stored as near/far pairs; the world distances are derived in UpdateWorldData.
void NiRangeLODData::LoadBinary(NiStream& kStream) {
    NiLODData::LoadBinary(kStream);
    m_kCenter.LoadBinary(kStream);

    uint32_t uiNumRanges;
    NiStreamLoadBinary(kStream, uiNumRanges);
    SetNumRanges(uiNumRanges);
    for (uint32_t i = 0; i < uiNumRanges; i++) {
        NiStreamLoadBinary(kStream, m_pkRanges[i].m_fNear);
        NiStreamLoadBinary(kStream, m_pkRanges[i].m_fFar);
    }
}

void NiRangeLODData::LinkObject(NiStream& kStream) {
    NiLODData::LinkObject(kStream);
}

bool NiRangeLODData::RegisterStreamables(NiStream& kStream) {
    return NiLODData::RegisterStreamables(kStream);
}

void NiRangeLODData::SaveBinary(NiStream& kStream) {
    NiLODData::SaveBinary(kStream);
    m_kCenter.SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_uiNumRanges);
    for (uint32_t i = 0; i < m_uiNumRanges; i++) {
        NiStreamSaveBinary(kStream, m_pkRanges[i].m_fNear);
        NiStreamSaveBinary(kStream, m_pkRanges[i].m_fFar);
    }
}

// Only the near/far pairs are compared; the base comparison and the centers are not.
bool NiRangeLODData::IsEqual(NiObject* pkObject) {
    NiRangeLODData* pkRange = static_cast<NiRangeLODData*>(pkObject);
    if (m_uiNumRanges != pkRange->m_uiNumRanges) {
        return false;
    }

    for (uint32_t i = 0; i < m_uiNumRanges; i++) {
        if (m_pkRanges[i].m_fNear != pkRange->m_pkRanges[i].m_fNear ||
            m_pkRanges[i].m_fFar != pkRange->m_pkRanges[i].m_fFar) {
            return false;
        }
    }
    return true;
}

// The base classes' strings are not listed.
void NiRangeLODData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(m_kCenter.GetViewerString("m_kCenter"));
    pkStrings->Add(m_kWorldCenter.GetViewerString("m_kWorldCenter"));

    for (uint32_t i = 0; i < m_uiNumRanges; i++) {
        char* pcString = static_cast<char*>(_NiMalloc(128));
        NiSprintf(
            pcString, 128, "range[%d] = %g   %g", i, static_cast<double>(m_pkRanges[i].m_fNear),
            static_cast<double>(m_pkRanges[i].m_fFar)
        );
        pkStrings->Add(pcString);
    }
}

// The distance is measured along the camera's view direction (the first column of its world rotation), scaled by
// the camera's LOD adjustment. Returns the first level whose world range holds it, or -1.
int32_t NiRangeLODData::GetLODLevel(const NiCamera* pkCamera, NiLODNode*) {
    const NiTransform& kCamera = pkCamera->m_kWorld;
    float fDiffX = m_kWorldCenter.x - kCamera.m_Translate.x;
    float fDiffY = m_kWorldCenter.y - kCamera.m_Translate.y;
    float fDiffZ = m_kWorldCenter.z - kCamera.m_Translate.z;
    float fDirX = kCamera.m_Rotate.m_pEntry[0][0];
    float fDirY = kCamera.m_Rotate.m_pEntry[1][0];
    float fDirZ = kCamera.m_Rotate.m_pEntry[2][0];

    float fDistance = fDirX * fDiffX + fDirY * fDiffY + fDirZ * fDiffZ;
    fDistance *= pkCamera->m_fLODAdjust;
    fDistance = std::fabs(fDistance);

    for (uint32_t i = 0; i < m_uiNumRanges; i++) {
        if (m_pkRanges[i].m_fWorldNear <= fDistance && fDistance < m_pkRanges[i].m_fWorldFar) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

// Moves the center into world space and scales the ranges by the node's world scale.
void NiRangeLODData::UpdateWorldData(NiLODNode* pkLOD) {
    const NiTransform& kWorld = pkLOD->m_kWorld;
    float fScale = kWorld.m_fScale;
    float fX = fScale * m_kCenter.x;
    float fY = fScale * m_kCenter.y;
    float fZ = fScale * m_kCenter.z;

    const NiMatrix3& kRotate = kWorld.m_Rotate;
    float fRotatedX = kRotate.m_pEntry[0][0] * fX + kRotate.m_pEntry[0][1] * fY + kRotate.m_pEntry[0][2] * fZ;
    float fRotatedY = kRotate.m_pEntry[1][0] * fX + kRotate.m_pEntry[1][1] * fY + kRotate.m_pEntry[1][2] * fZ;
    float fRotatedZ = kRotate.m_pEntry[2][0] * fX + kRotate.m_pEntry[2][1] * fY + kRotate.m_pEntry[2][2] * fZ;
    m_kWorldCenter.x = fRotatedX + kWorld.m_Translate.x;
    m_kWorldCenter.y = fRotatedY + kWorld.m_Translate.y;
    m_kWorldCenter.z = fRotatedZ + kWorld.m_Translate.z;

    for (uint32_t i = 0; i < m_uiNumRanges; i++) {
        m_pkRanges[i].m_fWorldNear = m_pkRanges[i].m_fNear * kWorld.m_fScale;
        m_pkRanges[i].m_fWorldFar = m_pkRanges[i].m_fFar * kWorld.m_fScale;
    }
}

// The copy takes the center and the ranges, not the world center.
NiLODData* NiRangeLODData::Duplicate() {
    void* pMemory = NiMemObject::operator new(sizeof(NiRangeLODData));
    NiRangeLODData* pkDuplicate = nullptr;
    if (pMemory) {
        // EXE-INLINED 0xA97700
        pkDuplicate = ::new (pMemory) NiRangeLODData;
    }
    pkDuplicate->m_kCenter = m_kCenter;
    pkDuplicate->SetNumRanges(m_uiNumRanges);
    rt::memcpy(pkDuplicate->m_pkRanges, m_pkRanges, m_uiNumRanges * sizeof(Range));
    return pkDuplicate;
}

// Each pass takes the first range whose near distance lies beyond the last one taken; the level picks how many
// passes run (clamped to the range count). Returns -1 without ranges.
int32_t NiRangeLODData::GetLODIndex(int32_t iLODLevel) {
    int32_t iLevel = iLODLevel > 0 ? iLODLevel : 0;
    int32_t iLastLevel = static_cast<int32_t>(m_uiNumRanges) - 1;
    if (iLevel >= iLastLevel) {
        iLevel = iLastLevel;
    }

    int32_t iIndex = -1;
    float fLastNear = -FLT_MAX;
    for (int32_t iPass = 0; iPass <= iLevel; iPass++) {
        float fMinNear = FLT_MAX;
        for (int32_t i = 0; i < static_cast<int32_t>(m_uiNumRanges); i++) {
            if (fLastNear < m_pkRanges[i].m_fNear && fMinNear >= m_pkRanges[i].m_fNear) {
                fLastNear = m_pkRanges[i].m_fNear;
                iIndex = i;
                fMinNear = m_pkRanges[i].m_fNear;
            }
        }
    }
    return iIndex;
}

void NiRangeLODData::SetNumRanges(uint32_t uiNumRanges) {
    if (uiNumRanges == m_uiNumRanges) {
        return;
    }

    if (uiNumRanges) {
        Range* pkRanges = static_cast<Range*>(_NiMalloc(uiNumRanges * sizeof(Range)));
        if (m_pkRanges) {
            int32_t iNumKept = static_cast<int32_t>(m_uiNumRanges) < static_cast<int32_t>(uiNumRanges)
                ? static_cast<int32_t>(m_uiNumRanges)
                : static_cast<int32_t>(uiNumRanges);
            memcpy(pkRanges, m_pkRanges, iNumKept * sizeof(Range));
        }
        _NiFree(m_pkRanges);
        m_pkRanges = pkRanges;
        m_uiNumRanges = uiNumRanges;
    }
    else {
        _NiFree(m_pkRanges);
        m_pkRanges = nullptr;
        m_uiNumRanges = 0;
    }
}

void NiRangeLODData::CopyMembers(NiRangeLODData* pkDest, NiCloningProcess&) {
    pkDest->m_kCenter = m_kCenter;
    pkDest->m_kWorldCenter = m_kWorldCenter;
    pkDest->SetNumRanges(m_uiNumRanges);
    memcpy(pkDest->m_pkRanges, m_pkRanges, m_uiNumRanges * sizeof(Range));
}

NiObject* NiRangeLODData::CreateObject() {
    void* v0 = NiMemObject::operator new(sizeof(NiRangeLODData));
    if (!v0) {
        return nullptr;
    }
    return ::new (v0) NiRangeLODData;
}
