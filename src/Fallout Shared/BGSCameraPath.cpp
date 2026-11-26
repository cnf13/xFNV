#include "Fallout Shared/BGSCameraPath.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSCameraPathManager.h"
#include "Fallout Shared/BGSCameraShot.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/NiFormArray.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

void CAMERA_PATH_LINKS::Endian() {
    BSEndian(uiParentPathID, false);
    BSEndian(uiPrevPathID, false);
}

BGSCameraPath::BGSCameraPath() : pChildPaths(nullptr) {
    SetFormType(CPTH_ID);
    BGSCameraPath::InitializeData();
}

BGSCameraPath::~BGSCameraPath() {
    BGSCameraPath::ClearData();
    if (!pDataHandler->GetClearingData()) {
        if (pParentPath) {
            pParentPath->RemoveChild(this);
        }
        else {
            NiFormArray* pArray = &pCameraPathManager->PathArray;
            if (pArray) {
                uint32_t iIndex = pArray->GetIndexForForm(this);
                if (iIndex != 0xFFFFFFFF) {
                    // The next root path takes over this one's previous path.
                    BGSCameraPath* pNextPath = static_cast<BGSCameraPath*>(pArray->GetFormAt(iIndex + 1));
                    if (pNextPath) {
                        pNextPath->pPrevPath = pPrevPath;
                    }
                    pArray->RemoveAt(iIndex);
                    pArray->Compact();
                }
            }
        }
    }
    if (pChildPaths) {
        uint32_t iCount = GetChildCount();
        for (uint32_t i = 0; i < iCount; ++i) {
            BGSCameraPath* pChildPath = GetChildByIndex(i);
            pChildPath->pParentPath = nullptr;
            delete pChildPath;
        }
        delete pChildPaths;
    }
}

void BGSCameraPath::InitializeData() {
    data.cFlags = 0;
    pParentPath = nullptr;
    pPrevPath = nullptr;
}

void BGSCameraPath::ClearData() {
    Conditions.ClearAllConditionItems();
    ShotList.RemoveAll();
}

// Reads the path's conditions, flags, parent and previous paths and its shot list. The default path is dropped from
// the manager's path array first.
bool BGSCameraPath::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != CPTH_ID) {
        return false;
    }
    if (this == BGSCameraPathManager::GetDefaultPath()) {
        NiFormArray* pPaths = &pCameraPathManager->PathArray;
        uint32_t uiIndex = pPaths->GetIndexForForm(this);
        if (uiIndex != 0xFFFFFFFF) {
            pPaths->RemoveAt(uiIndex);
            pPaths->Compact();
        }
    }
    LoadForm(apFile);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        if (uiChunk == OBND_ID) {
            LoadObjectBound(apFile);
        }
        else if (uiChunk == CIDA_ID) {
            Conditions.Load(apFile);
        }
        else if (uiChunk == DATA_ID) {
            LoadData(apFile, static_cast<char*>(static_cast<void*>(&data)), sizeof(data));
        }
        else if (uiChunk == EDID_ID) {
            // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
            char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
            apFile->GetChunkData(pBuffer, 0x200);
            SetFormEditorID(pBuffer);
        }
        else if (uiChunk == ANAM_ID) {
            // The two form IDs are stored straight into the path pointers.
            CAMERA_PATH_LINKS kLinks;
            ourMemset(&kLinks, 0, sizeof(kLinks));
            apFile->GetChunkData(&kLinks, 8);
            if (apFile->QEndian()) {
                kLinks.Endian();
            }
            pParentPath = reinterpret_cast<BGSCameraPath*>(static_cast<uintptr_t>(kLinks.uiParentPathID));
            pPrevPath = reinterpret_cast<BGSCameraPath*>(static_cast<uintptr_t>(kLinks.uiPrevPathID));
            pCameraPathManager->AddLoadedPath(this);
        }
        else if (uiChunk == SNAM_ID) {
            uint32_t uiShotID = 0;
            apFile->GetChunkData(uiShotID);
            BGSCameraShot* pShot = reinterpret_cast<BGSCameraShot*>(static_cast<uintptr_t>(uiShotID));
            ShotList.AddTail(pShot);
        }
    }
    while (apFile->NextChunk());
    return true;
}

