#pragma once

#include "BSShader/BSBatchRenderer.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ReferenceVolume.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"

class BSInstanceRendering;
class BSOcclusionQuery;
class BSShader;
class NiTriShape;
class ShadowSceneNode;

// Signature of the per-render-mode function that registers a geometry with the accumulator, the RegisterObject_*
// functions' own type.
using BSShaderAccumulatorRegisterObjectFunc =
    bool (*)(class BSShaderAccumulator*, NiGeometry*, const NiPropertyState*, BSShaderProperty*, BSShader*);
using RegisterObjectFuncTable = BSShaderAccumulatorRegisterObjectFunc[BSShaderManager::BSSM_RENDER_COUNT];
// Signature of the per-render-mode function that finishes accumulation.
using BSShaderAccumulatorFinishAccumulatingFunc = void (*)(class BSShaderAccumulator*);
using FinishAccumulatingFuncTable = BSShaderAccumulatorFinishAccumulatingFunc[BSShaderManager::BSSM_RENDER_COUNT];

// One of the accumulator's sun occlusion queries and its last result.
struct SunOcclusionTest {
    BSOcclusionQuery* pOcclusionQuery;
    bool bWaiting;
    float fPercentOccluded;
    uint32_t uiPixelCount;
    uint32_t uiFrameCount;
};

// Sorts and draws the geometry of a frame.
class BSShaderAccumulator : public NiAlphaAccumulator {
public:
    // Which side of the water surface the alpha geometry is drawn on.
    enum WATER_SIDE : int32_t {
        ABOVE_WATER = 0,
        BELOW_WATER = 1,
    };

    // What the accumulator collects geometry for. The lighting properties' pass builders test BSSA_ACCUMLOD (2) to put
    // the passes in the LOD group.
    enum etAccumulationMode : uint32_t {
        BSSA_ACCUMNORMAL = 0,
        BSSA_ACCUMDEPTHMAP = 1,
        BSSA_ACCUMLOD = 2,
        BSSA_ACCUMNONE = 3,
    };

    // The counters of one frame's rendering statistics.
    struct DebugStats {
        uint32_t uiTotalGeometry;
        uint32_t uiTotalTriangles;
        uint32_t uiTotalPasses;
        uint32_t uiTotalPassTriangles;
        uint32_t uiQueueMemory;
        uint32_t kFadeNodeTriMap;
        uint32_t uiMaxFadeNodeTri;
        uint32_t uiSkinnedTriangles;
        uint32_t uiTreeTriangles;
        uint32_t uiBlendedTriangles;
        uint32_t uiDistantLODTriangles;
    };

    // The frame's rendering counters (11 of them: lTotalGeometry, lOccludedGeometry, lOcclusionGeometry,
    // lOcclusionTriangles, lTotalTriangles, lTotalPasses, lTotalPassTriangles, lQueueMemory, lWaitForQueryLoops,
    // lWaitForSunQueryFrames, lWaitForBoundVolumeQueryLoops, in that order).
    using DebugDataTable = uint32_t[11];

    // The first parameter is a plain integer rather than a BSShaderManager::BATCHRENDERER_CREATION_MODE, and the third
    // a count rather than a RenderPassType, because callers pass 99-101 and the pass count 759 (or 1, 4), which those
    // enums do not hold.
    BSShaderAccumulator(
        int32_t aeBatchRendererCreationType,
        uint32_t auiBatchRendererCount,
        uint32_t auiPassesPerBatchRenderer
    );
    ~BSShaderAccumulator() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void FinishAccumulating() override;
    bool RegisterObject(NiGeometry* apGeometry) override;
    bool AddShared(NiAVObject* apObject) override;
    void ClearSharedMap() override;
    virtual void FinishAccumulatingPreResolveDepth();
    virtual void FinishAccumulatingPostResolveDepth();
    // shared empty stub
    virtual void RenderPassList(uint16_t, bool) {}

