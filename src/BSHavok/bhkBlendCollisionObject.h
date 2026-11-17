#pragma once

#include "BSHavok/bhkCollisionObject.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

class bhkWorld;

void SetStoredWorldRecurse(NiAVObject* a1, bhkWorld* a2);

// The default gain and velocity gain of one ragdoll part.
struct BLENDVALUES {
	float fGain;
	float fVelGain;
};

// A collision object that blends between the animation's pose and the ragdoll's simulated pose.
class bhkBlendCollisionObject : public bhkCollisionObject {
public:
	// The default gains per ragdoll part, indexed with bits 8-12 of the collision filter info.
	using DefaultBlendArray = BLENDVALUES[32];

	bhkBlendCollisionObject();
	~bhkBlendCollisionObject() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	bhkBlendCollisionObject* IsBhkBlendCollisionObject() override { return this; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void AddViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void UpdateWorldData(NiUpdateData& arUpdateData) override;
	void hktoNiTransform() override;
	void SetMotionType(hkpMotion::MotionType aeType, bhkWorldObject* apObject, bool abAllowActivate) override;

	// Copies the members into apDest when cloning; it does only what bhkCollisionObject's CopyMembers does.
	void CopyMembers(bhkBlendCollisionObject* apDest, NiCloningProcess& arCloneData);
	float GetHeirGain() { return fHeirGain; }
	void UpdateAddedToWorld();
	void ForceUpdateWorldData();
	void SynchronizeMotionType();
	// Counts a forced add up; the first one updates the world data and re-adds a biped body.
	void IncForceAdd();
	// Counts a forced add down; at zero a biped body is re-added.
	void DecForceAdd();

	// The class loader's factory: a new object, or null when the allocation fails.
	static NiObject* CreateObject();
	// The blend value of the blend collision object on apRoot (or its child apName).
	static float GetBlendValue(NiAVObject* apRoot, const char* apName);

	bool bRagdollUpdated;
	uint8_t padding15[3];
	float fHeirGain;
	float fVelGain;
	// The Havok motion type the body had before the blend changed it.
	hkpMotion::MotionType ePrevType;
	bhkWorld* pStoredWorld;
	int32_t iForceAddCount;

	static NiRTTI ms_RTTI;
	// The object counter the constructor increments.
	static inline int32_t iCount;
	static inline DefaultBlendArray pDefaultBlendA{{1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f},
		{1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f},
		{1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f},
		{1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f},
		{1.0f, 1.0f}};
};