void BGSCameraPath::Save() {
    CAMERA_PATH_LINKS kLinks;
    StartForm();
    Conditions.Save();
    ourMemset(&kLinks, 0, sizeof(kLinks));
    if (pParentPath) {
        kLinks.uiParentPathID = pParentPath->iFormID;
    }
    if (!GetDelete()) {
        BGSCameraPath* pprev;
        for (pprev = pPrevPath; pprev && pprev->GetDelete(); pprev = pprev->pPrevPath) {
            ;
        }
        if (pprev) {
            kLinks.uiPrevPathID = pprev->iFormID;
        }
    }
    if (TESForm::QEndianSwapOnSave()) {
        kLinks.Endian();
    }
    TESForm::__AddChunkData(ANAM_ID, &kLinks, 8u);
    if (TESForm::QEndianSwapOnSave()) {
        kLinks.Endian();
    }
    if (TESForm::QEndianSwapOnSave()) {}
    __SaveData(&data, sizeof(data));
    if (TESForm::QEndianSwapOnSave()) {}
    BSSimpleList<BGSCameraShot*>* plist = &ShotList;
    while (plist && !plist->IsEmpty()) {
        BGSCameraShot* pshot = plist->m_item;
        plist = plist->m_pkNext;
        TESForm::AddChunk(SNAM_ID, pshot->iFormID);
    }
    CloseForm();
}

TESForm* BGSCameraPath::CreateDuplicateForm(bool abCreateEditorID, NiTPointerMap<TESForm*, TESForm*>* apCopyMap) {
    TESForm* pdupForm = TESForm::CreateDuplicateForm(abCreateEditorID, apCopyMap);
    BGSCameraPath* pduppath = dynamic_cast<BGSCameraPath*>(pdupForm);
    TESForm* pform = nullptr;
    if (!apCopyMap->GetAt(pParentPath, pform) || !pform) {
        pform = pParentPath;
    }
    pduppath->pParentPath = static_cast<BGSCameraPath*>(pform);
    pform = nullptr;
    if (!apCopyMap->GetAt(pPrevPath, pform) || !pform) {
        pform = pPrevPath;
    }
    pduppath->pPrevPath = static_cast<BGSCameraPath*>(pform);
    if (pChildPaths) {
        pChildPaths->Compact();
        uint32_t icount = pChildPaths->m_uiSize;
        NiFormArray* pduparray = pduppath->CreateChildArray();
        pduparray->SetSize(icount);
        BGSCameraPath* pdupchild = nullptr;
        for (uint32_t i = 0; i < icount; ++i) {
            BGSCameraPath* pchildpath = static_cast<BGSCameraPath*>(pChildPaths->GetAt(i));
            pdupchild = static_cast<BGSCameraPath*>(pchildpath->CreateDuplicateForm(abCreateEditorID, apCopyMap));
            pduparray->SetAt(i, pdupchild);
        }
    }
    return pduppath;
}

void BGSCameraPath::InitItem() {
    if (!GetInitialized()) {
        Conditions.InitItem(this);
        BSSimpleList<BGSCameraShot*>* plist = &ShotList;
        BSSimpleList<BGSCameraShot*>* plastlist = nullptr;
        while (plist && !plist->IsEmpty()) {
            uint32_t tempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(plist->m_item));
            TESForm::AddCompileIndex(tempint, GetFile(-1));
            TESForm* pForm = TESForm::GetFormByNumericID(tempint);
            BGSCameraShot* pshot = dynamic_cast<BGSCameraShot*>(pForm);
            if (pshot) {
                plist->SetItem(pshot);
                plastlist = plist;
                plist = plist->m_pkNext;
            }
            else {
                if (plastlist) {
                    plastlist->Remove(plist->m_item);
                    plist = plastlist->m_pkNext;
                }
                else {
                    plist->RemoveHead();
                }
                if (GetFormEditorIDLength()) {
                    const char* pEditorID = GetFormEditorID();
                    Warning(
                        "MASTERFILE: Unable to find Camera Shot Form (%08X) for Camera Path \"%s\".",
                        tempint,
                        pEditorID
                    );
                }
                else {
                    int32_t iPathID = iFormID;
                    Warning(
                        "MASTERFILE: Unable to find Camera Shot Form (%08X) for Camera Path (%08X).",
                        tempint,
                        iPathID
                    );
                }
            }
        }
        SetInitialized(true);
    }
}

