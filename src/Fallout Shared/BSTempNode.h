#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Reconstruction.hpp"

// A scene graph node with a time stamp (fTime), for the temporary nodes the game creates. The PDB lists the class
// under TES.cpp, whose header does not declare it.
class BSTempNode : public NiNode {
public:
	BSTempNode(float afTime);
	~BSTempNode() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }

	float fTime;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable BSTempNode_vftable{};
