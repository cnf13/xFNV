#include "BSMenu/Menu.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Controls.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/stdlib.h"

NiTStringPointerMap<XMLStorage*> Menu::XMLHash;
NiTStringPointerMap<Tile::BuildStorage*> Menu::XMLParsedHash;
Menu::xMenuList_t Menu::xMenuList(0, 1);

Menu::Menu()
  : pMenu(nullptr), pLastTile(nullptr), iIsModal(0), iMenuThickness(0), bRenderedMenu(false), bDeleteTemplates(true),
    xFadeState(Interface::FADE_HIDDEN) {}

Menu::~Menu() {
    if (iClass >= 1001 && iClass <= 1084) {
        Menu::pMenusVisible[iClass - 1001] = false;
    }
    if (iIsModal) {
        Interface::PopFromEnterStack(iIsModal, true);
    }
    Menu::SetMenuByClass(iClass, nullptr);
    if (pMenu) {
        pMenu->pMenu = nullptr;
        delete pMenu;
    }
    pMenu = nullptr;
    if (bDeleteTemplates) {
        for (Tile::TileTemplate* m_item : xTemplates) {
            delete m_item;
        }
    }
    xTemplates.RemoveAll();
    if (Interface::GetTopMenuID() == Interface::eNoMenu) {
        Controls* Controls_ = appMain->GetControls();
        if (Controls_->GetUserAction(Controls::UACTN_MENUMODE, 0)) {
            appMain->GetControls()->usOverrideFlags |= 8;
        }
    }
}

void Menu::StartFadeIn() {
    // The menu fade trait of this menu's tile menu, or the default fade when it has none.
    float fade = this->pMenu ? this->pMenu->GetFloat(Tile::kTileValue_MenuFade) : 0.25f;
    if (fade == 0.0f) {
        fade = this->pMenu->GetFloat(Tile::kTileValue_ExploreFade);
    }
    Interface::AddMenuFade(this, fade);
    this->xFadeState = Interface::FADING_IN;
    Interface::UpdateAllTimers();
}

// xFadeState 4 is hidden and 2 is fading out. 6002 is a menu trait with no known name.
void Menu::InstantFadeOut() {
    if (this->xFadeState == 4) {
        return;
    }
    this->pMenu->Set(Tile::kTileValue_Visible, 0.0f, true);
    Interface::RecursiveFade(this->pMenu->spModel.m_pObject, 1.0f, 0.0f);
    this->xFadeState = Interface::FADING_OUT;
    if (this->iIsModal && this->pMenu->IsTrue(6002)) {
        Interface::PopFromEnterStack(static_cast<Interface::enumMenus>(this->iIsModal), false);
        Interface::SetCurrentFocusTarget(nullptr);
    }
    Interface::ClearTimer(this);
}

// xFadeState 2 is fading out and 1 is shown.
void Menu::CancelFadeOut() {
    if (this->xFadeState != 2) {
        return;
    }
    this->pMenu->Set(Tile::kTileValue_Visible, 1.0f, true);
    Interface::RecursiveFade(this->pMenu->spModel.m_pObject, 1.0f, 0.0f);
    this->xFadeState = Interface::FADE_SHOWN;
    Interface::ClearTimer(this);
}

void Menu::PrepForVisibility(bool abStayHidden) {
    this->pMenu->Set(Tile::kTileValue_Visible, 0.0f, true);
    if (!abStayHidden) {
        this->StartFadeIn();
    }
    Interface::SetCurrentFocusTarget(nullptr);
    // The deepest menu level only matters to the caller, so the result is discarded, as the game does.
    Menu::GetMaxDepth();
}

void Menu::SetMenuTile(TileMenu* apMenu, bool abDontAddToStack) {
    this->pMenu = apMenu;
    if (!apMenu) {
        return;
    }
    // Only these stacking types put the menu on the modal stack. The trait is read again for each compare.
    if (!(apMenu->GetFloat(Tile::kTileValue_StackingType) == 102.0 ||
          apMenu->GetFloat(Tile::kTileValue_StackingType) == 6006.0 ||
          apMenu->GetFloat(Tile::kTileValue_StackingType) == 103.0 ||
          apMenu->GetFloat(Tile::kTileValue_StackingType) == 6007.0)) {
        return;
    }
    this->iIsModal = this->GetClass();
    // Calls GetClass again rather than reading iIsModal, as the game does.
    if (!abDontAddToStack) {
        Interface::AddToEnterStack(this->GetClass(), false);
    }
    if (InterfaceManager::GetOverTileMenu() != this) {
        // GetOverTileTarget is called again for the Set, as the game does.
        if (InterfaceManager::GetOverTileTarget()) {
            InterfaceManager::GetOverTileTarget()->Set(Tile::kTileValue_Mouseover, 0.0f, true);
        }
        InterfaceManager::SetOverTileTarget(nullptr);
        InterfaceManager::SetOverTileMenu(nullptr);
    }
}

