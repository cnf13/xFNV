#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedCubeMap.h"

class BSShaderAccumulator;
class ShadowSceneLight;
class ShadowSceneNode;

class BSCubeMapCamera : public NiCamera {
public:
    // How Click renders the cube map. Click runs ClickCubeMap for 0 and ClickCubeMapWithMask for 3.
    enum etCubeMapType : int32_t {
        BSCMC_CUBEMAP = 0,
        BSCMC_6TEXTURES = 1,
        BSCMC_TILEDCUBE = 2,
        BSCMC_CUBEMAPMASK = 3,
    };

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

    etCubeMapType eCubeMapType;
    NiRefObject* spRenderedTexture[6];
    BSRenderedTexture* spRenderedCubeMap;
    ShadowSceneLight* pShadowLight;
    ShadowSceneNode* spCubeMapScene;
    BSShaderAccumulator* spAccumulator;

    static NiRTTI ms_RTTI;
};
