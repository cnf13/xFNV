#pragma once

#include "BSHavok/bhkCharacterController.h"

class ArrowProjectile;
struct hkpSimplexSolverInput;

// The character controller of an arrow projectile: it reports what the arrow touches to its projectile. It has three
// more virtual functions than bhkCharacterController: Havok's older listener callbacks, without the proxy argument.
class bhkCharacterListenerArrow : public bhkCharacterController {
public:
	bhkCharacterListenerArrow(bhkCharacterControllerCinfo& arInfo, ArrowProjectile* apProjectile);
	~bhkCharacterListenerArrow() override;

	virtual void processConstraintsCallback(const hkArray<hkpRootCdPoint>& arManifold, hkpSimplexSolverInput& arInput);
	// Hands the contact to ProcessCollision unless the arrow is stuck or disabled.
	virtual void contactPointAddedCallback(const hkpRootCdPoint& point);
	virtual void contactPointRemovedCallback(const hkpRootCdPoint& point) {}
	using bhkCharacterController::contactPointAddedCallback;
	using bhkCharacterController::contactPointRemovedCallback;
	using bhkCharacterController::processConstraintsCallback;

	void characterInteractionCallback(hkpCharacterProxy* apProxy, hkpCharacterProxy* apOtherProxy,
		const hkContactPoint& arContact) override {}

	void objectInteractionCallback(hkpCharacterProxy* apProxy, const hkpCharacterObjectInteractionEvent& arInput,
		hkpCharacterObjectInteractionResult& arOutput) override {}

	// Reacts to the arrow touching apCollidee at aContactPoint.
	static void ProcessCollision(ArrowProjectile* apProjectile, const hkpCollidable* apCollidee,
		const hkVector4& aContactPoint, const hkVector4& aContactNormal);

	ArrowProjectile* pProj;
};
