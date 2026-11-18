#include "Fallout/Interface/Menus/LoveTesterMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
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
#include "Reconstruction.hpp"
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

void LoveTesterMenu::DoClick(int32_t a1, Tile* a2) {
    // The page turn's result is kept but never read.
    [[maybe_unused]] bool bPageTurned = false;
    switch (a1) {
        case 0:
        case 1: {
            if (bAnimFinished) {
                if (a1) {
                    bPageTurned = TurnPage(false);
                }
                else {
                    bPageTurned = TurnPage(true);
                }
                RefreshPage();
            }
            break;
        }
        case 2:
        case 3: {
            int32_t ioldValue = 0;
            int32_t inewValue = 0;
            if (GetSelectedSPECIAL() == ActorValue::VALUE_NONE) {
                if (InterfaceManager::Has360Controller() && eState == LoveTesterMenu::LTMS_SUMMARY) {
                    ActorValue::Index eav_3 = static_cast<ActorValue::Index>(eXBoxIndexState + 4);
                    ioldValue = pPlayer->GetPermanentActorValue(eav_3);
                    if (a1 == 2) {
                        if (PlayerCharacter::GetSumOfSPECIALStats() < iAvailablePoints) {
                            int32_t v10 = BSMIN(ioldValue + 1, 10);
                            pPlayer->SetActorBaseValue(eav_3, v10);
                        }
                    }
                    else {
                        int32_t v11 = BSMAX(ioldValue - 1, 1);
                        pPlayer->SetActorBaseValue(eav_3, v11);
                    }
                    inewValue = pPlayer->GetBaseActorValue(eav_3);
                }
            }
            else {
                ActorValue::Index SelectedSPECIAL = GetSelectedSPECIAL();
                ioldValue = pPlayer->GetPermanentActorValue(SelectedSPECIAL);
                if (a1 == 2) {
                    if (PlayerCharacter::GetSumOfSPECIALStats() < iAvailablePoints) {
                        int32_t v18 = BSMIN(ioldValue + 1, 10);
                        ActorValue::Index eav = GetSelectedSPECIAL();
                        pPlayer->SetActorBaseValue(eav, v18);
                    }
                }
                else {
                    int32_t v19 = BSMAX(ioldValue - 1, 1);
                    ActorValue::Index eav_2 = GetSelectedSPECIAL();
                    pPlayer->SetActorBaseValue(eav_2, v19);
                }
                ActorValue::Index v9 = GetSelectedSPECIAL();
                inewValue = pPlayer->GetBaseActorValue(v9);
            }
            if (ioldValue > inewValue) {
                BSAudio* v12 = &BSAudio::QInstance();
                v12->GetSoundHandleByName("OBJBookSpecialNumberDown", 289u).Play(false);
            }
            if (ioldValue < inewValue) {
                BSAudio* v13 = &BSAudio::QInstance();
                v13->GetSoundHandleByName("OBJBookSpecialNumber", 289u).Play(false);
            }
            RefreshPage();
            break;
        }
        case 4: {
            if (PlayerCharacter::GetSumOfSPECIALStats() == iAvailablePoints) {
                BSAudio* v3 = &BSAudio::QInstance();
                v3->GetSoundHandleByName("OBJBookSpecialNumberFinished", 289u).Play(false);
                Close();
            }
            else {
                BSAudio* v5 = &BSAudio::QInstance();
                v5->GetSoundHandleByName("UIActivateNothing", 289u).Play(false);
            }
            break;
        }
        case 5: {
            if (eState <= LoveTesterMenu::LTMS_SUMMARY) {
                InterfaceManager* pim = InterfaceManager::Instance();
                int32_t cx = static_cast<int32_t>(pim->sRealCursorPos.x);
                int32_t cy = static_cast<int32_t>(pim->sRealCursorPos.z);
                if (BSShaderManager::bLetterbox) {
                    // The cursor's y in the letterboxed viewport: the back buffer's height times the viewport's bottom,
                    // plus the viewport's height fraction times y. The bottom is read before the virtual call.
                    NiRenderer* Renderer_ = NiRenderer::GetRenderer();
                    double m_bottom = BSShaderManager::kFramebufferViewport.m_bottom;
                    uint32_t uiHeight = Renderer_->GetDefaultBackBuffer()->m_uiHeight;
                    cy = static_cast<int32_t>(
                        static_cast<double>(uiHeight) * m_bottom +
                        (BSShaderManager::kFramebufferViewport.m_top - BSShaderManager::kFramebufferViewport.m_bottom) *
                            static_cast<double>(cy)
                    );
                }
                for (int32_t iindex = 0;; ++iindex) {
                    NiPoint3 dir;
                    NiPoint3 origin;
                    uint32_t FormFlags = Pages[eState].Buttons.iSize;
                    if (iindex >= FormFlags) {
                        break;
                    }
                    NiPick activatePick(0, 8u);
                    activatePick.SetObserveAppCullFlag(true);
                    activatePick.SetTarget(Pages[eState].Buttons.operator[](iindex)->spButton.m_pObject);
                    activatePick.m_eSortType = NiPick::SORT;
                    activatePick.SetIsReturnTexture(true);
                    LoveTesterMenu::sp3DCamera.m_pObject->WindowPointToRay(cx, cy, origin, dir, nullptr);
                    if (activatePick.PickObjects(origin, dir, false)) {
                        uint32_t ipickIndex = 0;
                        if ((&activatePick.m_pickResults)->GetAt(ipickIndex)) {
                            // Fetches the argument before the callback.
                            LoveTesterButton* pButton = Pages[eState].Buttons.operator[](iindex);
                            Pages[eState].Buttons.operator[](iindex)->pfButtonClick(pButton);
                            break;
                        }
                    }
                }
                for (int32_t iindex_2 = 0;; ++iindex_2) {
                    NiPoint3 dir_2;
                    NiPoint3 origin_2;
                    uint32_t v15 = GlobalPage.Buttons.iSize;
                    if (iindex_2 >= v15) {
                        break;
                    }
                    NiPick activatePick_2(0, 8u);
                    activatePick_2.SetObserveAppCullFlag(true);
                    activatePick_2.SetTarget(GlobalPage.Buttons.operator[](iindex_2)->spButton.m_pObject);
                    activatePick_2.m_eSortType = NiPick::SORT;
                    activatePick_2.SetIsReturnTexture(true);
                    LoveTesterMenu::sp3DCamera.m_pObject->WindowPointToRay(cx, cy, origin_2, dir_2, nullptr);
                    if (activatePick_2.PickObjects(origin_2, dir_2, false)) {
                        uint32_t v37 = 0;
                        if ((&activatePick_2.m_pickResults)->GetAt(v37)) {
                            LoveTesterButton* pButton = GlobalPage.Buttons.operator[](iindex_2);
                            GlobalPage.Buttons.operator[](iindex_2)->pfButtonClick(pButton);
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
                BSAudio* v16 = &BSAudio::QInstance();
                v16->GetSoundHandleByName("OBJBookSpecialFocus", 289u).Play(false);
            }
            RefreshPage();
            break;
        }
        case 7: {
            if (eState == LoveTesterMenu::LTMS_SUMMARY && eXBoxIndexState < LoveTesterMenu::LTMS_LUCK) {
                eXBoxIndexState = static_cast<LoveTesterMenu::LOVE_TESTER_MENU_STATE>(eXBoxIndexState + 1);
                BSAudio* v17 = &BSAudio::QInstance();
                v17->GetSoundHandleByName("OBJBookSpecialFocus", 289u).Play(false);
            }
            RefreshPage();
            break;
        }
        default: {
            return;
        }
    }
}

void LoveTesterMenu::DoIdle() {
    if (Interface::IsTopMenuID(Interface::eLoveTesterMenu)) {
        Controls* Controls_ = appMain->GetControls();
        int32_t key = 30;
        const char* v1 = sLanguage.String();
        if (!BSstrcmp(v1, "FRENCH")) {
            key = 16;
        }
        if (Controls_->GetKeyboardButton(key, 1)) {
            DoClick(4, nullptr);
        }
        else {
            NiTimeController* pspecialBookGeomController = pDynamicGeometry->GetControllers();
            if (pspecialBookGeomController) {
                NiControllerManager* pspecialBookGeomControllerManager =
                    static_cast<NiControllerManager*>(pspecialBookGeomController);
                NiControllerSequence* pseq = nullptr;
                if (!bLoveTesterAnimForward || eState > LoveTesterMenu::LTMS_START) {
                    const char* pcString;
                    if (bLoveTesterAnimForward) {
                        pcString = LoveTesterReverseAnimations[eState - 1];
                    }
                    else {
                        pcString = LoveTesterAnimations[eState];
                    }
                    NiFixedString apString(pcString);
                    NiFixedString* kName = &apString;
                    pseq = pspecialBookGeomControllerManager->GetSequenceByName(*kName);
                }
                fFrameAnimTimer = appTimer.fDelta + fFrameAnimTimer;
                if (pseq) {
                    pseq->Update(fFrameAnimTimer, true);
                    double v2 = pseq->m_fLastTime;
                    bAnimFinished = !(pseq->m_fEndKeyTime > v2) || eState <= LoveTesterMenu::LTMS_START;
                }
                else {
                    bAnimFinished = true;
                }
                while (pspecialBookGeomController) {
                    if (NiObject::DynamicCastStatic(
                            &NiMultiTargetTransformController::ms_RTTI, pspecialBookGeomController
                        )) {
                        NiUpdateData UpdateData(fFrameAnimTimer, false, false);
                        pspecialBookGeomController->Update(UpdateData);
                        break;
                    }
                    pspecialBookGeomController = pspecialBookGeomController->GetNext();
                }
            }
            NiUpdateData update(0.0f, false, false);
            pDynamicGeometry->Update(update);
        }
    }
}

bool LoveTesterMenu::DoKey(int32_t a1) {
    bool bhandled;
    if (a1 > 43) {
        if (a1 != 45) {
            if (a1 == 61) {
                goto LABEL_8;
            }
            if (a1 != 95) {
                return false;
            }
        }
    LABEL_13:;
        ActorValue::Index SelectedSPECIAL = GetSelectedSPECIAL();
        if (SelectedSPECIAL != ActorValue::VALUE_NONE && pPlayer->GetPermanentActorValue(SelectedSPECIAL) > 1) {
            DoClick(3, nullptr);
            return true;
        }
        return false;
    }
    if (a1 == 43) {
    LABEL_8:;
        ActorValue::Index v5 = GetSelectedSPECIAL();
        if (v5 != ActorValue::VALUE_NONE &&
            (PlayerCharacter::GetSumOfSPECIALStats() < iAvailablePoints && pPlayer->GetPermanentActorValue(v5) < 10)) {
            DoClick(2, nullptr);
            return true;
        }
        return false;
    }
    switch (a1) {
        case -0x7FFFFFFF:
        case -0x7FFFFFFE: {
            DoClick(a1 == -0x7FFFFFFF, nullptr);
            bhandled = true;
            break;
        }
        case -0x7FFFFFFD: {
            goto LABEL_8;
        }
        case -0x7FFFFFFC: {
            goto LABEL_13;
        }
        default: {
            return false;
        }
    }
    return bhandled;
}

int32_t LoveTesterMenu::GetClass() {
    return 1074;
}

bool LoveTesterMenu::DoGamepad(int32_t a1, float a2) {
    bool result;
    if (!Interface::IsTopMenuID(Interface::eLoveTesterMenu)) {
        return false;
    }
    ActorValue::Index eav = GetSelectedSPECIAL();
    switch (a1) {
        case 1: {
            goto LABEL_13;
        }
        case 2: {
            goto LABEL_5;
        }
        case 3: {
            if (eState == LoveTesterMenu::LTMS_SUMMARY) {
            LABEL_13:;
                if (eState == LoveTesterMenu::LTMS_SUMMARY) {
                    result = false;
                }
                else {
                LABEL_15:;
                    if (eav != ActorValue::VALUE_NONE && PlayerCharacter::GetSumOfSPECIALStats() < iAvailablePoints &&
                        pPlayer->GetPermanentActorValue(eav) < 10) {
                        DoClick(2, nullptr);
                    }
                    result = true;
                }
            }
            else {
                result = false;
            }
            break;
        }
        case 4: {
            if (eState == LoveTesterMenu::LTMS_SUMMARY) {
            LABEL_5:;
                if (eState == LoveTesterMenu::LTMS_SUMMARY) {
                    result = false;
                }
                else {
                LABEL_7:;
                    if (eav != ActorValue::VALUE_NONE && pPlayer->GetPermanentActorValue(eav) > 1) {
                        DoClick(3, nullptr);
                    }
                    result = true;
                }
            }
            else {
                result = false;
            }
            break;
        }
        case 9: {
            goto LABEL_15;
        }
        case 10: {
            goto LABEL_7;
        }
        default: {
            return false;
        }
    }
    return result;
}

bool LoveTesterMenu::TurnPage(bool bForward) {
    XFNV_INLINED_LOCAL(BSSoundHandle, psoundHand, std::nullopt, std::nullopt);
    bool brunAnim = false;
    if (!bForward) {
        bLoveTesterAnimForward = false;
        if (eState <= LoveTesterMenu::LTMS_START) {
            return false;
        }
        eState = static_cast<LoveTesterMenu::LOVE_TESTER_MENU_STATE>(eState - 1);
        brunAnim = true;
        BSSoundHandle psoundHand_2;
        BSAudio* v7 = &BSAudio::QInstance();
        psoundHand_2 = v7->GetSoundHandleByName("OBJBookSpecialPageTurn", 289u);
        psoundHand_2.Play(false);
        goto LABEL_11;
    }
    bLoveTesterAnimForward = true;
    if (eState < LoveTesterMenu::LTMS_SUMMARY) {
        BSAudio* v2;
        eState = static_cast<LoveTesterMenu::LOVE_TESTER_MENU_STATE>(eState + 1);
        brunAnim = true;
        ::new (&psoundHand) BSSoundHandle;
        v2 = &BSAudio::QInstance();
        psoundHand = v2->GetSoundHandleByName("OBJBookSpecialPageTurn", 289u);
        psoundHand.Play(false);
    LABEL_11:;
        if (brunAnim) {
            NiTimeController* pSPECIALBookGeomController =
                LoveTesterMenu::pInstance->pDynamicGeometry->GetController(&NiControllerManager::ms_RTTI);
            if (pSPECIALBookGeomController) {
                NiControllerManager* pSPECIALBookGeomControllerManager =
                    static_cast<NiControllerManager*>(pSPECIALBookGeomController);
                NiControllerSequence* pseq = nullptr;
                if (!bLoveTesterAnimForward || eState > LoveTesterMenu::LTMS_START) {
                    const char* pcString;
                    if (bLoveTesterAnimForward) {
                        pcString = LoveTesterReverseAnimations[eState - 1];
                    }
                    else {
                        pcString = LoveTesterAnimations[eState];
                    }
                    NiFixedString apString(pcString);
                    NiFixedString* a2 = &apString;
                    pseq = pSPECIALBookGeomControllerManager->GetSequenceByName(*a2);
                }
                if (pseq) {
                    pSPECIALBookGeomControllerManager->DeactivateAll(0.0f);
                    pSPECIALBookGeomControllerManager->ActivateSequence(pseq, 0, true, 1.0f, 0.0f, nullptr);
                    pSPECIALBookGeomControllerManager->SetActive(true);
                    pseq->ResetSequence();
                }
            }
        }
        return true;
    }
    if (PlayerCharacter::GetSumOfSPECIALStats() == iAvailablePoints) {
        BSAudio* v3 = &BSAudio::QInstance();
        v3->GetSoundHandleByName("OBJBookSpecialNumberFinished", 289u).Play(false);
        Close();
    }
    else {
        BSAudio* v5 = &BSAudio::QInstance();
        v5->GetSoundHandleByName("UIActivateNothing", 289u).Play(false);
    }
    return false;
}

void LoveTesterMenu::RefreshPage() {
    if (eState <= LoveTesterMenu::LTMS_SUMMARY) {
        if (LoveTesterMenu::pInstance) {
            int32_t v11 = 0;
            if (LoveTesterMenu::pInstance->GetSelectedSPECIAL() != ActorValue::VALUE_NONE) {
                ActorValue::Index SelectedSPECIAL = LoveTesterMenu::pInstance->GetSelectedSPECIAL();
                v11 = pPlayer->GetPermanentActorValue(SelectedSPECIAL);
            }
            for (uint32_t auiIndex = 0; auiIndex < LoveTesterMenu::pInstance->LightBulbs.iSize; ++auiIndex) {
                int32_t nThreshold = LoveTesterMenu::pInstance->LightBulbs.operator[](auiIndex)->nThreshold;
                LoveTesterMenu::pInstance->LightBulbs.operator[](auiIndex);
            }
            for (uint32_t i = 0; i < LoveTesterMenu::pInstance->LightBulbGlows.iSize; ++i) {
                int32_t m_nValueToGlowAt = LoveTesterMenu::pInstance->LightBulbGlows.operator[](i)->m_nValueToGlowAt;
                if (LoveTesterMenu::pInstance->LightBulbGlows.operator[](i)->m_GlowCard) {
                    LoveTesterMenu::pInstance->LightBulbGlows.operator[](i)->m_GlowCard->SetAppCulled(
                        v11 < m_nValueToGlowAt
                    );
                }
            }
        }
        // Each callback's argument is fetched before the callback itself, as the game does.
        for (uint32_t j = 0; j < Pages[eState].TextItems.iSize; ++j) {
            LoveTesterTextItem* pItem = Pages[eState].TextItems.operator[](j);
            Pages[eState].TextItems.operator[](j)->pfRefreshField(pItem);
        }
        for (uint32_t k = 0; k < Pages[eState].Buttons.iSize; ++k) {
            LoveTesterButton* pButton = Pages[eState].Buttons.operator[](k);
            Pages[eState].Buttons.operator[](k)->pfRefresh(pButton);
        }
        for (uint32_t m = 0; m < GlobalPage.TextItems.iSize; ++m) {
            LoveTesterTextItem* pItem = GlobalPage.TextItems.operator[](m);
            GlobalPage.TextItems.operator[](m)->pfRefreshField(pItem);
        }
        for (uint32_t n = 0; n < GlobalPage.Buttons.iSize; ++n) {
            LoveTesterButton* pButton = GlobalPage.Buttons.operator[](n);
            GlobalPage.Buttons.operator[](n)->pfRefresh(pButton);
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
    // BBLTOn is formatted with the Goodsprings folder and no platform folder, as the game does.
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
// the third update and every later page, as the game does.
void LoveTesterMenu::InitMachine() {
    pInstance->pDynamicGeometry = pModelLoader->LoadFile(
        "Meshes\\architecture\\Goodsprings\\NV_VitoMaticVigorTester_Activate.NIF", LOD_MULT_NONE, true, false, false,
        false
    );
    SwapPlatformLanguageTextures(pInstance->pDynamicGeometry);
    pInstance->pCabinetGeometry = pModelLoader->LoadFile(
        "Meshes\\architecture\\Goodsprings\\NV_VitoMaticVigorTester_Cabinet.NIF", LOD_MULT_NONE, true, false, false,
        false
    );
    SwapPlatformLanguageTextures(pInstance->pCabinetGeometry);

    // The machine faces the camera, tilted back: X by pi/2, then Z by pi, then X by 0.12566371.
    NiMatrix3 kRotation;
    kRotation.~NiMatrix3();
    ::new (&kRotation) NiMatrix3;
    NiMatrix3 kStep;
    kStep.~NiMatrix3();
    ::new (&kStep) NiMatrix3;
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
    NiUpdateData kDynamicUpdate;
    kDynamicUpdate.~NiUpdateData();
    ::new (&kDynamicUpdate) NiUpdateData(0.0f, false, false);
    NiUpdateData kCabinetUpdate;
    kCabinetUpdate.~NiUpdateData();
    ::new (&kCabinetUpdate) NiUpdateData(0.0f, false, false);
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
            NiUpdateData kControllerUpdate;
            kControllerUpdate.~NiUpdateData();
            ::new (&kControllerUpdate) NiUpdateData(0.0f, false, false);
            pController->Update(kControllerUpdate);
            break;
        }
        pController = pController->GetNext();
    }
    NiUpdateData kUpdate;
    kUpdate.~NiUpdateData();
    ::new (&kUpdate) NiUpdateData(0.0f, false, false);
    pDynamicGeometry->Update(kUpdate);

    // Light the scene from the model's own lights; without any, a white point light whose specular red holds the radius
    // (20 times the bound's radius).
    const NiBound* pBound = &pInstance->pDynamicGeometry->GetWorldBound();
    const float fRadius = static_cast<float>(pBound->m_fRadius * 20.0);
    // IsNode is called before the scene is fetched.
    NiNode* pDynamicNode = pInstance->pDynamicGeometry->IsNode();
    ShadowSceneNode* pScene = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D);
    if (!pScene->AddAllLightsFromNode_Recurse(pDynamicNode, fRadius)) {
        void* pLightMemory = NiMemObject::operator new(sizeof(NiPointLight));
        NiPointLight* pLight = pLightMemory ? ::new (pLightMemory) NiPointLight : nullptr;
        NiPoint3 kTranslate;
        pLight->SetTranslate(*::new (&kTranslate) NiPoint3(0.0f, 0.0f, 0.0f));
        NiColor kDiffuse;
        pLight->SetDiffuseColor(*XFNV_RECONSTRUCT(NiColor)(&kDiffuse, 0.85f, 0.85f, 0.85f));
        NiColor kSpecular;
        pLight->SetSpecularColor(*XFNV_RECONSTRUCT(NiColor)(&kSpecular, fRadius, 0.0f, 0.0f));
        pLight->SetDimmer(1.0f);
        NiUpdateData kLightUpdate;
        kLightUpdate.~NiUpdateData();
        ::new (&kLightUpdate) NiUpdateData(0.0f, false, false);
        pLight->Update(kLightUpdate);
        BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D)->AttachChild(pLight, true);
        BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D)->AddLight(pLight, false);
    }
    BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D)
        ->AttachChild(pInstance->pDynamicGeometry, true);
    BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D)
        ->AttachChild(pInstance->pCabinetGeometry, true);

    // The global page's buttons: the points to spend down and up, and the page arrows.
    void* pMemory = ourMalloc(sizeof(ActorValueButton));
    LoveTesterButton* pButton = pMemory ? ::new (pMemory) ActorValueButton(
                                              &LoveTesterMenu::DecreaseCurrentActorValue,
                                              &LoveTesterMenu::RefreshLeftAVButton, "P1_Decrease_Btn:0", -1, 0
                                          )
                                        : nullptr;
    pInstance->GlobalPage.Buttons.Add(pButton);
    pMemory = ourMalloc(sizeof(ActorValueButton));
    pButton = pMemory ? ::new (pMemory) ActorValueButton(
                            &LoveTesterMenu::IncreaseCurrentActorValue, &LoveTesterMenu::RefreshRightAVButton,
                            "P1_Increase_Btn:0", -1, 0
                        )
                      : nullptr;
    pInstance->GlobalPage.Buttons.Add(pButton);
    pMemory = ourMalloc(sizeof(LoveTesterButton));
    pButton = pMemory
        ? ::new (pMemory)
              LoveTesterButton(&LoveTesterMenu::NextPage, &LoveTesterMenu::RefreshAllDoneButton, "P1_RT_Btn:0")
        : nullptr;
    pInstance->GlobalPage.Buttons.Add(pButton);
    pMemory = ourMalloc(sizeof(LoveTesterButton));
    pButton = pMemory
        ? ::new (pMemory)
              LoveTesterButton(&LoveTesterMenu::PreviousPage, &LoveTesterMenu::RefreshPreviousPageButton, "P1_LT_Btn:0")
        : nullptr;
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
                void* pGlowMemory = ourMalloc(sizeof(LoveTesterLightBulbGlows));
                LoveTesterLightBulbGlows* pGlow = nullptr;
                if (pGlowMemory) {
                    NiFixedString kCardName(acValueName);
                    NiAVObject* pCard = pInstance->pCabinetGeometry->GetObjectByName(kCardName);
                    pGlow = ::new (pGlowMemory) LoveTesterLightBulbGlows(pCard, iValue);
                }
                pInstance->LightBulbGlows.Add(pGlow);
            }
        }
        BSsprintf(acValueName, 30, "P1_PointVal_0%i_GLOW", iValue);
        NiFixedString kName(acValueName);
        NiAVObject* pGlowCard = pInstance->pCabinetGeometry->GetObjectByName(kName);
        if (pGlowCard) {
            void* pGlowMemory = ourMalloc(sizeof(LoveTesterLightBulbGlows));
            LoveTesterLightBulbGlows* pGlow = nullptr;
            if (pGlowMemory) {
                NiFixedString kCardName(acValueName);
                NiAVObject* pCard = pInstance->pCabinetGeometry->GetObjectByName(kCardName);
                pGlow = ::new (pGlowMemory) LoveTesterLightBulbGlows(pCard, iValue);
            }
            pInstance->LightBulbGlows.Add(pGlow);
        }
    }
    char acTenName[32];
    for (int32_t iBulb = 0; iBulb < 10; ++iBulb) {
        BSsprintf(acTenName, 30, "P1_PointVal_10_GLOW:%i", iBulb);
        NiFixedString kName(acTenName);
        NiAVObject* pGlowCard = pInstance->pCabinetGeometry->GetObjectByName(kName);
        if (pGlowCard) {
            void* pGlowMemory = ourMalloc(sizeof(LoveTesterLightBulbGlows));
            LoveTesterLightBulbGlows* pGlow = nullptr;
            if (pGlowMemory) {
                NiFixedString kCardName(acTenName);
                NiAVObject* pCard = pInstance->pCabinetGeometry->GetObjectByName(kCardName);
                pGlow = ::new (pGlowMemory) LoveTesterLightBulbGlows(pCard, 10);
            }
            pInstance->LightBulbGlows.Add(pGlow);
        }
    }
    BSsprintf(acTenName, 30, "P1_PointVal_10_GLOW");
    {
        NiFixedString kName(acTenName);
        NiAVObject* pGlowCard = pInstance->pCabinetGeometry->GetObjectByName(kName);
        if (pGlowCard) {
            void* pGlowMemory = ourMalloc(sizeof(LoveTesterLightBulbGlows));
            LoveTesterLightBulbGlows* pGlow = nullptr;
            if (pGlowMemory) {
                NiFixedString kCardName(acTenName);
                NiAVObject* pCard = pInstance->pCabinetGeometry->GetObjectByName(kCardName);
                pGlow = ::new (pGlowMemory) LoveTesterLightBulbGlows(pCard, 10);
            }
            pInstance->LightBulbGlows.Add(pGlow);
        }
    }

    // The points left (two digits of the dynamic geometry, every page) and the book's look-inside button.
    pMemory = ourMalloc(sizeof(LoveTesterTextItem));
    LoveTesterTextItem* pPointsText = pMemory ? ::new (pMemory) LoveTesterTextItem(
                                                    &LoveTesterMenu::RefreshTotalPointsValue, "P1_PointsRemainDigit1:0",
                                                    "P1_PointsRemainDigit2:0", -1, true, 0
                                                )
                                              : nullptr;
    GlobalPage.TextItems.Add(pPointsText);
    pMemory = ourMalloc(sizeof(LoveTesterButton));
    pButton = pMemory
        ? ::new (pMemory)
              LoveTesterButton(&LoveTesterMenu::NextPage, &LoveTesterMenu::RefreshAllDoneButton, "LookInside_Btn:0")
        : nullptr;
    Pages[0].Buttons.Add(pButton);

    // The index page: per SPECIAL actor value (5 Strength to 11 Luck) its value text, then its decrease and increase
    // buttons, which refresh that text and the points left.
    pMemory = ourMalloc(sizeof(LoveTesterTextItem));
    LoveTesterTextItem* pValueText = pMemory
        ? ::new (pMemory)
              LoveTesterTextItem(&LoveTesterMenu::RefreshActorValue, "Index_StrengthPointVal:0", nullptr, 5, true, 0)
        : nullptr;
    Pages[8].TextItems.Add(pValueText);
    pMemory = ourMalloc(sizeof(ActorValueButton));
    pButton = pMemory ? ::new (pMemory) ActorValueButton(
                            &LoveTesterMenu::DecreaseActorValue, &LoveTesterMenu::RefreshLeftAVButton,
                            "Index_StrengthDecrease_Btn:0", 5, 2, pValueText, pPointsText
                        )
                      : nullptr;
    Pages[8].Buttons.Add(pButton);
    pMemory = ourMalloc(sizeof(ActorValueButton));
    pButton = pMemory ? ::new (pMemory) ActorValueButton(
                            &LoveTesterMenu::IncreaseActorValue, &LoveTesterMenu::RefreshRightAVButton,
                            "Index_StrengthIncrease_Btn:0", 5, 2, pValueText, pPointsText
                        )
                      : nullptr;
    Pages[8].Buttons.Add(pButton);

    pMemory = ourMalloc(sizeof(LoveTesterTextItem));
    pValueText = pMemory
        ? ::new (pMemory)
              LoveTesterTextItem(&LoveTesterMenu::RefreshActorValue, "Index_PerceptionPointVal:0", nullptr, 6, true, 0)
        : nullptr;
    Pages[8].TextItems.Add(pValueText);
    pMemory = ourMalloc(sizeof(ActorValueButton));
    pButton = pMemory ? ::new (pMemory) ActorValueButton(
                            &LoveTesterMenu::DecreaseActorValue, &LoveTesterMenu::RefreshLeftAVButton,
                            "Index_PerceptionDecrease_Btn:0", 6, 2, pValueText, pPointsText
                        )
                      : nullptr;
    Pages[8].Buttons.Add(pButton);
    pMemory = ourMalloc(sizeof(ActorValueButton));
    pButton = pMemory ? ::new (pMemory) ActorValueButton(
                            &LoveTesterMenu::IncreaseActorValue, &LoveTesterMenu::RefreshRightAVButton,
                            "Index_PerceptionIncrease_Btn:0", 6, 2, pValueText, pPointsText
                        )
                      : nullptr;
    Pages[8].Buttons.Add(pButton);

    pMemory = ourMalloc(sizeof(LoveTesterTextItem));
    pValueText = pMemory
        ? ::new (pMemory)
              LoveTesterTextItem(&LoveTesterMenu::RefreshActorValue, "Index_EndurancePointVal:0", nullptr, 7, true, 0)
        : nullptr;
    Pages[8].TextItems.Add(pValueText);
    pMemory = ourMalloc(sizeof(ActorValueButton));
    pButton = pMemory ? ::new (pMemory) ActorValueButton(
                            &LoveTesterMenu::DecreaseActorValue, &LoveTesterMenu::RefreshLeftAVButton,
                            "Index_EnduranceDecrease_Btn:0", 7, 2, pValueText, pPointsText
                        )
                      : nullptr;
    Pages[8].Buttons.Add(pButton);
    pMemory = ourMalloc(sizeof(ActorValueButton));
    pButton = pMemory ? ::new (pMemory) ActorValueButton(
                            &LoveTesterMenu::IncreaseActorValue, &LoveTesterMenu::RefreshRightAVButton,
                            "Index_EnduranceIncrease_Btn:0", 7, 2, pValueText, pPointsText
                        )
                      : nullptr;
    Pages[8].Buttons.Add(pButton);

    pMemory = ourMalloc(sizeof(LoveTesterTextItem));
    pValueText = pMemory
        ? ::new (pMemory)
              LoveTesterTextItem(&LoveTesterMenu::RefreshActorValue, "Index_CharismaPointVal:0", nullptr, 8, true, 0)
        : nullptr;
    Pages[8].TextItems.Add(pValueText);
    pMemory = ourMalloc(sizeof(ActorValueButton));
    pButton = pMemory ? ::new (pMemory) ActorValueButton(
                            &LoveTesterMenu::DecreaseActorValue, &LoveTesterMenu::RefreshLeftAVButton,
                            "Index_CharismaDecrease_Btn:0", 8, 2, pValueText, pPointsText
                        )
                      : nullptr;
    Pages[8].Buttons.Add(pButton);
    pMemory = ourMalloc(sizeof(ActorValueButton));
    pButton = pMemory ? ::new (pMemory) ActorValueButton(
                            &LoveTesterMenu::IncreaseActorValue, &LoveTesterMenu::RefreshRightAVButton,
                            "Index_CharismaIncrease_Btn:0", 8, 2, pValueText, pPointsText
                        )
                      : nullptr;
    Pages[8].Buttons.Add(pButton);

    pMemory = ourMalloc(sizeof(LoveTesterTextItem));
    pValueText = pMemory ? ::new (pMemory) LoveTesterTextItem(
                               &LoveTesterMenu::RefreshActorValue, "Index_IntelligencePointVal:0", nullptr, 9, true, 0
                           )
                         : nullptr;
    Pages[8].TextItems.Add(pValueText);
    pMemory = ourMalloc(sizeof(ActorValueButton));
    pButton = pMemory ? ::new (pMemory) ActorValueButton(
                            &LoveTesterMenu::DecreaseActorValue, &LoveTesterMenu::RefreshLeftAVButton,
                            "Index_IntelligenceDecrease_Btn:0", 9, 2, pValueText, pPointsText
                        )
                      : nullptr;
    Pages[8].Buttons.Add(pButton);
    pMemory = ourMalloc(sizeof(ActorValueButton));
    pButton = pMemory ? ::new (pMemory) ActorValueButton(
                            &LoveTesterMenu::IncreaseActorValue, &LoveTesterMenu::RefreshRightAVButton,
                            "Index_IntelligenceIncrease_Btn:0", 9, 2, pValueText, pPointsText
                        )
                      : nullptr;
    Pages[8].Buttons.Add(pButton);

    pMemory = ourMalloc(sizeof(LoveTesterTextItem));
    pValueText = pMemory
        ? ::new (pMemory)
              LoveTesterTextItem(&LoveTesterMenu::RefreshActorValue, "Index_AgilityPointVal:0", nullptr, 10, true, 0)
        : nullptr;
    Pages[8].TextItems.Add(pValueText);
    pMemory = ourMalloc(sizeof(ActorValueButton));
    pButton = pMemory ? ::new (pMemory) ActorValueButton(
                            &LoveTesterMenu::DecreaseActorValue, &LoveTesterMenu::RefreshLeftAVButton,
                            "Index_AgilityDecrease_Btn:0", 10, 2, pValueText, pPointsText
                        )
                      : nullptr;
    Pages[8].Buttons.Add(pButton);
    pMemory = ourMalloc(sizeof(ActorValueButton));
    pButton = pMemory ? ::new (pMemory) ActorValueButton(
                            &LoveTesterMenu::IncreaseActorValue, &LoveTesterMenu::RefreshRightAVButton,
                            "Index_AgilityIncrease_Btn:0", 10, 2, pValueText, pPointsText
                        )
                      : nullptr;
    Pages[8].Buttons.Add(pButton);

    pMemory = ourMalloc(sizeof(LoveTesterTextItem));
    pValueText = pMemory
        ? ::new (pMemory)
              LoveTesterTextItem(&LoveTesterMenu::RefreshActorValue, "Index_LuckPointVal:0", nullptr, 11, true, 0)
        : nullptr;
    Pages[8].TextItems.Add(pValueText);
    pMemory = ourMalloc(sizeof(ActorValueButton));
    pButton = pMemory ? ::new (pMemory) ActorValueButton(
                            &LoveTesterMenu::DecreaseActorValue, &LoveTesterMenu::RefreshLeftAVButton,
                            "Index_LuckDecrease_Btn:0", 11, 2, pValueText, pPointsText
                        )
                      : nullptr;
    Pages[8].Buttons.Add(pButton);
    pMemory = ourMalloc(sizeof(ActorValueButton));
    pButton = pMemory ? ::new (pMemory) ActorValueButton(
                            &LoveTesterMenu::IncreaseActorValue, &LoveTesterMenu::RefreshRightAVButton,
                            "Index_LuckIncrease_Btn:0", 11, 2, pValueText, pPointsText
                        )
                      : nullptr;
    Pages[8].Buttons.Add(pButton);

    pMemory = ourMalloc(sizeof(LoveTesterButton));
    pButton = pMemory ? ::new (pMemory) LoveTesterButton(
                            &LoveTesterMenu::AllDoneButtonClick, &LoveTesterMenu::RefreshAllDoneButton, "AllDone_Btn:0"
                        )
                      : nullptr;
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

