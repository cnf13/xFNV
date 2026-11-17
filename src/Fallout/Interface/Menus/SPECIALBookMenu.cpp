#include "Fallout/Interface/Menus/SPECIALBookMenu.h"

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
#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <cmath>
#include <cstdarg>
#include <stdint.h>

NiPointer<NiCamera> SPECIALBookMenu::sp3DCamera;

void SPECIALBookDoneCallback() {
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	if (SPECIALBookMenu::pSPECIALBookMenu &&
		PlayerCharacter::GetSumOfSPECIALStats() == SPECIALBookMenu::pSPECIALBookMenu->iAvailablePoints) {
		SPECIALBookMenu::pSPECIALBookMenu->Exit();
	} else {
		BSAudio* v0 = &BSAudio::QInstance();
		BSSoundHandle* SoundHandleByName = v0->GetSoundHandleByName(&a1, "UIActivateNothing", 289u);
		SoundHandleByName->Play(0);
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
	if (pProperty && static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() >= 8 &&
		static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() <= 12) {
		pImage = static_cast<BSShaderPPLightingProperty*>(pProperty);
	} else {
		pImage = nullptr;
	}
}

SPECIALBookMenu::SpecialBookTextItem::
	SpecialBookTextItem(void (*apRefreshFieldCallback)(SPECIALBookMenu::SpecialBookTextItem*),
		const char* apTextObjectName1, const char* apTextObjectName2, ActorValue::Index aeActorValue)
	: pfRefreshField(apRefreshFieldCallback), eActorValue(aeActorValue) {
	// The second object and its property are set only when a second name is given, as the game does.
	const char* apNames[2] = {apTextObjectName1, apTextObjectName2};
	for (int32_t i = 0; i < 2; ++i) {
		if (i == 1 && !apTextObjectName2) {
			break;
		}
		pTextObj[i] = SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->GetObjectByName(NiFixedString(apNames[i]));
		// Shader property types 8 to 12 are the per-pixel lighting properties.
		NiProperty* pProperty = pTextObj[i]->GetProperty(3);
		if (pProperty && static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() >= 8 &&
			static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() <= 12) {
			pText[i] = static_cast<BSShaderPPLightingProperty*>(pProperty);
		} else {
			pText[i] = nullptr;
		}
	}
}

SPECIALBookMenu::ActorValueButton::ActorValueButton(void* apButtonClick, void* apRefresh, const char* apButtonName,
	int32_t aeActorValue, int32_t aiNumAffectedFields, ...)
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
	if (((uint32_t)aiID) < 8) {
		pTiles[aiID] = apTile;
	}
}

