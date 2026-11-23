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
    uint32_t uiSize = LoadedPathArray.m_usSize;
    for (uint32_t i = 0; i < uiSize; ++i) {
        BGSCameraPath* pPath = LoadedPathArray[i];
        if (!pPath || pPath->GetInitialized()) {
            BGSCameraPath* pNull = nullptr;
            LoadedPathArray.SetAt(i, pNull);
        }
        else {
            pPath->InitItem();
            if (pPath->InitParents()) {
                BGSCameraPath* pNull = nullptr;
                LoadedPathArray.SetAt(i, pNull);
            }
        }
    }
    LoadedPathArray.Compact();
}

void BGSCameraPathManager::CreateDefaultPath() {
    if (!BGSCameraPathManager::pDefaultPath) {
        TESForm* pForm = TESForm::GetFormByNumericID(22u);
        BGSCameraPathManager::pDefaultPath = dynamic_cast<BGSCameraPath*>(pForm);
        if (!BGSCameraPathManager::pDefaultPath) {
            BGSCameraPath* pPath = new BGSCameraPath;
            BGSCameraPathManager::pDefaultPath = pPath;
            pPath->SetFormID(22u, true);
            BGSCameraPathManager::pDefaultPath->SetFormEditorID("DefaultCameraPath");
            pPath = BGSCameraPathManager::pDefaultPath;
            uint32_t uiIndex = PathArray.m_uiSize;
            PathArray.InsertFormAt(uiIndex, pPath);
        }
    }
}

void BGSCameraPathManager::AddLoadedPath(BGSCameraPath* apPath) {
    if (apPath) {
        LoadedPathArray.Add(apPath);
    }
}

BGSCameraPath* BGSCameraPathManager::GetCameraPathToPlay(TESObjectREFR* apActionRef, TESObjectREFR* apTargetRef) {
    BGSCameraPath* pRetPath = nullptr;
    if (apActionRef) {
        uint32_t uiCount = PathArray.GetSize();
        for (uint32_t i = 0; i < uiCount; ++i) {
            BGSCameraPath* pPath = static_cast<BGSCameraPath*>(PathArray.GetFormAt(i));
            if (pPath) {
                pRetPath = pPath->GetCameraPathToPlay(apActionRef, apTargetRef);
            }
            if (pRetPath) {
                break;
            }
        }
    }
    if (pRetPath && pRetPath->GetAllowNoShots() && !pRetPath->ShotList.IsEmpty()) {
        return nullptr;
    }

    return pRetPath;
}

void BGSCameraPathManager::DestroyRootPathArrayForms() {
    uint32_t uiCount = PathArray.m_uiSize;
    for (uint32_t i = 0; i < uiCount; ++i) {
        TESForm* pForm = PathArray.GetFormAt(i);
        BGSCameraPath* pPath = dynamic_cast<BGSCameraPath*>(pForm);
        if (pPath) {
            delete pPath;
        }
    }
    uint32_t uiSize = LoadedPathArray.m_usSize;
    for (uint32_t i = 0; i < uiSize; ++i) {
        BGSCameraPath* pPath = LoadedPathArray[i];
        if (pPath) {
            delete pPath;
        }
    }
    LoadedPathArray.RemoveAll();
    BGSCameraPathManager::pDefaultPath = nullptr;
    PathArray.RemoveAll();
}

BGSCameraPath* BGSCameraPathManager::GetDefaultPath() {
    return BGSCameraPathManager::pDefaultPath;
}