    // Runs the sun occlusion query test aiTestNumber on apSunGeometry.
    void DoSunOcclusionQuery(NiTriShape* apSunGeometry, uint32_t aiTestNumber, NiCamera* apCamera);
    void SetAccumulate(bool abTF);
    bool RenderVolumetricFog(NiCamera* apCamera, BSRenderedTexture* apCurRenderTarget);
    void ResetSunOcclusion();
    void RenderScreenSplatter();
    void RenderBatches(
        BSShaderManager::eBSSMPassEnum aePass,
        BSShaderManager::eBSSMPassEnum aePassEnd,
        int32_t abAlphaPass,
        int32_t aeGroup
    );
    // Renders the static and dynamic decal groups with z writes off and the depth bias on.
    void RenderDecals();
    bool GetRenderDecals();
    void SetRenderDecals(bool abRenderDecals);
    // Renders one BSBatchRenderer geometry group.
    void RenderGeometryGroup(uint32_t auiGeometryGroup, int32_t abAlphaPass);
    // Draws the alpha-blended geometry on one side of the water surface.
    void RenderAlphaGeometry(WATER_SIDE aeSide);
    void SetShadowSceneNode(ShadowSceneNode* apShadowSceneNode);
    void SetRenderMode(BSShaderManager::etRenderMode aeRenderMode);
    void SetUnderWater(bool abUnderWater);
    void SetHasWater(bool abHasWater);

    // Empties the accumulated geometry list.
    void ClearRenderPasses();

    // Clears the render passes of every batch renderer.
    void ClearBloodSplatter();

    // Clears the active pass index list of every batch renderer.
    void ClearActivePasses();

    void SortAlphaPasses();
    bool QWaterRefractionPassWithinRange(
        BSShaderManager::eBSSMPassEnum aeStartRange,
        BSShaderManager::eBSSMPassEnum aeEndRange
    );
    uint32_t GetRefractivePassCount();

    // Sorts the alphas registered until StopGroupingAlphas as one group around apBound, in the current thread's
    // batch renderer.
    void StartGroupingAlphas(const NiBound& aBound, bool abZPrePass);
    void StopGroupingAlphas();
    // Forgets the bound volumes collected so far.
    void ClearAllBoundVolumes();
    // Forgets the bound volume of the reference auiReferenceID.
    void RemoveBoundVolume(uint32_t auiReferenceID);
    // Whether the bound volume of the reference auiReferenceID is occluded.
    bool IsBoundVolumeOccluded(uint32_t auiReferenceID, bool abRunTest);
    // Renders the depth groups into the scene depth texture: the world group from apWorldCamera, then, given
    // ap1stPersonCamera, the player group.
    void RenderDepth(NiCamera* apWorldCamera, NiCamera* ap1stPersonCamera);
    // Renders the refraction normal groups into the refraction effect's texture, from apCamera, and hands that
    // texture to the debug shader when bShowRefractionDebugTexture is set; false on every path.
    bool RenderNormals(NiCamera* apCamera, BSRenderedTexture* apCurRenderTarget);
    void SetupDepth(NiCamera* apWorldCamera, NiCamera* ap1stPersonCamera, BSRenderedTexture* apCurRenderTarget);
    void SetCurrentWaterHeight(int32_t aiHeight);
    NiGeometry* GetBoundTestGeometry();
    void SetWaterReflection(bool abWaterReflection);
    void SetSilhouetteColor(NiColorA& aColor);
    void SetWaterRefraction(bool abWaterRefraction);
    // Renders the tile shader's passes.
    void RenderTiles();
    void ClearGroupPasses(int32_t aeGroup);
    void SetShadowLight(ShadowSceneLight* apLight);
    void GetDebugStats(
        uint32_t& alGeom,
        uint32_t& alOccludedGeom,
        uint32_t& alTri,
        uint32_t& alPass,
        uint32_t& alPassTri,
        uint32_t& alMemory,
        uint32_t& alOcclusionGeom,
        uint32_t& alOcclusionTri,
        uint32_t& alWaitForQueryLoops,
        uint32_t& alWaitForBoundVolumeQueryLoops,
        uint32_t& alWaitForSunQueryLoops
    );
    void ClearAllQueries();
    void AddAndUpdateBoundVolume(uint32_t auiReferenceID, const NiBound& aBound, bool abRunTest);

protected:
    // Builds the empty "RenderPasses" child the accumulated geometry hangs under, and updates it.
    NiNode* CreateRenderPassTree();

public:
    void EvaluateSunOcclusionResults(uint32_t aiTestNumber);
    void ClearSunQueries();
    void ClearRenderedGeometryCache();

