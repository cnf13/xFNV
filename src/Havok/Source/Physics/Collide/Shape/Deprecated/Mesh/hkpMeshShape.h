#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"

#include <stdint.h>

class hkpMeshMaterial;

// Defined in hkpShape.cpp.
void finishLoadedObjecthkpMeshShape(void* p, int32_t finishing);

// Havok's triangle mesh shape: a collection of subparts, each a strided vertex and triangle index buffer, with an
// optional material (collision filter info) per triangle.
class hkpMeshShape : public hkpShapeCollection {
public:
    enum MeshShapeIndexStridingType : int32_t {
        INDICES_INVALID = 0x0,
        INDICES_INT16 = 0x1,
        INDICES_INT32 = 0x2,
        INDICES_MAX_ID = 0x3,
    };

    enum MeshShapeMaterialIndexStridingType : int32_t {
        MATERIAL_INDICES_INVALID = 0x0,
        MATERIAL_INDICES_INT8 = 0x1,
        MATERIAL_INDICES_INT16 = 0x2,
        MATERIAL_INDICES_MAX_ID = 0x3,
    };

    // One part of the mesh: where its vertices, triangle indices and materials are and how they are strided.
    struct Subpart {
        static const hkClass& staticClass();

        const float* m_vertexBase;
        int32_t m_vertexStriding;
        int32_t m_numVertices;
        const void* m_indexBase;
        hkEnum<MeshShapeIndexStridingType, int8_t> m_stridingType;
        hkEnum<MeshShapeMaterialIndexStridingType, int8_t> m_materialIndexStridingType;
        int32_t m_indexStriding;
        int32_t m_flipAlternateTriangles;
        int32_t m_numTriangles;
        const void* m_materialIndexBase;
        int32_t m_materialIndexStriding;
        const hkpMeshMaterial* m_materialBase;
        int32_t m_materialStriding;
        int32_t m_numMaterials;
        int32_t m_triangleOffset;
    };

    hkpMeshShape(float a1, int32_t a2);
    hkpMeshShape(hkFinishLoadedObjectFlag flag);
    ~hkpMeshShape() override;

    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
    void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
    void initWeldingInfo(hkpWeldingUtility::WeldingType weldingType) override;
    void setWeldingInfo(uint32_t key, int16_t weldingInfo) override;
    virtual void addSubpart(const Subpart& part);
    // hkpShapeContainer's vtable at +0x10.
    uint32_t getFirstKey() const override;
    // hkpShapeContainer's vtable at +0x10.
    uint32_t getNextKey(uint32_t initialKey) const override;
    // hkpShapeContainer's vtable at +0x10.
    uint32_t getCollisionFilterInfo(uint32_t key) const override;
    // hkpShapeContainer's vtable at +0x10.
    const hkpShape* getChildShape(uint32_t key, char (&buffer)[512]) const override;

    void setScaling(const hkVector4& scaling);
    // Checks part's members in a debug build: empty here.
    void assertSubpartValidity(const Subpart& part);

    // The material index of the triangle key names, or -1 where its subpart has no material indices.
    int32_t getMaterialIndex(uint32_t key) const {
        const uint32_t subPartIndex = key >> (32 - m_numBitsForSubpartIndex);
        const Subpart& part = m_subparts.m_data[subPartIndex];

        if (part.m_materialIndexBase != nullptr) {
            const uint32_t triangleIndex = key & (~0U >> m_numBitsForSubpartIndex);
            const void* keyAddress =
                static_cast<const char*>(part.m_materialIndexBase) + triangleIndex * part.m_materialIndexStriding;
            int32_t materialId = (part.m_materialIndexStridingType == MATERIAL_INDICES_INT8)
                ? *static_cast<const uint8_t*>(keyAddress)
                : *static_cast<const uint16_t*>(keyAddress);
            return materialId;
        }
        return -1;
    }

    // Widens aabb by the vertex apVertex, scaled by scaling and transformed by localToWorld.
    static void
    addToAabb(hkAabb& aabb, const hkTransform& localToWorld, const float* apVertex, const hkVector4& scaling);
    static const hkClass& staticClass();

    hkVector4 m_scaling;
    int32_t m_numBitsForSubpartIndex;
    hkArray<Subpart> m_subparts;
    hkArray<uint16_t> m_weldingInfo;
    hkEnum<hkpWeldingUtility::WeldingType, uint8_t> m_weldingType;
    float m_radius;
    int32_t m_pad[3];
};

extern hkClass hkpMeshShapeSubpartClass;
extern hkClass hkpMeshShapeClass;
extern hkTypeInfo hkpMeshShapeTypeInfo;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpMeshShapeSubpartTypeInfo;
