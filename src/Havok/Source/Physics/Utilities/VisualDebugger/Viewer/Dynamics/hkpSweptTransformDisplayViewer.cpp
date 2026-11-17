#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpSweptTransformDisplayViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometryTypes.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/hkpShapeDisplayBuilder.h"

#include <xmmintrin.h>

int32_t HK_DEFAULT_SWEPT_TRANSFORM_COLOR_T0 = hkColor::rgbFromChars(200, 200, 255, 80);
int32_t HK_DEFAULT_SWEPT_TRANSFORM_COLOR_T1 = hkColor::rgbFromChars(200, 255, 200, 150);

hkpSweptTransformDisplayViewer::hkpSweptTransformDisplayViewer(const hkArray<hkProcessContext*,
	hkContainerHeapAllocator>& contexts)
	: hkpWorldViewerBase(contexts) {}

// Removing each world removes its display geometries and frees its record; the array frees itself.
hkpSweptTransformDisplayViewer::~hkpSweptTransformDisplayViewer() {
	for (int32_t i = m_worldEntities.m_size - 1; i >= 0; --i) {
		removeWorld(i);
	}
}

void hkpSweptTransformDisplayViewer::init() {
	hkpPhysicsContext* m_context = this->m_context;
	if (m_context) {
		for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
			addWorld(m_context->m_worlds.m_data[i]);
			m_context = this->m_context;
		}
	}
}

