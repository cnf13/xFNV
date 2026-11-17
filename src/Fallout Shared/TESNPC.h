#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/FaceGen/BSFaceGenNiNode.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class Actor;
class BipedAnim;
class Character;
class TESClass;
class TESCombatStyle;
class TESEyes;
class TESHair;
class TESRace;

int32_t PresetIndexSortFunction(const void* apPtr1, const void* apPtr2);
// Not named in the PDB or IDA; name chosen here. VC9's stdext::unchecked_uninitialized_fill_n for float: the float
// vector's _Ufill (0x60B8A0) passes its _Ptr, _Count, _Val and _Alval; nothing reads the eax it leaves.
void UninitFillN(float* a1, size_t a2, const float& a3, std::allocator<float>& x0);

// The skill values and offsets of an NPC.
struct TESNPC_Data {
	uint8_t cSkillValues[14];
	uint8_t cSkillOffsets[14];
};

// The face the NPC had before its face coordinates changed: the race's face coordinates and the NPC's offsets,
// each for both sexes, and whether only the race's values changed (TESNPC::StoreUndo saves it into
// TESNPC::FaceGenUndoStates and RecallUndo restores it). The PDB's members are Race, NPC_Offset and bRaceOnly
// (env/cache/pdb-types.txt record 0x25478); the exe never reads them (its only FaceGenUndo code is the
// NiTPrimitiveArray<FaceGenUndo*> instantiations), so no offset is pinned here: the PDB's 0x70/0xE0 follow its
// 28-byte FR2MatrixVTC<float>, and src's FR2MatrixVTC<float> is 0x20 (FR2MatrixVTC.h).
struct FaceGenUndo {
	FR2MatrixVTC<float> Race[2][2];
	FR2MatrixVTC<float> NPC_Offset[2][2];
	bool bRaceOnly;
};

// A non-player character's base form.
class TESNPC : public TESActorBase, public TESRaceForm {
public:
	using TESActorBase::InitDefaultWorn;

