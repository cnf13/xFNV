#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSSceneGraph.h"
#include "Fallout Shared/Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cmath>
#include <cstring>

NiRTTI NiCamera::ms_RTTI("NiCamera", &NiAVObject::ms_RTTI);

NiCamera::NiCamera()
  : m_kViewFrustum(false), m_fMinNearPlaneDist(0.1f), m_fMaxFarNearRatio(10000.0f), m_fLODAdjust(1.0f) {
    m_kViewFrustum.m_fNear = 1.0f;
    m_kViewFrustum.m_fFar = 2.0f;
    m_kViewFrustum.m_fTop = 0.5f;
    m_kViewFrustum.m_fBottom = -0.5f;
    m_kViewFrustum.m_fLeft = -0.5f;
    m_kViewFrustum.m_fRight = 0.5f;
    m_kPort.m_left = 0.0f;
    m_kPort.m_right = 1.0f;
    m_kPort.m_top = 1.0f;
    m_kPort.m_bottom = 0.0f;
    NiUpdateData kUpdateData;
    kUpdateData.fTime = 0.0f;
    kUpdateData.bUpdateControllers = false;
    kUpdateData.bParallelUpdate = false;
    kUpdateData.bFoundParticles = false;
    kUpdateData.bFoundMorphController = false;
    kUpdateData.bSceneGraphChange = false;
    NiAVObject::UpdateWorldData(kUpdateData);
    WorldToCameraMatrix();
    NiCamera::UpdateWorldBound();
}

NiCamera::~NiCamera() {}

NiObject* NiCamera::CreateClone(NiCloningProcess& kCloning) {
    NiCamera* v4;
    void* v3 = NiMemObject::operator new(sizeof(NiCamera));
    if (v3) {
        v4 = ::new (v3) NiCamera;
        CopyMembers(v4, kCloning);
        return v4;
    }
    else {
        CopyMembers(nullptr, kCloning);
        return nullptr;
    }
}

// Files before 4.1.0.4 and between 10.0.1.7 and 10.0.1.15 carry links (scene, screen polygons, screen textures) the
// camera reads and drops.
void NiCamera::LoadBinary(NiStream& kStream) {
    NiAVObject::LoadBinary(kStream);

    if (kStream.GetFileVersion() >= 0x0A000102) {
        uint16_t usObsoleteFlags;
        NiStreamLoadBinary(kStream, usObsoleteFlags);
    }

    NiStreamLoadBinary(kStream, m_kViewFrustum.m_fLeft);
    NiStreamLoadBinary(kStream, m_kViewFrustum.m_fRight);
    NiStreamLoadBinary(kStream, m_kViewFrustum.m_fTop);
    NiStreamLoadBinary(kStream, m_kViewFrustum.m_fBottom);
    NiStreamLoadBinary(kStream, m_kViewFrustum.m_fNear);
    NiStreamLoadBinary(kStream, m_kViewFrustum.m_fFar);
    if (kStream.GetFileVersion() >= 0x0A000105) {
        uint8_t ucOrtho;
        NiStreamLoadBinary(kStream, ucOrtho);
        m_kViewFrustum.m_bOrtho = ucOrtho != 0;
    }
    else {
        m_kViewFrustum.m_bOrtho = false;
    }

    NiStreamLoadBinary(kStream, m_kPort.m_left);
    NiStreamLoadBinary(kStream, m_kPort.m_right);
    NiStreamLoadBinary(kStream, m_kPort.m_top);
    NiStreamLoadBinary(kStream, m_kPort.m_bottom);
    NiStreamLoadBinary(kStream, m_fLODAdjust);

    kStream.ReadLinkID();
    kStream.ReadMultipleLinkIDs();
    if (kStream.GetFileVersion() >= 0x04010004) {
        kStream.ReadMultipleLinkIDs();
        if (kStream.GetFileVersion() >= 0x0A000107 && kStream.GetFileVersion() < 0x0A00010F) {
            kStream.ReadMultipleLinkIDs();
        }
    }
}

// KEEP: niread: from 10.0.1.5 on the exe reads the ortho byte into the stream argument's slot (0xA715A2), which the
// frustum reads before it used for their component size 4, so a short read (malformed data) leaves 4 there and
// m_bOrtho becomes 1; src's byte is an unwritten local and src cannot express the slot's reuse. porting.md's
// unwritten-slot exception covers GetChunkData only.
void NiCamera::LinkObject(NiStream& kStream) {
    NiAVObject::LinkObject(kStream);

    NiPointer<NiObject> spScene = kStream.GetObjectFromLinkID();

    uint32_t uiNumLinks = kStream.GetNumberOfLinkIDs();
    for (uint32_t i = 0; i < uiNumLinks; i++) {
        NiPointer<NiObject> spObject = kStream.GetObjectFromLinkID();
    }

    if (kStream.GetFileVersion() >= 0x04010004) {
        uiNumLinks = kStream.GetNumberOfLinkIDs();
        for (uint32_t i = 0; i < uiNumLinks; i++) {
            NiPointer<NiObject> spObject = kStream.GetObjectFromLinkID();
        }

        if (kStream.GetFileVersion() >= 0x0A000107 && kStream.GetFileVersion() < 0x0A00010F) {
            uiNumLinks = kStream.GetNumberOfLinkIDs();
            for (uint32_t i = 0; i < uiNumLinks; i++) {
                kStream.GetObjectFromLinkID();
            }
        }
    }
}

