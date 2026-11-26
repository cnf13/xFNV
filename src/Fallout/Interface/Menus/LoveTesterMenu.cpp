#include "Fallout/Interface/Menus/LoveTesterMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSDevices/Controls.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/Setting.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/MenuManager.h"
#include "Fallout/Interface/Menus/CreditsMenu.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/SPECIALBookMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Interface/Menus/TraitMenu.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFileEntry.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMultiTargetTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SourceTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/ehvec.h"

#include <cmath>
#include <cstdarg>
#include <cstdint>

NiPointer<NiCamera> LoveTesterMenu::sp3DCamera;

LoveTesterMenu::LoveTesterButton::LoveTesterButton(
    ButtonClickFunc apfnButtonClick,
    RefreshFunc apfnRefresh,
    const char* apName
)
  : pfButtonClick(apfnButtonClick), pfRefresh(apfnRefresh) {
    NiFixedString kName(apName);
    spButton = LoveTesterMenu::pInstance->pDynamicGeometry->GetObjectByName(kName);
    NiAVObject* pObj = LoveTesterMenu::pInstance->pDynamicGeometry->GetObjectByName(kName);
    NiProperty* pProperty = pObj->GetProperty(NiProperty::SHADE);
    pImage = nullptr;
    if (pProperty &&
        static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType >=
            BSShaderManager::BSSM_PROPERTYTYPE_PPLIGHTING_FIRST &&
        static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType <=
            BSShaderManager::BSSM_PROPERTYTYPE_PPLIGHTING_LAST) {
        pImage = static_cast<BSShaderPPLightingProperty*>(pProperty);
    }
}

LoveTesterMenu::LoveTesterButton::~LoveTesterButton() {}

LoveTesterMenu::ActorValueButton::ActorValueButton(
    ButtonClickFunc apfnButtonClick,
    RefreshFunc apfnRefresh,
    const char* apName,
    int32_t aiActorValue,
    int32_t aiCount,
    ...
)
  : LoveTesterButton(apfnButtonClick, apfnRefresh, apName), eActorValue(static_cast<ActorValue::Index>(aiActorValue)) {
    va_list args;
    va_start(args, aiCount);
    for (int32_t i = 0; i < aiCount; ++i) {
        AffectedFields.Add(va_arg(args, LoveTesterTextItem*));
    }
    va_end(args);
}

LoveTesterMenu::LoveTesterTextItem::LoveTesterTextItem(
    RefreshFieldFunc apRefreshFieldCallback,
    const char* apTextObjectName1,
    const char* apTextObjectName2,
    int32_t aeActorValue,
    bool bElementOnDynamicGeometry,
    int32_t anThresholdToLightAt
)
  : pfRefreshField(apRefreshFieldCallback), eActorValue(static_cast<ActorValue::Index>(aeActorValue)),
    nThreshold(anThresholdToLightAt) {
    NiAVObject* pGeometry = bElementOnDynamicGeometry ? LoveTesterMenu::pInstance->pDynamicGeometry
                                                      : LoveTesterMenu::pInstance->pCabinetGeometry;
    const char* apNames[2] = {apTextObjectName1, apTextObjectName2};
    for (int32_t i = 0; i < 2; ++i) {
        pTextObj[i] = nullptr;
        pText[i] = nullptr;
        // Only the second name may be null: the first one is looked up unconditionally.
        if (i > 0 && !apNames[i]) {
            continue;
        }
        pTextObj[i] = pGeometry->GetObjectByName(NiFixedString(apNames[i]));
        NiProperty* pProperty = pTextObj[i]->GetProperty(NiProperty::SHADE);
        if (pProperty &&
            static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType >=
                BSShaderManager::BSSM_PROPERTYTYPE_PPLIGHTING_FIRST &&
            static_cast<NiShadeProperty*>(pProperty)->iShaderPropertyType <=
                BSShaderManager::BSSM_PROPERTYTYPE_PPLIGHTING_LAST) {
            pText[i] = static_cast<BSShaderPPLightingProperty*>(pProperty);
        }
    }
}

LoveTesterMenu::LoveTesterLightBulbGlows::LoveTesterLightBulbGlows(NiAVObject* a_GlowCard, int32_t a_nValueToGlowAt)
  : m_GlowCard(a_GlowCard), m_nValueToGlowAt(a_nValueToGlowAt) {}

LoveTesterMenu::LoveTesterPage::LoveTesterPage() {}

LoveTesterMenu::~LoveTesterMenu() {}

void LoveTesterMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (aiID >= 0 && aiID < 8) {
        pTiles[aiID] = apTile;
    }
}