	TESNPC();
	~TESNPC() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	uint16_t GetSaveSize(uint32_t aiFlags) override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void SaveGame(uint32_t aiFlags) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void Revert(uint32_t aiFlags) override;
	bool FindInFileFast(TESFile* apFile) override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	bool Activate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abIdFlag,
		TESBoundObject* apObjecttoGet, int32_t aicount) override;
	void UnClone3D(TESObjectREFR* apRequester) override;
	NiAVObject* Clone3D(TESObjectREFR* apRef) override;
	TESCombatStyle* GetCombatStyle() const override;
	void SetCombatStyle(TESCombatStyle* apCombatStyle) override;
	void SetActorValue(ActorValue::Index aiWhich, int32_t aiValue) override;
	// TESActorBaseData
	void CopyFromTemplateForm(TESActorBase* apForceTemplate) override;
	bool GetAllowPCDialogue() const override { return true; }
	bool GetAllowPickPocket() const override { return true; }
	bool GetNoPersuasion() const override;
	bool GetNoBloodSpray() const override;
	void SetNoBloodSpray(bool abVal) override;
	bool GetNoBloodDecal() const override;
	void SetNoBloodDecal(bool abVal) override;

	// folded with Character::GetBiped at 0x5D9F90
	BGSImpactDataSet::IMPACT_MATERIAL_TYPE GetBloodImpactMaterial() const override { return eBloodImpactMaterial; }

	// folded with TESNPC::SetAlternateOffsetFaceCoord at 0x5D9F70
	void SetBloodImpactMaterial(BGSImpactDataSet::IMPACT_MATERIAL_TYPE aeMat) override { eBloodImpactMaterial = aeMat; }

	BGSVoiceType* GetVoiceType() const override;
	// TESHealthForm
	uint32_t GetFormHealth() const override;
	// TESModel
	const char* GetModel() override;
	// ActorValueOwner
	int32_t GetActorValue(ActorValue::Index aiWhich) override;

	using TESActorBase::Clone3D;
	using TESActorBase::SetActorValue;
	using TESForm::Save;

	// Gives the NPC apSwapToEyes.
	void SwapEyes(TESEyes* apSwapToEyes);
	TESClass* GetClass() const;
	TESNPC* GetFaceNPC() const;
	float GetHeight() const;
	TESRace* GetRace() const;
	FR2MatrixVTC<float> (*GetOffsetFaceCoord())[2][2];
	BSSimpleList<void*>* GetHeadParts();
	TESNPC_Data* GetSkills();
	float GetHairLength() const;
	TESHair* GetHair();
	// The race's height for this NPC's sex; 0 without a race.
	float GetRaceHeight();
	// Clears the NPC's head data.
	void ClearHead();
	// Builds the NPC's FaceGen head into apOutNodeBiped and apOutNodeSkinned.
	void InitHead(BSFaceGenNiNode*& apOutNodeBiped, BSFaceGenNiNode*& apOutNodeSkinned);
	// Attaches the biped head to apBipedAnim's head bone and the skinned head to apRef's root, sets up the
	// animation data and the actor's look-at IK, and updates apRef's 3D.
	void AttachHead(TESObjectREFR* apRef, BipedAnim* apBipedAnim, BSFaceGenNiNode* apOutNodeBiped,
		BSFaceGenNiNode* apOutNodeSkinned);
	// The NPC's weight: its own, else the race's for its sex; any NPC but the player's base form gives its height.
	float GetWeight();
	// Resets aOutCoord, then merges the race's mean face with this NPC's offsets into it; without a race it takes
	// the manager's current face.
	void GetFaceCoord(FR2MatrixVTC<float> (&aOutCoord)[2][2]);
	// The face-mod texture path of head part aePart, from the face NPC chain or the owning master; false without
	// a master.
	bool GetHeadPartModTextureFileName(RACE_HEAD_PART aePart, char* apFileName, uint32_t aiSize);
	// The face-mod texture of head part aePart into aspTexture: none when EGT loading is on or for the player's
	// base, the manager's default base texture for parts from 1 on, else the texture file of the face-mod path
	// (thiscall, ret 8, 0x6044E0; IDA's GetBaseTexture).
	void GetHeadPartModTexture(RACE_HEAD_PART aePart, NiPointer<NiTexture>& aspTexture);
	// Gives apRef a new biped unless it already has one; returns apRef's biped.
	BipedAnim* CreateBipedAnim(TESObjectREFR* apRef);
	// Same function as MiddleHighProcess::GetAnimation (0x602150); the exe calls that shared one-field getter.
	TESEyes* GetEyeColor() const;
	// Same function as TESDataHandler::GetRegionList (0x4169D0); the exe calls that shared one-field getter.
	uint32_t GetHairColor();
	// Clears the head parts and takes apNPC's hair, eyes, head parts and face coordinates; a non-player NPC also
	// takes apNPC as its face NPC.
	void CopyHeadData(TESNPC* apNPC);

	void SetFaceNPC(TESNPC* apNPC);
	void SetHairLength(float afHairLength);
	void SetHair(TESHair* apHair);
	void SetEyeColor(TESEyes* apEyeColor);
	void SetWeight(float afWeight);
	// Same function as PlayerCharacter::SetTimeSinceLastAmmoRegenTick (0x9442E0); the exe calls that shared
	// one-field setter.
	void SetHeight(float afHeight);
	void SetClass(TESClass* apClass);
	// Recomputes an auto-calc NPC's attributes, skills and services (abPC for the player).
	void InitValues(bool abPC);
	// Equips the NPC's default outfit and weapons on apRef.
	void InitDefaultWorn(TESObjectREFR* apRef, bool abArmor, bool abWeapon, bool abOnlyifClothes, bool abAllowChanges);
	// Puts every worn item of apRef on apBipedAnim, the left hand and the weapon last.
	void InitWorn(TESObjectREFR* apRef, BipedAnim* apBipedAnim);
	// Rebuilds apRef's models.
	void ReplaceRefModel(TESObjectREFR* apRef);
	// Puts apObject (a weapon or a biped model form) and its biped model list on apBipedAnim; always true.
	bool InitWornObject(TESObjectREFR* apRef, BipedAnim* apBipedAnim, TESBoundObject* apObject);
	// Shows or hides the FaceGen head, ears and hair of apRef's 3D (apNode, else apRef's) by its worn items.
	void FixDisplayedHeadParts(TESObjectREFR* apRef, NiNode* apNode);
	// Dresses apRef's biped: the naked body, the default outfit when needed, the head and the worn items.
	void InitParts(TESObjectREFR* apRef, BipedAnim* apBipedAnim, bool abForceInitDefaultWorn);
	// Loads the FaceGen head on this thread.
	void LinearFaceGenHeadLoad(TESObjectREFR* apRef, BipedAnim* apBipedAnim);
	// Reads the face from the old-format save: the race face offsets, race, hair, eyes, hair length and color and
	// the female flag; when any of them changed, rebuilds apCharacter's FaceGen head.
	void LoadFaceGen(Character* apCharacter);
	// Randomizes the face's FaceGen coordinates.
	void RandomizeHead(bool a2, bool a3, bool a4);
	// Dresses apRef's biped with the objects of apObjectArray.
	void LoadObjectArray(TESObjectREFR* apRef, BipedAnim* apBipedAnim, BSSimpleArray<TESBoundObject*>* apObjectArray);
	// Fills apObjectArray with the objects worn on apRef's biped.
	void BuildObjectArray(TESObjectREFR* apRef, BipedAnim* apBipedAnim, BSSimpleArray<TESBoundObject*>* apObjectArray);
	void SetHairColor(uint32_t aiVal);
	// Fixes the normals of apRef's head geometry for this NPC.
	void FixNPCNormals(TESObjectREFR* apRef);
	void InitLoadFaceGen(Character* apCharacter);
	void SaveFaceGen(Character* apCharacter);
	void ClearFaceGenModels();
	uint16_t GetFaceGenSaveSize(Character* apCharacter);
	BSSimpleList<const char*>* BuildDefaultModelList(bool abArmor, bool abWeapon);
	void SwitchRace(TESRace* apRace, Actor* apActor);
	uint32_t GetFormHealthLeveled(bool abAddBase);
	uint32_t GetFormFatigueLeveled(bool abAddBase);

	static void FillCharGenPresetList(TESRace* apRace, TESActorBase::SEX aeSex,
		BSSimpleArray<TESNPC*, 1024>* apOutList);

	TESNPC_Data data;
	TESClass* pCl;
	FR2MatrixVTC<float> RaceFaceOffsetCoord[2][2];
	FR2MatrixVTC<float> (*pAlternateFaceOffsetCoord)[2][2];
	TESHair* pHair;
	float fHairLength;
	TESEyes* pEyeColor;
	NiPointer<BSFaceGenNiNode> spHeadBiped;
	NiPointer<BSFaceGenNiNode> spHeadSkinned;
	NiPointer<BSFaceGenNiNode> spBodyModTexture;
	uint16_t sLastRaceFaceNum;
	uint8_t padding1D2[2];
	TESCombatStyle* pCombatStyle;
	uint32_t iHairColor;
	BSSimpleList<void*> listHeadParts;
	BGSImpactDataSet::IMPACT_MATERIAL_TYPE eBloodImpactMaterial;
	uint32_t iFileOffset;
	TESRace* pOriginalRace;
	TESNPC* pFaceNPC;
	float fHeight;
	float fWeight;
	NiTPrimitiveArray<FaceGenUndo*> FaceGenUndoStates;

	// The NPCs whose faces are character-generation presets.
	static BSSimpleList<TESNPC*> CharGenPresetFaceList;
	static SettingT<GameSettingCollection> iVampirismAgeOffset;
};

