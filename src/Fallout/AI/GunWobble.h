#pragma once

#include "Fallout Shared/TESAnimGroup.h"
#include "Reconstruction.hpp"

class NiMatrix3;
class NiNode;

// The wobble of weapon type aeWeaponType at afTime into apReturnWobble; the name of the node
// it moves, or null.
char* GetGunWobble(ANIM_GROUP_WEAPON_ENUM aeWeaponType, float afTime, NiMatrix3& apReturnWobble);
// cdecl.
void ClearGunWobble(ANIM_GROUP_WEAPON_ENUM aeWeaponType, NiNode* apRoot);
// Loads the wobble model of each weapon kind (the scope wobble for kind 0), keeps its root's name as the wobbled
// bone's name and renames the root to the model's path.
void InitGunWobble();
void FreeGunWobble();

using WobbleBoneNameTable = char* [12];
inline WobbleBoneNameTable pWobbleBoneName{};
// The wobble model of each weapon kind, null when it has none.
using WobbleNodeTable = NiNode* [12];
inline WobbleNodeTable pWobbleNode{};
// Whether loaded models are renamed; cleared while the wobble models load.
inline bool bRenameModel = true;
