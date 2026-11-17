#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

struct hkpMoppSplittingPlaneDirection;

// Scores the candidate splitting planes while a MOPP tree is built, weighted by m_userScaling.
class hkpMoppCostFunction : public hkReferencedObject {
public:
    // The weights of each cost term.
    struct hkpMoppSplitCostParams {
        // Sets every weight to 1 and the query tolerance to its default. The int argument is not read. The defaulted
        // constructor is used for the stack params hkpMoppUtility builds.
        hkpMoppSplitCostParams() = default;
        hkpMoppSplitCostParams(int32_t aiUnused);

        float m_weightPrimitiveSplit;
        float m_weightPlaneDistance;
        float m_weightNumUnbalancedTriangles;
        float m_weightPlanePosition;
        float m_weightPrimitiveIdSpread;
        float m_queryErrorTolerance;
    };

    // The state of a split's right plane the cost of its position is computed from. Members and layout from Havok's
    // release hkpinternal.pdb (sizeof 0x30).
    struct hkpPlaneRightParams {
        int32_t m_currentDepth;
        int32_t m_optimalDepth;
        float m_absoluteMin;
        float m_absoluteMax;
        float m_extentsInv;
        int32_t m_currentNumPrimitivesRight;
        int32_t m_numPrimitives;
        float m_numPrimitivesInv;
        float m_planeRightPosition;
        const hkpMoppSplittingPlaneDirection* m_plane;
        uint32_t m_minPrimitiveId;
        uint32_t m_maxPrimitiveId;
    };

    // Both planes of a split: the right plane's state, the primitives split and left of it, and the left plane's
    // position. Members and layout from Havok's release hkpinternal.pdb (sizeof 0x3C).
    struct hkpPlanesParams : public hkpPlaneRightParams {
        int32_t m_numSplitPrimitives;
        int32_t m_currentNumPrimitivesLeft;
        float m_planeLeftPosition;
    };

    // The cost function scaled by params.
    hkpMoppCostFunction(const hkpMoppSplitCostParams& params);
    ~hkpMoppCostFunction() override;

    // Copies params into m_userScaling.
    void updateParams(const hkpMoppSplitCostParams& bp);
    // The cost of the primitives left unbalanced between the planes.
    float costNumUnBalanced(const hkpPlanesParams& p);
    // The cost of the right plane's position (+0x20) at its depth.
    float costUnbalancedPlaneRight(const hkpPlaneRightParams& p);
    // The cost of the left plane's position (+0x38) at its depth.
    float costUnbalancedPlaneLeft(const hkpPlanesParams& p);

    hkpMoppSplitCostParams m_userScaling;
};