void Menu::AddTemplate(Tile::TileTemplate* apTemplate) {
    if (apTemplate) {
        this->xTemplates.AddHead(apTemplate);
    }
}

// When no template matches, menuTemplate keeps the last list item it looked at, and that template is built, as the game
// does.
Tile* Menu::RenderTemplate(Tile* apParent, const char* axName, Tile* apLast) {
    Tile* tile = nullptr;
    if (apLast) {
        this->pLastTile = apLast;
    }
    Tile::TileTemplate* menuTemplate = nullptr;
    BSSimpleList<Tile::TileTemplate*>* node = &this->xTemplates;
    NiGlobalStringTable::GlobalStringHandle name =
        axName ? NiGlobalStringTable::AddString(axName) : NiGlobalStringTable::NULL_STRING;
    for (; node; node = node->m_pkNext) {
        menuTemplate = node->m_item;
        if (!menuTemplate) {
            continue;
        }
        const char* templateName = menuTemplate->xName.GetHandle();
        bool equal;
        if (name == templateName) {
            equal = true;
        }
        else if (!templateName || !name) {
            equal = false;
        }
        else {
            equal = std::strcmp(name, templateName) == 0;
        }
        if (equal) {
            break;
        }
    }
    if (menuTemplate) {
        // Build under a parse result of its own, then put the previous one back.
        Tile::BuildStorage* previous = Tile::pBuild;
        void* memory = MemoryManager_Singleton.Allocate(sizeof(Tile::BuildStorage));
        Tile::pBuild = memory ? ::new (memory) Tile::BuildStorage : nullptr;
        Tile::pBuild->SetTemplate(menuTemplate);
        tile = apParent->BuildAndNameTree(menuTemplate);
        tile->ConnectTraitsToTree(menuTemplate);
        // The template belongs to the menu, so the storage must not delete it.
        Tile::pBuild->pTemplate = nullptr;
        Tile::BuildStorage* storage = Tile::pBuild;
        if (storage) {
            storage->~BuildStorage();
            MemoryManager_Singleton.Deallocate(storage);
        }
        Tile::pBuild = previous;
    }
    if (name != NiGlobalStringTable::NULL_STRING) {
        InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(name) - 8));
    }
    return tile;
}

TileMenu* Menu::GetMenuTile() {
    return pMenu;
}

Interface::FADE_STATE Menu::GetFadeState() {
    return xFadeState;
}

void Menu::SetFadeState(Interface::FADE_STATE afade) {
    xFadeState = afade;
}

void Menu::SetRenderedMenu() {
    this->bRenderedMenu = true;
}

void Menu::StartFadeOut() {
    float afSecs;
    if (pMenu->Tile::IsTrue(4003)) {
        afSecs = pMenu->Tile::GetFloat(4033);
        if (afSecs == 0.0) {
            afSecs = pMenu->Tile::GetFloat(4034);
        }
        Interface::AddMenuFade(this, afSecs);
        xFadeState = Interface::FADING_OUT;
        if ((iIsModal) && (pMenu->Tile::IsTrue(6002))) {
            Interface::PopFromEnterStack(static_cast<Interface::enumMenus>(iIsModal), false);
            Interface::SetCurrentFocusTarget(nullptr);
        }

        Interface::UpdateAllTimers();
    }
}

void Menu::CancelFadeIn() {
    if (xFadeState == 8) {
        pMenu->Set(4003, static_cast<float>(0.0), true);
        Interface::RecursiveFade(pMenu->spModel.m_pObject, 1.0, 0.0);
        xFadeState = Interface::FADE_HIDDEN;
        Interface::ClearTimer(this);
    }
}

XMLStorage* Menu::GetXML(const char* apFilename, bool abAdd) {
    const char* name = Menu::StripFileName(apFilename);
    XMLStorage* xml = nullptr;
    Menu::XMLHash.GetAt(name, xml);
    if (!xml) {
        // The archive name is the full path; only the hash key is stripped.
        xml = Menu::CompressAndStoreXML(apFilename, nullptr);
        if (abAdd) {
            Menu::XMLHash.SetAt(name, xml);
        }
    }
    return xml;
}