char LoveTesterMenu::CheckTilesExist(int32_t* a1) {
    for (*a1 = 0; *a1 < 8; ++*a1) {
        if (!pTiles[*a1]) {
            return 0;
        }
    }
    return 1;
}

NiCamera* LoveTesterMenu::SetupCamera() {
    int32_t ScreenHeight;
    NiCamera* v10;
    NiPoint3 v12;
    NiPoint3 dst;
    NiRect<float> a2;
    void* apMemory = NiMemObject::operator new(sizeof(NiCamera));
    if (apMemory) {
        v10 = ::new (apMemory) NiCamera;
    }
    else {
        v10 = nullptr;
    }
    ::new (&a2) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
    if (BSShaderManager::bLetterbox) {
        a2 = BSShaderManager::kFramebufferViewport;
    }
    v10->SetViewPort(a2);
    NiFrustum kFrustum(false);
    SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
    kFrustum.m_fFar = WorldSceneGraph->GetFarDistance();
    kFrustum.m_fNear = BSSceneGraph::fNearDistance.Float();
    float v8 = kFrustum.m_fFar / kFrustum.m_fNear;
    v10->SetMaxFarNearRatio(v8);
    if (BSShaderManager::bLetterbox) {
        ScreenHeight = BSShaderManager::iLetterboxHeight;
    }
    else {
        ScreenHeight = Renderer::GetScreenHeight();
    }
    float v19 = static_cast<double>(ScreenHeight) / static_cast<double>(Renderer::GetScreenWidth());
    float v17 = 0.75f;
    float v16 = BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369 * 0.6499999761581421;
    kFrustum.m_fLeft = -std::tan(v16) * 0.75f;
    kFrustum.m_fRight = std::tan(v16) * 0.75f;
    double v1 = std::tan(v16);
    kFrustum.m_fBottom = -v1 * v19 * 0.75f;
    double v2 = std::tan(v16);
    kFrustum.m_fTop = v2 * v19 * 0.75f;
    kFrustum.m_bOrtho = false;
    v10->SetViewFrustum(kFrustum);
    v10->SetTranslate(0.0f, 0.0f, 1.0f);
    NiUpdateData arData(0.0f, false, false);
    v10->Update(arData);
    NiPoint3* a4 = ::new (&dst) NiPoint3(0.0f, 1.0f, 0.0f);
    NiPoint3* v3 = ::new (&v12) NiPoint3(0.0f, 0.0f, 0.0f);
    v10->LookAtWorldPoint(*v3, *a4);
    v10->SetMinNearPlaneDist(1.0f);
    NiFixedString apString("Surgery3DCamera");
    NiFixedString* kName = &apString;
    v10->SetName(*kName);
    return v10;
}

