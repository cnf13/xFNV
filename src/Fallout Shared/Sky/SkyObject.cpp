#include "Fallout Shared/Sky/SkyObject.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSTaskManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

SkyObject::SkyObject() : spRoot(nullptr) {}

SkyObject::~SkyObject() {}

// The engine's SkyObject::Initialize has SEH and creates the sky root object.
// The exe's cleanup-only C++ EH frame is left out.
void SkyObject::Initialize(NiNode* apSkyRoot) {
	NiNode* NiNode_;
	NiNode* apMemory = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
	if (apMemory) {
		NiNode_ = ::new (apMemory) NiNode(0);
	} else {
		NiNode_ = 0;
	}
	spRoot.operator=(NiNode_);
	spRoot.m_pObject->SetSelectiveUpdate(1);
	spRoot.m_pObject->SetDisplayObject(1);
	apSkyRoot->AttachChild(spRoot.m_pObject, 1);
}

void SkyObject::operator delete(void* apMemory) {
	ourFree(apMemory);
}