bool NiCamera::RegisterStreamables(NiStream& kStream) {
    return NiAVObject::RegisterStreamables(kStream);
}

// The obsolete links are saved empty: a null scene and no screen polygons or textures.
void NiCamera::SaveBinary(NiStream& kStream) {
    NiAVObject::SaveBinary(kStream);

    uint16_t usObsoleteFlags = 0;
    NiStreamSaveBinary(kStream, usObsoleteFlags);
    NiStreamSaveBinary(kStream, m_kViewFrustum.m_fLeft);
    NiStreamSaveBinary(kStream, m_kViewFrustum.m_fRight);
    NiStreamSaveBinary(kStream, m_kViewFrustum.m_fTop);
    NiStreamSaveBinary(kStream, m_kViewFrustum.m_fBottom);
    NiStreamSaveBinary(kStream, m_kViewFrustum.m_fNear);
    NiStreamSaveBinary(kStream, m_kViewFrustum.m_fFar);
    uint8_t ucOrtho = m_kViewFrustum.m_bOrtho;
    NiStreamSaveBinary(kStream, ucOrtho);
    NiStreamSaveBinary(kStream, m_kPort.m_left);
    NiStreamSaveBinary(kStream, m_kPort.m_right);
    NiStreamSaveBinary(kStream, m_kPort.m_top);
    NiStreamSaveBinary(kStream, m_kPort.m_bottom);
    NiStreamSaveBinary(kStream, m_fLODAdjust);

    kStream.SaveLinkID(nullptr);
    uint32_t uiNumScreenPolygons = 0;
    NiStreamSaveBinary(kStream, uiNumScreenPolygons);
    uint32_t uiNumScreenTextures = 0;
    NiStreamSaveBinary(kStream, uiNumScreenTextures);
}

// The orthographic flag and the near-plane limits are not compared.
bool NiCamera::IsEqual(NiObject* pkObject) {
    if (!NiAVObject::IsEqual(pkObject)) {
        return false;
    }

    NiCamera* pkCamera = static_cast<NiCamera*>(pkObject);
    if (pkCamera->m_kViewFrustum.m_fLeft != m_kViewFrustum.m_fLeft ||
        pkCamera->m_kViewFrustum.m_fRight != m_kViewFrustum.m_fRight ||
        pkCamera->m_kViewFrustum.m_fTop != m_kViewFrustum.m_fTop ||
        pkCamera->m_kViewFrustum.m_fBottom != m_kViewFrustum.m_fBottom ||
        pkCamera->m_kViewFrustum.m_fNear != m_kViewFrustum.m_fNear ||
        pkCamera->m_kViewFrustum.m_fFar != m_kViewFrustum.m_fFar) {
        return false;
    }
    if (pkCamera->m_kPort.m_left != m_kPort.m_left || pkCamera->m_kPort.m_right != m_kPort.m_right ||
        pkCamera->m_kPort.m_top != m_kPort.m_top || pkCamera->m_kPort.m_bottom != m_kPort.m_bottom) {
        return false;
    }
    return pkCamera->m_fLODAdjust == m_fLODAdjust;
}

void NiCamera::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiAVObject::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(m_kViewFrustum.GetViewerString("m_kViewFrustum"));
    pkStrings->Add(m_kPort.GetViewerString("m_kPort"));
    // EXE-INLINED 0x45BBA0
    NiPoint3 kWorldDir;
    kWorldDir.x = m_kWorld.m_Rotate.m_pEntry[0][0];
    kWorldDir.y = m_kWorld.m_Rotate.m_pEntry[1][0];
    kWorldDir.z = m_kWorld.m_Rotate.m_pEntry[2][0];
    pkStrings->Add(kWorldDir.GetViewerString("m_kWorldDir"));
    // EXE-INLINED 0x4E9C10
    NiPoint3 kWorldUp;
    kWorldUp.x = m_kWorld.m_Rotate.m_pEntry[0][1];
    kWorldUp.y = m_kWorld.m_Rotate.m_pEntry[1][1];
    kWorldUp.z = m_kWorld.m_Rotate.m_pEntry[2][1];
    pkStrings->Add(kWorldUp.GetViewerString("m_kWorldUp"));
    // EXE-INLINED 0x4E9C50
    NiPoint3 kWorldRight;
    kWorldRight.x = m_kWorld.m_Rotate.m_pEntry[0][2];
    kWorldRight.y = m_kWorld.m_Rotate.m_pEntry[1][2];
    kWorldRight.z = m_kWorld.m_Rotate.m_pEntry[2][2];
    pkStrings->Add(kWorldRight.GetViewerString("m_kWorldRight"));
    pkStrings->Add(NiGetViewerString("m_fMinNearPlaneDist", m_fMinNearPlaneDist));
    pkStrings->Add(NiGetViewerString("m_fMaxFarNearRatio", m_fMaxFarNearRatio));
}

void NiCamera::UpdateWorldData(NiUpdateData& UpdateData) {
    NiAVObject::UpdateWorldData(UpdateData);
    WorldToCameraMatrix();
}

void NiCamera::UpdateWorldBound() {
    if (!m_kWorldBound) {
        SetBound(static_cast<BSNiBound*>(MemoryManager_Singleton.Allocate(sizeof(NiBound))));
        m_kWorldBound->m_kCenter = NiPoint3_ZERO;
        m_kWorldBound->m_fRadius = 0.0f;
    }
    m_kWorldBound->m_kCenter = m_kWorld.m_Translate;
}

