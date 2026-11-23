#include "Fallout/Interface/InterfaceManager.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Controls.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/DebugText.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "BSMenu/TileRect.h"
#include "BSShader/BSFogProperty.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectMenuBG.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/MTRenderingSystem.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/TileShaderProperty.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSMessage.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/ViewCaster.h"
#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFurniture.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESKey.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CameraShake.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/GunWobble.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/EditableTextField.h"
#include "Fallout/Interface/FOHUDEffectManager.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Fallout/Interface/FORenderedTerminal.h"
#include "Fallout/Interface/FOVATSEffectManager.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/MenuConsole.h"
#include "Fallout/Interface/MenuManager.h"
#include "Fallout/Interface/Menus/BarterMenu.h"
#include "Fallout/Interface/Menus/ContainerMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/InventoryMenu.h"
#include "Fallout/Interface/Menus/LevelUpMenu.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/LockPickMenu.h"
#include "Fallout/Interface/Menus/MapMenu.h"
#include "Fallout/Interface/Menus/MessageMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Interface/Menus/TutorialMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParallelUpdateTaskManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSemaphore.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

// Idle's function-local static clast_menu_mode: the menu mode the HUD was last set for.
static int32_t clast_menu_mode = 99;

// Idle's guarded function-local static: the controller state of the previous frame, set from the first XInputGetState
// result.
static bool bLastControllerState;

// bLastControllerState's guard (bit 0).
static uint32_t uiLastControllerStateGuard;

// The game settings this file defines, with the game's IDs and values, in its initialization order.
SettingT<INISettingCollection> fXenonMenuMouseXYMult("fXenonMenuMouseXYMult:Controls", 0.0003f);
SettingT<INISettingCollection> iXenonMenuStickDeadZone("iXenonMenuStickDeadZone:Controls", 20000);
SettingT<INISettingCollection> fXenonMenuStickSpeed("fXenonMenuStickSpeed:Controls", 300.0f);
SettingT<INISettingCollection> fXenonMenuDpadRepeatSpeed("fXenonMenuDpadRepeatSpeed:Controls", 25.0f);
SettingT<INISettingCollection> fActivatePickSphereRadius("fActivatePickSphereRadius:Interface", 16.0f);
SettingT<INISettingCollection> bActivatePickUseGamebryoPick("bActivatePickUseGamebryoPick:Interface", false);
SettingT<INISettingCollection> fKeyRepeatTime("fKeyRepeatTime:Interface", 500.0f);
SettingT<INISettingCollection> fKeyRepeatInterval("fKeyRepeatInterval:Interface", 50.0f);
SettingT<INISettingCollection> bAllowConsole("bAllowConsole:Interface", true);
SettingT<INISettingCollection> fMenuPlayerLightAmbientRed("fMenuPlayerLightAmbientRed:Interface", 0.25f);
SettingT<INISettingCollection> fMenuPlayerLightAmbientGreen("fMenuPlayerLightAmbientGreen:Interface", 0.25f);
SettingT<INISettingCollection> fMenuPlayerLightAmbientBlue("fMenuPlayerLightAmbientBlue:Interface", 0.25f);
SettingT<INISettingCollection> fMenuPlayerLightDiffuseRed("fMenuPlayerLightDiffuseRed:Interface", 0.8f);
SettingT<INISettingCollection> fMenuPlayerLightDiffuseGreen("fMenuPlayerLightDiffuseGreen:Interface", 0.8f);
SettingT<INISettingCollection> fMenuPlayerLightDiffuseBlue("fMenuPlayerLightDiffuseBlue:Interface", 0.8f);
SettingT<INISettingCollection> iSafeZoneX("iSafeZoneX:Interface", 15);
SettingT<INISettingCollection> iSafeZoneY("iSafeZoneY:Interface", 15);
SettingT<INISettingCollection> iSafeZoneXWide("iSafeZoneXWide:Interface", 15);
SettingT<INISettingCollection> iSafeZoneYWide("iSafeZoneYWide:Interface", 15);
SettingT<INISettingCollection> fXenonMenuStickSpeedMaxMod("fXenonMenuStickSpeedMaxMod:Controls", 5.0f);
SettingT<INISettingCollection> bUseImageSpaceMenuFX("bUseImageSpaceMenuFX:InterfaceFX", true);
SettingT<INISettingCollection> bEnableMenuRenderDebugMode("bEnableMenuRenderDebugMode:InterfaceFX", false);
BSSimpleList<ListBoxBase*> InterfaceManager::xActiveListBoxes;
InterfaceManager::TileUpdateArray InterfaceManager::xTileUpdateList;
SettingT<INIPrefSettingCollection> gs_bDisable360Controller("bDisable360Controller:Interface", false);
SettingT<INIPrefSettingCollection> SystemColorManager::uHUDColor("uHUDColor:Interface", 0u);
SettingT<INIPrefSettingCollection> SystemColorManager::uPipboyColor("uPipboyColor:Interface", 0u);
BSCriticalSection kRenderedMenuCriticalSection;
NiTPointerList<FORenderedMenu*> s_RenderedMenuDeleteList;

int32_t GetIsControllerConnected() {
    return InterfaceManager::Has360Controller();
}

void QueueRenderedMenuDelete(FORenderedMenu* pRenderedMenu) {
    kRenderedMenuCriticalSection.Enter(nullptr);
    s_RenderedMenuDeleteList.AddTail(pRenderedMenu);
    kRenderedMenuCriticalSection.Leave();
}

int32_t ConvertSpecialInputKeyToControllerKey(int32_t aiKey) {
    int32_t iControllerKey;
    switch (aiKey) {
        case 1: {
            iControllerKey = 1;
            break;
        }
        case 2: {
            iControllerKey = 2;
            break;
        }
        case 3: {
            iControllerKey = 4;
            break;
        }
        case 4: {
            iControllerKey = 5;
            break;
        }
        case 5: {
            iControllerKey = 6;
            break;
        }
        case 6: {
            iControllerKey = 7;
            break;
        }
        case 9: {
            iControllerKey = 10;
            break;
        }
        case 10: {
            iControllerKey = 11;
            break;
        }
        case 11: {
            iControllerKey = 12;
            break;
        }
        case 12: {
            iControllerKey = 13;
            break;
        }
        case 13: {
            iControllerKey = 16;
            break;
        }
        case 14: {
            iControllerKey = 17;
            break;
        }
        case 15: {
            iControllerKey = 15;
            break;
        }
        case 16: {
            iControllerKey = 14;
            break;
        }
        case 17: {
            iControllerKey = 8;
            break;
        }
        case 18: {
            iControllerKey = 9;
            break;
        }
        default: {
            iControllerKey = -1;
            break;
        }
    }
    return iControllerKey;
}

// The menu height goes through x87 as an int into a double local, and the quotient is rounded to a float local.
float GetMenuHeightRatio() {
    double dMenuHeight = InterfaceManager::GetMenuHeight();
    float fResult = static_cast<float>(dMenuHeight / InterfaceManager::GetScreenHeight());
    return fResult;
}

// These five are declared in InterfaceManager.h, so they keep external linkage: defined inside the anonymous
// namespace above they are different, internal entities, and the header's declarations stay undefined.
void ToggleFullHelp() {
    if (InterfaceManager::Instance()) {
        if (InterfaceManager::Instance()->IsDoneFirstInit()) {
            // Tests the flag as a raw byte: any byte but 0 reads as set, and it stores 0 or 1.
            bool bNewFullHelp = static_cast<uint8_t>(InterfaceManager::Instance()->bFullHelp) == 0;
            InterfaceManager::Instance()->bFullHelp = bNewFullHelp;
        }
    }
}

bool GetIsFullHelp() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return InterfaceManager::Instance()->bFullHelp;
    }

    return false;
}

bool GetMapMenuFogOfWar() {
    return !MapMenu::pMe || MapMenu::pMe->bFogOfWarEnabled;
}

void SetMapMenuFogOfWar(bool abFogOfWar) {
    if (MapMenu::pMe) {
        MapMenu::pMe->bFogOfWarEnabled = abFogOfWar;
    }
}

float GetEyefinityScreenWidth() {
    return fEyefinityScreenWidth;
}

float GetEyefinityScreenHeight() {
    return fEyefinityScreenHeight;
}

// True while the display spans several monitors.
bool IsEyefinity() {
    return bIsEyefinity && bIsEyefinityActive;
}

// Returns null on both failed tests: the null singleton, then IsDoneFirstInit.
TileMenu* ShowBarterMenu_(TESObjectREFR* apRef, int32_t aiMode) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return static_cast<TileMenu*>(BarterMenu::Create(apRef, aiMode));
    }
    return nullptr;
}

uint32_t GetContainerMenuMode() {
    if (ContainerMenu::pMe) {
        return ContainerMenu::pMe->eContainerMode;
    }

    return 0;
}

// Returns null on both failed tests: the null singleton, then IsDoneFirstInit.
TileMenu* _ShowLockPickMenu(TESObjectREFR* apRef, int32_t aiLockLevel, bool abFlag) {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->IsDoneFirstInit()) {
        return static_cast<TileMenu*>(LockPickMenu::Create(apRef, aiLockLevel, abFlag));
    }
    return nullptr;
}

void ClearRenderedMenuDeleteList() {
    kRenderedMenuCriticalSection.Enter(nullptr);
    while (!s_RenderedMenuDeleteList.IsEmpty()) {
        FORenderedMenu* pRenderedMenu = s_RenderedMenuDeleteList.RemoveHead();
        if (pRenderedMenu) {
            delete pRenderedMenu;
        }
    }
    kRenderedMenuCriticalSection.Leave();
}

uint32_t MakeRGBA(uint32_t aiRed, uint32_t aiGreen, uint32_t aiBlue, uint32_t aiAlpha) {
    return (aiRed << 24) | (static_cast<uint8_t>(aiGreen) << 16) | (static_cast<uint8_t>(aiBlue) << 8) |
        static_cast<uint8_t>(aiAlpha);
}

int32_t SplitColorComponents(int32_t aiColor, uint32_t* apRed, uint32_t* apGreen, uint32_t* apBlue, uint32_t* apAlpha) {
    *apRed = static_cast<uint8_t>(aiColor >> 24);
    *apGreen = static_cast<uint8_t>(aiColor >> 16);
    *apBlue = static_cast<uint8_t>(aiColor >> 8);
    *apAlpha = static_cast<uint8_t>(aiColor);
    return static_cast<uint8_t>(aiColor);
}

void RecursivelySetVisibilityDirty(Tile* apTile) {
    NiTListItem<Tile*>* pPos = apTile->xChildren.m_pkHead;
    while (pPos) {
        Tile* pChild = apTile->xChildren.GetNext(pPos);
        RecursivelySetVisibilityDirty(pChild);
    }
    if (!BSstrcmp(apTile->GetName(), "xbox_button") || !BSstrcmp(apTile->GetName(), "PCShortcutLabel")) {
        InterfaceManager::Instance();
        bool bHas360Controller = InterfaceManager::Has360Controller();
        int32_t iTrait = Tile::TextToTrait("_360Controller");
        apTile->Set(iTrait, static_cast<int32_t>(bHas360Controller));
        apTile->SetNeedsUpdate(4);
    }
}

InterfaceManager::Timer::Timer() : pIndex(nullptr), fElapsed(0.0f), fEnd(0.001f), pPrev(nullptr), pNext(nullptr) {}

InterfaceManager::TutorialManager::TutorialManager() {
    Reset();
}

// Tests 0 <= aeID < 41 signed and the menu ID unsigned. The tutorial messages are the forms from 0x168 on, in
// TutorialMessageID order.
bool InterfaceManager::TutorialManager::ShowMessage(
    Interface::TutorialMessageID aeID,
    int32_t aiMenuID,
    uint32_t auiShowDelay
) {
    bool bShow = false;
    if (aeID >= 0 && aeID <= Interface::TUT_HARDCORE_NEEDS) {
        tMessages[aeID].uiShowDelay = auiShowDelay;
        if ((static_cast<uint32_t>(aiMenuID) >= static_cast<uint32_t>(Interface::eFirstMenu) &&
             static_cast<uint32_t>(aiMenuID) <= static_cast<uint32_t>(Interface::eLastMenu)) ||
            !aiMenuID) {
            if (aiMenuID) {
                tMessages[aeID].uiMenuOffset = static_cast<uint32_t>(aiMenuID - Interface::eFirstMenu);
            }
            else {
                tMessages[aeID].uiMenuOffset = 0;
            }
            TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(aeID) + 0x168u);
            BGSMessage* pMessage = dynamic_cast<BGSMessage*>(pForm);
            bShow = pMessage && pMessage->IsMsgFlag2Set() && ReturnTrue() && !tMessages[aeID].bMessageShown;
        }
        tMessages[aeID].bShowMessage = bShow;
    }
    return bShow;
}

// The range check on aeID is signed.
bool InterfaceManager::TutorialManager::HasBeenShown(Interface::TutorialMessageID aeID) {
    return aeID >= 0 && aeID <= Interface::TUT_HARDCORE_NEEDS && tMessages[aeID].bMessageShown;
}

void InterfaceManager::TutorialManager::Load(char* apLoadString) {
    for (uint32_t i = 0; i < GetSaveLoadSize(); ++i) {
        uint8_t cData = apLoadString[i];
        for (uint32_t j = 0; j < 8; ++j) {
            uint32_t uiID = j + 8 * i;
            if (uiID == 41) {
                break;
            }
            tMessages[uiID].bMessageShown = (1 << j & cData) != 0;
        }
    }
}

void InterfaceManager::TutorialManager::Save(char* apSaveString) {
    for (uint32_t i = 0; i < GetSaveLoadSize(); ++i) {
        int8_t cData = 0;
        for (uint32_t j = 0; j < 8; ++j) {
            int32_t iID = static_cast<int32_t>(j + 8 * i);
            if (iID == 41) {
                break;
            }
            if (HasBeenShown(static_cast<Interface::TutorialMessageID>(iID))) {
                cData |= 1 << j;
            }
        }
        apSaveString[i] = cData;
    }
}

void InterfaceManager::TutorialManager::Update() {
    if (StartMenu::PauseMenuIsActive() || Interface::IsMenuQueuedToOpen()) {
        return;
    }
    // Set again when a message with no show delay is queued, so that it is shown in the same call.
    bool bCheckAgain = true;
    while (bCheckAgain) {
        bCheckAgain = false;
        if (!ReturnTrue() || InterfaceManager::pInstance->cMenuMode != InterfaceManager::eMenuModeOn) {
            uiNextDisplay = 0;
            eDisplayID = Interface::TUT_COUNT;
        }
        else if (
            uiNextDisplay &&
            BSGetTickCount() >= uiNextDisplay &&
            eDisplayID < Interface::TUT_COUNT &&
            (!tMessages[eDisplayID].uiMenuOffset ||
             Interface::IsTopMenuID(
                 static_cast<Interface::enumMenus>(tMessages[eDisplayID].uiMenuOffset + Interface::eMessageMenu)
             )) &&
            Interface::IsTopOfMenuStackReady()
        ) {
            // The tutorial messages' form IDs start at 0x168, in TutorialMessageID order.
            TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(eDisplayID) + 0x168);
            BGSMessage* pMessage = dynamic_cast<BGSMessage*>(pForm);
            if (!pMessage || !TutorialMenu::Create(pMessage, false) || !FlagAsShown(eDisplayID)) {
                Warning("Error occurred while trying to display tutorial message");
            }
            uiNextDisplay = 0;
            eDisplayID = Interface::TUT_COUNT;
        }
        else {
            for (int32_t i = 0; i < Interface::TUT_COUNT; ++i) {
                // Tests the menu of the message on display (eDisplayID; at TUT_COUNT that reads past tMessages) and
                // then passes message i's menu to IsTopMenuID.
                if (tMessages[i].bShowMessage &&
                    !tMessages[i].bMessageShown &&
                    eDisplayID != i &&
                    (!tMessages[eDisplayID].uiMenuOffset ||
                     Interface::IsTopMenuID(
                         static_cast<Interface::enumMenus>(tMessages[i].uiMenuOffset + Interface::eMessageMenu)
                     )) &&
                    Interface::IsTopOfMenuStackReady()) {
                    if (eDisplayID < Interface::TUT_COUNT &&
                        (!tMessages[eDisplayID].uiMenuOffset ||
                         Interface::IsTopMenuID(
                             static_cast<Interface::enumMenus>(
                                 tMessages[eDisplayID].uiMenuOffset + Interface::eMessageMenu
                             )
                         ))) {
                        tMessages[eDisplayID].bShowMessage = 0;
                    }
                    eDisplayID = static_cast<Interface::TutorialMessageID>(i);
                    uint32_t uiNow = BSGetTickCount();
                    uiNextDisplay = uiNow + tMessages[i].uiShowDelay;
                    bCheckAgain = tMessages[i].uiShowDelay == 0;
                }
            }
        }
    }
}

bool InterfaceManager::TutorialManager::FlagAsShown(Interface::TutorialMessageID aeID) {
    if (aeID < 0 || aeID >= Interface::TUT_COUNT) {
        return false;
    }
    tMessages[aeID].bShowMessage = 0;
    tMessages[aeID].bMessageShown = 1;
    return true;
}

InterfaceManager::InterfaceManager()
  : bFirstInit(false), bSecondInit(false), spSceneGraph(nullptr), sp3DSceneGraph(nullptr),
    cMenuMode(InterfaceManager::eMenuModeOff), bExternalForcedMenuMode(false), bLockMenuModeForFade(false),
    cStatsPageNumber(-1), cInventoryPageNumber(-1), cMagicPageNumber(-1), cMapPageNumber(-1), iPickDistance(80),
    pPlayerLight(nullptr), pCursor(nullptr), fMouseWheel(0.0f), fMouseHeldTime(0.0f), pDragTarget(nullptr),
    iDragStartX(0), iDragStartY(0), iCursorOffsetX(0), iCursorOffsetY(0), ConsolePickRefs(), iCurrentPickIndex(0),
    bShowMouse(false), pInterfaceRoot(nullptr), pCursorRoot(nullptr), pPlayerRoot(nullptr), spInterfaceAccum(nullptr),
    sp3dInterfaceAccum(nullptr), pShadowNode(nullptr), pMenusRoot(nullptr), pDebugTextRoot(nullptr),
    fOneToOneDistance(0.0f), spAlphaProp(nullptr), iCharHit(0), pTargetReticle(nullptr), pSafeZone(nullptr),
    pMouseOverTarget(nullptr), iLastXDefault(100), bPreLoadMainMenus(true), bFirstChanceLoad(false),
    pOverTileTarget(nullptr), pOverTileMenu(nullptr), pDragOverTileTarget(nullptr), pDragOverTileMenu(nullptr),
    bFullHelp(false), cLastMessageButtonClicked(-1), fpResult(nullptr), bTextureRelease(false), bMouseInMotion(true),
    pPickRef(nullptr), pReticleRef(nullptr), pCrossHairRef(nullptr), pActivateRef(nullptr), pTelekinesisRef(nullptr),
    pActivatePickLocation(0.0f, 0.0f, 0.0f), bFuzzyActivatePick(false), iEnterStack(), bClickMultithreaded(false),
    bShiftDown(false), iModifierKeys(0), iRepeatingKey(0), uKeyDownTime(0), iLastGamepadEvent(Interface::eButtonNone),
    bIsInRenderedMenu(false), pCurrentRenderedMenu(nullptr), bMouseOverRenderedMenu(false), pPipboy(nullptr),
    HUDEffects(), VATSEffect(), fPipboyMouseX(0.0f), fPipboyMouseY(0.0f), iPipboyMenuToSet(0),
    ePipboyState(InterfaceManager::PS_CLOSED), pPipboyCallback(nullptr), xGlowBranchTiles(),
    bQueueCloseSupportMenus(false), bContainerOpenQueued(false), fAspectRatioRatio(1.0f), xTutorialManager() {
    AutoMemContext kMemContext(MEM_CONTEXT_INTERFACE);
    InterfaceManager::fCurrAlphaDPS = 0.0f;
    InterfaceManager::bAlphaGoingDown = false;
    InterfaceManager::iLastDPSFadeTime = appTimer.iLastTime;
    double fHalfWidth = InterfaceManager::GetScreenWidth() / 2.0;
    float fHeight = fHalfWidth / std::sin(0.65449846f);
    fOneToOneDistance = std::cos(0.65449846f) * fHeight;
    if (!spAlphaProp.m_pObject) {
        spAlphaProp = new NiAlphaProperty();
        spAlphaProp->SetAlphaBlending(true);
        spAlphaProp->SetNoSorter(false);
        spAlphaProp->SetSrcBlendMode(NiAlphaProperty::ALPHA_SRCALPHA);
        spAlphaProp->SetDestBlendMode(NiAlphaProperty::ALPHA_INVSRCALPHA);
    }
    sp3DSceneGraph = InitSceneGraph(Main::MenuRootNode3D(), "Menu3DRoot", true);
    spSceneGraph = InitSceneGraph(Main::MenuRootNode(), "MenuRoot", false);
    MenuManager::Instance(true);
    pViewCaster = new ViewCaster();
    pTimers = new InterfaceManager::Timer();
    pTimers->pPrev = pTimers;
    spInterfaceAccum = new BSShaderAccumulator(100, 1u, 759u);
    spInterfaceAccum->SetRenderMode(BSShaderManager::BSSM_RENDER_INTERFACE);
    spInterfaceAccum->SetInterfaceSort(true);
    sp3dInterfaceAccum = new BSShaderAccumulator(99, 1u, 759u);
}

InterfaceManager::~InterfaceManager() {
    if (pTargetReticle) {
        delete pTargetReticle;
        pTargetReticle = nullptr;
    }
    LoadingMenu::KillThread();
    MenuConsole::Free();
    DebugText::Free();
    FontManager::Free();
    bClosingAllSupportMenus = 1;
    delete pStringRoot;
    delete pMenusRoot;
    bClosingAllSupportMenus = 0;
    MenuManager::Free();
    delete pCursor;
    InterfaceManager::pInstance = nullptr;
    Tile::FreeTraitList();
    Tile::FreeMaybePostUpdateList();
    delete pViewCaster;
    FreeTimerList();
    Menu::FreeXMLArchive();
    InterfaceManager::xActiveListBoxes.RemoveAll();
    SystemColorManager::DeleteInstance();
    MessageMenu::ClearMessageQueue();
    Interface::ClearTempModel();
    delete pPipboy;
    delete pShadowNode;
    delete pUIPlayerNode;
    StartMenu::DestroyMenuElements();
    ClearRenderedMenuDeleteList();
}

void InterfaceManager::ToggleDebugTextVisible() {
    if (bDebugTextVisible) {
        SetDebugTextVisible(false);
    }
    else {
        SetDebugTextVisible(true);
    }
}

bool InterfaceManager::GetMouseVisible() {
    return !InterfaceManager::Has360Controller() || bShowMouse;
}

InterfaceManager* InterfaceManager::SetQueuedPipboyTab(
    uint8_t aucOverPipBoyScreen,
    float afMouseX,
    float afMouseY,
    uint32_t auiMenuToSet
) {
    // Stores the argument's raw byte as it is, without converting it to bool.
    *reinterpret_cast<uint8_t*>(&bOverPipboyScreen) = aucOverPipBoyScreen;
    fPipboyMouseX = afMouseX;
    fPipboyMouseY = afMouseY;
    iPipboyMenuToSet = auiMenuToSet;
    return this;
}

void InterfaceManager::HandleControllerStateChange(int32_t aiState) {
    InterfaceManager::Instance();
    bool bHas360Controller = InterfaceManager::Has360Controller();
    Tile::AddToTable("&xenon;", bHas360Controller);
    InterfaceManager::Instance();
    bHas360Controller = InterfaceManager::Has360Controller();
    Tile::AddToTable("&xbox;", bHas360Controller);
    int32_t iHas360Controller = InterfaceManager::Has360Controller();
    int32_t iTrait = Tile::TextToTrait("_Has360Controller");
    pStringRoot->Set(iTrait, iHas360Controller);
    RecursivelySetVisibilityDirty(pMenusRoot);
    NiTListItem<Tile*>* pPos = pMenusRoot ? pMenusRoot->xChildren.m_pkHead : nullptr;
    while (pPos) {
        Tile* pChild = pMenusRoot->xChildren.GetNext(pPos);
        if (pChild && pChild->GetMenu()) {
            Menu* pMenu = pChild->GetMenu();
            bHas360Controller = InterfaceManager::Has360Controller();
            pMenu->DoControllerSwitch(bHas360Controller);
        }
    }
    StartMenu::GoBackIfInActionMapping();
}

char InterfaceManager::GetMapPageNumber() {
    return cMapPageNumber;
}

void InterfaceManager::DumpMenuPackingInfo() {
    HUDEffects.GetPacker()->DumpInfo();
}

void InterfaceManager::ToggleSafeZone(int32_t abForceSetTo) {
    if (pSafeZone && abForceSetTo == 2) {
        pSafeZone->IsTrue(4003);
    }

    if (abForceSetTo == 3) {
        if (pSafeZone) {
            delete pSafeZone;
        }
        pSafeZone = nullptr;
    }
    if (pSafeZone) {
        delete pSafeZone;
        pSafeZone = nullptr;
    }
    else {
        pSafeZone = GetMenusRoot()->ReadFile("Data\\Menus\\Main\\safe_zone.xml");
        pSafeZone->Set(4003, 1);
        RecursiveFade(pSafeZone->GetModel(), 1.0f, 0.0f);
    }
}

void InterfaceManager::UpdateScreenShape(int32_t isizex, int32_t isizey) {
    if (isizex > 0) {
        iSafeZoneX = isizex;
        if (isizey <= 0) {
            iSafeZoneY = isizex;
        }
        else {
            iSafeZoneY = isizey;
        }
    }
    float fSafeX = InterfaceManager::GetScreenSafeX();
    pMenusRoot->Set(4032, fSafeX, true);
    float fSafeY = InterfaceManager::GetScreenSafeY();
    pMenusRoot->Set(4031, fSafeY, true);
    float fScreenWidth = InterfaceManager::GetScreenWidth();
    pMenusRoot->Set(4017, fScreenWidth, true);
    float fScreenHeight = InterfaceManager::GetScreenHeight();
    pMenusRoot->Set(4016, fScreenHeight, true);
}

void InterfaceManager::SetCursorPos(float afX, float afY) {
    NiPoint3 xNewPos;
    static NiPoint3 xLastPos;
    sCursorPos = pCursor->GetModel()->GetTranslate();
    double dScaledX = InterfaceManager::GetScreenWidth() * afX;
    sCursorPos.x = dScaledX - InterfaceManager::GetScreenWidth() / 2.0;
    double dScaledZ = InterfaceManager::GetScreenHeight() * -afY;
    sCursorPos.z = InterfaceManager::GetScreenHeight() / 2.0 + dScaledZ;
    pCursor->GetModel()->SetTranslate(sCursorPos);
    NiUpdateData UpdateData(0.0f, false, false);
    pCursor->GetModel()->Update(UpdateData);
    double dCursorX = sCursorPos.x + 3.0;
    double dRealScreenWidth = InterfaceManager::GetRealScreenWidth();
    double dRealX = dRealScreenWidth / InterfaceManager::GetScreenWidth() * dCursorX;
    xNewPos.x = InterfaceManager::GetRealScreenWidth() / 2.0 + dRealX;
    xNewPos.y = 0.0f;
    double dHalfRealHeight = InterfaceManager::GetRealScreenHeight() / 2.0;
    double dCursorZ = sCursorPos.z;
    double dRealScreenHeight = InterfaceManager::GetRealScreenHeight();
    xNewPos.z = dHalfRealHeight - dRealScreenHeight / InterfaceManager::GetScreenHeight() * dCursorZ;
    if (xNewPos != xLastPos) {
        xLastPos = xNewPos;
        xNewPos.y = pCursor->GetFloat(4013) * -0.00800000037997961;
        sRealCursorPos = xNewPos;
        if (!InterfaceManager::Has360Controller() || bShowMouse) {
            bMouseInMotion = true;
            if (GetCurrentMode() == 2) {
                pCursor->GetModel()->SetAppCulled(false);
                pCursor->Set(4003, 1);
            }
        }
    }
    if (xNewPos.x != sRealCursorPos.x || xNewPos.z != sRealCursorPos.z) {
        bMouseInMotion = true;
        dRealScreenWidth = InterfaceManager::GetRealScreenWidth();
        dRealX = dRealScreenWidth / InterfaceManager::GetScreenWidth() * sRealCursorPos.x;
        sRealCursorPos.x = InterfaceManager::GetRealScreenWidth() / 2.0 + dRealX;
        sRealCursorPos.y = pCursor->GetFloat(4013) * -0.00800000037997961;
        dHalfRealHeight = InterfaceManager::GetRealScreenHeight() / 2.0;
        dRealScreenHeight = InterfaceManager::GetRealScreenHeight();
        sRealCursorPos.z = dHalfRealHeight - dRealScreenHeight / InterfaceManager::GetScreenHeight() * sRealCursorPos.z;
    }
}