void SPECIALBookMenu::DoClick(int32_t a1, Tile* a2) {
	FNVD_INLINED_LOCAL(BSSoundHandle, v25, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, apResult, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, a12, std::nullopt, std::nullopt);
	bool bPageTurned = 0;
	switch (a1) {
	case 0:
	case 1: {
		if (bAnimFinished) {
			if (a1) {
				bPageTurned = TurnPage(0);
			} else {
				bPageTurned = TurnPage(1);
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
				ActorValue::Index eav_3 = static_cast<ActorValue::Index>(eXBoxIndexState + 4);
				ioldValue = pPlayer->GetPermanentActorValue(eav_3);
				if (a1 == 2) {
					if (PlayerCharacter::GetSumOfSPECIALStats() < iAvailablePoints) {
						int32_t v9 = BSMIN(ioldValue + 1, 10);
						pPlayer->SetActorBaseValue(eav_3, v9);
					}
				} else {
					int32_t v10 = BSMAX(ioldValue - 1, 1);
					pPlayer->SetActorBaseValue(eav_3, v10);
				}
				inewValue = pPlayer->GetBaseActorValue(eav_3);
			}
		} else {
			ActorValue::Index SelectedSPECIAL = GetSelectedSPECIAL();
			ioldValue = pPlayer->GetPermanentActorValue(SelectedSPECIAL);
			if (a1 == 2) {
				if (PlayerCharacter::GetSumOfSPECIALStats() < iAvailablePoints) {
					int32_t v15 = BSMIN(ioldValue + 1, 10);
					ActorValue::Index eav = GetSelectedSPECIAL();
					pPlayer->SetActorBaseValue(eav, v15);
				}
			} else {
				int32_t v16 = BSMAX(ioldValue - 1, 1);
				ActorValue::Index eav_2 = GetSelectedSPECIAL();
				pPlayer->SetActorBaseValue(eav_2, v16);
			}
			ActorValue::Index v8 = GetSelectedSPECIAL();
			inewValue = pPlayer->GetBaseActorValue(v8);
		}
		if (ioldValue != inewValue) {
			BSSoundHandle psoundHand;
			BSAudio* v11 = &BSAudio::QInstance();
			BSSoundHandle* SoundHandleByName = v11->GetSoundHandleByName(&arHandle, "OBJBookSpecialNumber", 289u);
			psoundHand.operator=(*SoundHandleByName);
			psoundHand.Play(0);
		}
		CallPageCallbacks();
		break;
	}
	case 4: {
		if (PlayerCharacter::GetSumOfSPECIALStats() == iAvailablePoints) {
			Exit();
		} else {
			BSAudio* v4 = &BSAudio::QInstance();
			BSSoundHandle* v23 = v4->GetSoundHandleByName(&a12, "UIActivateNothing", 289u);
			v23->Play(0);
		}
		break;
	}
	case 5: {
		if (eState < SPECIALBookMenu::SBMS_CLOSED) {
			InterfaceManager* pim = InterfaceManager::Instance();
			int32_t cx = static_cast<int32_t>(pim->sRealCursorPos.x);
			int32_t cy = static_cast<int32_t>(pim->sRealCursorPos.z);
			if (BSShaderManager::bLetterbox) {
				NiRenderer* Renderer_ = NiRenderer::GetRenderer();
				double m_bottom = BSShaderManager::kFramebufferViewport.m_bottom;
				cy =
					static_cast<int32_t>(static_cast<double>(Renderer_->GetDefaultBackBuffer()->m_uiHeight) * m_bottom +
						(BSShaderManager::kFramebufferViewport.m_top - BSShaderManager::kFramebufferViewport.m_bottom) *
							static_cast<double>(cy));
			}
			for (int32_t iindex = 0;; ++iindex) {
				NiPoint3 dir;
				NiPoint3 origin;
				uint32_t FormFlags = Pages[eState].Buttons.iSize;
				if (iindex >= FormFlags) {
					break;
				}
				NiPick activatePick(0, 8u);
				activatePick.SetObserveAppCullFlag(1);
				activatePick.SetTarget(Pages[eState].Buttons.operator[](iindex)->spButton.m_pObject);
				activatePick.m_eSortType = NiPick::SORT;
				activatePick.SetIsReturnTexture(1);
				SPECIALBookMenu::sp3DCamera.m_pObject->WindowPointToRay(cx, cy, origin, dir, 0);
				if (activatePick.PickObjects(origin, dir, 0)) {
					uint32_t uiIndex = 0;
					if ((&activatePick.m_pickResults)->GetAt(0)) {
						// The button is fetched once for the argument and again for the callback, as the game does.
						SpecialBookButton* pButton = Pages[eState].Buttons.operator[](iindex);
						Pages[eState].Buttons.operator[](iindex)->pfButtonClick(pButton);
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
			BSAudio* v13 = &BSAudio::QInstance();
			BSSoundHandle* v19 = v13->GetSoundHandleByName(&apResult, "OBJBookSpecialFocus", 289u);
			v19->Play(0);
		}
		CallPageCallbacks();
		break;
	}
	case 7: {
		if (eState == SPECIALBookMenu::SBMS_SPECIAL && eXBoxIndexState < SPECIALBookMenu::SBMS_LUCK) {
			eXBoxIndexState = static_cast<SPECIALBookMenu::SPECIAL_BOOK_MENU_STATE>(eXBoxIndexState + 1);
			BSAudio* v14 = &BSAudio::QInstance();
			BSSoundHandle* v18 = v14->GetSoundHandleByName(&v25, "OBJBookSpecialFocus", 289u);
			v18->Play(0);
		}
		CallPageCallbacks();
		break;
	}
	default: {
		return;
	}
	}
}

void SPECIALBookMenu::DoIdle() {
	if (Interface::IsTopMenuID(Interface::eSPECIALBookMenu)) {
		Controls* Controls_ = appMain->GetControls();
		int32_t key = 30;
		const char* v1 = sLanguage.String();
		if (!BSstrcmp(v1, "FRENCH")) {
			key = 16;
		}
		if (Controls_->GetKeyboardButton(key, 1)) {
			DoClick(4, 0);
		} else {
			NiTimeController* pspecialBookGeomController = pBookGeometry->GetControllers();
			if (pspecialBookGeomController) {
				NiControllerManager* pspecialBookGeomControllerManager =
					static_cast<NiControllerManager*>(pspecialBookGeomController);
				NiControllerSequence* pseq = 0;
				if (!bAnimForward || eState > SPECIALBookMenu::SBMS_START) {
					char* pcString;
					if (bAnimForward) {
						pcString = aForwardAnimations[eState];
					} else {
						pcString = ReverseAnimations[eState];
					}
					NiFixedString apString(static_cast<const char*>(pcString));
					NiFixedString* kName = &apString;
					pseq = pspecialBookGeomControllerManager->GetSequenceByName(*kName);
				}
				fFrameAnimTimer = appTimer.fDelta + fFrameAnimTimer;
				if (pseq) {
					pseq->Update(fFrameAnimTimer, 1);
					double v2 = pseq->m_fLastTime;
					bAnimFinished = !(pseq->m_fEndKeyTime > v2) || eState <= SPECIALBookMenu::SBMS_START;
				} else {
					bAnimFinished = 1;
				}
				while (pspecialBookGeomController) {
					if (NiObject::DynamicCastStatic(&NiMultiTargetTransformController::ms_RTTI,
							pspecialBookGeomController)) {
						NiUpdateData UpdateData(fFrameAnimTimer, 0, 0);
						pspecialBookGeomController->Update(UpdateData);
						break;
					}
					pspecialBookGeomController = pspecialBookGeomController->GetNext();
				}
			}
			NiUpdateData update(0.0f, 0, 0);
			pBookGeometry->Update(update);
		}
	}
}

bool SPECIALBookMenu::DoKey(int32_t a2) {
	int8_t v4 = 0;
	if ((a2 == ((-2147483647))) || (a2 == ((-2147483646)))) {
		DoClick((int32_t)(a2 == ((-2147483647))), nullptr);
		v4 = 1;
	}
	if ((a2 == ((-2147483645))) || (a2 == ((-2147483644)))) {
		DoGamepad(((uint8_t)(a2 != ((-2147483645)))) + 3, (float)1.0);
		return (bool)1;
	}
	return (bool)v4;
}

int32_t SPECIALBookMenu::GetClass() {
	return 1060;
}

bool SPECIALBookMenu::DoGamepad(int32_t a1, float a2) {
	bool result;
	if (!Interface::IsTopMenuID(Interface::eSPECIALBookMenu)) {
		return 0;
	}
	ActorValue::Index eav = static_cast<ActorValue::Index>(GetSelectedSPECIAL());
	switch (a1) {
	case 3:
	case 9: {
		if (eav != ActorValue::VALUE_NONE && PlayerCharacter::GetSumOfSPECIALStats() < iAvailablePoints &&
			pPlayer->GetPermanentActorValue(eav) < 10) {
			DoClick(2, 0);
		}
		result = 1;
		break;
	}
	case 4:
	case 10: {
		if (eav != ActorValue::VALUE_NONE && pPlayer->GetPermanentActorValue(eav) > 1) {
			DoClick(3, 0);
		}
		result = 1;
		break;
	}
	default: {
		return 0;
	}
	}
	return result;
}

// The menu's close code.
void SPECIALBookMenu::Exit() {
	int32_t ClassStatic = SPECIALBookMenu::GetClassStatic();
	Tile* MenuByClass = Menu::GetMenuByClass(ClassStatic);
	if (MenuByClass) {
		pModelLoader->ReleaseFile("Meshes\\Terminals\\Babybook02.NIF");
		ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
		ShadowSceneNode_->RemoveAllChildren();
		ShadowSceneNode* v3 = BSShaderManager::GetShadowSceneNode(1);
		v3->RemoveAllLights();
		SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry = 0;
		SPECIALBookMenu::sp3DCamera.operator=(static_cast<NiCamera*>(0));
		for (int32_t i = 0; i < 9; ++i) {
			SPECIALBookMenu::pSPECIALBookMenu->Pages[i].Buttons.Clear(1);
			SPECIALBookMenu::pSPECIALBookMenu->Pages[i].TextItems.Clear(1);
		}
		if (SPECIALBookMenu::pSPECIALBookMenu) {
			MenuByClass->Set(Tile::kTileValue_DeleteOnFade, 1);
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
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	bool brunAnim = 0;
	if (bForward) {
		bAnimForward = 1;
		if (eState >= SPECIALBookMenu::SBMS_CLOSED) {
			return 0;
		}
		eState = static_cast<SPECIALBookMenu::SPECIAL_BOOK_MENU_STATE>(eState + 1);
		brunAnim = 1;
		BSSoundHandle psoundHand;
		BSAudio* v2 = &BSAudio::QInstance();
		BSSoundHandle* SoundHandleByName = v2->GetSoundHandleByName(&a1, "OBJBookSpecialPageTurn", 289u);
		psoundHand.operator=(*SoundHandleByName);
		psoundHand.Play(0);
	} else {
		bAnimForward = 0;
		if (eState < SPECIALBookMenu::SBMS_STRENGTH) {
			return 0;
		}
		eState = static_cast<SPECIALBookMenu::SPECIAL_BOOK_MENU_STATE>(eState - 1);
		brunAnim = 1;
		BSSoundHandle psoundHand_2;
		BSAudio* v4 = &BSAudio::QInstance();
		BSSoundHandle* v7 = v4->GetSoundHandleByName(&arHandle, "OBJBookSpecialPageTurn", 289u);
		psoundHand_2.operator=(*v7);
		psoundHand_2.Play(0);
	}
	if (brunAnim) {
		NiTimeController* pSPECIALBookGeomController =
			SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->GetController(&NiControllerManager::ms_RTTI);
		if (pSPECIALBookGeomController) {
			NiControllerManager* pSPECIALBookGeomControllerManager =
				static_cast<NiControllerManager*>(pSPECIALBookGeomController);
			NiControllerSequence* pseq = 0;
			if (!bAnimForward || eState > SPECIALBookMenu::SBMS_START) {
				char* pcString;
				if (bAnimForward) {
					pcString = aForwardAnimations[eState];
				} else {
					pcString = ReverseAnimations[eState];
				}
				NiFixedString apString(pcString);
				NiFixedString* a2 = &apString;
				pseq = pSPECIALBookGeomControllerManager->GetSequenceByName(*a2);
			}
			if (pseq) {
				pSPECIALBookGeomControllerManager->DeactivateAll(0.0f);
				pSPECIALBookGeomControllerManager->ActivateSequence(pseq, 0, 1, 1.0f, 0.0f, 0);
				pSPECIALBookGeomControllerManager->SetActive(1);
				pseq->ResetSequence();
			}
		}
	}
	return 1;
}

void SPECIALBookMenu::CallPageCallbacks() {
	if (eState < SPECIALBookMenu::SBMS_CLOSED) {
		for (uint32_t auiIndex = 0; auiIndex < Pages[eState].TextItems.iSize; ++auiIndex) {
			// Fetches the item for the argument first, then again for the callback, as the game does.
			SpecialBookTextItem* pItem = Pages[eState].TextItems.operator[](auiIndex);
			Pages[eState].TextItems.operator[](auiIndex)->pfRefreshField(pItem);
		}
		for (uint32_t i = 0; i < Pages[eState].Buttons.iSize; ++i) {
			SpecialBookButton* pButton = Pages[eState].Buttons.operator[](i);
			Pages[eState].Buttons.operator[](i)->pfRefresh(pButton);
		}
	}
}

// Loads the book's model, starts its "Forward" sequence and lights it, then builds the nine pages: the cover's
// look-inside button, a page per SPECIAL (its value, message and points-remaining items, the decrease and increase
// buttons on them, the page arrows) and the index page, and runs every page's refresh callbacks once.
void SPECIALBookMenu::InitBook() {
	FNVD_INLINED_LOCAL(NiFixedString, kForward, std::nullopt, std::nullopt);
	SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry =
		(NiAVObject*)pModelLoader->LoadFile("Meshes\\Terminals\\Babybook02.NIF", LOD_MULT_NONE, 1, 0, 0, 0);
	SwapPlatformLanguageTextures(SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry);
	SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->SetScale(0.05f);
	SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->SetTranslate(0.0f, 0.0f, -60.0f);
	SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->SetRotate(1.5707964f, 1.0f, 0.0f, 0.0f);
	NiUpdateData arData(0.0f, 0, 0);
	SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->Update(arData);
	SPECIALBookMenu::pSPECIALBookMenu->fFrameAnimTimer =
		appTimer.fDelta + SPECIALBookMenu::pSPECIALBookMenu->fFrameAnimTimer;
	NiTimeController* pController =
		SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->GetController(&NiControllerManager::ms_RTTI);
	if (pController) {
		NiControllerManager* pManager = static_cast<NiControllerManager*>(pController);
		NiFixedString* pName = ::new (static_cast<void*>(&kForward)) NiFixedString("Forward");
		NiControllerSequence* pkSequence = pManager->GetSequenceByName(*pName);
		kForward.~NiFixedString();
		if (pkSequence) {
			pManager->DeactivateAll(0.0f);
			pManager->ActivateSequence(pkSequence, 0, 1, 1.0f, 0.0f, 0);
			pManager->SetActive(1);
			pkSequence->ResetSequence();
		}
	}
	NiTimeController::StartAnimations(pBookGeometry);
	while (pController) {
		if (NiObject::DynamicCastStatic(&NiMultiTargetTransformController::ms_RTTI, pController)) {
			NiUpdateData kControllerData(0.0f, 0, 0);
			pController->Update(kControllerData);
			break;
		}
		pController = pController->GetNext();
	}
	NiUpdateData arUpdateData(0.0f, 0, 0);
	pBookGeometry->Update(arUpdateData);
	const NiBound* pWorldBound = &SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->GetWorldBound();
	float fLightRadius = pWorldBound->m_fRadius * 20.0;
	NiNode* pBookNode = SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry->IsNode();
	ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(1);
	if (!pShadowSceneNode->AddAllLightsFromNode_Recurse(pBookNode, fLightRadius)) {
		NiPointLight* pLight;
		NiPointLight* pLightMemory = static_cast<NiPointLight*>(NiMemObject::operator new(sizeof(NiPointLight)));
		if (pLightMemory) {
			pLight = ::new (pLightMemory) NiPointLight;
		} else {
			pLight = 0;
		}
		NiPoint3 kTranslate;
		pLight->SetTranslate(*::new (&kTranslate) NiPoint3(0.0f, 0.0f, 0.0f));
		NiColor kDiffuse;
		pLight->SetDiffuseColor(*FNVD_RECONSTRUCT(NiColor)(&kDiffuse, 0.85f, 0.85f, 0.85f));
		// The specular colour's red channel is given the light radius, deliberately, as the game does.
		NiColor kSpecular;
		pLight->SetSpecularColor(*FNVD_RECONSTRUCT(NiColor)(&kSpecular, fLightRadius, 0.0f, 0.0f));
		pLight->SetDimmer(1.0f);
		NiUpdateData kLightData(0.0f, 0, 0);
		pLight->Update(kLightData);
		BSShaderManager::GetShadowSceneNode(1)->AttachChild(pLight, 1);
		BSShaderManager::GetShadowSceneNode(1)->AddLight(pLight, 0);
	}
	BSShaderManager::GetShadowSceneNode(1)->AttachChild(SPECIALBookMenu::pSPECIALBookMenu->pBookGeometry, 1);

	// The cover: the look-inside button.
	void* pMemory = ourMalloc(sizeof(SpecialBookButton));
	SpecialBookButton* pNext = pMemory
		? ::new (pMemory) SpecialBookButton(reinterpret_cast<void*>(&SPECIALBookMenu::NextPage),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshNextPageButton), "LookInside_Btn:0")
		: nullptr;
	Pages[0].Buttons.Add(pNext);

	// Page 1, Strength.
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	SpecialBookTextItem* pPointVal = pMemory
		? ::new (pMemory)
			  SpecialBookTextItem(&SPECIALBookMenu::RefreshActorValue, "P1_PointVal:0", nullptr, ActorValue::STRENGTH)
		: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	SpecialBookTextItem* pMessage = pMemory
		? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshTotalPointsText, "P1_Message:0", nullptr,
			  ActorValue::VALUE_NONE)
		: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	SpecialBookTextItem* pPointsRemain = pMemory
		? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshTotalPointsValue, "P1_PointsRemainDigit1:0",
			  "P1_PointsRemainDigit2:0", ActorValue::VALUE_NONE)
		: nullptr;
	pMemory = ourMalloc(sizeof(ActorValueButton));
	ActorValueButton* pDecrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton), "P1_Decrease_Btn:0", ActorValue::STRENGTH,
			  2, pPointVal, pPointsRemain)
		: nullptr;
	pMemory = ourMalloc(sizeof(ActorValueButton));
	ActorValueButton* pIncrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton), "P1_Increase_Btn:0",
			  ActorValue::STRENGTH, 2, pPointVal, pPointsRemain)
		: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookButton));
	pNext = pMemory ? ::new (pMemory) SpecialBookButton(reinterpret_cast<void*>(&SPECIALBookMenu::NextPage),
						  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshNextPageButton), "P1_RT_Btn:0")
					: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookButton));
	SpecialBookButton* pPrevious = pMemory
		? ::new (pMemory) SpecialBookButton(reinterpret_cast<void*>(&SPECIALBookMenu::PreviousPage),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshPreviousPageButton), "P1_LT_Btn:0")
		: nullptr;
	Pages[1].TextItems.Add(pPointVal);
	Pages[1].TextItems.Add(pMessage);
	Pages[1].TextItems.Add(pPointsRemain);
	Pages[1].Buttons.Add(pDecrease);
	Pages[1].Buttons.Add(pIncrease);
	Pages[1].Buttons.Add(pNext);
	Pages[1].Buttons.Add(pPrevious);

	// Page 2, Perception.
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointVal = pMemory
		? ::new (pMemory)
			  SpecialBookTextItem(&SPECIALBookMenu::RefreshActorValue, "P2_PointVal:0", nullptr, ActorValue::PERCEPTION)
		: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pMessage = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshTotalPointsText, "P2_Message:0",
							 nullptr, ActorValue::VALUE_NONE)
					   : nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointsRemain = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshTotalPointsValue,
								  "P2_PointsRemainDigit1:0", "P2_PointsRemainDigit2:0", ActorValue::VALUE_NONE)
							: nullptr;
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pDecrease = pMemory ? ::new (pMemory)
							  ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
								  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton), "P2_Decrease_Btn:0",
								  ActorValue::PERCEPTION, 2, pPointVal, pPointsRemain)
						: nullptr;
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pIncrease = pMemory ? ::new (pMemory)
							  ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
								  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton), "P2_Increase_Btn:0",
								  ActorValue::PERCEPTION, 2, pPointVal, pPointsRemain)
						: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookButton));
	pNext = pMemory ? ::new (pMemory) SpecialBookButton(reinterpret_cast<void*>(&SPECIALBookMenu::NextPage),
						  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshNextPageButton), "P2_RT_Btn:0")
					: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookButton));
	pPrevious = pMemory ? ::new (pMemory) SpecialBookButton(reinterpret_cast<void*>(&SPECIALBookMenu::PreviousPage),
							  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshPreviousPageButton), "P2_LT_Btn:0")
						: nullptr;
	Pages[2].TextItems.Add(pPointVal);
	Pages[2].TextItems.Add(pMessage);
	Pages[2].TextItems.Add(pPointsRemain);
	Pages[2].Buttons.Add(pDecrease);
	Pages[2].Buttons.Add(pIncrease);
	Pages[2].Buttons.Add(pNext);
	Pages[2].Buttons.Add(pPrevious);

	// Page 3, Endurance.
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointVal = pMemory
		? ::new (pMemory)
			  SpecialBookTextItem(&SPECIALBookMenu::RefreshActorValue, "P3_PointVal:0", nullptr, ActorValue::ENDURANCE)
		: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pMessage = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshTotalPointsText, "P3_Message:0",
							 nullptr, ActorValue::VALUE_NONE)
					   : nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointsRemain = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshTotalPointsValue,
								  "P3_PointsRemainDigit1:0", "P3_PointsRemainDigit2:0", ActorValue::VALUE_NONE)
							: nullptr;
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pDecrease = pMemory ? ::new (pMemory)
							  ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
								  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton), "P3_Decrease_Btn:0",
								  ActorValue::ENDURANCE, 2, pPointVal, pPointsRemain)
						: nullptr;
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pIncrease = pMemory ? ::new (pMemory)
							  ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
								  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton), "P3_Increase_Btn:0",
								  ActorValue::ENDURANCE, 2, pPointVal, pPointsRemain)
						: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookButton));
	pNext = pMemory ? ::new (pMemory) SpecialBookButton(reinterpret_cast<void*>(&SPECIALBookMenu::NextPage),
						  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshNextPageButton), "P3_RT_Btn:0")
					: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookButton));
	pPrevious = pMemory ? ::new (pMemory) SpecialBookButton(reinterpret_cast<void*>(&SPECIALBookMenu::PreviousPage),
							  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshPreviousPageButton), "P3_LT_Btn:0")
						: nullptr;
	Pages[3].TextItems.Add(pPointVal);
	Pages[3].TextItems.Add(pMessage);
	Pages[3].TextItems.Add(pPointsRemain);
	Pages[3].Buttons.Add(pDecrease);
	Pages[3].Buttons.Add(pIncrease);
	Pages[3].Buttons.Add(pNext);
	Pages[3].Buttons.Add(pPrevious);

	// Page 4, Charisma.
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointVal = pMemory
		? ::new (pMemory)
			  SpecialBookTextItem(&SPECIALBookMenu::RefreshActorValue, "P4_PointVal:0", nullptr, ActorValue::CHARISMA)
		: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pMessage = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshTotalPointsText, "P4_Message:0",
							 nullptr, ActorValue::VALUE_NONE)
					   : nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointsRemain = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshTotalPointsValue,
								  "P4_PointsRemainDigit1:0", "P4_PointsRemainDigit2:0", ActorValue::VALUE_NONE)
							: nullptr;
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pDecrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton), "P4_Decrease_Btn:0", ActorValue::CHARISMA,
			  2, pPointVal, pPointsRemain)
		: nullptr;
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pIncrease = pMemory ? ::new (pMemory)
							  ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
								  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton), "P4_Increase_Btn:0",
								  ActorValue::CHARISMA, 2, pPointVal, pPointsRemain)
						: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookButton));
	pNext = pMemory ? ::new (pMemory) SpecialBookButton(reinterpret_cast<void*>(&SPECIALBookMenu::NextPage),
						  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshNextPageButton), "P4_RT_Btn:0")
					: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookButton));
	pPrevious = pMemory ? ::new (pMemory) SpecialBookButton(reinterpret_cast<void*>(&SPECIALBookMenu::PreviousPage),
							  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshPreviousPageButton), "P4_LT_Btn:0")
						: nullptr;
	Pages[4].TextItems.Add(pPointVal);
	Pages[4].TextItems.Add(pMessage);
	Pages[4].TextItems.Add(pPointsRemain);
	Pages[4].Buttons.Add(pDecrease);
	Pages[4].Buttons.Add(pIncrease);
	Pages[4].Buttons.Add(pNext);
	Pages[4].Buttons.Add(pPrevious);

	// Page 5, Intelligence.
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointVal = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshActorValue, "P5_PointVal:0",
							  nullptr, ActorValue::INTELLIGENCE)
						: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pMessage = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshTotalPointsText, "P5_Message:0",
							 nullptr, ActorValue::VALUE_NONE)
					   : nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointsRemain = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshTotalPointsValue,
								  "P5_PointsRemainDigit1:0", "P5_PointsRemainDigit2:0", ActorValue::VALUE_NONE)
							: nullptr;
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pDecrease = pMemory ? ::new (pMemory)
							  ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
								  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton), "P5_Decrease_Btn:0",
								  ActorValue::INTELLIGENCE, 2, pPointVal, pPointsRemain)
						: nullptr;
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pIncrease = pMemory ? ::new (pMemory)
							  ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
								  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton), "P5_Increase_Btn:0",
								  ActorValue::INTELLIGENCE, 2, pPointVal, pPointsRemain)
						: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookButton));
	pNext = pMemory ? ::new (pMemory) SpecialBookButton(reinterpret_cast<void*>(&SPECIALBookMenu::NextPage),
						  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshNextPageButton), "P5_RT_Btn:0")
					: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookButton));
	pPrevious = pMemory ? ::new (pMemory) SpecialBookButton(reinterpret_cast<void*>(&SPECIALBookMenu::PreviousPage),
							  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshPreviousPageButton), "P5_LT_Btn:0")
						: nullptr;
	Pages[5].TextItems.Add(pPointVal);
	Pages[5].TextItems.Add(pMessage);
	Pages[5].TextItems.Add(pPointsRemain);
	Pages[5].Buttons.Add(pDecrease);
	Pages[5].Buttons.Add(pIncrease);
	Pages[5].Buttons.Add(pNext);
	Pages[5].Buttons.Add(pPrevious);

	// Page 6, Agility.
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointVal = pMemory
		? ::new (pMemory)
			  SpecialBookTextItem(&SPECIALBookMenu::RefreshActorValue, "P6_PointVal:0", nullptr, ActorValue::AGILITY)
		: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pMessage = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshTotalPointsText, "P6_Message:0",
							 nullptr, ActorValue::VALUE_NONE)
					   : nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointsRemain = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshTotalPointsValue,
								  "P6_PointsRemainDigit1:0", "P6_PointsRemainDigit2:0", ActorValue::VALUE_NONE)
							: nullptr;
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pDecrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton), "P6_Decrease_Btn:0", ActorValue::AGILITY,
			  2, pPointVal, pPointsRemain)
		: nullptr;
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pIncrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton), "P6_Increase_Btn:0", ActorValue::AGILITY,
			  2, pPointVal, pPointsRemain)
		: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookButton));
	pNext = pMemory ? ::new (pMemory) SpecialBookButton(reinterpret_cast<void*>(&SPECIALBookMenu::NextPage),
						  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshNextPageButton), "P6_RT_Btn:0")
					: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookButton));
	pPrevious = pMemory ? ::new (pMemory) SpecialBookButton(reinterpret_cast<void*>(&SPECIALBookMenu::PreviousPage),
							  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshPreviousPageButton), "P6_LT_Btn:0")
						: nullptr;
	Pages[6].TextItems.Add(pPointVal);
	Pages[6].TextItems.Add(pMessage);
	Pages[6].TextItems.Add(pPointsRemain);
	Pages[6].Buttons.Add(pDecrease);
	Pages[6].Buttons.Add(pIncrease);
	Pages[6].Buttons.Add(pNext);
	Pages[6].Buttons.Add(pPrevious);

	// Page 7, Luck.
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointVal = pMemory
		? ::new (pMemory)
			  SpecialBookTextItem(&SPECIALBookMenu::RefreshActorValue, "P7_PointVal:0", nullptr, ActorValue::LUCK)
		: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pMessage = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshTotalPointsText, "P7_Message:0",
							 nullptr, ActorValue::VALUE_NONE)
					   : nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointsRemain = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshTotalPointsValue,
								  "P7_PointsRemainDigit1:0", "P7_PointsRemainDigit2:0", ActorValue::VALUE_NONE)
							: nullptr;
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pDecrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton), "P7_Decrease_Btn:0", ActorValue::LUCK, 2,
			  pPointVal, pPointsRemain)
		: nullptr;
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pIncrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton), "P7_Increase_Btn:0", ActorValue::LUCK, 2,
			  pPointVal, pPointsRemain)
		: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookButton));
	pNext = pMemory ? ::new (pMemory) SpecialBookButton(reinterpret_cast<void*>(&SPECIALBookMenu::NextPage),
						  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshNextPageButton), "P7_RT_Btn:0")
					: nullptr;
	pMemory = ourMalloc(sizeof(SpecialBookButton));
	pPrevious = pMemory ? ::new (pMemory) SpecialBookButton(reinterpret_cast<void*>(&SPECIALBookMenu::PreviousPage),
							  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshPreviousPageButton), "P7_LT_Btn:0")
						: nullptr;
	Pages[7].TextItems.Add(pPointVal);
	Pages[7].TextItems.Add(pMessage);
	Pages[7].TextItems.Add(pPointsRemain);
	Pages[7].Buttons.Add(pDecrease);
	Pages[7].Buttons.Add(pIncrease);
	Pages[7].Buttons.Add(pNext);
	Pages[7].Buttons.Add(pPrevious);

	// Page 8, the index: each SPECIAL's value with its decrease and increase buttons. Those buttons are handed page 7's
	// points-remaining item, as the game does; the index's own is built after them.
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointVal = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshActorValue,
							  "Index_StrengthPointVal:0", nullptr, ActorValue::STRENGTH)
						: nullptr;
	Pages[8].TextItems.Add(pPointVal);
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pDecrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton), "Index_StrengthDecrease_Btn:0",
			  ActorValue::STRENGTH, 2, pPointVal, pPointsRemain)
		: nullptr;
	Pages[8].Buttons.Add(pDecrease);
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pIncrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton), "Index_StrengthIncrease_Btn:0",
			  ActorValue::STRENGTH, 2, pPointVal, pPointsRemain)
		: nullptr;
	Pages[8].Buttons.Add(pIncrease);
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointVal = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshActorValue,
							  "Index_PerceptionPointVal:0", nullptr, ActorValue::PERCEPTION)
						: nullptr;
	Pages[8].TextItems.Add(pPointVal);
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pDecrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton), "Index_PerceptionDecrease_Btn:0",
			  ActorValue::PERCEPTION, 2, pPointVal, pPointsRemain)
		: nullptr;
	Pages[8].Buttons.Add(pDecrease);
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pIncrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton), "Index_PerceptionIncrease_Btn:0",
			  ActorValue::PERCEPTION, 2, pPointVal, pPointsRemain)
		: nullptr;
	Pages[8].Buttons.Add(pIncrease);
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointVal = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshActorValue,
							  "Index_EndurancePointVal:0", nullptr, ActorValue::ENDURANCE)
						: nullptr;
	Pages[8].TextItems.Add(pPointVal);
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pDecrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton), "Index_EnduranceDecrease_Btn:0",
			  ActorValue::ENDURANCE, 2, pPointVal, pPointsRemain)
		: nullptr;
	Pages[8].Buttons.Add(pDecrease);
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pIncrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton), "Index_EnduranceIncrease_Btn:0",
			  ActorValue::ENDURANCE, 2, pPointVal, pPointsRemain)
		: nullptr;
	Pages[8].Buttons.Add(pIncrease);
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointVal = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshActorValue,
							  "Index_CharismaPointVal:0", nullptr, ActorValue::CHARISMA)
						: nullptr;
	Pages[8].TextItems.Add(pPointVal);
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pDecrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton), "Index_CharismaDecrease_Btn:0",
			  ActorValue::CHARISMA, 2, pPointVal, pPointsRemain)
		: nullptr;
	Pages[8].Buttons.Add(pDecrease);
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pIncrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton), "Index_CharismaIncrease_Btn:0",
			  ActorValue::CHARISMA, 2, pPointVal, pPointsRemain)
		: nullptr;
	Pages[8].Buttons.Add(pIncrease);
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointVal = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshActorValue,
							  "Index_IntelligencePointVal:0", nullptr, ActorValue::INTELLIGENCE)
						: nullptr;
	Pages[8].TextItems.Add(pPointVal);
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pDecrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton), "Index_IntelligenceDecrease_Btn:0",
			  ActorValue::INTELLIGENCE, 2, pPointVal, pPointsRemain)
		: nullptr;
	Pages[8].Buttons.Add(pDecrease);
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pIncrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton), "Index_IntelligenceIncrease_Btn:0",
			  ActorValue::INTELLIGENCE, 2, pPointVal, pPointsRemain)
		: nullptr;
	Pages[8].Buttons.Add(pIncrease);
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointVal = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshActorValue,
							  "Index_AgilityPointVal:0", nullptr, ActorValue::AGILITY)
						: nullptr;
	Pages[8].TextItems.Add(pPointVal);
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pDecrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton), "Index_AgilityDecrease_Btn:0",
			  ActorValue::AGILITY, 2, pPointVal, pPointsRemain)
		: nullptr;
	Pages[8].Buttons.Add(pDecrease);
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pIncrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton), "Index_AgilityIncrease_Btn:0",
			  ActorValue::AGILITY, 2, pPointVal, pPointsRemain)
		: nullptr;
	Pages[8].Buttons.Add(pIncrease);
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointVal = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshActorValue,
							  "Index_LuckPointVal:0", nullptr, ActorValue::LUCK)
						: nullptr;
	Pages[8].TextItems.Add(pPointVal);
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pDecrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::DecreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshLeftAVButton), "Index_LuckDecrease_Btn:0",
			  ActorValue::LUCK, 2, pPointVal, pPointsRemain)
		: nullptr;
	Pages[8].Buttons.Add(pDecrease);
	pMemory = ourMalloc(sizeof(ActorValueButton));
	pIncrease = pMemory
		? ::new (pMemory) ActorValueButton(reinterpret_cast<void*>(&SPECIALBookMenu::IncreaseActorValue),
			  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshRightAVButton), "Index_LuckIncrease_Btn:0",
			  ActorValue::LUCK, 2, pPointVal, pPointsRemain)
		: nullptr;
	Pages[8].Buttons.Add(pIncrease);
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pMessage = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshTotalPointsText,
							 "Index_Message:0", nullptr, ActorValue::VALUE_NONE)
					   : nullptr;
	Pages[8].TextItems.Add(pMessage);
	pMemory = ourMalloc(sizeof(SpecialBookTextItem));
	pPointsRemain = pMemory ? ::new (pMemory) SpecialBookTextItem(&SPECIALBookMenu::RefreshTotalPointsValue,
								  "Index_PointsRemainDigit1:0", "Index_PointsRemainDigit2:0", ActorValue::VALUE_NONE)
							: nullptr;
	Pages[8].TextItems.Add(pPointsRemain);
	pMemory = ourMalloc(sizeof(SpecialBookButton));
	pNext = pMemory ? ::new (pMemory) SpecialBookButton(reinterpret_cast<void*>(&SPECIALBookDoneCallback),
						  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshAllDoneButton), "AllDone_Btn:0")
					: nullptr;
	Pages[8].Buttons.Add(pNext);
	pMemory = ourMalloc(sizeof(SpecialBookButton));
	pPrevious = pMemory ? ::new (pMemory) SpecialBookButton(reinterpret_cast<void*>(&SPECIALBookMenu::PreviousPage),
							  reinterpret_cast<void*>(&SPECIALBookMenu::RefreshPreviousPageButton), "Index_LT_Btn:0")
						: nullptr;
	Pages[8].Buttons.Add(pPrevious);

	LoadBBTextures();
	// Every page's refresh callbacks, each item and button fetched once for the argument and once for the call.
	for (int32_t i = 0; i < 9; ++i) {
		for (uint32_t uiItem = 0; uiItem < Pages[i].TextItems.iSize; ++uiItem) {
			SpecialBookTextItem* pItem = Pages[i].TextItems.operator[](uiItem);
			Pages[i].TextItems.operator[](uiItem)->pfRefreshField(pItem);
		}
		for (uint32_t uiButton = 0; uiButton < Pages[i].Buttons.iSize; ++uiButton) {
			SpecialBookButton* pButton = Pages[i].Buttons.operator[](uiButton);
			Pages[i].Buttons.operator[](uiButton)->pfRefresh(pButton);
		}
	}
}