void BGSCameraPath::Copy(TESForm* apCopy) {
    BGSCameraPath* pcopy = dynamic_cast<BGSCameraPath*>(apCopy);
    if (pcopy) {
        CopyAllComponents(pcopy);
        Conditions.Copy(&pcopy->Conditions, nullptr);
        ourMemcpy(&data, &pcopy->data, sizeof(data));
        ShotList.RemoveAll();
        BSSimpleList<BGSCameraShot*>* plist = &pcopy->ShotList;
        while (plist && !plist->IsEmpty()) {
            BGSCameraShot* pshot = plist->m_item;
            plist = plist->GetNext();
            ShotList.AddTail(pshot);
        }
    }
}

bool BGSCameraPath::Compare(TESForm* apCompare) {
    BGSCameraPath* pcomp = dynamic_cast<BGSCameraPath*>(apCompare);
    if (!pcomp) {
        return true;
    }
    if (CompareAllComponents(pcomp)) {
        return true;
    }
    if (Conditions.Compare(&pcomp->Conditions)) {
        return true;
    }
    if (std::memcmp(&data, &pcomp->data, sizeof(data))) {
        return true;
    }
    BSSimpleList<BGSCameraShot*>* plist = &ShotList;
    BSSimpleList<BGSCameraShot*>* pcomplist = &pcomp->ShotList;
    while (pcomplist && !pcomplist->IsEmpty()) {
        if (!plist) {
            return true;
        }
        if (plist->m_item != pcomplist->m_item) {
            return true;
        }
        pcomplist = pcomplist->m_pkNext;
        plist = plist->m_pkNext;
    }
    return plist && !plist->IsEmpty();
}

bool BGSCameraPath::GetAllowNoShots() {
    return (data.cFlags & PATH_DATA::FLAG_ALLOW_NO_SHOTS) != 0;
}

bool BGSCameraPath::GetZoomDisable() {
    return (data.cFlags & PATH_DATA::FLAG_ZOOM_DISABLE) != 0;
}

bool BGSCameraPath::GetZoomShotList() {
    return (data.cFlags & PATH_DATA::FLAG_ZOOM_SHOT_LIST) != 0;
}

uint32_t BGSCameraPath::GetIndexForChild(BGSCameraPath* apChild) {
    if (pChildPaths) {
        return pChildPaths->GetIndexForForm(apChild);
    }
    return 0xFFFFFFFF;
}

void BGSCameraPath::RemoveChildAtIndex(uint32_t aiIndex) {
    if (pChildPaths) {
        BGSCameraPath* pnextpath = GetChildByIndex(aiIndex + 1);
        BGSCameraPath* pprevpath = GetChildByIndex(aiIndex - 1);
        if (pnextpath) {
            pnextpath->pPrevPath = pprevpath;
        }
        pChildPaths->RemoveAt(aiIndex);
        pChildPaths->Compact();
    }
}

BGSCameraPath* BGSCameraPath::GetChildByIndex(uint32_t aiIndex) {
    if (pChildPaths) {
        TESForm* pForm = pChildPaths->GetFormAt(aiIndex);
        return dynamic_cast<BGSCameraPath*>(pForm);
    }
    return nullptr;
}

BGSCameraPath* BGSCameraPath::GetCameraPathToPlay(TESObjectREFR* apActionRef, TESObjectREFR* apTargetRef) {
    BGSCameraPath* pCamera = nullptr;
    if (!GetDelete() && Conditions.IsTrue(apActionRef, apTargetRef)) {
        if (GetAllowNoShots() || !ShotList.IsEmpty()) {
            pCamera = this;
        }
        uint32_t uiNumChildren = GetChildCount();
        if (uiNumChildren) {
            BGSCameraPath* pPathToPlay = nullptr;
            for (uint32_t uiIndex = 0; uiIndex < uiNumChildren; ++uiIndex) {
                BGSCameraPath* pChild = GetChildByIndex(uiIndex);
                if (pChild) {
                    pPathToPlay = pChild->GetCameraPathToPlay(apActionRef, apTargetRef);
                }
                if (pPathToPlay) {
                    break;
                }
            }
            if (pPathToPlay) {
                return pPathToPlay;
            }
        }
    }

    return pCamera;
}

uint32_t BGSCameraPath::GetChildCount() {
    uint32_t uiCount = 0;
    if (pChildPaths) {
        uiCount = pChildPaths->GetSize();
    }
    return uiCount;
}