void LoveTesterMenu::NextPage([[maybe_unused]] LoveTesterMenu::LoveTesterButton* apButton, ...) {
    if ((LoveTesterMenu::pInstance) && (LoveTesterMenu::pInstance->bAnimFinished)) {
        LoveTesterMenu::pInstance->TurnPage(true);
        LoveTesterMenu::pInstance->RefreshPage();
    }
}

void LoveTesterMenu::Draw3DElements() {
    if ((LoveTesterMenu::pInstance) && (LoveTesterMenu::sp3DCamera.m_pObject)) {
        NiRenderer* Renderer_ = NiRenderer::GetRenderer();
        Renderer_->ClearBuffer(nullptr, 4u);
        SceneGraph* apSceneGraph = Main::GetWorldSceneGraph();
        BSCullingProcess* apCuller = apSceneGraph->pCuller;
        apCuller->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
        NiCamera* pcam = LoveTesterMenu::sp3DCamera.m_pObject;
        NiCamera::HandleEyeFinityFOVScale(pcam, 0.65f);
        InterfaceManager* v1 = InterfaceManager::Instance();
        BSShaderAccumulator* v2 = v1->Get3dAccumulator();
        NiPointer<BSShaderAccumulator> spaccum(v2);
        BSShaderAccumulator* afScale = spaccum.m_pObject;
        apCuller->SetAccumulator(afScale);
        ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
        BSShaderUtil::AccumulateScene(pcam, ShadowSceneNode_, *apCuller);
        BSShaderAccumulator* v4 = spaccum.m_pObject;
        BSShaderUtil::RenderScene(pcam, v4, false);
        apCuller->SetAccumulator(nullptr);
        apCuller->PopCullMode();
    }
}

