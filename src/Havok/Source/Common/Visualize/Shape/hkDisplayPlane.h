#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"

// Builds the visual debugger geometry for a plane.
class hkDisplayPlane : public hkDisplayGeometry {
public:
    hkDisplayPlane();
    hkDisplayPlane(
        const hkVector4& normal,
        const hkVector4& perpToNormal,
        const hkVector4& center,
        const hkVector4& extent
    );

    void getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) override;
    void buildGeometry() override;

    using hkDisplayGeometry::getWireframeGeometry;

    // Stores the plane's normal, in-plane direction, centre and extent.
    void setParameters(
        const hkVector4& normal,
        const hkVector4& perpToNormal,
        const hkVector4& center,
        const hkVector4& extent
    );
    hkVector4& getNormal();
    hkVector4& getCenter();
    hkVector4& getPerpToNormal();
    hkVector4& getExtents();
    void createPlanePoints(hkVector4* points);

    hkVector4 m_normal;
    hkVector4 m_center;
    hkVector4 m_perpToNormal;
    hkVector4 m_extent;
};