bool BGSCameraPath::InitParents() {
    bool bretval = true;
    BGSCameraPath* pparent = nullptr;
    uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pParentPath));
    if (itempint) {
        TESForm::AddCompileIndex(itempint, GetFile(-1));
        TESForm* pForm = TESForm::GetFormByNumericID(itempint);
        pparent = dynamic_cast<BGSCameraPath*>(pForm);
        if (!pparent) {
            BSStringT<char> tempstring;
            GetFormDetailedString(tempstring);
            char* pDetails = tempstring.pString;
            Warning("DEFAULT: Could not find parent path (%08X) for %s.", itempint, pDetails);
            bretval = false;
        }
        pParentPath = pparent;
    }
    BGSCameraPath* pprev = nullptr;
    itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPrevPath));
    if (itempint) {
        TESForm::AddCompileIndex(itempint, GetFile(-1));
        TESForm* pForm = TESForm::GetFormByNumericID(itempint);
        pprev = dynamic_cast<BGSCameraPath*>(pForm);
        if (!pprev) {
            BSStringT<char> tempstring;
            GetFormDetailedString(tempstring);
            char* pDetails = tempstring.pString;
            Warning("DEFAULT: Could not find previous path (%08X) for %s.", itempint, pDetails);
            bretval = false;
        }
        pPrevPath = pprev;
    }
    uint32_t iindex = 0;
    NiFormArray* parray = nullptr;
    if (pparent) {
        parray = pparent->CreateChildArray();
    }
    else {
        parray = &pCameraPathManager->PathArray;
    }
    if (parray) {
        if (pprev) {
            uint32_t iprevindex = parray->GetIndexForForm(pprev);
            if (iprevindex != -1) {
                ++iprevindex;
            }
            iindex = iprevindex;
        }
        if (iindex == -1 || GetDelete()) {
            iindex = parray->m_uiSize + 1;
        }
        parray->InsertFormAt(iindex, this);
        FixNextPaths(parray);
    }
    else {
        BSStringT<char> tempstring;
        GetFormDetailedString(tempstring);
        Warning("DEFAULT: Could not find parent array for %s.", tempstring.String());
        bretval = false;
    }
    return bretval;
}

void BGSCameraPath::RemoveChild(BGSCameraPath* apChild) {
    if (apChild && pChildPaths) {
        uint32_t uiIndex = GetIndexForChild(apChild);
        RemoveChildAtIndex(uiIndex);
    }
}

NiFormArray* BGSCameraPath::CreateChildArray() {
    if (pChildPaths) {
        return pChildPaths;
    }
    NiFormArray* pArray = ::new NiFormArray;
    pChildPaths = pArray;
    return pArray;
}

bool BGSCameraPath::GetZoomDefault() {
    return (data.cFlags & PATH_DATA::FLAG_ZOOM_DISABLE) == 0 && (data.cFlags & PATH_DATA::FLAG_ZOOM_SHOT_LIST) == 0;
}

void BGSCameraPath::FixNextPaths(NiFormArray* apArray) {
    if (apArray) {
        uint32_t izerocount = 0;
        uint32_t iindex = -1;
        uint32_t isize = apArray->m_uiSize;
        uint32_t inextindex = -1;
        BGSCameraPath* pnextpath = nullptr;
        for (uint32_t i = 0; i < isize; ++i) {
            BGSCameraPath* ppath = dynamic_cast<BGSCameraPath*>(apArray->GetAt(i));
            if (ppath) {
                if (ppath == this) {
                    iindex = i;
                }
                else if (ppath->pPrevPath == this) {
                    if (pnextpath) {
                        BSStringT<char> tempstring;
                        GetFormDetailedString(tempstring);
                        Warning("DEFAULT: Multiple next paths for %s.", tempstring.String());
                    }
                    else {
                        pnextpath = ppath;
                        inextindex = i;
                    }
                }
            }
            else {
                ++izerocount;
            }
        }
        if (iindex != -1 && inextindex != -1 && inextindex != iindex + 1) {
            TESForm* pEmpty = nullptr;
            apArray->SetAt(inextindex, pEmpty);
            apArray->Compact();
            if (inextindex < iindex) {
                ++izerocount;
            }
            iindex -= izerocount;
            apArray->InsertFormAt(iindex + 1, pnextpath);
            apArray->Compact();
            pnextpath->FixNextPaths(apArray);
        }
    }
}

NiFormArray::~NiFormArray() {}
