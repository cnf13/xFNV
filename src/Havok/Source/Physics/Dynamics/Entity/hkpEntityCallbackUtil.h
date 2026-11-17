#pragma once

class hkpCollisionEvent;
class hkpContactListener;
struct hkpContactPointAddedEvent;
class hkpContactPointEvent;
struct hkpContactPointRemovedEvent;
struct hkpContactProcessEvent;
class hkpEntity;

template <typename T> class hkSmallArray;

// The notifications an entity runs after it changed: each tells every listener the entity holds.
class hkpEntityCallbackUtil {
public:
	// Tells apEntity's listeners that it was added to a world.
	static void fireEntityAdded(hkpEntity* entity);
	// Tells apEntity's listeners that it was removed from its world.
	static void fireEntityRemoved(hkpEntity* entity);
	// Tells apEntity's listeners that its shape was replaced.
	static void fireEntityShapeSet(hkpEntity* entity);
	// Tells apEntity's contact listeners that a collision was added.
	static void fireContactConstraintAddedCallback(hkpEntity* entity, hkpCollisionEvent& event);
	// Tells apEntity's contact listeners that a collision was removed.
	static void fireContactConstraintRemovedCallback(hkpEntity* entity, hkpCollisionEvent& event);
	// Tells apEntity's contact listeners that a contact point was added.
	static void fireContactPointAddedInternal(hkpEntity* entity, hkpContactPointAddedEvent& event);
	// Tells apEntity's contact listeners that a contact point was removed.
	static void fireContactPointRemovedInternal(hkpEntity* entity, hkpContactPointRemovedEvent& event);
	// Tells apEntity's contact listeners about a contact point, through the newer contactPointCallback.
	static void fireContactPointCallbackInternal(hkpEntity* entity, hkpContactPointEvent& event);
	// Tells apEntity's contact listeners that its contact points were processed.
	static void fireContactProcessInternal(hkpEntity* entity, hkpContactProcessEvent& event);
	// Removes the null entries from a listener array, keeping the order.
	static void cleanupNullPointers(hkSmallArray<hkpContactListener*>& arArray);
	// Tells apEntity's listeners that its motion type changed.
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
	static void fireEntitySetMotionType(hkpEntity* entity);
	// Tells apEntity's listeners that it is being deleted.
	static void fireEntityDeleted(hkpEntity* entity);
};
