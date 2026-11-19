#include "Fallout/Interface/Menus/CreditsMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSDevices/Controls.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
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
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"
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
            if ((pMenu->spScrollRoot != static_cast<NiNode*>(nullptr)) && (pMenu->spScrollRoot->m_pkParent)) {
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
    StartMenu::pMe->SetFlag(StartMenu::kSomethingSave, 0);
    if ((!Interface::GetMessageMenuResult()) && (hooverDamSave)) {
        FalloutAudio::SetMainMenuMusic(false);
        FalloutAudioMedia::MediaStop();
        hooverDamSave->LoadGame();
    }
}

CreditsMenu::CreditsMenu()
  : Menu(), pClickRect(nullptr), pBackgroundTile(nullptr), spScrollRoot(nullptr), iFileOffset(0), iFileSize(0),
    pFileData(nullptr), eFont(FONT_2), iJustification(1), iX(0), iY(0), Color(NiColorA_WHITE), uiCreditsTotalHeight(0),
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
    else {
        if (!aiID) {
            pBackgroundTile = apTile;
        }
    }
}

void CreditsMenu::DoIdle() {
    static char bCloseCreditsBecauseTheGameShipsSoon;
    Controls* pcontrols = appMain->GetControls();
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
        if (pcontrols->GetMouseButton(0, 1) ||
            (pcontrols->GetUserAction(Controls::UACTN_UP, 1) ||
             InterfaceManager::CheckMenuButton(Interface::eButtonA, true) == 2)) {
            CreditsMenu::Close();
        }
        if (spScrollRoot.m_pObject) {
            if (BackgroundTextureFilenames.iSize) {
                fBackgroundSwapTimer = fBackgroundSwapTimer - appTimer.fDelta;
                if (fBackgroundSwapTimer < 0.0 && uiBackgroundIndexCounter < BackgroundTextureFilenames.iSize) {
                    uint32_t uiIndex = uiBackgroundIndexCounter;
                    uiBackgroundIndexCounter = uiIndex + 1;
                    const char* HandleString = (&BackgroundTextureFilenames.GetAt(uiIndex))->GetHandleString();
                    pBackgroundTile->Set(4044, HandleString, true);
                    fBackgroundSwapTimer = fBackgroundSwapInterval;
                }
            }
            NiPoint3 pos = spScrollRoot.m_pObject->GetTranslate();
            float* v2 = &fCreditsScrollSpeed.Float();
            pos.z = appTimer.fDelta * *v2 + pos.z;
            spScrollRoot.m_pObject->SetTranslate(pos);
            NiUpdateData UpdateData(0.0f, false, false);
            spScrollRoot.m_pObject->Update(UpdateData);
            const NiBound* WorldBound = &spScrollRoot.m_pObject->GetWorldBound();
            double Seconds = WorldBound->m_fRadius;
            float fdoubleBound = Seconds + Seconds;
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
    NiPoint3 dst;
    int32_t iwidth;
    BSFile* pFileToUse_2;
    char Dst[260];
    Controls* Controls_ = appMain->GetControls();
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(0xED568u);
    BGSPerk* v35 = dynamic_cast<BGSPerk*>(FormByNumericID);
    bool v18 = Controls_->GetKeyboardButton(29, 0) ||
        (Controls_->GetKeyboardButton(157, 0) || pPlayer->GetPerkRank(v35, false));
    bool bUseWackyCredits = v18;
    if ((pPlayCredits) && (pPlayCredits->fValue == 2.0)) {
        bUseWackyCredits = true;
    }

    static constexpr char kDataPrefix[] = "DATA\\";
    for (uint32_t i = 0; i < sizeof(kDataPrefix); ++i) {
        Dst[i] = kDataPrefix[i];
    }
    std::memset(&Dst[6], 0, 254u);
    LANGUAGE elang = GetCurrentTexLanguage();
    const char* LanguageString = GetLanguageString(elang);
    BSstrcat(Dst, 260, LanguageString);
    BSstrcat(Dst, 260, "\\");
    if (bUseWackyCredits) {
        BSstrcat(Dst, 260, "CREDITSWACKY.TXT");
    }
    else {
        BSstrcat(Dst, 260, "CREDITS.TXT");
    }
    BSFile LangSpecificFile(Dst, NiFile::READ_ONLY, 0x4000u, false);
    BSFile DefaultCreditsFile(
        bUseWackyCredits ? "DATA\\CREDITSWACKY.TXT" : "DATA\\CREDITS.TXT", NiFile::READ_ONLY, 0x4000u, false
    );
    BSstrcpy(Dst, 260, "DATA\\");
    const char* v3 = GetLanguageString(elang);
    BSstrcat(Dst, 260, v3);
    BSstrcat(Dst, 260, "\\");
    if (bUseWackyCredits) {
        BSstrcat(Dst, 260, "CREDITSWACKYPATCH.TXT");
    }
    else {
        BSstrcat(Dst, 260, "CREDITSPATCH.TXT");
    }
    BSFile LangSpecificFile_2(Dst, NiFile::READ_ONLY, 0x4000u, false);
    BSFile LangSpecificFile_3(
        bUseWackyCredits ? "DATA\\CREDITSWACKYPATCH.TXT" : "DATA\\CREDITSPATCH.TXT", NiFile::READ_ONLY, 0x4000u, false
    );
    if (LangSpecificFile.BSFile::Exist()) {
        BSFile* pFileToUse;
        if (LangSpecificFile_2.BSFile::Exist()) {
            pFileToUse = &LangSpecificFile_2;
        }
        else {
            pFileToUse = &LangSpecificFile;
        }
        pFileToUse_2 = pFileToUse;
    }
    else {
        BSFile* pFileToUse_3;
        if (LangSpecificFile_3.BSFile::Exist()) {
            pFileToUse_3 = &LangSpecificFile_3;
        }
        else {
            pFileToUse_3 = &DefaultCreditsFile;
        }
        pFileToUse_2 = pFileToUse_3;
    }
    if (pFileToUse_2 && pFileToUse_2->Exist()) {
        iFileSize = pFileToUse_2->GetSize() + 1;
        char* v26 = static_cast<char*>(ourMalloc(iFileSize));
        pFileData = v26;
        pFileData[iFileSize - 1] = 0;
        if (pFileToUse_2->NiBinaryStream::Read(pFileData, iFileSize - 1) == iFileSize - 1) {
            NiAlphaProperty* palpha_2;
            NiNode* v15;
            NiNode* v24 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
            if (v24) {
                v15 = ::new (v24) NiNode(0);
            }
            else {
                v15 = nullptr;
            }
            NiRefObject* v25 = (NiRefObject*)v15;
            spScrollRoot.operator=(v15);
            NiPoint3* v4 = ::new (&dst) NiPoint3(0.0f, 0.0f, -1020.0f);
            spScrollRoot.m_pObject->SetTranslate(*v4);
            NiDX9Renderer* NIRenderer = Renderer::GetNIRenderer();
            NIRenderer->LockRenderer();
            NiNode* Model_ = pClickRect->GetModel();
            Model_->AttachChild(spScrollRoot.m_pObject, 1);
            NiAlphaProperty* v21 = static_cast<NiAlphaProperty*>(NiMemObject::operator new(sizeof(NiAlphaProperty)));
            if (v21) {
                palpha_2 = ::new (v21) NiAlphaProperty;
            }
            else {
                palpha_2 = nullptr;
            }
            palpha_2->SetAlphaBlending(true);
            spScrollRoot.m_pObject->AttachProperty(palpha_2);
            Font* pfont = FontManager::Instance()->GetFont(static_cast<int32_t>(eFont));
            BSStringT<char> string;
            while (iFileOffset < iFileSize) {
                if (pFileData[iFileOffset] == 13) {
                    iY -= static_cast<int32_t>(pfont->fFontHeight);
                    uiCreditsTotalHeight += static_cast<int32_t>(pfont->fFontHeight);
                }
                while (pFileData[iFileOffset] != 10 && iFileOffset < iFileSize) {
                    char v12 = pFileData[iFileOffset];
                    if (v12 == 42) {
                        SkipLine();
                    }
                    else {
                        if (v12 == 60) {
                            switch (pFileData[++iFileOffset]) {
                                case 66: {
                                    iFileOffset += 2;
                                    GetString(&string);
                                    BSStringT<char> strBuffer;
                                    strBuffer.SPrintF("Interface\\Credits\\%s", string.pString);
                                    NiFixedString apString(static_cast<const char*>(strBuffer.pString));
                                    NiFixedString* aItem = &apString;
                                    BackgroundTextureFilenames.Add(*aItem);
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
                                    float fimageAlpha =
                                        static_cast<double>(std::atoi(&pFileData[iFileOffset + 12])) / 255.0;
                                    iFileOffset += 16;
                                    GetString(&string);
                                    NiNode* pimage = CreateImage(&string, iimageWidth, iimageHeight, fimageAlpha);
                                    NiNode* v11 = spScrollRoot.m_pObject;
                                    v11->AttachChild(pimage, 1);
                                    iY -= iimageHeight;
                                    uiCreditsTotalHeight += iimageHeight;
                                    break;
                                }
                                case 74: {
                                    if (pFileData[iFileOffset + 2] == 67) {
                                        iJustification = 2;
                                    }
                                    else {
                                        if (pFileData[iFileOffset + 2] == 82) {
                                            iJustification = 4;
                                        }
                                        else {
                                            iJustification = 1;
                                        }
                                    }
                                    break;
                                }
                                case 88: {
                                    if (pFileData[iFileOffset + 2] == 43) {
                                        iX += std::atoi(&pFileData[iFileOffset + 3]);
                                    }
                                    else {
                                        if (pFileData[iFileOffset + 2] == 45) {
                                            iX -= std::atoi(&pFileData[iFileOffset + 3]);
                                        }
                                        else {
                                            iX = std::atoi(&pFileData[iFileOffset + 2]);
                                        }
                                    }
                                    break;
                                }
                                case 89: {
                                    if (pFileData[iFileOffset + 2] == 43) {
                                        iY -= std::atoi(&pFileData[iFileOffset + 3]);
                                    }
                                    else {
                                        if (pFileData[iFileOffset + 2] == 45) {
                                            iY += std::atoi(&pFileData[iFileOffset + 3]);
                                        }
                                        else {
                                            iY = std::atoi(&pFileData[iFileOffset + 2]);
                                        }
                                    }
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                            SkipKey();
                        }
                        else {
                            if (static_cast<uint8_t>(pFileData[iFileOffset]) >= 32u) {
                                GetString(&string);
                                iwidth = 1280;
                                NiAVObject* pobj = pfont->MakeString(
                                    0.0f, 0.0f, 0.0f, string, iwidth, iJustification, Color, true, true
                                );
                                pobj->SetTranslate(static_cast<float>(iX), 0.0f, static_cast<float>(iY));
                                NiNode* v9 = spScrollRoot.m_pObject;
                                v9->AttachChild(pobj, 1);
                                iY -= static_cast<int32_t>(pfont->fFontHeight);
                                uiCreditsTotalHeight += static_cast<int32_t>(pfont->fFontHeight);
                            }
                            else {
                                ++iFileOffset;
                            }
                        }
                    }
                }
                SkipLine();
                ++iFileOffset;
            }
            ourFree(pFileData);
            pFileData = nullptr;
            BSShaderManager::PrepareObject(spScrollRoot.m_pObject, 0, 0);
            NiUpdateData UpdateData(0.0f, false, false);
            spScrollRoot.m_pObject->Update(UpdateData);
            NiDX9Renderer* v6 = Renderer::GetNIRenderer();
            v6->UnlockRenderer();
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

bool CreditsMenu::GetString(BSStringT<char>* a1) {
    char apString[264];
    char ch = pFileData[iFileOffset];
    uint32_t istringOffset = 0;
    while (ch != 13 && (ch != 60 && (ch != 62 && (ch != 42 && iFileOffset < iFileSize)))) {
        apString[istringOffset++] = ch;
        if (++iFileOffset < iFileSize) {
            ch = pFileData[iFileOffset];
        }
    }
    apString[istringOffset] = 0;
    return a1->Set(apString, 0);
}

NiNode* CreditsMenu::CreateImage(BSStringT<char>* apImageFilename, int32_t aiWidth, int32_t aiHeight, float afAlpha) {
    NiMaterialProperty* pmaterial;
    TileShaderProperty* ptprop;
    NiTriShape* ptri;
    NiAlphaProperty* palphaProp;
    NiNode* pnode;
    NiPoint2* ptextures;
    NiPoint3* pvertices;
    NiPoint2 v55;
    NiPoint2 v56;
    NiPoint2 v57;
    NiPoint2 v58;
    NiPoint3 v59;
    NiPoint3 v60;
    NiPoint3 v61;
    NiPoint3 dst;
    char apDest[268];
    void* ptr = ourMalloc(4 * sizeof(NiPoint3));
    if (ptr) {
        rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint3), 4, &emptyPointConstructor);
        pvertices = static_cast<NiPoint3*>(ptr);
    }
    else {
        pvertices = nullptr;
    }
    void* v63 = ourMalloc(4 * sizeof(NiPoint2));
    if (v63) {
        rt::eh_vector_constructor_iterator(v63, sizeof(NiPoint2), 4, &emptyPointConstructor);
        ptextures = static_cast<NiPoint2*>(v63);
    }
    else {
        ptextures = nullptr;
    }
    void* v64 = ptextures;
    uint16_t* pvertOrder = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
    NiPoint3* v5 = ::new (&dst) NiPoint3(0.0f, 0.01f, 0.0f);
    *pvertices = *v5;
    NiPoint3* v6 = ::new (&v61) NiPoint3(0.0f, 0.01f, static_cast<float>(-aiHeight));
    pvertices[1] = *v6;
    NiPoint3* v7 = ::new (&v60) NiPoint3(static_cast<float>(aiWidth), 0.01f, 0.0f);
    pvertices[2] = *v7;
    NiPoint3* v8 = ::new (&v59) NiPoint3(static_cast<float>(aiWidth), 0.01f, static_cast<float>(-aiHeight));
    pvertices[3] = *v8;
    NiPoint2* v9 = ::new (&v58) NiPoint2(0.0f, 0.0f);
    float x = v9->x;
    float y = v9->y;
    ptextures->x = x;
    ptextures->y = y;
    NiPoint2* v13 = ::new (&v57) NiPoint2(0.0f, 1.0f);
    float v14 = v13->x;
    float v15 = v13->y;
    ptextures[1].x = v14;
    ptextures[1].y = v15;
    NiPoint2* v17 = ::new (&v56) NiPoint2(1.0f, 0.0f);
    float v18 = v17->x;
    float v19 = v17->y;
    ptextures[2].x = v18;
    ptextures[2].y = v19;
    NiPoint2* v21 = ::new (&v55) NiPoint2(1.0f, 1.0f);
    float v22 = v21->x;
    float v23 = v21->y;
    ptextures[3].x = v22;
    ptextures[3].y = v23;
    *pvertOrder = 0;
    pvertOrder[1] = 1;
    pvertOrder[2] = 2;
    pvertOrder[3] = 2;
    pvertOrder[4] = 1;
    pvertOrder[5] = 3;
    void* apMemory = NiMemObject::operator new(sizeof(NiNode));
    if (apMemory) {
        pnode = ::new (apMemory) NiNode(0);
    }
    else {
        pnode = nullptr;
    }
    int32_t ix = iX;
    if (iJustification == 2) {
        ix -= aiWidth / 2;
    }
    else {
        if (iJustification == 4) {
            ix -= aiWidth;
        }
    }
    pnode->SetTranslate(static_cast<float>(ix), 0.0f, static_cast<float>(iY));
    NiAlphaProperty* v51 = static_cast<NiAlphaProperty*>(NiMemObject::operator new(sizeof(NiAlphaProperty)));
    if (v51) {
        palphaProp = ::new (v51) NiAlphaProperty;
    }
    else {
        palphaProp = nullptr;
    }
    palphaProp->SetAlphaBlending(true);
    pnode->AttachProperty(palphaProp);
    NiTriShape* v49 = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
    if (v49) {
        ptri = ::new (v49)
            NiTriShape(4, pvertices, nullptr, nullptr, ptextures, 1, NiGeometryData::NBT_METHOD_NONE, 2, pvertOrder);
    }
    else {
        ptri = nullptr;
    }
    pnode->AttachChild(ptri, 1);
    NiFixedString textureFilename;
    BSsprintf(apDest, 260, "Textures\\Interface\\Credits\\%s", apImageFilename->pString);
    ptri->SetName(static_cast<const char*>(apDest));
    NiSourceTexture* ptexture =
        NiSourceTexture::Create(static_cast<const char*>(apDest), NiSourceTexture::ms_kDefaultPrefs, true, false);
    TileShaderProperty* v46 = static_cast<TileShaderProperty*>(NiMemObject::operator new(sizeof(TileShaderProperty)));
    if (v46) {
        ptprop = ::new (v46) TileShaderProperty(false);
    }
    else {
        ptprop = nullptr;
    }
    ptprop->SetTileTexture(ptexture);
    ptprop->SetTileAlpha(afAlpha);
    ptri->AttachProperty(ptprop);
    NiMaterialProperty* v45 = static_cast<NiMaterialProperty*>(NiMemObject::operator new(sizeof(NiMaterialProperty)));
    if (v45) {
        pmaterial = ::new (v45) NiMaterialProperty;
    }
    else {
        pmaterial = nullptr;
    }
    pmaterial->SetEmittance(NiColor_WHITE);
    ptri->AttachProperty(pmaterial);
    NiBound* ModelBound = &ptri->GetModelBound();
    ModelBound->ComputeFromData(4, pvertices);
    BSShaderManager::PrepareObject(pnode, 0, 1);
    return pnode;
}

bool CreditsMenu::Create() {
    int32_t v0;
    Tile* pold_menu_tile = Menu::GetMenuByClass(1047);
    if (pold_menu_tile) {
        delete pold_menu_tile;
    }
    InterfaceManager* pin = InterfaceManager::Instance();
    float ftop_depth = Tile::GetMaximumDepth();
    Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\Options\\credits_menu.xml");
    Menu* pmenu = ptile->GetMenu();
    if (pmenu && (v0 = pmenu->GetClass(), v0 == CreditsMenu::GetClassStatic())) {
        TileMenu* v1 = dynamic_cast<TileMenu*>(ptile);
        pmenu->SetMenuTile(v1, false);
        CreditsMenu* pme = dynamic_cast<CreditsMenu*>(pmenu);
        if (pme->IsWellFormedMenu()) {
            if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                ptile->Set(4013, ftop_depth, true);
            }
            bool v3 = pin->IsInMenuMode();
            ptile->Set(Tile::kTileValue_User0, static_cast<int32_t>(v3));
            pme->InitCreditsRoot();
            if (pme->BackgroundTextureFilenames.iSize) {
                double uiCreditsTotalHeight = static_cast<double>(pme->uiCreditsTotalHeight);
                double v9 = uiCreditsTotalHeight - pme->spScrollRoot->GetTranslate().z;
                double v8 = v9 / fCreditsScrollSpeed.Float();
                pme->fBackgroundSwapInterval = v8 / static_cast<double>(pme->BackgroundTextureFilenames.iSize);
                pme->fBackgroundSwapTimer = pme->fBackgroundSwapInterval;
                uint32_t uiIndex = pme->uiBackgroundIndexCounter;
                pme->uiBackgroundIndexCounter = uiIndex + 1;
                const char* HandleString =
                    (&pme->BackgroundTextureFilenames.GetAt(static_cast<int32_t>(uiIndex)))->GetHandleString();
                pme->pBackgroundTile->Set(4044, HandleString, true);
            }
            else {
                if ((bPlayingCreditsMenuMovie || !pme->BackgroundTextureFilenames.iSize) && (pme->pBackgroundTile)) {
                    pme->pBackgroundTile->Set(4009, 0);
                }
            }
            double ScreenHeight = static_cast<double>(Renderer::GetScreenHeight());
            if (ScreenHeight / static_cast<double>(Renderer::GetScreenWidth()) != 0.75) {
                NiNode* pnode = pme->pClickRect->GetModel();
                NiPoint3 pos = pnode->GetTranslate();
                pos.x = pos.x + 214.0;
                pnode->SetTranslate(pos);
                NiUpdateData UpdateData(0.0f, false, false);
                pnode->Update(UpdateData);
            }
            return true;
        }

        Warning("Credits Menu Creation Failed... Are your menu and art resources up to date?");
        return false;
    }

    if ((pmenu) && (pmenu->pMenu)) {
        delete pmenu;
    }

    return false;
}

int32_t CreditsMenu::GetClassStatic() {
    return Interface::eCreditsMenu;
}

void CreditsMenu::Close() {
    pPlayCredits->SetValue(0.0f);
    if ((StartMenu::pMe->IsFlagSet(StartMenu::kSomethingSave)) && (BSstrlen(pSaveLoadManager->pSaveName.pString))) {
        pSaveLoadManager->SetupSavesList();
        if ((pSaveLoadManager) && (pSaveLoadManager->pSaveGameList)) {
            for (BGSSaveLoadFileEntry*& rpItem : pSaveLoadManager->pSaveGameList->ItemsUntilNull()) {
                hooverDamSave = rpItem;
                if ((hooverDamSave->IsSystemSave()) && (hooverDamSave->GetPlayerName())) {
                    char* afValue = hooverDamSave->pFileName;
                    if (BSstristr(pSaveLoadManager->pSaveName.pString, afValue)) {
                        const char* v1 = sNo.String();
                        const char* v0 = sYes.String();
                        Interface::CreateMessageMenu(
                            gs_sContinuePlayingAfterCredits.operator const char*(), static_cast<const char*>(nullptr),
                            static_cast<const char*>(nullptr), &ContinuePlayingAfterCreditsCallback, 0, 18, 0.0f, 0.0f,
                            v0, v1, nullptr
                        );
                        return;
                    }
                }
            }
        }
    }

    StartMenu::pMe->SetFlag(StartMenu::kSomethingSave, 0);
    CloseCreditsMenu();
}
