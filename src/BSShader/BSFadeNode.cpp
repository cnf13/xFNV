#include "BSShader/BSFadeNode.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSMain/BSPortal.h"
#include "BSMain/BSXFlags.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderUtil.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cmath>

NiRTTI BSFadeNode::ms_RTTI("BSFadeNode", &NiNode::ms_RTTI);
BSFadeNode::MinNearDistanceSetting BSFadeNode::iMinNearDistance("iFadeNodeMinNearDistance:LOD", 400);
NiPointer<NiAlphaProperty> BSFadeNode::spFadeAlpha;
SettingT<INISettingCollection> gs_fLODMultTrees_LOD("fLODMultTrees:LOD", 1.2f);

// Fade distances at the float maximum, no fade yet, and the faded-in, ignore-fade and moving-sound flags cleared.
BSFadeNode::BSFadeNode()
  : NiNode(0), fNearDistSqr(3.4028235e+38f), fFarDistSqr(3.4028235e+38f), fLastFade(0.0f), fCurrentFade(0.0f),
    fBoundRadius(0.0f), fTimeSinceUpdate(0.0f), eMultType(LOD_MULT_NONE), pUserData(nullptr), pBoundArray(nullptr) {
    m_uFlags &= ~(NiAVObject::FADED_IN | NiAVObject::IGNORE_FADE | NiAVObject::HAS_MOVING_SOUND);
}

// Clears both parent lists; the lists and the node destroy themselves.
BSFadeNode::~BSFadeNode() {
    ParentRoomList.RemoveAll();
    ParentPortalList.RemoveAll();
}

// Clones the node, then copies the two fade distances and the LOD multiplier type. The clone's fade-state flags
// (FADED_IN, IGNORE_FADE and HAS_MOVING_SOUND) are cleared in one 4-byte store.
NiObject* BSFadeNode::CreateClone(NiCloningProcess& arCloneData) {
    BSFadeNode* pClone = static_cast<BSFadeNode*>(NiMemObject::operator new(sizeof(BSFadeNode)));
    if (pClone) {
        pClone = ::new (pClone) BSFadeNode;
    }
    NiNode::CopyMembers(pClone, arCloneData);
    pClone->m_uFlags = static_cast<NiAVObject::NiFlags>(
        pClone->m_uFlags & ~(NiAVObject::FADED_IN | NiAVObject::IGNORE_FADE | NiAVObject::HAS_MOVING_SOUND)
    );
    pClone->fNearDistSqr = fNearDistSqr;
    pClone->fFarDistSqr = fFarDistSqr;
    pClone->eMultType = eMultType;
    return pClone;
}

void BSFadeNode::LoadBinary(NiStream& arStream) {
    NiNode::LoadBinary(arStream);
}

void BSFadeNode::LinkObject(NiStream& arStream) {
    NiNode::LinkObject(arStream);
}

bool BSFadeNode::RegisterStreamables(NiStream& arStream) {
    return NiNode::RegisterStreamables(arStream);
}

void BSFadeNode::SaveBinary(NiStream& arStream) {
    NiNode::SaveBinary(arStream);
}

bool BSFadeNode::IsEqual(NiObject* apObject) {
    return NiNode::IsEqual(apObject);
}

