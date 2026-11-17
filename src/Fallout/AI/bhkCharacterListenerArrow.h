#pragma once

#include "BSHavok/bhkCharacterController.h"
#include "Reconstruction.hpp"

class ArrowProjectile;
struct hkpSimplexSolverInput;

// The character controller of an arrow projectile: it reports what the arrow touches to its projectile.
// ArrowProjectile::InitHavok (0x8CAEA0) allocates 0x660 bytes (16-aligned). The exe's vtables are 0x1086684
// (primary), 0x1086674 (hkpCharacterContext at 0x3E0) and 0x1086658 (bhkCharacterListener at 0x410). The primary
// table has 56 slots, three more than bhkCharacterController's: slots 53 to 55 are Havok's older listener callbacks,
// without the proxy argument. The deleting destructor 0x8D1130 is shared with bhkCharacterListenerSpell's.
class bhkCharacterListenerArrow : public bhkCharacterController {
public:
	bhkCharacterListenerArrow(bhkCharacterControllerCinfo& arInfo, ArrowProjectile* apProjectile);
	~bhkCharacterListenerArrow() override;

	// slot 53.
	virtual void processConstraintsCallback(const hkArray<hkpRootCdPoint>& arManifold, hkpSimplexSolverInput& arInput);
	// slot 54: hands the contact to ProcessCollision unless the arrow is stuck or disabled.
	virtual void contactPointAddedCallback(const hkpRootCdPoint& point);
	// slot 55: does nothing.
	virtual void contactPointRemovedCallback(const hkpRootCdPoint& point) {}
	using bhkCharacterController::contactPointAddedCallback;
	using bhkCharacterController::contactPointRemovedCallback;
	using bhkCharacterController::processConstraintsCallback;

	// The listener's interaction slots 4 and 5 (bhkCharacterListener subobject at 0x410) both point to the nullsub
	// 0x8D05E0 (ret 0xC).
	void characterInteractionCallback(hkpCharacterProxy* apProxy, hkpCharacterProxy* apOtherProxy,
		const hkContactPoint& arContact) override {}

	void objectInteractionCallback(hkpCharacterProxy* apProxy, const hkpCharacterObjectInteractionEvent& arInput,
		hkpCharacterObjectInteractionResult& arOutput) override {}

	// Reacts to the arrow touching apCollidee at aContactPoint.
	static void ProcessCollision(ArrowProjectile* apProjectile, const hkpCollidable* apCollidee,
		const hkVector4& aContactPoint, const hkVector4& aContactNormal);

	ArrowProjectile* pProj;
};

inline constexpr fnvd::Vtable bhkCharacterListenerArrow_vftable{};
inline constexpr fnvd::Vtable bhkCharacterListenerArrow_vftable_hkpCharacterContext{};
inline constexpr fnvd::Vtable bhkCharacterListenerArrow_vftable_bhkCharacterListener{};