void LoveTesterMenu::DoClick(int32_t aiID, Tile* apTarget) {
    switch (aiID) {
        case 0:
        case 1: {
            if (bAnimFinished) {
                if (aiID) {
                    TurnPage(false);
                }
                else {
                    TurnPage(true);
                }
                RefreshPage();
            }
            break;
        }
        case 2:
        case 3: {
            int32_t iOldValue = 0;
            int32_t iNewValue = 0;
            if (GetSelectedSPECIAL() == ActorValue::VALUE_NONE) {
                if (InterfaceManager::Has360Controller() && eState == LoveTesterMenu::LTMS_SUMMARY) {
                    ActorValue::Index eAV = static_cast<ActorValue::Index>(eXBoxIndexState + 4);
                    iOldValue = pPlayer->GetPermanentActorValue(eAV);
                    if (aiID == 2) {
                        if (PlayerCharacter::GetSumOfSPECIALStats() < iAvailablePoints) {
                            pPlayer->SetActorBaseValue(eAV, BSMIN(iOldValue + 1, 10));
                        }
                    }
                    else {
                        pPlayer->SetActorBaseValue(eAV, BSMAX(iOldValue - 1, 1));
                    }
                    iNewValue = pPlayer->GetBaseActorValue(eAV);
                }
            }
            else {
                ActorValue::Index eAV = GetSelectedSPECIAL();
                iOldValue = pPlayer->GetPermanentActorValue(eAV);
                if (aiID == 2) {
                    if (PlayerCharacter::GetSumOfSPECIALStats() < iAvailablePoints) {
                        int32_t iValue = BSMIN(iOldValue + 1, 10);
                        eAV = GetSelectedSPECIAL();
                        pPlayer->SetActorBaseValue(eAV, iValue);
                    }
                }
                else {
                    int32_t iValue = BSMAX(iOldValue - 1, 1);
                    eAV = GetSelectedSPECIAL();
                    pPlayer->SetActorBaseValue(eAV, iValue);
                }
                eAV = GetSelectedSPECIAL();
                iNewValue = pPlayer->GetBaseActorValue(eAV);
            }
            if (iOldValue > iNewValue) {
                BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialNumberDown", 289u).Play(false);
            }
            if (iOldValue < iNewValue) {
                BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialNumber", 289u).Play(false);
            }
            RefreshPage();
            break;
        }
        case 4: {
            if (PlayerCharacter::GetSumOfSPECIALStats() == iAvailablePoints) {
                BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialNumberFinished", 289u).Play(false);
                Close();
            }
            else {
                BSAudio::QInstance().GetSoundHandleByName("UIActivateNothing", 289u).Play(false);
            }
            break;
        }
        case 5: {
            if (eState <= LoveTesterMenu::LTMS_SUMMARY) {
                InterfaceManager* pim = InterfaceManager::Instance();
                int32_t iCursorX = static_cast<int32_t>(pim->sRealCursorPos.x);
                int32_t iCursorY = static_cast<int32_t>(pim->sRealCursorPos.z);
                if (BSShaderManager::bLetterbox) {
                    // The cursor's y in the letterboxed viewport: the back buffer's height times the viewport's bottom,
                    // plus the viewport's height fraction times y. The bottom is read before the virtual call.
                    NiRenderer* pRenderer = NiRenderer::GetRenderer();
                    double dBottom = BSShaderManager::kFramebufferViewport.m_bottom;
                    uint32_t uiHeight = pRenderer->GetDefaultBackBuffer()->m_uiHeight;
                    iCursorY = static_cast<int32_t>(
                        static_cast<double>(uiHeight) * dBottom +
                        (BSShaderManager::kFramebufferViewport.m_top - BSShaderManager::kFramebufferViewport.m_bottom) *
                            static_cast<double>(iCursorY)
                    );
                }
                for (int32_t iIndex = 0; iIndex < Pages[eState].Buttons.iSize; ++iIndex) {
                    NiPoint3 dir;
                    NiPoint3 origin;
                    NiPick activatePick(0, 8u);
                    activatePick.SetObserveAppCullFlag(true);
                    activatePick.SetTarget(Pages[eState].Buttons[iIndex]->spButton.m_pObject);
                    activatePick.m_eSortType = NiPick::SORT;
                    activatePick.SetIsReturnTexture(true);
                    LoveTesterMenu::sp3DCamera->WindowPointToRay(iCursorX, iCursorY, origin, dir, nullptr);
                    if (activatePick.PickObjects(origin, dir, false)) {
                        uint32_t uiPickIndex = 0;
                        if (activatePick.m_pickResults.GetAt(uiPickIndex)) {
                            // Fetches the argument before the callback.
                            LoveTesterButton* pButton = Pages[eState].Buttons[iIndex];
                            Pages[eState].Buttons[iIndex]->pfButtonClick(pButton);
                            break;
                        }
                    }
                }
                for (int32_t iIndex = 0; iIndex < GlobalPage.Buttons.iSize; ++iIndex) {
                    NiPoint3 dir;
                    NiPoint3 origin;
                    NiPick activatePick(0, 8u);
                    activatePick.SetObserveAppCullFlag(true);
                    activatePick.SetTarget(GlobalPage.Buttons[iIndex]->spButton.m_pObject);
                    activatePick.m_eSortType = NiPick::SORT;
                    activatePick.SetIsReturnTexture(true);
                    LoveTesterMenu::sp3DCamera->WindowPointToRay(iCursorX, iCursorY, origin, dir, nullptr);
                    if (activatePick.PickObjects(origin, dir, false)) {
                        uint32_t uiPickIndex = 0;
                        if (activatePick.m_pickResults.GetAt(uiPickIndex)) {
                            LoveTesterButton* pButton = GlobalPage.Buttons[iIndex];
                            GlobalPage.Buttons[iIndex]->pfButtonClick(pButton);
                            return;
                        }
                    }
                }
            }
            break;
        }
        case 6: {
            if (eState == LoveTesterMenu::LTMS_SUMMARY && eXBoxIndexState > LoveTesterMenu::LTMS_STRENGTH) {
                eXBoxIndexState = static_cast<LoveTesterMenu::LOVE_TESTER_MENU_STATE>(eXBoxIndexState - 1);
                BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialFocus", 289u).Play(false);
            }
            RefreshPage();
            break;
        }
        case 7: {
            if (eState == LoveTesterMenu::LTMS_SUMMARY && eXBoxIndexState < LoveTesterMenu::LTMS_LUCK) {
                eXBoxIndexState = static_cast<LoveTesterMenu::LOVE_TESTER_MENU_STATE>(eXBoxIndexState + 1);
                BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialFocus", 289u).Play(false);
            }
            RefreshPage();
            break;
        }
        default: {
            break;
        }
    }
}

void LoveTesterMenu::DoIdle() {
    if (Interface::IsTopMenuID(Interface::eLoveTesterMenu)) {
        Controls* pControls = appMain->GetControls();
        int32_t iKey = 30;
        if (!BSstrcmp(sLanguage.String(), "FRENCH")) {
            iKey = 16;
        }
        if (pControls->GetKeyboardButton(iKey, 1)) {
            DoClick(4, nullptr);
        }
        else {
            NiTimeController* pSpecialBookGeomController = pDynamicGeometry->GetControllers();
            if (pSpecialBookGeomController) {
                NiControllerManager* pSpecialBookGeomControllerManager = static_cast<NiControllerManager*>(
                    pSpecialBookGeomController
                );
                NiControllerSequence* pSeq = nullptr;
                if (!bLoveTesterAnimForward || eState > LoveTesterMenu::LTMS_START) {
                    const char* pcString = bLoveTesterAnimForward ? LoveTesterReverseAnimations[eState - 1]
                                                                  : LoveTesterAnimations[eState];
                    NiFixedString kName(pcString);
                    pSeq = pSpecialBookGeomControllerManager->GetSequenceByName(kName);
                }
                fFrameAnimTimer = appTimer.fDelta + fFrameAnimTimer;
                if (pSeq) {
                    pSeq->Update(fFrameAnimTimer, true);
                    double dLastTime = pSeq->m_fLastTime;
                    bAnimFinished = !(pSeq->m_fEndKeyTime > dLastTime) || eState <= LoveTesterMenu::LTMS_START;
                }
                else {
                    bAnimFinished = true;
                }
                while (pSpecialBookGeomController) {
                    if (NiObject::DynamicCastStatic(
                            &NiMultiTargetTransformController::ms_RTTI,
                            pSpecialBookGeomController
                        )) {
                        NiUpdateData UpdateData(fFrameAnimTimer, false, false);
                        pSpecialBookGeomController->Update(UpdateData);
                        break;
                    }
                    pSpecialBookGeomController = pSpecialBookGeomController->GetNext();
                }
            }
            NiUpdateData update(0.0f, false, false);
            pDynamicGeometry->Update(update);
        }
    }
}

bool LoveTesterMenu::DoKey(int32_t aiKey) {
    switch (aiKey) {
        case -0x7FFFFFFF:
        case -0x7FFFFFFE: {
            DoClick(aiKey == -0x7FFFFFFF, nullptr);
            return true;
        }
        case -0x7FFFFFFD:
        case 43:
        case 61: {
            ActorValue::Index eSPECIAL = GetSelectedSPECIAL();
            if (eSPECIAL != ActorValue::VALUE_NONE &&
                PlayerCharacter::GetSumOfSPECIALStats() < iAvailablePoints &&
                pPlayer->GetPermanentActorValue(eSPECIAL) < 10) {
                DoClick(2, nullptr);
                return true;
            }
            return false;
        }
        case -0x7FFFFFFC:
        case 45:
        case 95: {
            ActorValue::Index eSPECIAL = GetSelectedSPECIAL();
            if (eSPECIAL != ActorValue::VALUE_NONE && pPlayer->GetPermanentActorValue(eSPECIAL) > 1) {
                DoClick(3, nullptr);
                return true;
            }
            return false;
        }
        default: {
            return false;
        }
    }
}

int32_t LoveTesterMenu::GetClass() {
    return 1074;
}

