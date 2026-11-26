#include "Fallout Shared/TESImageSpaceModifier.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceManager.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Region System/TESRegionList.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinColorKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

#include <cstdint>

TESImageSpaceModifier TESImageSpaceModifier::Default;

// Swaps the fields read from a big-endian file: the duration, every key count, then the rest. The flag bytes and the
// depth of field mode need no swap.
void TESImageSpaceModifier::ImageSpaceModifierData::Endian() {
    BSEndian(fDuration, false);
    for (int32_t i = 0; i < NUM_OP_PARAMS; ++i) {
        for (int32_t j = 0; j < IMAOP_COUNT; ++j) {
            BSEndian(ppuiKeySize[i][j], false);
        }
    }
    BSEndian(uiBlurKeySize, false);
    BSEndian(uiDoubleKeySize, false);
    BSEndian(uiTintColorKeySize, false);
    BSEndian(uiFadeColorKeySize, false);
    BSEndian(uiRadialBlurStrengthKeySize, false);
    BSEndian(uiRadialBlurRampupKeySize, false);
    BSEndian(uiRadialBlurStartKeySize, false);
    BSEndian(uiRadialBlurRampDownKeySize, false);
    BSEndian(uiRadialBlurDownStartKeySize, false);
    BSEndian(RadialBlurCenter.x, false);
    BSEndian(RadialBlurCenter.y, false);
    BSEndian(uiDepthOfFieldStrengthKeySize, false);
    BSEndian(uiDepthOfFieldDistanceKeySize, false);
    BSEndian(uiDepthOfFieldRangeKeySize, false);
    BSEndian(uiMotionBlurStrengthKeySize, false);
}

// The interpolator arrays and members are built with no data.
TESImageSpaceModifier::TESImageSpaceModifier()
  : TESForm(), BlurInterpolator(static_cast<NiFloatData*>(nullptr)),
    DoubleInterpolator(static_cast<NiFloatData*>(nullptr)), TintColorInterpolator(static_cast<NiColorData*>(nullptr)),
    FadeColorInterpolator(static_cast<NiColorData*>(nullptr)),
    RadialBlurStrengthInterpolator(static_cast<NiFloatData*>(nullptr)),
    RadialBlurRampupInterpolator(static_cast<NiFloatData*>(nullptr)),
    RadialBlurStartInterpolator(static_cast<NiFloatData*>(nullptr)),
    RadialBlurRampDownInterpolator(static_cast<NiFloatData*>(nullptr)),
    RadialBlurDownStartInterpolator(static_cast<NiFloatData*>(nullptr)),
    DepthOfFieldStrengthInterpolator(static_cast<NiFloatData*>(nullptr)),
    DepthOfFieldDistanceInterpolator(static_cast<NiFloatData*>(nullptr)),
    DepthOfFieldRangeInterpolator(static_cast<NiFloatData*>(nullptr)),
    MotionBlurStrengthInterpolator(static_cast<NiFloatData*>(nullptr)) {
    cFormType = IMAD_ID;
    TESImageSpaceModifier::InitializeData();
    if (this == &TESImageSpaceModifier::Default) {
        SetTemporary();
    }
}

TESImageSpaceModifier::~TESImageSpaceModifier() {
    TESImageSpaceModifier::ClearData();
}

// Every parameter gets a two-key linear curve at the default key times: the operation's default value, 0 for the named
// float effects, (1, 1, 1, 0) for the colors. Each key array is a new[] of two keys.
void TESImageSpaceModifier::InitializeData() {
    ourMemset(&Data, 0, sizeof(Data));
    Data.bAnimatable = false;
    Data.fDuration = 1.0f;
    Data.bUseTargetForRadialBlur = false;
    Data.RadialBlurCenter = NiPoint2(0.5f, 0.5f);
    Data.bUseTargetForDepthOfField = false;
    Data.ucDepthOfFieldMode = 0;

    int32_t i;
    for (i = 0; i < NUM_OP_PARAMS; ++i) {
        for (int32_t j = 0; j < IMAOP_COUNT; ++j) {
            Data.ppuiKeySize[i][j] = 2;
            pppFloatKey[i][j] = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(2);
            for (int32_t k = 0; k < 2; ++k) {
                pppFloatKey[i][j][k].SetTime(fDefaultKeyTimes[k]);
                pppFloatKey[i][j][k].SetValue(fOpDefault[j]);
            }
            ppInterpolator[i][j].ReplaceKeys(pppFloatKey[i][j], Data.ppuiKeySize[i][j], NiAnimationKey::LINKEY);
        }
    }

    Data.uiTintColorKeySize = 2;
    pTintColorKey = NiAnimationKey::AllocateKeyArray<NiLinColorKey>(2);
    for (i = 0; i < 2; ++i) {
        pTintColorKey[i].SetTime(fDefaultKeyTimes[i]);
        NiColorA kColor(1.0f, 1.0f, 1.0f, 0.0f);
        pTintColorKey[i].SetColor(kColor);
    }
    TintColorInterpolator.ReplaceKeys(pTintColorKey, Data.uiTintColorKeySize, NiAnimationKey::LINKEY);

    Data.uiFadeColorKeySize = 2;
    pFadeColorKey = NiAnimationKey::AllocateKeyArray<NiLinColorKey>(2);
    for (i = 0; i < 2; ++i) {
        pFadeColorKey[i].SetTime(fDefaultKeyTimes[i]);
        NiColorA kColor(1.0f, 1.0f, 1.0f, 0.0f);
        pFadeColorKey[i].SetColor(kColor);
    }
    FadeColorInterpolator.ReplaceKeys(pFadeColorKey, Data.uiFadeColorKeySize, NiAnimationKey::LINKEY);

    Data.uiBlurKeySize = 2;
    pBlurFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(2);
    for (i = 0; i < 2; ++i) {
        pBlurFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pBlurFloatKey[i].SetValue(0.0f);
    }
    BlurInterpolator.ReplaceKeys(pBlurFloatKey, Data.uiBlurKeySize, NiAnimationKey::LINKEY);

    Data.uiDoubleKeySize = 2;
    pDoubleFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(2);
    for (i = 0; i < 2; ++i) {
        pDoubleFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pDoubleFloatKey[i].SetValue(0.0f);
    }
    DoubleInterpolator.ReplaceKeys(pDoubleFloatKey, Data.uiDoubleKeySize, NiAnimationKey::LINKEY);

    Data.uiRadialBlurStrengthKeySize = 2;
    pRadialBlurStrengthFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(2);
    for (i = 0; i < 2; ++i) {
        pRadialBlurStrengthFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pRadialBlurStrengthFloatKey[i].SetValue(0.0f);
    }
    RadialBlurStrengthInterpolator
        .ReplaceKeys(pRadialBlurStrengthFloatKey, Data.uiRadialBlurStrengthKeySize, NiAnimationKey::LINKEY);

    Data.uiRadialBlurRampupKeySize = 2;
    pRadialBlurRampupFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(2);
    for (i = 0; i < 2; ++i) {
        pRadialBlurRampupFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pRadialBlurRampupFloatKey[i].SetValue(0.0f);
    }
    RadialBlurRampupInterpolator
        .ReplaceKeys(pRadialBlurRampupFloatKey, Data.uiRadialBlurRampupKeySize, NiAnimationKey::LINKEY);

    Data.uiRadialBlurStartKeySize = 2;
    pRadialBlurStartFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(2);
    for (i = 0; i < 2; ++i) {
        pRadialBlurStartFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pRadialBlurStartFloatKey[i].SetValue(0.0f);
    }
    RadialBlurStartInterpolator
        .ReplaceKeys(pRadialBlurStartFloatKey, Data.uiRadialBlurStartKeySize, NiAnimationKey::LINKEY);

    Data.uiRadialBlurRampDownKeySize = 2;
    pRadialBlurRampDownFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(2);
    for (i = 0; i < 2; ++i) {
        pRadialBlurRampDownFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pRadialBlurRampDownFloatKey[i].SetValue(0.0f);
    }
    RadialBlurRampDownInterpolator
        .ReplaceKeys(pRadialBlurRampDownFloatKey, Data.uiRadialBlurRampDownKeySize, NiAnimationKey::LINKEY);

    Data.uiRadialBlurDownStartKeySize = 2;
    pRadialBlurDownStartFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(2);
    for (i = 0; i < 2; ++i) {
        pRadialBlurDownStartFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pRadialBlurDownStartFloatKey[i].SetValue(0.0f);
    }
    RadialBlurDownStartInterpolator
        .ReplaceKeys(pRadialBlurDownStartFloatKey, Data.uiRadialBlurDownStartKeySize, NiAnimationKey::LINKEY);

    Data.uiDepthOfFieldStrengthKeySize = 2;
    pDepthOfFieldStrengthFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(2);
    for (i = 0; i < 2; ++i) {
        pDepthOfFieldStrengthFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pDepthOfFieldStrengthFloatKey[i].SetValue(0.0f);
    }
    DepthOfFieldStrengthInterpolator
        .ReplaceKeys(pDepthOfFieldStrengthFloatKey, Data.uiDepthOfFieldStrengthKeySize, NiAnimationKey::LINKEY);

    Data.uiDepthOfFieldDistanceKeySize = 2;
    pDepthOfFieldDistanceFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(2);
    for (i = 0; i < 2; ++i) {
        pDepthOfFieldDistanceFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pDepthOfFieldDistanceFloatKey[i].SetValue(0.0f);
    }
    DepthOfFieldDistanceInterpolator
        .ReplaceKeys(pDepthOfFieldDistanceFloatKey, Data.uiDepthOfFieldDistanceKeySize, NiAnimationKey::LINKEY);

    Data.uiDepthOfFieldRangeKeySize = 2;
    pDepthOfFieldRangeFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(2);
    for (i = 0; i < 2; ++i) {
        pDepthOfFieldRangeFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pDepthOfFieldRangeFloatKey[i].SetValue(0.0f);
    }
    DepthOfFieldRangeInterpolator
        .ReplaceKeys(pDepthOfFieldRangeFloatKey, Data.uiDepthOfFieldRangeKeySize, NiAnimationKey::LINKEY);

    Data.uiMotionBlurStrengthKeySize = 2;
    pMotionBlurStrengthFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(2);
    for (i = 0; i < 2; ++i) {
        pMotionBlurStrengthFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pMotionBlurStrengthFloatKey[i].SetValue(0.0f);
    }
    MotionBlurStrengthInterpolator
        .ReplaceKeys(pMotionBlurStrengthFloatKey, Data.uiMotionBlurStrengthKeySize, NiAnimationKey::LINKEY);

    pIntroSound = nullptr;
    pOutroSound = nullptr;
}

