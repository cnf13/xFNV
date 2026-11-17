#include "Havok/Source/Physics/Dynamics/Phantom/hkpCachingShapePhantom.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomOverlapListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpShapePhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "SDK/MSVC/string.h"

#include <xmmintrin.h>

hkpCachingShapePhantom::hkpCachingShapePhantom(const hkpShape* a1, const hkTransform& a2, uint32_t a3)
	: hkpShapePhantom(const_cast<hkpShape*>(a1), const_cast<hkTransform*>(&a2)) {
	m_orderDirty.m_bool = 0;
	m_collidable.m_broadPhaseHandle.m_collisionFilterInfo = a3;
}

// The SDK's finish constructor: hkpShapePhantom's, and the collision details left empty.
hkpCachingShapePhantom::hkpCachingShapePhantom(hkFinishLoadedObjectFlag aFlag) : hkpShapePhantom(aFlag) {}

// The SDK's destructor: each cached agent is cleaned up against a stack constraint owner and the array cleared; the
// compiler frees its storage, then runs the base's destructor.
hkpCachingShapePhantom::~hkpCachingShapePhantom() {
	hkpConstraintOwner constraintOwner;
	for (int32_t i = m_collisionDetails.m_size - 1; i >= 0; --i) {
		m_collisionDetails.m_data[i].m_agent->cleanup(constraintOwner);
	}
	m_collisionDetails.m_size = 0;
}

void hkpCachingShapePhantom::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
	hkpPhantom::calcContentStatistics(collector, &hkpPhantomClass);
	int32_t m_capacityAndFlags = m_collisionDetails.m_capacityAndFlags;
	if (m_capacityAndFlags >= 0 || collector->m_includeUnmanagedArrayMemory) {
		int32_t AllocatedSize = hkStatisticsCollector::getAllocatedSize(m_collisionDetails.begin(),
			static_cast<int32_t>(sizeof(*m_collisionDetails.begin())) * m_capacityAndFlags);
		collector->addChunk(hkStatisticsCollector::MEM_ARRAY, "AgentPtr", m_collisionDetails.begin(),
			static_cast<int32_t>(sizeof(*m_collisionDetails.begin())) * m_collisionDetails.m_size, AllocatedSize);
	}
	collector->pushDir("agents");
	for (int32_t i = 0; i < m_collisionDetails.m_size; ++i) {
		collector->addReferencedObject(0, m_collisionDetails.m_data[i].m_agent, 0);
	}
	collector->popDir();
}

hkpPhantomType hkpCachingShapePhantom::getType() const {
	return HK_PHANTOM_CACHING_SHAPE;
}

// A collidable with a shape that the listeners accept gets a new detail holding the agent the dispatcher creates for
// the pair, with a copy of the world's collision input that creates no predictive agents and uses the PSI quality; the
// copy's spare agent sector is freed.
void hkpCachingShapePhantom::addOverlappingCollidable(hkpCollidable* collidable) {
	if (!collidable->m_shape) {
		return;
	}
	if (fireCollidableAdded(collidable) != HK_COLLIDABLE_ACCEPT) {
		return;
	}
	hkMemoryRouter* router = &hkMemoryRouter::getInstance();
	if (m_collisionDetails.m_size ==
		(m_collisionDetails.m_capacityAndFlags & hkArrayBase<hkpCollisionDetail>::CAPACITY_MASK)) {
		hkArrayUtil::_reserveMore(*router->m_heap, &m_collisionDetails, sizeof(hkpCollisionDetail));
	}
	int32_t size = m_collisionDetails.m_size;
	hkpCollisionDetail* detail = &m_collisionDetails.m_data[size];
	m_collisionDetails.m_size = size + 1;
	const hkpProcessCollisionInput* worldInput = m_world->m_collisionInput;
	FNVD_INLINED_LOCAL(hkpProcessCollisionInput, input, std::nullopt, std::nullopt);
	static_cast<hkpCollisionInput&>(input) = *worldInput;
	input.m_spareAgentSector = nullptr;
	::new (&input) hkpProcessCollisionInput(*worldInput);
	input.m_createPredictiveAgents.m_storage = 0;
	input.m_collisionQualityInfo =
		&input.m_dispatcher->m_collisionQualityInfo[hkpCollisionDispatcher::COLLISION_QUALITY_PSI];
	hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
	detail->m_agent =
		dispatcher
			->m_agent2Func[dispatcher
					->m_agent2Types[m_collidable.m_shape->m_type.m_storage][collidable->m_shape->m_type.m_storage]]
			.m_createFunc(m_collidable, *collidable, input, nullptr);
	detail->m_collidable = collidable;
	m_orderDirty.m_bool = 1;
	if (input.m_spareAgentSector) {
		hkMemoryRouter::getInstance().m_heap->blockFree(input.m_spareAgentSector, sizeof(hkpAgent1nSector));
	}
}