void SPECIALBookMenu::LoadBBTextures() {
	char Dst[8];
	char apDest[264];
	char acPath[260];
	for (int32_t i = 0; i < 11; ++i) {
		BSsprintf(apDest, 260, "%s%s%i.dds", "Textures\\Terminals\\", "BBNumber", i);
		SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
		TES::pInstance->CreateTextureImage(acPath, spAVValues[i], 0, 0);
	}
	// Entries 1 to 4 land in spPointsText[0] to [3].
	for (int32_t j = 1; j < 5; ++j) {
		BSsprintf(apDest, 260, "%s%s%i.dds", "Textures\\Terminals\\ENGLISH\\", "BBMessage0", j);
		SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
		TES::pInstance->CreateTextureImage(acPath, spPointsText[j - 1], 0, 0);
	}
	if (Interface::Has360Controller()) {
		BSstrcpy(Dst, 5, "XBOX");
	} else {
		BSstrcpy(Dst, 5, "PC");
	}
	BSsprintf(acPath, 260, "%s\\%s\\%s.dds", "Textures\\Terminals\\", Dst, "BBRTOff");
	TES::pInstance->CreateTextureImage(acPath, spNextPageButtonImage[1], 0, 0);
	BSsprintf(apDest, 260, "%s%s.dds", "Textures\\Terminals\\", "BBRTOn");
	SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
	TES::pInstance->CreateTextureImage(acPath, spNextPageButtonImage[0], 0, 0);
	BSsprintf(acPath, 260, "%s\\%s\\%s.dds", "Textures\\Terminals\\", Dst, "BBLTOff");
	TES::pInstance->CreateTextureImage(acPath, spPreviousPageButtonImage[0], 0, 0);
	BSsprintf(apDest, 260, "%s%s.dds", "Textures\\Terminals\\", "BBLTOn");
	SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
	TES::pInstance->CreateTextureImage(acPath, spPreviousPageButtonImage[1], 0, 0);
	BSsprintf(acPath, 260, "%s\\%s\\%s.dds", "Textures\\Terminals\\", Dst, "BBXOff");
	TES::pInstance->CreateTextureImage(acPath, spOpenCloseBookButtonImage[0], 0, 0);
	BSsprintf(acPath, 260, "%s\\%s\\%s.dds", "Textures\\Terminals\\", Dst, "BBXOn");
	TES::pInstance->CreateTextureImage(acPath, spOpenCloseBookButtonImage[1], 0, 0);
	BSsprintf(apDest, 260, "%s%s.dds", "Textures\\Terminals\\", "BBDecreaseOff");
	SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
	TES::pInstance->CreateTextureImage(acPath, spAVLeftArrowButtonImage[0], 0, 0);
	BSsprintf(apDest, 260, "%s%s.dds", "Textures\\Terminals\\", "BBDecreaseOn");
	SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
	TES::pInstance->CreateTextureImage(acPath, spAVLeftArrowButtonImage[1], 0, 0);
	BSsprintf(apDest, 260, "%s%s.dds", "Textures\\Terminals\\", "BBIncreaseOff");
	SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
	TES::pInstance->CreateTextureImage(acPath, spAVRightArrowButtonImage[0], 0, 0);
	BSsprintf(apDest, 260, "%s%s.dds", "Textures\\Terminals\\", "BBIncreaseOn");
	SwapPlatformLanguageTexturePath(apDest, acPath, 260u);
	TES::pInstance->CreateTextureImage(acPath, spAVRightArrowButtonImage[1], 0, 0);
}