NiPoint3 NiCamera::GetWorldDirection() const {
    NiPoint3 kDirection;
    m_kWorld.m_Rotate.GetCol(0, kDirection);
    return static_cast<const NiPoint3&>(kDirection);
}

// Builds the column in a local and copies it into the result at the end, as GetWorldDirection does.
NiPoint3 NiCamera::GetWorldUpVector() const {
    NiPoint3 kUp;
    m_kWorld.m_Rotate.GetCol(1, kUp);
    return static_cast<const NiPoint3&>(kUp);
}

// Builds the column in a local and copies it into the result at the end, as GetWorldDirection does.
NiPoint3 NiCamera::GetWorldRightVector() const {
    NiPoint3 kRight;
    m_kWorld.m_Rotate.GetCol(2, kRight);
    return static_cast<const NiPoint3&>(kRight);
}

const NiFrustum& NiCamera::GetViewFrustum() const {
    return m_kViewFrustum;
}

void NiCamera::SetViewFrustum(const NiFrustum& kFrustum) {
    float fNear = kFrustum.m_fNear;
    m_kViewFrustum.m_fNear = fNear;
    float fRatioNear = kFrustum.m_fFar / m_fMaxFarNearRatio;
    if (fRatioNear > fNear) {
        m_kViewFrustum.m_fNear = fRatioNear;
    }
    if (m_fMinNearPlaneDist > m_kViewFrustum.m_fNear) {
        m_kViewFrustum.m_fNear = m_fMinNearPlaneDist;
    }

    m_kViewFrustum.m_fLeft = kFrustum.m_fLeft;
    m_kViewFrustum.m_fRight = kFrustum.m_fRight;
    m_kViewFrustum.m_fTop = kFrustum.m_fTop;
    m_kViewFrustum.m_fBottom = kFrustum.m_fBottom;
    m_kViewFrustum.m_fFar = kFrustum.m_fFar;
    m_kViewFrustum.m_bOrtho = kFrustum.m_bOrtho;
}

void NiCamera::SetMinNearPlaneDist(float fMin) {
    m_fMinNearPlaneDist = fMin;
}

float NiCamera::GetMaxFarNearRatio() const {
    return m_fMaxFarNearRatio;
}

void NiCamera::SetMaxFarNearRatio(float fMax) {
    m_fMaxFarNearRatio = fMax;
}

void NiCamera::SetViewPort(const NiRect<float>& kPort) {
    m_kPort = kPort;
}

void NiCamera::AdjustAspectRatio(float fTargetAspectRatio, bool bAdjustVerticalDims) {
    if (bAdjustVerticalDims) {
        float fWidth = m_kViewFrustum.m_fRight - m_kViewFrustum.m_fLeft;
        float fHalfHeight = fWidth / fTargetAspectRatio * 0.5;
        float fCenter = 0.5 * (m_kViewFrustum.m_fBottom + m_kViewFrustum.m_fTop);
        m_kViewFrustum.m_fTop = fCenter + fHalfHeight;
        m_kViewFrustum.m_fBottom = fCenter - fHalfHeight;
    }
    else {
        float fHeight = m_kViewFrustum.m_fTop - m_kViewFrustum.m_fBottom;
        float fHalfWidth = fHeight * fTargetAspectRatio * 0.5;
        float fCenter = 0.5 * (m_kViewFrustum.m_fRight + m_kViewFrustum.m_fLeft);
        m_kViewFrustum.m_fRight = fCenter + fHalfWidth;
        m_kViewFrustum.m_fLeft = fCenter - fHalfWidth;
    }
}

bool NiCamera::WorldPtToScreenPt(const NiPoint3& kPt, float& fBx, float& fBy, float fZeroTolerance) {
    float fW = m_aafWorldToCam[3][0] * kPt.x + m_aafWorldToCam[3][1] * kPt.y + m_aafWorldToCam[3][2] * kPt.z +
        m_aafWorldToCam[3][3];
    if (!(fZeroTolerance < fW)) {
        return false;
    }

    float fInvW = 1.0f / fW;
    fBx = m_aafWorldToCam[0][0] * kPt.x + m_aafWorldToCam[0][1] * kPt.y + m_aafWorldToCam[0][2] * kPt.z +
        m_aafWorldToCam[0][3];
    fBy = m_aafWorldToCam[1][0] * kPt.x + m_aafWorldToCam[1][1] * kPt.y + m_aafWorldToCam[1][2] * kPt.z +
        m_aafWorldToCam[1][3];
    fBx = fBx * fInvW;
    fBy = fInvW * fBy;

    // outside the frustum (or unordered)
    if (!(fBx >= -1.0f) || fBx > 1.0f || !(fBy >= -1.0f) || fBy > 1.0f) {
        return false;
    }

    fBx = (m_kPort.m_right - m_kPort.m_left) * 0.5 * fBx;
    fBy = (m_kPort.m_top - m_kPort.m_bottom) * 0.5 * fBy;
    fBx = (m_kPort.m_right + m_kPort.m_left) * 0.5 + fBx;
    fBy = 0.5 * (m_kPort.m_bottom + m_kPort.m_top) + fBy;
    return true;
}

