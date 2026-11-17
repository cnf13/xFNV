#pragma once

#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Reconstruction.hpp"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

#include <stdint.h>

class hkClass;
class hkCriticalSection;
class hkStatisticsCollector;
class hkpWorld;

#include "Havok/Source/Common/Base/Object/hkBaseObject.h"

// A reference counted Havok object: the memory size and flags word and the count are the only data.
class hkReferencedObject : public hkBaseObject {
public:
	enum LockMode {
		LOCK_MODE_NONE = 0x0,
		LOCK_MODE_AUTO = 0x1,
		LOCK_MODE_MANUAL = 0x2,
	};

	enum {
		// The part of m_memSizeAndFlags that is the object's allocated size.
		MASK_MEMSIZE = 0x7FFF,
	};

	hkReferencedObject();
	// The SDK's copy constructor: a copy starts with one reference of its own, not the original's count.
	hkReferencedObject(const hkReferencedObject&) : hkBaseObject(), m_referenceCount(1) {}
	// The SDK's finish constructor: a loaded object keeps the fields the load wrote.
	hkReferencedObject(hkFinishLoadedObjectFlag) {}
	virtual ~hkReferencedObject();

	// the class descriptor; the game's build carries none
	virtual const hkClass* getClassType() const { return nullptr; }
	virtual void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const;

	// The reference count.
	int32_t getReferenceCount() const { return static_cast<int16_t>(m_referenceCount); }
	// The size the object was allocated with.
	int32_t getAllocatedSize() const { return m_memSizeAndFlags & MASK_MEMSIZE; }
	// The four reference functions are const in the PDB (?addReference@hkReferencedObject@@QBAXXZ): a reference is
	// taken on and dropped from const objects too (hkpConvexListShape::setShapesAndRadius, 0xD5E6E0).
	void removeReferenceLockUnchecked() const;
	void addReferenceLockUnchecked() const;
	// Drops one reference, deleting the object with the last.
	void removeReference() const;
	// Takes one more reference.
	void addReference() const;
	// Not named in the PDB or IDA (IDA's j_ thunk name); name chosen here. A jump to
	// addReference (0xC90510) after hkpTypedBroadPhaseDispatcher's code; the exe has no caller or pointer of it
	// (thiscall, 0xCF7490).
	void addReference_CF7490();
	// A jump to
	// removeReference, beside addReference_CF7490.
	void removeReference_CF74A0();

	// Takes one more reference on each of aiNumObjects objects at apObjects, aiPointerStriding bytes apart; under the
	// lock unless the calling thread already holds it.
	static void addReferences(const hkReferencedObject* const* objects, int32_t numObjects, int32_t pointerStriding);

	// The SDK's inline overload for an array of pointers to a class derived from hkReferencedObject, cast as in
	// removeReferences below.
	template <typename T>
	static void addReferences(const T* const* objects, int32_t numObjects, int32_t pointerStriding = sizeof(T*)) {
		addReferences(reinterpret_cast<const hkReferencedObject* const*>(objects), numObjects, pointerStriding);
	}

	static void lockAll();
	static void unlockAll();
	// Havok SDK signature (hkBase.lib, tied by reloc).
	static void setLockMode(hkReferencedObject::LockMode mode);
	static hkReferencedObject::LockMode getLockMode();
	static hkCriticalSection* getLockCriticalSection();
	// The PDB's hkReferencedObject::operator delete (cdecl, ret 0, 0x538EB0): returns p's block,
	// of the size in its m_memSizeAndFlags, to the calling thread's heap allocator. Not named operator delete here: a
	// member operator delete makes the destructors of classes with two hkReferencedObject bases ambiguous (deleted).
	// Name chosen here.
	static void deleteObject(void* p);
	// The PDB's hkReferencedObject::operator new (cdecl, ret 0, 0x56D280): a block of
	// auiSize bytes from the calling thread's heap allocator, with auiSize stored in its m_memSizeAndFlags. Not
	// named operator new here, like deleteObject. Name chosen here.
	static void* newObject(uint32_t auiSize);
	// The SDK's class allocator (HK_DECLARE_CLASS_ALLOCATOR): `new` takes the block from the Havok heap and records its
	// size in m_memSizeAndFlags, which delete hands back to the heap and removeReference tests; the global operator
	// new (MemoryManager) leaves that field unset. Placement new is declared too, as the member new hides the global.
	static void* operator new(size_t auiSize) { return newObject(static_cast<uint32_t>(auiSize)); }
	static void* operator new(size_t, void* apMemory) { return apMemory; }
	static void operator delete(void* apMemory) { deleteObject(apMemory); }
	// Drops one reference from each of aiNumObjects objects at apObjects, aiPointerStriding bytes apart; under the
	// lock unless locking is off or the calling thread already holds it.
	static void removeReferences(const hkReferencedObject* const* objects, int32_t numObjects, int32_t pointerStriding);

	// The SDK's inline overload for an array of pointers to a class derived from hkReferencedObject. The SDK casts
	// the array itself: the base sits at offset 0 of every such class, so each element is also its base's address.
	template <typename T>
	static void removeReferences(const T* const* objects, int32_t numObjects, int32_t pointerStriding = sizeof(T*)) {
		removeReferences(reinterpret_cast<const hkReferencedObject* const*>(objects), numObjects, pointerStriding);
	}

	static void lockInit(hkReferencedObject::LockMode lockMode);

	uint16_t m_memSizeAndFlags;
	// Havok SDK: `mutable hkInt16`; the exe sign-extends it (movsx at 0xC87EDD, bhkRefObject::GetViewerStrings).
	mutable int16_t m_referenceCount;
};

// Havok SDK signature (hkBase.lib, tied by reloc,target).
const void* getVtablehkBaseObject();
// hkBaseObject's finish function in its type info: nothing to finish (cdecl, ret 0, an empty function, 0xC90330).
void finishLoadedObjecthkBaseObject(void* p, int32_t finishing);
// hkBaseObject's cleanup function in its type info: nothing to clean up (cdecl, ret 0, an empty function, 0xC90340).
void cleanupLoadedObjecthkBaseObject(void* p);
// hkReferencedObjectLock's singleton create function: a new lock from the heap (PDB hkReferencedObjectLockcreate;
// cdecl, ret 0, 0xC903E0).
hkReferencedObject* hkReferencedObjectLockcreate();
hkClass* getReferencedObjectClass();

inline constexpr fnvd::Vtable hkReferencedObject_vftable{};

// The critical section's thread nesting slot: TlsGetValue of its slot is 0x23DF4554 while the calling thread holds
// the section. Its initialiser and atexit thunk are hkThreadLocalData's inline constructor and
// destructor.
extern hkThreadLocalData<int32_t> s_intLockCount;
extern hkClass hkReferencedObjectClass;
extern hkTypeInfo hkReferencedObjectTypeInfo;
