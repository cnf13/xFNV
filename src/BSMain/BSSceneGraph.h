#pragma once

#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class BSCullingProcess;

// The root node of a scene: the node a scene's camera and its view settings hang from.
class BSSceneGraph : public NiNode {
public:
    BSSceneGraph(const char* a1, bool a2, NiCamera* a3);

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    virtual float GetFarDistance();
    virtual void SetViewDistanceBasedOnFrameRate(float afTime);
    ~BSSceneGraph() override;

    // The scene's camera.
    NiCamera* GetCamera();
    // Attaches the camera to the scene's first child when it is not already there, then updates it.
    void SetCameraScene();
    // Sets the camera's field of view (apCamera null: the scene's own camera).
    void SetCameraFOV(float afFOV, bool abForce, NiCamera* apCamera, bool abSetLODMult);
    // The culling process the scene is culled with, pCuller.
    BSCullingProcess* GetCullingProcess() {
        return pCuller;
    }
    // Returns the camera's root node, child 0.
    NiNode* GetCameraRoot();
    // Returns fCurrentFOV.
    float GetCameraFOV();
    void UpdateCameraFOV();

    NiPointer<NiCamera> spCamera;
    NiVisibleArray* pVisArray;
    // The constructor stores the BSCullingProcess it builds; every reader treats it as one.
    BSCullingProcess* pCuller;
    bool bMenuSceneGraph;
    float fCurrentFOV;

    static NiRTTI ms_RTTI;
    // The display settings the INI holds. All are SettingT<INISettingCollection> and are added to the INI collection,
    // the field of view ones included.
    static SettingT<INISettingCollection> fNoLODFarDistanceMin;
    static SettingT<INISettingCollection> fNoLODFarDistanceMax;
    static SettingT<INISettingCollection> fNoLODFarDistancePct;
    static SettingT<INISettingCollection> fNearDistance;
    static SettingT<INISettingCollection> fDefaultFOV;
    static SettingT<INISettingCollection> fDefaultWorldFOV;
    static SettingT<INISettingCollection> fDefault1stPersonFOV;
    static SettingT<INISettingCollection> fPipboy1stPersonFOV;
};

extern SettingT<INISettingCollection> gs_bAutoViewDistance_Display;

extern SettingT<INISettingCollection> gs_iAutoViewLowFrameRate_Display;

extern SettingT<INISettingCollection> gs_iAutoViewHiFrameRate_Display;

extern SettingT<INISettingCollection> gs_iAutoViewMinDistance_Display;
