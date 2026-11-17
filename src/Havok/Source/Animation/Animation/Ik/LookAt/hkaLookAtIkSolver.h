#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

class hkQsTransform;

// Turns a bone (the neck or head) so that its forward axis looks at a target, within a cone about a limit axis and,
// optionally, within up/down/left/right range limits. Only static members: the exe makes no instance.
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

	// Rotates neckModelSpaceInOut towards targetMS by gain; the hkBool (true when the target was reached without
	// clamping) comes back through the hidden pointer apResult.
	static hkBool* solve(hkBool* apResult, const Setup& setup, const hkVector4& targetMS, float gain,
		hkQsTransform& neckModelSpaceInOut, const RangeLimits* range);

	// The PDB's sphericalFromCartesian, matched by its place in solve: converts the vector in place to (length,
	// azimuth, elevation, 0). The exe's caller also leaves v's address in ecx, but the body reads only its stack
	// argument (esi from [ebp+8]; caller-popped) (cdecl, ret 0, 0xCE01C0).
	static void sphericalFromCartesian(hkVector4& cartesianInSphericalOut);
	// The angle of (x, y): hkMath::atan2fApproximation's.
	static double lookAtAtan2(float y, float x);
	// The PDB's cartesianFromSpherical, matched by its place in solve: converts v in place. The
	// body reads only its one stack argument ([ebp+8]); solve's `add esp, 8` (0xCE0685) also pops
	// sphericalFromCartesian's argument (cdecl, 0xCE0110).
	static void cartesianFromSpherical(hkVector4& sphericalInCartesianOut);
};
