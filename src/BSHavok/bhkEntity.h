#pragma once

#include "BSHavok/bhkWorldObject.h"
#include "Reconstruction.hpp"

class hkpContactListener;
class hkpEntityActivationListener;
class hkpEntityListener;

// A world object that is simulated as a body (rigid bodies and their kin); it adds no data to bhkWorldObject.
class bhkEntity : public bhkWorldObject {
public:
	bhkEntity();
	~bhkEntity() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	bool Remove() override;
	void Kill() override;

	// Tells the world to re-evaluate the wrapped entity's collision pairs after its filter info changed.
	void UpdateCFilter() override;

	// IDA name; not in the PDB (IDA types this as bhkRefObject). Adds apListener to the wrapped entity.
	// Inferred type: apListener: hkpEntityListener* from 0x6c1d86 (NavMeshObstacleManager::pRemoveListener (this of
	// NavMeshObstacleManager::AddActivationListener))
	void AddEntityListener(hkpEntityListener* apListener);
	// Removes apListener from the wrapped entity's listeners.
	void RemoveEntityListener(hkpEntityListener* apList);
	void AddEntityActivationListener(hkpEntityActivationListener* apList);
	void AddContactListener(hkpContactListener* apList);
	// Deactivates the wrapped entity; nothing for a null wrapper or no entity.
	void Deactivate();
	// Removes apList from the wrapped entity's activation listeners.
	void RemoveEntityActivationListener(hkpEntityActivationListener* apList);

	// The wrapper of apEnt.
	static bhkEntity* Getbhk(const hkpWorldObject* apEnt);

	static NiRTTI ms_RTTI;
	// Live wrappers of this class, counted by the inlined constructors.
	static inline uint32_t iCount;
};

inline constexpr fnvd::Vtable bhkEntity_vftable{};
