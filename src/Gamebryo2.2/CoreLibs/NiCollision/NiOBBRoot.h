#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Reconstruction.hpp"

class NiAVObject;
class NiOBBNode;
class NiPick;

// The root of an oriented-bounding-box tree: the tree, the object's velocity and transform and the change stamp that
// tells the tree's boxes whether they are current.
class NiOBBRoot : public NiMemObject {
public:
	NiOBBRoot(int32_t aiTriangles, const uint16_t* pTriList, const NiPoint3* pVertex, const NiPoint3* pWVertex,
		int32_t iBinSize);
	virtual ~NiOBBRoot();

	virtual bool TestCollision(NiOBBRoot* pTestRoot, NiAVObject* pThisObj, NiAVObject* pOtherObj, int32_t iMaxDepth,
		int32_t iTestMaxDepth);
	virtual int32_t FindCollisions(NiOBBRoot* pTestRoot, NiAVObject* pThisRoot, NiAVObject* pOtherRoot,
		NiAVObject* pThisObj, NiAVObject* pOtherObj, int32_t iMaxDepth, int32_t iTestMaxDepth, bool& bCollision);
	virtual int32_t FindIntersections(const NiPoint3& origin, const NiPoint3& dir, NiPick& pick, bool& bIntersects,
		NiAVObject* pObj, uint32_t uiChangeStamp);

	// The exe frees the root through MemoryManager::Deallocate.
	static void operator delete(void* apMemory);

	float m_fDeltaTime;
	NiOBBNode* m_pTree;
	bool m_bTransformChanged;
	uint8_t padding0D[3];
	NiPoint3 m_velocity;
	NiTransform m_xform;
	uint32_t m_uiChangeStamp;
};
inline constexpr fnvd::Vtable NiOBBRoot_vftable{};