// Builds the display geometries of the entity's shape, drops the convex ones without geometry with a report and, unless
// the entity is fixed, adds them twice: at the swept transform's start and end, under the collidable's address plus 1
// and plus 2 (the address is treated as an untyped number), each id kept in the world's list.
void hkpSweptTransformDisplayViewer::entityAddedCallback(hkpEntity* entity) {
	if (!entity->m_collidable.m_shape) {
		return;
	}
	int32_t index = findWorld(entity->m_world);
	if (index < 0) {
		return;
	}
	WorldToEntityData* data = m_worldEntities.m_data[index];
	// EXE-INLINED none: hkArray<hkDisplayGeometry*> constructor
	FNVD_INLINED_LOCAL(hkArray<hkDisplayGeometry*>, displayGeometries, std::nullopt, std::nullopt);
	displayGeometries.m_data = nullptr;
	displayGeometries.m_size = 0;
	displayGeometries.m_capacityAndFlags = hkArrayBase<hkDisplayGeometry*>::DONT_DEALLOCATE_FLAG;
	// No destructor is run for either env or builder, as the game does.
	hkpShapeDisplayBuilder::hkpShapeDisplayBuilderEnvironment env;
	hkpShapeDisplayBuilder builder(env);
	builder.buildDisplayGeometries(entity->m_collidable.m_shape, displayGeometries);

	for (int32_t i = displayGeometries.m_size - 1; i >= 0; i--) {
		hkDisplayGeometry* geometry = displayGeometries.m_data[i];
		if (geometry->m_type == HK_DISPLAY_CONVEX && !geometry->m_geometry) {
			// EXE-INLINED none: HK_REPORT
			alignas(16) char buf[512];
			FNVD_INLINED_LOCAL(hkErrStream, errStream, std::nullopt, std::nullopt);
			::new (&errStream) hkErrStream(buf, 512);
			errStream << "Unable to build display geometry from hkpShape geometry data";
			hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf,
				".\\VisualDebugger\\Viewer\\Dynamics\\hkpSweptTransformDisplayViewer.cpp", 168);
			FNVD_DESTRUCT(hkOstream)(&errStream);
			// EXE-INLINED none: hkArrayBase<hkDisplayGeometry*>::removeAt
			displayGeometries.m_size--;
			if (displayGeometries.m_size != i) {
				displayGeometries.m_data[i] = displayGeometries.m_data[displayGeometries.m_size];
			}
		}
	}

	if (displayGeometries.m_size > 0 && entity->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED) {
		const hkSweptTransform& swept = entity->m_motion.m_motionState.m_sweptTransform;
		const hkUlong id = reinterpret_cast<hkUlong>(&entity->m_collidable);
		hkArray<hkUlong>& created = data->entitiesCreated;
		{
			// EXE-INLINED none: hkArray<hkUlong>::pushBack
			hkMemoryRouter& router = hkMemoryRouter::getInstance();
			if (created.m_size == (created.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*router.m_heap, &created, sizeof(hkUlong));
			}
			created.m_data[created.m_size] = id + 1;
			created.m_size++;
		}

		hkTransform trans0;
		// EXE-INLINED none: hkSweptTransformUtil::calcTransAtT0
		trans0.m_rotation.set(swept.m_rotation0);
		// EXE-INLINED 0xC7F700
		const hkQuadReal local0 = swept.m_centerOfMassLocal.m_quad;
		const hkQuadReal centerShift0 =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(local0, local0, _MM_SHUFFLE(1, 1, 1, 1)),
									  trans0.m_rotation.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(local0, local0, _MM_SHUFFLE(0, 0, 0, 0)),
							   trans0.m_rotation.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(local0, local0, _MM_SHUFFLE(2, 2, 2, 2)), trans0.m_rotation.m_col2.m_quad));
		trans0.m_translation.m_quad = _mm_sub_ps(swept.m_centerOfMass0.m_quad, centerShift0);

		hkTransform trans1;
		// EXE-INLINED none: hkSweptTransformUtil::calcTransAtT1
		trans1.m_rotation.set(swept.m_rotation1);
		// EXE-INLINED 0xC7F700
		const hkQuadReal local1 = swept.m_centerOfMassLocal.m_quad;
		const hkQuadReal centerShift1 =
			_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(local1, local1, _MM_SHUFFLE(1, 1, 1, 1)),
									  trans1.m_rotation.m_col1.m_quad),
						   _mm_mul_ps(_mm_shuffle_ps(local1, local1, _MM_SHUFFLE(0, 0, 0, 0)),
							   trans1.m_rotation.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(local1, local1, _MM_SHUFFLE(2, 2, 2, 2)), trans1.m_rotation.m_col2.m_quad));
		trans1.m_translation.m_quad = _mm_sub_ps(swept.m_centerOfMass1.m_quad, centerShift1);

		m_displayHandler->addGeometry(displayGeometries, trans0, id + 1, m_tag, 0);
		m_displayHandler->setGeometryColor(HK_DEFAULT_SWEPT_TRANSFORM_COLOR_T0, id + 1, m_tag);
		{
			// EXE-INLINED none: hkArray<hkUlong>::pushBack
			hkMemoryRouter& router = hkMemoryRouter::getInstance();
			if (created.m_size == (created.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*router.m_heap, &created, sizeof(hkUlong));
			}
			created.m_data[created.m_size] = id + 2;
			created.m_size++;
		}
		m_displayHandler->addGeometry(displayGeometries, trans1, id + 2, m_tag, 0);
		m_displayHandler->setGeometryColor(HK_DEFAULT_SWEPT_TRANSFORM_COLOR_T1, id + 2, m_tag);
	}

	for (int32_t i = 0; i < displayGeometries.m_size; i++) {
		hkDisplayGeometry* geometry = displayGeometries.m_data[i];
		if (geometry) {
			delete geometry;
		}
	}
	// EXE-INLINED none: hkArray<hkDisplayGeometry*> destructor
	hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
	int32_t capacityAndFlags = displayGeometries.m_capacityAndFlags;
	displayGeometries.m_size = 0;
	if (capacityAndFlags >= 0) {
		heap->bufFree(displayGeometries.m_data,
			static_cast<int32_t>((capacityAndFlags & hkArrayBase<hkDisplayGeometry*>::CAPACITY_MASK) *
				sizeof(hkDisplayGeometry*)));
	}
}

// Removes the entity's two display objects and their ids from the world's list. The ids are the collidable's address
// plus 1 and plus 2, with the address treated as an untyped number.
void hkpSweptTransformDisplayViewer::entityRemovedCallback(hkpEntity* entity) {
	if (!entity->m_collidable.m_shape) {
		return;
	}
	int32_t index = findWorld(entity->m_world);
	if (index < 0) {
		return;
	}
	WorldToEntityData* data = m_worldEntities.m_data[index];
	hkArray<hkUlong>& created = data->entitiesCreated;
	const hkUlong id = reinterpret_cast<hkUlong>(&entity->m_collidable);
	for (uint32_t step = 1; step <= 2; step++) {
		m_displayHandler->removeGeometry(id + step, m_tag, 0);
		// EXE-INLINED none: hkArrayBase<hkUlong>::indexOf
		int32_t found = -1;
		for (int32_t i = 0; i < created.m_size; i++) {
			if (created.m_data[i] == id + step) {
				found = i;
				break;
			}
		}
		if (found >= 0) {
			// EXE-INLINED none: hkArrayBase<hkUlong>::removeAt
			created.m_size--;
			if (created.m_size != found) {
				created.m_data[found] = created.m_data[created.m_size];
			}
		}
	}
}

