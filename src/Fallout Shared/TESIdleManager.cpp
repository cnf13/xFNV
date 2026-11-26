#include "Fallout Shared/TESIdleManager.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/BSTCaseInsensitiveStringMap.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/IDLE_ANIM_ROOT.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/NiFormArray.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "SDK/MSVC/string.h"

using IdleRootStringMap = NiTStringTemplateMap<NiTMap<const char*, IDLE_ANIM_ROOT*>, IDLE_ANIM_ROOT*>;

TESIdleManager::TESIdleManager() : IdleArrayMap(0x25u, true), bInitialized(false), LoadedIdleArray(0, 1) {}

TESIdleManager::~TESIdleManager() {
    DestroyRootIdleArrays();
}

// NiTStringTemplateMap<NiTMap<const char*, Setting*>, Setting*>::SetValue, ::ClearValue and the TESForm*,
// NiPointer<BSFaceGenModelMap::Entry> and char* instantiations' (and their NiTStringMap and
// BSTCaseInsensitiveStringMap slots):
void TESIdleManager::InitLoadedIdles() {
    uint32_t isize = LoadedIdleArray.m_usSize;
    for (uint32_t i = 0; i < isize; ++i) {
        TESIdleForm* pidle = LoadedIdleArray[i];
        if (!pidle || pidle->GetInitialized()) {
            TESIdleForm* pignore = nullptr;
            LoadedIdleArray.SetAt(i, pignore);
        }
        else {
            pidle->InitItem();
        }
    }
    for (uint32_t i = 0; i < isize; ++i) {
        TESIdleForm* pidle = LoadedIdleArray[i];
        if (pidle && pidle->InitParents()) {
            TESIdleForm* proot = nullptr;
            LoadedIdleArray.SetAt(i, proot);
        }
    }
    LoadedIdleArray.Compact();
    bInitialized = true;
}

BSSimpleList<const char*>*
TESIdleManager::GetRootFilenameList(const char* apIdleDir, ANIM_GROUP_WEAPON_ENUM aeWeaponGroup) {
    BSSimpleList<const char*>* pret = nullptr;
    if (apIdleDir && *apIdleDir) {
        char acPath[132];
        IDLE_ANIM_ROOT* proot = nullptr;
        BSstrcpy(acPath, 128, apIdleDir);
        char* plastSlash = std::strrchr(acPath, 92);
        BSstrcpy(plastSlash + 1, 128 - (plastSlash - acPath) - 1, "IdleAnims");
        if (IdleArrayMap.GetAt(acPath, proot) && proot) {
            return proot->pKFFileList[aeWeaponGroup];
        }
    }

    return pret;
}

NiFormArray*
TESIdleManager::AddRootIdleArray(const char* apIdleDir, BSSimpleList<const char*>* apList, bool abForceList) {
    IDLE_ANIM_ROOT* proot;
    char acPath[128];
    NiFormArray* pretarray = nullptr;
    if (bCreatedByThisFunction && !abForceList) {
        return pretarray;
    }
    if (!apIdleDir) {
        return pretarray;
    }
    if (!*apIdleDir) {
        return pretarray;
    }
    pretarray = &GetIdleAnimRoot(apIdleDir)->IdleArray;
    if (pretarray) {
        return pretarray;
    }
    if (!apList) {
        proot = new IDLE_ANIM_ROOT(apIdleDir);
        bCreatedByThisFunction = true;
        BSSimpleList<const char*>* pFileList = pModelLoader->BuildKFFileList(apIdleDir, true, false, AGW_NONE);
        proot->AddRootFilenameList(pFileList);
        bCreatedByThisFunction = false;
        IdleArrayMap.SetAt(apIdleDir, proot);
        return &proot->IdleArray;
    }
    BSstrcpy(acPath, 128, apIdleDir);
    char* plastSlash = std::strrchr(acPath, 92);
    BSstrcpy(plastSlash + 1, 128 - (plastSlash - acPath) - 1, "IdleAnims");
    proot = new IDLE_ANIM_ROOT(apIdleDir);
    BSSimpleList<const char*>* pFileList = pModelLoader->CopyFilenameList(apList, nullptr);
    proot->AddRootFilenameList(pFileList);
    IdleArrayMap.SetAt(acPath, proot);
    return &proot->IdleArray;
}

