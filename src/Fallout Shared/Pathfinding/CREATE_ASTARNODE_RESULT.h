#pragma once

#include <cstdint>

// Values are what GetOrCreateAStarNode returns: new, improved existing, unchanged existing.
enum CREATE_ASTARNODE_RESULT : int32_t {
    ASTAR_CREATED_NEW_NODE = 0,
    ASTAR_MUST_SET_LINKDATA = 1,
    ASTAR_OLD_COST_IS_BETTER = 2,
};
