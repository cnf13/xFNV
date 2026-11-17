#include "Gamebryo2.2/CoreLibs/NiParticle/BSStripPSysData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticlesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/string.h"

#include <cfloat>

NiRTTI BSStripPSysData::ms_RTTI("BSStripPSysData", &NiPSysData::ms_RTTI);

// Only the strip list is cleared; the other fields are deliberately left uninitialised, as the game does.
BSStripPSysData::BSStripPSysData() : pStripList(nullptr) {}

// Frees each strip's points, then the strip list; NiPSysData's destructor runs after.
BSStripPSysData::~BSStripPSysData() {
    for (uint32_t ui = 0; ui < m_usVertices; ++ui) {
        _NiFree(pStripList[ui].pPointList);
        pStripList[ui].pPointList = nullptr;
    }
    _NiFree(pStripList);
}

NiObject* BSStripPSysData::CreateClone(NiCloningProcess& kCloning) {
    BSStripPSysData* pkObject_2 = static_cast<BSStripPSysData*>(NiMemObject::operator new(sizeof(BSStripPSysData)));
    if (pkObject_2) {
        ::new (pkObject_2) BSStripPSysData;
        CopyMembers(pkObject_2, kCloning);
        return pkObject_2;
    }
    else {
        CopyMembers(0, kCloning);
        return 0;
    }
}

// Each vertex gets a strip of usMaxPointCount points; the point buffers are not constructed.
void BSStripPSysData::LoadBinary(NiStream& kStream) {
    NiPSysData::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, usMaxPointCount);
    NiStreamLoadBinary(kStream, fStartCapSize);
    NiStreamLoadBinary(kStream, fEndCapSize);
    NiStreamLoadBinary(kStream, bDoZPrePass);

    uint32_t uiStrips = m_usVertices;
    pStripList = static_cast<StripData*>(_NiMalloc(uiStrips * sizeof(StripData)));
    for (uint32_t i = 0; i < uiStrips; i++) {
        pStripList[i].pPointList = static_cast<PointData*>(_NiMalloc(usMaxPointCount * sizeof(PointData)));
        // EXE-INLINED 0xC25D40
        uint16_t usParticle = static_cast<uint16_t>(i);
        pStripList[usParticle].usPointCount = 0;
        pStripList[usParticle].usHighestPointCount = 0;
        pStripList[usParticle].usMaxPointCount = usMaxPointCount;
        pStripList[usParticle].fLastTime = -FLT_MAX;
        pStripList[usParticle].iFirstIndex = -1;
        pStripList[usParticle].iLastIndex = 0;
        pStripList[usParticle].iStopIndex = usMaxPointCount;
        pStripList[usParticle].iStartIndex = usMaxPointCount;
    }
}

void BSStripPSysData::LinkObject(NiStream& kStream) {
    NiPSysData::LinkObject(kStream);
}

bool BSStripPSysData::RegisterStreamables(NiStream& kStream) {
    return NiPSysData::RegisterStreamables(kStream);
}

void BSStripPSysData::SaveBinary(NiStream& kStream) {
    NiPSysData::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, usMaxPointCount);
    NiStreamSaveBinary(kStream, fStartCapSize);
    NiStreamSaveBinary(kStream, fEndCapSize);
    NiStreamSaveBinary(kStream, bDoZPrePass);
}

bool BSStripPSysData::IsEqual(NiObject* pkObject) {
    if (!NiPSysData::IsEqual(pkObject)) {
        return false;
    }

    BSStripPSysData* pkOther = static_cast<BSStripPSysData*>(pkObject);
    if (pkOther->usMaxPointCount != usMaxPointCount || pkOther->fStartCapSize != fStartCapSize ||
        pkOther->fEndCapSize != fEndCapSize || pkOther->bDoZPrePass != bDoZPrePass) {
        return false;
    }

    return true;
}

void BSStripPSysData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysData::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("usMaxPointCount", usMaxPointCount));
    pkStrings->Add(NiGetViewerString("fStartCapSize", fStartCapSize));
    pkStrings->Add(NiGetViewerString("fEndCapSize", fEndCapSize));
    pkStrings->Add(NiGetViewerString("bDoZPrePass", bDoZPrePass));
}

