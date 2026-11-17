#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

class BGSDistantObjectBlock;
class BGSDistantTreeBlock;
class BGSTerrainChunk;
class BGSTerrainManager;
class BSSegmentedTriShape;
class NiNode;

class BGSTerrainNode {
public:
	// Which quarter of its parent a node is (ParentPosition); the root is PP_TOPLEVEL.
	enum PARENT_POSITION : uint32_t {
		PP_TOPLEVEL = 0,
		PP_UPPER_LEFT = 1,
		PP_UPPER_RIGHT = 2,
		PP_LOWER_LEFT = 3,
		PP_LOWER_RIGHT = 4,
	};

	// The parts of a node UnloadData unloads (PDB names and values).
	enum DATA_TO_UNLOAD : int32_t {
		UNLOAD_TERRAIN_ONLY = 1,
		UNLOAD_OBJECTS_ONLY = 2,
		UNLOAD_TREES_ONLY = 4,
		UNLOAD_ALL = 7,
	};

	BGSTerrainNode(BGSTerrainManager* apManager, BGSTerrainNode* apParent, const int16_t asBaseCellX,
		const int16_t asBaseCellY, const uint32_t aiLevel, bool abPersistentNode);
	~BGSTerrainNode();

	BGSTerrainChunk* GetChunkForPoint(const NiPoint3& aPoint);
	// The tree block holding aPoint below this node.
	BGSDistantTreeBlock* GetTreeBlockForPoint(const NiPoint3& aPoint);
	// Switches this node and its children to the apocalypse LOD.
	void TriggerApocalypseRecurse();
	// Unloads the aeDataToUnload data of this node and its children.
	void UnloadDataRecurse(DATA_TO_UNLOAD aeDataToUnload);
	bool CheckMeshSolid(bool abCheckIfLoadedOnly);
	bool CheckMeshSolidUnderMe(bool abCheckIfLoadedOnly);
	bool QSplitNode();
	// Whether the node's range intersects the loaded cells.
	bool CheckIntersectsLoadedRange();
	// bPersistentNode (thiscall, ret 0, 0x6FB150; the exe's shared one-field getter).
	bool QPersistentNode();
	// Updates the morph parameters of the node and its children for arPos.
	void UpdateMorphParameters(const NiPoint3& aViewPosition);
	// The morph parameter of the node for arPos (thiscall, ret 4, ST0 float, 0x6FEB20).
	float ComputeMorphParameter(const NiPoint3& aViewPosition);
	// Updates the texture fade of the node and its children.
	void UpdateFadeParameters();
	// Whether the view position is within the block (low priority block, tree) load distance of the node's square.
	bool ShouldLoadBlock(const NiPoint3& aViewPosition);
	bool ShouldLoadBlockLowPriority(const NiPoint3& aViewPosition);
	bool ShouldLoadTrees(const NiPoint3& aViewPosition);
	// The distance from aPt to the square of side afWidth centred on aCenter; 0 inside it.
	float DistToSquare(NiPoint2 aCenter, float afWidth, NiPoint2 aPt);
	// Where the node sits in its parent, as a fraction of the parent's side, by ParentPosition.
	NiPoint2 GetOffsetFromParent();
	bool ShouldSplit(const NiPoint3& aViewPosition);
	// pManager; the exe calls the shared one-field getter (NiPointer::operator->,
	// 0x559450).
	BGSTerrainManager* GetManager();
	void UpdateBlockVisibility(bool abDebugOutput);
	bool GetPointIsInNode(const NiPoint3& aPoint);
	void UpdateWaterVisibility(BSSegmentedTriShape* apWater, bool abDebugOutput);

protected:
	// Unloads the parts of the node named by aeDataToUnload (bit 1 the terrain chunk, bit 2 the distant object
	// block, bit 4 the distant tree block); a persistent node of the active manager only hides its chunk.
	void UnloadData(DATA_TO_UNLOAD aeDataToUnload);

public:
	void DestroyRecurse();
	void UpdateTerrain(const NiPoint3& aViewPosition, bool abUpdateWaterVisibility);
	void UpdateBlocks(const NiPoint3& aViewPosition, bool abUpdateVisibility);
	void UpdateTrees(const NiPoint3& aViewPosition);
	void StartTextureMorph();
	void EndTextureMorph();

	static void operator delete(void* apMemory);

	BGSTerrainManager* pManager;
	uint32_t iLevel;
	int16_t sBaseCellX;
	int16_t sBaseCellY;
	bool bHasGeometry;
	bool bChildrenLoaded;
	uint8_t padding0E[2];
	BGSTerrainChunk* pChunk;
	BGSDistantObjectBlock* pBlock;
	BGSDistantTreeBlock* pTrees;
	NiNode* pSceneNode;
	BGSTerrainNode* pChildren[4];
	BGSTerrainNode* pParent;
	NiPoint3 Center;
	float fCornerRadius;
	float fSplitDistance;
	float fStartMorphDistance;
	float fEndMorphDistance;
	uint32_t uiFadeStartTime;
	bool bTexFading;
	uint8_t padding55[3];
	PARENT_POSITION ParentPosition;
	bool bPersistentNode;
	bool bSplitNode;
	uint8_t padding5E[2];
};

// The game settings BGSTerrainNode.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> fLowBlockLoadDistanceLow;
extern SettingT<INISettingCollection> fMorphEndDistanceMult;
extern SettingT<RendererSettingCollection> fSplitDistanceMult;
extern SettingT<INISettingCollection> fHighBlockLoadDistanceLow;
extern SettingT<INISettingCollection> fDetailTextureScale;
extern SettingT<INISettingCollection> fDefaultBlockLoadDistanceLow;
extern SettingT<INISettingCollection> fHighTreeLoadDistance;
extern SettingT<RendererSettingCollection> fBlockLoadDistanceLow;
extern SettingT<INISettingCollection> fLowTreeLoadDistance;
extern SettingT<INISettingCollection> uTerrainTextureFadeTime;
extern SettingT<INISettingCollection> fMorphStartDistanceMult;
extern SettingT<INISettingCollection> bKeepLowDetailTerrain;
extern SettingT<INISettingCollection> fDefaultTreeLoadDistance;
extern SettingT<RendererSettingCollection> fBlockLoadDistance;
extern SettingT<RendererSettingCollection> fTreeLoadDistance;
