#include "Fallout/Interface/MenuConsole.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSMenu/DebugText.h"
#include "BSMenu/Font.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSShaderProperty.h"
#include "BSSystem/BSFile.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/BookMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

// The game settings this file defines, in their initialization order.
SettingT<INISettingCollection> iConsoleHistorySize("iConsoleHistorySize:Menu", 50);
SettingT<INISettingCollection> iConsoleVisibleLines("iConsoleVisibleLines:Menu", 15);
SettingT<INISettingCollection> iConsoleFont("iConsoleFont:Menu", 2);
SettingT<INISettingCollection> iConsoleTextXPos("iConsoleTextXPos:Menu", 30);
SettingT<INISettingCollection> iConsoleTextYPos("iConsoleTextYPos:Menu", 940);
SettingT<INISettingCollection> iConsoleLineSpacing("iConsoleLineSpacing:Menu", 35);

MenuConsole::MenuConsole()
  : pConsoleCompiler(nullptr), xStrings(), xInputs(), iInputsPos(0), iLastString(0), cVisible(0), bIsActive(false),
    bRecordingBatch(false), iRecordingLineCount(0) {
    int32_t iFont = iConsoleFont.Int() - 1;
    iConsoleLineSpacing = static_cast<int32_t>(FontManager::Instance()->GetFont(iFont)->GetBaseLine()) + 3;
    iConsoleXPos = iConsoleTextXPos.Int();
    iConsoleYPos = iConsoleTextYPos.Int();
    ourMemset(pOutputFileName, 0, 260u);
}

MenuConsole::~MenuConsole() {
    delete pConsoleCompiler;
}

void MenuConsole::Init() {
    pConsoleCompiler = new ScriptCompiler();
}

void MenuConsole::ScrollBy(int32_t aiNumLines) {
    int32_t iLast = static_cast<int32_t>(aiNumLines + iLastString);
    if (iLast > static_cast<int32_t>(xStrings.m_uiCount)) {
        iLast = static_cast<int32_t>(xStrings.m_uiCount);
    }
    if (static_cast<int32_t>(iLast - iConsoleVisibleLines.UInt()) <= 0) {
        iLast = static_cast<int32_t>(iConsoleVisibleLines.UInt());
    }
    if (iLast > static_cast<int32_t>(xStrings.m_uiCount)) {
        iLast = static_cast<int32_t>(xStrings.m_uiCount);
    }
    iLastString = static_cast<uint32_t>(iLast);
}

void MenuConsole::RefreshLines() {
    NiTListItem<BSStringT<char>>* pPos = xStrings.NiTListBase<BSStringT<char>>::GetHeadPos();
    int32_t iY = static_cast<int32_t>(iConsoleYPos - iConsoleLineSpacing * iConsoleVisibleLines.UInt());
    int32_t iLast = static_cast<int32_t>(iLastString);
    int32_t i;
    for (i = static_cast<int32_t>(iLast - iConsoleVisibleLines.UInt()); i < 0; ++i) {
        float fY = static_cast<float>(static_cast<double>(iY));
        float fX = static_cast<float>(static_cast<double>(iConsoleXPos));
        DebugText* pDebugText = DebugText::Instance(true);
        pDebugText->Print(nullptr, fX, fY, 1, -1, -1.0f, 0, nullptr);
        iY += static_cast<int32_t>(iConsoleLineSpacing);
    }
    for (int32_t j = 0; j < i; ++j) {
        pPos = xStrings.NiTListBase<BSStringT<char>>::GetNextPos(pPos);
    }
    while (i < iLast) {
        uint32_t uiFont = iConsoleFont.UInt();
        float fY = static_cast<float>(static_cast<double>(iY));
        float fX = static_cast<float>(static_cast<double>(iConsoleXPos));
        char* pcLine = xStrings.Get(pPos).pString;
        DebugText* pDebugText = DebugText::Instance(true);
        pDebugText->Print(pcLine, fX, fY, 1, -1, -1.0f, uiFont, nullptr);
        pPos = xStrings.NiTListBase<BSStringT<char>>::GetNextPos(pPos);
        ++i;
        iY += static_cast<int32_t>(iConsoleLineSpacing);
    }
}

bool MenuConsole::IsVisible() {
    // The state byte is sign-extended: zero or negative reads as closed.
    return this->cVisible > 0;
}

void MenuConsole::OnEnterMenuMode() {
    if (cVisible > 0) {
        int32_t iFont = iConsoleFont.Int();
        float fY = static_cast<float>(iConsoleYPos);
        float fX = static_cast<float>(iConsoleXPos);
        DebugText* pDebugText = DebugText::Instance(true);
        pDebugText->Print("|", fX, fY, 1, -1, -1.0f, iFont, nullptr);
        ScrollBy(xStrings.m_uiCount);
        RefreshLines();
        Interface::SetCrosshairRef(0);
    }
}

bool MenuConsole::ToggleVisible() {
    BSShaderProperty::PreviousTransform::ToggleConsole();
    if (!cVisible) {
        SetVisible(1);
        return true;
    }
    // Only state 1 may switch to state 2 (shift held, one menu entered, none on the second stack); every other
    // state closes the console.
    if (cVisible == 1 &&
        InterfaceManager::Instance()->IsShiftDown() &&
        InterfaceManager::Instance()->GetEnterStack(0) == 3 &&
        !InterfaceManager::Instance()->GetEnterStack(1)) {
        SetVisible(2);
    }
    else {
        SetVisible(0);
    }
    return false;
}

// The exe's cleanup-only C++ EH frame is left out.
bool MenuConsole::Idle(int32_t aiKey) {
    NiFrustum xfrust;
    if (cVisible <= 0) {
        return false;
    }
    if (!InterfaceManager::Instance()->IsInMenuMode()) {
        return false;
    }
    if (aiKey) {
        char cDebugText[2052];
        int32_t ipos;
        // Starts zeroed (a departure): mbsnbcpy leaves it unterminated past 2046 bytes, and the length scan and the
        // key edits then read past the copy; the exe reads those bytes unset.
        char cLine[2052] = {};
        int32_t ilen;
        float fY = static_cast<float>(iConsoleYPos);
        float fX = static_cast<float>(iConsoleXPos);
        DebugText* pDebugText = DebugText::Instance(true);
        pDebugText->GetDebugText_0(cDebugText, fX, fY, 1);
        mbsnbcpy_(cLine, cDebugText, 2046u);
        for (ilen = 0; cLine[ilen]; ++ilen) {
            ;
        }
        // Tested only on a non-empty line (a departure): the exe tests the byte before the buffer, unset, when the
        // line is empty.
        if (ilen > 0 && cLine[ilen - 1] == 124) {
            cLine[--ilen] = 0;
        }
        for (ipos = 0; ipos < ilen && cLine[ipos] != 124; ++ipos) {
            ;
        }
        if (ipos == ilen) {
            cLine[ipos] = 124;
            ++ilen;
        }
        if (aiKey == 0x80000008u) {
            for (int32_t i = 0; i < ilen + 1; ++i) {
                if (cLine[i] == 124) {
                    cLine[i] = cLine[i + 1];
                    cLine[i + 1] = 124;
                }
            }
            for (int32_t i = 0; i < ilen + 1; ++i) {
                if (cLine[i] == 10) {
                    if (cLine[i - 1] == 45) {
                        for (int32_t j = i - 1; j < ilen + 1; ++j) {
                            cLine[j] = cLine[j + 2];
                        }
                    }
                    else {
                        cLine[i] = 32;
                    }
                }
                if (cLine[i] == 10) {
                    cLine[i] = 32;
                }
            }
            if (cLine[0]) {
                g_bHasPlayerUsedConsole = true;
                InterfaceManager* pim = InterfaceManager::Instance();
                ScrollBy(xStrings.m_uiCount);
                PrintLine(cLine);
                BSStringT<char> xstr(static_cast<const char*>(cLine));
                xInputs.AddHead(xstr);
                iInputsPos = 0;
                int32_t iFont = iConsoleFont.Int();
                fY = static_cast<float>(iConsoleYPos);
                fX = static_cast<float>(iConsoleXPos);
                pDebugText = DebugText::Instance(true);
                pDebugText->Print("|", fX, fY, 1, -1, -1.0f, iFont, nullptr);
                if (!_tcsnicmp(cLine, "dof", 3u)) {
                    float frange = 0.0f;
                    std::sscanf(cLine, "dof %f", &frange);
                    xfrust = pim->GetSceneGraph()->GetCamera()->GetViewFrustum();
                    xfrust.m_fFar = frange;
                    pim->GetSceneGraph()->GetCamera()->SetViewFrustum(xfrust);
                }
                else if (!_tcsnicmp(cLine, "usz", 3u)) {
                    int32_t isizex = 0;
                    int32_t isizey = 0;
                    std::sscanf(cLine, "usz %i %i", &isizex, &isizey);
                    Interface::UpdateScreenShape(isizex, isizey);
                }
                else if (!_tcsicmp(cLine, "clr") || !_tcscmp(cLine, "cls")) {
                    xStrings.RemoveAll();
                    iLastString = 0;
                    RefreshLines();
                }
                else if (!_tcsicmp(cLine, "tag")) {
                    TESObjectREFR* ppickRef = pim->GetPickedREFR();
                    if (ppickRef) {
                        BSStringT<char> xMessage;
                        int32_t iFormID = ppickRef->iFormID;
                        xMessage.SPrintF("\"%s\" (%08x)", ppickRef->GetFullName(), iFormID);
                        NiNode* p3D = ppickRef->Get3D();
                        NiNode* pnode = static_cast<NiNode*>(NiObject::DynamicCastStatic(&NiNode::ms_RTTI, p3D));
                        if (!pnode) {
                            p3D = ppickRef->Get3D();
                            pnode = static_cast<NiNode*>(
                                NiObject::DynamicCastStatic(&NiNode::ms_RTTI, p3D->m_pkParent)
                            );
                        }
                        DebugText::AddFloatingText(pnode, xMessage.pString, 0, -1.0f);
                    }
                }
                else if (!_tcsnicmp(cLine, "reload strings", 14u)) {
                    Tile* poldStringRoot = pim->pStringRoot;
                    if (poldStringRoot) {
                        delete poldStringRoot;
                    }
                    Menu::RemoveFromXMLArchive("Data\\Menus\\globals.xml", nullptr);
                    Tile* pstring = pim->GetMenusRoot()->ReadFile("Data\\Menus\\globals.xml");
                    pstring->SetParent(nullptr, nullptr);
                    pim->pStringRoot = pstring;
                }
                else if (!_tcsnicmp(cLine, "reload HUDReticle", 17u)) {
                    Menu::FreeXMLArchive();
                }
                else if (!_tcsnicmp(cLine, "reload HUDSafeZone", 18u)) {
                    Menu::FreeXMLArchive();
                    Interface::ToggleSafeZone(3);
                }
                else if (!_tcsicmp(cLine, "reload mainmenu")) {
                    Menu::FreeXMLArchive();
                    int32_t iClass = StartMenu::GetClassStatic();
                    Tile* pmainMenu = Menu::GetMenuByClass(iClass);
                    if (pmainMenu) {
                        delete pmainMenu;
                    }
                    StartMenu::Create(false, false);
                }
                else if (!_tcsnicmp(cLine, "reload ", 7u)) {
                    char cTrait[1028];
                    Menu::FreeXMLArchive();
                    std::sscanf(cLine, "reload %s", cTrait);
                    if (cTrait[0] != 38) {
                        int32_t i;
                        for (i = 0; cTrait[i]; ++i) {
                            ;
                        }
                        cTrait[i + 2] = 0;
                        cTrait[i + 1] = 59;
                        while (i > 0) {
                            cTrait[i] = cTrait[i - 1];
                            --i;
                        }
                        cTrait[0] = 38;
                    }
                    int32_t iTrait = Tile::TextToTrait(cTrait);
                    MenuManager::Instance(true)->ReloadMenu(iTrait);
                }
                else if (!_tcsnicmp(cLine, "delete ", 7u)) {
                    char cTrait[1028];
                    std::sscanf(cLine, "delete %s", cTrait);
                    int32_t iTrait = Tile::TextToTrait(cTrait);
                    MenuManager::Instance(true)->DeleteMenu(iTrait);
                }
                else if (!_tcsncmp(cLine, "front", 5u)) {
                    Menu* pFrontmost = MenuManager::Instance(true)->GetFrontmostMenu();
                    PrintLine("Frontmost: %s", pFrontmost->pMenu->GetName());
                }
                else if (!_tcsncmp(cLine, "stack", 5u)) {
                    for (int32_t i = 0; pim->GetEnterStack(i++); PrintLine("%i: %i", i, pim->GetEnterStack(i))) {
                        ;
                    }
                }
                else if (!_tcsncmp(cLine, "visible", 7u)) {
                    int32_t iClass = 0;
                    std::sscanf(cLine, "visible %i", &iClass);
                    PrintLine("Is Visible: %i", MenuManager::Instance(true)->IsMenuOpen(iClass));
                }
                else if (!_tcsncmp(cLine, "exit menus", 10u)) {
                    for (int32_t top = pim->GetEnterStackTop(); top; top = pim->GetEnterStackTop()) {
                        pim->PopFromEnterStack(top, false);
                    }
                }
                else if (!_tcsncmp(cLine, "emergency texture release", 25u)) {
                    Interface::SetFirstChanceTextureRelease();
                    PrintLine("First change release of hidden textures activated.");
                }
                else if (!_tcsncmp(cLine, "set trait", 9u)) {
                    char ctarget[128];
                    char ctrait[128];
                    float fval = 0.0f;
                    std::sscanf(cLine, "set trait %s %s %f", ctarget, ctrait, &fval);
                    Tile* ptile = Tile::GetTileByName(nullptr, ctarget);
                    int32_t itrait = Tile::TextToTrait(ctrait);
                    if (ptile && itrait > 0) {
                        PrintLine("Trait set.");
                        ptile->Set(itrait, fval, true);
                    }
                    else if (ptile) {
                        PrintLine("Unknown trait '%s'", ctrait);
                    }
                    else {
                        PrintLine("Can't find tile '%s'", ctarget);
                    }
                }
                else if (!_tcsncmp(cLine, "depth", 5u)) {
                    PrintLine("Max Depth: %i", static_cast<int32_t>(Tile::GetMaximumDepth()));
                    PrintLine(
                        "Cursor Depth: %i",
                        static_cast<int32_t>(InterfaceManager::Instance()->pCursor->GetFloat(4013))
                    );
                }
                else if (!_tcsncmp(cLine, "new fonts", 9u)) {
                    FontManager::Instance()->SetNewFonts(true);
                }
                else if (!_tcsncmp(cLine, "old fonts", 9u)) {
                    FontManager::Instance()->SetNewFonts(false);
                }
                else if (!_tcsncmp(cLine, "iso", 3u)) {
                    bool bfirst = cLine[3] == 49;
                    bool bsecond = cLine[4] == 49;
                    Interface::IsolateMenuElements(bfirst, bsecond);
                }
                else if (!_tcsncmp(cLine, "restore menus", 13u)) {
                    Interface::RestoreMenuElements();
                }
                else if (!_tcsncmp(cLine, "use workbook", 12u)) {
                    Tile* pmenuTile = Menu::GetMenuByClass(1026);
                    TileMenu* ptile = dynamic_cast<TileMenu*>(pmenuTile);
                    if (ptile) {
                        BookMenu* pbook = dynamic_cast<BookMenu*>(ptile->pMenu);
                        if (pbook) {
                            pbook->UseWorkbook();
                        }
                    }
                    else {
                        PrintLine("You must open a book (or scroll) before using the workbook.");
                    }
                }
                else if (!_tcsncmp(cLine, "dump textures", 13u)) {
                    DumpTextures();
                }
                else if (!_tcsncmp(cLine, "bat", 3u)) {
                    char cName[132];
                    std::memset(cName, 0, 128);
                    std::sscanf(cLine, "bat %s", cName);
                    if (std::strlen(cName)) {
                        char cTxtPath[264];
                        char cPath[264];
                        std::memset(cPath, 0, 260);
                        std::memset(cTxtPath, 0, 260);
                        BSstrcat(cPath, 260, cName);
                        BSFile* pfile = new BSFile(cPath, NiFile::READ_ONLY, 0x4000u, false);
                        uint32_t uiLength = BSstrlen(cName);
                        if ((!pfile || !pfile->Open(false, false)) &&
                            (uiLength <= 4 || BSstrnicmp(&cName[uiLength - 4], ".txt", 4u))) {
                            if (pfile) {
                                delete pfile;
                            }
                            BSstrcpy(cTxtPath, 260, cPath);
                            BSstrcat(cTxtPath, 260, ".txt");
                            pfile = new BSFile(cTxtPath, NiFile::READ_ONLY, 0x4000u, false);
                        }
                        if (pfile && pfile->Open(false, false)) {
                            char newText[2600];
                            while (pfile->GetLine(newText, 2600u, 13)) {
                                for (size_t k = 0; k < std::strlen(newText); ++k) {
                                    if (newText[k] == 10) {
                                        newText[k] = 32;
                                    }
                                }
                                if (std::strlen(newText) > 1) {
                                    Interface::PrintLine("> %s", newText);
                                    Script xScript;
                                    xScript.SetTemporary();
                                    xScript.SetText(newText);
                                    InterfaceManager* pmanager = InterfaceManager::Instance();
                                    xScript.CompileAndRun(
                                        pConsoleCompiler,
                                        SYSTEM_WINDOW_COMPILER,
                                        pmanager->GetPickedREFR()
                                    );
                                }
                            }
                        }
                        else if (BSstrlen(cTxtPath)) {
                            Interface::PrintLine("Neither file '%s' or '%s' could be opened.", cPath, cTxtPath);
                        }
                        else {
                            Interface::PrintLine("The file '%s' could not be opened.", cPath);
                        }
                        if (pfile) {
                            delete pfile;
                        }
                    }
                    else {
                        Interface::PrintLine("You must enter a filename.");
                    }
                }
                else if (!_tcsncmp(cLine, "rbat", 3u)) {
                    if (bRecordingBatch) {
                        char cName[132];
                        std::memset(cName, 0, 128);
                        std::sscanf(cLine, "rbat %s", cName);
                        if (std::strlen(cName)) {
                            if (iRecordingLineCount > 0) {
                                BSFile* pfile = new BSFile(cName, NiFile::WRITE_ONLY, 0x1000u, true);
                                if (pfile && pfile->Open(false, false)) {
                                    for (int32_t i = 0; i < iRecordingLineCount; ++i) {
                                        char cRecordLine[104];
                                        BSsprintf(cRecordLine, 100, "%s\r\n", cRecording[i]);
                                        uint32_t uiLength = BSstrlen(cRecordLine);
                                        pfile->WriteHere(cRecordLine, uiLength);
                                    }
                                    pfile->Close();
                                    Interface::PrintLine("Batch file '%s' created.", cName);
                                }
                                else {
                                    Interface::PrintLine("The file '%s' could not be opened.", cName);
                                }
                                if (pfile) {
                                    delete pfile;
                                }
                            }
                            else {
                                Interface::PrintLine("No commands recorded; recording cancelled.");
                            }
                        }
                        else {
                            Interface::PrintLine("No filename specified; recording cancelled.");
                        }
                        bRecordingBatch = false;
                        iRecordingLineCount = 0;
                    }
                    else {
                        Interface::PrintLine("Batch Recording Started.");
                        Interface::PrintLine(
                            "Use rbat {filename} to save batch file or rbat with no filename to cancel."
                        );
                        bRecordingBatch = true;
                        iRecordingLineCount = 0;
                        for (int32_t j = 0; j < 20; ++j) {
                            ourStrcpy(cRecording[j], "");
                        }
                    }
                }
                else {
                    if (bRecordingBatch) {
                        if (iRecordingLineCount >= 20) {
                            Interface::PrintLine(
                                "Recording full; command ignored. Use rbat to cancel or rbat {filename} to save."
                            );
                        }
                        else {
                            ourStrcpy(cRecording[iRecordingLineCount++], cLine);
                            Interface::PrintLine("Recorded.");
                        }
                    }
                    for (uint32_t i = 0; i < std::strlen(cLine); ++i) {
                        if (cLine[i] == 10) {
                            cLine[i] = 32;
                        }
                    }
                    Script xScript;
                    xScript.SetTemporary();
                    xScript.SetText(cLine);
                    InterfaceManager* pmanager = InterfaceManager::Instance();
                    xScript.CompileAndRun(pConsoleCompiler, SYSTEM_WINDOW_COMPILER, pmanager->GetPickedREFR());
                }
            }
        }
        else {
            switch (aiKey) {
                case (-2147483647 - 1): {
                    if (ilen && ipos) {
                        --ilen;
                        for (int32_t itpos = ipos; itpos < ilen; ++itpos) {
                            cLine[itpos] = cLine[itpos + 1];
                        }
                        cLine[ilen] = 0;
                        cLine[ipos - 1] = 124;
                    }

                    break;
                }
                case -0x7FFFFFF9: {
                    if (ipos < ilen) {
                        --ilen;
                        for (int32_t itpos = ipos; itpos < ilen; ++itpos) {
                            cLine[itpos] = cLine[itpos + 1];
                        }
                        cLine[ilen] = 0;
                        cLine[ipos] = 124;
                    }
                    break;
                }
                case -0x7FFFFFF7: {
                    ScrollBy(1 - iConsoleVisibleLines.Int());
                    RefreshLines();
                    break;
                }
                case -0x7FFFFFF6: {
                    ScrollBy(iConsoleVisibleLines.Int() - 1);
                    RefreshLines();
                    break;
                }
                case -0x7FFFFFFD: {
                    if (xInputs.m_uiCount) {
                        ++iInputsPos;
                        iInputsPos = BSMIN(iInputsPos, static_cast<int32_t>(xInputs.m_uiCount - 1));
                        NiTListItem<BSStringT<char>>* pkPos = xInputs.m_pkHead;
                        for (int32_t i = 0; i < iInputsPos; ++i) {
                            pkPos = xInputs.GetNextPos(pkPos);
                        }
                        std::sprintf(cLine, "%s", xInputs.Get(pkPos).String());
                    }
                    break;
                }
                case -0x7FFFFFFC: {
                    if (xInputs.m_uiCount) {
                        --iInputsPos;
                        iInputsPos = BSMAX(iInputsPos, 0);
                        BSStringT<char> xstr;
                        NiTListItem<BSStringT<char>>* pkPos = nullptr;
                        if (iInputsPos) {
                            pkPos = xInputs.m_pkHead;
                            for (int32_t i = 0; i < iInputsPos; ++i) {
                                pkPos = xInputs.GetNextPos(pkPos);
                            }
                        }
                        if (pkPos) {
                            xstr.Copy(xInputs.Get(pkPos));
                        }
                        else {
                            xstr = "";
                        }
                        char* pcText = xstr.GetLength() ? xstr.pString : const_cast<char*>("");
                        std::sprintf(cLine, "%s", pcText);
                    }
                    break;
                }
                case -0x7FFFFFFF: {
                    if (ilen && ipos) {
                        cLine[ipos] = cLine[ipos - 1];
                        cLine[ipos - 1] = 124;
                    }

                    break;
                }
                case -0x7FFFFFFE: {
                    if (ilen && ipos + 1 < ilen) {
                        cLine[ipos] = cLine[ipos + 1];
                        cLine[ipos + 1] = 124;
                    }

                    break;
                }
                case -0x7FFFFFFB: {
                    if (ilen && ipos) {
                        for (int32_t c = ipos; c > 0; --c) {
                            cLine[c] = cLine[c - 1];
                        }
                        cLine[0] = 124;
                    }

                    break;
                }
                case -0x7FFFFFFA: {
                    if (ilen && ipos + 1 < ilen) {
                        for (int32_t c = ipos; c < ilen - 1; ++c) {
                            cLine[c] = cLine[c + 1];
                        }
                        cLine[ilen - 1] = 124;
                    }

                    break;
                }
                default: {
                    cLine[ipos] = aiKey;
                    for (int32_t itpos = ++ilen; itpos > ipos; --itpos) {
                        cLine[itpos + 1] = cLine[itpos];
                    }
                    cLine[ilen + 1] = 0;
                    cLine[ipos + 1] = 124;
                    break;
                }
            }
            bool bhas_bar = false;
            int32_t i;
            for (i = 0; cLine[i]; ++i) {
                if (cLine[i] == 124) {
                    bhas_bar = true;
                    break;
                }
            }
            if (!bhas_bar) {
                cLine[i] = 124;
                cLine[i + 1] = 0;
            }
            BSStringT<char> xstr(static_cast<const char*>(cLine));
            if (xInputs.m_uiCount) {
                BSStringT<char> kRemoved = xInputs.RemoveHead();
                xInputs.AddHead(xstr);
            }
            else {
                xInputs.AddHead(xstr);
            }
            int32_t iFont = iConsoleFont.Int();
            fY = static_cast<float>(iConsoleYPos);
            fX = static_cast<float>(iConsoleXPos);
            pDebugText = DebugText::Instance(true);
            pDebugText->Print(cLine, fX, fY, 1, -1, -1.0f, iFont, nullptr);
        }
    }
    return true;
}

