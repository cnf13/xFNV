#pragma once

// The product key codes the game's keycode.cxx defines from its KeyCode.h macros, read by the products'
// ProcessFlyingColors checks.
using HkAnimationKeyCodeText = const char[77];
inline HkAnimationKeyCodeText HK_ANIMATION_KEYCODE{
    "0x54a14fd9-0x1db7f331:2011-10-29.Animation.Obsidian_Bethesda_FalloutNewVegas"
};

using HkPhysicsKeyCodeText = const char[75];
inline HkPhysicsKeyCodeText HK_PHYSICS_KEYCODE{
    "0x72a843c2-0x3bbeff2a:2011-10-29.Physics.Obsidian_Bethesda_FalloutNewVegas"
};
