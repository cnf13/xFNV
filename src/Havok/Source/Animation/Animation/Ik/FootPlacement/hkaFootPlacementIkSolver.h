#pragma once

#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

class hkaPose;
class hkaRaycastInterface;
class hkaSkeleton;

// Havok's ray cast interface for animation (the PDB puts this class in bhkRagdollController.cpp).
class hkaRaycastInterface {
public:
	virtual ~hkaRaycastInterface();

	virtual hkBool castRay(const hkVector4&, const hkVector4&, float&, hkVector4&);
	virtual hkBool castRay(const hkVector4&, const hkVector4&, uint32_t, float&, hkVector4&);

	static void operator delete(void* apMemory);
};

// Havok's foot placement solver for one leg: it casts the foot onto the ground, eases the ankle height and locks
// the planted foot. The hkVector4 members keep the 16-byte alignment the exe's layout has.
class hkaFootPlacementIkSolver : public hkReferencedObject {
public:
	// What one doFootPlacement call is given: the foot's transform in model and world space, whether to place the
	// foot, the raycast interface and the gains that ease the ankle.
	struct Input {
		Input();

		hkQsTransform m_originalAnkleTransformMS;
		// The transform that converts from model to world.
		hkQsTransform m_worldFromModel;
		// If false, foot placement is eased out or kept off.
		bool m_footPlacementOn;
		// Casts the foot onto the ground the user wants to place feet on.
		hkaRaycastInterface* m_raycastInterface;
		// Gain of the on/off transition (0.2 by default).
		float m_onOffGain;
		// Gain when the ground rises, and when it falls (1.0 by default).
		float m_groundAscendingGain;
		float m_groundDescendingGain;
		// Gains when the foot is fully planted and fully raised (1.0 by default).
		float m_footPlantedGain;
		float m_footRaisedGain;
		// Gain of the horizontal move when the foot unlocks (1.0 by default).
		float m_footUnlockGain;
		// Passed to hkaRaycastInterface::castRay when m_useCollisionFilterInfo.
		uint32_t m_collisionFilterInfo;
		bool m_useCollisionFilterInfo;
	};

	// The leg's bones and the heights and angles the solver works within.
	struct Setup {
		Setup();

		const hkaSkeleton* m_skeleton;
		int16_t m_hipIndex;
		int16_t m_kneeIndex;
		int16_t m_ankleIndex;
		hkVector4 m_kneeAxisLS;
		hkVector4 m_footEndLS;
		hkVector4 m_worldUpDirectionWS;
		hkVector4 m_modelUpDirectionMS;
		float m_originalGroundHeightMS;
		float m_footPlantedAnkleHeightMS;
		float m_footRaisedAnkleHeightMS;
		float m_maxAnkleHeightMS;
		float m_minAnkleHeightMS;
		float m_cosineMaxKneeAngle;
		float m_cosineMinKneeAngle;
		float m_raycastDistanceUp;
		float m_raycastDistanceDown;
		bool m_useFootLocking;
	};

	// What one doFootPlacement call reports back: how far the foot ended up from the ground and whether the
	// downward cast hit anything (the PDB's hkaFootPlacementIkSolver::Output, whose m_verticalError is an hkReal, a
	// float; the Havok SDK declares the same two members).
	struct Output {
		float m_verticalError;
		bool m_hitSomething;
	};

	hkaFootPlacementIkSolver(const hkaFootPlacementIkSolver::Setup& setup);
	~hkaFootPlacementIkSolver() override;

	bool castFoot(const hkaFootPlacementIkSolver::Input& input, const hkVector4& footWS, const hkVector4& footEndWS,
		hkVector4& projectedFootWSOut, hkVector4& groundNormalWSOut) const;
	void doFootPlacement(const hkaFootPlacementIkSolver::Input& input, hkaFootPlacementIkSolver::Output& output,
		hkaPose& poseInOut);

	Setup m_setup;
	float m_currentWeight;
	float m_previousGroundHeightWS;
	hkVector4 m_previousGroundNormalWS;
	float m_previousVerticalDisplacement;
	hkBool m_isFootLocked;
	hkVector4 m_lockedFootPositionInWS;
	hkVector4 m_lockedFootEndPositionInWS;
	hkVector4 m_footUnlockingOffset;
};
inline constexpr fnvd::Vtable hkaFootPlacementIkSolver_vftable{};

inline constexpr fnvd::Vtable hkaRaycastInterface_vftable{};
