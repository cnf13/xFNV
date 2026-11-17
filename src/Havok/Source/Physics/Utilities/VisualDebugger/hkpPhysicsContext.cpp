#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Common/Visualize/hkProcessFactory.h"
#include "Havok/Source/Common/Visualize/hkVisualDebugger.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpActionListener.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstanceClass.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityClass.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldDeletionListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCinfo.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpActiveContactPointViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpBroadphaseViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpConvexRadiusViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpInactiveContactPointViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpMidphaseViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpShapeDisplayViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpToiContactPointViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpToiCountViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpWeldingViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpConstraintViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpPhantomDisplayViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpRigidBodyCentreOfMassViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpRigidBodyInertiaViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpSimulationIslandViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpSweptTransformDisplayViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldMemoryViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldSnapshotViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Utilities/hkpMousePickingViewer.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Vehicle/hkpVehicleViewer.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpPhysicsSystem.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"

hkpPhysicsContextWorldListener::~hkpPhysicsContextWorldListener() {}

void hkpPhysicsContextWorldListener::operator delete(void* apMemory) {
	ourFree(apMemory);
}

hkpPhysicsContext::hkpPhysicsContext() {}

// Removing each world also unregisters the context's listeners from it; the arrays free themselves.
hkpPhysicsContext::~hkpPhysicsContext() {
	for (int32_t i = m_worlds.m_size - 1; i >= 0; --i) {
		removeWorld(m_worlds.m_data[i]);
	}
}

void hkpPhysicsContext::setOwner(hkVisualDebugger* vdb) {
	if (m_owner) {
		for (int32_t i = 0; i < m_worlds.m_size; ++i) {
			removeFromInspection(m_worlds.m_data[i]);
		}
	}
	m_owner = vdb;
	if (vdb) {
		for (int32_t j = 0; j < m_worlds.m_size; ++j) {
			addForInspection(m_worlds.m_data[j]);
		}
	}
}

void hkpPhysicsContext::worldDeletedCallback(hkpWorld* world) {
	removeWorld(world);
}

void hkpPhysicsContext::entityAddedCallback(hkpEntity* entity) {
	hkVisualDebugger* m_owner = this->m_owner;
	if (m_owner) {
		// The SDK's tag id is an hkUlong: the collidable's address.
		m_owner->addTrackedObject(entity, hkpEntityClass, "Entities",
			reinterpret_cast<uintptr_t>(&entity->m_collidable));
	}
}

void hkpPhysicsContext::phantomAddedCallback(hkpPhantom* phantom) {
	hkVisualDebugger* m_owner = this->m_owner;
	if (m_owner) {
		// The SDK's tag id is an hkUlong: the collidable's address.
		m_owner->addTrackedObject(phantom, hkpPhantomClass, "Phantoms",
			reinterpret_cast<uintptr_t>(&phantom->m_collidable));
	}
}

void hkpPhysicsContext::constraintAddedCallback(hkpConstraintInstance* constraint) {
	if (m_owner) {
		hkpConstraintData* m_data = constraint->getDataRw();
		if (m_data) {
			if (m_data->getType() != 11) {
				m_owner->addTrackedObject(constraint, hkpConstraintInstanceClass, "Constraints", 0);
			}
		}
	}
}

void hkpPhysicsContext::constraintRemovedCallback(hkpConstraintInstance* constraint) {
	if (m_owner) {
		hkpConstraintData* m_data = constraint->getDataRw();
		if (m_data) {
			if (m_data->getType() != 11) {
				m_owner->removeTrackedObject(constraint);
			}
		}
	}
}

void hkpPhysicsContext::actionAddedCallback(hkpAction* action) {
	hkVisualDebugger* m_owner = this->m_owner;
	if (m_owner) {
		m_owner->addTrackedObject(action, hkpActionClass, "Actions", 0);
	}
}

