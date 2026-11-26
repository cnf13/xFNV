#include "Fallout/Interface/Menus/CreditsMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSDevices/Controls.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/TileShaderProperty.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Audio/FalloutAudioMedia.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/LoveTesterMenu.h"
#include "Fallout/Interface/Menus/SPECIALBookMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Interface/Menus/TraitMenu.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFileEntry.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SourceTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> fCreditsScrollSpeed("fCreditsScrollSpeed:Menu", 55.0f);

// Closes the credits: under the renderer lock, detaches the scroll root from its parent and releases it, then fades
// the menu out; marks that the user pressed start.
void CloseCreditsMenu() {
    Tile* pMenuTile = Menu::GetMenuByClass(Interface::eCreditsMenu);
    if (pMenuTile) {
        // The credits menu's tile holds the CreditsMenu (class 0x417).
        CreditsMenu* pMenu = static_cast<CreditsMenu*>(pMenuTile->GetMenu());
        if (pMenu) {
            Renderer::GetNIRenderer()->LockRenderer();
            if (pMenu->spScrollRoot != static_cast<NiNode*>(nullptr) && pMenu->spScrollRoot->m_pkParent) {
                NiNode* pParent = pMenu->spScrollRoot->m_pkParent;
                pParent->DetachChild(pMenu->spScrollRoot);
            }

            pMenu->spScrollRoot = static_cast<NiNode*>(nullptr);
            pMenuTile->Set(Tile::kTileValue_DeleteOnFade, 1);
            pMenu->StartFadeOut();
            Renderer::GetNIRenderer()->UnlockRenderer();
        }
        StartMenu::pMe->SetFlag(StartMenu::FLAG_USER_PRESSED_START, 1);
    }
}

void ContinuePlayingAfterCreditsCallback() {
    CloseCreditsMenu();
    StartMenu::pMe->SetFlag(StartMenu::FLAG_END_GAME_CREDITS, 0);
    if (!Interface::GetMessageMenuResult() && hooverDamSave) {
        FalloutAudio::SetMainMenuMusic(false);
        FalloutAudioMedia::MediaStop();
        hooverDamSave->LoadGame();
    }
}

CreditsMenu::CreditsMenu()
  : Menu(), pClickRect(nullptr), pBackgroundTile(nullptr), spScrollRoot(nullptr), iFileOffset(0), iFileSize(0),
    pFileData(nullptr), eFont(FONT_2), iJustification(1), iX(0), iY(0), Color(NiColorA::WHITE), uiCreditsTotalHeight(0),
    fBackgroundSwapTimer(0.0f), uiBackgroundIndexCounter(0) {
    hooverDamSave = nullptr;
}

CreditsMenu::~CreditsMenu() {
    pPlayCredits->SetValue(0.0f);
    if (StartMenu::pMe) {
        StartMenu::pMe->Music();
    }
}

void CreditsMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (aiID == 1) {
        pClickRect = apTile;
    }
    else if (!aiID) {
        pBackgroundTile = apTile;
    }
}

