#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "Reconstruction.hpp"

class hkpMoppNodeMgr;
struct hkpMoppSplittingPlaneDirection;
class hkpMoppTreeNode;

// Interface and fitting parameters for assembling MOPP bytecode.
class hkpMoppAssembler : public hkReferencedObject {
public:
	// The assembler's fit tolerances and build options (PDB: sizeof 0x30). m_absoluteFitToleranceOfAxisAlignedTriangles
	// is an hkVector4, which puts it at 0x10, and the struct's 16-byte alignment pads the last member to 0x30; the PDB
	// names no member at either place.
	struct hkpMoppAssemblerParams {
		float m_relativeFitToleranceOfInternalNodes;
		float m_absoluteFitToleranceOfInternalNodes;
		float m_absoluteFitToleranceOfTriangles;
		hkVector4 m_absoluteFitToleranceOfAxisAlignedTriangles;
		int32_t m_groupLevels;
		hkBool m_interleavedBuildingEnabled;
	};

	virtual int32_t getNumSplittingPlaneDirections() const = 0;
	virtual const hkpMoppSplittingPlaneDirection* getSplittingPlaneDirections() const = 0;
	virtual void assemble(hkpMoppTreeNode*, hkpMoppNodeMgr*, int32_t) = 0;
	virtual void getScaleInfo(hkpMoppTreeNode*, hkpMoppCode::CodeInfo*) = 0;
};