// A strip with more than one point only drops its oldest point; the particle itself goes once its strip is down to one
// point, and its strip trades places with the last active particle's like the base's per-particle data. The base gets
// the caller's whole ausParticle, not the truncated usParticle.
void BSStripPSysData::RemoveParticle(int32_t ausParticle) {
    uint16_t usParticle = static_cast<uint16_t>(ausParticle);
    StripData& kStrip = pStripList[usParticle];
    if (kStrip.usPointCount > 1) {
        kStrip.IncFirstIndex();
        pStripList[usParticle].usPointCount--;
        pStripList[usParticle].iStopIndex--;
        return;
    }

    uint16_t usLast = m_usActive - 1;
    NiPSysData::RemoveParticle(ausParticle);
    if (usParticle != usLast) {
        StripData kTemp = pStripList[usParticle];
        pStripList[usParticle] = pStripList[usLast];
        pStripList[usLast] = kTemp;
    }
}

void BSStripPSysData::InitializePoint(uint16_t ausParticle) {
    pStripList[ausParticle].usPointCount = 0;
    pStripList[ausParticle].usHighestPointCount = 0;
    pStripList[ausParticle].usMaxPointCount = usMaxPointCount;
    pStripList[ausParticle].fLastTime = -FLT_MAX;
    pStripList[ausParticle].iFirstIndex = -1;
    pStripList[ausParticle].iLastIndex = 0;
    pStripList[ausParticle].iStopIndex = usMaxPointCount;
    pStripList[ausParticle].iStartIndex = usMaxPointCount;
}

float BSStripPSysData::GetPointLastUpdateTime(uint16_t ausParticle) const {
    return pStripList[ausParticle].fLastTime;
}

uint16_t BSStripPSysData::GetPointCount(uint16_t ausParticle) const {
    return pStripList[ausParticle].usPointCount;
}

const NiPoint3* BSStripPSysData::GetPointPoint(uint16_t ausParticle, uint16_t ausPoint) const {
    return &pStripList[ausParticle].pPointList[ausPoint].kPoint;
}

float BSStripPSysData::GetPointSize(uint16_t ausParticle, uint16_t ausPoint) const {
    return pStripList[ausParticle].pPointList[ausPoint].fSize;
}

float BSStripPSysData::GetPointRadii(uint16_t ausParticle, uint16_t ausPoint) const {
    return pStripList[ausParticle].pPointList[ausPoint].fRadii;
}

const NiPoint3* BSStripPSysData::GetPointVerts(uint16_t ausParticle, uint16_t ausPoint) const {
    return pStripList[ausParticle].pPointList[ausPoint].kVertex;
}

const NiPoint3* BSStripPSysData::GetPointNorms(uint16_t ausParticle, uint16_t ausPoint) const {
    return pStripList[ausParticle].pPointList[ausPoint].kNormal;
}

float BSStripPSysData::GetTexCoordDelta(uint16_t ausParticle) const {
    return (float)(1.0 / (((double)pStripList[ausParticle].usHighestPointCount) - 1.0));
}

const NiColorA* BSStripPSysData::GetPointColor(uint16_t ausParticle, uint16_t ausPoint) const {
    if (m_pkColor) {
        return &pStripList[ausParticle].pPointList[ausPoint].kColor;
    }
    else {
        return &NiColorA_WHITE;
    }
}

