#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESBipedModelForm.h"

class BSShaderPPLightingProperty;
class QueuedFile;
class QueuedHelmet;
class TESNPC;
class TESObjectWEAP;
class TESRace;

int32_t BSstrlwr(char* apString1, int32_t aiSize);

enum BIPED_BONE : int32_t {
	BIPED_BONE_NONE = -1,
	BIPED_BONE_HEAD = 0,
	BIPED_BONE_WEAPON = 1,
	BIPED_BONE_PIP_BOY = 2,
	BIPED_BONE_BACKPACK = 3,
	BIPED_BONE_NECKLACE = 4,
	BIPED_BONE_COUNT = 5,
};

// A bone of the biped skeleton: bit 1 of cFlags is set when the skeleton has it.
struct BIPBONE {
	uint8_t cFlags;
	uint8_t padding01[0x3];
	NiNode* pParent;
};

// A worn object of the biped: the form that owns it, its model and the loaded 3D.
struct BIPOBJECT {
	TESForm* pParent;
	TESModel* pPart;
	NiAVObject* pPartClone;
	bool bSkinned;
	uint8_t padding0D[0x3];
};

// The biped parts (bones and worn objects) of an actor's 3D.
class BipedAnim {
public:
	BipedAnim(TESObjectREFR* apRef, NiNode* apNode);
	~BipedAnim();

	// Loads the 3D of the worn parts, now when abForceLoadImmediate.
	void LoadBipedParts(bool abForceLoadImmediate);
	NiNode* GetRoot();
	// Finds the root and the bones of the skeleton under apNode.
	void InitRoot(NiNode* apNode);
	// Removes every worn and buffered object.
	void RemoveAllParts();
	// Removes the worn weapon's 3D.
	void RemoveBipedWeapon();
	// Wears apWeapon (with the mods iModSlots) and attaches its 3D.
	void AttachBipedWeapon(TESObjectWEAP* apWeapon, uint8_t iModSlots);
	// Removes every worn object owned by apObj.
	void RemovePart(TESForm* apObj);
	// Removes the worn object in slot aeObj; abClear also forgets its owner and stores aiClearValue as its model.
	void RemovePart(BIPED_OBJECT aeObj, bool abClear, int32_t aiClearValue);
	// Detaches and releases apObj's 3D; abClear also forgets its owner and stores aiClearValue as its model.
	void RemovePart(BIPOBJECT* apObj, bool abClear, int32_t aiClearValue);
	// Drops the weapon bone's decal placement vector extra data for apRoot's geometry, or walks apRoot's children.
	void RecursivelyRemoveAllDecalPlacementVectorDataFromWeaponBone(NiAVObject* apRoot);
	// The skeleton node of bone aeType.
	NiNode* GetParentBone(BIPED_BONE aeType);
	// The form owning the worn object in slot aeBipedObject.
	TESForm* GetParent(BIPED_OBJECT aeBipedObject);
	BIPOBJECT* GetBipObject(BIPED_OBJECT aeBipedObject);
	// The slot whose worn object apObj owns; null when none does.
	BIPOBJECT* FindBipObject(TESForm* apObj);
	// The buffered objects.
	BIPOBJECT* QBufferedObjects();
	// Wears apWeapon (with the mods iModSlots), unless it is worn and loaded already.
	void SetBipedWeapon(TESObjectWEAP* apWeapon, uint8_t iModSlots);
	// The reference the biped belongs to.
	TESObjectREFR* GetRequester();
	// True when the model of the object in slot aeBipedObject has FaceGen data.
	bool IsFaceGenObject(BIPED_OBJECT aeBipedObject);
	// Queues the load of the helmet worn in aeBipedObject.
	void QueueHelmetLoad(IO_TASK_PRIORITY aePriority, QueuedFile* apParent, BIPED_OBJECT aeBipedObject);
	// Queues the load of the helmet worn in aeBipedObject unless aspQueuedHelmet holds one; aspQueuedHelmet receives
	// the queued load, or null when the slot has no model.
	void QueueHelmetLoad(IO_TASK_PRIORITY aePriority, QueuedFile* apParent, NiPointer<QueuedHelmet>& aspQueuedHelmet,
		BIPED_OBJECT aeBipedObject);
	// Moves the worn objects to the buffer (when it is empty) and wears apRace's naked body parts for aeSex in the
	// slots whose bone the skeleton has.
	void InitNaked(TESRace* apRace, TESActorBase::SEX aeSex);
	// Wears apModel of apParent in the slots apParent's biped model fills (in aeObjectSlot for a race part), first
	// removing the parts there; the player's first-person biped wears the "1st.nif" model when it exists.
	void SetBipedPart(TESForm* apParent, TESModel* apModel, BIPED_OBJECT aeObjectSlot);
	// Queues the loads of the worn models (the helmet into aspQueuedHelmet when FaceGen heads load in the
	// background), then of the skin textures their skin flags ask for.
	void QueueBipedParts(IO_TASK_PRIORITY aePriority, NiPointer<QueuedHelmet>& aspQueuedHelmet, QueuedFile* apParent);
	// Queues the requester NPC's skin textures of the slots in aiModelSkinFlags (2 upper body, 4 right hand, 8 left
	// hand) and its body mod texture.
	void QueueSkinTextures(uint32_t aiModelSkinFlags, IO_TASK_PRIORITY aePriority, QueuedFile* apParent);
	// Queues the load of apNpc's race body texture for aeBipedObject and of its "_n" normal map.
	void QueueSkinTexture(TESNPC* apNpc, BIPED_OBJECT aeBipedObject, IO_TASK_PRIORITY aePriority, QueuedFile* apParent);
	// Records whether apClone has skinned geometry and attaches it (each child of a fade node) to the skeleton; the
	// attached object, null when it is not skinned.
	NiAVObject* ApplySkinnedObjects(NiNode* apClone, BIPED_OBJECT aeBipedObject, bool abFirstPerson, bool abPipboy,
		BSFaceGenModel* apfgModel);
	// Attaches the skinned apNode under apParentNode.
	NiAVObject* AttachSkinnedObject(NiNode* apNode, NiNode* apParentNode, BIPED_OBJECT aeBipedObject,
		bool abFirstPerson, bool abPipboy, BSFaceGenModel* apfgModel);
	// The FaceGen model of the .egm file of the model worn in aeBipedObject; null when the slot has no model.
	BSFaceGenModel* LoadFaceGenModel(BIPED_OBJECT aeBipedObject);
	// Applies the requester's race body textures to apObj's skin shader: the base and FaceGen textures, the base's
	// "_n" normal map and the default detail modulation texture.
	void AdjustSkinComplexion(NiAVObject* apObj, bool abForceReadjust);
	// Writes apModel's path with "1st.nif" for its extension to ap1stPersonFileName (260 bytes); true when that
	// mesh exists.
	bool Get1stPersonModelFileName(TESModel* apModel, char* ap1stPersonFileName);
	// The shared first-person model of slot aeBipedObject, set to ap1stPersonFileName.
	TESModel* Store1stPersonModel(BIPED_OBJECT aeBipedObject, const char* ap1stPersonFileName);
	// The first-person model for apModel in slot aeBipedObject: the stored "1st.nif" model when that mesh exists,
	// else apModel.
	TESModel* Get1stPersonModel(BIPED_OBJECT aeBipedObject, TESModel* apModel);
	NiNode* CloneHelmet(BSFaceGenModel* apfgModel, NiNode* apNode, BIPED_OBJECT aeBipedObject);
	void AttachHelmet(BSFaceGenModel* apfgModel, NiNode* apNode, BIPED_OBJECT aeBipedObject);

