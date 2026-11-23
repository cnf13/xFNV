#pragma once

#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class TESObjectREFR;
class TESObjectSTAT;

// A pathing avoid area the AI keeps for a while: the avoid node, when it expires and, for a radiation source, the
// marker and the level it avoids.
struct AvoidAreaStruct {
    AvoidAreaStruct();

    bool LoadGameBGS(BGSLoadFormBuffer* apLoadGameBuffer);
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    // Saves the avoid node, fTimeExpire, afRadiationLevel and the two forms' IDs; HighProcess::SaveGame calls it on
    // each pListOfAvoidAreas item.
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);

    PathingAvoidNode pAvoidNode;
    float fTimeExpire;
    TESObjectSTAT* pRadiationMarker;
    float afRadiationLevel;
    TESObjectREFR* pRefObj;
};
