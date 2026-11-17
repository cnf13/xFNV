#include "Fallout Shared/TESModelTextureSwap.h"

#include "BSCore/BSMap.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/MessageHandler.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "Fallout Shared/BGSTextureSet.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/rtti.h"

TEX_SWAP::TEX_SWAP() : pNewTexture(nullptr), iGeomIndex(-1) {
    pGeomName[0] = 0;
}

TESModelTextureSwap::TESModelTextureSwap() : TESModel(), TextureSwapList() {
    TESModelTextureSwap::InitializeDataComponent();
}

TESModelTextureSwap::~TESModelTextureSwap() {
    TESModelTextureSwap::ClearDataComponent();
}

void TESModelTextureSwap::InitializeDataComponent() {
    TESModel::InitializeDataComponent();
}

void TESModelTextureSwap::ClearDataComponent() {
    ClearTexSwapList();
    TESModel::ClearDataComponent();
}

void TESModelTextureSwap::CopyComponent(BaseFormComponent* apCopyFC) {
    TESModel::CopyComponent(apCopyFC);
    TESModelTextureSwap* pcopy = dynamic_cast<TESModelTextureSwap*>(apCopyFC);
    if (pcopy) {
        ClearTexSwapList();
        for (BSSimpleList<TEX_SWAP*>* plist = &pcopy->TextureSwapList; plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            TEX_SWAP* m_item = plist->m_item;
            AddTexSwap(m_item->pGeomName, m_item->iGeomIndex, m_item->pNewTexture);
        }
    }
}

bool TESModelTextureSwap::CompareComponent(BaseFormComponent* apCompareFC) {
    if (TESModel::CompareComponent(apCompareFC)) {
        return true;
    }
    TESModelTextureSwap* pcomp = dynamic_cast<TESModelTextureSwap*>(apCompareFC);
    if (!pcomp) {
        return true;
    }
    int32_t v3 = (&pcomp->TextureSwapList)->ItemsInList();
    if (v3 != (&TextureSwapList)->ItemsInList()) {
        return true;
    }
    for (BSSimpleList<TEX_SWAP*>* plist = &pcomp->TextureSwapList; plist && !plist->IsEmpty();
         plist = plist->m_pkNext) {
        TEX_SWAP** v4 = &plist->m_item;
        TEX_SWAP* pcompswap = *v4;
        TEX_SWAP* pswap = GetTexSwap((*v4)->iGeomIndex);
        if (!pswap) {
            return true;
        }
        if (pswap->pNewTexture != pcompswap->pNewTexture) {
            return true;
        }
        if (pswap->pGeomName[0u] && (pcompswap->pGeomName[0u] && BSstricmp(pswap->pGeomName, pcompswap->pGeomName))) {
            return true;
        }
    }
    return false;
}

// The chunk is a packed byte stream: the count, then per swap the name's length, the name, the texture set's form ID
// and the geometry index, each 32-bit value stored unaligned. The second endian swap deliberately swaps the count's
// local, not the name's length it stores, as the game does.
bool TESModelTextureSwap::BuildTextureSwapChunkData(void*& apOutData, uint32_t& aiOutDataSize) {
    apOutData = nullptr;
    aiOutDataSize = 0;
    BSSimpleList<TEX_SWAP*>* plist = &TextureSwapList;
    if (!plist->IsEmpty()) {
        uint32_t icount = 0;
        aiOutDataSize = 4;
        while (plist && !plist->IsEmpty()) {
            TEX_SWAP* pswap = plist->m_item;
            aiOutDataSize += 4;
            aiOutDataSize += BSstrlen(pswap->pGeomName);
            aiOutDataSize += 4;
            aiOutDataSize += 4;
            ++icount;
            plist = plist->m_pkNext;
        }
        apOutData = ourMalloc(aiOutDataSize);
        uint8_t* pdata = static_cast<uint8_t*>(apOutData);
        uint32_t itempint = icount;
        if (TESForm::QEndianSwapOnSave()) {
            ENDIAN<uint32_t>(itempint, false);
        }
        *reinterpret_cast<uint32_t*>(pdata) = itempint;
        pdata += 4;
        for (plist = &TextureSwapList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
            TEX_SWAP* pswap_2 = plist->m_item;
            uint32_t isize = BSstrlen(pswap_2->pGeomName);
            if (TESForm::QEndianSwapOnSave()) {
                ENDIAN<uint32_t>(itempint, false);
            }
            *reinterpret_cast<uint32_t*>(pdata) = isize;
            pdata += 4;
            ourMemcpy(pdata, pswap_2->pGeomName, isize);
            pdata += isize;
            uint32_t iid = 0;
            if (pswap_2->pNewTexture) {
                iid = pswap_2->pNewTexture->iFormID;
            }
            *reinterpret_cast<uint32_t*>(pdata) = iid;
            pdata += 4;
            *reinterpret_cast<int32_t*>(pdata) = pswap_2->iGeomIndex;
            pdata += 4;
        }
    }
    return apOutData != nullptr;
}