bool NiCamera::WorldPtToScreenPt3(const NiPoint3& kPt, float& fBx, float& fBy, float& fBz, float fZeroTolerance) {
    float fW = m_aafWorldToCam[3][0] * kPt.x + m_aafWorldToCam[3][1] * kPt.y + m_aafWorldToCam[3][2] * kPt.z +
        m_aafWorldToCam[3][3];
    fW = std::fabs(fW);
    if (!(fZeroTolerance < fW)) {
        return false;
    }

    float fInvW = 1.0f / fW;
    fBx = m_aafWorldToCam[0][0] * kPt.x + m_aafWorldToCam[0][1] * kPt.y + m_aafWorldToCam[0][2] * kPt.z +
        m_aafWorldToCam[0][3];
    fBy = m_aafWorldToCam[1][0] * kPt.x + m_aafWorldToCam[1][1] * kPt.y + m_aafWorldToCam[1][2] * kPt.z +
        m_aafWorldToCam[1][3];
    fBx = fBx * fInvW;
    fBy = fBy * fInvW;

    fBx = (m_kPort.m_right - m_kPort.m_left) * 0.5 * fBx;
    fBy = (m_kPort.m_top - m_kPort.m_bottom) * 0.5 * fBy;
    fBx = (m_kPort.m_right + m_kPort.m_left) * 0.5 + fBx;
    fBy = 0.5 * (m_kPort.m_bottom + m_kPort.m_top) + fBy;

    float fZ = m_aafWorldToCam[2][0] * kPt.x + m_aafWorldToCam[2][1] * kPt.y + m_aafWorldToCam[2][2] * kPt.z +
        m_aafWorldToCam[2][3];
    fBz = fInvW * fZ;
    return true;
}

bool NiCamera::FitNearAndFarToBound(const NiBound& kBound) {
    NiUpdateData kData;
    float fDirX = m_kWorld.m_Rotate.m_pEntry[0][0];
    float fDirY = m_kWorld.m_Rotate.m_pEntry[1][0];
    float fDirZ = m_kWorld.m_Rotate.m_pEntry[2][0];
    // The same untyped storage holds fDirZ's bytes first and the update data's bytes later, so fDirZ's bytes are
    // deliberately copied into kData starting at bSceneGraphChange, as the game does.
    __builtin_memcpy(&kData.bSceneGraphChange, &fDirZ, sizeof(fDirZ));
    float fDiffX = kBound.m_kCenter.x - m_kWorld.m_Translate.x;
    float fDiffY = kBound.m_kCenter.y - m_kWorld.m_Translate.y;
    float fDiffZ = kBound.m_kCenter.z - m_kWorld.m_Translate.z;
    float fDist = fDiffY * fDirY + fDiffX * fDirX + fDirZ * fDiffZ;
    float fNear = fDist - kBound.m_fRadius;
    float fFar = fDist + kBound.m_fRadius;
    if (fFar <= 0.0f) {
        return false;
    }

    float fRatioNear = fFar / m_fMaxFarNearRatio;
    if (fRatioNear > fNear) {
        fNear = fRatioNear;
    }
    if (m_fMinNearPlaneDist > fNear) {
        fNear = m_fMinNearPlaneDist;
    }
    m_kViewFrustum.m_fNear = fNear;
    m_kViewFrustum.m_fFar = fFar;

    kData.fTime = 0.0f;
    kData.bUpdateControllers = false;
    kData.bParallelUpdate = false;
    kData.bFoundParticles = false;
    kData.bFoundMorphController = false;
    kData.bSceneGraphChange = false;
    UpdateWorldData(kData);
    UpdateWorldBound();
    return true;
}

bool NiCamera::LookAtWorldPoint(const NiPoint3& kWorldPt, const NiPoint3& kWorldUp) {
    // the view direction points from the target to the camera here and is negated into column 0
    NiPoint3 kDir;
    kDir.x = m_kWorld.m_Translate.x - kWorldPt.x;
    kDir.y = m_kWorld.m_Translate.y - kWorldPt.y;
    kDir.z = m_kWorld.m_Translate.z - kWorldPt.z;
    float fSqrLength = kDir.x * kDir.x + kDir.y * kDir.y + kDir.z * kDir.z;
    if (fSqrLength < 1e-10f) {
        return false;
    }
    kDir.Unitize();

    NiPoint3 kRight = kWorldUp.UnitCross(kDir);
    fSqrLength = kRight.x * kRight.x + kRight.y * kRight.y + kRight.z * kRight.z;
    if (fSqrLength < 0.5f) {
        return false;
    }

    NiPoint3 kUp = kDir.UnitCross(kRight);
    fSqrLength = kUp.x * kUp.x + kUp.y * kUp.y + kUp.z * kUp.z;
    if (fSqrLength < 0.5f) {
        return false;
    }

    NiPoint3 kNegDir;
    kNegDir.x = -kDir.x;
    kNegDir.y = -kDir.y;
    kNegDir.z = -kDir.z;
    NiMatrix3 kRot;
    kRot.m_pEntry[0][0] = kNegDir.x;
    kRot.m_pEntry[1][0] = kNegDir.y;
    kRot.m_pEntry[2][0] = kNegDir.z;
    kRot.m_pEntry[0][1] = kUp.x;
    kRot.m_pEntry[1][1] = kUp.y;
    kRot.m_pEntry[2][1] = kUp.z;
    kRot.m_pEntry[0][2] = kRight.x;
    kRot.m_pEntry[1][2] = kRight.y;
    kRot.m_pEntry[2][2] = kRight.z;
    if (m_pkParent) {
        kRot = m_pkParent->m_kWorld.m_Rotate.TransposeTimes(kRot);
    }
    m_kLocal.m_Rotate = kRot;

    NiUpdateData kData;
    kData.fTime = 0.0f;
    kData.bUpdateControllers = false;
    kData.bParallelUpdate = false;
    kData.bFoundParticles = false;
    kData.bFoundMorphController = false;
    kData.bSceneGraphChange = false;
    UpdateWorldData(kData);
    UpdateWorldBound();
    return true;
}

