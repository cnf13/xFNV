#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Reconstruction.hpp"

class BSShaderAccumulator;
class NiCamera;
class TESObjectREFR;
class TESWaterForm;

// IDA's name for the PDB's PlaceableWaterGroup (TESWaterSystem.h): the same 0xB0-byte object, built and destroyed
// by PlaceableWaterGroup::Construct, Destruct and DeletingDestruct. Kept for the marked bodies still written
// against IDA's field names.
class WaterGroup {
public:
	TESWaterForm* pWaterForm;
	NiPlane kMainPlane;
	NiPlane kInvPlane;
	// IDA name; not in the PDB. The placeable water references (TESWaterSystem::AddWaterRef 0x4E4730).
	NiTPointerList<TESObjectREFR*> kPlaceableWaters;
	// TESWaterSystem::AddTESObjectToWaterGroup adds the reference.
	NiTPointerList<TESObjectREFR*> kRefsInWater;
	// TESWaterSystem::AddTESObjectToWaterGroup adds the actor's reference.
	NiTPointerList<TESObjectREFR*> kActorsInWater;
	// IDA: NiTListBase_void dList048.
	uint8_t padding48[12];
	BSRenderedTexture* spGroupReflectionMap;
	NiPointer<NiRefObject> spWadingWaterGeometry;
	bool bGroupAtWorldSpaceWaterHeight;
	bool bRenderGroup;
	bool bRenderDepth;
	bool bRenderGroupReflections;
	bool bRenderSilhouetteReflections;
	uint8_t padding61[3];
	// IDA: NiTListBase_void kStaticReflectiveObjects; src has no class NiTListBase_void.
	uint8_t padding64[12];
	// IDA: NiTListBase_void kDynamicReflectiveObjects; src has no class NiTListBase_void.
	uint8_t padding70[12];
	// IDA: NiTListBase_void kStaticDepthObjects; src has no class NiTListBase_void.
	uint8_t padding7C[12];
	// IDA: NiTListBase_void kDynamicDepthObjects; src has no class NiTListBase_void.
	uint8_t padding88[12];
	NiPointer<BSShaderAccumulator> spGroupReflectionSorter;
	// IDA name; not in the PDB (the PDB's PlaceableWaterGroup::spDepthSorter, NiPointer<BSShaderAccumulator>). The
	// exe's constructor (0x4ED5F0) and destructor (0x4ED3E0) assign it through NiPointer's operator= (0x66B0D0).
	NiPointer<BSShaderAccumulator> spDepthShaderAccum;
	uint32_t uiReflectionGroupCount;
	uint32_t uiDepthGroupCount;
	NiPointer<NiCamera> spCamera;
	NiPointer<NiCamera> spDepthCamera;
	uint32_t uiStencilMask;
};
