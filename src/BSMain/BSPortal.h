#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSMain/BSOcclusionPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class BSCompoundFrustum;
class BSMultiBoundRoom;
class BSPortal;
class ShadowSceneLight;

// The node a portal's geometry hangs from: the compound frustum seen through the portal and the lights that shine
// through it.
class BSPortalSharedNode : public NiNode {
public:
    BSPortalSharedNode(BSPortal* apPortal);

    const NiRTTI* GetRTTI() const override {
        return &BSPortalSharedNode::ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void OnVisible(NiCullingProcess& kCuller) override;
    ~BSPortalSharedNode() override;

    BSCompoundFrustum* QCompoundFrustum();

protected:
    void CopyMembers(BSPortalSharedNode* pDest, NiCloningProcess& arCloneData);

public:
    BSCompoundFrustum* pCompoundFrustum;
    BSSimpleArray<ShadowSceneLight*, 1024> kLights;
    BSPortal* pPortal;

    static NiRTTI ms_RTTI;
};

NiSmartPointer(BSPortalSharedNode);

// An occlusion plane that joins two rooms; the geometry seen through it hangs from its shared node.
class BSPortal : public BSOcclusionPlane {
public:
    BSPortal();
    BSPortal(NiPoint3* apPosition, NiPoint2* apBounds, NiMatrix3* apMatrix);

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    ~BSPortal() override;

    // Set the rooms on either side: pMultiBoundRoom[0] and [1].
    void SetMultiBoundRoomLeft(BSMultiBoundRoom* apRoom);
    void SetMultiBoundRoomRight(BSMultiBoundRoom* apRoom);
    // spSharedGeometry = apNode.
    void SetSharedGeometryNode(BSPortalSharedNode* apNode);
    BSPortalSharedNode* QSharedGeometryNode();
    uint32_t AnyRoomsVisible();
    // The portal's members into apDest; not virtual.
    void CopyMembers(BSPortal* pDest, NiCloningProcess& arCloneData);

    // The two rooms the portal joins.
    BSMultiBoundRoom* pMultiBoundRoom[2];
    NiPointer<BSPortalSharedNode> spSharedGeometry;

    static NiRTTI ms_RTTI;
};

inline BSPortalSharedNode* BSPortal::QSharedGeometryNode() {
    return spSharedGeometry;
}

inline BSCompoundFrustum* BSPortalSharedNode::QCompoundFrustum() {
    return pCompoundFrustum;
}