void InterfaceManager::EmergencyCloseAllMenusAndBreakStuff() {
    for (int32_t i = 9; i >= 0; --i) {
        if (iEnterStack[i]) {
            Tile* pMenuTile = Menu::GetMenuByClass(iEnterStack[i]);
            if (pMenuTile) {
                Menu* pMenu = pMenuTile->GetMenu();
                if (pMenu) {
                    if (pMenuTile == Menu::GetMenuByClass(1007)) {
                        Interface::CloseLoadingMenu();
                    }
                    else {
                        delete pMenu;
                    }
                }
            }
            else if (iEnterStack[i] == 3 && MenuConsole::Instance(false)) {
                MenuConsole* pConsole = MenuConsole::Instance(true);
                pConsole->ToggleVisible();
            }
            iEnterStack[i] = 0;
        }
    }
    Interface::HideMenus();
    cMenuMode = eMenuModeLeaving;
    Interface::bAllowPauseMenu = true;
}

void InterfaceManager::ClosePipboy(void (*apCallback)()) {
    if (ePipboyState || !apCallback) {
        if (ePipboyState == 3) {
            ePipboyState = PS_CLOSING_SETUP;
            pPipboyCallback = apCallback;
        }
    }
    else {
        apCallback();
    }
}

int32_t InterfaceManager::GetMouseWheel() {
    return static_cast<int32_t>(fMouseWheel);
}

bool InterfaceManager::IsInMenuMode() {
    return this->cMenuMode != 1;
}

uint8_t InterfaceManager::GetCurrentMode() {
    return static_cast<uint8_t>(this->cMenuMode);
}

int8_t InterfaceManager::GetStatsPageNumber() {
    return this->cStatsPageNumber;
}

int8_t InterfaceManager::GetMagicPageNumber() {
    return this->cMagicPageNumber;
}

void InterfaceManager::SetStatsPageNumber(char acTab) {
    this->cStatsPageNumber = acTab;
    Tile* const menuTile = Menu::GetMenuByClass(1003 /* kMenu_Stats */);
    Menu* const statsMenu = menuTile ? static_cast<Tile*>(menuTile)->GetMenu() : nullptr;
    if (statsMenu) {
        static_cast<StatsMenu*>(statsMenu)->SetCurrentTab(static_cast<int32_t>(static_cast<uint8_t>(acTab)));
    }
}

void InterfaceManager::SetInventoryPageNumber(char acTab) {
    this->cInventoryPageNumber = acTab;
    Tile* const menuTile = Menu::GetMenuByClass(1002 /* kMenu_Inventory */);
    Menu* const inventoryMenu = menuTile ? static_cast<Tile*>(menuTile)->GetMenu() : nullptr;
    if (inventoryMenu) {
        InventoryMenu::SetCurrentTab(static_cast<int32_t>(acTab));
    }
}

void InterfaceManager::SetMapPageNumber(char acTab) {
    this->cMapPageNumber = acTab;
    Tile* const menuTile = Menu::GetMenuByClass(1023 /* kMenu_Map */);
    Menu* const mapMenu = menuTile ? static_cast<Tile*>(menuTile)->GetMenu() : nullptr;
    if (mapMenu) {
        static_cast<MapMenu*>(mapMenu)->SetPageNumber(static_cast<int32_t>(acTab));
    }
}

bool InterfaceManager::IsDebugTextVisible() {
    return this->bDebugTextVisible;
}

void InterfaceManager::SetDebugTextVisible(bool abVisible) {
    this->bDebugTextVisible = abVisible;
}

NiNode* InterfaceManager::GetCursorRoot() {
    return this->pCursorRoot;
}

NiNode* InterfaceManager::GetDebugTextRoot() {
    return this->pDebugTextRoot;
}

TESObjectREFR* InterfaceManager::GetConsoleSelection() {
    return this->pPickRef;
}

FOPipboyManager* InterfaceManager::GetPipboy() {
    return this->pPipboy;
}

bool InterfaceManager::GetFirstChanceTextureRelease() {
    return this->bTextureRelease;
}

void InterfaceManager::SetPickDistance(int32_t aiDistance) {
    iPickDistance = aiDistance;
}

TileImage* InterfaceManager::GetCursorTile() {
    return pCursor;
}

float InterfaceManager::GetPipboyMouseX() {
    double dMouseX = fPipboyMouseX * 1280.0;
    return static_cast<float>(dMouseX / Renderer::GetScreenWidth() * fAspectRatioRatio);
}

float InterfaceManager::GetPipboyMouseY() {
    double dMouseY = fPipboyMouseY * 960.0;
    return static_cast<float>(dMouseY / Renderer::GetScreenHeight());
}

Tile* InterfaceManager::GetStringRoot() {
    return pStringRoot;
}

void InterfaceManager::ClearCurrentFocusTarget() {
    this->pMouseOverTarget = nullptr;
}

void InterfaceManager::SetCurrentMainMenu([[maybe_unused]] int32_t aiMenu) {}

void InterfaceManager::NeedToUpdate(bool abNeedToUpdate) {
    this->bNeedToUpdate = abNeedToUpdate;
}

int32_t InterfaceManager::GetEnterStack(int32_t aiIndex) {
    return this->iEnterStack[aiIndex];
}

int32_t InterfaceManager::GetEnterStackTop() {
    int32_t i = 0;
    while (i < 10 && this->iEnterStack[i] != 0) {
        ++i;
    }
    if (i >= 10) {
        return this->iEnterStack[9];
    }
    if (i == 0) {
        return 0;
    }
    return this->iEnterStack[i - 1];
}

bool InterfaceManager::TileIsAcceptingEvents(Tile* pTile) {
    Tile* const topMenu = Menu::GetMenuByClass(this->GetEnterStackTop());
    if (!topMenu) {
        return true;
    }
    const int32_t stackingType = static_cast<int32_t>(
        static_cast<Tile*>(topMenu)->GetFloat(Tile::kTileValue_StackingType)
    );
    if (stackingType != 102 && stackingType != 6006) {
        return true;
    }
    return pTile->GetMenu() == static_cast<Tile*>(topMenu)->GetMenu();
}

void InterfaceManager::UpdateQueuedMenuClosings() {
    if (this->bQueueCloseSupportMenus) {
        Interface::CloseAllSupportMenus(false);
        this->bQueueCloseSupportMenus = false;
    }
}

FORenderedMenu* InterfaceManager::GetCurrentRenderedMenu() {
    if (InterfaceManager::IsInPipboyMenu()) {
        return reinterpret_cast<FORenderedMenu*>(this->pPipboy);
    }
    return this->pCurrentRenderedMenu;
}

bool InterfaceManager::IsInRenderedMenu() {
    return this->bIsInRenderedMenu || InterfaceManager::IsInPipboyMenu();
}

void InterfaceManager::AddTileToUpdateList(Tile* apTile) {
    if (!apTile) {
        return;
    }
    if (xTileUpdateList.IsInArray(apTile)) {
        return;
    }
    xTileUpdateList.Add(apTile);
}

void InterfaceManager::RemoveTileFromUpdateList(Tile* apTile) {
    // Find uses the array's shared 4-byte compare.
    const int32_t index = xTileUpdateList.Find(apTile, 0, &BSSimpleArray<Tile*>::DefaultEqualFunctionByAddress);
    if (index != -1) {
        xTileUpdateList.RemoveFast(index, 1);
    }
}

// Stores the argument byte as it is; a bool argument keeps clang from normalising it to 0/1.
void InterfaceManager::SetClickMultithreaded(bool abValue) {
    this->bClickMultithreaded = abValue;
}

void InterfaceManager::ForceTextureRelease() {
    FORenderedTerminal::ReleaseStaticGeometry();
    this->GetMenusRoot()->ForceTextureRelease();
    MapMenu::ClearWorldMapTexture();
    TES::pInstance->CleanUpUnusedTextures(true);
}

void InterfaceManager::UpdateTileModels() {
    for (uint32_t i = 0; i < InterfaceManager::xTileUpdateList.iSize; ++i) {
        Tile* const tile = static_cast<Tile*>(InterfaceManager::xTileUpdateList.GetAt(i));
        NiNode* const model = tile ? tile->GetModel() : nullptr;
        if (model) {
            NiUpdateData updateData(0.0f, false, false);
            model->Update(updateData);
        }
        tile->ClearNeedsNiUpdate();
    }
    // Only the main thread empties the list.
    if (BSGetCurrentThreadID() == appMain->GetThread()) {
        InterfaceManager::xTileUpdateList.Clear(true);
    }
}

void InterfaceManager::Accumulate() {
    this->bClickMultithreaded = true;
    SceneGraph* const sceneGraph = this->GetSceneGraph();
    if (!this->IsInMenuMode() && !Interface::IsInGameLoadingMenuOpen()) {
        this->LastMinuteUpdate();
    }
    if (this->IsInRenderedMenu() && !Interface::IsInGameLoadingMenuOpen()) {
        Interface::IsolateMenuElements(false, false);
    }
    BSShaderAccumulator* const accumulator = this->spInterfaceAccum.m_pObject;
    NiCamera* const camera = sceneGraph->GetCamera();
    MTRenderingSystem::QInstance()->AddAccumTask(
        camera,
        nullptr,
        sceneGraph,
        nullptr,
        nullptr,
        accumulator,
        BSCullingProcess::BSCP_CULL_ALLPASS,
        0x17,
        0
    );
    MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 0x17);
}

// Draws the menus. Outside a frame it starts the default render target itself (letterboxed to the framebuffer viewport,
// and with the side screens cleared to black on Eyefinity), and afterwards resets the device and the cached render
// states and stops the target. Neither argument is read.
void InterfaceManager::Click(BSRenderedTexture*, BSRenderedTexture*) {
    NiRenderer* const pRenderer = NiRenderer::GetRenderer();
    bool bStarted = false;
    if (BSRenderedTexture::QOutsideFrame()) {
        if (BSShaderManager::bLetterbox) {
            const uint32_t
                uiWidth = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->GetDefaultBackBuffer()->GetWidth();
            const uint32_t
                uiHeight = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->GetDefaultBackBuffer()->GetHeight();
            const NiRect<float>& rPort = BSShaderManager::kFramebufferViewport;
            D3DVIEWPORT9 kViewport;
            kViewport.MinZ = 0.0f;
            kViewport.MaxZ = 1.0f;
            // The sizes are converted as unsigned 64-bit integers; each product is truncated to 64 bits and its low 32
            // bits kept. The edge differences are computed in double, so they are not rounded to float.
            kViewport.X = static_cast<uint32_t>(static_cast<int64_t>(static_cast<double>(uiWidth) * rPort.m_left));
            kViewport.Width = static_cast<uint32_t>(
                static_cast<int64_t>((static_cast<double>(rPort.m_right) - rPort.m_left) * static_cast<double>(uiWidth))
            );
            kViewport.Y = static_cast<uint32_t>(static_cast<int64_t>(static_cast<double>(uiHeight) * rPort.m_bottom));
            kViewport.Height = static_cast<uint32_t>(static_cast<int64_t>(
                (static_cast<double>(rPort.m_top) - rPort.m_bottom) * static_cast<double>(uiHeight)
            ));
            static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->GetD3DDevice()->SetViewport(&kViewport);
        }
        BSRenderedTexture::Start(NiRenderer::CLEAR_ALL, nullptr);
        bStarted = true;
        if (IsEyefinity()) {
            alignas(NiColorA) uint8_t aucBackground[sizeof(NiColorA)];
            NiColorA& kBackground = *::new (aucBackground) NiColorA(0.0f, 0.0f, 0.0f, 0.0f);
            static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->GetBackgroundColor(kBackground);
            static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->SetBackgroundColorAlpha(NiColorA::BLACK);
            // Builds the rect with NiColorA's constructor and then stores the edges. The outer thirds of the screen are
            // cleared.
            NiRect<float> kRect;
            ::new (&kRect) NiColorA(0.0f, 0.0f, 0.0f, 0.0f);
            kRect.m_left = 0.0f;
            kRect.m_right = 0.33f;
            kRect.m_top = 1.0f;
            kRect.m_bottom = 0.0f;
            NiRenderer::GetRenderer()->ClearBuffer(&kRect, NiRenderer::CLEAR_BACKBUFFER);
            kRect = {0.66f, 1.0f, 1.0f, 0.0f};
            NiRenderer::GetRenderer()->ClearBuffer(&kRect, NiRenderer::CLEAR_BACKBUFFER);
            static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->SetBackgroundColorAlpha(kBackground);
        }
        if (BSShaderManager::bLetterbox) {
            this->GetSceneGraph()->GetCamera()->SetViewPort(BSShaderManager::kFramebufferViewport);
        }
    }

    // The AutoMemContext is constructed and destroyed explicitly on a stack buffer and the constructor's result is
    // ignored; the culler is destroyed before the context.
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\InterfaceManager.cpp",
        4201
    );
    SceneGraph* const pSceneGraph = this->GetSceneGraph();
    if (!pSceneGraph) {
        pContext->~AutoMemContext();
        return;
    }
    alignas(BSCullingProcess) uint8_t aCullerStorage[sizeof(BSCullingProcess)];
    BSCullingProcess* const pCuller = ::new (aCullerStorage)
        BSCullingProcess(pSceneGraph->GetCullingProcess()->GetVisibleSet());
    {
        // A reference to the renderer's sorter is held while the menus are drawn.
        NiPointer<NiAccumulator> spSorter(pRenderer->GetSorter());
        if (NiParallelUpdateTaskManager::Get()) {
            if (NiParallelUpdateTaskManager::IsActive()) {
                NiParallelUpdateTaskManager::EndUpdate();
            }
            NiParallelUpdateTaskManager::WaitRenderSema(-1);
        }
        // The setting's and the flag's whole bytes are tested, so they are read as integers.
        bool bPipboyVisible;
        if (reinterpret_cast<const uint8_t&>(bUsePipboyMode.Bool()) != 0) {
            bPipboyVisible = this->ePipboyState == 2 ||
                ((InterfaceManager::IsInPipboyMenu() ||
                  reinterpret_cast<const uint8_t&>(this->bIsInRenderedMenu) != 0) &&
                 InterfaceManager::IsCurrentRenderedMenuTopmost());
        }
        else {
            bPipboyVisible = false;
        }
        this->DrawInterface_Screen(pCuller, bPipboyVisible);

        if (bStarted) {
            D3DVIEWPORT9 kViewport;
            kViewport.X = 0;
            kViewport.Y = 0;
            kViewport.MinZ = 0.0f;
            kViewport.MaxZ = 1.0f;
            NiRenderTargetGroup* pTarget = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())
                                               ->GetCurrentRenderTargetGroup();
            if (!pTarget) {
                pTarget = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->GetDefaultRenderTargetGroup();
            }
            kViewport.Width = pTarget->GetWidth(0);
            kViewport.Height = pTarget->GetHeight(0);
            static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->GetD3DDevice()->SetViewport(&kViewport);
            // The declaration's reference from GetVertexDeclaration is never released.
            IDirect3DVertexDeclaration9* pDeclaration = nullptr;
            static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->GetD3DDevice()->GetVertexDeclaration(&pDeclaration);
            static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())
                ->GetD3DDevice()
                ->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
            static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->GetRenderState()->SetVertexShader(nullptr, false);
            static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->GetRenderState()->SetPixelShader(nullptr, false);
            static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->GetD3DDevice()->SetPixelShader(nullptr);
            static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->GetD3DDevice()->SetVertexShader(nullptr);
            static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->GetD3DDevice()->SetVertexDeclaration(pDeclaration);
            BSRenderState::RestoreZEnable(BSRS_NOLOCK);
            BSRenderState::RestoreZWriteEnable(BSRS_NOLOCK);
            BSRenderState::RestoreZFunc(BSRS_NOLOCK);
            BSRenderState::RestoreAlphaTestEnable(BSRS_NOLOCK);
            BSRenderState::RestoreAlphaTestFuncRef(BSRS_NOLOCK);
            BSRenderState::RestoreAlphaBlendEnable(BSRS_NOLOCK);
            BSRenderState::RestoreAlphaBlendFunc(BSRS_NOLOCK);
            BSRenderState::RestoreColorWriteEnable(BSRS_NOLOCK);
            BSRenderState::RestoreStencilEnable(BSRS_NOLOCK);
            BSRenderState::RestoreStencilAction(BSRS_NOLOCK);
            BSRenderState::RestoreStencilFunc(BSRS_NOLOCK);
            BSRenderState::RestoreStencilWriteMask(BSRS_NOLOCK);
            BSRenderState::RestoreScissorEnable(BSRS_NOLOCK);
            BSRenderState::RestoreScissorRect(BSRS_NOLOCK);
            BSRenderState::RestoreUserClipPlaneEnables(BSRS_NOLOCK);
            BSRenderState::RestoreDepthBias(BSRS_NOLOCK);
            BSRenderState::RestoreWireframeEnable(BSRS_NOLOCK);
            BSRenderState::RestoreAlphaToCoverageEnable(BSRS_NOLOCK);
            BSRenderedTexture::Stop();
            NiDX9RenderState* const pRenderState = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())
                                                       ->GetRenderState();
            pRenderState->ClearDeclaration(pRenderState->GetDeclaration());
            pRenderState->ClearVertexShader(pRenderState->GetVertexShader());
            pRenderState->ClearPixelShader(pRenderState->GetPixelShader());
        }
        if (NiParallelUpdateTaskManager::Get()) {
            NiParallelUpdateTaskManager::SignalRenderSema();
        }
    }
    pCuller->~BSCullingProcess();
    pContext->~AutoMemContext();
}

int32_t InterfaceManager::AddToEnterStack(int32_t aiID) {
    if (aiID == 3) {
        // The console goes to the bottom of the stack: everything above moves up one slot.
        int32_t previous = this->iEnterStack[0];
        this->iEnterStack[0] = 3;
        int32_t i;
        for (i = 1; i < 9; ++i) {
            const int32_t current = this->iEnterStack[i];
            this->iEnterStack[i] = previous;
            previous = current;
            if (current == 0) {
                break;
            }
        }
        if (i == 9) {
            Warning("MENUS: ### Menu Stack Size is too small - a menu may have been lost");
        }
        if (i == 1) {
            this->cMenuMode = eMenuModeEntering;
            if (BSShaderManager::GetImageSpaceManager()) {
                BSShaderManager::GetImageSpaceManager()->SetMenuBGBlendEnabled(true);
            }
        }
        return i;
    }
    int32_t i;
    for (i = 0; i < 10; ++i) {
        if (this->iEnterStack[i] == 0) {
            break;
        }
    }
    if (i >= 10) {
        return -1;
    }
    this->iEnterStack[i] = aiID;
    if (i == 0) {
        this->cMenuMode = eMenuModeEntering;
        if (aiID != Interface::eMessageMenu && BSShaderManager::GetImageSpaceManager()) {
            BSShaderManager::GetImageSpaceManager()->SetMenuBGBlendEnabled(true);
        }
    }
    else if (i == 1 && this->iEnterStack[0] == Interface::eMessageMenu && BSShaderManager::GetImageSpaceManager()) {
        BSShaderManager::GetImageSpaceManager()->SetMenuBGBlendEnabled(true);
    }
    return i;
}

int32_t InterfaceManager::PopFromEnterStack(int32_t aiID, bool abForceRemoval) {
    if (this->iEnterStack[0] == 0) {
        return -1;
    }
    if (!abForceRemoval) {
        // Refused (-2) when a menu above the message menu sits over aiID; -1 when aiID is not on the stack.
        bool found = false;
        int32_t i;
        for (i = 0; i < 10; ++i) {
            if (found && this->iEnterStack[i] > Interface::eMessageMenu) {
                break;
            }
            if (this->iEnterStack[i] == aiID) {
                found = true;
            }
        }
        if (found && i < 10 && aiID != 3) {
            return -2;
        }
        if (!found) {
            return -1;
        }
    }
    // Squeezes every aiID out of the stack. When the stack is full, the read index deliberately runs past the ten slots
    // into the fields after the stack.
    int32_t* const stack = this->iEnterStack;
    int32_t i = 0;
    int32_t j = 0;
    for (; i < 10; ++i, ++j) {
        if (stack[i] == aiID) {
            ++j;
        }
        stack[i] = stack[j];
        if (stack[i] == 0) {
            break;
        }
    }
    if (this->iEnterStack[0] == 3 && this->iEnterStack[1] == 0) {
        // Only the console is left: close it and pop it too.
        if (MenuConsole::Instance(false)) {
            MenuConsole::Instance(true)->ToggleVisible();
        }
        return this->PopFromEnterStack(3, false);
    }
    if (i > 0) {
        if (i == 1 && this->iEnterStack[0] == Interface::eMessageMenu) {
            BSShaderManager::GetImageSpaceManager()->SetMenuBGBlendEnabled(true);
        }
        return this->iEnterStack[i - 1];
    }
    if (aiID == Interface::eMainFour) {
        Interface::HideMenus();
    }
    this->cMenuMode = eMenuModeLeaving;
    BSShaderManager::GetImageSpaceManager()->SetMenuBGBlendEnabled(aiID != Interface::eMessageMenu);
    return 0;
}

int32_t InterfaceManager::GetChar(KEY_EVENT_TYPE aiEventType, int32_t aikey) {
    // The release masks also clear the upper half of iModifierKeys, deliberately.
    int32_t key = 0;
    if (aiEventType == 2) {
        if (aikey == 0x2A || aikey == 0x36) {
            this->iModifierKeys &= 0xFFFF & ~MODIFIER_SHIFT;
            this->bShiftDown = false;
        }
        else if (aikey == 0x38 || aikey == 0xB8) {
            this->iModifierKeys &= 0xFFFF & ~MODIFIER_ALT;
        }
        else if (aikey == 0x1D || aikey == 0x9D) {
            this->iModifierKeys &= 0xFFFF & ~MODIFIER_CONTROL;
        }
        this->SetRepeatingKey(0);
    }
    else if (aiEventType == 1) {
        const bool shiftDown = this->bShiftDown;
        const int32_t translated = appMain->GetControls()->TranslateKeystroke(aikey, shiftDown);
        if (translated == 0x1B && !StartMenu::IsInPreIIS()) {
            key = 0;
        }
        else if (translated == 8) {
            key = static_cast<int32_t>(0x80000000);
            this->SetRepeatingKey(static_cast<int32_t>(0x80000000));
        }
        else if (translated == 0x7C) {
            key = 0;
        }
        else if (translated == 0xD) {
            key = static_cast<int32_t>(0x80000008);
        }
        else if (aikey == 0x2A || aikey == 0x36) {
            this->iModifierKeys |= MODIFIER_SHIFT;
            this->bShiftDown = true;
        }
        else if (aikey == 0x38 || aikey == 0xB8) {
            this->iModifierKeys |= MODIFIER_ALT;
        }
        else if (aikey == 0x1D || aikey == 0x9D) {
            this->iModifierKeys |= MODIFIER_CONTROL;
        }
        else if (aikey == 0xCB) {
            key = static_cast<int32_t>(0x80000001);
            this->SetRepeatingKey(static_cast<int32_t>(0x80000001));
        }
        else if (aikey == 0xCD) {
            key = static_cast<int32_t>(0x80000002);
            this->SetRepeatingKey(static_cast<int32_t>(0x80000002));
        }
        else if (aikey == 0xC8) {
            key = static_cast<int32_t>(0x80000003);
            this->SetRepeatingKey(static_cast<int32_t>(0x80000003));
        }
        else if (aikey == 0xD0) {
            key = static_cast<int32_t>(0x80000004);
            this->SetRepeatingKey(static_cast<int32_t>(0x80000004));
        }
        else if (aikey == 0xC7) {
            key = static_cast<int32_t>(0x80000005);
        }
        else if (aikey == 0xCF) {
            key = static_cast<int32_t>(0x80000006);
        }
        else if (aikey == 0xD1) {
            key = static_cast<int32_t>(0x8000000A);
        }
        else if (aikey == 0xC9) {
            key = static_cast<int32_t>(0x80000009);
        }
        else if (aikey == 0xD3) {
            key = static_cast<int32_t>(0x80000007);
            this->SetRepeatingKey(static_cast<int32_t>(0x80000007));
        }
        else {
            key = translated;
        }
    }
    return key;
}

void InterfaceManager::ClearOverTileTarget(bool abDoLeave) {
    if (abDoLeave && this->pOverTileMenu && this->pOverTileTarget) {
        Tile* const target = this->pOverTileTarget;
        Menu* const menu = this->pOverTileMenu;
        if (target) {
            target->Set(Tile::kTileValue_Mouseover, 0);
            target->Set(Tile::kTileValue_Clicked, 0);
            if (menu) {
                this->DoLeave(menu, static_cast<int32_t>(target->GetFloat(Tile::kTileValue_ID)), target);
            }
        }
    }
    this->pOverTileTarget = nullptr;
    this->pOverTileMenu = nullptr;
}

void InterfaceManager::SetCurrentFocusTarget(Tile* apTarget, int32_t aiType, bool abPlaySounds) {
    // The tile losing the mouse-over remembers when it last had it, through an increasing xdefault stamp.
    if (this->pMouseOverTarget &&
        this->pMouseOverTarget != apTarget &&
        this->pMouseOverTarget->GetFloat(Tile::kTileValue_xdefault) > 0.0) {
        ++this->iLastXDefault;
        this->pMouseOverTarget->Set(Tile::kTileValue_xdefault, static_cast<uint32_t>(this->iLastXDefault));
    }
    if (apTarget) {
        if (this->pOverTileTarget) {
            this->pOverTileTarget->Set(Tile::kTileValue_Mouseover, 0);
            Tile* const target = this->pOverTileTarget;
            const int32_t id = static_cast<int32_t>(this->pOverTileTarget->GetFloat(Tile::kTileValue_ID));
            Menu* const menu = this->pOverTileTarget->GetMenu();
            this->DoLeave(menu, id, target);
        }
        this->pOverTileTarget = nullptr;
        this->pOverTileMenu = nullptr;
    }
    if (!apTarget) {
        if (this->pMouseOverTarget) {
            this->pMouseOverTarget->Set(Tile::kTileValue_Mouseover, 0);
            Tile* const target = this->pMouseOverTarget;
            const int32_t id = static_cast<int32_t>(this->pMouseOverTarget->GetFloat(Tile::kTileValue_ID));
            Menu* const menu = this->pMouseOverTarget->GetMenu();
            this->DoLeave(menu, id, target);
        }
        this->ClearOverTileTarget(false);
        this->pMouseOverTarget = nullptr;
    }
    else if (aiType == Tile::kTileValue_Mouseover && this->pMouseOverTarget != apTarget) {
        if (this->pMouseOverTarget) {
            this->pMouseOverTarget->Set(Tile::kTileValue_Mouseover, 0);
            Tile* const target = this->pMouseOverTarget;
            const int32_t id = static_cast<int32_t>(this->pMouseOverTarget->GetFloat(Tile::kTileValue_ID));
            Menu* const menu = this->pMouseOverTarget->GetMenu();
            this->DoLeave(menu, id, target);
        }
        this->pMouseOverTarget = apTarget;
        if (this->pMouseOverTarget && this->pMouseOverTarget->GetFloat(Tile::kTileValue_xdefault) > 0.0) {
            ++this->iLastXDefault;
            this->pMouseOverTarget->Set(Tile::kTileValue_xdefault, static_cast<uint32_t>(this->iLastXDefault));
        }
        apTarget->Set(Tile::kTileValue_Mouseover, 1);
        if (abPlaySounds) {
            apTarget->PlayTileSound(Tile::kTileValue_MouseoverSound);
        }
        Tile* const target = this->pMouseOverTarget;
        const int32_t id = static_cast<int32_t>(this->pMouseOverTarget->GetFloat(Tile::kTileValue_ID));
        Menu* const menu = this->pMouseOverTarget->GetMenu();
        this->DoEnter(menu, id, target);
    }
    else if (aiType == Tile::kTileValue_Clicked) {
        const int32_t clickSound = static_cast<int32_t>(apTarget->GetFloat(Tile::kTileValue_ClickSound));
        if (abPlaySounds && clickSound) {
            InterfaceManager::PlayMenuSound(static_cast<MENU_SOUNDS>(clickSound));
        }
        apTarget->Set(Tile::kTileValue_Clicked, 1);
        apTarget->Set(Tile::kTileValue_Clicked, 0);
        Menu* const menu = apTarget->GetMenu();
        menu->DoClick(static_cast<int32_t>(apTarget->GetFloat(Tile::kTileValue_ID)), apTarget);
        apTarget->UpdateChildren(false);
        if (this->pMouseOverTarget &&
            (!this->pMouseOverTarget->IsVisible() || !this->pMouseOverTarget->IsTrue(Tile::kTileValue_Target))) {
            // The click hid the tile under the mouse: leave it.
            this->pMouseOverTarget->Set(Tile::kTileValue_Mouseover, 0);
            Tile* const target = this->pMouseOverTarget;
            const int32_t id = static_cast<int32_t>(this->pMouseOverTarget->GetFloat(Tile::kTileValue_ID));
            Menu* const leaveMenu = this->pMouseOverTarget->GetMenu();
            this->DoLeave(leaveMenu, id, target);
            this->ClearOverTileTarget(false);
            this->pMouseOverTarget = nullptr;
        }
    }
}

void InterfaceManager::GetDefaultFocus() {
    int32_t maxFocus = std::numeric_limits<int32_t>::min();
    Tile* const focus = this->ScanForMaxFocus(maxFocus, nullptr);
    if (focus) {
        this->SetCurrentFocusTarget(focus, Tile::kTileValue_Mouseover, true);
        if (!InterfaceManager::Has360Controller() || this->bShowMouse) {
            this->pCursor->GetModel()->SetAppCulled(true);
            this->pCursor->Set(Tile::kTileValue_Visible, 0);
        }
        this->bMouseInMotion = false;
    }
    else {
        this->SetCurrentFocusTarget(nullptr, Tile::kTileValue_Mouseover, true);
    }
}

