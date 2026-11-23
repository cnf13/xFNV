#include "BSShader/BSClearZNode.h"

#include "BSMain/BSCullingProcess.h"
#include "BSShader/BSShaderAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

NiRTTI BSClearZNode::ms_RTTI("BSClearZNode", &NiNode::ms_RTTI);

BSClearZNode::BSClearZNode()
  : NiNode(0), bChangeFarClip(false), bChangeNearClip(false), fFarClip(283840.0f), fNearClip(100.0f) {}

// The destructor puts the clip defaults back before the node's.
BSClearZNode::~BSClearZNode() {
    fNearClip = 100.0f;
    fFarClip = 283840.0f;
    bChangeNearClip = false;
    bChangeFarClip = false;
}

const NiRTTI* BSClearZNode::GetRTTI() const {
    return &BSClearZNode::ms_RTTI;
}

NiObject* BSClearZNode::CreateClone(NiCloningProcess& arCloneData) {
    BSClearZNode* pClone = new BSClearZNode;
    NiNode::CopyMembers(pClone, arCloneData);
    return pClone;
}

void BSClearZNode::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiNode::GetViewerStrings(pkStrings);
}

// While the children are culled the accumulator collects nothing and is in LOD mode; afterwards it is back to
// normal accumulation.
void BSClearZNode::OnVisible(NiCullingProcess& arCuller) {
    BSShaderAccumulator* pkAccumulator = static_cast<BSShaderAccumulator*>(
        static_cast<BSCullingProcess&>(arCuller).spAccumulator.m_pObject
    );
    if (pkAccumulator) {
        pkAccumulator->bAccumulate = false;
        pkAccumulator->eAccumulationMode = BSShaderAccumulator::BSSA_ACCUMLOD;
    }
    NiNode::OnVisible(arCuller);
    if (pkAccumulator) {
        pkAccumulator->bAccumulate = true;
        pkAccumulator->eAccumulationMode = BSShaderAccumulator::BSSA_ACCUMNORMAL;
    }
}