NiCamera* SPECIALBookMenu::InitCamera() {
	int32_t ScreenHeight;
	NiCamera* pcam;
	NiPoint3 v12;
	NiPoint3 dst;
	NiRect<float> port;
	void* apMemory = NiMemObject::operator new(sizeof(NiCamera));
	if (apMemory) {
		pcam = ::new (apMemory) NiCamera;
	} else {
		pcam = 0;
	}
	::new (&port) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
	if (BSShaderManager::bLetterbox) {
		port = BSShaderManager::kFramebufferViewport;
	}
	pcam->SetViewPort(port);
	NiFrustum fr(0);
	SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
	fr.m_fFar = WorldSceneGraph->GetFarDistance();
	fr.m_fNear = BSSceneGraph::fNearDistance.Float();
	float v8 = fr.m_fFar / fr.m_fNear;
	pcam->SetMaxFarNearRatio(v8);
	if (BSShaderManager::bLetterbox) {
		ScreenHeight = BSShaderManager::iLetterboxHeight;
	} else {
		ScreenHeight = Renderer::GetScreenHeight();
	}
	float fHeightByWidth = static_cast<double>(ScreenHeight) / static_cast<double>(Renderer::GetScreenWidth());
	float faspectDelta = 0.75f;
	float theta = BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369 * 0.15000000596046448;
	fr.m_fLeft = -std::tan(theta) * 0.75f;
	fr.m_fRight = std::tan(theta) * 0.75f;
	double v1 = std::tan(theta);
	fr.m_fBottom = -v1 * fHeightByWidth * 0.75f;
	double v2 = std::tan(theta);
	fr.m_fTop = v2 * fHeightByWidth * 0.75f;
	fr.m_bOrtho = 0;
	pcam->SetViewFrustum(fr);
	pcam->SetTranslate(0.0f, 0.0f, 1.0f);
	NiUpdateData UpdateData2(0.0f, 0, 0);
	pcam->Update(UpdateData2);
	NiPoint3* a4 = ::new (&dst) NiPoint3(0.0f, 1.0f, 0.0f);
	NiPoint3* v3 = ::new (&v12) NiPoint3(0.0f, 0.0f, 0.0f);
	pcam->LookAtWorldPoint(*v3, *a4);
	pcam->SetMinNearPlaneDist(1.0f);
	NiFixedString apString("Surgery3DCamera");
	NiFixedString* kName = &apString;
	pcam->SetName(*kName);
	return pcam;
}