const char* Menu::StripFileName(const char* apFilename) {
    const char* slash = std::strrchr(apFilename, '\\');
    // Returns char* into the caller's const text.
    return const_cast<char*>(slash ? slash + 1 : apFilename);
}

Tile* Menu::GetMenuByClass(int32_t aiClass) {
    if (aiClass < 1001 || aiClass > 1084) {
        return nullptr;
    }
    const int32_t index = aiClass - 1001;
    if (static_cast<int32_t>(Menu::xMenuList.m_usSize) <= index) {
        return nullptr;
    }
    return Menu::xMenuList.m_pBase[index];
}

float Menu::GetMaxDepth() {
    float maxDepth = 0.0f;
    Tile* root = Interface::GetMenusRoot();
    NiTListItem<Tile*>* position = root->xChildren.m_pkHead;
    while (position) {
        Tile* child = root->xChildren.GetNext(position);
        if (!child || !child->GetMenu()) {
            continue;
        }
        const double thickness = child->GetMenu()->iMenuThickness;
        const float depth = static_cast<float>(child->GetFloat(Tile::kTileValue_Depth) + thickness);
        maxDepth = depth < maxDepth ? maxDepth : depth;
    }
    // The cursor goes at least 100 levels above depth 10.
    const float whole = static_cast<float>(static_cast<int32_t>(maxDepth));
    const float cursorDepth = static_cast<float>((whole < 10.0f ? 10.0f : whole) + 100.0);
    if (Interface::GetCursorTile()) {
        Interface::GetCursorTile()->Set(Tile::kTileValue_Depth, cursorDepth, true);
        Tile* cursor = Interface::GetCursorTile();
        NiPoint3 translate = cursor->spModel.m_pObject->m_kLocal.m_Translate;
        translate.y = static_cast<float>(cursorDepth * -0.00800000037997961);
        cursor->spModel.m_pObject->m_kLocal.m_Translate = translate;
    }
    return static_cast<float>(maxDepth + 2.0);
}

// Both hash entries are removed whether or not they were found. Returns the second RemoveAt's bool.
bool Menu::RemoveFromXMLArchive(const char* apFilename, Menu* apMenu) {
    const char* name = Menu::StripFileName(apFilename);
    XMLStorage* xml = nullptr;
    Menu::XMLHash.GetAt(name, xml);
    if (xml) {
        XMLStorage* storage = xml;
        if (storage) {
            delete storage;
        }
        xml = nullptr;
    }
    Menu::XMLHash.RemoveAt(name);
    Tile::BuildStorage* parsed = nullptr;
    Menu::XMLParsedHash.GetAt(name, parsed);
    if (parsed) {
        // The menu takes over the templates the parsed tree owned.
        if (apMenu && parsed->bDeleteTemplates) {
            parsed->bDeleteTemplates = false;
            apMenu->bDeleteTemplates = true;
        }
        Tile::BuildStorage* storage = parsed;
        if (storage) {
            storage->~BuildStorage();
            MemoryManager_Singleton.Deallocate(storage);
        }
        parsed = nullptr;
    }
    return Menu::XMLParsedHash.RemoveAt(name);
}

void Menu::SetParsedXML(const char* apFilename, Tile::BuildStorage* apBuild) {
    Menu::XMLParsedHash.SetAt(apFilename, apBuild);
}

