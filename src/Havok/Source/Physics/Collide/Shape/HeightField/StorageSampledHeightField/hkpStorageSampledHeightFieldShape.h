#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldBaseCinfo.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// A sampled heightfield whose heights come from a float buffer, offset by fOffset; the buffer is freed with the
// shape when bowned is set. The PDB's members (invZRes, fOffset, bowned) differ from Havok's stock class.
class hkpStorageSampledHeightFieldShape : public hkpSampledHeightFieldShape {
public:
	hkpStorageSampledHeightFieldShape(const hkpSampledHeightFieldShape* hf);
	hkpStorageSampledHeightFieldShape(const hkpSampledHeightFieldBaseCinfo& ci, float* storage, float offset);
	hkpStorageSampledHeightFieldShape(const hkpSampledHeightFieldBaseCinfo& ci, hkArray<float>& samples, float offset);

	// The SDK's finish constructor of a loaded object.
	hkpStorageSampledHeightFieldShape(hkFinishLoadedObjectFlag aFlag) : hkpSampledHeightFieldShape(aFlag) {
		if (aFlag.m_finishing) {
			m_heightfieldType.m_storage = HEIGHTFIELD_STORAGE;
		}
	}

	~hkpStorageSampledHeightFieldShape() override;

	// Havok SDK signature (hkpCollide.lib, tied by vtable).
	int32_t calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const override;
	void collideSpheres(const CollideSpheresInput& input, hkVector4* outputArray) const override;
	float getHeightAtImpl(int32_t x, int32_t z) const override;
	hkBool* getTriangleFlipImpl(hkBool* arResult) const override;

	static const hkClass& staticClass();

	float* m_storage;
	hkBool m_triangleFlip;
	int32_t invZRes;
	float fOffset;
	bool bowned;
};

extern hkClass hkpStorageSampledHeightFieldShapeClass;
extern hkTypeInfo hkpStorageSampledHeightFieldShapeTypeInfo;
inline constexpr fnvd::Vtable hkpStorageSampledHeightFieldShape_vftable{};