void CreditsMenu::DoIdle() {
    static char bCloseCreditsBecauseTheGameShipsSoon;
    Controls* pControls = appMain->GetControls();
    if (Interface::IsMenuIDVisible(Interface::eMessageMenu, 0)) {
        pClickRect->Set(4013, 0);
        pClickRect->Set(4003, 0);
        bCloseCreditsBecauseTheGameShipsSoon = 1;
    }
    else {
        if (bCloseCreditsBecauseTheGameShipsSoon) {
            bCloseCreditsBecauseTheGameShipsSoon = 0;
            CreditsMenu::Close();
        }
        if (pControls->GetMouseButton(0, 1) ||
            pControls->GetUserAction(Controls::UACTN_UP, 1) ||
            InterfaceManager::CheckMenuButton(Interface::eButtonA, true) == 2) {
            CreditsMenu::Close();
        }
        if (spScrollRoot.m_pObject) {
            if (BackgroundTextureFilenames.iSize) {
                fBackgroundSwapTimer = fBackgroundSwapTimer - appTimer.fDelta;
                if (fBackgroundSwapTimer < 0.0 && uiBackgroundIndexCounter < BackgroundTextureFilenames.iSize) {
                    uint32_t uiIndex = uiBackgroundIndexCounter;
                    uiBackgroundIndexCounter = uiIndex + 1;
                    const char* pcFilename = BackgroundTextureFilenames.GetAt(uiIndex).GetHandleString();
                    pBackgroundTile->Set(4044, pcFilename, true);
                    fBackgroundSwapTimer = fBackgroundSwapInterval;
                }
            }
            NiPoint3 pos = spScrollRoot->GetTranslate();
            pos.z = appTimer.fDelta * fCreditsScrollSpeed.Float() + pos.z;
            spScrollRoot->SetTranslate(pos);
            NiUpdateData kUpdateData(0.0f, false, false);
            spScrollRoot->Update(kUpdateData);
            double dRadius = spScrollRoot->GetWorldBound().m_fRadius;
            float fdoubleBound = dRadius + dRadius;
            if (fdoubleBound < static_cast<double>(pos.z)) {
                CreditsMenu::Close();
            }
        }
    }
}

bool CreditsMenu::DoGamepad(int32_t aiButton, float afValue) {
    if (aiButton >= -1 && aiButton <= 4) {
        return false;
    }
    if (!Interface::IsTopMenuID(Interface::eCreditsMenu) || Interface::IsMenuVisible(Interface::eMessageMenu)) {
        return false;
    }
    Close();
    return true;
}

bool CreditsMenu::IsWellFormedMenu() {
    return pClickRect && pBackgroundTile;
}

