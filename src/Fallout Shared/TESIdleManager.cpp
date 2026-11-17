#include "Fallout Shared/TESIdleManager.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
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

using data_11CB720_t = uint32_t[2];
static data_11CB720_t data_11CB720{};

TESIdleManager::TESIdleManager() : IdleArrayMap(0x25u, 1), bInitialized(false), LoadedIdleArray(0, 1) {}

TESIdleManager::~TESIdleManager() {
    DestroyRootIdleArrays();
}

// NiTStringTemplateMap<NiTMap<const char*, Setting*>, Setting*>::SetValue, ::ClearValue and the TESForm*,
// NiPointer<BSFaceGenModelMap::Entry> and char* instantiations' (and their NiTStringMap and
// BSTCaseInsensitiveStringMap slots):
void TESIdleManager::InitLoadedIdles() {
    uint32_t isize = LoadedIdleArray.m_usSize;
    for (uint32_t i = 0; i < isize; ++i) {
        TESIdleForm* pidle = LoadedIdleArray.operator[](i);
        if (!pidle || pidle->GetInitialized()) {
            TESIdleForm* pignore = 0;
            LoadedIdleArray.SetAt(i, pignore);
        }
        else {
            pidle->InitItem();
        }
    }
    for (uint32_t i_2 = 0; i_2 < isize; ++i_2) {
        TESIdleForm* pidle_2 = LoadedIdleArray.operator[](i_2);
        if (pidle_2) {
            if (pidle_2->InitParents()) {
                TESIdleForm* proot = 0;
                LoadedIdleArray.SetAt(i_2, proot);
            }
        }
    }
    LoadedIdleArray.Compact();
    bInitialized = 1;
}

BSSimpleList<const char*>*
TESIdleManager::GetRootFilenameList(const char* apIdleDir, ANIM_GROUP_WEAPON_ENUM aeWeaponGroup) {
    BSSimpleList<const char*>* pret = 0;
    if (apIdleDir) {
        if (*apIdleDir) {
            char Dst[132];
            IDLE_ANIM_ROOT* proot = 0;
            BSstrcpy(Dst, 128, apIdleDir);
            char* plastSlash = std::strrchr(Dst, 92);
            BSstrcpy(plastSlash + 1, 128 - (plastSlash - Dst) - 1, "IdleAnims");
            if (IdleArrayMap.GetAt(Dst, proot)) {
                if (proot) {
                    return proot->pKFFileList[aeWeaponGroup];
                }
            }
        }
    }
    return pret;
}

NiFormArray*
TESIdleManager::AddRootIdleArray(const char* apIdleDir, BSSimpleList<const char*>* apList, bool abForceList) {
    IDLE_ANIM_ROOT* proot;
    char Dst[128];
    NiFormArray* pretarray = 0;
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
        IDLE_ANIM_ROOT* proot_2;
        IDLE_ANIM_ROOT* v10 = static_cast<IDLE_ANIM_ROOT*>(ourMalloc(sizeof(IDLE_ANIM_ROOT)));
        if (v10) {
            proot_2 = ::new (v10) IDLE_ANIM_ROOT(apIdleDir);
        }
        else {
            proot_2 = 0;
        }
        bCreatedByThisFunction = 1;
        BSSimpleList<const char*>* v6 = pModelLoader->BuildKFFileList(apIdleDir, 1, 0, AGW_NONE);
        proot_2->AddRootFilenameList(v6);
        bCreatedByThisFunction = 0;
        IdleArrayMap.SetAt(apIdleDir, proot_2);
        return &proot_2->IdleArray;
    }
    BSstrcpy(Dst, 128, apIdleDir);
    char* plastSlash = std::strrchr(Dst, 92);
    BSstrcpy(plastSlash + 1, 128 - (plastSlash - Dst) - 1, "IdleAnims");
    IDLE_ANIM_ROOT* apMemory = static_cast<IDLE_ANIM_ROOT*>(ourMalloc(sizeof(IDLE_ANIM_ROOT)));
    if (apMemory) {
        proot = ::new (apMemory) IDLE_ANIM_ROOT(apIdleDir);
    }
    else {
        proot = 0;
    }
    BSSimpleList<const char*>* v4 = pModelLoader->CopyFilenameList(apList, 0);
    proot->AddRootFilenameList(v4);
    IdleArrayMap.SetAt(Dst, proot);
    return &proot->IdleArray;
}

bool TESIdleManager::GetInitialized() {
    return bInitialized;
}

TESIdleForm* TESIdleManager::GetIdleToPlay(TESObjectREFR* apActionRef, TESObjectREFR* apTargetRef) {
    TESIdleForm* pretidle = 0;
    if (apActionRef) {
        char Dst[268];
        const char* Model_ = apActionRef->GetModel();
        TESIdleManager::GetIdleDirName(Model_, Dst);
        Animation* pactionAnim = apActionRef->GetAnimation();
        NiFormArray* parray = &GetIdleAnimRoot(Dst)->IdleArray;
        if (parray) {
            uint32_t icount = parray->m_uiSize;
            for (uint32_t i = 0; i < icount; ++i) {
                TESIdleForm* pidle = static_cast<TESIdleForm*>(parray->GetFormAt(i));
                if (pidle) {
                    BSSimpleList<IDLE_REPLAY_DELAY*>* apReplayDelayList;
                    if (pactionAnim) {
                        apReplayDelayList = &pactionAnim->replayDelayList;
                    }
                    else {
                        apReplayDelayList = 0;
                    }
                    pretidle = pidle->GetIdleToPlay(apActionRef, apTargetRef, apReplayDelayList);
                }
                if (pretidle) {
                    break;
                }
            }
        }
    }
    if (pretidle) {
        if (pretidle->GetAllowNoFile() && !pretidle->CheckIdleFile()) {
            return 0;
        }
    }
    return pretidle;
}