void MenuConsole::PrintLine(const char* apString, va_list xlist) {
    if (!Interface::IsInGameLoadingMenuOpen() || StartMenu::MainMenuIsActive()) {
        BSStringT<char> xstring;
        xstring.VSPrintF(apString, xlist);
        WriteToOutputFile(xstring.pString);
        int32_t iwidth = 1200;
        int32_t iheight = 0;
        int32_t iFont = iConsoleFont.Int() - 1;
        int32_t inum_lines = FontManager::Instance()->GetFont(iFont)->LineBreakText(xstring, iwidth, iheight, false, 0);
        if (inum_lines) {
            char cText[2052];
            for (uint32_t i = 0; i <= xstring.GetLength(); ++i) {
                cText[i] = xstring.pString[i];
            }
            char* pstring = cText;
            for (int32_t i = 0; i < inum_lines; ++i) {
                bool bat_end = iLastString == xStrings.m_uiCount;
                BSStringT<char> psave_me;
                psave_me = pstring;
                xStrings.AddTail(psave_me);
                if (bat_end) {
                    iLastString = xStrings.m_uiCount;
                }
                while (true) {
                    int32_t iCount = xStrings.m_uiCount;
                    if (iCount <= iConsoleHistorySize.Int()) {
                        break;
                    }
                    BSStringT<char> kRemoved = xStrings.RemoveHead();
                    --iLastString;
                }
                if (i + 1 < inum_lines) {
                    while (*pstring) {
                        ++pstring;
                    }
                    ++pstring;
                }
            }
            if (iLastString == xStrings.m_uiCount) {
                RefreshLines();
            }
        }
    }
}

