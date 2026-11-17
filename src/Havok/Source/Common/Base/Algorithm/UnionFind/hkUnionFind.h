#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalBuffer.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// Union-find over the integers 0..m_numNodes-1: groups nodes by the edges added to it.
class hkUnionFind {
public:
    hkUnionFind(hkFixedArray<int32_t>* apParents, int32_t aiNumNodes);

    // Points every node at its root.
    void collapseTree();
    // Joins the groups of the nodes i1 and i2.
    void addEdge(int32_t i1, int32_t i2);
    // Numbers the groups and gives every node its group's number; arElementsPerGroup receives the group sizes.
    void assignGroups(hkArray<int32_t>& elementsPerGroup);
    // Orders the group numbers by the size of the group.
    void sortByGroupId(const hkArray<int32_t>& elementsPerGroup, hkArray<int32_t>& orderedGroups) const;
    // Gives the groups the numbers in arReindex.
    void reindex(const hkFixedArray<int32_t>& rindex, int32_t numNewGroups, hkArray<int32_t>& elementsPerGroup);
    // Swaps the biggest group's number with group 0's; returns the biggest group's old number.
    int32_t moveBiggestGroupToIndexZero(hkArray<int32_t>& elementsPerGroup);
    // The root of the node's group, collapsing the path on the way.
    int32_t findRootOfNode(int32_t i);
    // Joins the groups of the two roots.
    void unionRoots(int32_t r1, int32_t r2);

    // Whether all nodes are in one group (the root's parent word counts the nodes, negated).
    hkBool isOneGroup() const {
        hkBool kResult;
        kResult.m_bool = (m_parents->m_data.m_storage[0] == -m_numNodes);
        return kResult;
    }

    // Declared as a pointer in place of a hkFixedArray<int>& reference; it is the pointer a reference compiles to.
    hkFixedArray<int32_t>* m_parents;
    int32_t m_numNodes;
};
