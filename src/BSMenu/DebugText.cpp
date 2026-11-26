#include "BSMenu/DebugText.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSMenu/Font.h"
#include "BSMenu/IDebugText.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/TileShaderProperty.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cmath>

SettingT<INISettingCollection> xDebugTextColor("rDebugTextColor:Menu", 0xFFFBE9FFu);
SettingT<INISettingCollection> iDebugTextFont("iDebugTextFont:Menu", 3);

NiRTTI DebugTextExtraData::ms_RTTI("DebugTextExtraData", &NiExtraData::ms_RTTI);
_RTL_CRITICAL_SECTION DebugText::cs;
// The section is a raw CRITICAL_SECTION, which C++ does not construct: this file's static object initialises it before
// the program runs and deletes it at exit.
static struct DebugTextCriticalSectionLifetime {
    DebugTextCriticalSectionLifetime() {
        InitializeCriticalSection(&DebugText::cs);
    }
    ~DebugTextCriticalSectionLifetime() {
        DeleteCriticalSection(&DebugText::cs);
    }
} s_kDebugTextCriticalSectionLifetime;

// Color starts black through NiColorA's own constructor.
DebugLine::DebugLine() : fX(0.0f), fY(0.0f), iJustify(0), fDie(-1.0f) {}

DebugLine::~DebugLine() {}

DebugText::DebugText() : iFontHeight(0) {
    for (DebugLine& rLine : pDebugTextList) {
        rLine.fX = 0.0f;
        rLine.fY = 0.0f;
        rLine.iJustify = 0;
        rLine.spNode = nullptr;
        rLine.pText.Set(nullptr, 0);
        rLine.fDie = -1.0f;
    }
    for (int32_t i = 0; i < 3; ++i) {
        iHeight[i] = 0;
        iXLocation[i] = 0;
    }
}

DebugText::~DebugText() {
    DeleteAllDebugText();
    NiTListItem<DebugText::DebugTextData*>* m_pkHead = xFloatingTextList.m_pkHead;
    DebugText::Lock();
    while (m_pkHead) {
        DebugText::DebugTextData* pitem = m_pkHead->m_element;
        m_pkHead = m_pkHead->m_pkNext;
        if (pitem->spNode->m_uiRefCount > 2) {
            pitem->spNode->m_pkParent->DetachChild(pitem->spNode.m_pObject);
            pitem->spNode = nullptr;
        }
        delete xFloatingTextList.Remove(pitem);
    }
    DebugText::Unlock();
}

void DebugText::Reset() {
    Setting::Info* pLeftOffsetSetting;
    Setting::Info* pRightOffsetSetting;
    float fBaseLine;
    Setting::Info* pFontSetting;
    // The setting's address is never null.
    pFontSetting = &iDebugTextFont.uValue;
    Font* pFont = FontManager::pInstance->pFont[pFontSetting->uint - 1];
    if (pFont->pFontData) {
        fBaseLine = pFont->pFontData->fBaseLine;
    }
    else {
        fBaseLine = 0.0f;
    }
    iFontHeight = static_cast<int32_t>(fBaseLine + 3.0);
    for (int32_t& riHeight : iHeight) {
        Setting::Info* pTopBottomSetting;
        // The setting's address is never null.
        pTopBottomSetting = &iDebugTextTopBottomOffset.uValue;
        riHeight = pTopBottomSetting->uint + iFontHeight * (Interface::GetFullHelp() ? 4 : 1);
    }
    // The setting's address is never null.
    pRightOffsetSetting = &iDebugTextLeftRightOffset.uValue;
    iXLocation[1] = 1280 - pRightOffsetSetting->uint;
    // The setting's address is never null.
    pLeftOffsetSetting = &iDebugTextLeftRightOffset.uValue;
    iXLocation[0] = pLeftOffsetSetting->i;
    iXLocation[2] = iXLocation[0] + ((iXLocation[1] - iXLocation[0]) >> 1);
}

void DebugText::SkipLine(DEBUGTEXT_LOCATION aeLocation, int32_t aiSkip) {
    iHeight[aeLocation] += aiSkip * iFontHeight;
}

// Each C-variadic Print overload below takes its arguments as the va_list and calls VAPrintX or VAPrintLine through the
// vtable. This one prints the text at aiX, left side, left-justified, no colour.
int32_t DebugText::PrintX(int32_t aiX, const char* apFormatString, ...) {
    va_list args;
    va_start(args, apFormatString);
    return VAPrintX(aiX, DTL_LEFT, DTJ_LEFT, apFormatString, args, nullptr);
}

// The text at aiX, no colour.
int32_t DebugText::PrintX(
    int32_t aiX,
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify,
    const char* apFormatString,
    ...
) {
    va_list args;
    va_start(args, apFormatString);
    return VAPrintX(aiX, aeLocation, aeJustify, apFormatString, args, nullptr);
}

// Print's aiJustify is an int, so the enum is converted.
int32_t DebugText::VAPrintX(
    int32_t aiX,
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify,
    const char* apFormatString,
    va_list aArgs,
    const NiColorA* apColor
) {
    char buffer[200];
    _vsnprintf(buffer, 200, apFormatString, aArgs);
    return Print(
        buffer,
        static_cast<float>(aiX),
        static_cast<float>(iHeight[aeLocation]),
        static_cast<int32_t>(aeJustify),
        -1,
        -1.0f,
        0,
        apColor
    );
}