void SPECIALBookMenu::Create(int32_t aiTotalPoints) {
	int32_t v2;
	int32_t imissingIndex;
	InterfaceManager* v1 = InterfaceManager::Instance();
	Tile* ptile = v1->GetMenusRoot()->ReadFile("Data\\Menus\\CharGen\\SPECIALBookMenu.xml");
	Menu* pmenu = ptile->GetMenu();
	if (pmenu && (v2 = pmenu->GetClass(), v2 == SPECIALBookMenu::GetClassStatic())) {
		pmenu->SetMenuTile(static_cast<TileMenu*>(ptile), 0);
		SPECIALBookMenu::pSPECIALBookMenu = static_cast<SPECIALBookMenu*>(pmenu);
		if (static_cast<LoveTesterMenu*>(pmenu)->LoveTesterMenu::CheckTilesExist(&imissingIndex)) {
			SPECIALBookMenu::pSPECIALBookMenu->iAvailablePoints = aiTotalPoints;
			SPECIALBookMenu::pSPECIALBookMenu->InitBook();
			SPECIALBookMenu::sp3DCamera.operator=(SPECIALBookMenu::pSPECIALBookMenu->InitCamera());
			HUDMainMenu::ForceObjectiveReminders(1);
			pmenu->PrepForVisibility(0);
		} else {
			Warning("MENUS:  Special book menu creation failed... Missing index = #%d", imissingIndex);
			if (ptile) {
				delete ptile;
			}
			Interface::EmergencyCloseAllMenusAndBreakStuff();
		}
	} else {
		if (pmenu) {
			if (pmenu->pMenu) {
				delete pmenu;
			}
		}
	}
}

