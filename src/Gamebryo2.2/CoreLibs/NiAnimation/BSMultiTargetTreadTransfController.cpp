#include "Gamebryo2.2/CoreLibs/NiAnimation/BSMultiTargetTreadTransfController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/BSBlendTreadTransfInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSTreadTransfInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

#include <cstring>
#include <limits>

NiRTTI BSMultiTargetTreadTransfController::ms_RTTI("BSMultiTargetTreadTransfController", &NiInterpController::ms_RTTI);

BSMultiTargetTreadTransfController::BSMultiTargetTreadTransfController()
  : NiInterpController(), BlendInterpsA(nullptr), usNumInterps(0), pLinksA(nullptr), usNumLinks(0) {}

BSMultiTargetTreadTransfController::BSMultiTargetTreadTransfController(uint16_t ausCount, uint16_t ausLinkCount)
  : NiInterpController(), BlendInterpsA(nullptr), usNumInterps(0), pLinksA(nullptr), usNumLinks(0) {
    AllocateStorage(ausCount, ausLinkCount);
}

BSMultiTargetTreadTransfController::~BSMultiTargetTreadTransfController() {
    if (BlendInterpsA) {
        delete[] BlendInterpsA;
    }
    for (uint16_t us = 0; us < usNumLinks; ++us) {
        MemoryManager::Instance().Deallocate(pLinksA[us]);
    }
    _NiFree(pLinksA);
}

const NiRTTI* BSMultiTargetTreadTransfController::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* BSMultiTargetTreadTransfController::CreateClone(NiCloningProcess& kCloning) {
    BSMultiTargetTreadTransfController* pkObject = new BSMultiTargetTreadTransfController;
    CopyMembers(pkObject, kCloning);
    return pkObject;
}

// Only the base's part is streamed.
void BSMultiTargetTreadTransfController::LoadBinary(NiStream& arStream) {
    NiInterpController::LoadBinary(arStream);
}

void BSMultiTargetTreadTransfController::LinkObject(NiStream& arStream) {
    NiInterpController::LinkObject(arStream);
}

bool BSMultiTargetTreadTransfController::RegisterStreamables(NiStream& arStream) {
    return NiInterpController::RegisterStreamables(arStream);
}

void BSMultiTargetTreadTransfController::SaveBinary(NiStream& arStream) {
    NiInterpController::SaveBinary(arStream);
}

bool BSMultiTargetTreadTransfController::IsEqual(NiObject* apObject) {
    return NiInterpController::IsEqual(apObject);
}

// Each blend interpolator that updates gives a lerp; every selectively updated link target of that interpolator gets
// its link's Start and End transforms mixed by it (End at a lerp of 1). The scale is left alone.
void BSMultiTargetTreadTransfController::Update(NiUpdateData& UpdateData) {
    if (!(m_usFlags & ACTIVE_MASK)) {
        return;
    }

    NiQuatTransform kTransform;
    kTransform.m_fScale = -std::numeric_limits<float>::max();
    kTransform.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
    kTransform.m_kRotate = NiQuatTransform::INVALID_ROTATE;
    for (uint16_t i = 0; i < usNumInterps; i++) {
        if (!BlendInterpsA[i].Update(UpdateData.fTime, nullptr, kTransform)) {
            continue;
        }

        float fLerp = BlendInterpsA[i].fCurrLerp;
        for (uint16_t j = 0; j < usNumLinks; j++) {
            LinkAVObj* pkLinkObj = pLinksA[j];
            if (pkLinkObj->usBlendIdx != i) {
                continue;
            }

            NiAVObject* pkTarget = pkLinkObj->pTarget;
            if (!pkTarget || !(pkTarget->m_uFlags & NiAVObject::SELECTIVE_UPDATE)) {
                continue;
            }

            BSTreadTransfInterpolator::LinkInfo* pkLink = pkLinkObj->pLink;
            float fStartWeight = 1.0f - fLerp;
            NiPoint3 kTranslate = pkLink->Start.m_kTranslate * fStartWeight + pkLink->End.m_kTranslate * fLerp;
            NiQuaternion kRotate;
            kRotate.Slerp(fStartWeight, pkLink->End.m_kRotate, pkLink->Start.m_kRotate);
            pLinksA[j]->pTarget->m_kLocal.m_Translate = kTranslate;
            kRotate.ToRotation(pLinksA[j]->pTarget->m_kLocal.m_Rotate);
        }
    }
}

