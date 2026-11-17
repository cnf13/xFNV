#pragma once

#include "BSCore/BSSpinLock.h"
#include "BSMain/BSCompoundFrustum.h"
#include "BSShader/ShadowSceneLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"

class BSCubeMapCamera;
class BSCullingProcess;
class BSFogProperty;
class BSMultiBoundNode;
class BSMultiBoundRoom;
class BSPortal;
class BSPortalGraph;
class BSPortalSharedNode;
class NiCamera;
class NiCullingProcess;
class NiDirectionalLight;
class NiTriBasedGeom;

// The root of a lit scene graph.
class ShadowSceneNode : public NiNode {
public:
	using ShadowSceneLightList = NiTPointerListBase<NiPointer<ShadowSceneLight>>;
	using ShadowSceneLightItem = NiTListItem<NiPointer<ShadowSceneLight>>;

	ShadowSceneNode();
	~ShadowSceneNode() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void OnVisible(NiCullingProcess& arCuller) override;

	// Removes apNode's geometry and lights below it from the scene.
	void RemoveObjectRecurse(NiNode* apNode);
	// Culls or unculls the lights in apNode's subtree; nothing without a node. abCull is int32_t because the caller's
	// whole value is passed on; bool would be zero-extended.
	void FindAndCullLight(NiNode* apNode, int32_t abCull);
	// The recursive walk of FindAndCullLight.
	void FindAndCullLightRecurse(NiNode* apNode, int32_t abCull);
	ShadowSceneLight* GetLight(NiLight* apLight);
	// Relights the scene; abForceFull relights every object and abAddLights adds the queued
	// lights.
	void UpdateLighting(bool abForceFull, bool abAddLights);

	// Adds, updates and removes the queued lights, relights the queued objects and empties every queue.
	void ProcessQueuedLights();
	// Queues apObject to be relit by the next ProcessQueuedLights (abActiveLightsOnly: only by lights in view).
	void UpdateObjectLighting(NiAVObject* apObject, bool abActiveLightsOnly);
	// Empties the queues and the lights' room and portal lists of the old graph, removes every light and takes
	// apBSPortalGraph.
	void SetPortalGraph(BSPortalGraph* apBSPortalGraph);
	// Removes dead lights, tests the live ones against the frustum and sets the frame's shader constants.
	void PreOnVisible(NiCullingProcess& arCuller);
	// Adds apLight to the scene's lights (or updates the light already wrapping the same NiLight, and returns it).
	ShadowSceneLight* AddQueuedLight(ShadowSceneLight* apLight);
	// Queues the removal of the scene light that wraps apLight.
	void RemoveLight(NiLight* apLight);
	// Queues the removal of apSSLight.
	void RemoveLight(const NiPointer<ShadowSceneLight>& apSSLight);
	// Removes apSSLight from the scene now.
	void RemoveQueuedLight(const NiPointer<ShadowSceneLight>& apSSLight);
	// Removes apSSLight from the scene now; arIter is its node in its light list.
	void RemoveQueuedLight(const NiPointer<ShadowSceneLight>& apSSLight, ShadowSceneLightItem*& arIter);
	// Builds apPortal's shared compound frustum for apCamera.
	void BuildSharedCompoundFrustum(const NiCamera* apCamera, BSPortal* apPortal);
	// Culls apMultibound's shape against akCuller (a BSCullingProcess), then against the visible occluders seen from
	// akViewPoint.
	void UpdateMultiboundVisibility(BSMultiBoundNode* apMultibound, NiPoint3 akViewPoint, NiCullingProcess& akCuller);
	// Walks the rooms visible from apRoom through their portals, building each room's compound frustum.
	void UpdateRoomAndPortalVisibility(BSMultiBoundRoom* apRoom, const NiCamera* apCamera,
		const NiFrustumPlanes& apPlanes, bool abFirstRoom);

	void SetFogProperty(BSFogProperty* apFog);
	void SetSunLight(NiDirectionalLight* apLight);
	// Marks each occlusion plane of the portal graph visible or culled for apCamera.
	void UpdateOcclusionPlaneVisibility(NiCamera* apCamera);
	void RemoveAllShadowLights();
	// Drops apObject from the scene's lighting.
	void RemoveObject(NiAVObject* apObject);
	// Removes the shadow light that casts apObject's shadow.
	void RemoveShadowLight(NiAVObject* apObject);
	// Queues a new scene light for a directional or point light; returns it, or null for other lights.
	ShadowSceneLight* AddLight(NiLight* apLight, bool abDynamic);
	// Adds every light under apNode, giving point lights afDefaultRadius; returns the number added.
	uint32_t AddAllLightsFromNode_Recurse(NiNode* apNode, float afDefaultRadius);
	// Fades in the shadow light that casts apObject's shadow, or queues a new one.
	ShadowSceneLight* AddShadowLight(NiAVObject* apObject);
	ShadowSceneLight* AddQueuedShadowLight(ShadowSceneLight* apLight);
	void RemoveAllLights();
	void UpdateLight(ShadowSceneLight* apSSLight);
	// Queues the scene light of apLight for an update (finds it, then calls UpdateLight).
	void UpdateLight_0(NiLight* apLight);
	void UpdateQueuedLight(ShadowSceneLight* apSSLight);
	void UpdateQueuedDynamicLight(ShadowSceneLight* apSSLight);

