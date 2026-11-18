#include "BSMenu/Font.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSMain/BSScissorTriShape.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileImage.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/TileShaderProperty.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cmath>

SettingT<INISettingCollection>
    FontManager::sFont_1_Filename("sFontFile_1:Fonts", "Data\\Fonts\\Glow_Monofonto_Large.fnt");
SettingT<INISettingCollection> FontManager::sFont_2_Filename("sFontFile_2:Fonts", "Data\\Fonts\\Monofonto_Large.fnt");
SettingT<INISettingCollection>
    FontManager::sFont_3_Filename("sFontFile_3:Fonts", "Data\\Fonts\\Glow_Monofonto_Medium.fnt");
SettingT<INISettingCollection>
    FontManager::sFont_4_Filename("sFontFile_4:Fonts", "Data\\Fonts\\Monofonto_VeryLarge02_Dialogs2.fnt");
SettingT<INISettingCollection>
    FontManager::sFont_5_Filename("sFontFile_5:Fonts", "Data\\Fonts\\Fixedsys_Comp_uniform_width.fnt");
SettingT<INISettingCollection>
    FontManager::sFont_6_Filename("sFontFile_6:Fonts", "Data\\Fonts\\Glow_Monofonto_VL_dialogs.fnt");
SettingT<INISettingCollection>
    FontManager::sFont_7_Filename("sFontFile_7:Fonts", "Data\\Fonts\\Baked-in_Monofonto_Large.fnt");
SettingT<INISettingCollection>
    FontManager::sFont_8_Filename("sFontFile_8:Fonts", "Data\\Fonts\\Glow_Futura_Caps_Large.fnt");

Font::TextData::TextData(int32_t w, int32_t h, int32_t s, int32_t e, char c)
  : iWidth(w), iHeight(h), iLineStart(s), iLineEnd(e), iCharCount(0), cLineSep(c) {
    xNewText = "";
}

Font::TextData::~TextData() {
    xLineWidths.RemoveAll();
}

Font::Font(int32_t aiFontNum, char* apFilename, bool abLoad) : iRefCount(0), pFontFile(nullptr), pFontData(nullptr) {
    MEM_CONTEXT eMemContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_FONTS;
    if (apFilename) {
        size_t v6 = std::strlen(apFilename);
        if (v6) {
            pFontFile = static_cast<char*>(MemoryManager_Singleton.Allocate(v6 + 1));
            strcpy_s(pFontFile, v6 + 1, apFilename);
        }
        iFontNum = aiFontNum;
        if (abLoad) {
            Load();
        }
    }
    GetThreadTLSData()->eMemContext = eMemContext;
}

Font::~Font() {
    MemoryManager_Singleton.Deallocate(fontBuffer);
    fontBufferSize = 0;
    fontBuffer = nullptr;
    KillData();
    MemoryManager_Singleton.Deallocate(pFontFile);
}

float Font::GetBaseLine() {
    if (pFontData) {
        return pFontData->fBaseLine;
    }
    return 0.0f;
}

// Lays the text out through a TextData (PrepText wraps it), then writes one quad per glyph, or per button icon
// (character 1), advancing the pen along the line; a line separator starts a new line, a tab advances to the next
// multiple of 75. aiFlags 4 right-aligns and 2 centres each line on x = 0 (no enum names these values).
void Font::CreateText(
    BSStringT<char>& axTextString,
    int32_t& aiWidth,
    int32_t& aiHeight,
    int32_t aiLineStart,
    int32_t aiLineEnd,
    int32_t aiFlags,
    char aiLineBreakChar,
    const NiColorA& axFontColor,
    NiTriShape*& apTextShape,
    NiTriShape*& apIconShape
) {
    if (!aiHeight) {
        aiHeight = 0x7FFFFFFF;
    }
    if (!aiLineEnd) {
        aiLineEnd = 0x7FFFFFFF;
    }
    const float flinePadding = FontManager::pInstance->GetLinePadding(iFontNum);
    alignas(TextData) uint8_t aDataStorage[sizeof(TextData)];
    TextData& xdata = *::new (aDataStorage) TextData(aiWidth, aiHeight, aiLineStart, aiLineEnd, aiLineBreakChar);
    PrepText(axTextString.pString, xdata);
    aiWidth = xdata.GetWidth();
    aiHeight = xdata.GetHeight();

    int32_t iline_num = 0;
    int32_t start_x = 0;
    if (aiFlags == 4) {
        start_x = -xdata.GetLineWidth(iline_num);
    }
    else if (aiFlags == 2) {
        start_x = -(xdata.GetLineWidth(iline_num) / 2);
    }
    NiPoint3 xpos;
    xpos.x = static_cast<float>(start_x);
    xpos.z = (pFontData->fBaseLine - fFontHeight) * 2.0f;
    xpos.y = 0.0f;

    apTextShape = MakeTriShape(xdata.GetCharCount(), axFontColor, true);
    NiPoint3 kTranslate;
    kTranslate.x = 0.0f;
    kTranslate.y = xpos.y;
    kTranslate.z = xpos.z;
    apTextShape->m_kLocal.m_Translate = kTranslate;
    if (!ButtonIcons.IsEmpty()) {
        apIconShape = MakeIconsTriShape();
        apIconShape->m_kLocal.m_Translate = kTranslate;
        apIconShape->m_spModelData.m_pObject->SetConsistency(NiGeometryData::STATIC);
    }

    const float fstart_x = xpos.x;
    int32_t iverts = 0;
    int32_t iiconIndex = 0;
    // Each character is read as pString[pString ? a : 0], as the game does.
    for (uint32_t a = 0; xdata.xNewText.pString[xdata.xNewText.pString ? a : 0]; ++a) {
        if (xdata.xNewText.pString[xdata.xNewText.pString ? a : 0] == aiLineBreakChar) {
            ++iline_num;
            xpos.x = 0.0f;
            if (aiFlags == 4) {
                xpos.x = static_cast<float>(-xdata.GetLineWidth(iline_num));
            }
            else if (aiFlags == 2) {
                xpos.x = static_cast<float>(-(xdata.GetLineWidth(iline_num) / 2));
            }
            xpos.z = xpos.z - (pFontData->fBaseLine + flinePadding);
        }
        else if (xdata.xNewText.pString[xdata.xNewText.pString ? a : 0] == '\t') {
            // fmod's double result is stored as a float before the step (75.0 is a double constant here).
            const float fRemainder = static_cast<float>(std::fmod(static_cast<double>(xpos.x), 75.0));
            xpos.x = static_cast<float>(75.0 - fRemainder + xpos.x);
        }
        uint8_t ch = xdata.xNewText.pString[xdata.xNewText.pString ? a : 0];
        ConvertCharacter(ch);
        const int32_t ich = ch;
        if (ich > 0x100) {
            ch = 0;
        }
        if (ch == 1) {
            if (ButtonIcons.iSize > 0) {
                AddIcon(iiconIndex++, apIconShape, xpos);
            }
        }
        else {
            FontLetter* pfont_letter = &pFontData->pFontLetters[ch];
            AddChar(pfont_letter, iverts++, apTextShape, xpos, &axFontColor);
        }
        const int32_t iOldWidth = aiWidth;
        const int32_t iNewWidth = static_cast<int32_t>(xpos.x - fstart_x);
        aiWidth = iNewWidth > iOldWidth ? iNewWidth : iOldWidth;
    }

    NiPoint3* ptemp = apTextShape->m_spModelData.m_pObject->m_pkVertex;
    uint16_t usVertices = apTextShape->m_spModelData.m_pObject->m_usVertices;
    apTextShape->m_spModelData.m_pObject->m_kBound.ComputeFromData(usVertices, ptemp);
    if (apIconShape) {
        ptemp = apIconShape->m_spModelData.m_pObject->m_pkVertex;
        usVertices = apIconShape->m_spModelData.m_pObject->m_usVertices;
        apIconShape->m_spModelData.m_pObject->m_kBound.ComputeFromData(usVertices, ptemp);
    }
    ButtonIcons.Clear(true);
    xdata.~TextData();
}

// Pass 1 copies the text into pdest, replacing each "&name;" (or "&-name;", which skips the push text) found by
// FindTextReplacementString or TestConstantForGameSettings; a replacement ending in '\' names a button icon, which is
// loaded and stands in the text as character 1. Pass 2 re-reads the result and lays it out: it breaks lines at the last
// space, or at the last '~' (a soft hyphen, drawn as "-" only where the line breaks), or with a "-" inside the word
// when there is neither, and stops after iLineEnd lines. A tab advances the width to the next multiple of 75. The
// character count is counted but never stored.
void Font::PrepText(const char* apOrigString, TextData& axData) {
    if (!apOrigString) {
        return;
    }
    if (axData.iWidth <= 0) {
        axData.iWidth = 0x7FFFFFFF;
    }
    if (axData.iHeight <= 0) {
        axData.iHeight = 0x7FFFFFFF;
    }
    if (axData.iLineEnd <= 0) {
        axData.iLineEnd = 0x7FFFFFFF;
    }
    const float flinePadding = FontManager::pInstance->GetLinePadding(iFontNum);
    uint32_t iindex = 0;
    int32_t iindex_width = 0;
    int32_t iindex_width_2 = 0;
    int32_t iwidth = 0;
    int32_t imax_width = 0;
    float fheight = pFontData->pFontLetters[' '].fHeight;
    int32_t iline_count = 1;
    uint32_t isrc_len = std::strlen(apOrigString);
    const int32_t imax_lines = axData.iLineEnd;
    char* psrc = static_cast<char*>(MemoryManager_Singleton.Allocate(isrc_len + 4));
    std::memset(psrc, 0, isrc_len + 4);
    char* pdest = static_cast<char*>(MemoryManager_Singleton.Allocate(isrc_len + 4));
    std::memset(pdest, 0, isrc_len + 4);
    std::sprintf(psrc, "%s", apOrigString);
    uint32_t idest_len = 0;
    uint32_t idest_max = isrc_len + 4;
    int32_t ichar_count = 0;
    bool bsoftHyphen = false;
    char psub[1024];
    psub[0] = 0;
    bool bsubstituted = false;

    for (uint32_t a = 0; a < isrc_len; ++a) {
        if (psrc[a] == '&') {
            int32_t c = 0;
            int32_t b = 1;
            bool baddPushText = true;
            if (psrc[a + 1] == '-') {
                baddPushText = false;
                b = 2;
            }
            char ptemp[128];
            while (psrc[a + c + b] && c < 127 && psrc[a + c] != ';' && psrc[a + c] != '\n' &&
                   psrc[a + c] != axData.cLineSep) {
                ptemp[c] = psrc[a + c + b];
                ++c;
            }
            // "&-x" ending the text leaves c = 1 < b: this writes ptemp[-1] and strlen reads ptemp's uninitialised
            // bytes. Malformed input only; kept deliberately, as the game does.
            ptemp[c ? c - b : c] = 0;
            uint32_t icmd_len = std::strlen(ptemp) + 1;
            if (psrc[a + c] == ';') {
                icmd_len += b;
            }
            if (Interface::FindTextReplacementString(ptemp, psub, 0x400, baddPushText) ||
                Interface::TestConstantForGameSettings(ptemp, psub)) {
                uint32_t isub_len = std::strlen(psub);
                int32_t idelta = isub_len - icmd_len;
                // For an empty replacement, clast is 0, matching the byte the game reads before psub in this pass.
                const char clast = isub_len ? psub[isub_len - 1] : 0;
                if (clast == '\\') {
                    int32_t ifirstBackslashIndex = 0;
                    while (psub[ifirstBackslashIndex] != '\\') {
                        ++ifirstBackslashIndex;
                    }
                    char striconFilename[260];
                    striconFilename[0] = 0;
                    std::strcpy(striconFilename, &psub[ifirstBackslashIndex + 1]);
                    striconFilename[std::strlen(striconFilename) - 1] = 0;
                    // Font 7 draws its icons from the glow set.
                    if (iFontNum == 7) {
                        char strtemp[260];
                        strcpy_s(strtemp, 260, striconFilename);
                        BSsprintf(striconFilename, 260, "glow_%s", strtemp);
                    }
                    AddTextIcon(striconFilename);
                    psub[ifirstBackslashIndex] = 1;
                    psub[ifirstBackslashIndex + 1] = 0;
                    isub_len = std::strlen(psub);
                    idelta = isub_len - icmd_len;
                }
                if (idelta > 0) {
                    idest_max += idelta;
                    pdest = static_cast<char*>(MemoryManager_Singleton.Reallocate(pdest, idest_max));
                }
                for (uint32_t d = 0; d < isub_len; ++d) {
                    pdest[idest_len] = psub[d];
                    ++idest_len;
                }
                a = a + icmd_len - 1;
            }
            else {
                pdest[idest_len] = psrc[a];
                ++idest_len;
            }
            bsubstituted = true;
        }
        else {
            pdest[idest_len] = psrc[a];
            ++idest_len;
        }
    }
    pdest[idest_len] = 0;
    if (bsubstituted) {
        isrc_len = idest_len;
        psrc = static_cast<char*>(MemoryManager_Singleton.Reallocate(psrc, isrc_len + 4));
        std::strcpy(psrc, pdest);
    }

    pdest[0] = 0;
    idest_len = 0;
    int32_t iiconIndex = 0;
    for (uint32_t a = 0; a < isrc_len; ++a) {
        if (!psrc[a]) {
            break;
        }
        if (psrc[a] == axData.cLineSep) {
            pdest[idest_len] = axData.cLineSep;
            ++idest_len;
            if (idest_len >= idest_max) {
                pdest = static_cast<char*>(MemoryManager_Singleton.Reallocate(pdest, idest_len + 4));
                idest_max = idest_len + 4;
            }
            fheight = pFontData->fBaseLine + flinePadding + fheight;
            axData.xLineWidths.AddTail(iwidth);
            imax_width = imax_width > iwidth ? imax_width : iwidth;
            iwidth = 0;
            iindex = 0;
            ++iline_count;
        }
        else if (psrc[a] == '\t') {
            // Skips the line-limit check below.
            iwidth = 75 - iwidth % 75 + iwidth;
            continue;
        }
        else {
            uint8_t ch = psrc[a];
            ConvertCharacter(ch);
            FontLetter* pfont_letter = &pFontData->pFontLetters[ch];
            // A button icon takes the icon's width and spacing into the font's glyph 1.
            if (ch == 1) {
                if (static_cast<uint32_t>(iiconIndex) < ButtonIcons.iSize) {
                    pfont_letter->fWidth = ButtonIcons.pBuffer[iiconIndex].fWidth;
                    pfont_letter->fSpacing = ButtonIcons.pBuffer[iiconIndex].fSpacing;
                }
                ++iiconIndex;
            }
            iwidth += static_cast<int32_t>(pfont_letter->fWidth + pfont_letter->fSpacing);
            if (ch == ' ') {
                iindex = idest_len;
                iindex_width = iwidth - static_cast<int32_t>(pfont_letter->fWidth + pfont_letter->fSpacing);
                iindex_width_2 = iwidth;
                bsoftHyphen = false;
            }
            else if (ch == '~') {
                iindex = idest_len;
                iindex_width = iwidth;
                iindex_width_2 = iwidth;
                bsoftHyphen = true;
                iwidth = iwidth - static_cast<int32_t>(pfont_letter->fWidth + pfont_letter->fSpacing);
            }
            if (iwidth > axData.iWidth) {
                if (iindex) {
                    if (bsoftHyphen) {
                        // The '~' becomes "-\n"; the line ends after the hyphen.
                        bsoftHyphen = false;
                        idest_max += 4;
                        pdest = static_cast<char*>(MemoryManager_Singleton.Reallocate(pdest, idest_max));
                        std::memmove(pdest + iindex + 2, pdest + iindex, idest_len - iindex);
                        pdest[iindex + 1] = '\n';
                        pdest[iindex] = '-';
                        idest_len += 2;
                        ichar_count += 2;
                        pfont_letter = &pFontData->pFontLetters['-'];
                        fheight = pFontData->fBaseLine + flinePadding + fheight;
                        iwidth = iwidth - static_cast<int32_t>(pfont_letter->fWidth + pfont_letter->fSpacing);
                        axData.xLineWidths.AddTail(iwidth);
                        imax_width = imax_width > iwidth ? imax_width : iwidth;
                        iindex = 0;
                        ++iline_count;
                        // iindex is 0 here, so this reads the byte before pdest (as a signed char), as the game does.
                        pfont_letter = &pFontData->pFontLetters[static_cast<int32_t>(pdest[iindex - 1])];
                        iwidth = static_cast<int32_t>(pfont_letter->fWidth + pfont_letter->fSpacing);
                        pfont_letter = &pFontData->pFontLetters[ch];
                        iwidth += static_cast<int32_t>(pfont_letter->fWidth + pfont_letter->fSpacing);
                    }
                    else {
                        // The space becomes the line separator (or, as the current character, is replaced).
                        if (iindex == idest_len) {
                            ch = axData.cLineSep;
                        }
                        else {
                            pdest[iindex] = axData.cLineSep;
                        }
                        fheight = pFontData->fBaseLine + flinePadding + fheight;
                        axData.xLineWidths.AddTail(iindex_width);
                        imax_width = imax_width > iindex_width ? imax_width : iindex_width;
                        iindex = 0;
                        ++iline_count;
                        iwidth = iwidth - iindex_width_2;
                    }
                }
                else {
                    // No break point: the last written character moves to the next line after "-\n".
                    if (idest_len + 3 >= idest_max) {
                        pdest = static_cast<char*>(MemoryManager_Singleton.Reallocate(pdest, idest_len + 6));
                        idest_max = idest_len + 6;
                    }
                    pdest[idest_len + 2] = pdest[idest_len];
                    pdest[idest_len + 1] = pdest[idest_len - 1];
                    pdest[idest_len] = '\n';
                    pdest[idest_len - 1] = '-';
                    idest_len += 2;
                    ichar_count += 2;
                    pfont_letter = &pFontData->pFontLetters['-'];
                    fheight = pFontData->fBaseLine + flinePadding + fheight;
                    iwidth = iwidth - static_cast<int32_t>(pfont_letter->fWidth + pfont_letter->fSpacing);
                    axData.xLineWidths.AddTail(iwidth);
                    imax_width = imax_width > iwidth ? imax_width : iwidth;
                    iindex = 0;
                    ++iline_count;
                    pfont_letter = &pFontData->pFontLetters[static_cast<int32_t>(pdest[idest_len - 1])];
                    iwidth = static_cast<int32_t>(pfont_letter->fWidth + pfont_letter->fSpacing);
                    pfont_letter = &pFontData->pFontLetters[ch];
                    iwidth += static_cast<int32_t>(pfont_letter->fWidth + pfont_letter->fSpacing);
                }
            }
            if (ch != '~') {
                pdest[idest_len] = ch;
                ++idest_len;
            }
            if (idest_len >= idest_max) {
                pdest = static_cast<char*>(MemoryManager_Singleton.Reallocate(pdest, idest_len + 4));
                idest_max = idest_len + 4;
            }
        }
        // Past the line limit, the text is cut at the last line separator.
        if (imax_lines > 0 && iline_count > imax_lines && idest_len > 0) {
            while (pdest[idest_len] != axData.cLineSep) {
                --idest_len;
            }
            pdest[idest_len] = 0;
            fheight = fheight - (pFontData->fBaseLine + flinePadding);
            break;
        }
    }

    // Keeps only lines iLineStart up to iLineEnd (the compares are unsigned).
    if (pdest[0] && axData.iLineStart) {
        uint32_t bb = 0;
        uint32_t cc = 0;
        for (uint32_t aa = 0; aa < idest_len; ++aa) {
            if (cc >= static_cast<uint32_t>(axData.iLineStart) && cc < static_cast<uint32_t>(axData.iLineEnd)) {
                pdest[bb] = pdest[aa];
                ++bb;
            }
            if (pdest[aa] == '\n') {
                ++cc;
            }
        }
        pdest[bb] = 0;
        idest_len = bb;
    }
    // An empty text lays out as one space.
    if (!pdest[0]) {
        pdest[0] = ' ';
        pdest[1] = 0;
        idest_len = 1;
        iline_count = 1;
        fheight = pFontData->pFontLetters[' '].fHeight;
        iwidth = static_cast<int32_t>(pFontData->pFontLetters[' '].fWidth);
    }
    axData.xLineWidths.AddTail(iwidth);
    imax_width = imax_width > iwidth ? imax_width : iwidth;
    pdest[idest_len] = 0;
    axData.xNewText.Set(pdest, 0);
    axData.iWidth = imax_width;
    // The height is rounded, not truncated.
    axData.iHeight = NiFastFloatToInt(fheight);
    axData.iLineStart = 0;
    axData.iLineEnd = iline_count;
    axData.iCharCount = idest_len;
    MemoryManager_Singleton.Deallocate(psrc);
    MemoryManager_Singleton.Deallocate(pdest);
}