// Prints the text on the next line, with no colour.
int32_t
DebugText::PrintLine(DEBUGTEXT_LOCATION aeLocation, DEBUGTEXT_JUSTIFY aeJustify, const char* apFormatString, ...) {
    va_list args;
    va_start(args, apFormatString);
    return VAPrintLine(aeLocation, aeJustify, apFormatString, args, nullptr);
}

// Print's aiJustify is an int, so the enum is converted.
int32_t DebugText::VAPrintLine(
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify,
    const char* apFormatString,
    va_list aArgs,
    const NiColorA* apColor
) {
    char buffer[200];
    _vsnprintf(buffer, 200, apFormatString, aArgs);
    int32_t iret = Print(
        buffer,
        static_cast<float>(iXLocation[aeLocation]),
        static_cast<float>(iHeight[aeLocation]),
        static_cast<int32_t>(aeJustify),
        -1,
        -1.0f,
        0,
        apColor
    );
    iHeight[aeLocation] += iFontHeight;
    return iret;
}

// The text at aiX in aColor, left side, left-justified.
int32_t DebugText::PrintXC(int32_t aiX, const NiColorA& aColor, const char* apFormatString, ...) {
    va_list args;
    va_start(args, apFormatString);
    return VAPrintX(aiX, DTL_LEFT, DTJ_LEFT, apFormatString, args, &aColor);
}

// Prints the text at aiX in aColor.
int32_t DebugText::PrintXC(
    int32_t aiX,
    const NiColorA& aColor,
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify,
    const char* apFormatString,
    ...
) {
    va_list args;
    va_start(args, apFormatString);
    return VAPrintX(aiX, aeLocation, aeJustify, apFormatString, args, &aColor);
}

// Prints the text on the next line in aColor.
int32_t DebugText::PrintLineC(
    const NiColorA& aColor,
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify,
    const char* apFormatString,
    ...
) {
    va_list args;
    va_start(args, apFormatString);
    return VAPrintLine(aeLocation, aeJustify, apFormatString, args, &aColor);
}

int32_t DebugText::GetCurrentLine(DEBUGTEXT_LOCATION aeLocation) {
    return iHeight[aeLocation];
}

void DebugText::SetCurrentLine(DEBUGTEXT_LOCATION aeLoc, int32_t aiLine) {
    iHeight[aeLoc] = aiLine;
}