	void SetSceneGraphIndex(uint8_t acSceneGraphIndex);
	BSPortalGraph* GetPortalGraph() const;
	bool GetDisableLightUpdate() const;
	void SetDisableLightUpdate(bool abDisableLightUpdate);
	bool QWireframe() const;
	void SetEyePosition(NiPoint3 aPosition);
	void SetLightingOffset(NiPoint3 kOffset);
	NiPoint3 GetCameraPos() const;
	void AddObject(NiAVObject* apObject);
	ShadowSceneLight* GetLightAt(uint32_t auiIndex) const;
	// Where to place a light for shading a target at aTargetCenter facing aTargetDir; returns whether one was found.
	bool GetAverageLightPos(const NiPoint3& aTargetCenter, const NiPoint3& aTargetDir, NiPoint3& aLoc);
	BSFogProperty* GetFogProperty();
	bool HasVisibleUnboundSpace();
	void* GetLightingOffset(NiPoint3* pOut);
	void ReleaseAllShadowMaps();
	void ToggleWireframe();
	uint16_t GetActiveLightCount();
	ShadowSceneLight* GetFirstShadowLight();
	ShadowSceneLight* GetNextShadowLight();
	void DisableAllShadowLights();
	float GetLuminanceAtPoint(NiPoint3 apPoint, int32_t& inumberlights, float& afAmbientLum, float& afDirectionalLum,
		NiLight* apIgnore);
	float GetViewLuminanceAtPoint(NiPoint3 apPoint, int32_t& inumberlights, float& afAmbientLum,
		float& afDirectionalLum, NiPoint3 kViewPoint, NiLight* apIgnore);
	void UpdateDynamicShadows(BSCullingProcess& arCuller);
	void SetLightCastShadows(NiLight* apLight, int32_t abCastShadows);
	void SortShadowLights(ShadowSceneLight* apPlayerShadow);

	static void RegisterRTTI();
	// Relights apObject: small objects (abSingleLightList) from one sorted list of the lights reaching their bound,
	// gathered from the parent room, else the parent portal node, else apSceneNode; other objects geometry by geometry.
	// abActiveLightsOnly is an int32_t because the caller's whole 32-bit value is passed on; a bool would be
	// zero-extended.
	static bool UpdateQueuedObjectLightingStart(NiAVObject* apObject, ShadowSceneNode* apSceneNode,
		int32_t abActiveLightsOnly, bool abSingleLightList);
	// Fences apGeom's light list, adds the lights of apRoom, else of apPortal, else of apSceneNode (null: the active
	// scene node) whose radius reaches its world bound, and drops the lights left behind the fence. abActiveLightsOnly
	// is an int32_t rather than a bool because the caller's whole value is passed on; a bool would be zero-extended
	// (also below).
	static bool UpdateQueuedObjectLighting(NiTriBasedGeom* apGeom, ShadowSceneNode* apSceneNode,
		int32_t abActiveLightsOnly, BSMultiBoundRoom* apRoom, BSPortalSharedNode* apPortal);
	// The same for every geometry under apObject (only the active child of a switch node).
	static bool UpdateQueuedObjectLighting(NiAVObject* apObject, ShadowSceneNode* apSceneNode,
		int32_t abActiveLightsOnly, BSMultiBoundRoom* apRoom, BSPortalSharedNode* apPortal);
	// Fences apGeom's light list, adds each light of the null-terminated apLightList and drops the rest.
	static bool UpdateQueuedObjectLightingList(NiTriBasedGeom* apGeom, ShadowSceneNode* apSceneNode,
		ShadowSceneLight** apLightList, int32_t abActiveLightsOnly);
	// The same for every geometry under apObject (only the active child of a switch node).
	static bool UpdateQueuedObjectLightingList(NiAVObject* apObject, ShadowSceneNode* apSceneNode,
		ShadowSceneLight** apLightList, int32_t abActiveLightsOnly);
	// Removes apGeom from every light list its lighting property fences it to.
	static void RemoveLightsFromGeom(NiTriBasedGeom* apGeom);

	// bFrustumAddedToVis and fStoredFarClip are never read or written on PC.
	bool bFrustumAddedToVis;
	uint8_t paddingAD[3];
	float fStoredFarClip;
	// The light lists and light queues hold references: AddQueuedShadowLight adds to kShadowLights with IncRefCount,
	// RemoveAllShadowLights empties it with DecRefCount on each element, and PreOnVisible removes from it.
	ShadowSceneLightList lLightList;
	ShadowSceneLightList lShadowLightList;
	ShadowSceneLightItem* kShadowLightListIterator;
	ShadowSceneLightItem* kShadowLightActiveFence;
	ShadowSceneLightItem* kShadowLightFadingInFence;
	NiPointer<ShadowSceneLight> kShadowLightActiveFenceObject;
	NiPointer<ShadowSceneLight> kShadowLightFadingInFenceObject;
	ShadowSceneLight* pSunLight;
	ShadowSceneLightList lLightQueueAdd;
	ShadowSceneLightList lLightQueueRemove;
	ShadowSceneLightList lLightQueueUpdate;
	NiTPointerListBase<NiPointer<NiAVObject>> lObjectQueueUpdate;
	NiTPointerListBase<NiPointer<NiAVObject>> lObjectQueueUpdateActiveOnly;
	uint8_t cSceneGraphIndex;
	uint8_t padding121[3];
	NiObject* pWindowReflectionEnvMap;
	BSCubeMapCamera* spCubeMapCamera;
	uint32_t iCurrentCubeMapFace;
	bool bDisableLightUpdate;
	bool bWireframe;
	uint8_t padding132[2];
	BSFogProperty* spFog;
	BSCompoundFrustum kCompoundFrustum;
	bool bVisibleUnboundSpace;
	uint8_t padding1DD[3];
	BSPortalGraph* pBSPortalGraph;
	NiPoint3 kLightingOffset;
	NiPoint3 kEyePosition;
	bool bAllowLightRemoveQueues;

	static NiRTTI ms_RTTI;
	// Guards the light queues.
	static BSSpinLock SSNLightQueueCrit;
	// Guards the object queues.
	static BSSpinLock SSNObjectQueueCrit;
};
