#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"

class hkxMaterialShader;

// Holds the shaders associated with a scene material.
class hkxMaterialShaderSet : public hkReferencedObject {
public:
    hkxMaterialShaderSet();
    hkxMaterialShaderSet(hkFinishLoadedObjectFlag flag);
    ~hkxMaterialShaderSet() override;

    // The reflection class hkxMaterialShaderSetClass.
    static const hkClass& staticClass();

    hkArray<hkRefPtr<hkxMaterialShader>, hkContainerHeapAllocator> m_shaders;
};