bool TESIdleManager::GetInitialized() {
    return bInitialized;
}

TESIdleForm* TESIdleManager::GetIdleToPlay(TESObjectREFR* apActionRef, TESObjectREFR* apTargetRef) {
    TESIdleForm* pretidle = nullptr;
    if (apActionRef) {
        char acIdleDir[268];
        const char* pcModel = apActionRef->GetModel();
        TESIdleManager::GetIdleDirName(pcModel, acIdleDir);
        Animation* pactionAnim = apActionRef->GetAnimation();
        NiFormArray* parray = &GetIdleAnimRoot(acIdleDir)->IdleArray;
        if (parray) {
            uint32_t icount = parray->m_uiSize;
            for (uint32_t i = 0; i < icount; ++i) {
                TESIdleForm* pidle = static_cast<TESIdleForm*>(parray->GetFormAt(i));
                if (pidle) {
                    BSSimpleList<IDLE_REPLAY_DELAY*>* pReplayDelayList = pactionAnim ? &pactionAnim->replayDelayList
                                                                                     : nullptr;
                    pretidle = pidle->GetIdleToPlay(apActionRef, apTargetRef, pReplayDelayList);
                }
                if (pretidle) {
                    break;
                }
            }
        }
    }
    if (pretidle && pretidle->GetAllowNoFile() && !pretidle->CheckIdleFile()) {
        return nullptr;
    }

    return pretidle;
}

void TESIdleManager::AddLoadedIdle(TESIdleForm* apIdle) {
    if (apIdle) {
        LoadedIdleArray.Add(apIdle);
    }
}

NiFormArray* TESIdleManager::GetLooseIdleArray(const char* apIdleDir) {
    NiFormArray* pretarray = nullptr;
    if (apIdleDir && *apIdleDir) {
        IDLE_ANIM_ROOT* proot = nullptr;
        if (IdleArrayMap.GetAt(apIdleDir, proot) && proot) {
            return &proot->LooseIdleArray;
        }
    }

    return pretarray;
}

// The loose idle array of apForm's model's idle directory, or null.
NiFormArray* TESIdleManager::GetLooseIdleArray(TESForm* apForm) {
    if (!apForm) {
        return nullptr;
    }
    char pIdleDir[0x108];
    TESIdleManager::GetIdleDirName(TESModel::GetModel(apForm), pIdleDir);
    return GetLooseIdleArray(pIdleDir);
}

// The root idle array of apForm's model's idle directory, created when missing.
NiFormArray* TESIdleManager::GetRootIdleArray(TESForm* apForm) {
    if (!apForm) {
        return nullptr;
    }
    // Fills the directory name buffer before the array lookup.
    char pIdleDir[0x108];
    TESIdleManager::GetIdleDirName(TESModel::GetModel(apForm), pIdleDir);
    return AddRootIdleArray(pIdleDir, nullptr, false);
}

IDLE_ANIM_ROOT* TESIdleManager::GetIdleAnimRoot(const char* apIdleDir) {
    IDLE_ANIM_ROOT* pret = nullptr;
    if (apIdleDir && *apIdleDir) {
        IDLE_ANIM_ROOT* proot = nullptr;
        if (IdleArrayMap.GetAt(apIdleDir, proot) && proot) {
            return proot;
        }
    }

    return pret;
}

// The idle root of apForm's model's idle directory, or null.
IDLE_ANIM_ROOT* TESIdleManager::GetIdleAnimRoot(TESForm* apForm) {
    IDLE_ANIM_ROOT* pRoot = nullptr;
    if (apForm) {
        char pIdleDir[0x108];
        TESIdleManager::GetIdleDirName(TESModel::GetModel(apForm), pIdleDir);
        pRoot = GetIdleAnimRoot(pIdleDir);
    }
    return pRoot;
}

