#include "Gamebryo2.2/CoreLibs/NiCollision/NiOBBRoot.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiOBBLeaf.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiOBBNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"

// One triangle or none makes a single leaf; more build a node tree.
NiOBBRoot::NiOBBRoot(
    int32_t aiTriangles,
    const uint16_t* pTriList,
    const NiPoint3* pVertex,
    const NiPoint3* pWVertex,
    int32_t iBinSize
)
  : m_fDeltaTime(0.0f), m_pTree(nullptr), m_bTransformChanged(false), m_velocity(NiPoint3_ZERO), m_uiChangeStamp(0) {
    if (static_cast<uint16_t>(aiTriangles) <= 1u) {
        m_pTree = new NiOBBLeaf(this, pTriList, pVertex, pWVertex, 0);
    }
    else {
        m_pTree = new NiOBBNode(this, aiTriangles, pTriList, pVertex, pWVertex, iBinSize);
    }
    m_xform.MakeIdentity();
}

NiOBBRoot::~NiOBBRoot() {
    delete m_pTree;
}

bool NiOBBRoot::TestCollision(
    NiOBBRoot* pTestRoot,
    NiAVObject* pThisObj,
    NiAVObject* pOtherObj,
    int32_t iMaxDepth,
    int32_t iTestMaxDepth
) {
    if (this == pTestRoot) {
        return false;
    }
    else {
        return (bool)((int32_t)m_pTree->TestCollision(
            pTestRoot->m_pTree, pThisObj, pOtherObj, iMaxDepth, iTestMaxDepth, m_uiChangeStamp,
            pTestRoot->m_uiChangeStamp
        ));
    }
}

int32_t NiOBBRoot::FindCollisions(
    NiOBBRoot* pTestRoot,
    NiAVObject* pThisRoot,
    NiAVObject* pOtherRoot,
    NiAVObject* pThisObj,
    NiAVObject* pOtherObj,
    int32_t iMaxDepth,
    int32_t iTestMaxDepth,
    bool& bCollision
) {
    if (this == pTestRoot) {
        return 0;
    }
    else {
        return m_pTree->FindCollisions(
            pTestRoot->m_pTree, pThisRoot, pOtherRoot, pThisObj, pOtherObj, iMaxDepth, iTestMaxDepth, m_uiChangeStamp,
            pTestRoot->m_uiChangeStamp, bCollision
        );
    }
}

// Picks against the tree.
int32_t NiOBBRoot::FindIntersections(
    const NiPoint3& origin,
    const NiPoint3& dir,
    NiPick& pick,
    bool& bIntersects,
    NiAVObject* pObj,
    uint32_t uiChangeStamp
) {
    return m_pTree->FindIntersections(origin, dir, pick, bIntersects, pObj, uiChangeStamp);
}

void NiOBBRoot::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}
