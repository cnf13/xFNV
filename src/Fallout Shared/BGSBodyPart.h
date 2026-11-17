#pragma once

#include "BSCore/BSStringT.h"
#include "BSMain/IOManager.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelPSA.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

class BGSDebris;
class BGSExplosion;
class BGSImpactDataSet;
class NiAVObject;
class QueuedFile;
class TESForm;

// A body part's data (BPND): damage, explosion and dismemberment settings.
struct PART_DATA {
	PART_DATA();

	// Swaps the float fields read from a big-endian file; the form pointers still hold IDs and are not swapped.
	void Endian();
	// Sets the default values: health percent 5, damage mult 1, no actor value (-1), to-hit
	// chance 100, the scales 1, the rest 0 (thiscall, ret 0, 0x5E3A00; eax holds this only from the last store).
	void Initialize();

	float fDamageMult;
	uint8_t cFlags;
	uint8_t cType;
	uint8_t cHealthPercent;
	char cActorValue;
	uint8_t cToHitChance;
	uint8_t cExplosionChance;
	uint8_t cExplosionGenericDebrisCount;
	uint8_t padding0B;
	BGSDebris* pExplosionGenericDebris;
	BGSExplosion* pExplosion;
	float fTrackingMaxAngle;
	float fExplosionGenericDebrisScale;
	uint8_t cDismemberGenericDebrisCount;
	uint8_t padding1D[3];
	BGSDebris* pDismemberGenericDebris;
	BGSExplosion* pDismemberExplosion;
	float fDismemberGenericDebrisScale;
	NiPoint3 kGoreTranslate;
	NiPoint3 kGoreRotate;
	BGSImpactDataSet* pDismemberImpactDataSet;
	BGSImpactDataSet* pExplosionImpactDataSet;
	uint8_t cDismemberDecalCount;
	uint8_t cExplosionDecalCount;
	uint8_t padding4E[2];
	float fExplosionSpecialDebrisScale;
};

// One body part of a body part data form. Only what the reconstructed code calls is declared. The exe's class has
// no base (RTTI 0x110FBB4) and one virtual, QueueFiles (vtable 0x1045404).
class BGSBodyPart {
public:
	// The body part slots; the enumerators are not needed yet.
	enum LIMB_ENUM : int32_t;

	BGSBodyPart();
	~BGSBodyPart();

	// Slot 0: queues the part's limb replacement model and its explosions', debris' and
	// impact data sets' files at aePriority under apParent.
	virtual void QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent);

	// Resolves the part's explosion, debris and impact data set references against the
	// owning form's file.
	void InitItem(TESForm* apParentForm);
	// The part's BODY_PART_TYPE, Data.ucType.
	LIMB_ENUM GetPartType();

	// Whether the part is an IK biped part (flag 4 of the byte at +0x60; thiscall, ret 0,
	// 0x87E920).
	bool IsIKBiped();
	// Whether the part does head tracking (flag 0x20 of the byte at +0x60; thiscall, ret 0,
	// 0x87E940).
	bool DoesHeadtracking();
	// The head tracking's maximum angle in degrees; the exe's shared float getter of +0x70 (thiscall, ret 0,
	// 0x51B860).
	float GetTrackingMaxAngle();
	// The name of the node the part's IK chain starts at.
	const char* GetIKStartNodeName();
	// The actor value whose damage dismembers the part.
	char GetActorValue();
	uint8_t GetHealthPercent();
	// Data.cToHitChance.
	uint8_t GetToHitChance() { return Data.cToHitChance; }
	bool IsExplodeChanceAbsolute();
	uint8_t GetExplosionChance();
	void InitializeData();
	void ClearData();
	bool Load(TESFile* apFile);
	bool IsIKData();
	uint8_t GetDebrisCount();
	uint8_t GetSeveredDecalCount();
	bool IsExplodablePart();
	bool IsSeverablePart();
	// PartName's text: the exe's shared getter 0x6838B0 (NiAVObject::GetCollisionObject's
	// address: NiPointer::operator-> on +0x1C, the first dword of PartName), which BGSBodyPartData::Load and the
	// VATS menu call on the part out of line.
	const char* GetPartName();
	// Data.pDismemberImpactDataSet (+0xA0): the exe's shared one-dword getter (thiscall, ret 0,
	// 0x4FD400).
	BGSImpactDataSet* GetDismemberImpactDataSet();
	NiPoint3* GetGoreTranslate(NiPoint3* apResult);
	// GoreObjectName's text: NiPointer::operator-> on +0x24, the first dword of GoreObjectName.
	const char* GetGoreObjectName();
	NiPoint3* GetGoreRotate(NiPoint3* apResult);
	const char* GetTargetName();
	const char* GetNodeName();

	// Unhides the limb's geometry under apObj.
	static void UnhideLimb(LIMB_ENUM aeLimb, NiAVObject* apObj);
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static void HideLimb(BGSBodyPart::LIMB_ENUM aeLimb, NiAVObject* apObj);
	// Inferred type: a2: NiObject* from 0x8b645b (result of NiObject::Clone), its members used at 0x5e4979.
	// usBodyPart: its one caller, Actor::HideDismemberedLimb, passes its LIMB_ENUM, which the exe compares with the
	// limbs it enumerates.
	static void ShowLimb(BGSBodyPart::LIMB_ENUM usBodyPart, NiObject* a2);

	BSStringT<char> NodeName;
	BSStringT<char> TargetName;
	BSStringT<char> IKStartNodeName;
	BSStringT<char> PartName;
	BSStringT<char> GoreObjectName;
	// the model of the special debris the part's explosion leaves (IDA: kLimbReplacement)
	TESModel ExplosionSpecialDebris;
	TESModelPSA PSAModel;
	PART_DATA Data;
};

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
uint8_t GetGenericDebrisCount(BGSBodyPart* apThis, [[maybe_unused]] void* apEdx);

// The body part slots (PDB enumerators; values from the exe).
enum BGSBodyPart::LIMB_ENUM : int32_t {
	LIMB_NONE = -1,
	LIMB_TORSO = 0,
	LIMB_HEAD_1 = 1,
	LIMB_HEAD_2 = 2,
	LIMB_LEFT_ARM_1 = 3,
	LIMB_LEFT_ARM_2 = 4,
	LIMB_RIGHT_ARM_1 = 5,
	LIMB_RIGHT_ARM_2 = 6,
	LIMB_LEFT_LEG_1 = 7,
	LIMB_LEFT_LEG_2 = 8,
	LIMB_LEFT_LEG_3 = 9,
	LIMB_RIGHT_LEG_1 = 10,
	LIMB_RIGHT_LEG_2 = 11,
	LIMB_RIGHT_LEG_3 = 12,
	LIMB_BRAIN = 13,
	LIMB_WEAPON = 14,
	LIMB_COUNT = 15,
};

inline constexpr fnvd::Vtable BGSBodyPart_vftable{};
