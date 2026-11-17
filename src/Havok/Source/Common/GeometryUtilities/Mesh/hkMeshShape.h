#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Base/Types/hkTypeTraits.h"

class hkClassEnum;
class hkMeshVertexBuffer;
struct hkVertexFormat;

// The material of a mesh section: the vertex format and vertex buffer it accepts, and its name. No data of its own (the
// layout is the hkReferencedObject base).
class hkMeshMaterial : public hkReferencedObject {
public:
    virtual hkResult createCompatibleVertexFormat(const hkVertexFormat& format, hkVertexFormat& compatibleFormat) = 0;
    virtual hkMeshVertexBuffer* createCompatibleVertexBuffer(hkMeshVertexBuffer* buffer) = 0;
    virtual bool isCompatible(const hkMeshVertexBuffer* buffer) = 0;
    virtual const char* getName() const {
        return nullptr;
    }
};

// One section of a locked mesh: its primitives, their indices, and the vertex buffer and material they use.
struct hkMeshSection {
    // The kind of the index data.
    enum MeshSectionIndexType : int32_t {
        INDEX_TYPE_NONE = 0,
        INDEX_TYPE_UINT16 = 1,
        INDEX_TYPE_UINT32 = 2,
    };

    // The kind of the primitives.
    enum PrimitiveType : int32_t {
        PRIMITIVE_TYPE_UNKNOWN = 0,
        PRIMITIVE_TYPE_POINT_LIST = 1,
        PRIMITIVE_TYPE_LINE_LIST = 2,
        PRIMITIVE_TYPE_TRIANGLE_LIST = 3,
        PRIMITIVE_TYPE_TRIANGLE_STRIP = 4,
    };

    static const hkClass& staticClass();

    hkEnum<PrimitiveType, uint8_t> m_primitiveType;
    int32_t m_numPrimitives;
    int32_t m_numIndices;
    int32_t m_vertexStartIndex;
    int32_t m_transformIndex;
    hkEnum<MeshSectionIndexType, uint8_t> m_indexType;
    const void* m_indices;
    hkMeshVertexBuffer* m_vertexBuffer;
    hkMeshMaterial* m_material;
    int32_t m_sectionIndex;
};

// What a mesh section is built from.
struct hkMeshSectionCinfo {
    // The SDK's: its arrays treat it as plain data (hkTypeTraits.h).
    HK_DECLARE_POD_TYPE();

    static const hkClass& staticClass();

    hkMeshVertexBuffer* m_vertexBuffer;
    hkMeshMaterial* m_material;
    hkEnum<hkMeshSection::PrimitiveType, uint8_t> m_primitiveType;
    int32_t m_numPrimitives;
    hkEnum<hkMeshSection::MeshSectionIndexType, uint8_t> m_indexType;
    const void* m_indices;
    int32_t m_vertexStartIndex;
    int32_t m_transformIndex;
};

// A renderable mesh split into sections that are locked one at a time.
class hkMeshShape : public hkReferencedObject {
public:
    // What lockSection hands out besides the section's counts: hkMemoryMeshShape::lockSection tests 0x1
    // for the indices and 0x2 for the vertex buffer.
    enum AccessFlags : int32_t {
        ACCESS_INDICES = 0x1,
        ACCESS_VERTEX_BUFFER = 0x2,
    };

    // The constructor the derived classes' own use, and the SDK's finish constructor of a loaded object (the derived
    // classes' finish constructors call it): it keeps the fields the load wrote.
    hkMeshShape() = default;
    hkMeshShape(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag) {}

    virtual int32_t getNumSections() const = 0;
    virtual void lockSection(int32_t sectionIndex, uint8_t accessFlags, hkMeshSection& sectionOut) const = 0;
    virtual void unlockSection(const hkMeshSection& section) const = 0;
    virtual const char* getName() const {
        return nullptr;
    }
    virtual void setName(const char* n) {}
};

extern hkClass hkMeshSectionClass;
extern hkClass hkMeshSectionCinfoClass;

// The records of hkMeshSection's two enums (entries of hkMeshShapeClass.cpp's enum table), which the member tables of
// hkMeshSection and hkMeshSectionCinfo read at start-up.
extern const hkClassEnum* hkMeshSectionMeshSectionIndexTypeEnum;
extern const hkClassEnum* hkMeshSectionPrimitiveTypeEnum;

// The record of hkMeshShape's enum (from hkMeshShapeClass.cpp's enum table), by pointer.
extern const hkClassEnum* hkMeshShapeAccessFlagsEnum;

// The class's reflection record.
extern hkClass hkMeshMaterialClass;

// The class's reflection record.
extern hkClass hkMeshShapeClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkMeshSectionTypeInfo;
extern hkTypeInfo hkMeshSectionCinfoTypeInfo;
