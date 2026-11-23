#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

struct bhkLookIKData;
class hkaSkeleton;
class hkaSkeletonMapper;
struct RagdollFeedbackData;

// What the ragdolls of one creature type share: the low and high skeletons, the mappers between the scene graph and the
// ragdoll skeleton, the feedback data and the look IK data.
class bhkRagdollShareData : public NiObject {
public:
    bhkRagdollShareData();
    ~bhkRagdollShareData() override;

    hkaSkeleton* pLowSkel;
    hkaSkeleton* pHighSkel;
    hkaSkeletonMapper* pSGToRagdoll;
    hkaSkeletonMapper* pRagdollToSG;
    RagdollFeedbackData* pFeedbackData;
    bhkLookIKData* pSharedLookData;
};

NiSmartPointer(bhkRagdollShareData);