Tile* InterfaceManager::ScanForMaxFocus(int32_t& aiMaxFocus, Tile* apTarget) {
    Tile* found = nullptr;
    int32_t maxFocus = aiMaxFocus;
    // Among equal focus values the lowest list index wins.
    int32_t minListIndex = std::numeric_limits<int32_t>::max();
    aiMaxFocus = std::numeric_limits<int32_t>::min();
    int32_t focus = std::numeric_limits<int32_t>::min();
    if (!apTarget) {
        Menu* const frontMenu = MenuManager::Instance(true)->GetFrontmostMenu();
        if (!frontMenu) {
            return nullptr;
        }
        apTarget = frontMenu->GetMenuTile();
        // A menu tile whose menu fade state is neither 1 nor 8 gives no focus.
        if (apTarget->GetType() == Tile::kTileValue_Menu &&
            apTarget->GetMenu()->GetFadeState() != 1 &&
            apTarget->GetMenu()->GetFadeState() != 8) {
            return nullptr;
        }
    }
    if (!apTarget->IsTrue(Tile::kTileValue_Visible)) {
        return nullptr;
    }
    NiTListItem<Tile*>* pos = apTarget->xChildren.GetHeadPos();
    while (pos) {
        Tile* const child = apTarget->xChildren.GetNext(pos);
        focus = std::numeric_limits<int32_t>::min();
        Tile* const childFound = this->ScanForMaxFocus(focus, child);
        if (childFound && focus > maxFocus) {
            maxFocus = focus;
            found = childFound;
        }
        else if (childFound && focus == maxFocus) {
            Tile::Value* const listIndex = childFound->GetValue(Tile::kTileValue_ListIndex);
            if (listIndex) {
                const int32_t index = static_cast<int32_t>(listIndex->QFloat());
                if (index < minListIndex) {
                    minListIndex = index;
                    found = childFound;
                }
            }
        }
    }
    if (apTarget->IsTrue(Tile::kTileValue_Target) && apTarget->IsTrue(Tile::kTileValue_Visible)) {
        Tile::Value* const xdefault = apTarget->GetValueQ(Tile::kTileValue_xdefault);
        if (xdefault) {
            focus = static_cast<int32_t>(xdefault->QFloat());
            if (focus > maxFocus) {
                maxFocus = focus;
                found = apTarget;
            }
            else if (focus == maxFocus) {
                Tile::Value* const listIndex = apTarget->GetValue(Tile::kTileValue_ListIndex);
                if (listIndex) {
                    const int32_t index = static_cast<int32_t>(listIndex->QFloat());
                    if (index < minListIndex) {
                        minListIndex = index;
                        found = apTarget;
                    }
                }
            }
        }
    }
    aiMaxFocus = maxFocus;
    return found;
}

void InterfaceManager::ClearReferencePointers(TESObjectREFR* apRef) {
    if (this->pPickRef == apRef) {
        this->pPickRef = nullptr;
    }
    if (this->pReticleRef == apRef) {
        this->pReticleRef = nullptr;
    }
    if (this->pCrossHairRef == apRef) {
        this->pCrossHairRef = nullptr;
    }
    if (this->pActivateRef == apRef) {
        this->pActivateRef = nullptr;
    }
    if (this->pTelekinesisRef == apRef) {
        this->pTelekinesisRef = nullptr;
    }
    HUDMainMenu::ClearLastObjectIfItIs(apRef);
}

void InterfaceManager::UpdateAllTimers() {
    InterfaceManager::Timer* timer = this->pTimers->pNext;
    while (timer) {
        // During VATS playback the fades run in real time.
        if (pVATS.GetMode() == VATS::VATS_PLAYBACK) {
            const double seconds = appTimer.GetSeconds();
            timer->fElapsed = seconds / BSTimer::QGlobalTimeMultiplier() + timer->fElapsed;
        }
        else {
            timer->fElapsed = appTimer.GetSeconds() + timer->fElapsed;
        }
        const float ratio = timer->fElapsed / timer->fEnd;
        if (BSMAX(0.0f, BSMIN(1.0f, ratio)) == 1.0) {
            InterfaceManager::Timer* const next = timer->pNext;
            timer->pPrev->pNext = next;
            if (next) {
                next->pPrev = timer->pPrev;
            }
            else {
                InterfaceManager::Instance()->pTimers->pPrev = timer->pPrev;
            }
            ourFree(timer);
            timer = next;
        }
        else {
            timer = timer->pNext;
        }
    }
}

void InterfaceManager::PreLoadMainMenus() {
    if (!this->bPreLoadMainMenus) {
        return;
    }
    Tile* menu = Interface::CreateStatsMenu();
    if (menu) {
        menu->Set(Tile::kTileValue_Visible, 0);
    }
    menu = Interface::CreateInventoryMenu();
    if (menu) {
        menu->Set(Tile::kTileValue_Visible, 0);
    }
    menu = Interface::CreateRepairMenu();
    if (menu) {
        menu->Set(Tile::kTileValue_Visible, 0);
    }
    // Interface::CreateItemModMenu returns the item mod menu's tile, declared there as an int.
    menu = reinterpret_cast<Tile*>(Interface::CreateItemModMenu());
    if (menu) {
        menu->Set(Tile::kTileValue_Visible, 0);
    }
    menu = Interface::CreateMapMenu();
    if (menu) {
        menu->Set(Tile::kTileValue_Visible, 0);
    }
    Interface::SetMainMenuDefaults(0, 0, 4, Interface::eStatsMenu);
    Interface::HideMenus();
    this->bPreLoadMainMenus = false;
}

void InterfaceManager::DoEnter(Menu* apMenu, int32_t aiID, Tile* apTarget) {
    // The first active list box that takes the tile highlights it.
    for (BSSimpleList<ListBoxBase*>* node = &InterfaceManager::xActiveListBoxes; node; node = node->GetNext()) {
        if (!node->GetItem()) {
            break;
        }
        if (node->GetItem()->Highlight(apTarget)) {
            break;
        }
    }
    apMenu->DoEnter(aiID, apTarget);
}

void InterfaceManager::DoLeave(Menu* apMenu, int32_t aiID, Tile* apTarget) {
    for (BSSimpleList<ListBoxBase*>* pNode = &InterfaceManager::xActiveListBoxes; pNode; pNode = pNode->m_pkNext) {
        if (!pNode->m_item) {
            break;
        }
        ListBoxBase* pListBox = pNode->m_item;
        if (pListBox->GetHighlightedItem() == apTarget) {
            if (pNode->m_item->Highlight(nullptr)) {
                break;
            }
        }
    }
    apMenu->DoLeave(aiID, apTarget);
}

bool InterfaceManager::DoGamepad(Menu* apMenu, Interface::enumGamepadButtons aiButton, float afValue) {
    if (apMenu->DoGamepad(aiButton, afValue)) {
        return true;
    }
    for (BSSimpleList<ListBoxBase*>* node = &InterfaceManager::xActiveListBoxes; node; node = node->GetNext()) {
        ListBoxBase* const listBox = node->GetItem();
        if (!listBox) {
            break;
        }
        if (listBox->BelongsTo(apMenu) &&
            Interface::IsTopMenuID(static_cast<Interface::enumMenus>(apMenu->GetClass())) &&
            listBox->GetHighlightedItem()) {
            Tile* const target = listBox->GetHighlightTarget(aiButton);
            if (target) {
                this->SetCurrentFocusTarget(target, Tile::kTileValue_Mouseover, true);
                listBox->ScrollToHighlight();
                return true;
            }
        }
    }
    return false;
}

void InterfaceManager::EnterRenderedMenu(FORenderedMenu* apRenderedMenu) {
    if (apRenderedMenu) {
        if (!apRenderedMenu->GetInitialized()) {
            apRenderedMenu->Initialize();
        }
        apRenderedMenu->OnMenuOpen();
        this->bIsInRenderedMenu = true;
        if (apRenderedMenu->pMenuTile) {
            apRenderedMenu->pMenuTile->Set(Tile::kTileValue_Visible, 0);
        }
    }
    else {
        if (this->pCurrentRenderedMenu) {
            this->pCurrentRenderedMenu->SetScreenTexture(nullptr);
            this->pCurrentRenderedMenu->OnMenuClose();
            if (this->pCurrentRenderedMenu != this->GetPipboy()) {
                QueueRenderedMenuDelete(this->pCurrentRenderedMenu);
            }
        }
        this->bIsInRenderedMenu = false;
    }
    this->pCurrentRenderedMenu = apRenderedMenu;
}

TESImageSpaceModifier* InterfaceManager::GetCurrentImagespaceMod() {
    TESImageSpaceModifier* imagespaceMod = nullptr;
    const Interface::enumMenus topMenu = Interface::GetTopMenuID();
    if (StartMenu::PauseMenuIsActive()) {
        imagespaceMod = static_cast<TESImageSpaceModifier*>(TESForm::GetFormByNumericID(0x4EEE8));
    }
    else if (pPlayer->IsPipboyActive()) {
        imagespaceMod = static_cast<TESImageSpaceModifier*>(TESForm::GetFormByNumericID(0x96389));
    }
    else if (topMenu == 0x3F6) {
        imagespaceMod = static_cast<TESImageSpaceModifier*>(TESForm::GetFormByNumericID(0x44F34));
    }
    else if (!InterfaceManager::Instance()->GetMenusRoot()->GetChildByName("Player Name Entry Menu")) {
        imagespaceMod = static_cast<TESImageSpaceModifier*>(TESForm::GetFormByNumericID(0x32B38));
    }
    return imagespaceMod;
}

void InterfaceManager::SetRepeatingKey(int32_t aiKey) {
    uKeyDownTime = appTimer.BSTimer::GetTime();
    uKeyRepeatStartTime = 0;
    iRepeatingKey = aiKey;
}

// AutoMemContext as in UpdateCursor; the fog pointer is released before the context.
void InterfaceManager::Init(bool abInitialLoad) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\InterfaceManager.cpp",
        538
    );
    if (abInitialLoad) {
        Tile::LoadTextTable();
    }

    this->pMenusRoot = new TileRect;
    this->pMenusRoot->Init(nullptr, "MenuRoot", nullptr);
    this->pMenusRoot->Set(Tile::kTileValue_Locus, 1);
    // 6001 is a trait id outside the named traits.
    this->pMenusRoot->Set(6001, 1003);
    this->pMenusRoot->Set(Tile::kTileValue_Alpha, 0.0f, true);
    this->pMenusRoot->Set(Tile::kTileValue_Width, InterfaceManager::GetScreenWidth(), true);
    this->pMenusRoot->Set(Tile::kTileValue_Height, InterfaceManager::GetScreenHeight(), true);
    this->pMenusRoot->Set(Tile::kTileValue_CropOffsetX, InterfaceManager::GetScreenSafeX(), true);
    this->pMenusRoot->Set(Tile::kTileValue_CropOffsetY, InterfaceManager::GetScreenSafeY(), true);
    this->pMenusRoot
        ->Set(Tile::kTileValue_ResolutionConverter, Interface::SizeOfAScreenPixelInCrazyAssMenuCoordinates(), true);
    Tile::AddDirtyTile(this->pMenusRoot);
    Tile::UpdateAll(false);

    SystemColorManager::GetInstance();
    this->pStringRoot = this->pMenusRoot->ReadFile("Data\\Menus\\globals.xml");
    Menu::RemoveFromXMLArchive("Data\\Menus\\globals.xml", nullptr);
    this->pStringRoot->SetParent(nullptr, nullptr);
    // The product is stored as a float before it is passed.
    const float fFillAlpha = static_cast<float>(fMenuBackgroundOpacity.Float() * 255.0);
    this->pStringRoot->Set(Tile::TextToTrait("_background_fill_alpha"), fFillAlpha, true);
    const bool bHas360 = InterfaceManager::Has360Controller();
    this->pStringRoot->Set(Tile::TextToTrait("_Has360Controller"), static_cast<int32_t>(bHas360));

    this->pDebugTextRoot = new NiNode(0);
    {
        // The temporary name is destroyed after SetName, dropping its reference.
        NiFixedString kName("InterfaceManager: DebugText Root");
        this->pDebugTextRoot->SetName(kName);
    }
    this->pDebugTextRoot->SetAppCulled(false);
    this->pInterfaceRoot->AttachChild(this->pDebugTextRoot, true);

    SceneGraph* const pSceneGraph = this->GetSceneGraph();
    pSceneGraph->UpdateProperties();
    NiUpdateData kUpdateData(0.0f, false, false);
    pSceneGraph->Update(kUpdateData);

    // The width is kept as a double, the quotient is stored as a float.
    const double dWidth = InterfaceManager::GetRealScreenWidth();
    const float fAspect = static_cast<float>(dWidth / InterfaceManager::GetRealScreenHeight());
    if (fAspect == 1.600000023841858 || 1.0 / fAspect == 0.6000000238418579) {
        iDebugTextLeftRightOffset = 80;
        iConsoleTextXPos = 80;
    }

    {
        alignas(NiPointer<BSFogProperty>) uint8_t aFogStorage[sizeof(NiPointer<BSFogProperty>)];
        NiPointer<BSFogProperty>& spFog = *::new (aFogStorage) NiPointer<BSFogProperty>(new BSFogProperty);
        spFog->SetDistance(0.0f, 0.0f);
        // The 3D scene graph is fetched and not used.
        this->Get3DSceneGraph();
        this->SetDebugTextVisible(false);

        // FOPipboyManager is 0x170 bytes.
        this->pPipboy = new FOPipboyManager;
        this->bFirstInit = true;
        if (abInitialLoad) {
            Interface::CreateLoadingMenu(nullptr, false);
        }
        spFog.~NiPointer<BSFogProperty>();
    }
    pContext->~AutoMemContext();
}

// AutoMemContext as in UpdateCursor.
void InterfaceManager::InitSecond() {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\InterfaceManager.cpp",
        667
    );
    FontManager::Init();

    TileImage* const pCursorTile = new TileImage;
    pCursorTile->Init(nullptr, "Cursor", nullptr);
    pCursorTile->Set(Tile::kTileValue_Depth, 2000);
    pCursorTile->Set(Tile::kTileValue_Width, 32);
    pCursorTile->Set(Tile::kTileValue_Height, 32);
    pCursorTile->Set(Tile::kTileValue_Filename, sCursorFilename.String(), true);
    pCursorTile->Set(Tile::kTileValue_Texatlas, sDefaultTexAtlas.String(), true);
    pCursorTile->Set(Tile::kTileValue_X, 0);
    pCursorTile->Set(Tile::kTileValue_Y, 0);
    pCursorTile->Set(Tile::kTileValue_Visible, 0);
    pCursorTile->Set(Tile::kTileValue_Alpha, 255);
    pCursorTile->Set(Tile::kTileValue_Red, 255);
    pCursorTile->Set(Tile::kTileValue_Green, 255);
    pCursorTile->Set(Tile::kTileValue_Blue, 255);
    pCursorTile->UpdateTile(true);
    this->pCursorRoot->AttachChild(pCursorTile->GetModel(), true);
    pCursorTile->GetModel()->SetTranslate(0.0f, -1.0f, 0.0f);
    pCursorTile->GetModel()->UpdateProperties();
    NiUpdateData kUpdateData(0.0f, false, false);
    pCursorTile->GetModel()->Update(kUpdateData);
    this->pCursor = pCursorTile;
    this->UpdateCursor();
    this->pCursor->GetModel()->SetAppCulled(true);

    // ShadowSceneNode is 0x200 bytes.
    this->pShadowNode = new ShadowSceneNode;
    this->pShadowNode->SetSceneGraphIndex(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D);
    this->pUIPlayerNode = new ShadowSceneNode;
    this->pUIPlayerNode->SetSceneGraphIndex(BSShaderManager::BSSM_SSN_MENU_PLAYER_3D);
    this->spInterfaceAccum->SetShadowSceneNode(this->pShadowNode);
    this->sp3dInterfaceAccum->SetShadowSceneNode(this->pShadowNode);

    Tile* const pHUD = HUDMainMenu::Create();
    HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_START);
    if (pHUD) {
        pHUD->Set(Tile::kTileValue_Visible, 0);
    }
    this->pCursor->GetModel()->SetAppCulled(true);
    this->pCursor->Set(Tile::kTileValue_Visible, 0);
    this->pCursor->UpdateTile(true);
    this->bSecondInit = true;
    pContext->~AutoMemContext();
}

// AutoMemContext as in UpdateCursor.
void InterfaceManager::PreIdleStuff() {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\InterfaceManager.cpp",
        826
    );
    Controls* const pControls = appMain->GetControls();
    this->UpdateAllTimers();
    if (this->cMenuMode == 3 && !this->bLockMenuModeForFade) {
        pControls->ClearKeystrokes();
        this->cMenuMode = eMenuModeFirstPass;
        if (!InterfaceManager::Has360Controller() || this->bShowMouse) {
            this->pCursor->GetModel()->SetAppCulled(false);
            this->pCursor->Set(Tile::kTileValue_Visible, 1);
        }
        this->pCursor->Set(Tile::kTileValue_Filename, sCursorFilename.String(), true);
        this->pCursor->UpdateTile(true);
        HUDMainMenu::SetInfoForRef(nullptr, false, false);
        this->pMouseOverTarget = nullptr;
        if (InterfaceManager::Has360Controller() && !this->bShowMouse) {
            this->bMouseInMotion = false;
        }
        MenuConsole* const pConsole = MenuConsole::Instance(false);
        if (pConsole) {
            pConsole->OnEnterMenuMode();
        }
        ++InterfaceManager::Instance()->iSoundPauses;
        BSAudio::QInstance().PauseAnimationSounds();
        ++InterfaceManager::Instance()->iVoicePauses;
        BSAudio::QInstance().PauseWorldVoices();
    }
    else if (this->cMenuMode == 5) {
        this->cMenuMode = eMenuModeOn;
    }

    if (this->cMenuMode == 1) {
        if (this->IsInMenuMode()) {
            if (!this->bExternalForcedMenuMode) {
                this->bExternalForcedMenuMode = true;
                ++InterfaceManager::Instance()->iSoundPauses;
                BSAudio::QInstance().PauseAnimationSounds();
                ++InterfaceManager::Instance()->iVoicePauses;
                BSAudio::QInstance().PauseWorldVoices();
            }
        }
        else if (this->bExternalForcedMenuMode) {
            this->bExternalForcedMenuMode = false;
            if (InterfaceManager::Instance()->iVoicePauses) {
                --InterfaceManager::Instance()->iVoicePauses;
                BSAudio::QInstance().UnPauseWorldVoices();
            }
            if (InterfaceManager::Instance()->iSoundPauses) {
                --InterfaceManager::Instance()->iSoundPauses;
                BSAudio::QInstance().UnPauseAnimationSounds();
            }
        }
    }
    pContext->~AutoMemContext();
}

