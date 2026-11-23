#include "Fallout/Interface/Menus/SPECIALBookMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Controls.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/Setting.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderAccumulator.h"
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
#include "Fallout/Interface/Menus/LoveTesterMenu.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/ehvec.h"

#include <cmath>
#include <cstdarg>
#include <cstdint>

NiPointer<NiCamera> SPECIALBookMenu::sp3DCamera;

void SPECIALBookDoneCallback() {
    if (SPECIALBookMenu::pSPECIALBookMenu &&
        PlayerCharacter::GetSumOfSPECIALStats() == SPECIALBookMenu::pSPECIALBookMenu->iAvailablePoints) {
        SPECIALBookMenu::pSPECIALBookMenu->Exit();
    }
    else {
        BSAudio::QInstance().GetSoundHandleByName("UIActivateNothing", 289u).Play(false);
    }
}

// The callers pass the callbacks untyped: the casts give the untyped pointers their callback types.
SPECIALBookMenu::SpecialBookButton::SpecialBookButton(void* apButtonClick, void* apRefresh, const char* apButtonName)
  : pfButtonClick(reinterpret_cast<void (*)(SPECIALBookMenu::SpecialBookButton*, ...)>(apButtonClick)),
    pfRefresh(reinterpret_cast<void (*)(SPECIALBookMenu::SpecialBookButton*)>(apRefresh)), spButton() {
    NiAVObject* pBook = SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry;
    spButton = pBook->GetObjectByName(NiFixedString(apButtonName));
    NiAVObject* pobj = pBook->GetObjectByName(NiFixedString(apButtonName));
    // Shader property types 8 to 12 are the per-pixel lighting properties.
    NiProperty* pProperty = pobj->GetProperty(3);
    if (pProperty &&
        static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() >= 8 &&
        static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() <= 12) {
        pImage = static_cast<BSShaderPPLightingProperty*>(pProperty);
    }
    else {
        pImage = nullptr;
    }
}

SPECIALBookMenu::SpecialBookTextItem::SpecialBookTextItem(
    void (*apRefreshFieldCallback)(SPECIALBookMenu::SpecialBookTextItem*),
    const char* apTextObjectName1,
    const char* apTextObjectName2,
    ActorValue::Index aeActorValue
)
  : pfRefreshField(apRefreshFieldCallback), eActorValue(aeActorValue) {
    // The second object and its property are set only when a second name is given.
    const char* apNames[2] = {apTextObjectName1, apTextObjectName2};
    for (int32_t i = 0; i < 2; ++i) {
        if (i == 1 && !apTextObjectName2) {
            break;
        }
        pTextObj[i] = SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->GetObjectByName(NiFixedString(apNames[i]));
        // Shader property types 8 to 12 are the per-pixel lighting properties.
        NiProperty* pProperty = pTextObj[i]->GetProperty(3);
        if (pProperty &&
            static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() >= 8 &&
            static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() <= 12) {
            pText[i] = static_cast<BSShaderPPLightingProperty*>(pProperty);
        }
        else {
            pText[i] = nullptr;
        }
    }
}

SPECIALBookMenu::ActorValueButton::ActorValueButton(
    void* apButtonClick,
    void* apRefresh,
    const char* apButtonName,
    int32_t aeActorValue,
    int32_t aiNumAffectedFields,
    ...
)
  : SpecialBookButton(apButtonClick, apRefresh, apButtonName), AffectedFields(),
    eActorValue(static_cast<ActorValue::Index>(aeActorValue)) {
    va_list ap;
    va_start(ap, aiNumAffectedFields);
    for (int32_t i = 0; i < aiNumAffectedFields; ++i) {
        SpecialBookTextItem* pItem = va_arg(ap, SpecialBookTextItem*);
        AffectedFields.Add(pItem);
    }
    va_end(ap);
}

// The page arrays, the NiPointer arrays and the Menu base release themselves.
SPECIALBookMenu::~SPECIALBookMenu() {}

void SPECIALBookMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (static_cast<uint32_t>(aiID) < 8) {
        pTiles[aiID] = apTile;
    }
}

void SPECIALBookMenu::DoClick(int32_t aiID, Tile* apTarget) {
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
                CallPageCallbacks();
            }
            break;
        }
        case 2:
        case 3: {
            int32_t ioldValue = 0;
            int32_t inewValue = 0;
            if (GetSelectedSPECIAL() == ActorValue::VALUE_NONE) {
                if (InterfaceManager::Has360Controller() && eState == SPECIALBookMenu::SBMS_SPECIAL) {
                    ActorValue::Index eActorValue = static_cast<ActorValue::Index>(eXBoxIndexState + 4);
                    ioldValue = pPlayer->GetPermanentActorValue(eActorValue);
                    if (aiID == 2) {
                        if (PlayerCharacter::GetSumOfSPECIALStats() < iAvailablePoints) {
                            pPlayer->SetActorBaseValue(eActorValue, BSMIN(ioldValue + 1, 10));
                        }
                    }
                    else {
                        pPlayer->SetActorBaseValue(eActorValue, BSMAX(ioldValue - 1, 1));
                    }
                    inewValue = pPlayer->GetBaseActorValue(eActorValue);
                }
            }
            else {
                ActorValue::Index eActorValue = GetSelectedSPECIAL();
                ioldValue = pPlayer->GetPermanentActorValue(eActorValue);
                if (aiID == 2) {
                    if (PlayerCharacter::GetSumOfSPECIALStats() < iAvailablePoints) {
                        int32_t iValue = BSMIN(ioldValue + 1, 10);
                        eActorValue = GetSelectedSPECIAL();
                        pPlayer->SetActorBaseValue(eActorValue, iValue);
                    }
                }
                else {
                    int32_t iValue = BSMAX(ioldValue - 1, 1);
                    eActorValue = GetSelectedSPECIAL();
                    pPlayer->SetActorBaseValue(eActorValue, iValue);
                }
                eActorValue = GetSelectedSPECIAL();
                inewValue = pPlayer->GetBaseActorValue(eActorValue);
            }
            if (ioldValue != inewValue) {
                BSSoundHandle psoundHand;
                psoundHand = BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialNumber", 289u);
                psoundHand.Play(false);
            }
            CallPageCallbacks();
            break;
        }
        case 4: {
            if (PlayerCharacter::GetSumOfSPECIALStats() == iAvailablePoints) {
                Exit();
            }
            else {
                BSAudio::QInstance().GetSoundHandleByName("UIActivateNothing", 289u).Play(false);
            }
            break;
        }
        case 5: {
            if (eState < SPECIALBookMenu::SBMS_CLOSED) {
                InterfaceManager* pim = InterfaceManager::Instance();
                int32_t cx = static_cast<int32_t>(pim->sRealCursorPos.x);
                int32_t cy = static_cast<int32_t>(pim->sRealCursorPos.z);
                if (BSShaderManager::bLetterbox) {
                    NiRenderer* pRenderer = NiRenderer::GetRenderer();
                    double dBottom = BSShaderManager::kFramebufferViewport.m_bottom;
                    cy = static_cast<int32_t>(
                        static_cast<double>(pRenderer->GetDefaultBackBuffer()->m_uiHeight) * dBottom +
                        (BSShaderManager::kFramebufferViewport.m_top - BSShaderManager::kFramebufferViewport.m_bottom) *
                            static_cast<double>(cy)
                    );
                }
                for (int32_t iindex = 0; iindex < Pages[eState].Buttons.iSize; ++iindex) {
                    NiPoint3 dir;
                    NiPoint3 origin;
                    NiPick activatePick(0, 8u);
                    activatePick.SetObserveAppCullFlag(true);
                    activatePick.SetTarget(Pages[eState].Buttons[iindex]->spButton.m_pObject);
                    activatePick.m_eSortType = NiPick::SORT;
                    activatePick.SetIsReturnTexture(true);
                    SPECIALBookMenu::sp3DCamera->WindowPointToRay(cx, cy, origin, dir, nullptr);
                    if (activatePick.PickObjects(origin, dir, false)) {
                        if (activatePick.m_pickResults.GetAt(0)) {
                            // The button is fetched once for the argument and again for the callback.
                            SpecialBookButton* pButton = Pages[eState].Buttons[iindex];
                            Pages[eState].Buttons[iindex]->pfButtonClick(pButton);
                            return;
                        }
                    }
                }
            }
            break;
        }
        case 6: {
            if (eState == SPECIALBookMenu::SBMS_SPECIAL && eXBoxIndexState > SPECIALBookMenu::SBMS_STRENGTH) {
                eXBoxIndexState = static_cast<SPECIALBookMenu::SPECIAL_BOOK_MENU_STATE>(eXBoxIndexState - 1);
                BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialFocus", 289u).Play(false);
            }
            CallPageCallbacks();
            break;
        }
        case 7: {
            if (eState == SPECIALBookMenu::SBMS_SPECIAL && eXBoxIndexState < SPECIALBookMenu::SBMS_LUCK) {
                eXBoxIndexState = static_cast<SPECIALBookMenu::SPECIAL_BOOK_MENU_STATE>(eXBoxIndexState + 1);
                BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialFocus", 289u).Play(false);
            }
            CallPageCallbacks();
            break;
        }
        default: {
            break;
        }
    }
}