int32_t SPECIALBookMenu::GetClassStatic() {
	return 1060;
}

void SPECIALBookMenu::Draw3DElements() {
	if (SPECIALBookMenu::pSPECIALBookMenu) {
		if (SPECIALBookMenu::sp3DCamera.m_pObject) {
			NiRenderer* Renderer_ = NiRenderer::GetRenderer();
			Renderer_->ClearBuffer(0, 4u);
			SceneGraph* apSceneGraph = Main::GetWorldSceneGraph();
			BSCullingProcess* apCuller = apSceneGraph->pCuller;
			apCuller->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
			NiCamera* pcam = SPECIALBookMenu::sp3DCamera.m_pObject;
			NiCamera::HandleEyeFinityFOVScale(pcam, 0.15f);
			InterfaceManager* v1 = InterfaceManager::Instance();
			NiPointer<BSShaderAccumulator> spaccum(v1->Get3dAccumulator());
			BSShaderAccumulator* afScale = spaccum.m_pObject;
			apCuller->SetAccumulator(afScale);
			ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(1);
			BSShaderUtil::AccumulateScene(pcam, ShadowSceneNode_, *apCuller);
			BSShaderAccumulator* v3 = spaccum.m_pObject;
			BSShaderUtil::RenderScene(pcam, v3, 0);
			apCuller->SetAccumulator(0);
			apCuller->PopCullMode();
		}
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
	// The player is cast to ActorValueOwner through a reference, without a null check, as the game does.
	int32_t iValue = static_cast<ActorValueOwner&>(*pPlayer)
						 .GetPermanentActorValue(static_cast<ActorValue::Index>(apTextItem->eActorValue));
	NiSourceTexture* pTexture = pSPECIALBookMenu->spAVValues[iValue].operator->();
	static_cast<BSShaderPPLightingProperty*>(apTextItem->pText[0])
		->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, pTexture);
}

void SPECIALBookMenu::RefreshTotalPointsText(SPECIALBookMenu::SpecialBookTextItem* apTextItem) {
	int32_t v1 = SPECIALBookMenu::pSPECIALBookMenu->iAvailablePoints - PlayerCharacter::GetSumOfSPECIALStats();
	if (v1 <= 1) {
		if (v1 == 1) {
			apTextItem->pText[0u]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0,
				SPECIALBookMenu::pSPECIALBookMenu->spPointsText[2].m_pObject);
		} else {
			if (!v1) {
				apTextItem->pText[0u]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0,
					SPECIALBookMenu::pSPECIALBookMenu->spPointsText[3].m_pObject);
			}
		}
	} else {
		apTextItem->pText[0u]->SetDiffuseTexture(0, SPECIALBookMenu::pSPECIALBookMenu->spPointsText[1].m_pObject);
	}
}

void SPECIALBookMenu::RefreshTotalPointsValue(SPECIALBookMenu::SpecialBookTextItem* apTextItem) {
	if (apTextItem->pText[0u]) {
		if (apTextItem->pText[1]) {
			int32_t v1 = SPECIALBookMenu::pSPECIALBookMenu->iAvailablePoints - PlayerCharacter::GetSumOfSPECIALStats();
			if (v1) {
				if (v1 / 10 <= 0) {
					apTextItem->pTextObj[0u]->SetAppCulled(1);
				} else {
					apTextItem->pTextObj[0u]->SetAppCulled(0);
					apTextItem->pText[0u]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0,
						SPECIALBookMenu::pSPECIALBookMenu->spAVValues[v1 / 10].m_pObject);
				}
				apTextItem->pTextObj[1]->SetAppCulled(0);
				apTextItem->pText[1]->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0,
					SPECIALBookMenu::pSPECIALBookMenu->spAVValues[v1 % 10].m_pObject);
			} else {
				apTextItem->pTextObj[0u]->SetAppCulled(1);
				apTextItem->pTextObj[1]->SetAppCulled(1);
			}
		}
	}
}