void DebugText::HandleQueuedPrints() {
    for (uint32_t i = 0; i < xPrintData.iSize; ++i) {
        DebugText::DEBUG_TEXT_PRINT_TYPE eType = xPrintData.pBuffer[i]->eType;
        if (eType) {
            if (eType == DebugText::DTPT_SPECIFIED) {
                size_t stLen;
                int32_t iListIndex;
                AutoMemContext kMemContext(MEM_CONTEXT_INTERFACE);
                if (!Interface::GetDebugTextRoot()) {
                    return;
                }
                bool btext_is_the_same = false;
                uint32_t ichars_remaining = 0;
                bool bempty_text = false;
                int32_t ifont = xPrintData.pBuffer[i]->iFont;
                if (!ifont) {
                    Setting::Info* pFontSetting;
                    // The setting's address is never null.
                    pFontSetting = &iDebugTextFont.uValue;
                    ifont = pFontSetting->uint;
                }
                BSStringT<char> string;
                if (xPrintData.pBuffer[i]->pTextData->pText.pString &&
                    (xPrintData.pBuffer[i]->pTextData->pText.sLen != 0xFFFF
                         ? stLen = xPrintData.pBuffer[i]->pTextData->pText.sLen
                         : stLen = std::strlen(xPrintData.pBuffer[i]->pTextData->pText.pString),
                     stLen)) {
                    string.Set(xPrintData.pBuffer[i]->pTextData->pText);
                    int32_t iwidth = 1200;
                    int32_t iheight = 0;
                    if (Renderer::bWidescreen) {
                        iwidth = static_cast<int32_t>(static_cast<double>(iwidth) * 1.3333300352096558);
                    }
                    FontManager::pInstance->pFont[ifont - 1]->LineBreakText(string, iwidth, iheight, false, 10);
                }
                else {
                    bempty_text = true;
                    xPrintData.pBuffer[i]->iLineNumber = 0;
                }
                if (xPrintData.pBuffer[i]->iLineNumber > 0) {
                    uint32_t uiTextLength;
                    uint32_t a;
                    int32_t iline_count = 0;
                    if (string.sLen == 0xFFFF) {
                        uiTextLength = std::strlen(string.pString);
                    }
                    else {
                        uiTextLength = string.sLen;
                    }
                    uint32_t ilen = uiTextLength;
                    for (a = 0; a < ilen && iline_count < xPrintData.pBuffer[i]->iLineNumber - 1; ++a) {
                        if (string.pString[string.pString != nullptr ? a : 0] == 10) {
                            ++iline_count;
                        }
                    }
                    if (a == ilen) {
                        return;
                    }
                    uint32_t b = 0;
                    while (string.pString[string.pString != nullptr ? a : 0] != 10 &&
                           string.pString[string.pString != nullptr ? a : 0]) {
                        string.pString[string.pString != nullptr ? b++ : 0] = string.pString
                                                                                  [string.pString != nullptr ? a++ : 0];
                    }
                    if (string.pString[string.pString != nullptr ? a : 0]) {
                        for (ichars_remaining = b; string.pString[string.pString != nullptr ? ichars_remaining : 0];
                             ++ichars_remaining) {
                            ;
                        }
                    }
                    string.pString[string.pString != nullptr ? b : 0] = 0;
                }
                for (iListIndex = 0; iListIndex < 200 &&
                     (xPrintData.pBuffer[i]->pTextData->fX != pDebugTextList[iListIndex].fX ||
                      (xPrintData.pBuffer[i]->pTextData->fY != pDebugTextList[iListIndex].fY ||
                       pDebugTextList[iListIndex].iJustify != xPrintData.pBuffer[i]->pTextData->iJustify));
                     ++iListIndex) {
                    ;
                }
                if (iListIndex == 200) {
                    for (iListIndex = 0; iListIndex < 200 && pDebugTextList[iListIndex].iJustify; ++iListIndex) {
                        ;
                    }
                    if (iListIndex == 200) {
                        Warning("DEFAULT: Too many unique debug text items. \n");
                        return;
                    }
                }
                else {
                    if (bempty_text) {
                        btext_is_the_same = false;
                    }
                    else {
                        if (pDebugTextList[iListIndex].spNode.m_pObject &&
                            (!pDebugTextList[iListIndex].pText.StrCmp(static_cast<const char*>(string.pString), true) &&
                             (xPrintData.pBuffer[i]->pTextData->Color.r == pDebugTextList[iListIndex].Color.r &&
                              (xPrintData.pBuffer[i]->pTextData->Color.g == pDebugTextList[iListIndex].Color.g &&
                               (xPrintData.pBuffer[i]->pTextData->Color.b == pDebugTextList[iListIndex].Color.b &&
                                xPrintData.pBuffer[i]->pTextData->Color.a == pDebugTextList[iListIndex].Color.a))))) {
                            btext_is_the_same = true;
                        }
                    }
                    if ((!btext_is_the_same) && pDebugTextList[iListIndex].spNode.m_pObject) {
                        DebugText::Lock();
                        NiNode* DebugTextRoot = Interface::GetDebugTextRoot();
                        DebugTextRoot->DetachChild(pDebugTextList[iListIndex].spNode.m_pObject);
                        pDebugTextList[iListIndex].spNode = static_cast<NiNode*>(nullptr);
                        DebugText::Unlock();
                    }
                }
                if (!btext_is_the_same) {
                    DebugText::Lock();
                    if (bempty_text) {
                        pDebugTextList[iListIndex].spNode = static_cast<NiNode*>(nullptr);
                    }
                    else {
                        NiNode* DebugText_ = CreateDebugText(
                            string.pString,
                            xPrintData.pBuffer[i]->pTextData->fX,
                            xPrintData.pBuffer[i]->pTextData->fY,
                            xPrintData.pBuffer[i]->pTextData->iJustify,
                            ifont,
                            &xPrintData.pBuffer[i]->pTextData->Color
                        );
                        pDebugTextList[iListIndex].spNode = DebugText_;
                    }
                    if (pDebugTextList[iListIndex].spNode.m_pObject) {
                        pDebugTextList[iListIndex].pText.Set(string);
                        pDebugTextList[iListIndex].spNode->SetName(string.pString);
                        pDebugTextList[iListIndex].fX = xPrintData.pBuffer[i]->pTextData->fX;
                        pDebugTextList[iListIndex].fY = xPrintData.pBuffer[i]->pTextData->fY;
                        pDebugTextList[iListIndex].iJustify = xPrintData.pBuffer[i]->pTextData->iJustify;
                        pDebugTextList[iListIndex].fDie = xPrintData.pBuffer[i]->fDie;
                        NiColorA* pSourceColor = &xPrintData.pBuffer[i]->pTextData->Color;
                        NiColorA* pListColor = &pDebugTextList[iListIndex].Color;
                        pListColor->r = pSourceColor->r;
                        pListColor->g = pSourceColor->g;
                        pListColor->b = pSourceColor->b;
                        pListColor->a = pSourceColor->a;
                    }
                    else {
                        pDebugTextList[iListIndex].iJustify = 0;
                        pDebugTextList[iListIndex].fX = 0.0f;
                        pDebugTextList[iListIndex].fY = 0.0f;
                        pDebugTextList[iListIndex].fDie = -1.0f;
                    }
                    DebugText::Unlock();
                }
            }
        }
        else {
            if (pDebugTextList[xPrintData.pBuffer[i]->iIndex].spNode.m_pObject) {
                DebugText::Lock();
                NiNode* pkDebugTextRoot = Interface::GetDebugTextRoot();
                pkDebugTextRoot->DetachChild(pDebugTextList[xPrintData.pBuffer[i]->iIndex].spNode.m_pObject);
                pDebugTextList[xPrintData.pBuffer[i]->iIndex].spNode = static_cast<NiNode*>(nullptr);
                pDebugTextList[xPrintData.pBuffer[i]->iIndex].iJustify = 0;
                pDebugTextList[xPrintData.pBuffer[i]->iIndex].fX = 0.0f;
                pDebugTextList[xPrintData.pBuffer[i]->iIndex].fY = 0.0f;
                pDebugTextList[xPrintData.pBuffer[i]->iIndex].pText.Set(nullptr, 0);
                pDebugTextList[xPrintData.pBuffer[i]->iIndex].fDie = -1.0f;
                DebugText::Unlock();
            }
            bool btext_is_the_same = false;
            if (pDebugTextList[xPrintData.pBuffer[i]->iIndex].spNode.m_pObject) {
                btext_is_the_same = std::strcmp(
                                        pDebugTextList[xPrintData.pBuffer[i]->iIndex].pText.pString,
                                        xPrintData.pBuffer[i]->pTextData->pText.pString
                                    ) == 0;
            }
            if (!btext_is_the_same) {
                DebugText::Lock();
                pDebugTextList[xPrintData.pBuffer[i]->iIndex].spNode = CreateDebugText(
                    xPrintData.pBuffer[i]->pTextData->pText.pString,
                    pDebugTextList[xPrintData.pBuffer[i]->iIndex].fX,
                    pDebugTextList[xPrintData.pBuffer[i]->iIndex].fY,
                    pDebugTextList[xPrintData.pBuffer[i]->iIndex].iJustify,
                    0,
                    nullptr
                );
                DebugText::Unlock();
            }
        }
    }
    for (sPrintData* pPrintData : xPrintData) {
        delete pPrintData->pTextData;
        MemoryManager::Instance().Deallocate(pPrintData);
    }
    xPrintData.Clear(true);
}

