#pragma once

#include "BSMain/Setting.h"

// Actor look (a mouse spring turning an actor's look at a point). Only its setting is declared.
class ActorLook {
public:
	static SettingT<INISettingCollection> bActorLookWithHavok;
};