void CreditsMenu::InitCreditsRoot() {
    BSFile* pFileToUse;
    char cPath[260];
    Controls* pControls = appMain->GetControls();
    TESForm* pForm = TESForm::GetFormByNumericID(0xED568u);
    BGSPerk* pPerk = dynamic_cast<BGSPerk*>(pForm);
    bool bUseWackyCredits = pControls->GetKeyboardButton(29, 0) ||
        pControls->GetKeyboardButton(157, 0) ||
        pPlayer->GetPerkRank(pPerk, false);
    if (pPlayCredits && pPlayCredits->fValue == 2.0) {
        bUseWackyCredits = true;
    }

    static constexpr char kDataPrefix[] = "DATA\\";
    for (uint32_t i = 0; i < sizeof(kDataPrefix); ++i) {
        cPath[i] = kDataPrefix[i];
    }
    std::memset(&cPath[6], 0, 254u);
    LANGUAGE elang = GetCurrentTexLanguage();
    BSstrcat(cPath, 260, GetLanguageString(elang));
    BSstrcat(cPath, 260, "\\");
    if (bUseWackyCredits) {
        BSstrcat(cPath, 260, "CREDITSWACKY.TXT");
    }
    else {
        BSstrcat(cPath, 260, "CREDITS.TXT");
    }
    BSFile LangSpecificFile(cPath, NiFile::READ_ONLY, 0x4000u, false);
    BSFile DefaultCreditsFile(
        bUseWackyCredits ? "DATA\\CREDITSWACKY.TXT" : "DATA\\CREDITS.TXT",
        NiFile::READ_ONLY,
        0x4000u,
        false
    );
    BSstrcpy(cPath, 260, "DATA\\");
    BSstrcat(cPath, 260, GetLanguageString(elang));
    BSstrcat(cPath, 260, "\\");
    if (bUseWackyCredits) {
        BSstrcat(cPath, 260, "CREDITSWACKYPATCH.TXT");
    }
    else {
        BSstrcat(cPath, 260, "CREDITSPATCH.TXT");
    }
    BSFile LangSpecificPatchFile(cPath, NiFile::READ_ONLY, 0x4000u, false);
    BSFile DefaultPatchFile(
        bUseWackyCredits ? "DATA\\CREDITSWACKYPATCH.TXT" : "DATA\\CREDITSPATCH.TXT",
        NiFile::READ_ONLY,
        0x4000u,
        false
    );
    if (LangSpecificFile.BSFile::Exist()) {
        if (LangSpecificPatchFile.BSFile::Exist()) {
            pFileToUse = &LangSpecificPatchFile;
        }
        else {
            pFileToUse = &LangSpecificFile;
        }
    }
    else if (DefaultPatchFile.BSFile::Exist()) {
        pFileToUse = &DefaultPatchFile;
    }
    else {
        pFileToUse = &DefaultCreditsFile;
    }
    if (pFileToUse && pFileToUse->Exist()) {
        iFileSize = pFileToUse->GetSize() + 1;
        pFileData = static_cast<char*>(ourMalloc(iFileSize));
        pFileData[iFileSize - 1] = 0;
        if (pFileToUse->NiBinaryStream::Read(pFileData, iFileSize - 1) == iFileSize - 1) {
            spScrollRoot = new NiNode(0);
            spScrollRoot->SetTranslate(NiPoint3(0.0f, 0.0f, -1020.0f));
            Renderer::GetNIRenderer()->LockRenderer();
            pClickRect->GetModel()->AttachChild(spScrollRoot.m_pObject, 1);
            NiAlphaProperty* pAlpha = new NiAlphaProperty;
            pAlpha->SetAlphaBlending(true);
            spScrollRoot->AttachProperty(pAlpha);
            Font* pfont = FontManager::Instance()->GetFont(static_cast<int32_t>(eFont));
            BSStringT<char> string;
            while (iFileOffset < iFileSize) {
                if (pFileData[iFileOffset] == 13) {
                    iY -= static_cast<int32_t>(pfont->fFontHeight);
                    uiCreditsTotalHeight += static_cast<int32_t>(pfont->fFontHeight);
                }
                while (pFileData[iFileOffset] != 10 && iFileOffset < iFileSize) {
                    char ch = pFileData[iFileOffset];
                    if (ch == 42) {
                        SkipLine();
                    }
                    else if (ch == 60) {
                        switch (pFileData[++iFileOffset]) {
                            case 66: {
                                iFileOffset += 2;
                                GetString(&string);
                                BSStringT<char> strBuffer;
                                strBuffer.SPrintF("Interface\\Credits\\%s", string.pString);
                                NiFixedString kFilename(static_cast<const char*>(strBuffer.pString));
                                BackgroundTextureFilenames.Add(kFilename);
                                break;
                            }
                            case 67: {
                                Color.r = static_cast<double>(std::atoi(&pFileData[iFileOffset + 2])) / 255.0;
                                Color.g = static_cast<double>(std::atoi(&pFileData[iFileOffset + 6])) / 255.0;
                                Color.b = static_cast<double>(std::atoi(&pFileData[iFileOffset + 10])) / 255.0;
                                Color.a = static_cast<double>(std::atoi(&pFileData[iFileOffset + 14])) / 255.0;
                                break;
                            }
                            case 70: {
                                eFont = static_cast<FONTS>(pFileData[iFileOffset + 2] - 49);
                                pfont = FontManager::Instance()->GetFont(static_cast<int32_t>(eFont));
                                break;
                            }
                            case 73: {
                                int32_t iimageWidth = std::atoi(&pFileData[iFileOffset + 2]);
                                int32_t iimageHeight = std::atoi(&pFileData[iFileOffset + 7]);
                                float fimageAlpha = static_cast<double>(std::atoi(&pFileData[iFileOffset + 12])) /
                                    255.0;
                                iFileOffset += 16;
                                GetString(&string);
                                NiNode* pimage = CreateImage(&string, iimageWidth, iimageHeight, fimageAlpha);
                                spScrollRoot->AttachChild(pimage, 1);
                                iY -= iimageHeight;
                                uiCreditsTotalHeight += iimageHeight;
                                break;
                            }
                            case 74: {
                                if (pFileData[iFileOffset + 2] == 67) {
                                    iJustification = 2;
                                }
                                else if (pFileData[iFileOffset + 2] == 82) {
                                    iJustification = 4;
                                }
                                else {
                                    iJustification = 1;
                                }
                                break;
                            }
                            case 88: {
                                if (pFileData[iFileOffset + 2] == 43) {
                                    iX += std::atoi(&pFileData[iFileOffset + 3]);
                                }
                                else if (pFileData[iFileOffset + 2] == 45) {
                                    iX -= std::atoi(&pFileData[iFileOffset + 3]);
                                }
                                else {
                                    iX = std::atoi(&pFileData[iFileOffset + 2]);
                                }
                                break;
                            }
                            case 89: {
                                if (pFileData[iFileOffset + 2] == 43) {
                                    iY -= std::atoi(&pFileData[iFileOffset + 3]);
                                }
                                else if (pFileData[iFileOffset + 2] == 45) {
                                    iY += std::atoi(&pFileData[iFileOffset + 3]);
                                }
                                else {
                                    iY = std::atoi(&pFileData[iFileOffset + 2]);
                                }
                                break;
                            }
                            default: {
                                break;
                            }
                        }
                        SkipKey();
                    }
                    else if (static_cast<uint8_t>(pFileData[iFileOffset]) >= 32u) {
                        GetString(&string);
                        int32_t iWidth = 1280;
                        NiAVObject* pobj = pfont->MakeString(
                            0.0f,
                            0.0f,
                            0.0f,
                            string,
                            iWidth,
                            iJustification,
                            Color,
                            true,
                            true
                        );
                        pobj->SetTranslate(static_cast<float>(iX), 0.0f, static_cast<float>(iY));
                        spScrollRoot->AttachChild(pobj, 1);
                        iY -= static_cast<int32_t>(pfont->fFontHeight);
                        uiCreditsTotalHeight += static_cast<int32_t>(pfont->fFontHeight);
                    }
                    else {
                        ++iFileOffset;
                    }
                }
                SkipLine();
                ++iFileOffset;
            }
            ourFree(pFileData);
            pFileData = nullptr;
            BSShaderManager::PrepareObject(spScrollRoot.m_pObject, 0, 0);
            NiUpdateData kUpdateData(0.0f, false, false);
            spScrollRoot->Update(kUpdateData);
            Renderer::GetNIRenderer()->UnlockRenderer();
        }
        else {
            ourFree(pFileData);
            pFileData = nullptr;
        }
    }
}