// The AutoMemContext and the sound handles are not destroyed if a call throws; the explicit Construct/Destruct calls
// are made where the game makes them. Arguments with calls are evaluated into locals first, in the game's order.
// Intermediates are computed in double.
void InterfaceManager::Idle() {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = static_cast<AutoMemContext*>(static_cast<void*>(aucContext));
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\InterfaceManager.cpp",
        1146
    );

    lastControllerState = pState;
    InterfaceManager::bHas360Controller = XInputGetState(0, &pState) == 0;
    if (!(uiLastControllerStateGuard & 1)) {
        uiLastControllerStateGuard |= 1;
        bLastControllerState = InterfaceManager::bHas360Controller;
    }
    if (bLastControllerState != InterfaceManager::bHas360Controller) {
        if (bLastControllerState) {
            const char* pOk = sOk.String();
            const char* pText = sLostController.String();
            Interface::CreateMessageMenu(
                pText,
                nullptr,
                nullptr,
                nullptr,
                0,
                0x17,
                0.0f,
                0.0f,
                pOk,
                static_cast<const char*>(nullptr)
            );
        }
        else {
            const char* pOk = sOk.String();
            const char* pText = sControllerOption.String();
            Interface::CreateMessageMenu(
                pText,
                nullptr,
                nullptr,
                nullptr,
                0,
                0x17,
                0.0f,
                0.0f,
                pOk,
                static_cast<const char*>(nullptr)
            );
        }
        this->HandleControllerStateChange(InterfaceManager::bHas360Controller);
        bLastControllerState = InterfaceManager::bHas360Controller;
    }
    if (this->bPreLoadMainMenus) {
        this->PreLoadMainMenus();
    }

    MenuConsole* pConsole = nullptr;
    Controls* pControls = appMain->GetControls();
    // The x and y mouse axes (1, 2) are polled and not used.
    pControls->GetMouseAxis(1);
    pControls->GetMouseAxis(2);
    if (InterfaceManager::Has360Controller()) {
        if (Interface::IsConsoleVisible()) {
            this->bShowMouse = true;
            this->pCursor->GetModel()->SetAppCulled(false);
            this->pCursor->Set(Tile::kTileValue_Visible, 1);
        }
        else {
            this->bShowMouse = false;
            this->pCursor->GetModel()->SetAppCulled(true);
            this->pCursor->Set(Tile::kTileValue_Visible, 0);
        }
    }
    // Menu mode 1 is the game itself.
    else if (this->cMenuMode != 1) {
        this->bShowMouse = true;
        this->pCursor->GetModel()->SetAppCulled(false);
        this->pCursor->Set(Tile::kTileValue_Visible, 1);
    }

    // Whether the pip-boy button must not open or close the pip-boy this frame.
    bool bPipboyBlocked = true;
    if (InterfaceManager::IsPipboyMenuTopmost() &&
        (!InterfaceManager::Has360Controller() ||
         (!Interface::IsTopMenuID(Interface::ePipBoyRepair) && !Interface::IsTopMenuID(Interface::eItemModMenu))) &&
        (!Interface::IsTopMenuID(Interface::eInventoryMenu) || !InventoryMenu::IsKeyringOpen()) &&
        (!StatsMenu::IsTargetingBodyPart() || !InterfaceManager::bHas360Controller) &&
        !Menu::GetMenuByClass(StartMenu::GetClassStatic())) {
        bPipboyBlocked = false;
    }

    if (this->pCurrentRenderedMenu) {
        this->pCurrentRenderedMenu->OnIdle();
    }
    if (this->GetPipboy()->QPipboyLightFading()) {
        float fTime = static_cast<float>(static_cast<uint64_t>(appTimer.GetTime()));
        this->GetPipboy()->UpdateLightEffect(fTime);
    }
    this->HUDEffects.OnIdle();
    this->VATSEffect.OnIdle();

    if (clast_menu_mode != this->cMenuMode) {
        if (this->cMenuMode == 1) {
            while (InterfaceManager::Instance()->iVoicePauses) {
                uint8_t* pPauses = &InterfaceManager::Instance()->iVoicePauses;
                --*pPauses;
                BSAudio::QInstance().UnPauseWorldVoices();
            }
            while (InterfaceManager::Instance()->iSoundPauses) {
                uint8_t* pPauses = &InterfaceManager::Instance()->iSoundPauses;
                --*pPauses;
                BSAudio::QInstance().UnPauseAnimationSounds();
            }
            if (pPlayer->IsDead(false)) {
                HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_VATS);
            }
            else if (pPlayer->GetSitSleepState()) {
                HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_CHANGED);
            }
            else if (HUDMainMenu::GetMenuMode() != HUDMainMenu::HMM_MOVIE) {
                if (pPlayer->GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT)) {
                    HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_MOVEMENT_DISABLED);
                }
                else if (pPlayer->IsVanityMode()) {
                    HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_VANITY);
                }
                else if (pPlayer->GetIronSights() && !pVATS.GetMode()) {
                    HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_IRON_SIGHTS);
                }
                else if (pVATS.GetMode() == VATS::VATS_PLAYBACK) {
                    HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_VATS_PLAYBACK);
                }
                else {
                    HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_RUN_AND_GUN);
                }
            }
        }
        else if (this->cMenuMode == 5) {
            switch (this->GetEnterStackTop()) {
                case Interface::eMainFour:
                // A menu class with no enumMenus name.
                case 0x13:
                case Interface::eInventoryMenu:
                case Interface::eStatsMenu:
                case Interface::eLockPickMenu:
                case Interface::ePipBoyData:
                case Interface::ePipBoyRepair:
                case Interface::eItemModMenu: HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_PIPBOY); break;
                case Interface::eMessageMenu: HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_MESSAGE_MENU); break;
                case Interface::eLoadingMenu: HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_LOAD); break;
                case Interface::eContainerMenu: HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_CONTAINER); break;
                case Interface::eDialogMenu: HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_DIALOGUE); break;
                case Interface::eLevelUpMenu:
                case Interface::eCharGenMenu: HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_CHARGEN_MENU); break;
                case Interface::eHackingMenu: HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_HACKING); break;
                case Interface::eVATSMenu: HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_VATS); break;
                case Interface::eComputersMenu: HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_TERMINAL); break;
                case Interface::eSPECIALBookMenu: HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_SPECIAL_BOOK); break;
                case Interface::eLoveTesterMenu: HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_LOVE_TESTER); break;
                case Interface::eSlotMachineMenu:
                case Interface::eBlackJackMenu:
                case Interface::eRouletteMenu:
                case Interface::eCaravanMenu: HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_CASINO_GAME); break;
                default: HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_START); break;
            }
        }
        clast_menu_mode = this->cMenuMode;
    }

    if (InterfaceManager::QLockpickControlsDisable() &&
        !appMain->GetControls()->GetUserAction(Controls::UACTN_SLDLEFT, Controls::isHeld) &&
        !appMain->GetControls()->GetUserAction(Controls::UACTN_SLDRIGHT, Controls::isHeld) &&
        !appMain->GetControls()->GetUserAction(Controls::UACTN_FORWARD, Controls::isHeld) &&
        !appMain->GetControls()->GetUserAction(Controls::UACTN_BACK, Controls::isHeld)) {
        this->SetLockpickControlsDisable(false);
    }

    if (this->cMenuMode == 1) {
        // The HUD menu is created when missing; the result is not used further.
        if (!Menu::GetMenuByClass(Interface::eHUDMainMenu)) {
            HUDMainMenu::Create();
        }
        if (Interface::bIsDroppingInInventory) {
            const char* pFilename = sCursorFilename.String();
            this->pCursor->Set(Tile::kTileValue_Filename, pFilename, true);
            this->pCursor->UpdateTile(true);
            Interface::bIsDroppingInInventory = false;
        }
        if (Interface::bNeedLevelUpMenu) {
            LevelUpMenu::CreateMenu();
            Interface::bNeedLevelUpMenu = false;
        }
        pPlayer->CastEatDrinkItems();
        pPlayer->CastQueuedEnchantments();
    }

    // Menu modes 2 and 5: a menu has the input.
    if (this->cMenuMode == 2 || this->cMenuMode == 5) {
        if (InterfaceManager::Has360Controller() && !this->bShowMouse) {
            if (!Interface::IsConsoleVisible()) {
                this->bMouseInMotion = false;
            }
        }
        else if (Interface::IsInGameLoadingMenuOpen() && !StartMenu::MainMenuIsActive()) {
            this->pCursor->GetModel()->SetAppCulled(true);
            this->pCursor->Set(Tile::kTileValue_Visible, 0);
        }
        else {
            this->pCursor->GetModel()->SetAppCulled(false);
            this->pCursor->Set(Tile::kTileValue_Visible, 1);
        }
        this->UpdateCursor();
        if (!this->bMouseInMotion &&
            (!this->GetCurrentFocusTarget() ||
             !this->GetCurrentFocusTarget()->IsVisible() ||
             !this->GetCurrentFocusTarget()->IsTrue(Tile::kTileValue_Target))) {
            this->GetDefaultFocus();
        }

        // The left button (or the 360 A button for the console): pressed, released and held this frame.
        int32_t iPressed = 0;
        int32_t iReleased = 0;
        int32_t iHeld = 0;
        if (!InterfaceManager::Has360Controller() || this->bShowMouse) {
            iPressed = pControls->GetMouseButton(0, Controls::isPressed);
            iReleased = pControls->GetMouseButton(0, Controls::isDepressed);
            iHeld = pControls->GetMouseButton(0, Controls::isHeld);
            if (iHeld) {
                this->fMouseHeldTime = appTimer.GetSeconds() + this->fMouseHeldTime;
            }
            else {
                this->fMouseHeldTime = 0.0f;
            }
        }
        else if (MenuConsole::Instance(true)->IsVisible()) {
            iPressed = pControls->GetUserAction(Controls::UACTN_ACTIVATE, Controls::isPressed);
            iReleased = pControls->GetUserAction(Controls::UACTN_ACTIVATE, Controls::isDepressed);
            iHeld = pControls->GetUserAction(Controls::UACTN_ACTIVATE, Controls::isHeld);
            if (iHeld) {
                this->fMouseHeldTime = appTimer.GetSeconds() + this->fMouseHeldTime;
            }
            else {
                this->fMouseHeldTime = 0.0f;
            }
        }
        // Axis 3 is the mouse wheel.
        this->fMouseWheel = static_cast<float>(pControls->GetMouseAxis(3));

        if (InterfaceManager::Has360Controller() || (!InterfaceManager::Has360Controller() && this->bMouseInMotion)) {
            Tile* pPicked = this->PickMenus(false);
            Menu* pPickedMenu = nullptr;
            if (pPicked) {
                pPickedMenu = pPicked->GetMenu();
                if ((!InterfaceManager::Has360Controller() || this->bShowMouse) && this->pMouseOverTarget) {
                    this->pMouseOverTarget->Set(Tile::kTileValue_Mouseover, 0);
                    Tile* pOverTarget = this->pOverTileTarget;
                    int32_t iID = static_cast<int32_t>(this->pMouseOverTarget->GetFloat(Tile::kTileValue_ID));
                    Menu* pMenu = this->pMouseOverTarget->GetMenu();
                    this->DoLeave(pMenu, iID, pOverTarget);
                    this->pMouseOverTarget = nullptr;
                }
            }
            if ((!InterfaceManager::Has360Controller() || this->bShowMouse) && this->pDragTarget) {
                float fY = this->sRealCursorPos.z / InterfaceManager::GetScreenAspect();
                float fX = this->sRealCursorPos.x / InterfaceManager::GetScreenAspect();
                NiPoint2 kCursor;
                ::new (&kCursor) NiPoint2(fX, fY);
                if (InterfaceManager::IsPipboyMenuTopmost()) {
                    // The quotient stays on the x87 stack; 195.0 is a double constant.
                    kCursor.x = static_cast<float>(static_cast<double>(kCursor.x) / GetMenuHeightRatio() - 195.0);
                    kCursor.y = static_cast<float>(static_cast<double>(kCursor.y) / GetMenuHeightRatio() - 195.0);
                }
                float fStartX = this->pDragTarget->GetFloat(Tile::kTileValue_DragStartX);
                this->SetCursorOffsetX(static_cast<int32_t>(static_cast<double>(kCursor.x) - fStartX));
                float fStartY = this->pDragTarget->GetFloat(Tile::kTileValue_DragStartY);
                this->SetCursorOffsetY(static_cast<int32_t>(static_cast<double>(kCursor.y) - fStartY));
                // GetRealX's result is spilled as a double across the GetFloat call.
                double dRealX = this->pDragTarget->GetRealX();
                float fTileX = this->pDragTarget->GetFloat(Tile::kTileValue_X);
                float fDragX = static_cast<float>(static_cast<double>(kCursor.x) - (dRealX - fTileX));
                this->pDragTarget->Set(Tile::kTileValue_DragX, fDragX, true);
                double dRealY = this->pDragTarget->GetRealY();
                float fTileY = this->pDragTarget->GetFloat(Tile::kTileValue_Y);
                float fDragY = static_cast<float>(static_cast<double>(kCursor.y) - (dRealY - fTileY));
                this->pDragTarget->Set(Tile::kTileValue_DragY, fDragY, true);
            }
            if (pPicked != this->pOverTileTarget && iHeld) {
                // The menu fade state 1: the menu is shown.
                if (this->pDragOverTileMenu && this->pDragOverTileMenu->GetFadeState() == 1) {
                    Tile* pOverTarget = this->pOverTileTarget;
                    Tile* pDragOverTarget = this->pDragOverTileTarget;
                    int32_t iID = static_cast<int32_t>(this->pDragOverTileTarget->GetFloat(Tile::kTileValue_ID));
                    this->pDragOverTileMenu->DoDragLeave(iID, pDragOverTarget, pOverTarget);
                }
                this->pDragOverTileTarget = pPicked;
                this->pDragOverTileMenu = pPickedMenu;
                if (this->pDragOverTileMenu && this->pDragOverTileMenu->GetFadeState() == 1) {
                    Tile* pOverTarget = this->pOverTileTarget;
                    Tile* pDragOverTarget = this->pDragOverTileTarget;
                    int32_t iID = static_cast<int32_t>(this->pDragOverTileTarget->GetFloat(Tile::kTileValue_ID));
                    this->pDragOverTileMenu->DoDragEnter(iID, pDragOverTarget, pOverTarget);
                }
            }
            if (iReleased) {
                if (this->pOverTileMenu &&
                    this->pDragOverTileTarget == this->pOverTileTarget &&
                    this->pOverTileTarget->GetValue(Tile::kTileValue_ID) &&
                    this->pOverTileMenu->GetFadeState() == 1) {
                    this->pOverTileTarget->PlayTileSound(Tile::kTileValue_ClickSound);
                    Tile* pTarget = this->pOverTileTarget;
                    Menu* pMenu = this->pOverTileMenu;
                    if (pTarget) {
                        pTarget->Set(Tile::kTileValue_Clicked, 1);
                        pTarget->Set(Tile::kTileValue_Clicked, 0);
                        if (pMenu) {
                            pMenu->DoClick(static_cast<int32_t>(pTarget->GetFloat(Tile::kTileValue_ID)), pTarget);
                        }
                    }
                    if (!this->pOverTileTarget) {
                        pPicked = nullptr;
                        pPickedMenu = nullptr;
                    }
                }
                this->pDragOverTileTarget = nullptr;
                this->pDragOverTileMenu = nullptr;
                if (this->pDragTarget) {
                    this->pDragTarget->Set(Tile::kTileValue_DragStartX, -1);
                    this->pDragTarget->Set(Tile::kTileValue_DragStartY, -1);
                    this->pDragTarget = nullptr;
                }
            }
            if (pPicked != this->pOverTileTarget &&
                !iHeld &&
                (!InterfaceManager::Has360Controller() || this->bShowMouse)) {
                if (this->pOverTileMenu && this->pOverTileMenu->GetFadeState() == 1) {
                    this->pOverTileTarget->Set(Tile::kTileValue_Mouseover, 0);
                    if (this->pOverTileTarget->xChildren.FindPos(pPicked)) {
                        Tile* pTarget = this->pOverTileTarget;
                        int32_t iID = static_cast<int32_t>(this->pOverTileTarget->GetFloat(Tile::kTileValue_ID));
                        this->pOverTileMenu->DoLeave(iID, pTarget);
                    }
                    else {
                        Tile* pTarget = this->pOverTileTarget;
                        int32_t iID = static_cast<int32_t>(this->pOverTileTarget->GetFloat(Tile::kTileValue_ID));
                        this->DoLeave(this->pOverTileMenu, iID, pTarget);
                    }
                }
                if (pPickedMenu && pPickedMenu->GetFadeState() == 1) {
                    this->pOverTileTarget = pPicked;
                    this->pOverTileMenu = pPickedMenu;
                }
                else {
                    this->pOverTileTarget = nullptr;
                    this->pOverTileMenu = nullptr;
                }
                if (this->pOverTileMenu && this->pOverTileMenu->GetFadeState() == 1) {
                    if (!InterfaceManager::Has360Controller() || this->bShowMouse) {
                        this->pMouseOverTarget = this->pOverTileTarget;
                    }
                    this->pOverTileTarget->PlayTileSound(Tile::kTileValue_MouseoverSound);
                    this->pOverTileTarget->Set(Tile::kTileValue_Mouseover, 1);
                    Tile* pTarget = this->pOverTileTarget;
                    int32_t iID = static_cast<int32_t>(this->pOverTileTarget->GetFloat(Tile::kTileValue_ID));
                    this->DoEnter(this->pOverTileMenu, iID, pTarget);
                }
                if (this->bMouseInMotion) {
                    this->ClearCurrentFocusTarget();
                }
            }
        }

        if (!InterfaceManager::Has360Controller() || this->bShowMouse) {
            if (!this->GetCurrentRenderedMenu()) {
                this->bOverPipboyScreen = false;
            }
            FORenderedMenu* pRendered = this->GetCurrentRenderedMenu();
            if (pRendered != this->GetPipboy()) {
                bool bOver = this->bMouseInMotion &&
                    this->pCurrentRenderedMenu &&
                    !StartMenu::PauseMenuIsActive() &&
                    !Interface::IsTopMenuID(Interface::eMessageMenu) &&
                    !Interface::IsTopMenuID(Interface::eTextEditMenu) &&
                    !Interface::IsTopMenuID(Interface::eTutorialMenu) &&
                    this->pCurrentRenderedMenu->Pick(iReleased > 0, &this->fPipboyMouseX, &this->fPipboyMouseY);
                this->bMouseOverRenderedMenu = bOver;
            }
            else {
                bool bOver = this->bMouseInMotion &&
                    this->pCurrentRenderedMenu &&
                    !StartMenu::PauseMenuIsActive() &&
                    !Interface::IsTopMenuID(Interface::eMessageMenu) &&
                    !Interface::IsTopMenuID(Interface::eTextEditMenu) &&
                    this->bOverPipboyScreen;
                this->bMouseOverRenderedMenu = bOver;
                if (this->iPipboyMenuToSet > 0) {
                    BSSoundHandle kSound;
                    kSound = BSAudio::QInstance().GetSoundHandleByName(
                        "UIMenuMode",
                        BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                            BSGameSound::SOUND_FLAG_ONE_SHOT
                    );
                    switch (this->iPipboyMenuToSet) {
                        case 1:
                            kSound.Play(false);
                            Interface::SetStatsMenuVisible(true, false);
                            break;
                        case 2:
                            kSound.Play(false);
                            Interface::SetInventoryMenuVisible(true, false, true);
                            break;
                        case 3:
                            kSound.Play(false);
                            Interface::SetMapMenuVisible(true, false);
                            break;
                        default: kSound.Release(); break;
                    }
                }
            }
        }

        if (this->bMouseInMotion && iPressed) {
            if (this->pOverTileTarget && this->pOverTileMenu && this->pOverTileMenu->GetFadeState() == 1) {
                this->pDragOverTileTarget = this->pOverTileTarget;
                this->pDragOverTileMenu = this->pOverTileMenu;
                Tile* pTarget = this->pOverTileTarget;
                int32_t iID = static_cast<int32_t>(this->pOverTileTarget->GetFloat(Tile::kTileValue_ID));
                this->pOverTileMenu->DoDownClick(iID, pTarget);
                if (this->pOverTileTarget && this->pOverTileTarget->IsTrue(Tile::kTileValue_Draggable)) {
                    this->pDragTarget = this->pOverTileTarget;
                    float fY = this->sRealCursorPos.z / InterfaceManager::GetScreenAspect();
                    float fX = this->sRealCursorPos.x / InterfaceManager::GetScreenAspect();
                    NiPoint2 kStart;
                    ::new (&kStart) NiPoint2(fX, fY);
                    float fRealX = this->pDragTarget->GetRealX();
                    float fRealY = this->pDragTarget->GetRealY();
                    if (InterfaceManager::IsPipboyMenuTopmost()) {
                        kStart.x = static_cast<float>(static_cast<double>(kStart.x) / GetMenuHeightRatio() - 195.0);
                        kStart.y = static_cast<float>(static_cast<double>(kStart.y) / GetMenuHeightRatio() - 195.0);
                    }
                    this->iDragStartX = static_cast<int32_t>(kStart.x);
                    this->iDragStartY = static_cast<int32_t>(kStart.y);
                    this->SetCursorOffsetX(0);
                    this->SetCursorOffsetY(0);
                    float fTileX = this->pDragTarget->GetFloat(Tile::kTileValue_X);
                    float fDragX = static_cast<float>(
                        static_cast<double>(kStart.x) - (static_cast<double>(fRealX) - fTileX)
                    );
                    this->pDragTarget->Set(Tile::kTileValue_DragX, fDragX, true);
                    float fTileY = this->pDragTarget->GetFloat(Tile::kTileValue_Y);
                    float fDragY = static_cast<float>(
                        static_cast<double>(kStart.y) - (static_cast<double>(fRealY) - fTileY)
                    );
                    this->pDragTarget->Set(Tile::kTileValue_DragY, fDragY, true);
                    this->pDragTarget->Set(Tile::kTileValue_DragStartX, this->iDragStartX);
                    this->pDragTarget->Set(Tile::kTileValue_DragStartY, this->iDragStartY);
                    float fOffsetX = kStart.x - fRealX;
                    this->pDragTarget->Set(Tile::kTileValue_DragOffsetX, fOffsetX, true);
                    float fOffsetY = kStart.y - fRealY;
                    this->pDragTarget->Set(Tile::kTileValue_DragOffsetY, fOffsetY, true);
                    this->fDragOffsetLastX = 0.0f;
                    this->fDragOffsetLastY = 0.0f;
                }
            }
            else if (MenuConsole::Instance(true)->IsVisible()) {
                this->GetRefsUnderCrosshair();
                this->DisplayCurrentPickRef();
            }
        }

        if (iHeld) {
            if (this->pOverTileMenu) {
                Tile* pTarget = this->pOverTileTarget;
                int32_t iID = static_cast<int32_t>(this->pOverTileTarget->GetFloat(Tile::kTileValue_ID));
                this->pOverTileMenu->DoStillDown(iID, pTarget);
            }
            if (this->pDragOverTileMenu && this->pOverTileTarget) {
                Tile* pTarget = this->pOverTileTarget;
                int32_t iID = static_cast<int32_t>(this->pOverTileTarget->GetFloat(Tile::kTileValue_ID));
                this->pDragOverTileMenu->DoStillDragOver(iID, pTarget);
            }
            if (this->pDragTarget) {
                float fDeltaX = static_cast<float>(
                    static_cast<double>(this->GetCursorOffsetX()) - this->fDragOffsetLastX
                );
                this->pDragTarget->Set(Tile::kTileValue_DragDeltax, fDeltaX, true);
                float fDeltaY = static_cast<float>(
                    static_cast<double>(this->GetCursorOffsetY()) - this->fDragOffsetLastY
                );
                this->pDragTarget->Set(Tile::kTileValue_DragDeltay, fDeltaY, true);
                this->fDragOffsetLastX = static_cast<float>(this->GetCursorOffsetX());
                this->fDragOffsetLastY = static_cast<float>(this->GetCursorOffsetY());
            }
        }
        // Compares against the double 0.0, so a NaN fMouseWheel takes the wheel branch.
        else if (this->fMouseWheel != 0.0) {
            if (this->pOverTileMenu) {
                Tile* pTile = this->pOverTileTarget;
                while (!pTile->IsTrue(Tile::kTileValue_Wheelable) && pTile->GetParent()) {
                    pTile = pTile->GetParent();
                }
                if (pTile && pTile->IsTrue(Tile::kTileValue_Wheelable)) {
                    // One wheel notch is 120; up scrolls up (signed division).
                    pTile->Set(Tile::kTileValue_Wheelmoved, this->GetMouseWheel() / -120);
                    pTile->Set(Tile::kTileValue_Wheelmoved, 0);
                    int32_t iID = static_cast<int32_t>(pTile->GetFloat(Tile::kTileValue_ID));
                    this->HandleMenuAndListboxScroll(this->pOverTileMenu, iID, pTile);
                }
            }
            else if (MenuConsole::Instance(true)->IsVisible()) {
                this->iCurrentPickIndex += 2 * (this->GetMouseWheel() >= 0) - 1;
                if (this->iCurrentPickIndex < -1) {
                    this->iCurrentPickIndex = this->ConsolePickRefs.ItemsInList() - 1;
                }
                // An unsigned compare: -1 stays -1.
                else if (
                    static_cast<uint32_t>(this->iCurrentPickIndex) >
                    static_cast<uint32_t>(this->ConsolePickRefs.ItemsInList() - 1)
                ) {
                    this->iCurrentPickIndex = -1;
                }
                this->DisplayCurrentPickRef();
            }
        }

        if (InterfaceManager::Has360Controller() && !this->bShowMouse) {
            this->DoGamepadEvent(Interface::eButtonNone);
        }

        // Key up events only stop the key repeat; the first other event is translated.
        int32_t iKey = 0;
        KEY_EVENT_TYPE iEvent = appMain->GetControls()->GetNextKeyEvent(iKey);
        while (iEvent == KEY_EVENT_UP) {
            this->SetRepeatingKey(0);
            this->GetChar(iEvent, iKey);
            iEvent = appMain->GetControls()->GetNextKeyEvent(iKey);
        }
        int32_t iChar = this->GetChar(iEvent, iKey);
        if (!iChar) {
            iChar = this->GetShouldKeyBeRepeated(1.0f);
        }
        bool bConsoleUsedKey = false;
        pConsole = MenuConsole::Instance(false);
        if (iChar && pConsole) {
            do {
                // 0x29 is the console key's scan code (DIK_GRAVE).
                if (iKey == 0x29) {
                    bConsoleUsedKey = false;
                }
                else {
                    bConsoleUsedKey = pConsole->Idle(iChar);
                }
                if (bConsoleUsedKey) {
                    iEvent = appMain->GetControls()->GetNextKeyEvent(iKey);
                    while (iEvent == KEY_EVENT_UP) {
                        this->SetRepeatingKey(0);
                        this->GetChar(iEvent, iKey);
                        iEvent = appMain->GetControls()->GetNextKeyEvent(iKey);
                    }
                    iChar = this->GetChar(iEvent, iKey);
                }
            }
            while (iChar && bConsoleUsedKey);
        }
        if (!iChar &&
            (!InterfaceManager::Has360Controller() || this->bShowMouse) &&
            pControls->GetUserAction(Controls::UACTN_UP, Controls::isPressed) &&
            Interface::IsTopMenuID(Interface::eMessageMenu)) {
            iChar = 1;
            pControls->RemoveKeybind(Controls::UACTN_UP, 0);
        }
        if (iChar && !bConsoleUsedKey) {
            bool bMenuUsedKey = false;
            Menu* pFront = MenuManager::Instance(true)->GetFrontmostMenu();
            // The pip-boy button closes the pip-boy rather than reaching the menu (Tab, DIK 0xF, excepted).
            bool bPipboyButton = InterfaceManager::IsPipboyMenuTopmost() &&
                (appMain->GetControls()->GetUserAction(Controls::UACTN_MENUMODE, Controls::isPressed) ||
                 appMain->GetControls()->GetUserAction(Controls::UACTN_MENUMODE, Controls::isDepressed)) &&
                !appMain->GetControls()->GetKeyboardButton(0xF, Controls::isHeld);
            if (pFront && !bPipboyButton) {
                bMenuUsedKey = pFront->DoKey(iChar);
            }
            if (!bMenuUsedKey && pFront && !bPipboyButton) {
                Tile* pButton = nullptr;
                char acTraitName[0x10];
                // 0xFF: the raw translation table, without the keyboard language.
                int32_t iTyped = appMain->GetControls()->TranslateKeystroke(iKey, 0xFF);
                BSsprintf(acTraitName, 0x10, "%s%c", "_PCButton_", iTyped);
                int32_t iTrait = Tile::TextToTrait(acTraitName);
                const char* pName = pFront->GetMenuTile()->GetString(iTrait);
                pButton = pFront->GetMenuTile()->GetChildByName(pName);
                if (pButton && pButton->IsTrue(Tile::kTileValue_Visible)) {
                    if (pButton->IsTrue(Tile::kTileValue_Target)) {
                        pButton->PlayTileSound(Tile::kTileValue_ClickSound);
                        pButton->Set(Tile::kTileValue_Clicked, 1);
                        pButton->Set(Tile::kTileValue_Clicked, 0);
                        pFront->DoClick(static_cast<int32_t>(pButton->GetFloat(Tile::kTileValue_ID)), pButton);
                    }
                    else {
                        // Menu sound 2: the refused click.
                        InterfaceManager::PlayMenuSound(MENU_SOUND_CANCEL);
                    }
                }
            }
            if (!bMenuUsedKey) {
                // The special menu keys (GetChar's 0x80000001..) become gamepad events.
                Interface::enumGamepadButtons eButton = Interface::eButtonNone;
                switch (iChar) {
                    case static_cast<int32_t>(0x80000001): eButton = Interface::eButtonDPadLeft; break;
                    case static_cast<int32_t>(0x80000002): eButton = Interface::eButtonDPadRight; break;
                    case static_cast<int32_t>(0x80000003): eButton = Interface::eButtonDPadUp; break;
                    case static_cast<int32_t>(0x80000004): eButton = Interface::eButtonDPadDown; break;
                    case static_cast<int32_t>(0x80000008):
                        if (this->IsShiftDown()) {
                            eButton = Interface::eButtonX;
                        }
                        else if (this->IsAltDown()) {
                            eButton = Interface::eButtonY;
                        }
                        else {
                            eButton = Interface::eButtonClick;
                        }
                        break;
                    case static_cast<int32_t>(0x80000009): eButton = Interface::eButtonLB; break;
                    case static_cast<int32_t>(0x8000000A): eButton = Interface::eButtonRB; break;
                    default: eButton = Interface::eButtonNone; break;
                }
                if (eButton == Interface::eButtonDPadLeft && this->IsShiftDown()) {
                    eButton = Interface::eButtonLT;
                }
                if (eButton == Interface::eButtonDPadRight && this->IsShiftDown()) {
                    eButton = Interface::eButtonRT;
                }
                if (eButton) {
                    this->DoGamepadEvent(eButton);
                    pControls->ClearKeystrokes();
                }
            }
        }
        if (pControls->GetUserAction(Controls::UACTN_QUICKSAVE, Controls::isPressed) &&
            !StartMenu::MainMenuIsActive()) {
            const char* pText = sCantQuickSave.String();
            Interface::ShowMessage(
                pText,
                0,
                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                nullptr,
                2.0f,
                false
            );
        }
        if (pControls->GetUserAction(Controls::UACTN_QUICKLOAD, Controls::isPressed) &&
            !StartMenu::MainMenuIsActive()) {
            const char* pText = sCantQuickLoad.String();
            Interface::ShowMessage(
                pText,
                0,
                "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                nullptr,
                2.0f,
                false
            );
        }
    }

    bool bOpenConsole = false;
    if (Main::GetOpenConsole()) {
        Main::SetOpenConsole(false);
        if (!MenuConsole::Instance(true)->IsVisible()) {
            bOpenConsole = true;
        }
    }
    if (pControls->GetUserAction(Controls::UACTN_DOWN, Controls::isPressed) || bOpenConsole) {
        pConsole = MenuConsole::Instance(true);
        pControls->ClearKeystrokes();
        if (pConsole->ToggleVisible()) {
            this->AddToEnterStack(Interface::eConsoleMenu);
        }
        else {
            this->PopFromEnterStack(Interface::eConsoleMenu, false);
        }
    }
    if (pControls->GetUserAction(Controls::UACTN_UP, Controls::isPressed) &&
        !MenuManager::Instance(true)->IsMenuOpen(Interface::eLoadingMenu) &&
        !StartMenu::MainMenuIsActive() &&
        (this->ePipboyState == PS_OPEN || this->ePipboyState == PS_CLOSED)) {
        if (StartMenu::PauseMenuIsActive() &&
            Interface::IsTopMenuID(Interface::eStartMenu) &&
            Interface::IsTopOfMenuStackReady()) {
            int32_t iTop = this->GetEnterStackTop();
            if (iTop == StartMenu::GetClassStatic() && !StartMenu::IsStartMenuSaving()) {
                StartMenu::Close(true);
            }
        }
        else if (Interface::bAllowPauseMenu) {
            if (iNumHWThreads.UInt() == 1) {
                StartMenu::Create(true, false);
            }
            else if (!Interface::IsMenuQueuedToOpen()) {
                Interface::QueueMenuCreate(
                    Interface::OPEN_PAUSE,
                    nullptr,
                    nullptr,
                    nullptr,
                    Interface::CMM_STANDARD,
                    false
                );
            }
        }
    }
    DebugText::Idle();

    // The dialog-pause switch is always false here, so the menus' idle pass always runs.
    bool bDialogPause = false;
    bIsReverting = false;
    if (!bDialogPause) {
        NiTListItem<Tile*>* pPos = this->pMenusRoot->xChildren.GetHeadPos();
        while (pPos && !bIsReverting) {
            Tile* pChild = this->pMenusRoot->xChildren.GetNext(pPos);
            if (pChild) {
                Menu* pMenu = pChild->GetMenu();
                if (pMenu) {
                    pMenu->DoIdle();
                }
            }
        }
    }
    else if (Interface::InDialog()) {
        Interface::SetDialogPause(true);
    }
    if (bIsReverting) {
        bIsReverting = false;
        pContext->~AutoMemContext();
        return;
    }

    bool bShouldOpenPipboy = false;
    if (this->ePipboyState == PS_OPEN) {
        // 360 buttons 10 and 6, with the thumbstick input.
        if (InterfaceManager::Has360Controller()) {
            bShouldOpenPipboy = (InterfaceManager::CheckMenuButton(Interface::eButtonB, true) == 1 ||
                                 InterfaceManager::CheckMenuButton(Interface::eButtonBack, true) == 1) &&
                !bPipboyBlocked;
        }
        else {
            bShouldOpenPipboy = pControls->GetUserAction(Controls::UACTN_MENUMODE, Controls::isDepressed) &&
                !bPipboyBlocked;
        }
    }
    else {
        bShouldOpenPipboy = pPlayer->GetMenuModeButtonClicked();
    }
    if (bShouldOpenPipboy && this->ePipboyState == PS_CLOSED) {
        this->OpenPipboy(nullptr, Interface::eNoMenu);
    }
    else {
        bool bClose = true;
        if (!pVATS.GetMode()) {
            bool bDead = pPlayer->IsDead(false);
            bClose = static_cast<uint8_t>(bDead) != 0 || bShouldOpenPipboy;
        }
        if (bClose && (!InterfaceManager::Has360Controller() || !this->bShowMouse)) {
            int32_t eState = this->ePipboyState;
            if (eState > PS_CLOSED && eState <= PS_OPEN) {
                this->ClosePipboy(nullptr);
                appMain->GetControls()->ClearUserActions();
            }
        }
    }

    // F1, F2 and F3 (DIK 0x3B..0x3D) open the stats, inventory and map tabs, or close the pip-boy on its own tab.
    Interface::enumMenus eMenu = Interface::eNoMenu;
    bool bTabKey = false;
    if (pControls->GetKeyboardButton(0x3B, Controls::isPressed)) {
        eMenu = Interface::eStatsMenu;
        bTabKey = true;
    }
    else if (pControls->GetKeyboardButton(0x3C, Controls::isPressed)) {
        eMenu = Interface::eInventoryMenu;
        bTabKey = true;
    }
    else if (pControls->GetKeyboardButton(0x3D, Controls::isPressed)) {
        eMenu = Interface::ePipBoyData;
        bTabKey = true;
    }
    if (bTabKey && this->ePipboyState == PS_CLOSED) {
        this->OpenPipboy(nullptr, eMenu);
    }
    if (bTabKey && this->ePipboyState == PS_OPEN) {
        // Trait 6001 of the menus' root: the class of the pip-boy menu shown. fucompp: NaN takes the switch.
        float fShown = this->GetMenusRoot()->GetFloat(6001);
        if (static_cast<double>(eMenu) == fShown) {
            this->ClosePipboy(nullptr);
        }
        else {
            switch (eMenu) {
                case Interface::eInventoryMenu: Interface::SetInventoryMenuVisible(true, false, true); break;
                case Interface::eStatsMenu: Interface::SetStatsMenuVisible(true, false); break;
                case Interface::ePipBoyData: Interface::SetMapMenuVisible(true, false); break;
            }
        }
    }
    if (this->GetEnterStackTop() == Interface::eNoMenu || InterfaceManager::IsPipboyMenuTopmost()) {
        this->UpdatePipboy();
    }
    this->xTutorialManager.Update();
    pContext->~AutoMemContext();
}

