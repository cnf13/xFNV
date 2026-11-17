#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Havok/bhkArrowTargetListener.h"

class bhkRigidBody;
class NiAVObject;

// The arrows stuck in one target object: their combined mass and how many there are.
struct ArrowTargetGroup {
	bhkRigidBody* pBody;
	NiAVObject* pObject;
	float fTotalMass;
	int32_t iArrowCount;
};

// The game's arrow-target listener for Havok entity removal and deletion.
class TESArrowTargetListener : public bhkArrowTargetListener {
public:
	TESArrowTargetListener();
	~TESArrowTargetListener() override;

	void entityRemovedCallback(hkpEntity* apEntity) override;
	void entityDeletedCallback(hkpEntity* apEntity) override;
	void Update(float __formal) override {}

	// Adds an arrow's mass to apTargetObject's group (made on the first arrow): moves the body's centre of mass
	// toward the arrow and wakes the body.
	static void AddArrowTarget(hkVector4& aArrowCenterOfMassWorld, NiAVObject* apTargetObject);
	// Takes an arrow's mass back out of apTargetObject's group: moves the body's centre of mass back, pushes the
	// body away from the arrow, and frees the group once its last arrow is gone.
	static void RemoveArrowTarget(hkVector4& aArrowCenterOfMassWorld, NiAVObject* apTargetObject);

	static BSSimpleList<ArrowTargetGroup*> TargetList;
};