// Allocates aiChars quads (4 vertices, 2 triangles each) for a scissored shape named "Interface: Text", tinted
// axColor through a tile shader with the font's first base texture.
NiTriShape* Font::MakeTriShape(int32_t aiChars, const NiColorA& axColor, bool abPrepareObject) {
    const int32_t iverts = aiChars * 4;
    // new NiPoint3[iverts] and new NiPoint2[iverts]: the size saturates on overflow; the element constructors
    // are empty.
    uint64_t ullSize = uint64_t{sizeof(NiPoint3)} * static_cast<uint32_t>(iverts);
    NiPoint3* pvertices = static_cast<NiPoint3*>(
        MemoryManager_Singleton.Allocate(ullSize >> 32 ? SIZE_MAX : static_cast<size_t>(ullSize))
    );
    ullSize = uint64_t{sizeof(NiPoint2)} * static_cast<uint32_t>(iverts);
    NiPoint2* ptextures = static_cast<NiPoint2*>(
        MemoryManager_Singleton.Allocate(ullSize >> 32 ? SIZE_MAX : static_cast<size_t>(ullSize))
    );
    uint16_t* pvert_order = static_cast<uint16_t*>(_NiMalloc(aiChars * 6 * sizeof(uint16_t)));

    void* pMemory = NiMemObject::operator new(sizeof(BSScissorTriShape));
    NiTriShape* pshape = pMemory
        ? ::new (pMemory) BSScissorTriShape(
              static_cast<uint16_t>(iverts), pvertices, nullptr, nullptr, ptextures, 1, NiGeometryData::NBT_METHOD_NONE,
              static_cast<uint16_t>(aiChars * 2), pvert_order, 0, 0, 0, 0
          )
        : nullptr;
    NiGeometryData* pd = pshape->m_spModelData.m_pObject;
    pd->m_ucKeepFlags = 0;
    pd->m_ucCompressFlags = 1;

    NiGlobalStringTable::GlobalStringHandle kName = NiGlobalStringTable::AddString("Interface: Text");
    pshape->SetName(reinterpret_cast<const NiFixedString&>(kName));
    if (kName != NiGlobalStringTable::NULL_STRING) {
        InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kName) - 8));
    }
    pshape->m_spModelData.m_pObject->SetConsistency(NiGeometryData::STATIC);

    pMemory = NiMemObject::operator new(sizeof(TileShaderProperty));
    TileShaderProperty* ptsp = pMemory ? ::new (pMemory) TileShaderProperty(false) : nullptr;
    pshape->AttachProperty(ptsp);
    ptsp->TintColor = axColor;
    if (abPrepareObject) {
        BSShaderManager::PrepareObject(pshape, false, false);
    }
    NiTexturingProperty::Map* pMap = pTextureData[0].m_pObject->m_kMaps.GetAt(0);
    ptsp->SetTileTexture(pMap ? pMap->m_spTexture.m_pObject : nullptr);
    pshape->UpdateProperties();
    Interface::AddAlphaProperty(pshape);
    return pshape;
}

NiTriShape* Font::MakeIconsTriShape() {
    TileShaderProperty* ptsp;
    NiTriShape* pshape;
    NiPoint2* ptextures;
    NiPoint3* pvertices;
    XFNV_INLINED_LOCAL(NiFixedString, v13, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(NiFixedString, v16, std::nullopt, std::nullopt);
    char pcString[268];
    char pcDest[268];
    uint32_t iSize = ButtonIcons.iSize;
    uint32_t iverts = 4 * iSize;
    int32_t v24 = 4 * iSize;
    NiPoint3* v22 = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(
        4 * iSize <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * (4 * iSize) : SIZE_MAX
    ));
    if (v22) {
        int32_t v10 = v24;
        for (NiPoint3* i = v22; --v10 >= 0; ++i) {
            ;
        }
        pvertices = v22;
    }
    else {
        pvertices = nullptr;
    }
    NiPoint2* v19 = static_cast<NiPoint2*>(
        MemoryManager_Singleton.Allocate(iverts <= SIZE_MAX / sizeof(NiPoint2) ? sizeof(NiPoint2) * iverts : SIZE_MAX)
    );
    if (v19) {
        uint32_t v8 = iverts;
        for (NiPoint2* j = v19; (--v8 & 0x80000000u) == 0; ++j) {
            ;
        }
        ptextures = v19;
    }
    else {
        ptextures = nullptr;
    }
    uint16_t* pvert_order = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t) * ButtonIcons.iSize));
    void* apMemory = NiMemObject::operator new(sizeof(BSScissorTriShape));
    if (apMemory) {
        pshape = ::new (apMemory) BSScissorTriShape(
            iverts, pvertices, nullptr, nullptr, ptextures, 1, NiGeometryData::NBT_METHOD_NONE, 2 * ButtonIcons.iSize,
            pvert_order, 0, 0, 0, 0
        );
    }
    else {
        pshape = nullptr;
    }
    NiTriShapeData* pd = static_cast<NiTriShapeData*>(pshape->m_spModelData.m_pObject);
    pd->m_ucKeepFlags = 0;
    pd->m_ucCompressFlags = NiGeometryData::VERTEX_MASK;
    if ("Interface: TextIcons") {
        v16.m_kHandle = NiGlobalStringTable::AddString("Interface: TextIcons");
    }
    else {
        v16.m_kHandle = NiGlobalStringTable::NULL_STRING;
    }
    pshape->SetName(v16);
    // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
    NiGlobalStringTable::ReleaseHandle(v16.GetHandle());
    pshape->m_spModelData.m_pObject->SetConsistency(NiGeometryData::STATIC);
    TileShaderProperty* v14 = static_cast<TileShaderProperty*>(NiMemObject::operator new(sizeof(TileShaderProperty)));
    if (v14) {
        ptsp = ::new (v14) TileShaderProperty(false);
    }
    else {
        ptsp = nullptr;
    }
    pshape->AttachProperty(ptsp);
    ptsp->TintColor = NiColorA_WHITE;
    BSShaderManager::PrepareObject(pshape, 0, 0);
    strncpy_s(pcDest, 260u, IconAtlasTextureName.pString, 259u);
    BSsprintf(pcString, 260, "Data\\Textures\\%s", pcDest);
    // pcString is an array, so its address is never null.
    v13.m_kHandle = NiGlobalStringTable::AddString(pcString);
    NiSourceTexture* v1 = NiSourceTexture::Create(v13, NiSourceTexture::ms_kDefaultPrefs, true, false);
    ptsp->SetTileTexture(v1);
    // The destructor of v13.
    // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
    NiGlobalStringTable::ReleaseHandle(v13.GetHandle());
    pshape->UpdateProperties();
    Interface::AddAlphaProperty(pshape);
    return pshape;
}

// The quad's corners are top left, bottom left, top right, bottom right; its triangles are (0, 1, 2) and (2, 1, 3).
// A glyph with no width adds no spacing.
void Font::AddChar(
    FontLetter* apLetter,
    int32_t aiVert,
    NiTriShape* apShape,
    NiPoint3& axPos,
    const NiColorA* apColor
) {
    axPos.x = axPos.x + apLetter->fLeadingEdge;
    const int32_t iindex = aiVert * 4;
    NiPoint3* pvertices = &apShape->m_spModelData.m_pObject->m_pkVertex[iindex];
    float fX = axPos.x;
    float fY = axPos.y;
    float fZ = apLetter->fTopEdge + axPos.z;
    pvertices[0].x = fX;
    pvertices[0].y = fY;
    pvertices[0].z = fZ;
    fX = axPos.x;
    fY = axPos.y;
    fZ = apLetter->fTopEdge + axPos.z - apLetter->fHeight;
    pvertices[1].x = fX;
    pvertices[1].y = fY;
    pvertices[1].z = fZ;
    fX = apLetter->fWidth + axPos.x;
    fY = axPos.y;
    fZ = apLetter->fTopEdge + axPos.z;
    pvertices[2].x = fX;
    pvertices[2].y = fY;
    pvertices[2].z = fZ;
    fX = apLetter->fWidth + axPos.x;
    fY = axPos.y;
    fZ = apLetter->fTopEdge + axPos.z - apLetter->fHeight;
    pvertices[3].x = fX;
    pvertices[3].y = fY;
    pvertices[3].z = fZ;

    NiPoint2* ptextures = &apShape->m_spModelData.m_pObject->m_pkTexture[iindex];
    ptextures[0].x = apLetter->pMapping[0].fU;
    ptextures[0].y = apLetter->pMapping[0].fV;
    ptextures[1].x = apLetter->pMapping[2].fU;
    ptextures[1].y = apLetter->pMapping[2].fV;
    ptextures[2].x = apLetter->pMapping[1].fU;
    ptextures[2].y = apLetter->pMapping[1].fV;
    ptextures[3].x = apLetter->pMapping[3].fU;
    ptextures[3].y = apLetter->pMapping[3].fV;

    NiTriShapeData* pd = static_cast<NiTriShapeData*>(apShape->m_spModelData.m_pObject);
    uint16_t* pvert_order = &pd->GetTriList()[aiVert * 6];
    pvert_order[0] = static_cast<uint16_t>(iindex);
    pvert_order[1] = static_cast<uint16_t>(iindex + 1);
    pvert_order[2] = static_cast<uint16_t>(iindex + 2);
    pvert_order[3] = static_cast<uint16_t>(iindex + 2);
    pvert_order[4] = static_cast<uint16_t>(iindex + 1);
    pvert_order[5] = static_cast<uint16_t>(iindex + 3);

    // The width is compared against the double 0.0: a NaN width takes the 0 spacing.
    float fSpacing;
    if (apLetter->fWidth > 0.0) {
        fSpacing = apLetter->fSpacing;
    }
    else {
        fSpacing = 0.0f;
    }
    axPos.x = apLetter->fWidth + fSpacing + axPos.x;
}

