#pragma once

#include "BSMain/Setting.h"
#include "BSShader/BSFadeNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

class bhkRigidBody;
class bhkSimpleShapePhantom;
class BSTreeModel;
class CSpeedTreeRT;
class NiTexturingProperty;
class NiTriBasedGeom;

// A scene graph node the SpeedTree forest renders through.
// its C++ base is BSFadeNode, but its NiRTTI record chains to NiNode's (see RegisterRTTI)
class BSTreeNode : public BSFadeNode {
public:
    BSTreeNode(BSTreeModel* apModel, bhkRigidBody* apRigid);
    ~BSTreeNode() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
    void UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
    void OnVisible(NiCullingProcess& arCuller) override;
    virtual BSTreeModel* GetModel() const;
    virtual NiNode* GetBranchNode();
    virtual NiNode* GetLeafNode();
    virtual NiNode* GetBillboardNode();
    virtual NiTriBasedGeom* GetBranchLevel(uint16_t asLevel);
    virtual NiTriBasedGeom* GetLeafLevel(uint16_t asLevel);
    virtual NiTriBasedGeom* GetBillboard();
    virtual bool AddBranchLevel(uint16_t asLevel, NiTriBasedGeom* apGeom);
    virtual bool AddLeafLevel(uint16_t asLevel, NiTriBasedGeom* apGeom);
    virtual void SetBillboard(NiTriBasedGeom* apGeom);
    virtual bool SetVisibilityForBranches(bool abVisible);
    virtual bool SetVisibilityForLeaves(bool abVisible);
    virtual bool SetVisibilityForBillboards(bool abVisible);
    virtual bool SetVisibilityOnBranch(uint16_t asLevel, uint8_t acTest);
    virtual bool SetVisibilityOnLeaf(uint16_t asLevel, uint8_t acTest);
    virtual bool SetVisibilityOnBillboard(uint8_t acTest);
    virtual bool SetTexturePropOnBillboard(NiTexturingProperty* apProp);
    virtual void UpdateBSTreeNode(float afTime);

    bool CheckForCollision(float afTime);
    // Gives the leaf node a Havok phantom shaped by apST's collision objects, scaled by afScale.
    void MakeHavokLeaves(CSpeedTreeRT* apST, float afScale);
    bhkSimpleShapePhantom* GetLeafObject();

    NiPointer<BSTreeModel> spModel;
    NiPointer<NiTriBasedGeom>* pspBranches;
    NiPointer<NiTriBasedGeom>* pspLeaves;
    NiPointer<NiTriBasedGeom> spBillboard;
    float fNextCollisionCheck;

    static inline uint8_t ucNextGroup;

    static NiRTTI ms_RTTI;
};

// The seconds between two collision checks of a tree node.
extern SettingT<GameSettingCollection> fUpdateInterval;

// "iLowDamp", default 50.
extern SettingT<GameSettingCollection> iLowDamp;

extern SettingT<GameSettingCollection> gs_iHighDamp;

extern SettingT<GameSettingCollection> gs_iCollFreq;
