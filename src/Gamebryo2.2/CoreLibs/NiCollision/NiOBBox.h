#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class NiOBBRoot;
class NiTransform;

// An oriented bounding box: centre, basis and extents in model space and in world space, and its OBB tree root.
class NiOBBox : public NiMemObject {
public:
	void Transform(const NiTransform& xform);
	void ComputeNormal(const NiPoint3& p, NiPoint3& normal) const;
	int32_t TestIntersection(const NiOBBox& box);
	int32_t FindIntersection(const NiOBBox& box, float& t, NiPoint3& p);
	int32_t TestIntersection(const NiPoint3& origin, const NiPoint3& dir);

	NiPoint3 m_center;
	NiPoint3 m_basis[3];
	float m_extent[3];
	NiPoint3 m_wCenter;
	NiPoint3 m_wBasis[3];
	float m_wExtent[3];
	NiOBBRoot* m_pRoot;
};