void hkpPhysicsContext::addForInspection(hkpWorld* w) {
	if (this->m_owner) {
		if (w) {
			w->lock();
			hkpEntityListener* v19 = static_cast<hkpEntityListener*>(this);
			w->addEntityListener(this);
			hkpPhantomListener* v16 = static_cast<hkpPhantomListener*>(this);
			w->addPhantomListener(this);
			hkpActionListener* v17 = static_cast<hkpActionListener*>(this);
			w->addActionListener(this);
			hkpConstraintListener* v18 = static_cast<hkpConstraintListener*>(this);
			w->addConstraintListener(this);
			hkArray<hkpWorldCinfo, hkContainerHeapAllocator>* p_m_worldCinfos = &m_worldCinfos;
			hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
			if (m_worldCinfos.m_size ==
				(m_worldCinfos.m_capacityAndFlags & hkArrayBase<hkpWorldCinfo>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*Instance->m_heap, &m_worldCinfos, sizeof(hkpWorldCinfo));
			}
			hkpWorldCinfo* v6 = &p_m_worldCinfos->m_data[m_worldCinfos.m_size];
			if (v6) {
				::new (v6) hkpWorldCinfo;
			}
			int32_t m_size = m_worldCinfos.m_size;
			hkpWorldCinfo* v8 = &p_m_worldCinfos->m_data[m_size];
			m_worldCinfos.m_size = m_size + 1;
			w->getCinfo(*v8);
			hkVisualDebugger* m_owner = this->m_owner;
			if (m_owner) {
				m_owner->addTrackedObject(v8, hkpWorldCinfoClass, "hkpWorldCinfo", 0);
			}
			hkpPhysicsSystem* WorldAsOneSystem = w->getWorldAsOneSystem();
			for (int32_t i = 0; i < WorldAsOneSystem->m_rigidBodies.m_size; ++i) {
				v19->entityAddedCallback(WorldAsOneSystem->m_rigidBodies.m_data[i]);
			}
			for (int32_t j = 0; j < WorldAsOneSystem->m_phantoms.m_size; ++j) {
				v16->phantomAddedCallback(WorldAsOneSystem->m_phantoms.m_data[j]);
			}
			for (int32_t k = 0; k < WorldAsOneSystem->m_actions.m_size; ++k) {
				v17->actionAddedCallback(WorldAsOneSystem->m_actions.m_data[k]);
			}
			for (int32_t m = 0; m < WorldAsOneSystem->m_constraints.m_size; ++m) {
				v18->constraintAddedCallback(WorldAsOneSystem->m_constraints.m_data[m]);
			}
			WorldAsOneSystem->removeReference();
			w->unlock();
		}
	}
}

void hkpPhysicsContext::addWorld(hkpWorld* newWorld) {
	hkpWorld** m_data;
	int32_t m_size = m_worlds.m_size;
	hkArray<hkpWorld*, hkContainerHeapAllocator>* p_m_worlds = &m_worlds;
	int32_t v5 = 0;
	if (m_size <= 0) {
		goto LABEL_7;
	}
	m_data = p_m_worlds->begin();
	while (*m_data != newWorld) {
		++v5;
		++m_data;
		if (v5 >= m_size) {
			goto LABEL_7;
		}
	}
	if (v5 < 0) {
		hkpWorldDeletionListener* v7;
	LABEL_7:;
		if (this) {
			v7 = static_cast<hkpWorldDeletionListener*>(this);
		} else {
			v7 = 0;
		}
		newWorld->addWorldDeletionListener(v7);
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		if (m_worlds.m_size == (m_worlds.m_capacityAndFlags & hkArrayBase<hkpWorld*>::CAPACITY_MASK)) {
			hkArrayUtil::_reserveMore(*Instance->m_heap, &m_worlds, sizeof(hkpWorld*));
		}
		hkpWorld** v9 = p_m_worlds->begin();
		m_worlds.m_data[m_worlds.m_size++] = newWorld;
		for (int32_t i = 0; i < m_addListeners.m_size; ++i) {
			hkpPhysicsContextWorldListener* v11 = m_addListeners.m_data[i];
			v11->worldAddedCallback(newWorld);
		}
		addForInspection(newWorld);
	}
}

