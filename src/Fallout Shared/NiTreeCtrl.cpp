#include "Fallout Shared/NiTreeCtrl.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkBinaryAction.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkListShape.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkTransformShape.h"
#include "BSHavok/bhkWorldObject.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMultiTargetTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSequence.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDefaultAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <SDL3/SDL.h>

int32_t TreeCtrlCallback(NiObject* apObj, int32_t iMessage, uint32_t uiParam1, uint32_t uiParam2) {
    int32_t result = 0;
    switch (iMessage) {
        case 0: {
            return 1;
        }
        case 1: {
            return 1;
        }
        case 3: {
            if (apObj) {
                NiNode* v9 = static_cast<NiNode*>(apObj);
                switch (uiParam1) {
                    case 112u: {
                        v9->SetAppCulled(true);
                        return 1;
                    }
                    case 113u: {
                        v9->SetAppCulled(false);
                        return 1;
                    }
                    case 114u: {
                        v9->UpdateProperties();
                        NiUpdateData a2(0.0f, false, false);
                        v9->Update(a2);
                        return 1;
                    }
                    case 122u: {
                        char pcFileName[264];
                        NiStream stream;
                        stream.InsertObject(v9);
                        const char* name = (&v9->m_kName)->GetHandleString();
                        if (!name) {
                            name = "NullObject";
                        }
                        BSstrcpy(pcFileName, 260, "c:\\");
                        BSstrcat(pcFileName, 260, name);
                        BSstrcat(pcFileName, 260, ".nif");
                        stream.NiStream::Save(pcFileName);
                        result = 1;
                        break;
                    }
                }
            }
            break;
        }
    }
    return result;
}

// pName is copied into memory from ourMalloc.
NiTreeCtrl::NiTreeCtrl(
    void* hInstance,
    void* hParentWnd,
    NiAVObject* pObject,
    const char* pName,
    int32_t iXPos,
    int32_t iYPos,
    int32_t iWidth,
    int32_t iHeight
)
  : m_hInstance(hInstance), m_hParentWnd(hParentWnd), TreeItemMap(37), m_pObject(pObject),
    m_pName(static_cast<char*>(ourMalloc(BSstrlen(pName) + 1))) {
    ourStrcpy(m_pName, pName);
    CreateWindows(iXPos, iYPos, iWidth, iHeight);
    SetImageList();
    if (pObject) {
        CreateTree(pObject, nullptr);
    }
}

NiTreeCtrl::~NiTreeCtrl() {
    ourFree(m_pName);
    DestroyTree();
    DestroyImageList();
    DestroyWindows();
    pLastViewerStringsControllerManager[1] = nullptr;
    pLastViewerStringsControllerManager[0] = pLastViewerStringsControllerManager[1];
}

// The tree window was a Win32 window with a common-controls tree view (RegisterClassA, CreateWindowExA of a
// SysTreeView32, an image list, TVM_* messages), which has no SDL form: the window is not created (logged), and the
// tree is not filled.
void* NiTreeCtrl::CreateWindows(
    [[maybe_unused]] int32_t X,
    [[maybe_unused]] int32_t Y,
    [[maybe_unused]] int32_t nWidth,
    [[maybe_unused]] int32_t nHeight
) {
    SDL_Log("NiTreeCtrl \"%s\": the debug tree window is not available", m_pName);
    m_hTreeWnd = nullptr;
    m_hTreeView = nullptr;
    return nullptr;
}

void NiTreeCtrl::SetImageList() {
    m_hImageList = nullptr;
}

void NiTreeCtrl::CreateTree([[maybe_unused]] NiAVObject* apObject, [[maybe_unused]] const char* apName) {
    TreeItemMap.RemoveAll();
}

// No window to destroy (CreateWindows).
void NiTreeCtrl::DestroyWindows() {}

// There is no tree (CreateWindows): nothing to select, destroy or empty. The results are those of the Win32 calls on
// null handles (FALSE, 0).
void NiTreeCtrl::BuildTree([[maybe_unused]] NiNode* a1) {}

int32_t NiTreeCtrl::DestroyImageList() {
    return 0;
}

int32_t NiTreeCtrl::DestroyTree() {
    return 0;
}

void NiTreeCtrl::SetCallback(CallbackFunction* apCallback) {
    NiTreeCtrl::stpCallback = apCallback;
}