int32_t DebugText::Print(
    const char* apText,
    float afX,
    float afY,
    int32_t aiJustify,
    int32_t aiLine_num,
    float afDie,
    int32_t aiFont,
    const NiColorA* apColor
) {
    DebugText::sPrintData* pItem;
    char cFoundText = 0;
    for (uint32_t i = 0; !cFoundText && i < xPrintData.iSize; ++i) {
        if (afX == xPrintData.pBuffer[i]->pTextData->fX &&
            afY == xPrintData.pBuffer[i]->pTextData->fY &&
            xPrintData.pBuffer[i]->pTextData->iJustify == aiJustify) {
            xPrintData.pBuffer[i]->pTextData->pText.Set(apText, 0);
            xPrintData.pBuffer[i]->eType = DebugText::DTPT_SPECIFIED;
            xPrintData.pBuffer[i]->iLineNumber = aiLine_num;
            xPrintData.pBuffer[i]->fDie = afDie;
            xPrintData.pBuffer[i]->iFont = aiFont;
            if (apColor) {
                xPrintData.pBuffer[i]->pTextData->Color = *apColor;
            }
            else {
                Setting::Info* pColorSetting;
                // The setting's address is never null.
                pColorSetting = &xDebugTextColor.uValue;
                uint32_t uiColor = pColorSetting->uint;
                float fRed = static_cast<double>(static_cast<uint8_t>(uiColor >> 24)) / 255.0;
                float fGreen = static_cast<double>(static_cast<uint8_t>(uiColor >> 16)) / 255.0;
                float fBlue = static_cast<double>(static_cast<uint8_t>(uiColor >> 8)) / 255.0;
                float fAlpha = static_cast<double>(static_cast<uint8_t>(uiColor)) / 255.0;
                NiColorA* pColor = &xPrintData.pBuffer[i]->pTextData->Color;
                pColor->r = fRed;
                pColor->g = fGreen;
                pColor->b = fBlue;
                pColor->a = fAlpha;
            }
            cFoundText = 1;
        }
    }
    if (!cFoundText) {
        DebugText::DebugTextData* pDebugTextData;
        pDebugTextData = new DebugLine;
        pDebugTextData->pText.Set(apText, 0);
        pDebugTextData->fX = afX;
        pDebugTextData->fY = afY;
        pDebugTextData->iJustify = aiJustify;
        if (apColor) {
            pDebugTextData->Color = *apColor;
        }
        else {
            Setting::Info* pColorSetting;
            // The setting's address is never null.
            pColorSetting = &xDebugTextColor.uValue;
            float fRed = static_cast<double>(static_cast<uint8_t>(pColorSetting->uint >> 24)) / 255.0;
            float fGreen = static_cast<double>(static_cast<uint8_t>(pColorSetting->uint >> 16)) / 255.0;
            float fBlue = static_cast<double>(static_cast<uint8_t>(pColorSetting->uint >> 8)) / 255.0;
            float fAlpha = static_cast<double>(static_cast<uint8_t>(pColorSetting->uint)) / 255.0;
            pDebugTextData->Color.r = fRed;
            pDebugTextData->Color.g = fGreen;
            pDebugTextData->Color.b = fBlue;
            pDebugTextData->Color.a = fAlpha;
        }
        pItem = static_cast<DebugText::sPrintData*>(MemoryManager::Instance().Allocate(sizeof(DebugText::sPrintData)));
        pItem->pTextData = pDebugTextData;
        pItem->eType = DebugText::DTPT_SPECIFIED;
        pItem->iLineNumber = aiLine_num;
        pItem->fDie = afDie;
        pItem->iFont = aiFont;
        xPrintData.Add(pItem);
    }
    return 0;
}

void DebugText::DeleteAllDebugText() {
    NiNode* debugTextRoot;
    int32_t i;
    DebugText::Lock();
    for (i = 0; i < 200; ++i) {
        if (pDebugTextList[i].spNode.m_pObject) {
            debugTextRoot = Interface::GetDebugTextRoot();
            debugTextRoot->DetachChild(pDebugTextList[i].spNode.m_pObject);
            pDebugTextList[i].spNode = nullptr;
            pDebugTextList[i].iJustify = 0;
            pDebugTextList[i].fX = 0.0f;
            pDebugTextList[i].fY = 0.0f;
            pDebugTextList[i].pText.Set(nullptr, 0);
            pDebugTextList[i].fDie = (-0x1p+0f);
        }
    }
    DebugText::Unlock();
}