bool NiCamera::WindowPointToRay(
    int32_t lWx,
    int32_t lWy,
    NiPoint3& kOrigin,
    NiPoint3& kDir,
    const NiRenderTargetGroup* pkTarget
) {
    NiRenderer* pkRenderer = NiRenderer::ms_pkRenderer;
    if (!pkRenderer) {
        return false;
    }

    float fX;
    float fY;
    // MapWindowPointToBufferPoint is declared with a non-const target group, hence the const_cast.
    if (!pkRenderer->MapWindowPointToBufferPoint(lWx, lWy, fX, fY, const_cast<NiRenderTargetGroup*>(pkTarget))) {
        return false;
    }
    if (m_kPort.m_right < fX || m_kPort.m_left > fX || m_kPort.m_top < fY || m_kPort.m_bottom > fY) {
        return false;
    }

    float fVy = (fY - m_kPort.m_bottom) / (m_kPort.m_top - m_kPort.m_bottom);
    fVy = fVy * (m_kViewFrustum.m_fTop - m_kViewFrustum.m_fBottom) + m_kViewFrustum.m_fBottom;
    float fVx = (fX - m_kPort.m_left) / (m_kPort.m_right - m_kPort.m_left);
    fVx = fVx * (m_kViewFrustum.m_fRight - m_kViewFrustum.m_fLeft) + m_kViewFrustum.m_fLeft;
    ViewPointToRay(fVx, fVy, kOrigin, kDir);
    return true;
}

// The world vectors and the NiPoint3 products and sums are computed per coordinate, rounding each coordinate to float
// as it is stored; only Unitize is called.
void NiCamera::ViewPointToRay(float fVx, float fVy, NiPoint3& kOrigin, NiPoint3& kDir) {
    const NiMatrix3& kRot = m_kWorld.m_Rotate;
    if (m_kViewFrustum.m_bOrtho) {
        // EXE-INLINED 0x45BBA0
        kDir.x = kRot.m_pEntry[0][0];
        kDir.y = kRot.m_pEntry[1][0];
        kDir.z = kRot.m_pEntry[2][0];
        kDir.Unitize();
        // EXE-INLINED 0x4E9C10
        // EXE-INLINED 0x45BB20
        float fUpX = kRot.m_pEntry[0][1] * fVy;
        float fUpY = kRot.m_pEntry[1][1] * fVy;
        float fUpZ = kRot.m_pEntry[2][1] * fVy;
        // EXE-INLINED 0x4E9C50
        // EXE-INLINED 0x45BB20
        float fRightX = kRot.m_pEntry[0][2] * fVx;
        float fRightY = kRot.m_pEntry[1][2] * fVx;
        float fRightZ = kRot.m_pEntry[2][2] * fVx;
        float fSumX = m_kWorld.m_Translate.x + fRightX;
        float fSumY = m_kWorld.m_Translate.y + fRightY;
        float fSumZ = m_kWorld.m_Translate.z + fRightZ;
        float fOriginX = fSumX + fUpX;
        float fOriginY = fSumY + fUpY;
        float fOriginZ = fSumZ + fUpZ;
        kOrigin.x = fOriginX;
        kOrigin.y = fOriginY;
        kOrigin.z = fOriginZ;
    }
    else {
        // EXE-INLINED 0x4E9C10
        // EXE-INLINED 0x45BB20
        float fUpX = kRot.m_pEntry[0][1] * fVy;
        float fUpY = kRot.m_pEntry[1][1] * fVy;
        float fUpZ = kRot.m_pEntry[2][1] * fVy;
        // EXE-INLINED 0x4E9C50
        // EXE-INLINED 0x45BB20
        float fRightX = kRot.m_pEntry[0][2] * fVx;
        float fRightY = kRot.m_pEntry[1][2] * fVx;
        float fRightZ = kRot.m_pEntry[2][2] * fVx;
        // EXE-INLINED 0x45BBA0
        float fSumX = kRot.m_pEntry[0][0] + fRightX;
        float fSumY = kRot.m_pEntry[1][0] + fRightY;
        float fSumZ = kRot.m_pEntry[2][0] + fRightZ;
        float fDirX = fSumX + fUpX;
        float fDirY = fSumY + fUpY;
        float fDirZ = fSumZ + fUpZ;
        kDir.x = fDirX;
        kDir.y = fDirY;
        kDir.z = fDirZ;
        kDir.Unitize();
        kOrigin = m_kWorld.m_Translate;
    }
}

