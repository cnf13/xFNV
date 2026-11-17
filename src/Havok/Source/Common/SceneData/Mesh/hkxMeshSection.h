#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Base/Types/hkRefVariant.h"

#include <stdint.h>

class hkGeometry;
class hkxIndexBuffer;
class hkxMaterial;
class hkxVertexBuffer;

// Describes one scene mesh section, including its vertex/index buffers and material.
class hkxMeshSection : public hkReferencedObject {
public:
    hkxMeshSection(const hkxMeshSection& other);
    // The SDK's finish constructor: a loaded object keeps the fields the load wrote (defined in hkxMeshSection.cpp,
    // where the referenced classes are complete).
    hkxMeshSection(hkFinishLoadedObjectFlag aFlag);
    ~hkxMeshSection() override;

    // Havok SDK signature (hkSceneData.lib, tied by bytes,reloc,string).
    void appendGeometry(hkGeometry& geometryInOut, int32_t materialIndex);
    // Havok SDK signature (hkSceneData.lib, tied by reloc).
    void collectVertexPositions(hkArray<hkVector4, hkContainerHeapAllocator>& verticesInOut) const;
    uint32_t getNumTriangles();
    void getTriangleIndices(uint32_t triIndex, uint32_t& indexAOut, uint32_t& indexBOut, uint32_t& indexCOut);
    // Copies the reference-object words, the vertex buffer and material references and the index buffer and user
    // channel arrays from other.
    hkxMeshSection& operator=(const hkxMeshSection& other);

    // The reflection class hkxMeshSectionClass.
    static const hkClass& staticClass();

    hkRefPtr<hkxVertexBuffer> m_vertexBuffer;
    hkArray<hkRefPtr<hkxIndexBuffer>, hkContainerHeapAllocator> m_indexBuffers;
    hkRefPtr<hkxMaterial> m_material;
    hkArray<hkRefVariant, hkContainerHeapAllocator> m_userChannels;
};