const char* DebugText::GetDebugText(float afX, float afY, int32_t aiJustify) {
    int32_t j;
    uint32_t i;
    for (i = 0; i < xPrintData.iSize; ++i) {
        if ((afX == xPrintData.pBuffer[i]->pTextData->fX) &&
            ((afY == xPrintData.pBuffer[i]->pTextData->fY) &&
             (xPrintData.pBuffer[i]->pTextData->iJustify == aiJustify))) {
            return xPrintData.pBuffer[i]->pTextData->pText.pString;
        }
    }
    for (j = 0; j < 200; ++j) {
        if ((afX == pDebugTextList[j].fX) &&
            ((afY == pDebugTextList[j].fY) && (pDebugTextList[j].iJustify == aiJustify))) {
            return pDebugTextList[j].pText.pString;
        }
    }
    return nullptr;
}

NiNode* DebugText::CreateDebugText(
    const char* apText,
    float afX,
    float afY,
    int32_t aiJustify,
    int32_t aiFont,
    const NiColorA* apColor
) {
    int32_t iwidth;
    AutoMemContext kMemContext(MEM_CONTEXT_INTERFACE);
    if (*apText && (Interface::IsConsoleVisible() || Interface::IsDebugTextVisible())) {
        NiNode* pnode;
        NiUpdateData UpdateData;
        float faspectMult = 1.0f;
        if (Renderer::bWidescreen) {
            faspectMult = 1.33333f;
        }
        float fNodeX = (afX - 640.0) * faspectMult;
        double dFlippedY = Interface::GetScreenHeight() - afY;
        float fNodeY = dFlippedY - Interface::GetScreenHeight() / 2.0;
        uint32_t istr_justify = 1;
        if (aiJustify == 3) {
            istr_justify = 4;
        }
        else {
            if (aiJustify == 2) {
                istr_justify = 2;
            }
        }
        iwidth = 0;
        pnode = new NiNode(0);
        NiColorA xcolor;
        if (apColor) {
            xcolor = *apColor;
        }
        else {
            Setting::Info* pColorSetting;
            // The setting's address is never null.
            pColorSetting = &xDebugTextColor.uValue;
            NiColorA kColor(
                static_cast<double>(static_cast<uint8_t>(pColorSetting->uint >> 24)) / 255.0,
                static_cast<double>(static_cast<uint8_t>(pColorSetting->uint >> 16)) / 255.0,
                static_cast<double>(static_cast<uint8_t>(pColorSetting->uint >> 8)) / 255.0,
                static_cast<double>(static_cast<uint8_t>(pColorSetting->uint)) / 255.0
            );
            xcolor = kColor;
        }
        BSStringT<char> string(apText);
        int32_t ifont = aiFont;
        if (!aiFont) {
            Setting::Info* pFontSetting;
            // The setting's address is never null.
            pFontSetting = &iDebugTextFont.uValue;
            ifont = pFontSetting->uint;
        }
        NiAVObject* ptri = FontManager::pInstance->pFont[ifont - 1]->MakeString(
            0.0f,
            0.0f,
            0.0f,
            string,
            iwidth,
            static_cast<int32_t>(istr_justify),
            xcolor,
            true,
            true
        );
        ptri->m_kLocal.m_Translate.x = 0.0f;
        ptri->m_kLocal.m_Translate.y = 0.0f;
        ptri->m_kLocal.m_Translate.z = 0.0f;
        pnode->AttachChild(ptri, 1);
        TileShaderProperty* ptsp = static_cast<TileShaderProperty*>(ptri->GetProperty(3));
        if (ptsp) {
            ptsp->TintColor = xcolor;
        }
        pnode->m_uFlags &= ~NiAVObject::APP_CULLED;
        DebugText::Lock();
        if (Interface::GetDebugTextRoot()) {
            NiNode* DebugTextRoot = Interface::GetDebugTextRoot();
            DebugTextRoot->AttachChild(pnode, 1);
        }
        NiPoint3* pTranslate = &pnode->m_kLocal.m_Translate;
        pnode->m_kLocal.m_Translate.x = fNodeX;
        pTranslate->y = -50.0f;
        pTranslate->z = fNodeY;
        pnode->UpdateProperties();
        UpdateData.fTime = 0.0f;
        UpdateData.bUpdateControllers = false;
        UpdateData.bParallelUpdate = false;
        UpdateData.bFoundParticles = false;
        UpdateData.bFoundMorphController = false;
        UpdateData.bSceneGraphChange = false;
        pnode->Update(UpdateData);
        DebugText::Unlock();
        return pnode;
    }

    return nullptr;
}

bool DebugText::GetDebugText_0(char* apBuf, float afX, float afY, int32_t aiJustify) {
    bool cFoundText = false;
    *apBuf = 0;
    for (uint32_t iprintItemCt = 0; !cFoundText && iprintItemCt < xPrintData.iSize; ++iprintItemCt) {
        if (afX == xPrintData.pBuffer[iprintItemCt]->pTextData->fX &&
            afY == xPrintData.pBuffer[iprintItemCt]->pTextData->fY &&
            xPrintData.pBuffer[iprintItemCt]->pTextData->iJustify == aiJustify) {
            if (apBuf) {
                size_t stLen;
                if (xPrintData.pBuffer[iprintItemCt]->pTextData->pText.sLen == 0xFFFF) {
                    stLen = std::strlen(xPrintData.pBuffer[iprintItemCt]->pTextData->pText.pString);
                }
                else {
                    stLen = xPrintData.pBuffer[iprintItemCt]->pTextData->pText.sLen;
                }
                if (stLen) {
                    std::strncpy(apBuf, xPrintData.pBuffer[iprintItemCt]->pTextData->pText.pString, 2046u);
                }
            }
            cFoundText = true;
        }
    }
    for (int32_t a = 0; !cFoundText && a < 200; ++a) {
        if (afX == pDebugTextList[a].fX && afY == pDebugTextList[a].fY && pDebugTextList[a].iJustify == aiJustify) {
            if (apBuf) {
                size_t stLen;
                if (pDebugTextList[a].pText.sLen == 0xFFFF) {
                    stLen = std::strlen(pDebugTextList[a].pText.pString);
                }
                else {
                    stLen = pDebugTextList[a].pText.sLen;
                }
                if (stLen) {
                    std::strncpy(apBuf, pDebugTextList[a].pText.pString, 2046u);
                }
            }
            cFoundText = true;
        }
    }
    return cFoundText;
}