void Font::AddIcon(int32_t aiIconIndex, NiTriShape* apShape, NiPoint3& aPos) {
    float m_top;
    float m_bottom;
    float y;
    float fWidth;
    fWidth = ButtonIcons.pBuffer[aiIconIndex].fWidth;
    float v18 = (&aPos)->z + ButtonIcons.pBuffer[aiIconIndex].fZOffset;
    float v21 = (&aPos)->x + ButtonIcons.pBuffer[aiIconIndex].fXOffset;
    y = (&aPos)->y;
    NiPoint3* v4 = &apShape->m_spModelData.m_pObject->m_pkVertex[4 * aiIconIndex];
    v4->x = v21;
    v4->y = y;
    v4->z = v18;
    float v15 = (&aPos)->y;
    float v16 = v18 - fWidth;
    v4[1].x = v21;
    v4[1].y = v15;
    v4[1].z = v16;
    float v13 = fWidth + v21;
    float v14 = (&aPos)->y;
    v4[2].x = v13;
    v4[2].y = v14;
    v4[2].z = v18;
    float v12 = (&aPos)->y;
    v4[3].x = v13;
    v4[3].y = v12;
    v4[3].z = v16;
    ButtonIcon* v11 = &ButtonIcons.pBuffer[aiIconIndex];
    m_top = v11->UVCoords.m_top;
    NiPoint2* v6 = &apShape->m_spModelData.m_pObject->m_pkTexture[4 * aiIconIndex];
    v6->x = v11->UVCoords.m_left;
    v6->y = m_top;
    m_bottom = ButtonIcons.pBuffer[aiIconIndex].UVCoords.m_bottom;
    v6[1].x = ButtonIcons.pBuffer[aiIconIndex].UVCoords.m_left;
    v6[1].y = m_bottom;
    float v8 = ButtonIcons.pBuffer[aiIconIndex].UVCoords.m_top;
    v6[2].x = ButtonIcons.pBuffer[aiIconIndex].UVCoords.m_right;
    v6[2].y = v8;
    float v9 = ButtonIcons.pBuffer[aiIconIndex].UVCoords.m_bottom;
    v6[3].x = ButtonIcons.pBuffer[aiIconIndex].UVCoords.m_right;
    v6[3].y = v9;
    uint16_t* v19 =
        &static_cast<NiTriShapeData*>(apShape->m_spModelData.m_pObject)->NiTriShapeData::GetTriList()[6 * aiIconIndex];
    *v19 = (uint16_t)(4 * aiIconIndex);
    v19[1] = (uint16_t)((4 * aiIconIndex) + 1);
    v19[2] = (uint16_t)((4 * aiIconIndex) + 2);
    v19[3] = (uint16_t)((4 * aiIconIndex) + 2);
    v19[4] = (uint16_t)((4 * aiIconIndex) + 1);
    v19[5] = (uint16_t)((4 * aiIconIndex) + 3);
    (&aPos)->x = ((fWidth + ButtonIcons.pBuffer[aiIconIndex].fSpacing) + (&aPos)->x);
}

void Font::AddTextIcon(const char* astrIcon) {
    char* Source;
    int32_t iletterCt;
    char apCharacter[4];
    ButtonIcon iconData;
    int32_t ilastUnderscoreIndex;
    iconData.UVCoords.m_left = 0.0f;
    iconData.UVCoords.m_right = 0.0f;
    iconData.UVCoords.m_top = 0.0f;
    iconData.UVCoords.m_bottom = 0.0f;
    // The setting's address is never null.
    Source = sDefaultTexAtlas.uValue.str;
    Tile::GetTextureAtlasInfo(Source, astrIcon, &IconAtlasTextureName, &iconData.UVCoords);
    for (ilastUnderscoreIndex = std::strlen(astrIcon) - 1; astrIcon[ilastUnderscoreIndex] != 95;
         --ilastUnderscoreIndex) {
        ;
    }
    for (iletterCt = 0; iletterCt < 3 && astrIcon[iletterCt + 1 + ilastUnderscoreIndex] != 46; ++iletterCt) {
        apCharacter[iletterCt] = astrIcon[iletterCt + 1 + ilastUnderscoreIndex];
    }
    apCharacter[iletterCt] = 0;
    SetSpacingForButtonIcon(&iconData, apCharacter);
    uint32_t v4 = ButtonIcons.AddUninitialized();
    ButtonIcons._ConstructItems(&ButtonIcons.pBuffer[v4], 1u);
    ButtonIcons.pBuffer[v4] = iconData;
}

void Font::KillData() {
    int32_t i;
    for (i = 0; pFontData && (i < pFontData->iTextureCount); ++i) {
        (&pTextureData[i])->operator=(nullptr);
    }
    (&MemoryManager_Singleton)->Deallocate(pFontData);
    pFontData = nullptr;
}

void Font::Unload() {
    if ((iRefCount) && (!(--iRefCount))) {
        KillData();
    }
}

// The first reference reads the font file (glyph data) and its texture pages (TEXTURES\FONTS\<name>.TEX: width and
// height, then RGBA32 pixels) into texturing properties, under the fonts heap context. The glyph metrics give the
// font's height and drop; the space, non-breaking space, 127 and 0 glyphs are patched. A missing file or texture, or
// more than 8 textures, warns and returns without taking the reference.
void Font::Load() {
    AutoMemContext kMemContext(MEM_CONTEXT_FONTS);
    if (iRefCount || !pFontFile) {
        ++iRefCount;
        return;
    }
    BSFile* pFile =
        static_cast<BSFile*>(FileFinder::GetFile(pFontFile, NiFile::READ_ONLY, 0x4000, ARCHIVE_TYPE_TEXTURES));
    if (!pFile || !pFile->m_bGood) {
        Warning("DEFAULT: Font file not found: %s. \n", pFontFile);
        delete pFile;
        return;
    }
    pFontData = static_cast<FontData*>(MemoryManager_Singleton.Allocate(sizeof(FontData)));
    uint32_t uiFileSize = pFile->GetSize();
    uint32_t uiComponentSize = 1;
    // EXE-INLINED 0x462D80
    pFile->BinaryRead(pFontData, uiFileSize, &uiComponentSize, 1);
    delete pFile;

    fFontHeight = 0.0f;
    float fMaxHeight = 0.0f;
    fMaxDrop = 0.0f;
    for (int32_t i = 0; i < 256; ++i) {
        float fLetterHeight = pFontData->fBaseLine - pFontData->pFontLetters[i].fTopEdge;
        fLetterHeight = fLetterHeight + pFontData->pFontLetters[i].fHeight;
        // The asm keeps the new value unless it is below the old one (an unordered compare keeps it too).
        float fLargest = fFontHeight;
        fFontHeight = fLetterHeight < fLargest ? fLargest : fLetterHeight;
        float fHeight = pFontData->pFontLetters[i].fHeight;
        fMaxHeight = fHeight < fMaxHeight ? fMaxHeight : fHeight;
        // Here the new value is kept unless it is above the old one.
        float fSmallest = fMaxDrop;
        float fDrop = pFontData->pFontLetters[i].fTopEdge - pFontData->pFontLetters[i].fHeight;
        fMaxDrop = fDrop > fSmallest ? fSmallest : fDrop;
    }

    // The space: its width and spacing swapped, the font's full height.
    float fSpaceWidth = pFontData->pFontLetters[' '].fWidth;
    pFontData->pFontLetters[' '].fWidth = pFontData->pFontLetters[' '].fSpacing;
    pFontData->pFontLetters[' '].fSpacing = fSpaceWidth;
    pFontData->pFontLetters[' '].fHeight = fMaxHeight;
    pFontData->pFontLetters[' '].fTopEdge = fMaxDrop + fMaxHeight;
    // The non-breaking space (0xA0) measures as the space.
    pFontData->pFontLetters[0xA0].fWidth = pFontData->pFontLetters[' '].fWidth;
    pFontData->pFontLetters[0xA0].fSpacing = pFontData->pFontLetters[' '].fSpacing;
    pFontData->pFontLetters[0xA0].fHeight = pFontData->pFontLetters[' '].fHeight;
    pFontData->pFontLetters[0xA0].fTopEdge = pFontData->pFontLetters[' '].fTopEdge;
    // 127 measures as '|'.
    pFontData->pFontLetters[127].fWidth = pFontData->pFontLetters['|'].fWidth;
    pFontData->pFontLetters[127].fLeadingEdge = pFontData->pFontLetters['|'].fLeadingEdge;
    pFontData->pFontLetters[127].fSpacing = pFontData->pFontLetters['|'].fSpacing;
    pFontData->pFontLetters[127].fHeight = pFontData->pFontLetters['|'].fHeight;
    pFontData->pFontLetters[127].fTopEdge = pFontData->pFontLetters['|'].fTopEdge;
    // Character 0: no width, the full height, no texture area.
    pFontData->pFontLetters[0].fWidth = 0.0f;
    pFontData->pFontLetters[0].fSpacing = 0.0f;
    pFontData->pFontLetters[0].fHeight = fMaxHeight;
    pFontData->pFontLetters[0].fTopEdge = fMaxDrop + fMaxHeight;
    pFontData->pFontLetters[0].pMapping[0].fU = 0.0f;
    pFontData->pFontLetters[0].pMapping[1].fU = 0.0f;
    pFontData->pFontLetters[0].pMapping[2].fU = 0.0f;
    pFontData->pFontLetters[0].pMapping[3].fU = 0.0f;
    pFontData->pFontLetters[0].pMapping[0].fV = 0.0f;
    pFontData->pFontLetters[0].pMapping[1].fV = 0.0f;
    pFontData->pFontLetters[0].pMapping[2].fV = 0.0f;
    pFontData->pFontLetters[0].pMapping[3].fV = 0.0f;

    if (pFontData->iTextureCount > 8) {
        Warning(
            "TEXTURES: Too many font textures for %s.\nMax textures is %d yours has %d.\n", pFontFile, 8,
            pFontData->iTextureCount
        );
        return;
    }
    for (int32_t j = 0; j < pFontData->iTextureCount; ++j) {
        char acTextureName[256];
        BSsprintf(
            acTextureName, sizeof(acTextureName), "TEXTURES\\FONTS\\%s.TEX", pFontData->pTextureFiles[j].pFilename
        );
        BSFile* pTexFile =
            static_cast<BSFile*>(FileFinder::GetFile(acTextureName, NiFile::READ_ONLY, 0x4000, ARCHIVE_TYPE_TEXTURES));
        if (!pTexFile || !pTexFile->m_bGood) {
            // The format deliberately uses %S (a wide string) for the narrow name, as the game does.
            Warning("DEFAULT: Font file not found: %S. \n", acTextureName);
            delete pTexFile;
            return;
        }
        // The texture's width and height.
        uint32_t auiSize[2];
        uiComponentSize = 1;
        // EXE-INLINED 0x462D80
        pTexFile->BinaryRead(auiSize, sizeof(auiSize), &uiComponentSize, 1);
        XFNV_INLINED_LOCAL(NiTexture::FormatPrefs, kPrefs, std::nullopt, std::nullopt);
        // EXE-INLINED 0x4F31D0
        kPrefs.m_ePixelLayout = NiTexture::FormatPrefs::PIX_DEFAULT;
        kPrefs.m_eAlphaFmt = NiTexture::FormatPrefs::ALPHA_DEFAULT;
        kPrefs.m_eMipMapped = NiTexture::FormatPrefs::MIP_DEFAULT;
        void* pMemory = NiMemObject::operator new(sizeof(NiPixelData));
        NiPixelData* pPixelData =
            pMemory ? ::new (pMemory) NiPixelData(auiSize[0], auiSize[1], NiPixelFormat_RGBA32, 1, 1) : nullptr;
        uint32_t uiBytes = auiSize[0] * auiSize[1] * 4;
        // GetPixels(0, 0).
        // EXE-INLINED 0x64DAC0
        uint8_t* pPixels = pPixelData->m_pucPixels + pPixelData->m_puiOffsetInBytes[pPixelData->m_uiMipmapLevels] * 0 +
            pPixelData->m_puiOffsetInBytes[0];
        uiComponentSize = 1;
        // EXE-INLINED 0x462D80
        pTexFile->BinaryRead(pPixels, uiBytes, &uiComponentSize, 1);
        pPixelData->bNoConvert = true;
        pMemory = NiMemObject::operator new(sizeof(NiTexturingProperty));
        NiTexturingProperty* pTexProp = nullptr;
        if (pMemory) {
            // EXE-INLINED 0x438170
            NiGlobalStringTable::GlobalStringHandle kHandle =
                pFontFile ? NiGlobalStringTable::AddString(pFontFile) : NiGlobalStringTable::NULL_STRING;
            XFNV_INLINED_LOCAL(NiFixedString, kFilePath, std::nullopt, std::nullopt);
            kFilePath.SetHandle(kHandle);
            pTexProp = ::new (pMemory) NiTexturingProperty(pPixelData, kFilePath, kPrefs);
            // The reference count is the untyped first word of the string's buffer, 8 bytes before the characters; it
            // is decremented as raw memory.
            // EXE-INLINED 0x4381B0
            if (kFilePath.GetHandle() != NiGlobalStringTable::NULL_STRING) {
                InterlockedDecrementUInt32(
                    reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kFilePath.GetHandle()) - 8)
                );
            }
        }
        delete pTexFile;
        pTexProp->SetBaseFilterMode(NiTexturingProperty::FILTER_BILERP);
        pTextureData[j] = pTexProp;
    }
    ++iRefCount;
}

NiAVObject* Font::MakeString(
    float afStartX,
    float afStartY,
    float afZ,
    BSStringT<char>& apTextString,
    int32_t& aiWidth,
    int32_t aiFlags,
    const NiColorA& axFontColor,
    bool abUpperLeftCorner,
    bool abPrepareObject
) {
    size_t sLen;
    uint32_t itext_count;
    NiPoint3 xpos;
    XFNV_INLINED_LOCAL(NiColorA, pspecial_color, std::nullopt, std::nullopt);
    float afY[2];
    if (apTextString.sLen == 0xFFFF) {
        sLen = std::strlen(apTextString.pString);
    }
    else {
        sLen = apTextString.sLen;
    }
    if (!sLen || !pFontData) {
        return nullptr;
    }
    float fx_offset = static_cast<float>(aiWidth);
    GetLineOffsets(apTextString.pString, fx_offset, *afY, aiFlags, 0);
    xpos.x = afStartX + fx_offset;
    xpos.z = afStartY;
    xpos.y = afZ;
    if (abUpperLeftCorner) {
        double v11 = pFontData->pFontLetters[32].fHeight - pFontData->fBaseLine;
        xpos.z = xpos.z - (v11 + v11);
    }
    for (itext_count = 0;; ++itext_count) {
        uint32_t v16 = apTextString.sLen == 0xFFFF ? std::strlen(apTextString.pString) : apTextString.sLen;
        if (itext_count >= v16 || !apTextString.pString[apTextString.pString != nullptr ? itext_count : 0]) {
            break;
        }
    }
    if (!itext_count) {
        return nullptr;
    }
    NiTriShape* pflat = MakeTriShape(static_cast<int32_t>(itext_count), axFontColor, abPrepareObject);
    afY[1] = afStartX;
    float y = xpos.y;
    float z = xpos.z;
    pflat->m_kLocal.m_Translate.x = afStartX;
    pflat->m_kLocal.m_Translate.y = y;
    pflat->m_kLocal.m_Translate.z = z;
    NiColorA* pcurrent_color = nullptr;
    // MAYBE-INLINED 0xA125F6: a write opening NiColorA's uses: may be its inlined constructor
    pspecial_color.r = 0.0f;
    // MAYBE-INLINED 0xA125FB: a write opening NiColorA's uses: may be its inlined constructor
    pspecial_color.g = 0.0f;
    // MAYBE-INLINED 0xA12600: a write opening NiColorA's uses: may be its inlined constructor
    pspecial_color.b = 1.0f;
    // MAYBE-INLINED 0xA12605: a write opening NiColorA's uses: may be its inlined constructor
    pspecial_color.a = 1.0f;
    aiWidth = 0;
    float fstart_x = xpos.x;
    int32_t iverts = 0;
    for (uint32_t a = 0; apTextString.pString[apTextString.pString != nullptr ? a : 0]; ++a) {
        int32_t v13;
        uint8_t ch[5];
        if (apTextString.pString[apTextString.pString != nullptr ? a : 0] == 3) {
            pcurrent_color = nullptr;
        }
        char v15 = apTextString.pString[apTextString.pString != nullptr ? a : 0];
        if (v15 == 9) {
            float v14 = std::fmod(static_cast<double>(xpos.x), static_cast<double>(75.0f));
            xpos.x = 75.0 - v14 + xpos.x;
        }
        else {
            if (v15 == 10) {
                float fyoff;
                float fxoff = static_cast<float>(aiWidth);
                GetLineOffsets(apTextString.pString, fxoff, fyoff, aiFlags, a + 1);
                xpos.x = fxoff;
                xpos.z = xpos.z - pFontData->fBaseLine;
            }
        }
        ch[0] = apTextString.pString[apTextString.pString != nullptr ? a : 0];
        Font::ConvertCharacter(*ch);
        // Writes the character's code as a 32-bit value over the raw bytes at ch+1.
        *reinterpret_cast<uint32_t*>(&ch[1]) = ch[0];
        FontLetter* pfont_letter = &pFontData->pFontLetters[ch[0]];
        AddChar(pfont_letter, iverts++, pflat, xpos, pcurrent_color);
        if (static_cast<int32_t>(xpos.x - fstart_x) <= aiWidth) {
            v13 = aiWidth;
        }
        else {
            v13 = static_cast<int32_t>(xpos.x - fstart_x);
        }
        aiWidth = v13;
        if (apTextString.pString[apTextString.pString != nullptr ? a : 0] == 2) {
            pcurrent_color = &pspecial_color;
        }
    }
    NiPoint3* ptemp = pflat->m_spModelData.m_pObject->m_pkVertex;
    pflat->m_spModelData.m_pObject->m_kBound.ComputeFromData(pflat->m_spModelData.m_pObject->m_usVertices, ptemp);
    return (NiAVObject*)pflat;
}

