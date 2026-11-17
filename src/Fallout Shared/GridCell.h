#pragma once

#include "BSMain/Setting.h"
#include "BSShader/BSRenderedTexture.h"
#include "Fallout Shared/GridArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class TESObjectCELL;

// One slot of the grid array: the loaded cell, or empty.
struct GridCell {
	TESObjectCELL* pCell;
};

// The grid of loaded cells around the player, as the world renders and simulates them.
class GridCellArray : public GridArray {
public:
	GridCellArray();
	~GridCellArray() override;

	// also loads the statics shared by all cells
	void Init() override;
	void KillAll() override;
	bool SetCenter(int32_t aiX, int32_t aiY) override;
	// detaches the cell at the position from the world, then clears its item
	void Detach(uint32_t aiX, uint32_t aiY) override;
	void ClearItem(uint32_t aiX, uint32_t aiY) override;
	void MoveItem(uint32_t aiX1, uint32_t aiY1, uint32_t aiX2, uint32_t aiY2) override;
	void SwapItem(uint32_t aiX1, uint32_t aiY1, uint32_t aiX2, uint32_t aiY2) override;

	// Updates the transform and the bounds of the grid cells.
	void UpdateManagedNodeTransforms();
	// Reinitialises the distant texture blending of the land of every loaded cell.
	void UpdateDistantLandBlending();
	void ShowLandBorders(bool abShow);
	void UpdateRefSounds();
	// Shows or hides the collision geometry of every attached cell, the opposite of the exterior world's current
	// debug display state.
	void ToggleCollisionGeometry();
	GridCell* Get(uint32_t aiX, uint32_t aiY);
	// Stores apCell's cell at the position; a null apCell clears the position through the vtable instead.
	void Set(uint32_t aiX, uint32_t aiY, GridCell* apCell);
	// Releases the canopy shadow mask texture, so the next reload recreates it.
	void ClearCanopyShadowMaskTexture();
	// Renders the canopy shadow masks of the loaded cells into one texture for the projected shadow pass; uses the
	// default full mask, or turns the pass off, when the cells give no partial masks.
	void RecreateCanopyShadowMaskTexture();
	void AttachToWorld(NiNode* apRoot, uint32_t aiX, uint32_t aiY);
	// Updates the managed nodes of every loaded cell.
	void UpdateManagedNodes(float afTime);
	// Runs the animations of every loaded cell.
	void RunAnimations();
	bool GetAttached(uint32_t aiX, uint32_t aiY);
	void ShowPrimitives(bool abShow);
	// Puts apCell in the grid cell at (aiX, aiY), or clears that cell (ClearItem, vtable
	// +0x1C) when apCell is null (thiscall, ret 0xC, 0x4BA550; TES::LoadGridCell 0x451A50 passes a TESObjectCELL*).
	void Set(uint32_t aiX, uint32_t aiY, TESObjectCELL* apCell);
	void AttachAllLand3D();

	static void SetRecreateCanopyShadowMaskTexture(bool abRecreate);
	static void FixGridsToLoadINISetting();
	static bool GetRecreateCanopyShadowMaskTexture() { return GridCellArray::bRecreateCanopyShadowMaskTexture; }

	GridCell* pGridCellA;
	NiPoint3 WorldCenter;
	bool bLandAttached;
	uint8_t padding21[3];
	NiPointer<BSRenderedTexture> spTexRendered;

	static inline bool bRecreateCanopyShadowMaskTexture = true;
};
inline constexpr fnvd::Vtable GridCellArray_vftable{};

// The game settings GridCell.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> uGridsToLoad;