int32_t DebugText::PrintByIndex(char* apString, int32_t aiIndex) {
    char cFoundText = 0;
    for (uint32_t i = 0; !cFoundText && i < xPrintData.iSize; ++i) {
        if (xPrintData.pBuffer[i]->iIndex == aiIndex) {
            xPrintData.pBuffer[i]->pTextData->pText.Set(apString, 0);
            xPrintData.pBuffer[i]->eType = DebugText::DTPT_BY_INDEX;
            cFoundText = 1;
        }
    }
    if (!cFoundText) {
        DebugText::DebugTextData* pDebugTextData;
        pDebugTextData = new DebugLine;
        pDebugTextData->pText.Set(apString, 0);
        DebugText::sPrintData* aItem = static_cast<DebugText::sPrintData*>(
            MemoryManager::Instance().Allocate(sizeof(DebugText::sPrintData))
        );
        aItem->iIndex = aiIndex;
        aItem->pTextData = pDebugTextData;
        aItem->eType = DebugText::DTPT_BY_INDEX;
        xPrintData.Add(aItem);
    }
    return aiIndex;
}

DebugText* DebugText::Instance(bool abCreate) {
    if ((!pInstance) && abCreate) {
        AutoMemContext kMemContext(MEM_CONTEXT_INTERFACE);
        pInstance = new DebugText;
    }
    return pInstance;
}

void DebugText::Idle() {
    DebugText::DebugTextData* pitem;
    if (!Interface::IsInMenuMode()) {
        DebugText* pme = DebugText::Instance(true);
        for (DebugLine& rLine : pme->pDebugTextList) {
            if (rLine.fDie > 0.0) {
                rLine.fDie = rLine.fDie - appTimer.fDelta;
                if ((rLine.fDie <= 0.0) && rLine.spNode.m_pObject) {
                    DebugText::Lock();
                    NiNode* DebugTextRoot = Interface::GetDebugTextRoot();
                    DebugTextRoot->DetachChild(rLine.spNode.m_pObject);
                    rLine.spNode = static_cast<NiNode*>(nullptr);
                    rLine.iJustify = 0;
                    rLine.fX = 0.0f;
                    rLine.fY = 0.0f;
                    rLine.pText.Set(nullptr, 0);
                    rLine.fDie = -1.0f;
                    DebugText::Unlock();
                }
            }
        }
        NiTListItem<DebugText::DebugTextData*>* xelem = pme->xFloatingTextList.m_pkHead;
        while (xelem) {
            DebugText::DebugTextData** p_m_element = &xelem->m_element;
            xelem = xelem->m_pkNext;
            pitem = *p_m_element;
            if (pitem->spNode->m_uiRefCount >= 2) {
                if (pitem->fDie > 0.0) {
                    pitem->fDie = pitem->fDie - appTimer.fDelta;
                    if (pitem->fDie < 0.0) {
                        DebugText::Lock();
                        pitem->spNode->m_pkParent->DetachChild(pitem->spNode.m_pObject);
                        pitem->spNode = static_cast<NiNode*>(nullptr);
                        DebugText::Unlock();
                        delete pme->xFloatingTextList.Remove(pitem);
                    }
                }
            }
            else {
                delete pme->xFloatingTextList.Remove(pitem);
            }
        }
    }
}

// Walks apTarget's extra data in order and returns the first floating text line of the wanted height. The line found so
// far is kept in a local and cleared when the line's height does not match, so the search goes on.
DebugTextExtraData*
DebugText::GetFloatingText(NiNode* apTarget, const char* /*apString*/, int32_t aiHeight, int32_t& aiOldCenter) {
    if (!apTarget) {
        return nullptr;
    }

    DebugTextExtraData* pResult = nullptr;
    int32_t i = 0;
    // The extra data size is reloaded on every iteration and compared unsigned.
    while (static_cast<uint32_t>(i) < apTarget->m_usExtraDataSize) {
        // Truncates the index to 16 bits before indexing (a no-op below the size).
        NiExtraData* pExtra = apTarget->m_ppkExtra[static_cast<uint16_t>(i++)];
        bool bFloatingText;
        if (pExtra) {
            // The line's own class or a class derived from it.
            const NiRTTI* pRTTI = pExtra->GetRTTI();
            while (pRTTI && pRTTI != &DebugTextExtraData::ms_RTTI) {
                pRTTI = pRTTI->m_pkBaseRTTI;
            }
            bFloatingText = pRTTI != nullptr;
        }
        else {
            bFloatingText = false;
        }
        if (bFloatingText) {
            pResult = static_cast<DebugTextExtraData*>(pExtra);
            if (pResult) {
                aiOldCenter = pResult->iOldCenter;
                if (aiHeight == pResult->iHeight) {
                    return pResult;
                }
            }
            pResult = nullptr;
        }
    }
    return pResult;
}