void CreditsMenu::SkipLine() {
    while (pFileData[iFileOffset] != 10 && iFileOffset < iFileSize) {
        ++iFileOffset;
    }
}

// The one caller ignores the return value.
void CreditsMenu::SkipKey() {
    do {
        char ch = pFileData[iFileOffset];
        ++iFileOffset;
        if (ch == 62) {
            break;
        }
    }
    while (iFileOffset < iFileSize);
}

bool CreditsMenu::GetString(BSStringT<char>* apString) {
    char cBuffer[264];
    char ch = pFileData[iFileOffset];
    uint32_t istringOffset = 0;
    while (ch != 13 && ch != 60 && ch != 62 && ch != 42 && iFileOffset < iFileSize) {
        cBuffer[istringOffset++] = ch;
        if (++iFileOffset < iFileSize) {
            ch = pFileData[iFileOffset];
        }
    }
    cBuffer[istringOffset] = 0;
    return apString->Set(cBuffer, 0);
}

NiNode* CreditsMenu::CreateImage(BSStringT<char>* apImageFilename, int32_t aiWidth, int32_t aiHeight, float afAlpha) {
    NiMaterialProperty* pmaterial;
    TileShaderProperty* ptprop;
    NiTriShape* ptri;
    NiAlphaProperty* palphaProp;
    NiNode* pnode;
    char cPath[268];
    NiPoint3* pvertices = new NiPoint3[4];
    NiPoint2* ptextures = new NiPoint2[4];
    uint16_t* pvertOrder = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
    pvertices[0] = NiPoint3(0.0f, 0.01f, 0.0f);
    pvertices[1] = NiPoint3(0.0f, 0.01f, static_cast<float>(-aiHeight));
    pvertices[2] = NiPoint3(static_cast<float>(aiWidth), 0.01f, 0.0f);
    pvertices[3] = NiPoint3(static_cast<float>(aiWidth), 0.01f, static_cast<float>(-aiHeight));
    NiPoint2 kTexCoord(0.0f, 0.0f);
    ptextures[0].x = kTexCoord.x;
    ptextures[0].y = kTexCoord.y;
    kTexCoord = NiPoint2(0.0f, 1.0f);
    ptextures[1].x = kTexCoord.x;
    ptextures[1].y = kTexCoord.y;
    kTexCoord = NiPoint2(1.0f, 0.0f);
    ptextures[2].x = kTexCoord.x;
    ptextures[2].y = kTexCoord.y;
    kTexCoord = NiPoint2(1.0f, 1.0f);
    ptextures[3].x = kTexCoord.x;
    ptextures[3].y = kTexCoord.y;
    pvertOrder[0] = 0;
    pvertOrder[1] = 1;
    pvertOrder[2] = 2;
    pvertOrder[3] = 2;
    pvertOrder[4] = 1;
    pvertOrder[5] = 3;
    pnode = new NiNode(0);
    int32_t ix = iX;
    if (iJustification == 2) {
        ix -= aiWidth / 2;
    }
    else if (iJustification == 4) {
        ix -= aiWidth;
    }
    pnode->SetTranslate(static_cast<float>(ix), 0.0f, static_cast<float>(iY));
    palphaProp = new NiAlphaProperty;
    palphaProp->SetAlphaBlending(true);
    pnode->AttachProperty(palphaProp);
    ptri = new NiTriShape(4, pvertices, nullptr, nullptr, ptextures, 1, NiGeometryData::NBT_METHOD_NONE, 2, pvertOrder);
    pnode->AttachChild(ptri, 1);
    NiFixedString textureFilename;
    BSsprintf(cPath, 260, "Textures\\Interface\\Credits\\%s", apImageFilename->pString);
    ptri->SetName(static_cast<const char*>(cPath));
    NiSourceTexture* ptexture = NiSourceTexture::Create(
        static_cast<const char*>(cPath),
        NiSourceTexture::ms_kDefaultPrefs,
        true,
        false
    );
    ptprop = new TileShaderProperty(false);
    ptprop->SetTileTexture(ptexture);
    ptprop->SetTileAlpha(afAlpha);
    ptri->AttachProperty(ptprop);
    pmaterial = new NiMaterialProperty;
    pmaterial->SetEmittance(NiColor::WHITE);
    ptri->AttachProperty(pmaterial);
    ptri->GetModelBound().ComputeFromData(4, pvertices);
    BSShaderManager::PrepareObject(pnode, 0, 1);
    return pnode;
}