// One pass over the text that remembers the last three characters kept (last is the newest). The output buffer is size
// bytes with no room reserved for the terminator or the added line breaks, deliberately, as the game does.
XMLStorage* Menu::CompressAndStoreXML(const char* apFilename, const char* apFileData) {
    char* out = nullptr;
    const char* data = nullptr;
    bool ownsData = false;
    uint32_t size = 0;
    if (!apFileData) {
        // Read the whole file from the misc archives (type 0x100) with a 0x4000-byte buffer.
        BSFile* file = static_cast<BSFile*>(FileFinder::GetFile(apFilename, NiFile::READ_ONLY, 0x4000, 0x100));
        if (!file || !file->m_bGood) {
            delete file;
            return nullptr;
        }
        size = file->GetSize();
        char* buffer = static_cast<char*>(MemoryManager_Singleton.Allocate(size));
        data = buffer;
        ownsData = true;
        uint32_t uiSize = 1;
        file->BinaryRead(buffer, size, &uiSize, 1);
        delete file;
    }
    else {
        data = apFileData;
        size = std::strlen(data);
    }
    out = static_cast<char*>(MemoryManager_Singleton.Allocate(size));
    uint32_t lineLength = 0;
    bool closingTag = false;
    bool breakLine = false;
    bool inComment = false;
    uint32_t outLength = 0;
    char last = 0;
    char last2 = 0;
    char last3 = 0;
    for (uint32_t i = 0; i < size && data[i]; ++i) {
        if (data[i] == '\t' || data[i] == '\n' || data[i] == '\r') {
            continue;
        }
        if (inComment) {
            if (last2 == '-' && last == '-' && data[i] == '>') {
                inComment = false;
            }
            last3 = last2;
            last2 = last;
            last = data[i];
            continue;
        }
        if (data[i] == ' ') {
            if (last == ' ' || last == '>') {
                continue;
            }
        }
        else if (last == ' ' && (data[i] == '<' || data[i] == '>')) {
            // A space before a tag bracket is dropped.
            --outLength;
        }
        else if (last2 == ' ' && last == '/' && data[i] == '>') {
            // " />" becomes "/>".
            --outLength;
            out[outLength - 1] = '/';
        }
        else if (last3 == '<' && last2 == '!' && last == '-' && data[i] == '-') {
            // "<!--" opens a comment; its first three characters are taken back out.
            last = 0;
            last2 = last;
            last3 = last2;
            outLength -= 3;
            inComment = true;
            continue;
        }
        // Past 100000 characters on a line, the next closing tag ends the line.
        if (lineLength > 100000) {
            if (closingTag && data[i] == '>') {
                breakLine = true;
            }
            else if (last == '<' && data[i] == '/') {
                closingTag = true;
            }
        }
        out[outLength] = data[i];
        ++outLength;
        last3 = last2;
        last2 = last;
        last = data[i];
        ++lineLength;
        if (breakLine) {
            closingTag = false;
            breakLine = false;
            out[outLength] = '\n';
            ++outLength;
            lineLength = 0;
        }
    }
    out[outLength] = '\0';
    if (ownsData) {
        MemoryManager_Singleton.Deallocate(const_cast<char*>(data));
    }
    void* memory = MemoryManager_Singleton.Allocate(sizeof(XMLStorage));
    XMLStorage* storage = nullptr;
    if (memory) {
        storage = static_cast<XMLStorage*>(memory);
        storage->iFileSize = outLength;
        storage->pXMLData = out;
    }
    return storage;
}

bool Menu::LoadFinalMasterXML() {
    bool bLoaded = false;
    BSFile* pFile = static_cast<BSFile*>(FileFinder::GetFile("DATA\\FINAL_MASTER_XML.DAT", NiFile::READ_ONLY, 0));
    if (!pFile) {
        return bLoaded;
    }
    pFile->Open(false, false);
    FinalMasterXMLHeader kHeader;
    // Calls ReadF directly.
    if (pFile->BSFile::ReadF(&kHeader, 12) == 12 && kHeader.uiVersion == 100) {
        char* pData = static_cast<char*>(MemoryManager_Singleton.Allocate(kHeader.uiSize));
        if (pFile->BSFile::ReadF(pData, kHeader.uiSize) == kHeader.uiSize) {
            bLoaded = true;
            char* pCurrent = pData;
            for (uint32_t i = 0; i < kHeader.uiCount; ++i) {
                // The packed data is raw bytes: each entry header is followed by its XML.
                FinalMasterXMLEntry* pEntry = reinterpret_cast<FinalMasterXMLEntry*>(pCurrent);
                char* pXML = reinterpret_cast<char*>(pEntry + 1);
                Tile::BuildStorage* pBuild = Tile::ParseFile(pEntry->acFileName, pXML, pEntry->uiSize);
                SetParsedXML(pEntry->acFileName, pBuild);
                pCurrent = pXML + pEntry->uiSize;
            }
            Tile::TileTemplate::ClearAll();
            MemoryManager_Singleton.Deallocate(pData);
        }
        else {
            MemoryManager_Singleton.Deallocate(pData);
        }
    }
    delete pFile;
    return bLoaded;
}

void Menu::SetMenuByClass(int32_t aiClass, Tile* apTile) {
    int32_t iindex = aiClass - 1001;
    if (aiClass - 1001 >= 0 && iindex < Menu::xMenuList.m_usMaxSize) {
        Menu::xMenuList.SetAt(static_cast<uint32_t>(iindex), apTile);
    }
}