// Moves the two display objects of every entity of the active islands to the entity's swept transform at the step's
// start and end, under the viewer's timer. Their ids are the collidable's address plus 1 and plus 2, with the address
// treated as an untyped number.
void hkpSweptTransformDisplayViewer::postSimulationCallback(hkpWorld* world) {
	HK_TIMER_BEGIN("hkpSweptTransformDisplayViewer", this);
	for (int32_t i = 0; i < world->m_activeSimulationIslands.m_size; i++) {
		const hkArrayBase<hkpEntity*>& entities = world->m_activeSimulationIslands.m_data[i]->m_entities;
		for (int32_t j = 0; j < entities.m_size; j++) {
			hkpEntity* entity = entities.m_data[j];
			const hkSweptTransform& swept = entity->m_motion.m_motionState.m_sweptTransform;
			const hkUlong id = reinterpret_cast<hkUlong>(&entity->m_collidable);
			hkTransform trans;

			// EXE-INLINED none: hkSweptTransformUtil::calcTransAtT0
			trans.m_rotation.set(swept.m_rotation0);
			// EXE-INLINED 0xC7F700
			const hkQuadReal local0 = swept.m_centerOfMassLocal.m_quad;
			const hkQuadReal centerShift0 =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(local0, local0, _MM_SHUFFLE(1, 1, 1, 1)),
										  trans.m_rotation.m_col1.m_quad),
							   _mm_mul_ps(_mm_shuffle_ps(local0, local0, _MM_SHUFFLE(0, 0, 0, 0)),
								   trans.m_rotation.m_col0.m_quad)),
					_mm_mul_ps(_mm_shuffle_ps(local0, local0, _MM_SHUFFLE(2, 2, 2, 2)),
						trans.m_rotation.m_col2.m_quad));
			trans.m_translation.m_quad = _mm_sub_ps(swept.m_centerOfMass0.m_quad, centerShift0);
			m_displayHandler->updateGeometry(trans, id + 1, m_tag);

			// EXE-INLINED none: hkSweptTransformUtil::calcTransAtT1
			trans.m_rotation.set(swept.m_rotation1);
			// EXE-INLINED 0xC7F700
			const hkQuadReal local1 = swept.m_centerOfMassLocal.m_quad;
			const hkQuadReal centerShift1 =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(local1, local1, _MM_SHUFFLE(1, 1, 1, 1)),
										  trans.m_rotation.m_col1.m_quad),
							   _mm_mul_ps(_mm_shuffle_ps(local1, local1, _MM_SHUFFLE(0, 0, 0, 0)),
								   trans.m_rotation.m_col0.m_quad)),
					_mm_mul_ps(_mm_shuffle_ps(local1, local1, _MM_SHUFFLE(2, 2, 2, 2)),
						trans.m_rotation.m_col2.m_quad));
			trans.m_translation.m_quad = _mm_sub_ps(swept.m_centerOfMass1.m_quad, centerShift1);
			m_displayHandler->updateGeometry(trans, id + 2, m_tag);
		}
	}
	HK_TIMER_END();
}

void hkpSweptTransformDisplayViewer::worldAddedCallback(hkpWorld* world) {
	addWorld(world);
}

void hkpSweptTransformDisplayViewer::worldRemovedCallback(hkpWorld* world) {
	hkpSweptTransformDisplayViewer* v2 = this;
	int32_t v3 = findWorld(world);
	if (v3 >= 0) {
		v2->removeWorld(v3);
	}
}

