#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkSmallArray.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Dynamics/Common/hkpMaterial.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

class hkdBreakableBody;
class hkLocalFrame;
class hkpAction;
class hkpBreakableBody;
class hkpConstraintInstance;
class hkpContactListener;
class hkpDynamicsContactMgr;
class hkpEntityActivationListener;
class hkpEntityListener;
class hkpShape;
class hkpSimulationIsland;
class hkpWorld;
class hkSpuCollisionCallbackUtil;
class hkStatisticsCollector;

// A simulated object of a Havok world. Only the entry points this code calls are declared.
class hkpEntity : public hkpWorldObject {
public:
	// The listeners of an entity, allocated once it has some: the activation listeners and the entity listeners.
	struct ExtendedListeners {
		ExtendedListeners();
		~ExtendedListeners();

		// Havok's class allocator: a block of the thread's heap. noexcept: a null block makes the new-expression null.
		static void* operator new(size_t aiNumBytes) noexcept;
		// The block goes back to the heap with the struct's size.
		static void operator delete(void* apMemory);
		// The reflection class hkpEntityExtendedListenersClass.
		static const hkClass& staticClass();

		hkSmallArray<hkpEntityActivationListener*> m_activationListeners;
		hkSmallArray<hkpEntityListener*> m_entityListeners;
	};

	// The events the SPU collision callback is sent.
	enum SpuCollisionCallbackEventFilter : int32_t {
		SPU_SEND_NONE = 0x0,
		SPU_SEND_CONTACT_POINT_ADDED = 0x1,
		SPU_SEND_CONTACT_POINT_PROCESS = 0x2,
		SPU_SEND_CONTACT_POINT_REMOVED = 0x4,
		SPU_SEND_CONTACT_POINT_ADDED_OR_PROCESS = 0x3,
	};

	// What the entity's collision callbacks tell the SPU: the callback utility, its capacity and the event and user
	// filters. hkUint16 and hkUint8 are plain integers here.
	struct SpuCollisionCallback {
		// The reflection class hkpEntitySpuCollisionCallbackClass.
		static const hkClass& staticClass();

		hkSpuCollisionCallbackUtil* m_util;
		uint16_t m_capacity;
		uint8_t m_eventFilter;
		uint8_t m_userFilter;
	};

	// What is substituted for a small array member when serializing: the small array's data, size and capacityAndFlags.
	struct SmallArraySerializeOverrideType {
		// The reflection class hkpEntitySmallArraySerializeOverrideTypeClass.
		static const hkClass& staticClass();

		void* m_data;
		uint16_t m_size;
		uint16_t m_capacityAndFlags;
	};

	hkpEntity(hkFinishLoadedObjectFlag aFlag);
	hkpEntity(const hkpShape* shape);
	~hkpEntity() override;

	// Havok SDK signature (hkpDynamics.lib, tied by bytes,string,vtable).
	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	hkMotionState* getMotionState() override { return nullptr; }
	virtual void deallocateInternalArrays();

	// Adds el to the entity listeners.
	void addEntityListener(hkpEntityListener* el);
	// The listeners, allocated on first use.
	ExtendedListeners* getExtendedListeners();
	hkpMotion* getMotion() { return &m_motion; }

	// Wakes the entity's island.
	void activate();
	void addEntityActivationListener(hkpEntityActivationListener* el);
	void addContactListener(hkpContactListener* cl);
	// Sets the maximum linear velocity.
	uint8_t* setMaxLinearVelocity(float afMaxLinearVelocity);
	// Removes apListener.
	void removeEntityListener(hkpEntityListener* el);
	// Removes apListener.
	void removeEntityActivationListener(hkpEntityActivationListener* el);
	// Removes apListener.
	void removeContactListener(hkpContactListener* cl);
	void deactivate();
	void activateAsCriticalOperation();
	void deactivateAsCriticalOperation();
	int32_t getNumConstraints() const;
	// Constraint i: the master constraints first, then the slave ones.
	hkpConstraintInstance* getConstraint(int32_t i);
	// The const overload: the same code at its own address.
	const hkpConstraintInstance* getConstraint(int32_t i) const;
	// Havok SDK.
	void getAllConstraints(hkArray<hkpConstraintInstance*>& constraints);
	void sortConstraintsSlavesDeterministically();
	hkpDynamicsContactMgr* findContactMgrTo(const hkpEntity* entity) const;
	hkdBreakableBody* getBreakableBody() const { return reinterpret_cast<hkdBreakableBody*>(m_breakableBody); }
	hkBool isActive() const;
	// Havok SDK.
	void setCachedShapeData(const hkpWorld* world, const hkpShape* shape);
	void updateCachedAabb();