void TESModelTextureSwap::Save(CHUNK_ID aeModelID, CHUNK_ID aeTextureID, CHUNK_ID aeSwapID) {
    TESModel::Save(aeModelID, aeTextureID);
    void* Block = nullptr;
    uint32_t aiOutDataSize = 0;
    if (BuildTextureSwapChunkData(Block, aiOutDataSize)) {
        TESForm::__AddChunkData(aeSwapID, Block, aiOutDataSize);
        ourFree(Block);
    }
}

void TESModelTextureSwap::InitItem(TESForm* apOwner) {
    BSSimpleList<TEX_SWAP*>* plist = &TextureSwapList;
    BSSimpleList<TEX_SWAP*>* plastlist = nullptr;
    while (plist && !plist->IsEmpty()) {
        TEX_SWAP* pswap = plist->m_item;
        uint32_t iid = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pswap->pNewTexture));
        TESFile* File = apOwner->GetFile(-1);
        TESForm::AddCompileIndex(iid, File);
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(iid);
        BGSTextureSet* v4 = dynamic_cast<BGSTextureSet*>(FormByNumericID);
        pswap->pNewTexture = v4;
        if (pswap->pNewTexture) {
            plastlist = plist;
            plist = plist->m_pkNext;
        }
        else {
            int32_t CastingType = apOwner->iFormID;
            const char* v5 = apOwner->GetFormEditorID();
            Warning(
                "MASTERFILE: Failed to find texture set (%08X) in model texture swap init for form '%s' (%08X).", iid,
                v5, CastingType
            );
            if (plastlist) {
                plastlist->Remove(pswap);
                plist = plastlist->m_pkNext;
            }
            else {
                plist->RemoveHead();
            }
            ourFree(pswap);
        }
    }
}

// apData is the chunk's packed byte stream (BuildTextureSwapChunkData's layout).
bool TESModelTextureSwap::UnpackTextureSwapChunkData(void* apData, uint32_t aiDataSize, uint32_t aiVersion) {
    if (apData && aiDataSize > 4) {
        char* pdata = static_cast<char*>(apData);
        uint32_t icount = *reinterpret_cast<uint32_t*>(pdata);
        uint32_t ireadsize = 4;
        pdata += ireadsize;
        uint32_t auiSize = aiDataSize - ireadsize;
        while (icount) {
            char apDest[268];
            --icount;
            BGSTextureSet* pnewtexture = nullptr;
            int32_t igeomindex = -1;
            apDest[0] = 0;
            ireadsize = 4;
            if (ireadsize > auiSize) {
                return false;
            }
            uint32_t isize = *reinterpret_cast<uint32_t*>(pdata);
            pdata += ireadsize;
            auiSize -= ireadsize;
            ireadsize = isize;
            if (ireadsize > auiSize || ireadsize >= 260) {
                return false;
            }
            ourMemcpy(apDest, pdata, isize);
            apDest[isize] = 0;
            pdata += ireadsize;
            auiSize -= ireadsize;
            ireadsize = 4;
            if (ireadsize > auiSize) {
                return false;
            }
            // The stream holds the texture set's form ID, kept in the pointer until InitItem resolves it.
            BGSTextureSet* v6 =
                reinterpret_cast<BGSTextureSet*>(static_cast<uintptr_t>(*reinterpret_cast<uint32_t*>(pdata)));
            pdata += ireadsize;
            auiSize -= ireadsize;
            pnewtexture = v6;
            if (aiVersion >= 15) {
                ireadsize = 4;
                if (ireadsize > auiSize) {
                    return false;
                }
                igeomindex = *reinterpret_cast<int32_t*>(pdata);
                pdata += ireadsize;
                auiSize -= ireadsize;
            }
            if (apDest[0]) {
                if (pnewtexture) {
                    AddTexSwap(apDest, igeomindex, pnewtexture);
                }
            }
        }
    }
    return true;
}

