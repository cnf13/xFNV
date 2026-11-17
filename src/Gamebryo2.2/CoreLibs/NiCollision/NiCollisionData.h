#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiCollisionGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

class NiAVObject;
class NiBoundingVolume;
class NiOBBRoot;

NiAVObject* FindCollisionAncestor(NiAVObject* a1);

// Gamebryo's own collision representation: velocities, a bounding volume or an OBB tree, and a collide callback
class NiCollisionData : public NiCollisionObject {
public:
    enum PropagationMode : int32_t {
        PROPAGATE_ON_SUCCESS = 0,
        PROPAGATE_ON_FAILURE = 1,
        PROPAGATE_ALWAYS = 2,
        PROPAGATE_NEVER = 3,
        PROPAGATIONMODE_POS = 4,
    };

    enum CollisionMode : int32_t {
        USE_OBB = 0,
        USE_TRI = 1,
        USE_ABV = 2,
        NOTEST = 3,
        USE_NIBOUND = 4,
        COLLISIONMODE_POS = 5,
    };

    enum CollisionTest : int32_t {
        OBB_OBB = 0,
        OBB_TRI = 1,
        OBB_ABV = 2,
        OBB_NIBOUND = 3,
        TRI_TRI = 4,
        TRI_ABV = 5,
        TRI_NIBOUND = 6,
        ABV_ABV = 7,
        ABV_NIBOUND = 8,
        NIBOUND_NIBOUND = 9,
        NOTEST_NOTEST = 10,
    };

    NiCollisionData();
    ~NiCollisionData() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void AddViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void SetSceneGraphObject(NiAVObject* pkSceneObject) override;
    void UpdateWorldData(NiUpdateData& UpdateData) override;
    // shared empty stub
    void RecreateWorldData() override {}
    void Initialize(void* apData) override;
    void Convert(uint32_t uiVersion, void* pkData) override;
    virtual NiPoint3 GetVelocity();

    // Replaces both volumes (deleting the old ones) by apABV and a world copy of it; null clears them.
    void SetModelSpaceABV(NiBoundingVolume* pkBV);
    // Sets the mode; the vertex and triangle modes make the geometry keep its data on the CPU.
    void SetCollisionMode(CollisionMode eCollision);
    void CreateWorldVertices();
    void UpdateWorldVertices();
    void CreateOBB(int32_t iBinSize);
    // "<prefix> = <mode name>" in a fresh _NiMalloc block.
    char* GetViewerString(const char* apcPrefix, PropagationMode aeMode) const;
    char* GetViewerString(const char* apcPrefix, CollisionMode aeMode) const;

protected:
    void CopyMembers(NiCollisionData* pkDest, NiCloningProcess& kCloning);

public:
    static NiObject* CreateObject();

    NiPoint3 m_kLocalVelocity;
    NiPoint3 m_kWorldVelocity;
    PropagationMode m_ePropagationMode;
    CollisionMode m_eCollisionMode;
    NiBoundingVolume* m_pkModelABV;
    NiBoundingVolume* m_pkWorldABV;
    NiOBBRoot* m_pkOBBRoot;
    NiCollisionGroup::Callback m_pfnCollideCallback;
    void* m_pvCollideCallbackData;
    NiPoint3* m_pkWorldVertex;
    NiPoint3* m_pkWorldNormal;
    bool m_bWorldVerticesNeedUpdate;
    bool m_bWorldNormalsNeedUpdate;
    uint16_t m_usNumVertices;
    uint16_t m_usNumTriangles;
    bool m_bAuxCallbacks;

    static NiRTTI ms_RTTI;
    // whether UpdateWorldData derives the world velocity from the local one
    static inline bool ms_bComputeWorldVelocity;
};
