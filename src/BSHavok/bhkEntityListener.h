#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Reconstruction.hpp"

class bhkEntity;
class bhkRigidBody;
struct hkpRootCdPoint;

// An entity listener that keeps the rigid bodies it is told about; the bodies it accepts come from GetRB.
class bhkEntityListener : public hkpEntityListener {
public:
	bhkEntityListener(uint32_t a1);
	~bhkEntityListener() override;

	void entityAddedCallback(hkpEntity* apEntity) override;
	void entityRemovedCallback(hkpEntity* apEntity) override;
	void entityDeletedCallback(hkpEntity*) override {}

	virtual void Update(float afTimeDelta) = 0;
	virtual bool contactPointAddedCallback(hkpRootCdPoint&) { return false; }
	virtual bool contactPointRemovedCallback(hkpRootCdPoint&) { return false; }
	// the rigid body of an entity whose collision layer the listener's flags accept, else null
	virtual bhkRigidBody* GetRB(hkpEntity* apEntity);

	// Adds apEntity to the listener's list, which holds a reference on it.
	void AddEntity(bhkEntity* apEntity);
	// Removes apEntity from the listener's list.
	void RemoveEntity(bhkEntity* apEntity);
	// bEnabled = abTF (thiscall, ret 4; the exe's shared +8 byte setter 0x937090).
	void SetEnabled(bool abTF);

	// The exe's deleting destructor frees through ourFree.
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	uint32_t iFlag;
	bool bEnabled;
	BSSimpleList<NiPointer<bhkEntity>> Entities;
};
inline constexpr fnvd::Vtable bhkEntityListener_vftable{};
