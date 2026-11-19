#include "Fallout Shared/TESImageSpaceModifier.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/SettingCollection.h"
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
    ENDIAN(fDuration, false);
    for (int32_t i = 0; i < NUM_OP_PARAMS; ++i) {
        for (int32_t j = 0; j < IMAOP_COUNT; ++j) {
            ENDIAN(ppuiKeySize[i][j], false);
        }
    }
    ENDIAN(uiBlurKeySize, false);
    ENDIAN(uiDoubleKeySize, false);
    ENDIAN(uiTintColorKeySize, false);
    ENDIAN(uiFadeColorKeySize, false);
    ENDIAN(uiRadialBlurStrengthKeySize, false);
    ENDIAN(uiRadialBlurRampupKeySize, false);
    ENDIAN(uiRadialBlurStartKeySize, false);
    ENDIAN(uiRadialBlurRampDownKeySize, false);
    ENDIAN(uiRadialBlurDownStartKeySize, false);
    ENDIAN(RadialBlurCenter.x, false);
    ENDIAN(RadialBlurCenter.y, false);
    ENDIAN(uiDepthOfFieldStrengthKeySize, false);
    ENDIAN(uiDepthOfFieldDistanceKeySize, false);
    ENDIAN(uiDepthOfFieldRangeKeySize, false);
    ENDIAN(uiMotionBlurStrengthKeySize, false);
}

