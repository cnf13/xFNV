#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Cpu/hkpCpuShapeRaycastJob.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpFixedBufferRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

#include <xmmintrin.h>

// KEEP: a hit copies the shape-level output into the command's results (0xDF9740-0xDF9794), m_pad and the shape keys
// past those castRayImpl wrote included. Nothing in this function writes m_pad or m_shapeKeys[1..7], so the exe
// stores its own stack garbage into the results there: an uninitialised read.
hkJobQueue::JobStatus hkCpuShapeRayCastJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
	HK_TIMER_BEGIN("CollQueryShapeRayCast", nullptr);

	// The queue entry holds the job: both derive from hkJob, so the entry's job is the shape ray cast job.
	const hkpShapeRayCastJob& shapeRayCastJob = static_cast<const hkpShapeRayCastJob&>(static_cast<hkJob&>(nextJobOut));
	for (int32_t commandIdx = 0; commandIdx < shapeRayCastJob.m_numCommands; commandIdx++) {
		// The SDK writes the result count back into the command array the job holds as const.
		hkpShapeRayCastCommand* command =
			const_cast<hkpShapeRayCastCommand*>(&shapeRayCastJob.m_commandArray[commandIdx]);

		// EXE-INLINED 0x4A3D00
		alignas(16) hkpWorldRayCastOutput output;
		output.m_hitFraction = 1.0f;
		output.m_extraInfo = -1;
		output.m_shapeKeys[0] = static_cast<uint32_t>(-1);
		output.m_shapeKeyIndex = 0;
		output.m_rootCollidable = nullptr;

		hkpWorldRayCastOutput* nextFreeResult = command->m_results;

		hkpFixedBufferRayHitCollector collector(command->m_results, command->m_resultsCapacity);
		g_FixedBufferRayHitCollectorAddRayHitCallbackFunc = &hkpFixedBufferRayHitCollector::addRayHitImplementation;

		hkpShapeRayCastInput rayInput = command->m_rayInput;

		for (int32_t i = 0; i < command->m_numCollidables; i++) {
			const hkpCollidable* collidable = command->m_collidables[i];
			const hkpShape* shape = collidable->m_shape;
			if (!shape) {
				continue;
			}
			const hkTransform& transform = *collidable->m_motion.pMotion;

			// The ray's ends in the shape's space: the transposed rotation times the end less the translation.
			{
				// EXE-INLINED 0xC7F690
				__m128 col0 = transform.m_rotation.m_col0.m_quad;
				__m128 col1 = transform.m_rotation.m_col1.m_quad;
				__m128 col2 = transform.m_rotation.m_col2.m_quad;
				__m128 v = _mm_sub_ps(command->m_rayInput.m_to.m_quad, transform.m_translation.m_quad);
				__m128 lo = _mm_unpacklo_ps(col0, col1);
				__m128 hi = _mm_unpackhi_ps(col0, col1);
				__m128 row0 = _mm_movelh_ps(lo, col2);
				__m128 row1 = _mm_shuffle_ps(_mm_movehl_ps(row0, lo), col2, _MM_SHUFFLE(3, 1, 1, 0));
				__m128 row2 = _mm_shuffle_ps(hi, col2, _MM_SHUFFLE(3, 2, 1, 0));
				rayInput.m_to.m_quad =
					_mm_add_ps(_mm_add_ps(_mm_mul_ps(row1, _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1))),
								   _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0)), row0)),
						_mm_mul_ps(row2, _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2))));
			}
			{
				// EXE-INLINED 0xC7F690
				__m128 col0 = transform.m_rotation.m_col0.m_quad;
				__m128 col1 = transform.m_rotation.m_col1.m_quad;
				__m128 col2 = transform.m_rotation.m_col2.m_quad;
				__m128 v = _mm_sub_ps(command->m_rayInput.m_from.m_quad, transform.m_translation.m_quad);
				__m128 lo = _mm_unpacklo_ps(col0, col1);
				__m128 hi = _mm_unpackhi_ps(col0, col1);
				__m128 row0 = _mm_movelh_ps(lo, col2);
				__m128 row1 = _mm_shuffle_ps(_mm_movehl_ps(row0, lo), col2, _MM_SHUFFLE(3, 1, 1, 0));
				__m128 row2 = _mm_shuffle_ps(hi, col2, _MM_SHUFFLE(3, 2, 1, 0));
				rayInput.m_from.m_quad =
					_mm_add_ps(_mm_add_ps(_mm_mul_ps(row1, _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1))),
								   _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0)), row0)),
						_mm_mul_ps(row2, _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2))));
			}

			// EXE-INLINED 0x62A2B0
			output.m_rootCollidable = nullptr;
			output.m_hitFraction = 1.0f;
			output.m_extraInfo = -1;
			output.m_shapeKeyIndex = 0;
			output.m_shapeKeys[0] = static_cast<uint32_t>(-1);

			if (command->m_useCollector.m_bool) {
				shape->castRayWithCollectorImpl(rayInput, *collidable, collector);
				continue;
			}
			if (!shape->castRay(rayInput, output).m_bool) {
				continue;
			}

			// The next free result while the buffer has room, else the result with the largest hit fraction when
			// this hit is nearer.
			hkpWorldRayCastOutput* dest;
			if (command->m_numResultsOut < command->m_resultsCapacity) {
				command->m_numResultsOut++;
				dest = nextFreeResult;
				nextFreeResult++;
			} else {
				int32_t numResults = command->m_numResultsOut;
				hkpWorldRayCastOutput* worst = command->m_results;
				for (int32_t k = 1; k < numResults; k++) {
					if (worst->m_hitFraction < command->m_results[k].m_hitFraction) {
						worst = &command->m_results[k];
					}
				}
				if (!(worst->m_hitFraction > output.m_hitFraction)) {
					continue;
				}
				dest = worst;
			}
			if (!dest) {
				continue;
			}

			// EXE-INLINED 0xC7F700
			const hkRotation& rotation = transform.m_rotation;
			__m128 n = output.m_normal.m_quad;
			output.m_normal.m_quad =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(n, n, _MM_SHUFFLE(0, 0, 0, 0)), rotation.m_col0.m_quad),
							   _mm_mul_ps(_mm_shuffle_ps(n, n, _MM_SHUFFLE(1, 1, 1, 1)), rotation.m_col1.m_quad)),
					_mm_mul_ps(_mm_shuffle_ps(n, n, _MM_SHUFFLE(2, 2, 2, 2)), rotation.m_col2.m_quad));

			// EXE-INLINED 0x9C1AA0
			dest->m_normal.m_quad = output.m_normal.m_quad;
			dest->m_hitFraction = output.m_hitFraction;
			dest->m_extraInfo = output.m_extraInfo;
			dest->m_pad[0] = output.m_pad[0];
			dest->m_pad[1] = output.m_pad[1];
			for (int32_t k = 0; k < hkpShapeRayCastOutput::MAX_HIERARCHY_DEPTH; k++) {
				dest->m_shapeKeys[k] = output.m_shapeKeys[k];
			}
			dest->m_shapeKeyIndex = output.m_shapeKeyIndex;
			// m_rootCollidable is declared as a non-const pointer, so the const is cast away.
			dest->m_rootCollidable = const_cast<hkpCollidable*>(collidable);
		}

		if (command->m_useCollector.m_bool) {
			command->m_numResultsOut = collector.m_numOutputs.m_storage;
		}
	}

	HK_TIMER_END();
	return jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}
