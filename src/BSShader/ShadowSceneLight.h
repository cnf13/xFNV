#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "BSShader/BSRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BSCullingProcess;
class BSMultiBoundNode;
class BSPortal;
class BSPortalGraph;
class BSShaderAccumulator;
class BSShaderLightingProperty;
class NiCamera;
class NiCullingProcess;
class NiTriBasedGeom;

// A light of the scene together with the geometry it affects and its shadow state.
class ShadowSceneLight : public NiRefObject {
public:
	// The bits of usFrustumCull: the frusta the light is culled from.
	enum eFrustumCull : uint16_t {
		SSL_NONE = 0x0,
		SSL_POSX = 0x1,
		SSL_NEGX = 0x2,
		SSL_POSY = 0x4,
		SSL_NEGY = 0x8,
		SSL_POSZ = 0x10,
		SSL_NEGZ = 0x20,
		SSL_ALL = 0xFF,
	};

	ShadowSceneLight();
	~ShadowSceneLight() override;

	// The luminance this light gives apPoint, leaving out apIgnore.
	float GetLuminanceAtPoint(NiPoint3 apPoint, NiLight* apIgnore);
	void SetShadowCaster(NiNode* apNode);
	// Sets each geometry's depth-map shader constants for this light (unless the renderer is accumulating) and culls
	// it; returns true.
	bool Display(NiCullingProcess& arCuller);
	void SetLight(NiLight* apLight);
	void SetCastShadow(int32_t abCastShadow);
	void SetFadeTarget(float afTarget, bool abForce);
	bool BoundWithinLightRadius(const NiBound& apBound, NiLight* apLight, float afRadiusScale);
	void AddTriGeom(NiTriBasedGeom* apTriGeom);
	void RemoveAllTriShapes();
	void SetDynamicPosition(NiPoint3 abDynamicPosition);
	void InsertUnusedFence();
	void AddObject(NiAVObject* apAVObject);
	// Drops apTriGeom from the geometry list (akIterator: its node, or null to search) and detaches this light from
	// its lighting property.
	void RemoveTriGeom(NiTriBasedGeom* apTriGeom, void* akIterator);
	void RemoveUnusedGeom();
	// Returns in ST(0).
	float GetLuminanceToBound(NiBound* apBound);
	void TestFrustumCull(NiCullingProcess& arCuller);
	void SetDynamicShadowMap(BSRenderedTexture* apDynamicShadowMap);
	uint16_t GetFrustumCull();
	bool IsFadedOut();
	void ShowDebugTexture(bool abShow);
	NiLight* GetLight();
	NiAVObject* GetShadowScene();

protected:
	void ParseObjectAddFrustum(NiAVObject* apObject);
	void ParseObjectAddRadius(NiAVObject* apObject);

public:
	// Copies the 24 clip-space plane floats from apfClipPlanes and marks them ready; null marks them not ready.
	void SetClipSpacePlanes(float* apfClipPlanes);
	float GetViewLuminanceAtPoint(NiPoint3 apPoint, NiPoint3 apView, NiLight* apIgnore);
	void ComputeWeightedShadowLight(BSShaderLightingProperty* apLightingProperty, NiPoint3 aTargetCenter);
	void AccumulateGeometry(BSCullingProcess* a1, int32_t a2);
	void MTRenderFrustumDepthMap(BSCullingProcess& arCuller, int32_t aiIndex);
	bool BoundWithinLightFrustum(const NiBound& arBound);
	bool FrustumFrustumIntersection(NiCamera* apCamera1, NiCamera* apCamera2);
	bool CubeFaceFrustumFrustumIntersection(NiBound aBound, NiCullingProcess& arCuller, uint16_t ausCubeFace);
	void ClearRenderPasses();

	// Orders lights from the brightest to the dimmest; a qsort comparator (UpdateObjectLighting hands it to qsort):
	// the arguments point at ShadowSceneLight* array elements.
	static int32_t SortByLuminance(const void* appLight1, const void* appLight2);
	static bool IsEqual(ShadowSceneLight** a1, ShadowSceneLight** a2);

	uint32_t iEnabledPasses;
	float fLuminance;
	D3DXMATRIX kViewProjection;
	D3DXMATRIX kShadowView;
	D3DXMATRIX kShadowProj;
	float fLODDimmer;
	float fShadowLODDimmer;
	float fShadowLODDimmerTarget;
	float fShadowLODDimmerElapsed;
	NiTPointerListBase<NiPointer<NiGeometry>> lGeomList;
	uint8_t bCastShadow;
	bool bCubeShadowMap;
	uint8_t padding0EE[2];
	NiObject* spSpecularAttenuationTexture;
	bool bPointLight;
	bool bAmbientLight;
	uint8_t padding0F6[2];

	NiPointer<NiLight> spLight;
	bool bDynamicLight;
	uint8_t padding0FD[3];
	NiPoint3 bPointPosition;
	NiPointer<BSRenderedTexture> spDynamicShadowMap;
	uint16_t usFrustumCull;
	uint8_t padding112[2];
	NiObject* spFrustumVis;
	bool bShadowSpotlight;
	uint8_t padding119[3];
	float fSpotlightFOV;
	float fSpotlightFalloff;
	uint8_t bShowDebugTexture;
	uint8_t padding125[3];
	NiPointer<NiNode> spShadowCaster;
	// The PDB gives the list's name only; its element type is taken from RemoveAll's shared NiPointer code.
	NiTPointerListBase<NiPointer<NiAVObject>> lRootList;
	NiTListItem<NiPointer<NiGeometry>>* kGeomListFence;
	NiPointer<NiGeometry> kGeomListFenceObject;
	NiPointer<NiCamera> spShadowCamera;
	NiFrustumPlanes kShadowCameraPlanes;
	float pfClipSpacePlanes[24];
	bool bClipSpacePlanesReady;
	uint8_t padding209[3];
	NiPointer<BSShaderAccumulator> spShadowAccumulator;
	BSSimpleArray<BSMultiBoundNode*, 1024> kMultiboundRooms;
	BSSimpleArray<BSPortal*, 1024> kPortals;
	BSSimpleArray<NiNode*, 1024> kProcessedNodes;
	BSPortalGraph* pPortalGraph;
	uint8_t padding244[0xC];
};

NiSmartPointer(ShadowSceneLight);

// Not named in the PDB or IDA; name chosen here. A dword the exe never reads or writes, between AddTriGeom's guard
// and ImageSpaceEffectWaterDisplacement::bClearSimulation.
inline int32_t data_11FFFE8;
inline constexpr fnvd::Vtable ShadowSceneLight_vftable{};

// Not named in the PDB or IDA; name chosen here. ComputeWeightedShadowLight's per-light weights, one per lit light
// (at most 40); the exe indexes both tables from 4 bytes below their first entry (0x11FFE9C, 0x11FFF3C).
using fLightWeights_t = float[40];
inline fLightWeights_t fLightWeights{};

// The same weights again; only written.
using fLightDistances_t = float[40];
inline fLightDistances_t fLightDistances{};
