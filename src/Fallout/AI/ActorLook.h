#pragma once

#include "BSMain/Setting.h"
#include "Reconstruction.hpp"

// The PDB's actor look (a mouse spring turning an actor's look at a point). The PC exe keeps none of its code or
// instances, only its setting, so only the setting is declared.
class ActorLook {
public:
	// Whether actors look with Havok (IDA: g_bActorLookWithHavok:General).
	static SettingT<INISettingCollection> bActorLookWithHavok;
};
