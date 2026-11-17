#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpHeightFieldShape.h"

class hkpSampledHeightFieldBaseCinfo;

// Base of the heightfields sampled on a regular grid: the resolution, the scales between grid and shape space and the
// extents. Derived classes store the samples and answer the height at a grid point.
class hkpSampledHeightFieldShape : public hkpHeightFieldShape {
public:
	enum HeightFieldType : int32_t {
		HEIGHTFIELD_STORAGE = 0x0,
		HEIGHTFIELD_COMPRESSED = 0x1,
		HEIGHTFIELD_USER = 0x2,
		HEIGHTFIELD_MAX_ID = 0x3,
	};

	// The SDK's protected constructor for derived shapes that fill the fields themselves.
	hkpSampledHeightFieldShape();
	hkpSampledHeightFieldShape(const hkpSampledHeightFieldBaseCinfo& ci,
		hkpSampledHeightFieldShape::HeightFieldType hfType);

	// The SDK's finish constructor of a loaded object.
	hkpSampledHeightFieldShape(hkFinishLoadedObjectFlag aFlag) : hkpHeightFieldShape(aFlag) {
		if (aFlag.m_finishing) {
			m_type.m_storage = HK_SHAPE_SAMPLED_HEIGHT_FIELD;
		}
	}

	~hkpSampledHeightFieldShape() override;

	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
	void castRayWithCollectorImpl(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
		hkpRayHitCollector& collector) const override;
	void castSphere(const hkpSphereCastInput& input, const hkpCdBody& cdBody,
		hkpRayHitCollector& collector) const override;
	// The height of the sample at grid point (x, z).
	virtual float getHeightAtImpl(int32_t x, int32_t z) const = 0;
	// Whether the grid cells are split along the other diagonal.
	virtual hkBool* getTriangleFlipImpl(hkBool* arResult) const = 0;

	// Havok SDK.
	void getCinfo(hkpSampledHeightFieldBaseCinfo& cinfo) const;
	// The height and normal at (subX, subZ) inside grid cell (xPos, zPos), and which of the cell's triangles holds it.
	// Returns &heightOut.
	float* getHeightAndNormalAt(int32_t xPos, int32_t zPos, float subX, float subZ, hkVector4& normalOut,
		float& heightOut, int32_t& triangleIndexOut) const;
	float getHeightAt(int32_t x, int32_t z) const { return getHeightAtImpl(x, z); }

	hkBool getTriangleFlip() const {
		hkBool kResult;
		getTriangleFlipImpl(&kResult);
		return kResult;
	}

	// Walks the ray over the grid cells, crossing the x, diagonal and z grid lines in order of their ray fraction, and
	// reports where the ray goes below the surface.
	void castRayInternal(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
		hkBool reportPenetratingStartPosition, float maxExtraPenetration, hkpRayHitCollector& collector) const;

	// Havok SDK; fill the shape function tables.
	static void registerSimulationFunctions(ShapeFuncs& sf);
	static void registerCollideQueryFunctions(ShapeFuncs& sf);
	static void registerRayCastFunctions(ShapeFuncs& sf);
	static void registerHeightFieldFunctions();

	int32_t m_xRes;
	int32_t m_zRes;
	float m_heightCenter;
	hkBool m_useProjectionBasedHeight;
	hkEnum<HeightFieldType, uint8_t> m_heightfieldType;
	hkVector4 m_intToFloatScale;
	hkVector4 m_floatToIntScale;
	hkVector4 m_floatToIntOffsetFloorCorrected;
	hkVector4 m_extents;
};