int32_t LoveTesterMenu::GetClassStatic() {
    return 1074;
}

void LoveTesterMenu::Close() {
    int32_t ClassStatic = LoveTesterMenu::GetClassStatic();
    Tile* MenuByClass = Menu::GetMenuByClass(ClassStatic);
    if (MenuByClass) {
        pModelLoader->ReleaseFile("Meshes\\architecture\\Goodsprings\\NV_VitoMaticVigorTester_Activate.NIF");
        pModelLoader->ReleaseFile("Meshes\\architecture\\Goodsprings\\NV_VitoMaticVigorTester_Cabinet.NIF");
        ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
        ShadowSceneNode_->RemoveAllChildren();
        ShadowSceneNode* v3 = BSShaderManager::GetShadowSceneNode(1);
        v3->RemoveAllLights();
        LoveTesterMenu::pInstance->pDynamicGeometry = nullptr;
        LoveTesterMenu::pInstance->pCabinetGeometry = nullptr;
        LoveTesterMenu::sp3DCamera.operator=(static_cast<NiCamera*>(nullptr));
        for (int32_t i = 0; i < 9; ++i) {
            for (uint32_t auiIndex = 0; auiIndex < LoveTesterMenu::pInstance->Pages[i].Buttons.iSize; ++auiIndex) {
                LoveTesterMenu::LoveTesterButton* apThis =
                    LoveTesterMenu::pInstance->Pages[i].Buttons.operator[](auiIndex);
                if (apThis) {
                    delete apThis;
                }
            }
            for (uint32_t j = 0; j < LoveTesterMenu::pInstance->Pages[i].TextItems.iSize; ++j) {
                ourFree(LoveTesterMenu::pInstance->Pages[i].TextItems.operator[](j));
            }
            LoveTesterMenu::pInstance->Pages[i].Buttons.Clear(true);
            LoveTesterMenu::pInstance->Pages[i].TextItems.Clear(true);
        }
        for (uint32_t k = 0; k < LoveTesterMenu::pInstance->GlobalPage.Buttons.iSize; ++k) {
            LoveTesterMenu::LoveTesterButton* v4 = LoveTesterMenu::pInstance->GlobalPage.Buttons.operator[](k);
            if (v4) {
                delete v4;
            }
        }
        for (uint32_t m = 0; m < LoveTesterMenu::pInstance->GlobalPage.TextItems.iSize; ++m) {
            ourFree(LoveTesterMenu::pInstance->GlobalPage.TextItems.operator[](m));
        }
        LoveTesterMenu::pInstance->GlobalPage.Buttons.Clear(true);
        LoveTesterMenu::pInstance->GlobalPage.TextItems.Clear(true);
        LoveTesterMenu::pInstance->LightBulbs.Clear(true);
        LoveTesterMenu::pInstance->LightBulbGlows.Clear(true);
        if (LoveTesterMenu::pInstance) {
            MenuByClass->Set(Tile::kTileValue_DeleteOnFade, 1);
            LoveTesterMenu::pInstance->StartFadeOut();
        }
    }
}

