#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"

#include <stdint.h>
#include <xmmintrin.h>

struct hkEntry;
class hkpBvTreeShape;
class hkpShapeCollection;
class hkpTriangleShape;

// Clips the edge from edgeStart to edgeEnd against numPlanes planes (xyz the normal, w the offset): false when both
// ends lie on the positive side of one plane, else true with tStart and tEnd the clipped ends' parameters along the
// edge. The hkBool is written to apResult, which is returned.
hkBool hkClipEdgeAgainstPlanes(
    const hkVector4& edgeStart,
    const hkVector4& edgeEnd,
    int32_t numPlanes,
    const hkVector4* planes,
    float& tStart,
    float& tEnd
);
int32_t getSectorFromWeldingInfo(uint16_t weldingInfo, char edgeIndex);
bool* entriesDiffer(bool* result, const hkEntry* a, const hkEntry* b);
void hkBuildClipPlanesForTriangle(hkVector4* out, const hkpTriangleShape* triangle, float extraRadius);
uint8_t* isSameTriangle(uint8_t* result, const hkpTriangleShape* a, const hkpTriangleShape* b);
uint8_t* triangleHasVertices(
    uint8_t* result,
    const hkVector4* pointA,
    const hkVector4* pointB,
    const hkpTriangleShape* triangle
);
bool* isPerpendicular(bool* result, const hkVector4* dirA, const hkVector4* dirB);
int32_t testWindingConsistency(
    const hkVector4* point,
    const hkVector4* normal1,
    const hkVector4* normal2,
    const hkpTriangleShape* triangle,
    const hkVector4* dir
);
int32_t createSingularVertexArray(
    const hkVector4* vertices,
    const hkVector4* otherVertices,
    int32_t edgeIndex,
    hkVector4* out,
    int32_t* sharedIndices
);

// Havok's utility that welds the triangles of a mesh shape so contacts across their edges are smooth. It only
// groups functions and types; it has no members.
class hkpMeshWeldingUtility {
public:
    enum WindingConsistency : int32_t {
        WINDING_IGNORE_CONSISTENCY = 0x0,
        WINDING_TEST_CONSISTENCY = 0x1,
    };

    // A mesh shape and the transform that places it. The trailing 12 bytes are the 16-byte alignment's padding.
    struct ShapeInfo {
        // The copy constructor hides the implicit default one, which the arrays of shape infos need.
        ShapeInfo() = default;
        ShapeInfo(const hkpMeshWeldingUtility::ShapeInfo& other);

        hkTransform m_transform;
        const hkpBvTreeShape* m_shape;
        uint8_t padding44[12];
    };

    // Havok SDK signature (hkpCollide.lib, tied by bytes).
    static uint16_t modifyCombinedEdgesBitcode(uint16_t combinedBitcode, int32_t edgeIndex, int32_t bitcode);
    static int32_t calcEdgeAngleBitcode(float cosAngle);
    static hkResult calcBitcodeForTriangleEdge(
        const hkpBvTreeShape* bvTreeShape,
        const hkpTriangleShape* triangleShape,
        uint32_t triangleShapeKey,
        int32_t edgeIndex,
        hkpMeshWeldingUtility::WindingConsistency testConsistency,
        uint16_t& combinedBitcodesOut
    );
    static float
    calcAngleForEdge(const hkVector4& edgeNormal, const hkVector4& edgeOrtho, const hkVector4& triangleNormal);
    static void calcAntiClockwiseTriangleNormal(
        const hkVector4& vertex0,
        const hkVector4& vertex1,
        const hkVector4& vertex2,
        hkVector4& normal
    );
    static void computeWeldingInfoMultiShape(
        const hkTransform& meshTransform,
        hkpShapeCollection* mesh,
        hkpWeldingUtility::WeldingType weldingType,
        hkArray<hkpMeshWeldingUtility::ShapeInfo, hkContainerHeapAllocator>& allShapes,
        bool weldOpenEdges
    );
    static void computeWeldingInfo(
        hkpShapeCollection* collection,
        const hkpBvTreeShape* bvTree,
        hkpWeldingUtility::WeldingType weldingType,
        bool weldOpenEdges
    );
    // Havok SDK signature (hkpCollide.lib, tied by reloc).
    static float calcAngleFromVertices(const hkVector4* vertices, float& sinAngleOut, float& cosAngleOut);
    // Havok SDK signature (hkpCollide.lib, tied by bytes).
    static hkResult calcWeldingInfoForTriangle(
        uint32_t shapeKey,
        const hkpBvTreeShape* bvTreeShape,
        hkpMeshWeldingUtility::WindingConsistency testConsistency,
        uint16_t& info
    );
    static hkBool isTriangleWindingValid(uint32_t shapeKey, const hkpBvTreeShape* bvTreeShape);
    static int32_t calcEdgeAngleBitcode(hkVector4* vertices);
};
