#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiCamera;
class NiNode;
class NiTexture;

// The kind of blood splatter; Activate takes it and does not read it.
enum ScreenSplatterType : int32_t {
    SCREENSPLATTER_BLOOD = 0,
    SCREENSPLATTER_BLOOD2 = 1,
    SCREENSPLATTER_BLOOD3 = 2,
    SCREENSPLATTER_COUNT = 3,
};

// Only some of the class's members are declared.
class ScreenSplatter {
public:
    using spTextures_t = NiPointer<NiTexture>[3];

    // Starts auiCount splatters of type aeType for afDuration seconds.
    static void Activate(uint32_t auiCount, float afDuration, ScreenSplatterType aeType);
    // Removes every splatter from the screen while splatters are enabled.
    static void Clear();
    static bool IsActiveSplatter();
    static NiCamera* GetCamera();
    static NiNode* GetRoot();
    // Advances the splatters.
    static void Update();
    static void Init();
    // Releases the splatter scene and its textures.
    static void Shutdown();
    static NiNode* CreateGeometry(uint32_t auiCount);

    // Set while the screen splatter scene is drawn: NiDX9Renderer::Do_SetCameraData then keeps the camera location in
    // the view matrix instead of moving it into NiRenderer::PosAdjust. A PC-only static.
    static inline bool bResetCameraPos = true;
    static inline bool bEnabled;
    static NiPointer<NiNode> spRoot;
    static NiPointer<NiCamera> spCamera;
    static inline uint32_t uiCount;
    static inline float fWRadius = 0.5f;
    static inline float fHRadius = 0.5f;
    static spTextures_t spTextures;
};

extern SettingT<INISettingCollection> bBloodSplatterEnabled;
