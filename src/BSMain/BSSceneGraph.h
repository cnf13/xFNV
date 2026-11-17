#pragma once

#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BSCullingProcess;

// The root node of a scene: the node a scene's camera and its view settings hang from.
class BSSceneGraph : public NiNode {
public:
	BSSceneGraph(const char* a1, bool a2, NiCamera* a3);

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	// The scene's far distance (slot 64).
	virtual float GetFarDistance();
	virtual void SetViewDistanceBasedOnFrameRate(float afTime);
	~BSSceneGraph() override;

	// The scene's camera.
	NiCamera* GetCamera();
	// Attaches the camera to the scene's first child when it is not already there, then updates it.
	void SetCameraScene();
	// Sets the camera's field of view (apCamera null: the scene's own camera).
	void SetCameraFOV(float afFOV, bool abForce, NiCamera* apCamera, bool abSetLODMult);
	// The culling process the scene is culled with, pCuller. Same function as
	// MiddleLowProcess::GetPackageEvaluateHour (0x8D80E0), registered there.
	BSCullingProcess* GetCullingProcess() { return pCuller; }
	// The camera's root node, child 0 (thiscall, ret 0). Defined out of line: the exe calls its copy at 0x558310.
	NiNode* GetCameraRoot();
	// fCurrentFOV. Defined out of line: the exe's copy is at 0x99E040.
	float GetCameraFOV();
	void UpdateCameraFOV();

	NiPointer<NiCamera> spCamera;
	NiVisibleArray* pVisArray;
	// The constructor stores the BSCullingProcess it builds; every reader treats it as one.
	BSCullingProcess* pCuller;
	bool bMenuSceneGraph;
	float fCurrentFOV;

	static NiRTTI ms_RTTI;
	// The display settings the INI holds. All are SettingT<INISettingCollection>: their initialisers (0xFBBDA0 ..
	// 0xFBC200) store that vtable (0x1017720) and add them to the INI collection, the field of view ones included.
	// The exe assigns fDefaultWorldFOV and fDefault1stPersonFOV with the operator=(float) body all instantiations
	// share (0x4ED780).
	static SettingT<INISettingCollection> fNoLODFarDistanceMin;
	static SettingT<INISettingCollection> fNoLODFarDistanceMax;
	static SettingT<INISettingCollection> fNoLODFarDistancePct;
	static SettingT<INISettingCollection> fNearDistance;
	static SettingT<INISettingCollection> fDefaultFOV;
	static SettingT<INISettingCollection> fDefaultWorldFOV;
	static SettingT<INISettingCollection> fDefault1stPersonFOV;
	static SettingT<INISettingCollection> fPipboy1stPersonFOV;
};

inline constexpr fnvd::Vtable BSSceneGraph_vftable{};

// INISettingCollection: 0xfbc308 calls 0x44f570.
extern SettingT<INISettingCollection> gs_bAutoViewDistance_Display;

// INISettingCollection: 0xfbc3ab calls 0x44f570.
extern SettingT<INISettingCollection> gs_iAutoViewLowFrameRate_Display;

// INISettingCollection: 0xfbc44b calls 0x44f570.
extern SettingT<INISettingCollection> gs_iAutoViewHiFrameRate_Display;

// INISettingCollection: 0xfbc4eb calls 0x44f570.
extern SettingT<INISettingCollection> gs_iAutoViewMinDistance_Display;