hkBool* hkpCachingShapePhantom::isOverlappingCollidableAdded(hkBool* arResult, const hkpCollidable* apCollidable) {
	int32_t m_size = m_collisionDetails.m_size;
	int32_t v4 = 0;
	if (m_size <= 0) {
	LABEL_5:;
		arResult->m_bool = 0;
		return arResult;
	} else {
		hkpCollidable** p_m_collidable =
			&static_cast<hkArrayBase<hkpCachingShapePhantom::hkpCollisionDetail>&>(m_collisionDetails)
				 .begin()
				 ->m_collidable;
		while (*p_m_collidable != apCollidable) {
			++v4;
			p_m_collidable += 2;
			if (v4 >= m_size) {
				goto LABEL_5;
			}
		}
		arResult->m_bool = 1;
		return arResult;
	}
}

// The detail of collidable is looked for from the last; the listeners are told whether it was found; a found detail's
// agent is cleaned up against a stack constraint owner and the detail removed. A collidable without a shape was never
// added: nothing is done.
void hkpCachingShapePhantom::removeOverlappingCollidable(hkpCollidable* collidable) {
	FNVD_INLINED_LOCAL(hkpConstraintOwner, constraintOwner, hkpConstraintOwner_vftable, std::nullopt);
	constraintOwner.m_referenceCount = 1;
	if (!collidable->m_shape) {
		return;
	}
	int32_t i = m_collisionDetails.m_size - 1;
	for (; i >= 0; --i) {
		if (m_collisionDetails.m_data[i].m_collidable == collidable) {
			break;
		}
	}
	if (i < 0) {
		fireCollidableRemoved(collidable, hkBool(false));
		return;
	}
	fireCollidableRemoved(collidable, hkBool(true));
	hkpCollisionAgent* agent = m_collisionDetails.m_data[i].m_agent;
	if (agent) {
		agent->cleanup(constraintOwner);
	}
	--m_collisionDetails.m_size;
	if (m_collisionDetails.m_size != i) {
		// EXE-INLINED 0x6267C0
		m_collisionDetails.m_data[i] = m_collisionDetails.m_data[m_collisionDetails.m_size];
	}
	m_orderDirty.m_bool = 1;
}

// Sorts the cached collision details once a query has changed them, then clears the dirty flag.
void hkpCachingShapePhantom::ensureDeterministicOrder() {
	if (m_orderDirty.m_bool) {
		int32_t size = (int32_t)m_collisionDetails.m_size;
		if (size > 1) {
			quickSortRecursive(m_collisionDetails.m_data, 0, size - 1, 0);
		}
		m_orderDirty.m_bool = 0;
	}
}

// Creates a new caching shape phantom with this one's shape, transform and filter info, with the listener arrays and
// the world object's properties copied.
hkpPhantom* hkpCachingShapePhantom::clone() const {
	hkpCachingShapePhantom* pMemory = static_cast<hkpCachingShapePhantom*>(hkMemoryRouter::getInstance()
			.m_heap->blockAlloc(sizeof(hkpCachingShapePhantom)));
	pMemory->m_memSizeAndFlags = sizeof(hkpCachingShapePhantom);
	hkpCachingShapePhantom* pClone = ::new (pMemory) hkpCachingShapePhantom(m_collidable.m_shape,
		m_motionState.m_transform, m_collidable.m_broadPhaseHandle.m_collisionFilterInfo);
	pClone->m_overlapListeners.copyFromArray(hkContainerHeapAllocator().get(&pClone->m_overlapListeners),
		m_overlapListeners, hkTypeIsPod());
	pClone->m_phantomListeners.copyFromArray(hkContainerHeapAllocator().get(&pClone->m_phantomListeners),
		m_phantomListeners, hkTypeIsPod());
	pClone->copyProperties(this);
	return pClone;
}

