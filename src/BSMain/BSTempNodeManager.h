#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

struct NiUpdateData;

// A scene graph node with a time stamp (fTime), for the temporary nodes the game creates.
class BSTempNode : public NiNode {
public:
    BSTempNode(float afTime);
    ~BSTempNode() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }

    float fTime;

    static NiRTTI ms_RTTI;
};

// The node the temporary effects hang under: each update first removes the children whose time is up.
class BSTempNodeManager : public NiNode {
public:
    BSTempNodeManager();

    ~BSTempNodeManager() override;

    const NiRTTI* GetRTTI() const override;
    void UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
    void UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
    void UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;

protected:
    // Removes the children that expired by afTime.
    void RemoveExpired(float afTime);

public:
    float fPrevTime;

    static NiRTTI ms_RTTI;
};

inline BSTempNode::BSTempNode(float afTime) : NiNode(0), fTime(afTime) {}

inline BSTempNode::~BSTempNode() {}