void TESModelTextureSwap::ClearTexSwapList() {
    while (!TextureSwapList.IsEmpty()) {
        TEX_SWAP* pTexture = TextureSwapList.GetItem();
        TextureSwapList.RemoveHead();
        ourFree(pTexture);
    }
}

void TESModelTextureSwap::SwapTextures(NiAVObject* ap3D) {
    if (ap3D) {
        int32_t iwarncount = MessageHandler::QWarningCount();
        MessageHandler::IncDisableWarningCount(true);
        using swapmap_t = BSMap<int32_t, TEX_SWAP*>;
        swapmap_t swapmap(37u);
        BSSimpleList<TEX_SWAP*>* plist = &TextureSwapList;
        while (plist && !plist->IsEmpty()) {
            TEX_SWAP* pswap = plist->m_item;
            plist = plist->m_pkNext;
            if (pswap->iGeomIndex == -1) {
                Warning("Found invalid swap index.");
            }
            else {
                swapmap.SetAt(pswap->iGeomIndex, pswap);
            }
        }
        if (swapmap.m_uiCount) {
            int32_t iindex = 0;
            TESModelTextureSwap::SwapTexturesRecurse(ap3D, &swapmap, iindex);
            if (swapmap.m_uiCount) {
                Warning("Not all texture swaps were performed.");
            }
        }
        MessageHandler::IncDisableWarningCount(false);
        if (iwarncount != MessageHandler::QWarningCount()) {
            Warning(
                "Warnings were encountered during texture swap for model '%s'. Check warnings file for more "
                "details.",
                GetModel()
            );
        }
    }
}

TEX_SWAP* TESModelTextureSwap::GetTexSwap(int32_t aiIndex) {
    TEX_SWAP* pretswap = nullptr;
    if (aiIndex != -1) {
        for (BSSimpleList<TEX_SWAP*>* plist = &TextureSwapList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
            if (aiIndex == plist->m_item->iGeomIndex) {
                return plist->m_item;
            }
        }
    }
    return pretswap;
}

void TESModelTextureSwap::AddTexSwap(const char* apObjName, int32_t aiIndex, BGSTextureSet* apTexSet) {
    if (apObjName) {
        if (apTexSet) {
            TEX_SWAP* a2 = GetTexSwap(aiIndex);
            if (!a2) {
                TEX_SWAP* v4;
                TEX_SWAP* apMemory = static_cast<TEX_SWAP*>(ourMalloc(sizeof(TEX_SWAP)));
                if (apMemory) {
                    v4 = ::new (apMemory) TEX_SWAP;
                }
                else {
                    v4 = nullptr;
                }
                a2 = v4;
                BSstrcpy(v4->pGeomName, 128, apObjName);
                v4->iGeomIndex = aiIndex;
                TextureSwapList.AddHead(a2);
            }
            a2->pNewTexture = apTexSet;
        }
    }
}