inline constexpr fnvd::Vtable TESNPC_vftable_TESRaceForm{};
inline constexpr fnvd::Vtable TESNPC_vftable_BGSDestructibleObjectForm{};
inline constexpr fnvd::Vtable TESNPC_vftable_ActorValueOwner{};
inline constexpr fnvd::Vtable TESNPC_vftable_TESScriptableForm{};
inline constexpr fnvd::Vtable TESNPC_vftable_TESModel{};
inline constexpr fnvd::Vtable TESNPC_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESNPC_vftable_TESAnimation{};
inline constexpr fnvd::Vtable TESNPC_vftable_TESAttributes{};
inline constexpr fnvd::Vtable TESNPC_vftable_TESHealthForm{};
inline constexpr fnvd::Vtable TESNPC_vftable_TESAIForm{};
inline constexpr fnvd::Vtable TESNPC_vftable_TESSpellList{};
inline constexpr fnvd::Vtable TESNPC_vftable_BGSTouchSpellForm{};
inline constexpr fnvd::Vtable TESNPC_vftable_TESContainer{};
inline constexpr fnvd::Vtable TESNPC_vftable_TESActorBaseData{};
inline constexpr fnvd::Vtable TESNPC_vftable{};

// The game settings TESNPC.cpp defines (PDB), at their exe addresses.
extern SettingT<GameSettingCollection> fAttributeClassSecondaryBonus;
extern SettingT<GameSettingCollection> fAttributeClassPrimaryBonus;

// GameSettingCollection: 0xf526e2 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fNPCGeneticVariation;

// GameSettingCollection: 0xf78aed calls 0x40c150.
extern SettingT<GameSettingCollection> gs_sNoPickPocketAgain;