// Drops the key data of every interpolator and the sounds; the key arrays stay allocated.
void TESImageSpaceModifier::ClearData() {
    for (int32_t i = 0; i < NUM_OP_PARAMS; ++i) {
        for (int32_t j = 0; j < IMAOP_COUNT; ++j) {
            ppInterpolator[i][j].SetFloatData(nullptr);
        }
    }
    TintColorInterpolator.SetColorData(nullptr);
    FadeColorInterpolator.SetColorData(nullptr);
    BlurInterpolator.SetFloatData(nullptr);
    DoubleInterpolator.SetFloatData(nullptr);
    RadialBlurStrengthInterpolator.SetFloatData(nullptr);
    RadialBlurRampupInterpolator.SetFloatData(nullptr);
    RadialBlurStartInterpolator.SetFloatData(nullptr);
    RadialBlurRampDownInterpolator.SetFloatData(nullptr);
    RadialBlurDownStartInterpolator.SetFloatData(nullptr);
    DepthOfFieldStrengthInterpolator.SetFloatData(nullptr);
    DepthOfFieldDistanceInterpolator.SetFloatData(nullptr);
    DepthOfFieldRangeInterpolator.SetFloatData(nullptr);
    MotionBlurStrengthInterpolator.SetFloatData(nullptr);
    pIntroSound = nullptr;
    pOutroSound = nullptr;
}

// Reads the modifier's chunks. The EDID buffer is output only (GetChunkData fills it before any read). The data chunk
// (DNAM) is read into Data; every key chunk allocates a count-prefixed array of the size the data announced, reads it,
// swaps it four bytes at a time for a big-endian file and hands it to its interpolator. A DNAI chunk names a parameter
// by its low six bits and the operation by bits 6 and 7.
bool TESImageSpaceModifier::Load(TESFile* apFile) {
    uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (ucFormType != IMAD_ID) {
        return false;
    }
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case DNAM_ID:
                apFile->GetChunkData(&Data, 0xF4u);
                if (apFile->QEndian()) {
                    Data.Endian();
                }
                break;
            case EDID_ID: {
                // A 16-byte aligned buffer of the chunk's size, read up to 0x200.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case RDSD_ID: {
                uint32_t uiIntroSoundID = 0;
                apFile->GetChunkData(uiIntroSoundID);
                // the sound's form ID stands in for it until InitItem resolves it
                pIntroSound = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiIntroSoundID));
                break;
            }
            case RDSI_ID: {
                uint32_t uiOutroSoundID = 0;
                apFile->GetChunkData(uiOutroSoundID);
                pOutroSound = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiOutroSoundID));
                break;
            }
            case BNAM_ID: {
                uint32_t uiNumKeys = Data.uiBlurKeySize;
                pBlurFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
                apFile->GetChunkData(pBlurFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pBlurFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        BSEndian(puiDwords[i], false);
                    }
                }
                BlurInterpolator.ReplaceKeys(pBlurFloatKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case VNAM_ID: {
                uint32_t uiNumKeys = Data.uiDoubleKeySize;
                pDoubleFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
                apFile->GetChunkData(pDoubleFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pDoubleFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        BSEndian(puiDwords[i], false);
                    }
                }
                DoubleInterpolator.ReplaceKeys(pDoubleFloatKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case RNAM_ID: {
                uint32_t uiNumKeys = Data.uiRadialBlurStrengthKeySize;
                pRadialBlurStrengthFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
                apFile->GetChunkData(pRadialBlurStrengthFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pRadialBlurStrengthFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        BSEndian(puiDwords[i], false);
                    }
                }
                RadialBlurStrengthInterpolator
                    .ReplaceKeys(pRadialBlurStrengthFloatKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case SNAM_ID: {
                uint32_t uiNumKeys = Data.uiRadialBlurRampupKeySize;
                pRadialBlurRampupFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
                apFile->GetChunkData(pRadialBlurRampupFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pRadialBlurRampupFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        BSEndian(puiDwords[i], false);
                    }
                }
                RadialBlurRampupInterpolator.ReplaceKeys(pRadialBlurRampupFloatKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case UNAM_ID: {
                uint32_t uiNumKeys = Data.uiRadialBlurStartKeySize;
                pRadialBlurStartFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
                apFile->GetChunkData(pRadialBlurStartFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pRadialBlurStartFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        BSEndian(puiDwords[i], false);
                    }
                }
                RadialBlurStartInterpolator.ReplaceKeys(pRadialBlurStartFloatKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case NAM1_ID: {
                uint32_t uiNumKeys = Data.uiRadialBlurRampDownKeySize;
                pRadialBlurRampDownFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
                apFile->GetChunkData(pRadialBlurRampDownFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pRadialBlurRampDownFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        BSEndian(puiDwords[i], false);
                    }
                }
                RadialBlurRampDownInterpolator
                    .ReplaceKeys(pRadialBlurRampDownFloatKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case NAM2_ID: {
                uint32_t uiNumKeys = Data.uiRadialBlurDownStartKeySize;
                pRadialBlurDownStartFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
                apFile->GetChunkData(pRadialBlurDownStartFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pRadialBlurDownStartFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        BSEndian(puiDwords[i], false);
                    }
                }
                RadialBlurDownStartInterpolator
                    .ReplaceKeys(pRadialBlurDownStartFloatKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case TNAM_ID: {
                uint32_t uiNumKeys = Data.uiTintColorKeySize;
                pTintColorKey = NiAnimationKey::AllocateKeyArray<NiLinColorKey>(uiNumKeys);
                apFile->GetChunkData(pTintColorKey, 20 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pTintColorKey);
                    for (uint32_t i = 0; i < (20 * uiNumKeys) >> 2; ++i) {
                        BSEndian(puiDwords[i], false);
                    }
                }
                TintColorInterpolator.ReplaceKeys(pTintColorKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case NAM3_ID: {
                uint32_t uiNumKeys = Data.uiFadeColorKeySize;
                pFadeColorKey = NiAnimationKey::AllocateKeyArray<NiLinColorKey>(uiNumKeys);
                apFile->GetChunkData(pFadeColorKey, 20 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pFadeColorKey);
                    for (uint32_t i = 0; i < (20 * uiNumKeys) >> 2; ++i) {
                        BSEndian(puiDwords[i], false);
                    }
                }
                FadeColorInterpolator.ReplaceKeys(pFadeColorKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case WNAM_ID: {
                uint32_t uiNumKeys = Data.uiDepthOfFieldStrengthKeySize;
                pDepthOfFieldStrengthFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
                apFile->GetChunkData(pDepthOfFieldStrengthFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pDepthOfFieldStrengthFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        BSEndian(puiDwords[i], false);
                    }
                }
                DepthOfFieldStrengthInterpolator
                    .ReplaceKeys(pDepthOfFieldStrengthFloatKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case XNAM_ID: {
                uint32_t uiNumKeys = Data.uiDepthOfFieldDistanceKeySize;
                pDepthOfFieldDistanceFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
                apFile->GetChunkData(pDepthOfFieldDistanceFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pDepthOfFieldDistanceFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        BSEndian(puiDwords[i], false);
                    }
                }
                DepthOfFieldDistanceInterpolator
                    .ReplaceKeys(pDepthOfFieldDistanceFloatKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case YNAM_ID: {
                uint32_t uiNumKeys = Data.uiDepthOfFieldRangeKeySize;
                pDepthOfFieldRangeFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
                apFile->GetChunkData(pDepthOfFieldRangeFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pDepthOfFieldRangeFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        BSEndian(puiDwords[i], false);
                    }
                }
                DepthOfFieldRangeInterpolator
                    .ReplaceKeys(pDepthOfFieldRangeFloatKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case NAM4_ID: {
                uint32_t uiNumKeys = Data.uiMotionBlurStrengthKeySize;
                pMotionBlurStrengthFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
                apFile->GetChunkData(pMotionBlurStrengthFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pMotionBlurStrengthFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        BSEndian(puiDwords[i], false);
                    }
                }
                MotionBlurStrengthInterpolator
                    .ReplaceKeys(pMotionBlurStrengthFloatKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            default: {
                // the DNAI chunks: the parameter's index in the low six bits, the operation in bits 6 and 7
                if (uiChunk >= 0x44414900u && uiChunk <= 0x444149FFu) {
                    uint32_t uiIndex = uiChunk & 0x3Fu;
                    uint32_t uiOp = (uiChunk & 0xC0u) >> 6;
                    uint32_t uiNumKeys = Data.ppuiKeySize[uiIndex][uiOp];
                    pppFloatKey[uiIndex][uiOp] = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
                    apFile->GetChunkData(pppFloatKey[uiIndex][uiOp], 8 * uiNumKeys);
                    if (apFile->QEndian()) {
                        uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pppFloatKey[uiIndex][uiOp]);
                        for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                            BSEndian(puiDwords[i], false);
                        }
                    }
                    ppInterpolator[uiIndex][uiOp]
                        .ReplaceKeys(pppFloatKey[uiIndex][uiOp], uiNumKeys, NiAnimationKey::LINKEY);
                }
                break;
            }
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESImageSpaceModifier::Save() {
    StartForm();
    FlushData();
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    TESForm::__AddChunkData(DNAM_ID, &Data, 244u);
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    uint32_t uiCount = 8 * Data.uiBlurKeySize >> 2;
    uint32_t* pEndianData = static_cast<uint32_t*>(
        ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
    );
    ourMemcpy(pEndianData, pBlurFloatKey, sizeof(uint32_t) * uiCount);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i = 0; i < uiCount; ++i) {
            BSEndian<uint32_t>(pEndianData[i], false);
        }
    }
    TESForm::__AddChunkData(BNAM_ID, pEndianData, 4 * uiCount);
    ourFree(pEndianData);
    uiCount = 8 * Data.uiDoubleKeySize >> 2;
    pEndianData = static_cast<uint32_t*>(
        ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
    );
    ourMemcpy(pEndianData, pDoubleFloatKey, sizeof(uint32_t) * uiCount);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i = 0; i < uiCount; ++i) {
            BSEndian<uint32_t>(pEndianData[i], false);
        }
    }
    TESForm::__AddChunkData(VNAM_ID, pEndianData, 4 * uiCount);
    ourFree(pEndianData);
    uiCount = 20 * Data.uiTintColorKeySize >> 2;
    pEndianData = static_cast<uint32_t*>(
        ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
    );
    ourMemcpy(pEndianData, pTintColorKey, sizeof(uint32_t) * uiCount);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i = 0; i < uiCount; ++i) {
            BSEndian<uint32_t>(pEndianData[i], false);
        }
    }
    TESForm::__AddChunkData(TNAM_ID, pEndianData, 4 * uiCount);
    ourFree(pEndianData);
    uiCount = 20 * Data.uiFadeColorKeySize >> 2;
    pEndianData = static_cast<uint32_t*>(
        ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
    );
    ourMemcpy(pEndianData, pFadeColorKey, sizeof(uint32_t) * uiCount);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i = 0; i < uiCount; ++i) {
            BSEndian<uint32_t>(pEndianData[i], false);
        }
    }
    TESForm::__AddChunkData(NAM3_ID, pEndianData, 4 * uiCount);
    ourFree(pEndianData);
    uiCount = 8 * Data.uiRadialBlurStrengthKeySize >> 2;
    pEndianData = static_cast<uint32_t*>(
        ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
    );
    ourMemcpy(pEndianData, pRadialBlurStrengthFloatKey, sizeof(uint32_t) * uiCount);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i = 0; i < uiCount; ++i) {
            BSEndian<uint32_t>(pEndianData[i], false);
        }
    }
    TESForm::__AddChunkData(RNAM_ID, pEndianData, 4 * uiCount);
    ourFree(pEndianData);
    uiCount = 8 * Data.uiRadialBlurRampupKeySize >> 2;
    pEndianData = static_cast<uint32_t*>(
        ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
    );
    ourMemcpy(pEndianData, pRadialBlurRampupFloatKey, sizeof(uint32_t) * uiCount);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i = 0; i < uiCount; ++i) {
            BSEndian<uint32_t>(pEndianData[i], false);
        }
    }
    TESForm::__AddChunkData(SNAM_ID, pEndianData, 4 * uiCount);
    ourFree(pEndianData);
    uiCount = 8 * Data.uiRadialBlurStartKeySize >> 2;
    pEndianData = static_cast<uint32_t*>(
        ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
    );
    ourMemcpy(pEndianData, pRadialBlurStartFloatKey, sizeof(uint32_t) * uiCount);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i = 0; i < uiCount; ++i) {
            BSEndian<uint32_t>(pEndianData[i], false);
        }
    }
    TESForm::__AddChunkData(UNAM_ID, pEndianData, 4 * uiCount);
    ourFree(pEndianData);
    uiCount = 8 * Data.uiRadialBlurRampDownKeySize >> 2;
    pEndianData = static_cast<uint32_t*>(
        ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
    );
    ourMemcpy(pEndianData, pRadialBlurRampDownFloatKey, sizeof(uint32_t) * uiCount);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i = 0; i < uiCount; ++i) {
            BSEndian<uint32_t>(pEndianData[i], false);
        }
    }
    TESForm::__AddChunkData(NAM1_ID, pEndianData, 4 * uiCount);
    ourFree(pEndianData);
    uiCount = 8 * Data.uiRadialBlurDownStartKeySize >> 2;
    pEndianData = static_cast<uint32_t*>(
        ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
    );
    ourMemcpy(pEndianData, pRadialBlurDownStartFloatKey, sizeof(uint32_t) * uiCount);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i = 0; i < uiCount; ++i) {
            BSEndian<uint32_t>(pEndianData[i], false);
        }
    }
    TESForm::__AddChunkData(NAM2_ID, pEndianData, 4 * uiCount);
    ourFree(pEndianData);
    uiCount = 8 * Data.uiDepthOfFieldStrengthKeySize >> 2;
    pEndianData = static_cast<uint32_t*>(
        ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
    );
    ourMemcpy(pEndianData, pDepthOfFieldStrengthFloatKey, sizeof(uint32_t) * uiCount);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i = 0; i < uiCount; ++i) {
            BSEndian<uint32_t>(pEndianData[i], false);
        }
    }
    TESForm::__AddChunkData(WNAM_ID, pEndianData, 4 * uiCount);
    ourFree(pEndianData);
    uiCount = 8 * Data.uiDepthOfFieldDistanceKeySize >> 2;
    pEndianData = static_cast<uint32_t*>(
        ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
    );
    ourMemcpy(pEndianData, pDepthOfFieldDistanceFloatKey, sizeof(uint32_t) * uiCount);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i = 0; i < uiCount; ++i) {
            BSEndian<uint32_t>(pEndianData[i], false);
        }
    }
    TESForm::__AddChunkData(XNAM_ID, pEndianData, 4 * uiCount);
    ourFree(pEndianData);
    uiCount = 8 * Data.uiDepthOfFieldRangeKeySize >> 2;
    pEndianData = static_cast<uint32_t*>(
        ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
    );
    ourMemcpy(pEndianData, pDepthOfFieldRangeFloatKey, sizeof(uint32_t) * uiCount);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i = 0; i < uiCount; ++i) {
            BSEndian<uint32_t>(pEndianData[i], false);
        }
    }
    TESForm::__AddChunkData(YNAM_ID, pEndianData, 4 * uiCount);
    ourFree(pEndianData);
    uiCount = 8 * Data.uiMotionBlurStrengthKeySize >> 2;
    pEndianData = static_cast<uint32_t*>(
        ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
    );
    ourMemcpy(pEndianData, pMotionBlurStrengthFloatKey, sizeof(uint32_t) * uiCount);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i = 0; i < uiCount; ++i) {
            BSEndian<uint32_t>(pEndianData[i], false);
        }
    }
    TESForm::__AddChunkData(NAM4_ID, pEndianData, 4 * uiCount);
    ourFree(pEndianData);
    for (int32_t i = 0; i < 21; ++i) {
        for (int32_t j = 0; j < 2; ++j) {
            uiCount = 8 * Data.ppuiKeySize[i][j] >> 2;
            pEndianData = static_cast<uint32_t*>(
                ourMalloc(uiCount <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * uiCount : SIZE_MAX)
            );
            ourMemcpy(pEndianData, pppFloatKey[i][j], sizeof(uint32_t) * uiCount);
            if (TESForm::QEndianSwapOnSave()) {
                for (uint32_t iM = 0; iM < uiCount; ++iM) {
                    BSEndian<uint32_t>(pEndianData[iM], false);
                }
            }
            // The key arrays' chunk IDs are computed: 'DAI' with the key index in the low byte, +0x40 for the second
            // key.
            TESForm::__AddChunkData(static_cast<CHUNK_ID>(j << 6 | (i | 0x44414900)), pEndianData, 4 * uiCount);
            ourFree(pEndianData);
        }
    }
    if (pIntroSound) {
        TESForm::AddChunk(RDSD_ID, pIntroSound->iFormID);
    }
    if (pOutroSound) {
        TESForm::AddChunk(RDSI_ID, pOutroSound->iFormID);
    }
    CloseForm();
}