// Shows apText over apObject: a line that already exists for the object only gets the new die time. Otherwise the
// object's old billboard is detached, a new one is built around a shape the font makes, turned to face the camera,
// scaled by |0.5| and alpha blended, and a DebugLine is queued on the floating-text list.
bool DebugText::AddFloatingText(NiNode* apObject, const char* apText, int32_t aiHeight, float afDie) {
    if (!apObject) {
        return false;
    }
    bool bHadText = false;
    NiBound* pBound = apObject->m_kWorldBound ? apObject->m_kWorldBound : &NiAVObject::NullBoundS;
    int32_t iCenter = static_cast<int32_t>(pBound->m_kCenter.z);
    DebugTextExtraData* pExtra = GetFloatingText(apObject, apText, aiHeight, iCenter);
    NiBillboardNode* pNode;
    if (pExtra) {
        NiTListItem<DebugLine*>* pItem = Instance(true)->xFloatingTextList.m_pkHead;
        while (pItem) {
            Instance(true);
            DebugLine* pLine = pItem->m_element;
            pItem = pItem->m_pkNext;
            if (pLine->spNode.m_pObject == pExtra->pBillboard) {
                pLine->fDie = afDie;
                return true;
            }
        }
        apObject->DetachChild(pExtra->pBillboard);
        pNode = new NiBillboardNode;
        pNode->SetName("FloatingText");
        bHadText = true;
    }
    else {
        pNode = new NiBillboardNode;
        pNode->SetName("FloatingText");
        pExtra = new DebugTextExtraData(pNode, apText, aiHeight, iCenter);
    }
    int32_t iZ = pExtra->iOldCenter + aiHeight;
    int32_t iWidth = 10000;
    BSStringT<char> kString;
    kString.~BSStringT<char>();
    ::new (&kString) BSStringT<char>(apText);
    NiTriShape* pShape = static_cast<NiTriShape*>(
        FontManager::pInstance->pFont[1]->MakeString(0.0f, 0.0f, 0.0f, kString, iWidth, 2, NiColorA::WHITE, false, true)
    );
    if (!pShape) {
        kString.Set(nullptr, 0);
        return false;
    }
    pNode->AttachChild(pShape, true);
    pExtra->xString.Set(apText, 0);
    NiMatrix3 kRot;
    kRot.MakeXRotation(1.5707964f);
    pShape->m_kLocal.m_Rotate = kRot * pShape->m_kLocal.m_Rotate;
    NiPoint3 kTranslate(0.0f, 0.0f, static_cast<float>(iZ));
    pNode->m_kLocal.m_Translate = kTranslate;
    kRot.MakeXRotation(-1.5707964f);
    NiMatrix3 kRotY;
    kRotY.MakeYRotation(3.1415927f);
    pNode->m_kLocal.m_Rotate = (kRot * kRotY) * pNode->m_kLocal.m_Rotate;
    pNode->m_kLocal.m_fScale = static_cast<float>(std::fabs(0.5));
    pExtra->pBillboard = pNode;
    NiAlphaProperty* pAlpha = new NiAlphaProperty;
    pAlpha->SetBit(true, NiAlphaProperty::ALPHA_BLEND_MASK);
    pAlpha->SetBit(false, NiAlphaProperty::ALPHA_NOSORTER_MASK);
    pNode->AttachProperty(pAlpha);
    BSShaderManager::PrepareObject(pShape, 0, 0);
    if (!bHadText) {
        apObject->AddExtraData(pExtra);
    }
    apObject->AttachChild(pNode, true);
    pNode->UpdateProperties();
    NiUpdateData kData(0.0f, false, false);
    pNode->Update(kData);
    DebugLine* pLine = new DebugLine;
    pLine->fX = static_cast<float>(iZ);
    pLine->fY = static_cast<float>(iZ);
    pLine->fDie = afDie;
    pLine->pText.Set(apText, 0);
    pLine->spNode = pNode;
    Instance(true)->xFloatingTextList.AddTail(pLine);
    return true;
}

bool DebugText::AddFloatingText(NiAVObject* apObject, const char* apText, int32_t aiHeight, float afDie) {
    NiNode* pNode;
    if (apObject) {
        char cIsNode = 0;
        for (const NiRTTI* i = apObject->GetRTTI(); i; i = i->GetBaseRTTI()) {
            if (i == &NiNode::ms_RTTI) {
                cIsNode = 1;
                break;
            }
        }
        pNode = cIsNode != 0 ? static_cast<NiNode*>(apObject) : nullptr;
    }
    else {
        pNode = nullptr;
    }
    if (!pNode && apObject) {
        NiNode* pkParent = apObject->m_pkParent;
        if (pkParent) {
            char cParentIsNode = 0;
            for (const NiRTTI* j = pkParent->GetRTTI(); j; j = j->GetBaseRTTI()) {
                if (j == &NiNode::ms_RTTI) {
                    cParentIsNode = 1;
                    break;
                }
            }
            pNode = cParentIsNode != 0 ? pkParent : nullptr;
        }
        else {
            pNode = nullptr;
        }
    }
    return pNode && DebugText::AddFloatingText(pNode, apText, aiHeight, afDie);
}

