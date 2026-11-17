#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "Reconstruction.hpp"

class hkpMoppCompilerInput;
class hkpMoppMediator;
class hkpShapeContainer;

// Builds the MOPP code of a shape container. It has no instances: static functions only.
class hkpMoppUtility {
public:
	// Builds the code through a cached shape mediator or a plain one (req.m_cachePrimitiveExtents); the code's build
	// type is set from req.m_enableChunkSubdivision.
	static hkpMoppCode* buildCode(const hkpShapeContainer* shapeContainer, const hkpMoppCompilerInput& moppInput,
		hkArray<hkpMoppCodeReindexedTerminal>* reindexInfo);

	// Compiles the mediator's primitives with moppInput's tolerances into a new MOPP code, appending the reindexed
	// terminals to reindexInfo when it is given; shapeContainer is not read. Protected in the SDK.
	static hkpMoppCode* buildCodeInternal(hkpMoppMediator& mediator, const hkpShapeContainer* shapeContainer,
		const hkpMoppCompilerInput& moppInput, hkArray<hkpMoppCodeReindexedTerminal>* reindexInfo);
};
