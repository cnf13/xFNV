#pragma once

#include "BSMain/BSMultiBoundShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class NiLines;
class NiTriShape;

// An occlusion plane, linked to up to four neighbouring planes along its edges.
class BSOcclusionPlane : public NiObject {
public:
    // The edges a neighbouring plane can be linked on.
    enum BSOP_LINK : int32_t {
        BSOP_LINK_XNEG = 0,
        BSOP_LINK_XPOS = 1,
        BSOP_LINK_ZNEG = 2,
        BSOP_LINK_ZPOS = 3,
        BSOP_LINK_COUNT = 4,
    };

    // How a plane is joined to its neighbour.
    enum BSOP_LINKTYPE : int32_t {
        BSOP_LINKTYPE_NONE = 0,
        BSOP_LINKTYPE_PARENT = 1,
        BSOP_LINKTYPE_CHILD = 2,
    };

    BSOcclusionPlane();
    BSOcclusionPlane(NiPoint3* apCenter, NiPoint2* apHalfExtents, NiMatrix3* apRotation);
    ~BSOcclusionPlane() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess&) override;
    virtual int32_t QDebugGeomLineSize();
    virtual int32_t QDebugGeomShapeSize();

    // Writes the four corner points of the plane to apPoints.
    void GetVertices(NiPoint3* apPoints);
    // Builds the frustum planes from akViewPoint through the plane's edges into apPlanes.
    void GetFrustumPlanes(NiPoint3 akViewPoint, NiFrustumPlanes& akPlanes);
    void SetLinkedPlane(BSOP_LINK akConnection, BSOcclusionPlane* apPlane);
    // Whether the plane lies within aPlanes.
    bool WithinFrustum(const NiFrustumPlanes& aPlanes, bool abViewHitPlane, NiPoint3 akViewIntersection);
    // Whether akViewPoint is on the plane's occluding side.
    bool ViewerOnCorrectSide(NiPoint3 akViewPoint);
    // Whether the plane lies within aPlanes, testing its distance from aViewPos first.
    bool WithinFrustumDistFirst(const NiFrustumPlanes& aPlanes, NiPoint3& aViewPos);
    // One bit per edge whose visibility flag is clear (0xF: all four).
    int32_t QEdgeVisibleState();
    // Set the plane's centre, half extents and rotation.
    void SetCenter(const NiPoint3& arCenter);
    void SetHalfExtents(const NiPoint2& arHalfExtents);
    void SetRotation(const NiMatrix3& arRotation);
    // The plane's rotation, returned by value.
    const NiMatrix3 QRotation() const;
    // Whether aBound intersects the plane.
    bool TestIntersection(const NiBound& aBound);
    // Marks the plane untested for this frame's culling.
    void ResetCullResult();
    BSOcclusionPlane::BSOP_LINKTYPE GetJoinedPlaneType();
    bool TestLineSegmentAllPlanes(
        NiPoint3& akPoint0,
        NiPoint3& akPoint1,
        const NiPlane& akPlane0,
        const NiFrustumPlanes& akPlanes
    );
    // The plane's members into apDest; not virtual.
    void CopyMembers(BSOcclusionPlane* pDest, NiCloningProcess& arCloneData);
    bool WithinFrustumFullTest(const NiFrustumPlanes& aPlanes);
    bool CompletelyWithinFrustum(const NiFrustumPlanes& aPlanes);
    void MakeDebugGeom(
        NiPoint3 akViewPoint,
        NiLines* apLines,
        NiTriShape* apShape,
        NiColorA akBaseColor,
        bool abWithinFrustum
    );

    // Whether the segment a1-a2 crosses the plane a3 at a point on the inner side of the planes a4 and a5.
    static bool TestLineSegment(NiPoint3* a1, NiPoint3* a2, float* a3, NiPoint3* a4, NiPoint3* a5);

    NiPoint3 kCenter;
    NiPoint2 kHalfExtents;
    NiMatrix3 kRotation;
    BSMultiBoundShape::BSMBCullResult kCullResult;
    NiFrustumPlanes kCachedPlanes;
    NiPoint3 kCachedVertices[4];
    NiPoint3 kCachedViewPoint;
    bool bDirtyVertices;
    bool bDirtyPlanes;
    bool bDisabledPlanes[4];
    bool bTwoSided;
    uint8_t paddingEB;
    BSOcclusionPlane* pLinkedPlanes[4];

    static NiRTTI ms_RTTI;
};
