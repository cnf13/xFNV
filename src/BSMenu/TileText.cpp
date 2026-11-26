#include "BSMenu/TileText.h"

#include "BSCore/BSStringT.h"
#include "BSMenu/Font.h"
#include "Fallout/Interface/Interface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"

#include <limits>

void TileText::Init(Tile* apParent, const char* apName, Tile* apReplacedTile) {
    Tile::Init(apParent, apName, apReplacedTile);
    Set(kTileValue_String, "", true);
    Set(kTileValue_Brightness, 255.0f, true);
    Set(kTileValue_Red, 255.0f, true);
    Set(kTileValue_Green, 255.0f, true);
    Set(kTileValue_Blue, 255.0f, true);
    Set(kTileValue_Alpha, 255.0f, true);
    Set(kTileValue_Visible, 1.0f, true);
    SetNeedsUpdate(CHANGE_POSITION | CHANGE_CREATE);
}

// Lays the string out in the tile's font: as hypertext through the font manager (also when it uses the new fonts), else
// as the font's text and icon shapes placed at the tile's position and depth. The font's base line is raised by the
// line gap during the layout.
NiNode* TileText::MakeNode() {
    NiNode* pParentModel = GetParentModel();
    const float fX = GetFloat(kTileValue_X);
    const float fY = -GetFloat(kTileValue_Y);
    int32_t iWrapWidth = static_cast<int32_t>(GetFloat(kTileValue_WrapWidth));
    int32_t iWrapLimit = static_cast<int32_t>(GetFloat(kTileValue_WrapLimit));
    int32_t iWrapLines = static_cast<int32_t>(GetFloat(kTileValue_WrapLines));
    const int32_t iLineGap = static_cast<int32_t>(GetFloat(kTileValue_LineGap));
    const int32_t iJustify = static_cast<int32_t>(GetFloat(kTileValue_Justify));
    const int32_t iFontTrait = static_cast<int32_t>(GetFloat(kTileValue_Font));
    const int32_t iFont = (iFontTrait > 1 ? iFontTrait : 1) - 1;
    FontManager* pManager = FontManager::pInstance;
    Font* pFont = pManager ? pManager->pFont[iFont] : nullptr;
    const int32_t iPageNum = static_cast<int32_t>(GetFloat(kTileValue_PageNum));
    const bool bIsHTML = GetFloat(kTileValue_IsHTML) > 0.0;
    const float fAlpha = GetFloat(kTileValue_Alpha);
    GetFloat(kTileValue_Brightness);
    const double dAlpha = fAlpha != 0.0 ? fAlpha / 255.0 : 0.0;
    NiColorA kColor(1.0f, 1.0f, 1.0f, static_cast<float>(dAlpha));
    if (!pFont || !pFont->pFontData) {
        return nullptr;
    }

    alignas(BSStringT<char>) uint8_t aTextStorage[sizeof(BSStringT<char>)];
    BSStringT<char>* pText = ::new (aTextStorage) BSStringT<char>(" ");
    const char* pString = GetString(kTileValue_String);
    if (pString && *pString) {
        pText->Set(pString, 0);
    }
    if (iWrapWidth < 1) {
        iWrapWidth = static_cast<int32_t>(Interface::GetScreenWidth());
    }
    if (iWrapLines < 1) {
        iWrapLines = std::numeric_limits<int32_t>::max();
    }
    const float fBaseLine = pFont->pFontData ? pFont->pFontData->fBaseLine : 0.0f;
    const int32_t iBaseLine = static_cast<int32_t>(fBaseLine);
    if (pFont->pFontData) {
        pFont->pFontData->fBaseLine = static_cast<float>(iBaseLine + iLineGap);
    }

    if (bIsHTML || FontManager::pInstance->bUseNewFonts) {
        NiNode* pNode = new NiNode(0);
        spModel = pNode;
        alignas(FontManager::TextData) uint8_t aDataStorage[sizeof(FontManager::TextData)];
        FontManager::TextData& kData = *reinterpret_cast<FontManager::TextData*>(aDataStorage);
        kData.xNewText.pString = nullptr;
        kData.xNewText.sLen = 0;
        kData.xNewText.sMaxLen = 0;
        kData.iDefaultFont = iFont;
        kData.cLineSep = '\n';
        kData.iNumPages = 0;
        kData.iNumLines = 0;
        kData.iWidth = iWrapWidth ? iWrapWidth : std::numeric_limits<int32_t>::max();
        kData.iJustification = iJustify;
        kData.xColor = kColor;
        kData.iHeight = iWrapLimit ? iWrapLimit : std::numeric_limits<int32_t>::max();
        kData.iPageNum = iPageNum;
        kData.iLines = iWrapLines;
        kData.bIsHypertext = bIsHTML;
        NiNode* pModel = spModel.m_pObject;
        alignas(BSStringT<char>) uint8_t aArgumentStorage[sizeof(BSStringT<char>)];
        BSStringT<char>* pArgument = ::new (aArgumentStorage) BSStringT<char>(*pText);
        if (!FontManager::pInstance->CreateText(*pArgument, kData, pModel)) {
            kData.xNewText.Set(nullptr, 0);
            pText->Set(nullptr, 0);
            return nullptr;
        }
        iWrapWidth = kData.iWidth;
        iWrapLimit = kData.iHeight;
        Set(kTileValue_PageCount, static_cast<float>(kData.iNumPages), true);
        bIsHyper = kData.bIsHypertext;
        kData.xNewText.Set(nullptr, 0);
    }
    else {
        NiTriShape* pTextShape = nullptr;
        NiTriShape* pIconShape = nullptr;
        NiNode* pNode = new NiNode(0);
        spModel = pNode;
        pFont
            ->CreateText(*pText, iWrapWidth, iWrapLimit, 0, iWrapLines, iJustify, '\n', kColor, pTextShape, pIconShape);
        xOffset = pTextShape->m_kLocal.m_Translate;
        const float fDepth = GetFloat(kTileValue_Depth) * -0.00800000037997961;
        pTextShape->m_kLocal.m_Translate.x = fX;
        pTextShape->m_kLocal.m_Translate.y = fDepth;
        pTextShape->m_kLocal.m_Translate.z = fY;
        spModel->AttachChild(pTextShape, true);
        if (pIconShape) {
            const float fIconDepth = GetFloat(kTileValue_Depth) * -0.00800000037997961;
            pIconShape->m_kLocal.m_Translate.x = fX;
            pIconShape->m_kLocal.m_Translate.y = fIconDepth;
            pIconShape->m_kLocal.m_Translate.z = fY;
            spModel->AttachChild(pIconShape, true);
        }
    }

    if (pFont->pFontData) {
        pFont->pFontData->fBaseLine = static_cast<float>(iBaseLine);
    }
    Set(kTileValue_Width, static_cast<float>(iWrapWidth), true);
    Set(kTileValue_Height, static_cast<float>(iWrapLimit), true);
    AddNeedsUpdate(CHANGE_POSITION | CHANGE_COLOR);
    if (IsTrue(kTileValue_Clips)) {
        AddNeedsUpdate(CHANGE_SCISSOR);
    }
    if (!pParentModel) {
        pParentModel = Interface::GetInterfaceRoot();
    }
    pParentModel->AttachChild(spModel.m_pObject, true);
    // Same as NiAVObject::SetAppCulled: a hidden tile's node is culled.
    NiNode* pModel = spModel.m_pObject;
    if (!IsTrue(kTileValue_Visible)) {
        pModel->m_uFlags = NiAVObject::NiFlags(pModel->m_uFlags | NiAVObject::APP_CULLED);
    }
    else {
        pModel->m_uFlags = NiAVObject::NiFlags(pModel->m_uFlags & ~NiAVObject::APP_CULLED);
    }
    spModel->UpdateProperties();
    Interface::SetNeedToUpdate(true);
    Extra* pExtra = new Extra(this, spModel.m_pObject);
    spModel->AddExtraData(pExtra);
    NiNode* pResult = spModel.m_pObject;
    pText->Set(nullptr, 0);
    return pResult;
}

int32_t TileText::GetType() {
    return kTileValue_Text;
}

Tile* TileText::PostParse(int32_t aiTrait, float afValue, const char* axString) {
    if (aiTrait == 0xFC4 /* kTileValue_String */) {
        const char* text = GetString(0xFC4 /* kTileValue_String */);
        if (!text || !std::strcmp(axString, text)) {
            AddNeedsUpdate(2);
            return this;
        }
    }
    else if (aiTrait == 0xFB7 /* kTileValue_Justify */ && afValue != GetFloat(0xFB7 /* kTileValue_Justify */)) {
        AddNeedsUpdate(2);
        return this;
    }
    return nullptr;
}

FontManager::TextData::~TextData() {}