// The text items' refresh callback: the player's permanent actor value picks the digit texture, set as the item's first
// text property's texture 0 of type 0.
void LoveTesterMenu::RefreshActorValue(LoveTesterTextItem* apTextItem) {
    // The ActorValueOwner base is reached without a null check, as the game does: cast through a reference.
    int32_t iValue = static_cast<ActorValueOwner&>(*pPlayer).GetPermanentActorValue(apTextItem->eActorValue);
    NiSourceTexture* pTexture = pInstance->spAVValues[iValue].operator->();
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
    int32_t v2;
    int32_t imissingIndex;
    InterfaceManager* v1 = InterfaceManager::Instance();
    Tile* ptile = v1->GetMenusRoot()->ReadFile("Data\\Menus\\CharGen\\love_tester_menu.xml");
    Menu* pmenu = ptile->GetMenu();
    if (pmenu && (v2 = pmenu->GetClass(), v2 == LoveTesterMenu::GetClassStatic())) {
        pmenu->SetMenuTile(static_cast<TileMenu*>(ptile), false);
        LoveTesterMenu::pInstance = static_cast<LoveTesterMenu*>(pmenu);
        if (static_cast<LoveTesterMenu*>(pmenu)->LoveTesterMenu::CheckTilesExist(&imissingIndex)) {
            LoveTesterMenu::pInstance->iAvailablePoints = aiTotalPoints;
            LoveTesterMenu::pInstance->InitMachine();
            LoveTesterMenu::sp3DCamera.operator=(LoveTesterMenu::pInstance->SetupCamera());
            HUDMainMenu::ForceObjectiveReminders(true);
            pmenu->PrepForVisibility(false);
        }
        else {
            Warning("MENUS:  Special book menu creation failed... Missing index = #%d", imissingIndex);
            if (ptile) {
                delete ptile;
            }
            Interface::EmergencyCloseAllMenusAndBreakStuff();
        }
    }
    else {
        if ((pmenu) && (pmenu->pMenu)) {
            delete pmenu;
        }
    }
}

