#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Reconstruction.hpp"

// hkpCylinderShape.cpp's file-local helper: the smallest multiple of 0.01 that a float-to-int cast rounds up to 1,
// else 1 (cdecl, ST0, 0xD571D0). The callers inline hkpCylinderShape::initRoundUpThreshold around it.
float minValueRoundedUpTo1();
void assertRoundUpThreshold();
// Havok SDK name; not in the PDB.
// Inferred type: a1: int32_t: signed uses from 0xd5716a
int32_t setNumberOfVirtualSideSegements(int32_t a1);

// A cylinder between two end points with its own radius, plus two vectors perpendicular to its axis; the convex
// shape's radius rounds it off.
class hkpCylinderShape : public hkpConvexShape {
public:
	enum VertexIdEncoding : int32_t {
		VERTEX_ID_ENCODING_IS_BASE_A_SHIFT = 0x7,
		VERTEX_ID_ENCODING_SIN_SIGN_SHIFT = 0x6,
		VERTEX_ID_ENCODING_COS_SIGN_SHIFT = 0x5,
		VERTEX_ID_ENCODING_IS_SIN_LESSER_SHIFT = 0x4,
		VERTEX_ID_ENCODING_VALUE_MASK = 0xf,
	};

	hkpCylinderShape(const hkVector4& vertexA, const hkVector4& vertexB, float cylinderRadius, float paddingRadius);
	hkpCylinderShape(hkFinishLoadedObjectFlag flag);
	~hkpCylinderShape() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	int32_t calcSizeForSpu(const CalcSizeForSpuInput&, int32_t) const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
	int32_t getNumCollisionSpheresImpl() const override;
	const hkSphere* getCollisionSpheresImpl(hkSphere* sphereBuffer) const override;
	void getFirstVertex(hkVector4& v) const override;
	void getSupportingVertexImpl(const hkVector4& direction, hkpCdVertex& supportingVertexOut) const override;
	void convertVertexIdsToVerticesImpl(const uint16_t* ids, int32_t numIds, hkpCdVertex* verticesOut) const override;
	void getCentreImpl(hkVector4& centreOut) const override;

	// Sets m_cylRadius and the cylinder's extents from it.
	void setCylinderRadius(float radius);
	// m_cylRadius through the exe's out-of-line getter (thiscall, ST(0), 0xD571A0).
	float getCylinderRadius() const;
	// Fills m_perpendicular1/2 from the axis.
	void presetPerpendicularVector();

	// Havok SDK signature (hkpCollide.lib, tied by target).
	static const hkClass& staticClass();

	float m_cylRadius;
	float m_cylBaseRadiusFactorForHeightFieldCollisions;
	hkVector4 m_vertexA;
	hkVector4 m_vertexB;
	hkVector4 m_perpendicular1;
	hkVector4 m_perpendicular2;
};

inline constexpr fnvd::Vtable hkpCylinderShape_vftable{};

// hkpCylinderShape::s_intRoundUpThreshold: -1 until the first constructor sets it.
inline float hkpCylinderShape_s_intRoundUpThreshold = -1.0f;

// hkpCylinderShape::s_virtualTesselationParameter and its inverse, set by setNumberOfVirtualSideSegements.
inline float hkpCylinderShape_s_virtualTesselationParameter = 2.6874115f;
inline float hkpCylinderShape_s_virtualTesselationParameterInv = 0.37210527f;
extern hkClass hkpCylinderShapeClass;
extern hkTypeInfo hkpCylinderShapeTypeInfo;