void SPECIALBookMenu::DoIdle() {
    if (Interface::IsTopMenuID(Interface::eSPECIALBookMenu)) {
        Controls* pControls = appMain->GetControls();
        int32_t iKey = 30;
        if (!BSstrcmp(sLanguage.String(), "FRENCH")) {
            iKey = 16;
        }
        if (pControls->GetKeyboardButton(iKey, 1)) {
            DoClick(4, nullptr);
        }
        else {
            NiTimeController* pspecialBookGeomController = pBookGeometry->GetControllers();
            if (pspecialBookGeomController) {
                NiControllerManager* pspecialBookGeomControllerManager = static_cast<NiControllerManager*>(
                    pspecialBookGeomController
                );
                NiControllerSequence* pseq = nullptr;
                if (!bAnimForward || eState > SPECIALBookMenu::SBMS_START) {
                    char* pcString = bAnimForward ? aForwardAnimations[eState] : ReverseAnimations[eState];
                    NiFixedString kName(static_cast<const char*>(pcString));
                    pseq = pspecialBookGeomControllerManager->GetSequenceByName(kName);
                }
                fFrameAnimTimer = appTimer.fDelta + fFrameAnimTimer;
                if (pseq) {
                    pseq->Update(fFrameAnimTimer, true);
                    double dLastTime = pseq->m_fLastTime;
                    bAnimFinished = !(pseq->m_fEndKeyTime > dLastTime) || eState <= SPECIALBookMenu::SBMS_START;
                }
                else {
                    bAnimFinished = true;
                }
                while (pspecialBookGeomController) {
                    if (NiObject::DynamicCastStatic(
                            &NiMultiTargetTransformController::ms_RTTI,
                            pspecialBookGeomController
                        )) {
                        NiUpdateData UpdateData(fFrameAnimTimer, false, false);
                        pspecialBookGeomController->Update(UpdateData);
                        break;
                    }
                    pspecialBookGeomController = pspecialBookGeomController->GetNext();
                }
            }
            NiUpdateData update(0.0f, false, false);
            pBookGeometry->Update(update);
        }
    }
}

bool SPECIALBookMenu::DoKey(int32_t aiKey) {
    int8_t bHandled = 0;
    if (aiKey == -2147483647 || aiKey == -2147483646) {
        DoClick(aiKey == -2147483647, nullptr);
        bHandled = 1;
    }
    if (aiKey == -2147483645 || aiKey == -2147483644) {
        DoGamepad((aiKey != -2147483645) + 3, 1.0f);
        return true;
    }
    return bHandled;
}

int32_t SPECIALBookMenu::GetClass() {
    return 1060;
}

bool SPECIALBookMenu::DoGamepad(int32_t aiButton, float afValue) {
    bool bHandled;
    if (!Interface::IsTopMenuID(Interface::eSPECIALBookMenu)) {
        return false;
    }
    ActorValue::Index eav = GetSelectedSPECIAL();
    switch (aiButton) {
        case 3:
        case 9: {
            if (eav != ActorValue::VALUE_NONE &&
                PlayerCharacter::GetSumOfSPECIALStats() < iAvailablePoints &&
                pPlayer->GetPermanentActorValue(eav) < 10) {
                DoClick(2, nullptr);
            }
            bHandled = true;
            break;
        }
        case 4:
        case 10: {
            if (eav != ActorValue::VALUE_NONE && pPlayer->GetPermanentActorValue(eav) > 1) {
                DoClick(3, nullptr);
            }
            bHandled = true;
            break;
        }
        default: {
            return false;
        }
    }
    return bHandled;
}

// The menu's close code.
void SPECIALBookMenu::Exit() {
    Tile* pMenuTile = Menu::GetMenuByClass(SPECIALBookMenu::GetClassStatic());
    if (pMenuTile) {
        pModelLoader->ReleaseFile("Meshes\\Terminals\\Babybook02.NIF");
        BSShaderManager::GetShadowSceneNode(1)->RemoveAllChildren();
        BSShaderManager::GetShadowSceneNode(1)->RemoveAllLights();
        SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry = nullptr;
        SPECIALBookMenu::sp3DCamera = static_cast<NiCamera*>(nullptr);
        for (int32_t i = 0; i < 9; ++i) {
            SPECIALBookMenu::pSPECIALBookMenu->Pages[i].Buttons.Clear(true);
            SPECIALBookMenu::pSPECIALBookMenu->Pages[i].TextItems.Clear(true);
        }
        if (SPECIALBookMenu::pSPECIALBookMenu) {
            pMenuTile->Set(Tile::kTileValue_DeleteOnFade, 1);
            SPECIALBookMenu::pSPECIALBookMenu->StartFadeOut();
        }
    }
}

