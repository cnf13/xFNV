#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedCubeMap.h"
#include "Reconstruction.hpp"

class BSShaderAccumulator;
class ShadowSceneLight;
class ShadowSceneNode;

class BSCubeMapCamera : public NiCamera {
public:
	// How Click renders the cube map. Names from the Xbox PDB; Click (0xBD84B0) runs ClickCubeMap for 0 and
	// ClickCubeMapWithMask for 3, and the dword is compared as such.
	enum etCubeMapType : int32_t {
		BSCMC_CUBEMAP = 0,
		BSCMC_6TEXTURES = 1,
		BSCMC_TILEDCUBE = 2,
		BSCMC_CUBEMAPMASK = 3,
	};

	// The local the exe constructs in place (FNVD_INLINED_LOCAL + FNVD_RECONSTRUCT): the reconstruct runs the
	// destructor first, which releases the textures, the scene and the accumulator, so they start empty.
	BSCubeMapCamera()
		: spRenderedTexture{}, spRenderedCubeMap(nullptr), spCubeMapScene(nullptr), spAccumulator(nullptr) {}

	explicit BSCubeMapCamera(int32_t aeCubeMapType);
	~BSCubeMapCamera() override;

	const NiRTTI* GetRTTI() const override;
	virtual void Click(int32_t aiFace);

	void SetupTransformation(NiRenderedCubeMap::FaceID aeFace);
	// Sets the renderer's camera data from this camera, then accumulates and renders the geometry apLight lights.
	void RenderGeomForLight(ShadowSceneLight* apLight);
	// Renders the six faces into the cube map: the shadow light's geometry in depth-map mode (faces the light
	// frustum-culls are skipped), else the cube map scene.
	void ClickCubeMap();
	// As ClickCubeMap, for face aiFace only (-1: every face); each face is then blended through the window
	// environment mask, rotated per face.
	void ClickCubeMapWithMask(int32_t aiFace);

	// PC's NiCamera base is 0x114 bytes, not the Xbox 0x140 bytes.
	etCubeMapType eCubeMapType;
	NiRefObject* spRenderedTexture[6];
	BSRenderedTexture* spRenderedCubeMap;
	ShadowSceneLight* pShadowLight;
	ShadowSceneNode* spCubeMapScene;
	BSShaderAccumulator* spAccumulator;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable BSCubeMapCamera_vftable{};
