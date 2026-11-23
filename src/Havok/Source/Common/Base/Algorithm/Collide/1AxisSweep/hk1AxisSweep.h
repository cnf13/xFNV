#pragma once

#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <stdint.h>

// A pair of overlapping boxes' keys, as hk1AxisSweep::collide writes them.
struct hkKeyPair {
    uint32_t m_keyA;
    uint32_t m_keyB;
};

// Finds the overlapping pairs of integer boxes sorted along x: static functions only.
class hk1AxisSweep {
public:
    // A box with its key in the word after m_min (m_min[3], over the expansion bytes); no fields of its own.
    struct AabbInt : public hkAabbUint32 {
        // The SDK's accessor: the key is the word after m_min, over the expansion bytes.
        uint32_t& getKey() {
            uint32_t* minP = m_min;
            return minP[3];
        }

        const uint32_t& getKey() const {
            const uint32_t* minP = m_min;
            return minP[3];
        }

        void operator=(const AabbInt& other) {
            hkString::memCpy16<sizeof(AabbInt)>(this, &other);
        }

        void set(const hkAabb& aabbIn, int32_t key);

        // The SDK's inline: the sign bit set when the two boxes are apart in y or z.
        static uint32_t yzDisjoint(const hk1AxisSweep::AabbInt& aabb, const hk1AxisSweep::AabbInt& other) {
            uint32_t yab = aabb.m_max[1] - other.m_min[1];
            uint32_t yba = other.m_max[1] - aabb.m_min[1];
            uint32_t zab = aabb.m_max[2] - other.m_min[2];
            uint32_t zba = other.m_max[2] - aabb.m_min[2];
            uint32_t combined = (yab | yba) | (zab | zba);
            return combined & 0x80000000u;
        }
    };

    // Two-list collide: the pairs of boxes of pa and pb that overlap, at most maxNumPairs, each list ending in four end
    // markers.
    static int32_t collide(
        const hk1AxisSweep::AabbInt* pa,
        int32_t numA,
        const hk1AxisSweep::AabbInt* pb,
        int32_t numB,
        hkKeyPair* pairsOut,
        int32_t maxNumPairs,
        hkPadSpu<int32_t>& numPairsSkipped
    );
    // The SDK's one-list collide: the pairs of boxes of pa that overlap, at most maxNumPairs, the list ending in four
    // end markers.
    static int32_t collide(
        const hk1AxisSweep::AabbInt* pa,
        int32_t numA,
        hkKeyPair* pairsOut,
        int32_t maxNumPairs,
        hkPadSpu<int32_t>& numPairsSkipped
    );
    // Havok SDK signature (hkBase.lib, tied by bytes).
    static void sortAabbs(hk1AxisSweep::AabbInt* aabbs, int32_t size);
};
