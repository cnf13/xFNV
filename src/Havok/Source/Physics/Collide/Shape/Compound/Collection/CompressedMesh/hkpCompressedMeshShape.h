#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/Mesh/hkpNamedMeshMaterial.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"

#include <stdint.h>
#include <xmmintrin.h>

void cleanupLoadedObjecthkpCompressedMeshShape(void* p);
// The type info's finish-loaded-object callback.
void finishLoadedObjecthkpCompressedMeshShape(void* p, int32_t finishing);
__m128* calcVerticesAabb(
    hkArrayBase<uint16_t>* vertices,
    hkVector4* offset,
    float error,
    __m128* aabbOut,
    const hkVector4* scaling
);

// A triangle mesh stored compressed: chunks of quantized vertices and triangle strips, plus big triangles and
// convex pieces kept apart, with per-triangle materials in one of several widths.
class hkpCompressedMeshShape : public hkpShapeCollection {
public:
    enum MaterialType : int32_t {
        MATERIAL_NONE = 0x0,
        MATERIAL_SINGLE_VALUE_PER_CHUNK = 0x1,
        MATERIAL_ONE_BYTE_PER_TRIANGLE = 0x2,
        MATERIAL_TWO_BYTES_PER_TRIANGLE = 0x3,
        MATERIAL_FOUR_BYTES_PER_TRIANGLE = 0x4,
    };

    // A triangle kept out of the chunks, indexing m_bigVertices.
    struct BigTriangle {
        static const hkClass& staticClass();

        uint16_t m_a;
        uint16_t m_b;
        uint16_t m_c;
        uint32_t m_material;
        uint16_t m_weldingInfo;
    };

    // A piece of the mesh: quantized vertices relative to m_offset and triangle strips over them.
    struct alignas(16) Chunk {
        ~Chunk();

        void
        getTriangle(int32_t index, float afScale, hkVector4* arVertex0, hkVector4* arVertex1, hkVector4* arVertex2);
        int32_t getNextIndex(int32_t index, int32_t& winding);
        void calcAabb(float error, hkAabb& out, const hkVector4& scaling);
        int32_t getNumTriangles();

        static const hkClass& staticClass();

        hkVector4 m_offset;
        hkArray<uint16_t> m_vertices;
        hkArray<uint16_t> m_indices;
        hkArray<uint16_t> m_stripLengths;
        hkArray<uint16_t> m_weldingInfo;
        uint32_t m_materialInfo;
        uint16_t m_reference;
        uint16_t m_transformIndex;
    };

    // A convex piece of the mesh: quantized vertices relative to m_offset and its faces.
    struct alignas(16) ConvexPiece {
        ~ConvexPiece();

        void getVertices(
            float quantization,
            const hkTransform& transform,
            hkArray<hkVector4, hkContainerHeapAllocator>& vertices
        );

        static const hkClass& staticClass();

        hkVector4 m_offset;
        hkArray<uint16_t> m_vertices;
        hkArray<uint8_t> m_faceVertices;
        hkArray<uint16_t> m_faceOffsets;
        uint16_t m_reference;
        uint16_t m_transformIndex;
    };

    hkpCompressedMeshShape(float radius);
    // The finish constructor: a loaded shape keeps its arrays.
    hkpCompressedMeshShape(hkFinishLoadedObjectFlag flag);
    ~hkpCompressedMeshShape() override;

    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
    // Havok SDK signature (hkpCollide.lib, tied by vtable).
    int32_t calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const override;
    void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
    void initWeldingInfo(hkpWeldingUtility::WeldingType weldingType) override;
    void setWeldingInfo(uint32_t key, int16_t weldingInfo) override;
    // hkpShapeContainer's vtable at +0x10.
    uint32_t getFirstKey() const override;
    // hkpShapeContainer's vtable at +0x10.
    // Havok SDK signature (hkpCollide.lib, tied by bytes,vtable).
    uint32_t getNextKey(uint32_t oldKey) const override;
    // hkpShapeContainer's vtable at +0x10.
    // Havok SDK signature (hkpCollide.lib, tied by vtable).
    uint32_t getCollisionFilterInfo(uint32_t key) const override;
    // hkpShapeContainer's vtable at +0x10.
    const hkpShape* getChildShape(uint32_t, char (&)[512]) const override;

    void setScaling(const hkVector4& scaling);
    // Havok SDK signature (hkpCollide.lib, tied by bytes).
    int32_t isValidShapeKey(uint32_t key);
    const hkpMeshMaterial* getMaterial(uint32_t key);
    // apChunk: the chunk whose m_reference and m_transformIndex are read; arOut: the AABB it fills through
    // Chunk::calcAabb.
    void getChunkAabb(Chunk* apChunk, hkAabb& arOut);
    void recalcAabb(hkAabb& out);

    // The three vertices at the given indices of apChunk's m_vertices (each already multiplied by 3), dequantized:
    // m_error times the component plus the chunk's m_offset.
    static void getChunkVertex(
        Chunk* apChunk,
        int32_t aiIndex0,
        int32_t aiIndex1,
        int32_t aiIndex2,
        float afScale,
        hkVector4& arVertex0,
        hkVector4& arVertex1,
        hkVector4& arVertex2
    );
    static const hkClass& staticClass();

    int32_t m_bitsPerIndex;
    int32_t m_bitsPerWIndex;
    int32_t m_wIndexMask;
    int32_t m_indexMask;
    float m_radius;
    hkEnum<hkpWeldingUtility::WeldingType, uint8_t> m_weldingType;
    hkEnum<MaterialType, uint8_t> m_materialType;
    hkArray<uint32_t> m_materials;
    hkArray<uint16_t> m_materials16;
    hkArray<uint8_t> m_materials8;
    hkArray<hkTransform> m_transforms;
    hkArray<hkVector4> m_bigVertices;
    hkArray<BigTriangle> m_bigTriangles;
    hkArray<Chunk> m_chunks;
    hkArray<ConvexPiece> m_convexPieces;
    float m_error;
    hkAabb m_bounds;
    hkpMeshMaterial* m_meshMaterials;
    uint16_t m_materialStriding;
    uint16_t m_numMaterials;
    hkArray<hkpNamedMeshMaterial> m_namedMaterials;
    hkVector4 m_scaling;
};

extern hkClass hkpCompressedMeshShapeChunkClass;
extern hkClass hkpCompressedMeshShapeBigTriangleClass;
extern hkClass hkpCompressedMeshShapeConvexPieceClass;
extern hkClass hkpCompressedMeshShapeClass;
extern hkTypeInfo hkpCompressedMeshShapeTypeInfo;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpCompressedMeshShapeChunkTypeInfo;
extern hkTypeInfo hkpCompressedMeshShapeBigTriangleTypeInfo;
extern hkTypeInfo hkpCompressedMeshShapeConvexPieceTypeInfo;