bool CreditsMenu::Create() {
    Tile* pold_menu_tile = Menu::GetMenuByClass(1047);
    if (pold_menu_tile) {
        delete pold_menu_tile;
    }
    InterfaceManager* pin = InterfaceManager::Instance();
    float ftop_depth = Tile::GetMaximumDepth();
    Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\Options\\credits_menu.xml");
    Menu* pmenu = ptile->GetMenu();
    if (pmenu && pmenu->GetClass() == CreditsMenu::GetClassStatic()) {
        TileMenu* pMenuTile = dynamic_cast<TileMenu*>(ptile);
        pmenu->SetMenuTile(pMenuTile, false);
        CreditsMenu* pme = dynamic_cast<CreditsMenu*>(pmenu);
        if (pme->IsWellFormedMenu()) {
            if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                ptile->Set(4013, ftop_depth, true);
            }
            bool bInMenuMode = pin->IsInMenuMode();
            ptile->Set(Tile::kTileValue_User0, static_cast<int32_t>(bInMenuMode));
            pme->InitCreditsRoot();
            if (pme->BackgroundTextureFilenames.iSize) {
                double dTotalHeight = static_cast<double>(pme->uiCreditsTotalHeight);
                double dScrollDistance = dTotalHeight - pme->spScrollRoot->GetTranslate().z;
                double dScrollTime = dScrollDistance / fCreditsScrollSpeed.Float();
                pme->fBackgroundSwapInterval = dScrollTime / static_cast<double>(pme->BackgroundTextureFilenames.iSize);
                pme->fBackgroundSwapTimer = pme->fBackgroundSwapInterval;
                uint32_t uiIndex = pme->uiBackgroundIndexCounter;
                pme->uiBackgroundIndexCounter = uiIndex + 1;
                const char* pcFilename = pme->BackgroundTextureFilenames.GetAt(static_cast<int32_t>(uiIndex))
                                             .GetHandleString();
                pme->pBackgroundTile->Set(4044, pcFilename, true);
            }
            else if ((bPlayingCreditsMenuMovie || !pme->BackgroundTextureFilenames.iSize) && pme->pBackgroundTile) {
                pme->pBackgroundTile->Set(4009, 0);
            }
            double dScreenHeight = static_cast<double>(Renderer::GetScreenHeight());
            if (dScreenHeight / static_cast<double>(Renderer::GetScreenWidth()) != 0.75) {
                NiNode* pnode = pme->pClickRect->GetModel();
                NiPoint3 pos = pnode->GetTranslate();
                pos.x = pos.x + 214.0;
                pnode->SetTranslate(pos);
                NiUpdateData kUpdateData(0.0f, false, false);
                pnode->Update(kUpdateData);
            }
            return true;
        }

        Warning("Credits Menu Creation Failed... Are your menu and art resources up to date?");
        return false;
    }

    if (pmenu && pmenu->pMenu) {
        delete pmenu;
    }

    return false;
}