void Font::GetLineOffsets(const char* apTextString, float& afX, float& afY, int32_t aiFlags, uint32_t aiStart) {
    float v7;
    NiPoint3 v10;
    if (!(afX > 0.0)) {
        v7 = 0x1.fffffe0000000p+127f;
    }
    else {
        v7 = afX;
    }
    float v11 = v7;
    int32_t v6 = ((int8_t)aiFlags) & 15;
    if (v6 == 2) {
        v10 = FontManager::pInstance->CalculateStringDimensions(apTextString, iFontNum, v11, aiStart);
        afX = (float)((-v10.x) / 2.0);
    }
    else {
        if (v6 == 4) {
            v10 = FontManager::pInstance->CalculateStringDimensions(apTextString, iFontNum, v11, aiStart);
            afX = (-v10.x);
        }
        else {
            afX = (float)0.0;
        }
    }
}

void Font::SetSpacingForButtonIcon(ButtonIcon* apIconData, const char* apIconID) {
    apIconData->fWidth = 60.0f;
    apIconData->fZOffset = fFontHeight;
    if (*apIconID == 97 || (*apIconID == 98 || (*apIconID == 120 || *apIconID == 121))) {
        apIconData->fXOffset = -15.0f;
        apIconData->fSpacing = -30.0f;
    }
    else {
        if ((*apIconID == 108 || *apIconID == 114) && (apIconID[1] == 115 || apIconID[1] == 116)) {
            apIconData->fXOffset = -12.5f;
            apIconData->fSpacing = -25.0f;
        }
        else {
            apIconData->fXOffset = -10.0f;
            apIconData->fSpacing = -20.0f;
        }
    }
    uint32_t iFontNum = this->iFontNum;
    if (iFontNum == 1) {
        apIconData->fXOffset = apIconData->fXOffset - 2.5;
        apIconData->fZOffset = apIconData->fZOffset - 3.0;
        apIconData->fSpacing = apIconData->fSpacing - 5.0;
    }
    else {
        if (iFontNum == 7) {
            apIconData->fXOffset = apIconData->fXOffset + 2.5;
            apIconData->fZOffset = apIconData->fZOffset - 10.0;
            apIconData->fSpacing = apIconData->fSpacing - 5.0;
        }
    }
}

// The string's length field holds 0xFFFF when the length is unknown.
void Font::TrimToLine(BSStringT<char>& axTextString, uint32_t aiLine) {
    size_t sLen;
    int32_t v5 = 1;
    int32_t v9 = -1;
    int32_t v6 = -1;
    if (axTextString.sLen == 0xFFFF) {
        sLen = std::strlen(axTextString.pString);
    }
    else {
        sLen = axTextString.sLen;
    }
    for (size_t i = 0; i < sLen && axTextString.pString[axTextString.pString != nullptr ? i : 0]; ++i) {
        if (v5 == aiLine && v9 == -1) {
            v9 = i;
        }
        if (axTextString.pString[axTextString.pString != nullptr ? i : 0] == 10) {
            ++v5;
        }
        if (v5 == aiLine + 1) {
            v6 = i;
            break;
        }
    }
    if (v9 || v6 != -1) {
        if (v9 == -1) {
            axTextString.Set(" ", 0);
        }
        else {
            uint32_t j;
            if (v6 == -1) {
                v6 = sLen;
            }
            void* v4 = MemoryManager_Singleton.Allocate(sizeof(char) * static_cast<size_t>(v6 - v9 + 1));
            std::memset(v4, 0, v6 - v9 + 1);
            for (j = 0; j < v6 - v9; ++j) {
                *(static_cast<uint8_t*>(v4) + j) = axTextString.pString[axTextString.pString != nullptr ? v9 + j : 0];
            }
            *(static_cast<uint8_t*>(v4) + j) = 0;
            axTextString.Set(" ", 0);
            axTextString.Set(static_cast<char*>(v4), 0);
            MemoryManager_Singleton.Deallocate(v4);
        }
    }
}

int32_t Font::LineBreakText(
    BSStringT<char>& axTextString,
    int32_t& aiWidth,
    int32_t& aiHeight,
    bool abSkipHyperLinkedCharacters,
    char acEolChar
) {
    int32_t imax_width_2;
    uint32_t sLen;
    uint32_t n;
    if (!axTextString.pString || !pFontData) {
        return 0;
    }
    uint32_t iindex = 0;
    int32_t iindex_width = 0;
    int32_t iwidth = 0;
    int32_t imax_width = 0;
    int32_t iheight = static_cast<int32_t>(pFontData->pFontLetters[32].fHeight);
    int32_t iline_count = 0;
    if (axTextString.sLen == 0xFFFF) {
        sLen = std::strlen(axTextString.pString);
    }
    else {
        sLen = axTextString.sLen;
    }
    int32_t imax_lines = aiHeight;
    if (!aiWidth) {
        aiWidth = 0xF423F;
    }
    for (n = 0; n < sLen && axTextString.pString[axTextString.pString != nullptr ? n : 0]; ++n) {
        char v17 = axTextString.pString[axTextString.pString != nullptr ? n : 0];
        if (v17 == 9) {
            iwidth += 75 - iwidth % 75;
        }
        else {
            if (v17 == 10) {
                int32_t v16;
                axTextString.pString[axTextString.pString != nullptr ? n : 0] = acEolChar;
                iheight += static_cast<int32_t>(pFontData->fBaseLine);
                if (imax_width <= iwidth) {
                    v16 = iwidth;
                }
                else {
                    v16 = imax_width;
                }
                imax_width = v16;
                iwidth = 0;
                iindex = 0;
                ++iline_count;
            }
            else {
                if (v17 == 32) {
                    iindex = n;
                }
                uint8_t ch = axTextString.pString[axTextString.pString != nullptr ? n : 0];
                Font::ConvertCharacter(ch);
                FontLetter* pfont_letter = &pFontData->pFontLetters[ch];
                iwidth += static_cast<int32_t>(pfont_letter->fWidth + pfont_letter->fSpacing);
                if (iindex == n) {
                    iindex_width = iwidth;
                }
                if (iwidth > aiWidth) {
                    if (iindex) {
                        int32_t v15;
                        axTextString.pString[axTextString.pString != nullptr ? iindex : 0] = acEolChar;
                        iheight += static_cast<int32_t>(pFontData->fBaseLine);
                        if (imax_width <= iindex_width) {
                            v15 = iindex_width;
                        }
                        else {
                            v15 = imax_width;
                        }
                        imax_width = v15;
                        iindex = 0;
                        ++iline_count;
                        iwidth -= iindex_width;
                    }
                    else {
                        int32_t v9;
                        uint32_t v14;
                        if (axTextString.sLen == 0xFFFF) {
                            v14 = std::strlen(axTextString.pString);
                        }
                        else {
                            v14 = axTextString.sLen;
                        }
                        if (v14 >= axTextString.sMaxLen) {
                            size_t v13;
                            if (axTextString.sLen == 0xFFFF) {
                                v13 = std::strlen(axTextString.pString);
                            }
                            else {
                                v13 = axTextString.sLen;
                            }
                            if (v13 + 1 < axTextString.sMaxLen) {
                                size_t v10;
                                axTextString.operator+=("\n");
                                if (axTextString.sLen == 0xFFFF) {
                                    v10 = std::strlen(axTextString.pString);
                                }
                                else {
                                    v10 = axTextString.sLen;
                                }
                                axTextString.pString[axTextString.pString != nullptr ? v10 - 1 : 0] = 0;
                            }
                            else {
                                size_t v11;
                                size_t v12;
                                axTextString.operator+=("\n\n");
                                if (axTextString.sLen == 0xFFFF) {
                                    v12 = std::strlen(axTextString.pString);
                                }
                                else {
                                    v12 = axTextString.sLen;
                                }
                                axTextString.pString[axTextString.pString != nullptr ? v12 - 1 : 0] = 0;
                                if (axTextString.sLen == 0xFFFF) {
                                    v11 = std::strlen(axTextString.pString);
                                }
                                else {
                                    v11 = axTextString.sLen;
                                }
                                axTextString.pString[axTextString.pString != nullptr ? v11 - 2 : 0] = 0;
                            }
                        }
                        for (uint32_t a = sLen + 2; a > n; --a) {
                            axTextString.pString[axTextString.pString != nullptr ? a : 0] =
                                axTextString.pString[axTextString.pString != nullptr ? a - 2 : 0];
                        }
                        axTextString.pString[axTextString.pString != nullptr ? n - 1 : 0] = 45;
                        axTextString.pString[axTextString.pString != nullptr ? n : 0] = acEolChar;
                        iheight += static_cast<int32_t>(pFontData->fBaseLine);
                        if (imax_width <=
                            iwidth - static_cast<int32_t>(pfont_letter->fSpacing + pfont_letter->fWidth)) {
                            v9 = iwidth - static_cast<int32_t>(pfont_letter->fSpacing + pfont_letter->fWidth);
                        }
                        else {
                            v9 = imax_width;
                        }
                        imax_width = v9;
                        iindex = 0;
                        ++iline_count;
                        iwidth = 0;
                    }
                }
            }
        }
        if (imax_lines > 0 && iline_count >= imax_lines) {
            while (axTextString.pString[axTextString.pString != nullptr ? n : 0] != acEolChar) {
                --n;
            }
            axTextString.pString[axTextString.pString != nullptr ? n : 0] = 0;
            iheight -= static_cast<int32_t>(pFontData->fBaseLine);
            break;
        }
    }
    char* pString = axTextString.pString;
    if ((*axTextString.pString) && (n >= sLen)) {
        ++iline_count;
    }

    if (imax_width <= iwidth) {
        imax_width_2 = iwidth;
    }
    else {
        imax_width_2 = imax_width;
    }
    aiWidth = imax_width_2;
    aiHeight = iheight;
    return iline_count;
}

void Font::GetLineNumber(char* apSrcString, BSStringT<char>& axDestString, int32_t aiLineNum, char acSepChar) {
    int32_t i;
    char* apString = apSrcString;
    int32_t v6 = 0;
    for (i = 0; apSrcString[i] && v6 != aiLineNum; ++i) {
        if (apSrcString[i] == acSepChar) {
            apString = &apSrcString[i];
            ++v6;
        }
    }
    while (apSrcString[i] && apSrcString[i] != acSepChar) {
        ++i;
    }
    apSrcString[i] = 0;
    axDestString.Set(apString, 0u);
    apSrcString[i] = acSepChar;
}

void Font::ConvertCharacter(uint8_t& acChar) {
    switch (acChar) {
        case 0x91:
        case 0x92: acChar = '\''; break;
        case 0x93:
        case 0x94: acChar = '"'; break;
    }
}

FontManager::CharData::CharData(int32_t aiFontIndex, uint8_t acChar, NiColorA axColor, int32_t aiJustification)
  : iFontIndex(aiFontIndex >= 8 ? 8 : (aiFontIndex >= 0 ? aiFontIndex : 0)), cChar(acChar), xColor(axColor),
    iJustification(aiJustification), iLeadingEdge(0), iX(0) {
    FontLetter* pLetter = &FontManager::pInstance->pFont[iFontIndex]->pFontData->pFontLetters[cChar];
    double fWidth = 0.0;
    if (pLetter->fWidth + pLetter->fWidth != 0.0) {
        fWidth = pLetter->fLeadingEdge + pLetter->fSpacing;
    }
    iWidth = static_cast<int32_t>(fWidth);
    iRise = static_cast<int32_t>(FontManager::pInstance->pFont[iFontIndex]->pFontData->fBaseLine);
    iDrop = static_cast<int32_t>(pLetter->fHeight - pLetter->fTopEdge);
}

FontManager::CharData::~CharData() {}

FontManager::CharData* FontManager::CharData::Copy() {
    FontManager::CharData* pnew;
    FontManager::CharData* v4 =
        static_cast<FontManager::CharData*>(MemoryManager_Singleton.Allocate(sizeof(FontManager::CharData)));
    if (v4) {
        pnew = ::new (v4) FontManager::CharData(iFontIndex, cChar, xColor, iJustification);
    }
    else {
        pnew = nullptr;
    }
    pnew->xFilename.Set(xFilename);
    pnew->iWidth = iWidth;
    pnew->iRise = iRise;
    pnew->iDrop = iDrop;
    pnew->iX = iX;
    pnew->iLeadingEdge = 0;
    return pnew;
}

void FontManager::CharData::RevertToDefault() {
    iFontIndex = 0;
    NiColorA* p_xColor = &xColor;
    p_xColor->r = 0.4588f;
    p_xColor->g = 0.2314f;
    p_xColor->b = 0.1294f;
    p_xColor->a = 1.0f;
    iJustification = 1;
    xFilename.Set(static_cast<const char*>(nullptr), 0u);
    SetChar(32);
}

void FontManager::CharData::SetChar(uint8_t acChar) {
    char* pString;
    cChar = acChar;
    if (!xFilename.pString || (pString = xFilename.pString, !*pString)) {
        double v4;
        if (FontManager::pInstance->pFont[iFontIndex]->pFontData->pFontLetters[cChar].fWidth == 0.0) {
            v4 = 0.0;
        }
        else {
            FontLetter* v3 = &FontManager::pInstance->pFont[iFontIndex]->pFontData->pFontLetters[cChar];
            FontLetter* v7 = &FontManager::pInstance->pFont[iFontIndex]->pFontData->pFontLetters[cChar];
            v4 = v7->fSpacing + v7->fLeadingEdge;
        }
        iWidth =
            static_cast<int32_t>(FontManager::pInstance->pFont[iFontIndex]->pFontData->pFontLetters[cChar].fWidth + v4);
        iRise = static_cast<int32_t>(FontManager::pInstance->pFont[iFontIndex]->pFontData->fBaseLine);
        iDrop = static_cast<int32_t>(-FontManager::pInstance->pFont[iFontIndex]->fMaxDrop);
        iLeadingEdge = 0;
    }
    iX = 0;
}

