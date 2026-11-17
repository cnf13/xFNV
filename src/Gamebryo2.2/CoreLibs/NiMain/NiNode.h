#pragma once

#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"

#include <stdint.h>

class NavMesh;
class TESObjectCELL;

// NiTArray<NiPointer<NiAVObject>>::Compact and ::UpdateSize, defined in NiNode.cpp.
void NiAVObjectArray_Compact(NiTObjectArray<NiPointer<NiAVObject>>* apArray);
void NiAVObjectArray_UpdateSize(NiTObjectArray<NiPointer<NiAVObject>>* apArray);

NiSmartPointer(NiNode);

// A scene graph object with children.
class NiNode : public NiAVObject {
public:
	NiNode(uint32_t auiNumChildren = 0);
	~NiNode() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiNode* IsNode() override { return this; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void ProcessClone(NiCloningProcess& kCloning) override;
	void SetBound(BSNiBound* apBound) override;
	void UpdateControllers(NiUpdateData& UpdateData) override;
	void ApplyTransform(const NiMatrix3& kMat, const NiPoint3& kTrn, bool bOnLeft) override;
	NiAVObject* GetObjectByName(const NiFixedString& kName) override;
	void SetSelectiveUpdateFlags(bool& bSelectiveUpdate, int32_t bSelectiveUpdateTransforms, bool& bRigid) override;
	void UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
	void UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
	void UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
	void UpdatePropertiesDownward(NiPropertyState* pkParentState) override;
	void UpdateWorldBound() override;
	void UpdateTransformAndBounds(NiUpdateData& UpdateData) override;
	void OnVisible(NiCullingProcess& kCuller) override;
	void PurgeRendererData(NiRenderer* apkRenderer) override;
	virtual void AttachChild(NiAVObject* pkChild, int32_t bFirstAvail);
	virtual void InsertChildAt(uint32_t i, NiAVObject* pkChild);
	virtual void DetachChild(NiAVObject* apChild);
	virtual void DetachChild(NiAVObject* apChild, NiPointer<NiAVObject>& arspAVObject);
	virtual void DetachChildAt(uint32_t auiIndex);
	virtual void DetachChildAt(uint32_t auiIndex, NiPointer<NiAVObject>& arspAVObject);
	virtual void SetAt(uint32_t auiIndex, NiAVObject* apChild);
	virtual void SetAt(uint32_t auiIndex, NiAVObject* apChild, NiPointer<NiAVObject>& arspAVObject);
	virtual void UpdateUpwardPass();

	const char* GetNameHandle() { return m_kName.GetHandle(); }
	uint32_t GetArrayCount() const;
	uint32_t GetChildCount() const;
	NiAVObject* GetAt(uint32_t i) const;
	NiAVObject* GetAtChecked(uint32_t i) const;
	// Detaches every child.
	void DetachAllChildren();
	// Reallocates the child array to its used size.
	void CompactChildArray();
	// Empties the child array, releasing every child.
	void RemoveAllChildren();
	// Resizes the child array to aiSize slots.
	void SetChildArraySize(uint32_t aiSize);
	void CopyMembers(NiNode* pkDest, NiCloningProcess& kCloning);
	// Rebuilds the property state from the parents down, then pushes this node's own properties onto it.
	void UpdatePropertiesUpward(NiPropertyState*& aspPropertyState);
	NiTObjectArray<NiPointer<NiAVObject>>* GetChildren();
	float GetLocalScale() const;
	char GetBlastNodePos(NiAVObject* arg0, NiPoint3* a2);

	// Whether the object named apString under apObject (apObject
	// itself when there is none) has a positive world rotation entry [2][2].
	static bool IsLeftFacingUp(NiAVObject* apObject, char* apString);
	// Gives apNode a wireframe no-lighting shader property and prepares it for rendering.
	static void SetWireframe(NiNode* apNode);
	static NiObject* CreateObject();

	NiTObjectArray<NiPointer<NiAVObject>> m_kChildren;

	static NiRTTI ms_RTTI;
};