// The interpolator arrays and members are built with no data, as the game does.
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

    void* pMemory;
    int32_t i;
    for (i = 0; i < NUM_OP_PARAMS; ++i) {
        for (int32_t j = 0; j < IMAOP_COUNT; ++j) {
            Data.ppuiKeySize[i][j] = 2;
            pMemory = ourMalloc(2 * sizeof(NiLinFloatKey) + sizeof(size_t));
            if (pMemory) {
                *static_cast<size_t*>(pMemory) = 2;
                pppFloatKey[i][j] = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[2];
            }
            else {
                pppFloatKey[i][j] = nullptr;
            }
            for (int32_t k = 0; k < 2; ++k) {
                pppFloatKey[i][j][k].SetTime(fDefaultKeyTimes[k]);
                pppFloatKey[i][j][k].SetValue(fOpDefault[j]);
            }
            ppInterpolator[i][j].ReplaceKeys(pppFloatKey[i][j], Data.ppuiKeySize[i][j], NiAnimationKey::LINKEY);
        }
    }

    Data.uiTintColorKeySize = 2;
    pMemory = ourMalloc(2 * sizeof(NiLinColorKey) + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = 2;
        pTintColorKey = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinColorKey[2];
    }
    else {
        pTintColorKey = nullptr;
    }
    for (i = 0; i < 2; ++i) {
        pTintColorKey[i].SetTime(fDefaultKeyTimes[i]);
        NiColorA kColor(1.0f, 1.0f, 1.0f, 0.0f);
        pTintColorKey[i].SetColor(kColor);
    }
    TintColorInterpolator.ReplaceKeys(pTintColorKey, Data.uiTintColorKeySize, NiAnimationKey::LINKEY);

    Data.uiFadeColorKeySize = 2;
    pMemory = ourMalloc(2 * sizeof(NiLinColorKey) + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = 2;
        pFadeColorKey = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinColorKey[2];
    }
    else {
        pFadeColorKey = nullptr;
    }
    for (i = 0; i < 2; ++i) {
        pFadeColorKey[i].SetTime(fDefaultKeyTimes[i]);
        NiColorA kColor(1.0f, 1.0f, 1.0f, 0.0f);
        pFadeColorKey[i].SetColor(kColor);
    }
    FadeColorInterpolator.ReplaceKeys(pFadeColorKey, Data.uiFadeColorKeySize, NiAnimationKey::LINKEY);

    Data.uiBlurKeySize = 2;
    pMemory = ourMalloc(2 * sizeof(NiLinFloatKey) + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = 2;
        pBlurFloatKey = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[2];
    }
    else {
        pBlurFloatKey = nullptr;
    }
    for (i = 0; i < 2; ++i) {
        pBlurFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pBlurFloatKey[i].SetValue(0.0f);
    }
    BlurInterpolator.ReplaceKeys(pBlurFloatKey, Data.uiBlurKeySize, NiAnimationKey::LINKEY);

    Data.uiDoubleKeySize = 2;
    pMemory = ourMalloc(2 * sizeof(NiLinFloatKey) + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = 2;
        pDoubleFloatKey = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[2];
    }
    else {
        pDoubleFloatKey = nullptr;
    }
    for (i = 0; i < 2; ++i) {
        pDoubleFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pDoubleFloatKey[i].SetValue(0.0f);
    }
    DoubleInterpolator.ReplaceKeys(pDoubleFloatKey, Data.uiDoubleKeySize, NiAnimationKey::LINKEY);

    Data.uiRadialBlurStrengthKeySize = 2;
    pMemory = ourMalloc(2 * sizeof(NiLinFloatKey) + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = 2;
        pRadialBlurStrengthFloatKey = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[2];
    }
    else {
        pRadialBlurStrengthFloatKey = nullptr;
    }
    for (i = 0; i < 2; ++i) {
        pRadialBlurStrengthFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pRadialBlurStrengthFloatKey[i].SetValue(0.0f);
    }
    RadialBlurStrengthInterpolator.ReplaceKeys(
        pRadialBlurStrengthFloatKey, Data.uiRadialBlurStrengthKeySize, NiAnimationKey::LINKEY
    );

    Data.uiRadialBlurRampupKeySize = 2;
    pMemory = ourMalloc(2 * sizeof(NiLinFloatKey) + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = 2;
        pRadialBlurRampupFloatKey = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[2];
    }
    else {
        pRadialBlurRampupFloatKey = nullptr;
    }
    for (i = 0; i < 2; ++i) {
        pRadialBlurRampupFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pRadialBlurRampupFloatKey[i].SetValue(0.0f);
    }
    RadialBlurRampupInterpolator.ReplaceKeys(
        pRadialBlurRampupFloatKey, Data.uiRadialBlurRampupKeySize, NiAnimationKey::LINKEY
    );

    Data.uiRadialBlurStartKeySize = 2;
    pMemory = ourMalloc(2 * sizeof(NiLinFloatKey) + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = 2;
        pRadialBlurStartFloatKey = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[2];
    }
    else {
        pRadialBlurStartFloatKey = nullptr;
    }
    for (i = 0; i < 2; ++i) {
        pRadialBlurStartFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pRadialBlurStartFloatKey[i].SetValue(0.0f);
    }
    RadialBlurStartInterpolator.ReplaceKeys(
        pRadialBlurStartFloatKey, Data.uiRadialBlurStartKeySize, NiAnimationKey::LINKEY
    );

    Data.uiRadialBlurRampDownKeySize = 2;
    pMemory = ourMalloc(2 * sizeof(NiLinFloatKey) + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = 2;
        pRadialBlurRampDownFloatKey = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[2];
    }
    else {
        pRadialBlurRampDownFloatKey = nullptr;
    }
    for (i = 0; i < 2; ++i) {
        pRadialBlurRampDownFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pRadialBlurRampDownFloatKey[i].SetValue(0.0f);
    }
    RadialBlurRampDownInterpolator.ReplaceKeys(
        pRadialBlurRampDownFloatKey, Data.uiRadialBlurRampDownKeySize, NiAnimationKey::LINKEY
    );

    Data.uiRadialBlurDownStartKeySize = 2;
    pMemory = ourMalloc(2 * sizeof(NiLinFloatKey) + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = 2;
        pRadialBlurDownStartFloatKey = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[2];
    }
    else {
        pRadialBlurDownStartFloatKey = nullptr;
    }
    for (i = 0; i < 2; ++i) {
        pRadialBlurDownStartFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pRadialBlurDownStartFloatKey[i].SetValue(0.0f);
    }
    RadialBlurDownStartInterpolator.ReplaceKeys(
        pRadialBlurDownStartFloatKey, Data.uiRadialBlurDownStartKeySize, NiAnimationKey::LINKEY
    );

    Data.uiDepthOfFieldStrengthKeySize = 2;
    pMemory = ourMalloc(2 * sizeof(NiLinFloatKey) + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = 2;
        pDepthOfFieldStrengthFloatKey = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[2];
    }
    else {
        pDepthOfFieldStrengthFloatKey = nullptr;
    }
    for (i = 0; i < 2; ++i) {
        pDepthOfFieldStrengthFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pDepthOfFieldStrengthFloatKey[i].SetValue(0.0f);
    }
    DepthOfFieldStrengthInterpolator.ReplaceKeys(
        pDepthOfFieldStrengthFloatKey, Data.uiDepthOfFieldStrengthKeySize, NiAnimationKey::LINKEY
    );

    Data.uiDepthOfFieldDistanceKeySize = 2;
    pMemory = ourMalloc(2 * sizeof(NiLinFloatKey) + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = 2;
        pDepthOfFieldDistanceFloatKey = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[2];
    }
    else {
        pDepthOfFieldDistanceFloatKey = nullptr;
    }
    for (i = 0; i < 2; ++i) {
        pDepthOfFieldDistanceFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pDepthOfFieldDistanceFloatKey[i].SetValue(0.0f);
    }
    DepthOfFieldDistanceInterpolator.ReplaceKeys(
        pDepthOfFieldDistanceFloatKey, Data.uiDepthOfFieldDistanceKeySize, NiAnimationKey::LINKEY
    );

    Data.uiDepthOfFieldRangeKeySize = 2;
    pMemory = ourMalloc(2 * sizeof(NiLinFloatKey) + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = 2;
        pDepthOfFieldRangeFloatKey = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[2];
    }
    else {
        pDepthOfFieldRangeFloatKey = nullptr;
    }
    for (i = 0; i < 2; ++i) {
        pDepthOfFieldRangeFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pDepthOfFieldRangeFloatKey[i].SetValue(0.0f);
    }
    DepthOfFieldRangeInterpolator.ReplaceKeys(
        pDepthOfFieldRangeFloatKey, Data.uiDepthOfFieldRangeKeySize, NiAnimationKey::LINKEY
    );

    Data.uiMotionBlurStrengthKeySize = 2;
    pMemory = ourMalloc(2 * sizeof(NiLinFloatKey) + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = 2;
        pMotionBlurStrengthFloatKey = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[2];
    }
    else {
        pMotionBlurStrengthFloatKey = nullptr;
    }
    for (i = 0; i < 2; ++i) {
        pMotionBlurStrengthFloatKey[i].SetTime(fDefaultKeyTimes[i]);
        pMotionBlurStrengthFloatKey[i].SetValue(0.0f);
    }
    MotionBlurStrengthInterpolator.ReplaceKeys(
        pMotionBlurStrengthFloatKey, Data.uiMotionBlurStrengthKeySize, NiAnimationKey::LINKEY
    );

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
    FlushData();
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
    // Set by the data chunk but never read; kept deliberately, as the game does.
    [[maybe_unused]] uint8_t ucDataLoaded = 0;
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
                ucDataLoaded = 1;
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
                size_t uiBytes =
                    uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
                void* pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
                if (pMemory) {
                    *static_cast<size_t*>(pMemory) = uiNumKeys;
                    pBlurFloatKey =
                        ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
                }
                else {
                    pBlurFloatKey = nullptr;
                }
                apFile->GetChunkData(pBlurFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pBlurFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        ENDIAN(puiDwords[i], false);
                    }
                }
                BlurInterpolator.ReplaceKeys(pBlurFloatKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case VNAM_ID: {
                uint32_t uiNumKeys = Data.uiDoubleKeySize;
                size_t uiBytes =
                    uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
                void* pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
                if (pMemory) {
                    *static_cast<size_t*>(pMemory) = uiNumKeys;
                    pDoubleFloatKey =
                        ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
                }
                else {
                    pDoubleFloatKey = nullptr;
                }
                apFile->GetChunkData(pDoubleFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pDoubleFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        ENDIAN(puiDwords[i], false);
                    }
                }
                DoubleInterpolator.ReplaceKeys(pDoubleFloatKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case RNAM_ID: {
                uint32_t uiNumKeys = Data.uiRadialBlurStrengthKeySize;
                size_t uiBytes =
                    uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
                void* pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
                if (pMemory) {
                    *static_cast<size_t*>(pMemory) = uiNumKeys;
                    pRadialBlurStrengthFloatKey =
                        ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
                }
                else {
                    pRadialBlurStrengthFloatKey = nullptr;
                }
                apFile->GetChunkData(pRadialBlurStrengthFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pRadialBlurStrengthFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        ENDIAN(puiDwords[i], false);
                    }
                }
                RadialBlurStrengthInterpolator.ReplaceKeys(
                    pRadialBlurStrengthFloatKey, uiNumKeys, NiAnimationKey::LINKEY
                );
                break;
            }
            case SNAM_ID: {
                uint32_t uiNumKeys = Data.uiRadialBlurRampupKeySize;
                size_t uiBytes =
                    uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
                void* pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
                if (pMemory) {
                    *static_cast<size_t*>(pMemory) = uiNumKeys;
                    pRadialBlurRampupFloatKey =
                        ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
                }
                else {
                    pRadialBlurRampupFloatKey = nullptr;
                }
                apFile->GetChunkData(pRadialBlurRampupFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pRadialBlurRampupFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        ENDIAN(puiDwords[i], false);
                    }
                }
                RadialBlurRampupInterpolator.ReplaceKeys(pRadialBlurRampupFloatKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case UNAM_ID: {
                uint32_t uiNumKeys = Data.uiRadialBlurStartKeySize;
                size_t uiBytes =
                    uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
                void* pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
                if (pMemory) {
                    *static_cast<size_t*>(pMemory) = uiNumKeys;
                    pRadialBlurStartFloatKey =
                        ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
                }
                else {
                    pRadialBlurStartFloatKey = nullptr;
                }
                apFile->GetChunkData(pRadialBlurStartFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pRadialBlurStartFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        ENDIAN(puiDwords[i], false);
                    }
                }
                RadialBlurStartInterpolator.ReplaceKeys(pRadialBlurStartFloatKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case NAM1_ID: {
                uint32_t uiNumKeys = Data.uiRadialBlurRampDownKeySize;
                size_t uiBytes =
                    uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
                void* pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
                if (pMemory) {
                    *static_cast<size_t*>(pMemory) = uiNumKeys;
                    pRadialBlurRampDownFloatKey =
                        ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
                }
                else {
                    pRadialBlurRampDownFloatKey = nullptr;
                }
                apFile->GetChunkData(pRadialBlurRampDownFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pRadialBlurRampDownFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        ENDIAN(puiDwords[i], false);
                    }
                }
                RadialBlurRampDownInterpolator.ReplaceKeys(
                    pRadialBlurRampDownFloatKey, uiNumKeys, NiAnimationKey::LINKEY
                );
                break;
            }
            case NAM2_ID: {
                uint32_t uiNumKeys = Data.uiRadialBlurDownStartKeySize;
                size_t uiBytes =
                    uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
                void* pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
                if (pMemory) {
                    *static_cast<size_t*>(pMemory) = uiNumKeys;
                    pRadialBlurDownStartFloatKey =
                        ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
                }
                else {
                    pRadialBlurDownStartFloatKey = nullptr;
                }
                apFile->GetChunkData(pRadialBlurDownStartFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pRadialBlurDownStartFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        ENDIAN(puiDwords[i], false);
                    }
                }
                RadialBlurDownStartInterpolator.ReplaceKeys(
                    pRadialBlurDownStartFloatKey, uiNumKeys, NiAnimationKey::LINKEY
                );
                break;
            }
            case TNAM_ID: {
                uint32_t uiNumKeys = Data.uiTintColorKeySize;
                size_t uiBytes =
                    uiNumKeys > SIZE_MAX / sizeof(NiLinColorKey) ? SIZE_MAX : sizeof(NiLinColorKey) * uiNumKeys;
                void* pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
                if (pMemory) {
                    *static_cast<size_t*>(pMemory) = uiNumKeys;
                    pTintColorKey =
                        ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinColorKey[uiNumKeys];
                }
                else {
                    pTintColorKey = nullptr;
                }
                apFile->GetChunkData(pTintColorKey, 20 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pTintColorKey);
                    for (uint32_t i = 0; i < (20 * uiNumKeys) >> 2; ++i) {
                        ENDIAN(puiDwords[i], false);
                    }
                }
                TintColorInterpolator.ReplaceKeys(pTintColorKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case NAM3_ID: {
                uint32_t uiNumKeys = Data.uiFadeColorKeySize;
                size_t uiBytes =
                    uiNumKeys > SIZE_MAX / sizeof(NiLinColorKey) ? SIZE_MAX : sizeof(NiLinColorKey) * uiNumKeys;
                void* pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
                if (pMemory) {
                    *static_cast<size_t*>(pMemory) = uiNumKeys;
                    pFadeColorKey =
                        ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinColorKey[uiNumKeys];
                }
                else {
                    pFadeColorKey = nullptr;
                }
                apFile->GetChunkData(pFadeColorKey, 20 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pFadeColorKey);
                    for (uint32_t i = 0; i < (20 * uiNumKeys) >> 2; ++i) {
                        ENDIAN(puiDwords[i], false);
                    }
                }
                FadeColorInterpolator.ReplaceKeys(pFadeColorKey, uiNumKeys, NiAnimationKey::LINKEY);
                break;
            }
            case WNAM_ID: {
                uint32_t uiNumKeys = Data.uiDepthOfFieldStrengthKeySize;
                size_t uiBytes =
                    uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
                void* pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
                if (pMemory) {
                    *static_cast<size_t*>(pMemory) = uiNumKeys;
                    pDepthOfFieldStrengthFloatKey =
                        ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
                }
                else {
                    pDepthOfFieldStrengthFloatKey = nullptr;
                }
                apFile->GetChunkData(pDepthOfFieldStrengthFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pDepthOfFieldStrengthFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        ENDIAN(puiDwords[i], false);
                    }
                }
                DepthOfFieldStrengthInterpolator.ReplaceKeys(
                    pDepthOfFieldStrengthFloatKey, uiNumKeys, NiAnimationKey::LINKEY
                );
                break;
            }
            case XNAM_ID: {
                uint32_t uiNumKeys = Data.uiDepthOfFieldDistanceKeySize;
                size_t uiBytes =
                    uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
                void* pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
                if (pMemory) {
                    *static_cast<size_t*>(pMemory) = uiNumKeys;
                    pDepthOfFieldDistanceFloatKey =
                        ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
                }
                else {
                    pDepthOfFieldDistanceFloatKey = nullptr;
                }
                apFile->GetChunkData(pDepthOfFieldDistanceFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pDepthOfFieldDistanceFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        ENDIAN(puiDwords[i], false);
                    }
                }
                DepthOfFieldDistanceInterpolator.ReplaceKeys(
                    pDepthOfFieldDistanceFloatKey, uiNumKeys, NiAnimationKey::LINKEY
                );
                break;
            }
            case YNAM_ID: {
                uint32_t uiNumKeys = Data.uiDepthOfFieldRangeKeySize;
                size_t uiBytes =
                    uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
                void* pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
                if (pMemory) {
                    *static_cast<size_t*>(pMemory) = uiNumKeys;
                    pDepthOfFieldRangeFloatKey =
                        ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
                }
                else {
                    pDepthOfFieldRangeFloatKey = nullptr;
                }
                apFile->GetChunkData(pDepthOfFieldRangeFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pDepthOfFieldRangeFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        ENDIAN(puiDwords[i], false);
                    }
                }
                DepthOfFieldRangeInterpolator.ReplaceKeys(
                    pDepthOfFieldRangeFloatKey, uiNumKeys, NiAnimationKey::LINKEY
                );
                break;
            }
            case NAM4_ID: {
                uint32_t uiNumKeys = Data.uiMotionBlurStrengthKeySize;
                size_t uiBytes =
                    uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
                void* pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
                if (pMemory) {
                    *static_cast<size_t*>(pMemory) = uiNumKeys;
                    pMotionBlurStrengthFloatKey =
                        ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
                }
                else {
                    pMotionBlurStrengthFloatKey = nullptr;
                }
                apFile->GetChunkData(pMotionBlurStrengthFloatKey, 8 * uiNumKeys);
                if (apFile->QEndian()) {
                    uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pMotionBlurStrengthFloatKey);
                    for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                        ENDIAN(puiDwords[i], false);
                    }
                }
                MotionBlurStrengthInterpolator.ReplaceKeys(
                    pMotionBlurStrengthFloatKey, uiNumKeys, NiAnimationKey::LINKEY
                );
                break;
            }
            default: {
                // the DNAI chunks: the parameter's index in the low six bits, the operation in bits 6 and 7
                if (uiChunk >= 0x44414900u && uiChunk <= 0x444149FFu) {
                    uint32_t uiIndex = uiChunk & 0x3Fu;
                    uint32_t uiOp = (uiChunk & 0xC0u) >> 6;
                    uint32_t uiNumKeys = Data.ppuiKeySize[uiIndex][uiOp];
                    size_t uiBytes =
                        uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
                    void* pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
                    if (pMemory) {
                        *static_cast<size_t*>(pMemory) = uiNumKeys;
                        pppFloatKey[uiIndex][uiOp] =
                            ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
                    }
                    else {
                        pppFloatKey[uiIndex][uiOp] = nullptr;
                    }
                    apFile->GetChunkData(pppFloatKey[uiIndex][uiOp], 8 * uiNumKeys);
                    if (apFile->QEndian()) {
                        uint32_t* puiDwords = reinterpret_cast<uint32_t*>(pppFloatKey[uiIndex][uiOp]);
                        for (uint32_t i = 0; i < (8 * uiNumKeys) >> 2; ++i) {
                            ENDIAN(puiDwords[i], false);
                        }
                    }
                    ppInterpolator[uiIndex][uiOp].ReplaceKeys(
                        pppFloatKey[uiIndex][uiOp], uiNumKeys, NiAnimationKey::LINKEY
                    );
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
    FlushData_0052ED90();
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    TESForm::__AddChunkData(DNAM_ID, &Data, 244u);
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    uint32_t v45 = 8 * Data.uiBlurKeySize >> 2;
    uint32_t* pendiandataM =
        static_cast<uint32_t*>(ourMalloc(v45 <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * v45 : SIZE_MAX));
    ourMemcpy(pendiandataM, pBlurFloatKey, sizeof(uint32_t) * v45);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i = 0; i < v45; ++i) {
            ENDIAN<uint32_t>(pendiandataM[i], false);
        }
    }
    TESForm::__AddChunkData(BNAM_ID, pendiandataM, 4 * v45);
    ourFree(pendiandataM);
    uint32_t v42 = 8 * Data.uiDoubleKeySize >> 2;
    uint32_t* pendiandataM_2 =
        static_cast<uint32_t*>(ourMalloc(v42 <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * v42 : SIZE_MAX));
    ourMemcpy(pendiandataM_2, pDoubleFloatKey, sizeof(uint32_t) * v42);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t j = 0; j < v42; ++j) {
            ENDIAN<uint32_t>(pendiandataM_2[j], false);
        }
    }
    TESForm::__AddChunkData(VNAM_ID, pendiandataM_2, 4 * v42);
    ourFree(pendiandataM_2);
    uint32_t v39 = 20 * Data.uiTintColorKeySize >> 2;
    uint32_t* pendiandataM_3 =
        static_cast<uint32_t*>(ourMalloc(v39 <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * v39 : SIZE_MAX));
    ourMemcpy(pendiandataM_3, pTintColorKey, sizeof(uint32_t) * v39);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t k = 0; k < v39; ++k) {
            ENDIAN<uint32_t>(pendiandataM_3[k], false);
        }
    }
    TESForm::__AddChunkData(TNAM_ID, pendiandataM_3, 4 * v39);
    ourFree(pendiandataM_3);
    uint32_t v36 = 20 * Data.uiFadeColorKeySize >> 2;
    uint32_t* pendiandataM_4 =
        static_cast<uint32_t*>(ourMalloc(v36 <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * v36 : SIZE_MAX));
    ourMemcpy(pendiandataM_4, pFadeColorKey, sizeof(uint32_t) * v36);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t m = 0; m < v36; ++m) {
            ENDIAN<uint32_t>(pendiandataM_4[m], false);
        }
    }
    TESForm::__AddChunkData(NAM3_ID, pendiandataM_4, 4 * v36);
    ourFree(pendiandataM_4);
    uint32_t v33 = 8 * Data.uiRadialBlurStrengthKeySize >> 2;
    uint32_t* pendiandataM_5 =
        static_cast<uint32_t*>(ourMalloc(v33 <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * v33 : SIZE_MAX));
    ourMemcpy(pendiandataM_5, pRadialBlurStrengthFloatKey, sizeof(uint32_t) * v33);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t n = 0; n < v33; ++n) {
            ENDIAN<uint32_t>(pendiandataM_5[n], false);
        }
    }
    TESForm::__AddChunkData(RNAM_ID, pendiandataM_5, 4 * v33);
    ourFree(pendiandataM_5);
    uint32_t v30 = 8 * Data.uiRadialBlurRampupKeySize >> 2;
    uint32_t* pendiandataM_6 =
        static_cast<uint32_t*>(ourMalloc(v30 <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * v30 : SIZE_MAX));
    ourMemcpy(pendiandataM_6, pRadialBlurRampupFloatKey, sizeof(uint32_t) * v30);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t ii = 0; ii < v30; ++ii) {
            ENDIAN<uint32_t>(pendiandataM_6[ii], false);
        }
    }
    TESForm::__AddChunkData(SNAM_ID, pendiandataM_6, 4 * v30);
    ourFree(pendiandataM_6);
    uint32_t v27 = 8 * Data.uiRadialBlurStartKeySize >> 2;
    uint32_t* pendiandataM_7 =
        static_cast<uint32_t*>(ourMalloc(v27 <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * v27 : SIZE_MAX));
    ourMemcpy(pendiandataM_7, pRadialBlurStartFloatKey, sizeof(uint32_t) * v27);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t jj = 0; jj < v27; ++jj) {
            ENDIAN<uint32_t>(pendiandataM_7[jj], false);
        }
    }
    TESForm::__AddChunkData(UNAM_ID, pendiandataM_7, 4 * v27);
    ourFree(pendiandataM_7);
    uint32_t v24 = 8 * Data.uiRadialBlurRampDownKeySize >> 2;
    uint32_t* pendiandataM_8 =
        static_cast<uint32_t*>(ourMalloc(v24 <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * v24 : SIZE_MAX));
    ourMemcpy(pendiandataM_8, pRadialBlurRampDownFloatKey, sizeof(uint32_t) * v24);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t kk = 0; kk < v24; ++kk) {
            ENDIAN<uint32_t>(pendiandataM_8[kk], false);
        }
    }
    TESForm::__AddChunkData(NAM1_ID, pendiandataM_8, 4 * v24);
    ourFree(pendiandataM_8);
    uint32_t v21 = 8 * Data.uiRadialBlurDownStartKeySize >> 2;
    uint32_t* pendiandataM_9 =
        static_cast<uint32_t*>(ourMalloc(v21 <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * v21 : SIZE_MAX));
    ourMemcpy(pendiandataM_9, pRadialBlurDownStartFloatKey, sizeof(uint32_t) * v21);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t mm = 0; mm < v21; ++mm) {
            ENDIAN<uint32_t>(pendiandataM_9[mm], false);
        }
    }
    TESForm::__AddChunkData(NAM2_ID, pendiandataM_9, 4 * v21);
    ourFree(pendiandataM_9);
    uint32_t v18 = 8 * Data.uiDepthOfFieldStrengthKeySize >> 2;
    uint32_t* pendiandataM_10 =
        static_cast<uint32_t*>(ourMalloc(v18 <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * v18 : SIZE_MAX));
    ourMemcpy(pendiandataM_10, pDepthOfFieldStrengthFloatKey, sizeof(uint32_t) * v18);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t nn = 0; nn < v18; ++nn) {
            ENDIAN<uint32_t>(pendiandataM_10[nn], false);
        }
    }
    TESForm::__AddChunkData(WNAM_ID, pendiandataM_10, 4 * v18);
    ourFree(pendiandataM_10);
    uint32_t v15 = 8 * Data.uiDepthOfFieldDistanceKeySize >> 2;
    uint32_t* pendiandataM_11 =
        static_cast<uint32_t*>(ourMalloc(v15 <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * v15 : SIZE_MAX));
    ourMemcpy(pendiandataM_11, pDepthOfFieldDistanceFloatKey, sizeof(uint32_t) * v15);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i1 = 0; i1 < v15; ++i1) {
            ENDIAN<uint32_t>(pendiandataM_11[i1], false);
        }
    }
    TESForm::__AddChunkData(XNAM_ID, pendiandataM_11, 4 * v15);
    ourFree(pendiandataM_11);
    uint32_t v12 = 8 * Data.uiDepthOfFieldRangeKeySize >> 2;
    uint32_t* pendiandataM_12 =
        static_cast<uint32_t*>(ourMalloc(v12 <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * v12 : SIZE_MAX));
    ourMemcpy(pendiandataM_12, pDepthOfFieldRangeFloatKey, sizeof(uint32_t) * v12);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i2 = 0; i2 < v12; ++i2) {
            ENDIAN<uint32_t>(pendiandataM_12[i2], false);
        }
    }
    TESForm::__AddChunkData(YNAM_ID, pendiandataM_12, 4 * v12);
    ourFree(pendiandataM_12);
    uint32_t v9 = 8 * Data.uiMotionBlurStrengthKeySize >> 2;
    uint32_t* pendiandataM_13 =
        static_cast<uint32_t*>(ourMalloc(v9 <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * v9 : SIZE_MAX));
    ourMemcpy(pendiandataM_13, pMotionBlurStrengthFloatKey, sizeof(uint32_t) * v9);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i3 = 0; i3 < v9; ++i3) {
            ENDIAN<uint32_t>(pendiandataM_13[i3], false);
        }
    }
    TESForm::__AddChunkData(NAM4_ID, pendiandataM_13, 4 * v9);
    ourFree(pendiandataM_13);
    for (int32_t i4 = 0; i4 < 21; ++i4) {
        for (int32_t i5 = 0; i5 < 2; ++i5) {
            uint32_t v4 = 8 * Data.ppuiKeySize[i4][i5] >> 2;
            uint32_t* pendiandataM_14 =
                static_cast<uint32_t*>(ourMalloc(v4 <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * v4 : SIZE_MAX));
            ourMemcpy(pendiandataM_14, pppFloatKey[i4][i5], sizeof(uint32_t) * v4);
            if (TESForm::QEndianSwapOnSave()) {
                for (uint32_t iM = 0; iM < v4; ++iM) {
                    ENDIAN<uint32_t>(pendiandataM_14[iM], false);
                }
            }
            // The key arrays' chunk IDs are computed: 'DAI' with the key index in the low byte, +0x40 for the second
            // key.
            TESForm::__AddChunkData(static_cast<CHUNK_ID>(i5 << 6 | (i4 | 0x44414900)), pendiandataM_14, 4 * v4);
            ourFree(pendiandataM_14);
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
                "MASTERFILE: Could not find pIntroSound (%08X) for ImageSpaceModifier '%s' (%08X).", iID, pEditorID,
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
                "MASTERFILE: Could not find pOutroSound (%08X) for ImageSpaceModifier '%s' (%08X).", iID, pEditorID,
                iFormID
            );
        }
    }
    SetInitialized(true);
}