void BSFadeNode::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiNode::GetViewerStrings(pkStrings);
    char* ViewerString = NiGetViewerString(BSFadeNode::ms_RTTI.GetName());
    uint32_t m_usSize = pkStrings->m_usSize;
    uint32_t m_usMaxSize = pkStrings->m_usMaxSize;
    char* v42 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        pkStrings->SetSize(m_usSize + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(m_usSize, v42);
    float fNearDistSqr = this->fNearDistSqr;
    float fFarDistSqr = this->fFarDistSqr;
    char* message = static_cast<char*>(MemoryManager_Singleton.Allocate(64u));
    v42 = message;
    if (fNearDistSqr == 3.4028234663852886e+38) {
        BSsprintf(message, 64, "fNearDistSqr = DISABLED");
    }
    else {
        BSsprintf(message, 64, "fNearDistSqr = %.2f", fNearDistSqr);
    }
    pkStrings->Add(v42);
    char* message_2 = static_cast<char*>(MemoryManager_Singleton.Allocate(64u));
    v42 = message_2;
    if (fFarDistSqr == 3.4028234663852886e+38) {
        BSsprintf(message_2, 64, "fFarDistSqr = DISABLED");
    }
    else {
        BSsprintf(message_2, 64, "fFarDistSqr = %.2f", fFarDistSqr);
    }
    pkStrings->Add(v42);
    char* message_3 = static_cast<char*>(MemoryManager_Singleton.Allocate(64u));
    ENUM_LOD_MULT eMultType = this->eMultType;
    v42 = message_3;
    BSsprintf(message_3, 64, "eMultType = %d", eMultType);
    pkStrings->Add(v42);
    char* ViewerString_7 = NiGetViewerString("fLastFade", fLastFade);
    uint32_t v14 = pkStrings->m_usSize;
    uint32_t v15 = pkStrings->m_usMaxSize;
    v42 = ViewerString_7;
    if (v14 >= v15) {
        pkStrings->SetSize(v14 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v14, v42);
    char* v16 = NiGetViewerString("fCurrentFade", fCurrentFade);
    uint32_t v17 = pkStrings->m_usSize;
    uint32_t v18 = pkStrings->m_usMaxSize;
    v42 = v16;
    if (v17 >= v18) {
        pkStrings->SetSize(v17 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v17, v42);
    char* v19 = NiGetViewerString("fTimeSinceUpdate", fTimeSinceUpdate);
    uint32_t v20 = pkStrings->m_usSize;
    uint32_t v21 = pkStrings->m_usMaxSize;
    v42 = v19;
    if (v20 >= v21) {
        pkStrings->SetSize(v20 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v20, v42);
    char* ViewerString_5 = NiGetViewerString("iFrameCounter", iFrameCounter);
    uint32_t v23 = pkStrings->m_usSize;
    v42 = ViewerString_5;
    if (v23 >= pkStrings->m_usMaxSize) {
        pkStrings->SetSize(v23 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v23, v42);
    char* ViewerString_0 = NiGetViewerString("bFadedIn", (m_uFlags & NiAVObject::FADED_IN) != 0);
    uint32_t v25 = pkStrings->m_usSize;
    uint32_t v26 = pkStrings->m_usMaxSize;
    v42 = ViewerString_0;
    if (v25 >= v26) {
        pkStrings->SetSize(v25 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v25, v42);
    char* v27 = NiGetViewerString("bIgnoreFade", (m_uFlags & NiAVObject::IGNORE_FADE) != 0);
    uint32_t v28 = pkStrings->m_usSize;
    uint32_t v29 = pkStrings->m_usMaxSize;
    v42 = v27;
    if (v28 >= v29) {
        pkStrings->SetSize(v28 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v28, v42);
    char* v30 = NiGetViewerString("bLODFadingOut", (m_uFlags & NiAVObject::LOD_FADING_OUT) != 0);
    uint32_t v31 = pkStrings->m_usSize;
    v42 = v30;
    if (v31 >= pkStrings->m_usMaxSize) {
        pkStrings->SetSize(v31 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v31, v42);
    char* v32 = NiGetViewerString("bHasMovingSound", (m_uFlags & NiAVObject::HAS_MOVING_SOUND) != 0);
    uint32_t v33 = pkStrings->m_usSize;
    uint32_t v34 = pkStrings->m_usMaxSize;
    v42 = v32;
    if (v33 >= v34) {
        pkStrings->SetSize(v33 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v33, v42);
    char* v35 = NiGetViewerString("bHighDetail", (m_uFlags & NiAVObject::HIGH_DETAIL) != 0);
    uint32_t v36 = pkStrings->m_usSize;
    uint32_t v37 = pkStrings->m_usMaxSize;
    v42 = v35;
    if (v36 >= v37) {
        pkStrings->SetSize(v36 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v36, v42);
    NiExtraData* pextraData = GetExtraData(*BSXFlags::pTag);
    if (pextraData) {
        pextraData->GetViewerStrings(pkStrings);
    }
}

void BSFadeNode::UpdateDownwardPass(NiUpdateData& arUpdateData, uint32_t aiFlags) {
    NiNode::UpdateDownwardPass(arUpdateData, aiFlags);
}

// Stamps the update time while fading is off. The first child's controller manager is looked up for an idle
// (inactive rigid body) node, but the result is unused.
void BSFadeNode::UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    if (!bFadeEnabled) {
        fTimeSinceUpdate = UpdateData.fTime;
        if (UpdateData.fTime > 0.0f && !bhkUtilFunctions::IsRigidBodyActive(this) && m_kChildren.m_usSize > 0) {
            NiAVObject* pChild = m_kChildren.m_pBase[0].m_pObject;
            if (pChild && pChild->m_spControllers) {
                pChild->m_spControllers->DynamicCastMember(&NiControllerManager::ms_RTTI);
            }
        }
    }
    NiNode::UpdateSelectedDownwardPass(UpdateData, uFlags);
}

void BSFadeNode::OnVisible(NiCullingProcess& arCuller) {
    uint32_t m_uFlags;
    ENUM_LOD_MULT eMultType;
    BSShaderAccumulator* pbsAccum;
    float v28;
    if (!BSFadeNode::bFadeEnabled) {
        goto LABEL_52;
    }
    m_uFlags = this->m_uFlags;
    if ((this->m_uFlags & NiAVObject::IGNORE_FADE) != 0) {
        goto LABEL_52;
    }
    eMultType = this->eMultType;
    if (eMultType == LOD_MULT_DISTANTLOD) {
        if ((m_uFlags & NiAVObject::FADED_IN) == 0) {
            float afA = appTimer.fDelta / BSFadeNode::fFadeInTime;
            float v26 = BSMIN(afA, BSFadeNode::fMaxFadeIncrement) + fCurrentFade;
            float x1 = BSMIN(1.0f, v26);
            fCurrentFade = x1;
            BSShaderUtil::RecursiveSetPropertyFadeAlpha(this, x1);
            fLastFade = fCurrentFade;
            if (fCurrentFade >= 1.0f) {
                this->m_uFlags |= NiAVObject::FADED_IN;
            }
        }
    LABEL_52:;
        NiNode::OnVisible(arCuller);
        return;
    }
    float v27 = BSFadeNode::pfFadeOutMultiplierA[eMultType];
    NiCamera* m_pkCamera = arCuller.m_pkCamera;
    if (!(v27 > 0.0f)) {
        v28 = 0.2f;
        this->m_uFlags = m_uFlags | NiAVObject::FADED_IN;
        fCurrentFade = 1.0f;
        fLastFade = 1.0f;
        BSShaderUtil::RecursiveSetPropertyFadeAlpha(this, fCurrentFade);
    }
    else {
        v28 = m_pkCamera->m_fLODAdjust / v27;
    }
    float x1a = m_kWorld.m_Translate.x - m_pkCamera->m_kWorld.m_Translate.x;
    float x1_4 = m_kWorld.m_Translate.y - m_pkCamera->m_kWorld.m_Translate.y;
    float zDiff = m_kWorld.m_Translate.z - m_pkCamera->m_kWorld.m_Translate.z;
    float x1b = x1_4 * x1_4 + x1a * x1a + zDiff * zDiff;
    float x1c = std::sqrt((double)x1b);
    float v29 = x1c * v28;
    float ffadeperc = 1.0f;
    float v31 = fNearDistSqr * BSFadeNode::fDistanceMultiplier;
    double v6 = v31;
    if (v31 < static_cast<double>(v29)) {
        double v7 = v31;
        float v32 = BSFadeNode::fDistanceMultiplier * fFarDistSqr;
        ffadeperc = 1.0 - (v29 - v6) / (v32 - v7);
    }
    ENUM_LOD_MULT v8 = this->eMultType;
    if (v8 == LOD_MULT_DIST_OVERRIDE) {
        if ((this->m_uFlags & NiAVObject::IMPOSTER_LOADED) != 0) {
            ffadeperc = 1.0f;
        }
        else {
            ffadeperc = 0.0f;
        }
    }
    double v9 = ffadeperc;
    if (v8 != LOD_MULT_RENDEREDMENU && iFrameCounter < BSFadeNode::iFadeCounter - 20 && fBoundRadius < 600.0f) {
        if (BSFadeNode::fFadeInAlphaThreshold < v9 ||
            ((this->m_uFlags & NiAVObject::FADED_IN) != 0 && BSFadeNode::fFadeOutAlphaThreshold < v9)) {
            BSShaderUtil::RecursiveSetPropertyFadeAlpha(this, 1.0f);
            fCurrentFade = 1.0f;
            this->m_uFlags |= NiAVObject::FADED_IN;
            fLastFade = 1.0f;
        }
        else {
            BSShaderUtil::RecursiveSetPropertyFadeAlpha(this, 0.0f);
            fCurrentFade = 0.0f;
            this->m_uFlags &= ~NiAVObject::FADED_IN;
            fLastFade = 0.0f;
        }
    }
    iFrameCounter = BSFadeNode::iFadeCounter;
    float fmaxA = GetMaxAlpha();
    double fmaxA_2 = fmaxA;
    char v11 = 0;
    if ((this->m_uFlags & NiAVObject::FADED_IN) == 0 || (!(fmaxA_2 > 0.0) || !(fmaxA_2 < 1.0))) {
        double v15;
        double v14 = ffadeperc;
        if ((this->m_uFlags & NiAVObject::FADED_IN) == 0 && v14 > BSFadeNode::fFadeInAlphaThreshold) {
            goto LABEL_37;
        }
        if (!(fCurrentFade > 0.0f) || !(fCurrentFade < 1.0f)) {
            v15 = 0.0f;
        }
        else {
            v15 = 0.0f;
            if (BSFadeNode::fFadeInAlphaThreshold < v14) {
            LABEL_37:;
                float v33 = appTimer.fDelta / BSFadeNode::fFadeInTime;
                float v34 = BSMIN(v33, BSFadeNode::fMaxFadeIncrement) + fCurrentFade;
                float x1d = BSMIN(1.0f, v34);
                fCurrentFade = x1d;
                BSShaderUtil::RecursiveSetPropertyFadeAlpha(this, x1d);
                fLastFade = fCurrentFade;
                if (fCurrentFade >= 1.0f) {
                    this->m_uFlags |= NiAVObject::FADED_IN;
                }
                goto LABEL_47;
            }
        }
        if (((this->m_uFlags & NiAVObject::FADED_IN) != 0 && BSFadeNode::fFadeOutAlphaThreshold > v14) ||
            (fCurrentFade < 1.0f && (v15 < fCurrentFade && v14 < BSFadeNode::fFadeOutAlphaThreshold))) {
            double x1e = fCurrentFade;
            float v35 = appTimer.fDelta / BSFadeNode::fFadeOutTime;
            float v36 = x1e - BSMIN(v35, BSFadeNode::fMaxFadeIncrement);
            float v37 = BSMAX(0.0f, v36);
            fCurrentFade = v37;
            BSShaderUtil::RecursiveSetPropertyFadeAlpha(this, v37);
            fLastFade = fCurrentFade;
            if (fCurrentFade <= 0.0f) {
                this->m_uFlags &= ~NiAVObject::FADED_IN;
            }
        }
        goto LABEL_47;
    }
    pbsAccum = static_cast<BSShaderAccumulator*>(NiRenderer::ms_pkRenderer->m_spAccum.m_pObject);
    if (NiObject::IsKindOf_0(&BSShaderAccumulator::ms_RTTI, pbsAccum)) {
        BSNiBound* m_kWorldBound = this->m_kWorldBound;
        if (!m_kWorldBound) {
            m_kWorldBound = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
        }
        pbsAccum->StartGroupingAlphas(*m_kWorldBound, true);
        v11 = 1;
        BSShaderUtil::RecursiveSetPropertyFadeAlpha(this, fmaxA);
        fCurrentFade = fmaxA;
        fLastFade = fmaxA;
    }
LABEL_47:;
    if (fCurrentFade > 0.0f && 0.0f != fmaxA) {
        NiNode::OnVisible(arCuller);
    }
    if (v11) {
        static_cast<BSShaderAccumulator*>(NiRenderer::ms_pkRenderer->m_spAccum.m_pObject)
            ->BSShaderAccumulator::StopGroupingAlphas();
    }
}

void BSFadeNode::SetFadingFlag(bool abFadedIn) {
    NiAVObject::SetFadedIn(abFadedIn);
}

void BSFadeNode::TurnFadeNodeOn() {
    fCurrentFade = 1.0f;
    fLastFade = 1.0f;
    SetFadingFlag(true);
}

// Reads the setting's value field directly, not through Setting::Int, which tests a null setting.
void BSFadeNode::SetRange(float afNearDist, float afFarDist) {
    float fMinNearDistance = float(iMinNearDistance.uValue.i);
    if (fMinNearDistance > afNearDist) {
        afNearDist = fMinNearDistance;
        float fMinFarDistance = fMinNearDistance + fMinNearDistance;
        if (afFarDist < fMinFarDistance) {
            afFarDist = fMinFarDistance;
        }
    }
    fNearDistSqr = afNearDist;
    fFarDistSqr = afFarDist;
}

uint32_t BSFadeNode::QFadingFlag() {
    return QFadedIn();
}

bool BSFadeNode::CheckFadeRadius(NiCamera* apCamera, bool abNoFade) {
    double v3 = 1.0;
    if (BSFadeNode::bFadeEnabled) {
        NiPoint3 kDiff;
        ENUM_LOD_MULT eMultType = this->eMultType;
        float fLODAdjust = apCamera->m_fLODAdjust;
        kDiff.x = m_kWorld.m_Translate.x - apCamera->m_kWorld.m_Translate.x;
        kDiff.y = m_kWorld.m_Translate.y - apCamera->m_kWorld.m_Translate.y;
        kDiff.z = m_kWorld.m_Translate.z - apCamera->m_kWorld.m_Translate.z;
        float apCameraa = 1.0f;
        if (eMultType > LOD_MULT_NONE && eMultType < LOD_MULT_TOTAL) {
            apCameraa = BSFadeNode::pfFadeOutMultiplierA[eMultType];
        }
        if (eMultType == LOD_MULT_DISTANTLOD) {
            goto LABEL_10;
        }
        if (eMultType == LOD_MULT_DIST_OVERRIDE) {
            return true;
        }
        if (fFarDistSqr > 2600.0f) {
            fFarDistSqr = 2600.0f;
            fNearDistSqr = 1300.0f;
            if (iFrameCounter < BSFadeNode::iFadeCounter - 20) {
            LABEL_10:;
                m_uFlags &= ~NiAVObject::FADED_IN;
                fCurrentFade = 0.0f;
                fLastFade = 0.0f;
            }
        }
        BSNiBound* m_kWorldBound = this->m_kWorldBound;
        if (m_kWorldBound) {
            fBoundRadius = static_cast<NiBound&>(*m_kWorldBound).m_fRadius;
        }
        if (eMultType == LOD_MULT_OBJECTS ||
            (eMultType == LOD_MULT_LANDSCAPE || (eMultType == LOD_MULT_TREES || eMultType == LOD_MULT_DISTANTLOD))) {
            iFrameCounter = BSFadeNode::iFadeCounter;
            return false;
        }
        if (apCameraa > 0.0f) {
            float fLODAdjusta = fLODAdjust / apCameraa;
            float fdiff = kDiff.Length();
            if (fdiff < 0.0f) {
                fdiff = 0.0f;
            }
            float apCamerab = fdiff * fLODAdjusta;
            float v14 = fNearDistSqr * BSFadeNode::fDistanceMultiplier;
            float fLODAdjustb = BSFadeNode::fDistanceMultiplier * fFarDistSqr;
            double v8 = apCamerab;
            if (fLODAdjustb < static_cast<double>(apCamerab)) {
                return false;
            }
            double v9 = v14;
            if (v14 < v8) {
                float apCamerac = fLODAdjustb - v9;
                float apCamerad = 1.0 - (v8 - v9) / apCamerac;
                if (BSFadeNode::fFadeOutAlphaThreshold > static_cast<double>(apCamerad)) {
                    return false;
                }
            }
            v3 = 1.0;
        }
    }
    m_uFlags |= NiAVObject::FADED_IN;
    fCurrentFade = v3;
    fLastFade = v3;
    BSShaderUtil::RecursiveSetPropertyFadeAlpha(this, v3);
    return true;
}

void BSFadeNode::SetLODMultType(ENUM_LOD_MULT aeMultType) {
    eMultType = aeMultType;
}

void BSFadeNode::ForceLODMultType(ENUM_LOD_MULT aeMultType) {
    eMultType = aeMultType;
}

ENUM_LOD_MULT BSFadeNode::GetMultType() const {
    return eMultType;
}

float BSFadeNode::GetLastFadePercent() const {
    return fLastFade;
}

TESObjectREFR* BSFadeNode::GetLinkedObj() const {
    return pUserData;
}

float BSFadeNode::GetMaxAlpha() {
    if (!pMaxAlphaAccessFunc) {
        return 1.0f;
    }
    float fAlpha = pMaxAlphaAccessFunc(this);
    return fAlpha;
}

void BSFadeNode::SetUserData(void* apData) {
    pUserData = static_cast<TESObjectREFR*>(apData);
}

uint32_t BSFadeNode::QHasMovingSound() {
    return NiAVObject::QHasMovingSound();
}

void BSFadeNode::TurnFadeNodeOff() {
    fCurrentFade = (float)0.0;
    fLastFade = (float)0.0;
    SetFadingFlag(false);
}

bool BSFadeNode::ShouldUpdate(NiCamera* apCamera, float afDistMult) {
    ENUM_LOD_MULT eMultType = this->eMultType;
    float lodAdjust = apCamera->m_fLODAdjust;
    if (eMultType == LOD_MULT_INVISIBLE) {
        return false;
    }
    if (static_cast<uint32_t>(eMultType) <= LOD_MULT_DIST_OVERRIDE) {
        float apCameraa = BSFadeNode::pfFadeOutMultiplierA[eMultType];
        if (apCameraa != 0.0) {
            lodAdjust = lodAdjust / apCameraa;
        }
    }
    float x_diff = m_kWorld.m_Translate.x - apCamera->m_kWorld.m_Translate.x;
    float y_diff = m_kWorld.m_Translate.y - apCamera->m_kWorld.m_Translate.y;
    float z_diff = m_kWorld.m_Translate.z - apCamera->m_kWorld.m_Translate.z;
    float apCamerac = x_diff * x_diff + y_diff * y_diff + z_diff * z_diff;
    float apCamerad = std::sqrt((double)apCamerac);
    float apCamerab = apCamerad * afDistMult;
    if (apCamerab < 0.0) {
        apCamerab = 0.0;
    }
    float apCamerae = apCamerab * lodAdjust;
    return !(fFarDistSqr * BSFadeNode::fDistanceMultiplier < apCamerae);
}

void BSFadeNode::SetFadeInTime(float afFadeTime) {
    fFadeInTime = afFadeTime;
}

void BSFadeNode::SetFadeOutTime(float afFadeTime) {
    fFadeOutTime = afFadeTime;
}

void BSFadeNode::SetFadeInThreshold(float afFadeIn) {
    fFadeInAlphaThreshold = afFadeIn;
}

void BSFadeNode::SetFadeOutThreshold(float afFadeOut) {
    fFadeOutAlphaThreshold = afFadeOut;
}

void BSFadeNode::SetDistanceMultiplier(float afDistanceMultiplier) {
    fDistanceMultiplier = afDistanceMultiplier;
}

void BSFadeNode::SetLODFadeOutObjectMult(float afMultiplier) {
    fLODFadeOutObjectMult = afMultiplier;
}

void BSFadeNode::AddMovingSoundCallback(NiAVObject* apObj) {
    if (!apObj) {
        return;
    }
    BSFadeNode* pFadeNode = apObj->IsFadeNode();
    if (pFadeNode) {
        // ORs the whole flags word.
        uint32_t& rFlags = pFadeNode->m_uFlags;
        rFlags = rFlags | NiAVObject::HAS_MOVING_SOUND;
    }
}

void BSFadeNode::SetLODFadeOutItemMult(float afMultiplier) {
    fLODFadeOutItemMult = afMultiplier;
}

void BSFadeNode::SetLODFadeOutActorMult(float afMultiplier) {
    fLODFadeOutActorMult = afMultiplier;
}

float BSFadeNode::QFadeOutMultiplier(ENUM_LOD_MULT aeType) {
    return pfFadeOutMultiplierA[aeType];
}

void BSFadeNode::SetFadeOutMultiplier(ENUM_LOD_MULT aeType, float afValue) {
    pfFadeOutMultiplierA[aeType] = afValue;
}

float BSFadeNode::GetLinkedActorAlpha(BSFadeNode* apNode) {
    TESObjectREFR* pRef = apNode->GetLinkedObj();
    if (pRef && pRef->IsActor()) {
        return static_cast<Actor*>(pRef)->GetAlpha();
    }
    return 1.0f;
}

void BSFadeNode::SetMaxAlphaAccessFunc(MaxAlphaAccessFunc apFunc) {
    pMaxAlphaAccessFunc = apFunc;
}

bool BSFadeNode::IsFadeEnabled() {
    return BSFadeNode::bFadeEnabled;
}

void BSFadeNode::SetFadeEnabled(bool abVal) {
    BSFadeNode::bFadeEnabled = (bool)((uint8_t)abVal);
}

void BSFadeNode::IncFadeCounter() {
    ++BSFadeNode::iFadeCounter;
}

// Creates the shared fade alpha property, holds it in spFadeAlpha, then interns the fade controller sequence's name.
void BSFadeNode::InitSDM() {
    NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(NiMemObject::operator new(sizeof(NiAlphaProperty)));
    if (pAlpha) {
        // SRCALPHA source blend and INVSRCALPHA destination blend, alpha test off.
        ::new (pAlpha) NiAlphaProperty;
    }
    else {
        pAlpha = nullptr;
    }

    spFadeAlpha = pAlpha;
    // Sets the bit with a 16-bit `or`, which rewrites both bytes.
    uint16_t& rusFlags = spFadeAlpha->m_uFlags;
    rusFlags |= NiAlphaProperty::ALPHA_BLEND_MASK;

    NiFixedString* pName = static_cast<NiFixedString*>(MemoryManager_Singleton.Allocate(sizeof(NiFixedString)));
    if (pName) {
        pName->SetHandle(NiGlobalStringTable::AddString("FadeNode Anim"));
        pFadeNodeAnimName = pName;
    }
    else {
        pFadeNodeAnimName = nullptr;
    }
}

// Allocates a fade node (0xE4 bytes) and runs its constructor; the stream's create-object callback.
NiObject* BSFadeNode::CreateObject() {
    void* pMemory = NiMemObject::operator new(sizeof(BSFadeNode));
    if (pMemory) {
        return ::new (pMemory) BSFadeNode;
    }
    return nullptr;
}

void BSFadeNode::KillSDM() {
    BSFadeNode::spFadeAlpha = nullptr;
    NiFixedString* v1 = BSFadeNode::pFadeNodeAnimName;
    if (BSFadeNode::pFadeNodeAnimName) {
        // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
        NiGlobalStringTable::ReleaseHandle(BSFadeNode::pFadeNodeAnimName->GetHandle());
        MemoryManager_Singleton.Deallocate(v1);
    }
    BSFadeNode::pFadeNodeAnimName = nullptr;
}
