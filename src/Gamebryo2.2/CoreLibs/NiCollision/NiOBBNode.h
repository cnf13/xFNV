#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiCollisionGroup.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiOBBox.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Reconstruction.hpp"

class NiAVObject;
class NiMatrix3;
class NiOBBRoot;
class NiPick;

// A node of an oriented-bounding-box tree: its box and two children, for collision and pick tests.
class NiOBBNode : public NiMemObject {
public:
	// Gamebryo's empty node (the exe inlines it at every use).
	NiOBBNode();
	NiOBBNode(NiOBBRoot* pRoot, int32_t aiTriangles, const uint16_t* pTriList, const NiPoint3* pVertex,
		const NiPoint3* pWVertex, int32_t iBinSize);
	virtual ~NiOBBNode();

	virtual bool IsLeaf();
	virtual bool TestCollision(NiOBBNode* pTestNode, NiAVObject* pkObj0, NiAVObject* pkObj1, int32_t iMaxDepth,
		int32_t iTestMaxDepth, uint32_t uiChangeStamp0, uint32_t uiChangeStamp1);
	virtual int32_t FindCollisions(NiOBBNode* pTestNode, NiAVObject* pkRoot0, NiAVObject* pkRoot1, NiAVObject* pkObj0,
		NiAVObject* pkObj1, int32_t iMaxDepth, int32_t iTestMaxDepth, uint32_t uiChangeStamp0, uint32_t uiChangeStamp1,
		bool& bCollision);
	virtual int32_t FindIntersections(const NiPoint3& origin, const NiPoint3& dir, NiPick& pick, bool& bIntersects,
		NiAVObject* pObj, uint32_t uiChangeStamp);

	bool HasChildren();

protected:
	// Builds the subtree of triangles pISplit[i0..i1].
	void CreateRecursive(NiOBBRoot* pRoot, const uint16_t* pTriList, const NiPoint3* pVertex, const NiPoint3* pWVertex,
		float* pArea, NiPoint3* pCentroid, int32_t i0, int32_t i1, int32_t* pISplit, int32_t* pOSplit,
		int32_t iBinSize);

public:
	// Sets m_box's centre to the area-weighted mean of the centroids of triangles pISplit[i0..i1] and returns their
	// covariance (thiscall, ret 0x20; the NiMatrix3 result through the hidden pointer apResult, returned in eax).
	// The exe truncates i0 to the PDB's unsigned short loop counter.
	NiMatrix3* ComputeCovarianceAndMean(NiMatrix3* apResult, const uint16_t* pTriList, const NiPoint3* pVertex,
		float* pArea, NiPoint3* pCentroid, int32_t i0, int32_t i1, int32_t* pISplit);
	// The box of triangles pISplit[i0..i1]: their covariance's eigenvectors as its basis.
	void ComputeOBB(NiOBBRoot* pRoot, const uint16_t* pTriList, const NiPoint3* pVertex, float* pArea,
		NiPoint3* pCentroid, int32_t i0, int32_t i1, int32_t* pISplit);
	int32_t BoxBoxCollision(NiOBBNode* pTestNode, NiAVObject* pkRoot0, NiAVObject* pkRoot1, NiAVObject* pkObj0,
		NiAVObject* pkObj1, bool& bCollision);
	// Runs both objects' collide callbacks on intr (thiscall, ret 4; this is unused).
	int32_t CollisionCallback(NiCollisionGroup::Intersect& intr);
	void DivideTriangles(NiPoint3* pCentroid, int32_t i0, int32_t i1, int32_t* pISplit, int32_t& j0, int32_t& j1,
		int32_t* pOSplit);

	// The exe frees tree nodes through MemoryManager::Deallocate.
	static void operator delete(void* apMemory);

	NiOBBox m_box;
	NiOBBNode* m_pLeft;
	NiOBBNode* m_pRight;
	uint32_t m_uiChangeStamp;
};

inline constexpr fnvd::Vtable NiOBBNode_vftable{};
