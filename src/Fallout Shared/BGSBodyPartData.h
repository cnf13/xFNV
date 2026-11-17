#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"

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
	BGSRagdoll* QRagdollData();
	// Adds the parts that take part in IK to apOutList.
	void BuildIKPartList(BSSimpleList<BGSBodyPart*>* apOutList);
	// The part for the limb aType, or null outside 0-14.
	BGSBodyPart* GetBodyPart(BGSBodyPart::LIMB_ENUM aType);
	// Adds apPart to the form.
	void AddBodyPart(BGSBodyPart* apPart);
	BGSBodyPart** GetPartArray() { return pPartArray; }
	BGSBodyPart* GetBodyPart_005E5130(ActorValue::Index aeIndex);
	void HideChildLimbs(NiAVObject* apParentObj, NiAVObject* apRootObj, BGSBodyPart::LIMB_ENUM aeParent,
		NiAVObject* apObj);

	static void GenerateDefaultBodyPartData();

	// the parts by limb slot, then the ragdoll data
	BGSBodyPart* pPartArray[15];
	BGSRagdoll* pRagdollData;

	// The body part data of actors that name none.
	static inline BGSBodyPartData* pDefaultBodyPartData;
	static inline BGSBodyPartData* pPlayerBodyPartData;
};