void LoveTesterMenu::RefreshTotalPointsValue(LoveTesterMenu::LoveTesterTextItem* apItem) {
    if ((apItem->pText[0u]) && (apItem->pText[1])) {
        int32_t v1;
        int32_t v2;
        int32_t v4;
        int32_t v5 = LoveTesterMenu::pInstance->iAvailablePoints - PlayerCharacter::GetSumOfSPECIALStats();
        if (v5 / 10 >= 9) {
            v4 = 9;
        }
        else {
            v4 = v5 / 10;
        }
        if (v4 <= 0) {
            v2 = 0;
        }
        else {
            int32_t v3;
            if (v5 / 10 >= 9) {
                v3 = 9;
            }
            else {
                v3 = v5 / 10;
            }
            v2 = v3;
        }
        if (v5 % 10 >= 0) {
            v1 = v5 % 10;
        }
        else {
            v1 = 0;
        }
        apItem->pTextObj[0u]->SetAppCulled(false);
        if (v2 <= 0) {
            apItem->pText[0u]->SetTexture(
                BSShaderProperty::TEXTYPE_DIFFUSE, 0, LoveTesterMenu::pInstance->spAVValues->m_pObject
            );
        }
        else {
            apItem->pText[0u]->SetTexture(
                BSShaderProperty::TEXTYPE_DIFFUSE, 0, LoveTesterMenu::pInstance->spAVValues[v2].m_pObject
            );
        }
        apItem->pTextObj[1]->SetAppCulled(false);
        apItem->pText[1]->SetTexture(
            BSShaderProperty::TEXTYPE_DIFFUSE, 0, LoveTesterMenu::pInstance->spAVValues[v1].m_pObject
        );
    }
}

