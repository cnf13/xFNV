#pragma once

#include "Reconstruction.hpp"

class hkpCollidable;

// A pair of bodies a collision query found, each with the root collidable and the shape key of its shape. Not
// polymorphic. IDA names it hkpCdBodyPair and spells the collidables hkCdBody pointers.
struct hkpRootCdBodyPair {
	const hkpCollidable* m_rootCollidableA;
	uint32_t m_shapeKeyA;
	const hkpCollidable* m_rootCollidableB;
	uint32_t m_shapeKeyB;
};
