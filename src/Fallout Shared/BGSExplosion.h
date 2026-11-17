#pragma once

#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESImageSpaceModifiableForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"
#include "Reconstruction.hpp"

class BGSImpactData;
class BGSImpactDataSet;
enum HavokMaterialType : int32_t;
enum SOUND_LEVEL : int32_t;
class TESObjectLIGH;
class TESSound;

// An explosion's DATA: force, damage, radius, light, sounds, flags, image space radius, impact data set, radiation
// and sound level.
struct BGSExplosionData {
	BGSExplosionData();

	// Byte-swaps all thirteen dwords read from a big-endian file.
	void Endian();
	// Zeroes the data and sets the normal sound level.
	void Initialize();

	float fForce;
	float fDamage;
	float fRadius;
	TESObjectLIGH* pLight;
	TESSound* pSound1;
	uint32_t iFlags;
	float fImageSpaceRadius;
	BGSImpactDataSet* pImpactDataSet;
	TESSound* pSound2;
	float fRadiationRads;
	float fRadiationDissipationTime;
	float fRadiationRadius;
	SOUND_LEVEL eSoundLevel;
};

// An explosion form (EXPL).
class BGSExplosion : public TESBoundObject,
					 public TESFullName,
					 public TESModel,
					 public TESEnchantableForm,
					 public BGSPreloadable,
					 public TESImageSpaceModifiableForm {
public:
	BGSExplosion();
	~BGSExplosion() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	NiAVObject* Clone3D(TESObjectREFR* pRequester, bool deepCopy) override;
	void UnClone3D(TESObjectREFR* apRequester) override;
	void QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) override;

	using TESBoundObject::Clone3D;
	using TESForm::Save;

	// The explosion's damage (thiscall, ST0 float, 0x6A78F0).
	float GetDamage();
	// The explosion's radius in game units, scaled unless flag 1 is set (thiscall, ST0
	// float, 0x477900).
	float GetRadiusBSUnits();
	// Whether any of the explosion's flag bits aiFlag is set (thiscall, ret 4, 0x477950; the
	// exe's folded flag test).
	bool GetFlag(int32_t aiFlag);
	// Inferred type: return: double: used as one at 0x9ae6c3
	double GetDissipationTime();
	double GetISRadius();
	// Inferred type: return: double: used as one at 0x9acfa2
	double GetRADRadius();
	// The address of Data (thiscall, 0x4FADC0); the exe folds BGSNote::GetQuestList into it.
	BGSExplosionData* GetData();
	// pImpactPlacedObject; the exe folds it with BGSBodyPartData::QRagdollData (thiscall,
	// 0x4FD380).
	TESBoundObject* QImpactPlacedObject();
	// Sets pImpactPlacedObject; the exe folds it with MobileObject::SetDialogueItemTarget
	// (thiscall, ret 4, 0x57BD60).
	void SetImpactPlacedObject(TESBoundObject* apObj);
	// Data.pImpactDataSet. Same function as TESObjectCONT::GetCloseSound (0x516BF0), registered there.
	BGSImpactDataSet* GetImpactDataSet() { return Data.pImpactDataSet; }
	// Sets Data.fForce.
	void SetForce(float afValue);
	// The impact data set's impact data for aehkMatType; null without a set.
	BGSImpactData* GetImpactData(HavokMaterialType aehkMatType);
	// The value of the gs_iSoundLevel setting for Data.eSoundLevel; 0 for an unknown level.
	int32_t GetSoundLevelValue();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	// IDA keeps this in Data; the PDB has it ahead of it
	TESBoundObject* pImpactPlacedObject;
	BGSExplosionData Data;
};
inline constexpr fnvd::Vtable BGSExplosion_vftable_TESImageSpaceModifiableForm{};
inline constexpr fnvd::Vtable BGSExplosion_vftable_BGSPreloadable{};
inline constexpr fnvd::Vtable BGSExplosion_vftable_TESEnchantableForm{};
inline constexpr fnvd::Vtable BGSExplosion_vftable_TESModel{};
inline constexpr fnvd::Vtable BGSExplosion_vftable_TESFullName{};
inline constexpr fnvd::Vtable BGSExplosion_vftable{};