bool LoveTesterMenu::DoGamepad(int32_t aiButton, float afValue) {
    bool bResult;
    if (!Interface::IsTopMenuID(Interface::eLoveTesterMenu)) {
        return false;
    }
    ActorValue::Index eAV = GetSelectedSPECIAL();
    switch (aiButton) {
        case 3: {
            if (eState != LoveTesterMenu::LTMS_SUMMARY) {
                bResult = false;
                break;
            }
            [[fallthrough]];
        }
        case 1: {
            if (eState == LoveTesterMenu::LTMS_SUMMARY) {
                bResult = false;
                break;
            }
            [[fallthrough]];
        }
        case 9: {
            if (eAV != ActorValue::VALUE_NONE &&
                PlayerCharacter::GetSumOfSPECIALStats() < iAvailablePoints &&
                pPlayer->GetPermanentActorValue(eAV) < 10) {
                DoClick(2, nullptr);
            }
            bResult = true;
            break;
        }
        case 4: {
            if (eState != LoveTesterMenu::LTMS_SUMMARY) {
                bResult = false;
                break;
            }
            [[fallthrough]];
        }
        case 2: {
            if (eState == LoveTesterMenu::LTMS_SUMMARY) {
                bResult = false;
                break;
            }
            [[fallthrough]];
        }
        case 10: {
            if (eAV != ActorValue::VALUE_NONE && pPlayer->GetPermanentActorValue(eAV) > 1) {
                DoClick(3, nullptr);
            }
            bResult = true;
            break;
        }
        default: {
            return false;
        }
    }
    return bResult;
}

bool LoveTesterMenu::TurnPage(bool bForward) {
    BSSoundHandle soundHand;
    bool bRunAnim = false;
    if (!bForward) {
        bLoveTesterAnimForward = false;
        if (eState <= LoveTesterMenu::LTMS_START) {
            return false;
        }
        eState = static_cast<LoveTesterMenu::LOVE_TESTER_MENU_STATE>(eState - 1);
        bRunAnim = true;
        soundHand = BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialPageTurn", 289u);
        soundHand.Play(false);
    }
    else {
        bLoveTesterAnimForward = true;
        if (eState >= LoveTesterMenu::LTMS_SUMMARY) {
            if (PlayerCharacter::GetSumOfSPECIALStats() == iAvailablePoints) {
                BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialNumberFinished", 289u).Play(false);
                Close();
            }
            else {
                BSAudio::QInstance().GetSoundHandleByName("UIActivateNothing", 289u).Play(false);
            }
            return false;
        }
        eState = static_cast<LoveTesterMenu::LOVE_TESTER_MENU_STATE>(eState + 1);
        bRunAnim = true;
        soundHand = BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialPageTurn", 289u);
        soundHand.Play(false);
    }
    if (bRunAnim) {
        NiTimeController* pSPECIALBookGeomController = LoveTesterMenu::pInstance->pDynamicGeometry->GetController(
            &NiControllerManager::ms_RTTI
        );
        if (pSPECIALBookGeomController) {
            NiControllerManager* pSPECIALBookGeomControllerManager = static_cast<NiControllerManager*>(
                pSPECIALBookGeomController
            );
            NiControllerSequence* pSeq = nullptr;
            if (!bLoveTesterAnimForward || eState > LoveTesterMenu::LTMS_START) {
                const char* pcString = bLoveTesterAnimForward ? LoveTesterReverseAnimations[eState - 1]
                                                              : LoveTesterAnimations[eState];
                NiFixedString kName(pcString);
                pSeq = pSPECIALBookGeomControllerManager->GetSequenceByName(kName);
            }
            if (pSeq) {
                pSPECIALBookGeomControllerManager->DeactivateAll(0.0f);
                pSPECIALBookGeomControllerManager->ActivateSequence(pSeq, 0, true, 1.0f, 0.0f, nullptr);
                pSPECIALBookGeomControllerManager->SetActive(true);
                pSeq->ResetSequence();
            }
        }
    }
    return true;
}

void LoveTesterMenu::RefreshPage() {
    if (eState <= LoveTesterMenu::LTMS_SUMMARY) {
        if (LoveTesterMenu::pInstance) {
            int32_t iSPECIALValue = 0;
            if (LoveTesterMenu::pInstance->GetSelectedSPECIAL() != ActorValue::VALUE_NONE) {
                ActorValue::Index eSPECIAL = LoveTesterMenu::pInstance->GetSelectedSPECIAL();
                iSPECIALValue = pPlayer->GetPermanentActorValue(eSPECIAL);
            }
            for (LoveTesterLightBulbGlows* pGlow : LoveTesterMenu::pInstance->LightBulbGlows) {
                if (pGlow->m_GlowCard) {
                    pGlow->m_GlowCard->SetAppCulled(iSPECIALValue < pGlow->m_nValueToGlowAt);
                }
            }
        }
        // Each callback's argument is fetched before the callback itself.
        for (uint32_t j = 0; j < Pages[eState].TextItems.iSize; ++j) {
            LoveTesterTextItem* pItem = Pages[eState].TextItems[j];
            Pages[eState].TextItems[j]->pfRefreshField(pItem);
        }
        for (uint32_t k = 0; k < Pages[eState].Buttons.iSize; ++k) {
            LoveTesterButton* pButton = Pages[eState].Buttons[k];
            Pages[eState].Buttons[k]->pfRefresh(pButton);
        }
        for (uint32_t m = 0; m < GlobalPage.TextItems.iSize; ++m) {
            LoveTesterTextItem* pItem = GlobalPage.TextItems[m];
            GlobalPage.TextItems[m]->pfRefreshField(pItem);
        }
        for (uint32_t n = 0; n < GlobalPage.Buttons.iSize; ++n) {
            LoveTesterButton* pButton = GlobalPage.Buttons[n];
            GlobalPage.Buttons[n]->pfRefresh(pButton);
        }
    }
}