void SPECIALBookMenu::RefreshLeftAVButton(SPECIALBookMenu::ActorValueButton* apButton) {
	ActorValue::Index eActorValue = apButton->eActorValue;
	if (pPlayer->GetPermanentActorValue(eActorValue) <= 1) {
		apButton->spButton.m_pObject->SetAppCulled(1);
	} else {
		apButton->spButton.m_pObject->SetAppCulled(0);
	}
	if (InterfaceManager::Has360Controller() &&
		SPECIALBookMenu::pSPECIALBookMenu->eState == SPECIALBookMenu::SBMS_SPECIAL &&
		eActorValue != SPECIALBookMenu::pSPECIALBookMenu->eXBoxIndexState + 4) {
		apButton->spButton.m_pObject->SetAppCulled(1);
	}
}

void SPECIALBookMenu::RefreshRightAVButton(SPECIALBookMenu::ActorValueButton* apButton) {
	ActorValue::Index eActorValue = apButton->eActorValue;
	if (pPlayer->GetPermanentActorValue(eActorValue) >= 10 ||
		(SPECIALBookMenu::pSPECIALBookMenu &&
			PlayerCharacter::GetSumOfSPECIALStats() >= SPECIALBookMenu::pSPECIALBookMenu->iAvailablePoints)) {
		apButton->spButton.m_pObject->SetAppCulled(1);
	} else {
		apButton->spButton.m_pObject->SetAppCulled(0);
	}
	if (InterfaceManager::Has360Controller() &&
		SPECIALBookMenu::pSPECIALBookMenu->eState == SPECIALBookMenu::SBMS_SPECIAL &&
		eActorValue != SPECIALBookMenu::pSPECIALBookMenu->eXBoxIndexState + 4) {
		apButton->spButton.m_pObject->SetAppCulled(1);
	}
}

// Called through SpecialBookButton::pfRefresh, whose callers ignore any return value.
void SPECIALBookMenu::RefreshNextPageButton(SPECIALBookMenu::SpecialBookButton* apButton) {
	apButton->pImage->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0,
		SPECIALBookMenu::pSPECIALBookMenu->spNextPageButtonImage[1].m_pObject);
}

// Called through SpecialBookButton::pfRefresh, whose callers ignore any return value.
void SPECIALBookMenu::RefreshPreviousPageButton(SPECIALBookMenu::SpecialBookButton* apButton) {
	apButton->pImage->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0,
		SPECIALBookMenu::pSPECIALBookMenu->spPreviousPageButtonImage[0].m_pObject);
}

// Called through SpecialBookButton::pfRefresh, whose callers ignore any return value.
void SPECIALBookMenu::RefreshAllDoneButton(SPECIALBookMenu::SpecialBookButton* apButton) {
	if (PlayerCharacter::GetSumOfSPECIALStats() >= SPECIALBookMenu::pSPECIALBookMenu->iAvailablePoints) {
		apButton->pImage->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0,
			SPECIALBookMenu::pSPECIALBookMenu->spOpenCloseBookButtonImage[0].m_pObject);
	} else {
		apButton->pImage->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0,
			SPECIALBookMenu::pSPECIALBookMenu->spOpenCloseBookButtonImage[1].m_pObject);
	}
}

void SPECIALBookMenu::IncreaseActorValue(SPECIALBookMenu::ActorValueButton* apButton) {
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	if (SPECIALBookMenu::pSPECIALBookMenu) {
		ActorValue::Index eActorValue = apButton->eActorValue;
		if (eActorValue != ActorValue::VALUE_NONE) {
			int32_t v6 = pPlayer->GetPermanentActorValue(eActorValue);
			if (v6 < 10) {
				BSSoundHandle th_s;
				BSAudio* v1 = &BSAudio::QInstance();
				BSSoundHandle* arHandle = v1->GetSoundHandleByName(&a1, "OBJBookSpecialNumber", 289u);
				th_s.operator=(*arHandle);
				th_s.Play(0);
			}
			if (PlayerCharacter::GetSumOfSPECIALStats() < SPECIALBookMenu::pSPECIALBookMenu->iAvailablePoints) {
				int32_t v2 = BSMIN(v6 + 1, 10);
				pPlayer->SetActorBaseValue(eActorValue, v2);
			}
		}
		SPECIALBookMenu::pSPECIALBookMenu->CallPageCallbacks();
	}
}

void SPECIALBookMenu::DecreaseActorValue(SPECIALBookMenu::ActorValueButton* apButton) {
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	if (SPECIALBookMenu::pSPECIALBookMenu) {
		ActorValue::Index eActorValue = apButton->eActorValue;
		if (eActorValue != ActorValue::VALUE_NONE) {
			int32_t v6 = pPlayer->GetPermanentActorValue(eActorValue);
			if (v6 > 1) {
				BSSoundHandle th_s;
				BSAudio* v1 = &BSAudio::QInstance();
				BSSoundHandle* arHandle = v1->GetSoundHandleByName(&a1, "OBJBookSpecialNumber", 289u);
				th_s.operator=(*arHandle);
				th_s.Play(0);
			}
			int32_t v2 = BSMAX(v6 - 1, 1);
			pPlayer->SetActorBaseValue(eActorValue, v2);
		}
		SPECIALBookMenu::pSPECIALBookMenu->CallPageCallbacks();
	}
}
