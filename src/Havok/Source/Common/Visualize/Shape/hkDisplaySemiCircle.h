#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"

// An arc the visual debugger displays: a centre, the plane's normal, the direction of angle 0 (m_perp), the angle
// range, the radius and the number of segments. Both constructors store type HK_DISPLAY_SEMICIRCLE.
class hkDisplaySemiCircle : public hkDisplayGeometry {
public:
	hkDisplaySemiCircle();
	hkDisplaySemiCircle(const hkVector4& center, const hkVector4& normal, const hkVector4& startPerp,
		const float thetaMin, const float thetaMax, const float radius, const int32_t numSegments);

	void getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) override;
	void buildGeometry() override;

	using hkDisplayGeometry::getWireframeGeometry;

	// Reserves and fills the vertex array buildGeometry and getWireframeGeometry pass.
	void generatePoints(hkArray<hkVector4>& points);
	hkVector4* getNormal(hkVector4* apResult);
	hkVector4* getPerp(hkVector4* apResult);
	float getThetaMin();
	float getThetaMax();
	float getRadius();
	int32_t getNumSegments();
	hkVector4* getCenter(hkVector4* apResult);
	// Stores the arc's parameters: radius, the angle range, the segment count, the centre,
	// the normal and startPerp as m_perp.
	void setParameters(const float radius, const float thetaMin, const float thetaMax, const int32_t numSegments,
		const hkVector4& center, const hkVector4& normal, const hkVector4& startPerp);

	hkVector4 m_normal;
	hkVector4 m_perp;
	hkVector4 m_center;
	float m_thetaMin;
	float m_thetaMax;
	float m_radius;
	int32_t m_numSegments;
};