// Moves the two display objects of an entity that moved while inactive to its swept transform's start and end. Their
// ids are the collidable's address plus 1 and plus 2, with the address treated as an untyped number.
void hkpSweptTransformDisplayViewer::inactiveEntityMovedCallback(hkpEntity* entity) {
	const hkSweptTransform& swept = entity->m_motion.m_motionState.m_sweptTransform;
	const hkUlong id = reinterpret_cast<hkUlong>(&entity->m_collidable);
	hkTransform trans;

	// EXE-INLINED none: hkSweptTransformUtil::calcTransAtT0
	trans.m_rotation.set(swept.m_rotation0);
	// EXE-INLINED 0xC7F700
	const hkQuadReal local0 = swept.m_centerOfMassLocal.m_quad;
	const hkQuadReal centerShift0 =
		_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(local0, local0, _MM_SHUFFLE(1, 1, 1, 1)),
								  trans.m_rotation.m_col1.m_quad),
					   _mm_mul_ps(_mm_shuffle_ps(local0, local0, _MM_SHUFFLE(0, 0, 0, 0)),
						   trans.m_rotation.m_col0.m_quad)),
			_mm_mul_ps(_mm_shuffle_ps(local0, local0, _MM_SHUFFLE(2, 2, 2, 2)), trans.m_rotation.m_col2.m_quad));
	trans.m_translation.m_quad = _mm_sub_ps(swept.m_centerOfMass0.m_quad, centerShift0);
	m_displayHandler->updateGeometry(trans, id + 1, m_tag);

	// EXE-INLINED none: hkSweptTransformUtil::calcTransAtT1
	trans.m_rotation.set(swept.m_rotation1);
	// EXE-INLINED 0xC7F700
	const hkQuadReal local1 = swept.m_centerOfMassLocal.m_quad;
	const hkQuadReal centerShift1 =
		_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(local1, local1, _MM_SHUFFLE(1, 1, 1, 1)),
								  trans.m_rotation.m_col1.m_quad),
					   _mm_mul_ps(_mm_shuffle_ps(local1, local1, _MM_SHUFFLE(0, 0, 0, 0)),
						   trans.m_rotation.m_col0.m_quad)),
			_mm_mul_ps(_mm_shuffle_ps(local1, local1, _MM_SHUFFLE(2, 2, 2, 2)), trans.m_rotation.m_col2.m_quad));
	trans.m_translation.m_quad = _mm_sub_ps(swept.m_centerOfMass1.m_quad, centerShift1);
	m_displayHandler->updateGeometry(trans, id + 2, m_tag);
}

// The full object as this: init calls it per world.
void hkpSweptTransformDisplayViewer::addWorld(hkpWorld* world) {
	hkpEntityListener* v4;
	hkpWorldPostSimulationListener* v6;
	hkpSweptTransformDisplayViewer* v2 = this;
	hkpSweptTransformDisplayViewer::WorldToEntityData* v3 = 0;
	if (this) {
		v4 = static_cast<hkpEntityListener*>(this);
	} else {
		v4 = 0;
	}
	world->addEntityListener(v4);
	if (v2) {
		v6 = static_cast<hkpWorldPostSimulationListener*>(v2);
	} else {
		v6 = 0;
	}
	world->addWorldPostSimulationListener(v6);
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkpSweptTransformDisplayViewer::WorldToEntityData* v8 =
		static_cast<hkpSweptTransformDisplayViewer::WorldToEntityData*>(Instance->m_heap
				->blockAlloc(sizeof(hkpSweptTransformDisplayViewer::WorldToEntityData)));
	if (v8) {
		v8->entitiesCreated.m_data = 0;
		v8->entitiesCreated.m_size = 0;
		v8->entitiesCreated.m_capacityAndFlags = hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG;
		v3 = v8;
	}
	v3->world = world;
	hkMemoryRouter* v9 = &hkMemoryRouter::getInstance();
	if (v2->m_worldEntities.m_size ==
		(v2->m_worldEntities.m_capacityAndFlags &
			hkArrayBase<hkpSweptTransformDisplayViewer::WorldToEntityData*>::CAPACITY_MASK)) {
		hkArrayUtil::_reserveMore(*v9->m_heap, &v2->m_worldEntities,
			sizeof(hkpSweptTransformDisplayViewer::WorldToEntityData*));
	}
	v2->m_worldEntities.m_data[v2->m_worldEntities.m_size++] = v3;
	int32_t v10 = 0;
	for (int32_t i = 0; v10 < world->m_activeSimulationIslands.m_size; i = v10) {
		hkpSimulationIsland** m_data = world->m_activeSimulationIslands.begin();
		hkInplaceArray<hkpEntity*, 1, hkContainerHeapAllocator>* p_m_entities = &m_data[v10]->m_entities;
		int32_t v13 = 0;
		if (m_data[v10]->m_entities.m_size > 0) {
			hkpEntityListener* v14 = static_cast<hkpEntityListener*>(v2);
			do {
				v14->entityAddedCallback(p_m_entities->m_data[v13++]);
			} while (v13 < p_m_entities->m_size);
			v2 = this;
			v10 = i;
		}
		++v10;
	}
	int32_t v15 = 0;
	for (int32_t j = 0; v15 < world->m_inactiveSimulationIslands.m_size; j = v15) {
		hkpSimulationIsland** v16 = world->m_inactiveSimulationIslands.begin();
		hkInplaceArray<hkpEntity*, 1, hkContainerHeapAllocator>* v17 = &v16[v15]->m_entities;
		int32_t v18 = 0;
		if (v16[v15]->m_entities.m_size > 0) {
			hkpEntityListener* v19 = static_cast<hkpEntityListener*>(v2);
			do {
				v19->entityAddedCallback(v17->m_data[v18++]);
			} while (v18 < v17->m_size);
			v15 = j;
			v2 = this;
		}
		++v15;
	}
	hkInplaceArray<hkpEntity*, 1, hkContainerHeapAllocator>* v20 = &world->getFixedIsland()->m_entities;
	for (int32_t k = 0; k < v20->m_size; ++k) {
		v2->entityAddedCallback(v20->m_data[k]);
	}
}

