#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "BSShader/BSRenderedTexture.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "SDK/MSVC/ehvec.h"

class BGSDecalNode;
class BGSTextureSet;
class BSOcclusionQuery;
class DECAL_CREATION_DATA;
struct DecalVertex;
class NiTriShapeData;

double BSMAX_0(double a1, float a2);

// A vertex of the decal geometry and its index in the temporary vertex array.
struct DecalVertex {
    DecalVertex();

    NiPoint3 fVert;
    int16_t iIndex;
    uint8_t padding0E[2];
};

// A simple projected decal effect: one texture stuck on a world surface.
class BSTempEffectSimpleDecal : public BSTempEffect {
public:
    BSTempEffectSimpleDecal(BGSDecalNode* apParent, float afLifetime, DECAL_CREATION_DATA& arDecalData);
    ~BSTempEffectSimpleDecal() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void Initialize() override;
    bool Update(float afTimeDelta) override;
    NiAVObject* Get3D() override;

    float GetDecalHeight() {
        return fHeight;
    }
    NiAVObject* GetDecalParent();
    bool GetIsValid();
    NiPoint3 GetOrigin();
    NiPoint3 GetDirection();
    // Builds the decal geometry from apAVObj; false when no decal results.
    bool ProcessGeometry(NiAVObject* apAVObj);
    // Clips a polygon against the six clipping planes; returns the vertex count left, 0 when nothing is.
    int32_t ClipPolygon(
        int32_t aiVertexCount,
        const int32_t* apInIndex,
        const NiPoint3* apInVertex,
        const NiPoint3* apInNormal,
        int32_t* apOutIndex,
        NiPoint3* apOutVertex,
        NiPoint3* apOutNormal
    );
    // Reads the occlusion query's result; once it is ready, deletes the query and returns true.
    bool CheckOcclusionQueryResults();
    // Adds a clipped polygon of aiVertexCount vertices to the temporary arrays, merging vertices already added, and
    // fans it into triangles; false when the decal would pass 512 vertices.
    bool AddPolygon(int32_t aiVertexCount, int32_t* apIndex, NiPoint3* apVertex, const NiPoint3* apNormal);
    void CleanUpMemory();
    bool FinalizeGeometry();
    NiTriShapeData* CreateTriData();
    void ClipMesh(NiAVObject* apAVObj);
    void CreateDebugBox(NiColorA& arColor, bool abWireframe);
    void ResizeDataArrays();

    // Clips a polygon against apPlane into the out arrays; returns the new vertex count.
    static int32_t ClipPolygonAgainstPlane(
        NiPlane* apPlane,
        int32_t aiVertexCount,
        const int32_t* apInIndex,
        const NiPoint3* apInVertex,
        const NiPoint3* apInNormal,
        int32_t* apOutIndex,
        NiPoint3* apOutVertex,
        NiPoint3* apOutNormal
    );
    static void PreloadDecalData();
    static void UnloadDecalData();

    bool bFinished;
    bool bFinalize;
    bool bOcclusionQuery;
    bool bClearQueryTexture;
    BSOcclusionQuery* pOcclusionQuery;
    NiPointer<BSRenderedTexture> spQueryTexture;
    uint32_t iCurrentOcclusionResult;
    NiPointer<NiAVObject> sp3D;
    bool bPermanent;
    bool bValidDecal;
    bool bForceFade;
    BGSTextureSet* pTextureSet;
    NiPoint3 pOrigin;
    NiPoint3 pVector;
    NiPoint3 pDecalCenter;
    NiPoint3 pDecalNormal;
    NiPlane pClippingPlanes[6];
    int32_t iDecalVertexCount;
    int32_t iDecalTriangleCount;
    NiPoint3* pTempVertexArray;
    NiPoint3* pFinalVertexArray;
    NiPoint3* pTempNormalArray;
    NiPoint3* pFinalNormalArray;
    NiPoint2* pTempTexcoordArray;
    NiPoint2* pFinalTexcoordArray;
    NiColorA* pTempColorArray;
    NiColorA* pFinalColorArray;
    uint16_t* pTempIndexArray;
    uint16_t* pFinalIndexArray;
    int16_t* pOldToNewIndexArray;
    BSSimpleArray<DecalVertex*> pNewIndexMap;
    BGSDecalNode* pParentNode;
    NiPointer<NiAVObject> spTargetNode;
    float fScale;
    float fRotation;
    float fWidth;
    float fHeight;
    float fDepth;
    uint8_t cSubTexture;
    bool bUseParallaxOcclusion;
    float fParallaxOcclusionScale;
    uint8_t ucParallaxOcclusionMaxPasses;
    bool bUseAlphaTesting;
    bool bUseAlphaBlending;
    float fSpecular;
    NiColor fVertexColor;
    bool bModelSpace;
    bool bTwoSided;

    static NiRTTI ms_RTTI;
    static inline int32_t iCurrentMaxDecalCount = -1;
    static inline float fDecalEpsilon = 0.25f;
    static NiPointer<QueuedFile> spPreloadedDecalTextureSets;
};

NiSmartPointer(BSTempEffectSimpleDecal);

extern SettingT<RendererSettingCollection> gs_uMaxDecals_Decals;