// The viewport maps to (-1, 1) in x and y and the frustum's depth to (0, 1); the perspective matrix leaves the
// distance along the view direction in w.
void NiCamera::WorldToCameraMatrix() {
    float fDirX = m_kWorld.m_Rotate.m_pEntry[0][0];
    float fDirY = m_kWorld.m_Rotate.m_pEntry[1][0];
    float fDirZ = m_kWorld.m_Rotate.m_pEntry[2][0];
    float fUpX = m_kWorld.m_Rotate.m_pEntry[0][1];
    float fUpY = m_kWorld.m_Rotate.m_pEntry[1][1];
    float fUpZ = m_kWorld.m_Rotate.m_pEntry[2][1];
    float fRightX = m_kWorld.m_Rotate.m_pEntry[0][2];
    float fRightY = m_kWorld.m_Rotate.m_pEntry[1][2];
    float fRightZ = m_kWorld.m_Rotate.m_pEntry[2][2];
    const NiPoint3& kLoc = m_kWorld.m_Translate;

    float fMinusRight = -(kLoc.x * fRightX + kLoc.y * fRightY + kLoc.z * fRightZ);
    float fMinusUp = -(kLoc.x * fUpX + kLoc.y * fUpY + kLoc.z * fUpZ);
    float fMinusDir = -(kLoc.x * fDirX + kLoc.y * fDirY + kLoc.z * fDirZ);

    float fRpL = m_kViewFrustum.m_fRight + m_kViewFrustum.m_fLeft;
    float fTpB = m_kViewFrustum.m_fBottom + m_kViewFrustum.m_fTop;
    float fInvRmL = 1.0f / (m_kViewFrustum.m_fRight - m_kViewFrustum.m_fLeft);
    float fInvTmB = 1.0f / (m_kViewFrustum.m_fTop - m_kViewFrustum.m_fBottom);
    float fInvFmN = 1.0f / (m_kViewFrustum.m_fFar - m_kViewFrustum.m_fNear);

    // Tests the whole byte; bool would assume 0/1.
    if (reinterpret_cast<const uint8_t&>(m_kViewFrustum.m_bOrtho) != 0) {
        float fScaleX = fInvRmL * 2.0;
        float fScaleY = 2.0 * fInvTmB;

        m_aafWorldToCam[0][0] = fRightX * fScaleX;
        m_aafWorldToCam[0][1] = fRightY * fScaleX;
        m_aafWorldToCam[0][2] = fRightZ * fScaleX;
        float fOffsetX = fInvRmL * fRpL;
        m_aafWorldToCam[0][3] = fScaleX * fMinusRight + fOffsetX;

        m_aafWorldToCam[1][0] = fUpX * fScaleY;
        m_aafWorldToCam[1][1] = fUpY * fScaleY;
        m_aafWorldToCam[1][2] = fUpZ * fScaleY;
        float fOffsetY = fInvTmB * fTpB;
        m_aafWorldToCam[1][3] = fScaleY * fMinusUp + fOffsetY;

        m_aafWorldToCam[2][0] = fDirX * fInvFmN;
        m_aafWorldToCam[2][1] = fDirY * fInvFmN;
        m_aafWorldToCam[2][2] = fDirZ * fInvFmN;
        float fOffsetZ = -m_kViewFrustum.m_fNear * fInvFmN;
        m_aafWorldToCam[2][3] = fInvFmN * fMinusDir + fOffsetZ;

        m_aafWorldToCam[3][0] = 0.0f;
        m_aafWorldToCam[3][1] = 0.0f;
        m_aafWorldToCam[3][2] = 0.0f;
        m_aafWorldToCam[3][3] = 1.0f;
    }
    else {
        float fScaleX = fInvRmL * 2.0;
        float fShearX = fInvRmL * -fRpL;
        float fScaleY = 2.0 * fInvTmB;
        float fShearY = fInvTmB * -fTpB;
        float fScaleZ = m_kViewFrustum.m_fFar * fInvFmN;

        m_aafWorldToCam[0][0] = fRightX * fScaleX + fShearX * fDirX;
        m_aafWorldToCam[0][1] = fShearX * fDirY + fScaleX * fRightY;
        m_aafWorldToCam[0][2] = fShearX * fDirZ + fScaleX * fRightZ;
        m_aafWorldToCam[0][3] = fScaleX * fMinusRight + fShearX * fMinusDir;

        m_aafWorldToCam[1][0] = fShearY * fDirX + fScaleY * fUpX;
        m_aafWorldToCam[1][1] = fShearY * fDirY + fScaleY * fUpY;
        m_aafWorldToCam[1][2] = fShearY * fDirZ + fScaleY * fUpZ;
        m_aafWorldToCam[1][3] = fShearY * fMinusDir + fScaleY * fMinusUp;

        m_aafWorldToCam[2][0] = fScaleZ * fDirX;
        m_aafWorldToCam[2][1] = fScaleZ * fDirY;
        m_aafWorldToCam[2][2] = fScaleZ * fDirZ;
        float fOffsetZ = -m_kViewFrustum.m_fNear * fScaleZ;
        m_aafWorldToCam[2][3] = fScaleZ * fMinusDir + fOffsetZ;

        m_aafWorldToCam[3][0] = fDirX;
        m_aafWorldToCam[3][1] = fDirY;
        m_aafWorldToCam[3][2] = fDirZ;
        m_aafWorldToCam[3][3] = fMinusDir;
    }
}

