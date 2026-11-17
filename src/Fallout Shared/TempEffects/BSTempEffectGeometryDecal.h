#pragma once

#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Reconstruction.hpp"

class BGSTextureSet;
class DECAL_CREATION_DATA;
class NiNode;
class NiSkinPartition;

// A decal effect projected onto arbitrary geometry, not just a flat surface.
class BSTempEffectGeometryDecal : public BSTempEffect {
public:
	BSTempEffectGeometryDecal(TESObjectCELL* apCell, float afLifetime, DECAL_CREATION_DATA* apDecalData,
		NiTriBasedGeom* apGeometry, NiPoint3 apOrigin, NiPoint3 apVector, float afScale, float afRotation,
		uint32_t aiBodyParts);
	~BSTempEffectGeometryDecal() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void Initialize() override;
	bool Update(float afTimeDelta) override;
	// folded with NiAVObject::GetCollisionObject at 0x6838B0
	NiAVObject* Get3D() override;
	TEMP_EFFECT_TYPE GetType() override { return TEMP_EFFECT_GEOMETRY_DECAL; }
	bool CheckShouldSave() override { return true; }
	uint16_t GetSaveSize() override;
	void SaveGame() override;
	bool LoadGame() override;
	using BSTempEffect::LoadGame;
	using BSTempEffect::SaveGame;

	// Sets bFinished (+0x28); the exe's shared byte setter (thiscall, ret 4, 0x929260).
	void SetFinished(bool abFinished);
	bool ShouldAddTriangle(uint32_t aiIndex, int32_t aiBaseVertex, bool abStrip, uint16_t* apIB, NiPoint3* apTexCoords,
		uint16_t* apusVertexMap);
	// Projects the decal onto a skinned target through its additional geometry data, with new skin partitions.
	void InitializeUsingAGD();
	// Builds the decal geometry from the skinned target's deformed vertices, for the body parts in iBodyParts: one
	// skin partition per visible body part that keeps triangles.
	void InitializeUsingData();
	// Creates the decal's geometry from aiNewVertex vertices with texture coordinates and
	// aiNewIndex indices, skinned by apGeomBoneData and apPartition when given.
	void CreateGeometry(NiTriBasedGeom* apGeometry, uint16_t aiNewVertex, uint16_t aiNewIndex, NiPoint3* apGeomVertex,
		NiPoint3* apGeomTexCoord, uint16_t* apGeomIndex, NiSkinData::BoneData* apGeomBoneData,
		NiSkinPartition* apPartition, uint16_t* apBodyPartIndex);
	// iNumVerts; the exe's shared one-field getter of +0x5C (thiscall, ret 0, 0x59E300).
	int32_t GetNumVerts();
	// Holds apNode as the decal's parent node (thiscall, ret 4, 0x4A2C00; defined in BGSDecalManager.cpp).
	void SetDecalParent(NiNode* apNode);

	DECAL_CREATION_DATA* pDecalData;
	NiPointer<NiTriBasedGeom> spDecalGeometry;
	NiTriBasedGeom* pGeometry;
	uint16_t* pOldIndex;
	bool bFinished;
	NiPointer<NiTriBasedGeom> spTargetGeom;
	NiPointer<NiNode> spParent;
	NiPointer<NiNode> spDecalParent;
	NiPoint3 pOrigin;
	NiPoint3 pVector;
	float fScale;
	float fRotation;
	BGSTextureSet* pTextureSet;
	int32_t iNumVerts;
	uint32_t iBodyParts;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable BSTempEffectGeometryDecal_vftable{};
