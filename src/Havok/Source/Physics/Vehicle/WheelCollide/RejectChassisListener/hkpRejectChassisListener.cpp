#include "Havok/Source/Physics/Vehicle/WheelCollide/RejectChassisListener/hkpRejectChassisListener.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomOverlapListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

static void finishLoadedObjecthkpRejectChassisListener(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by reloc).
static const void* getVtablehkpRejectChassisListener();
static void cleanupLoadedObjecthkpRejectChassisListener(void* p);

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpRejectChassisListener, as the exe's .rdata holds them.
using hkpRejectChassisListenerClass_MembersArray = const hkClassMember[1];
static hkpRejectChassisListenerClass_MembersArray hkpRejectChassisListenerClass_Members{
	{"chassis", nullptr, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_VOID}, 0, {0x400}, 0xC, nullptr}};

// One interface with no table, as the SDK's generated record has it.
hkClass hkpRejectChassisListenerClass("hkpRejectChassisListener", &hkReferencedObjectClass, 16, nullptr, 1, nullptr, 0,
	hkpRejectChassisListenerClass_Members, 1, nullptr, nullptr, 0, 1);
hkTypeInfo hkpRejectChassisListenerTypeInfo = {"hkpRejectChassisListener", finishLoadedObjecthkpRejectChassisListener,
	cleanupLoadedObjecthkpRejectChassisListener, getVtablehkpRejectChassisListener(), &typeid(hkpRejectChassisListener),
	sizeof(hkpRejectChassisListener)};

static const void* getVtablehkpRejectChassisListener() {
	return hkGetVtableOf<hkpRejectChassisListener>();
}

static void cleanupLoadedObjecthkpRejectChassisListener(void* p) {
	static_cast<hkpRejectChassisListener*>(p)->~hkpRejectChassisListener();
}

// The type info's finish-loaded-object callback for hkpRejectChassisListener: the null check, then its finish
// constructor, which the exe inlines.
static void finishLoadedObjecthkpRejectChassisListener(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkpRejectChassisListener(kFlag);
	}
}

hkpRejectChassisListener::~hkpRejectChassisListener() {}

void hkpRejectChassisListener::collidableAddedCallback(const hkpCollidableAddedEvent& event) {
	const hkpCollidable* m_collidable = event.m_collidable;
	if (m_collidable == m_chassis || m_collidable->m_broadPhaseHandle.m_type != hkpWorldObject::BROAD_PHASE_ENTITY) {
		event.m_collidableAccept = HK_COLLIDABLE_REJECT;
	}
}

void hkpRejectChassisListener::collidableRemovedCallback(const hkpCollidableRemovedEvent& event) {
	;
}

const hkClass& hkpRejectChassisListener::staticClass() {
	return hkpRejectChassisListenerClass;
}