FontManager::TextLine::TextLine(
    FontManager::TextPage* apPage,
    FontManager::CharData* apChar,
    int32_t aiSkippedSpace,
    int32_t aiPageWidth
)
  : iWidth(0), iNextSpacing(0), iRise(0), iDrop(0), iSkippedSpace(aiSkippedSpace), iJustify(apChar->iJustification),
    iPageWidth(aiPageWidth), pPage(apPage) {
    AddChar(apChar, false);
}

FontManager::TextLine::~TextLine() {
    while (xChars.m_uiCount) {
        delete xChars.RemoveHead();
    }
}

FontManager::TextLine* FontManager::TextLine::AddChar(FontManager::CharData* apChar, bool abToFront) {
    FontManager::TextLine* pline = nullptr;
    if (apChar->iWidth + this->iWidth <= iPageWidth) {
        char* pString;
        int32_t iWidth;
        if (!apChar->xFilename.pString || (pString = apChar->xFilename.pString, !*pString)) {
            int32_t iDrop;
            bool bAdjustLineHeight = false;
            if (!xChars.m_uiCount) {
                this->iRise = aFontRise[apChar->iFontIndex];
            }
            if (abToFront) {
                FontManager::CharData* pfirst = nullptr;
                if (xChars.m_uiCount) {
                    pfirst = xChars.m_pkHead->m_element;
                }
                if ((pfirst) && (apChar->iFontIndex != pfirst->iFontIndex)) {
                    bAdjustLineHeight = true;
                }

                apChar->iX = apChar->iLeadingEdge;
                this->iWidth = apChar->iWidth;
                NiTListItem<FontManager::CharData*>* elem_2 = xChars.m_pkHead;
                while (elem_2) {
                    FontManager::CharData** p_m_element = &elem_2->m_element;
                    elem_2 = elem_2->m_pkNext;
                    FontManager::CharData* pchar_2 = *p_m_element;
                    pchar_2->iX = pchar_2->iLeadingEdge + this->iWidth;
                    this->iWidth += pchar_2->iWidth;
                }
            }
            else {
                FontManager::CharData* pfirst_2 = nullptr;
                if (xChars.m_uiCount) {
                    pfirst_2 = xChars.m_pkTail->m_element;
                }
                if ((pfirst_2) && (apChar->iFontIndex != pfirst_2->iFontIndex)) {
                    bAdjustLineHeight = true;
                }

                apChar->iX = apChar->iLeadingEdge + this->iWidth;
                this->iWidth += apChar->iWidth;
            }
            if (bAdjustLineHeight) {
                int32_t iRise;
                if (this->iRise <= apChar->iRise) {
                    iRise = apChar->iRise;
                }
                else {
                    iRise = this->iRise;
                }
                this->iRise = iRise;
            }
            if (this->iDrop <= apChar->iDrop) {
                iDrop = apChar->iDrop;
            }
            else {
                iDrop = this->iDrop;
            }
            this->iDrop = iDrop;
        }
        else {
            int32_t v7;
            int32_t v8;
            if (abToFront) {
                apChar->iX = 0;
                this->iWidth = apChar->iWidth;
                NiTListItem<FontManager::CharData*>* elem_3 = xChars.m_pkHead;
                while (elem_3) {
                    FontManager::CharData** v17 = &elem_3->m_element;
                    elem_3 = elem_3->m_pkNext;
                    FontManager::CharData* pchar_3 = *v17;
                    pchar_3->iX = pchar_3->iLeadingEdge + this->iWidth;
                    this->iWidth += pchar_3->iWidth;
                }
            }
            else {
                apChar->iX = this->iWidth;
                this->iWidth += apChar->iWidth;
            }
            if (this->iRise <= apChar->iRise) {
                v8 = apChar->iRise;
            }
            else {
                v8 = this->iRise;
            }
            this->iRise = v8;
            if (this->iDrop <= apChar->iDrop) {
                v7 = apChar->iDrop;
            }
            else {
                v7 = this->iDrop;
            }
            this->iDrop = v7;
        }
        if (abToFront) {
            xChars.AddHead(apChar);
        }
        else {
            xChars.AddTail(apChar);
        }
        if (pPage->iWidth <= this->iWidth) {
            iWidth = this->iWidth;
        }
        else {
            iWidth = pPage->iWidth;
        }
        pPage->iWidth = iWidth;
    }
    else {
        if (apChar->cChar == 32) {
            FontManager::TextLine* pline_4;
            apChar->SetChar(0);
            FontManager::TextLine* v19 =
                static_cast<FontManager::TextLine*>(MemoryManager_Singleton.Allocate(sizeof(FontManager::TextLine)));
            if (v19) {
                pline_4 = ::new (v19) FontManager::TextLine(pPage, apChar, 0, iPageWidth);
            }
            else {
                pline_4 = nullptr;
            }
            pline = pline_4;
            pline_4->iRise = aFontRise[apChar->iFontIndex];
        }
        else {
            NiTListItem<FontManager::CharData*>* elem;
            for (elem = xChars.m_pkTail; elem && elem->m_element->cChar != 32; elem = elem->m_pkPrev) {
                ;
            }
            if (elem) {
                FontManager::TextLine* pline_2;
                FontManager::CharData* pchar;
                FontManager::TextLine* v22 = static_cast<FontManager::TextLine*>(
                    MemoryManager_Singleton.Allocate(sizeof(FontManager::TextLine))
                );
                if (v22) {
                    pline_2 = ::new (v22) FontManager::TextLine(pPage, apChar, 0, iPageWidth);
                }
                else {
                    pline_2 = nullptr;
                }
                pline = pline_2;
                pline_2->iRise = aFontRise[apChar->iFontIndex];
                for (pchar = xChars.RemoveTail(); xChars.m_uiCount && pchar->cChar != 32;
                     this->iWidth -= pchar->iWidth) {
                    pline_2->AddChar(pchar, true);
                    pchar = xChars.RemoveTail();
                }
                if (pchar) {
                    pchar->xFilename.Set(nullptr, 0);
                    MemoryManager_Singleton.Deallocate(pchar);
                }
            }
            else {
                FontManager::CharData* phyphen;
                FontManager::TextLine* pline_3;
                int32_t iend_width;
                FontManager::CharData* plast;
                FontManager::TextLine* v21 = static_cast<FontManager::TextLine*>(
                    MemoryManager_Singleton.Allocate(sizeof(FontManager::TextLine))
                );
                if (v21) {
                    pline_3 = ::new (v21) FontManager::TextLine(pPage, apChar, 0, iPageWidth);
                }
                else {
                    pline_3 = nullptr;
                }
                pline = pline_3;
                pline_3->iRise = aFontRise[apChar->iFontIndex];
                FontManager::CharData* v20 = static_cast<FontManager::CharData*>(
                    MemoryManager_Singleton.Allocate(sizeof(FontManager::CharData))
                );
                if (v20) {
                    XFNV_INLINED_LOCAL(NiColorA, v5, std::nullopt, std::nullopt);
                    // MAYBE-INLINED 0xA1A20F: a write opening NiColorA's uses: may be its inlined constructor
                    v5.r = 0.4588f;
                    // MAYBE-INLINED 0xA1A214: a write opening NiColorA's uses: may be its inlined constructor
                    v5.g = 0.2314f;
                    // MAYBE-INLINED 0xA1A21A: a write opening NiColorA's uses: may be its inlined constructor
                    v5.b = 0.1294f;
                    // MAYBE-INLINED 0xA1A220: a write opening NiColorA's uses: may be its inlined constructor
                    v5.a = 1.0f;
                    phyphen = ::new (v20) FontManager::CharData(0, 45, v5, 1);
                }
                else {
                    phyphen = nullptr;
                }
                for (iend_width = 0; xChars.m_uiCount && phyphen->iWidth + (this->iWidth - iend_width) > iPageWidth;
                     iend_width += plast->iWidth) {
                    plast = xChars.RemoveTail();
                    pline_3->AddChar(plast, true);
                }
                this->iWidth -= iend_width;
                AddChar(phyphen, false);
            }
        }
    }
    return pline;
}

FontManager::TextPage::TextPage(
    FontManager::TextDoc* apDoc,
    FontManager::CharData* apChar,
    int32_t aiPageWidth,
    int32_t aiPageHeight
)
  : iHeight(0), iPageWidth(aiPageWidth), iPageHeight(aiPageHeight), iLastFontHeight(35), pDoc(apDoc) {
    for (int32_t i = 0; i < 8; ++i) {
        pCharsPerFont[i] = 0;
    }
    AddChar(apChar, 0);
}

// The page starts as high as its first line; iWidth is deliberately left unset, as the game does.
FontManager::TextPage::TextPage(
    FontManager::TextDoc* apDoc,
    FontManager::TextLine* apLine,
    int32_t aiPageWidth,
    int32_t aiPageHeight
)
  : iHeight(apLine->iDrop + apLine->iRise), iPageWidth(aiPageWidth), iPageHeight(aiPageHeight), iLastFontHeight(35),
    pDoc(apDoc) {
    xLines.AddTail(apLine);
    apLine->pPage = this;
    for (int32_t i = 0; i < 8; ++i) {
        pCharsPerFont[i] = 0;
    }
}

FontManager::TextPage::~TextPage() {
    while (xLines.m_uiCount) {
        delete xLines.RemoveHead();
    }
}

int32_t FontManager::TextPage::GetCharCountForFont(int32_t aiFont) {
    int32_t v12 = 0;
    NiTListItem<FontManager::TextLine*>* m_pkHead = xLines.m_pkHead;
    while (m_pkHead) {
        FontManager::TextLine** p_m_element = &m_pkHead->m_element;
        m_pkHead = m_pkHead->m_pkNext;
        if (*p_m_element) {
            NiTListItem<FontManager::CharData*>* v10 = (*p_m_element)->xChars.m_pkHead;
            while (v10) {
                FontManager::CharData** v7 = &v10->m_element;
                v10 = v10->m_pkNext;
                FontManager::CharData* v9 = *v7;
                if (*v7) {
                    char* pString;
                    FontManager::CharData* v2 = *v7;
                    if (!v9->xFilename.pString || (pString = v9->xFilename.pString, !*pString)) {
                        FontManager::CharData* v4 = *v7;
                        if (v9->iFontIndex == aiFont) {
                            ++v12;
                        }
                    }
                }
            }
        }
    }
    return v12;
}

FontManager::TextPage* FontManager::TextPage::AddChar(FontManager::CharData* apChar, int32_t aiNewLines) {
    FontManager::TextLine* pline = nullptr;
    int32_t iskipped = 0;
    while (aiNewLines > 1) {
        iskipped += iLastFontHeight;
        --aiNewLines;
    }
    if (!xLines.m_uiCount || aiNewLines) {
        FontManager::TextLine* pline_2;
        FontManager::TextLine* v15 =
            static_cast<FontManager::TextLine*>(MemoryManager_Singleton.Allocate(sizeof(FontManager::TextLine)));
        if (v15) {
            pline_2 = ::new (v15) FontManager::TextLine(this, apChar, iskipped, iPageWidth);
        }
        else {
            pline_2 = nullptr;
        }
        pline = pline_2;
    }
    else {
        pline = xLines.m_pkTail->m_element->AddChar(apChar, false);
    }
    if (apChar) {
        char* pString;
        if (!apChar->xFilename.pString || (pString = apChar->xFilename.pString, !*pString)) {
            FontLetter* v4 = &FontManager::pInstance->pFont[apChar->iFontIndex]->pFontData->pFontLetters[apChar->cChar];
            iLastFontHeight = static_cast<int32_t>(v4->fHeight - v4->fTopEdge) +
                static_cast<int32_t>(FontManager::pInstance->pFont[apChar->iFontIndex]->pFontData->fBaseLine);
        }
    }
    if (pline) {
        char* pString_1;
        FontManager::TextPage* ppage = this;
        if (pline->iSkippedSpace + (pline->iRise + iHeight) <= iPageHeight) {
            xLines.AddTail(pline);
            iHeight += pline->iSkippedSpace + pline->iRise;
        }
        else {
            FontManager::TextPage* ppage_2;
            pline->iSkippedSpace = 0;
            FontManager::TextPage* v14 =
                static_cast<FontManager::TextPage*>(MemoryManager_Singleton.Allocate(sizeof(FontManager::TextPage)));
            if (v14) {
                ppage_2 = ::new (v14) FontManager::TextPage(pDoc, pline, iPageWidth, iPageHeight);
            }
            else {
                ppage_2 = nullptr;
            }
            ppage = ppage_2;
            ppage_2->iHeight = pline->iRise;
            pline->pPage = ppage_2;
        }
        if (!apChar->xFilename.pString || (pString_1 = apChar->xFilename.pString, !*pString_1)) {
            ++ppage->pCharsPerFont[apChar->iFontIndex];
        }
        if (ppage != this) {
            return ppage;
        }
    }
    else {
        if (apChar) {
            char* pString_2;
            if (!apChar->xFilename.pString || (pString_2 = apChar->xFilename.pString, !*pString_2)) {
                ++pCharsPerFont[apChar->iFontIndex];
            }
        }
    }
    return nullptr;
}

FontManager::TextDoc::~TextDoc() {
    while (xPages.m_uiCount) {
        delete xPages.RemoveHead();
    }
}

void FontManager::TextDoc::AddChar(FontManager::CharData* apChar, int32_t aiNewLines, bool abNewPage) {
    FontManager::TextPage* ppage;
    ppage = nullptr;
    if (!xPages.m_uiCount || abNewPage) {
        FontManager::TextPage* ppage_2;
        FontManager::TextPage* v6 =
            static_cast<FontManager::TextPage*>(MemoryManager_Singleton.Allocate(sizeof(FontManager::TextPage)));
        if (v6) {
            ppage_2 = ::new (v6) FontManager::TextPage(this, apChar, iPageWidth, iPageHeight);
        }
        else {
            ppage_2 = nullptr;
        }
        ppage = ppage_2;
    }
    else {
        ppage = xPages.m_pkTail->m_element->AddChar(apChar, aiNewLines);
    }
    if (ppage) {
        xPages.AddTail(ppage);
    }
}

FontManager::TextPage* FontManager::TextDoc::CurrentPage() {
    NiTListItem<FontManager::TextPage*>* i;
    if (!xPages.m_uiCount) {
        return nullptr;
    }
    int32_t a = iPageNum;
    for (i = xPages.m_pkHead; i; i = i->m_pkNext) {
        if (!a--) {
            break;
        }
    }
    if (i) {
        return i->m_element;
    }

    return nullptr;
}

