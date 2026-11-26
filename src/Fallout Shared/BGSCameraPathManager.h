#pragma once

#include "BSCore/NewOverloads.h"
#include "Fallout Shared/NiFormArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

class BGSCameraPath;

// Owns the camera path forms.
class BGSCameraPathManager {
public:
    BGSCameraPathManager();

    // Links the camera paths loaded from the data files.
    void InitLoadedPaths();
    // Creates the default camera path.
    void CreateDefaultPath();
    // Appends apPath to the loaded paths.
    void AddLoadedPath(BGSCameraPath* apPath);
    BGSCameraPath* GetCameraPathToPlay(TESObjectREFR* apActionRef, TESObjectREFR* apTargetRef);
    void DestroyRootPathArrayForms();

    static BGSCameraPath* GetDefaultPath();
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    NiFormArray PathArray;
    // The paths loaded from the data files.
    NiTPrimitiveArray<BGSCameraPath*> LoadedPathArray;

    static inline BGSCameraPath* pDefaultPath;
};

// The engine's camera path manager.
inline BGSCameraPathManager* pCameraPathManager;
