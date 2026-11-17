#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkQsTransform;

// Turns a bone (the neck or head) so that its forward axis looks at a target, within a cone about a limit axis and,
// optionally, within up/down/left/right range limits. Only static members; no instance is made.
class hkaLookAtIkSolver : public hkReferencedObject {
public:
	// What the solver needs about the bone: its forward axis and eye position in its local space, and the cone the
	// look direction must stay in (an axis in model space and a half angle in radians).
	struct Setup {
		hkVector4 m_fwdLS;
		hkVector4 m_eyePositionLS;
		hkVector4 m_limitAxisMS;
		float m_limitAngle;
	};

	// Separate angle limits about the up axis (left/right) and away from it (up/down), in radians.
	struct RangeLimits {
		float m_limitAngleUp;
		float m_limitAngleDown;
		float m_limitAngleLeft;
		float m_limitAngleRight;
		hkVector4 m_upAxisMS;
	};

	// Rotates neckModelSpaceInOut towards targetMS by gain; the hkBool written to apResult is true when the target was
	// reached without clamping.
	static hkBool* solve(hkBool* apResult, const Setup& setup, const hkVector4& targetMS, float gain,
		hkQsTransform& neckModelSpaceInOut, const RangeLimits* range);

	// Converts the vector in place to (length, azimuth, elevation, 0).
	static void sphericalFromCartesian(hkVector4& cartesianInSphericalOut);
	// The angle of (x, y): hkMath::atan2fApproximation's.
	static double lookAtAtan2(float y, float x);
	// Converts v in place.
	static void cartesianFromSpherical(hkVector4& sphericalInCartesianOut);
};
