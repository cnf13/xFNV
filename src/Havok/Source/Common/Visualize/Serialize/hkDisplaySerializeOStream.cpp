#include "Havok/Source/Common/Visualize/Serialize/hkDisplaySerializeOStream.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/System/Io/OArchive/hkOArchive.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Buffered/hkBufferedStreamWriter.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayAABB.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayBox.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayCapsule.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayCone.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayCylinder.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometryTypes.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayPlane.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplaySemiCircle.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplaySphere.h"
#include "xfnv/Compiler.h"

hkDisplaySerializeOStream::hkDisplaySerializeOStream(hkArray<char, hkContainerHeapAllocator>& a1)
  : hkOArchive(a1, hkBool(false)) {}

// Passes a buffer of 8192 bytes rather than the default of 4096; the archive holds the only reference.
hkDisplaySerializeOStream::hkDisplaySerializeOStream(hkStreamWriter* a1)
  : hkOArchive(
        new (hkReferencedObject::newObject(sizeof(hkBufferedStreamWriter))) hkBufferedStreamWriter(a1, 8192),
        hkBool(false)
    ) {
    m_writer->removeReference();
}

hkDisplaySerializeOStream::~hkDisplaySerializeOStream() {}

void hkDisplaySerializeOStream::writeQuadVector4(const hkVector4& v) {
    writeArrayFloat32(&v(0), 3);
}

void hkDisplaySerializeOStream::writeTriangle(const hkGeometry::Triangle& ti) {
    write32(ti.m_a);
    write32(ti.m_b);
    write32(ti.m_c);
}

// The translation is copied and written from the copy as 3 floats (writeArrayFloat32).
void hkDisplaySerializeOStream::writeTransform(const hkTransform& t) {
    hkVector4 translation = t.m_translation;
    hkQuaternion v10;
    hkRotation r;
    r.m_col0 = t.m_rotation.m_col0;
    r.m_col1.m_quad = t.m_rotation.m_col1.m_quad;
    r.m_col2.m_quad = t.m_rotation.m_col2.m_quad;
    v10.set(r);
    // EXE-INLINED 0xDDA730
    writeArrayFloat32(&translation(0), 3);
    writeArrayFloat32(&v10.m_vec(0), 4);
}

void hkDisplaySerializeOStream::writeGeometry(const hkGeometry& g) {
    int32_t m_size = g.m_vertices.m_size;
    write32(m_size);
    if (m_size > 0) {
        int32_t v4 = 0;
        do {
            writeQuadVector4(g.m_vertices.m_data[v4++]);
            --m_size;
        }
        while (m_size);
    }
    int32_t m_size_1 = g.m_triangles.m_size;
    write32(m_size_1);
    if (m_size_1 > 0) {
        int32_t v6 = 0;
        do {
            writeTriangle(g.m_triangles.m_data[v6++]);
            --m_size_1;
        }
        while (m_size_1);
    }
}

