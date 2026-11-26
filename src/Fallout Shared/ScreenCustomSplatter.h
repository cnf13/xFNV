#pragma once

#include "BSCore/BSRandom.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class NiCamera;
class NiNode;
class NiTexture;

// Its one caller, Script::SetScreenSplatterFade, passes a bool that it stores in ScreenCustomSplatter::bNoFade.
char SetCustomSplatterFade(float afFadeStart, bool abNoFade);
void CustomBloodSplatter();

// The kind of custom splatter; Activate takes it and does not read it.
enum ScreenCustomSplatterType : int32_t {
    SCREENSCUSTOMPLATTER_BLOOD = 0,
    SCREENSCUSTOMPLATTER_BLOOD2 = 1,
    SCREENSCUSTOMPLATTER_BLOOD3 = 2,
    SCREENSCUSTOMPLATTER_COUNT = 3,
};

class ScreenCustomSplatter {
public:
    using spTextures_t = NiPointer<NiTexture>[3];

    static void Update();
    // Starts a splatter of auiCount drops of type aeType.
    static void Activate(
        uint32_t auiCount,
        float afDuration,
        ScreenCustomSplatterType aeType,
        float fSplatScale,
        float fOpacityScale
    );
    static void Init();
    static NiCamera* GetCamera();
    static bool IsActiveSplatter();
    static NiNode* GetCurrentRoot();
    static NiNode* CreateGeometry(uint32_t auiCount, float fSplatScale, float fOpacityScale);
    static char SetCustomSplatterTexture(char* apFilename, uint8_t aucIndex);
    static void Shutdown();

    static NiPointer<NiNode> spRoot;
    static NiPointer<NiCamera> spCamera;
    static inline int32_t uiCount;
    static inline bool bEnabled;
    static inline bool bNoFade;
    // Holds the camera frustum's m_fRight, as ScreenSplatter's does.
    static inline float fWRadius = 0.5f;
    // Holds the camera frustum's m_fTop.
    static inline float fHRadius = 0.5f;
    static spTextures_t spTextures;
};

extern SettingT<INISettingCollection> bCustomSplatterEnabled;
inline float fBloodSplatterFadeStart_0;
