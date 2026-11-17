#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

class BSShaderAccumulator;
class NiCamera;
class TESObjectREFR;
class TESWaterForm;

// The same object as PlaceableWaterGroup (TESWaterSystem.h), built and destroyed by PlaceableWaterGroup::Construct,
// Destruct and DeletingDestruct.
class WaterGroup {
public:
    TESWaterForm* pWaterForm;
    NiPlane kMainPlane;
    NiPlane kInvPlane;
    // The placeable water references, added by TESWaterSystem::AddWaterRef.
    NiTPointerList<TESObjectREFR*> kPlaceableWaters;
    // TESWaterSystem::AddTESObjectToWaterGroup adds the reference.
    NiTPointerList<TESObjectREFR*> kRefsInWater;
    // TESWaterSystem::AddTESObjectToWaterGroup adds the actor's reference.
    NiTPointerList<TESObjectREFR*> kActorsInWater;
    uint8_t padding48[12];
    BSRenderedTexture* spGroupReflectionMap;
    NiPointer<NiRefObject> spWadingWaterGeometry;
    bool bGroupAtWorldSpaceWaterHeight;
    bool bRenderGroup;
    bool bRenderDepth;
    bool bRenderGroupReflections;
    bool bRenderSilhouetteReflections;
    uint8_t padding61[3];
    uint8_t padding64[12];
    // Holds the list kDynamicReflectiveObjects, declared as raw bytes because there is no class NiTListBase_void.
    uint8_t padding70[12];
    // padding7C holds kStaticDepthObjects, a NiTListBase_void, which is kept as raw bytes because there is no
    // NiTListBase_void class.
    uint8_t padding7C[12];
    // padding88 holds NiTListBase_void kDynamicDepthObjects; there is no class NiTListBase_void declared.
    uint8_t padding88[12];
    NiPointer<BSShaderAccumulator> spGroupReflectionSorter;
    NiPointer<BSShaderAccumulator> spDepthShaderAccum;
    uint32_t uiReflectionGroupCount;
    uint32_t uiDepthGroupCount;
    NiPointer<NiCamera> spCamera;
    NiPointer<NiCamera> spDepthCamera;
    uint32_t uiStencilMask;
};
