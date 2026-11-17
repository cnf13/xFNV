#pragma once

#include "Fallout Shared/ActorValue.h"
#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Reconstruction.hpp"

class BGSRagdoll;

// A body part data form (BPTD).
class BGSBodyPartData : public TESForm, public TESModel, public BGSPreloadable {
public:
	BGSBodyPartData();
	~BGSBodyPartData() override;

	void InitializeData() override;
	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void InitItem() override;
	// BGSPreloadable
	void QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) override;

	// Shows the child limbs of limb aePart below apNode.
	void ShowChildLimbs(NiAVObject* apParentObj, NiAVObject* apRootObj, BGSBodyPart::LIMB_ENUM aeParent,
		NiAVObject* apObj);
	// Deletes the 15 body parts and clears their pointers.
	void ClearParts();
	// The ragdoll data; the exe's shared one-field getter 0x4FD380 (thiscall, ret 0).
	BGSRagdoll* QRagdollData();
	// Adds the parts that take part in IK to apOutList.
	void BuildIKPartList(BSSimpleList<BGSBodyPart*>* apOutList);
	// The part for the limb aType, or null outside 0-14.
	BGSBodyPart* GetBodyPart(BGSBodyPart::LIMB_ENUM aType);
	// Adds apPart to the form.
	void AddBodyPart(BGSBodyPart* apPart);
	// Not named in the PDB or IDA; name chosen here. The address of pPartArray: the exe's shared getter 0x6A9540
	// (this + 0x34; thiscall, ret 0), which the exe calls out of line.
	BGSBodyPart** GetPartArray() { return pPartArray; }
	BGSBodyPart* GetBodyPart_005E5130(ActorValue::Index aeIndex);
	void HideChildLimbs(NiAVObject* apParentObj, NiAVObject* apRootObj, BGSBodyPart::LIMB_ENUM aeParent,
		NiAVObject* apObj);

	// Creates the default body part data; cdecl at 0x5E53A0.
	static void GenerateDefaultBodyPartData();

	// the parts by limb slot, then the ragdoll data
	BGSBodyPart* pPartArray[15];
	BGSRagdoll* pRagdollData;

	// The body part data of actors that name none.
	static inline BGSBodyPartData* pDefaultBodyPartData;
	static inline BGSBodyPartData* pPlayerBodyPartData;
};
inline constexpr fnvd::Vtable BGSBodyPartData_vftable{};
inline constexpr fnvd::Vtable BGSBodyPartData_vftable_BGSPreloadable{};
inline constexpr fnvd::Vtable BGSBodyPartData_vftable_TESModel{};