const char* BSMultiTargetTreadTransfController::GetInterpolatorID(uint16_t ausIndex) {
    return pLinksA[ausIndex]->pTarget->m_kName;
}

// The index of the first link whose target is named apcID.
uint16_t BSMultiTargetTreadTransfController::GetInterpolatorIndex(const char* pcID) const {
    if (!pcID) {
        return INVALID_INDEX;
    }

    for (uint16_t i = 0; i < usNumLinks; i++) {
        NiAVObject* pkTarget = pLinksA[i]->pTarget;
        if (pkTarget && strcmp(pkTarget->m_kName, pcID) == 0) {
            return i;
        }
    }

    return INVALID_INDEX;
}

// As GetInterpolatorIndex, comparing the fixed strings' handles. The search of the links is bounded by the interpolator
// count, not the link count.
uint16_t BSMultiTargetTreadTransfController::GetInterpolatorIndexFx(const NiFixedString& arName) const {
    for (uint16_t i = 0; i < usNumInterps; i++) {
        NiAVObject* pkTarget = pLinksA[i]->pTarget;
        if (pkTarget && pkTarget->m_kName == arName) {
            return i;
        }
    }

    return INVALID_INDEX;
}

NiInterpolator* BSMultiTargetTreadTransfController::GetInterpolator(uint16_t ausIndex) const {
    return &BlendInterpsA[ausIndex];
}

// The blend interpolator at ausIndex; no new one is built.
NiBlendInterpolator*
BSMultiTargetTreadTransfController::CreateBlendInterpolator(uint16_t ausIndex, int32_t, bool, float, int32_t) const {
    return &BlendInterpsA[ausIndex];
}

void BSMultiTargetTreadTransfController::GuaranteeTimeRange(float afStartTime, float afEndTime) {
    for (uint16_t i = 0; i < usNumInterps; i++) {
        BlendInterpsA[i].GuaranteeTimeRange(afStartTime, afEndTime);
    }
}

bool BSMultiTargetTreadTransfController::TargetIsRequiredType() const {
    return NiObject::IsKindOf(&NiAVObject::ms_RTTI, m_pkTarget);
}

bool BSMultiTargetTreadTransfController::InterpolatorIsCorrectType(NiInterpolator* apInterpolator, uint16_t) const {
    return apInterpolator->IsTransformValueSupported();
}

uint16_t BSMultiTargetTreadTransfController::GetInterpolatorTargetIndex(NiAVObject* apTarget) {
    uint16_t usNumLinks;
    usNumLinks = this->usNumLinks;
    uint16_t result = 0;
    if (!usNumLinks) {
        return INVALID_INDEX;
    }
    BSMultiTargetTreadTransfController::LinkAVObj** ppLinks = pLinksA;
    while (static_cast<const void*>(ppLinks[result]->pTarget) != static_cast<const void*>(apTarget)) {
        if (++result >= usNumLinks) {
            return INVALID_INDEX;
        }
    }
    return result;
}

void BSMultiTargetTreadTransfController::RemoveNodeRecurse(NiNode* apObject) {
    if (apObject) {
        uint16_t InterpolatorTargetIndex = GetInterpolatorTargetIndex(apObject);
        if (InterpolatorTargetIndex != NiInterpController::INVALID_INDEX) {
            A35FC0(InterpolatorTargetIndex, nullptr);
        }
    }
    NiNode* pNode = apObject->IsNode();
    if (pNode) {
        for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
            NiNode* pChild = static_cast<NiNode*>(pNode->GetAt(i));
            RemoveNodeRecurse(pChild);
        }
    }
}

void BSMultiTargetTreadTransfController::A35FC0(uint16_t ausIndex, NiNode* apTarget) {
    pLinksA[ausIndex]->pTarget = apTarget;
    if (apTarget) {
        apTarget->m_uFlags |= NiAVObject::SELECTIVE_UPDATE_TRANSFORMS_OVERRIDE;
    }
}

