#include "Fallout Shared/BGSTextureUseMap.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderProperty.h"
#include "BSSystem/BSFile.h"
#include "Fallout Shared/BGSSceneInfo.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TEX_USER_DATA.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

BGSTextureUseMap::~BGSTextureUseMap() {
    ClearAllTexUseData();
}

void BGSTextureUseMap::CollectTexUseDataRecursive(NiAVObject* ap3D, bool abCheckActors) {
    if (ap3D && !ap3D->GetAppCulled()) {
        NiNode* pNode = ap3D->IsNode();
        if (pNode) {
            bool bIgnore = false;
            bool bSetRef = false;
            BSFadeNode* pFadeNode = pNode->IsFadeNode() ? static_cast<BSFadeNode*>(pNode) : nullptr;
            if (pFadeNode) {
                TESObjectREFR* pRef = pFadeNode->BSFadeNode::GetLinkedObj();
                if (BGSSceneInfo::pReferenceIgnoreMap) {
                    BGSSceneInfo::pReferenceIgnoreMap->GetAt(pRef, bIgnore);
                }
                if (pRef && !bIgnore) {
                    bool bIsActor = false;
                    int32_t iFormType = pRef->GetObjectReference()->GetFormType();
                    if (iFormType >= NPC__ID) {
                        bIsActor = iFormType <= LVLN_ID;
                    }
                    if ((bIsActor && !abCheckActors) || pRef->IsMarker()) {
                        return;
                    }
                    bSetRef = true;
                    pactiveref = pRef;
                }
            }
            if (!bIgnore) {
                uint32_t uiCount = pNode->GetArrayCount();
                for (uint32_t i = 0; i < uiCount; ++i) {
                    NiAVObject* pChild = pNode->GetAt(i);
                    CollectTexUseDataRecursive(pChild, abCheckActors);
                }
            }
            if (bSetRef) {
                pactiveref = nullptr;
            }
            return;
        }
        NiGeometry* pGeom = ap3D->IsGeometry();
        if (pGeom) {
            NiShadeProperty* pProp = static_cast<NiShadeProperty*>(pGeom->GetProperty(NiProperty::SHADE));
            BSShaderProperty* pBSSProp = nullptr;
            if (pProp && pProp->iShaderPropertyType != -1) {
                pBSSProp = static_cast<BSShaderProperty*>(pProp);
            }
            if (pBSSProp) {
                void* kParam[2];
                kParam[0] = this;
                TESObjectREFR* pUser;
                if (pactiveref) {
                    pUser = pactiveref;
                }
                else {
                    pUser = static_cast<TESObjectREFR*>(pDefaultUser);
                }
                kParam[1] = pUser;
                pBSSProp->GetTextureUse(&BGSTextureUseMap::TexUseCB, kParam);
            }
        }
    }
}

void BGSTextureUseMap::AddTexUse(NiTexture* apTex, uint32_t aulMemorySize, TESForm* apForm) {
    if (apTex && apForm) {
        TEX_USER_DATA* pData = nullptr;
        if (!UserMap.GetAt(apTex, pData) || !pData) {
            pData = new TEX_USER_DATA;
            UserMap.SetAt(apTex, pData);
            pData->spTexture = apTex;
            pData->ulTextureSize = aulMemorySize;
        }
        if (!pData->UserList.IsInList(apForm)) {
            pData->UserList.AddHead(apForm);
        }
    }
}

void BGSTextureUseMap::CollectTexUseData(NiNode* apNode, TESForm* apDefaultUser, bool abCheckActors) {
    pDefaultUser = apDefaultUser;
    CollectTexUseDataRecursive(apNode, abCheckActors);
}

