#pragma once

#include <cstdint>

class hkpCollidable;

// A pair of bodies a collision query found, each with the root collidable and the shape key of its shape. Not
// polymorphic.
struct hkpRootCdBodyPair {
    const hkpCollidable* m_rootCollidableA;
    uint32_t m_shapeKeyA;
    const hkpCollidable* m_rootCollidableB;
    uint32_t m_shapeKeyB;
};
