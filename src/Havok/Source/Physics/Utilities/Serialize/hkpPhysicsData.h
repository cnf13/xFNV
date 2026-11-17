#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

class hkpPhysicsSystem;
class hkpRigidBody;
class hkpWorld;
class hkpWorldCinfo;

hkClass* getPhysicsDataClass();

// A snapshot's world configuration and physics systems.
class hkpPhysicsData : public hkReferencedObject {
public:
	struct SplitPhysicsSystemsOutput {
		hkpPhysicsSystem* m_unconstrainedFixedBodies;
		hkpPhysicsSystem* m_unconstrainedKeyframedBodies;
		hkpPhysicsSystem* m_unconstrainedMovingBodies;
		hkpPhysicsSystem* m_phantoms;
		hkArray<hkpPhysicsSystem*> m_constrainedSystems;
	};

	// The reflection's member table (the PDB's hkpPhysicsData::Members).
	using MembersArray = const hkClassMember[2];

	hkpPhysicsData();
	// The SDK's finish constructor: a loaded object keeps the fields the load wrote.
	hkpPhysicsData(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_systems(aFlag) {}
	~hkpPhysicsData() override;

	hkpWorldCinfo* getWorldCinfo();
	void setWorldCinfo(hkpWorldCinfo* info);
	void addPhysicsSystem(hkpPhysicsSystem* system);
	void removePhysicsSystem(int32_t i);
	const hkArray<hkpPhysicsSystem*>& getPhysicsSystems() const;
	hkpPhysicsSystem* findPhysicsSystemByName(const char* name) const;
	hkpRigidBody* findRigidBodyByName(const char* name) const;
	void populateFromWorld(const hkpWorld* world, int32_t saveContactPoints = 0);
	hkpWorld* createWorld(hkBool registerAllAgents);

	static void splitPhysicsSystems(const hkpPhysicsSystem* inputSystemConst, SplitPhysicsSystemsOutput& output);

	hkpWorldCinfo* m_worldCinfo;
	hkArray<hkpPhysicsSystem*> m_systems;

	static MembersArray Members;
};

inline constexpr fnvd::Vtable hkpPhysicsData_vftable{};
extern hkClass hkpPhysicsDataClass;
extern hkTypeInfo hkpPhysicsDataTypeInfo;
