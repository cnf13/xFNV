#pragma once

// The native finish constructor's base subobject is destroyed through ~hkRefPtr, which releases the object. After
// the class: hkReferencedObject.h's includes reach headers holding an hkRefVariant by value (hkxAttribute.h).
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"

// A reference counted pointer to an object of any class; the class is looked up from the object's vtable. Its one
// field is the base's pointer.
class hkRefVariant : public hkRefPtr<hkReferencedObject> {
public:
	// A null variant: the classes holding one as a member build it empty.
	hkRefVariant() = default;
	// The SDK's finish constructor: a loaded variant keeps what was loaded.
	hkRefVariant(hkFinishLoadedObjectFlag flag) : hkRefPtr<hkReferencedObject>(flag) {}
	hkRefVariant(void* a1, const hkClass* a2);

	// The object's class from the vtable class registry, or null.
	const hkClass* getClass() const;
	// The SDK's hkRefVariant(const hkVariant&) body: takes a reference on the variant's object and holds it; returns
	// this.
	hkReferencedObject** assignVariant(const hkVariant& v);
	void set(void* o, const hkClass* k);
};