ActorValue::Index SPECIALBookMenu::GetSelectedSPECIAL() {
    if (eState <= SPECIALBookMenu::SBMS_START || eState >= SPECIALBookMenu::SBMS_CLOSED) {
        return ActorValue::VALUE_NONE;
    }
    if (InterfaceManager::Has360Controller() && eState == SPECIALBookMenu::SBMS_SPECIAL) {
        return static_cast<ActorValue::Index>(eXBoxIndexState + 4);
    }
    return static_cast<ActorValue::Index>(eState + 4);
}

bool SPECIALBookMenu::TurnPage(bool bForward) {
    bool brunAnim = false;
    if (bForward) {
        bAnimForward = 1;
        if (eState >= SPECIALBookMenu::SBMS_CLOSED) {
            return false;
        }
        eState = static_cast<SPECIALBookMenu::SPECIAL_BOOK_MENU_STATE>(eState + 1);
        brunAnim = true;
        BSSoundHandle psoundHand;
        psoundHand = BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialPageTurn", 289u);
        psoundHand.Play(false);
    }
    else {
        bAnimForward = 0;
        if (eState < SPECIALBookMenu::SBMS_STRENGTH) {
            return false;
        }
        eState = static_cast<SPECIALBookMenu::SPECIAL_BOOK_MENU_STATE>(eState - 1);
        brunAnim = true;
        BSSoundHandle psoundHand;
        psoundHand = BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialPageTurn", 289u);
        psoundHand.Play(false);
    }
    if (brunAnim) {
        NiTimeController* pSPECIALBookGeomController = SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->GetController(
            &NiControllerManager::ms_RTTI
        );
        if (pSPECIALBookGeomController) {
            NiControllerManager* pSPECIALBookGeomControllerManager = static_cast<NiControllerManager*>(
                pSPECIALBookGeomController
            );
            NiControllerSequence* pseq = nullptr;
            if (!bAnimForward || eState > SPECIALBookMenu::SBMS_START) {
                char* pcString = bAnimForward ? aForwardAnimations[eState] : ReverseAnimations[eState];
                NiFixedString kName(pcString);
                pseq = pSPECIALBookGeomControllerManager->GetSequenceByName(kName);
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

void SPECIALBookMenu::CallPageCallbacks() {
    if (eState < SPECIALBookMenu::SBMS_CLOSED) {
        for (uint32_t auiIndex = 0; auiIndex < Pages[eState].TextItems.iSize; ++auiIndex) {
            // Fetches the item for the argument first, then again for the callback.
            SpecialBookTextItem* pItem = Pages[eState].TextItems[auiIndex];
            Pages[eState].TextItems[auiIndex]->pfRefreshField(pItem);
        }
        for (uint32_t i = 0; i < Pages[eState].Buttons.iSize; ++i) {
            SpecialBookButton* pButton = Pages[eState].Buttons[i];
            Pages[eState].Buttons[i]->pfRefresh(pButton);
        }
    }
}

// Loads the book's model, starts its "Forward" sequence and lights it, then builds the nine pages: the cover's
// look-inside button, a page per SPECIAL (its value, message and points-remaining items, the decrease and increase
// buttons on them, the page arrows) and the index page, and runs every page's refresh callbacks once.
void SPECIALBookMenu::InitBook() {
    SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry = pModelLoader->LoadFile(
        "Meshes\\Terminals\\Babybook02.NIF",
        LOD_MULT_NONE,
        true,
        false,
        false,
        false
    );
    SwapPlatformLanguageTextures(SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry);
    SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->SetScale(0.05f);
    SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->SetTranslate(0.0f, 0.0f, -60.0f);
    SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->SetRotate(1.5707964f, 1.0f, 0.0f, 0.0f);
    NiUpdateData arData(0.0f, false, false);
    SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->Update(arData);
    SPECIALBookMenu::pSPECIALBookMenu->fFrameAnimTimer = appTimer.fDelta +
        SPECIALBookMenu::pSPECIALBookMenu->fFrameAnimTimer;
    NiTimeController* pController = SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->GetController(
        &NiControllerManager::ms_RTTI
    );
    if (pController) {
        NiControllerManager* pManager = static_cast<NiControllerManager*>(pController);
        NiControllerSequence* pkSequence = pManager->GetSequenceByName("Forward");
        if (pkSequence) {
            pManager->DeactivateAll(0.0f);
            pManager->ActivateSequence(pkSequence, 0, true, 1.0f, 0.0f, nullptr);
            pManager->SetActive(true);
            pkSequence->ResetSequence();
        }
    }
    NiTimeController::StartAnimations(pBookGeometry);
    while (pController) {
        if (NiObject::DynamicCastStatic(&NiMultiTargetTransformController::ms_RTTI, pController)) {
            NiUpdateData kControllerData(0.0f, false, false);
            pController->Update(kControllerData);
            break;
        }
        pController = pController->GetNext();
    }
    NiUpdateData arUpdateData(0.0f, false, false);
    pBookGeometry->Update(arUpdateData);
    const NiBound* pWorldBound = &SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->GetWorldBound();
    float fLightRadius = pWorldBound->m_fRadius * 20.0;
    NiNode* pBookNode = SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->IsNode();
    ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(1);
    if (!pShadowSceneNode->AddAllLightsFromNode_Recurse(pBookNode, fLightRadius)) {
        NiPointLight* pLight;
        pLight = new NiPointLight;
        NiPoint3 kTranslate;
        pLight->SetTranslate(*::new (&kTranslate) NiPoint3(0.0f, 0.0f, 0.0f));
        pLight->SetDiffuseColor(NiColor(0.85f, 0.85f, 0.85f));
        // The specular colour's red channel is given the light radius, deliberately.
        pLight->SetSpecularColor(NiColor(fLightRadius, 0.0f, 0.0f));
        pLight->SetDimmer(1.0f);
        NiUpdateData kLightData(0.0f, false, false);
        pLight->Update(kLightData);
        BSShaderManager::GetShadowSceneNode(1)->AttachChild(pLight, 1);
        BSShaderManager::GetShadowSceneNode(1)->AddLight(pLight, false);
    }
    BSShaderManager::GetShadowSceneNode(1)->AttachChild(SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry, 1);

    // The cover: the look-inside button.
    SpecialBookButton* pNext = new SpecialBookButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::NextPage),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshNextPageButton),
        "LookInside_Btn:0"
    );
    Pages[0].Buttons.Add(pNext);

    // Page 1, Strength.
    SpecialBookTextItem* pPointVal = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshActorValue,
        "P1_PointVal:0",
        nullptr,
        ActorValue::STRENGTH
    );
    SpecialBookTextItem* pMessage = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshTotalPointsText,
        "P1_Message:0",
        nullptr,
        ActorValue::VALUE_NONE
    );
    SpecialBookTextItem* pPointsRemain = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshTotalPointsValue,
        "P1_PointsRemainDigit1:0",
        "P1_PointsRemainDigit2:0",
        ActorValue::VALUE_NONE
    );
    ActorValueButton* pDecrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton),
        "P1_Decrease_Btn:0",
        ActorValue::STRENGTH,
        2,
        pPointVal,
        pPointsRemain
    );
    ActorValueButton* pIncrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton),
        "P1_Increase_Btn:0",
        ActorValue::STRENGTH,
        2,
        pPointVal,
        pPointsRemain
    );
    pNext = new SpecialBookButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::NextPage),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshNextPageButton),
        "P1_RT_Btn:0"
    );
    SpecialBookButton* pPrevious = new SpecialBookButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::PreviousPage),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshPreviousPageButton),
        "P1_LT_Btn:0"
    );
    Pages[1].TextItems.Add(pPointVal);
    Pages[1].TextItems.Add(pMessage);
    Pages[1].TextItems.Add(pPointsRemain);
    Pages[1].Buttons.Add(pDecrease);
    Pages[1].Buttons.Add(pIncrease);
    Pages[1].Buttons.Add(pNext);
    Pages[1].Buttons.Add(pPrevious);

    // Page 2, Perception.
    pPointVal = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshActorValue,
        "P2_PointVal:0",
        nullptr,
        ActorValue::PERCEPTION
    );
    pMessage = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshTotalPointsText,
        "P2_Message:0",
        nullptr,
        ActorValue::VALUE_NONE
    );
    pPointsRemain = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshTotalPointsValue,
        "P2_PointsRemainDigit1:0",
        "P2_PointsRemainDigit2:0",
        ActorValue::VALUE_NONE
    );
    pDecrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton),
        "P2_Decrease_Btn:0",
        ActorValue::PERCEPTION,
        2,
        pPointVal,
        pPointsRemain
    );
    pIncrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton),
        "P2_Increase_Btn:0",
        ActorValue::PERCEPTION,
        2,
        pPointVal,
        pPointsRemain
    );
    pNext = new SpecialBookButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::NextPage),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshNextPageButton),
        "P2_RT_Btn:0"
    );
    pPrevious = new SpecialBookButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::PreviousPage),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshPreviousPageButton),
        "P2_LT_Btn:0"
    );
    Pages[2].TextItems.Add(pPointVal);
    Pages[2].TextItems.Add(pMessage);
    Pages[2].TextItems.Add(pPointsRemain);
    Pages[2].Buttons.Add(pDecrease);
    Pages[2].Buttons.Add(pIncrease);
    Pages[2].Buttons.Add(pNext);
    Pages[2].Buttons.Add(pPrevious);

    // Page 3, Endurance.
    pPointVal = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshActorValue,
        "P3_PointVal:0",
        nullptr,
        ActorValue::ENDURANCE
    );
    pMessage = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshTotalPointsText,
        "P3_Message:0",
        nullptr,
        ActorValue::VALUE_NONE
    );
    pPointsRemain = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshTotalPointsValue,
        "P3_PointsRemainDigit1:0",
        "P3_PointsRemainDigit2:0",
        ActorValue::VALUE_NONE
    );
    pDecrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton),
        "P3_Decrease_Btn:0",
        ActorValue::ENDURANCE,
        2,
        pPointVal,
        pPointsRemain
    );
    pIncrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton),
        "P3_Increase_Btn:0",
        ActorValue::ENDURANCE,
        2,
        pPointVal,
        pPointsRemain
    );
    pNext = new SpecialBookButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::NextPage),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshNextPageButton),
        "P3_RT_Btn:0"
    );
    pPrevious = new SpecialBookButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::PreviousPage),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshPreviousPageButton),
        "P3_LT_Btn:0"
    );
    Pages[3].TextItems.Add(pPointVal);
    Pages[3].TextItems.Add(pMessage);
    Pages[3].TextItems.Add(pPointsRemain);
    Pages[3].Buttons.Add(pDecrease);
    Pages[3].Buttons.Add(pIncrease);
    Pages[3].Buttons.Add(pNext);
    Pages[3].Buttons.Add(pPrevious);

    // Page 4, Charisma.
    pPointVal = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshActorValue,
        "P4_PointVal:0",
        nullptr,
        ActorValue::CHARISMA
    );
    pMessage = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshTotalPointsText,
        "P4_Message:0",
        nullptr,
        ActorValue::VALUE_NONE
    );
    pPointsRemain = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshTotalPointsValue,
        "P4_PointsRemainDigit1:0",
        "P4_PointsRemainDigit2:0",
        ActorValue::VALUE_NONE
    );
    pDecrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton),
        "P4_Decrease_Btn:0",
        ActorValue::CHARISMA,
        2,
        pPointVal,
        pPointsRemain
    );
    pIncrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton),
        "P4_Increase_Btn:0",
        ActorValue::CHARISMA,
        2,
        pPointVal,
        pPointsRemain
    );
    pNext = new SpecialBookButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::NextPage),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshNextPageButton),
        "P4_RT_Btn:0"
    );
    pPrevious = new SpecialBookButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::PreviousPage),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshPreviousPageButton),
        "P4_LT_Btn:0"
    );
    Pages[4].TextItems.Add(pPointVal);
    Pages[4].TextItems.Add(pMessage);
    Pages[4].TextItems.Add(pPointsRemain);
    Pages[4].Buttons.Add(pDecrease);
    Pages[4].Buttons.Add(pIncrease);
    Pages[4].Buttons.Add(pNext);
    Pages[4].Buttons.Add(pPrevious);

    // Page 5, Intelligence.
    pPointVal = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshActorValue,
        "P5_PointVal:0",
        nullptr,
        ActorValue::INTELLIGENCE
    );
    pMessage = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshTotalPointsText,
        "P5_Message:0",
        nullptr,
        ActorValue::VALUE_NONE
    );
    pPointsRemain = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshTotalPointsValue,
        "P5_PointsRemainDigit1:0",
        "P5_PointsRemainDigit2:0",
        ActorValue::VALUE_NONE
    );
    pDecrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton),
        "P5_Decrease_Btn:0",
        ActorValue::INTELLIGENCE,
        2,
        pPointVal,
        pPointsRemain
    );
    pIncrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton),
        "P5_Increase_Btn:0",
        ActorValue::INTELLIGENCE,
        2,
        pPointVal,
        pPointsRemain
    );
    pNext = new SpecialBookButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::NextPage),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshNextPageButton),
        "P5_RT_Btn:0"
    );
    pPrevious = new SpecialBookButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::PreviousPage),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshPreviousPageButton),
        "P5_LT_Btn:0"
    );
    Pages[5].TextItems.Add(pPointVal);
    Pages[5].TextItems.Add(pMessage);
    Pages[5].TextItems.Add(pPointsRemain);
    Pages[5].Buttons.Add(pDecrease);
    Pages[5].Buttons.Add(pIncrease);
    Pages[5].Buttons.Add(pNext);
    Pages[5].Buttons.Add(pPrevious);

    // Page 6, Agility.
    pPointVal = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshActorValue,
        "P6_PointVal:0",
        nullptr,
        ActorValue::AGILITY
    );
    pMessage = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshTotalPointsText,
        "P6_Message:0",
        nullptr,
        ActorValue::VALUE_NONE
    );
    pPointsRemain = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshTotalPointsValue,
        "P6_PointsRemainDigit1:0",
        "P6_PointsRemainDigit2:0",
        ActorValue::VALUE_NONE
    );
    pDecrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton),
        "P6_Decrease_Btn:0",
        ActorValue::AGILITY,
        2,
        pPointVal,
        pPointsRemain
    );
    pIncrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton),
        "P6_Increase_Btn:0",
        ActorValue::AGILITY,
        2,
        pPointVal,
        pPointsRemain
    );
    pNext = new SpecialBookButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::NextPage),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshNextPageButton),
        "P6_RT_Btn:0"
    );
    pPrevious = new SpecialBookButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::PreviousPage),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshPreviousPageButton),
        "P6_LT_Btn:0"
    );
    Pages[6].TextItems.Add(pPointVal);
    Pages[6].TextItems.Add(pMessage);
    Pages[6].TextItems.Add(pPointsRemain);
    Pages[6].Buttons.Add(pDecrease);
    Pages[6].Buttons.Add(pIncrease);
    Pages[6].Buttons.Add(pNext);
    Pages[6].Buttons.Add(pPrevious);

    // Page 7, Luck.
    pPointVal = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshActorValue,
        "P7_PointVal:0",
        nullptr,
        ActorValue::LUCK
    );
    pMessage = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshTotalPointsText,
        "P7_Message:0",
        nullptr,
        ActorValue::VALUE_NONE
    );
    pPointsRemain = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshTotalPointsValue,
        "P7_PointsRemainDigit1:0",
        "P7_PointsRemainDigit2:0",
        ActorValue::VALUE_NONE
    );
    pDecrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton),
        "P7_Decrease_Btn:0",
        ActorValue::LUCK,
        2,
        pPointVal,
        pPointsRemain
    );
    pIncrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton),
        "P7_Increase_Btn:0",
        ActorValue::LUCK,
        2,
        pPointVal,
        pPointsRemain
    );
    pNext = new SpecialBookButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::NextPage),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshNextPageButton),
        "P7_RT_Btn:0"
    );
    pPrevious = new SpecialBookButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::PreviousPage),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshPreviousPageButton),
        "P7_LT_Btn:0"
    );
    Pages[7].TextItems.Add(pPointVal);
    Pages[7].TextItems.Add(pMessage);
    Pages[7].TextItems.Add(pPointsRemain);
    Pages[7].Buttons.Add(pDecrease);
    Pages[7].Buttons.Add(pIncrease);
    Pages[7].Buttons.Add(pNext);
    Pages[7].Buttons.Add(pPrevious);

    // Page 8, the index: each SPECIAL's value with its decrease and increase buttons. Those buttons are handed page 7's
    // points-remaining item; the index's own is built after them.
    pPointVal = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshActorValue,
        "Index_StrengthPointVal:0",
        nullptr,
        ActorValue::STRENGTH
    );
    Pages[8].TextItems.Add(pPointVal);
    pDecrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton),
        "Index_StrengthDecrease_Btn:0",
        ActorValue::STRENGTH,
        2,
        pPointVal,
        pPointsRemain
    );
    Pages[8].Buttons.Add(pDecrease);
    pIncrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton),
        "Index_StrengthIncrease_Btn:0",
        ActorValue::STRENGTH,
        2,
        pPointVal,
        pPointsRemain
    );
    Pages[8].Buttons.Add(pIncrease);
    pPointVal = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshActorValue,
        "Index_PerceptionPointVal:0",
        nullptr,
        ActorValue::PERCEPTION
    );
    Pages[8].TextItems.Add(pPointVal);
    pDecrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton),
        "Index_PerceptionDecrease_Btn:0",
        ActorValue::PERCEPTION,
        2,
        pPointVal,
        pPointsRemain
    );
    Pages[8].Buttons.Add(pDecrease);
    pIncrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton),
        "Index_PerceptionIncrease_Btn:0",
        ActorValue::PERCEPTION,
        2,
        pPointVal,
        pPointsRemain
    );
    Pages[8].Buttons.Add(pIncrease);
    pPointVal = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshActorValue,
        "Index_EndurancePointVal:0",
        nullptr,
        ActorValue::ENDURANCE
    );
    Pages[8].TextItems.Add(pPointVal);
    pDecrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton),
        "Index_EnduranceDecrease_Btn:0",
        ActorValue::ENDURANCE,
        2,
        pPointVal,
        pPointsRemain
    );
    Pages[8].Buttons.Add(pDecrease);
    pIncrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton),
        "Index_EnduranceIncrease_Btn:0",
        ActorValue::ENDURANCE,
        2,
        pPointVal,
        pPointsRemain
    );
    Pages[8].Buttons.Add(pIncrease);
    pPointVal = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshActorValue,
        "Index_CharismaPointVal:0",
        nullptr,
        ActorValue::CHARISMA
    );
    Pages[8].TextItems.Add(pPointVal);
    pDecrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton),
        "Index_CharismaDecrease_Btn:0",
        ActorValue::CHARISMA,
        2,
        pPointVal,
        pPointsRemain
    );
    Pages[8].Buttons.Add(pDecrease);
    pIncrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton),
        "Index_CharismaIncrease_Btn:0",
        ActorValue::CHARISMA,
        2,
        pPointVal,
        pPointsRemain
    );
    Pages[8].Buttons.Add(pIncrease);
    pPointVal = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshActorValue,
        "Index_IntelligencePointVal:0",
        nullptr,
        ActorValue::INTELLIGENCE
    );
    Pages[8].TextItems.Add(pPointVal);
    pDecrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton),
        "Index_IntelligenceDecrease_Btn:0",
        ActorValue::INTELLIGENCE,
        2,
        pPointVal,
        pPointsRemain
    );
    Pages[8].Buttons.Add(pDecrease);
    pIncrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton),
        "Index_IntelligenceIncrease_Btn:0",
        ActorValue::INTELLIGENCE,
        2,
        pPointVal,
        pPointsRemain
    );
    Pages[8].Buttons.Add(pIncrease);
    pPointVal = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshActorValue,
        "Index_AgilityPointVal:0",
        nullptr,
        ActorValue::AGILITY
    );
    Pages[8].TextItems.Add(pPointVal);
    pDecrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton),
        "Index_AgilityDecrease_Btn:0",
        ActorValue::AGILITY,
        2,
        pPointVal,
        pPointsRemain
    );
    Pages[8].Buttons.Add(pDecrease);
    pIncrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton),
        "Index_AgilityIncrease_Btn:0",
        ActorValue::AGILITY,
        2,
        pPointVal,
        pPointsRemain
    );
    Pages[8].Buttons.Add(pIncrease);
    pPointVal = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshActorValue,
        "Index_LuckPointVal:0",
        nullptr,
        ActorValue::LUCK
    );
    Pages[8].TextItems.Add(pPointVal);
    pDecrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton),
        "Index_LuckDecrease_Btn:0",
        ActorValue::LUCK,
        2,
        pPointVal,
        pPointsRemain
    );
    Pages[8].Buttons.Add(pDecrease);
    pIncrease = new ActorValueButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton),
        "Index_LuckIncrease_Btn:0",
        ActorValue::LUCK,
        2,
        pPointVal,
        pPointsRemain
    );
    Pages[8].Buttons.Add(pIncrease);
    pMessage = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshTotalPointsText,
        "Index_Message:0",
        nullptr,
        ActorValue::VALUE_NONE
    );
    Pages[8].TextItems.Add(pMessage);
    pPointsRemain = new SpecialBookTextItem(
        &SPECIALBookMenu::RefreshTotalPointsValue,
        "Index_PointsRemainDigit1:0",
        "Index_PointsRemainDigit2:0",
        ActorValue::VALUE_NONE
    );
    Pages[8].TextItems.Add(pPointsRemain);
    pNext = new SpecialBookButton(
        reinterpret_cast<void*>(&SPECIALBookDoneCallback),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshAllDoneButton),
        "AllDone_Btn:0"
    );
    Pages[8].Buttons.Add(pNext);
    pPrevious = new SpecialBookButton(
        reinterpret_cast<void*>(&SPECIALBookMenu::PreviousPage),
        reinterpret_cast<void*>(&SPECIALBookMenu::RefreshPreviousPageButton),
        "Index_LT_Btn:0"
    );
    Pages[8].Buttons.Add(pPrevious);

    LoadBBTextures();
    // Every page's refresh callbacks, each item and button fetched once for the argument and once for the call.
    for (int32_t i = 0; i < 9; ++i) {
        for (uint32_t uiItem = 0; uiItem < Pages[i].TextItems.iSize; ++uiItem) {
            SpecialBookTextItem* pItem = Pages[i].TextItems[uiItem];
            Pages[i].TextItems[uiItem]->pfRefreshField(pItem);
        }
        for (uint32_t uiButton = 0; uiButton < Pages[i].Buttons.iSize; ++uiButton) {
            SpecialBookButton* pButton = Pages[i].Buttons[uiButton];
            Pages[i].Buttons[uiButton]->pfRefresh(pButton);
        }
    }
}