// The refresh callbacks take the base button; InitMachine stores this one only in ActorValueButtons.
void LoveTesterMenu::RefreshLeftAVButton(LoveTesterMenu::LoveTesterButton* apButton) {
    ActorValue::Index v4 = static_cast<LoveTesterMenu::ActorValueButton*>(apButton)->eActorValue;
    if (v4 != ActorValue::VALUE_NONE) {
        if (pPlayer->GetPermanentActorValue(v4) <= 1) {
            apButton->spButton->SetAppCulled(true);
        }
        else {
            apButton->spButton->SetAppCulled(false);
        }
        if (InterfaceManager::Has360Controller() && LoveTesterMenu::pInstance->eState == LoveTesterMenu::LTMS_SUMMARY &&
            v4 != LoveTesterMenu::pInstance->eXBoxIndexState + 4) {
            apButton->spButton->SetAppCulled(true);
        }
    }
}

// The refresh callbacks take the base button; InitMachine stores this one only in ActorValueButtons.
void LoveTesterMenu::RefreshRightAVButton(LoveTesterMenu::LoveTesterButton* apButton) {
    ActorValue::Index v4 = static_cast<LoveTesterMenu::ActorValueButton*>(apButton)->eActorValue;
    if (v4 != ActorValue::VALUE_NONE) {
        if (pPlayer->GetPermanentActorValue(v4) >= 10 ||
            (LoveTesterMenu::pInstance &&
             PlayerCharacter::GetSumOfSPECIALStats() >= LoveTesterMenu::pInstance->iAvailablePoints)) {
            apButton->spButton->SetAppCulled(true);
        }
        else {
            apButton->spButton->SetAppCulled(false);
        }
        if (InterfaceManager::Has360Controller() && LoveTesterMenu::pInstance->eState == LoveTesterMenu::LTMS_SUMMARY &&
            v4 != LoveTesterMenu::pInstance->eXBoxIndexState + 4) {
            apButton->spButton->SetAppCulled(true);
        }
    }
}

