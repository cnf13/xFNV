#pragma once

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/Mesh/hkpMeshMaterial.h"

// The type info's cleanup-loaded-object callback: it runs the destructor, which only destroys m_name.
void cleanupLoadedObjecthkpNamedMeshMaterial(void* p);

// A mesh material with a name, as the storage mesh shapes keep per subpart. It has no vtable.
class hkpNamedMeshMaterial : public hkpMeshMaterial {
public:
    // The class's reflection record, hkpNamedMeshMaterialClass.
    static const hkClass& staticClass();

    hkStringPtr m_name;
};

// The class's reflection record.
extern hkClass hkpNamedMeshMaterialClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpNamedMeshMaterialTypeInfo;
