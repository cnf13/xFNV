#pragma once

#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/TESIconTree.h"
#include "Fallout Shared/TESModelTree.h"
#include "Fallout Shared/TESObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

class NiTriShapeData;
class TESObjectTREE;

// A tree's DATA chunk.
struct OBJ_TREE {
	// Byte swaps every field of a chunk read from a big-endian file.
	void Endian();

	float fCurveScalar;
	float fMinimumLeafAngle;
	float fMaximumLeafAngle;
	float fBranchDimming;
	float fLeafDimming;
	int32_t iCanopyShadowRadius;
	float fRockSpeed;
	float fRustleSpeed;
};

// What a queued tree billboard builds from: the tree, the cell chunk it belongs to, the node to instance into and the
// per-instance locations and colors.
struct TREE_BILLBOARD_DATA {
	TREE_BILLBOARD_DATA(TESObjectTREE* apTree, uint32_t aiCellChunk, uint32_t aiCellKey, NiNode* apInstancedNode,
		uint32_t aiArraySize, NiPoint3* apLocArray, float* apColorArray);
	~TREE_BILLBOARD_DATA();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	TESObjectTREE* pTree;
	uint32_t iCellChunk;
	uint32_t iCellKey;
	NiNode* pInstancedNode;
	uint32_t iArraySize;
	NiPoint3* pLocArray;
	float* pColorArray;
};

// A tree form (TREE).
class TESObjectTREE : public TESBoundTreeObject,
					  public TESModelTree,
					  public TESIconTree,
					  public BGSDestructibleObjectForm {
public:
	TESObjectTREE();
	~TESObjectTREE() override;

	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	// the index of aiSeed in the tree's seed list, 0xFF when it is not there
	virtual uint8_t GetIndexForSeed(uint32_t aiSeed);
	virtual uint32_t GetSeedAtIndex(uint8_t aiIndex);
	virtual OBJ_TREE* GetData() { return &Data; }
	virtual uint32_t GetRandomSeed();
	virtual float GetCurveScalar();
	virtual void SetCurveScalar(float afCurveScalar);
	virtual float GetMinimumLeafAngle() { return Data.fMinimumLeafAngle; }
	virtual void SetMinimumLeafAngle(float afAngle);
	virtual float GetMaximumLeafAngle();
	virtual void SetMaximumLeafAngle(float afAngle);
	virtual float GetBranchDimming();
	virtual void SetBranchDimming(float afDimming);
	virtual float GetLeafDimming();
	virtual void SetLeafDimming(float afDimming);
	virtual float GetShadowRadius();
	// Takes a float and stores it truncated to an integer.
	virtual void SetShadowRadius(float aiRadius);
	virtual float GetRockSpeed();
	virtual void SetRockSpeed(float afRockSpeed);
	virtual float GetRustleSpeed();
	virtual void SetRustleSpeed(float afRustleSpeed);
	virtual void CreateDistant3D(NiPoint3* apLocArray, NiPoint3* apRotArray, float* apColorArray, uint32_t aiArraySize,
		uint32_t aiCellChunk, uint32_t aiCellKey, NiNode* apInstancedNode, NiNode* apFullNode);
	using TESForm::Save;

	const NiPoint2& GetBillboardSize();
	bool IsLargeEnoughForDistantLOD();
	void GetBillboardTextureFilename(char* apOutString);
	void CreateBillboardGeomData(bool abZUp, NiPointer<NiTriShapeData>& aspTriShapeData);
	NiNode* BuildDistant3D(bool abZUp);

	NiTPrimitiveArray<uint32_t> SeedArray;
	OBJ_TREE Data;
	NiPoint2 BillboardSize;
};