void TESIdleManager::DestroyRootIdleArrays() {
    IDLE_ANIM_ROOT* proot;
    const char* pignore;
    NiTMapItem<const char*, IDLE_ANIM_ROOT*>* iter = IdleArrayMap.GetFirstPos();
    while (iter) {
        proot = nullptr;
        pignore = nullptr;
        IdleArrayMap.GetNext(iter, pignore, proot);
        if (proot) {
            uint32_t icount = proot->IdleArray.m_uiSize;
            for (uint32_t i = 0; i < icount; ++i) {
                TESForm* pidle = proot->IdleArray.GetFormAt(i);
                if (pidle) {
                    delete pidle;
                }
            }
            icount = proot->LooseIdleArray.m_uiSize;
            for (uint32_t i = 0; i < icount; ++i) {
                TESForm* pidle = proot->LooseIdleArray.GetFormAt(i);
                if (pidle) {
                    delete pidle;
                }
            }
            for (uint32_t n = 0; n < 12; ++n) {
                if (proot->pKFFileList[n]) {
                    while (!proot->pKFFileList[n]->IsEmpty()) {
                        // The lists own the file names they hold.
                        ourFree(const_cast<char*>(proot->pKFFileList[n]->m_item));
                        proot->pKFFileList[n]->RemoveHead();
                    }
                    BSSimpleList<const char*>* plist = proot->pKFFileList[n];
                    if (plist) {
                        delete plist;
                    }
                }
            }
            if (proot) {
                delete proot;
            }
        }
    }
    uint32_t isize = LoadedIdleArray.m_usSize;
    for (uint32_t uiIndex = 0; uiIndex < isize; ++uiIndex) {
        TESIdleForm* pidle = LoadedIdleArray[uiIndex];
        if (pidle) {
            delete pidle;
        }
    }
    LoadedIdleArray.RemoveAll();
    IdleArrayMap.RemoveAll();
}

void TESIdleManager::DestroyRootIdleArrayForms() {
    IDLE_ANIM_ROOT* proot;
    const char* pignore;
    NiTMapItem<const char*, IDLE_ANIM_ROOT*>* iter = IdleArrayMap.GetFirstPos();
    while (iter) {
        proot = nullptr;
        pignore = nullptr;
        IdleArrayMap.GetNext(iter, pignore, proot);
        if (proot) {
            uint32_t icount = proot->IdleArray.m_uiSize;
            for (uint32_t i = 0; i < icount; ++i) {
                TESForm* pidle = proot->IdleArray.GetFormAt(i);
                if (pidle) {
                    delete pidle;
                }
            }
            proot->IdleArray.RemoveAll();
            icount = proot->LooseIdleArray.m_uiSize;
            for (uint32_t i = 0; i < icount; ++i) {
                TESForm* pidle = proot->LooseIdleArray.GetFormAt(i);
                if (pidle) {
                    delete pidle;
                }
            }
            proot->LooseIdleArray.RemoveAll();
        }
    }
}

void TESIdleManager::InitRootIdleArrays() {
    const char* pidleDir;
    IDLE_ANIM_ROOT* proot;
    NiTMapItem<const char*, IDLE_ANIM_ROOT*>* iter = IdleArrayMap.GetFirstPos();
    while (iter) {
        proot = nullptr;
        pidleDir = nullptr;
        IdleArrayMap.GetNext(iter, pidleDir, proot);
        if (proot) {
            for (uint32_t i = 0; i < 12; ++i) {
                if (proot->pKFFileList[i]) {
                    while (!proot->pKFFileList[i]->IsEmpty()) {
                        // The lists own the file names they hold.
                        ourFree(const_cast<char*>(proot->pKFFileList[i]->m_item));
                        proot->pKFFileList[i]->RemoveHead();
                    }
                    BSSimpleList<const char*>* plist = proot->pKFFileList[i];
                    if (plist) {
                        delete plist;
                    }
                    proot->pKFFileList[i] = nullptr;
                }
            }
            BSSimpleList<const char*>* pfileList = pModelLoader->BuildKFFileList(pidleDir, true, false, AGW_NONE);
            proot->AddRootFilenameList(pfileList);
        }
    }
}

