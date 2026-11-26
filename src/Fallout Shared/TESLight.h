#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class ExtraTimeLeft;
class LightRefData;
class NiLight;
class TESSound;

void SetLightAttenuation(NiLight* apLight, uint32_t aiRadius);
NiLight* FindLight(NiNode* apNode);
bool GetAttenuationSettings();

// The light flags (OBJ_LIGH::flags).
enum TES_LIGHT_FLAGS : int32_t {
    LF_DYNAMIC = 0x1,
    LF_CAN_CARRY = 0x2,
    LF_NEGATIVE = 0x4,
    LF_FLICKER = 0x8,
    LF_DEEP_COPY = 0x10,
    LF_OFF_BY_DEFAULT = 0x20,
    LF_FLICKER_SLOW = 0x40,
    LF_PULSE = 0x80,
    LF_PULSE_SLOW = 0x100,
    LF_SPOT_LIGHT = 0x200,
    LF_SPOT_SHADOW = 0x400,
    LF_RADIUS = -0x7FFFFFFF - 1,
};

// The DATA chunk of a light.
struct OBJ_LIGH {
    // Byte-swaps the chunk.
    void Endian();

    int32_t time;
    uint32_t radius;
    uint32_t color;
    uint32_t flags;
    float fFallOffExponent;
    float fFOV;
};

// The light attenuation settings. cUsageFlags selects the constant, linear and quadratic terms.
namespace ATTENUATION_SETTINGS {
    enum USAGE_FLAGS : uint8_t {
        USE_CONSTANT = 0x1,
        USE_LINEAR = 0x2,
        USE_QUADRATIC = 0x4,
    };

    inline uint8_t cUsageFlags;
    // The [LightAttenuation] ini settings: each is constructed by SettingT<INISettingCollection>'s constructors and
    // destroyed by its destructor at exit.
    extern SettingT<INISettingCollection> eLinearMethod;
    extern SettingT<INISettingCollection> eQuadraticMethod;
    extern SettingT<INISettingCollection> fLinearValue;
    extern SettingT<INISettingCollection> fQuadraticValue;
    extern SettingT<INISettingCollection> fConstantValue;
    extern SettingT<INISettingCollection> bOutQuadInLin;
    extern SettingT<INISettingCollection> fLinearRadiusMult;
    extern SettingT<INISettingCollection> fQuadraticRadiusMult;
} // namespace ATTENUATION_SETTINGS

// A light base form (LIGH).
class TESObjectLIGH : public TESBoundAnimObject,
                      public TESFullName,
                      public TESModelTextureSwap,
                      public TESIcon,
                      public BGSMessageIcon,
                      public TESScriptableForm,
                      public TESWeightForm,
                      public TESValueForm,
                      public BGSDestructibleObjectForm {
public:
    TESObjectLIGH();
    ~TESObjectLIGH() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    // GetSaveSize, SaveGame(uint32_t) and LoadGame(uint32_t, uint32_t) also override TESValueForm's
    uint16_t GetSaveSize(uint32_t aiFlags) override;
    void SaveGame(uint32_t aiFlags) override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitItem() override;
    NiColor* GetEmittanceColor() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    bool Activate(
        TESObjectREFR* apItemActivated,
        TESObjectREFR* apActionRef,
        bool abIdFlag,
        TESBoundObject* apObjecttoGet,
        int32_t aiCount
    ) override;
    void UnClone3D(TESObjectREFR* pRequester) override;
    NiAVObject* LoadGraphics(TESObjectREFR* apRef) override;
    NiAVObject* Clone3D(TESObjectREFR* pRequester) override;
    using TESBoundObject::Clone3D;
    using TESForm::Save;

    // Turns the base model into the light's node tree.
    NiLight* GenDynamic(TESObjectREFR* pRequester, NiAVObject* pModel, bool abForceDynamic);
    void SetSound(TESSound* pSound);
    bool GetFlicker();
    bool GetFlickerSlow();
    bool GetPulse();
    bool GetPulseSlow();
    // Whether the light is dynamic and whether it can be carried.
    bool GetDynamic();
    bool GetCanCarry();
    // FixGravity is empty on the PC build.
    void FixGravity(const NiPoint3& angle);
    // Whether the light's flicker flags are all clear.
    bool GetNoFlicker();
    // The light's sound (m_pSound).
    TESSound* GetSound();
    // The light's radius (data's uiRadius).
    uint32_t GetRadius();
    // The light's color, red in the low byte (data's color).
    uint32_t GetColor();
    // The light's fade (fFade).
    float GetFade();
    void SetFade(float afVal);
    // Applies the light's data to apLight's light.
    bool Update(LightRefData* apLight, ExtraTimeLeft* apExtra);
    // Animates aspLight's light by the flicker and pulse flags, writes *apWantDimmer, reads apExtra's time left, and
    // returns the killTheLight flag. The smart pointer is taken by value and released.
    bool Update(NiPointer<NiLight> aspLight, float* apWantDimmer, ExtraTimeLeft* apExtra);
    bool GetOffByDefault();
    void SetDynamic(bool abVal);

    static void StopParticlesRecursive(NiNode* pNode);

    OBJ_LIGH data;
    float fFade;
    TESSound* m_pSound;
    NiColor EmittanceColor;
};

extern SettingT<INISettingCollection> fFlickerMovement;
