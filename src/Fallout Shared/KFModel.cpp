#include "Fallout Shared/KFModel.h"

#include "Fallout Shared/Animation.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"

// The sequence is released before the anim group, as the exe does; the pointers' destructors then find them null.
KFModel::~KFModel() {
	ourFree(pFilename);
	spSequence = nullptr;
	spAnimGroup = nullptr;
}

void KFModel::DecRefCount() {
	NiAtomicDecrement(iRefCount);
}

int32_t KFModel::GetRefCount() const {
	return iManualRefCount + iRefCount;
}

TESAnimGroup* KFModel::GetAnimGroup() const {
	return spAnimGroup;
}

// spSequence's object.
NiControllerSequence* KFModel::GetSequence() {
	return spSequence;
}

void KFModel::IncManualRefCount() {
	NiAtomicIncrement(iManualRefCount);
}

void KFModel::DecUseCount() {
	if (iManualRefCount > 0) {
		NiAtomicDecrement(iManualRefCount);
	}
}

void KFModel::operator delete(void* apMemory) {
	ourFree(apMemory);
}