void LoveTesterMenu::RefreshAllDoneButton(LoveTesterMenu::LoveTesterButton* apButton) {
    if (LoveTesterMenu::pInstance->eState == LoveTesterMenu::LTMS_SUMMARY &&
        PlayerCharacter::GetSumOfSPECIALStats() < LoveTesterMenu::pInstance->iAvailablePoints) {
        apButton->pImage->SetTexture(
            BSShaderProperty::TEXTYPE_DIFFUSE, 0, LoveTesterMenu::pInstance->spNextPageButtonImage->m_pObject
        );
    }
    else {
        apButton->pImage->SetTexture(
            BSShaderProperty::TEXTYPE_DIFFUSE, 0, LoveTesterMenu::pInstance->spNextPageButtonImage[1].m_pObject
        );
    }
}

void LoveTesterMenu::RefreshPreviousPageButton(LoveTesterMenu::LoveTesterButton* apButton) {
    apButton->pImage->SetTexture(
        BSShaderProperty::TEXTYPE_DIFFUSE, 0, LoveTesterMenu::pInstance->spPreviousPageButtonImage->m_pObject
    );
}

void LoveTesterMenu::DecreaseActorValue(LoveTesterMenu::LoveTesterButton* apButton, ...) {
    if (LoveTesterMenu::pInstance) {
        ActorValue::Index eActorValue = static_cast<LoveTesterMenu::ActorValueButton*>(apButton)->eActorValue;
        if (eActorValue != ActorValue::VALUE_NONE) {
            int32_t v6 = pPlayer->GetPermanentActorValue(eActorValue);
            if (v6 > 1) {
                BSSoundHandle th_s;
                BSAudio* v1 = &BSAudio::QInstance();
                th_s = v1->GetSoundHandleByName("OBJBookSpecialNumberDown", 289u);
                th_s.Play(false);
            }
            int32_t v2 = BSMAX(v6 - 1, 1);
            pPlayer->SetActorBaseValue(eActorValue, v2);
        }
        LoveTesterMenu::pInstance->RefreshPage();
    }
}

void LoveTesterMenu::DecreaseCurrentActorValue([[maybe_unused]] LoveTesterMenu::LoveTesterButton* apButton, ...) {
    if (LoveTesterMenu::pInstance) {
        ActorValue::Index SelectedSPECIAL = LoveTesterMenu::pInstance->GetSelectedSPECIAL();
        if (SelectedSPECIAL != ActorValue::VALUE_NONE) {
            int32_t v5 = pPlayer->GetPermanentActorValue(SelectedSPECIAL);
            if (v5 > 1) {
                BSSoundHandle th_s;
                BSAudio* v0 = &BSAudio::QInstance();
                th_s = v0->GetSoundHandleByName("OBJBookSpecialNumberDown", 289u);
                th_s.Play(false);
            }
            int32_t v1 = BSMAX(v5 - 1, 1);
            pPlayer->SetActorBaseValue(SelectedSPECIAL, v1);
        }
        LoveTesterMenu::pInstance->RefreshPage();
    }
}

// InitMachine stores this callback only in ActorValueButtons.
void LoveTesterMenu::IncreaseActorValue(LoveTesterMenu::LoveTesterButton* apButton, ...) {
    if (LoveTesterMenu::pInstance) {
        ActorValue::Index v7 = static_cast<LoveTesterMenu::ActorValueButton*>(apButton)->eActorValue;
        if (v7 != ActorValue::VALUE_NONE) {
            int32_t v6 = pPlayer->GetPermanentActorValue(v7);
            if (v6 < 10) {
                BSSoundHandle th_s;
                BSAudio* v1 = &BSAudio::QInstance();
                th_s = v1->GetSoundHandleByName("OBJBookSpecialNumber", 289u);
                th_s.Play(false);
            }
            if (PlayerCharacter::GetSumOfSPECIALStats() < LoveTesterMenu::pInstance->iAvailablePoints) {
                int32_t v2 = BSMIN(v6 + 1, 10);
                pPlayer->SetActorBaseValue(v7, v2);
            }
        }
        LoveTesterMenu::pInstance->RefreshPage();
    }
}

void LoveTesterMenu::IncreaseCurrentActorValue([[maybe_unused]] LoveTesterMenu::LoveTesterButton* apButton, ...) {
    if (LoveTesterMenu::pInstance) {
        ActorValue::Index SelectedSPECIAL = LoveTesterMenu::pInstance->GetSelectedSPECIAL();
        if (SelectedSPECIAL != ActorValue::VALUE_NONE) {
            int32_t v5 = pPlayer->GetPermanentActorValue(SelectedSPECIAL);
            if (v5 < 10) {
                BSSoundHandle th_s;
                BSAudio* v0 = &BSAudio::QInstance();
                th_s = v0->GetSoundHandleByName("OBJBookSpecialNumber", 289u);
                th_s.Play(false);
            }
            if (PlayerCharacter::GetSumOfSPECIALStats() < LoveTesterMenu::pInstance->iAvailablePoints) {
                int32_t v1 = BSMIN(v5 + 1, 10);
                pPlayer->SetActorBaseValue(SelectedSPECIAL, v1);
            }
        }
        LoveTesterMenu::pInstance->RefreshPage();
    }
}

void LoveTesterMenu::AllDoneButtonClick([[maybe_unused]] LoveTesterMenu::LoveTesterButton* apButton, ...) {
    if (LoveTesterMenu::pInstance &&
        PlayerCharacter::GetSumOfSPECIALStats() == LoveTesterMenu::pInstance->iAvailablePoints) {
        BSAudio* v0 = &BSAudio::QInstance();
        v0->GetSoundHandleByName("OBJBookSpecialNumberFinished", 289u).Play(false);
        LoveTesterMenu::Close();
    }
    else {
        BSAudio* v2 = &BSAudio::QInstance();
        v2->GetSoundHandleByName("UIActivateNothing", 289u).Play(false);
    }
}
