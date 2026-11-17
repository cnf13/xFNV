#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

// A set of reflection classes found by name. The exe's vtable overrides the destructor and adds slots 3 to 5 (all
// pure here).
class hkClassNameRegistry : public hkReferencedObject {
public:
	virtual const char* getName() const = 0;
	virtual const hkClass* getClassByName(const char* className) const = 0;
	virtual void getClasses(hkArray<const hkClass*>& classes) const = 0;
};