// Menus laid out in screen pixels rather than the 1280-wide menu space: the pip-boy menu (id 0x70), the computers,
// hacking and race/sex menus (classes 0x421, 0x41F, 0x40C). Both values are truncated before the compares.
bool Menu::ShouldScaleByResolutionRatio(Menu* apMenu) {
    TileMenu* menuTile = apMenu ? apMenu->GetMenuTile() : nullptr;
    if (!menuTile) {
        return false;
    }
    const int32_t id = static_cast<int32_t>(menuTile->GetFloat(Tile::kTileValue_ID));
    const int32_t menuClass = static_cast<int32_t>(menuTile->GetFloat(Tile::kTileValue_Class));
    return id == 0x70 || menuClass == 0x421 || menuClass == 0x41F || menuClass == 0x40C;
}

void Menu::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

// Allocates and interns the "Tileptr" fixed string the tile extra data is named with.
void Menu::InitSDM() {
    NiFixedString* pStr = static_cast<NiFixedString*>(MemoryManager_Singleton.Allocate(sizeof(NiFixedString)));
    if (pStr) {
        pStr->SetHandle(NiGlobalStringTable::AddString("Tileptr"));
    }
    Menu::pTileptrStr = pStr;
}

void Menu::KillSDM() {
    NiFixedString* Block = Menu::pTileptrStr;
    if (Menu::pTileptrStr) {
        // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
        NiGlobalStringTable::ReleaseHandle(Menu::pTileptrStr->GetHandle());
        MemoryManager_Singleton.Deallocate(Block);
    }
}

void Menu::FreeXMLArchive() {
    NiTMapItem<const char*, XMLStorage*>* pos = Menu::XMLHash.GetFirstPos();
    while (pos) {
        const char* ptempkey;
        XMLStorage* ptempval;
        Menu::XMLHash.GetNext(pos, ptempkey, ptempval);
        if (ptempval) {
            delete ptempval;
        }
    }
    Menu::XMLHash.RemoveAll_0();
    NiTMapItem<const char*, Tile::BuildStorage*>* pos_2 = Menu::XMLParsedHash.GetFirstPos();
    while (pos_2) {
        const char* ptempkey_2;
        Tile::BuildStorage* ptempval_2;
        Menu::XMLParsedHash.GetNext(pos_2, ptempkey_2, ptempval_2);
        if (ptempval_2) {
            Tile::BuildStorage* Block = ptempval_2;
            ptempval_2->~BuildStorage();
            MemoryManager_Singleton.Deallocate(Block);
        }
    }
    Menu::XMLParsedHash.RemoveAll_0();
}

Tile::BuildStorage* Menu::GetParsedXML(const char* apFilename) {
    Tile::BuildStorage* val = nullptr;
    Menu::XMLParsedHash.GetAt(apFilename, val);
    return val;
}

int32_t Menu::LoadMasterXMLFile() {
    uint32_t v2;
    BSFile* pinput =
        static_cast<BSFile*>(FileFinder::GetFile("Data\\Menus\\master_menu_file.txt", NiFile::READ_ONLY, 0x4000u, 4u));
    if (!pinput) {
        return 0;
    }
    if (!pinput->m_bGood) {
        return 0;
    }
    int32_t ilength = pinput->GetSize() + 1;
    char* pstart = static_cast<char*>(MemoryManager_Singleton.Allocate(ilength));
    uint32_t icount = 0;
    v2 = 1;
    int32_t v1 = pinput->m_pfnRead(pinput, pstart, ilength, &v2, 1);
    pinput->m_uiAbsoluteCurrentPos += v1;
    ilength = v1;
    if (pinput) {
        delete pinput;
    }
    const char* pstart_2 = pstart;
    // strstr is called on a const pointer and the terminator is written through its result, so the const is cast away.
    char* pend = const_cast<char*>(std::strstr(pstart_2, "@#$%"));
    int32_t ilength_2 = 256;
    while (pend) {
        char pname[256];
        std::memset(pname, 0, ilength_2);
        const char* pnamestart = std::strstr(pstart_2, ".xml");
        const char* pnameend = pnamestart + 4;
        do {
            --pnamestart;
        }
        while (*pnamestart != 32 && *pnamestart != 45);
        ++pnamestart;
        rt::memcpy(pname, pnamestart, pnameend - pnamestart);
        *pend = 0;
        XMLStorage* pnewdata = Menu::CompressAndStoreXML(pname, pstart_2);
        const char* pfname = Menu::StripFileName(pname);
        Menu::XMLHash.SetAt(pfname, pnewdata);
        ++icount;
        pstart_2 = pend + 4;
        pend = nullptr;
        if (pstart_2 - pstart < ilength) {
            pend = const_cast<char*>(std::strstr(pstart_2, "@#$%"));
        }
    }
    MemoryManager_Singleton.Deallocate(pstart);
    return icount;
}

