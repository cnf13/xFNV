#pragma once

#include "SpeedTree/CRotTransform.h"
#include "SpeedTree/SpeedTreeRT.h"

struct SIdvBranchVertex {
    SIdvBranchVertex();

    CVec3 m_cDirection;
    CVec3 m_cPos;
    float m_fRadius;
    CRotTransform m_cTrans;
    float m_fRunningLength;
    float m_fWindWeight;
};
