#pragma once

#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSMultiBoundSphere.h"
#include "BSShader/BSFadeNodeCuller.h"
#include "Fallout/AI/ProcessLists.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

// Walks the high process actors and culls the ones outside the view, through a multibound sphere.
class HighActorCuller : public MobIterOperator {
public:
	HighActorCuller();
	~HighActorCuller();

	void Operate(MobileObject* apMob) override;

	void Init(NiCamera* apCamera);
	void Run(NiCamera* apCamera);

	BSFadeNodeCuller Culler;
	NiPointer<BSMultiBoundNode> spMBNode;
	NiPointer<BSMultiBound> spMB;
	NiPointer<BSMultiBoundSphere> spMBSphere;
};
inline constexpr fnvd::Vtable HighActorCuller_vftable{};