void SPECIALBookMenu::LoadBBTextures() {
    char acPlatform[8];
    char acDest[264];
    char acPath[260];
    for (int32_t i = 0; i < 11; ++i) {
        BSsprintf(acDest, 260, "%s%s%i.dds", "Textures\\Terminals\\", "BBNumber", i);
        SwapPlatformLanguageTexturePath(acDest, acPath, 260u);
        TES::pInstance->CreateTextureImage(acPath, spAVValues[i], false, false);
    }
    // Entries 1 to 4 land in spPointsText[0] to [3].
    for (int32_t j = 1; j < 5; ++j) {
        BSsprintf(acDest, 260, "%s%s%i.dds", "Textures\\Terminals\\ENGLISH\\", "BBMessage0", j);
        SwapPlatformLanguageTexturePath(acDest, acPath, 260u);
        TES::pInstance->CreateTextureImage(acPath, spPointsText[j - 1], false, false);
    }
    if (Interface::Has360Controller()) {
        BSstrcpy(acPlatform, 5, "XBOX");
    }
    else {
        BSstrcpy(acPlatform, 5, "PC");
    }
    BSsprintf(acPath, 260, "%s\\%s\\%s.dds", "Textures\\Terminals\\", acPlatform, "BBRTOff");
    TES::pInstance->CreateTextureImage(acPath, spNextPageButtonImage[1], false, false);
    BSsprintf(acDest, 260, "%s%s.dds", "Textures\\Terminals\\", "BBRTOn");
    SwapPlatformLanguageTexturePath(acDest, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spNextPageButtonImage[0], false, false);
    BSsprintf(acPath, 260, "%s\\%s\\%s.dds", "Textures\\Terminals\\", acPlatform, "BBLTOff");
    TES::pInstance->CreateTextureImage(acPath, spPreviousPageButtonImage[0], false, false);
    BSsprintf(acDest, 260, "%s%s.dds", "Textures\\Terminals\\", "BBLTOn");
    SwapPlatformLanguageTexturePath(acDest, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spPreviousPageButtonImage[1], false, false);
    BSsprintf(acPath, 260, "%s\\%s\\%s.dds", "Textures\\Terminals\\", acPlatform, "BBXOff");
    TES::pInstance->CreateTextureImage(acPath, spOpenCloseBookButtonImage[0], false, false);
    BSsprintf(acPath, 260, "%s\\%s\\%s.dds", "Textures\\Terminals\\", acPlatform, "BBXOn");
    TES::pInstance->CreateTextureImage(acPath, spOpenCloseBookButtonImage[1], false, false);
    BSsprintf(acDest, 260, "%s%s.dds", "Textures\\Terminals\\", "BBDecreaseOff");
    SwapPlatformLanguageTexturePath(acDest, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spAVLeftArrowButtonImage[0], false, false);
    BSsprintf(acDest, 260, "%s%s.dds", "Textures\\Terminals\\", "BBDecreaseOn");
    SwapPlatformLanguageTexturePath(acDest, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spAVLeftArrowButtonImage[1], false, false);
    BSsprintf(acDest, 260, "%s%s.dds", "Textures\\Terminals\\", "BBIncreaseOff");
    SwapPlatformLanguageTexturePath(acDest, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spAVRightArrowButtonImage[0], false, false);
    BSsprintf(acDest, 260, "%s%s.dds", "Textures\\Terminals\\", "BBIncreaseOn");
    SwapPlatformLanguageTexturePath(acDest, acPath, 260u);
    TES::pInstance->CreateTextureImage(acPath, spAVRightArrowButtonImage[1], false, false);
}

