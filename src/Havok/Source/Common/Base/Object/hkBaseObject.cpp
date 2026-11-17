#include "Havok/Source/Common/Base/Object/hkBaseObject.h"

// The class's reflection record (the SDK's hkBaseObjectClass.cpp, PDB module hkBaseObjectClass.obj; the SDK has no
// hkBaseObject.cpp, so the record lives beside the class's header): no parent, enums or members; one implemented
// interface, the vtable.
hkClass hkBaseObjectClass("hkBaseObject", nullptr, 4, nullptr, 1, nullptr, 0, nullptr, 0, nullptr, nullptr, 0, 0);