// Loads the filament, number, message and button textures into the menu's smart pointers: each path is formatted into
// acName, swapped for the platform's and language's texture path into acPath, then loaded by TES.
void LoveTesterMenu::LoadButtonAndTextImages() {
    char acPlatform[8];
    char acName[260];
    char acPath[260];
    BSsprintf(acName, 260, "%s%s", "Textures\\architecture\\Goodsprings\\", "NV_Vigor-Tester_Filament-GlowOFF.dds");
    SwapPlatformLanguageTexturePath(acName, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spOnOffImage[0], false, false);
    BSsprintf(acName, 260, "%s%s", "Textures\\architecture\\Goodsprings\\", "NV_Vigor-Tester_Filament-GlowON.dds");
    SwapPlatformLanguageTexturePath(acName, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spOnOffImage[1], false, false);
    for (int32_t i = 0; i < 11; ++i) {
        BSsprintf(acName, 260, "%s%s%i.dds", "Textures\\terminals\\", "BBNumber", i);
        SwapPlatformLanguageTexturePath(acName, acPath, 260u);
        TES::pInstance->CreateTextureImage(acPath, spAVValues[i], false, false);
    }
    // BBMessage01..04 go to spPointsText[0..3].
    for (int32_t j = 1; j < 5; ++j) {
        BSsprintf(acName, 260, "%s%s%i.dds", "Textures\\architecture\\Goodsprings\\", "BBMessage0", j);
        SwapPlatformLanguageTexturePath(acName, acPath, 260u);
        TES::pInstance->CreateTextureImage(acPath, spPointsText[j - 1], false, false);
    }
    if (Interface::Has360Controller()) {
        BSstrcpy(acPlatform, 5, "XBOX");
    }
    else {
        BSstrcpy(acPlatform, 5, "PC");
    }
    BSsprintf(acName, 260, "%s%s\\%s.dds", "Textures\\terminals\\", acPlatform, "BBRTOff");
    SwapPlatformLanguageTexturePath(acName, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spNextPageButtonImage[1], false, false);
    BSsprintf(acName, 260, "%s%s\\%s.dds", "Textures\\terminals\\", acPlatform, "BBRTOn");
    SwapPlatformLanguageTexturePath(acName, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spNextPageButtonImage[0], false, false);
    BSsprintf(acName, 260, "%s%s\\%s.dds", "Textures\\terminals\\", acPlatform, "BBLTOff");
    SwapPlatformLanguageTexturePath(acName, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spPreviousPageButtonImage[0], false, false);
    // BBLTOn is formatted with the Goodsprings folder and no platform folder.
    BSsprintf(acName, 260, "%s%s.dds", "Textures\\architecture\\Goodsprings\\", "BBLTOn");
    SwapPlatformLanguageTexturePath(acName, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spPreviousPageButtonImage[1], false, false);
    BSsprintf(acName, 260, "%s%s\\%s.dds", "Textures\\terminals\\", acPlatform, "BBXOff");
    SwapPlatformLanguageTexturePath(acName, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spOpenCloseBookButtonImage[0], false, false);
    BSsprintf(acName, 260, "%s%s\\%s.dds", "Textures\\terminals\\", acPlatform, "BBXOn");
    SwapPlatformLanguageTexturePath(acName, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spOpenCloseBookButtonImage[1], false, false);
    BSsprintf(acName, 260, "%s%s.dds", "Textures\\terminals\\", "BBDecreaseOff");
    SwapPlatformLanguageTexturePath(acName, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spAVLeftArrowButtonImage[0], false, false);
    BSsprintf(acName, 260, "%s%s.dds", "Textures\\terminals\\", "BBDecreaseOn");
    SwapPlatformLanguageTexturePath(acName, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spAVLeftArrowButtonImage[1], false, false);
    BSsprintf(acName, 260, "%s%s.dds", "Textures\\terminals\\", "BBIncreaseOff");
    SwapPlatformLanguageTexturePath(acName, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spAVRightArrowButtonImage[0], false, false);
    BSsprintf(acName, 260, "%s%s.dds", "Textures\\terminals\\", "BBIncreaseOn");
    SwapPlatformLanguageTexturePath(acName, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spAVRightArrowButtonImage[1], false, false);
}