// The player singleton is re-read at every use. The heading is a virtual call, which for the player
// is PlayerCharacter::GetHeading.
void InterfaceManager::RefreshActivateREFR() {
    if (this->IsInMenuMode()) {
        return;
    }
    Tile* pHUD = Menu::GetMenuByClass(Interface::eHUDMainMenu);
    if (!pHUD) {
        pHUD = HUDMainMenu::Create();
    }
    TESObjectREFR* pTelekinesisRef = nullptr;
    TESObjectREFR* pActivateRef = nullptr;
    NiPoint3 kViewPos;
    NiPoint3 kViewDirection;
    NiMatrix3 kRotation;
    NiMatrix3 kLookRotation;
    if (p1stPersonCamera) {
        kViewPos = *pPlayer->GetCached1stPersonCameraPos();
    }
    else {
        kViewPos = pPlayer->GetLocationOnReference();
        // The eye height is spilled as a double; scale * height + z is stored as a float.
        double dEyeHeight;
        dEyeHeight = pPlayer->GetEyeHeight();
        kViewPos.z = static_cast<float>(pPlayer->GetScale() * dEyeHeight + kViewPos.z);
    }
    if (pPlayer->IsActually3rdPerson()) {
        kViewPos += pPlayer->GetCamera3rdPersonShoulderOffset();
    }
    kRotation.MakeZRotation(pPlayer->GetHeading(false));
    kLookRotation.MakeXRotation(pPlayer->GetLooking());
    kRotation = kRotation * kLookRotation;
    kRotation.GetCol(1, kViewDirection);
    const float fPickDistance = static_cast<float>(this->iPickDistance);
    if (!pHUD) {
        return;
    }

    uiInterfaceCheckpoint = 1;
    TESObjectREFR* pRef = nullptr;
    float fDistance = 0.0f;
    if (bActivatePickUseGamebryoPick.Bool() || bPreventHavokAddClutter.Bool() || bPreventHavokAddAll.Bool()) {
        alignas(NiPick) uint8_t aPickStorage[sizeof(NiPick)];
        NiPick* const pPick = ::new (aPickStorage) NiPick(0, 8);
        // Builds an item-pick filter (layer LAYER_ITEMPICK, the player's group) that the pick never takes.
        CFilter kPlayerFilter;
        pPlayer->GetCollisionFilter(&kPlayerFilter.iFilter);
        alignas(CFilter) uint8_t aItemFilter[sizeof(CFilter)];
        // The constructor's result is ignored and the storage is addressed again.
        ::new (aItemFilter) CFilter(0);
        CFilter* const pItemFilter = reinterpret_cast<CFilter*>(aItemFilter);
        pItemFilter->SetLayer(L_ITEMPICKER);
        pItemFilter->SetGroup(kPlayerFilter.GetGroup());
        if (pHUD && TES::pInstance->PickWorldDistance(pPick, kViewPos, kViewDirection, fPickDistance, 0x1F)) {
            for (uint32_t i = 0;; ++i) {
                uint32_t uiSize;
                uiSize = pPick->GetResults().GetSize();
                if (i >= uiSize) {
                    break;
                }
                pRef = TESObjectREFR::FindReferenceFor3D(
                    static_cast<NiAVObject*>(pPick->GetResults().GetAt(i)->m_spObject)
                );
                if (pRef != pPlayer) {
                    fDistance = pPick->GetResults().GetAt(i)->GetDistance();
                    break;
                }
                pRef = nullptr;
            }
        }
        pPick->~NiPick();
    }
    else {
        uiInterfaceCheckpoint = 2;
        if (!this->pViewCaster->GetInit() && pPlayer) {
            bhkCharacterController* const pController = static_cast<bhkCharacterController*>(
                pPlayer->GetCharController()
            );
            if (pController) {
                CFilter kFilter;
                pController->GetCollisionFilter(&kFilter);
                const int32_t iGroup = static_cast<int32_t>(kFilter.GetGroup());
                this->pViewCaster->Init(fActivatePickSphereRadius.Float(), iGroup);
            }
        }
        uiInterfaceCheckpoint = 3;
        if (this->pViewCaster->GetInit()) {
            uiInterfaceCheckpoint = 0x21;
            if (pPlayer->GetParentCell()) {
                bhkWorld* const pWorld = this->pViewCaster->GetWorld();
                if (pWorld != pPlayer->GetParentCell()->GetbhkWorld()) {
                    this->pViewCaster->Add(pPlayer->GetParentCell()->GetbhkWorld());
                }
            }
            uiInterfaceCheckpoint = 0x22;
            CFilter kFilter;
            const int32_t iGroup = static_cast<int32_t>(
                reinterpret_cast<CFilter*>(pPlayer->GetCollisionFilter(&kFilter.iFilter))->GetGroup()
            );
            if (this->pViewCaster->GetGroup() != iGroup) {
                this->pViewCaster->SetShapeGroup(iGroup);
            }
            uiInterfaceCheckpoint = 0x23;
            pRef = this->pViewCaster
                       ->FindActivateREFR(kViewPos, kViewDirection, fPickDistance, fDistance, this->bFuzzyActivatePick);
            bool bPickWater = true;
            if (pRef) {
                const uint8_t ucType = pRef->GetObjectReference()->GetFormType();
                switch (ucType) {
                    case ACTI_ID:
                    case CONT_ID:
                    case DOOR_ID:
                    case NPC__ID:
                    case CREA_ID: bPickWater = false; break;
                    default: bPickWater = !TESContainer::ContainerCanHoldType(ucType); break;
                }
            }
            if (bPickWater) {
                alignas(NiPick) uint8_t aWaterPickStorage[sizeof(NiPick)];
                NiPick* const pWaterPick = ::new (aWaterPickStorage) NiPick(0, 8);
                const float fWaterDistance = BSMIN(fPickDistance, fDistance);
                if (TES::pInstance->PickWaterDistance(pWaterPick, kViewPos, kViewDirection, fWaterDistance)) {
                    TESObjectREFR* pWaterRef;
                    pWaterRef = TESObjectREFR::FindReferenceFor3D(
                        static_cast<NiAVObject*>(pWaterPick->GetResults().GetAt(0)->m_spObject)
                    );
                    if (pWaterRef) {
                        pRef = pWaterRef;
                    }
                }
                pWaterPick->~NiPick();
            }
        }
    }

    uiInterfaceCheckpoint = 4;
    if (pRef) {
        if (pPlayer->GetTelekinesisSelected()) {
            pTelekinesisRef = pRef;
            // The pick distance is compared unrounded.
            if (fDistance <= static_cast<double>(this->iPickDistance)) {
                pActivateRef = pRef;
            }
        }
        else {
            pActivateRef = pRef;
        }
    }
    else {
        pActivateRef = nullptr;
        pTelekinesisRef = nullptr;
    }
    this->pTelekinesisRef = pTelekinesisRef;
    this->pActivateRef = pActivateRef;
    this->pActivatePickLocation = kViewPos + kViewDirection * fDistance;
    uiInterfaceCheckpoint = 5;
    HUDMainMenu::SetInfoForRef(pRef, this->pActivateRef == nullptr, false);
}

// The player singleton is deliberately re-read at every use.
void InterfaceManager::UpdatePipboy() {
    switch (this->ePipboyState) {
        case PS_OPENING_SETUP:
            if (Interface::IsTopMenuID(Interface::eContainerMenu)) {
                ContainerMenu::Close();
            }
            else if (
                Interface::IsTopMenuID(Interface::eComputersMenu) || Interface::IsTopMenuID(Interface::eHackingMenu)
            ) {
                static_cast<FORenderedTerminal*>(this->pCurrentRenderedMenu)->CloseMenu();
            }
            else {
                switch (pPlayer->GetAnimAction()) {
                    case ANIM_ACTION_EQUIP_WEAPON:
                    case ANIM_ACTION_UNEQUIP_WEAPON:
                    case ANIM_ACTION_ATTACK:
                    case ANIM_ACTION_ATTACK_WAIT_FOR_EJECT:
                    case ANIM_ACTION_ATTACK_FOLLOW_THROUGH:
                    case ANIM_ACTION_ATTACK_THROW:
                    case ANIM_ACTION_ATTACK_THROW_ATTACH:
                    case ANIM_ACTION_RECOIL:
                    case ANIM_ACTION_RELOAD:
                    case ANIM_ACTION_STAGGER:
                    case ANIM_ACTION_DODGE:
                    case ANIM_ACTION_WAIT_FOR_LOWER_BODY_ANIM:
                    case ANIM_ACTION_WAIT_FOR_SPECIAL_IDLE:
                    case ANIM_ACTION_RELOAD_LOOP: return;
                    default: break;
                }
                Animation* pAnimation = pPlayer->GetAnimation(false);
                if (pAnimation) {
                    pAnimation->ClearGroup(AGS_MOVEMENT, 0.0f);
                    pAnimation = pPlayer->GetAnimation(true);
                }
                if (pAnimation) {
                    pAnimation->ClearGroup(AGS_MOVEMENT, 0.0f);
                    pPlayer->ForceTemp1stPerson(false);
                    pAnimation->PlayGroup(ANIM_GROUP_PIPBOY, ACTION_START, -1, AGS_NONE);
                    Interface::GetPipboy()->FadeLightEffect(true);
                    BSAudio::QInstance()
                        .GetSoundHandleByName(
                            "UIPipBoyAccessUp",
                            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                BSGameSound::SOUND_FLAG_ONE_SHOT
                        )
                        .Play(false);
                    ImageSpaceModifierInstanceForm::Stop(TESImageSpaceModifier::GetGetHit());
                    ++InterfaceManager::Instance()->iSoundPauses;
                    BSAudio::QInstance().PauseAnimationSounds();
                    pPlayer->EnteringMenuMode();
                    this->AddToEnterStack(Interface::eMainFour);
                    Interface::ShowMenus();
                    this->ePipboyState = PS_OPENING;
                }
            }
            break;
        case PS_OPENING: {
            Animation* const pAnimation = pPlayer->GetAnimation(true);
            if (pAnimation && pAnimation->GetCurrentSequence(AGS_LEFT_ARM)) {
                if (pAnimation->GetGroupAction(AGS_LEFT_ARM) == 1) {
                    HUDMainMenu::SetMenuMode(HUDMainMenu::HMM_PIPBOY);
                    if (pPlayer->IsWeaponDrawn()) {
                        TESObjectWEAP* const pWeapon = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon()
                            ? static_cast<TESObjectWEAP*>(
                                  pPlayer->GetCurrentAIProcess()->GetCurrentWeapon()->GetContainerObject()
                              )
                            : nullptr;
                        if (pWeapon && pPlayer->GetAnimation(true) && pPlayer->GetAnimation(true)->GetSceneRoot()) {
                            NiNode* const pRoot = pPlayer->GetAnimation(true)->GetSceneRoot();
                            ClearGunWobble(weaponToAnimGroupWeapon[pWeapon->GetWeaponType()], pRoot);
                        }
                    }
                    // The negated offset is stored as a float before the group's time is added to it.
                    float fTime = -pAnimation->GetCurrentSequence(AGS_LEFT_ARM)->GetOffset();
                    fTime = pAnimation->GetCurrentSequence(AGS_LEFT_ARM)->GetAnimGroup()->GetTime(AGA_STOP) + fTime;
                    pAnimation->Update(pPlayer, 0.0f, fTime);
                    pAnimation->UpdateBipOnly(fTime, nullptr, true);
                    pAnimation->UpdateSceneGraphNoController();
                    bUseOldMenuModeScriptFunction = true;
                    BaseProcess* const pProcess = pPlayer->GetCurrentAIProcess();
                    if (pProcess->LoadDynamicIdle(pPlayer)) {
                        pPlayer->GetAnimation(false)->ClearGroup(AGS_IDLE, 0.0f);
                    }
                    else {
                        Warning("ANIMATION: Missing dynamic idle for the pipboy waiver in the idle manager.");
                    }
                    bUseOldMenuModeScriptFunction = false;
                    if (this->pPipboyCallback) {
                        this->pPipboyCallback();
                        this->pPipboyCallback = nullptr;
                    }
                    InterfaceManager::Instance()->GetPipboy()->TriggerRandomEffect();
                    FOPipboyManager* const pPipboy = InterfaceManager::Instance()->GetPipboy();
                    InterfaceManager::Instance()->EnterRenderedMenu(pPipboy);
                    this->ePipboyState = PS_OPEN;
                }
            }
            else {
                Interface::GetPipboy()->FadeLightEffect(false);
                this->ePipboyState = PS_CLOSING_SETUP;
            }
            if (MapMenu::ShouldRenderLocalMap()) {
                MapMenu::RenderLocalMapGrid(false);
            }
            break;
        }
        case PS_OPEN:
            if (MapMenu::ShouldRenderLocalMap()) {
                MapMenu::RenderLocalMapGrid(false);
            }
            break;
        case PS_CLOSING_SETUP:
            if (this->PopFromEnterStack(Interface::eMainFour, false) >= 0) {
                Interface::GetPipboy()->FadeLightEffect(false);
                if (pPlayer->GetSitSleepState() == SIT_SLEEP_IS_SITTING) {
                    static_cast<MiddleHighProcess*>(pPlayer->GetCurrentAIProcess())
                        ->ReloadDynamicIdleOn1stPerson(pPlayer);
                }
                const uint16_t usGroup = pPlayer->GetAnimation(false)->GetGroup(AGS_IDLE);
                pPlayer->StartAnimOn1stPerson(usGroup, ACTION_START);
                MapMenu::ClearMapMemory();
                while (InterfaceManager::Instance()->iSoundPauses) {
                    --InterfaceManager::Instance()->iSoundPauses;
                    BSAudio::QInstance().UnPauseAnimationSounds();
                }
                this->ePipboyState = PS_CLOSING;
            }
            break;
        case PS_CLOSING:
            if (!pPlayer->IsPipboyActive()) {
                const uint16_t usGroup = pPlayer->GetAnimation(false)->GetGroup(AGS_LEFT_ARM);
                pPlayer->StartAnimOn1stPerson(usGroup, ACTION_START);
                this->ePipboyState = PS_CLOSED;
                if (this->pPipboyCallback) {
                    this->pPipboyCallback();
                    this->pPipboyCallback = nullptr;
                }
            }
            break;
        default: break;
    }
}

// The exe's cleanup-only C++ EH frame is left out.
void InterfaceManager::DoGamepadEvent(Interface::enumGamepadButtons aiButton) {
    // Starts at 0 (a departure): GetFirstRefCopy writes it only when a tile up the chain has the button's trait, which
    // never happens for eButtonClick, so a click takes the plain click on the target; the exe tests an unset value.
    int32_t iType = 0;
    bool bIsUpEvent = false;
    NiTListItem<Tile*>* pPos = GetMenusRoot()->xChildren.m_pkTail;
    while (pPos) {
        Tile* pMenuTile = GetMenusRoot()->xChildren.GetPrev(pPos);
        Menu* pMenu = nullptr;
        if (pMenuTile) {
            pMenu = pMenuTile->GetMenu();
        }
        if (pMenu && pMenuTile->IsVisible() && DoGamepad(pMenu, Interface::eButtonIdleEvent, 0.0f)) {
            return;
        }
    }
    if (InterfaceManager::Has360Controller() && !bShowMouse) {
        bool bRepeatsV = false;
        bool bRepeatsH = false;
        Tile* pTarget = pMouseOverTarget;
        if (pTarget && pTarget->GetMenu() && pTarget->GetMenu()->xFadeState != Interface::FADE_SHOWN) {
            return;
        }
        if (pTarget) {
            bRepeatsV = pTarget->IsTrue(4047);
            bRepeatsH = pTarget->IsTrue(4048);
        }
        if (aiButton == Interface::eButtonNone) {
            aiButton = GetTopGamepadButton(bRepeatsV, bRepeatsH, bIsUpEvent);
        }
        if (aiButton &&
            aiButton != Interface::eButtonDPadLeft &&
            aiButton != Interface::eButtonDPadRight &&
            aiButton != Interface::eButtonDPadUp &&
            aiButton != Interface::eButtonDPadDown) {
            iDragDroppingState = 0;
        }
    }
    Menu* pFrontMenu = MenuManager::Instance(true)->GetFrontmostMenu();
    Tile* pFrontMenuTile = pFrontMenu ? pFrontMenu->pMenu : nullptr;
    if (!pFrontMenu || bIsUpEvent || !DoGamepad(pFrontMenu, aiButton, 1.0f)) {
        if (bIsUpEvent) {
            if (pFrontMenu) {
                // Menu's DoGamepadUpEvent takes the button as an int32_t.
                pFrontMenu->DoGamepadUpEvent(static_cast<int32_t>(aiButton), 1.0f);
            }
        }
        else if (pFrontMenu && pFrontMenuTile && aiButton) {
            if (aiButton == Interface::eButtonDPadUp ||
                aiButton == Interface::eButtonDPadDown ||
                aiButton == Interface::eButtonDPadLeft ||
                aiButton == Interface::eButtonDPadRight) {
                bMouseInMotion = false;
                iType = 4035;
                int32_t iID = 0;
                switch (aiButton) {
                    case Interface::eButtonDPadUp: {
                        iID = 4055;
                        break;
                    }
                    case Interface::eButtonDPadDown: {
                        iID = 4056;
                        break;
                    }
                    case Interface::eButtonDPadLeft: {
                        iID = 4057;
                        break;
                    }
                    default: {
                        iID = 4058;
                        break;
                    }
                }
                Tile* pTarget = pMouseOverTarget;
                if (pTarget) {
                    Tile* pNext = pTarget->GetFirstRefCopy(iID, iType);
                    BSSimpleList<Tile*> xLast;
                    xLast.AddHead(pTarget);
                    while (pNext && !xLast.IsInList(pNext) && (!pNext->IsTrue(4003) || !pNext->IsTrue(4015))) {
                        xLast.AddHead(pNext);
                        pNext = pNext->GetFirstRefCopy(iID, iType);
                    }
                    if (pNext && xLast.IsInList(pNext) && (!pNext->IsTrue(4003) || !pNext->IsTrue(4015))) {
                        pNext = nullptr;
                    }

                    xLast.RemoveAll();
                    if (pNext) {
                        if (pNext != pTarget && iType == 4035) {
                            pNext->PlayTileSound(4072);
                        }
                        if (pNext != pTarget && iType == 4039) {
                            pNext->PlayTileSound(4043);
                        }
                        SetCurrentFocusTarget(pNext, iType, true);
                    }
                    else if (DoGamepad(pFrontMenu, aiButton, 1.0f) || pTarget == pMouseOverTarget) {
                        if (pTarget && iType == 4039) {
                            pTarget->PlayTileSound(4043);
                        }
                    }
                    else {
                        SetCurrentFocusTarget(pTarget, iType, true);
                        if (pTarget) {
                            pTarget->PlayTileSound(4072);
                        }
                    }
                }
                else {
                    int32_t iMaxFocus = INT32_MIN;
                    Tile* pFocusTile = ScanForMaxFocus(iMaxFocus, nullptr);
                    SetCurrentFocusTarget(pFocusTile, 4035, true);
                    if (!pFocusTile) {
                        Tile* pRefTile = pFrontMenuTile->GetFirstRefCopy(iID, iType);
                        if (pRefTile) {
                            pRefTile->PlayTileSound(4043);
                            pRefTile->Set(4039, 1);
                            pRefTile->Set(4039, 0);
                            double dClickID = pRefTile->GetFloat(4010);
                            pFrontMenu->DoClick(static_cast<int32_t>(dClickID), pRefTile);
                        }
                        else if (InterfaceManager::Has360Controller() || !bShowMouse) {
                            DoGamepad(pFrontMenu, aiButton, static_cast<float>(iID));
                        }
                    }
                }
            }
            else {
                pPos = GetMenusRoot()->xChildren.m_pkTail;
                while (pPos) {
                    Tile* pMenuTile = GetMenusRoot()->xChildren.GetPrev(pPos);
                    Menu* pMenu = nullptr;
                    if (pMenuTile) {
                        pMenu = pMenuTile->GetMenu();
                    }
                    if (pMenu && pMenuTile->IsVisible()) {
                        float fValue = static_cast<float>(InterfaceManager::CheckMenuButton(aiButton, true));
                        if (DoGamepad(pMenu, aiButton, fValue)) {
                            InterfaceManager::ClearMenuButton(aiButton);
                            return;
                        }
                    }
                }
                int32_t iID = 0;
                switch (aiButton) {
                    case Interface::eButtonA: {
                        iID = 4061;
                        break;
                    }
                    case Interface::eButtonB: {
                        iID = 4062;
                        break;
                    }
                    case Interface::eButtonX: {
                        iID = 4063;
                        break;
                    }
                    case Interface::eButtonY: {
                        iID = 4064;
                        break;
                    }
                    case Interface::eButtonRT: {
                        iID = 4066;
                        break;
                    }
                    case Interface::eButtonLT: {
                        iID = 4065;
                        break;
                    }
                    case Interface::eButtonRB: {
                        iID = 4068;
                        break;
                    }
                    case Interface::eButtonLB: {
                        iID = 4067;
                        break;
                    }
                    case Interface::eButtonStart: {
                        iID = 4071;
                        break;
                    }
                }
                // One local holds the mouse-over target, else the tile target, else the menu's tile, then walks up
                // the parents to the first visible one.
                Tile* pTarget = pMouseOverTarget;
                if (!pTarget) {
                    pTarget = pOverTileTarget;
                }
                if (!pTarget) {
                    pTarget = pFrontMenuTile;
                }
                while (pTarget && !pTarget->IsVisible()) {
                    pTarget = pTarget->pParent;
                }
                if (pTarget) {
                    Tile* pRefTile = pTarget->GetFirstRefCopy(iID, iType);
                    Tile* pParent = pTarget;
                    while (!pRefTile && pParent) {
                        pParent = pParent->pParent;
                        if (pParent) {
                            pRefTile = pParent->GetFirstRefCopy(iID, iType);
                        }
                    }
                    if (aiButton == Interface::eButtonClick) {
                        pRefTile = pTarget;
                        aiButton = Interface::eButtonA;
                        iID = 4061;
                    }
                    if (pRefTile && pRefTile->IsVisible() && (iType == 4039 || iType == 4037)) {
                        if (pRefTile->IsTrue(4015)) {
                            if (iType == 4037) {
                                iModifierKeys |= MODIFIER_SHIFT;
                            }
                            pRefTile->PlayTileSound(4043);
                            pRefTile->Set(4039, 1);
                            pRefTile->Set(4039, 0);
                            Menu* pMenu = pRefTile->GetMenu();
                            double dClickID = pRefTile->GetFloat(4010);
                            pMenu->DoClick(static_cast<int32_t>(dClickID), pRefTile);
                            if (iType == 4037) {
                                iModifierKeys &= 0xFFFF & ~MODIFIER_SHIFT;
                            }
                            InterfaceManager::ClearMenuButton(aiButton);
                            // Sets the button to B and clears Back when it is set.
                            aiButton = Interface::eButtonB;
                            if (aiButton) {
                                InterfaceManager::ClearMenuButton(Interface::eButtonBack);
                            }
                        }
                        else {
                            InterfaceManager::PlayMenuSound(MENU_SOUND_CANCEL);
                        }
                    }
                    else if (aiButton == Interface::eButtonA && pTarget->IsTrue(4015)) {
                        pTarget->PlayTileSound(4043);
                        pTarget->Set(4039, 1);
                        pTarget->Set(4039, 0);
                        Menu* pMenu = pTarget->GetMenu();
                        double dClickID = pTarget->GetFloat(4010);
                        pMenu->DoClick(static_cast<int32_t>(dClickID), pTarget);
                        InterfaceManager::ClearMenuButton(Interface::eButtonA);
                    }
                }
            }
        }
        else {
            MenuConsole* pConsole = MenuConsole::Instance(true);
            if (pConsole->IsVisible() &&
                (aiButton == Interface::eButtonDPadUp || aiButton == Interface::eButtonDPadDown)) {
                iCurrentPickIndex += 2 * (aiButton != Interface::eButtonDPadDown) - 1;
                if (iCurrentPickIndex >= -1) {
                    if (static_cast<uint32_t>(iCurrentPickIndex) >
                        static_cast<uint32_t>(ConsolePickRefs.ItemsInList() - 1)) {
                        iCurrentPickIndex = -1;
                    }
                }
                else {
                    iCurrentPickIndex = ConsolePickRefs.ItemsInList() - 1;
                }
                DisplayCurrentPickRef();
            }
        }
    }
}

// Repeat deadlines: the tick count is widened to a 64-bit integer and then to a double, the setting is added in
// double precision, and the sum is truncated to 64 bits and stored to its low 32 bits.
Interface::enumGamepadButtons
InterfaceManager::GetTopGamepadButton(bool abRepeatVertical, bool abRepeatHorizontal, bool& abIsUpEvent) {
    Interface::enumGamepadButtons eCode = Interface::eButtonNone;
    if (!InterfaceManager::Has360Controller() || this->bShowMouse) {
        return eCode;
    }
    // The controls are fetched and not used.
    appMain->GetControls();
    bool bResetRepeat = true;
    // A, B, X, Y, the right shoulder, Back and the right trigger, in this order: the first pressed or released one.
    static constexpr Interface::enumGamepadButtons kButtons[] = {
        Interface::eButtonA,
        Interface::eButtonB,
        Interface::eButtonX,
        Interface::eButtonY,
        Interface::eButtonRB,
        Interface::eButtonBack,
        Interface::eButtonRT
    };
    bool bButton = false;
    for (const Interface::enumGamepadButtons eButton : kButtons) {
        if (InterfaceManager::CheckMenuButton(eButton, true) == 2 ||
            InterfaceManager::CheckMenuButton(eButton, true) == 1) {
            eCode = eButton;
            this->iLastGamepadEvent = Interface::eButtonNone;
            bResetRepeat = false;
            abIsUpEvent = InterfaceManager::CheckMenuButton(eButton, true) == 1;
            bButton = true;
            break;
        }
    }
    if (!bButton) {
        // The d-pad (1 up, 2 down, 3 right, 4 left): pressed this frame (2) or held (0); 17 and 18 are the thumbstick
        // clicks.
        const int32_t iPressedLeft = InterfaceManager::CheckMenuButton(Interface::eButtonDPadLeft, true) == 2;
        const int32_t iPressedRight = InterfaceManager::CheckMenuButton(Interface::eButtonDPadRight, true) == 2;
        const int32_t iPressedUp = InterfaceManager::CheckMenuButton(Interface::eButtonDPadUp, true) == 2;
        const int32_t iPressedDown = InterfaceManager::CheckMenuButton(Interface::eButtonDPadDown, true) == 2;
        const int32_t iHeldLeft = InterfaceManager::CheckMenuButton(Interface::eButtonDPadLeft, true) == 0;
        const int32_t iHeldRight = InterfaceManager::CheckMenuButton(Interface::eButtonDPadRight, true) == 0;
        const int32_t iHeldUp = InterfaceManager::CheckMenuButton(Interface::eButtonDPadUp, true) == 0;
        const int32_t iHeldDown = InterfaceManager::CheckMenuButton(Interface::eButtonDPadDown, true) == 0;
        const int32_t iPressedLeftThumb = InterfaceManager::CheckMenuButton(Interface::eButtonLThumb, true) == 2;
        const int32_t iPressedRightThumb = InterfaceManager::CheckMenuButton(Interface::eButtonRThumb, true) == 2;
        // A newly pressed direction counts only while the other axis is idle.
        if (iPressedUp && !iPressedLeft && !iPressedRight && !iHeldLeft && !iHeldRight) {
            eCode = Interface::eButtonDPadUp;
        }
        else if (iPressedDown && !iPressedLeft && !iPressedRight && !iHeldLeft && !iHeldRight) {
            eCode = Interface::eButtonDPadDown;
        }
        else if (iPressedLeft && !iPressedUp && !iPressedDown && !iHeldUp && !iHeldDown) {
            eCode = Interface::eButtonDPadLeft;
        }
        else if (iPressedRight && !iPressedUp && !iPressedDown && !iHeldUp && !iHeldDown) {
            eCode = Interface::eButtonDPadRight;
        }
        else if (iPressedLeftThumb) {
            eCode = Interface::eButtonLThumb;
        }
        else if (iPressedRightThumb) {
            eCode = Interface::eButtonRThumb;
        }

        // The held direction that continues the last event.
        Interface::enumGamepadButtons eHeldEvent = Interface::eButtonNone;
        if (iHeldUp && this->iLastGamepadEvent == Interface::eButtonDPadUp) {
            eHeldEvent = Interface::eButtonDPadUp;
        }
        else if (iHeldDown && this->iLastGamepadEvent == Interface::eButtonDPadDown) {
            eHeldEvent = Interface::eButtonDPadDown;
        }
        else if (iHeldLeft && this->iLastGamepadEvent == Interface::eButtonDPadLeft) {
            eHeldEvent = Interface::eButtonDPadLeft;
        }
        else if (iHeldRight && this->iLastGamepadEvent == Interface::eButtonDPadRight) {
            eHeldEvent = Interface::eButtonDPadRight;
        }

        if (!eCode && !eHeldEvent && iHeldLeft + iHeldRight + iHeldUp + iHeldDown) {
            if (iHeldUp) {
                eCode = Interface::eButtonDPadUp;
            }
            else if (iHeldDown) {
                eCode = Interface::eButtonDPadDown;
            }
            else if (iHeldLeft) {
                eCode = Interface::eButtonDPadLeft;
            }
            else if (iHeldRight) {
                eCode = Interface::eButtonDPadRight;
            }
        }

        if (eCode && this->iLastGamepadEvent != eCode) {
            this->iLastGamepadEvent = eCode;
            if ((abRepeatVertical && iPressedUp + iPressedDown + iHeldUp + iHeldDown) ||
                (abRepeatHorizontal && iPressedLeft + iPressedRight + iHeldLeft + iHeldRight)) {
                const double dTime = static_cast<double>(static_cast<int64_t>(appTimer.GetTime()));
                this->uGamepadRepeatStartTime = static_cast<uint32_t>(
                    static_cast<int64_t>(dTime + fKeyRepeatTime.Float())
                );
            }
            else {
                this->uGamepadRepeatStartTime = appTimer.GetTime() * 2;
            }
        }
        else if (this->iLastGamepadEvent == eHeldEvent && this->uGamepadRepeatStartTime <= appTimer.GetTime()) {
            eCode = this->iLastGamepadEvent;
            if ((abRepeatVertical && iPressedUp + iPressedDown + iHeldUp + iHeldDown) ||
                (abRepeatHorizontal && iPressedLeft + iPressedRight + iHeldLeft + iHeldRight)) {
                const double dTime = static_cast<double>(static_cast<int64_t>(appTimer.GetTime()));
                this->uGamepadRepeatStartTime = static_cast<uint32_t>(
                    static_cast<int64_t>(dTime + fXenonMenuDpadRepeatSpeed.Float())
                );
            }
        }
        else if (this->iLastGamepadEvent != eHeldEvent) {
            eCode = Interface::eButtonNone;
            this->iLastGamepadEvent = eCode;
        }
        else {
            eCode = Interface::eButtonNone;
        }
        if (iPressedLeft + iPressedRight + iPressedUp + iPressedDown + iHeldLeft + iHeldRight + iHeldUp + iHeldDown) {
            bResetRepeat = false;
        }
    }

    if (!eCode) {
        // The shoulders (15 left, 16 right) and the triggers (13 left, 14 right): one of a pair alone.
        const int32_t iPressedLB = InterfaceManager::CheckMenuButton(Interface::eButtonLB, true) == 2;
        const int32_t iPressedRB = InterfaceManager::CheckMenuButton(Interface::eButtonRB, true) == 2;
        const int32_t iPressedLT = InterfaceManager::CheckMenuButton(Interface::eButtonLT, true) == 2;
        const int32_t iPressedRT = InterfaceManager::CheckMenuButton(Interface::eButtonRT, true) == 2;
        if (iPressedLB + iPressedRB + iPressedLT + iPressedRT) {
            this->iLastGamepadEvent = Interface::eButtonNone;
            bResetRepeat = false;
        }
        if (iPressedLB && !iPressedRB) {
            eCode = Interface::eButtonLB;
        }
        else if (iPressedRB && !iPressedLB) {
            eCode = Interface::eButtonRB;
        }
        else if (iPressedLT && !iPressedRT) {
            eCode = Interface::eButtonLT;
        }
        else if (iPressedRT && !iPressedLT) {
            eCode = Interface::eButtonRT;
        }
    }
    // Start.
    if (!eCode && InterfaceManager::CheckMenuButton(Interface::eButtonStart, true) == 2) {
        eCode = Interface::eButtonStart;
        this->iLastGamepadEvent = Interface::eButtonNone;
        bResetRepeat = false;
    }
    if (bResetRepeat) {
        this->iLastGamepadEvent = Interface::eButtonNone;
        this->uGamepadRepeatStartTime = appTimer.GetTime() * 2;
    }
    return eCode;
}

