#pragma once

#include "BSShader/BSFadeNode.h"
#include "BSMain/IOManager.h"
#include "Fallout Shared/ModelLoader.h"
#include "Reconstruction.hpp"

class TESObjectTREE;

// A queued load of a tree's model for a reference.
class QueuedTreeModel : public QueuedModel {
public:
	QueuedTreeModel(TESObjectREFR* apRef, TESObjectTREE* apTree, IO_TASK_PRIORITY aePriority,
		ENUM_LOD_MULT aeLODFadeMult);

	void Run() override;
	void Finish() override;
	void Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) override;
	bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;
	void QueueMe() override;
	~QueuedTreeModel() override;

	TESObjectREFR* pRef;
	TESObjectTREE* pTree;
};
inline constexpr fnvd::Vtable QueuedTreeModel_vftable{};
