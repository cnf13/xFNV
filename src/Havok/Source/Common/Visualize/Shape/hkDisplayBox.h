#pragma once

#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"

// A box the visual debugger displays: half extents about its transform. Only some of its members are declared.
class hkDisplayBox : public hkDisplayGeometry {
public:
    hkDisplayBox();
    hkDisplayBox(const hkVector4& halfExtents);
    ~hkDisplayBox() override;

    // The box's twelve edges as 24 line end points in world space.
    void getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) override;
    // Builds the box's triangle mesh into m_geometry.
    void buildGeometry() override;

    void setParameters(const hkVector4& halfExtents, const hkTransform& t);

    hkVector4 m_halfExtents;
};
