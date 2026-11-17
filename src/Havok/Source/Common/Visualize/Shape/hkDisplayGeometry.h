#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometryTypes.h"

class hkGeometry;

// A shape the visual debugger displays: its triangle mesh (built on demand), its wireframe and its transform. It
// overrides the destructor and adds two pure virtual functions.
class hkDisplayGeometry : public hkReferencedObject {
public:
    hkDisplayGeometry(hkDisplayGeometryType type);
    ~hkDisplayGeometry() override;

    virtual void getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) = 0;
    virtual void buildGeometry() = 0;

    // The SDK's inline overload that gets the array's allocator and calls the virtual one.
    void getWireframeGeometry(hkArray<hkVector4>& lines);
    hkGeometry* getGeometry();
    const hkTransform& getTransform() const;
    hkTransform& getTransform();
    hkDisplayGeometryType getType() const;

    hkGeometry* m_geometry;
    hkTransform m_transform;
    hkDisplayGeometryType m_type;
};