    static void FinishAccumulating_Standard_PostResolveDepth(BSShaderAccumulator* apAccumulator);
    static void FinishAccumulating_Standard_PreResolveDepth(BSShaderAccumulator* apAccumulator);
    static void SetRunBoundingSphereTests(bool abEnable, bool abReset);
    static void SetDisplayBoundingSpheres(bool abShow);
    static void ToggleRefractionDebugTexture();
    // Registers the geometry's passes with the batch renderers.
    static bool RegisterObject_Standard(
        BSShaderAccumulator* apAccumulator,
        NiGeometry* apGeometry,
        const NiPropertyState* apState,
        BSShaderProperty* apShaderProp,
        BSShader* apShader
    );
    // The local-map render mode's register function: skips small, unlit and hidden-from-local-map geometry.
    static bool RegisterObject_LocalMap(
        BSShaderAccumulator* apAccumulator,
        NiGeometry* apGeometry,
        const NiPropertyState* apState,
        BSShaderProperty* apShaderProp,
        BSShader* apShader
    );
    // The constant-alpha render mode's register function: skips unlit geometry.
    static bool RegisterObject_ConstAlpha(
        BSShaderAccumulator* apAccumulator,
        NiGeometry* apGeometry,
        const NiPropertyState* apState,
        BSShaderProperty* apShaderProp,
        BSShader* apShader
    );
    // The screen-splatter render mode's register function: hands each of the property's passes that has a pass
    // enum to the current thread's batch renderer.
    static bool RegisterObject_ScreenSplatter(
        BSShaderAccumulator* apAccumulator,
        NiGeometry* apGeometry,
        const NiPropertyState* apState,
        BSShaderProperty* apShaderProp,
        BSShader* apShader
    );
    // The frozen render mode's register function: registers nothing and reports success.
    static bool RegisterObject_Frozen(
        BSShaderAccumulator* apAccumulator,
        NiGeometry* apGeometry,
        const NiPropertyState* apState,
        BSShaderProperty* apShaderProp,
        BSShader* apShader
    );
    // The screen-splatter render mode's finish: does nothing.
    static void FinishAccumulating_ScreenSplatter(BSShaderAccumulator* apAccumulator);

    // Whether the render pass belongs to a shadow-casting variant of the lighting shaders.
    static bool IsShadowPass(uint32_t auiPassEnum);
    static void FinishAccumulating_Standard(BSShaderAccumulator* apAccumulator);
    static void FinishAccumulating_WaterDepth(BSShaderAccumulator* apAccumulator);
    // The interface render mode's finish: sorts the items, then renders the tiles with alpha blending off after.
    static void FinishAccumulating_Interface(BSShaderAccumulator* apAccumulator);
    static void RenderPassImmediately(BSShaderProperty::RenderPass* apRenderPass, uint32_t auiPassEnum);
    static bool RegisterObject_WaterDepth(
        BSShaderAccumulator* apAccumulator,
        NiGeometry* apGeometry,
        const NiPropertyState* apState,
        BSShaderProperty* apShaderProp,
        BSShader* apShader
    );
    static bool RegisterObject_Interface(
        BSShaderAccumulator* apAccumulator,
        NiGeometry* apGeometry,
        const NiPropertyState* apState,
        BSShaderProperty* apShaderProp,
        BSShader* apShader
    );
    static void FinishAccumulating_FirstPerson(BSShaderAccumulator* apAccumulator);

