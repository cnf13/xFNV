#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpTreeWorldManager.h"

class hkJobQueue;
class hkJobThreadPool;
class hkKdTree;
class hkpCollidable;
class hkpEntity;
class hkpPhantom;
class hkpWorld;
class hkpWorldObject;

// The tree world manager that keeps the world's collidables in a kd-tree, rebuilt when an object is added. As an
// hkpEntityListener it overrides the added, removed and set-motion-type callbacks, and as an hkpPhantomListener the
// added and removed callbacks.
class hkpKdTreeWorldManager : public hkpTreeWorldManager, public hkpEntityListener, public hkpPhantomListener {
public:
	enum ObjectRemovedBehavior : int32_t {
		MARK_TREE_AS_DIRTY = 0x0,
		REMOVE_OBJECT_FROM_TREE = 0x1,
	};

	// The manager of world's kd tree.
	hkpKdTreeWorldManager(hkpWorld* world, ObjectRemovedBehavior behav);
	~hkpKdTreeWorldManager() override;

	void entityAddedCallback(hkpEntity* entity) override;
	void entityRemovedCallback(hkpEntity* entity) override;
	void entitySetMotionTypeCallback(hkpEntity* entity) override;
	void phantomAddedCallback(hkpPhantom* phantom) override;
	void phantomRemovedCallback(hkpPhantom* phantom) override;
	void updateFromWorld(hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool) override;
	bool isUpToDate() const override { return m_mustUpdateFromWorld.m_bool == 0; }
	void setUpToDate(bool upToDate) override { m_mustUpdateFromWorld = !upToDate; }
	void castRay(const hkpWorldRayCastInput& input, const hkpCollisionFilter* filter,
		hkpWorldRayCastOutput& output) const override;
	void castRay(const hkpWorldRayCastInput& input, const hkpCollisionFilter* filter,
		hkpRayHitCollector& collector) const override;
	void linearCast(const hkpCollidable* collA, const hkpLinearCastInput& input,
		const hkpCollidableCollidableFilter* filter, const hkpCollisionInput& collInput,
		hkpCollisionAgentConfig* config, hkpCdPointCollector& castCollector,
		hkpCdPointCollector* startPointCollector) override;

	// Frees tree and builds it again from collidables: distributed when both jobQueue and
	// jobThreadPool are given, else single-threaded.
	void buildTree(hkKdTree*& tree, hkArray<const hkpCollidable*>& collidables, hkJobQueue* jobQueue,
		hkJobThreadPool* jobThreadPool);

protected:
	// Appends the world's collidables to collidables.
	void gatherCollidablesFromWorld(hkArray<const hkpCollidable*>& collidables);
	// Takes the object's collidable out of the tree by calling hkKdTree::invalidatePrimitive with the collidable's
	// address as the primitive id.
	void removeObjectFromTree(const hkpWorldObject* object);

public:
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc,string).
	void buildTreeSingleThreaded(hkKdTree*& tree, hkArray<const hkpCollidable*, hkContainerHeapAllocator>& collidables);
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
	void buildTreeDistributed(hkKdTree*& tree, hkArray<const hkpCollidable*, hkContainerHeapAllocator>& collidables,
		hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool);

	// Several bases declare one: DeletingDestruct's free, the Havok heap's block of m_memSizeAndFlags.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

	hkpWorld* m_world;
	hkBool m_mustUpdateFromWorld;
	hkEnum<ObjectRemovedBehavior, uint8_t> m_objectRemovedBehavior;
};
