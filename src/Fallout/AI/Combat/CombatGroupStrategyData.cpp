#include "Fallout/AI/Combat/CombatGroupStrategyData.h"

#include "BSCore/BSSimpleArray.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/GroupMemberData.h"
#include "Fallout/AI/CombatController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "xfnv/Compiler.h"

// Sorts the group's initialised members into the melee and ranged lists.
CombatGroupData::CombatGroupData(CombatGroup* apCombatGroup)
  : pCombatGroup(apCombatGroup), iMemberCount(0), iMembersTargetsIntersecting(-1), iMinimumTargetDetection(INT32_MAX),
    iMaximumTargetDetection(INT32_MIN), fMemberHealthPercentage(0.0f) {
    uint32_t imemberCount = apCombatGroup->GetMemberCount();
    for (uint32_t i = 0; i < imemberCount; ++i) {
        Actor* pmember = apCombatGroup->GetMember(i);
        CombatState* pcombatState = pmember->GetCombatController()->pCombatState;
        if (!pcombatState->bInitializing) {
            ++iMemberCount;
            if (pcombatState->ShouldUseMelee()) {
                MeleeMembers.Add(GroupMemberData(pmember, i));
            }
            else {
                RangedMembers.Add(GroupMemberData(pmember, i));
            }
        }
    }
}

void CombatGroupData::ComputeMinMaxTargetDetection() {
    uint32_t MemberCount = pCombatGroup->GetMemberCount();
    for (uint32_t aiIndex = 0; aiIndex < MemberCount; ++aiIndex) {
        Actor* Member = pCombatGroup->GetMember(aiIndex);
        CombatController* pCombatController = Member->GetCombatController();
        if (pCombatController) {
            int32_t TalkingActivator = pCombatController->pCombatState->iTargetDetectionLevel;
            if (TalkingActivator < iMinimumTargetDetection) {
                iMinimumTargetDetection = TalkingActivator;
            }
            if (TalkingActivator < iMaximumTargetDetection) {
                iMaximumTargetDetection = TalkingActivator;
            }
        }
    }
}

int32_t CombatGroupData::GetMemberMinimumTargetDetection() {
    if (iMinimumTargetDetection == 0x7FFFFFFF) {
        ComputeMinMaxTargetDetection();
    }
    return iMinimumTargetDetection;
}

int32_t CombatGroupData::GetMemberMaximumTargetDetection() {
    if (iMaximumTargetDetection == INT32_MIN) {
        ComputeMinMaxTargetDetection();
    }
    return iMaximumTargetDetection;
}

GroupMemberData* CombatGroupData::GetMeleeMember(uint32_t aiIndex) {
    return &MeleeMembers[aiIndex];
}

GroupMemberData* CombatGroupData::GetRangedMember(uint32_t aiIndex) {
    return &RangedMembers[aiIndex];
}

void CombatGroupData::SortRangedMembers() {
    uint32_t uiNumMembers = GetRangedMemberCount();
    NiPoint3* kActorPositions = static_cast<NiPoint3*>(XFNV_ALLOCA16(12 * uiNumMembers));
    NiPoint3 kSummedActorPositions = NiPoint3_ZERO;
    for (uint32_t i = 0; i < uiNumMembers; ++i) {
        GroupMemberData& rMember = RangedMembers[i];
        kActorPositions[i] = rMember.pActor->GetLocationOnReference();
        kSummedActorPositions += kActorPositions[i];
    }
    kSummedActorPositions /= static_cast<float>(uiNumMembers);
    float* fDistances = static_cast<float*>(XFNV_ALLOCA16(4 * uiNumMembers));
    for (uint32_t j = 0; j < uiNumMembers; ++j) {
        fDistances[j] = (kActorPositions[j] - kSummedActorPositions).SqrLength();
    }
    for (uint32_t k = 0; k < uiNumMembers - 1; ++k) {
        uint32_t uiMinIndex = k;
        float fMinDistance = fDistances[k];
        for (uint32_t m = k + 1; m < uiNumMembers; ++m) {
            if (fDistances[m] < fMinDistance) {
                uiMinIndex = m;
                fMinDistance = fDistances[m];
            }
        }
        if (k != uiMinIndex) {
            float fTemp = fDistances[k];
            fDistances[k] = fDistances[uiMinIndex];
            fDistances[uiMinIndex] = fTemp;
            GroupMemberData kTemp = RangedMembers.GetAt(k);
            GroupMemberData kMin = RangedMembers.GetAt(uiMinIndex);
            RangedMembers.GetAt(k) = kMin;
            RangedMembers.GetAt(uiMinIndex) = kTemp;
        }
    }
}

bool CombatGroupData::GetMembersTargetsIntersecting() {
    if (iMembersTargetsIntersecting == -1) {
        iMembersTargetsIntersecting = pCombatGroup->CheckMemberTargetIntersection();
    }
    return iMembersTargetsIntersecting == 1;
}

double CombatGroupData::GetAverageGroupHealth() {
    if (fMemberHealthPercentage == 0.0) {
        fMemberHealthPercentage = pCombatGroup->ComputeMemberHealthPercentage();
    }
    return fMemberHealthPercentage;
}