    etAccumulationMode eAccumulationMode;
    bool bWorldGeometry;
    bool bCacheGeometry;
    bool bDoDepthPasses;
    uint32_t uiSunPixelCount;
    bool bWaitingForSunQueury;
    float fPercentSunOccludedStored;
    SunOcclusionTest pSunTests[3];
    bool bAccumulate;
    bool b1stPerson;
    bool bRenderNoShader;
    uint16_t usQueryCount;
    uint16_t usQuerySize;
    // The usQuerySize occlusion queries; ClearAllQueries reads and destroys each.
    BSOcclusionQuery** ppOcclusionQuery;
    // The frame's volumetric-fog geometry. The constructor clears it; RenderVolumetricFog walks it.
    NiTPointerList<NiGeometry*> pVolumetricFogGeometry;
    // The passes kept from earlier frames, per pass group, for the world and the first-person geometry.
    PersistentPassList pRenderedGeometryCache[4];
    PersistentPassList pRendered1stPersonGeometryCache[4];
    NiPointer<NiObject> spBoundTestGeometry;
    // The frame's reference bound volumes, each with its occlusion query.
    NiTPointerList<ReferenceVolume*> ReferenceVolumeList;
    uint32_t uiLastOcclusionRef;
    bool bLastOcclusionResult;
    NiColorA kSilhouetteColor;
    bool bWaterReflection;
    bool bWaterRefraction;
    bool bCellHasWater;
    bool bUnderWater;
    // Read as a signed value; SetCurrentWaterHeight takes an int.
    int32_t iCurrentWaterHeight;
    bool bRenderDecals;
    BSInstanceRendering* pInstanceRenderer;
    BSBatchRenderer* pBatchRenderer[2];
    uint32_t* puicurrentPassA;
    int32_t* picurrentBucketA;
    bool* pbcurrentFirstPassA;
    bool* pbcurrentActiveA;
    uint32_t uiBatchRendererCount;
    uint32_t uiBatchRendererPassCount;
    ShadowSceneNode* pActiveShadowSceneNode;
    NiPointer<NiRefObject> spWaterFogRenderTarget;
    BSShaderManager::etRenderMode eRenderMode;
    NiPointer<ShadowSceneLight> spShadowLight;
    BSShaderProperty::RenderPass kAlphaRenderPass;
    // The room-shared objects of the frame; the constructor builds 37 buckets.
    NiTPointerMap<NiAVObject*, bool> kRoomSharedMap;
    // Guards kRoomSharedMap.
    NiCriticalSection BSCPSharedCrit;

    static inline RegisterObjectFuncTable pRegisterObjectFunc{};
    static inline DebugDataTable kDebugData{};
    static inline BSShaderAccumulatorRegisterObjectFunc pRegisterObjectCurrent{};
    static inline FinishAccumulatingFuncTable pFinishAccumulatingFunc{};
    static inline BSShaderAccumulatorFinishAccumulatingFunc pFinishAccumulatingCurrent{};
    static inline bool bRunBoundingSphereOcclusion;
    static inline bool bResetBoundingSphereOcclusion;
    static inline bool bShowBoundingSpheres;
    static inline bool bShowRefractionDebugTexture;
    static inline bool bRequestRenderPassesScenegraph;
    // The batch renderer this thread registers its passes with.
    static inline thread_local uint32_t iBatchRendererIndex;

    static NiRTTI ms_RTTI;
    static inline bool bShowRenderPassesScenegraph;
    static NiPointer<NiNode> spRenderPassesScenegraph;
};

NiSmartPointer(BSShaderAccumulator);
