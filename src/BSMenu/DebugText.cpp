#include "BSMenu/DebugText.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
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
    for (int32_t a = 0; a < 200; ++a) {
        pDebugTextList[a].fX = 0.0f;
        pDebugTextList[a].fY = 0.0f;
        pDebugTextList[a].iJustify = 0;
        pDebugTextList[a].spNode = nullptr;
        pDebugTextList[a].pText.Set(nullptr, 0);
        pDebugTextList[a].fDie = -1.0f;
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
        if (pitem->spNode.m_pObject->m_uiRefCount > 2) {
            pitem->spNode.m_pObject->m_pkParent->DetachChild(pitem->spNode.m_pObject);
            pitem->spNode = nullptr;
        }
        delete xFloatingTextList.Remove(pitem);
    }
    DebugText::Unlock();
}

void DebugText::Reset() {
    Setting::Info* v2;
    Setting::Info* v3;
    float fBaseLine;
    Setting::Info* p_uValue;
    // The setting's address is never null.
    p_uValue = &iDebugTextFont.uValue;
    Font* v8 = FontManager::pInstance->pFont[p_uValue->uint - 1];
    if (v8->pFontData) {
        Font* v1 = FontManager::pInstance->pFont[p_uValue->uint - 1];
        fBaseLine = v8->pFontData->fBaseLine;
    }
    else {
        fBaseLine = 0.0f;
    }
    iFontHeight = static_cast<int32_t>(fBaseLine + 3.0);
    for (int32_t i = 0; i < 3; ++i) {
        Setting::Info* v4;
        // The setting's address is never null.
        v4 = &iDebugTextTopBottomOffset.uValue;
        iHeight[i] = v4->uint + iFontHeight * (GetIsFullHelp() ? 4 : 1);
    }
    // The setting's address is never null.
    v3 = &iDebugTextLeftRightOffset.uValue;
    iXLocation[1] = 1280 - v3->uint;
    // The setting's address is never null.
    v2 = &iDebugTextLeftRightOffset.uValue;
    iXLocation[0] = v2->i;
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
    char a2[200];
    _vsnprintf(a2, 200, apFormatString, aArgs);
    return Print(
        a2, static_cast<float>(aiX), static_cast<float>(iHeight[aeLocation]), static_cast<int32_t>(aeJustify), -1,
        -1.0f, 0, apColor
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
    char a2[200];
    _vsnprintf(a2, 200, apFormatString, aArgs);
    int32_t iret = Print(
        a2, static_cast<float>(iXLocation[aeLocation]), static_cast<float>(iHeight[aeLocation]),
        static_cast<int32_t>(aeJustify), -1, -1.0f, 0, apColor
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
    return iHeight[(int32_t)aeLocation];
}

void DebugText::SetCurrentLine(DEBUGTEXT_LOCATION aeLoc, int32_t aiLine) {
    iHeight[(int32_t)aeLoc] = aiLine;
}

// The memory-context guard's fields are written directly here.
void DebugText::HandleQueuedPrints() {
    for (uint32_t i = 0; i < xPrintData.iSize; ++i) {
        DebugText::DEBUG_TEXT_PRINT_TYPE eType = xPrintData.pBuffer[i]->eType;
        if (eType) {
            if (eType == DebugText::DTPT_SPECIFIED) {
                size_t sLen;
                FNVD_INLINED_LOCAL(BSStringT<char>, string, std::nullopt, std::nullopt);
                FNVD_INLINED_LOCAL(AutoMemContext, tempMemContext, std::nullopt, std::nullopt);
                int32_t a_2;
                tempMemContext.iOldMemContext = GetThreadTLSData()->eMemContext;
                GetThreadTLSData()->eMemContext = MEM_CONTEXT_INTERFACE;
                if (!Interface::GetDebugTextRoot()) {
                    GetThreadTLSData()->eMemContext = tempMemContext.iOldMemContext;
                    return;
                }
                bool btext_is_the_same_2 = false;
                uint32_t ichars_remaining = 0;
                bool bempty_text = false;
                int32_t ifont = xPrintData.pBuffer[i]->iFont;
                if (!ifont) {
                    Setting::Info* p_uValue;
                    // The setting's address is never null.
                    p_uValue = &iDebugTextFont.uValue;
                    ifont = p_uValue->uint;
                }
                // MAYBE-INLINED 0xA0EC5D: a write opening BSStringT<char>'s uses: may be its inlined constructor
                string.pString = nullptr;
                // MAYBE-INLINED 0xA0EC8C: a write opening BSStringT<char>'s uses: may be its inlined constructor
                string.sLen = 0;
                // MAYBE-INLINED 0xA0ECB7: a write opening BSStringT<char>'s uses: may be its inlined constructor
                string.sMaxLen = 0;
                if (xPrintData.pBuffer[i]->pTextData->pText.pString &&
                    (xPrintData.pBuffer[i]->pTextData->pText.sLen != 0xFFFF
                         ? sLen = xPrintData.pBuffer[i]->pTextData->pText.sLen
                         : sLen = std::strlen(xPrintData.pBuffer[i]->pTextData->pText.pString),
                     sLen)) {
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
                    uint32_t v6;
                    uint32_t a;
                    int32_t iline_count = 0;
                    if (string.sLen == 0xFFFF) {
                        v6 = std::strlen(string.pString);
                    }
                    else {
                        v6 = string.sLen;
                    }
                    uint32_t ilen = v6;
                    for (a = 0; a < ilen && iline_count < xPrintData.pBuffer[i]->iLineNumber - 1; ++a) {
                        if (string.pString[string.pString != nullptr ? a : 0] == 10) {
                            ++iline_count;
                        }
                    }
                    if (a == ilen) {
                        string.Set(nullptr, 0);
                        GetThreadTLSData()->eMemContext = tempMemContext.iOldMemContext;
                        return;
                    }
                    uint32_t b = 0;
                    while (string.pString[string.pString != nullptr ? a : 0] != 10 &&
                           string.pString[string.pString != nullptr ? a : 0]) {
                        string.pString[string.pString != nullptr ? b++ : 0] =
                            string.pString[string.pString != nullptr ? a++ : 0];
                    }
                    if (string.pString[string.pString != nullptr ? a : 0]) {
                        for (ichars_remaining = b; string.pString[string.pString != nullptr ? ichars_remaining : 0];
                             ++ichars_remaining) {
                            ;
                        }
                    }
                    string.pString[string.pString != nullptr ? b : 0] = 0;
                }
                for (a_2 = 0; a_2 < 200 &&
                     (xPrintData.pBuffer[i]->pTextData->fX != pDebugTextList[a_2].fX ||
                      (xPrintData.pBuffer[i]->pTextData->fY != pDebugTextList[a_2].fY ||
                       pDebugTextList[a_2].iJustify != xPrintData.pBuffer[i]->pTextData->iJustify));
                     ++a_2) {
                    ;
                }
                if (a_2 == 200) {
                    for (a_2 = 0; a_2 < 200 && pDebugTextList[a_2].iJustify; ++a_2) {
                        ;
                    }
                    if (a_2 == 200) {
                        Warning("DEFAULT: Too many unique debug text items. \n");
                        string.Set(nullptr, 0);
                        GetThreadTLSData()->eMemContext = tempMemContext.iOldMemContext;
                        return;
                    }
                }
                else {
                    if (bempty_text) {
                        btext_is_the_same_2 = false;
                    }
                    else {
                        if (pDebugTextList[a_2].spNode.m_pObject) {
                            if (!pDebugTextList[a_2].pText.StrCmp(static_cast<const char*>(string.pString), true) &&
                                (xPrintData.pBuffer[i]->pTextData->Color.r == pDebugTextList[a_2].Color.r &&
                                 (xPrintData.pBuffer[i]->pTextData->Color.g == pDebugTextList[a_2].Color.g &&
                                  (xPrintData.pBuffer[i]->pTextData->Color.b == pDebugTextList[a_2].Color.b &&
                                   xPrintData.pBuffer[i]->pTextData->Color.a == pDebugTextList[a_2].Color.a)))) {
                                btext_is_the_same_2 = true;
                            }
                        }
                    }
                    if (!btext_is_the_same_2) {
                        if (pDebugTextList[a_2].spNode.m_pObject) {
                            DebugText::Lock();
                            NiNode* DebugTextRoot = Interface::GetDebugTextRoot();
                            DebugTextRoot->DetachChild(pDebugTextList[a_2].spNode.m_pObject);
                            pDebugTextList[a_2].spNode.operator=(static_cast<NiNode*>(nullptr));
                            DebugText::Unlock();
                        }
                    }
                }
                if (!btext_is_the_same_2) {
                    DebugText::Lock();
                    if (bempty_text) {
                        pDebugTextList[a_2].spNode.operator=(static_cast<NiNode*>(nullptr));
                    }
                    else {
                        NiNode* DebugText_ = CreateDebugText(
                            string.pString, xPrintData.pBuffer[i]->pTextData->fX, xPrintData.pBuffer[i]->pTextData->fY,
                            xPrintData.pBuffer[i]->pTextData->iJustify, ifont, &xPrintData.pBuffer[i]->pTextData->Color
                        );
                        pDebugTextList[a_2].spNode.operator=(DebugText_);
                    }
                    if (pDebugTextList[a_2].spNode.m_pObject) {
                        FNVD_INLINED_LOCAL(NiFixedString, v13, std::nullopt, std::nullopt);
                        pDebugTextList[a_2].pText.Set(string);
                        if (string.pString) {
                            v13.m_kHandle = NiGlobalStringTable::AddString(string.pString);
                        }
                        else {
                            v13.m_kHandle = NiGlobalStringTable::NULL_STRING;
                        }
                        pDebugTextList[a_2].spNode.m_pObject->SetName(v13);
                        // Releases v13's string handle, as its destructor would.
                        // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
                        NiGlobalStringTable::ReleaseHandle(v13.GetHandle());
                        pDebugTextList[a_2].fX = xPrintData.pBuffer[i]->pTextData->fX;
                        pDebugTextList[a_2].fY = xPrintData.pBuffer[i]->pTextData->fY;
                        pDebugTextList[a_2].iJustify = xPrintData.pBuffer[i]->pTextData->iJustify;
                        pDebugTextList[a_2].fDie = xPrintData.pBuffer[i]->fDie;
                        NiColorA* p_Color = &xPrintData.pBuffer[i]->pTextData->Color;
                        NiColorA* v2 = &pDebugTextList[a_2].Color;
                        v2->r = p_Color->r;
                        v2->g = p_Color->g;
                        v2->b = p_Color->b;
                        v2->a = p_Color->a;
                    }
                    else {
                        pDebugTextList[a_2].iJustify = 0;
                        pDebugTextList[a_2].fX = 0.0f;
                        pDebugTextList[a_2].fY = 0.0f;
                        pDebugTextList[a_2].fDie = -1.0f;
                    }
                    DebugText::Unlock();
                }
                string.Set(nullptr, 0);
                GetThreadTLSData()->eMemContext = tempMemContext.iOldMemContext;
            }
        }
        else {
            if (pDebugTextList[xPrintData.pBuffer[i]->iIndex].spNode.m_pObject) {
                DebugText::Lock();
                NiNode* v9 = Interface::GetDebugTextRoot();
                v9->DetachChild(pDebugTextList[xPrintData.pBuffer[i]->iIndex].spNode.m_pObject);
                pDebugTextList[xPrintData.pBuffer[i]->iIndex].spNode.operator=(static_cast<NiNode*>(nullptr));
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
                pDebugTextList[xPrintData.pBuffer[i]->iIndex].spNode.operator=(CreateDebugText(
                    xPrintData.pBuffer[i]->pTextData->pText.pString, pDebugTextList[xPrintData.pBuffer[i]->iIndex].fX,
                    pDebugTextList[xPrintData.pBuffer[i]->iIndex].fY,
                    pDebugTextList[xPrintData.pBuffer[i]->iIndex].iJustify, 0, nullptr
                ));
                DebugText::Unlock();
            }
        }
    }
    for (uint32_t iprintCt = 0; iprintCt < xPrintData.iSize; ++iprintCt) {
        DebugText::DebugTextData* Block = xPrintData.pBuffer[iprintCt]->pTextData;
        if (Block) {
            FNVD_DESTRUCT(DebugText::DebugTextData)(Block);
            MemoryManager_Singleton.Deallocate(Block);
        }
        MemoryManager_Singleton.Deallocate(xPrintData.pBuffer[iprintCt]);
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
    char v27 = 0;
    for (uint32_t i = 0; !v27 && i < xPrintData.iSize; ++i) {
        if (afX == xPrintData.pBuffer[i]->pTextData->fX && afY == xPrintData.pBuffer[i]->pTextData->fY &&
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
                Setting::Info* p_uValue;
                // The setting's address is never null.
                p_uValue = &xDebugTextColor.uValue;
                uint32_t uint = p_uValue->uint;
                float v19 = static_cast<double>(static_cast<uint8_t>(uint >> 24)) / 255.0;
                float v20 = static_cast<double>(static_cast<uint8_t>(uint >> 16)) / 255.0;
                float v21 = static_cast<double>(static_cast<uint8_t>(uint >> 8)) / 255.0;
                float v22 = static_cast<double>(static_cast<uint8_t>(uint)) / 255.0;
                NiColorA* p_Color = &xPrintData.pBuffer[i]->pTextData->Color;
                p_Color->r = v19;
                p_Color->g = v20;
                p_Color->b = v21;
                p_Color->a = v22;
            }
            v27 = 1;
        }
    }
    if (!v27) {
        DebugText::DebugTextData* v12;
        DebugLine* v24 = static_cast<DebugLine*>(MemoryManager_Singleton.Allocate(sizeof(DebugLine)));
        if (v24) {
            v12 = ::new (v24) DebugLine;
        }
        else {
            v12 = nullptr;
        }
        v12->pText.Set(apText, 0);
        v12->fX = afX;
        v12->fY = afY;
        v12->iJustify = aiJustify;
        if (apColor) {
            v12->Color = *apColor;
        }
        else {
            Setting::Info* v11;
            // The setting's address is never null.
            v11 = &xDebugTextColor.uValue;
            float v15 = static_cast<double>(static_cast<uint8_t>(v11->uint >> 24)) / 255.0;
            float v16 = static_cast<double>(static_cast<uint8_t>(v11->uint >> 16)) / 255.0;
            float v17 = static_cast<double>(static_cast<uint8_t>(v11->uint >> 8)) / 255.0;
            float v18 = static_cast<double>(static_cast<uint8_t>(v11->uint)) / 255.0;
            v12->Color.r = v15;
            v12->Color.g = v16;
            v12->Color.b = v17;
            v12->Color.a = v18;
        }
        pItem = static_cast<DebugText::sPrintData*>(MemoryManager_Singleton.Allocate(sizeof(DebugText::sPrintData)));
        pItem->pTextData = v12;
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
            (&pDebugTextList[i].spNode)->operator=(nullptr);
            pDebugTextList[i].iJustify = 0;
            pDebugTextList[i].fX = (float)0.0;
            pDebugTextList[i].fY = (float)0.0;
            (&pDebugTextList[i].pText)->Set(nullptr, (uint32_t)0);
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

// The memory-context guard's fields are written directly here.
NiNode* DebugText::CreateDebugText(
    const char* apText,
    float afX,
    float afY,
    int32_t aiJustify,
    int32_t aiFont,
    const NiColorA* apColor
) {
    int32_t iwidth;
    FNVD_INLINED_LOCAL(AutoMemContext, tempMemContext, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiColorA, xcolor_2, std::nullopt, std::nullopt);
    tempMemContext.iOldMemContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_INTERFACE;
    if (*apText && (Interface::IsConsoleVisible() || Interface::IsDebugTextVisible())) {
        NiNode* pnode;
        NiUpdateData UpdateData;
        float faspectMult = 1.0f;
        if (Renderer::bWidescreen) {
            faspectMult = 1.33333f;
        }
        float afXa = (afX - 640.0) * faspectMult;
        double v14 = Interface::GetScreenHeight() - afY;
        float afYa = v14 - Interface::GetScreenHeight() / 2.0;
        uint32_t istr_justify = 1;
        if (aiJustify == 3) {
            istr_justify = 4;
        }
        else {
            if (aiJustify == 2) {
                istr_justify = 2;
            }
        }
        NiNode* v25 = nullptr;
        iwidth = 0;
        NiNode* apMemory = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
        if (apMemory) {
            pnode = ::new (apMemory) NiNode(0);
        }
        else {
            pnode = nullptr;
        }
        // MAYBE-INLINED 0xA0FFEE: a write opening NiColorA's uses: may be its inlined constructor
        xcolor_2.r = 0.0f;
        // MAYBE-INLINED 0xA0FFF3: a write opening NiColorA's uses: may be its inlined constructor
        xcolor_2.g = 0.0f;
        // MAYBE-INLINED 0xA0FFF8: a write opening NiColorA's uses: may be its inlined constructor
        xcolor_2.b = 0.0f;
        // MAYBE-INLINED 0xA0FFFD: a write opening NiColorA's uses: may be its inlined constructor
        xcolor_2.a = 0.0f;
        if (apColor) {
            xcolor_2 = *apColor;
        }
        else {
            Setting::Info* p_uValue;
            FNVD_INLINED_LOCAL(NiColorA, v32, std::nullopt, std::nullopt);
            // The setting's address is never null.
            p_uValue = &xDebugTextColor.uValue;
            // MAYBE-INLINED 0xA100CB: a write opening NiColorA's uses: may be its inlined constructor
            v32.r = static_cast<double>(static_cast<uint8_t>(p_uValue->uint >> 24)) / 255.0;
            // MAYBE-INLINED 0xA100F0: a write opening NiColorA's uses: may be its inlined constructor
            v32.g = static_cast<double>(static_cast<uint8_t>(p_uValue->uint >> 16)) / 255.0;
            // MAYBE-INLINED 0xA10115: a write opening NiColorA's uses: may be its inlined constructor
            v32.b = static_cast<double>(static_cast<uint8_t>(p_uValue->uint >> 8)) / 255.0;
            // MAYBE-INLINED 0xA1013A: a write opening NiColorA's uses: may be its inlined constructor
            v32.a = static_cast<double>(static_cast<uint8_t>(p_uValue->uint)) / 255.0;
            xcolor_2 = v32;
        }
        BSStringT<char> string(apText);
        int32_t ifont = aiFont;
        if (!aiFont) {
            Setting::Info* v11;
            // The setting's address is never null.
            v11 = &iDebugTextFont.uValue;
            ifont = v11->uint;
        }
        NiAVObject* ptri = FontManager::pInstance->pFont[ifont - 1]->MakeString(
            0.0f, 0.0f, 0.0f, string, iwidth, static_cast<int32_t>(istr_justify), xcolor_2, true, true
        );
        ptri->m_kLocal.m_Translate.x = 0.0f;
        ptri->m_kLocal.m_Translate.y = 0.0f;
        ptri->m_kLocal.m_Translate.z = 0.0f;
        pnode->AttachChild(ptri, 1);
        TileShaderProperty* ptsp = static_cast<TileShaderProperty*>(ptri->GetProperty(3));
        if (ptsp) {
            ptsp->TintColor = xcolor_2;
        }
        pnode->m_uFlags &= ~1u;
        DebugText::Lock();
        if (Interface::GetDebugTextRoot()) {
            NiNode* DebugTextRoot = Interface::GetDebugTextRoot();
            DebugTextRoot->AttachChild(pnode, 1);
        }
        NiPoint3* p_m_Translate = &pnode->m_kLocal.m_Translate;
        pnode->m_kLocal.m_Translate.x = afXa;
        p_m_Translate->y = -50.0f;
        p_m_Translate->z = afYa;
        pnode->UpdateProperties();
        UpdateData.fTime = 0.0f;
        UpdateData.bUpdateControllers = false;
        UpdateData.bParallelUpdate = false;
        UpdateData.bFoundParticles = false;
        UpdateData.bFoundMorphController = false;
        UpdateData.bSceneGraphChange = false;
        pnode->Update(UpdateData);
        DebugText::Unlock();
        string.Set(nullptr, 0);
        GetThreadTLSData()->eMemContext = tempMemContext.iOldMemContext;
        return pnode;
    }
    else {
        GetThreadTLSData()->eMemContext = tempMemContext.iOldMemContext;
        return nullptr;
    }
}

bool DebugText::GetDebugText_0(char* apBuf, float afX, float afY, int32_t aiJustify) {
    bool bfoundText = false;
    *apBuf = 0;
    for (uint32_t iprintItemCt = 0; !bfoundText && iprintItemCt < xPrintData.iSize; ++iprintItemCt) {
        if (afX == xPrintData.pBuffer[iprintItemCt]->pTextData->fX &&
            afY == xPrintData.pBuffer[iprintItemCt]->pTextData->fY &&
            xPrintData.pBuffer[iprintItemCt]->pTextData->iJustify == aiJustify) {
            if (apBuf) {
                size_t sLen;
                if (xPrintData.pBuffer[iprintItemCt]->pTextData->pText.sLen == 0xFFFF) {
                    sLen = std::strlen(xPrintData.pBuffer[iprintItemCt]->pTextData->pText.pString);
                }
                else {
                    sLen = xPrintData.pBuffer[iprintItemCt]->pTextData->pText.sLen;
                }
                if (sLen) {
                    std::strncpy(apBuf, xPrintData.pBuffer[iprintItemCt]->pTextData->pText.pString, 2046u);
                }
            }
            bfoundText = true;
        }
    }
    for (int32_t a = 0; !bfoundText && a < 200; ++a) {
        if (afX == pDebugTextList[a].fX && afY == pDebugTextList[a].fY && pDebugTextList[a].iJustify == aiJustify) {
            if (apBuf) {
                size_t v6;
                if (pDebugTextList[a].pText.sLen == 0xFFFF) {
                    v6 = std::strlen(pDebugTextList[a].pText.pString);
                }
                else {
                    v6 = pDebugTextList[a].pText.sLen;
                }
                if (v6) {
                    std::strncpy(apBuf, pDebugTextList[a].pText.pString, 2046u);
                }
            }
            bfoundText = true;
        }
    }
    return bfoundText;
}

int32_t DebugText::PrintByIndex(char* apString, int32_t a2) {
    char v9 = 0;
    for (uint32_t i = 0; !v9 && i < xPrintData.iSize; ++i) {
        if (xPrintData.pBuffer[i]->iIndex == a2) {
            xPrintData.pBuffer[i]->pTextData->pText.Set(apString, 0);
            xPrintData.pBuffer[i]->eType = DebugText::DTPT_BY_INDEX;
            v9 = 1;
        }
    }
    if (!v9) {
        DebugText::DebugTextData* v4;
        DebugLine* v6 = static_cast<DebugLine*>(MemoryManager_Singleton.Allocate(sizeof(DebugLine)));
        if (v6) {
            v4 = ::new (v6) DebugLine;
        }
        else {
            v4 = nullptr;
        }
        v4->pText.Set(apString, 0);
        DebugText::sPrintData* aItem =
            static_cast<DebugText::sPrintData*>(MemoryManager_Singleton.Allocate(sizeof(DebugText::sPrintData)));
        aItem->iIndex = a2;
        aItem->pTextData = v4;
        aItem->eType = DebugText::DTPT_BY_INDEX;
        xPrintData.Add(aItem);
    }
    return a2;
}

DebugText* DebugText::Instance(bool abCreate) {
    DebugText* v2;
    void* v3;
    int32_t eMemContext;
    if ((!pInstance) && abCreate) {
        eMemContext = GetThreadMemContext();
        GetThreadMemContext() = 13;
        v3 = (&MemoryManager_Singleton)->Allocate(sizeof(DebugText));
        if (v3) {
            v2 = ::new (v3) DebugText;
        }
        else {
            v2 = nullptr;
        }
        pInstance = v2;
        GetThreadMemContext() = eMemContext;
    }
    return pInstance;
}

void DebugText::Idle() {
    DebugText::DebugTextData* pitem;
    if (!Interface::IsInMenuMode()) {
        DebugText* pme = DebugText::Instance(true);
        for (int32_t a = 0; a < 200; ++a) {
            if (pme->pDebugTextList[a].fDie > 0.0) {
                pme->pDebugTextList[a].fDie = pme->pDebugTextList[a].fDie - appTimer.fDelta;
                if (pme->pDebugTextList[a].fDie <= 0.0) {
                    if (pme->pDebugTextList[a].spNode.m_pObject) {
                        DebugText::Lock();
                        NiNode* DebugTextRoot = Interface::GetDebugTextRoot();
                        DebugTextRoot->DetachChild(pme->pDebugTextList[a].spNode.m_pObject);
                        pme->pDebugTextList[a].spNode.operator=(static_cast<NiNode*>(nullptr));
                        pme->pDebugTextList[a].iJustify = 0;
                        pme->pDebugTextList[a].fX = 0.0f;
                        pme->pDebugTextList[a].fY = 0.0f;
                        pme->pDebugTextList[a].pText.Set(nullptr, 0);
                        pme->pDebugTextList[a].fDie = -1.0f;
                        DebugText::Unlock();
                    }
                }
            }
        }
        NiTListItem<DebugText::DebugTextData*>* xelem = pme->xFloatingTextList.m_pkHead;
        while (xelem) {
            DebugText::DebugTextData** p_m_element = &xelem->m_element;
            xelem = xelem->m_pkNext;
            pitem = *p_m_element;
            if (pitem->spNode.m_pObject->m_uiRefCount >= 2) {
                if (pitem->fDie > 0.0) {
                    pitem->fDie = pitem->fDie - appTimer.fDelta;
                    if (pitem->fDie < 0.0) {
                        DebugText::Lock();
                        pitem->spNode.m_pObject->m_pkParent->DetachChild(pitem->spNode.m_pObject);
                        pitem->spNode.operator=(static_cast<NiNode*>(nullptr));
                        DebugText::Unlock();
                        DebugText::DebugTextData* apThis = pme->xFloatingTextList.Remove(pitem);
                        if (apThis) {
                            FNVD_DESTRUCT(DebugText::DebugTextData)(apThis);
                            MemoryManager_Singleton.Deallocate(apThis);
                        }
                    }
                }
            }
            else {
                DebugText::DebugTextData* Block = pme->xFloatingTextList.Remove(pitem);
                if (Block) {
                    FNVD_DESTRUCT(DebugText::DebugTextData)(Block);
                    MemoryManager_Singleton.Deallocate(Block);
                }
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
        void* pMemory = NiMemObject::operator new(sizeof(NiBillboardNode));
        pNode = pMemory ? ::new (pMemory) NiBillboardNode : nullptr;
        // EXE-INLINED none: the NiFixedString(const char*) constructor and destructor of the name temporary
        FNVD_INLINED_LOCAL(NiFixedString, kName, std::nullopt, std::nullopt);
        kName.SetHandle(NiGlobalStringTable::AddString("FloatingText"));
        pNode->SetName(kName);
        // EXE-INLINED 0x4381D0
        NiGlobalStringTable::ReleaseHandle(kName.GetHandle());
        bHadText = true;
    }
    else {
        void* pMemory = NiMemObject::operator new(sizeof(NiBillboardNode));
        pNode = pMemory ? ::new (pMemory) NiBillboardNode : nullptr;
        // EXE-INLINED none: the NiFixedString(const char*) constructor and destructor of the name temporary
        FNVD_INLINED_LOCAL(NiFixedString, kName, std::nullopt, std::nullopt);
        kName.SetHandle(NiGlobalStringTable::AddString("FloatingText"));
        pNode->SetName(kName);
        // Destructor of kName.
        // EXE-INLINED 0x4381D0
        NiGlobalStringTable::ReleaseHandle(kName.GetHandle());
        void* pExtraMemory = NiMemObject::operator new(sizeof(DebugTextExtraData));
        pExtra = pExtraMemory ? ::new (pExtraMemory) DebugTextExtraData(pNode, apText, aiHeight, iCenter) : nullptr;
    }
    int32_t iZ = pExtra->iOldCenter + aiHeight;
    int32_t iWidth = 10000;
    BSStringT<char> kString;
    // SWEEP-INLINE 0x40C0E0
    kString.~BSStringT<char>();
    ::new (&kString) BSStringT<char>(apText);
    NiTriShape* pShape = static_cast<NiTriShape*>(
        FontManager::pInstance->pFont[1]->MakeString(0.0f, 0.0f, 0.0f, kString, iWidth, 2, NiColorA_WHITE, false, true)
    );
    if (!pShape) {
        kString.Set(nullptr, 0);
        return false;
    }
    pNode->AttachChild(pShape, true);
    pExtra->xString.Set(apText, 0);
    float fSine;
    float fCosine;
    NiMatrix3 kRot;
    // EXE-INLINED 0x524AC0
    float fAngle = 1.5707964f;
    // EXE-INLINED 0x4169A0
    fSine = std::sin(fAngle);
    fCosine = std::cos(fAngle);
    kRot.m_pEntry[0][0] = 1.0f;
    kRot.m_pEntry[0][1] = 0.0f;
    kRot.m_pEntry[0][2] = 0.0f;
    kRot.m_pEntry[1][0] = 0.0f;
    kRot.m_pEntry[1][1] = fCosine;
    kRot.m_pEntry[1][2] = fSine;
    kRot.m_pEntry[2][0] = 0.0f;
    kRot.m_pEntry[2][1] = -fSine;
    kRot.m_pEntry[2][2] = fCosine;
    pShape->m_kLocal.m_Rotate = kRot * pShape->m_kLocal.m_Rotate;
    NiPoint3 kTranslate;
    // EXE-INLINED 0x416870
    kTranslate.x = 0.0f;
    kTranslate.y = 0.0f;
    kTranslate.z = static_cast<float>(iZ);
    pNode->m_kLocal.m_Translate = kTranslate;
    // EXE-INLINED 0x524AC0
    fAngle = -1.5707964f;
    // EXE-INLINED 0x4169A0
    fSine = std::sin(fAngle);
    fCosine = std::cos(fAngle);
    kRot.m_pEntry[0][0] = 1.0f;
    kRot.m_pEntry[0][1] = 0.0f;
    kRot.m_pEntry[0][2] = 0.0f;
    kRot.m_pEntry[1][0] = 0.0f;
    kRot.m_pEntry[1][1] = fCosine;
    kRot.m_pEntry[1][2] = fSine;
    kRot.m_pEntry[2][0] = 0.0f;
    kRot.m_pEntry[2][1] = -fSine;
    kRot.m_pEntry[2][2] = fCosine;
    NiMatrix3 kRotY;
    // EXE-INLINED 0x43F850
    fAngle = 3.1415927f;
    // EXE-INLINED 0x4169A0
    fSine = std::sin(fAngle);
    fCosine = std::cos(fAngle);
    kRotY.m_pEntry[0][0] = fCosine;
    kRotY.m_pEntry[0][1] = 0.0f;
    kRotY.m_pEntry[0][2] = -fSine;
    kRotY.m_pEntry[1][0] = 0.0f;
    kRotY.m_pEntry[1][1] = 1.0f;
    kRotY.m_pEntry[1][2] = 0.0f;
    kRotY.m_pEntry[2][0] = fSine;
    kRotY.m_pEntry[2][1] = 0.0f;
    kRotY.m_pEntry[2][2] = fCosine;
    pNode->m_kLocal.m_Rotate = (kRot * kRotY) * pNode->m_kLocal.m_Rotate;
    pNode->m_kLocal.m_fScale = static_cast<float>(std::fabs(0.5));
    pExtra->pBillboard = pNode;
    void* pAlphaMemory = NiMemObject::operator new(sizeof(NiAlphaProperty));
    NiAlphaProperty* pAlpha = pAlphaMemory ? ::new (pAlphaMemory) NiAlphaProperty : nullptr;
    pAlpha->SetBit(true, NiAlphaProperty::ALPHA_BLEND_MASK);
    pAlpha->SetBit(false, NiAlphaProperty::ALPHA_NOSORTER_MASK);
    pNode->AttachProperty(pAlpha);
    BSShaderManager::PrepareObject(pShape, 0, 0);
    if (!bHadText) {
        apObject->AddExtraData(pExtra);
    }
    apObject->AttachChild(pNode, true);
    pNode->UpdateProperties();
    NiUpdateData kData;
    kData.~NiUpdateData();
    ::new (&kData) NiUpdateData(0.0f, false, false);
    pNode->Update(kData);
    void* pLineMemory = MemoryManager_Singleton.Allocate(sizeof(DebugLine));
    DebugLine* pLine = pLineMemory ? ::new (pLineMemory) DebugLine : nullptr;
    pLine->fX = static_cast<float>(iZ);
    pLine->fY = static_cast<float>(iZ);
    pLine->fDie = afDie;
    pLine->pText.Set(apText, 0);
    pLine->spNode = pNode;
    Instance(true)->xFloatingTextList.AddTail(pLine);
    return true;
}

bool DebugText::AddFloatingText(NiAVObject* apObject, const char* apText, int32_t aiHeight, float afDie) {
    NiNode* arg0;
    if (apObject) {
        char v8;
        for (const NiRTTI* i = apObject->GetRTTI(); i; i = i->GetBaseRTTI()) {
            if (i == &NiNode::ms_RTTI) {
                v8 = 1;
                goto LABEL_9;
            }
        }
        v8 = 0;
    LABEL_9:;
        arg0 = v8 != 0 ? static_cast<NiNode*>(apObject) : nullptr;
    }
    else {
        arg0 = nullptr;
    }
    if (!arg0 && apObject) {
        NiNode* m_pkParent = apObject->m_pkParent;
        if (m_pkParent) {
            char v5;
            for (const NiRTTI* j = m_pkParent->GetRTTI(); j; j = j->GetBaseRTTI()) {
                if (j == &NiNode::ms_RTTI) {
                    v5 = 1;
                    goto LABEL_20;
                }
            }
            v5 = 0;
        LABEL_20:;
            arg0 = v5 != 0 ? m_pkParent : nullptr;
        }
        else {
            arg0 = nullptr;
        }
    }
    return arg0 && DebugText::AddFloatingText(arg0, apText, aiHeight, afDie);
}

void DebugText::ClearFloatingText(NiAVObject* apTarget, int32_t aiHeight) {
    NiNode* pnode;
    if (apTarget) {
        char v5;
        for (const NiRTTI* i = apTarget->GetRTTI(); i; i = i->GetBaseRTTI()) {
            if (i == &NiNode::ms_RTTI) {
                v5 = 1;
                goto LABEL_9;
            }
        }
        v5 = 0;
    LABEL_9:;
        pnode = v5 != 0 ? static_cast<NiNode*>(apTarget) : nullptr;
    }
    else {
        pnode = nullptr;
    }
    if (!pnode && apTarget) {
        NiNode* m_pkParent = apTarget->m_pkParent;
        if (m_pkParent) {
            char v2;
            for (const NiRTTI* j = m_pkParent->GetRTTI(); j; j = j->GetBaseRTTI()) {
                if (j == &NiNode::ms_RTTI) {
                    v2 = 1;
                    goto LABEL_20;
                }
            }
            v2 = 0;
        LABEL_20:;
            pnode = v2 != 0 ? m_pkParent : nullptr;
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
    uint32_t v6 = 0;
    while (v6 < apTarget->m_usExtraDataSize) {
        char v2;
        DebugTextExtraData* ptile_extra = static_cast<DebugTextExtraData*>(
            static_cast<NiObjectNET&>(static_cast<NiAVObject&>(*apTarget)).m_ppkExtra[static_cast<uint16_t>(v6++)]
        );
        if (ptile_extra) {
            char v3;
            for (const NiRTTI* i = ptile_extra->GetRTTI(); i; i = i->GetBaseRTTI()) {
                if (i == &DebugTextExtraData::ms_RTTI) {
                    v3 = 1;
                    goto LABEL_11;
                }
            }
            v3 = 0;
        LABEL_11:;
            v2 = v3;
        }
        else {
            v2 = 0;
        }
        if (v2 && ptile_extra) {
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
    void* pMemory = NiMemObject::operator new(sizeof(NiBillboardNode));
    NiBillboardNode* pNode = pMemory ? ::new (pMemory) NiBillboardNode : nullptr;
    // EXE-INLINED none: the NiFixedString(const char*) constructor and destructor of the name temporary
    NiFixedString kName;
    kName.SetHandle(NiGlobalStringTable::AddString("FloatingText"));
    pNode->SetName(kName);
    NiGlobalStringTable::GlobalStringHandle kHandle = kName.GetHandle();
    NiGlobalStringTable::DecHandleRefCount(kHandle);
    int32_t iWidth = 10000;
    BSStringT<char> kString;
    // SWEEP-INLINE 0x40C0E0
    kString.~BSStringT<char>();
    ::new (&kString) BSStringT<char>(apText);
    NiTriShape* pShape = static_cast<NiTriShape*>(FontManager::pInstance->pFont[aiFont]->MakeString(
        afStartX, afStartY, 0.0f, kString, iWidth, aiFlags, *apColor, false, abPrepareObject
    ));
    if (!pShape) {
        kString.Set(nullptr, 0);
        return nullptr;
    }
    NiNode* pBaseNode = pNode;
    pBaseNode->AttachChild(pShape, true);
    NiMatrix3 kRot;
    float fSine;
    float fCosine;
    // EXE-INLINED 0x524AC0
    float fAngle = 1.5707964f;
    // EXE-INLINED 0x4169A0
    fSine = std::sin(fAngle);
    fCosine = std::cos(fAngle);
    kRot.m_pEntry[0][0] = 1.0f;
    kRot.m_pEntry[0][1] = 0.0f;
    kRot.m_pEntry[0][2] = 0.0f;
    kRot.m_pEntry[1][0] = 0.0f;
    kRot.m_pEntry[1][1] = fCosine;
    kRot.m_pEntry[1][2] = fSine;
    kRot.m_pEntry[2][0] = 0.0f;
    kRot.m_pEntry[2][1] = -fSine;
    kRot.m_pEntry[2][2] = fCosine;
    pShape->m_kLocal.m_Rotate = kRot * pShape->m_kLocal.m_Rotate;
    NiPoint3 kTranslate;
    // EXE-INLINED 0x416870
    kTranslate.x = 0.0f;
    kTranslate.y = 0.0f;
    kTranslate.z = 0.0f;
    pNode->m_kLocal.m_Translate = kTranslate;
    // EXE-INLINED 0x524AC0
    fAngle = -1.5707964f;
    // EXE-INLINED 0x4169A0
    fSine = std::sin(fAngle);
    fCosine = std::cos(fAngle);
    kRot.m_pEntry[0][0] = 1.0f;
    kRot.m_pEntry[0][1] = 0.0f;
    kRot.m_pEntry[0][2] = 0.0f;
    kRot.m_pEntry[1][0] = 0.0f;
    kRot.m_pEntry[1][1] = fCosine;
    kRot.m_pEntry[1][2] = fSine;
    kRot.m_pEntry[2][0] = 0.0f;
    kRot.m_pEntry[2][1] = -fSine;
    kRot.m_pEntry[2][2] = fCosine;
    NiMatrix3 kRotY;
    // EXE-INLINED 0x43F850
    fAngle = 3.1415927f;
    // EXE-INLINED 0x4169A0
    fSine = std::sin(fAngle);
    fCosine = std::cos(fAngle);
    kRotY.m_pEntry[0][0] = fCosine;
    kRotY.m_pEntry[0][1] = 0.0f;
    kRotY.m_pEntry[0][2] = -fSine;
    kRotY.m_pEntry[1][0] = 0.0f;
    kRotY.m_pEntry[1][1] = 1.0f;
    kRotY.m_pEntry[1][2] = 0.0f;
    kRotY.m_pEntry[2][0] = fSine;
    kRotY.m_pEntry[2][1] = 0.0f;
    kRotY.m_pEntry[2][2] = fCosine;
    pNode->m_kLocal.m_Rotate = (kRot * kRotY) * pNode->m_kLocal.m_Rotate;
    // Widens the scale to double and calls the double fabs, not the float overload.
    float fScale = static_cast<float>(std::fabs(static_cast<double>(afScale)));
    pNode->m_kLocal.m_fScale = fScale;
    void* pAlphaMemory = NiMemObject::operator new(sizeof(NiAlphaProperty));
    NiAlphaProperty* pAlpha = pAlphaMemory ? ::new (pAlphaMemory) NiAlphaProperty : nullptr;
    pAlpha->SetBit(true, NiAlphaProperty::ALPHA_BLEND_MASK);
    pAlpha->SetBit(false, NiAlphaProperty::ALPHA_NOSORTER_MASK);
    pNode->AttachProperty(pAlpha);
    pNode->UpdateProperties();
    NiUpdateData kData;
    kData.~NiUpdateData();
    ::new (&kData) NiUpdateData(0.0f, false, false);
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
        DebugText* Block = pInstance;
        pInstance->~DebugText();
        MemoryManager_Singleton.Deallocate(Block);
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
