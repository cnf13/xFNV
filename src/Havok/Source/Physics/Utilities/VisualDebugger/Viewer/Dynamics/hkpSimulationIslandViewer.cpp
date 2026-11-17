#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpSimulationIslandViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayAABB.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"

#include <xmmintrin.h>

hkpSimulationIslandViewer::hkpSimulationIslandViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>&
		contexts)
	: hkpWorldViewerBase(contexts), m_showActiveIslands(true), m_showInactiveIslands(true) {
	if (m_context) {
		for (int32_t i = 0; i < m_context->m_worlds.m_size; i++) {
			m_context->m_worlds.m_data[i]->addWorldPostSimulationListener(this);
		}
	}
}

hkpSimulationIslandViewer::~hkpSimulationIslandViewer() {
	if (m_context) {
		for (int32_t i = 0; i < m_context->m_worlds.m_size; i++) {
			m_context->m_worlds.m_data[i]->removeWorldPostSimulationListener(this);
		}
	}
}

// Draws a box around each active island (blue) and each inactive one (green), when shown: the union of its entities'
// broad phase boxes, starting from an empty box. The display geometries grow to the island count as needed. The active
// pass also fills a temporary array of empty boxes that nothing reads.
void hkpSimulationIslandViewer::postSimulationCallback(hkpWorld* world) {
	HK_TIMER_BEGIN("hkpSimulationIslandViewer", this);
	if (m_showActiveIslands) {
		int32_t numIslands = world->m_activeSimulationIslands.m_size;
		if (numIslands > m_activeIslandDisplayGeometries.m_size) {
			// EXE-INLINED none: hkArray<hkDisplayAABB>::setSize
			hkArray<hkDisplayAABB>& geometries = m_activeIslandDisplayGeometries;
			hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
			int32_t capacity =
				static_cast<int32_t>(geometries.m_capacityAndFlags & hkArrayBase<hkDisplayAABB>::CAPACITY_MASK);
			if (capacity < numIslands) {
				int32_t newCapacity = capacity * 2;
				if (numIslands >= newCapacity) {
					newCapacity = numIslands;
				}
				hkArrayUtil::_reserve(heap, &geometries, newCapacity, sizeof(hkDisplayAABB));
			}
			hkDisplayAABB* dead = geometries.m_data + numIslands;
			for (int32_t k = geometries.m_size - numIslands; k > 0; k--) {
				dead->~hkDisplayAABB();
				dead++;
			}
			hkDisplayAABB* fresh = geometries.m_data + geometries.m_size;
			for (int32_t k = numIslands - geometries.m_size; k > 0; k--) {
				if (fresh != nullptr) {
					::new (fresh) hkDisplayAABB;
				}
				fresh++;
			}
			geometries.m_size = numIslands;
		}

		// EXE-INLINED none: hkArray<hkDisplayGeometry*, hkContainerTempAllocator>::hkArray
		using displayGeometries_t = hkArray<hkDisplayGeometry*, hkContainerTempAllocator>;
		FNVD_INLINED_LOCAL(displayGeometries_t, displayGeometries, std::nullopt, std::nullopt);
		displayGeometries.m_data = nullptr;
		displayGeometries.m_size = 0;
		displayGeometries.m_capacityAndFlags = hkArrayBase<hkDisplayGeometry*>::DONT_DEALLOCATE_FLAG;
		// EXE-INLINED none: hkArray<hkDisplayGeometry*, hkContainerTempAllocator>::setSize
		numIslands = world->m_activeSimulationIslands.m_size;
		hkMemoryAllocator* temp = hkMemoryRouter::getInstance().m_temp;
		int32_t capacity =
			static_cast<int32_t>(displayGeometries.m_capacityAndFlags & hkArrayBase<hkDisplayGeometry*>::CAPACITY_MASK);
		if (capacity < numIslands) {
			int32_t newCapacity = capacity * 2;
			if (numIslands >= newCapacity) {
				newCapacity = numIslands;
			}
			hkArrayUtil::_reserve(temp, &displayGeometries, newCapacity, sizeof(hkDisplayGeometry*));
		}
		displayGeometries.m_size = numIslands;

		// EXE-INLINED none: hkArray<hkAabb, hkContainerTempAllocator>::hkArray
		using islandAabbs_t = hkArray<hkAabb, hkContainerTempAllocator>;
		FNVD_INLINED_LOCAL(islandAabbs_t, islandAabbs, std::nullopt, std::nullopt);
		islandAabbs.m_data = nullptr;
		islandAabbs.m_size = 0;
		islandAabbs.m_capacityAndFlags = hkArrayBase<hkAabb>::DONT_DEALLOCATE_FLAG;
		// EXE-INLINED none: hkArray<hkAabb, hkContainerTempAllocator>::setSize
		numIslands = world->m_activeSimulationIslands.m_size;
		temp = hkMemoryRouter::getInstance().m_temp;
		capacity = static_cast<int32_t>(islandAabbs.m_capacityAndFlags & hkArrayBase<hkAabb>::CAPACITY_MASK);
		if (capacity < numIslands) {
			int32_t newCapacity = capacity * 2;
			if (numIslands >= newCapacity) {
				newCapacity = numIslands;
			}
			hkArrayUtil::_reserve(temp, &islandAabbs, newCapacity, sizeof(hkAabb));
		}
		islandAabbs.m_size = numIslands;

		for (int32_t i = 0; i < world->m_activeSimulationIslands.m_size; i++) {
			hkpSimulationIsland* island = world->m_activeSimulationIslands.m_data[i];
			islandAabbs.m_data[i].m_max.m_quad = _mm_set1_ps(rdata_104E148);
			islandAabbs.m_data[i].m_min.m_quad = _mm_set1_ps(rdata_104DA48);
			hkVector4 islandMax;
			islandMax.m_quad = _mm_set1_ps(rdata_104E148);
			hkVector4 islandMin;
			islandMin.m_quad = _mm_set1_ps(rdata_104DA48);
			hkInplaceArray<hkpEntity*, 1>& entities = island->m_entities;
			for (int32_t j = 0; j < entities.m_size; j++) {
				hkpCollidable* collidable = &entities.m_data[j]->m_collidable;
				if (collidable->m_shape != nullptr) {
					hkAabb entityAabb;
					world->m_broadPhase->getAabb(collidable->getBroadPhaseHandle(), entityAabb);
					islandMax.setMax4(islandMax, entityAabb.m_max);
					islandMin.setMin4(islandMin, entityAabb.m_min);
				}
			}
			m_activeIslandDisplayGeometries.m_data[i].setExtents(islandMin, islandMax);
			displayGeometries.m_data[i] = &m_activeIslandDisplayGeometries.m_data[i];
		}
		m_displayHandler->displayGeometry(displayGeometries, static_cast<int32_t>(hkColor::BLUE), 0, m_tag);

		// EXE-INLINED none: hkArray<hkAabb, hkContainerTempAllocator>::clearAndDeallocate
		temp = hkMemoryRouter::getInstance().m_temp;
		islandAabbs.m_size = 0;
		if (!(islandAabbs.m_capacityAndFlags & hkArrayBase<hkAabb>::DONT_DEALLOCATE_FLAG)) {
			temp->bufFree(islandAabbs.m_data,
				static_cast<int32_t>((islandAabbs.m_capacityAndFlags & hkArrayBase<hkAabb>::CAPACITY_MASK) *
					sizeof(hkAabb)));
		}
		islandAabbs.m_data = nullptr;
		islandAabbs.m_capacityAndFlags = hkArrayBase<hkAabb>::DONT_DEALLOCATE_FLAG;
		// EXE-INLINED none: hkArray<hkDisplayGeometry*, hkContainerTempAllocator>::~hkArray
		temp = hkMemoryRouter::getInstance().m_temp;
		displayGeometries.m_size = 0;
		if (!(displayGeometries.m_capacityAndFlags & hkArrayBase<hkDisplayGeometry*>::DONT_DEALLOCATE_FLAG)) {
			temp->bufFree(displayGeometries.m_data,
				static_cast<int32_t>((displayGeometries.m_capacityAndFlags &
										 hkArrayBase<hkDisplayGeometry*>::CAPACITY_MASK) *
					sizeof(hkDisplayGeometry*)));
		}
	}
	if (m_showInactiveIslands) {
		int32_t numIslands = world->m_inactiveSimulationIslands.m_size;
		if (numIslands > m_inactiveIslandDisplayGeometries.m_size) {
			// EXE-INLINED none: hkArray<hkDisplayAABB>::setSize
			hkArray<hkDisplayAABB>& geometries = m_inactiveIslandDisplayGeometries;
			hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
			int32_t capacity =
				static_cast<int32_t>(geometries.m_capacityAndFlags & hkArrayBase<hkDisplayAABB>::CAPACITY_MASK);
			if (capacity < numIslands) {
				int32_t newCapacity = capacity * 2;
				if (numIslands >= newCapacity) {
					newCapacity = numIslands;
				}
				hkArrayUtil::_reserve(heap, &geometries, newCapacity, sizeof(hkDisplayAABB));
			}
			hkDisplayAABB* dead = geometries.m_data + numIslands;
			for (int32_t k = geometries.m_size - numIslands; k > 0; k--) {
				dead->~hkDisplayAABB();
				dead++;
			}
			hkDisplayAABB* fresh = geometries.m_data + geometries.m_size;
			for (int32_t k = numIslands - geometries.m_size; k > 0; k--) {
				if (fresh != nullptr) {
					::new (fresh) hkDisplayAABB;
				}
				fresh++;
			}
			geometries.m_size = numIslands;
		}

		// EXE-INLINED none: hkArray<hkDisplayGeometry*>::hkArray
		FNVD_INLINED_LOCAL(hkArray<hkDisplayGeometry*>, displayGeometries, std::nullopt, std::nullopt);
		displayGeometries.m_data = nullptr;
		displayGeometries.m_size = 0;
		displayGeometries.m_capacityAndFlags = hkArrayBase<hkDisplayGeometry*>::DONT_DEALLOCATE_FLAG;
		// EXE-INLINED none: hkArray<hkDisplayGeometry*>::setSize
		numIslands = world->m_inactiveSimulationIslands.m_size;
		hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
		int32_t capacity =
			static_cast<int32_t>(displayGeometries.m_capacityAndFlags & hkArrayBase<hkDisplayGeometry*>::CAPACITY_MASK);
		if (capacity < numIslands) {
			int32_t newCapacity = capacity * 2;
			if (numIslands >= newCapacity) {
				newCapacity = numIslands;
			}
			hkArrayUtil::_reserve(heap, &displayGeometries, newCapacity, sizeof(hkDisplayGeometry*));
		}
		displayGeometries.m_size = numIslands;

		for (int32_t i = 0; i < world->m_inactiveSimulationIslands.m_size; i++) {
			hkpSimulationIsland* island = world->m_inactiveSimulationIslands.m_data[i];
			hkVector4 islandMax;
			islandMax.m_quad = _mm_set1_ps(rdata_104E148);
			hkVector4 islandMin;
			islandMin.m_quad = _mm_set1_ps(rdata_104DA48);
			hkInplaceArray<hkpEntity*, 1>& entities = island->m_entities;
			for (int32_t j = 0; j < entities.m_size; j++) {
				hkpCollidable* collidable = &entities.m_data[j]->m_collidable;
				if (collidable->m_shape != nullptr) {
					hkAabb entityAabb;
					world->m_broadPhase->getAabb(collidable->getBroadPhaseHandle(), entityAabb);
					islandMax.setMax4(islandMax, entityAabb.m_max);
					islandMin.setMin4(islandMin, entityAabb.m_min);
				}
			}
			m_inactiveIslandDisplayGeometries.m_data[i].setExtents(islandMin, islandMax);
			displayGeometries.m_data[i] = &m_inactiveIslandDisplayGeometries.m_data[i];
		}
		m_displayHandler->displayGeometry(displayGeometries, static_cast<int32_t>(hkColor::GREEN), 0, m_tag);

		// EXE-INLINED none: hkArray<hkDisplayGeometry*>::~hkArray
		heap = hkMemoryRouter::getInstance().m_heap;
		displayGeometries.m_size = 0;
		if (!(displayGeometries.m_capacityAndFlags & hkArrayBase<hkDisplayGeometry*>::DONT_DEALLOCATE_FLAG)) {
			heap->bufFree(displayGeometries.m_data,
				static_cast<int32_t>((displayGeometries.m_capacityAndFlags &
										 hkArrayBase<hkDisplayGeometry*>::CAPACITY_MASK) *
					sizeof(hkDisplayGeometry*)));
		}
	}
	HK_TIMER_END();
}

void hkpSimulationIslandViewer::worldAddedCallback(hkpWorld* world) {
	if (this == nullptr) {
		world->addWorldPostSimulationListener(0);
	} else {
		world->addWorldPostSimulationListener(this);
	}
}

void hkpSimulationIslandViewer::worldRemovedCallback(hkpWorld* world) {
	if (this == nullptr) {
		world->removeWorldPostSimulationListener(0);
	} else {
		world->removeWorldPostSimulationListener(this);
	}
}

hkProcess* hkpSimulationIslandViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkpSimulationIslandViewer* v2 =
		static_cast<hkpSimulationIslandViewer*>(Instance->m_heap->blockAlloc(sizeof(hkpSimulationIslandViewer)));
	v2->m_memSizeAndFlags = sizeof(hkpSimulationIslandViewer);
	hkpSimulationIslandViewer* v3 = ::new (v2) hkpSimulationIslandViewer(contexts);
	if (v3) {
		return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*v3));
	} else {
		return 0;
	}
}

void hkpSimulationIslandViewer::registerViewer() {
	hkpSimulationIslandViewer::m_tag =
		hkProcessFactory::s_instance->registerProcess("Simulation Islands", &hkpSimulationIslandViewer::create);
}
