#pragma once

#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldBaseCinfo.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldShape.h"
#include "Reconstruction.hpp"

class NiPoint3;

// The construction data of Bethesda's heightfield shape: the base's grid description plus the terrain's vertex
// array and the height offset. The PDB places its constructor in CellMopp.cpp and CalcMinMax in this file's .cpp.
class hkBSHeightFieldShapeCinfo : public hkpSampledHeightFieldBaseCinfo {
public:
	hkBSHeightFieldShapeCinfo();

	void CalcMinMax();

	NiPoint3* pData;
	float fOffset;
};

// Bethesda's sampled heightfield shape: the terrain's vertex array (x, y, z per grid point, rows from the last to the
// first) with an offset added to every height. IDA types pData as float* (pfHeightData). The exe's
// slots 11, 13 and 14 are its own, the rest is hkpSampledHeightFieldShape's. The PDB places its constructors and
// QData in this file's .cpp.
class hkBSHeightFieldShape : public hkpSampledHeightFieldShape {
public:
	hkBSHeightFieldShape(const hkBSHeightFieldShapeCinfo& arInfo);
	~hkBSHeightFieldShape() override;

	void collideSpheres(const CollideSpheresInput& arInput, hkVector4* apOutputArray) const override;
	float getHeightAtImpl(int32_t aX, int32_t aZ) const override;
	hkBool* getTriangleFlipImpl(hkBool* arResult) const override;

	NiPoint3* pData;
	// The PDB calls it invZRes too.
	int32_t invZRes;
	float fOffset;
};
inline constexpr fnvd::Vtable hkBSHeightFieldShape_vftable{};