	// Frees through ourFree.
	static void operator delete(void* apMemory);
	// Loads apModel of apObject and attaches it to apRequester's 3D in slot
	// aeAttachmentType.
	static NiAVObject* LoadAndAttachAddOn(TESForm* apObject, TESModel* apModel, BIPED_OBJECT aeAttachmentType,
		TESObjectREFR* apRequester, NiAVObject* apObj3D);
	// Detaches the part's 3D from the skeleton.
	static void RunBiped3DDetach(NiAVObject* ap3D);
	// Attaches the skinned apSkin to the skeleton apSkeleton.
	static void AttachToSkeleton(NiNode* apSkeleton, NiAVObject* apSkin, NiNode* apParentNode, bool abShowWarnings);
	// Attaches apNode under the skeleton apSkeleton. The body never reads
	// apfgModel; the callers push it.
	static void AttachToParent(NiNode* apSkeleton, NiNode* apNode, NiNode* apClonedFromNode, BipedAnim* apBipedAnim,
		BIPED_OBJECT aeBipedObject, BSFaceGenModel* apfgModel);
	// The skin slot apObj's name prefix stands for; BO_NONE without a FaceGen-blended lighting property.
	static BIPED_OBJECT GetSkinBipedObject(NiAVObject* apObj, BSShaderPPLightingProperty* apLightingProperty);
	// Attaches the add-on nodes under apNode when apClonedFrom's BSXFlags have the add-on flag.
	static void AddAddonNodes(NiNode* apClonedFrom, NiNode* apNode, BIPED_OBJECT aeAttachmentType,
		TESObjectREFR* apRequester);
	// apObj's shade property when apObj is a geometry with a per-pixel lighting shader (types 8 to 12), else null.
	static BSShaderPPLightingProperty* GetLightingProperty(NiAVObject* apObj);

	NiNode* root;
	BIPBONE bone[BIPED_BONE_COUNT];
	BIPOBJECT object[20];
	BIPOBJECT bufferedObjects[20];
	float m_fWeaponOffset;
	TESObjectREFR* m_pRequester;
};

// The names of the skeleton nodes of the BIPED_BONE bones.
using BipedBoneNameArray = const char* [BIPED_BONE_COUNT];
inline BipedBoneNameArray bipedBoneName{"Bip01 Head", "Weapon", "Bip01 L ForeTwist", "Bip01 Spine2", "Bip01 Neck1"};

// The game settings BipedAnim.cpp defines.
extern SettingT<GameSettingCollection> sHandToHandIcon;
extern SettingT<INISettingCollection> bLoadHelmetsInBackground;

// The skeleton bone of each biped slot, BIPED_BONE_NONE for a slot without one.
using objToBone_t = BIPED_BONE[BIPED_OBJECT_COUNT];
inline objToBone_t objToBone{BIPED_BONE_HEAD, BIPED_BONE_HEAD, BIPED_BONE_NONE, BIPED_BONE_NONE, BIPED_BONE_NONE,
	BIPED_BONE_WEAPON, BIPED_BONE_PIP_BOY, BIPED_BONE_BACKPACK, BIPED_BONE_NECKLACE, BIPED_BONE_HEAD, BIPED_BONE_HEAD,
	BIPED_BONE_HEAD, BIPED_BONE_HEAD, BIPED_BONE_HEAD, BIPED_BONE_HEAD, BIPED_BONE_HEAD, BIPED_BONE_HEAD,
	BIPED_BONE_NONE, BIPED_BONE_NONE, BIPED_BONE_NONE};