void BSStripPSysData::UpdatePoints(uint16_t ausParticle, float) {
    NiParticleInfo& kInfo = m_pkParticleInfo[ausParticle];
    if (kInfo.m_fLifeSpan < kInfo.m_fAge) {
        return;
    }

    StripData& kStrip = pStripList[ausParticle];
    // The previous point's direction and two normals, which GetVerts continues the ribbon from.
    const NiPoint3* apkPrevious[3] = {nullptr, nullptr, nullptr};

    // The newest point so far is rebuilt with the particle's position as its next point.
    int32_t iNewest = kStrip.PrevIndex(kStrip.iLastIndex);
    if (kStrip.usPointCount > 0) {
        if (kStrip.usPointCount > 1) {
            int32_t iPrevious = kStrip.PrevIndex(iNewest);
            apkPrevious[0] = &kStrip.pPointList[iPrevious].kDir;
            apkPrevious[1] = &kStrip.pPointList[iPrevious].kNormal[0];
            apkPrevious[2] = &kStrip.pPointList[iPrevious].kNormal[1];
        }
        PointData& kNewest = kStrip.pPointList[iNewest];
        float fSize = kNewest.fRadii * kNewest.fSize;
        NiRenderer::PackParticleStripUtil_GetVerts(
            apkPrevious, &kNewest.kPoint, &m_pkVertex[ausParticle], fSize, kNewest.kVertex, kNewest.kNormal,
            kNewest.kDir
        );
    }
    else {
        kStrip.iFirstIndex = 0;
    }

    int32_t iLast = kStrip.iLastIndex;
    if (kStrip.usPointCount > 0) {
        int32_t iPrevious = kStrip.PrevIndex(iLast);
        apkPrevious[0] = &kStrip.pPointList[iPrevious].kDir;
        apkPrevious[1] = &kStrip.pPointList[iPrevious].kNormal[0];
        apkPrevious[2] = &kStrip.pPointList[iPrevious].kNormal[1];
    }
    else {
        apkPrevious[0] = nullptr;
        apkPrevious[1] = nullptr;
        apkPrevious[2] = nullptr;
    }

    PointData& kPoint = kStrip.pPointList[iLast];
    kPoint.kPoint = m_pkVertex[ausParticle];
    kPoint.fSize = m_pfSizes[ausParticle];
    kPoint.fRadii = m_pfRadii[ausParticle];
    if (m_pkColor) {
        kPoint.kColor = m_pkColor[ausParticle];
    }
    float fSize = kPoint.fRadii * kPoint.fSize;
    NiRenderer::PackParticleStripUtil_GetVerts(
        apkPrevious, &kPoint.kPoint, nullptr, fSize, kPoint.kVertex, kPoint.kNormal, kPoint.kDir
    );

    if (kStrip.usPointCount < usMaxPointCount) {
        kStrip.usHighestPointCount++;
        kStrip.iStartIndex--;
        kStrip.usPointCount++;
    }
    else {
        kStrip.IncFirstIndex();
    }
    kStrip.IncLastIndex();
}

// The strips are copied field by field; each strip's point buffer is sized by its usMaxPointCount, read back from the
// new strip, and only its usPointCount points are copied.
void BSStripPSysData::CopyMembers(BSStripPSysData* pkDest, NiCloningProcess& kCloning) {
    NiPSysData::CopyMembers(pkDest, kCloning);
    pkDest->usMaxPointCount = GetMaxPointCount();
    pkDest->fStartCapSize = fStartCapSize;
    pkDest->fEndCapSize = fEndCapSize;
    pkDest->bDoZPrePass = bDoZPrePass;
    uint32_t uiCount = m_usVertices;
    StripData* pNewStripList = static_cast<StripData*>(_NiMalloc(sizeof(StripData) * uiCount));
    for (uint32_t ui = 0; ui < uiCount; ++ui) {
        StripData& kNew = pNewStripList[ui];
        kNew.fLastTime = pStripList[ui].fLastTime;
        kNew.usPointCount = pStripList[ui].usPointCount;
        kNew.usHighestPointCount = pStripList[ui].usHighestPointCount;
        kNew.iStopIndex = pStripList[ui].iStopIndex;
        kNew.iStartIndex = pStripList[ui].iStartIndex;
        kNew.usMaxPointCount = pStripList[ui].usMaxPointCount;
        kNew.iFirstIndex = pStripList[ui].iFirstIndex;
        kNew.iLastIndex = pStripList[ui].iLastIndex;
        kNew.pPointList = static_cast<PointData*>(_NiMalloc(sizeof(PointData) * kNew.usMaxPointCount));
        rt::memcpy(kNew.pPointList, pStripList[ui].pPointList, sizeof(PointData) * pStripList[ui].usPointCount);
    }
    pkDest->pStripList = pNewStripList;
}

NiObject* BSStripPSysData::CreateObject() {
    BSStripPSysData* pkObject = static_cast<BSStripPSysData*>(NiMemObject::operator new(sizeof(BSStripPSysData)));
    if (!pkObject) {
        return 0;
    }
    return ::new (pkObject) BSStripPSysData;
}
