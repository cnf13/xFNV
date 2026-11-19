#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <type_traits>

class hkKdTreeCastCollector;
// A tree node and a primitive's projected bounds.
struct hkKdTreeNode;
struct hkKdTreeProjectedEntry;

// A tree node, 8 bytes: an inner node or a leaf, told apart by the sign of the first word. Declared as a struct to
// match the existing forward declarations.
struct hkKdTreeNode {
    // A leaf: the index of its first projected entry (0x7FFFFFFF for an empty leaf) and their number.
    struct Leaf {
        int32_t m_type_primId;
        uint32_t m_numPrimitives;
    };

    // An inner node: the sign bit set, the byte offset from this node to its left child (the right one follows it)
    // in bits 2 to 30 and the split axis in bits 0 and 1; the split planes quantised to 16 bits across the tree's
    // bounds, the left child's upper bound and the right child's lower bound.
    struct Node {
        int32_t m_type_left_axis;
        uint16_t m_splitMin;
        uint16_t m_splitMax;
    };

    union {
        Leaf m_leaf;
        Node m_node;
    } m_data;
};

// The id of a primitive in the tree: the address of its collidable, kept as an integer the width of a pointer.
using hkPrimitiveId = uintptr_t;

// Half of a primitive id: 16 bits where a pointer has 32, 32 bits where a pointer has 64.
using hkHalfPointer = std::conditional_t<sizeof(void*) == 8, uint32_t, uint16_t>;

// A primitive's bounds quantised to 16 bits per axis, with its id split over the two spare words. It is 16-aligned.
struct alignas(16) hkKdTreeProjectedEntry {
    // Its arrays treat it as plain data (hkTypeTraits.h): an array's sized constructor leaves the entries unwritten, as
    // the game's does.
    HK_DECLARE_POD_TYPE();

    // The primitive id as one integer and as its two halves: m_halfPointer[0] is m_primIdHigh (the low half on the
    // little-endian PC), m_halfPointer[1] m_primIdLow (the SDK's getPrimitiveId and setPrimitiveId). The SDK's
    // hkUlong and 32-bit build's hkHalfPointer (hkinternal.pdb: sizeof 4).
    union PointerHalfUnion {
        uintptr_t m_ulong;
        hkHalfPointer m_halfPointer[2];
    };

    // The SDK's inline getPrimitiveId and setPrimitiveId, written as shifts: m_primIdHigh holds the id's low half.
    hkPrimitiveId getPrimitiveId() const {
        return hkPrimitiveId(m_primIdHigh) | (hkPrimitiveId(m_primIdLow) << (8 * sizeof(hkHalfPointer)));
    }

    void setPrimitiveId(hkPrimitiveId primId) {
        m_primIdHigh = static_cast<hkHalfPointer>(primId);
        m_primIdLow = static_cast<hkHalfPointer>(primId >> (8 * sizeof(hkHalfPointer)));
    }

    uint16_t m_min[3];
    hkHalfPointer m_primIdHigh;
    uint16_t m_max[3];
    hkHalfPointer m_primIdLow;
};

// What a tree is built for: the primitive count, how many empty nodes to reserve and the bounds.
struct hkKdTreeCinfo {
    // The SDK's values.
    enum EmptyNodeAllocation : uint8_t {
        NO_EMPTY_NODES = 0,
        FEW_EMTPY_NODES = 8,
        STANDARD_EMPTY_NODES = 2,
        LOTS_OF_EMPTY_NODES = 1,
    };

    hkKdTreeCinfo();

    int32_t m_numPrimitives;
    hkEnum<EmptyNodeAllocation, uint8_t> m_emptyNodeAllocation;
    hkBool m_isDistributedBuild;
    hkAabb m_aabb;
};

// Havok's kd-tree over the primitives of a world: the node array, the primitives projected onto the split axes and
// the bounds of the whole tree.
class hkKdTree {
public:
    hkKdTree(int32_t numNodes);
    hkKdTree(const hkKdTreeCinfo& cinfo);
    ~hkKdTree();

protected:
    // Grows m_nodes to numNodes (at least 2) and empties the root node.
    void initNodeArray(int32_t numNodes);

public:
    // Empties the projected entry whose id is aId: its three minimum words become 0xFFFF and
    // its three maximum words 0, the id words untouched.
    void invalidatePrimitive(hkPrimitiveId aId);
    // Sets the depth to -1, takes cinfo's bounds, sizes the node array for its primitive count and empty-node
    // allocation and the projected entries for its primitives.
    void reset(const hkKdTreeCinfo& cinfo);

    hkArray<hkKdTreeNode> m_nodes;
    int32_t m_maxDepth;
    int32_t m_numPrimitives;
    int32_t m_numRegularNodes;
    int32_t m_numReservedEmptyNodes;
    hkArray<hkKdTreeProjectedEntry> m_projectedEntries;
    hkAabb m_aabb;
};

// A cast from apTree's root through castRayRecursive, with the tree's AABB minimum and extent.
void castRayKdTree(hkKdTree* apTree, __m128* a2, __m128* a3, float a4, float a5, float a6, hkKdTreeCastCollector* a7);
// a1 is the tree's node array (the root), a2 the node to cast from, a3 the tree's projected entries; a11 is the
// collector, on which addMidPhaseHit, hasHit and getHitFraction are called.
void castRayRecursive(
    const hkKdTreeNode* a1,
    const hkKdTreeNode* a2,
    const hkKdTreeProjectedEntry* a3,
    __m128* a4,
    __m128* a5,
    __m128* a6,
    __m128* a7,
    float a8,
    float a9,
    float a10,
    hkKdTreeCastCollector* a11
);