// Constructs and destroys an AutoMemContext on a stack buffer and ignores the constructor's result.
void InterfaceManager::UpdateCursor() {
    if (InterfaceManager::Has360Controller() && !this->bShowMouse && !MenuConsole::Instance(true)->IsVisible()) {
        return;
    }
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\InterfaceManager.cpp",
        3153
    );
    Controls* pControls = appMain->GetControls();
    if ((InterfaceManager::Has360Controller() && !this->bShowMouse) || pControls->IsMouseInBackground()) {
        pContext->~AutoMemContext();
        return;
    }
    this->pCursor->GetModel()->SetAppCulled(false);
    this->pCursor->Set(Tile::kTileValue_Visible, 1);
    int32_t iX = 0;
    int32_t iY = 0;
    if (InterfaceManager::Has360Controller() && !this->bShowMouse) {
        iX = pControls->GetControllerValue(0, 7);
        iY = pControls->GetControllerValue(0, 8);
        const double dX = iX;
        iX = static_cast<int32_t>(fXenonMenuMouseXYMult.Float() * dX);
        const double dY = -static_cast<double>(iY);
        iY = static_cast<int32_t>(fXenonMenuMouseXYMult.Float() * dY);
    }
    else {
        iX = pControls->GetMouseAxis(1);
        iY = pControls->GetMouseAxis(2);
    }
    float fSpeed = 1.0f;
    if (iX || iY) {
        if (pControls->GetForegroundMouseBase() > 0.0) {
            const double dBase = pControls->GetForegroundMouseBase();
            const double dMult = pControls->GetForegroundMouseMult();
            const int32_t iSensitivity = pControls->GetMouseSensitivity();
            fSpeed = static_cast<float>(iSensitivity / 20.0 * dMult + dBase);
        }
        if (pControls->GetForegroundMouseAccelTop() > 0.0) {
            const double dAccelBase = pControls->GetForegroundMouseAccelBase();
            const int32_t iAbsX = std::abs(iX);
            const double dSum = iAbsX + std::abs(iY);
            fSpeed = static_cast<float>((dSum / pControls->GetForegroundMouseAccelTop() + dAccelBase) * fSpeed);
        }
    }
    NiPoint3 kPos = this->pCursor->GetModel()->GetTranslate();
    const double dX = iX;
    const float fNewX = static_cast<float>(InterfaceManager::GetScreenWidth() * dX / 1280.0 * fSpeed + kPos.x);
    const float fRight = static_cast<float>(InterfaceManager::GetScreenRight() - 2.0);
    const float fClampedX = BSMIN(fRight, fNewX);
    const float fLeft = static_cast<float>(InterfaceManager::GetScreenLeft() - 2.0);
    kPos.x = BSMAX(fLeft, fClampedX);
    kPos.y = static_cast<float>(this->pCursor->GetFloat(Tile::kTileValue_Depth) * -0.00800000037997961);
    const double dY = iY;
    const float fNewZ = static_cast<float>(kPos.z - InterfaceManager::GetScreenHeight() * dY / 960.0 * fSpeed);
    const float fTop = static_cast<float>(InterfaceManager::GetScreenTop() + 1.0);
    const float fClampedZ = BSMIN(fTop, fNewZ);
    const float fBottom = static_cast<float>(InterfaceManager::GetScreenBottom() + 1.0);
    kPos.z = BSMAX(fBottom, fClampedZ);
    this->pCursor->GetModel()->SetTranslate(kPos);
    if (kPos.x != this->sCursorPos.x || kPos.z != this->sCursorPos.z) {
        this->sCursorPos = kPos;
        this->bMouseInMotion = true;
        const double dRealWidth = InterfaceManager::GetRealScreenWidth();
        const double dScaledX = dRealWidth / InterfaceManager::GetScreenWidth() * this->sCursorPos.x;
        this->sRealCursorPos.x = static_cast<float>(InterfaceManager::GetRealScreenWidth() / 2.0 + dScaledX);
        this->sRealCursorPos.y = static_cast<float>(
            this->pCursor->GetFloat(Tile::kTileValue_Depth) * -0.00800000037997961
        );
        const double dHalfHeight = InterfaceManager::GetRealScreenHeight() / 2.0;
        const double dRealHeight = InterfaceManager::GetRealScreenHeight();
        this->sRealCursorPos.z = static_cast<float>(
            dHalfHeight - dRealHeight / InterfaceManager::GetScreenHeight() * this->sCursorPos.z
        );
    }
    NiUpdateData kUpdateData(0.0f, false, false);
    this->pCursor->GetModel()->Update(kUpdateData);
    pContext->~AutoMemContext();
}

// AutoMemContext as in UpdateCursor.
void InterfaceManager::PostIdleStuff() {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\InterfaceManager.cpp",
        3233
    );
    Controls* pControls = appMain->GetControls();
    this->bLockMenuModeForFade = false;
    NiTListItem<Tile*>* pIter = this->GetMenusRoot()->xChildren.GetHeadPos();
    while (pIter) {
        Tile* pChild = this->GetMenusRoot()->xChildren.GetNext(pIter);
        Menu* pMenu = pChild ? pChild->GetMenu() : nullptr;
        if (!pMenu || !pMenu->GetMenuTile() || pMenu->GetMenuTile()->GetNeedsNiUpdate()) {
            continue;
        }
        const float fFade = InterfaceManager::GetTimerPercent(pMenu);
        const bool bFadeDone = std::fabs(fFade) == 1.0;
        const int32_t eFadeState = pMenu->GetFadeState();
        const int32_t iTop = this->GetEnterStackTop();
        const int32_t iStack0 = this->GetEnterStack(0);
        const int32_t iStack1 = this->GetEnterStack(1);
        if (eFadeState == 2) {
            const bool bBlend = (!iTop && !iStack0) || (iStack0 && !iStack1 && iStack0 == Interface::eMessageMenu);
            if (pMenu && bFadeDone) {
                pMenu->SetFadeState(Interface::FADE_HIDDEN);
                // 6002 is a menu trait with no known name.
                if (pMenu->GetMenuTile()->IsTrue(6002)) {
                    if (pMenu->GetMenuTile()) {
                        TileMenu* pTileMenu = pMenu->GetMenuTile();
                        delete pTileMenu;
                    }
                    pMenu = nullptr;
                    pIter = nullptr;
                }
                else {
                    pMenu->GetMenuTile()->Set(Tile::kTileValue_Visible, 0);
                    this->RecursiveFade(pMenu->GetMenuTile()->GetModel(), 0.0f, 0.0f);
                    if (bBlend) {
                        BSShaderManager::GetImageSpaceManager()->SetMenuBGBlendValue(0.0f);
                        BSShaderManager::GetImageSpaceManager()->SetMenuBGBlendEnabled(false);
                    }
                }
            }
            else {
                this->bLockMenuModeForFade = true;
                pMenu->GetMenuTile()->Set(Tile::kTileValue_Visible, 1);
                const float fAmount = 1.0f - fFade;
                this->RecursiveFade(pMenu->GetMenuTile()->GetModel(), fAmount, 0.0f);
                if (bBlend) {
                    const float fStrength = 1.0f - fFade;
                    BSShaderManager::GetImageSpaceManager()->SetMenuBGBlendValue(fStrength);
                }
            }
        }
        else if (eFadeState == 8) {
            const bool bBlend = (iTop && iStack0 == iTop && iTop != Interface::eMessageMenu) ||
                (iStack1 && iStack1 == iTop && iStack0 == Interface::eMessageMenu);
            if (bFadeDone) {
                pMenu->GetMenuTile()->Set(Tile::kTileValue_Visible, 1);
                this->RecursiveFade(pMenu->GetMenuTile()->GetModel(), 1.0f, 0.0f);
                pMenu->SetFadeState(Interface::FADE_SHOWN);
                if (bBlend) {
                    BSShaderManager::GetImageSpaceManager()->SetMenuBGBlendValue(1.0f);
                    BSShaderManager::GetImageSpaceManager()->SetMenuBGBlendEnabled(false);
                }
            }
            else {
                this->bLockMenuModeForFade = true;
                pMenu->GetMenuTile()->Set(Tile::kTileValue_Visible, 1);
                this->RecursiveFade(pMenu->GetMenuTile()->GetModel(), fFade, 0.0f);
                if (bBlend) {
                    BSShaderManager::GetImageSpaceManager()->SetMenuBGBlendValue(fFade);
                }
            }
        }
    }
    if (this->cMenuMode == 4 && (!this->bLockMenuModeForFade || TES::pInstance->CellTestRunning())) {
        pControls->ClearKeystrokes();
        this->cMenuMode = eMenuModeOff;
        if (!InterfaceManager::Has360Controller() || this->bShowMouse) {
            this->pCursor->GetModel()->SetAppCulled(true);
            this->pCursor->Set(Tile::kTileValue_Visible, 0);
        }
        this->pCursor->UpdateTile(true);
        this->iModifierKeys = 0;
    }
    uiInterfaceCheckpoint = 99;
    pContext->~AutoMemContext();
}

// AutoMemContext as in UpdateCursor.
void InterfaceManager::RecursiveFade(NiAVObject* apObject, float afAmount, float afLastAlpha) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\InterfaceManager.cpp",
        3371
    );
    if (!apObject) {
        pContext->~AutoMemContext();
        return;
    }
    NiNode* pNode = apObject->IsNode();
    Tile* pTile = Tile::GetTileFromNode(pNode);
    while (!pTile && pNode) {
        pNode = pNode->GetParent();
        pTile = Tile::GetTileFromNode(pNode);
    }
    pNode = apObject->IsNode();
    float fAlpha = afLastAlpha;
    if (pTile) {
        if (pTile->IsTrue(Tile::kTileValue_DisableFade)) {
            if (std::fabs(afAmount) < 1.0) {
                pTile->Set(Tile::kTileValue_Visible, 0);
            }
            else {
                pTile->Set(Tile::kTileValue_Visible, 1);
            }
            pContext->~AutoMemContext();
            return;
        }
        fAlpha = static_cast<float>(pTile->GetFloat(Tile::kTileValue_Alpha) / 255.0);
    }
    NiTriBasedGeom* pGeometry = apObject->IsTriBasedGeom();
    if (pGeometry) {
        float fTileAlpha;
        // The threshold is the float 1e-4 widened to a double.
        if (std::fabs(afAmount) < 9.999999747378752e-05) {
            fTileAlpha = 0.0f;
        }
        else {
            const float fScaled = afAmount * fAlpha;
            fTileAlpha = BSMAX(0.0f, BSMIN(fScaled, fAlpha));
        }
        const int32_t iType = NiShadeProperty::GetType();
        NiProperty* pProperty = pGeometry->GetProperty(iType);
        if (pProperty) {
            static_cast<TileShaderProperty*>(pProperty)->SetTileAlpha(fTileAlpha);
        }
    }
    if (pNode && (!pTile || pTile->GetFloat(Tile::kTileValue_ID) != 9000.0)) {
        for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
            this->RecursiveFade(pNode->GetAt(i), afAmount, fAlpha);
        }
    }
    pContext->~AutoMemContext();
}

// The pick lives in raw stack storage: NiPick's constructor and destructor run through Construct and Destruct, which a
// C++ object would follow with its member constructors and destructors a second time. AutoMemContext as in
// UpdateCursor; the pick is destroyed before the context.
Tile* InterfaceManager::PickMenus(bool abExactTargetOnly) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\InterfaceManager.cpp",
        3420
    );
    Tile* pPicked = nullptr;
    this->GetCursorRoot()->SetAppCulled(true);
    NiAVObject* pTarget = this->GetSceneGraph();
    if (this->pCurrentRenderedMenu) {
        this->pCurrentRenderedMenu->Unknown07();
        if (StartMenu::PauseMenuIsActive()) {
            pTarget = Menu::GetMenuByClass(this->GetEnterStackTop())->GetModel();
        }
    }
    alignas(NiPick) uint8_t aPickStorage[sizeof(NiPick)];
    NiPick* pPick = ::new (aPickStorage) NiPick(0, 8);
    pPick->SetFrontOnly(false);
    pPick->SetObserveAppCullFlag(true);
    pPick->SetTarget(pTarget);
    NiPoint3 kOrigin;
    NiPoint3 kDir;
    const float fRealWidth = InterfaceManager::GetRealScreenWidth();
    float fX = BSMIN(BSMAX(0.0f, this->sRealCursorPos.x), fRealWidth);
    const float fRealHeight = InterfaceManager::GetRealScreenHeight();
    float fY = BSMIN(BSMAX(0.0f, this->sRealCursorPos.z), fRealHeight);
    if (bUsePipboyMode.Bool()) {
        if (this->bMouseOverRenderedMenu) {
            fX = this->fPipboyMouseX;
            fY = this->fPipboyMouseY;
        }
        else if (InterfaceManager::IsCurrentRenderedMenuTopmost()) {
            pPick->~NiPick();
            pContext->~AutoMemContext();
            return nullptr;
        }
    }
    if (BSShaderManager::bLetterbox) {
        // The rect is deliberately built with NiColorA's constructor.
        NiRect<float> kPort;
        ::new (&kPort) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
        this->GetSceneGraph()->GetCamera()->SetViewPort(kPort);
    }
    const int32_t iY = static_cast<int32_t>(fY);
    const int32_t iX = static_cast<int32_t>(fX);
    this->GetSceneGraph()->GetCamera()->WindowPointToRay(iX, iY, kOrigin, kDir, nullptr);
    if (pPick->PickObjects(kOrigin, kDir, false)) {
        int32_t i = 0;
        while (true) {
            int32_t iSize;
            iSize = static_cast<int32_t>(pPick->GetResults().GetSize());
            if (i >= iSize) {
                break;
            }
            NiPick::Record* pRecord;
            pRecord = pPick->GetResults().GetAt(i);
            ++i;
            NiAVObject* pObject;
            pObject = static_cast<NiAVObject*>(pRecord->m_spObject);
            NiNode* pNode = nullptr;
            if (pObject) {
                pNode = static_cast<NiAVObject*>(pRecord->m_spObject)->IsNode();
            }
            if (!pNode) {
                NiNode* pParent;
                pParent = static_cast<NiAVObject*>(pRecord->m_spObject)->GetParent();
                if (pParent) {
                    pNode = static_cast<NiAVObject*>(pRecord->m_spObject)->GetParent()->IsNode();
                }
            }
            Tile* pTile = pNode ? Tile::GetTileFromNode(pNode) : nullptr;
            while (!pTile && pNode) {
                pNode = pNode->GetParent();
                pTile = Tile::GetTileFromNode(pNode);
            }
            if (pTile) {
                if (!this->TileIsAcceptingEvents(pTile)) {
                    continue;
                }
                if (pTile->IsTrue(Tile::kTileValue_Clips) && pTile->GetModel()) {
                    const bool bAspect = pTile->GetMenu()->GetMenuTile()->GetFloat(Tile::kTileValue_ID) == 112.0;
                    pTile->GetModel()->CompactChildArray();
                    if (pTile->GetModel()->GetAtChecked(0)) {
                        const int32_t iType = NiShadeProperty::GetType();
                        NiProperty* pProperty = pTile->GetModel()->GetAt(0)->GetProperty(iType);
                        if (pProperty) {
                            int32_t aRect[4];
                            const int32_t* pRect = static_cast<TileShaderProperty*>(pProperty)->GetScissorRect();
                            aRect[0] = pRect[0];
                            aRect[1] = pRect[1];
                            aRect[2] = pRect[2];
                            aRect[3] = pRect[3];
                            if (bUsePipboyMode.Bool() && this->bMouseOverRenderedMenu) {
                                const double dRealWidth = Interface::GetRealScreenWidth();
                                const float fScale = static_cast<float>(
                                    dRealWidth / Interface::GetRenderedMenuScreenWidth()
                                );
                                aRect[0] = static_cast<int32_t>(static_cast<double>(aRect[0]) * fScale);
                                aRect[2] = static_cast<int32_t>(static_cast<double>(aRect[2]) * fScale);
                                aRect[1] = static_cast<int32_t>(static_cast<double>(aRect[1]) * fScale);
                                aRect[3] = static_cast<int32_t>(static_cast<double>(aRect[3]) * fScale);
                            }
                            if (aRect[0] || aRect[2] || aRect[1] || aRect[3]) {
                                float fTestX = fX;
                                if (bAspect) {
                                    fTestX = fTestX * InterfaceManager::Instance()->fAspectRatioRatio;
                                }
                                const float fTestY = fY;
                                if (static_cast<double>(fTestX) < aRect[0]) {
                                    continue;
                                }
                                if (static_cast<double>(fTestX) >= aRect[2]) {
                                    continue;
                                }
                                if (static_cast<double>(fTestY) < aRect[1]) {
                                    continue;
                                }
                                if (static_cast<double>(fTestY) >= aRect[3]) {
                                    continue;
                                }
                            }
                        }
                    }
                }
            }
            if (pTile && !pTile->SpecialBoundsCheck(fX, fY)) {
                continue;
            }
            if (!pTile || !pTile->IsTrue(Tile::kTileValue_Target)) {
                continue;
            }
            if (!abExactTargetOnly && !pTile->GetValue(Tile::kTileValue_ID)) {
                Tile* pParent = pTile->GetParent();
                while (pParent && !pParent->GetValue(Tile::kTileValue_ID)) {
                    pParent = pParent->GetParent();
                }
                if (pParent) {
                    pTile = pParent;
                }
            }
            pPicked = pTile;
            if (pPicked->GetType() == Tile::kTileValue_Text) {
                this->iCharHit = static_cast<int16_t>(pRecord->GetTriangleIndex() / 2);
            }
            else {
                this->iCharHit = -1;
            }
            break;
        }
    }
    this->GetCursorRoot()->SetAppCulled(false);
    if (this->pCurrentRenderedMenu) {
        this->pCurrentRenderedMenu->Unknown08();
    }
    pPick->~NiPick();
    pContext->~AutoMemContext();
    return pPicked;
}

// AutoMemContext as in UpdateCursor.
SceneGraph* InterfaceManager::InitSceneGraph(SceneGraph* apSceneGraph, const char* apName, bool abIs3D) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\InterfaceManager.cpp",
        3601
    );
    SceneGraph* pSceneGraph = apSceneGraph;
    if (!pSceneGraph) {
        pSceneGraph = new SceneGraph(apName, false, nullptr);
    }
    NiRenderer* pRenderer = NiRenderer::GetRenderer();
    NiMatrix3 kRotate;
    kRotate.SetRow(0, 0.0f, 0.0f, 1.0f);
    kRotate.SetRow(1, 1.0f, 0.0f, 0.0f);
    kRotate.SetRow(2, 0.0f, 1.0f, 0.0f);
    pSceneGraph->GetCamera()->SetRotate(kRotate);
    const double dWidth = pRenderer->GetScreenWidth();
    const float fAspect = static_cast<float>(dWidth / static_cast<double>(pRenderer->GetScreenHeight()));
    // The divisor is the float 4/3 widened to a double.
    this->fAspectRatioRatio = static_cast<float>(fAspect / 1.3333333730697632);
    NiFrustum kFrustum(false);
    kFrustum.m_bOrtho = true;
    kFrustum.m_fLeft = 0.0f;
    kFrustum.m_fRight = static_cast<float>(this->fAspectRatioRatio * 1280.0);
    kFrustum.m_fTop = 960.0f;
    kFrustum.m_fBottom = 0.0f;
    kFrustum.m_fNear = 0.0f;
    kFrustum.m_fFar = 10000.0f;
    pSceneGraph->GetCamera()->SetViewFrustum(kFrustum);
    const float fCameraZ = static_cast<float>(-kFrustum.m_fTop * 0.5);
    const float fCameraX = static_cast<float>(-kFrustum.m_fRight * 0.5);
    // The point is constructed before GetCamera is called.
    const NiPoint3 kCameraTranslate(fCameraX, 0.0f, fCameraZ);
    pSceneGraph->GetCamera()->SetTranslate(kCameraTranslate);
    if (!abIs3D) {
        pFaderManager->AddRoot(pSceneGraph, this->fOneToOneDistance, true);
        this->pInterfaceRoot = new NiNode(0);
        NiFixedString kMainName("InterfaceManager: Main Root");
        this->pInterfaceRoot->SetName(kMainName);
        pSceneGraph->AttachChild(this->pInterfaceRoot, true);
        this->pInterfaceRoot->SetTranslate(NiPoint3(0.0f, this->fOneToOneDistance, 0.0f));
        this->pCursorRoot = new NiNode(0);
        NiFixedString kCursorName("InterfaceManager: Cursor Root");
        this->pCursorRoot->SetName(kCursorName);
        pSceneGraph->AttachChild(this->pCursorRoot, true);
        this->pCursorRoot->SetTranslate(NiPoint3(0.0f, this->fOneToOneDistance, 0.0f));
        pFaderManager->AddRoot(pSceneGraph, this->fOneToOneDistance, false);
    }
    else {
        pSceneGraph->SetCameraFOV(35.0f, false, nullptr, false);
    }
    pContext->~AutoMemContext();
    return pSceneGraph;
}

void InterfaceManager::ReinitialisePipboyManager() {
    if (this->pPipboy) {
        delete this->pPipboy;
    }
    this->pPipboy = new FOPipboyManager;
    this->pPipboy->Initialize();
}

void InterfaceManager::DrawInterface_Screen(BSCullingProcess* apCuller, bool abPipboyVisible) {
    // abPipboyVisible is never read, but callers still store it and pass it. Without a use, clang's dead argument
    // elimination leaves the caller's argument unset; KeepLive keeps it live.
    xfnv::KeepLive(abPipboyVisible);
    apCuller->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
    NiRenderer* pRenderer = NiRenderer::GetRenderer();
    if (bUseImageSpaceMenuFX.Bool() && bUsePipboyMode.Bool()) {
        SceneGraph* pSceneGraph = this->GetSceneGraph();
        if (this->bClickMultithreaded) {
            MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 0x17);
        }
        else {
            if (!this->IsInMenuMode() && !Interface::IsInGameLoadingMenuOpen()) {
                this->LastMinuteUpdate();
            }
            if (this->IsInRenderedMenu() && !Interface::IsInGameLoadingMenuOpen()) {
                Interface::IsolateMenuElements(false, false);
            }
        }
        Tile* apMenus[3] = {
            Menu::GetMenuByClass(Interface::eComputersMenu),
            Menu::GetMenuByClass(Interface::eRaceSexMenu),
            Menu::GetMenuByClass(Interface::eHackingMenu)
        };
        for (Tile*& rpMenu : apMenus) {
            if (rpMenu && rpMenu->GetModel() && !rpMenu->GetModel()->GetAppCulled()) {
                rpMenu->GetModel()->SetAppCulled(true);
            }
            else {
                rpMenu = nullptr;
            }
        }
        if (!this->bClickMultithreaded) {
            apCuller->SetAccumulator(this->spInterfaceAccum.m_pObject);
            BSShaderUtil::AccumulateScene(pSceneGraph->GetCamera(), pSceneGraph, *apCuller);
            MTRenderingSystem::QInstance()->SetThreadStage(MTRenderingSystem::THREAD_1, 0x17);
            MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 0x17);
        }
        NiRenderer* pCurrentRenderer = NiRenderer::GetRenderer();
        NiRenderTargetGroup* pDefaultTarget = NiRenderer::GetRenderer()->GetDefaultRenderTargetGroup();
        if (pCurrentRenderer->GetCurrentRenderTargetGroup() == pDefaultTarget) {
            pSceneGraph->GetCamera()->SetViewPort(BSShaderManager::kFramebufferViewport);
        }
        BSShaderUtil::RenderScene(pSceneGraph->GetCamera(), this->spInterfaceAccum.m_pObject, false);
        apCuller->SetAccumulator(nullptr);
        for (Tile* pMenu : apMenus) {
            if (pMenu && pMenu->GetModel() && pMenu->GetModel()->GetAppCulled()) {
                pMenu->GetModel()->SetAppCulled(false);
            }
        }
        if (this->IsInRenderedMenu() && !Interface::IsInGameLoadingMenuOpen()) {
            Interface::RestoreMenuElements();
        }
        if (bEnableMenuRenderDebugMode.Bool()) {
            Interface::IsolateMenuElements(true, false);
            SceneGraph* pDebugSceneGraph = this->GetSceneGraph();
            apCuller->SetAccumulator(this->spInterfaceAccum.m_pObject);
            BSShaderUtil::AccumulateScene(pDebugSceneGraph->GetCamera(), pDebugSceneGraph, *apCuller);
            BSShaderUtil::RenderScene(pDebugSceneGraph->GetCamera(), this->spInterfaceAccum.m_pObject, false);
            apCuller->SetAccumulator(nullptr);
            Interface::RestoreMenuElements();
        }
    }
    else if (!bUseImageSpaceMenuFX.Bool() && !bUsePipboyMode.Bool()) {
        if (this->bClickMultithreaded) {
            MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 0x17);
        }
        Interface::RestoreMenuElements();
        SceneGraph* pSceneGraph = this->GetSceneGraph();
        if (!this->bClickMultithreaded) {
            apCuller->SetAccumulator(this->spInterfaceAccum.m_pObject);
            BSShaderUtil::AccumulateScene(pSceneGraph->GetCamera(), pSceneGraph, *apCuller);
        }
        BSShaderUtil::RenderScene(pSceneGraph->GetCamera(), this->spInterfaceAccum.m_pObject, false);
        apCuller->SetAccumulator(nullptr);
    }
    else if (!bUseImageSpaceMenuFX.Bool()) {
        if (this->bClickMultithreaded) {
            MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 0x17);
        }
        Interface::IsolateMenuElements(true, false);
        SceneGraph* pSceneGraph = this->GetSceneGraph();
        pRenderer->SetCameraData(pSceneGraph->GetCamera());
        if (!this->bClickMultithreaded) {
            apCuller->SetAccumulator(this->spInterfaceAccum.m_pObject);
            BSShaderUtil::AccumulateScene(pSceneGraph->GetCamera(), pSceneGraph, *apCuller);
        }
        BSShaderUtil::RenderScene(pSceneGraph->GetCamera(), this->spInterfaceAccum.m_pObject, false);
        apCuller->SetAccumulator(nullptr);
        Interface::RestoreMenuElements();
        Interface::IsolateMenuElements(false, false);
        pSceneGraph = this->GetSceneGraph();
        pRenderer->SetCameraData(pSceneGraph->GetCamera());
        apCuller->SetAccumulator(this->spInterfaceAccum.m_pObject);
        BSShaderUtil::AccumulateScene(pSceneGraph->GetCamera(), pSceneGraph, *apCuller);
        BSShaderUtil::RenderScene(pSceneGraph->GetCamera(), this->spInterfaceAccum.m_pObject, false);
        apCuller->SetAccumulator(nullptr);
        Interface::RestoreMenuElements();
    }
    else if (!bUsePipboyMode.Bool()) {
        if (this->bClickMultithreaded) {
            MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 0x17);
        }
        Interface::IsolateMenuElements(true, true);
        SceneGraph* pSceneGraph = this->GetSceneGraph();
        pRenderer->SetCameraData(pSceneGraph->GetCamera());
        if (!this->bClickMultithreaded) {
            apCuller->SetAccumulator(this->spInterfaceAccum.m_pObject);
            BSShaderUtil::AccumulateScene(pSceneGraph->GetCamera(), pSceneGraph, *apCuller);
        }
        if (!NiRenderer::GetRenderer()->GetCurrentRenderTargetGroup()) {
            pSceneGraph->GetCamera()->SetViewPort(BSShaderManager::kFramebufferViewport);
        }
        BSShaderUtil::RenderScene(pSceneGraph->GetCamera(), this->spInterfaceAccum.m_pObject, false);
        apCuller->SetAccumulator(nullptr);
        Interface::RestoreMenuElements();
    }
    else if (this->bClickMultithreaded) {
        MTRenderingSystem::QInstance()->WaitOnThreadStage(MTRenderingSystem::THREAD_2, 0x17);
    }
    apCuller->PopCullMode();
}