void NiCamera::CopyMembers(NiCamera* pkDest, NiCloningProcess& kCloning) {
    NiAVObject::CopyMembers(pkDest, kCloning);

    memcpy(pkDest->m_aafWorldToCam, m_aafWorldToCam, sizeof(m_aafWorldToCam));
    pkDest->m_kWorld.m_Translate = m_kWorld.m_Translate;
    pkDest->m_kViewFrustum = m_kViewFrustum;
    pkDest->m_kPort = m_kPort;
    pkDest->m_fLODAdjust = m_fLODAdjust;
    pkDest->m_fMinNearPlaneDist = m_fMinNearPlaneDist;
    pkDest->m_fMaxFarNearRatio = m_fMaxFarNearRatio;
}

void NiCamera::ScreenSpaceBoundBound(
    const NiBound& kSBound,
    NiPoint3& kMinBound,
    NiPoint3& kMaxBound,
    float fZeroTolerance
) {
    double v11;
    NiPoint3* v16;
    double v17;
    double v18;
    double v19;
    double v20;
    double v21;
    double v22;
    double v23;
    NiPoint3 v31;
    NiPoint3 result;
    NiPoint3 pt;
    float kMinBoundb;
    float kMinBoundc;
    float kMinBoundd;
    float y = kSBound.m_kCenter.y;
    float m_fRadius = kSBound.m_fRadius;
    bool v8 = !m_kViewFrustum.m_bOrtho;
    float x = kSBound.m_kCenter.x;
    float z = kSBound.m_kCenter.z;
    v31.x = kSBound.m_kCenter.x - m_kWorld.m_Translate.x;
    v31.y = y - m_kWorld.m_Translate.y;
    v31.z = z - m_kWorld.m_Translate.z;
    float v27 = 1.0 / (m_kViewFrustum.m_fFar - m_kViewFrustum.m_fNear);
    result.x = m_kWorld.m_Rotate.m_pEntry[0][0];
    result.y = m_kWorld.m_Rotate.m_pEntry[1][0];
    result.z = m_kWorld.m_Rotate.m_pEntry[2][0];
    float v25 = result.y * v31.y + result.x * v31.x + result.z * v31.z;
    float v26 = v25 - m_kViewFrustum.m_fNear;
    double v10 = m_fRadius;
    if (v8) {
        float kMinBounde = v26 - v10;
        kMinBound.z = kMinBounde * v27;
        float kMinBoundf = v10 + v26;
        v11 = v27 * kMinBoundf;
    }
    else {
        v11 = 0.0;
        kMinBound.z = 0.0;
    }
    kMaxBound.z = v11;
    pt.x = m_kWorld.m_Rotate.m_pEntry[0][1];
    pt.y = m_kWorld.m_Rotate.m_pEntry[1][1];
    pt.z = m_kWorld.m_Rotate.m_pEntry[2][1];
    NiPoint3* v12 = &(result = v31.UnitCross(pt));
    pt.x = v12->x * m_fRadius;
    pt.y = v12->y * m_fRadius;
    pt.z = m_fRadius * v12->z;
    double v13 = pt.x;
    result.x = x - pt.x;
    double v14 = pt.y;
    result.y = y - pt.y;
    double v15 = pt.z;
    result.z = z - pt.z;
    float kMinBounda = m_aafWorldToCam[3][0] * result.x + m_aafWorldToCam[3][1] * result.y +
        result.z * m_aafWorldToCam[3][2] + m_aafWorldToCam[3][3];
    if (!(fZeroTolerance < static_cast<double>(kMinBounda))) {
        goto LABEL_32;
    }
    kMinBound.x = (result.y * m_aafWorldToCam[0][1] + result.x * m_aafWorldToCam[0][0] +
                   m_aafWorldToCam[0][2] * result.z + m_aafWorldToCam[0][3]) /
        kMinBounda;
    pt.x = v13 + x;
    pt.y = v14 + y;
    pt.z = v15 + z;
    kMinBoundb = m_aafWorldToCam[3][1] * pt.y + pt.x * m_aafWorldToCam[3][0] + pt.z * m_aafWorldToCam[3][2] +
        m_aafWorldToCam[3][3];
    if (!(fZeroTolerance < static_cast<double>(kMinBoundb))) {
        goto LABEL_32;
    }
    kMaxBound.x = (pt.y * m_aafWorldToCam[0][1] + pt.x * m_aafWorldToCam[0][0] + pt.z * m_aafWorldToCam[0][2] +
                   m_aafWorldToCam[0][3]) /
        kMinBoundb;
    pt.x = m_kWorld.m_Rotate.m_pEntry[0][2];
    pt.y = m_kWorld.m_Rotate.m_pEntry[1][2];
    pt.z = m_kWorld.m_Rotate.m_pEntry[2][2];
    v16 = &(result = pt.UnitCross(v31));
    pt.x = v16->x * m_fRadius;
    pt.y = v16->y * m_fRadius;
    pt.z = m_fRadius * v16->z;
    v17 = pt.x;
    result.x = x - pt.x;
    v18 = pt.y;
    result.y = y - pt.y;
    v19 = pt.z;
    result.z = z - pt.z;
    kMinBoundc = m_aafWorldToCam[3][1] * result.y + result.x * m_aafWorldToCam[3][0] +
        result.z * m_aafWorldToCam[3][2] + m_aafWorldToCam[3][3];
    if (!(fZeroTolerance < static_cast<double>(kMinBoundc))) {
        kMinBound.y = -1.0f;
        kMinBound.x = -1.0f;
        v23 = 1.0;
        kMaxBound.y = 1.0;
        kMaxBound.x = 1.0;
        goto LABEL_25;
    }
    kMinBound.y = (result.y * m_aafWorldToCam[1][1] + result.x * m_aafWorldToCam[1][0] +
                   m_aafWorldToCam[1][2] * result.z + m_aafWorldToCam[1][3]) /
        kMinBoundc;
    pt.x = v17 + x;
    pt.y = v18 + y;
    pt.z = v19 + z;
    kMinBoundd = m_aafWorldToCam[3][1] * pt.y + pt.x * m_aafWorldToCam[3][0] + pt.z * m_aafWorldToCam[3][2] +
        m_aafWorldToCam[3][3];
    if (!(fZeroTolerance < static_cast<double>(kMinBoundd))) {
    LABEL_32:;
        kMinBound.y = -1.0f;
        kMinBound.x = -1.0f;
        v23 = 1.0;
        kMaxBound.y = 1.0;
        kMaxBound.x = 1.0;
        goto LABEL_25;
    }
    kMaxBound.y = (pt.y * m_aafWorldToCam[1][1] + pt.x * m_aafWorldToCam[1][0] + pt.z * m_aafWorldToCam[1][2] +
                   m_aafWorldToCam[1][3]) /
        kMinBoundd;
    v20 = -1.0f;
    v21 = 1.0;
    if (!(kMinBound.x < -1.0f)) {
        if (kMaxBound.x > 1.0) {
            kMaxBound.x = 1.0;
            if (kMinBound.x > 1.0) {
                kMinBound.x = 1.0;
            }
        }
    }
    else {
        v22 = -1.0f;
        v23 = 1.0;
        kMinBound.x = -1.0f;
        if (kMaxBound.x < -1.0f) {
            kMaxBound.x = -1.0f;
            goto LABEL_17;
        }
        v21 = 1.0;
        v20 = -1.0f;
        if (kMaxBound.x > 1.0) {
            kMaxBound.x = 1.0;
        }
    }
    v22 = v20;
    v23 = v21;
LABEL_17:;
    if (!(v22 > kMinBound.y)) {
        if (v23 < kMaxBound.y) {
            kMaxBound.y = v23;
            if (v23 < kMinBound.y) {
                kMinBound.y = v23;
            }
        }
    }
    else {
        kMinBound.y = v22;
        if (!(v22 > kMaxBound.y)) {
            if (v23 < kMaxBound.y) {
                kMaxBound.y = v23;
            }
        }
        else {
            kMaxBound.y = v22;
        }
    }
LABEL_25:;
    if (!(kMinBound.z < 0.0)) {
        if (v23 < kMaxBound.z) {
            kMaxBound.z = v23;
            if (v23 < kMinBound.z) {
                kMinBound.z = v23;
            }
        }
    }
    else {
        kMinBound.z = 0.0;
        if (kMaxBound.z < 0.0) {
            v23 = 0.0;
        LABEL_28:;
            kMaxBound.z = v23;
            return;
        }
        if (v23 < kMaxBound.z) {
            goto LABEL_28;
        }
    }
}