bool Menu::CreateFinalMasterXMLFile() {
    bool bSuccess = true;
    // The file's 12-byte header, written whole: the version (100), the file count and the total size of the entries.
    uint32_t auiHeader[3];
    auiHeader[0] = 100;
    auiHeader[2] = 0;
    auiHeader[1] = 0;
    NiTPrimitiveArray<XMLStorage*> kStorages(0, 1);
    NiTPrimitiveArray<const char*> kNames(0, 1);
    NiTMapItem<const char*, XMLStorage*>* pos = Menu::XMLHash.GetFirstPos();
    while (pos) {
        char a[260];
        const char* pxmlname;
        XMLStorage* pstorage;
        Menu::XMLHash.GetNext(pos, pxmlname, pstorage);
        Tile::InsertIncludedFiles(pstorage);
        kStorages.SetAtGrow(kStorages.m_usSize, pstorage);
        kNames.Add(pxmlname);
        BSsprintf(a, 260, "Added %s of size %i to FinalMasterXML\n", pxmlname, pstorage->iFileSize);
        InitializationFlow(a);
        auiHeader[2] += pstorage->iFileSize + 132;
        ++auiHeader[1];
    }
    BSFile* pfile;
    void* apMemory = MemoryManager_Singleton.Allocate(sizeof(BSFile));
    if (apMemory) {
        pfile = ::new (apMemory) BSFile("D:\\Data\\final_master_xml.dat", NiFile::WRITE_ONLY, 0x4000u, false);
    }
    else {
        pfile = nullptr;
    }
    uint32_t uiHeaderSizes = 1;
    uint32_t uiWritten = pfile->m_pfnWrite(pfile, auiHeader, sizeof(auiHeader), &uiHeaderSizes, 1);
    pfile->m_uiAbsoluteCurrentPos += uiWritten;
    if (uiWritten == 12) {
        for (uint32_t I = 0; I < auiHeader[1]; ++I) {
            char fullname[128];
            XMLStorage* pstorage_2 = kStorages.m_pBase[I];
            const char* pxmlname_2 = kNames.m_pBase[I];
            std::memset(fullname, 0, sizeof(fullname));
            strcpy_s(fullname, 128, pxmlname_2);
            uint32_t v9 = 1;
            uint32_t v7 = pfile->m_pfnWrite(pfile, fullname, 128, &v9, 1);
            pfile->m_uiAbsoluteCurrentPos += v7;
            if (v7 != 128) {
                bSuccess = false;
            }
            uint32_t v6 = 1;
            uint32_t v4 = pfile->m_pfnWrite(pfile, pstorage_2, 4, &v6, 1);
            pfile->m_uiAbsoluteCurrentPos += v4;
            if (v4 != 4) {
                bSuccess = false;
            }
            uint32_t iFileSize = pstorage_2->iFileSize;
            char* pXMLData = pstorage_2->pXMLData;
            uint32_t v3 = 1;
            uint32_t v2 = pfile->m_pfnWrite(pfile, pXMLData, iFileSize, &v3, 1);
            pfile->m_uiAbsoluteCurrentPos += v2;
            if (v2 != pstorage_2->iFileSize) {
                bSuccess = false;
            }
            if (!bSuccess) {
                break;
            }
        }
    }
    else {
        bSuccess = false;
    }
    if (pfile) {
        delete pfile;
    }
    return bSuccess;
}

XMLStorage::~XMLStorage() {
    if (pXMLData) {
        MemoryManager_Singleton.Deallocate(pXMLData);
    }
    pXMLData = nullptr;
}

void Tile::BuildStorage::SetTemplate(Tile::TileTemplate* apTemplate) {
    if (pTemplate) {
        Tile::TileTemplate* Block = pTemplate;
        pTemplate->~TileTemplate();
        MemoryManager_Singleton.Deallocate(Block);
    }
    pTemplate = apTemplate;
}