void hkpCachingShapePhantom::updateShapeCollectionFilter() {
	FNVD_INLINED_LOCAL(hkpConstraintOwner, v4, hkpConstraintOwner_vftable, std::nullopt);
	hkpCollisionInput v5;
	rt::memcpy(&v5, m_world->m_collisionInput, sizeof(v5));
	int32_t v2 = 0;
	bool v3 = m_collisionDetails.m_size <= 0;
	// MAYBE-INLINED 0xD4CA5C: before the vptr store of hkpConstraintOwner's inlined constructor
	v4.m_referenceCount = 1;
	if (!v3) {
		do {
			m_collisionDetails.m_data[v2].m_agent->updateShapeCollectionFilter(m_collidable,
				*m_collisionDetails.m_data[v2].m_collidable, v5, v4);
			++v2;
		} while (v2 < m_collisionDetails.m_size);
	}
}

void hkpCachingShapePhantom::deallocateInternalArrays() {
	hkMemoryAllocator* m_heap;
	int32_t capacityAndFlags;
	if (!m_collisionDetails.m_size) {
		m_heap = (&hkMemoryRouter::getInstance())->m_heap;
		capacityAndFlags = (int32_t)m_collisionDetails.m_capacityAndFlags;
		m_collisionDetails.m_size = (uint32_t)0;
		if (capacityAndFlags >= 0) {
			m_heap->bufFree(m_collisionDetails.m_data,
				sizeof(hkpCachingShapePhantom::hkpCollisionDetail) * capacityAndFlags);
		}
		m_collisionDetails.m_data = nullptr;
		m_collisionDetails.m_capacityAndFlags = 0x80000000u;
	}
	hkpShapePhantom::deallocateInternalArrays();
}

// The new position goes into the motion state; the shape's AABB, grown by half the world's tolerance plus the cast's
// start tolerance and swept along the path, goes to the broad phase; then each cached agent casts along the path.
void hkpCachingShapePhantom::setPositionAndLinearCast(const hkVector4& position, const hkpLinearCastInput& input,
	hkpCdPointCollector& castCollector, hkpCdPointCollector* startCollector) {
	hkAabb aabb;
	m_motionState.m_transform.m_translation.m_quad = position.m_quad;
	float tolerance = m_world->m_collisionInput->m_tolerance * 0.5;
	tolerance = tolerance + input.m_startPointTolerance;
	m_collidable.m_shape->getAabb(m_motionState.m_transform, tolerance, aabb);
	__m128 path = _mm_sub_ps(input.m_to.m_quad, position.m_quad);
	aabb.m_min.m_quad = _mm_add_ps(_mm_min_ps(_mm_setzero_ps(), path), aabb.m_min.m_quad);
	aabb.m_max.m_quad = _mm_add_ps(_mm_max_ps(_mm_setzero_ps(), path), aabb.m_max.m_quad);
	updateBroadPhase(aabb);
	hkpLinearCastCollisionInput castInput;
	castInput.m_maxExtraPenetration = 1.1920929e-07f;
	castInput.m_weldClosestPoints.m_storage = 0;
	const hkpProcessCollisionInput* collisionInput = m_world->m_collisionInput;
	// Copies hkpCollisionInput's members one by one, as the game does (not the padding after m_createPredictiveAgents).
	castInput.m_dispatcher = collisionInput->m_dispatcher;
	castInput.m_weldClosestPoints.m_storage = collisionInput->m_weldClosestPoints.m_storage;
	castInput.m_tolerance = collisionInput->m_tolerance;
	castInput.m_filter = collisionInput->m_filter;
	castInput.m_convexListFilter = collisionInput->m_convexListFilter;
	castInput.m_createPredictiveAgents.m_storage = collisionInput->m_createPredictiveAgents.m_storage;
	castInput.m_aabb32Info = collisionInput->m_aabb32Info;
	castInput.m_config = collisionInput->m_config;
	castInput.m_path.m_quad = path;
	// The path's length3: (y + x) + z in single precision.
	__m128 squared = _mm_mul_ps(path, path);
	__m128 lengthSquared =
		_mm_add_ss(_mm_shuffle_ps(squared, squared, 0xAA), _mm_add_ss(_mm_shuffle_ps(squared, squared, 0x55), squared));
	castInput.m_cachedPathLength = _mm_cvtss_f32(_mm_sqrt_ss(lengthSquared));
	castInput.m_tolerance = input.m_startPointTolerance;
	castInput.m_maxExtraPenetration = input.m_maxExtraPenetration;
	for (int32_t i = m_collisionDetails.m_size - 1; i >= 0; --i) {
		m_collisionDetails.m_data[i].m_agent->linearCast(m_collidable, *m_collisionDetails.m_data[i].m_collidable,
			castInput, castCollector, startCollector);
	}
}

