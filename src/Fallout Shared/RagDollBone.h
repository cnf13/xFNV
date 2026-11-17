#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// The saved pose of one bone of a rag doll.
struct RagDollBone {
	// Swaps the position and rotation read from a big-endian file, in field order; the bone ID is left alone.
	void Endian();

	char iBoneID;
	uint8_t padding01[3];
	NiPoint3 BonePos;
	NiPoint3 BoneRot;
};