void DebugText::ClearFloatingText(NiAVObject* apTarget, int32_t aiHeight) {
    NiNode* pnode;
    if (apTarget) {
        char cIsNode = 0;
        for (const NiRTTI* i = apTarget->GetRTTI(); i; i = i->GetBaseRTTI()) {
            if (i == &NiNode::ms_RTTI) {
                cIsNode = 1;
                break;
            }
        }
        pnode = cIsNode != 0 ? static_cast<NiNode*>(apTarget) : nullptr;
    }
    else {
        pnode = nullptr;
    }
    if (!pnode && apTarget) {
        NiNode* pkParent = apTarget->m_pkParent;
        if (pkParent) {
            char cParentIsNode = 0;
            for (const NiRTTI* j = pkParent->GetRTTI(); j; j = j->GetBaseRTTI()) {
                if (j == &NiNode::ms_RTTI) {
                    cParentIsNode = 1;
                    break;
                }
            }
            pnode = cParentIsNode != 0 ? pkParent : nullptr;
        }
        else {
            pnode = nullptr;
        }
    }
    if (pnode) {
        DebugText::ClearFloatingText(pnode, aiHeight);
    }
}

void DebugText::ClearFloatingText(NiNode* apTarget, int32_t aiHeight) {
    uint32_t uiExtraIndex = 0;
    while (uiExtraIndex < apTarget->m_usExtraDataSize) {
        char cIsDebugTextExtra;
        DebugTextExtraData* ptile_extra = static_cast<DebugTextExtraData*>(
            static_cast<NiObjectNET&>(static_cast<NiAVObject&>(*apTarget))
                .m_ppkExtra[static_cast<uint16_t>(uiExtraIndex++)]
        );
        if (ptile_extra) {
            char cMatchesRTTI = 0;
            for (const NiRTTI* i = ptile_extra->GetRTTI(); i; i = i->GetBaseRTTI()) {
                if (i == &DebugTextExtraData::ms_RTTI) {
                    cMatchesRTTI = 1;
                    break;
                }
            }
            cIsDebugTextExtra = cMatchesRTTI;
        }
        else {
            cIsDebugTextExtra = 0;
        }
        if (cIsDebugTextExtra && ptile_extra) {
            if (aiHeight >= 0) {
                if (aiHeight == ptile_extra->iHeight) {
                    apTarget->DetachChild(ptile_extra->pBillboard);
                    return;
                }
            }
            else {
                apTarget->DetachChild(ptile_extra->pBillboard);
            }
        }
    }
}

NiBillboardNode* DebugText::CreateBillboardText(
    const char* apText,
    float afStartX,
    float afStartY,
    float afScale,
    int32_t aiFont,
    int32_t aiFlags,
    NiColorA* apColor,
    bool abPrepareObject
) {
    NiBillboardNode* pNode = new NiBillboardNode;
    pNode->SetName(NiFixedString("FloatingText"));
    int32_t iWidth = 10000;
    BSStringT<char> kString;
    kString.~BSStringT<char>();
    ::new (&kString) BSStringT<char>(apText);
    NiTriShape* pShape = static_cast<NiTriShape*>(
        FontManager::pInstance->pFont[aiFont]
            ->MakeString(afStartX, afStartY, 0.0f, kString, iWidth, aiFlags, *apColor, false, abPrepareObject)
    );
    if (!pShape) {
        kString.Set(nullptr, 0);
        return nullptr;
    }
    NiNode* pBaseNode = pNode;
    pBaseNode->AttachChild(pShape, true);
    NiMatrix3 kRot;
    kRot.MakeXRotation(1.5707964f);
    pShape->m_kLocal.m_Rotate = kRot * pShape->m_kLocal.m_Rotate;
    NiPoint3 kTranslate(0.0f, 0.0f, 0.0f);
    pNode->m_kLocal.m_Translate = kTranslate;
    kRot.MakeXRotation(-1.5707964f);
    NiMatrix3 kRotY;
    kRotY.MakeYRotation(3.1415927f);
    pNode->m_kLocal.m_Rotate = (kRot * kRotY) * pNode->m_kLocal.m_Rotate;
    // Widens the scale to double and calls the double fabs, not the float overload.
    float fScale = static_cast<float>(std::fabs(static_cast<double>(afScale)));
    pNode->m_kLocal.m_fScale = fScale;
    NiAlphaProperty* pAlpha = new NiAlphaProperty;
    pAlpha->SetBit(true, NiAlphaProperty::ALPHA_BLEND_MASK);
    pAlpha->SetBit(false, NiAlphaProperty::ALPHA_NOSORTER_MASK);
    pNode->AttachProperty(pAlpha);
    pNode->UpdateProperties();
    NiUpdateData kData(0.0f, false, false);
    pNode->Update(kData);
    kString.Set(nullptr, 0);
    return pNode;
}

void DebugText::Lock() {
    if (Interface::IsDebugTextVisible()) {
        EnterCriticalSection(&cs);
    }
}

void DebugText::Unlock() {
    if (Interface::IsDebugTextVisible()) {
        LeaveCriticalSection(&cs);
    }
}

void DebugText::Free() {
    if (pInstance) {
        delete pInstance;
        pInstance = nullptr;
    }
}

DebugTextExtraData::DebugTextExtraData(NiNode* apOwner, const char* apText, int32_t aiHeight, int32_t aiCenter)
  : pBillboard(apOwner), iHeight(aiHeight), iOldCenter(aiCenter) {
    xString.Set(apText, 0u);
}

DebugTextExtraData::~DebugTextExtraData() {}

const NiRTTI* DebugTextExtraData::GetRTTI() const {
    return &DebugTextExtraData::ms_RTTI;
}