// The rotation goes straight into the motion state; the translation goes through setPositionAndLinearCast, which is
// called directly.
void hkpCachingShapePhantom::setTransformAndLinearCast(const hkTransform& transform, const hkpLinearCastInput& input,
	hkpCdPointCollector& castCollector, hkpCdPointCollector* startCollector) {
	m_motionState.m_transform.m_rotation = transform.m_rotation;
	hkpCachingShapePhantom::setPositionAndLinearCast(transform.m_translation, input, castCollector, startCollector);
}

void hkpCachingShapePhantom::getClosestPoints(hkpCdPointCollector& a1, const hkpCollisionInput* a2) {
	if (!a2) {
		a2 = m_world->m_collisionInput;
	}
	for (int32_t i = m_collisionDetails.m_size - 1; i >= 0; --i) {
		m_collisionDetails.m_data[i].m_agent->getClosestPoints(m_collidable, *m_collisionDetails.m_data[i].m_collidable,
			*a2, a1);
	}
}

void hkpCachingShapePhantom::getPenetrations(hkpCdBodyPairCollector& a1, const hkpCollisionInput* a2) {
	if (!a2) {
		a2 = m_world->m_collisionInput;
	}
	for (int32_t i = m_collisionDetails.m_size - 1; i >= 0; --i) {
		m_collisionDetails.m_data[i].m_agent->getPenetrations(m_collidable, *m_collisionDetails.m_data[i].m_collidable,
			*a2, a1);
		if (a1.m_earlyOut.m_bool) {
			break;
		}
	}
}

// hkAlgorithm::quickSortRecursive instantiation for hkpCollisionDetail and OrderByUid.
void quickSortRecursive(hkpCachingShapePhantom::hkpCollisionDetail* apDetails, int32_t aFirst, int32_t aLast,
	uint8_t aFlag) {
	int32_t v4 = aLast;
	for (int32_t i = aFirst;; aFirst = i) {
		hkpCachingShapePhantom::hkpCollisionDetail v13 = apDetails[(i + v4) >> 1];
		do {
			uint32_t m_id = v13.m_collidable->m_broadPhaseHandle.m_id;
			hkpCollidable** p_m_collidable = &apDetails[i].m_collidable;
			if ((*p_m_collidable)->m_broadPhaseHandle.m_id < m_id) {
				hkpCollidable* v8;
				do {
					v8 = p_m_collidable[2];
					p_m_collidable += 2;
					++i;
				} while (v8->m_broadPhaseHandle.m_id < m_id);
			}
			hkpCollidable** v9 = &apDetails[v4].m_collidable;
			if (m_id < (*v9)->m_broadPhaseHandle.m_id) {
				hkpCollidable* v10;
				do {
					v10 = *(v9 - 2);
					v9 -= 2;
					--v4;
				} while (m_id < v10->m_broadPhaseHandle.m_id);
			}
			if (v4 < i) {
				break;
			}
			if (v4 != i) {
				hkpCollisionAgent* m_agent = apDetails[v4].m_agent;
				hkpCollidable* m_collidable = apDetails[v4].m_collidable;
				apDetails[v4] = apDetails[i];
				apDetails[i].m_agent = m_agent;
				apDetails[i].m_collidable = m_collidable;
			}
			--v4;
			++i;
		} while (i <= v4);
		if (aFirst < v4) {
			quickSortRecursive(apDetails, aFirst, v4, aFlag);
		}
		v4 = aLast;
		if (i >= aLast) {
			break;
		}
	}
}
