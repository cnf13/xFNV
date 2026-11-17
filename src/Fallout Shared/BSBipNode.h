#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Reconstruction.hpp"

// The root node of an animated skeleton, which updates its bip (skeleton) nodes apart from the rest of the graph.
// Only the members the reconstructed code calls are declared.
class BSBipNode : public NiNode {
public:
	// Updates every child but the bip nodes at afTime.
	void UpdateAllButBip(float afTime);
	// Removes apObject from controller targets.
	void RemoveObject(NiAVObject* apObject);
	// Updates the bip nodes at afTime around the accumulation root apAccumRoot.
	void UpdateBipOnly(float afTime, NiAVObject* apAccumRoot);
};
