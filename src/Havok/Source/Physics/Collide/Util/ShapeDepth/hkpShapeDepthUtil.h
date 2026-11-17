#pragma once

#include "Reconstruction.hpp"

class hkpShape;

// Measures how deeply shapes nest through their containers. It has no fields; the PDB sizes it at 1.
class hkpShapeDepthUtil {
public:
	// 0 for a shape without a container, else 1 more than its deepest child.
	static int32_t getShapeDepth(const hkpShape* shape);
};
