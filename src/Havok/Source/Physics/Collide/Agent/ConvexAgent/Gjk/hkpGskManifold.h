#pragma once

#include <cstdint>

class hkpConstraintOwner;
class hkpContactMgr;

// The contact manifold the GJK agents keep between frames: up to four contact points, each remembered by the vertices
// of the shapes it was found on.
class hkpGskManifold {
public:
    enum : int32_t {
        MAX_POINTS = 0x4,
    };

    struct ContactPoint {
        uint8_t m_dimA;
        uint8_t m_dimB;
        uint16_t m_id;

        union {
            uint8_t m_vert[4];
            uint32_t m_allVerts;
        };
    };

    void init() {
        m_numVertsA = 0;
        m_numVertsB = 0;
        m_numContactPoints = 0;
    }

    // The manifold's own bytes, the counts and the contact points' vertex ids. The agents' create adds the cache's size
    // and rounds the sum up to 16.
    int32_t getTotalSizeInBytes() const {
        return 4 + 2 * (m_numVertsA + m_numVertsB + 4 * m_numContactPoints);
    }

    // The vertex ids of both shapes, A's then B's, stored after the contact points in use. The contact points and
    // m_padding are one variable-length byte area addressed by byte offset, so the ids start wherever the used points
    // end.
    uint16_t* getVertexIds() {
        return reinterpret_cast<uint16_t*>(
            reinterpret_cast<char*>(&m_contactPoints) + m_numContactPoints * sizeof(ContactPoint)
        );
    }

    uint8_t m_numVertsA;
    uint8_t m_numVertsB;
    uint8_t m_numContactPoints;
    ContactPoint m_contactPoints[MAX_POINTS];
    uint8_t m_padding[32];
};

// Drops the manifold's contact points from the contact manager.
void hkGskManifold_cleanup(hkpGskManifold& manifold, hkpContactMgr* mgr, hkpConstraintOwner& constraintOwner);