int32_t hkpSweptTransformDisplayViewer::findWorld(hkpWorld* world) {
	int32_t m_size = m_worldEntities.m_size;
	int32_t result = 0;
	if (m_size <= 0) {
		return -1;
	}
	for (hkpSweptTransformDisplayViewer::WorldToEntityData** i =
			 static_cast<hkArrayBase<hkpSweptTransformDisplayViewer::WorldToEntityData*>&>(m_worldEntities).begin();
		(*i)->world != world; ++i) {
		if (++result >= m_size) {
			return -1;
		}
	}
	return result;
}

void hkpSweptTransformDisplayViewer::removeWorld(int32_t i) {
	hkpEntityListener* v3;
	if (this) {
		v3 = static_cast<hkpEntityListener*>(this);
	} else {
		v3 = 0;
	}
	m_worldEntities.m_data[i]->world->removeEntityListener(v3);
	m_worldEntities.m_data[i]->world->removeWorldPostSimulationListener(this);
	removeAllGeometries(i);
	hkpSweptTransformDisplayViewer::WorldToEntityData* v4 = m_worldEntities.m_data[i];
	if (v4) {
		hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
		int32_t m_capacityAndFlags = v4->entitiesCreated.m_capacityAndFlags;
		v4->entitiesCreated.m_size = 0;
		if (m_capacityAndFlags >= 0) {
			m_heap->bufFree(v4->entitiesCreated.begin(), static_cast<int32_t>(sizeof(hkUlong)) * m_capacityAndFlags);
		}
		v4->entitiesCreated.m_data = 0;
		v4->entitiesCreated.m_capacityAndFlags = hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG;
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		Instance->m_heap->blockFree(v4, sizeof(*v4));
	}
	int32_t v8 = --m_worldEntities.m_size;
	if (v8 != i) {
		m_worldEntities.m_data[i] = m_worldEntities.m_data[v8];
	}
}

void hkpSweptTransformDisplayViewer::removeAllGeometries(int32_t worldIndex) {
	hkpSweptTransformDisplayViewer::WorldToEntityData* v3 = m_worldEntities.m_data[worldIndex];
	for (int32_t i = 0; i < v3->entitiesCreated.m_size; ++i) {
		m_displayHandler->removeGeometry(v3->entitiesCreated.m_data[i], m_tag, 0);
	}
	hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t v6 = v3->entitiesCreated.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK;
	v3->entitiesCreated.m_size = 0;
}

hkProcess*
hkpSweptTransformDisplayViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkpSweptTransformDisplayViewer* v2 = static_cast<hkpSweptTransformDisplayViewer*>(Instance->m_heap
			->blockAlloc(sizeof(hkpSweptTransformDisplayViewer)));
	v2->m_memSizeAndFlags = sizeof(hkpSweptTransformDisplayViewer);
	hkpSweptTransformDisplayViewer* v3 = ::new (v2) hkpSweptTransformDisplayViewer(contexts);
	if (v3) {
		return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*v3));
	} else {
		return 0;
	}
}

void hkpSweptTransformDisplayViewer::registerViewer() {
	m_tag = hkProcessFactory::s_instance->registerProcess("Swept Transforms", &hkpSweptTransformDisplayViewer::create);
}