	// The contact listeners added with addContactListener.
	const hkSmallArray<hkpContactListener*>& getContactListeners() const { return m_contactListeners; }
	int32_t getNumActions() const { return m_actions.getSize(); }
	hkpAction* getAction(int32_t i) { return m_actions.m_data[i]; }
	hkpMaterial& getMaterial() { return m_material; }
	const hkpMaterial& getMaterial() const { return m_material; }
	uint32_t getUid() const { return m_uid; }
	uint16_t getContactPointCallbackDelay() const { return m_contactPointCallbackDelay; }
	void setContactPointCallbackDelay(uint16_t delay) { m_contactPointCallbackDelay = delay; }
	hkpSimulationIsland* getSimulationIsland() const { return m_simulationIsland; }
	bool areContactListenersAdded() const { return m_contactListeners.getSize() > 0; }
	// The entity listeners and the activation listeners, allocated by getExtendedListeners on first use.
	const hkSmallArray<hkpEntityListener*>& getEntityListeners() { return getExtendedListeners()->m_entityListeners; }

	const hkSmallArray<hkpEntityActivationListener*>& getEntityActivationListeners() {
		return getExtendedListeners()->m_activationListeners;
	}

	const hkSmallArray<hkConstraintInternal>& getConstraintMasters() const { return m_constraintsMaster; }
	hkSmallArray<hkConstraintInternal>& getConstraintMastersRw() { return m_constraintsMaster; }
	const hkArray<hkpConstraintInstance*>& getConstraintSlaves() const { return m_constraintsSlave; }

	hkBool* isFixed(hkBool* apResult) const {
		apResult->m_bool = m_motion.m_type == hkpMotion::MOTION_FIXED;
		return apResult;
	}

	// Stores in apResult whether the motion is fixed or keyframed and returns it.
	hkBool* isFixedOrKeyframed(hkBool* apResult) const {
		apResult->m_bool =
			(m_motion.m_type == hkpMotion::MOTION_FIXED) || (m_motion.m_type == hkpMotion::MOTION_KEYFRAMED);
		return apResult;
	}

	// hkpEntity's data members end here; the rest of the object is the class's 16-byte alignment padding (m_motion
	// holds hkVector4s), which neither constructor touches.
	const hkSmallArray<hkConstraintInternal>& getConstraintMastersImpl();
	hkSmallArray<hkConstraintInternal>& getConstraintMastersRwImpl();
	const hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& getConstraintSlavesImpl();
	double getMotionMassInv();

	hkpMaterial m_material;
	void* m_limitContactImpulseUtil;
	float m_damageMultiplier;
	hkpBreakableBody* m_breakableBody;
	uint32_t m_solverData;
	uint16_t m_storageIndex;
	uint16_t m_contactPointCallbackDelay;
	hkSmallArray<hkConstraintInternal> m_constraintsMaster;
	hkArray<hkpConstraintInstance*> m_constraintsSlave;
	hkArray<uint8_t> m_constraintRuntime;
	// The island the entity is simulated in; null until the world has placed it.
	hkpSimulationIsland* m_simulationIsland;
	int8_t m_autoRemoveLevel;
	uint8_t m_numShapeKeysInContactPointProperties;
	uint8_t m_responseModifierFlags;
	uint8_t paddingD3[1];
	uint32_t m_uid;
	SpuCollisionCallback m_spuCollisionCallback;
	// The entity's motion, held in place.
	hkpMaxSizeMotion m_motion;
	hkSmallArray<hkpContactListener*> m_contactListeners;
	hkSmallArray<hkpAction*> m_actions;
	// hkRefPtr<hkLocalFrame> in the SDK.
	hkLocalFrame* m_localFrame;
	// Allocated by getExtendedListeners on first use.
	ExtendedListeners* m_extendedListeners;
};

hkBool* hkEntity_isActive(const hkpEntity* entity, hkBool* apResult);

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpEntitySmallArraySerializeOverrideTypeTypeInfo;
extern hkTypeInfo hkpEntitySpuCollisionCallbackTypeInfo;
extern hkTypeInfo hkpEntityExtendedListenersTypeInfo;