// A texture swap chunk's data is a run of swap records, unpacked with the file's form version; any other model chunk
// goes to TESModel::LoadModelChunk. The chunk is not consumed past its own data.
// The alloca'd buffer is passed with the chunk's own size, which GetChunkData writes in full (an empty chunk passes
// size 0 on), so no slot is read uninitialised.
void TESModelTextureSwap::LoadModelTextureSwapChunk(TESModelTextureSwap* apModel, TESFile* apFile) {
    if (!apFile || !apModel) {
        return;
    }
    uint32_t uiChunk = apFile->GetTESChunk();
    switch (uiChunk) {
        case MO2S_ID:
        case MO3S_ID:
        case MO4S_ID:
        case MODS_ID: {
            uint32_t uiChunkSize = apFile->GetChunkSize();
            // A 16-byte aligned stack buffer of the chunk's size.
            char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
            apFile->GetChunkData(pBuffer, uiChunkSize);
            uint16_t usFormVersion = apFile->GetFormVersion();
            apModel->UnpackTextureSwapChunkData(pBuffer, uiChunkSize, usFormVersion);
            break;
        }
        default:
            if (TESModel::IsModelChunk(static_cast<CHUNK_ID>(uiChunk))) {
                TESModel::LoadModelChunk(apModel, apFile);
            }
            break;
    }
}

bool TESModelTextureSwap::HasTextureSwap(TESForm* apForm, TESObjectREFR* apRequester) {
    bool bHasSwap = false;
    TESModelTextureSwap* pModel = TESBipedModelForm::GetWorldTESModel(apForm, apRequester);
    if (!pModel) {
        pModel = dynamic_cast<TESModelTextureSwap*>(apForm);
    }
    if (pModel && !pModel->GetTexSwapList()->IsEmpty()) {
        bHasSwap = true;
    }
    return bHasSwap;
}

bool TESModelTextureSwap::SwapTexturesRecurse(
    NiAVObject* ap3D,
    BSMap<int32_t, TEX_SWAP*>* apSwapMap,
    int32_t& aiGeomIndex
) {
    if (ap3D && apSwapMap) {
        NiNode* pnode = ap3D->IsNode();
        if (pnode) {
            uint32_t icount = pnode->GetArrayCount();
            for (uint32_t i = 0; i < icount; ++i) {
                NiAVObject* pchild = pnode->GetAt(i);
                if (pchild && TESModelTextureSwap::SwapTexturesRecurse(pchild, apSwapMap, aiGeomIndex)) {
                    return true;
                }
            }
        }
        else {
            if (ap3D->IsGeometry()) {
                TEX_SWAP* pswap = nullptr;
                if (apSwapMap->GetAt(aiGeomIndex, pswap)) {
                    if (pswap) {
                        if (pswap->pNewTexture == BGSTextureSet::GetNullTextureSet()) {
                            ap3D->SetAppCulled(true);
                        }
                        else {
                            int32_t v5;
                            NiShadeProperty* pprop = static_cast<NiShadeProperty*>(ap3D->GetProperty(3));
                            if (pprop) {
                                int32_t bPPLightingProp =
                                    pprop->iShaderPropertyType >= 8 && pprop->iShaderPropertyType <= 12;
                                v5 = bPPLightingProp;
                            }
                            else {
                                v5 = 0;
                            }
                            BSShaderPPLightingProperty* pbssprop =
                                v5 ? static_cast<BSShaderPPLightingProperty*>(pprop) : nullptr;
                            if (pbssprop) {
                                BSShaderTextureSet* pnewset = pswap->pNewTexture->CreateShaderTextureSet();
                                pbssprop->SetTextureSet(pnewset);
                                pbssprop->LoadTextureSet(nullptr, nullptr);
                                pbssprop->SetFlagsFromTextures();
                            }
                        }
                        apSwapMap->RemoveAt(aiGeomIndex);
                        if (!apSwapMap->m_uiCount) {
                            return true;
                        }
                    }
                }
                ++aiGeomIndex;
            }
        }
    }
    return false;
}

bool NiFixedString::EqualsNoCase(const char* pcStr) {
    if (this->GetHandle() == pcStr) {
        return true;
    }
    if (pcStr && this->GetHandle()) {
        return NiStricmp(this->GetHandle(), pcStr) == 0;
    }
    return false;
}
