#pragma once

#include "Fallout Shared/FaceGen/NiGeometryDataIteratorNiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

class Actor;
class BSFaceGenBaseMorphExtraData;
class NiTriBasedGeom;
struct NiUpdateData;

// One head part a FaceGen head morphs in an update: its geometry, an iterator over its vertices, its morph data
// and base morph, and whether the base morph has been restored this update.
struct FaceGenUpdateData {
	FaceGenUpdateData();

	NiTriBasedGeom* ptri;
	NiGeometryDataIteratorNiPoint3 iter;
	BSFaceGenMorphDataHead* pmorph;
	BSFaceGenBaseMorphExtraData* pmorphBase;
	uint32_t ivertCount;
	bool bbaseApplied;
};

// The head node a FaceGen face hangs under: it carries the face's animation data and base rotation. Only the
// virtuals are declared; the overrides other than GetRTTI forward to the exe.
class BSFaceGenNiNode : public NiNode {
public:
	// Destroys the node in place: clears pActor, releases the animation data, then destroys the node's NiNode part
	// (thiscall, no ret: the exe calls NiNode::~NiNode, 0x662B00).
	BSFaceGenNiNode();
	~BSFaceGenNiNode() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
	void UpdateTransformAndBounds(NiUpdateData& UpdateData) override;
	void OnVisible(NiCullingProcess& arCuller) override;

	virtual BSFaceGenAnimationData* GetAnimationData();
	// The exe's body is the shared setter 0x439920 (thiscall, ret 4).
	virtual void SetAnimationData(BSFaceGenAnimationData* apData);
	// A copy of the base rotation (thiscall, ret 4 for the result pointer, 0x662A10).
	virtual NiMatrix3 GetBaseRotation();
	virtual void SetBaseRotation(const NiMatrix3& aRot);
	virtual bool GetAnimationUpdate();
	virtual void SetAnimationUpdate(bool abUpdate);
	virtual bool GetApplyRotToParent();
	// The exe's body is a shared byte setter (thiscall, ret 4, 0x97CE70).
	virtual void SetApplyRotToParent(bool abApply);
	virtual bool GetFixedNormals();
	virtual void SetFixedNormals(bool abFixedNormals);
	virtual void FixSkinInstances(NiNode* apRoot, bool abWarnOnFail);

	// Marks the head as the player character's.
	void SetIAmPlayerCharacter(bool abIAmPC);
	// bIAmPlayerCharacter; the exe's body is a byte getter shared with other classes
	// (thiscall, 0x9D7C60).
	bool GetIAmPlayerCharacter();
	// Morphs every child head part from the animation data; abChanged when a value changed.
	void UpdateAllChildrenMorphData(bool abChanged);
	// Applies the animation data's expressions, modifiers, phonemes and custom morphs in (0, 1] to arObjData's
	// geometry, restoring its base morph before the first one.
	void ApplyFacialExpressions(FaceGenUpdateData& arObjData);
	void ApplyFacialModifiers(FaceGenUpdateData& arObjData);
	void ApplyFacialPhonemes(FaceGenUpdateData& arObjData);
	void ApplyFacialCustomMorphs(FaceGenUpdateData& arObjData);
	// Switches the head to its LOD model by distance, or to aiForceLOD when not -1.
	void CheckLOD(BS_FACEGEN_LOD aiForceLOD);
	void SetForceBaseMorphs(bool abForceBaseMorph);
	void SetDisableMorphUpdates(bool abDisable);
	void SetIAmInDialogue(bool abIAmInDialogue);
	void SetForceMorphUpdates(bool abForce);
	void CopyMembers(BSFaceGenNiNode* apDest, NiCloningProcess& arCloneData);
	// Name not in the PDB (the field at +0xE0); folded accessor, IDA: MiddleHighProcess::GetIdleDoneOnce.
	bool GetUsingLoResHead();

protected:
	// Advances the animation data and the eye tracking, then morphs the children when something changed and the
	// head is close or talking to the player.
	void UpdateMorphing(NiUpdateData& UpdateData);

public:
	NiPointer<BSFaceGenAnimationData> spAnimationData;
	NiMatrix3 BaseRotation;
	bool bForceBaseMorph;
	bool bFixedNormals;
	bool bAnimationUpdate;
	bool bRotatedLastUpdate;
	bool bApplyRotationToParent;
	float fLastTime;
	bool bUsingLoResHead;
	bool bIAmPlayerCharacter;
	bool bIAmInDialogue;
	bool bDisableMorphUpdates;
	bool bForceMorphUpdates;
	Actor* pActor;
	bool bCycleFaceGenCoord;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(BSFaceGenNiNode);

inline constexpr fnvd::Vtable BSFaceGenNiNode_vftable{};
