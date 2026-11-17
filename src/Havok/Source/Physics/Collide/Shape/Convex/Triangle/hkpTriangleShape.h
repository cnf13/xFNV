#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"
#include "Reconstruction.hpp"

#include <stdint.h>
#include <xmmintrin.h>

class hkTransform;

// The SDK's inline hkpTriangleShape::getSingleEdgeBitcode, out of line in the exe (0xD1FEF0: triangleEdgesBitcode
// in edx, edgeIndex on the stack, ret 0, the result in eax).
int32_t getSingleEdgeBitcode(uint16_t triangleEdgesBitcode, int32_t edgeIndex);
// Not named in the PDB or IDA; name chosen here. The unit normal of the triangle a1, r_eax,
// a2 into a3, returned; the exe (0xD1FF80) only reads a1, a2 and r_eax.
// Inferred type: a1: hkVector4* from 0xd20506 (&hkpTriangleShape+0x20 (this of
// hkpTriangleShape::weldContactPointImpl)), its members used at 0xd1ff89; a2: hkVector4* from 0xd20506
// (&hkpTriangleShape+0x40 (this of hkpTriangleShape::weldContactPointImpl)), its members used at 0xd1ffa5; r_eax:
// hkVector4* from 0xd20506 (&hkpTriangleShape+0x30 (this of hkpTriangleShape::weldContactPointImpl)), its members used
// at 0xd1ff9f
__m128* calcTriangleNormal(const __m128* a1, const __m128* a2, __m128* a3, const hkVector4* r_eax);
// Not named in the PDB or IDA; name chosen here. For the four rays of a bundle: the triple product of the triangle
// normal a3 with (a1 - p) and (a2 - p), p each ray's hit point, into *a4 (one lane per ray); returns a3 (0xD201A0,
// a1 in ecx, r_eax in eax, the rest on the stack, ret 0). r_eax holds the four hit points transposed: their x, y and z
// in r_eax[0], r_eax[1] and r_eax[2] (castRayBundleImpl's local at 0xD21102).
const hkVector4* calcEdgeSides(const hkVector4* a1, const hkVector4* a2, const hkVector4* a3, hkVector4* a4,
	const hkVector4* r_eax);

// A triangle given by its three vertices, optionally extruded along a vector, with the edge welding information the
// contact welding uses.
class hkpTriangleShape : public hkpConvexShape {
public:
	hkpTriangleShape(const hkVector4& v0, const hkVector4& v1, const hkVector4& v2, float radius);
	// Assumes hkpConvexShape's finish constructor (the SDK's).
	hkpTriangleShape(hkFinishLoadedObjectFlag aFlag) : hkpConvexShape(aFlag) { m_type.m_storage = HK_SHAPE_TRIANGLE; }

	// Assumes hkpConvexShape's constructor from the shape type and the radius (the SDK's).
	hkpTriangleShape(float radius, uint16_t weldingInfo, hkpWeldingUtility::WeldingType type)
		: hkpConvexShape(HK_SHAPE_TRIANGLE, radius), m_weldingInfo(weldingInfo), m_isExtruded(0) {
		m_weldingType = type;
		m_extrusion.setZero4();
	}

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	int32_t calcSizeForSpu(const CalcSizeForSpuInput&, int32_t) const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
	hkVector4Comparison* castRayBundleImpl(hkVector4Comparison* apResult, const hkpShapeRayBundleCastInput& input,
		hkpShapeRayBundleCastOutput& output, const hkVector4Comparison& mask) const override;
	int32_t getNumCollisionSpheresImpl() const override;
	const hkSphere* getCollisionSpheresImpl(hkSphere* sphereBuffer) const override;
	void getFirstVertex(hkVector4& v) const override;
	void getSupportingVertexImpl(const hkVector4& aDirection, hkpCdVertex& supportingVertexOut) const override;
	void convertVertexIdsToVerticesImpl(const uint16_t* ids, int32_t numIds, hkpCdVertex* verticesOut) const override;
	// Havok SDK signature (hkpCollide.lib, tied by vtable).
	int32_t weldContactPointImpl(uint16_t* featurePoints, uint8_t& numFeaturePoints, hkVector4& contactPointWs,
		const hkTransform* thisObjTransform, const hkpConvexShape* collidingShape,
		const hkTransform* collidingTransform, hkVector4& separatingNormalInOut) const override;
	void getCentreImpl(hkVector4& centreOut) const override;

	// The SDK's inline getVertex: the three vertices are consecutive.
	const hkVector4& getVertex(int32_t i) const { return (&m_vertexA)[i]; }

	static const hkClass& staticClass();

	uint16_t m_weldingInfo;
	hkEnum<hkpWeldingUtility::WeldingType, uint8_t> m_weldingType;
	uint8_t m_isExtruded;
	hkVector4 m_vertexA;
	hkVector4 m_vertexB;
	hkVector4 m_vertexC;
	hkVector4 m_extrusion;
};

extern hkClass hkpTriangleShapeClass;
extern hkTypeInfo hkpTriangleShapeTypeInfo;
inline constexpr fnvd::Vtable hkpTriangleShape_vftable{};

// Not named in the PDB or IDA; name chosen here. Per vertex id (0 to 5, the extruded copies 3 to 5): the factor of
// the extrusion added to its vertex; the exe reads it with movss (0xD203AA). Values 0, 0, 0, 1, 1, 1.
using addExtrusionTable_t = float[6];
inline addExtrusionTable_t addExtrusionTable{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};

// Not named in the PDB or IDA; name chosen here. Per one-sided welding type (WELDING_TYPE_ANTICLOCKWISE 0 to
// WELDING_TYPE_CLOCKWISE 4): the sign the triangle normal is scaled by; the exe reads it with movss (0xD2072E).
// Values 1, 0, 0, 0, -1.
using flipNormalBasedOnWeldingTypeTable_t = float[5];
inline flipNormalBasedOnWeldingTypeTable_t flipNormalBasedOnWeldingTypeTable{1.0f, 0.0f, 0.0f, 0.0f, -1.0f};

// Not named in the PDB or IDA; name chosen here. Per vertex id (0 to 5): its vertex, the id modulo 3 (values 0, 1, 2,
// 0, 1, 2). Read at [id*4+0x11B3B4C] (0xD20393, 0xD2041D) and, for the next vertex of an edge, at [i*4+0x11B3B50]
// (0xD2046E, 0xD20510): element i + 1 of this table.
using mod3Table_t = int32_t[6];
inline mod3Table_t mod3Table{0, 1, 2, 0, 1, 2};