void TESIdleManager::GetIdleDirName(const char* apInDir, char* apOutDir) {
    *apOutDir = 0;
    if (apInDir && *apInDir) {
        char acDir[264];
        BSstrcpy(acDir, 260, apInDir);
        char* pfind = std::strstr(acDir, "IdleAnims");
        if (pfind) {
            uint32_t uiLength = BSstrlen("IdleAnims");
            pfind += uiLength;
            *pfind = 0;
            BSstrcpy(apOutDir, 260, acDir);
        }
        else {
            pfind = std::strrchr(acDir, 92);
            if (pfind) {
                *++pfind = 0;
                BSstrcpy(apOutDir, 260, acDir);
                BSstrcat(apOutDir, 260, "IdleAnims");
            }
        }
    }
}

int32_t TESIdleManager::GetUsedItemLevel() {
    return TESIdleManager::iUsedItemLevel;
}

void TESIdleManager::SetUsedItemLevel(int32_t aiLevel) {
    TESIdleManager::iUsedItemLevel = aiLevel;
}

bool TESIdleManager::GetUsedItemActivate() {
    return TESIdleManager::bActivate;
}

TESObject* TESIdleManager::GetUsedItem() {
    return TESIdleManager::pUsedItem;
}

void TESIdleManager::SetUsedItemActivate(bool abactive) {
    TESIdleManager::bActivate = abactive;
}

void TESIdleManager::SetUsedItem(TESObject* apObject) {
    TESIdleManager::pUsedItem = apObject;
}

void TESIdleManager::operator delete(void* apMemory) {
    ourFree(apMemory);
}

IDLE_ANIM_ROOT::IDLE_ANIM_ROOT(const char* apIdleDir) : pKFFileList{} {
    IdleDir.Set(apIdleDir, 0);
    IdleArray.SetSize(1u);
    IdleArray.m_uiGrowBy = 1u;
    LooseIdleArray.SetSize(1u);
    LooseIdleArray.m_uiGrowBy = 1u;
}

IDLE_ANIM_ROOT::~IDLE_ANIM_ROOT() {}

void IDLE_ANIM_ROOT::AddRootFilenameList(BSSimpleList<const char*>* apList) {
    ANIM_GROUP_WEAPON_ENUM eanimWeapon;
    bool bpowerArmor;
    ANIM_GROUP_MOVE_ENUM eanimMove;
    if (apList) {
        while (!apList->IsEmpty()) {
            const char* pfilename = apList->m_item;
            TESAnimGroup::ParseAnimGroupFilename(pfilename, &eanimMove, &eanimWeapon, &bpowerArmor);
            if (eanimWeapon == AGW_ONE_HAND_PISTOL) {
                const char* plastSlash = std::strrchr(pfilename, 92);
                if (plastSlash) {
                    const char* pcName = plastSlash + 1;
                    if (!_stricmp(pcName, "1HPLeft.kf") ||
                        !_stricmp(pcName, "1HPFastLeft.kf") ||
                        !_stricmp(pcName, "1HMLeft.kf") ||
                        !_stricmp(pcName, "1HMFastLeft.kf")) {
                        eanimWeapon = AGW_NONE;
                    }
                }
            }
            if (!pKFFileList[eanimWeapon]) {
                pKFFileList[eanimWeapon] = new BSSimpleList<const char*>;
            }
            pKFFileList[eanimWeapon]->AddHead(pfilename);
            apList->RemoveHead();
        }
        if (apList) {
            delete apList;
        }
    }
}