// Each key array is a new[] of the source's key count, as Load builds it (the count ahead of the keys, then the keys),
// filled from the source's keys and handed to its interpolator. The new[] block is raw memory: the keys start one
// count in.
void TESImageSpaceModifier::Copy(TESForm* apCopy) {
    TESImageSpaceModifier* pCopy = dynamic_cast<TESImageSpaceModifier*>(apCopy);
    if (!pCopy) {
        return;
    }
    pCopy->FlushData_0052ED90();
    CopyAllComponents(pCopy);
    ClearData();
    pIntroSound = pCopy->pIntroSound;
    pOutroSound = pCopy->pOutroSound;
    ourMemcpy(&Data, &pCopy->Data, sizeof(Data));

    uint32_t uiNumKeys = Data.uiBlurKeySize;
    size_t uiBytes = uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
    void* pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = uiNumKeys;
        pBlurFloatKey = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
    }
    else {
        pBlurFloatKey = nullptr;
    }
    ourMemcpy(pBlurFloatKey, pCopy->pBlurFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    BlurInterpolator.ReplaceKeys(pBlurFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiDoubleKeySize;
    uiBytes = uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
    pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = uiNumKeys;
        pDoubleFloatKey = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
    }
    else {
        pDoubleFloatKey = nullptr;
    }
    ourMemcpy(pDoubleFloatKey, pCopy->pDoubleFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    DoubleInterpolator.ReplaceKeys(pDoubleFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiTintColorKeySize;
    uiBytes = uiNumKeys > SIZE_MAX / sizeof(NiLinColorKey) ? SIZE_MAX : sizeof(NiLinColorKey) * uiNumKeys;
    pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = uiNumKeys;
        pTintColorKey = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinColorKey[uiNumKeys];
    }
    else {
        pTintColorKey = nullptr;
    }
    ourMemcpy(pTintColorKey, pCopy->pTintColorKey, sizeof(NiLinColorKey) * uiNumKeys);
    TintColorInterpolator.ReplaceKeys(pTintColorKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiFadeColorKeySize;
    uiBytes = uiNumKeys > SIZE_MAX / sizeof(NiLinColorKey) ? SIZE_MAX : sizeof(NiLinColorKey) * uiNumKeys;
    pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = uiNumKeys;
        pFadeColorKey = ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinColorKey[uiNumKeys];
    }
    else {
        pFadeColorKey = nullptr;
    }
    ourMemcpy(pFadeColorKey, pCopy->pFadeColorKey, sizeof(NiLinColorKey) * uiNumKeys);
    FadeColorInterpolator.ReplaceKeys(pFadeColorKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiRadialBlurStrengthKeySize;
    uiBytes = uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
    pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = uiNumKeys;
        pRadialBlurStrengthFloatKey =
            ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
    }
    else {
        pRadialBlurStrengthFloatKey = nullptr;
    }
    ourMemcpy(pRadialBlurStrengthFloatKey, pCopy->pRadialBlurStrengthFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    RadialBlurStrengthInterpolator.ReplaceKeys(pRadialBlurStrengthFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiRadialBlurRampupKeySize;
    uiBytes = uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
    pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = uiNumKeys;
        pRadialBlurRampupFloatKey =
            ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
    }
    else {
        pRadialBlurRampupFloatKey = nullptr;
    }
    ourMemcpy(pRadialBlurRampupFloatKey, pCopy->pRadialBlurRampupFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    RadialBlurRampupInterpolator.ReplaceKeys(pRadialBlurRampupFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiRadialBlurStartKeySize;
    uiBytes = uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
    pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = uiNumKeys;
        pRadialBlurStartFloatKey =
            ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
    }
    else {
        pRadialBlurStartFloatKey = nullptr;
    }
    ourMemcpy(pRadialBlurStartFloatKey, pCopy->pRadialBlurStartFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    RadialBlurStartInterpolator.ReplaceKeys(pRadialBlurStartFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiRadialBlurRampDownKeySize;
    uiBytes = uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
    pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = uiNumKeys;
        pRadialBlurRampDownFloatKey =
            ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
    }
    else {
        pRadialBlurRampDownFloatKey = nullptr;
    }
    ourMemcpy(pRadialBlurRampDownFloatKey, pCopy->pRadialBlurRampDownFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    RadialBlurRampDownInterpolator.ReplaceKeys(pRadialBlurRampDownFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiRadialBlurDownStartKeySize;
    uiBytes = uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
    pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = uiNumKeys;
        pRadialBlurDownStartFloatKey =
            ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
    }
    else {
        pRadialBlurDownStartFloatKey = nullptr;
    }
    ourMemcpy(pRadialBlurDownStartFloatKey, pCopy->pRadialBlurDownStartFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    RadialBlurDownStartInterpolator.ReplaceKeys(pRadialBlurDownStartFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiDepthOfFieldStrengthKeySize;
    uiBytes = uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
    pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = uiNumKeys;
        pDepthOfFieldStrengthFloatKey =
            ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
    }
    else {
        pDepthOfFieldStrengthFloatKey = nullptr;
    }
    ourMemcpy(pDepthOfFieldStrengthFloatKey, pCopy->pDepthOfFieldStrengthFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    DepthOfFieldStrengthInterpolator.ReplaceKeys(pDepthOfFieldStrengthFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiDepthOfFieldDistanceKeySize;
    uiBytes = uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
    pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = uiNumKeys;
        pDepthOfFieldDistanceFloatKey =
            ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
    }
    else {
        pDepthOfFieldDistanceFloatKey = nullptr;
    }
    ourMemcpy(pDepthOfFieldDistanceFloatKey, pCopy->pDepthOfFieldDistanceFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    DepthOfFieldDistanceInterpolator.ReplaceKeys(pDepthOfFieldDistanceFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiDepthOfFieldRangeKeySize;
    uiBytes = uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
    pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = uiNumKeys;
        pDepthOfFieldRangeFloatKey =
            ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
    }
    else {
        pDepthOfFieldRangeFloatKey = nullptr;
    }
    ourMemcpy(pDepthOfFieldRangeFloatKey, pCopy->pDepthOfFieldRangeFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    DepthOfFieldRangeInterpolator.ReplaceKeys(pDepthOfFieldRangeFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    uiNumKeys = Data.uiMotionBlurStrengthKeySize;
    uiBytes = uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
    pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
    if (pMemory) {
        *static_cast<size_t*>(pMemory) = uiNumKeys;
        pMotionBlurStrengthFloatKey =
            ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
    }
    else {
        pMotionBlurStrengthFloatKey = nullptr;
    }
    ourMemcpy(pMotionBlurStrengthFloatKey, pCopy->pMotionBlurStrengthFloatKey, sizeof(NiLinFloatKey) * uiNumKeys);
    MotionBlurStrengthInterpolator.ReplaceKeys(pMotionBlurStrengthFloatKey, uiNumKeys, NiAnimationKey::LINKEY);

    for (int32_t i = 0; i < NUM_OP_PARAMS; ++i) {
        for (int32_t j = 0; j < IMAOP_COUNT; ++j) {
            uiNumKeys = Data.ppuiKeySize[i][j];
            uiBytes = uiNumKeys > SIZE_MAX / sizeof(NiLinFloatKey) ? SIZE_MAX : sizeof(NiLinFloatKey) * uiNumKeys;
            pMemory = ourMalloc(uiBytes > 0xFFFFFFFBu ? 0xFFFFFFFFu : uiBytes + sizeof(size_t));
            if (pMemory) {
                *static_cast<size_t*>(pMemory) = uiNumKeys;
                pppFloatKey[i][j] =
                    ::new (static_cast<void*>(static_cast<size_t*>(pMemory) + 1)) NiLinFloatKey[uiNumKeys];
            }
            else {
                pppFloatKey[i][j] = nullptr;
            }
            ourMemcpy(pppFloatKey[i][j], pCopy->pppFloatKey[i][j], sizeof(NiLinFloatKey) * uiNumKeys);
            ppInterpolator[i][j].ReplaceKeys(pppFloatKey[i][j], uiNumKeys, NiAnimationKey::LINKEY);
        }
    }
}

bool TESImageSpaceModifier::Compare(TESForm* apCompare) {
    int32_t j;
    int32_t i;
    TESImageSpaceModifier* v6 = dynamic_cast<TESImageSpaceModifier*>(apCompare);
    if (!v6) {
        return true;
    }
    this->FlushData_0052ED90();
    v6->FlushData_0052ED90();
    if (pIntroSound != v6->pIntroSound) {
        return true;
    }
    if (pOutroSound != v6->pOutroSound) {
        return true;
    }
    if (TESForm::CompareAllComponents(v6)) {
        return true;
    }
    if (std::memcmp(&Data, &v6->Data, sizeof(Data))) {
        return true;
    }
    if (std::memcmp(pBlurFloatKey, v6->pBlurFloatKey, sizeof(NiLinFloatKey) * Data.uiBlurKeySize)) {
        return true;
    }
    if (std::memcmp(pDoubleFloatKey, v6->pDoubleFloatKey, sizeof(NiLinFloatKey) * Data.uiDoubleKeySize)) {
        return true;
    }
    if (std::memcmp(pTintColorKey, v6->pTintColorKey, sizeof(NiLinColorKey) * Data.uiTintColorKeySize)) {
        return true;
    }
    if (std::memcmp(pFadeColorKey, v6->pFadeColorKey, sizeof(NiLinColorKey) * Data.uiFadeColorKeySize)) {
        return true;
    }
    if (std::memcmp(
            pRadialBlurStrengthFloatKey, v6->pRadialBlurStrengthFloatKey,
            sizeof(NiLinFloatKey) * Data.uiRadialBlurStrengthKeySize
        )) {
        return true;
    }
    if (std::memcmp(
            pRadialBlurRampupFloatKey, v6->pRadialBlurRampupFloatKey,
            sizeof(NiLinFloatKey) * Data.uiRadialBlurRampupKeySize
        )) {
        return true;
    }
    if (std::memcmp(
            pRadialBlurStartFloatKey, v6->pRadialBlurStartFloatKey,
            sizeof(NiLinFloatKey) * Data.uiRadialBlurStartKeySize
        )) {
        return true;
    }
    if (std::memcmp(
            pRadialBlurRampDownFloatKey, v6->pRadialBlurRampDownFloatKey,
            sizeof(NiLinFloatKey) * Data.uiRadialBlurRampDownKeySize
        )) {
        return true;
    }
    if (std::memcmp(
            pRadialBlurDownStartFloatKey, v6->pRadialBlurDownStartFloatKey,
            sizeof(NiLinFloatKey) * Data.uiRadialBlurDownStartKeySize
        )) {
        return true;
    }
    if (std::memcmp(
            pDepthOfFieldStrengthFloatKey, v6->pDepthOfFieldStrengthFloatKey,
            sizeof(NiLinFloatKey) * Data.uiDepthOfFieldStrengthKeySize
        )) {
        return true;
    }
    if (std::memcmp(
            pDepthOfFieldDistanceFloatKey, v6->pDepthOfFieldDistanceFloatKey,
            sizeof(NiLinFloatKey) * Data.uiDepthOfFieldDistanceKeySize
        )) {
        return true;
    }
    if (std::memcmp(
            pDepthOfFieldRangeFloatKey, v6->pDepthOfFieldRangeFloatKey,
            sizeof(NiLinFloatKey) * Data.uiDepthOfFieldRangeKeySize
        )) {
        return true;
    }
    if (std::memcmp(
            pMotionBlurStrengthFloatKey, v6->pMotionBlurStrengthFloatKey,
            sizeof(NiLinFloatKey) * Data.uiMotionBlurStrengthKeySize
        )) {
        return true;
    }
    for (i = 0; i < 21; ++i) {
        for (j = 0; j < 2; ++j) {
            if (std::memcmp(pppFloatKey[i][j], v6->pppFloatKey[i][j], sizeof(NiLinFloatKey) * Data.ppuiKeySize[i][j])) {
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
    NiPoint2 v15;
    float fmbstr;
    float fradialblurrampdown;
    float fradialblurstart;
    float ftime;
    float fradialblurrampup;
    float fdofrange;
    float fdoubleval;
    float v26;
    float fdofdistance;
    float fradialblurdownstart;
    float fdofstrength;
    float fblurval;
    if (Data.bAnimatable) {
        ftime = (TESImageSpaceModifier::fCurrentTime - afStartTime) / Data.fDuration;
    }
    else {
        ftime = 0.0f;
    }
    BSClamp(ftime, 0.0f, 1.0f);
    ImageSpaceManager* pmanager = BSShaderManager::GetImageSpaceManager();
    for (int32_t i = 0; i < 21; ++i) {
        float fval[2];
        for (int32_t j = 0; j < 2; ++j) {
            ppInterpolator[i][j].Update(ftime, nullptr, fval[j]);
        }
        if (i >= 0 && i <= 13) {
            pmanager->ApplyParamModifier(ImageSpaceManager::EFFECT_HDR, i, fval, afStrength);
        }
        if (i >= 14 && i <= 16) {
            pmanager->ApplyParamModifier(ImageSpaceManager::EFFECT_BLOOM, i - 14, fval, afStrength);
        }
        if (i >= 17 && i <= 20) {
            pmanager->ApplyParamModifier(ImageSpaceManager::EFFECT_CINEMATIC, i - 17, fval, afStrength);
        }
    }
    NiColorA colval(0.0f, 0.0f, 0.0f, 0.0f);
    TintColorInterpolator.Update(ftime, nullptr, colval);
    colval.a = colval.a * afStrength;
    BSClamp(colval.a, 0.0f, 1.0f);
    pmanager->ApplyTintColorParamModifier(colval);
    FadeColorInterpolator.Update(ftime, nullptr, colval);
    colval.a = colval.a * afStrength;
    BSClamp(colval.a, 0.0f, 1.0f);
    pmanager->ApplyFadeColorParamModifier(colval);
    BlurInterpolator.Update(ftime, nullptr, fblurval);
    DoubleInterpolator.Update(ftime, nullptr, fdoubleval);
    RadialBlurStrengthInterpolator.Update(ftime, nullptr, v26);
    RadialBlurRampupInterpolator.Update(ftime, nullptr, fradialblurrampup);
    RadialBlurStartInterpolator.Update(ftime, nullptr, fradialblurstart);
    RadialBlurRampDownInterpolator.Update(ftime, nullptr, fradialblurrampdown);
    RadialBlurDownStartInterpolator.Update(ftime, nullptr, fradialblurdownstart);
    DepthOfFieldStrengthInterpolator.Update(ftime, nullptr, fdofstrength);
    DepthOfFieldDistanceInterpolator.Update(ftime, nullptr, fdofdistance);
    DepthOfFieldRangeInterpolator.Update(ftime, nullptr, fdofrange);
    MotionBlurStrengthInterpolator.Update(ftime, nullptr, fmbstr);
    fblurval = fblurval * afStrength;
    BSClamp(fblurval, 0.0f, 7.0f);
    pmanager->ApplyBlurRadiusParamModifier(fblurval);
    float v13 = fdoubleval * afStrength;
    pmanager->ApplyDoubleVisionParamModifier(v13);
    if (Data.bUseTargetForRadialBlur && apTarget) {
        apTarget->UpdateWorldBound();
        const NiBound* WorldBound = &apTarget->GetWorldBound();
        float v12 = v26 * afStrength;
        pmanager->ApplyRadialBlurParamModifier(
            v12, fradialblurrampup, fradialblurstart, fradialblurrampdown, fradialblurdownstart, *WorldBound
        );
    }
    else {
        if (Data.bUseTargetForRadialBlur) {
            NiPoint2* v5 = ::new (&v15) NiPoint2(0.5f, 0.5f);
            float v11 = v26 * afStrength;
            pmanager->ApplyRadialBlurParamModifier(
                v11, fradialblurrampup, fradialblurstart, fradialblurrampdown, fradialblurdownstart, *v5
            );
        }
        else {
            float v10 = v26 * afStrength;
            pmanager->ApplyRadialBlurParamModifier(
                v10, fradialblurrampup, fradialblurstart, fradialblurrampdown, fradialblurdownstart,
                Data.RadialBlurCenter
            );
        }
    }
    if (Data.bUseTargetForDepthOfField && apTarget) {
        apTarget->UpdateWorldBound();
        const NiBound* v6 = &apTarget->GetWorldBound();
        float v9 = fdofstrength * afStrength;
        pmanager->ApplyDepthOfFieldParamModifier_0(v9, Data.ucDepthOfFieldMode, 0.0f, &v6->m_kCenter);
    }
    else {
        if (Data.bUseTargetForDepthOfField) {
            float v8 = fdofstrength * afStrength;
            pmanager->ApplyDepthOfFieldParamModifier(v8, Data.ucDepthOfFieldMode, 0.0f, 0.0f, 1.0f);
        }
        else {
            float v7 = fdofstrength * afStrength;
            pmanager->ApplyDepthOfFieldParamModifier(v7, Data.ucDepthOfFieldMode, 0.0f, fdofdistance, fdofrange);
        }
    }
    pmanager->ApplyMotionBlurStrengthParamModifier(fmbstr);
}

void TESImageSpaceModifier::ApplyWeather(float afStartTime, float afStrength) {
    float fmbstr;
    float fradialblurrampdown;
    float fradialblurstart;
    float ftime;
    float fradialblurrampup;
    float fdofrange;
    float fdoubleval;
    float fradialblurstr;
    float fdofdistance;
    float fradialblurdownstart;
    float fdofstrength;
    float fblurval;
    if (Data.bAnimatable) {
        ftime = (TESImageSpaceModifier::fCurrentTime - afStartTime) / Data.fDuration;
    }
    else {
        ftime = 0.0f;
    }
    ImageSpaceManager* pmanager = BSShaderManager::GetImageSpaceManager();
    for (int32_t i = 0; i < 21; ++i) {
        float fval[2];
        for (int32_t j = 0; j < 2; ++j) {
            ppInterpolator[i][j].Update(ftime, nullptr, fval[j]);
        }
        if (i >= 0 && i <= 13) {
            pmanager->ApplyParamModifierWeather(ImageSpaceManager::EFFECT_HDR, i, fval, afStrength);
        }
        if (i >= 14 && i <= 16) {
            pmanager->ApplyParamModifierWeather(ImageSpaceManager::EFFECT_BLOOM, i - 14, fval, afStrength);
        }
        if (i >= 17 && i <= 20) {
            pmanager->ApplyParamModifierWeather(ImageSpaceManager::EFFECT_CINEMATIC, i - 17, fval, afStrength);
        }
    }
    NiColorA colval(0.0f, 0.0f, 0.0f, 0.0f);
    TintColorInterpolator.Update(ftime, nullptr, colval);
    colval.Scale(afStrength);
    pmanager->ApplyTintColorParamModifierWeather(colval);
    FadeColorInterpolator.Update(ftime, nullptr, colval);
    colval.Scale(afStrength);
    pmanager->ModFadeColor(colval);
    BlurInterpolator.Update(ftime, nullptr, fblurval);
    DoubleInterpolator.Update(ftime, nullptr, fdoubleval);
    RadialBlurStrengthInterpolator.Update(ftime, nullptr, fradialblurstr);
    RadialBlurRampupInterpolator.Update(ftime, nullptr, fradialblurrampup);
    RadialBlurStartInterpolator.Update(ftime, nullptr, fradialblurstart);
    RadialBlurRampDownInterpolator.Update(ftime, nullptr, fradialblurrampdown);
    RadialBlurDownStartInterpolator.Update(ftime, nullptr, fradialblurdownstart);
    DepthOfFieldStrengthInterpolator.Update(ftime, nullptr, fdofstrength);
    DepthOfFieldDistanceInterpolator.Update(ftime, nullptr, fdofdistance);
    DepthOfFieldRangeInterpolator.Update(ftime, nullptr, fdofrange);
    MotionBlurStrengthInterpolator.Update(ftime, nullptr, fmbstr);
    float v14 = fblurval * afStrength;
    pmanager->ModBlurRadius(v14);
    float v13 = fdoubleval * afStrength;
    pmanager->ModDoubleVisionStrength(v13);
    NiPoint2 result = Data.RadialBlurCenter * afStrength;
    float v12 = fradialblurdownstart * afStrength;
    float v11 = fradialblurrampdown * afStrength;
    float afStart = fradialblurstart * afStrength;
    float afRampup = fradialblurrampup * afStrength;
    float v8 = fradialblurstr * afStrength;
    pmanager->ModRadialBlur(v8, afRampup, afStart, v11, v12, &result);
    float v7 = fdofrange * afStrength;
    float v6 = fdofdistance * afStrength;
    float v5 = fdofstrength * afStrength;
    pmanager->ModDepthOfField(v5, Data.ucDepthOfFieldMode, 0.0f, v6, v7);
    float v4 = fmbstr * afStrength;
    pmanager->ModFullScreenMotionBlurStrength(v4);
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

void TESImageSpaceModifier::FlushData() {}

void TESImageSpaceModifier::FlushData_0052ED90() {
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
    TESForm* formByNumericID = TESForm::GetFormByNumericID(354);
    g_GetHitIMOD = dynamic_cast<TESImageSpaceModifier*>(formByNumericID);
    return g_GetHitIMOD;
}

TESImageSpaceModifier* TESImageSpaceModifier::QDefault() {
    return &Default;
}

// The second radial blur target updates apTarget1's world bound, not apTarget2's, deliberately, as the game does.
void TESImageSpaceModifier::BlendApply(
    float afPercent,
    TESImageSpaceModifier* apMod1,
    TESImageSpaceModifier* apMod2,
    float afStrength1,
    float afStrength2,
    NiAVObject* apTarget1,
    NiAVObject* apTarget2
) {
    uint8_t ucDOFMode;
    NiPoint2 v35;
    NiPoint2 v36;
    NiPoint2 v37;
    NiPoint2 v38;
    NiPoint2 v39;
    NiPoint2 v40;
    NiPoint2 apResult;
    NiPoint2 result;
    float fdofrange2;
    float fval1_2;
    NiPoint2 radialBlurCenter;
    NiPoint2 kDOFParams;
    float fdofdistance2;
    float fdofrange1;
    float fdofdistance1;
    float fval2_2;
    float fblurval;
    if (!apMod1) {
        apMod1 = TESImageSpaceModifier::QDefault();
    }
    if (!apMod2) {
        apMod2 = TESImageSpaceModifier::QDefault();
    }
    float fTime = 0.0f;
    ImageSpaceManager* pmanager = BSShaderManager::GetImageSpaceManager();
    float fStrength = (afStrength2 - afStrength1) * afPercent + afStrength1;
    for (int32_t i = 0; i < 21; ++i) {
        float a2_8[2];
        for (int32_t j = 0; j < 2; ++j) {
            float fval1;
            float fval2;
            apMod1->ppInterpolator[i][j].Update(fTime, nullptr, fval1);
            apMod2->ppInterpolator[i][j].Update(fTime, nullptr, fval2);
            a2_8[j] = (fval2 - fval1) * afPercent + fval1;
        }
        if (i >= 0 && i <= 13) {
            pmanager->ApplyParamModifier(ImageSpaceManager::EFFECT_HDR, static_cast<uint32_t>(i), a2_8, fStrength);
        }
        if (i >= 14 && i <= 16) {
            pmanager->ApplyParamModifier(ImageSpaceManager::EFFECT_BLOOM, i - 14, a2_8, fStrength);
        }
        if (i >= 17 && i <= 20) {
            pmanager->ApplyParamModifier(ImageSpaceManager::EFFECT_CINEMATIC, i - 17, a2_8, fStrength);
        }
    }
    NiColorA colval(0.0f, 0.0f, 0.0f, 0.0f);
    NiColorA colval1(0.0f, 0.0f, 0.0f, 0.0f);
    NiColorA colval2(0.0f, 0.0f, 0.0f, 0.0f);
    apMod1->TintColorInterpolator.Update(fTime, nullptr, colval1);
    apMod2->TintColorInterpolator.Update(fTime, nullptr, colval2);
    NiColorA v48(colval2 - colval1);
    NiColorA* v7 = &v48;
    colval = colval1 + NiColorA::Multiply(afPercent, v7);
    colval.a = colval.a * fStrength;
    BSClamp(colval.a, 0.0f, 1.0f);
    pmanager->ApplyTintColorParamModifier(colval);
    apMod1->FadeColorInterpolator.Update(fTime, nullptr, colval1);
    apMod2->FadeColorInterpolator.Update(fTime, nullptr, colval2);
    NiColorA v45(colval2 - colval1);
    NiColorA* v9 = &v45;
    colval = colval1 + NiColorA::Multiply(afPercent, v9);
    colval.a = colval.a * fStrength;
    BSClamp(colval.a, 0.0f, 1.0f);
    pmanager->ApplyFadeColorParamModifier(colval);
    apMod1->BlurInterpolator.Update(fTime, nullptr, fval1_2);
    apMod2->BlurInterpolator.Update(fTime, nullptr, fval2_2);
    float afValue = (fval2_2 - fval1_2) * afPercent + fval1_2;
    apMod1->DoubleInterpolator.Update(fTime, nullptr, fval1_2);
    apMod2->DoubleInterpolator.Update(fTime, nullptr, fval2_2);
    float fdoubleval = (fval2_2 - fval1_2) * afPercent + fval1_2;
    apMod1->RadialBlurStrengthInterpolator.Update(fTime, nullptr, fval1_2);
    apMod2->RadialBlurStrengthInterpolator.Update(fTime, nullptr, fval2_2);
    float fradialblurstr = (fval2_2 - fval1_2) * afPercent + fval1_2;
    apMod1->RadialBlurRampupInterpolator.Update(fTime, nullptr, fval1_2);
    apMod2->RadialBlurRampupInterpolator.Update(fTime, nullptr, fval2_2);
    float fradialblurrampup = (fval2_2 - fval1_2) * afPercent + fval1_2;
    apMod1->RadialBlurStartInterpolator.Update(fTime, nullptr, fval1_2);
    apMod2->RadialBlurStartInterpolator.Update(fTime, nullptr, fval2_2);
    float fradialblurstart = (fval2_2 - fval1_2) * afPercent + fval1_2;
    apMod1->RadialBlurRampDownInterpolator.Update(fTime, nullptr, fval1_2);
    apMod2->RadialBlurRampDownInterpolator.Update(fTime, nullptr, fval2_2);
    float fradialblurrampdown = (fval2_2 - fval1_2) * afPercent + fval1_2;
    apMod1->RadialBlurDownStartInterpolator.Update(fTime, nullptr, fval1_2);
    apMod2->RadialBlurDownStartInterpolator.Update(fTime, nullptr, fval2_2);
    float fradialblurdownstart = (fval2_2 - fval1_2) * afPercent + fval1_2;
    apMod1->DepthOfFieldStrengthInterpolator.Update(fTime, nullptr, fval1_2);
    apMod2->DepthOfFieldStrengthInterpolator.Update(fTime, nullptr, fval2_2);
    float fdofstrength = (fval2_2 - fval1_2) * afPercent + fval1_2;
    apMod1->DepthOfFieldDistanceInterpolator.Update(fTime, nullptr, fdofdistance1);
    apMod2->DepthOfFieldDistanceInterpolator.Update(fTime, nullptr, fdofdistance2);
    apMod1->DepthOfFieldRangeInterpolator.Update(fTime, nullptr, fdofrange1);
    apMod2->DepthOfFieldRangeInterpolator.Update(fTime, nullptr, fdofrange2);
    apMod1->MotionBlurStrengthInterpolator.Update(fTime, nullptr, fval1_2);
    apMod2->MotionBlurStrengthInterpolator.Update(fTime, nullptr, fval2_2);
    float fmotionblurstr = (fval2_2 - fval1_2) * afPercent + fval1_2;
    fblurval = afValue * fStrength;
    BSClamp(fblurval, 0.0f, 7.0f);
    pmanager->ApplyBlurRadiusParamModifier(fblurval);
    float v34 = fdoubleval * fStrength;
    pmanager->ApplyDoubleVisionParamModifier(v34);
    NiPoint2 radialBlurCenter1(0.5f, 0.5f);
    NiPoint2 radialBlurCenter2(0.5f, 0.5f);
    if ((apMod1->Data.bUseTargetForRadialBlur) && (apTarget1)) {
        apTarget1->UpdateWorldBound();
        const NiBound* WorldBound = &apTarget1->GetWorldBound();
        ImageSpaceManager::GetRadialBlurCenterFromWorldBound(WorldBound, &radialBlurCenter1);
    }

    if ((apMod2->Data.bUseTargetForRadialBlur) && (apTarget2)) {
        apTarget1->UpdateWorldBound();
        const NiBound* v12 = &apTarget2->GetWorldBound();
        ImageSpaceManager::GetRadialBlurCenterFromWorldBound(v12, &radialBlurCenter2);
    }

    if (!apMod1->Data.bUseTargetForRadialBlur) {
        float y = apMod1->Data.RadialBlurCenter.y;
        radialBlurCenter1.x = apMod1->Data.RadialBlurCenter.x;
        radialBlurCenter1.y = y;
    }
    if (!apMod2->Data.bUseTargetForRadialBlur) {
        float v14 = apMod2->Data.RadialBlurCenter.y;
        radialBlurCenter2.x = apMod2->Data.RadialBlurCenter.x;
        radialBlurCenter2.y = v14;
    }
    NiPoint2* v15 = ::new (&result) NiPoint2(radialBlurCenter2 - radialBlurCenter1);
    NiPoint2* v16 = NiPoint2::Scale_(&apResult, afPercent, v15);
    NiPoint2* v17 = ::new (&v40) NiPoint2(radialBlurCenter1 + *v16);
    float v18 = v17->y;
    radialBlurCenter.x = v17->x;
    radialBlurCenter.y = v18;
    float v33 = fradialblurstr * fStrength;
    pmanager->ApplyRadialBlurParamModifier(
        v33, fradialblurrampup, fradialblurstart, fradialblurrampdown, fradialblurdownstart, radialBlurCenter
    );
    NiPoint2 v70(0.0f, 0.0f);
    NiPoint2 v78(0.0f, 0.0f);
    if ((apMod1->Data.bUseTargetForDepthOfField) && (apTarget1)) {
        apTarget1->UpdateWorldBound();
        const NiBound* v19 = &apTarget1->GetWorldBound();
        pmanager->GetDepthOfFieldParamsFromPoint(&v19->m_kCenter, &v70);
    }

    if ((apMod2->Data.bUseTargetForDepthOfField) && (apTarget2)) {
        apTarget2->UpdateWorldBound();
        const NiBound* v20 = &apTarget2->GetWorldBound();
        pmanager->GetDepthOfFieldParamsFromPoint(&v20->m_kCenter, &v78);
    }

    if (!apMod1->Data.bUseTargetForDepthOfField) {
        NiPoint2* v21 = ::new (&v39) NiPoint2(fdofdistance1, fdofrange1);
        float v22 = v21->y;
        v70.x = v21->x;
        v70.y = v22;
    }
    if (!apMod2->Data.bUseTargetForDepthOfField) {
        NiPoint2* v23 = ::new (&v38) NiPoint2(fdofdistance2, fdofrange2);
        float x = v23->x;
        float v25 = v23->y;
        v78.x = x;
        v78.y = v25;
    }
    NiPoint2* v26 = ::new (&v37) NiPoint2(v78 - v70);
    NiPoint2* v27 = NiPoint2::Scale_(&v36, afPercent, v26);
    NiPoint2* v28 = ::new (&v35) NiPoint2(v70 + *v27);
    kDOFParams.x = v28->x;
    kDOFParams.y = v28->y;
    if (!(afPercent < 0.5)) {
        ucDOFMode = apMod2->Data.ucDepthOfFieldMode;
    }
    else {
        ucDOFMode = apMod1->Data.ucDepthOfFieldMode;
    }
    float v31 = fdofstrength * fStrength;
    pmanager->ApplyDepthOfFieldParamModifier(v31, ucDOFMode, 0.0f, kDOFParams.x, kDOFParams.y);
    pmanager->ApplyMotionBlurStrengthParamModifier(fmotionblurstr);
}

void TESImageSpaceModifier::SetEnable(bool abEnable) {
    TESImageSpaceModifier::bEnable = (bool)((int8_t)abEnable);
}

void TESImageSpaceModifier::AllowGetHit(bool abEnable) {
    g_bEnableHitAndConcussionIMODs = (bool)((uint8_t)abEnable);
}

void TESImageSpaceModifier::AdvanceCurrentTime(float afAmount) {
    TESImageSpaceModifier::fCurrentTime = (TESImageSpaceModifier::fCurrentTime + afAmount);
}

TESImageSpaceModifier* TESImageSpaceModifier::GetExplosionInFace() {
    if (g_explosionInFaceIMOD) {
        return reinterpret_cast<TESImageSpaceModifier*>(reinterpret_cast<uintptr_t>(g_explosionInFaceIMOD));
    }
    TESForm* formByNumericID = TESForm::GetFormByNumericID(358);
    g_explosionInFaceIMOD = reinterpret_cast<TESImageSpaceModifier*>(
        reinterpret_cast<uintptr_t>(dynamic_cast<TESImageSpaceModifier*>(formByNumericID))
    );
    return reinterpret_cast<TESImageSpaceModifier*>(reinterpret_cast<uintptr_t>(g_explosionInFaceIMOD));
}

// Hands the keys to the color data, creating it on first use.
void NiColorInterpolator::ReplaceKeys(NiColorKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
    if (uiNumKeys) {
        if (!m_spColorData) {
            NiColorData* pkColorData = new NiColorData;
            m_spColorData = pkColorData;
        }
        m_spColorData->ReplaceAnim(pkKeys, uiNumKeys, eType);
        m_uiLastIdx = 0;
    }
    else {
        if (m_spColorData) {
            m_spColorData->ReplaceAnim(nullptr, 0, NiAnimationKey::NOINTERP);
        }
    }
}

NiFloatKey* NiFloatData::GetAnim(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) {
    uiNumKeys = m_uiNumKeys;
    eType = m_eType;
    ucSize = m_ucKeySize;
    return m_pkKeys;
}

void NiFloatInterpolator::ReplaceKeys(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
    if (uiNumKeys) {
        if (!m_spFloatData) {
            NiFloatData* pkData = new NiFloatData;
            m_spFloatData = pkData;
        }
        m_spFloatData->ReplaceAnim(pkKeys, uiNumKeys, eType);
        m_uiLastIdx = 0;
    }
    else {
        if (m_spFloatData) {
            m_spFloatData->ReplaceAnim(nullptr, 0, NiAnimationKey::NOINTERP);
        }
    }
}

NiLinColorKey::NiLinColorKey() {}

NiColorA NiColorA::operator*(float fScalar) const {
    NiColorA kResult = *this;
    kResult.r *= fScalar;
    kResult.g *= fScalar;
    kResult.b *= fScalar;
    kResult.a *= fScalar;
    return kResult;
}

NiColorA NiColorA::Multiply(float afScalar, const NiColorA* apColor) {
    return *apColor * afScalar;
}