void BGSTextureUseMap::CollectSceneTexUseData(bool abCheckActors) {
    if (TES::pInstance->TES::GetInterior()) {
        TESObjectCELL* pInterior = TES::pInstance->TES::GetInterior();
        CollectTexUseData(pInterior->Get3D(), pInterior, abCheckActors);
    }
    else {
        uint32_t uiCenter = uGridsToLoad.UInt() >> 1;
        uint32_t uiMax = uiCenter + 2;
        for (uint32_t i = uiCenter - 2; i <= uiMax; ++i) {
            for (uint32_t j = uiCenter - 2; j <= uiMax; ++j) {
                GridCell* pGridCell = TES::pInstance->GetGridCell(i, j);
                if (pGridCell->pCell) {
                    TESObjectCELL* pCell = pGridCell->pCell;
                    CollectTexUseData(pGridCell->pCell->Get3D(), pCell, abCheckActors);
                }
            }
        }
    }
}

uint32_t BGSTextureUseMap::GetTotalTextureSize() {
    uint32_t ulRetSize = 0;
    NiTMapItem<NiTexture*, TEX_USER_DATA*>* iter = UserMap.GetFirstPos();
    while (iter) {
        NiTexture* pIgnore = nullptr;
        TEX_USER_DATA* pData = nullptr;
        UserMap.GetNext(iter, pIgnore, pData);
        if (pData) {
            ulRetSize += pData->ulTextureSize;
        }
    }
    return ulRetSize;
}

void BGSTextureUseMap::ClearAllTexUseData() {
    NiTMapItem<NiTexture*, TEX_USER_DATA*>* iter = UserMap.GetFirstPos();
    while (iter) {
        NiTexture* pIgnore = nullptr;
        TEX_USER_DATA* pData = nullptr;
        UserMap.GetNext(iter, pIgnore, pData);
        if (pData) {
            delete pData;
        }
    }
    UserMap.RemoveAll();
}

bool BGSTextureUseMap::WriteToFile(const char* apFilename) {
    bool bRetVal = false;
    BSFile mapfile(apFilename, NiFile::WRITE_ONLY, 1024u, true);
    if (mapfile.BSFile::Open(false, false)) {
        bRetVal = true;
        NiTMapItem<NiTexture*, TEX_USER_DATA*>* iter = UserMap.GetFirstPos();
        while (iter) {
            NiTexture* pBaseTex = nullptr;
            TEX_USER_DATA* pData = nullptr;
            UserMap.GetNext(iter, pBaseTex, pData);
            NiSourceTexture* pTex = static_cast<NiSourceTexture*>(
                NiObject::DynamicCastStatic(&NiSourceTexture::ms_RTTI, pBaseTex)
            );
            if (pTex && pData) {
                BSStringT<char> outstring;
                int32_t iUsers = pData->UserList.ItemsInList();
                uint32_t ulTextureSize = pData->ulTextureSize;
                outstring.SPrintF(
                    "%s\t%d bytes\t%i users\r\n",
                    pTex->GetFilename().GetHandleString(),
                    ulTextureSize,
                    iUsers
                );
                mapfile.BSFile::Write(outstring, true);
                BSSimpleList<TESForm*>* pList = &pData->UserList;
                while (pList && !pList->IsEmpty()) {
                    TESForm* pUser = pList->m_item;
                    pList = pList->m_pkNext;
                    int32_t iFormID = pUser->iFormID;
                    const char* pcEditorID = pUser->GetFormEditorID();
                    outstring.SPrintF("%s\t%s\t%08X\r\n", pUser->GetFormTypeString(), pcEditorID, iFormID);
                    mapfile.BSFile::Write(outstring, true);
                }
                outstring.SPrintF("\r\n");
                mapfile.BSFile::Write(outstring, true);
            }
        }
        mapfile.Close();
    }
    return bRetVal;
}

// apParam is CollectTexUseData's two-pointer array: the map, then the default user form.
void BGSTextureUseMap::TexUseCB(NiTexture* apTexture, uint32_t aulMemorySize, void* apParam) {
    void** ppParam = static_cast<void**>(apParam);
    if (ppParam[0]) {
        static_cast<BGSTextureUseMap*>(ppParam[0])
            ->AddTexUse(apTexture, aulMemorySize, static_cast<TESForm*>(ppParam[1]));
    }
}

TEX_USER_DATA::TEX_USER_DATA() : spTexture(nullptr) {}

TEX_USER_DATA::~TEX_USER_DATA() {
    UserList.RemoveAll();
}
