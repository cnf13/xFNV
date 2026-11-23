#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/Mesh/hkpMeshMaterial.h"

#include <stdint.h>

class hkpNamedMeshMaterial;

// The index count of numTriangles triangles given as indicesPerTriangle indices each (1: a strip, 2: pairs, else four
// per triangle); a helper of hkpStorageExtendedMeshShape::addTrianglesSubpart.
int32_t calcNumIndices(int32_t numTriangles, int32_t indicesPerTriangle);

// An extended mesh shape that owns the memory of its subparts: each subpart added gets a storage object holding copies
// of its vertices, indices and materials. The base is 16-byte aligned.
class hkpStorageExtendedMeshShape : public hkpExtendedMeshShape {
public:
    struct Material : public hkpMeshMaterial {
        static const hkClass& staticClass();

        hkHalf m_restitution;
        hkHalf m_friction;
        hkUlong m_userData;
    };

    // The copies of one triangles subpart's data.
    class MeshSubpartStorage : public hkReferencedObject {
    public:
        MeshSubpartStorage();
        MeshSubpartStorage(hkFinishLoadedObjectFlag flag);
        ~MeshSubpartStorage() override;

        static const hkClass& staticClass();

        hkArray<hkVector4> m_vertices;
        hkArray<uint8_t> m_indices8;
        hkArray<uint16_t> m_indices16;
        hkArray<uint32_t> m_indices32;
        hkArray<uint8_t> m_materialIndices;
        hkArray<Material> m_materials;
        hkArray<hkpNamedMeshMaterial> m_namedMaterials;
        hkArray<uint16_t> m_materialIndices16;
    };

    // The copies of one shapes subpart's data.
    class ShapeSubpartStorage : public hkReferencedObject {
    public:
        ShapeSubpartStorage();
        ShapeSubpartStorage(hkFinishLoadedObjectFlag flag);
        ~ShapeSubpartStorage() override;

        static const hkClass& staticClass();

        hkArray<uint8_t> m_materialIndices;
        hkArray<Material> m_materials;
        hkArray<uint16_t> m_materialIndices16;
    };

    // hkpStorageExtendedMeshShape::Members: the member table hkpStorageExtendedMeshShapeClass describes the class with.
    using MembersArray = const hkClassMember[2];

    hkpStorageExtendedMeshShape(hkFinishLoadedObjectFlag flag);
    hkpStorageExtendedMeshShape(float radius, int32_t numBitsForSubpartIndex);
    hkpStorageExtendedMeshShape(const hkpExtendedMeshShape* mesh);
    ~hkpStorageExtendedMeshShape() override;

    void addTrianglesSubpart(const TrianglesSubpart& partIn) override;
    int32_t addShapesSubpart(const ShapesSubpart& partIn) override;

    static const hkClass& staticClass();

    hkArray<MeshSubpartStorage*> m_meshstorage;
    hkArray<ShapeSubpartStorage*> m_shapestorage;

    static MembersArray Members;
};

extern hkClass hkpStorageExtendedMeshShapeMaterialClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpStorageExtendedMeshShapeMaterialTypeInfo;