void TESIdleManager::AddLoadedIdle(TESIdleForm* apIdle) {
    if (apIdle) {
        (&LoadedIdleArray)->Add(apIdle);
    }
}

NiFormArray* TESIdleManager::GetLooseIdleArray(const char* apIdleDir) {
    NiFormArray* pretarray = 0;
    if (apIdleDir) {
        if (*apIdleDir) {
            IDLE_ANIM_ROOT* proot = 0;
            if (IdleArrayMap.GetAt(apIdleDir, proot)) {
                if (proot) {
                    return &proot->LooseIdleArray;
                }
            }
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
    IDLE_ANIM_ROOT* v4 = 0;
    if (apIdleDir) {
        if (*apIdleDir) {
            IDLE_ANIM_ROOT* proot = 0;
            if (IdleArrayMap.GetAt(apIdleDir, proot)) {
                if (proot) {
                    return proot;
                }
            }
        }
    }
    return v4;
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
        proot = 0;
        pignore = 0;
        IdleArrayMap.GetNext(iter, pignore, proot);
        if (proot) {
            uint32_t icount = proot->IdleArray.m_uiSize;
            for (uint32_t i = 0; i < icount; ++i) {
                TESForm* pidle = proot->IdleArray.GetFormAt(i);
                if (pidle) {
                    delete pidle;
                }
            }
            uint32_t icount_2 = proot->LooseIdleArray.m_uiSize;
            for (uint32_t i_2 = 0; i_2 < icount_2; ++i_2) {
                TESForm* pidle_2 = proot->LooseIdleArray.GetFormAt(i_2);
                if (pidle_2) {
                    delete pidle_2;
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
        TESIdleForm* pidle_3 = LoadedIdleArray.operator[](uiIndex);
        if (pidle_3) {
            delete pidle_3;
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
        proot = 0;
        pignore = 0;
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
            uint32_t icount_2 = proot->LooseIdleArray.m_uiSize;
            for (uint32_t i_2 = 0; i_2 < icount_2; ++i_2) {
                TESForm* pidle_2 = proot->LooseIdleArray.GetFormAt(i_2);
                if (pidle_2) {
                    delete pidle_2;
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
        proot = 0;
        pidleDir = 0;
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
                    proot->pKFFileList[i] = 0;
                }
            }
            BSSimpleList<const char*>* pfileList = pModelLoader->BuildKFFileList(pidleDir, 1, 0, AGW_NONE);
            proot->AddRootFilenameList(pfileList);
        }
    }
}

void TESIdleManager::GetIdleDirName(const char* apInDir, char* apOutDir) {
    *apOutDir = 0;
    if (apInDir) {
        if (*apInDir) {
            char Str[264];
            BSstrcpy(Str, 260, apInDir);
            char* pfind = std::strstr(Str, "IdleAnims");
            if (pfind) {
                uint32_t v2 = BSstrlen("IdleAnims");
                pfind += v2;
                *pfind = 0;
                BSstrcpy(apOutDir, 260, Str);
            }
            else {
                pfind = std::strrchr(Str, 92);
                if (pfind) {
                    *++pfind = 0;
                    BSstrcpy(apOutDir, 260, Str);
                    BSstrcat(apOutDir, 260, "IdleAnims");
                }
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
    return (bool)((int8_t)TESIdleManager::bActivate);
}

TESObject* TESIdleManager::GetUsedItem() {
    return TESIdleManager::pUsedItem;
}

void TESIdleManager::SetUsedItemActivate(bool abactive) {
    TESIdleManager::bActivate = (bool)((uint8_t)abactive);
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
            GetMoveHandAndPowerArmorTypeFromAnimName(pfilename, &eanimMove, &eanimWeapon, &bpowerArmor);
            if (eanimWeapon == AGW_ONE_HAND_PISTOL) {
                const char* plastSlash = std::strrchr(pfilename, 92);
                if (plastSlash) {
                    const char* String1 = plastSlash + 1;
                    if (!_stricmp(String1, "1HPLeft.kf") ||
                        (!_stricmp(String1, "1HPFastLeft.kf") ||
                         (!_stricmp(String1, "1HMLeft.kf") || !_stricmp(String1, "1HMFastLeft.kf")))) {
                        eanimWeapon = AGW_NONE;
                    }
                }
            }
            if (!pKFFileList[eanimWeapon]) {
                BSSimpleList<const char*>* v2;
                BSSimpleList<const char*>* v4 =
                    static_cast<BSSimpleList<const char*>*>(ourMalloc(sizeof(BSSimpleList<const char*>)));
                if (v4) {
                    v2 = ::new (v4) BSSimpleList<const char*>;
                }
                else {
                    v2 = 0;
                }
                pKFFileList[eanimWeapon] = v2;
            }
            pKFFileList[eanimWeapon]->AddHead(pfilename);
            apList->RemoveHead();
        }
        if (apList) {
            delete apList;
        }
    }
}
