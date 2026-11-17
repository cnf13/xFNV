#pragma once

#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Reconstruction.hpp"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class TESObjectREFR;
class TESObjectSTAT;

// A pathing avoid area the AI keeps for a while: the avoid node, when it expires and, for a radiation source, the
// marker and the level it avoids. The PDB's source module is HighProcess.cpp.
struct AvoidAreaStruct {
	AvoidAreaStruct();

	// Inferred type: arg0: BGSLoadFormBuffer* from 0x8fd8ec (parameter 0 of HighProcess::LoadGame)
	bool LoadGameBGS(BGSLoadFormBuffer* arg0);
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	// Saves the avoid node, fTimeExpire, afRadiationLevel and the two forms' IDs (thiscall,
	// ret 4, 0x8D73B0; HighProcess::SaveGame calls it on each pListOfAvoidAreas item).
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);

	PathingAvoidNode pAvoidNode;
	float fTimeExpire;
	TESObjectSTAT* pRadiationMarker;
	float afRadiationLevel;
	TESObjectREFR* pRefObj;
};
