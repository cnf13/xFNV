#pragma once

#include <cstdint>

class BSCullingProcess;
class BSShaderAccumulator;
class NiAVObject;
class NiCamera;
class NiFixedString;
class NiNode;
class NiPixelData;
class NiPixelFormat;
class NiSourceTexture;

template <class T>
class NiTPointerList;

// Helpers that cull and draw a whole scene through the shader accumulators.
class BSShaderUtil {
public:
    // Culls apScene from apCamera into the culler's accumulator.
    static void AccumulateScene(NiCamera* apCamera, NiAVObject* apScene, BSCullingProcess& aCuller);
    // Draws what apAccumulator collected.
    static void RenderScene(NiCamera* apCamera, BSShaderAccumulator* apAccumulator, bool abUseSPU);
    // Sets the fade alpha of every shader property under apNode.
    static void RecursiveSetPropertyFadeAlpha(NiNode* apNode, float afAlpha);
    // Finishes apAccumulator's post-resolve-depth accumulation. abUseSPU is unread.
    static void RenderScenePostResolveDepth(NiCamera* apCamera, BSShaderAccumulator* apAccumulator, bool abUseSPU);
    // Turns the wireframe flag on or off on every shader property under apObject. abWireframe is an int32_t so the
    // caller's whole value is passed on; bool would be zero-extended.
    static void RecursiveSetPropertyWireframe(NiAVObject* apObject, int32_t abWireframe);
    // Creates pConvertedTextureName and pBGConvertedTextureName.
    static void InitSDM();
    // Culls every scene of aSceneList from apCamera into the culler's accumulator, with that accumulator as the
    // renderer's sorter and the shader manager's accumulator.
    static void
    AccumulateSceneList(NiCamera* apCamera, NiTPointerList<NiAVObject*>& aSceneList, BSCullingProcess& aCuller);
    static void TintUncompressedPixelDataByMipMap(NiPixelData* apData);
    static NiSourceTexture* ConvertTexture(NiSourceTexture* apTex, const NiPixelFormat& arFormat);
    static void KillSDM();
    static void RenderScenePreResolveDepth(NiCamera* apCamera, BSShaderAccumulator* apAccumulator, bool abUseSPU);

    // The names ConvertTexture gives its textures, interned by InitSDM.
    static inline NiFixedString* pConvertedTextureName;
    static inline NiFixedString* pBGConvertedTextureName;
};