// Reads pInstance for the models, the lights and the global page's buttons, and this for the start of the animations,
// the third update and every later page.
void LoveTesterMenu::InitMachine() {
    pInstance->pDynamicGeometry = pModelLoader->LoadFile(
        "Meshes\\architecture\\Goodsprings\\NV_VitoMaticVigorTester_Activate.NIF",
        LOD_MULT_NONE,
        true,
        false,
        false,
        false
    );
    SwapPlatformLanguageTextures(pInstance->pDynamicGeometry);
    pInstance->pCabinetGeometry = pModelLoader->LoadFile(
        "Meshes\\architecture\\Goodsprings\\NV_VitoMaticVigorTester_Cabinet.NIF",
        LOD_MULT_NONE,
        true,
        false,
        false,
        false
    );
    SwapPlatformLanguageTextures(pInstance->pCabinetGeometry);

    // The machine faces the camera, tilted back: X by pi/2, then Z by pi, then X by 0.12566371.
    NiMatrix3 kRotation;
    NiMatrix3 kStep;
    kRotation.MakeXRotation(1.5707964f);
    kStep.MakeZRotation(3.1415927f);
    kRotation = kRotation * kStep;
    kStep.MakeXRotation(0.12566371f);
    kRotation = kRotation * kStep;

    // The width is compared with the double 640.0: a NaN width takes the else branch.
    const bool bLowResolution = InterfaceManager::GetRealScreenWidth() <= 640.0;
    if (bLowResolution) {
        pInstance->pDynamicGeometry->SetScale(1.0f);
        pInstance->pDynamicGeometry->SetTranslate(0.0f, -85.0f, -75.0f);
        pInstance->pDynamicGeometry->SetRotate(kRotation);
        pInstance->pCabinetGeometry->SetScale(1.0f);
        pInstance->pCabinetGeometry->SetTranslate(0.0f, -85.0f, -75.0f);
        pInstance->pCabinetGeometry->SetRotate(kRotation);
    }
    else {
        pInstance->pDynamicGeometry->SetScale(1.0f);
        pInstance->pDynamicGeometry->SetTranslate(0.0f, -85.0f, -85.0f);
        pInstance->pDynamicGeometry->SetRotate(kRotation);
        pInstance->pCabinetGeometry->SetScale(1.0f);
        pInstance->pCabinetGeometry->SetTranslate(0.0f, -85.0f, -85.0f);
        pInstance->pCabinetGeometry->SetRotate(kRotation);
    }
    NiUpdateData kDynamicUpdate(0.0f, false, false);
    NiUpdateData kCabinetUpdate(0.0f, false, false);
    pInstance->pDynamicGeometry->Update(kDynamicUpdate);
    pInstance->pCabinetGeometry->Update(kCabinetUpdate);
    pInstance->fFrameAnimTimer = appTimer.GetSeconds() + pInstance->fFrameAnimTimer;

    // Play the "Forward" sequence from its start.
    NiTimeController* pController = pInstance->pDynamicGeometry->GetController(&NiControllerManager::ms_RTTI);
    if (pController) {
        NiControllerManager* pManager = static_cast<NiControllerManager*>(pController);
        NiFixedString kForward("Forward");
        NiControllerSequence* pSequence = pManager->GetSequenceByName(kForward);
        if (pSequence) {
            pManager->DeactivateAll(0.0f);
            pManager->ActivateSequence(pSequence, 0, true, 1.0f, 0.0f, nullptr);
            pManager->SetActive(true);
            pSequence->ResetSequence();
        }
    }
    NiTimeController::StartAnimations(pDynamicGeometry);
    // The first multi-target transform controller from the manager on is updated once.
    while (pController) {
        if (NiObject::DynamicCastStatic(&NiMultiTargetTransformController::ms_RTTI, pController)) {
            NiUpdateData kControllerUpdate(0.0f, false, false);
            pController->Update(kControllerUpdate);
            break;
        }
        pController = pController->GetNext();
    }
    NiUpdateData kUpdate(0.0f, false, false);
    pDynamicGeometry->Update(kUpdate);

    // Light the scene from the model's own lights; without any, a white point light whose specular red holds the radius
    // (20 times the bound's radius).
    const NiBound* pBound = &pInstance->pDynamicGeometry->GetWorldBound();
    const float fRadius = static_cast<float>(pBound->m_fRadius * 20.0);
    // IsNode is called before the scene is fetched.
    NiNode* pDynamicNode = pInstance->pDynamicGeometry->IsNode();
    ShadowSceneNode* pScene = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D);
    if (!pScene->AddAllLightsFromNode_Recurse(pDynamicNode, fRadius)) {
        NiPointLight* pLight = new NiPointLight;
        NiPoint3 kTranslate;
        pLight->SetTranslate(*::new (&kTranslate) NiPoint3(0.0f, 0.0f, 0.0f));
        pLight->SetDiffuseColor(NiColor(0.85f, 0.85f, 0.85f));
        pLight->SetSpecularColor(NiColor(fRadius, 0.0f, 0.0f));
        pLight->SetDimmer(1.0f);
        NiUpdateData kLightUpdate(0.0f, false, false);
        pLight->Update(kLightUpdate);
        BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D)->AttachChild(pLight, true);
        BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D)->AddLight(pLight, false);
    }
    BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D)
        ->AttachChild(pInstance->pDynamicGeometry, true);
    BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D)
        ->AttachChild(pInstance->pCabinetGeometry, true);

    // The global page's buttons: the points to spend down and up, and the page arrows.
    LoveTesterButton* pButton = new ActorValueButton(
        &LoveTesterMenu::DecreaseCurrentActorValue,
        &LoveTesterMenu::RefreshLeftAVButton,
        "P1_Decrease_Btn:0",
        -1,
        0
    );
    pInstance->GlobalPage.Buttons.Add(pButton);
    pButton = new ActorValueButton(
        &LoveTesterMenu::IncreaseCurrentActorValue,
        &LoveTesterMenu::RefreshRightAVButton,
        "P1_Increase_Btn:0",
        -1,
        0
    );
    pInstance->GlobalPage.Buttons.Add(pButton);
    pButton = new LoveTesterButton(&LoveTesterMenu::NextPage, &LoveTesterMenu::RefreshAllDoneButton, "P1_RT_Btn:0");
    pInstance->GlobalPage.Buttons.Add(pButton);
    pButton = new LoveTesterButton(
        &LoveTesterMenu::PreviousPage,
        &LoveTesterMenu::RefreshPreviousPageButton,
        "P1_LT_Btn:0"
    );
    pInstance->GlobalPage.Buttons.Add(pButton);

    // The glow cards of the light bulbs: "P1_PointVal_0<value>_GLOW:<bulb>" and "P1_PointVal_0<value>_GLOW" for the
    // values 1 to 9, then those of 10. Each name is looked up again for the constructor.
    char acValueName[32];
    for (int32_t iValue = 1; iValue < 10; ++iValue) {
        for (int32_t iBulb = 0; iBulb < 10; ++iBulb) {
            BSsprintf(acValueName, 30, "P1_PointVal_0%i_GLOW:%i", iValue, iBulb);
            NiFixedString kName(acValueName);
            NiAVObject* pGlowCard = pInstance->pCabinetGeometry->GetObjectByName(kName);
            if (pGlowCard) {
                LoveTesterLightBulbGlows* pGlow = new LoveTesterLightBulbGlows(
                    pInstance->pCabinetGeometry->GetObjectByName(NiFixedString(acValueName)),
                    iValue
                );
                pInstance->LightBulbGlows.Add(pGlow);
            }
        }
        BSsprintf(acValueName, 30, "P1_PointVal_0%i_GLOW", iValue);
        NiFixedString kName(acValueName);
        NiAVObject* pGlowCard = pInstance->pCabinetGeometry->GetObjectByName(kName);
        if (pGlowCard) {
            LoveTesterLightBulbGlows* pGlow = new LoveTesterLightBulbGlows(
                pInstance->pCabinetGeometry->GetObjectByName(NiFixedString(acValueName)),
                iValue
            );
            pInstance->LightBulbGlows.Add(pGlow);
        }
    }
    char acTenName[32];
    for (int32_t iBulb = 0; iBulb < 10; ++iBulb) {
        BSsprintf(acTenName, 30, "P1_PointVal_10_GLOW:%i", iBulb);
        NiFixedString kName(acTenName);
        NiAVObject* pGlowCard = pInstance->pCabinetGeometry->GetObjectByName(kName);
        if (pGlowCard) {
            LoveTesterLightBulbGlows* pGlow = new LoveTesterLightBulbGlows(
                pInstance->pCabinetGeometry->GetObjectByName(NiFixedString(acTenName)),
                10
            );
            pInstance->LightBulbGlows.Add(pGlow);
        }
    }
    BSsprintf(acTenName, 30, "P1_PointVal_10_GLOW");
    {
        NiFixedString kName(acTenName);
        NiAVObject* pGlowCard = pInstance->pCabinetGeometry->GetObjectByName(kName);
        if (pGlowCard) {
            LoveTesterLightBulbGlows* pGlow = new LoveTesterLightBulbGlows(
                pInstance->pCabinetGeometry->GetObjectByName(NiFixedString(acTenName)),
                10
            );
            pInstance->LightBulbGlows.Add(pGlow);
        }
    }

    // The points left (two digits of the dynamic geometry, every page) and the book's look-inside button.
    LoveTesterTextItem* pPointsText = new LoveTesterTextItem(
        &LoveTesterMenu::RefreshTotalPointsValue,
        "P1_PointsRemainDigit1:0",
        "P1_PointsRemainDigit2:0",
        -1,
        true,
        0
    );
    GlobalPage.TextItems.Add(pPointsText);
    pButton = new LoveTesterButton(
        &LoveTesterMenu::NextPage,
        &LoveTesterMenu::RefreshAllDoneButton,
        "LookInside_Btn:0"
    );
    Pages[0].Buttons.Add(pButton);

    // The index page: per SPECIAL actor value (5 Strength to 11 Luck) its value text, then its decrease and increase
    // buttons, which refresh that text and the points left.
    LoveTesterTextItem* pValueText = new LoveTesterTextItem(
        &LoveTesterMenu::RefreshActorValue,
        "Index_StrengthPointVal:0",
        nullptr,
        5,
        true,
        0
    );
    Pages[8].TextItems.Add(pValueText);
    pButton = new ActorValueButton(
        &LoveTesterMenu::DecreaseActorValue,
        &LoveTesterMenu::RefreshLeftAVButton,
        "Index_StrengthDecrease_Btn:0",
        5,
        2,
        pValueText,
        pPointsText
    );
    Pages[8].Buttons.Add(pButton);
    pButton = new ActorValueButton(
        &LoveTesterMenu::IncreaseActorValue,
        &LoveTesterMenu::RefreshRightAVButton,
        "Index_StrengthIncrease_Btn:0",
        5,
        2,
        pValueText,
        pPointsText
    );
    Pages[8].Buttons.Add(pButton);

    pValueText = new LoveTesterTextItem(
        &LoveTesterMenu::RefreshActorValue,
        "Index_PerceptionPointVal:0",
        nullptr,
        6,
        true,
        0
    );
    Pages[8].TextItems.Add(pValueText);
    pButton = new ActorValueButton(
        &LoveTesterMenu::DecreaseActorValue,
        &LoveTesterMenu::RefreshLeftAVButton,
        "Index_PerceptionDecrease_Btn:0",
        6,
        2,
        pValueText,
        pPointsText
    );
    Pages[8].Buttons.Add(pButton);
    pButton = new ActorValueButton(
        &LoveTesterMenu::IncreaseActorValue,
        &LoveTesterMenu::RefreshRightAVButton,
        "Index_PerceptionIncrease_Btn:0",
        6,
        2,
        pValueText,
        pPointsText
    );
    Pages[8].Buttons.Add(pButton);

    pValueText = new LoveTesterTextItem(
        &LoveTesterMenu::RefreshActorValue,
        "Index_EndurancePointVal:0",
        nullptr,
        7,
        true,
        0
    );
    Pages[8].TextItems.Add(pValueText);
    pButton = new ActorValueButton(
        &LoveTesterMenu::DecreaseActorValue,
        &LoveTesterMenu::RefreshLeftAVButton,
        "Index_EnduranceDecrease_Btn:0",
        7,
        2,
        pValueText,
        pPointsText
    );
    Pages[8].Buttons.Add(pButton);
    pButton = new ActorValueButton(
        &LoveTesterMenu::IncreaseActorValue,
        &LoveTesterMenu::RefreshRightAVButton,
        "Index_EnduranceIncrease_Btn:0",
        7,
        2,
        pValueText,
        pPointsText
    );
    Pages[8].Buttons.Add(pButton);

    pValueText = new LoveTesterTextItem(
        &LoveTesterMenu::RefreshActorValue,
        "Index_CharismaPointVal:0",
        nullptr,
        8,
        true,
        0
    );
    Pages[8].TextItems.Add(pValueText);
    pButton = new ActorValueButton(
        &LoveTesterMenu::DecreaseActorValue,
        &LoveTesterMenu::RefreshLeftAVButton,
        "Index_CharismaDecrease_Btn:0",
        8,
        2,
        pValueText,
        pPointsText
    );
    Pages[8].Buttons.Add(pButton);
    pButton = new ActorValueButton(
        &LoveTesterMenu::IncreaseActorValue,
        &LoveTesterMenu::RefreshRightAVButton,
        "Index_CharismaIncrease_Btn:0",
        8,
        2,
        pValueText,
        pPointsText
    );
    Pages[8].Buttons.Add(pButton);

    pValueText = new LoveTesterTextItem(
        &LoveTesterMenu::RefreshActorValue,
        "Index_IntelligencePointVal:0",
        nullptr,
        9,
        true,
        0
    );
    Pages[8].TextItems.Add(pValueText);
    pButton = new ActorValueButton(
        &LoveTesterMenu::DecreaseActorValue,
        &LoveTesterMenu::RefreshLeftAVButton,
        "Index_IntelligenceDecrease_Btn:0",
        9,
        2,
        pValueText,
        pPointsText
    );
    Pages[8].Buttons.Add(pButton);
    pButton = new ActorValueButton(
        &LoveTesterMenu::IncreaseActorValue,
        &LoveTesterMenu::RefreshRightAVButton,
        "Index_IntelligenceIncrease_Btn:0",
        9,
        2,
        pValueText,
        pPointsText
    );
    Pages[8].Buttons.Add(pButton);

    pValueText = new LoveTesterTextItem(
        &LoveTesterMenu::RefreshActorValue,
        "Index_AgilityPointVal:0",
        nullptr,
        10,
        true,
        0
    );
    Pages[8].TextItems.Add(pValueText);
    pButton = new ActorValueButton(
        &LoveTesterMenu::DecreaseActorValue,
        &LoveTesterMenu::RefreshLeftAVButton,
        "Index_AgilityDecrease_Btn:0",
        10,
        2,
        pValueText,
        pPointsText
    );
    Pages[8].Buttons.Add(pButton);
    pButton = new ActorValueButton(
        &LoveTesterMenu::IncreaseActorValue,
        &LoveTesterMenu::RefreshRightAVButton,
        "Index_AgilityIncrease_Btn:0",
        10,
        2,
        pValueText,
        pPointsText
    );
    Pages[8].Buttons.Add(pButton);

    pValueText = new LoveTesterTextItem(
        &LoveTesterMenu::RefreshActorValue,
        "Index_LuckPointVal:0",
        nullptr,
        11,
        true,
        0
    );
    Pages[8].TextItems.Add(pValueText);
    pButton = new ActorValueButton(
        &LoveTesterMenu::DecreaseActorValue,
        &LoveTesterMenu::RefreshLeftAVButton,
        "Index_LuckDecrease_Btn:0",
        11,
        2,
        pValueText,
        pPointsText
    );
    Pages[8].Buttons.Add(pButton);
    pButton = new ActorValueButton(
        &LoveTesterMenu::IncreaseActorValue,
        &LoveTesterMenu::RefreshRightAVButton,
        "Index_LuckIncrease_Btn:0",
        11,
        2,
        pValueText,
        pPointsText
    );
    Pages[8].Buttons.Add(pButton);

    pButton = new LoveTesterButton(
        &LoveTesterMenu::AllDoneButtonClick,
        &LoveTesterMenu::RefreshAllDoneButton,
        "AllDone_Btn:0"
    );
    Pages[8].Buttons.Add(pButton);

    LoadButtonAndTextImages();
    pInstance->RefreshPage();
    NextPage(nullptr);
}