void hkDisplaySerializeOStream::writeDisplayGeometry(hkDisplayGeometry* dg) {
    hkDisplayGeometryType m_type = dg->m_type;
    write8(m_type);
    switch (m_type) {
        case HK_DISPLAY_SPHERE: {
            hkDisplaySphere* pSphere = static_cast<hkDisplaySphere*>(dg);
            writeTransform(dg->m_transform);
            // Copies the sphere and writes its radius (w) and position from the copy.
            hkSphere sphere = pSphere->m_sphere;
            writeFloat32(sphere.m_pos(3));
            writeQuadVector4(sphere.m_pos);
            write32(pSphere->m_xRes);
            write32(pSphere->m_yRes);
            break;
        }
        case HK_DISPLAY_BOX: {
            writeTransform(dg->m_transform);
            writeQuadVector4(static_cast<hkDisplayBox*>(dg)->m_halfExtents);
            break;
        }
        case HK_DISPLAY_AABB: {
            const hkVector4* MinExtent = &static_cast<hkDisplayAABB*>(dg)->hkDisplayAABB::getMinExtent();
            writeQuadVector4(*MinExtent);
            const hkVector4* MaxExtent = &static_cast<hkDisplayAABB*>(dg)->hkDisplayAABB::getMaxExtent();
            writeQuadVector4(*MaxExtent);
            break;
        }
        case HK_DISPLAY_CONE: {
            writeQuadVector4(static_cast<hkDisplayCone*>(dg)->hkDisplayCone::getPosition());
            writeQuadVector4(static_cast<hkDisplayCone*>(dg)->hkDisplayCone::getAxis());
            float Angle = static_cast<hkDisplayCone*>(dg)->hkDisplayCone::getAngle();
            writeFloat32(Angle);
            float Height = static_cast<hkDisplayCone*>(dg)->hkDisplayCone::getHeight();
            writeFloat32(Height);
            int32_t NumSegments = static_cast<hkDisplayCone*>(dg)->hkDisplayCone::getNumSegments();
            write32(NumSegments);
            break;
        }
        case HK_DISPLAY_SEMICIRCLE: {
            writeQuadVector4(static_cast<hkDisplaySemiCircle*>(dg)->hkDisplaySemiCircle::getCenter());
            writeQuadVector4(static_cast<hkDisplaySemiCircle*>(dg)->hkDisplaySemiCircle::getNormal());
            writeQuadVector4(static_cast<hkDisplaySemiCircle*>(dg)->hkDisplaySemiCircle::getPerp());
            float a1b = static_cast<hkDisplaySemiCircle*>(dg)->hkDisplaySemiCircle::getRadius();
            writeFloat32(a1b);
            float a1c = static_cast<hkDisplaySemiCircle*>(dg)->hkDisplaySemiCircle::getThetaMin();
            writeFloat32(a1c);
            float a1d = static_cast<hkDisplaySemiCircle*>(dg)->hkDisplaySemiCircle::getThetaMax();
            writeFloat32(a1d);
            int32_t NumSegments_1 = static_cast<hkDisplaySemiCircle*>(dg)->hkDisplaySemiCircle::getNumSegments();
            write32(NumSegments_1);
            break;
        }
        case HK_DISPLAY_CONVEX: {
            writeTransform(dg->m_transform);
            writeGeometry(*dg->m_geometry);
            break;
        }
        case HK_DISPLAY_PLANE: {
            hkVector4* Center_1 = &static_cast<hkDisplayPlane*>(dg)->hkDisplayPlane::getCenter();
            writeQuadVector4(*Center_1);
            hkVector4* Normal_1 = &static_cast<hkDisplayPlane*>(dg)->hkDisplayPlane::getNormal();
            writeQuadVector4(*Normal_1);
            hkVector4* PerpToNormal = &static_cast<hkDisplayPlane*>(dg)->hkDisplayPlane::getPerpToNormal();
            writeQuadVector4(*PerpToNormal);
            hkVector4* Extents = &static_cast<hkDisplayPlane*>(dg)->hkDisplayPlane::getExtents();
            writeFloat32(XFNV_M128_F32(Extents->m_quad, 0));
            break;
        }
        case HK_DISPLAY_CAPSULE: {
            hkDisplayCapsule* pCapsule = static_cast<hkDisplayCapsule*>(dg);
            writeTransform(dg->m_transform);
            writeFloat32(pCapsule->m_radius);
            writeQuadVector4(pCapsule->m_top);
            writeQuadVector4(pCapsule->m_bottom);
            write32(pCapsule->m_numSides);
            write32(pCapsule->m_numHeightSegments);
            break;
        }
        case HK_DISPLAY_CYLINDER: {
            hkDisplayCylinder* pCylinder = static_cast<hkDisplayCylinder*>(dg);
            writeTransform(dg->m_transform);
            writeFloat32(pCylinder->m_radius);
            writeQuadVector4(pCylinder->m_top);
            writeQuadVector4(pCylinder->m_bottom);
            write32(pCylinder->m_numSides);
            write32(pCylinder->m_numHeightSegments);
            break;
        }
        default: {
            return;
        }
    }
}

uint32_t hkDisplaySerializeOStream::computeBytesRequired(const hkGeometry& a1) {
    return 12 * (a1.m_vertices.m_size + a1.m_triangles.m_size) + 8;
}

uint32_t hkDisplaySerializeOStream::computeBytesRequired(hkDisplayGeometry* a1) {
    uint32_t result = 1;
    switch (a1->m_type) {
        case HK_DISPLAY_SPHERE:
        case HK_DISPLAY_SEMICIRCLE: {
            result = 53;
            break;
        }
        case HK_DISPLAY_BOX:
        case HK_DISPLAY_PLANE: {
            result = 41;
            break;
        }
        case HK_DISPLAY_AABB: {
            result = 25;
            break;
        }
        case HK_DISPLAY_CONE: {
            result = 37;
            break;
        }
        case HK_DISPLAY_CONVEX: {
            result = hkDisplaySerializeOStream::computeBytesRequired(*a1->m_geometry) + 29;
            break;
        }
        case HK_DISPLAY_CAPSULE:
        case HK_DISPLAY_CYLINDER: {
            result = 65;
            break;
        }
        default: {
            return result;
        }
    }
    return result;
}