int32_t CreditsMenu::GetClassStatic() {
    return Interface::eCreditsMenu;
}

void CreditsMenu::Close() {
    pPlayCredits->SetValue(0.0f);
    if (StartMenu::pMe->IsFlagSet(StartMenu::FLAG_END_GAME_CREDITS) && BSstrlen(pSaveLoadManager->pSaveName.pString)) {
        pSaveLoadManager->SetupSavesList();
        if (pSaveLoadManager && pSaveLoadManager->pSaveGameList) {
            for (BGSSaveLoadFileEntry*& rpItem : pSaveLoadManager->pSaveGameList->ItemsUntilNull()) {
                hooverDamSave = rpItem;
                if (hooverDamSave->IsSystemSave() && hooverDamSave->GetPlayerName()) {
                    char* pcFileName = hooverDamSave->pFileName;
                    if (BSstristr(pSaveLoadManager->pSaveName.pString, pcFileName)) {
                        const char* pcNo = sNo.String();
                        const char* pcYes = sYes.String();
                        Interface::CreateMessageMenu(
                            static_cast<const char*>(gs_sContinuePlayingAfterCredits),
                            static_cast<const char*>(nullptr),
                            static_cast<const char*>(nullptr),
                            &ContinuePlayingAfterCreditsCallback,
                            0,
                            18,
                            0.0f,
                            0.0f,
                            pcYes,
                            pcNo,
                            nullptr

                        );
                        return;
                    }
                }
            }
        }
    }

    StartMenu::pMe->SetFlag(StartMenu::FLAG_END_GAME_CREDITS, 0);
    CloseCreditsMenu();
}