// Load leaves the sounds' form IDs in the pointers: resolve them.
void TESImageSpaceModifier::InitItem() {
    if (GetInitialized()) {
        return;
    }

    uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pIntroSound));
    if (iID) {
        TESForm::AddCompileIndex(iID, GetFile(-1));
        pIntroSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(iID));
        if (!pIntroSound) {
            uint32_t iFormID = GetFormID();
            const char* pEditorID = GetFormEditorID();
            Warning(
                "MASTERFILE: Could not find pIntroSound (%08X) for ImageSpaceModifier '%s' (%08X).",
                iID,
                pEditorID,
                iFormID
            );
        }
    }
    iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pOutroSound));
    if (iID) {
        TESForm::AddCompileIndex(iID, GetFile(-1));
        pOutroSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(iID));
        if (!pOutroSound) {
            uint32_t iFormID = GetFormID();
            const char* pEditorID = GetFormEditorID();
            Warning(
                "MASTERFILE: Could not find pOutroSound (%08X) for ImageSpaceModifier '%s' (%08X).",
                iID,
                pEditorID,
                iFormID
            );
        }
    }
    SetInitialized(true);
}

// Each key array is a new array of the source's key count, filled from the source's keys and handed to its
// interpolator.
void TESImageSpaceModifier::Copy(TESForm* apCopy) {
    TESImageSpaceModifier* pCopy = dynamic_cast<TESImageSpaceModifier*>(apCopy);
    if (!pCopy) {
        return;
    }
    pCopy->FlushData();
    CopyAllComponents(pCopy);
    ClearData();
    pIntroSound = pCopy->pIntroSound;
    pOutroSound = pCopy->pOutroSound;
    ourMemcpy(&Data, &pCopy->Data, sizeof(Data));

    uint32_t uiNumKeys = Data.uiBlurKeySize;
    pBlurFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
    ourMemcpy(pBlurFloatKey, pCopy->pBlurFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    BlurInterpolator.ReplaceKeys(pBlurFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiDoubleKeySize;
    pDoubleFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
    ourMemcpy(pDoubleFloatKey, pCopy->pDoubleFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    DoubleInterpolator.ReplaceKeys(pDoubleFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiTintColorKeySize;
    pTintColorKey = NiAnimationKey::AllocateKeyArray<NiLinColorKey>(uiNumKeys);
    ourMemcpy(pTintColorKey, pCopy->pTintColorKey, sizeof(NiLinColorKey) * uiNumKeys);
    TintColorInterpolator.ReplaceKeys(pTintColorKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiFadeColorKeySize;
    pFadeColorKey = NiAnimationKey::AllocateKeyArray<NiLinColorKey>(uiNumKeys);
    ourMemcpy(pFadeColorKey, pCopy->pFadeColorKey, sizeof(NiLinColorKey) * uiNumKeys);
    FadeColorInterpolator.ReplaceKeys(pFadeColorKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiRadialBlurStrengthKeySize;
    pRadialBlurStrengthFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
    ourMemcpy(pRadialBlurStrengthFloatKey, pCopy->pRadialBlurStrengthFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    RadialBlurStrengthInterpolator.ReplaceKeys(pRadialBlurStrengthFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiRadialBlurRampupKeySize;
    pRadialBlurRampupFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
    ourMemcpy(pRadialBlurRampupFloatKey, pCopy->pRadialBlurRampupFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    RadialBlurRampupInterpolator.ReplaceKeys(pRadialBlurRampupFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiRadialBlurStartKeySize;
    pRadialBlurStartFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
    ourMemcpy(pRadialBlurStartFloatKey, pCopy->pRadialBlurStartFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    RadialBlurStartInterpolator.ReplaceKeys(pRadialBlurStartFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiRadialBlurRampDownKeySize;
    pRadialBlurRampDownFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
    ourMemcpy(pRadialBlurRampDownFloatKey, pCopy->pRadialBlurRampDownFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    RadialBlurRampDownInterpolator.ReplaceKeys(pRadialBlurRampDownFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiRadialBlurDownStartKeySize;
    pRadialBlurDownStartFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
    ourMemcpy(pRadialBlurDownStartFloatKey, pCopy->pRadialBlurDownStartFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    RadialBlurDownStartInterpolator.ReplaceKeys(pRadialBlurDownStartFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiDepthOfFieldStrengthKeySize;
    pDepthOfFieldStrengthFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
    ourMemcpy(pDepthOfFieldStrengthFloatKey, pCopy->pDepthOfFieldStrengthFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    DepthOfFieldStrengthInterpolator.ReplaceKeys(pDepthOfFieldStrengthFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiDepthOfFieldDistanceKeySize;
    pDepthOfFieldDistanceFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
    ourMemcpy(pDepthOfFieldDistanceFloatKey, pCopy->pDepthOfFieldDistanceFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    DepthOfFieldDistanceInterpolator.ReplaceKeys(pDepthOfFieldDistanceFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiDepthOfFieldRangeKeySize;
    pDepthOfFieldRangeFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
    ourMemcpy(pDepthOfFieldRangeFloatKey, pCopy->pDepthOfFieldRangeFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    DepthOfFieldRangeInterpolator.ReplaceKeys(pDepthOfFieldRangeFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiMotionBlurStrengthKeySize;
    pMotionBlurStrengthFloatKey = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
    ourMemcpy(pMotionBlurStrengthFloatKey, pCopy->pMotionBlurStrengthFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    MotionBlurStrengthInterpolator.ReplaceKeys(pMotionBlurStrengthFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    for (int32_t i = 0; i < NUM_OP_PARAMS; ++i) {
        for (int32_t j = 0; j < IMAOP_COUNT; ++j) {
            uiNumKeys = Data.ppuiKeySize[i][j];
            pppFloatKey[i][j] = NiAnimationKey::AllocateKeyArray<NiLinFloatKey>(uiNumKeys);
            ourMemcpy(pppFloatKey[i][j], pCopy->pppFloatKey[i][j], sizeof(NiLinFloatKey) * uiNumKeys);
            ppInterpolator[i][j].ReplaceKeys(pppFloatKey[i][j], uiNumKeys, NiAnimationKey::LINKEY);
        }
    }
}

bool TESImageSpaceModifier::Compare(TESForm* apCompare) {
    int32_t j;
    int32_t i;
    TESImageSpaceModifier* pCompare = dynamic_cast<TESImageSpaceModifier*>(apCompare);
    if (!pCompare) {
        return true;
    }
    this->FlushData();
    pCompare->FlushData();
    if (pIntroSound != pCompare->pIntroSound) {
        return true;
    }
    if (pOutroSound != pCompare->pOutroSound) {
        return true;
    }
    if (TESForm::CompareAllComponents(pCompare)) {
        return true;
    }
    if (std::memcmp(&Data, &pCompare->Data, sizeof(Data))) {
        return true;
    }
    if (std::memcmp(pBlurFloatKey, pCompare->pBlurFloatKey, sizeof(NiLinFloatKey) * Data.uiBlurKeySize)) {
        return true;
    }
    if (std::memcmp(pDoubleFloatKey, pCompare->pDoubleFloatKey, sizeof(NiLinFloatKey) * Data.uiDoubleKeySize)) {
        return true;
    }
    if (std::memcmp(pTintColorKey, pCompare->pTintColorKey, sizeof(NiLinColorKey) * Data.uiTintColorKeySize)) {
        return true;
    }
    if (std::memcmp(pFadeColorKey, pCompare->pFadeColorKey, sizeof(NiLinColorKey) * Data.uiFadeColorKeySize)) {
        return true;
    }
    if (std::memcmp(
            pRadialBlurStrengthFloatKey,
            pCompare->pRadialBlurStrengthFloatKey,
            sizeof(NiLinFloatKey) * Data.uiRadialBlurStrengthKeySize
        )) {
        return true;
    }
    if (std::memcmp(
            pRadialBlurRampupFloatKey,
            pCompare->pRadialBlurRampupFloatKey,
            sizeof(NiLinFloatKey) * Data.uiRadialBlurRampupKeySize
        )) {
        return true;
    }
    if (std::memcmp(
            pRadialBlurStartFloatKey,
            pCompare->pRadialBlurStartFloatKey,
            sizeof(NiLinFloatKey) * Data.uiRadialBlurStartKeySize
        )) {
        return true;
    }
    if (std::memcmp(
            pRadialBlurRampDownFloatKey,
            pCompare->pRadialBlurRampDownFloatKey,
            sizeof(NiLinFloatKey) * Data.uiRadialBlurRampDownKeySize
        )) {
        return true;
    }
    if (std::memcmp(
            pRadialBlurDownStartFloatKey,
            pCompare->pRadialBlurDownStartFloatKey,
            sizeof(NiLinFloatKey) * Data.uiRadialBlurDownStartKeySize
        )) {
        return true;
    }
    if (std::memcmp(
            pDepthOfFieldStrengthFloatKey,
            pCompare->pDepthOfFieldStrengthFloatKey,
            sizeof(NiLinFloatKey) * Data.uiDepthOfFieldStrengthKeySize
        )) {
        return true;
    }
    if (std::memcmp(
            pDepthOfFieldDistanceFloatKey,
            pCompare->pDepthOfFieldDistanceFloatKey,
            sizeof(NiLinFloatKey) * Data.uiDepthOfFieldDistanceKeySize
        )) {
        return true;
    }
    if (std::memcmp(
            pDepthOfFieldRangeFloatKey,
            pCompare->pDepthOfFieldRangeFloatKey,
            sizeof(NiLinFloatKey) * Data.uiDepthOfFieldRangeKeySize
        )) {
        return true;
    }
    if (std::memcmp(
            pMotionBlurStrengthFloatKey,
            pCompare->pMotionBlurStrengthFloatKey,
            sizeof(NiLinFloatKey) * Data.uiMotionBlurStrengthKeySize
        )) {
        return true;
    }
    for (i = 0; i < 21; ++i) {
        for (j = 0; j < 2; ++j) {
            if (std::memcmp(
                    pppFloatKey[i][j],
                    pCompare->pppFloatKey[i][j],
                    sizeof(NiLinFloatKey) * Data.ppuiKeySize[i][j]
                )) {
                return true;
            }
        }
    }
    return false;
}

void TESImageSpaceModifier::MakeDefault() {
    InitializeData();
    Data.bAnimatable = true;
    pDoubleFloatKey->SetValue(0.005f);
}

void TESImageSpaceModifier::Apply(float afStartTime, float afStrength, NiAVObject* apTarget) {
    float fMBStr;
    float fRadialBlurRampDown;
    float fRadialBlurStart;
    float fRadialBlurRampup;
    float fDOFRange;
    float fDoubleVal;
    float fRadialBlurStr;
    float fDOFDistance;
    float fRadialBlurDownStart;
    float fDOFStrength;
    float fBlurVal;
    float fTime = Data.bAnimatable ? (TESImageSpaceModifier::fCurrentTime - afStartTime) / Data.fDuration : 0.0f;
    BSClamp(fTime, 0.0f, 1.0f);
    ImageSpaceManager* pManager = BSShaderManager::GetImageSpaceManager();
    for (int32_t i = 0; i < 21; ++i) {
        float fVal[2];
        for (int32_t j = 0; j < 2; ++j) {
            ppInterpolator[i][j].Update(fTime, nullptr, fVal[j]);
        }
        if (i >= 0 && i <= 13) {
            pManager->ApplyParamModifier(ImageSpaceManager::EFFECT_HDR, i, fVal, afStrength);
        }
        if (i >= 14 && i <= 16) {
            pManager->ApplyParamModifier(ImageSpaceManager::EFFECT_BLOOM, i - 14, fVal, afStrength);
        }
        if (i >= 17 && i <= 20) {
            pManager->ApplyParamModifier(ImageSpaceManager::EFFECT_CINEMATIC, i - 17, fVal, afStrength);
        }
    }
    NiColorA kColVal(0.0f, 0.0f, 0.0f, 0.0f);
    TintColorInterpolator.Update(fTime, nullptr, kColVal);
    kColVal.a = kColVal.a * afStrength;
    BSClamp(kColVal.a, 0.0f, 1.0f);
    pManager->ApplyTintColorParamModifier(kColVal);
    FadeColorInterpolator.Update(fTime, nullptr, kColVal);
    kColVal.a = kColVal.a * afStrength;
    BSClamp(kColVal.a, 0.0f, 1.0f);
    pManager->ApplyFadeColorParamModifier(kColVal);
    BlurInterpolator.Update(fTime, nullptr, fBlurVal);
    DoubleInterpolator.Update(fTime, nullptr, fDoubleVal);
    RadialBlurStrengthInterpolator.Update(fTime, nullptr, fRadialBlurStr);
    RadialBlurRampupInterpolator.Update(fTime, nullptr, fRadialBlurRampup);
    RadialBlurStartInterpolator.Update(fTime, nullptr, fRadialBlurStart);
    RadialBlurRampDownInterpolator.Update(fTime, nullptr, fRadialBlurRampDown);
    RadialBlurDownStartInterpolator.Update(fTime, nullptr, fRadialBlurDownStart);
    DepthOfFieldStrengthInterpolator.Update(fTime, nullptr, fDOFStrength);
    DepthOfFieldDistanceInterpolator.Update(fTime, nullptr, fDOFDistance);
    DepthOfFieldRangeInterpolator.Update(fTime, nullptr, fDOFRange);
    MotionBlurStrengthInterpolator.Update(fTime, nullptr, fMBStr);
    fBlurVal = fBlurVal * afStrength;
    BSClamp(fBlurVal, 0.0f, 7.0f);
    pManager->ApplyBlurRadiusParamModifier(fBlurVal);
    float fScaledDoubleVision = fDoubleVal * afStrength;
    pManager->ApplyDoubleVisionParamModifier(fScaledDoubleVision);
    if (Data.bUseTargetForRadialBlur && apTarget) {
        apTarget->UpdateWorldBound();
        const NiBound* pWorldBound = &apTarget->GetWorldBound();
        float fScaledRadialBlur = fRadialBlurStr * afStrength;
        pManager->ApplyRadialBlurParamModifier(
            fScaledRadialBlur,
            fRadialBlurRampup,
            fRadialBlurStart,
            fRadialBlurRampDown,
            fRadialBlurDownStart,
            *pWorldBound
        );
    }
    else if (Data.bUseTargetForRadialBlur) {
        NiPoint2 kCenter(0.5f, 0.5f);
        float fScaledRadialBlur = fRadialBlurStr * afStrength;
        pManager->ApplyRadialBlurParamModifier(
            fScaledRadialBlur,
            fRadialBlurRampup,
            fRadialBlurStart,
            fRadialBlurRampDown,
            fRadialBlurDownStart,
            kCenter
        );
    }
    else {
        float fScaledRadialBlur = fRadialBlurStr * afStrength;
        pManager->ApplyRadialBlurParamModifier(
            fScaledRadialBlur,
            fRadialBlurRampup,
            fRadialBlurStart,
            fRadialBlurRampDown,
            fRadialBlurDownStart,
            Data.RadialBlurCenter
        );
    }
    if (Data.bUseTargetForDepthOfField && apTarget) {
        apTarget->UpdateWorldBound();
        const NiBound* pWorldBound = &apTarget->GetWorldBound();
        float fScaledDOFStrength = fDOFStrength * afStrength;
        pManager->ApplyDepthOfFieldParamModifier_0(
            fScaledDOFStrength,
            Data.ucDepthOfFieldMode,
            0.0f,
            &pWorldBound->m_kCenter
        );
    }
    else if (Data.bUseTargetForDepthOfField) {
        float fScaledDOFStrength = fDOFStrength * afStrength;
        pManager->ApplyDepthOfFieldParamModifier(fScaledDOFStrength, Data.ucDepthOfFieldMode, 0.0f, 0.0f, 1.0f);
    }
    else {
        float fScaledDOFStrength = fDOFStrength * afStrength;
        pManager->ApplyDepthOfFieldParamModifier(
            fScaledDOFStrength,
            Data.ucDepthOfFieldMode,
            0.0f,
            fDOFDistance,
            fDOFRange
        );
    }
    pManager->ApplyMotionBlurStrengthParamModifier(fMBStr);
}

void TESImageSpaceModifier::ApplyWeather(float afStartTime, float afStrength) {
    float fMBStr;
    float fRadialBlurRampDown;
    float fRadialBlurStart;
    float fRadialBlurRampup;
    float fDOFRange;
    float fDoubleVal;
    float fRadialBlurStr;
    float fDOFDistance;
    float fRadialBlurDownStart;
    float fDOFStrength;
    float fBlurVal;
    float fTime = Data.bAnimatable ? (TESImageSpaceModifier::fCurrentTime - afStartTime) / Data.fDuration : 0.0f;
    ImageSpaceManager* pManager = BSShaderManager::GetImageSpaceManager();
    for (int32_t i = 0; i < 21; ++i) {
        float fVal[2];
        for (int32_t j = 0; j < 2; ++j) {
            ppInterpolator[i][j].Update(fTime, nullptr, fVal[j]);
        }
        if (i >= 0 && i <= 13) {
            pManager->ApplyParamModifierWeather(ImageSpaceManager::EFFECT_HDR, i, fVal, afStrength);
        }
        if (i >= 14 && i <= 16) {
            pManager->ApplyParamModifierWeather(ImageSpaceManager::EFFECT_BLOOM, i - 14, fVal, afStrength);
        }
        if (i >= 17 && i <= 20) {
            pManager->ApplyParamModifierWeather(ImageSpaceManager::EFFECT_CINEMATIC, i - 17, fVal, afStrength);
        }
    }
    NiColorA kColVal(0.0f, 0.0f, 0.0f, 0.0f);
    TintColorInterpolator.Update(fTime, nullptr, kColVal);
    kColVal.Scale(afStrength);
    pManager->ApplyTintColorParamModifierWeather(kColVal);
    FadeColorInterpolator.Update(fTime, nullptr, kColVal);
    kColVal.Scale(afStrength);
    pManager->ModFadeColor(kColVal);
    BlurInterpolator.Update(fTime, nullptr, fBlurVal);
    DoubleInterpolator.Update(fTime, nullptr, fDoubleVal);
    RadialBlurStrengthInterpolator.Update(fTime, nullptr, fRadialBlurStr);
    RadialBlurRampupInterpolator.Update(fTime, nullptr, fRadialBlurRampup);
    RadialBlurStartInterpolator.Update(fTime, nullptr, fRadialBlurStart);
    RadialBlurRampDownInterpolator.Update(fTime, nullptr, fRadialBlurRampDown);
    RadialBlurDownStartInterpolator.Update(fTime, nullptr, fRadialBlurDownStart);
    DepthOfFieldStrengthInterpolator.Update(fTime, nullptr, fDOFStrength);
    DepthOfFieldDistanceInterpolator.Update(fTime, nullptr, fDOFDistance);
    DepthOfFieldRangeInterpolator.Update(fTime, nullptr, fDOFRange);
    MotionBlurStrengthInterpolator.Update(fTime, nullptr, fMBStr);
    float fScaledBlur = fBlurVal * afStrength;
    pManager->ModBlurRadius(fScaledBlur);
    float fScaledDoubleVision = fDoubleVal * afStrength;
    pManager->ModDoubleVisionStrength(fScaledDoubleVision);
    NiPoint2 kScaledCenter = Data.RadialBlurCenter * afStrength;
    float fScaledDownStart = fRadialBlurDownStart * afStrength;
    float fScaledRampDown = fRadialBlurRampDown * afStrength;
    float fScaledStart = fRadialBlurStart * afStrength;
    float fScaledRampup = fRadialBlurRampup * afStrength;
    float fScaledRadialBlur = fRadialBlurStr * afStrength;
    pManager->ModRadialBlur(
        fScaledRadialBlur,
        fScaledRampup,
        fScaledStart,
        fScaledRampDown,
        fScaledDownStart,
        &kScaledCenter
    );
    float fScaledDOFRange = fDOFRange * afStrength;
    float fScaledDOFDistance = fDOFDistance * afStrength;
    float fScaledDOFStrength = fDOFStrength * afStrength;
    pManager->ModDepthOfField(fScaledDOFStrength, Data.ucDepthOfFieldMode, 0.0f, fScaledDOFDistance, fScaledDOFRange);
    float fScaledMotionBlur = fMBStr * afStrength;
    pManager->ModFullScreenMotionBlurStrength(fScaledMotionBlur);
}

// An animated modifier started at afStartTime has run out once its duration has passed. A start time below -10000 marks
// one that never expires; a NaN start time takes the duration test.
bool TESImageSpaceModifier::IsExpired(float afStartTime) {
    if (!Data.bAnimatable) {
        return false;
    }
    if (afStartTime < -10000.0) {
        return false;
    }
    return Data.fDuration < fCurrentTime - afStartTime;
}

void TESImageSpaceModifier::FlushData() {
    for (int32_t i = 0; i < 21; ++i) {
        for (int32_t j = 0; j < 2; ++j) {
            Data.ppuiKeySize[i][j] = ppInterpolator[i][j].GetKeyCount(0);
            pppFloatKey[i][j] = static_cast<NiLinFloatKey*>(ppInterpolator[i][j].GetKeyArray(0));
        }
    }
    Data.uiBlurKeySize = BlurInterpolator.GetKeyCount(0);
    pBlurFloatKey = static_cast<NiLinFloatKey*>(BlurInterpolator.GetKeyArray(0));
    Data.uiDoubleKeySize = DoubleInterpolator.GetKeyCount(0);
    pDoubleFloatKey = static_cast<NiLinFloatKey*>(DoubleInterpolator.GetKeyArray(0));
    Data.uiTintColorKeySize = TintColorInterpolator.GetKeyCount(0);
    pTintColorKey = static_cast<NiLinColorKey*>(TintColorInterpolator.GetKeyArray(0));
    Data.uiFadeColorKeySize = FadeColorInterpolator.GetKeyCount(0);
    pFadeColorKey = static_cast<NiLinColorKey*>(FadeColorInterpolator.GetKeyArray(0));
    Data.uiRadialBlurStrengthKeySize = RadialBlurStrengthInterpolator.GetKeyCount(0);
    pRadialBlurStrengthFloatKey = static_cast<NiLinFloatKey*>(RadialBlurStrengthInterpolator.GetKeyArray(0));
    Data.uiRadialBlurRampupKeySize = RadialBlurRampupInterpolator.GetKeyCount(0);
    pRadialBlurRampupFloatKey = static_cast<NiLinFloatKey*>(RadialBlurRampupInterpolator.GetKeyArray(0));
    Data.uiRadialBlurStartKeySize = RadialBlurStartInterpolator.GetKeyCount(0);
    pRadialBlurStartFloatKey = static_cast<NiLinFloatKey*>(RadialBlurStartInterpolator.GetKeyArray(0));
    Data.uiRadialBlurRampDownKeySize = RadialBlurRampDownInterpolator.GetKeyCount(0);
    pRadialBlurRampDownFloatKey = static_cast<NiLinFloatKey*>(RadialBlurRampDownInterpolator.GetKeyArray(0));
    Data.uiRadialBlurDownStartKeySize = RadialBlurDownStartInterpolator.GetKeyCount(0);
    pRadialBlurDownStartFloatKey = static_cast<NiLinFloatKey*>(RadialBlurDownStartInterpolator.GetKeyArray(0));
    Data.uiDepthOfFieldStrengthKeySize = DepthOfFieldStrengthInterpolator.GetKeyCount(0);
    pDepthOfFieldStrengthFloatKey = static_cast<NiLinFloatKey*>(DepthOfFieldStrengthInterpolator.GetKeyArray(0));
    Data.uiDepthOfFieldDistanceKeySize = DepthOfFieldDistanceInterpolator.GetKeyCount(0);
    pDepthOfFieldDistanceFloatKey = static_cast<NiLinFloatKey*>(DepthOfFieldDistanceInterpolator.GetKeyArray(0));
    Data.uiDepthOfFieldRangeKeySize = DepthOfFieldRangeInterpolator.GetKeyCount(0);
    pDepthOfFieldRangeFloatKey = static_cast<NiLinFloatKey*>(DepthOfFieldRangeInterpolator.GetKeyArray(0));
    Data.uiMotionBlurStrengthKeySize = MotionBlurStrengthInterpolator.GetKeyCount(0);
    pMotionBlurStrengthFloatKey = static_cast<NiLinFloatKey*>(MotionBlurStrengthInterpolator.GetKeyArray(0));
}

TESImageSpaceModifier* TESImageSpaceModifier::GetGetHit() {
    if (!g_bEnableHitAndConcussionIMODs) {
        return nullptr;
    }
    if (g_GetHitIMOD) {
        return g_GetHitIMOD;
    }
    TESForm* pForm = TESForm::GetFormByNumericID(354);
    g_GetHitIMOD = dynamic_cast<TESImageSpaceModifier*>(pForm);
    return g_GetHitIMOD;
}

TESImageSpaceModifier* TESImageSpaceModifier::QDefault() {
    return &Default;
}

// The second radial blur target updates apTarget1's world bound, not apTarget2's, deliberately.
void TESImageSpaceModifier::BlendApply(
    float afPercent,
    TESImageSpaceModifier* apMod1,
    TESImageSpaceModifier* apMod2,
    float afStrength1,
    float afStrength2,
    NiAVObject* apTarget1,
    NiAVObject* apTarget2
) {
    float fDOFRange2;
    NiPoint2 radialBlurCenter;
    NiPoint2 kDOFParams;
    float fDOFDistance2;
    float fDOFRange1;
    float fDOFDistance1;
    float fBlurVal;
    if (!apMod1) {
        apMod1 = TESImageSpaceModifier::QDefault();
    }
    if (!apMod2) {
        apMod2 = TESImageSpaceModifier::QDefault();
    }
    float fTime = 0.0f;
    ImageSpaceManager* pManager = BSShaderManager::GetImageSpaceManager();
    float fStrength = (afStrength2 - afStrength1) * afPercent + afStrength1;
    for (int32_t i = 0; i < 21; ++i) {
        float fVal[2];
        for (int32_t j = 0; j < 2; ++j) {
            float fVal1;
            float fVal2;
            apMod1->ppInterpolator[i][j].Update(fTime, nullptr, fVal1);
            apMod2->ppInterpolator[i][j].Update(fTime, nullptr, fVal2);
            fVal[j] = (fVal2 - fVal1) * afPercent + fVal1;
        }
        if (i >= 0 && i <= 13) {
            pManager->ApplyParamModifier(ImageSpaceManager::EFFECT_HDR, static_cast<uint32_t>(i), fVal, fStrength);
        }
        if (i >= 14 && i <= 16) {
            pManager->ApplyParamModifier(ImageSpaceManager::EFFECT_BLOOM, i - 14, fVal, fStrength);
        }
        if (i >= 17 && i <= 20) {
            pManager->ApplyParamModifier(ImageSpaceManager::EFFECT_CINEMATIC, i - 17, fVal, fStrength);
        }
    }
    NiColorA kColVal(0.0f, 0.0f, 0.0f, 0.0f);
    NiColorA kColVal1(0.0f, 0.0f, 0.0f, 0.0f);
    NiColorA kColVal2(0.0f, 0.0f, 0.0f, 0.0f);
    apMod1->TintColorInterpolator.Update(fTime, nullptr, kColVal1);
    apMod2->TintColorInterpolator.Update(fTime, nullptr, kColVal2);
    NiColorA kTintDelta(kColVal2 - kColVal1);
    kColVal = kColVal1 + afPercent * kTintDelta;
    kColVal.a = kColVal.a * fStrength;
    BSClamp(kColVal.a, 0.0f, 1.0f);
    pManager->ApplyTintColorParamModifier(kColVal);
    apMod1->FadeColorInterpolator.Update(fTime, nullptr, kColVal1);
    apMod2->FadeColorInterpolator.Update(fTime, nullptr, kColVal2);
    NiColorA kFadeDelta(kColVal2 - kColVal1);
    kColVal = kColVal1 + afPercent * kFadeDelta;
    kColVal.a = kColVal.a * fStrength;
    BSClamp(kColVal.a, 0.0f, 1.0f);
    pManager->ApplyFadeColorParamModifier(kColVal);
    float fVal1;
    float fVal2;
    apMod1->BlurInterpolator.Update(fTime, nullptr, fVal1);
    apMod2->BlurInterpolator.Update(fTime, nullptr, fVal2);
    float fBlur = (fVal2 - fVal1) * afPercent + fVal1;
    apMod1->DoubleInterpolator.Update(fTime, nullptr, fVal1);
    apMod2->DoubleInterpolator.Update(fTime, nullptr, fVal2);
    float fDoubleVal = (fVal2 - fVal1) * afPercent + fVal1;
    apMod1->RadialBlurStrengthInterpolator.Update(fTime, nullptr, fVal1);
    apMod2->RadialBlurStrengthInterpolator.Update(fTime, nullptr, fVal2);
    float fRadialBlurStr = (fVal2 - fVal1) * afPercent + fVal1;
    apMod1->RadialBlurRampupInterpolator.Update(fTime, nullptr, fVal1);
    apMod2->RadialBlurRampupInterpolator.Update(fTime, nullptr, fVal2);
    float fRadialBlurRampup = (fVal2 - fVal1) * afPercent + fVal1;
    apMod1->RadialBlurStartInterpolator.Update(fTime, nullptr, fVal1);
    apMod2->RadialBlurStartInterpolator.Update(fTime, nullptr, fVal2);
    float fRadialBlurStart = (fVal2 - fVal1) * afPercent + fVal1;
    apMod1->RadialBlurRampDownInterpolator.Update(fTime, nullptr, fVal1);
    apMod2->RadialBlurRampDownInterpolator.Update(fTime, nullptr, fVal2);
    float fRadialBlurRampDown = (fVal2 - fVal1) * afPercent + fVal1;
    apMod1->RadialBlurDownStartInterpolator.Update(fTime, nullptr, fVal1);
    apMod2->RadialBlurDownStartInterpolator.Update(fTime, nullptr, fVal2);
    float fRadialBlurDownStart = (fVal2 - fVal1) * afPercent + fVal1;
    apMod1->DepthOfFieldStrengthInterpolator.Update(fTime, nullptr, fVal1);
    apMod2->DepthOfFieldStrengthInterpolator.Update(fTime, nullptr, fVal2);
    float fDOFStrength = (fVal2 - fVal1) * afPercent + fVal1;
    apMod1->DepthOfFieldDistanceInterpolator.Update(fTime, nullptr, fDOFDistance1);
    apMod2->DepthOfFieldDistanceInterpolator.Update(fTime, nullptr, fDOFDistance2);
    apMod1->DepthOfFieldRangeInterpolator.Update(fTime, nullptr, fDOFRange1);
    apMod2->DepthOfFieldRangeInterpolator.Update(fTime, nullptr, fDOFRange2);
    apMod1->MotionBlurStrengthInterpolator.Update(fTime, nullptr, fVal1);
    apMod2->MotionBlurStrengthInterpolator.Update(fTime, nullptr, fVal2);
    float fMotionBlurStr = (fVal2 - fVal1) * afPercent + fVal1;
    fBlurVal = fBlur * fStrength;
    BSClamp(fBlurVal, 0.0f, 7.0f);
    pManager->ApplyBlurRadiusParamModifier(fBlurVal);
    float fScaledDoubleVision = fDoubleVal * fStrength;
    pManager->ApplyDoubleVisionParamModifier(fScaledDoubleVision);
    NiPoint2 radialBlurCenter1(0.5f, 0.5f);
    NiPoint2 radialBlurCenter2(0.5f, 0.5f);
    if (apMod1->Data.bUseTargetForRadialBlur && apTarget1) {
        apTarget1->UpdateWorldBound();
        ImageSpaceManager::GetRadialBlurCenterFromWorldBound(&apTarget1->GetWorldBound(), &radialBlurCenter1);
    }

    if (apMod2->Data.bUseTargetForRadialBlur && apTarget2) {
        apTarget1->UpdateWorldBound();
        ImageSpaceManager::GetRadialBlurCenterFromWorldBound(&apTarget2->GetWorldBound(), &radialBlurCenter2);
    }

    if (!apMod1->Data.bUseTargetForRadialBlur) {
        float fCenterY = apMod1->Data.RadialBlurCenter.y;
        radialBlurCenter1.x = apMod1->Data.RadialBlurCenter.x;
        radialBlurCenter1.y = fCenterY;
    }
    if (!apMod2->Data.bUseTargetForRadialBlur) {
        float fCenterY = apMod2->Data.RadialBlurCenter.y;
        radialBlurCenter2.x = apMod2->Data.RadialBlurCenter.x;
        radialBlurCenter2.y = fCenterY;
    }
    NiPoint2 kCenterDelta = radialBlurCenter2 - radialBlurCenter1;
    NiPoint2 kScaledCenterDelta;
    NiPoint2* pScaledCenterDelta = NiPoint2::Scale_(&kScaledCenterDelta, afPercent, &kCenterDelta);
    NiPoint2 kBlendedCenter = radialBlurCenter1 + *pScaledCenterDelta;
    float fBlendedCenterY = kBlendedCenter.y;
    radialBlurCenter.x = kBlendedCenter.x;
    radialBlurCenter.y = fBlendedCenterY;
    float fScaledRadialBlur = fRadialBlurStr * fStrength;
    pManager->ApplyRadialBlurParamModifier(
        fScaledRadialBlur,
        fRadialBlurRampup,
        fRadialBlurStart,
        fRadialBlurRampDown,
        fRadialBlurDownStart,
        radialBlurCenter
    );
    NiPoint2 kDOFParams1(0.0f, 0.0f);
    NiPoint2 kDOFParams2(0.0f, 0.0f);
    if (apMod1->Data.bUseTargetForDepthOfField && apTarget1) {
        apTarget1->UpdateWorldBound();
        pManager->GetDepthOfFieldParamsFromPoint(&apTarget1->GetWorldBound().m_kCenter, &kDOFParams1);
    }

    if (apMod2->Data.bUseTargetForDepthOfField && apTarget2) {
        apTarget2->UpdateWorldBound();
        pManager->GetDepthOfFieldParamsFromPoint(&apTarget2->GetWorldBound().m_kCenter, &kDOFParams2);
    }

    if (!apMod1->Data.bUseTargetForDepthOfField) {
        kDOFParams1.x = fDOFDistance1;
        kDOFParams1.y = fDOFRange1;
    }
    if (!apMod2->Data.bUseTargetForDepthOfField) {
        kDOFParams2.x = fDOFDistance2;
        kDOFParams2.y = fDOFRange2;
    }
    NiPoint2 kDOFDelta = kDOFParams2 - kDOFParams1;
    NiPoint2 kScaledDOFDelta;
    NiPoint2* pScaledDOFDelta = NiPoint2::Scale_(&kScaledDOFDelta, afPercent, &kDOFDelta);
    NiPoint2 kBlendedDOFParams = kDOFParams1 + *pScaledDOFDelta;
    kDOFParams.x = kBlendedDOFParams.x;
    kDOFParams.y = kBlendedDOFParams.y;
    uint8_t ucDOFMode = afPercent < 0.5 ? apMod1->Data.ucDepthOfFieldMode : apMod2->Data.ucDepthOfFieldMode;
    float fScaledDOFStrength = fDOFStrength * fStrength;
    pManager->ApplyDepthOfFieldParamModifier(fScaledDOFStrength, ucDOFMode, 0.0f, kDOFParams.x, kDOFParams.y);
    pManager->ApplyMotionBlurStrengthParamModifier(fMotionBlurStr);
}

void TESImageSpaceModifier::SetEnable(bool abEnable) {
    TESImageSpaceModifier::bEnable = abEnable;
}

void TESImageSpaceModifier::AllowGetHit(bool abEnable) {
    g_bEnableHitAndConcussionIMODs = abEnable;
}

void TESImageSpaceModifier::AdvanceCurrentTime(float afAmount) {
    TESImageSpaceModifier::fCurrentTime = TESImageSpaceModifier::fCurrentTime + afAmount;
}

TESImageSpaceModifier* TESImageSpaceModifier::GetExplosionInFace() {
    if (g_explosionInFaceIMOD) {
        return g_explosionInFaceIMOD;
    }
    TESForm* pForm = TESForm::GetFormByNumericID(358);
    g_explosionInFaceIMOD = dynamic_cast<TESImageSpaceModifier*>(pForm);
    return g_explosionInFaceIMOD;
}