NiCamera* SPECIALBookMenu::InitCamera() {
    NiRect<float> port;
    NiCamera* pcam = new NiCamera;
    ::new (&port) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
    if (BSShaderManager::bLetterbox) {
        port = BSShaderManager::kFramebufferViewport;
    }
    pcam->SetViewPort(port);
    NiFrustum fr(false);
    fr.m_fFar = Main::GetWorldSceneGraph()->GetFarDistance();
    fr.m_fNear = BSSceneGraph::fNearDistance.Float();
    pcam->SetMaxFarNearRatio(fr.m_fFar / fr.m_fNear);
    int32_t iScreenHeight = BSShaderManager::bLetterbox ? BSShaderManager::iLetterboxHeight
                                                        : Renderer::GetScreenHeight();
    float fHeightByWidth = static_cast<double>(iScreenHeight) / static_cast<double>(Renderer::GetScreenWidth());
    float theta = BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369 * 0.15000000596046448;
    fr.m_fLeft = -std::tan(theta) * 0.75f;
    fr.m_fRight = std::tan(theta) * 0.75f;
    double dTan = std::tan(theta);
    fr.m_fBottom = -dTan * fHeightByWidth * 0.75f;
    dTan = std::tan(theta);
    fr.m_fTop = dTan * fHeightByWidth * 0.75f;
    fr.m_bOrtho = false;
    pcam->SetViewFrustum(fr);
    pcam->SetTranslate(0.0f, 0.0f, 1.0f);
    NiUpdateData UpdateData2(0.0f, false, false);
    pcam->Update(UpdateData2);
    NiPoint3 kUp(0.0f, 1.0f, 0.0f);
    NiPoint3 kTarget(0.0f, 0.0f, 0.0f);
    pcam->LookAtWorldPoint(kTarget, kUp);
    pcam->SetMinNearPlaneDist(1.0f);
    NiFixedString kName("Surgery3DCamera");
    pcam->SetName(kName);
    return pcam;
}

