#include "Fallout Shared/BGSDecalNode.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderProperty.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout Shared/TempEffects/BSTempEffectGeometryDecal.h"
#include "Fallout/AI/FixedStrings.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

BSSimpleList<BGSDecalNode*> BGSDecalNode::kDecalNodes;
NiRTTI BGSDecalNode::ms_RTTI("BGSDecalNode", &NiNode::ms_RTTI);

BGSDecalNode::BGSDecalNode() : NiNode(0) {
	Init();
	BGSDecalNode::kDecalNodes.AddHead(this);
}

BGSDecalNode::BGSDecalNode(NiNode* apNode, bool abSkinnedNode) : NiNode(0), bSkinnedNode(abSkinnedNode) {
	Init();
	apNode->AttachChild(this, true);
	NiUpdateData kUpdateData(0.0f, false, false);
	apNode->Update(kUpdateData);
	BGSDecalNode::kDecalNodes.AddHead(this);
}

// The exe calls NiNode's AttachChild rather than jumping to it, with abFirstAvail's low byte zero-extended
// (0x4EE4D7).
[[clang::disable_tail_calls]] void BGSDecalNode::AttachChild(NiAVObject* apChild, int32_t abFirstAvail) {
	NiNode::AttachChild(apChild, static_cast<uint8_t>(abFirstAvail));
}

// The exe's cleanup-only C++ EH frame is left out.
NiObject* BGSDecalNode::CreateClone(NiCloningProcess& arCloneData) {
	BGSDecalNode* v4 = static_cast<BGSDecalNode*>(NiMemObject::operator new(sizeof(BGSDecalNode)));
	if (v4) {
		return ::new (v4) BGSDecalNode;
	} else {
		return 0;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void BGSDecalNode::OnVisible(NiCullingProcess& arCuller) {
	if (IsVisualObject()) {
		BSSimpleList<NiPointer<NiRefObject>>* plist = pDecalList;
		while (plist && !plist->IsEmpty()) {
			BSTempEffectGeometryDecal* v2 = static_cast<BSTempEffectGeometryDecal*>(plist->m_item.m_pObject);
			FNVD_CONSTRUCTED_LOCAL(NiPointer<NiAVObject>, pdecal, &NiPointer<NiAVObject>::Destruct, v2->Get3D());
			if (pdecal.m_pObject) {
				pdecal.m_pObject->Cull(arCuller);
			}
			plist = plist->m_pkNext;
		}
	}
}

void BGSDecalNode::AttachChild(BSTempEffect* apDecal, bool bFirstAvail) {
	NiAVObject* v3 = apDecal->Get3D();
	BSShaderProperty* pprop = static_cast<BSShaderProperty*>(v3->GetProperty(3));
	if (pprop) {
		pprop->SetDepthBias(1.0f);
	}
	NiAVObject* v4 = apDecal->Get3D();
	// The exe passes bFirstAvail's raw byte, zero-extended (0x4EE489); read through a byte pointer, since clang would
	// reduce a bool read to 0 or 1 (declarations.md "Parameter widths").
	NiNode::AttachChild(v4, static_cast<uint8_t>(bFirstAvail));
}

BGSDecalNode::~BGSDecalNode() {
	BGSDecalNode::kDecalNodes.Remove(this);
	if (pDecalList) {
		while (!pDecalList->IsEmpty()) {
			NiRefObject* pDecal = pDecalList->GetItem().operator->();
			// The node's decal list holds plain references; every decal in it is a temp effect.
			BSTempEffect::RemoveDecalFromList(static_cast<BSTempEffect*>(pDecal));
			pDecalList->RemoveHead();
		}
		delete pDecalList;
		pDecalList = nullptr;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
// The exe's cleanup-only C++ EH frame is left out.
void BGSDecalNode::RemoveFromDecalList(BSTempEffect* apDecal) {
	FNVD_CONSTRUCTED_LOCAL(NiPointer<NiRefObject>, apMemory, &NiPointer<NiRefObject>::Destruct,
		static_cast<NiRefObject*>(apDecal));
	pDecalList->Remove(apMemory);
}

// The exe's cleanup-only C++ EH frame is left out.
void BGSDecalNode::DetachChild(BSTempEffect* apDecal) {
	FNVD_CONSTRUCTED_LOCAL(NiPointer<NiAVObject>, spobject, &NiPointer<NiAVObject>::Destruct,
		static_cast<NiAVObject*>(0));
	DetachChild(apDecal, spobject);
}

void BGSDecalNode::DetachChild(BSTempEffect* apDecal, NiPointer<NiAVObject>& aspAVObject) {
	RemoveFromDecalList(apDecal);
	NiAVObject* v3 = apDecal->Get3D();
	NiNode::DetachChild(v3, aspAVObject);
}

// Gives the node an empty decal list and its node name. The exe's cleanup-only EH frame (freeing the list's memory
// when the constructor throws) is left out.
void BGSDecalNode::Init() {
	void* pMemory = ourMalloc(sizeof(DecalList));
	DecalList* pList = pMemory ? DecalList_Construct(pMemory) : nullptr;
	pDecalList = pList;
	if (bSkinnedNode) {
		SetName(FixedStrings::QSkinnedDecalNode());
	} else {
		SetName(FixedStrings::QDecalNode());
	}
}

void BGSDecalNode::RemoveDecals() {
	if (pDecalList) {
		pDecalList->RemoveAll();
	}
	for (uint32_t i = 0; i < GetArrayCount(); ++i) {
		DetachChildAt(i);
	}
}

BGSDecalNode::DecalList* BGSDecalNode::FreeDecalList(BGSDecalNode::DecalList* apList, uint32_t auiFlags) {
	apList->~BSSimpleList<NiPointer<NiRefObject>>();
	if ((auiFlags & 1) != 0) {
		ourFree(apList);
	}
	return apList;
}

void BGSDecalNode::RemoveAllDecals() {
	for (BGSDecalNode* v0 : BGSDecalNode::kDecalNodes) {
		if (v0) {
			v0->RemoveDecals();
		}
	}
}

// The decal list's own copy of the list constructor (thiscall, ret 0, 0x4EE810): the item through NiPointer's
// folded constructor 0x633C90, then the link; src's BSSimpleList::Construct forwards to the pointer instantiations'
// body 0x96A2D0.
BGSDecalNode::DecalList* DecalList_Construct(void* apMemory) {
	BGSDecalNode::DecalList* pThis = static_cast<BGSDecalNode::DecalList*>(apMemory);
	::new (&pThis->m_item) NiPointer<NiRefObject>(nullptr);
	pThis->m_pkNext = nullptr;
	return pThis;
}

// The decal is removed by value: the exe builds a smart pointer for it, removes that from the list, releases the
// pointer and counts the decal out.
void BSTempEffect::RemoveDecalFromList(BSTempEffect* apDecal) {
	// The exe releases the pointer before it counts the decal out: the block ends its scope there.
	{
		NiPointer<NiRefObject> spDecal(apDecal);
		BSTempEffect::kDecals.Remove(spDecal);
	}
	--BSTempEffect::iNumDecals;
}

NiFixedString& FixedStrings::QDecalNode() {
	return *Decal_Node;
}
