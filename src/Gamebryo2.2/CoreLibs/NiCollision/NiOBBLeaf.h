#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiOBBNode.h"
#include "Reconstruction.hpp"

class NiAVObject;
class NiOBBRoot;
class NiPick;

// A leaf of an oriented-bounding-box tree: one triangle, with pointers to its three world-space vertices.
class NiOBBLeaf : public NiOBBNode {
public:
	NiOBBLeaf(NiOBBRoot* pRoot, const uint16_t* pTriList, const NiPoint3* pVertex, const NiPoint3* pWVertex,
		uint16_t usTriangle);
	~NiOBBLeaf() override;

	bool IsLeaf() override;
	bool TestCollision(NiOBBNode* pTestNode, NiAVObject* pkObj0, NiAVObject* pkObj1, int32_t iMaxDepth,
		int32_t iTestMaxDepth, uint32_t uiChangeStamp0, uint32_t uiChangeStamp1) override;
	int32_t FindCollisions(NiOBBNode* pTestNode, NiAVObject* pkRoot0, NiAVObject* pkRoot1, NiAVObject* pkObj0,
		NiAVObject* pkObj1, int32_t iMaxDepth, int32_t iTestMaxDepth, uint32_t uiChangeStamp0, uint32_t uiChangeStamp1,
		bool& bCollision) override;
	int32_t FindIntersections(const NiPoint3& origin, const NiPoint3& dir, NiPick& pick, bool& bIntersects,
		NiAVObject* pObj, uint32_t uiChangeStamp) override;

	// The triangle-triangle test against pTestLeaf: on a hit sets bCollision, fills an
	// intersection record (both leaves' world vertices, their box normals at the point) and returns the collision
	// callback's result, else CONTINUE_COLLISIONS (thiscall, ret 0x18, 0xEB85D0; protected in the engine).
	int32_t FaceFaceCollision(NiOBBLeaf* pTestLeaf, NiAVObject* pkRoot0, NiAVObject* pkRoot1, NiAVObject* pkObj0,
		NiAVObject* pkObj1, bool& bCollision);

	// The barycentric coordinates of p (a4) in the triangle a1 a2 a3, into a5 (cdecl, 0xEB72B0). Its callers keep edx
	// live across the call (it leaves edx as it was). Protected in the engine.
	static float* ComputeBarycenter(const float* a1, const float* a2, const float* a3, const float* a4, float* a5);
	// a1 fDT, a2 N, a3 fC, a4 V, a5 q0, a6 q1, a7 fT0, a8 p0, a9 fT1, a10 p1. Protected in the
	// engine.
	static int32_t TentativeCollision(float a1, const float* a2, float a3, const float* a4, const float* a5,
		const float* a6, float* a7, float* a8, float* a9, float* a10);
	// The earliest time and point at which the moving edge edge0-edge1 meets the triangle pVer.
	// Protected in the engine.
	static int32_t TestEdgeTri(const NiPoint3& edge0, const NiPoint3& edge1, const NiPoint3** pVer, const NiPoint3& N,
		float fC, const NiPoint3& V, float fDT, float& fTMin, NiPoint3& pmin);
	// The earliest contact of the edges of triangle pVer0 with triangle pVer1. Protected in the
	// engine.
	static int32_t FindIntersection(const NiPoint3** pVer0, const NiPoint3** pVer1, const NiPoint3& V, float fDT,
		float& fTMin, NiPoint3& pmin);
	// The earliest contact time and point of the two leaves' moving triangles. Protected in the
	// engine.
	static int32_t FindIntersection(NiOBBLeaf* pLeaf0, NiOBBLeaf* pLeaf1, float& fT, NiPoint3& p);

	const NiPoint3* m_pWorldVertex[3];
};
inline constexpr fnvd::Vtable NiOBBLeaf_vftable{};
