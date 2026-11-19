#include "Fallout Shared/BGSCameraPathManager.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSCameraPath.h"
#include "Fallout Shared/NiFormArray.h"
#include "Fallout Shared/TESForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "SDK/MSVC/rtti.h"

BGSCameraPathManager::BGSCameraPathManager() : PathArray(), LoadedPathArray(0, 1) {}

void BGSCameraPathManager::InitLoadedPaths() {
    uint32_t isize = LoadedPathArray.m_usSize;
    for (uint32_t i = 0; i < isize; ++i) {
        BGSCameraPath* ppath = LoadedPathArray[i];
        if (!ppath || ppath->GetInitialized()) {
            BGSCameraPath* ppath_2 = nullptr;
            LoadedPathArray.SetAt(i, ppath_2);
        }
        else {
            ppath->InitItem();
            if (ppath->InitParents()) {
                BGSCameraPath* pnull = nullptr;
                LoadedPathArray.SetAt(i, pnull);
            }
        }
    }
    LoadedPathArray.Compact();
}

void BGSCameraPathManager::CreateDefaultPath() {
    if (!BGSCameraPathManager::pDefaultPath) {
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(22u);
        BGSCameraPathManager::pDefaultPath = dynamic_cast<BGSCameraPath*>(FormByNumericID);
        if (!BGSCameraPathManager::pDefaultPath) {
            BGSCameraPath* v4;
            BGSCameraPath* apMemory = static_cast<BGSCameraPath*>(ourMalloc(sizeof(BGSCameraPath)));
            if (apMemory) {
                v4 = ::new (apMemory) BGSCameraPath;
            }
            else {
                v4 = nullptr;
            }
            BGSCameraPathManager::pDefaultPath = v4;
            v4->SetFormID(22u, true);
            BGSCameraPathManager::pDefaultPath->SetFormEditorID("DefaultCameraPath");
            BGSCameraPath* v3 = BGSCameraPathManager::pDefaultPath;
            uint32_t CastingType = PathArray.m_uiSize;
            PathArray.InsertFormAt(CastingType, v3);
        }
    }
}

void BGSCameraPathManager::AddLoadedPath(BGSCameraPath* apPath) {
    if (apPath) {
        (&LoadedPathArray)->Add(apPath);
    }
}

BGSCameraPath* BGSCameraPathManager::GetCameraPathToPlay(TESObjectREFR* apActionRef, TESObjectREFR* apTargetRef) {
    BGSCameraPath* pretpath = nullptr;
    if (apActionRef) {
        uint32_t icount = PathArray.GetSize();
        for (uint32_t i = 0; i < icount; ++i) {
            BGSCameraPath* ppath = static_cast<BGSCameraPath*>(PathArray.GetFormAt(i));
            if (ppath) {
                pretpath = ppath->GetCameraPathToPlay(apActionRef, apTargetRef);
            }
            if (pretpath) {
                break;
            }
        }
    }
    if ((pretpath) && (pretpath->GetAllowNoShots() && !(&pretpath->ShotList)->IsEmpty())) {
        return nullptr;
    }

    return pretpath;
}

void BGSCameraPathManager::DestroyRootPathArrayForms() {
    uint32_t icount = PathArray.m_uiSize;
    for (uint32_t i = 0; i < icount; ++i) {
        TESForm* FormAt = PathArray.GetFormAt(i);
        BGSCameraPath* ppath = dynamic_cast<BGSCameraPath*>(FormAt);
        if (ppath) {
            delete ppath;
        }
    }
    uint32_t isize = LoadedPathArray.m_usSize;
    for (uint32_t i_2 = 0; i_2 < isize; ++i_2) {
        BGSCameraPath* ppath_2 = LoadedPathArray[i_2];
        if (ppath_2) {
            delete ppath_2;
        }
    }
    LoadedPathArray.RemoveAll();
    BGSCameraPathManager::pDefaultPath = nullptr;
    PathArray.RemoveAll();
}

BGSCameraPath* BGSCameraPathManager::GetDefaultPath() {
    return BGSCameraPathManager::pDefaultPath;
}