int32_t hkpPhysicsContext::findWorld(hkpWorld* world) {
	int32_t m_size = m_worlds.m_size;
	int32_t result = 0;
	if (m_size <= 0) {
		return -1;
	}
	for (hkpWorld** i = m_worlds.begin(); *i != world; ++i) {
		if (++result >= m_size) {
			return -1;
		}
	}
	return result;
}

// Stops inspecting w: the four listeners come off the world, the world's tracked cinfo goes (when there is an owner and
// the world is one of ours), and the listeners hear about every rigid body, phantom, action and constraint of the
// world as removed. The callbacks are virtual calls through the listener subobjects.
void hkpPhysicsContext::removeFromInspection(hkpWorld* w) {
	if (m_owner && w) {
		hkpEntityListener* pEntityListener = static_cast<hkpEntityListener*>(this);
		w->removeEntityListener(pEntityListener);
		hkpPhantomListener* pPhantomListener = static_cast<hkpPhantomListener*>(this);
		w->removePhantomListener(pPhantomListener);
		hkpActionListener* pActionListener = static_cast<hkpActionListener*>(this);
		w->removeActionListener(pActionListener);
		hkpConstraintListener* pConstraintListener = static_cast<hkpConstraintListener*>(this);
		w->removeConstraintListener(pConstraintListener);
		// EXE-INLINED 0xCD2050
		int32_t iWorld = -1;
		for (int32_t i = 0; i < m_worlds.m_size; ++i) {
			if (m_worlds.m_data[i] == w) {
				iWorld = i;
				break;
			}
		}
		if (m_owner && iWorld != -1) {
			m_owner->removeTrackedObject(&m_worldCinfos.m_data[iWorld]);
			// EXE-INLINED none: hkArrayBase<hkpWorldCinfo>::removeAt (the element's destructor through its vtable)
			hkpWorldCinfo* pCinfo = &m_worldCinfos.m_data[iWorld];
			pCinfo->~hkpWorldCinfo();
			--m_worldCinfos.m_size;
			if (m_worldCinfos.m_size != iWorld) {
				hkMemUtil::memCpyOneAligned<sizeof(hkpWorldCinfo), alignof(hkpWorldCinfo)>(&m_worldCinfos
																							   .m_data[iWorld],
					&m_worldCinfos.m_data[m_worldCinfos.m_size]);
			}
		}
		hkpPhysicsSystem* pSystem = w->getWorldAsOneSystem();
		for (int32_t i = 0; i < pSystem->m_rigidBodies.m_size; ++i) {
			pEntityListener->entityRemovedCallback(pSystem->m_rigidBodies.m_data[i]);
		}
		for (int32_t j = 0; j < pSystem->m_phantoms.m_size; ++j) {
			pPhantomListener->phantomRemovedCallback(pSystem->m_phantoms.m_data[j]);
		}
		for (int32_t k = 0; k < pSystem->m_actions.m_size; ++k) {
			pActionListener->actionRemovedCallback(pSystem->m_actions.m_data[k]);
		}
		for (int32_t m = 0; m < pSystem->m_constraints.m_size; ++m) {
			pConstraintListener->constraintRemovedCallback(pSystem->m_constraints.m_data[m]);
		}
		pSystem->removeReference();
	}
}