// An unused empty string is built before the call and destroyed after it.
void MenuConsole::PrintLine(const char* apString, ...) {
    BSStringT<char> kUnused;
    va_list xlist;
    va_start(xlist, apString);
    // The va_list overload: an exact match for xlist, where the variadic one takes it through the ellipsis.
    this->PrintLine(apString, xlist);
    va_end(xlist);
}

void MenuConsole::SetVisible(char acState) {
    char cold_state = cVisible;
    cVisible = acState;
    if (acState > 0 != cold_state > 0) {
        if (acState <= 0) {
            InitializationFlow();
            DebugText::Instance(true)->DeleteAllDebugText();
            InterfaceManager::Instance()->ClearTargetREFR();
        }
        else {
            InitializationFlow();
            if (InterfaceManager::Instance()->IsInMenuMode()) {
                int32_t iFont = iConsoleFont.Int();
                float fY = static_cast<float>(iConsoleYPos);
                float fX = static_cast<float>(iConsoleXPos);
                DebugText* pDebugText = DebugText::Instance(true);
                pDebugText->Print("|", fX, fY, 1, -1, -1.0f, iFont, nullptr);
                ScrollBy(xStrings.m_uiCount);
            }
            else {
                float fY = static_cast<float>(iConsoleYPos);
                float fX = static_cast<float>(iConsoleXPos);
                DebugText* pDebugText = DebugText::Instance(true);
                pDebugText->Print("", fX, fY, 1, -1, -1.0f, 0, nullptr);
            }
            RefreshLines();
        }
    }
}

