#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"

class hkKdTreeCastCollector;

// File-static helper of hkKdTreeAabbCastUtils.cpp: returns its static x, 1 / 65535 in every lane.
__m128 hkGetOneOver65K();

// Casts an axis-aligned box along a line through a kd-tree, reporting every primitive whose bounds it sweeps.
class hkKdTreeAabbCastUtils {
public:
    enum { DEFAULT_STACK_SIZE = 128 };

    // A subtree still to visit: its node and the part [tNear, tFar] of the cast that crosses it.
    struct alignas(16) NodeState {
        const hkKdTreeNode* node;
        float tNear;
        float tFar;
        uint32_t m_padding;
    };

    // The box's centre at the start and at the end of the cast, and its half extents.
    struct hkCastAabbInput {
        hkVector4 m_from;
        hkVector4 m_to;
        hkVector4 m_halfExtents;
    };

    // Casts the box of castInput through root, passing each primitive whose bounds (grown by aabbCheckTolerance) the
    // cast reaches before the collector's hit fraction to collector->addMidPhaseHit; the result is collector->hasHit,
    // written to arResult.
    static hkBool* castAabb(
        hkBool* arResult,
        const hkKdTree* root,
        hkCastAabbInput& castInput,
        hkKdTreeCastCollector* collector,
        float aabbCheckTolerance
    );
};

// The movmskps bit of each split axis: 1, 2, 4 and 8.
using axisMaskBits_10C92E0_t = uint32_t[4];
inline axisMaskBits_10C92E0_t axisMaskBits_10C92E0{1, 2, 4, 8};
