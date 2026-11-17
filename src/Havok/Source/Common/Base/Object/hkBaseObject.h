#pragma once

#include "Reconstruction.hpp"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"

class hkClass;
class hkTypeInfo;

// The root of every Havok object; it exists to give its subclasses their vtable pointer.
class hkBaseObject {
public:
	hkBaseObject();
	// The SDK's finish constructor (hkBaseObject.h): nothing to finish; getVtablehkBaseObject builds one.
	hkBaseObject(hkFinishLoadedObjectFlag) {}
	virtual ~hkBaseObject();

	// DeletingDestruct's free (ourFree); hkReferencedObject's own hides it for its subclasses.
	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable hkBaseObject_vftable{};

// The class's reflection record.
extern hkClass hkBaseObjectClass;
// The PDB's name. The class's type info (its hkBaseObjectClass.cpp, defined in hkReferencedObject.cpp).
extern hkTypeInfo hkBaseObjectTypeInfo;

// After the class: hkClass.h reaches hkReferencedObject.h (through hkArray.h), which derives from hkBaseObject.
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