void MenuConsole::DumpTextures() {
    NiSourceTexture* pLargestTex[200];
    ourMemset(pLargestTex, 0, sizeof(pLargestTex));
    for (NiTexture* pTex = NiTexture::GetListHead(); pTex; pTex = pTex->m_pkNext) {
        if (NiObject::IsKindOf_0(&NiSourceTexture::ms_RTTI, pTex)) {
            NiSourceTexture* pSTex = static_cast<NiSourceTexture*>(pTex);
            if (pTex->m_pkRendererData && (bShowMenuTextureUse.Bool() || !IsMenuTexture(pSTex))) {
                int32_t idx = -1;
                for (int32_t i = 0; i < 200 && idx == -1; ++i) {
                    if (!pLargestTex[i]) {
                        idx = i;
                    }
                }
                if (idx != -1) {
                    for (int32_t i = 199; i > idx; --i) {
                        pLargestTex[i] = pLargestTex[i - 1];
                    }
                    pLargestTex[idx] = pSTex;
                }
            }
        }
    }
    BSFile* pfile = new BSFile("textures.txt", NiFile::WRITE_ONLY, 0x1000u, true);
    if (pfile && pfile->Open(false, false)) {
        char cLine[1024];
        uint32_t ultotalMemory = 0;
        const char* pcWorldName;
        if (TES::pInstance->GetCurrentCell() && TES::pInstance->GetCurrentCell()->GetWorldSpace()) {
            pcWorldName = TES::pInstance->GetCurrentCell()->GetWorldSpace()->GetFullName();
        }
        else {
            pcWorldName = "{unknown}";
        }
        const char* pcCellName;
        if (TES::pInstance->GetCurrentCell()) {
            pcCellName = TES::pInstance->GetCurrentCell()->GetFullName();
        }
        else {
            pcCellName = "{unknown}";
        }
        BSsprintf(
            cLine,
            1024,
            "Texture dump for cell: %s grid: (%d, %d) worldspace: %s \n---------------------------------------\n",
            pcCellName,
            TES::pInstance->iCurrentGridX,
            TES::pInstance->iCurrentGridY,
            pcWorldName
        );
        uint32_t uiLength = BSstrlen(cLine);
        pfile->WriteHere(cLine, uiLength);
        for (int32_t j = 0; j < 200 && pLargestTex[j]; ++j) {
            pLargestTex[j]->GetFilename().GetHandleString();
            uiLength = BSstrlen(cLine);
            pfile->WriteHere(cLine, uiLength);
        }
        BSsprintf(cLine, 1024, "---------------------------------------\nTotal memory: %dkb", ultotalMemory);
        uiLength = BSstrlen(cLine);
        pfile->WriteHere(cLine, uiLength);
        pfile->Close();
        Interface::PrintLine("200 largest loaded textures dumped to the file textures.txt.");
    }
    else {
        Interface::PrintLine("The file textures.txt could not be opened.");
    }
}

