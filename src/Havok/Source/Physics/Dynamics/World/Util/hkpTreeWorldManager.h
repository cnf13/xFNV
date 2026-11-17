#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Reconstruction.hpp"

template <class BOUNDING_VOLUME, typename IDX> class hkAabbTree;
class hkJobQueue;
class hkJobThreadPool;
class hkKdTree;
class hkpCdPointCollector;
class hkpCollidable;
class hkpCollidableCollidableFilter;
class hkpCollisionAgentConfig;
class hkpCollisionFilter;
class hkpCollisionInput;
class hkpLinearCastInput;
class hkpRayHitCollector;
struct hkpWorldRayCastInput;

// The base of the world managers that keep a tree of the world's collidables for ray and linear cast queries. The
// exe has no vtable of its own for this abstract class; its slots 3 to 10 are the PDB's intro virtuals.
class hkpTreeWorldManager : public hkReferencedObject {
public:
	// Values from the Xbox PDB.
	enum Type : int32_t {
		MANAGER_KDTREE = 0x0,
		MANAGER_AABBTREE = 0x1,
	};

	virtual void updateFromWorld(hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool) = 0;
	virtual bool isUpToDate() const = 0;
	virtual void setUpToDate(bool upToDate) = 0;
	// MSVC lays the two overloads out in reverse declaration order: the collector one is slot 6, this one slot 7.
	virtual void castRay(const hkpWorldRayCastInput& input, const hkpCollisionFilter* filter,
		hkpWorldRayCastOutput& output) const = 0;
	virtual void castRay(const hkpWorldRayCastInput& input, const hkpCollisionFilter* filter,
		hkpRayHitCollector& collector) const = 0;
	virtual void linearCast(const hkpCollidable* collA, const hkpLinearCastInput& input,
		const hkpCollidableCollidableFilter* filter, const hkpCollisionInput& collInput,
		hkpCollisionAgentConfig* config, hkpCdPointCollector& castCollector,
		hkpCdPointCollector* startPointCollector) = 0;
	virtual int32_t calcWorkingBufferSize(int32_t maxObjects);
	virtual void setWorkingBuffer(void* ptr) {}

	Type m_type;
	hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* m_inactiveTree;
	hkKdTree* m_kdTree;
};
