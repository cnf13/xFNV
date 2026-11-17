#pragma once

#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldBaseCinfo.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldShape.h"

class NiPoint3;

// The construction data of Bethesda's heightfield shape: the base's grid description plus the terrain's vertex array
// and the height offset.
class hkBSHeightFieldShapeCinfo : public hkpSampledHeightFieldBaseCinfo {
public:
	hkBSHeightFieldShapeCinfo();

	void CalcMinMax();

	NiPoint3* pData;
	float fOffset;
};

// Bethesda's sampled heightfield shape: the terrain's vertex array (x, y, z per grid point, rows from the last to the
// first) with an offset added to every height.
class hkBSHeightFieldShape : public hkpSampledHeightFieldShape {
public:
	hkBSHeightFieldShape(const hkBSHeightFieldShapeCinfo& arInfo);
	~hkBSHeightFieldShape() override;

	void collideSpheres(const CollideSpheresInput& arInput, hkVector4* apOutputArray) const override;
	float getHeightAtImpl(int32_t aX, int32_t aZ) const override;
	hkBool* getTriangleFlipImpl(hkBool* arResult) const override;

	NiPoint3* pData;
	int32_t invZRes;
	float fOffset;
};