void SPECIALBookMenu::Create(int32_t aiTotalPoints) {
    int32_t imissingIndex;
    Tile* ptile = InterfaceManager::Instance()->GetMenusRoot()->ReadFile("Data\\Menus\\CharGen\\SPECIALBookMenu.xml");
    Menu* pmenu = ptile->GetMenu();
    if (pmenu && pmenu->GetClass() == SPECIALBookMenu::GetClassStatic()) {
        pmenu->SetMenuTile(static_cast<TileMenu*>(ptile), false);
        SPECIALBookMenu::pSPECIALBookMenu = static_cast<SPECIALBookMenu*>(pmenu);
        if (static_cast<LoveTesterMenu*>(pmenu)->LoveTesterMenu::CheckTilesExist(&imissingIndex)) {
            SPECIALBookMenu::pSPECIALBookMenu->iAvailablePoints = aiTotalPoints;
            SPECIALBookMenu::pSPECIALBookMenu->InitBook();
            SPECIALBookMenu::sp3DCamera = SPECIALBookMenu::pSPECIALBookMenu->InitCamera();
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
    else if (pmenu && pmenu->pMenu) {
        delete pmenu;
    }
}

int32_t SPECIALBookMenu::GetClassStatic() {
    return 1060;
}

void SPECIALBookMenu::Draw3DElements() {
    if (SPECIALBookMenu::pSPECIALBookMenu && SPECIALBookMenu::sp3DCamera.m_pObject) {
        NiRenderer::GetRenderer()->ClearBuffer(nullptr, 4u);
        BSCullingProcess* pCuller = Main::GetWorldSceneGraph()->pCuller;
        pCuller->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
        NiCamera* pcam = SPECIALBookMenu::sp3DCamera.m_pObject;
        NiCamera::HandleEyeFinityFOVScale(pcam, 0.15f);
        NiPointer<BSShaderAccumulator> spaccum(InterfaceManager::Instance()->Get3dAccumulator());
        pCuller->SetAccumulator(spaccum.m_pObject);
        BSShaderUtil::AccumulateScene(pcam, BSShaderManager::GetShadowSceneNode(1), *pCuller);
        BSShaderUtil::RenderScene(pcam, spaccum.m_pObject, false);
        pCuller->SetAccumulator(nullptr);
        pCuller->PopCullMode();
    }
}

void SPECIALBookMenu::Create35() {
    SPECIALBookMenu::Create(35);
}

void SPECIALBookMenu::NextPage(SpecialBookButton* apButton) {
    if (pSPECIALBookMenu && pSPECIALBookMenu->bAnimFinished) {
        pSPECIALBookMenu->TurnPage(true);
        pSPECIALBookMenu->CallPageCallbacks();
    }
}

void SPECIALBookMenu::PreviousPage(SpecialBookButton* apButton) {
    if (pSPECIALBookMenu && pSPECIALBookMenu->bAnimFinished) {
        pSPECIALBookMenu->TurnPage(false);
        pSPECIALBookMenu->CallPageCallbacks();
    }
}

// The SPECIAL value fields' refresh callback (the text items' pfRefreshField): the player's permanent value of the
// field's actor value picks the digit texture, set as the field's first text property's texture 0 of type 0.
void SPECIALBookMenu::RefreshActorValue(SpecialBookTextItem* apTextItem) {
    // The player is cast to ActorValueOwner through a reference, without a null check.
    int32_t iValue = static_cast<ActorValueOwner&>(*pPlayer).GetPermanentActorValue(apTextItem->eActorValue);
    NiSourceTexture* pTexture = pSPECIALBookMenu->spAVValues[iValue];
    static_cast<BSShaderPPLightingProperty*>(apTextItem->pText[0])
        ->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pTexture);
}

void SPECIALBookMenu::RefreshTotalPointsText(SPECIALBookMenu::SpecialBookTextItem* apTextItem) {
    int32_t iPointsLeft = SPECIALBookMenu::pSPECIALBookMenu->iAvailablePoints - PlayerCharacter::GetSumOfSPECIALStats();
    if (iPointsLeft <= 1) {
        if (iPointsLeft == 1) {
            apTextItem->pText[0u]->SetTexture(
                BSShaderProperty::TEXTYPE_DIFFUSE,
                0,
                SPECIALBookMenu::pSPECIALBookMenu->spPointsText[2].m_pObject
            );
        }
        else if (!iPointsLeft) {
            apTextItem->pText[0u]->SetTexture(
                BSShaderProperty::TEXTYPE_DIFFUSE,
                0,
                SPECIALBookMenu::pSPECIALBookMenu->spPointsText[3].m_pObject
            );
        }
    }
    else {
        apTextItem->pText[0u]->SetDiffuseTexture(0, SPECIALBookMenu::pSPECIALBookMenu->spPointsText[1].m_pObject);
    }
}

void SPECIALBookMenu::RefreshTotalPointsValue(SPECIALBookMenu::SpecialBookTextItem* apTextItem) {
    if (apTextItem->pText[0u] && apTextItem->pText[1]) {
        int32_t iPointsLeft = SPECIALBookMenu::pSPECIALBookMenu->iAvailablePoints -
            PlayerCharacter::GetSumOfSPECIALStats();
        if (iPointsLeft) {
            if (iPointsLeft / 10 <= 0) {
                apTextItem->pTextObj[0u]->SetAppCulled(true);
            }
            else {
                apTextItem->pTextObj[0u]->SetAppCulled(false);
                apTextItem->pText[0u]->SetTexture(
                    BSShaderProperty::TEXTYPE_DIFFUSE,
                    0,
                    SPECIALBookMenu::pSPECIALBookMenu->spAVValues[iPointsLeft / 10].m_pObject
                );
            }
            apTextItem->pTextObj[1]->SetAppCulled(false);
            apTextItem->pText[1]->SetTexture(
                BSShaderProperty::TEXTYPE_DIFFUSE,
                0,
                SPECIALBookMenu::pSPECIALBookMenu->spAVValues[iPointsLeft % 10].m_pObject
            );
        }
        else {
            apTextItem->pTextObj[0u]->SetAppCulled(true);
            apTextItem->pTextObj[1]->SetAppCulled(true);
        }
    }
}

void SPECIALBookMenu::RefreshLeftAVButton(SPECIALBookMenu::ActorValueButton* apButton) {
    ActorValue::Index eActorValue = apButton->eActorValue;
    if (pPlayer->GetPermanentActorValue(eActorValue) <= 1) {
        apButton->spButton->SetAppCulled(true);
    }
    else {
        apButton->spButton->SetAppCulled(false);
    }
    if (InterfaceManager::Has360Controller() &&
        SPECIALBookMenu::pSPECIALBookMenu->eState == SPECIALBookMenu::SBMS_SPECIAL &&
        eActorValue != SPECIALBookMenu::pSPECIALBookMenu->eXBoxIndexState + 4) {
        apButton->spButton->SetAppCulled(true);
    }
}

void SPECIALBookMenu::RefreshRightAVButton(SPECIALBookMenu::ActorValueButton* apButton) {
    ActorValue::Index eActorValue = apButton->eActorValue;
    if (pPlayer->GetPermanentActorValue(eActorValue) >= 10 ||
        (SPECIALBookMenu::pSPECIALBookMenu &&
         PlayerCharacter::GetSumOfSPECIALStats() >= SPECIALBookMenu::pSPECIALBookMenu->iAvailablePoints)) {
        apButton->spButton->SetAppCulled(true);
    }
    else {
        apButton->spButton->SetAppCulled(false);
    }
    if (InterfaceManager::Has360Controller() &&
        SPECIALBookMenu::pSPECIALBookMenu->eState == SPECIALBookMenu::SBMS_SPECIAL &&
        eActorValue != SPECIALBookMenu::pSPECIALBookMenu->eXBoxIndexState + 4) {
        apButton->spButton->SetAppCulled(true);
    }
}

// Called through SpecialBookButton::pfRefresh, whose callers ignore any return value.
void SPECIALBookMenu::RefreshNextPageButton(SPECIALBookMenu::SpecialBookButton* apButton) {
    apButton->pImage->SetTexture(
        BSShaderProperty::TEXTYPE_DIFFUSE,
        0,
        SPECIALBookMenu::pSPECIALBookMenu->spNextPageButtonImage[1].m_pObject
    );
}

// Called through SpecialBookButton::pfRefresh, whose callers ignore any return value.
void SPECIALBookMenu::RefreshPreviousPageButton(SPECIALBookMenu::SpecialBookButton* apButton) {
    apButton->pImage->SetTexture(
        BSShaderProperty::TEXTYPE_DIFFUSE,
        0,
        SPECIALBookMenu::pSPECIALBookMenu->spPreviousPageButtonImage[0].m_pObject
    );
}

// Called through SpecialBookButton::pfRefresh, whose callers ignore any return value.
void SPECIALBookMenu::RefreshAllDoneButton(SPECIALBookMenu::SpecialBookButton* apButton) {
    if (PlayerCharacter::GetSumOfSPECIALStats() >= SPECIALBookMenu::pSPECIALBookMenu->iAvailablePoints) {
        apButton->pImage->SetTexture(
            BSShaderProperty::TEXTYPE_DIFFUSE,
            0,
            SPECIALBookMenu::pSPECIALBookMenu->spOpenCloseBookButtonImage[0].m_pObject
        );
    }
    else {
        apButton->pImage->SetTexture(
            BSShaderProperty::TEXTYPE_DIFFUSE,
            0,
            SPECIALBookMenu::pSPECIALBookMenu->spOpenCloseBookButtonImage[1].m_pObject
        );
    }
}

void SPECIALBookMenu::IncreaseActorValue(SPECIALBookMenu::ActorValueButton* apButton) {
    if (SPECIALBookMenu::pSPECIALBookMenu) {
        ActorValue::Index eActorValue = apButton->eActorValue;
        if (eActorValue != ActorValue::VALUE_NONE) {
            int32_t iValue = pPlayer->GetPermanentActorValue(eActorValue);
            if (iValue < 10) {
                BSSoundHandle th_s;
                th_s = BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialNumber", 289u);
                th_s.Play(false);
            }
            if (PlayerCharacter::GetSumOfSPECIALStats() < SPECIALBookMenu::pSPECIALBookMenu->iAvailablePoints) {
                pPlayer->SetActorBaseValue(eActorValue, BSMIN(iValue + 1, 10));
            }
        }
        SPECIALBookMenu::pSPECIALBookMenu->CallPageCallbacks();
    }
}

void SPECIALBookMenu::DecreaseActorValue(SPECIALBookMenu::ActorValueButton* apButton) {
    if (SPECIALBookMenu::pSPECIALBookMenu) {
        ActorValue::Index eActorValue = apButton->eActorValue;
        if (eActorValue != ActorValue::VALUE_NONE) {
            int32_t iValue = pPlayer->GetPermanentActorValue(eActorValue);
            if (iValue > 1) {
                BSSoundHandle th_s;
                th_s = BSAudio::QInstance().GetSoundHandleByName("OBJBookSpecialNumber", 289u);
                th_s.Play(false);
            }
            pPlayer->SetActorBaseValue(eActorValue, BSMAX(iValue - 1, 1));
        }
        SPECIALBookMenu::pSPECIALBookMenu->CallPageCallbacks();
    }
}