void hkpPhysicsContext::removeWorld(hkpWorld* oldWorld) {
	int32_t m_size = m_worlds.m_size;
	int32_t v4 = 0;
	if (m_size > 0) {
		for (hkpWorld** i = m_worlds.begin(); *i != oldWorld; ++i) {
			if (++v4 >= m_size) {
				return;
			}
		}
		if (v4 >= 0) {
			hkpWorldDeletionListener* v6;
			if (this) {
				v6 = static_cast<hkpWorldDeletionListener*>(this);
			} else {
				v6 = 0;
			}
			oldWorld->removeWorldDeletionListener(v6);
			for (int32_t j = 0; j < m_addListeners.m_size; ++j) {
				hkpPhysicsContextWorldListener* v8 = m_addListeners.m_data[j];
				v8->worldRemovedCallback(oldWorld);
			}
			removeFromInspection(oldWorld);
			int32_t v9 = --m_worlds.m_size;
			if (v9 != v4) {
				m_worlds.m_data[v4] = m_worlds.m_data[v9];
			}
		}
	}
}

void hkpPhysicsContext::addWorldAddedListener(hkpPhysicsContextWorldListener* cb) {
	hkpPhysicsContextWorldListener** m_data;
	hkArray<hkpPhysicsContextWorldListener*, hkContainerHeapAllocator>* p_m_addListeners = &m_addListeners;
	int32_t m_size = m_addListeners.m_size;
	int32_t v4 = 0;
	if (m_size <= 0) {
		goto LABEL_7;
	}
	m_data = p_m_addListeners->begin();
	while (*m_data != cb) {
		++v4;
		++m_data;
		if (v4 >= m_size) {
			goto LABEL_7;
		}
	}
	if (v4 < 0) {
	LABEL_7:;
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		if (p_m_addListeners->m_size ==
			(p_m_addListeners->m_capacityAndFlags & hkArrayBase<hkpPhysicsContextWorldListener*>::CAPACITY_MASK)) {
			hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_addListeners, sizeof(hkpPhysicsContextWorldListener*));
		}
		p_m_addListeners->m_data[p_m_addListeners->m_size++] = cb;
	}
}

void hkpPhysicsContext::removeWorldAddedListener(hkpPhysicsContextWorldListener* cb) {
	int32_t m_size = m_addListeners.m_size;
	int32_t v3 = 0;
	if (m_size > 0) {
		for (hkpPhysicsContextWorldListener** i = m_addListeners.begin(); *i != cb; ++i) {
			if (++v3 >= m_size) {
				return;
			}
		}
		if (v3 >= 0) {
			int32_t v5 = --m_addListeners.m_size;
			if (v5 != v3) {
				m_addListeners.m_data[v3] = m_addListeners.m_data[v5];
			}
		}
	}
}

void hkpPhysicsContext::registerAllPhysicsProcesses() {
	hkpBroadphaseViewer::registerViewer();
	hkpMidphaseViewer::registerViewer();
	hkpRigidBodyCentreOfMassViewer::registerViewer();
	hkpConstraintViewer::registerViewer();
	hkpConvexRadiusViewer::registerViewer();
	hkpActiveContactPointViewer::registerViewer();
	hkpInactiveContactPointViewer::registerViewer();
	hkpToiContactPointViewer::registerViewer();
	hkpToiCountViewer::registerViewer();
	hkpRigidBodyInertiaViewer::registerViewer();
	hkpMousePickingViewer::registerViewer();
	hkpPhantomDisplayViewer::registerViewer();
	hkpShapeDisplayViewer::registerViewer();
	hkpSimulationIslandViewer::registerViewer();
	hkpSweptTransformDisplayViewer::registerViewer();
	hkpVehicleViewer::registerViewer();
	hkpWorldMemoryViewer::registerViewer();
	hkpWorldSnapshotViewer::registerViewer();
	hkpWeldingViewer::registerViewer();
}

hkpActionListener::~hkpActionListener() {}

void hkpActionListener::operator delete(void* apMemory) {
	ourFree(apMemory);
}

hkpConstraintListener::~hkpConstraintListener() {}

void hkpConstraintListener::operator delete(void* apMemory) {
	ourFree(apMemory);
}