// The Eyefinity group is drawn at 4:3 per screen: the frustum's width is scaled by 0.75 and by one screen's aspect
// ratio, and the vertical field of view is afScale times fDefaultFOV in radians, widened by 1.1 and capped at
// 1.53938043117523193359375. Compares and rounds in double; NaN takes the cap.
void NiCamera::HandleEyeFinityFOVScale(NiCamera* apCamera, float afScale) {
    if (!apCamera) {
        return;
    }
    if (!Renderer::is_bEyeFinity()) {
        return;
    }
    float fAspectRatio = static_cast<float>(retn_0_75());
    apCamera->AdjustAspectRatio(fAspectRatio, false);
    double dWidth = Renderer::Get_1280();
    float fScreenRatio = static_cast<float>(dWidth / Renderer::Get_720());
    double dFOV;
    if (afScale * BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369049072265625 * 1.10000002384185791015625 <
        1.53938043117523193359375) {
        dFOV =
            afScale * BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369049072265625 * 1.10000002384185791015625;
    }
    else {
        dFOV = 1.53938043117523193359375;
    }
    float fFOV = static_cast<float>(dFOV);
    NiFrustum kFrustum;
    kFrustum.~NiFrustum();
    ::new (&kFrustum) NiFrustum(false);
    kFrustum.m_fLeft = -std::tan(fFOV) * fAspectRatio * fScreenRatio;
    kFrustum.m_fRight = std::tan(fFOV) * fAspectRatio * fScreenRatio;
    kFrustum.m_fBottom = -std::tan(fFOV) * fAspectRatio;
    kFrustum.m_fTop = std::tan(fFOV) * fAspectRatio;
    kFrustum.m_bOrtho = false;
    kFrustum.m_fNear = 1.0f;
    kFrustum.m_fFar = 5000.0f;
    apCamera->SetViewFrustum(kFrustum);
}

double NiCamera::retn_0_75() {
    return 0.75f;
}

NiObject* NiCamera::CreateObject() {
    void* v0 = NiMemObject::operator new(sizeof(NiCamera));
    if (v0) {
        return ::new (v0) NiCamera;
    }
    else {
        return nullptr;
    }
}