ActorValue::Index LoveTesterMenu::GetSelectedSPECIAL() {
    if (eState == LoveTesterMenu::LTMS_SUMMARY && InterfaceManager::Has360Controller()) {
        return static_cast<ActorValue::Index>(eXBoxIndexState + 4);
    }
    if (eState <= LoveTesterMenu::LTMS_START || eState >= LoveTesterMenu::LTMS_SUMMARY) {
        return ActorValue::VALUE_NONE;
    }
    return static_cast<ActorValue::Index>(eState + 4);
}

char LoveTesterMenu::CheckTilesExist(int32_t* apMissingIndex) {
    for (*apMissingIndex = 0; *apMissingIndex < 8; ++*apMissingIndex) {
        if (!pTiles[*apMissingIndex]) {
            return 0;
        }
    }
    return 1;
}

NiCamera* LoveTesterMenu::SetupCamera() {
    NiCamera* pCamera;
    NiRect<float> kViewPort;
    pCamera = new NiCamera;
    ::new (&kViewPort) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
    if (BSShaderManager::bLetterbox) {
        kViewPort = BSShaderManager::kFramebufferViewport;
    }
    pCamera->SetViewPort(kViewPort);
    NiFrustum kFrustum(false);
    kFrustum.m_fFar = Main::GetWorldSceneGraph()->GetFarDistance();
    kFrustum.m_fNear = BSSceneGraph::fNearDistance.Float();
    pCamera->SetMaxFarNearRatio(kFrustum.m_fFar / kFrustum.m_fNear);
    int32_t iScreenHeight = BSShaderManager::bLetterbox ? BSShaderManager::iLetterboxHeight
                                                        : Renderer::GetScreenHeight();
    float fAspect = static_cast<double>(iScreenHeight) / static_cast<double>(Renderer::GetScreenWidth());
    float fHalfFOV = BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369 * 0.6499999761581421;
    kFrustum.m_fLeft = -std::tan(fHalfFOV) * 0.75f;
    kFrustum.m_fRight = std::tan(fHalfFOV) * 0.75f;
    double dTan = std::tan(fHalfFOV);
    kFrustum.m_fBottom = -dTan * fAspect * 0.75f;
    dTan = std::tan(fHalfFOV);
    kFrustum.m_fTop = dTan * fAspect * 0.75f;
    kFrustum.m_bOrtho = false;
    pCamera->SetViewFrustum(kFrustum);
    pCamera->SetTranslate(0.0f, 0.0f, 1.0f);
    NiUpdateData arData(0.0f, false, false);
    pCamera->Update(arData);
    NiPoint3 kUp(0.0f, 1.0f, 0.0f);
    NiPoint3 kLookAt(0.0f, 0.0f, 0.0f);
    pCamera->LookAtWorldPoint(kLookAt, kUp);
    pCamera->SetMinNearPlaneDist(1.0f);
    NiFixedString kName("Surgery3DCamera");
    pCamera->SetName(kName);
    return pCamera;
}

