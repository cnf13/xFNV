#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

class hkMotionState;
class hkQuaternion;
struct hkStepInfo;
class hkSweptTransform;
class hkTransform;

// Helpers that move a motion state's swept transform.
namespace hkSweptTransformUtil {
// Moves the centre of rotation of ms to the local offset.
void setCentreOfRotationLocal(const hkVector4& newCenterOfRotation, hkMotionState& motionState);
// Moves ms to pos without sweeping.
void warpToPosition(const hkVector4& position, hkMotionState& ms);
// Turns ms to rotation without sweeping, keeping its position.
void warpToRotation(const hkQuaternion& rotation, hkMotionState& ms);
// Stores the step's start time and inverse duration in the w of the swept centres of mass.
void setTimeInformation(float startTime, float invDeltaTime, hkMotionState& motionState);
// Moves ms to position and rotation without sweeping.
void warpTo(const hkVector4& position, const hkQuaternion& rotation, hkMotionState& ms);
// Moves ms to transform without sweeping.
void warpTo(const hkTransform& transform, hkMotionState& ms);
// The transform sweptTrans has at time.
void lerp2(const hkSweptTransform& sweptTrans, float t, hkTransform& transformOut);
// Moves motionState back to time on its swept transform.
void backStepMotionState(float time, hkMotionState& motionState);
// Stops motionState at time: both ends of its swept transform become the transform at time.
void freezeMotionState(float time, hkMotionState& motionState);
// Keys ms to reach pos1 and rot1 at the end of the step stepInfo describes.
void keyframeMotionState(const hkStepInfo& stepInfo, const hkVector4& pos1, const hkQuaternion& rot1,
	hkMotionState& ms);
// The largest velocity component a motion state may step with, in each lane (the SDK's extern of
// hkSweptTransformUtil.h, read by _stepMotionState and hkRigidMotionUtilStep's validity test).
inline hkQuadReal _stepMotionStateMaxVelf{1e6f, 1e6f, 1e6f, 1e6f};
} // namespace hkSweptTransformUtil

namespace hkSweptTransformUtil {
void lerp2Ha(const hkSweptTransform& sweptTrans, float t, float tAddOn, hkTransform& transformOut);
}  // namespace hkSweptTransformUtil

namespace hkSweptTransformUtil {
void lerp2Rel(const hkSweptTransform& sweptTrans, float r, hkTransform& transformOut);
}  // namespace hkSweptTransformUtil