char MenuConsole::IsMenuTexture(NiSourceTexture* apTexture) {
    bool bretval = false;
    if (apTexture && apTexture->GetFilename().GetHandleString()) {
        const char* pcName = apTexture->GetFilename().GetHandleString();
        const char* pcSlash = std::strchr(pcName, 92);
        while (pcSlash) {
            if (!_strnicmp(pcSlash, "\\menus", 6u)) {
                bretval = true;
                pcSlash = nullptr;
            }
            else {
                pcSlash = std::strchr(pcSlash + 1, 92);
            }
        }
    }

    return bretval;
}

void MenuConsole::SetOutputFilename(const char* apOutputFileName) {
    if (apOutputFileName) {
        BSstrcpy(pOutputFileName, 260, apOutputFileName);
    }
    else {
        pOutputFileName[0] = 0;
    }
}

void MenuConsole::WriteToOutputFile(const char* apOutText) {
    if (pOutputFileName[0]) {
        uint32_t ilen = apOutText ? std::strlen(apOutText) : 0;
        if (ilen) {
            BSFile outfile(pOutputFileName, NiFile::WRITE_ONLY, 0x2800u, true);
            if (outfile.BSFile::Open(false, true)) {
                outfile.BSFile::Seek(0, NiFile::ms_iSeekEnd);
                outfile.BSFile::WriteF(apOutText, ilen);
                outfile.BSFile::WriteF("\r\n", 2u);
            }
            outfile.Close();
        }
    }
}

MenuConsole* MenuConsole::Instance(bool abCreate) {
    if (MenuConsole::pInstance || !abCreate) {
        return MenuConsole::pInstance;
    }
    MenuConsole* pConsole = new MenuConsole;
    MenuConsole::pInstance = pConsole;
    pConsole->Init();
    return MenuConsole::pInstance;
}

void MenuConsole::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void MenuConsole::Free() {
    if (MenuConsole::pInstance) {
        delete MenuConsole::pInstance;
    }
    MenuConsole::pInstance = nullptr;
}