void LoveTesterMenu::NextPage([[maybe_unused]] LoveTesterMenu::LoveTesterButton* apButton, ...) {
    if (LoveTesterMenu::pInstance && LoveTesterMenu::pInstance->bAnimFinished) {
        LoveTesterMenu::pInstance->TurnPage(true);
        LoveTesterMenu::pInstance->RefreshPage();
    }
}

void LoveTesterMenu::Draw3DElements() {
    if (LoveTesterMenu::pInstance && LoveTesterMenu::sp3DCamera.m_pObject) {
        NiRenderer::GetRenderer()->ClearBuffer(nullptr, 4u);
        BSCullingProcess* pCuller = Main::GetWorldSceneGraph()->pCuller;
        pCuller->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
        NiCamera* pCamera = LoveTesterMenu::sp3DCamera.m_pObject;
        HandleEyeFinityFOVScale(pCamera, 0.65f);
        NiPointer<BSShaderAccumulator> spAccum(InterfaceManager::Instance()->Get3dAccumulator());
        pCuller->SetAccumulator(spAccum.m_pObject);
        BSShaderUtil::AccumulateScene(pCamera, BSShaderManager::GetShadowSceneNode(1), *pCuller);
        BSShaderUtil::RenderScene(pCamera, spAccum.m_pObject, false);
        pCuller->SetAccumulator(nullptr);
        pCuller->PopCullMode();
    }
}

int32_t LoveTesterMenu::GetClassStatic() {
    return 1074;
}

void LoveTesterMenu::Close() {
    Tile* pMenuTile = Menu::GetMenuByClass(LoveTesterMenu::GetClassStatic());
    if (pMenuTile) {
        pModelLoader->ReleaseFile("Meshes\\architecture\\Goodsprings\\NV_VitoMaticVigorTester_Activate.NIF");
        pModelLoader->ReleaseFile("Meshes\\architecture\\Goodsprings\\NV_VitoMaticVigorTester_Cabinet.NIF");
        BSShaderManager::GetShadowSceneNode(1)->RemoveAllChildren();
        BSShaderManager::GetShadowSceneNode(1)->RemoveAllLights();
        LoveTesterMenu::pInstance->pDynamicGeometry = nullptr;
        LoveTesterMenu::pInstance->pCabinetGeometry = nullptr;
        LoveTesterMenu::sp3DCamera = static_cast<NiCamera*>(nullptr);
        for (int32_t i = 0; i < 9; ++i) {
            for (LoveTesterMenu::LoveTesterButton* pButton : LoveTesterMenu::pInstance->Pages[i].Buttons) {
                if (pButton) {
                    delete pButton;
                }
            }
            for (LoveTesterMenu::LoveTesterTextItem* pItem : LoveTesterMenu::pInstance->Pages[i].TextItems) {
                ourFree(pItem);
            }
            LoveTesterMenu::pInstance->Pages[i].Buttons.Clear(true);
            LoveTesterMenu::pInstance->Pages[i].TextItems.Clear(true);
        }
        for (LoveTesterMenu::LoveTesterButton* pButton : LoveTesterMenu::pInstance->GlobalPage.Buttons) {
            if (pButton) {
                delete pButton;
            }
        }
        for (LoveTesterMenu::LoveTesterTextItem* pItem : LoveTesterMenu::pInstance->GlobalPage.TextItems) {
            ourFree(pItem);
        }
        LoveTesterMenu::pInstance->GlobalPage.Buttons.Clear(true);
        LoveTesterMenu::pInstance->GlobalPage.TextItems.Clear(true);
        LoveTesterMenu::pInstance->LightBulbs.Clear(true);
        LoveTesterMenu::pInstance->LightBulbGlows.Clear(true);
        if (LoveTesterMenu::pInstance) {
            pMenuTile->Set(Tile::kTileValue_DeleteOnFade, 1);
            LoveTesterMenu::pInstance->StartFadeOut();
        }
    }
}

// The text items' refresh callback: the player's permanent actor value picks the digit texture, set as the item's first
// text property's texture 0 of type 0.
void LoveTesterMenu::RefreshActorValue(LoveTesterTextItem* apTextItem) {
    // The ActorValueOwner base is reached without a null check: cast through a reference.
    int32_t iValue = static_cast<ActorValueOwner&>(*pPlayer).GetPermanentActorValue(apTextItem->eActorValue);
    NiSourceTexture* pTexture = pInstance->spAVValues[iValue];
    static_cast<BSShaderPPLightingProperty*>(apTextItem->pText[0])
        ->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pTexture);
}

// pInstance is reloaded before each call.
void LoveTesterMenu::PreviousPage([[maybe_unused]] LoveTesterButton* apButton, ...) {
    if (pInstance && pInstance->bAnimFinished) {
        pInstance->TurnPage(false);
        pInstance->RefreshPage();
    }
}

void LoveTesterMenu::Create() {
    LoveTesterMenu::Create(35);
}

void LoveTesterMenu::Create(int32_t aiTotalPoints) {
    int32_t iMissingIndex;
    Tile* pTile = InterfaceManager::Instance()->GetMenusRoot()->ReadFile("Data\\Menus\\CharGen\\love_tester_menu.xml");
    Menu* pMenu = pTile->GetMenu();
    if (pMenu && pMenu->GetClass() == LoveTesterMenu::GetClassStatic()) {
        pMenu->SetMenuTile(static_cast<TileMenu*>(pTile), false);
        LoveTesterMenu::pInstance = static_cast<LoveTesterMenu*>(pMenu);
        if (static_cast<LoveTesterMenu*>(pMenu)->LoveTesterMenu::CheckTilesExist(&iMissingIndex)) {
            LoveTesterMenu::pInstance->iAvailablePoints = aiTotalPoints;
            LoveTesterMenu::pInstance->InitMachine();
            LoveTesterMenu::sp3DCamera = LoveTesterMenu::pInstance->SetupCamera();
            HUDMainMenu::ForceObjectiveReminders(true);
            pMenu->PrepForVisibility(false);
        }
        else {
            Warning("MENUS:  Special book menu creation failed... Missing index = #%d", iMissingIndex);
            if (pTile) {
                delete pTile;
            }
            Interface::EmergencyCloseAllMenusAndBreakStuff();
        }
    }
    else if (pMenu && pMenu->pMenu) {
        delete pMenu;
    }
}

void LoveTesterMenu::RefreshTotalPointsValue(LoveTesterMenu::LoveTesterTextItem* apItem) {
    if (apItem->pText[0u] && apItem->pText[1]) {
        int32_t iPointsLeft = LoveTesterMenu::pInstance->iAvailablePoints - PlayerCharacter::GetSumOfSPECIALStats();
        int32_t iTensCapped = iPointsLeft / 10 >= 9 ? 9 : iPointsLeft / 10;
        int32_t iTens;
        if (iTensCapped <= 0) {
            iTens = 0;
        }
        else {
            iTens = iPointsLeft / 10 >= 9 ? 9 : iPointsLeft / 10;
        }
        int32_t iOnes = iPointsLeft % 10 >= 0 ? iPointsLeft % 10 : 0;
        apItem->pTextObj[0u]->SetAppCulled(false);
        if (iTens <= 0) {
            apItem->pText[0u]
                ->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, LoveTesterMenu::pInstance->spAVValues->m_pObject);
        }
        else {
            apItem->pText[0u]->SetTexture(
                BSShaderProperty::TEXTYPE_DIFFUSE,
                0,
                LoveTesterMenu::pInstance->spAVValues[iTens].m_pObject
            );
        }
        apItem->pTextObj[1]->SetAppCulled(false);
        apItem->pText[1]
            ->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, LoveTesterMenu::pInstance->spAVValues[iOnes].m_pObject);
    }
}

