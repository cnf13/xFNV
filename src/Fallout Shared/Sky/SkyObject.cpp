#include "Fallout Shared/Sky/SkyObject.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/BSTaskManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"

SkyObject::SkyObject() : spRoot(nullptr) {}

SkyObject::~SkyObject() {}

// The engine's SkyObject::Initialize has SEH and creates the sky root object.
void SkyObject::Initialize(NiNode* apSkyRoot) {
    NiNode* NiNode_;
    NiNode_ = new NiNode(0);
    spRoot = NiNode_;
    spRoot->SetSelectiveUpdate(true);
    spRoot->SetDisplayObject(true);
    apSkyRoot->AttachChild(spRoot.m_pObject, 1);
}

void SkyObject::operator delete(void* apMemory) {
    ourFree(apMemory);
}