void InterfaceManager::LastMinuteUpdate() {
    if (static_cast<int32_t>(iNumHWThreads.UInt()) > 1) {
        AILinearTaskThreadManager::QInstance();
        if (AILinearTaskThreadManager::QRunningThreads()) {
            AILinearTaskThreadManager::QInstance()->WaitOnThreadStage(AILinearTaskThreadManager::AI_THREAD_1, 1);
        }
    }
    ClearRenderedMenuDeleteList();
    Interface::bLastMinuteUpdate = true;
    this->GetSceneGraph();
    Tile::PromoteHibernators();
    this->bTextWasCreated = false;
    if (Interface::IsInGameLoadingMenuOpen() && !StartMenu::MainMenuIsActive()) {
        // 0x3EF is the loading menu.
        static_cast<Tile*>(Menu::GetMenuByClass(0x3EF))->UpdateChildren(false);
    }
    else {
        Tile::UpdateAll(false);
    }
    Tile::UpdateFadeControls();
    Tile::ResetUpdateCount();
    if (!InterfaceManager::xTileUpdateList.IsEmpty()) {
        Tile::GetTileUpdateAllCriticalSectionLock()->Enter(nullptr);
        this->UpdateTileModels();
        Tile::GetTileUpdateAllCriticalSectionLock()->Leave();
    }
    Interface::bLastMinuteUpdate = false;
}

// spSceneGraph's object.
SceneGraph* InterfaceManager::GetSceneGraph() {
    return spSceneGraph.m_pObject;
}

SceneGraph* InterfaceManager::Get3DSceneGraph() {
    return sp3DSceneGraph.m_pObject;
}

Tile* InterfaceManager::GetMenusRoot() {
    return pMenusRoot;
}

BSShaderAccumulator* InterfaceManager::Get3dAccumulator() {
    return sp3dInterfaceAccum.m_pObject;
}

int32_t InterfaceManager::GetCursorY() {
    double screenHeight = InterfaceManager::GetScreenHeight();
    return static_cast<int32_t>(screenHeight - ((InterfaceManager::GetScreenHeight() / 2.0) + sCursorPos.z));
}

// Picks the reference at iCurrentPickIndex of ConsolePickRefs as the console's selected reference and prints its debug
// lines (with bFullHelp: script, furniture, owner, count, lock, teleport and action flags), then blanks the lines the
// previous selection printed below them.
void InterfaceManager::DisplayCurrentPickRef() {
    static int32_t ilastLoc;
    TESObjectREFR* pSelected = nullptr;
    // Reads the font setting before it fetches the font manager.
    int32_t iFont = iDebugTextFont.Int() - 1;
    int32_t iLineHeight = static_cast<int32_t>(FontManager::Instance()->GetFont(iFont)->GetBaseLine()) + 3;
    int32_t iTop = iDebugTextTopBottomOffset.Int();
    int32_t iY = iTop + 30;
    int32_t iIndex = 0;
    for (BSSimpleList<TESObjectREFR*>* pNode = &ConsolePickRefs; pNode && pNode->m_item && iIndex <= iCurrentPickIndex;
         pNode = pNode->m_pkNext) {
        pSelected = pNode->m_item;
        ++iIndex;
    }
    if (pSelected) {
        BSStringT<char> sLine;
        SetTargetREFR(pSelected);
        pPickRef = pSelected;
        uint32_t uiFormID = pPickRef->iFormID;
        sLine.SPrintF("\"%s\" (%08x)", pPickRef->GetFullName(), uiFormID);
        DebugText::Instance(true)->Print(sLine.pString, 640.0f, static_cast<float>(iTop), 2, -1, -1.0f, 0, nullptr);
        if (bFullHelp) {
            if (pPickRef->m_Extra.GetScript()) {
                Script* pScript = pPickRef->m_Extra.GetScript();
                uint32_t uiScriptID = pPickRef->m_Extra.GetScript()->iFormID;
                sLine.SPrintF("Script name '%s' (%08x)", pScript->GetFormEditorID(), uiScriptID);
                DebugText::Instance(true)
                    ->Print(sLine.pString, 640.0f, static_cast<float>(iY), 2, -1, -1.0f, 0, nullptr);
                iY += iLineHeight;
            }
            if (pPickRef->IsActor()) {
                // IsActor said so, so the reference is used as the actor without a checked cast.
                Actor* pActor = static_cast<Actor*>(pPickRef);
                if (pActor->GetCurrentAIProcess() && pActor->GetCurrentAIProcess()->GetCurrentFurniture()) {
                    BaseProcess* pIDProcess = pActor->GetCurrentAIProcess();
                    BaseProcess* pNameProcess = pActor->GetCurrentAIProcess();
                    TESObjectREFR* pFurniture = pNameProcess->GetCurrentFurniture();
                    uint32_t uiFurnitureID = pIDProcess->GetCurrentFurniture()->iFormID;
                    sLine.SPrintF("current Furniture '%s' (%08x)", pFurniture->GetFormEditorID(), uiFurnitureID);
                    DebugText::Instance(true)
                        ->Print(sLine.pString, 640.0f, static_cast<float>(iY), 2, -1, -1.0f, 0, nullptr);
                    iY += iLineHeight;
                }
            }
            if (pPickRef->GetOwner()) {
                TESForm* pOwner = pPickRef->GetOwner();
                uint32_t uiOwnerID = pPickRef->GetOwner()->iFormID;
                sLine.SPrintF("Owner name '%s' (%08x)", pOwner->GetFormEditorID(), uiOwnerID);
                DebugText::Instance(true)
                    ->Print(sLine.pString, 640.0f, static_cast<float>(iY), 2, -1, -1.0f, 0, nullptr);
                iY += iLineHeight;
            }
            if (pPickRef->m_Extra.GetCount() != 1) {
                sLine.SPrintF("Count %d", pPickRef->m_Extra.GetCount());
                DebugText::Instance(true)
                    ->Print(sLine.pString, 640.0f, static_cast<float>(iY), 2, -1, -1.0f, 0, nullptr);
                iY += iLineHeight;
            }
            if (pPickRef->IsActor()) {
                // The cast's result is deliberately not tested.
                Actor* pActor = dynamic_cast<Actor*>(pPickRef);
                if (pActor->GetSitSleepState()) {
                    if (pActor->GetCurrentAIProcess()->GetCurrentFurniture()) {
                        BaseProcess* pIndexProcess = pActor->GetCurrentAIProcess();
                        BaseProcess* pIDProcess = pActor->GetCurrentAIProcess();
                        BaseProcess* pNameProcess = pActor->GetCurrentAIProcess();
                        TESObjectREFR* pFurniture = pNameProcess->GetCurrentFurniture();
                        uint32_t uiMarkerIndex = pIndexProcess->GetCurrentFurnitureIndex();
                        uint32_t uiFurnitureID = pIDProcess->GetCurrentFurniture()->iFormID;
                        sLine.SPrintF(
                            "Furniture '%s' (%08x) index %d",
                            pFurniture->GetFormEditorID(),
                            uiFurnitureID,
                            uiMarkerIndex
                        );
                    }
                    else {
                        sLine.SPrintF(
                            "Furniture 'UNKNOWN' (UNKNOWN) index %d",
                            pActor->GetCurrentAIProcess()->GetCurrentFurnitureIndex()
                        );
                    }
                    DebugText::Instance(true)
                        ->Print(sLine.pString, 640.0f, static_cast<float>(iY), 2, -1, -1.0f, 0, nullptr);
                    iY += iLineHeight;
                }
            }
            if (pPickRef->IsFurniture()) {
                sLine.SPrintF(
                    "Orentation %d deg",
                    static_cast<int32_t>(pPickRef->GetAngleOnReference().z * 57.2957763671875)
                );
                DebugText::Instance(true)
                    ->Print(sLine.pString, 640.0f, static_cast<float>(iY), 2, -1, -1.0f, 0, nullptr);
                iY += iLineHeight;
                FurnitureMark kMarker(255u);
                for (uint32_t uiMarker = 0; pPickRef->GetMarkerAtIndex(uiMarker, kMarker); ++uiMarker) {
                    TESFurniture* pFurniture = static_cast<TESFurniture*>(pPickRef->GetObjectReference());
                    uint32_t uiType = kMarker.GetType();
                    if (pFurniture->GetMarkerEnabled(uiMarker)) {
                        float fScale = pPlayer->GetScale();
                        NiPoint3 kOffset = pFurniture->GetMarkerTargetOffset(kMarker.GetType(), fScale);
                        const char* pUsed = pPickRef->GetMarkerUsed(uiMarker, true) ? "USED" : "UNUSED";
                        const char* pKind = TESFurniture::IsSitMarker(uiType) ? "Sit" : "Sleep";
                        int32_t iHeading = static_cast<int32_t>(
                            pFurniture->GetMarkerHeadingOffset(uiType) * 57.2957763671875
                        );
                        sLine.SPrintF(
                            "%s Marker %d Delta %0.2f,%0.2f,%0.2f (%d deg) %s",
                            pKind,
                            uiType,
                            kOffset.x,
                            kOffset.y,
                            kOffset.z,
                            iHeading,
                            pUsed
                        );
                    }
                    else {
                        const char* pKind = TESFurniture::IsSitMarker(uiType) ? "Sit" : "Sleep";
                        sLine.SPrintF("Disabled %s Marker %d", pKind, uiType);
                    }
                    DebugText::Instance(true)
                        ->Print(sLine.pString, 640.0f, static_cast<float>(iY), 2, -1, -1.0f, 0, nullptr);
                    iY += iLineHeight;
                }
            }
            if (pPickRef->GetLock()) {
                LockData* pLock = pPickRef->GetLock();
                const char* pState = pLock->IsLocked() ? "Locked" : "Unlocked";
                int32_t iLockLevel = pLock->GetLockLevel(pPickRef);
                sLine.SPrintF("Lock '%s' %s.", pLockLevelNames[iLockLevel]->String(), pState);
                DebugText::Instance(true)
                    ->Print(sLine.pString, 640.0f, static_cast<float>(iY), 2, -1, -1.0f, 0, nullptr);
                iY += iLineHeight;
                if (pLock->pKey) {
                    sLine.SPrintF("Key '%s'.", static_cast<TESFullName*>(pLock->pKey)->GetFullName());
                    DebugText::Instance(true)
                        ->Print(sLine.pString, 640.0f, static_cast<float>(iY), 2, -1, -1.0f, 0, nullptr);
                    iY += iLineHeight;
                }
            }
            if (pPickRef->GetTeleport()) {
                DoorTeleportData* pTeleport = pPickRef->GetTeleport();
                TESObjectCELL* pCell = pTeleport->GetTeleportCell();
                TESObjectREFR* pDoor = pTeleport->pLinkedDoor;
                const char* pCellName = "Unknown";
                const char* pDoorName = "Unknown";
                if (pCell && static_cast<TESFullName*>(pCell)->GetFullName()) {
                    pCellName = static_cast<TESFullName*>(pCell)->GetFullName();
                }
                else if (pDoor && pDoor->m_Extra.GetPersistentCell()) {
                    pCellName = "Persistent";
                }
                if (pDoor && pDoor->GetFullName()) {
                    pDoorName = pDoor->GetFullName();
                }
                sLine.SPrintF("Teleport Cell '%s' Door '%s'.", pCellName, pDoorName);
                DebugText::Instance(true)
                    ->Print(sLine.pString, 640.0f, static_cast<float>(iY), 2, -1, -1.0f, 0, nullptr);
                iY += iLineHeight;
            }
            uint32_t uiActionFlags = pPickRef->GetActionFlag();
            if (uiActionFlags) {
                char acFlags[260];
                BSstrcpy(acFlags, 256, "Action Flags: ");
                if ((uiActionFlags & OBJECT_ACTION_USE_DEFAULT) != 0) {
                    BSstrcat(acFlags, 256, "USE_DEFAULT");
                }
                if ((uiActionFlags & OBJECT_ACTION_ACTIVATE) != 0) {
                    BSstrcat(acFlags, 256, ", ACTIVATE");
                }
                if ((uiActionFlags & OBJECT_ACTION_OPEN) != 0) {
                    BSstrcat(acFlags, 256, ", OPEN");
                }
                if ((uiActionFlags & OBJECT_ACTION_OPEN_BY_DEFAULT) != 0) {
                    BSstrcat(acFlags, 256, ", OPEN_BY_DEFAULT");
                }
                sLine.SPrintF(acFlags);
                DebugText::Instance(true)
                    ->Print(sLine.pString, 640.0f, static_cast<float>(iY), 2, -1, -1.0f, 0, nullptr);
                iY += iLineHeight;
            }
        }
    }
    else {
        pPickRef = nullptr;
        SetTargetREFR(nullptr);
        // The line height is measured here and discarded.
        int32_t iNoRefFont = iDebugTextFont.Int() - 1;
        static_cast<void>(static_cast<int32_t>(FontManager::Instance()->GetFont(iNoRefFont)->GetBaseLine()));
        float fTop = static_cast<float>(iDebugTextTopBottomOffset.Int());
        DebugText::Instance(true)->Print("", 640.0f, fTop, 2, -1, -1.0f, 0, nullptr);
    }
    int32_t iLastY = iY;
    while (iY < ilastLoc) {
        DebugText::Instance(true)->Print("", 640.0f, static_cast<float>(iY), 2, -1, -1.0f, 0, nullptr);
        iY += iLineHeight;
    }
    ilastLoc = iLastY;
}

void InterfaceManager::OpenPipboy(void (*apCallback)(), Interface::enumMenus aeMenuToOpen) {
    if (!pPlayer->GetControlsDisabled(PlayerCharacter::DPC_PIPBOY) && !pPlayer->IsDead(false)) {
        int32_t iWant3rdPerson = pPlayer->GetWant3rdPerson();
        if (iWant3rdPerson == pPlayer->Is3rdPerson() &&
            !pPlayer->GetUnderAIControl() &&
            !pPlayer->GetTempPerson() &&
            (!HUDMainMenu::pme || !HUDMainMenu::pme->IsHotKeyWheelVisible()) &&
            TileIsAcceptingEvents(nullptr) &&
            !pPlayer->IsKnockedOut() &&
            !pVATS.eMode &&
            !bDelayVATSMenu &&
            GetCameraShakeMult() == 0.0 &&
            !pFaderManager->IsFaderVisible(FADER_TO_BLACK)) {
            if (aeMenuToOpen) {
                GetMenusRoot()->Set(Tile::kTileValue_MenuLevel, aeMenuToOpen);
            }
            if (ePipboyState == 3 && apCallback) {
                apCallback();
            }
            else {
                if (!ePipboyState) {
                    ePipboyState = PS_OPENING_SETUP;
                    pPipboyCallback = apCallback;
                }
            }
        }
    }
}

// Fills ConsolePickRefs with the references the cursor's ray picks, in pick order, unless they are the ones already
// listed in that order.
void InterfaceManager::GetRefsUnderCrosshair() {
    NiPoint3 kOrigin;
    NiPoint3 kDir;
    NiPick kPick(0, 8u);
    BSSimpleList<TESObjectREFR*> kPicked;
    bool bSameRefs = true;
    kPick.SetObserveAppCullFlag(true);
    kPick.SetTarget(TES::pInstance->pObjRoot);
    kPick.m_eSortType = NiPick::SORT;
    float fWorldFOV = pPlayer->GetWorldFOV();
    Main::GetWorldSceneGraph()->SetCameraFOV(fWorldFOV, false, nullptr, false);
    int32_t iCursorY = static_cast<int32_t>(sRealCursorPos.z);
    int32_t iCursorX = static_cast<int32_t>(sRealCursorPos.x);
    Main::GetWorldSceneGraph()->GetCamera()->WindowPointToRay(iCursorX, iCursorY, kOrigin, kDir, nullptr);
    if (kPick.PickObjects(kOrigin, kDir, false)) {
        // The node of ConsolePickRefs the next picked reference is compared with.
        BSSimpleList<TESObjectREFR*>* pListed = &ConsolePickRefs;
        for (uint32_t i = 0; i < kPick.m_pickResults.GetSize(); ++i) {
            TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(kPick.m_pickResults.GetAt(i)->GetAVObject());
            if (!kPicked.IsInList(pRef)) {
                kPicked.AddTail(pRef);
                if (pListed && pListed->m_item != pRef) {
                    bSameRefs = false;
                }
                else if (bSameRefs && pListed) {
                    pListed = pListed->m_pkNext;
                }
            }
        }
    }
    ConsolePickRefs.RemoveAll();
    iCurrentPickIndex = 0;
    if (!bSameRefs) {
        for (BSSimpleList<TESObjectREFR*>* pNode = &kPicked; pNode && pNode->m_item; pNode = pNode->m_pkNext) {
            ConsolePickRefs.AddTail(pNode->m_item);
        }
    }
    kPicked.RemoveAll();
}

int32_t InterfaceManager::GetShouldKeyBeRepeated(float afRepeatScale) {
    uint32_t uiTime = appTimer.iLastTime;
    if (fKeyRepeatTime.Float() < 0.0) {
        return 0;
    }
    char cKeyReleased = 0;
    if (iRepeatingKey != -0x7FFFFFFF || appMain->GetControls()->GetKeyboardButton(203, 0)) {
        if (iRepeatingKey != -0x7FFFFFFE || appMain->GetControls()->GetKeyboardButton(205, 0)) {
            if (iRepeatingKey != -0x7FFFFFFD || appMain->GetControls()->GetKeyboardButton(200, 0)) {
                if (iRepeatingKey == -0x7FFFFFFC && !appMain->GetControls()->GetKeyboardButton(208, 0)) {
                    cKeyReleased = 1;
                }
            }
            else {
                cKeyReleased = 1;
            }
        }
        else {
            cKeyReleased = 1;
        }
    }
    else {
        cKeyReleased = 1;
    }
    if (cKeyReleased) {
        iRepeatingKey = 0;
        return 0;
    }

    if (uKeyRepeatStartTime) {
        double dSinceRepeatStart = static_cast<double>(uiTime - uKeyRepeatStartTime);
        if (!(fKeyRepeatInterval.Float() / afRepeatScale > dSinceRepeatStart)) {
            uKeyRepeatStartTime = uiTime;
            return iRepeatingKey;
        }

        return 0;
    }

    double dSinceKeyDown = static_cast<double>(uiTime - uKeyDownTime);
    if (!(fKeyRepeatTime.Float() > dSinceKeyDown)) {
        uKeyRepeatStartTime = uiTime;
        return iRepeatingKey;
    }

    return 0;
}

void InterfaceManager::HandleMenuAndListboxScroll(Menu* apMenu, float aiID, Tile* apTarget) {
    float fCursorZ = sRealCursorPos.z / InterfaceManager::GetScreenAspect();
    if (InterfaceManager::IsPipboyMenuTopmost()) {
        fCursorZ = fCursorZ / GetMenuHeightRatio() - 145.0;
    }
    for (BSSimpleList<ListBoxBase*>* pNode = &InterfaceManager::xActiveListBoxes; pNode && pNode->m_item;
         pNode = pNode->m_pkNext) {
        ListBoxBase* pListBox = pNode->m_item;
        if (pListBox->GetHighlightedItem()) {
            Tile* pTile;
            uint32_t uiIndex = 0;
            while (true) {
                pTile = pNode->m_item->GetTileAt(uiIndex++, false);
                if (!pTile) {
                    break;
                }
                if (!(static_cast<double>(fCursorZ) < pTile->GetRealY())) {
                    double dRealY = pTile->GetRealY();
                    if (!(fCursorZ > pTile->GetFloat(4016) + dRealY)) {
                        break;
                    }
                }
            }
            pNode->m_item->Highlight(pTile);
        }
    }
    apMenu->DoWheelMove(std::bit_cast<int32_t>(aiID), apTarget);
}

int32_t InterfaceManager::GetCursorOffsetX() {
    return iCursorOffsetX;
}

bool InterfaceManager::IsAltDown() {
    return (this->iModifierKeys & MODIFIER_ALT) != 0;
}

bool InterfaceManager::IsShiftDown() {
    return (this->iModifierKeys & MODIFIER_SHIFT) != 0;
}

bool InterfaceManager::IsActivatePickFuzzy() {
    return bFuzzyActivatePick;
}

NiPoint3 InterfaceManager::GetActivatePickLocation() {
    return pActivatePickLocation;
}

void InterfaceManager::SetFirstChanceTextureRelease(bool abState) {
    bTextureRelease = abState;
}

char InterfaceManager::GetInventoryPageNumber() {
    return cInventoryPageNumber;
}

void InterfaceManager::SetTargetREFR(TESObjectREFR* apRef) {
    pPickRef = apRef;
}

bool InterfaceManager::IsMenuInStack(uint32_t aiMenuID) {
    for (uint32_t i = 0; i < 10; ++i) {
        if (GetEnterStack(i) == static_cast<int32_t>(aiMenuID)) {
            return true;
        }
    }
    return false;
}

void InterfaceManager::PreLoadMenus() {
    if (pPlayer && bFirstChanceLoad && !pDataHandler->GetSaveLoad()) {
        int32_t iMenuMode = cMenuMode;
        PreLoadMainMenus();
        cMenuMode = static_cast<eMenuModeType>(iMenuMode);
        bFirstChanceLoad = false;
    }
}

void InterfaceManager::ClearTargetREFR() {
    pPickRef = nullptr;
}

int32_t InterfaceManager::GetCursorX() {
    return static_cast<int32_t>(InterfaceManager::GetScreenWidth() / 2.0 + sCursorPos.x);
}

int32_t InterfaceManager::GetLastPipboyTab() {
    return static_cast<int32_t>(pMenusRoot->GetFloat(0x1771));
}

void InterfaceManager::SetCrosshairRef(intptr_t aiCrosshairRef) {
    pCrossHairRef = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(aiCrosshairRef));
}

void InterfaceManager::SetLockpickControlsDisable(bool abDisable) {
    pPlayer->SetLockpickControlsDisable(abDisable);
    pPlayer->SetControlsDisabled(abDisable, PlayerCharacter::DPC_MOVEMENT);
}

void InterfaceManager::Disable360Controller(bool abDisable) {
    *static_cast<SettingT<INIPrefSettingCollection>*>(&gs_bDisable360Controller) = abDisable;
    InterfaceManager::Instance()->HandleControllerStateChange(!abDisable);
    Controls* pControls = appMain->GetControls();
    pControls->bIsControllerDisabled = abDisable;
}

void InterfaceManager::FreeTimerList() {
    InterfaceManager::Timer* pNext = pTimers->pNext;
    while (pNext) {
        InterfaceManager::Timer* pTimer = pNext;
        pNext = pNext->pNext;
        ourFree(pTimer);
    }
    ourFree(pTimers);
    pTimers = nullptr;
}

void InterfaceManager::Free() {
    if (InterfaceManager::pInstance) {
        delete InterfaceManager::pInstance;
        InterfaceManager::pInstance = nullptr;
    }
}

TESObjectREFR* InterfaceManager::GetTelekinesisTarget() {
    if (!InterfaceManager::Instance()) {
        return nullptr;
    }
    if (!InterfaceManager::Instance()->IsDoneFirstInit()) {
        return nullptr;
    }
    return InterfaceManager::Instance()->pTelekinesisRef;
}

InterfaceManager* InterfaceManager::Instance() {
    return InterfaceManager::pInstance;
}

Tile* InterfaceManager::GetGlobalsTile() {
    return InterfaceManager::Instance()->GetStringRoot();
}

bool InterfaceManager::QLockpickControlsDisable() {
    return pPlayer->GetIsDebounceWASD();
}

bool InterfaceManager::Has360Controller() {
    if (!InterfaceManager::bHas360Controller) {
        return false;
    }
    // Calls the singleton getter and drops its result.
    static_cast<void>(InterfaceManager::Instance());
    return !static_cast<SettingT<INIPrefSettingCollection>&>(gs_bDisable360Controller);
}

float InterfaceManager::GetRealScreenWidth() {
    return static_cast<float>(Renderer::GetScreenWidth());
}

float InterfaceManager::GetRealScreenHeight() {
    return static_cast<float>(Renderer::GetScreenHeight());
}

float InterfaceManager::GetScreenSafeY() {
    uint32_t safeZone;
    if (Interface::IsWideScreen()) {
        safeZone = iSafeZoneYWide.UInt();
    }
    else {
        safeZone = iSafeZoneY.UInt();
    }
    return static_cast<float>(static_cast<int32_t>(safeZone));
}

float InterfaceManager::GetScreenSafeX() {
    uint32_t safeZone;
    if (IsEyefinity()) {
        return static_cast<float>(GetEyefinityScreenWidth() * 0.4000000059604645);
    }
    if (Interface::IsWideScreen()) {
        safeZone = iSafeZoneXWide.UInt();
    }
    else {
        safeZone = iSafeZoneX.UInt();
    }
    return static_cast<float>(static_cast<int32_t>(safeZone));
}

float InterfaceManager::GetScreenWidth() {
    const float width = static_cast<float>(Renderer::GetScreenWidth());
    const float height = static_cast<float>(Renderer::GetScreenHeight());
    if (height < width) {
        return static_cast<float>(width / height * 960.0);
    }
    return 1280.0f;
}

float InterfaceManager::GetScreenHeight() {
    const float width = static_cast<float>(Renderer::GetScreenWidth());
    const float height = static_cast<float>(Renderer::GetScreenHeight());
    if (width < height) {
        return static_cast<float>(height / width * 1280.0);
    }
    return 960.0f;
}

bool InterfaceManager::IsPipboyMenuTopmost() {
    if (MenuConsole::Instance(true)->IsVisible()) {
        return false;
    }
    const int32_t topMenu = InterfaceManager::Instance()->GetEnterStackTop();
    return topMenu == 0x3EB /* eStatsMenu */ ||
        topMenu == 0x3FF /* ePipBoyData */
        ||
        topMenu == 0x3EA /* eInventoryMenu */
        ||
        topMenu == 0x40B /* ePipBoyRepair */
        || topMenu == 0x425 /* eItemModMenu */ || topMenu == 1 /* eMainFour */;
}

Tile* InterfaceManager::GetOverTileTarget() {
    return InterfaceManager::Instance()->pOverTileTarget;
}

void InterfaceManager::SetOverTileTarget(Tile* apTile) {
    InterfaceManager::Instance()->pOverTileTarget = apTile;
}

Menu* InterfaceManager::GetOverTileMenu() {
    return InterfaceManager::Instance()->pOverTileMenu;
}

void InterfaceManager::SetOverTileMenu(Menu* apMenu) {
    InterfaceManager::Instance()->pOverTileMenu = apMenu;
}

// AutoMemContext as in UpdateCursor.
void InterfaceManager::NewTimer(void* apIndex, float afSecs) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INTERFACE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Interface\\InterfaceManager.cpp",
        5210
    );
    InterfaceManager::ClearTimer(apIndex);
    InterfaceManager::Timer* const timer = new InterfaceManager::Timer;
    timer->pIndex = static_cast<Menu*>(apIndex);
    timer->fEnd = BSMAX(afSecs, 0.0f);
    // Appended at the tail of the list.
    timer->pPrev = InterfaceManager::Instance()->pTimers->pPrev;
    InterfaceManager::Instance()->pTimers->pPrev->pNext = timer;
    InterfaceManager::Instance()->pTimers->pPrev = timer;
    pContext->~AutoMemContext();
}

void InterfaceManager::ClearTimer(void* apIndex) {
    InterfaceManager::Timer* timer = InterfaceManager::Instance()->pTimers->pNext;
    while (timer) {
        if (timer->pIndex == apIndex) {
            InterfaceManager::Timer* const next = timer->pNext;
            timer->pPrev->pNext = next;
            if (next) {
                next->pPrev = timer->pPrev;
            }
            if (!timer->pNext) {
                InterfaceManager::Instance()->pTimers->pPrev = timer->pPrev;
            }
            ourFree(timer);
            return;
        }
        timer = timer->pNext;
    }
}

float InterfaceManager::GetTimerPercent(void* apIndex) {
    for (InterfaceManager::Timer* timer = InterfaceManager::Instance()->pTimers->pNext; timer; timer = timer->pNext) {
        if (apIndex == timer->pIndex) {
            if (timer->fEnd > 0.0) {
                const float ratio = timer->fElapsed / timer->fEnd;
                return BSMAX(0.0f, BSMIN(1.0f, ratio));
            }
            return -1.0f;
        }
    }
    return 1.0f;
}

void InterfaceManager::SetCursorAlpha(int32_t iAlpha) {
    Tile* const cursor = InterfaceManager::Instance()->GetCursorTile();
    NiNode* const model = cursor ? cursor->GetModel() : nullptr;
    if (!model) {
        return;
    }
    NiAVObject* const geometry = model->GetAtChecked(0);
    if (!geometry) {
        return;
    }
    NiProperty* const material = geometry->GetProperty(NiMaterialProperty::GetType());
    if (material) {
        static_cast<NiMaterialProperty*>(material)->SetAlpha(static_cast<float>(iAlpha));
    }
}

bool InterfaceManager::IsInPipboyMenu() {
    return Interface::IsMenuIDVisible(Interface::eStatsMenu, 0) ||
        Interface::IsMenuIDVisible(Interface::eInventoryMenu, 0) ||
        Interface::IsMenuIDVisible(Interface::ePipBoyRepair, 0) ||
        Interface::IsMenuIDVisible(Interface::eItemModMenu, 0) ||
        Interface::IsMenuIDVisible(Interface::ePipBoyData, 0);
}