// The refresh callbacks take the base button; InitMachine stores this one only in ActorValueButtons.
void LoveTesterMenu::RefreshLeftAVButton(LoveTesterMenu::LoveTesterButton* apButton) {
    ActorValue::Index eActorValue = static_cast<LoveTesterMenu::ActorValueButton*>(apButton)->eActorValue;
    if (eActorValue != ActorValue::VALUE_NONE) {
        if (pPlayer->GetPermanentActorValue(eActorValue) <= 1) {
            apButton->spButton->SetAppCulled(true);
        }
        else {
            apButton->spButton->SetAppCulled(false);
        }
        if (InterfaceManager::Has360Controller() &&
            LoveTesterMenu::pInstance->eState == LoveTesterMenu::LTMS_SUMMARY &&
            eActorValue != LoveTesterMenu::pInstance->eXBoxIndexState + 4) {
            apButton->spButton->SetAppCulled(true);
        }
    }
}

// The refresh callbacks take the base button; InitMachine stores this one only in ActorValueButtons.
void LoveTesterMenu::RefreshRightAVButton(LoveTesterMenu::LoveTesterButton* apButton) {
    ActorValue::Index eActorValue = static_cast<LoveTesterMenu::ActorValueButton*>(apButton)->eActorValue;
    if (eActorValue != ActorValue::VALUE_NONE) {
        if (pPlayer->GetPermanentActorValue(eActorValue) >= 10 ||
            (LoveTesterMenu::pInstance &&
             PlayerCharacter::GetSumOfSPECIALStats() >= LoveTesterMenu::pInstance->iAvailablePoints)) {
            apButton->spButton->SetAppCulled(true);
        }
        else {
            apButton->spButton->SetAppCulled(false);
        }
        if (InterfaceManager::Has360Controller() &&
            LoveTesterMenu::pInstance->eState == LoveTesterMenu::LTMS_SUMMARY &&
            eActorValue != LoveTesterMenu::pInstance->eXBoxIndexState + 4) {
            apButton->spButton->SetAppCulled(true);
        }
    }
}

void LoveTesterMenu::RefreshAllDoneButton(LoveTesterMenu::LoveTesterButton* apButton) {
    if (LoveTesterMenu::pInstance->eState == LoveTesterMenu::LTMS_SUMMARY &&
        PlayerCharacter::GetSumOfSPECIALStats() < LoveTesterMenu::pInstance->iAvailablePoints) {
        apButton->pImage->SetTexture(
            BSShaderProperty::TEXTYPE_DIFFUSE,
            0,
            LoveTesterMenu::pInstance->spNextPageButtonImage->m_pObject
        );
    }
    else {
        apButton->pImage->SetTexture(
            BSShaderProperty::TEXTYPE_DIFFUSE,
            0,
            LoveTesterMenu::pInstance->spNextPageButtonImage[1].m_pObject
        );
    }
}

void LoveTesterMenu::RefreshPreviousPageButton(LoveTesterMenu::LoveTesterButton* apButton) {
    apButton->pImage->SetTexture(
        BSShaderProperty::TEXTYPE_DIFFUSE,
        0,
        LoveTesterMenu::pInstance->spPreviousPageButtonImage->m_pObject
    );
}

void LoveTesterMenu::DecreaseActorValue(LoveTesterMenu::LoveTesterButton* apButton, ...) {
    if (LoveTesterMenu::pInstance) {
        ActorValue::Index eActorValue = static_cast<LoveTesterMenu::ActorValueButton*>(apButton)->eActorValue;
        if (eActorValue != ActorValue::VALUE_NONE) {
            int32_t iValue = pPlayer->GetPermanentActorValue(eActorValue);
            if (iValue > 1) {
                BSSoundHandle th_s;
                th_s = BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialNumberDown", 289u);
                th_s.Play(false);
            }
            pPlayer->SetActorBaseValue(eActorValue, BSMAX(iValue - 1, 1));
        }
        LoveTesterMenu::pInstance->RefreshPage();
    }
}

void LoveTesterMenu::DecreaseCurrentActorValue([[maybe_unused]] LoveTesterMenu::LoveTesterButton* apButton, ...) {
    if (LoveTesterMenu::pInstance) {
        ActorValue::Index eSPECIAL = LoveTesterMenu::pInstance->GetSelectedSPECIAL();
        if (eSPECIAL != ActorValue::VALUE_NONE) {
            int32_t iValue = pPlayer->GetPermanentActorValue(eSPECIAL);
            if (iValue > 1) {
                BSSoundHandle th_s;
                th_s = BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialNumberDown", 289u);
                th_s.Play(false);
            }
            pPlayer->SetActorBaseValue(eSPECIAL, BSMAX(iValue - 1, 1));
        }
        LoveTesterMenu::pInstance->RefreshPage();
    }
}

// InitMachine stores this callback only in ActorValueButtons.
void LoveTesterMenu::IncreaseActorValue(LoveTesterMenu::LoveTesterButton* apButton, ...) {
    if (LoveTesterMenu::pInstance) {
        ActorValue::Index eActorValue = static_cast<LoveTesterMenu::ActorValueButton*>(apButton)->eActorValue;
        if (eActorValue != ActorValue::VALUE_NONE) {
            int32_t iValue = pPlayer->GetPermanentActorValue(eActorValue);
            if (iValue < 10) {
                BSSoundHandle th_s;
                th_s = BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialNumber", 289u);
                th_s.Play(false);
            }
            if (PlayerCharacter::GetSumOfSPECIALStats() < LoveTesterMenu::pInstance->iAvailablePoints) {
                pPlayer->SetActorBaseValue(eActorValue, BSMIN(iValue + 1, 10));
            }
        }
        LoveTesterMenu::pInstance->RefreshPage();
    }
}

void LoveTesterMenu::IncreaseCurrentActorValue([[maybe_unused]] LoveTesterMenu::LoveTesterButton* apButton, ...) {
    if (LoveTesterMenu::pInstance) {
        ActorValue::Index eSPECIAL = LoveTesterMenu::pInstance->GetSelectedSPECIAL();
        if (eSPECIAL != ActorValue::VALUE_NONE) {
            int32_t iValue = pPlayer->GetPermanentActorValue(eSPECIAL);
            if (iValue < 10) {
                BSSoundHandle th_s;
                th_s = BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialNumber", 289u);
                th_s.Play(false);
            }
            if (PlayerCharacter::GetSumOfSPECIALStats() < LoveTesterMenu::pInstance->iAvailablePoints) {
                pPlayer->SetActorBaseValue(eSPECIAL, BSMIN(iValue + 1, 10));
            }
        }
        LoveTesterMenu::pInstance->RefreshPage();
    }
}

void LoveTesterMenu::AllDoneButtonClick([[maybe_unused]] LoveTesterMenu::LoveTesterButton* apButton, ...) {
    if (LoveTesterMenu::pInstance &&
        PlayerCharacter::GetSumOfSPECIALStats() == LoveTesterMenu::pInstance->iAvailablePoints) {
        BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialNumberFinished", 289u).Play(false);
        LoveTesterMenu::Close();
    }
    else {
        BSAudio::QInstance().GetSoundHandleByName("UIActivateNothing", 289u).Play(false);
    }
}