void FontManager::TextDoc::Render(NiNode* apParent, FontManager::TextData* apData) {
    XFNV_INLINED_LOCAL(BSStringT<char>, v6, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(NiColorA, axColor, std::nullopt, std::nullopt);
    NiTriShape* ptri_2;
    NiTListItem<FontManager::TextPage*>* i;
    NiTList<NiTriShape*> ximages;
    FontManager::TextPage* v86 = nullptr;
    int32_t a = iPageNum;
    int32_t iiconIndex = 0;
    NiTListItem<FontManager::TextPage*>* m_pkHead = xPages.m_pkHead;
    for (i = m_pkHead; i; i = i->m_pkNext) {
        if (!a--) {
            break;
        }
        FontManager::TextPage** p_m_element = &i->m_element;
    }
    if (i) {
        int32_t v9;
        int32_t ptri_index[8];
        NiTriShape* pflats[8];
        FontManager::TextPage* ppage = i->m_element;
        for (uint32_t a_2 = 0; a_2 < 8; ++a_2) {
            ptri_index[a_2] = 0;
            pflats[a_2] = nullptr;
            int32_t ichar_count = ppage->GetCharCountForFont(a_2);
            if (ichar_count > 0) {
                // MAYBE-INLINED 0xA19152: a write opening NiColorA's uses: may be its inlined constructor
                axColor.r = 0.45882353f;
                // MAYBE-INLINED 0xA1915E: a write opening NiColorA's uses: may be its inlined constructor
                axColor.g = 0.23137255f;
                // MAYBE-INLINED 0xA1916A: a write opening NiColorA's uses: may be its inlined constructor
                axColor.b = 0.12941177f;
                // MAYBE-INLINED 0xA19172: a write opening NiColorA's uses: may be its inlined constructor
                axColor.a = 1.0f;
                FontManager* v54 = FontManager::pInstance;
                Font* v53 = FontManager::pInstance->pFont[a_2];
                NiTriShape* TriShape = v53->MakeTriShape(ichar_count, axColor, true);
                pflats[a_2] = TriShape;
                NiTriShape* v52 = pflats[a_2];
                v52->m_kLocal.m_Translate = NiPoint3_ZERO;
            }
        }
        // EXE-INLINED none: NiTList<NiTriShape*>'s constructor (count, head and tail zeroed)
        ximages.m_uiCount = 0;
        ximages.m_pkHead = nullptr;
        ximages.m_pkTail = nullptr;
        NiTriShape* pIconShape = nullptr;
        FontManager* v51 = FontManager::pInstance;
        Font* v50 = FontManager::pInstance->pFont[0];
        if (v50->ButtonIcons.iSize) {
            FontManager* v49 = FontManager::pInstance;
            Font* v48 = FontManager::pInstance->pFont[0];
            pIconShape = v48->MakeIconsTriShape();
            pIconShape->m_kLocal.m_Translate = NiPoint3_ZERO;
            // EXE-INLINED none: NiTList<NiTriShape*>::AddHead (the heap node, then AddNodeHead)
            NiTListItem<NiTriShape*>* pNode = static_cast<NiTListItem<NiTriShape*>*>(
                MemoryManager_Singleton.Allocate(sizeof(NiTListItem<NiTriShape*>))
            );
            pNode->m_element = pIconShape;
            ximages.AddNodeHead(pNode);
        }
        if (ppage) {
            int32_t iline = 0;
            NiPoint3 xpos = NiPoint3_ZERO;
            NiTListItem<FontManager::TextLine*>* v44 = ppage->xLines.m_pkHead;
            NiTListItem<FontManager::TextLine*>* xelem_2 = v44;
            while (xelem_2) {
                int32_t iLines = apData->iLines;
                if (iline >= iLines) {
                    break;
                }
                FontManager::TextLine** v42 = &xelem_2->m_element;
                xelem_2 = xelem_2->m_pkNext;
                FontManager::TextLine* pline = *v42;
                xpos.x = 0.0f;
                int32_t izone = 0;
                if (apData->bIsHypertext) {
                    izone = this->iPageWidth;
                }
                if (pline->iJustify == 4) {
                    int32_t v17 = izone - pline->iWidth;
                    xpos.x = static_cast<float>(v17);
                }
                if (pline->iJustify == 2) {
                    int32_t v16 = (izone - pline->iWidth) / 2;
                    xpos.x = static_cast<float>(v16);
                }
                xpos.y = 0.0f;
                int32_t v15 = pline->iSkippedSpace + pline->iRise;
                xpos.z = xpos.z - static_cast<double>(v15);
                NiTListItem<FontManager::CharData*>* v40 = pline->xChars.m_pkHead;
                NiTListItem<FontManager::CharData*>* xelem2 = v40;
                while (xelem2) {
                    char* v5;
                    BSStringT<char>* p_xFilename;
                    FontManager::CharData** v39 = &xelem2->m_element;
                    xelem2 = xelem2->m_pkNext;
                    FontManager::CharData* pchar = *v39;
                    FontManager* v38 = FontManager::pInstance;
                    Font* pfont = FontManager::pInstance->pFont[pchar->iFontIndex];
                    char* pString = pchar->xFilename.pString;
                    if (!pString || (p_xFilename = &pchar->xFilename, (v5 = pchar->xFilename.pString, !*v5))) {
                        NiPoint3 xpos2 = xpos;
                        xpos2.x = static_cast<double>(pchar->iX) + xpos.x;
                        if (pchar->cChar == 1) {
                            int32_t aiIconIndex = iiconIndex;
                            FontManager* v35 = FontManager::pInstance;
                            Font* v34 = FontManager::pInstance->pFont[0];
                            v34->AddIcon(iiconIndex++, pIconShape, xpos2);
                        }
                        else {
                            int32_t aiVert = ptri_index[pchar->iFontIndex]++;
                            pfont->AddChar(
                                &pfont->pFontData->pFontLetters[pchar->cChar], aiVert, pflats[pchar->iFontIndex], xpos2,
                                &pchar->xColor
                            );
                        }
                    }
                    else {
                        NiPoint3 v7;
                        xpos.x = static_cast<double>(pchar->iX) + xpos.x;
                        v9 = pchar->iDrop + pchar->iRise;
                        int32_t iWidth = pchar->iWidth;
                        v7.x = xpos.x;
                        v7.y = xpos.y;
                        v7.z = xpos.z;
                        BSStringT<char>* v57 = &v6;
                        BSStringT<char>* v11 = ::new (&v6) BSStringT<char>(pchar->xFilename);
                        FontManager* v33 = FontManager::pInstance;
                        NiTriShape* ptri = FontManager::pInstance->MakeImage(v6, v7, iWidth, v9);
                        // MakeImage takes its argument by value and receives a copy of this raw-storage local, so the
                        // local is destroyed here.
                        v6.~BSStringT<char>();
                        ptri_2 = ptri;
                        if (ptri) {
                            ximages.AddTail(ptri_2);
                        }
                    }
                }
            }
            bool bvalid = false;
            for (uint32_t b = 0; b < 8; ++b) {
                if (pflats[b]) {
                    NiUpdateData UpdateData;
                    NiGeometryData* m_pObject = pflats[b]->m_spModelData.m_pObject;
                    NiPoint3* ptemp = m_pObject->m_pkVertex;
                    NiPoint3* pkData = ptemp;
                    NiGeometryData* v30 = pflats[b]->m_spModelData.m_pObject;
                    uint16_t m_usVertices = v30->m_usVertices;
                    NiGeometryData* v28 = pflats[b]->m_spModelData.m_pObject;
                    v28->m_kBound.ComputeFromData(m_usVertices, ptemp);
                    NiTriShape* v27 = pflats[b];
                    v27->m_kLocal.m_Translate = NiPoint3_ZERO;
                    pflats[b]->UpdateProperties();
                    UpdateData.fTime = 0.0f;
                    UpdateData.bUpdateControllers = false;
                    UpdateData.bParallelUpdate = false;
                    UpdateData.bFoundParticles = false;
                    UpdateData.bFoundMorphController = false;
                    UpdateData.bSceneGraphChange = false;
                    pflats[b]->Update(UpdateData);
                    apParent->AttachChild(pflats[b], 1);
                    bvalid = true;
                }
            }
            while (true) {
                NiUpdateData UpdateData_2;
                uint32_t m_uiCount = ximages.m_uiCount;
                if (!ximages.m_uiCount) {
                    break;
                }
                NiTriShape* ptri_3 = ximages.RemoveHead();
                NiGeometryData* v25 = ptri_3->m_spModelData.m_pObject;
                NiPoint3* ptemp_2 = v25->m_pkVertex;
                NiPoint3* v61 = ptemp_2;
                NiGeometryData* v23 = ptri_3->m_spModelData.m_pObject;
                uint16_t v22 = v23->m_usVertices;
                NiGeometryData* v21 = ptri_3->m_spModelData.m_pObject;
                v21->m_kBound.ComputeFromData(v22, ptemp_2);
                ptri_3->m_kLocal.m_Translate = NiPoint3_ZERO;
                ptri_3->UpdateProperties();
                UpdateData_2.fTime = 0.0f;
                UpdateData_2.bUpdateControllers = false;
                UpdateData_2.bParallelUpdate = false;
                UpdateData_2.bFoundParticles = false;
                UpdateData_2.bFoundMorphController = false;
                UpdateData_2.bSceneGraphChange = false;
                ptri_3->Update(UpdateData_2);
                apParent->AttachChild(ptri_3, 1);
            }
        }
        FontManager* v20 = FontManager::pInstance;
        Font* v19 = FontManager::pInstance->pFont[0];
        v9 = 1;
        v19->ButtonIcons.Clear(true);
        ximages.RemoveAll();
    }
}

FontManager::FontManager() {
    MEM_CONTEXT eMemContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_FONTS;
    pFont[0] = new Font(1, sFont_1_Filename.uValue.str, true);
    pFont[1] = new Font(2, sFont_2_Filename.uValue.str, true);
    pFont[2] = new Font(3, sFont_3_Filename.uValue.str, true);
    pFont[3] = new Font(4, sFont_4_Filename.uValue.str, true);
    pFont[4] = new Font(5, sFont_5_Filename.uValue.str, true);
    pFont[5] = new Font(6, sFont_6_Filename.uValue.str, true);
    pFont[6] = new Font(7, sFont_7_Filename.uValue.str, true);
    pFont[7] = new Font(8, sFont_8_Filename.uValue.str, true);
    bUseNewFonts = false;
    bool bHasFontData = false;
    for (int32_t i = 0; i < 8; ++i) {
        if (pFont[i]->pFontData) {
            bHasFontData = true;
            break;
        }
    }
    if (!bHasFontData) {
        InitializationFlow("Missing all font data. The program will be terminated.");
    }
    GetThreadTLSData()->eMemContext = eMemContext;
}

FontManager::~FontManager() {
    for (int32_t i = 0; i < 8; ++i) {
        delete pFont[i];
        pFont[i] = nullptr;
    }
}

bool FontManager::CreateText(BSStringT<char> axTextString, FontManager::TextData& axTextData, NiNode* apNode) {
    BSStringT<char> xtext(axTextString);
    FontManager::TextDoc* pdocument = PrepText(xtext, axTextData);
    if (pdocument) {
        pdocument->Render(apNode, &axTextData);
        pdocument->~TextDoc();
        MemoryManager_Singleton.Deallocate(pdocument);
    }
    xtext.Set(nullptr, 0);
    axTextString.Set(nullptr, 0);
    return true;
}

NiPoint3
FontManager::CalculateStringDimensions(const char* apText, int32_t aiFont, float afWrapwidth, int32_t aiStartIndex) {
    if (aiFont >= 1 && (aiFont <= 8 && apText)) {
        float v7;
        NiPoint3 v17 = NiPoint3_ZERO;
        int32_t v20 = std::strlen(apText);
        FontLetter* pFontLetters = pFont[aiFont - 1]->pFontData->pFontLetters;
        float v18 = 0.0f;
        float v19 = 0.0f;
        float LinePadding = GetLinePadding(aiFont);
        float v14 = 0.0f;
        char v16 = 0;
        int32_t v15 = 1;
        v17.y = pFontLetters[32].fHeight;
        for (int32_t i = aiStartIndex; i < v20; ++i) {
            uint8_t acChar = apText[i];
            float v12 = 0.0f;
            Font::ConvertCharacter(acChar);
            v12 = pFontLetters[acChar].fLeadingEdge + pFontLetters[acChar].fWidth + pFontLetters[acChar].fSpacing;
            switch (acChar) {
                case 9: {
                    float v9 = std::fmod(static_cast<double>(v19), static_cast<double>(75.0f));
                    v12 = 75.0 - v9;
                    break;
                }
                case 10:
                case 32: {
                    v18 = v19;
                    v16 = 0;
                    break;
                }
                case 126: {
                    v18 = v19 + pFontLetters[45].fLeadingEdge + pFontLetters[45].fWidth + pFontLetters[45].fSpacing;
                    v16 = 1;
                    break;
                }
                default: {
                    break;
                }
            }
            if (acChar != 126) {
                v19 = v19 + v12;
            }
            if (afWrapwidth < static_cast<double>(v19) || acChar == 10) {
                float x;
                if (!(v18 > 0.0)) {
                    v18 = v19 - v12 - v14 + pFontLetters[45].fLeadingEdge + pFontLetters[45].fWidth +
                        pFontLetters[45].fSpacing;
                    v19 = v12 + v14;
                }
                else {
                    v19 = v19 - v18;
                    if (!v16) {
                        if (acChar == 10) {
                            v19 = 0.0f;
                        }
                        else {
                            v19 = v19 -
                                (pFontLetters[32].fLeadingEdge + pFontLetters[32].fWidth + pFontLetters[32].fSpacing);
                        }
                    }
                }
                if (!(v18 < static_cast<double>(v17.x))) {
                    x = v18;
                }
                else {
                    x = v17.x;
                }
                v17.x = x;
                v17.y = LinePadding + pFont[aiFont - 1]->pFontData->fBaseLine + v17.y;
                v18 = 0.0f;
                ++v15;
            }
            v14 = v12;
        }
        if (!(v19 < static_cast<double>(v17.x))) {
            v7 = v19;
        }
        else {
            v7 = v17.x;
        }
        v17.z = static_cast<float>(v15);
        return NiPoint3(v7, v17.y, v17.z);
    }

    return NiPoint3_ZERO;
}

float FontManager::GetLinePadding(int32_t aiFont) {
    float flinePadding = 0.0f;
    switch (aiFont) {
        case 2:
        case 6: flinePadding = 4.0f; break;
        case 3: flinePadding = 6.0f; break;
        case 7: flinePadding = 2.0f; break;
    }
    return flinePadding;
}

int32_t FontManager::GetCharType(char acchar) {
    int32_t result;
    switch (acchar) {
        case 0: {
            result = 32;
            break;
        }
        case 34:
        case 39: {
            result = 8;
            break;
        }
        case 60:
        case 123: {
            result = 1;
            break;
        }
        case 61: {
            result = 16;
            break;
        }
        case 62:
        case 125: {
            result = 2;
            break;
        }
        default: {
            if (((int32_t)acchar) > 32) {
                result = 0;
            }
            else {
                result = 4;
            }
            break;
        }
    }
    return result;
}

// Collects the text of apSource from character *aiIndex on into apResult, up to the first character whose type has a
// bit of aebfMatches (a space ends it with type 0x20 written to *aeResultMatch) or of aebfInvMatches (consumed, its
// type written), or lacks every bit of aeResultMatch when that is set. Line breaks are skipped; other characters are
// kept only with bKeep, where an HTML entity ("&...;") is replaced by its text replacement or text icon. The character
// that ended the run is left in *acResultChar. Every other character is dropped when bKeep is clear, as the game does.
// The local buffer is appended to the string once it holds more than 2000 characters, without clearing its tail.
BSStringT<char>* FontManager::CollectTo(
    BSStringT<char>* apResult,
    const char* apSource,
    int32_t& aiIndex,
    int32_t aebfMatches,
    int32_t aebfInvMatches,
    int32_t& aeResultMatch,
    char& acResultChar,
    bool bKeep
) {
    XFNV_INLINED_LOCAL(BSStringT<char>, xString, std::nullopt, std::nullopt);
    // EXE-INLINED none: BSStringT<char>::Init
    xString.pString = nullptr;
    xString.sLen = 0;
    xString.sMaxLen = 0;
    int32_t iLength = 0;
    char acBuffer[0x800];
    std::memset(acBuffer, 0, 0x800);
    while (true) {
        acResultChar = apSource[aiIndex];
        int32_t eType = GetCharType(acResultChar);
        if (eType & 0x20) {
            aeResultMatch = 0x20;
            break;
        }
        if (eType & aebfMatches) {
            aeResultMatch = eType;
            ++aiIndex;
            break;
        }
        if (aebfInvMatches && !(eType & aebfInvMatches)) {
            acResultChar = apSource[aiIndex];
            aeResultMatch = GetCharType(acResultChar);
            break;
        }
        if (acResultChar == '\n' || acResultChar == '\r') {
            ++aiIndex;
            continue;
        }
        if (bKeep) {
            if (apSource[aiIndex] == '&') {
                int32_t iEntityLength = 1;
                while (!GetCharType(apSource[aiIndex + iEntityLength]) &&
                       apSource[aiIndex + iEntityLength - 1] != ';') {
                    ++iEntityLength;
                }
                if (apSource[aiIndex + iEntityLength - 1] != ';') {
                    Warning(WARN_MENUS, "Font::CollectTo() -- Found GameSetting w/o ending ; in HTML text.", 0);
                }
                else {
                    char* pcEntity = static_cast<char*>(MemoryManager_Singleton.Allocate(iEntityLength + 1));
                    strncpy_s(pcEntity, iEntityLength + 1, apSource + aiIndex, iEntityLength);
                    pcEntity[iEntityLength] = 0;
                    char acReplacement[0x104];
                    acReplacement[0] = 0;
                    Interface::FindTextReplacementString(pcEntity, acReplacement, 0x104, false);
                    if (acReplacement[0] == '\\') {
                        // A text icon: "\name\" without its last character, from the second one on.
                        acReplacement[std::strlen(acReplacement) - 1] = 0;
                        pFont[0]->AddTextIcon(acReplacement + 1);
                        acBuffer[iLength++] = 1;
                    }
                    else if (acReplacement[0]) {
                        size_t stReplacementLength = std::strlen(acReplacement);
                        memcpy_s(&acBuffer[iLength], stReplacementLength, acReplacement, stReplacementLength);
                        iLength += stReplacementLength;
                    }
                    else if (iEntityLength) {
                        memcpy_s(&acBuffer[iLength], iEntityLength, pcEntity, iEntityLength);
                        iLength += iEntityLength;
                    }
                    aiIndex = iEntityLength + aiIndex - 1;
                    MemoryManager_Singleton.Deallocate(pcEntity);
                }
            }
            else {
                acBuffer[iLength++] = apSource[aiIndex];
                if (iLength > 2000) {
                    xString += acBuffer;
                    iLength = 0;
                }
            }
        }
        ++aiIndex;
    }
    xString += acBuffer;
    // SWEEP-INLINE 0x4047F0
    ::new (apResult) BSStringT<char>(xString);
    // The local's destructor.
    xString.Set(nullptr, 0);
    return apResult;
}

// Parses the hypertext of axTextString into a new TextDoc, one CharData per character. Only a text whose first
// non-space character is '<' is hypertext; otherwise nothing is laid out and null is returned. Tags change the current
// character: <IMG SRC= WIDTH= HEIGHT=> makes it an image, <DIV ALIGN=> sets the justification after a line break, <FONT
// FACE= COLOR=RRGGBB> the font (by file name or 1-based number) and colour, </FONT> reverts to the default; <BR> and
// <P> add one and two line breaks, <HR> a new page (two breaks for an unbounded height). The text without its tags is
// written back to axTextString; axData gets the page count and the current page's line count, width and height.
FontManager::TextDoc* FontManager::PrepHypertext(BSStringT<char>& axTextString, FontManager::TextData& axData) {
    // EXE-INLINED none: BSStringT<char>::Length
    uint32_t uiLength = axTextString.sLen == 0xFFFF ? std::strlen(axTextString.pString) : axTextString.sLen;
    char* pcText = static_cast<char*>(MemoryManager_Singleton.Allocate(uiLength + 1));
    std::memset(pcText, 0, uiLength + 1);
    std::strcpy(pcText, axTextString.pString);
    int32_t iIndex = 0;
    int32_t eMatch = 0;
    char cMatch = 0;

    // The text run read last, the tag name, the attribute name, the attribute value, and the text without its tags.
    alignas(BSStringT<char>) uint8_t aucText[sizeof(BSStringT<char>)];
    BSStringT<char>& kText = *static_cast<BSStringT<char>*>(static_cast<void*>(aucText));
    // EXE-INLINED 0x403970
    kText.pString = nullptr;
    kText.sLen = 0;
    kText.sMaxLen = 0;
    alignas(BSStringT<char>) uint8_t aucTag[sizeof(BSStringT<char>)];
    BSStringT<char>& kTag = *static_cast<BSStringT<char>*>(static_cast<void*>(aucTag));
    // EXE-INLINED 0x403970
    kTag.pString = nullptr;
    kTag.sLen = 0;
    kTag.sMaxLen = 0;
    alignas(BSStringT<char>) uint8_t aucAttribute[sizeof(BSStringT<char>)];
    BSStringT<char>& kAttribute = *static_cast<BSStringT<char>*>(static_cast<void*>(aucAttribute));
    // EXE-INLINED 0x403970
    kAttribute.pString = nullptr;
    kAttribute.sLen = 0;
    kAttribute.sMaxLen = 0;
    alignas(BSStringT<char>) uint8_t aucValue[sizeof(BSStringT<char>)];
    BSStringT<char>& kValue = *static_cast<BSStringT<char>*>(static_cast<void*>(aucValue));
    // EXE-INLINED 0x403970
    kValue.pString = nullptr;
    kValue.sLen = 0;
    kValue.sMaxLen = 0;
    alignas(BSStringT<char>) uint8_t aucPlainText[sizeof(BSStringT<char>)];
    BSStringT<char>& kPlainText = *static_cast<BSStringT<char>*>(static_cast<void*>(aucPlainText));
    // EXE-INLINED 0x403970
    kPlainText.pString = nullptr;
    kPlainText.sLen = 0;
    kPlainText.sMaxLen = 0;

    alignas(CharData) uint8_t aucChar[sizeof(CharData)];
    CharData* pChar = static_cast<CharData*>(static_cast<void*>(aucChar));
    ::new (aucChar) CharData(0, ' ', NiColorA(0.4588f, 0.2314f, 0.1294f, 1.0f), 1);
    bool bImage = false;
    int32_t iNewLines = 0;
    bool bNewPage = false;
    TextDoc* pDoc = nullptr;
    int32_t iIcon = 0;

    // Each CollectTo result is released right after use. Each call site has its own result temporary, which CollectTo
    // fills.
    XFNV_INLINED_LOCAL(BSStringT<char>, kCollected0, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(BSStringT<char>, kCollected1, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(BSStringT<char>, kCollected2, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(BSStringT<char>, kCollected3, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(BSStringT<char>, kCollected4, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(BSStringT<char>, kCollected5, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(BSStringT<char>, kCollected6, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(BSStringT<char>, kCollected7, std::nullopt, std::nullopt);
    CollectTo(&kCollected0, pcText, iIndex, 0, 4, eMatch, cMatch, false);
    kCollected0.Set(nullptr, 0);
    if (eMatch == 1) {
        void* pDocMemory = MemoryManager_Singleton.Allocate(sizeof(TextDoc));
        TextDoc* pNewDoc;
        if (pDocMemory) {
            int32_t iPageNum = axData.iPageNum;
            int32_t iPageHeight = axData.iHeight;
            int32_t iPageWidth = axData.iWidth;
            // EXE-INLINED none: FontManager::TextDoc::TextDoc
            ::new (pDocMemory) NiTPointerList<TextPage*>;
            pNewDoc = static_cast<TextDoc*>(pDocMemory);
            pNewDoc->iPageWidth = iPageWidth;
            pNewDoc->iPageHeight = iPageHeight;
            pNewDoc->iPageNum = iPageNum;
        }
        else {
            pNewDoc = nullptr;
        }
        pDoc = pNewDoc;
        iIndex = 0;
        while (true) {
            // The text up to the next '<'. Copies from the pointer CollectTo returns.
            BSStringT<char>* pResult = CollectTo(&kCollected1, pcText, iIndex, 1, 0, eMatch, cMatch, true);
            kText.Set(*pResult);
            kCollected1.Set(nullptr, 0);
            if (kText.pString) {
                if (bImage) {
                    // The pending image goes in first.
                    bImage = false;
                    pChar->cChar = 0;
                    pDoc->AddChar(pChar->Copy(), iNewLines, bNewPage);
                    iNewLines = 0;
                    bNewPage = false;
                    pChar->xFilename.Set("", 0);
                }
                kPlainText += kText;
                // EXE-INLINED none: BSStringT<char>::Length
                for (uint32_t i = 0; i < (kText.sLen == 0xFFFF ? std::strlen(kText.pString) : kText.sLen); ++i) {
                    pChar->SetChar(kText.pString[kText.pString ? i : 0]);
                    if (pChar->cChar == 1) {
                        // A button icon is as wide as its glyph plus its spacing.
                        pChar->iWidth = static_cast<int32_t>(
                            pFont[0]->ButtonIcons.pBuffer[iIcon].fWidth + pFont[0]->ButtonIcons.pBuffer[iIcon].fSpacing
                        );
                        ++iIcon;
                    }
                    pDoc->AddChar(pChar->Copy(), iNewLines, bNewPage);
                    iNewLines = 0;
                    bNewPage = false;
                }
            }
            if (eMatch & 0x20) {
                break;
            }
            // The tag name, upper-cased.
            pResult = CollectTo(&kCollected2, pcText, iIndex, 6, 0, eMatch, cMatch, true);
            kTag.Set(*pResult);
            kCollected2.Set(nullptr, 0);
            // EXE-INLINED none: BSStringT<char>::Length
            for (uint32_t i = 0; i < (kTag.sLen == 0xFFFF ? std::strlen(kTag.pString) : kTag.sLen); ++i) {
                kTag.pString[kTag.pString ? i : 0] =
                    static_cast<char>(std::toupper(kTag.pString[kTag.pString ? i : 0]));
            }
            if (eMatch & 0x20) {
                break;
            }
            if (eMatch & 4) {
                // The attributes, up to the '>'.
                while (true) {
                    CollectTo(&kCollected3, pcText, iIndex, 0, 4, eMatch, cMatch, false);
                    kCollected3.Set(nullptr, 0);
                    if ((eMatch & 0x20) || (eMatch & 2)) {
                        break;
                    }
                    pResult = CollectTo(&kCollected4, pcText, iIndex, 0x16, 0, eMatch, cMatch, true);
                    kAttribute.Set(*pResult);
                    kCollected4.Set(nullptr, 0);
                    // EXE-INLINED none: BSStringT<char>::Length
                    for (uint32_t i = 0;
                         i < (kAttribute.sLen == 0xFFFF ? std::strlen(kAttribute.pString) : kAttribute.sLen); ++i) {
                        kAttribute.pString[kAttribute.pString ? i : 0] =
                            static_cast<char>(std::toupper(kAttribute.pString[kAttribute.pString ? i : 0]));
                    }
                    if ((eMatch & 0x20) || (eMatch & 2)) {
                        break;
                    }
                    if (eMatch & 4) {
                        // An attribute without a value.
                        kValue.Set("true", 0);
                    }
                    else {
                        CollectTo(&kCollected5, pcText, iIndex, 0, 4, eMatch, cMatch, false);
                        kCollected5.Set(nullptr, 0);
                        if ((eMatch & 0x20) || (eMatch & 2)) {
                            break;
                        }
                        if (eMatch == 8) {
                            // A quoted value keeps its case.
                            ++iIndex;
                            pResult = CollectTo(&kCollected6, pcText, iIndex, 10, 0, eMatch, cMatch, true);
                            kValue.Set(*pResult);
                            kCollected6.Set(nullptr, 0);
                            cMatch = pcText[iIndex];
                            ++iIndex;
                            eMatch = GetCharType(cMatch);
                        }
                        else {
                            pResult = CollectTo(&kCollected7, pcText, iIndex, 6, 0, eMatch, cMatch, true);
                            kValue.Set(*pResult);
                            kCollected7.Set(nullptr, 0);
                            // EXE-INLINED none: BSStringT<char>::Length
                            for (uint32_t i = 0;
                                 i < (kValue.sLen == 0xFFFF ? std::strlen(kValue.pString) : kValue.sLen); ++i) {
                                kValue.pString[kValue.pString ? i : 0] =
                                    static_cast<char>(std::toupper(kValue.pString[kValue.pString ? i : 0]));
                            }
                        }
                    }
                    if (kTag.StrCmp("IMG", true) == 0) {
                        bImage = true;
                        if (kAttribute.StrCmp("SRC", true) == 0) {
                            pChar->xFilename.Set(kValue);
                            // SWEEP-INLINE 0x408A80
                        }
                        else if (kAttribute.StrCmp("WIDTH", true) == 0) {
                            std::sscanf(kValue.pString, "%i", &pChar->iWidth);
                            // SWEEP-INLINE 0x408A80
                        }
                        else if (kAttribute.StrCmp("HEIGHT", true) == 0) {
                            std::sscanf(kValue.pString, "%i", &pChar->iRise);
                            pChar->iDrop = 0;
                        }
                    }
                    else if (bImage) {
                        // Another tag ends the pending image.
                        bImage = false;
                        pChar->cChar = 0;
                        pDoc->AddChar(pChar->Copy(), iNewLines, bNewPage);
                        iNewLines = 0;
                        bNewPage = false;
                        pChar->xFilename.Set("", 0);
                    }
                    if (kTag.StrCmp("DIV", true) == 0) {
                        ++iNewLines;
                        if (kAttribute.StrCmp("ALIGN", true) == 0) {
                            if (kValue.StrCmp("LEFT", true) == 0) {
                                pChar->iJustification = 1;
                                // SWEEP-INLINE 0x408A80
                            }
                            else if (kValue.StrCmp("CENTER", true) == 0) {
                                pChar->iJustification = 2;
                                // SWEEP-INLINE 0x408A80
                            }
                            else if (kValue.StrCmp("RIGHT", true) == 0) {
                                pChar->iJustification = 4;
                            }
                        }
                        // SWEEP-INLINE 0x408A80
                    }
                    else if (kTag.StrCmp("FONT", true) == 0) {
                        if (kAttribute.StrCmp("FACE", true) == 0) {
                            // The font by its file name or its 1-based number.
                            for (int32_t iFont = 0; iFont < 8; ++iFont) {
                                bool bFound = false;
                                if (_stricmp(kValue.pString, pFont[iFont]->pFontFile) == 0) {
                                    bFound = true;
                                }
                                else if (iFont + 1 == std::atoi(kValue.pString)) {
                                    bFound = true;
                                }
                                if (bFound) {
                                    pChar->iFontIndex = iFont;
                                    pChar->SetChar(pChar->cChar);
                                    break;
                                }
                            }
                        }
                        if (kAttribute.StrCmp("COLOR", true) == 0
                            // EXE-INLINED none: BSStringT<char>::Length
                            && (kValue.sLen == 0xFFFF ? std::strlen(kValue.pString) : kValue.sLen) == 6) {
                            // RRGGBB in hex, each channel clamped to 0..255. Each digit is upper-cased twice: once for
                            // the letter test, once for its value.
                            NiColorA kColor(0.0f, 0.0f, 0.0f, 0.0f);
                            int32_t iRed = (std::toupper(kValue.pString[kValue.pString ? 0 : 0]) < 'A'
                                                ? std::toupper(kValue.pString[kValue.pString ? 0 : 0]) - '0'
                                                : std::toupper(kValue.pString[kValue.pString ? 0 : 0]) - ('A' - 10))
                                << 4;
                            iRed += std::toupper(kValue.pString[kValue.pString ? 1 : 0]) < 'A'
                                ? std::toupper(kValue.pString[kValue.pString ? 1 : 0]) - '0'
                                : std::toupper(kValue.pString[kValue.pString ? 1 : 0]) - ('A' - 10);
                            int32_t iClamped = iRed > 0 ? iRed : 0;
                            iRed = iClamped < 255 ? iClamped : 255;
                            int32_t iGreen = (std::toupper(kValue.pString[kValue.pString ? 2 : 0]) < 'A'
                                                  ? std::toupper(kValue.pString[kValue.pString ? 2 : 0]) - '0'
                                                  : std::toupper(kValue.pString[kValue.pString ? 2 : 0]) - ('A' - 10))
                                << 4;
                            iGreen += std::toupper(kValue.pString[kValue.pString ? 3 : 0]) < 'A'
                                ? std::toupper(kValue.pString[kValue.pString ? 3 : 0]) - '0'
                                : std::toupper(kValue.pString[kValue.pString ? 3 : 0]) - ('A' - 10);
                            iClamped = iGreen > 0 ? iGreen : 0;
                            iGreen = iClamped < 255 ? iClamped : 255;
                            int32_t iBlue = (std::toupper(kValue.pString[kValue.pString ? 4 : 0]) < 'A'
                                                 ? std::toupper(kValue.pString[kValue.pString ? 4 : 0]) - '0'
                                                 : std::toupper(kValue.pString[kValue.pString ? 4 : 0]) - ('A' - 10))
                                << 4;
                            iBlue += std::toupper(kValue.pString[kValue.pString ? 5 : 0]) < 'A'
                                ? std::toupper(kValue.pString[kValue.pString ? 5 : 0]) - '0'
                                : std::toupper(kValue.pString[kValue.pString ? 5 : 0]) - ('A' - 10);
                            iClamped = iBlue > 0 ? iBlue : 0;
                            iBlue = iClamped < 255 ? iClamped : 255;
                            kColor = NiColorA(
                                static_cast<float>(iRed / 255.0), static_cast<float>(iGreen / 255.0),
                                static_cast<float>(iBlue / 255.0), 1.0f
                            );
                            pChar->xColor = kColor;
                        }
                    }
                    if ((eMatch & 0x20) || (eMatch & 2)) {
                        break;
                    }
                }
            }
            if (kTag.StrCmp("BR", true) == 0) {
                ++iNewLines;
                // SWEEP-INLINE 0x408A80
            }
            else if (kTag.StrCmp("P", true) == 0) {
                iNewLines += 2;
                // SWEEP-INLINE 0x408A80
            }
            else if (kTag.StrCmp("HR", true) == 0) {
                // An unbounded page cannot break: two line breaks instead.
                if (axData.iHeight == 0x7FFFFFFF) {
                    iNewLines += 2;
                }
                else {
                    bNewPage = true;
                    iNewLines = 0;
                }
                // SWEEP-INLINE 0x408A80
            }
            else if (kTag.StrCmp("/FONT", true) == 0) {
                pChar->RevertToDefault();
            }
        }
        axTextString.Set(kPlainText);
    }
    MemoryManager_Singleton.Deallocate(pcText);
    if (pDoc) {
        axData.bIsHypertext = true;
        // EXE-INLINED none: NiTListBase::GetSize
        axData.iNumPages = pDoc->xPages.m_uiCount;
        TextPage* pPage = pDoc->CurrentPage();
        if (pPage) {
            axData.iHeight = 0;
            axData.iWidth = 0;
            NiTListItem<TextLine*>* pPos = pPage->xLines.GetHeadPos();
            while (pPos) {
                TextLine* pLine = pPage->xLines.GetNext(pPos);
                axData.iHeight += pLine->iRise + pLine->iSkippedSpace;
                axData.iWidth = axData.iWidth > pLine->iWidth ? axData.iWidth : pLine->iWidth;
            }
            // EXE-INLINED none: NiTListBase::GetSize
            axData.iNumLines = pPage->xLines.m_uiCount;
        }
    }
    // The locals' strings are released with Set(nullptr, 0), the CharData's image name first.
    pChar->xFilename.Set(nullptr, 0);
    kPlainText.Set(nullptr, 0);
    kValue.Set(nullptr, 0);
    kAttribute.Set(nullptr, 0);
    kTag.Set(nullptr, 0);
    kText.Set(nullptr, 0);
    return pDoc;
}

void FontManager::SetNewFonts(bool a2) {
    bUseNewFonts = (bool)((uint8_t)a2);
}

FontManager::TextDoc* FontManager::PrepText(BSStringT<char>& axTextString, FontManager::TextData& axData) {
    XFNV_INLINED_LOCAL(NiColorA, v5, std::nullopt, std::nullopt);
    size_t sLen;
    FontManager::TextDoc* pdocument;
    XFNV_INLINED_LOCAL(FontManager::CharData, xchar, std::nullopt, std::nullopt);
    if (!axTextString.pString) {
        return nullptr;
    }
    if (axData.iWidth < 2) {
        axData.iWidth = 0x7FFFFFFF;
    }
    if (axData.iHeight < 2) {
        axData.iHeight = 0x7FFFFFFF;
    }
    FontManager::TextDoc* pdocument_2 = PrepHypertext(axTextString, axData);
    if (pdocument_2) {
        return pdocument_2;
    }
    FontManager::TextDoc* v19 =
        static_cast<FontManager::TextDoc*>(MemoryManager_Singleton.Allocate(sizeof(FontManager::TextDoc)));
    if (v19) {
        int32_t iPageNum = axData.iPageNum;
        int32_t iHeight = axData.iHeight;
        int32_t iWidth = axData.iWidth;
        ::new (&v19->xPages) NiTPointerList<FontManager::TextPage*>;
        v19->iPageWidth = iWidth;
        v19->iPageHeight = iHeight;
        v19->iPageNum = iPageNum;
        pdocument = v19;
    }
    else {
        pdocument = nullptr;
    }
    pdocument_2 = pdocument;
    // MAYBE-INLINED 0xA18BA3: a write opening NiColorA's uses: may be its inlined constructor
    v5.r = 0.4588f;
    // MAYBE-INLINED 0xA18BA8: a write opening NiColorA's uses: may be its inlined constructor
    v5.g = 0.2314f;
    // MAYBE-INLINED 0xA18BAE: a write opening NiColorA's uses: may be its inlined constructor
    v5.b = 0.1294f;
    // MAYBE-INLINED 0xA18BB4: a write opening NiColorA's uses: may be its inlined constructor
    v5.a = 1.0f;
    ::new (&xchar) FontManager::CharData(0, 32, v5, 1);
    xchar.RevertToDefault();
    xchar.iFontIndex = axData.iDefaultFont;
    float g = axData.xColor.g;
    float b = axData.xColor.b;
    float a = axData.xColor.a;
    xchar.xColor.r = axData.xColor.r;
    xchar.xColor.g = g;
    xchar.xColor.b = b;
    xchar.xColor.a = a;
    xchar.iJustification = axData.iJustification;
    if (axTextString.sLen == 0xFFFF) {
        sLen = std::strlen(axTextString.pString);
    }
    else {
        sLen = axTextString.sLen;
    }
    if (!sLen) {
        axTextString.Set(" ", 0);
    }
    axData.xNewText.Set(axTextString);
    int32_t inew_line = 0;
    for (uint32_t a_2 = 0;; ++a_2) {
        uint32_t v8 = axTextString.sLen == 0xFFFF ? std::strlen(axTextString.pString) : axTextString.sLen;
        if (a_2 >= v8) {
            break;
        }
        uint8_t ch = axTextString.pString[axTextString.pString != nullptr ? a_2 : 0];
        Font::ConvertCharacter(ch);
        if (ch == axData.cLineSep) {
            ++inew_line;
        }
        else {
            if (ch == 9 || ch >= 32u) {
                xchar.SetChar(ch);
                int32_t v6 = inew_line;
                FontManager::CharData* v4 = xchar.Copy();
                pdocument_2->AddChar(v4, v6, false);
                inew_line = 0;
            }
        }
    }
    axData.bIsHypertext = false;
    axData.iNumPages = 0;
    axData.iWidth = 0;
    axData.iHeight = 0;
    axData.iNumLines = 0;
    if (pdocument_2) {
        axData.iNumPages = pdocument_2->xPages.m_uiCount;
        FontManager::TextPage* ppage = pdocument_2->CurrentPage();
        if (ppage) {
            axData.iHeight = 0;
            axData.iWidth = 0;
            NiTListItem<FontManager::TextLine*>* ppage_2 = ppage->xLines.m_pkHead;
            while (ppage_2) {
                int32_t v7;
                FontManager::TextLine** p_m_element = &ppage_2->m_element;
                ppage_2 = ppage_2->m_pkNext;
                FontManager::TextLine* v20 = *p_m_element;
                axData.iHeight += (*p_m_element)->iSkippedSpace + (*p_m_element)->iRise;
                if (axData.iWidth <= v20->iWidth) {
                    v7 = v20->iWidth;
                }
                else {
                    v7 = axData.iWidth;
                }
                axData.iWidth = v7;
            }
            axData.iNumLines = ppage->xLines.m_uiCount;
        }
    }
    xchar.xFilename.Set(nullptr, 0);
    return pdocument_2;
}

NiTriShape* FontManager::MakeImage(BSStringT<char> axFilename, NiPoint3 axPos, int32_t aiWidth, int32_t aiHeight) {
    TileShaderProperty* ptsp;
    NiTriShape* pshape;
    NiPoint2* ptextures;
    NiPoint3* pvertices;
    NiPoint3 v64;
    NiPoint3 v65;
    float fscale;
    NiPoint3* v74 = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(4 * sizeof(NiPoint3)));
    if (v74) {
        int32_t v40 = 4;
        for (NiPoint3* i = v74; --v40 >= 0; ++i) {
            ;
        }
        pvertices = v74;
    }
    else {
        pvertices = nullptr;
    }
    NiPoint2* v72 = static_cast<NiPoint2*>(MemoryManager_Singleton.Allocate(4 * sizeof(NiPoint2)));
    if (v72) {
        int32_t v38 = 4;
        for (NiPoint2* j = v72; --v38 >= 0; ++j) {
            ;
        }
        ptextures = v72;
    }
    else {
        ptextures = nullptr;
    }
    uint16_t* pvert_order = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
    float x = axPos.x;
    float y = axPos.y;
    float v71 = static_cast<double>(aiHeight) + axPos.z;
    pvertices->x = axPos.x;
    pvertices->y = y;
    pvertices->z = v71;
    float v66 = axPos.x;
    float v67 = axPos.y;
    float z = axPos.z;
    NiPoint3* v6 = pvertices + 1;
    pvertices[1].x = axPos.x;
    v6->y = v67;
    v6->z = z;
    v65.x = static_cast<double>(aiWidth) + axPos.x;
    v65.y = axPos.y;
    v65.z = static_cast<double>(aiHeight) + axPos.z;
    pvertices[2] = v65;
    v64.x = static_cast<double>(aiWidth) + axPos.x;
    v64.y = axPos.y;
    v64.z = axPos.z;
    pvertices[3] = v64;
    float v62 = 0.0f;
    float v63 = 0.0f;
    ptextures->x = 0.0f;
    ptextures->y = 0.0f;
    float v60 = 0.0f;
    float v61 = 1.0f;
    ptextures[1].x = 0.0f;
    ptextures[1].y = 1.0f;
    float v58 = 1.0f;
    float v59 = 0.0f;
    ptextures[2].x = 1.0f;
    ptextures[2].y = 0.0f;
    float v56 = 1.0f;
    float v57 = 1.0f;
    ptextures[3].x = 1.0f;
    ptextures[3].y = 1.0f;
    for (uint32_t a = 0;; ++a) {
        uint32_t sLen;
        if (axFilename.sLen == 0xFFFF) {
            char* Str = axFilename.pString;
            sLen = std::strlen(axFilename.pString);
        }
        else {
            sLen = axFilename.sLen;
        }
        if (a >= sLen) {
            break;
        }
        if (axFilename.pString[axFilename.pString != nullptr ? a : 0] == 47) {
            axFilename.pString[axFilename.pString != nullptr ? a : 0] = 92;
        }
    }
    fscale = 1.0f;
    NiTexture* ptex = nullptr;
    if ((axFilename.pString) && (*axFilename.pString)) {
        uint8_t v18[8];
        NiPointer<NiTexture> spTempTexture;
        uint8_t* v55 = v18;
        uint8_t* v36 = v18;
        uint8_t* v25 = v18;
        char* apFilename = axFilename.pString;
        // Passes a null texture by value as the fifth argument.
        TileImage::AddToTesTextures(axFilename.pString, fscale, spTempTexture, 0.0f, NiPointer<NiTexture>());
        ptex = spTempTexture.m_pObject;
    }

    if (ptex) {
        float v53 = 0.0f;
        float v54 = 0.0f;
        ptextures->x = 0.0f;
        ptextures->y = 0.0f;
        int64_t v24 = ptex->GetHeight();
        float v33 = fscale * static_cast<double>(aiHeight) / static_cast<double>(v24);
        float v51 = 0.0f;
        ptextures[1].x = 0.0f;
        ptextures[1].y = v33;
        int64_t v23 = ptex->GetWidth();
        float v32 = fscale * static_cast<double>(aiWidth) / static_cast<double>(v23);
        float v50 = 0.0f;
        ptextures[2].x = v32;
        ptextures[2].y = 0.0f;
        int64_t v22 = ptex->GetWidth();
        float v30 = fscale * static_cast<double>(aiWidth) / static_cast<double>(v22);
        int64_t v21 = ptex->GetHeight();
        float v31 = fscale * static_cast<double>(aiHeight) / static_cast<double>(v21);
        ptextures[3].x = v30;
        ptextures[3].y = v31;
    }
    *pvert_order = 0;
    pvert_order[1] = 1;
    pvert_order[2] = 2;
    pvert_order[3] = 2;
    pvert_order[4] = 1;
    pvert_order[5] = 3;
    void* apMemory = NiMemObject::operator new(sizeof(BSScissorTriShape));
    if (apMemory) {
        pshape = (NiTriShape*)::new (apMemory) BSScissorTriShape(
            4, pvertices, nullptr, nullptr, ptextures, 1, NiGeometryData::NBT_METHOD_NONE, 2, pvert_order, 0, 0, 0, 0
        );
    }
    else {
        pshape = nullptr;
    }
    TileShaderProperty* v43 = static_cast<TileShaderProperty*>(NiMemObject::operator new(sizeof(TileShaderProperty)));
    if (v43) {
        ptsp = ::new (v43) TileShaderProperty(false);
    }
    else {
        ptsp = nullptr;
    }
    ptsp->SetTileTexture(ptex);
    pshape->AttachProperty(ptsp);
    NiD3DShader* pshader = (NiD3DShader*)BSShaderManager::GetShader(32);
    pshape->m_spShader = static_cast<BSShader*>(pshader);
    pshape->UpdateProperties();
    axFilename.Set(nullptr, 0);
    return pshape;
}

void FontManager::Init() {
    FontManager* v0;
    void* v1;
    if (!FontManager::pInstance) {
        v1 = (&MemoryManager_Singleton)->Allocate(sizeof(FontManager));
        if (v1) {
            v0 = ::new (v1) FontManager;
        }
        else {
            v0 = nullptr;
        }
        FontManager::pInstance = v0;
    }
}

FontManager* FontManager::Instance() {
    return FontManager::pInstance;
}

void FontManager::Free() {
    FontManager* block;
    if (FontManager::pInstance) {
        block = FontManager::pInstance;
        FontManager::pInstance->~FontManager();
        (&MemoryManager_Singleton)->Deallocate(block);
        FontManager::pInstance = nullptr;
    }
}