bool InterfaceManager::IsCurrentRenderedMenuTopmost() {
    if (!InterfaceManager::pInstance->pCurrentRenderedMenu) {
        return false;
    }
    if (InterfaceManager::IsInPipboyMenu()) {
        return InterfaceManager::IsPipboyMenuTopmost();
    }
    if (MenuConsole::Instance(true)->IsVisible()) {
        return false;
    }
    Tile* const topMenu = Menu::GetMenuByClass(InterfaceManager::Instance()->GetEnterStackTop());
    NiNode* const model = topMenu ? static_cast<Tile*>(topMenu)->GetModel() : nullptr;
    return model && static_cast<NiAVObject*>(model) == InterfaceManager::pInstance->pCurrentRenderedMenu->GetMenuRoot();
}

int32_t InterfaceManager::CheckMenuButton(Interface::enumGamepadButtons eAction, bool abUseThumbstickInput) {
    // The singleton getter is called before each Has360Controller test and its result dropped.
    static_cast<void>(InterfaceManager::Instance());
    if (!InterfaceManager::Has360Controller() || InterfaceManager::Instance()->bShowMouse) {
        return 1;
    }
    static_cast<void>(InterfaceManager::Instance());
    if (!InterfaceManager::Has360Controller() || InterfaceManager::Instance()->bShowMouse) {
        return -1;
    }
    // The XINPUT_GAMEPAD button mask of the action.
    int32_t buttonMask;
    switch (eAction) {
        case 1: buttonMask = 0x1; break;
        case 2: buttonMask = 0x2; break;
        case 3: buttonMask = 0x8; break;
        case 4: buttonMask = 0x4; break;
        case 5: buttonMask = 0x10; break;
        case 6: buttonMask = 0x20; break;
        case 9: buttonMask = 0x1000; break;
        case 10: buttonMask = 0x2000; break;
        case 11: buttonMask = 0x4000; break;
        case 12: buttonMask = 0x8000; break;
        case 15: buttonMask = 0x100; break;
        case 16: buttonMask = 0x200; break;
        case 17: buttonMask = 0x40; break;
        case 18: buttonMask = 0x80; break;
        default: buttonMask = 0; break;
    }
    // 0 held, 1 released, 2 pressed, -1 up. Actions 13 and 14 are the left and right triggers.
    if (eAction == 13) {
        if (pState.Gamepad.bLeftTrigger) {
            return lastControllerState.Gamepad.bLeftTrigger ? 0 : 2;
        }
        return lastControllerState.Gamepad.bLeftTrigger ? 1 : -1;
    }
    if (eAction == 14) {
        if (pState.Gamepad.bRightTrigger) {
            return lastControllerState.Gamepad.bRightTrigger ? 0 : 2;
        }
        return lastControllerState.Gamepad.bRightTrigger ? 1 : -1;
    }
    // The d-pad directions also read the left thumbstick past its dead zone.
    bool lastStick = false;
    bool currentStick = false;
    if (abUseThumbstickInput) {
        switch (buttonMask) {
            case 0x1:
                lastStick = lastControllerState.Gamepad.sThumbLY > 7849;
                currentStick = pState.Gamepad.sThumbLY > 7849;
                break;
            case 0x2:
                lastStick = lastControllerState.Gamepad.sThumbLY < -7849;
                currentStick = pState.Gamepad.sThumbLY < -7849;
                break;
            case 0x4:
                lastStick = lastControllerState.Gamepad.sThumbLX < -7849;
                currentStick = pState.Gamepad.sThumbLX < -7849;
                break;
            case 0x8:
                lastStick = lastControllerState.Gamepad.sThumbLX > 7849;
                currentStick = pState.Gamepad.sThumbLX > 7849;
                break;
            default: break;
        }
    }
    if (lastStick || currentStick) {
        if (lastStick && currentStick) {
            return 0;
        }
        if (!lastStick && currentStick) {
            return 2;
        }
        if (lastStick && !currentStick) {
            return 1;
        }
    }
    if (pState.Gamepad.wButtons & buttonMask) {
        return (lastControllerState.Gamepad.wButtons & buttonMask) ? 0 : 2;
    }
    return (lastControllerState.Gamepad.wButtons & buttonMask) ? 1 : -1;
}

void InterfaceManager::Initialize(uint32_t istage, bool abInitialLoad) {
    _XINPUT_STATE kState;
    InterfaceManager::bHas360Controller = XInputGetState(0, &kState) == 0;
    if (!InterfaceManager::pInstance) {
        InterfaceManager::pInstance = new InterfaceManager;
    }
    if (istage == 1) {
        InterfaceManager::pInstance->Init(abInitialLoad);
    }
    else if (istage == 2) {
        InterfaceManager::pInstance->InitSecond();
    }
    InterfaceManager::pInstance->iSoundPauses = 0;
    InterfaceManager::pInstance->iVoicePauses = 0;
}

float InterfaceManager::GetScreenLeft() {
    return static_cast<float>((-InterfaceManager::GetScreenWidth()) / 2.0);
}

float InterfaceManager::GetScreenRight() {
    return static_cast<float>(InterfaceManager::GetScreenWidth() / 2.0);
}

// Half the interface height, divided by the double 2.0 and rounded to a float local.
float InterfaceManager::GetScreenTop() {
    float fResult = static_cast<float>(InterfaceManager::GetScreenHeight() / 2.0);
    return fResult;
}

float InterfaceManager::GetScreenBottom() {
    return static_cast<float>(-InterfaceManager::GetScreenHeight() / 2.0);
}

int32_t InterfaceManager::GetMenuHeight() {
    return i768;
}

void InterfaceManager::PlayMenuSound(MENU_SOUNDS aeMenuSound) {
    BSSoundHandle kSoundHand;
    switch (aeMenuSound) {
        case 1: {
            kSoundHand = BSAudio::QInstance().GetSoundHandleByName("UIMenuOK", 289u);
            break;
        }
        case 2:
        case 20: {
            kSoundHand = BSAudio::QInstance().GetSoundHandleByName("UIMenuCancel", 289u);
            break;
        }
        case 3: {
            kSoundHand = BSAudio::QInstance().GetSoundHandleByName("UIMenuPrevNext", 289u);
            break;
        }
        case 4: {
            kSoundHand = BSAudio::QInstance().GetSoundHandleByName("UIMenuFocus", 289u);
            break;
        }
        case 8: {
            kSoundHand = BSAudio::QInstance().GetSoundHandleByName("UIPopUpQuestNew", 289u);
            break;
        }
        case 10:
        case 19: {
            kSoundHand = BSAudio::QInstance().GetSoundHandleByName("UIPopUpMessageGeneral", 289u);
            break;
        }
        case 21: {
            kSoundHand = BSAudio::QInstance().GetSoundHandleByName("UILevelUp", 289u);
            break;
        }
        case 36: {
            BSAudio* pAudio;
            pAudio = &BSAudio::QInstance();
            kSoundHand = pAudio->GetSoundHandleByName("UIMenuMode", 289u);
            break;
        }
        default: {
            return;
        }
    }
    kSoundHand.Play(false);
}

void InterfaceManager::ClearMenuButton(Interface::enumGamepadButtons eAction) {
    if (InterfaceManager::Has360Controller() && !InterfaceManager::Instance()->bShowMouse) {
        Controls* pControls = appMain->GetControls();
        int32_t iControllerKey = ConvertSpecialInputKeyToControllerKey(eAction);
        pControls->SyncLastButtonState(iControllerKey);
    }
}

void InterfaceManager::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void InterfaceManager::SetContainerMenuQueued(bool abQueued) {
    if (InterfaceManager::pInstance) {
        InterfaceManager::pInstance->bContainerOpenQueued = abQueued;
    }
}

bool InterfaceManager::IsPipboyClosed() {
    return InterfaceManager::Instance()->ePipboyState == 0;
}

float InterfaceManager::GetScreenAspect() {
    double realScreenHeight = InterfaceManager::GetRealScreenHeight();
    return static_cast<float>(realScreenHeight / InterfaceManager::GetScreenHeight());
}

void InterfaceManager::UpdatePipBoyInventoryEffects(
    Tile* pTileFade1,
    Tile* pTileFade2,
    Tile* pTilePulseSTR,
    ItemChange* pItemPulseSTRREQ,
    bool abSkill
) {
    float fMaxAlpha = 485.0f;
    float fMinAlpha = -230.0f;
    float fAlphaFadeSpeed = 0.25f;
    float fAlphaDPSdisp = InterfaceManager::fCurrAlphaDPS;
    BSClamp(fAlphaDPSdisp, 0.0f, 255.0f);
    float fAlphaDAMdisp = 255.0 - fAlphaDPSdisp;
    BSClamp(fAlphaDAMdisp, 0.0f, 255.0f);
    if (pTileFade1 && pTileFade2) {
        pTileFade1->Set(Tile::kTileValue_Alpha, fAlphaDAMdisp, true);
        pTileFade2->Set(Tile::kTileValue_Alpha, fAlphaDPSdisp, true);
    }
    if (pTilePulseSTR && pItemPulseSTRREQ && !abSkill) {
        TESBoundObject* pObject = pItemPulseSTRREQ ? pItemPulseSTRREQ->pContainerObj : nullptr;
        float fStrMod = 0.0f;
        BGSEntryPoint::HandleEntryPoint(
            BGSEntryPoint::WEAPON_HANDLING_STRENGTH_BONUS,
            pPlayer,
            pItemPulseSTRREQ->pContainerObj,
            &fStrMod
        );
        fStrMod = pPlayer->GetClampedActorFloatValue(ActorValue::STRENGTH) + fStrMod;
        // The strength requirement is read as a signed value.
        if (pObject &&
            pObject->GetFormType() == WEAP_ID &&
            fStrMod < static_cast<double>(
                          static_cast<int32_t>(static_cast<TESObjectWEAP*>(pObject)->TESObjectWEAP::GetStrRequired())
                      )) {
            pTilePulseSTR->Set(Tile::kTileValue_Alpha, fAlphaDAMdisp > 128.0 ? fAlphaDAMdisp : fAlphaDPSdisp, true);
        }
        else {
            pTilePulseSTR->Set(Tile::kTileValue_Alpha, 255.0f, true);
        }
    }
    else if (pTilePulseSTR && pItemPulseSTRREQ && abSkill) {
        TESBoundObject* pObject = pItemPulseSTRREQ ? pItemPulseSTRREQ->pContainerObj : nullptr;
        // The weapon's skill is read before the object's type is tested.
        TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(pObject);
        ActorValue::Index eSkill = pWeapon->TESObjectWEAP::GetWeaponSkill();
        float fSkill = pPlayer->GetActorFloatValue(eSkill);
        // The strength requirement is read as a signed value.
        if (pObject &&
            pObject->GetFormType() == WEAP_ID &&
            fSkill < static_cast<double>(static_cast<int32_t>(pWeapon->data.iSkillRequirement))) {
            pTilePulseSTR->Set(Tile::kTileValue_Alpha, fAlphaDAMdisp > 128.0 ? fAlphaDAMdisp : fAlphaDPSdisp, true);
        }
        else {
            pTilePulseSTR->Set(Tile::kTileValue_Alpha, 255.0f, true);
        }
    }
    uint32_t iLastDPSFadeTimeLocal = appTimer.iLastTime;
    uint32_t iCurrTimeDiff = iLastDPSFadeTimeLocal - InterfaceManager::iLastDPSFadeTime;
    InterfaceManager::iLastDPSFadeTime = iLastDPSFadeTimeLocal;
    if (InterfaceManager::bAlphaGoingDown) {
        InterfaceManager::fCurrAlphaDPS = InterfaceManager::fCurrAlphaDPS -
            static_cast<double>(iCurrTimeDiff) * fAlphaFadeSpeed;
    }
    else {
        InterfaceManager::fCurrAlphaDPS = static_cast<double>(iCurrTimeDiff) * fAlphaFadeSpeed +
            InterfaceManager::fCurrAlphaDPS;
    }
    BSClamp(InterfaceManager::fCurrAlphaDPS, fMinAlpha, fMaxAlpha);
    if (fMaxAlpha <= static_cast<double>(InterfaceManager::fCurrAlphaDPS)) {
        InterfaceManager::bAlphaGoingDown = true;
    }
    if (fMinAlpha >= static_cast<double>(InterfaceManager::fCurrAlphaDPS)) {
        InterfaceManager::bAlphaGoingDown = false;
    }
}

SystemColorManager::SystemColor::SystemColor(const char* apName, uint32_t aiID) : strXMLID() {
    strXMLID.SPrintF("&%s;", apName);
    strXMLID.MakeLower();
    // The constructor takes an unsigned ID and Tile::AddToTable an int: the ID is converted implicitly.
    Tile::AddToTable(strXMLID.pString, aiID);
}

SystemColorManager::SystemColor::~SystemColor() {}

bool SystemColorManager::SystemColor::IsHard() const {
    return false;
}

bool SystemColorManager::SystemColor::IsSoft() const {
    return false;
}

// Each channel is scaled by the double 255.0, stored as a float, rounded, and truncated through a 64-bit integer of
// which the low 32 bits are kept; blue first, then green, then red.
void SystemColorManager::SystemColor::SetColor(NiColorA* apColor) {
    float fBlue = apColor->b * 255.0;
    uint32_t uiBlue = static_cast<uint32_t>(static_cast<int64_t>(BSRoundToNearest(fBlue, 1.0f)));
    float fGreen = apColor->g * 255.0;
    uint32_t uiGreen = static_cast<uint32_t>(static_cast<int64_t>(BSRoundToNearest(fGreen, 1.0f)));
    float fRed = apColor->r * 255.0;
    uint32_t uiRed = static_cast<uint32_t>(static_cast<int64_t>(BSRoundToNearest(fRed, 1.0f)));
    uint32_t uiRGBA = MakeRGBA(uiRed, uiGreen, uiBlue, 255u);
    SetColor(uiRGBA);
}

const char* SystemColorManager::SystemColor::GetXMLID() {
    return strXMLID.String();
}

void SystemColorManager::SystemColor::operator delete(void* apMemory) {
    ourFree(apMemory);
}

SystemColorManager::SoftSystemColor::SoftSystemColor(
    const char* apName,
    SettingT<INIPrefSettingCollection>* apSetting,
    uint32_t aiID
)
  : SystemColor(apName, aiID), pSetting(apSetting) {}

SystemColorManager::SoftSystemColor::~SoftSystemColor() {}

uint32_t SystemColorManager::SoftSystemColor::GetColor() const {
    return pSetting->UInt();
}

void SystemColorManager::SoftSystemColor::SetColor(uint32_t aiColor) {
    pSetting->Int() = static_cast<int32_t>(aiColor);
}

bool SystemColorManager::SoftSystemColor::IsSoft() const {
    return true;
}

void SystemColorManager::SoftSystemColor::GetColor(NiColorA* apOut) {
    uint32_t uiRed;
    uint32_t uiGreen;
    uint32_t uiBlue;
    uint32_t uiAlpha;
    int32_t iColor = GetColor();
    SplitColorComponents(iColor, &uiRed, &uiGreen, &uiBlue, &uiAlpha);
    apOut->r = static_cast<double>(uiRed) * 0.003921569f;
    apOut->g = static_cast<double>(uiGreen) * 0.003921569f;
    apOut->b = static_cast<double>(uiBlue) * 0.003921569f;
}

SystemColorManager::HardSystemColor::HardSystemColor(const char* apName, uint32_t aiColor, uint32_t aiID)
  : SystemColor(apName, aiID), iRGB(aiColor) {}

SystemColorManager::HardSystemColor::~HardSystemColor() {}

bool SystemColorManager::HardSystemColor::IsHard() const {
    return true;
}

SystemColorManager::SystemColorManager() : SystemColorList(), iNextID(1) {
    Add("HUDMain", 255u, 182u, 66u, &SystemColorManager::uHUDColor, 1);
    Add("HUDAlt", 255u, 67u, 42u, 2);
    Add("Terminal", 33u, 231u, 121u, 3);
    Add("Pipboy", 255u, 182u, 66u, &SystemColorManager::uPipboyColor, 4);
    Add("MainMenu", 255u, 182u, 66u, -1);
    Add("System", 255u, 182u, 66u, -1);
}

SystemColorManager::SystemColor* SystemColorManager::Find(int32_t aiID) {
    NiTListItem<SystemColorManager::SystemColor*>* pIter = SystemColorList.m_pkHead;
    while (pIter) {
        SystemColorManager::SystemColor* pColor = SystemColorList.GetNext(pIter);
        if (pColor && Tile::TextToTrait(pColor->GetXMLID()) == aiID) {
            return pColor;
        }
    }
    return nullptr;
}

bool SystemColorManager::GetColor(int32_t aiID, NiColorA& aOutColor) {
    SystemColor* pColor = Find(aiID);
    if (!pColor) {
        return false;
    }
    // Every colour Find returns is read as a soft colour, through the base's virtual GetColor only.
    static_cast<SystemColorManager::SoftSystemColor*>(pColor)->GetColor(&aOutColor);
    return true;
}

bool SystemColorManager::SetColor(int32_t aiID, const NiColorA& aColor) {
    SystemColor* pColor = Find(aiID);
    if (!pColor) {
        return false;
    }
    pColor->SetColor(const_cast<NiColorA*>(&aColor));
    return true;
}

void SystemColorManager::Add(const char* apName, uint32_t aiRed, uint32_t aiGreen, uint32_t aiBlue, int32_t aiID) {
    if (aiID < iNextID) {
        aiID = iNextID;
    }
    SystemColorManager::HardSystemColor* pHardColor = new SystemColorManager::HardSystemColor(
        apName,
        MakeRGBA(aiRed, aiGreen, aiBlue, 255u),
        aiID
    );
    SystemColorManager::SystemColor* pColor = pHardColor;
    SystemColorList.AddTail(pColor);
    iNextID = aiID + 1;
}

void SystemColorManager::Add(
    const char* apName,
    uint32_t aiRed,
    uint32_t aiGreen,
    uint32_t aiBlue,
    SettingT<INIPrefSettingCollection>* apSetting,
    int32_t aiID
) {
    if (!apSetting->Int()) {
        uint32_t uiRGBA = MakeRGBA(aiRed, aiGreen, aiBlue, 255u);
        apSetting->Int() = static_cast<int32_t>(uiRGBA);
    }
    SystemColorManager::SoftSystemColor* pSoftColor = new SystemColorManager::SoftSystemColor(apName, apSetting, aiID);
    SystemColorManager::SystemColor* pColor = pSoftColor;
    SystemColorList.AddTail(pColor);
    iNextID = aiID + 1;
}

uint32_t SystemColorManager::GetColor(int32_t aiID) {
    SystemColorManager::SystemColor* pColor = Find(aiID);
    if (pColor) {
        return pColor->GetColor();
    }

    return 0;
}

void SystemColorManager::RemoveAll() {
    while (!SystemColorList.IsEmpty()) {
        SystemColorManager::SystemColor* pColor = SystemColorList.RemoveHead();
        if (pColor) {
            delete pColor;
        }
    }
}

bool SystemColorManager::SetColor(int32_t aiID, uint32_t aiColor) {
    SystemColorManager::SystemColor* pColor = Find(aiID);
    if (!pColor) {
        return false;
    }
    pColor->SetColor(aiColor);
    return true;
}

void SystemColorManager::operator delete(void* apMemory) {
    ourFree(apMemory);
}

SystemColorManager* SystemColorManager::GetInstance() {
    if (!g_sysColorManager) {
        SystemColorManager* pManager = new SystemColorManager;
        g_sysColorManager = pManager;
    }
    return g_sysColorManager;
}

void SystemColorManager::DeleteInstance() {
    g_sysColorManager->RemoveAll();
    if (g_sysColorManager) {
        delete g_sysColorManager;
    }
    g_sysColorManager = nullptr;
}

int32_t Controls::GetButtonState(int32_t aiDevice, uint8_t aucKey, CONTROL_TOGGLE_FLAGS aeFlags) {
    if (aiDevice <= 3) {
        if (aiDevice == 3) {
            return this->GetControllerButton(static_cast<GAMEPAD_BUTTON>(aucKey), aeFlags);
        }
        if (aiDevice == 0) {
            return this->GetKeyboardButton(aucKey, aeFlags);
        }
        if (aiDevice == 1) {
            return this->GetMouseButton(aucKey, aeFlags);
        }
    }
    else if (aiDevice == 255) {
        return 0;
    }
    if (aiDevice - 2 >= static_cast<int32_t>(this->uiJoyStickCount)) {
        return 0;
    }
    // Keys from 32 up are hat directions; the key's high word (always 0) is passed as the hat.
    if (aucKey < 32) {
        return this->GetJoyStickButton(aiDevice - 2, aucKey, aeFlags);
    }
    uint32_t uiKey = aucKey;
    return this->GetJoystickHat(aiDevice - 2, static_cast<uint16_t>(uiKey >> 16)) == static_cast<uint16_t>(uiKey);
}

float Tile::Value::QFloat() {
    return fValue;
}

bool Tile::GetNeedsNiUpdate() {
    return bNeedsNiUpdate;
}

const char* TileImage::GetTypeName() {
    return "IMGE";
}

// NEEDS: ImageSpaceEffectMenuBG and its ImageSpaceEffect base need typed class declarations for GetEffect's
// base-to-derived conversion before calling SetBlendEnabled.
void ImageSpaceManager::SetMenuBGBlendEnabled(bool abEnabled) {
    ImageSpaceEffectMenuBG* pEffect = static_cast<ImageSpaceEffectMenuBG*>(GetEffect(EFFECT_MENU_BG));
    pEffect->SetBlendEnabled(abEnabled);
}

void ImageSpaceManager::SetMenuBGBlendValue(float afStrength) {
    static_cast<ImageSpaceEffectMenuBG*>(GetEffect(EFFECT_MENU_BG))->SetBlendValue(afStrength);
}

EditableTextField::EditableTextField()
  : sText(), sDisplayText(), iCurPos(0), iMaxPixelSize(static_cast<uint32_t>(-1)), iFontIdx(1), uLastBlinkTime(0),
    bCursorOn(false), bEditing(false) {}

bool EditableTextField::EditingText() {
    return bEditing;
}

void EditableTextField::SetMaxSize(int32_t iSize) {
    iMaxPixelSize = static_cast<uint32_t>(iSize - 5);
    if (static_cast<int32_t>(iMaxPixelSize) < 0) {
        InterfaceManager::Instance();
        iMaxPixelSize = static_cast<uint32_t>(static_cast<int32_t>(InterfaceManager::GetScreenWidth()));
    }
}

// The editing keys arrive as 0x80000000 + n: 0 backspace, 1 left, 2 right, 5 home, 6 end, 7 delete, 8 enter, 9 and 10
// only drop the clear-on-key state; 3, 4 and every other code are typed as a character. iCurPos and the length are
// compared signed.
void EditableTextField::HandleKey(int32_t iKey) {
    char acText[1024];
    if (!bEditing) {
        return;
    }
    if (sText.GetLength()) {
        ourStrcpy(acText, sText.String());
    }
    else {
        ourStrcpy(acText, "");
    }
    int32_t iLength = sText.GetLength();
    if (iKey == 9) {
        return;
    }
    bool bInsert = false;
    switch (iKey) {
        case INT32_MIN + 0:
            if (bClearOnKey) {
                iLength = 0;
                acText[iLength] = 0;
                iCurPos = 0;
                sText = acText;
                bClearOnKey = false;
            }
            else if (static_cast<int32_t>(iCurPos) > 0) {
                for (int32_t i = iCurPos - 1; i < iLength; ++i) {
                    acText[i] = acText[i + 1];
                }
                --iLength;
                --iCurPos;
                acText[iLength] = 0;
                sText = acText;
            }
            break;
        case INT32_MIN + 1:
            if (static_cast<int32_t>(iCurPos) > 0) {
                --iCurPos;
            }
            bClearOnKey = false;
            break;
        case INT32_MIN + 2:
            if (static_cast<int32_t>(iCurPos) < iLength) {
                ++iCurPos;
            }
            bClearOnKey = false;
            break;
        case INT32_MIN + 5:
            iCurPos = 0;
            bClearOnKey = false;
            break;
        case INT32_MIN + 6:
            iCurPos = iLength;
            bClearOnKey = false;
            break;
        case INT32_MIN + 7:
            if (bClearOnKey) {
                iLength = 0;
                acText[iLength] = 0;
                iCurPos = 0;
                sText = acText;
                bClearOnKey = false;
            }
            else if (static_cast<int32_t>(iCurPos) < iLength) {
                for (int32_t i = iCurPos; i < iLength; ++i) {
                    acText[i] = acText[i + 1];
                }
                --iLength;
                acText[iLength] = 0;
                sText = acText;
            }
            break;
        case INT32_MIN + 8:
            if (bClearOnKey) {
                iLength = 0;
                acText[iLength] = 0;
                iCurPos = 0;
                sText = acText;
                bClearOnKey = false;
            }
            else {
                bEditing = false;
            }
            break;
        case INT32_MIN + 9:
        case INT32_MIN + 10: bClearOnKey = false; break;
        default: bInsert = true; break;
    }
    if (!bInsert) {
        return;
    }
    if (bClearOnKey) {
        iLength = 0;
        acText[iLength] = 0;
        iCurPos = 0;
        sText = acText;
        bClearOnKey = false;
    }
    int32_t i;
    for (i = iLength; i > static_cast<int32_t>(iCurPos); --i) {
        acText[i] = acText[i - 1];
    }
    acText[i] = static_cast<char>(iKey);
    acText[iLength + 1] = 0;
    if (static_cast<int32_t>(iMaxPixelSize) == -1 || StringIsOk(acText)) {
        ++iLength;
        ++iCurPos;
        sText = acText;
    }
}

bool EditableTextField::StringIsOk(char* sString) {
    uint32_t uiFontIdx = iFontIdx;
    NiPoint3 kDimensions = FontManager::Instance()
                               ->CalculateStringDimensions(sString, uiFontIdx, std::numeric_limits<float>::max(), 0);
    return static_cast<int32_t>(kDimensions.x) <= static_cast<int32_t>(iMaxPixelSize);
}

void EditableTextField::SetText(const char* sDefaultText) {
    sText = sDefaultText;
    sDisplayText = sDefaultText;
}

void EditableTextField::SetEditing(bool bEditingText) {
    if (!bEditing && static_cast<uint8_t>(bEditingText)) {
        iCurPos = sText.GetLength();
    }

    *reinterpret_cast<uint8_t*>(&bEditing) = static_cast<uint8_t>(bEditingText);
}

void EditableTextField::BlinkCursor() {
    uint32_t uiTime = appTimer.iLastTime;
    if (uiTime - uLastBlinkTime > 500) {
        // Operates on the raw byte: 1 when it is 0, else 0.
        *reinterpret_cast<uint8_t*>(&bCursorOn) = static_cast<uint8_t>(bCursorOn) == 0;
        uLastBlinkTime = uiTime;
    }
}

char* EditableTextField::GetDisplayText() {
    char cText[1024];
    char cDisplayText[1028];
    int32_t iLength = sText.GetLength();
    if (sText.GetLength()) {
        ourStrcpy(cText, sText.pString);
    }
    else {
        ourStrcpy(cText, "");
    }
    int32_t iIndex = 0;
    int32_t iOutIndex = 0;
    while (iIndex <= iLength) {
        if (bEditing && iIndex == iCurPos) {
            if (bCursorOn) {
                cDisplayText[iOutIndex] = 124;
            }
            else {
                cDisplayText[iOutIndex] = 127;
            }
            ++iOutIndex;
        }
        cDisplayText[iOutIndex++] = cText[iIndex++];
    }
    cDisplayText[iOutIndex] = 0;
    sDisplayText = cDisplayText;
    if (!sDisplayText.GetLength()) {
        sDisplayText = " ";
    }
    return sDisplayText.pString;
}

const char* EditableTextField::GetEnteredText() {
    return sText.String();
}

bool FOPipboyManager::QPipboyLightFading() {
    return this->bLightEffectFading;
}

NiAVObject* FORenderedMenu::GetMenuRoot() {
    return static_cast<NiAVObject*>(spMenuRoot.m_pObject);
}

bool Interface::IsMenuQueuedToOpen() {
    return Interface::QueuedMenuOpen.eOpenType != Interface::OPEN_NONE;
}

void LoadingMenu::KillThread() {
    if (LoadingMenu::pLoadingThread) {
        delete LoadingMenu::pLoadingThread;
    }
    LoadingMenu::pLoadingThread = nullptr;
}

// Only the low byte of the tab is read, sign-extended.
void StatsMenu::SetCurrentTab(char acTab) {
    SetCurrentPage(static_cast<StatsMenu::Page>(static_cast<int8_t>(acTab)));
}

bool StatsMenu::IsTargetingBodyPart() {
    return StatsMenu::pMe && StatsMenu::pMe->bLimbMode;
}

void NiAlphaAccumulator::SetInterfaceSort(bool abInterface) {
    m_bInterfaceSort = abInterface;
}

int32_t NiSemaphore::Signal() {
    NiAtomicIncrement(m_iCount);
    m_hSemaphore->Release();
    return m_iCount;
}

int32_t NiSemaphore::Wait(uint32_t aiTime) {
    if (m_hSemaphore->Wait(aiTime) == BSSystemWait::TIMED_OUT) {
        return 1;
    }

    return NiAtomicDecrement(m_iCount);
}
