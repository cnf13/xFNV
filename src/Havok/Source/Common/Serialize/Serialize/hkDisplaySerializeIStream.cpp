#include "Havok/Source/Common/Serialize/Serialize/hkDisplaySerializeIStream.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Io/IArchive/hkIArchive.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayAABB.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayBox.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayCapsule.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayCone.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayConvex.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayCylinder.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayPlane.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplaySemiCircle.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplaySphere.h"
#include "xfnv/Compiler.h"

hkDisplaySerializeIStream::hkDisplaySerializeIStream(hkStreamReader* apStream) : hkIArchive(apStream, false) {}

hkDisplaySerializeIStream::~hkDisplaySerializeIStream() {}

// Reads x, y and z and clears w.
void hkDisplaySerializeIStream::readQuadVector4(hkVector4& v) {
    readArrayGeneric(&v, 4, 3);
    XFNV_M128_F32(v.m_quad, 3) = 0.0f;
}

void hkDisplaySerializeIStream::readTransform(hkTransform& t) {
    hkVector4 array;
    hkQuaternion arQuaternion;
    readArrayGeneric(&array, 4, 3);
    XFNV_M128_F32(array.m_quad, 3) = 1.0f;
    readArrayGeneric(&arQuaternion, 4, 4);
    t.m_translation.m_quad = array.m_quad;
    t.m_rotation.set(arQuaternion);
}

// Reads the triangle's three vertex indices, one int each.
void hkDisplaySerializeIStream::readTriangle(hkGeometry::Triangle& ti) {
    int32_t a;
    readArrayGeneric(&a, 4, 1);
    ti.m_a = a;
    int32_t b;
    readArrayGeneric(&b, 4, 1);
    ti.m_b = b;
    int32_t c;
    readArrayGeneric(&c, 4, 1);
    ti.m_c = c;
}

// Reads a count and that many vertices, then a count and that many triangles, each read in place at the array's end.
void hkDisplaySerializeIStream::readGeometry(hkGeometry& g) {
    int32_t numVertices;
    readArrayGeneric(&numVertices, 4, 1);
    if (numVertices > 0) {
        int32_t i = numVertices;
        do {
            readQuadVector4(*g.m_vertices.expandBy(1));
        }
        while (--i);
    }
    int32_t numTriangles;
    readArrayGeneric(&numTriangles, 4, 1);
    if (numTriangles > 0) {
        hkArray<hkGeometry::Triangle>& rTriangles = g.m_triangles;
        int32_t i = numTriangles;
        do {
            readTriangle(*rTriangles.expandBy(1));
        }
        while (--i);
    }
}

// Reads a type byte, then that shape's fields in stream order; the shape is created with its class's operator new (the
// heap's block, its size in m_memSizeAndFlags) and, for the shapes with a transform, given the transform read first. An
// unknown type leaves apGeometry alone. Floats are read raw.
void hkDisplaySerializeIStream::readDisplayGeometry(hkDisplayGeometry*& dg) {
    int8_t type;
    readArrayGeneric(&type, 1, 1);
    hkTransform transform;
    switch (type) {
        case 1: {
            readTransform(transform);
            float radius;
            readArrayGeneric(&radius, 4, 1);
            hkVector4 pos;
            readQuadVector4(pos);
            hkSphere sphere(pos);
            sphere.m_pos(3) = radius;
            alignas(16) int32_t xRes;
            readArrayGeneric(&xRes, 4, 1);
            int32_t yRes;
            readArrayGeneric(&yRes, 4, 1);
            hkDisplaySphere* pSphere = new hkDisplaySphere(sphere, xRes, yRes);
            dg = pSphere;
            pSphere->m_transform.Assign(transform);
            return;
        }
        case 2: {
            readTransform(transform);
            hkVector4 halfExtents;
            readQuadVector4(halfExtents);
            hkDisplayBox* pBox = new hkDisplayBox(halfExtents);
            dg = pBox;
            pBox->m_transform.Assign(transform);
            return;
        }
        case 3: {
            hkVector4 minExtent;
            readQuadVector4(minExtent);
            hkVector4 maxExtent;
            readQuadVector4(maxExtent);
            dg = new hkDisplayAABB(minExtent, maxExtent);
            return;
        }
        case 4: {
            hkVector4 startPos;
            readQuadVector4(startPos);
            hkVector4 coneAxis;
            readQuadVector4(coneAxis);
            float coneAngle;
            readArrayGeneric(&coneAngle, 4, 1);
            alignas(16) float coneHeight;
            readArrayGeneric(&coneHeight, 4, 1);
            int32_t numSegments;
            readArrayGeneric(&numSegments, 4, 1);
            dg = new hkDisplayCone(coneAngle, coneHeight, numSegments, coneAxis, startPos);
            return;
        }
        case 5: {
            hkVector4 center;
            readQuadVector4(center);
            hkVector4 normal;
            readQuadVector4(normal);
            hkVector4 startPerp;
            readQuadVector4(startPerp);
            alignas(16) float radius;
            readArrayGeneric(&radius, 4, 1);
            float thetaMin;
            readArrayGeneric(&thetaMin, 4, 1);
            float thetaMax;
            readArrayGeneric(&thetaMax, 4, 1);
            int32_t numSegments;
            readArrayGeneric(&numSegments, 4, 1);
            dg = new hkDisplaySemiCircle(center, normal, startPerp, thetaMin, thetaMax, radius, numSegments);
            return;
        }
        case 6: {
            readTransform(transform);
            hkGeometry* pGeom = new hkGeometry;
            readGeometry(*pGeom);
            hkDisplayConvex* pConvex = new hkDisplayConvex(pGeom);
            dg = pConvex;
            pConvex->m_transform.Assign(transform);
            return;
        }
        case 7: {
            hkVector4 center;
            readQuadVector4(center);
            hkVector4 normal;
            readQuadVector4(normal);
            hkVector4 perpToNormal;
            readQuadVector4(perpToNormal);
            float halfExtent;
            readArrayGeneric(&halfExtent, 4, 1);
            hkVector4 extent;
            extent.setAll(halfExtent);
            dg = new hkDisplayPlane(normal, perpToNormal, center, extent);
            return;
        }
        case 8: {
            readTransform(transform);
            float radius;
            readArrayGeneric(&radius, 4, 1);
            hkVector4 top;
            readQuadVector4(top);
            hkVector4 bottom;
            readQuadVector4(bottom);
            int32_t numSides;
            readArrayGeneric(&numSides, 4, 1);
            alignas(16) int32_t numHeightSegments;
            readArrayGeneric(&numHeightSegments, 4, 1);
            hkDisplayCapsule* pCapsule = new hkDisplayCapsule(top, bottom, radius, numSides, numHeightSegments);
            dg = pCapsule;
            pCapsule->m_transform.Assign(transform);
            return;
        }
        case 9: {
            readTransform(transform);
            float radius;
            readArrayGeneric(&radius, 4, 1);
            hkVector4 top;
            readQuadVector4(top);
            hkVector4 bottom;
            readQuadVector4(bottom);
            alignas(16) int32_t numSides;
            readArrayGeneric(&numSides, 4, 1);
            int32_t numHeightSegments;
            readArrayGeneric(&numHeightSegments, 4, 1);
            hkDisplayCylinder* pCylinder = new hkDisplayCylinder(top, bottom, radius, numSides, numHeightSegments);
            dg = pCylinder;
            pCylinder->m_transform.Assign(transform);
            return;
        }
        default: return;
    }
}