void BSMultiTargetTreadTransfController::AllocateStorage(uint16_t usCount, uint16_t ausLinkCount) {
    usNumInterps = usCount;
    usNumLinks = ausLinkCount;
    if (usCount) {
        BSBlendTreadTransfInterpolator* pInterps;
        int32_t iRemaining = usCount;
        // A real new[]: clang gives the class a vector deleting destructor only where its new[] is seen, and the
        // destructor's delete[] goes through it.
        pInterps = new BSBlendTreadTransfInterpolator[usCount];
        BlendInterpsA = pInterps;
        int32_t iIndex = 0;
        do {
            InterlockedIncrementUInt32(&BlendInterpsA[iIndex++].m_uiRefCount);
            --iRemaining;
        }
        while (iRemaining);
        int32_t iLinksLeft = ausLinkCount;
        BSMultiTargetTreadTransfController::LinkAVObj**
            ppLinks = static_cast<BSMultiTargetTreadTransfController::LinkAVObj**>(
                _NiMalloc(sizeof(BSMultiTargetTreadTransfController::LinkAVObj*) * ausLinkCount)
            );
        pLinksA = ppLinks;
        std::memset(ppLinks, 0, sizeof(BSMultiTargetTreadTransfController::LinkAVObj*) * ausLinkCount);
        if (ausLinkCount) {
            int32_t iLink = 0;
            do {
                BSMultiTargetTreadTransfController::LinkAVObj*
                    pLink = static_cast<BSMultiTargetTreadTransfController::LinkAVObj*>(
                        MemoryManager::Instance().Allocate(sizeof(BSMultiTargetTreadTransfController::LinkAVObj))
                    );
                if (pLink) {
                    pLink->usBlendIdx = 0;
                    pLink->padding02 = 0;
                    pLink->pTarget = nullptr;
                    pLink->pLink = nullptr;
                }
                else {
                    pLink = nullptr;
                }
                pLinksA[iLink++] = pLink;
                --iLinksLeft;
            }
            while (iLinksLeft);
        }
    }
}

void BSMultiTargetTreadTransfController::SetInterpolatorTarget(
    uint16_t ausIndex,
    NiAVObject* apTarget,
    uint16_t ausBlendIdx,
    BSTreadTransfInterpolator::LinkInfo* apLink
) {
    int32_t v5 = ausIndex;
    pLinksA[v5]->pTarget = apTarget;
    pLinksA[v5]->pLink = apLink;
    pLinksA[v5]->usBlendIdx = ausBlendIdx;
    if (apTarget) {
        apTarget->m_uFlags |= NiAVObject::SELECTIVE_UPDATE_TRANSFORMS_OVERRIDE;
    }
}

uint16_t BSMultiTargetTreadTransfController::AddInterpolatorTarget(
    NiAVObject* apTarget,
    uint16_t ausBlendIdx,
    BSTreadTransfInterpolator::LinkInfo* apLink
) {
    uint16_t usNumLinks = this->usNumLinks;
    uint16_t usIndex = 0;
    if (!usNumLinks) {
        return NiInterpController::INVALID_INDEX;
    }
    BSMultiTargetTreadTransfController::LinkAVObj** pLinksA = this->pLinksA;
    while (pLinksA[usIndex]->pTarget) {
        if (++usIndex >= usNumLinks) {
            return NiInterpController::INVALID_INDEX;
        }
    }
    int32_t usIndex_2 = usIndex;
    pLinksA[usIndex_2]->pLink = apLink;
    this->pLinksA[usIndex_2]->usBlendIdx = ausBlendIdx;
    this->pLinksA[usIndex_2]->pTarget = apTarget;
    apTarget->m_uFlags |= NiAVObject::SELECTIVE_UPDATE_TRANSFORMS_OVERRIDE;
    return usIndex;
}

NiObject* BSMultiTargetTreadTransfController::CreateObject() {
    return new BSMultiTargetTreadTransfController;
}
