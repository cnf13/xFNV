#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"

#include <stdint.h>

class NiNode;
class TESForm;
class TESObjectREFR;
class TEX_USER_DATA;

// Which forms use which textures, collected over the loaded area to total up the texture memory they take.
class BGSTextureUseMap {
public:
    BGSTextureUseMap();
    ~BGSTextureUseMap();

    // Collects the texture use of ap3D and its children.
    void CollectTexUseDataRecursive(NiAVObject* ap3D, bool abCheckActors);
    // Records apForm as a user of apTex, whose size is aulMemorySize.
    void AddTexUse(NiTexture* apTex, uint32_t aulMemorySize, TESForm* apForm);
    void CollectTexUseData(NiNode* apNode, TESForm* apDefaultUser, bool abCheckActors);
    void CollectSceneTexUseData(bool abCheckActors);
    uint32_t GetTotalTextureSize();
    void ClearAllTexUseData();
    bool WriteToFile(const char* apFilename);

    static void TexUseCB(NiTexture* apTexture, uint32_t aulMemorySize, void* apParam);

    TESForm* pDefaultUser;
    // Texture to the TEX_USER_DATA that holds its size and the users of it.
    NiTMap<NiTexture*, TEX_USER_DATA*> UserMap;
};

inline TESObjectREFR* pactiveref;
