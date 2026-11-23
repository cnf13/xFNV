#include "BSMenu/Font.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
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
#include <limits>

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
    AutoMemContext kMemContext(MEM_CONTEXT_FONTS);
    if (apFilename) {
        size_t stLength = std::strlen(apFilename);
        if (stLength) {
            pFontFile = static_cast<char*>(MemoryManager::Instance().Allocate(stLength + 1));
            strcpy_s(pFontFile, stLength + 1, apFilename);
        }
        iFontNum = aiFontNum;
        if (abLoad) {
            Load();
        }
    }
}

Font::~Font() {
    MemoryManager::Instance().Deallocate(fontBuffer);
    fontBufferSize = 0;
    fontBuffer = nullptr;
    KillData();
    MemoryManager::Instance().Deallocate(pFontFile);
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
        aiHeight = std::numeric_limits<int32_t>::max();
    }
    if (!aiLineEnd) {
        aiLineEnd = std::numeric_limits<int32_t>::max();
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
        apIconShape->m_spModelData->SetConsistency(NiGeometryData::STATIC);
    }

    const float fstart_x = xpos.x;
    int32_t iverts = 0;
    int32_t iiconIndex = 0;
    // Each character is read as pString[pString ? a : 0].
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

    NiPoint3* ptemp = apTextShape->m_spModelData->m_pkVertex;
    uint16_t usVertices = apTextShape->m_spModelData->m_usVertices;
    apTextShape->m_spModelData->m_kBound.ComputeFromData(usVertices, ptemp);
    if (apIconShape) {
        ptemp = apIconShape->m_spModelData->m_pkVertex;
        usVertices = apIconShape->m_spModelData->m_usVertices;
        apIconShape->m_spModelData->m_kBound.ComputeFromData(usVertices, ptemp);
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
        axData.iWidth = std::numeric_limits<int32_t>::max();
    }
    if (axData.iHeight <= 0) {
        axData.iHeight = std::numeric_limits<int32_t>::max();
    }
    if (axData.iLineEnd <= 0) {
        axData.iLineEnd = std::numeric_limits<int32_t>::max();
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
    char* psrc = static_cast<char*>(MemoryManager::Instance().Allocate(isrc_len + 4));
    std::memset(psrc, 0, isrc_len + 4);
    char* pdest = static_cast<char*>(MemoryManager::Instance().Allocate(isrc_len + 4));
    std::memset(pdest, 0, isrc_len + 4);
    std::sprintf(psrc, "%s", apOrigString);
    uint32_t idest_len = 0;
    uint32_t idest_max = isrc_len + 4;
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
            while (psrc[a + c + b] &&
                   c < 127 &&
                   psrc[a + c] != ';' &&
                   psrc[a + c] != '\n' &&
                   psrc[a + c] != axData.cLineSep) {
                ptemp[c] = psrc[a + c + b];
                ++c;
            }
            // "&-x" ending the text leaves c = 1 < b: this writes ptemp[-1] and strlen reads ptemp's uninitialised
            // bytes. Malformed input only; kept deliberately.
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
                    pdest = static_cast<char*>(MemoryManager::Instance().Reallocate(pdest, idest_max));
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
        psrc = static_cast<char*>(MemoryManager::Instance().Reallocate(psrc, isrc_len + 4));
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
                pdest = static_cast<char*>(MemoryManager::Instance().Reallocate(pdest, idest_len + 4));
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
                        pdest = static_cast<char*>(MemoryManager::Instance().Reallocate(pdest, idest_max));
                        std::memmove(pdest + iindex + 2, pdest + iindex, idest_len - iindex);
                        pdest[iindex + 1] = '\n';
                        pdest[iindex] = '-';
                        idest_len += 2;
                        pfont_letter = &pFontData->pFontLetters['-'];
                        fheight = pFontData->fBaseLine + flinePadding + fheight;
                        iwidth = iwidth - static_cast<int32_t>(pfont_letter->fWidth + pfont_letter->fSpacing);
                        axData.xLineWidths.AddTail(iwidth);
                        imax_width = imax_width > iwidth ? imax_width : iwidth;
                        iindex = 0;
                        ++iline_count;
                        // iindex is 0 here, so this reads the byte before pdest (as a signed char).
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
                        pdest = static_cast<char*>(MemoryManager::Instance().Reallocate(pdest, idest_len + 6));
                        idest_max = idest_len + 6;
                    }
                    pdest[idest_len + 2] = pdest[idest_len];
                    pdest[idest_len + 1] = pdest[idest_len - 1];
                    pdest[idest_len] = '\n';
                    pdest[idest_len - 1] = '-';
                    idest_len += 2;
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
                pdest = static_cast<char*>(MemoryManager::Instance().Reallocate(pdest, idest_len + 4));
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
    MemoryManager::Instance().Deallocate(psrc);
    MemoryManager::Instance().Deallocate(pdest);
}

// Allocates aiChars quads (4 vertices, 2 triangles each) for a scissored shape named "Interface: Text", tinted
// axColor through a tile shader with the font's first base texture.
NiTriShape* Font::MakeTriShape(int32_t aiChars, const NiColorA& axColor, bool abPrepareObject) {
    const int32_t iverts = aiChars * 4;
    NiPoint3* pvertices = new NiPoint3[static_cast<uint32_t>(iverts)];
    NiPoint2* ptextures = new NiPoint2[static_cast<uint32_t>(iverts)];
    uint16_t* pvert_order = static_cast<uint16_t*>(_NiMalloc(aiChars * 6 * sizeof(uint16_t)));

    NiTriShape* pshape = new BSScissorTriShape(
        static_cast<uint16_t>(iverts),
        pvertices,
        nullptr,
        nullptr,
        ptextures,
        1,
        NiGeometryData::NBT_METHOD_NONE,
        static_cast<uint16_t>(aiChars * 2),
        pvert_order,
        0,
        0,
        0,
        0
    );
    NiGeometryData* pd = pshape->m_spModelData.m_pObject;
    pd->m_ucKeepFlags = 0;
    pd->m_ucCompressFlags = 1;

    NiGlobalStringTable::GlobalStringHandle kName = NiGlobalStringTable::AddString("Interface: Text");
    pshape->SetName(reinterpret_cast<const NiFixedString&>(kName));
    if (kName != NiGlobalStringTable::NULL_STRING) {
        InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kName) - 8));
    }
    pshape->m_spModelData->SetConsistency(NiGeometryData::STATIC);

    TileShaderProperty* ptsp = new TileShaderProperty(false);
    pshape->AttachProperty(ptsp);
    ptsp->TintColor = axColor;
    if (abPrepareObject) {
        BSShaderManager::PrepareObject(pshape, false, false);
    }
    NiTexturingProperty::Map* pMap = pTextureData[0]->m_kMaps.GetAt(0);
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
    char pcString[268];
    char pcDest[268];
    uint32_t iSize = ButtonIcons.iSize;
    uint32_t iverts = 4 * iSize;
    pvertices = new NiPoint3[4 * iSize];
    ptextures = new NiPoint2[iverts];
    uint16_t* pvert_order = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t) * ButtonIcons.iSize));
    pshape = new BSScissorTriShape(
        iverts,
        pvertices,
        nullptr,
        nullptr,
        ptextures,
        1,
        NiGeometryData::NBT_METHOD_NONE,
        2 * ButtonIcons.iSize,
        pvert_order,
        0,
        0,
        0,
        0
    );
    NiTriShapeData* pd = static_cast<NiTriShapeData*>(pshape->m_spModelData.m_pObject);
    pd->m_ucKeepFlags = 0;
    pd->m_ucCompressFlags = NiGeometryData::VERTEX_MASK;
    pshape->SetName("Interface: TextIcons");
    pshape->m_spModelData->SetConsistency(NiGeometryData::STATIC);
    ptsp = new TileShaderProperty(false);
    pshape->AttachProperty(ptsp);
    ptsp->TintColor = NiColorA::WHITE;
    BSShaderManager::PrepareObject(pshape, 0, 0);
    strncpy_s(pcDest, 260u, IconAtlasTextureName.pString, 259u);
    BSsprintf(pcString, 260, "Data\\Textures\\%s", pcDest);
    NiSourceTexture* pkTexture = NiSourceTexture::Create(pcString, NiSourceTexture::ms_kDefaultPrefs, true, false);
    ptsp->SetTileTexture(pkTexture);
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
    NiPoint3* pvertices = &apShape->m_spModelData->m_pkVertex[iindex];
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

    NiPoint2* ptextures = &apShape->m_spModelData->m_pkTexture[iindex];
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
    float fV0;
    float fV1;
    float fY0;
    float fWidth;
    fWidth = ButtonIcons.pBuffer[aiIconIndex].fWidth;
    float fTop = aPos.z + ButtonIcons.pBuffer[aiIconIndex].fZOffset;
    float fLeft = aPos.x + ButtonIcons.pBuffer[aiIconIndex].fXOffset;
    fY0 = aPos.y;
    float fBottom = fTop - fWidth;
    float fRight = fWidth + fLeft;
    NiPoint3* pvertices = &apShape->m_spModelData->m_pkVertex[4 * aiIconIndex];
    pvertices[0].x = fLeft;
    pvertices[0].y = fY0;
    pvertices[0].z = fTop;
    float fY1 = aPos.y;
    pvertices[1].x = fLeft;
    pvertices[1].y = fY1;
    pvertices[1].z = fBottom;
    float fY2 = aPos.y;
    pvertices[2].x = fRight;
    pvertices[2].y = fY2;
    pvertices[2].z = fTop;
    float fY3 = aPos.y;
    pvertices[3].x = fRight;
    pvertices[3].y = fY3;
    pvertices[3].z = fBottom;
    ButtonIcon* pIcon = &ButtonIcons.pBuffer[aiIconIndex];
    fV0 = pIcon->UVCoords.m_top;
    NiPoint2* ptextures = &apShape->m_spModelData->m_pkTexture[4 * aiIconIndex];
    ptextures[0].x = pIcon->UVCoords.m_left;
    ptextures[0].y = fV0;
    fV1 = ButtonIcons.pBuffer[aiIconIndex].UVCoords.m_bottom;
    ptextures[1].x = ButtonIcons.pBuffer[aiIconIndex].UVCoords.m_left;
    ptextures[1].y = fV1;
    float fV2 = ButtonIcons.pBuffer[aiIconIndex].UVCoords.m_top;
    ptextures[2].x = ButtonIcons.pBuffer[aiIconIndex].UVCoords.m_right;
    ptextures[2].y = fV2;
    float fV3 = ButtonIcons.pBuffer[aiIconIndex].UVCoords.m_bottom;
    ptextures[3].x = ButtonIcons.pBuffer[aiIconIndex].UVCoords.m_right;
    ptextures[3].y = fV3;
    uint16_t* pvert_order = &static_cast<NiTriShapeData*>(apShape->m_spModelData.m_pObject)
                                 ->NiTriShapeData::GetTriList()[6 * aiIconIndex];
    pvert_order[0] = static_cast<uint16_t>(4 * aiIconIndex);
    pvert_order[1] = static_cast<uint16_t>((4 * aiIconIndex) + 1);
    pvert_order[2] = static_cast<uint16_t>((4 * aiIconIndex) + 2);
    pvert_order[3] = static_cast<uint16_t>((4 * aiIconIndex) + 2);
    pvert_order[4] = static_cast<uint16_t>((4 * aiIconIndex) + 1);
    pvert_order[5] = static_cast<uint16_t>((4 * aiIconIndex) + 3);
    aPos.x = ((fWidth + ButtonIcons.pBuffer[aiIconIndex].fSpacing) + aPos.x);
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
    uint32_t uiIndex = ButtonIcons.AddUninitialized();
    ButtonIcons._ConstructItems(&ButtonIcons.pBuffer[uiIndex], 1u);
    ButtonIcons.pBuffer[uiIndex] = iconData;
}

void Font::KillData() {
    int32_t i;
    for (i = 0; pFontData && (i < pFontData->iTextureCount); ++i) {
        pTextureData[i] = nullptr;
    }
    MemoryManager::Instance().Deallocate(pFontData);
    pFontData = nullptr;
}

void Font::Unload() {
    if (iRefCount && (!(--iRefCount))) {
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
    BSFile* pFile = static_cast<BSFile*>(
        FileFinder::GetFile(pFontFile, NiFile::READ_ONLY, 0x4000, ARCHIVE_TYPE_TEXTURES)
    );
    if (!pFile || !pFile->m_bGood) {
        Warning("DEFAULT: Font file not found: %s. \n", pFontFile);
        delete pFile;
        return;
    }
    pFontData = static_cast<FontData*>(MemoryManager::Instance().Allocate(sizeof(FontData)));
    uint32_t uiFileSize = pFile->GetSize();
    pFile->NiBinaryStream::Read(pFontData, uiFileSize);
    delete pFile;

    fFontHeight = 0.0f;
    float fMaxHeight = 0.0f;
    fMaxDrop = 0.0f;
    for (const FontLetter& rLetter : pFontData->pFontLetters) {
        float fLetterHeight = pFontData->fBaseLine - rLetter.fTopEdge;
        fLetterHeight = fLetterHeight + rLetter.fHeight;
        // The asm keeps the new value unless it is below the old one (an unordered compare keeps it too).
        float fLargest = fFontHeight;
        fFontHeight = fLetterHeight < fLargest ? fLargest : fLetterHeight;
        float fHeight = rLetter.fHeight;
        fMaxHeight = fHeight < fMaxHeight ? fMaxHeight : fHeight;
        // Here the new value is kept unless it is above the old one.
        float fSmallest = fMaxDrop;
        float fDrop = rLetter.fTopEdge - rLetter.fHeight;
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
            "TEXTURES: Too many font textures for %s.\nMax textures is %d yours has %d.\n",
            pFontFile,
            8,
            pFontData->iTextureCount
        );
        return;
    }
    for (int32_t j = 0; j < pFontData->iTextureCount; ++j) {
        char acTextureName[256];
        BSsprintf(
            acTextureName,
            sizeof(acTextureName),
            "TEXTURES\\FONTS\\%s.TEX",
            pFontData->pTextureFiles[j].pFilename
        );
        BSFile* pTexFile = static_cast<BSFile*>(
            FileFinder::GetFile(acTextureName, NiFile::READ_ONLY, 0x4000, ARCHIVE_TYPE_TEXTURES)
        );
        if (!pTexFile || !pTexFile->m_bGood) {
            // The format deliberately uses %S (a wide string) for the narrow name.
            Warning("DEFAULT: Font file not found: %S. \n", acTextureName);
            delete pTexFile;
            return;
        }
        // The texture's width and height.
        uint32_t auiSize[2];
        pTexFile->NiBinaryStream::Read(auiSize, sizeof(auiSize));
        NiTexture::FormatPrefs kPrefs;
        NiPixelData* pPixelData = new NiPixelData(auiSize[0], auiSize[1], NiPixelFormat::RGBA32, 1, 1);
        uint32_t uiBytes = auiSize[0] * auiSize[1] * 4;
        uint8_t* pPixels = pPixelData->GetPixels(0, 0);
        pTexFile->NiBinaryStream::Read(pPixels, uiBytes);
        pPixelData->bNoConvert = true;
        NiTexturingProperty* pTexProp = new NiTexturingProperty(pPixelData, pFontFile, kPrefs);
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
        double dOffset = pFontData->pFontLetters[32].fHeight - pFontData->fBaseLine;
        xpos.z = xpos.z - (dOffset + dOffset);
    }
    for (itext_count = 0;; ++itext_count) {
        uint32_t uiLength = apTextString.sLen == 0xFFFF ? std::strlen(apTextString.pString) : apTextString.sLen;
        if (itext_count >= uiLength || !apTextString.pString[apTextString.pString != nullptr ? itext_count : 0]) {
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
    NiColorA pspecial_color(0.0f, 0.0f, 1.0f, 1.0f);
    aiWidth = 0;
    float fstart_x = xpos.x;
    int32_t iverts = 0;
    for (uint32_t a = 0; apTextString.pString[apTextString.pString != nullptr ? a : 0]; ++a) {
        int32_t iNewWidth;
        uint8_t ch[5];
        if (apTextString.pString[apTextString.pString != nullptr ? a : 0] == 3) {
            pcurrent_color = nullptr;
        }
        char cChar = apTextString.pString[apTextString.pString != nullptr ? a : 0];
        if (cChar == 9) {
            float fTabOffset = std::fmod(static_cast<double>(xpos.x), static_cast<double>(75.0f));
            xpos.x = 75.0 - fTabOffset + xpos.x;
        }
        else {
            if (cChar == 10) {
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
            iNewWidth = aiWidth;
        }
        else {
            iNewWidth = static_cast<int32_t>(xpos.x - fstart_x);
        }
        aiWidth = iNewWidth;
        if (apTextString.pString[apTextString.pString != nullptr ? a : 0] == 2) {
            pcurrent_color = &pspecial_color;
        }
    }
    NiPoint3* ptemp = pflat->m_spModelData->m_pkVertex;
    pflat->m_spModelData->m_kBound.ComputeFromData(pflat->m_spModelData->m_usVertices, ptemp);
    return pflat;
}

void Font::GetLineOffsets(const char* apTextString, float& afX, float& afY, int32_t aiFlags, uint32_t aiStart) {
    float fMaxWidth;
    NiPoint3 kDimensions;
    if (!(afX > 0.0)) {
        fMaxWidth = 0x1.fffffe0000000p+127f;
    }
    else {
        fMaxWidth = afX;
    }
    float fWrapWidth = fMaxWidth;
    int32_t iJustify = static_cast<int8_t>(aiFlags) & 15;
    if (iJustify == 2) {
        kDimensions = FontManager::pInstance->CalculateStringDimensions(apTextString, iFontNum, fWrapWidth, aiStart);
        afX = static_cast<float>((-kDimensions.x) / 2.0);
    }
    else {
        if (iJustify == 4) {
            kDimensions = FontManager::pInstance
                              ->CalculateStringDimensions(apTextString, iFontNum, fWrapWidth, aiStart);
            afX = (-kDimensions.x);
        }
        else {
            afX = 0.0f;
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
    int32_t iLine = 1;
    int32_t iStart = -1;
    int32_t iEnd = -1;
    if (axTextString.sLen == 0xFFFF) {
        sLen = std::strlen(axTextString.pString);
    }
    else {
        sLen = axTextString.sLen;
    }
    for (size_t i = 0; i < sLen && axTextString.pString[axTextString.pString != nullptr ? i : 0]; ++i) {
        if (iLine == aiLine && iStart == -1) {
            iStart = i;
        }
        if (axTextString.pString[axTextString.pString != nullptr ? i : 0] == 10) {
            ++iLine;
        }
        if (iLine == aiLine + 1) {
            iEnd = i;
            break;
        }
    }
    if (iStart || iEnd != -1) {
        if (iStart == -1) {
            axTextString.Set(" ", 0);
        }
        else {
            uint32_t j;
            if (iEnd == -1) {
                iEnd = sLen;
            }
            void* pBuffer = MemoryManager::Instance().Allocate(sizeof(char) * static_cast<size_t>(iEnd - iStart + 1));
            std::memset(pBuffer, 0, iEnd - iStart + 1);
            for (j = 0; j < iEnd - iStart; ++j) {
                *(static_cast<uint8_t*>(pBuffer) +
                  j) = axTextString.pString[axTextString.pString != nullptr ? iStart + j : 0];
            }
            *(static_cast<uint8_t*>(pBuffer) + j) = 0;
            axTextString.Set(" ", 0);
            axTextString.Set(static_cast<char*>(pBuffer), 0);
            MemoryManager::Instance().Deallocate(pBuffer);
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
    int32_t iFinalWidth;
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
        char cChar = axTextString.pString[axTextString.pString != nullptr ? n : 0];
        if (cChar == 9) {
            iwidth += 75 - iwidth % 75;
        }
        else {
            if (cChar == 10) {
                int32_t iNewMaxWidth;
                axTextString.pString[axTextString.pString != nullptr ? n : 0] = acEolChar;
                iheight += static_cast<int32_t>(pFontData->fBaseLine);
                if (imax_width <= iwidth) {
                    iNewMaxWidth = iwidth;
                }
                else {
                    iNewMaxWidth = imax_width;
                }
                imax_width = iNewMaxWidth;
                iwidth = 0;
                iindex = 0;
                ++iline_count;
            }
            else {
                if (cChar == 32) {
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
                        int32_t iNewMaxWidth;
                        axTextString.pString[axTextString.pString != nullptr ? iindex : 0] = acEolChar;
                        iheight += static_cast<int32_t>(pFontData->fBaseLine);
                        if (imax_width <= iindex_width) {
                            iNewMaxWidth = iindex_width;
                        }
                        else {
                            iNewMaxWidth = imax_width;
                        }
                        imax_width = iNewMaxWidth;
                        iindex = 0;
                        ++iline_count;
                        iwidth -= iindex_width;
                    }
                    else {
                        int32_t iNewMaxWidth;
                        uint32_t uiLength;
                        if (axTextString.sLen == 0xFFFF) {
                            uiLength = std::strlen(axTextString.pString);
                        }
                        else {
                            uiLength = axTextString.sLen;
                        }
                        if (uiLength >= axTextString.sMaxLen) {
                            size_t stLength;
                            if (axTextString.sLen == 0xFFFF) {
                                stLength = std::strlen(axTextString.pString);
                            }
                            else {
                                stLength = axTextString.sLen;
                            }
                            if (stLength + 1 < axTextString.sMaxLen) {
                                size_t stNewLength;
                                axTextString += "\n";
                                if (axTextString.sLen == 0xFFFF) {
                                    stNewLength = std::strlen(axTextString.pString);
                                }
                                else {
                                    stNewLength = axTextString.sLen;
                                }
                                axTextString.pString[axTextString.pString != nullptr ? stNewLength - 1 : 0] = 0;
                            }
                            else {
                                size_t stCurLength;
                                size_t stNewLength;
                                axTextString += "\n\n";
                                if (axTextString.sLen == 0xFFFF) {
                                    stNewLength = std::strlen(axTextString.pString);
                                }
                                else {
                                    stNewLength = axTextString.sLen;
                                }
                                axTextString.pString[axTextString.pString != nullptr ? stNewLength - 1 : 0] = 0;
                                if (axTextString.sLen == 0xFFFF) {
                                    stCurLength = std::strlen(axTextString.pString);
                                }
                                else {
                                    stCurLength = axTextString.sLen;
                                }
                                axTextString.pString[axTextString.pString != nullptr ? stCurLength - 2 : 0] = 0;
                            }
                        }
                        for (uint32_t a = sLen + 2; a > n; --a) {
                            axTextString.pString
                                [axTextString.pString != nullptr
                                     ? a
                                     : 0] = axTextString.pString[axTextString.pString != nullptr ? a - 2 : 0];
                        }
                        axTextString.pString[axTextString.pString != nullptr ? n - 1 : 0] = 45;
                        axTextString.pString[axTextString.pString != nullptr ? n : 0] = acEolChar;
                        iheight += static_cast<int32_t>(pFontData->fBaseLine);
                        if (imax_width <=
                            iwidth - static_cast<int32_t>(pfont_letter->fSpacing + pfont_letter->fWidth)) {
                            iNewMaxWidth = iwidth - static_cast<int32_t>(pfont_letter->fSpacing + pfont_letter->fWidth);
                        }
                        else {
                            iNewMaxWidth = imax_width;
                        }
                        imax_width = iNewMaxWidth;
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
    if ((*axTextString.pString) && (n >= sLen)) {
        ++iline_count;
    }

    if (imax_width <= iwidth) {
        iFinalWidth = iwidth;
    }
    else {
        iFinalWidth = imax_width;
    }
    aiWidth = iFinalWidth;
    aiHeight = iheight;
    return iline_count;
}

void Font::GetLineNumber(char* apSrcString, BSStringT<char>& axDestString, int32_t aiLineNum, char acSepChar) {
    int32_t i;
    char* apString = apSrcString;
    int32_t iLine = 0;
    for (i = 0; apSrcString[i] && iLine != aiLineNum; ++i) {
        if (apSrcString[i] == acSepChar) {
            apString = &apSrcString[i];
            ++iLine;
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
    FontManager::CharData* pnew = new FontManager::CharData(iFontIndex, cChar, xColor, iJustification);
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
    cChar = acChar;
    if (!xFilename.pString || !*xFilename.pString) {
        double dSpacing;
        if (FontManager::pInstance->pFont[iFontIndex]->pFontData->pFontLetters[cChar].fWidth == 0.0) {
            dSpacing = 0.0;
        }
        else {
            FontLetter* pLetter = &FontManager::pInstance->pFont[iFontIndex]->pFontData->pFontLetters[cChar];
            dSpacing = pLetter->fSpacing + pLetter->fLeadingEdge;
        }
        iWidth = static_cast<int32_t>(
            FontManager::pInstance->pFont[iFontIndex]->pFontData->pFontLetters[cChar].fWidth + dSpacing
        );
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
        int32_t iWidth;
        if (!apChar->xFilename.pString || !*apChar->xFilename.pString) {
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
                if (pfirst && (apChar->iFontIndex != pfirst->iFontIndex)) {
                    bAdjustLineHeight = true;
                }

                apChar->iX = apChar->iLeadingEdge;
                this->iWidth = apChar->iWidth;
                NiTListItem<FontManager::CharData*>* elem = xChars.m_pkHead;
                while (elem) {
                    FontManager::CharData** ppElement = &elem->m_element;
                    elem = elem->m_pkNext;
                    FontManager::CharData* pchar = *ppElement;
                    pchar->iX = pchar->iLeadingEdge + this->iWidth;
                    this->iWidth += pchar->iWidth;
                }
            }
            else {
                FontManager::CharData* pfirst_2 = nullptr;
                if (xChars.m_uiCount) {
                    pfirst_2 = xChars.m_pkTail->m_element;
                }
                if (pfirst_2 && (apChar->iFontIndex != pfirst_2->iFontIndex)) {
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
            int32_t iNewDrop;
            int32_t iNewRise;
            if (abToFront) {
                apChar->iX = 0;
                this->iWidth = apChar->iWidth;
                NiTListItem<FontManager::CharData*>* elem = xChars.m_pkHead;
                while (elem) {
                    FontManager::CharData** ppElement = &elem->m_element;
                    elem = elem->m_pkNext;
                    FontManager::CharData* pchar = *ppElement;
                    pchar->iX = pchar->iLeadingEdge + this->iWidth;
                    this->iWidth += pchar->iWidth;
                }
            }
            else {
                apChar->iX = this->iWidth;
                this->iWidth += apChar->iWidth;
            }
            if (this->iRise <= apChar->iRise) {
                iNewRise = apChar->iRise;
            }
            else {
                iNewRise = this->iRise;
            }
            this->iRise = iNewRise;
            if (this->iDrop <= apChar->iDrop) {
                iNewDrop = apChar->iDrop;
            }
            else {
                iNewDrop = this->iDrop;
            }
            this->iDrop = iNewDrop;
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
            pline_4 = new FontManager::TextLine(pPage, apChar, 0, iPageWidth);
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
                pline_2 = new FontManager::TextLine(pPage, apChar, 0, iPageWidth);
                pline = pline_2;
                pline_2->iRise = aFontRise[apChar->iFontIndex];
                for (pchar = xChars.RemoveTail(); xChars.m_uiCount && pchar->cChar != 32;
                     this->iWidth -= pchar->iWidth) {
                    pline_2->AddChar(pchar, true);
                    pchar = xChars.RemoveTail();
                }
                if (pchar) {
                    pchar->xFilename.Set(nullptr, 0);
                    MemoryManager::Instance().Deallocate(pchar);
                }
            }
            else {
                FontManager::CharData* phyphen;
                FontManager::TextLine* pline_3;
                int32_t iend_width;
                FontManager::CharData* plast;
                pline_3 = new FontManager::TextLine(pPage, apChar, 0, iPageWidth);
                pline = pline_3;
                pline_3->iRise = aFontRise[apChar->iFontIndex];
                phyphen = new FontManager::CharData(0, 45, NiColorA(0.4588f, 0.2314f, 0.1294f, 1.0f), 1);
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
    for (int32_t& riChars : pCharsPerFont) {
        riChars = 0;
    }
    AddChar(apChar, 0);
}

// The page starts as high as its first line; iWidth is deliberately left unset.
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
    for (int32_t& riChars : pCharsPerFont) {
        riChars = 0;
    }
}

FontManager::TextPage::~TextPage() {
    while (xLines.m_uiCount) {
        delete xLines.RemoveHead();
    }
}

int32_t FontManager::TextPage::GetCharCountForFont(int32_t aiFont) {
    int32_t iCount = 0;
    NiTListItem<FontManager::TextLine*>* pLineItem = xLines.m_pkHead;
    while (pLineItem) {
        FontManager::TextLine** ppLine = &pLineItem->m_element;
        pLineItem = pLineItem->m_pkNext;
        if (*ppLine) {
            NiTListItem<FontManager::CharData*>* pCharItem = (*ppLine)->xChars.m_pkHead;
            while (pCharItem) {
                FontManager::CharData** ppChar = &pCharItem->m_element;
                pCharItem = pCharItem->m_pkNext;
                FontManager::CharData* pChar = *ppChar;
                if (*ppChar) {
                    if (!pChar->xFilename.pString || !*pChar->xFilename.pString) {
                        if (pChar->iFontIndex == aiFont) {
                            ++iCount;
                        }
                    }
                }
            }
        }
    }
    return iCount;
}

FontManager::TextPage* FontManager::TextPage::AddChar(FontManager::CharData* apChar, int32_t aiNewLines) {
    FontManager::TextLine* pline = nullptr;
    int32_t iskipped = 0;
    while (aiNewLines > 1) {
        iskipped += iLastFontHeight;
        --aiNewLines;
    }
    if (!xLines.m_uiCount || aiNewLines) {
        pline = new FontManager::TextLine(this, apChar, iskipped, iPageWidth);
    }
    else {
        pline = xLines.m_pkTail->m_element->AddChar(apChar, false);
    }
    if (apChar) {
        if (!apChar->xFilename.pString || !*apChar->xFilename.pString) {
            FontLetter* pLetter = &FontManager::pInstance->pFont[apChar->iFontIndex]
                                       ->pFontData->pFontLetters[apChar->cChar];
            iLastFontHeight = static_cast<int32_t>(pLetter->fHeight - pLetter->fTopEdge) +
                static_cast<int32_t>(FontManager::pInstance->pFont[apChar->iFontIndex]->pFontData->fBaseLine);
        }
    }
    if (pline) {
        FontManager::TextPage* ppage = this;
        if (pline->iSkippedSpace + (pline->iRise + iHeight) <= iPageHeight) {
            xLines.AddTail(pline);
            iHeight += pline->iSkippedSpace + pline->iRise;
        }
        else {
            FontManager::TextPage* ppage_2;
            pline->iSkippedSpace = 0;
            ppage_2 = new FontManager::TextPage(pDoc, pline, iPageWidth, iPageHeight);
            ppage = ppage_2;
            ppage_2->iHeight = pline->iRise;
            pline->pPage = ppage_2;
        }
        if (!apChar->xFilename.pString || !*apChar->xFilename.pString) {
            ++ppage->pCharsPerFont[apChar->iFontIndex];
        }
        if (ppage != this) {
            return ppage;
        }
    }
    else {
        if (apChar) {
            if (!apChar->xFilename.pString || !*apChar->xFilename.pString) {
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
        ppage = new FontManager::TextPage(this, apChar, iPageWidth, iPageHeight);
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
    NiTriShape* ptri_2;
    NiTListItem<FontManager::TextPage*>* i;
    int32_t a = iPageNum;
    int32_t iiconIndex = 0;
    NiTListItem<FontManager::TextPage*>* pPageHead = xPages.m_pkHead;
    for (i = pPageHead; i; i = i->m_pkNext) {
        if (!a--) {
            break;
        }
    }
    if (i) {
        int32_t iImageHeight;
        int32_t ptri_index[8];
        NiTriShape* pflats[8];
        FontManager::TextPage* ppage = i->m_element;
        for (uint32_t uiFont = 0; uiFont < 8; ++uiFont) {
            ptri_index[uiFont] = 0;
            pflats[uiFont] = nullptr;
            int32_t ichar_count = ppage->GetCharCountForFont(uiFont);
            if (ichar_count > 0) {
                NiColorA axColor(0.45882353f, 0.23137255f, 0.12941177f, 1.0f);
                Font* pShapeFont = FontManager::pInstance->pFont[uiFont];
                NiTriShape* TriShape = pShapeFont->MakeTriShape(ichar_count, axColor, true);
                pflats[uiFont] = TriShape;
                NiTriShape* pkFlat = pflats[uiFont];
                pkFlat->m_kLocal.m_Translate = NiPoint3::ZERO;
            }
        }
        NiTList<NiTriShape*> ximages;
        NiTriShape* pIconShape = nullptr;
        Font* pFirstFont = FontManager::pInstance->pFont[0];
        if (pFirstFont->ButtonIcons.iSize) {
            Font* pIconFont = FontManager::pInstance->pFont[0];
            pIconShape = pIconFont->MakeIconsTriShape();
            pIconShape->m_kLocal.m_Translate = NiPoint3::ZERO;
            ximages.AddHead(pIconShape);
        }
        if (ppage) {
            int32_t iline = 0;
            NiPoint3 xpos = NiPoint3::ZERO;
            NiTListItem<FontManager::TextLine*>* pLineHead = ppage->xLines.m_pkHead;
            NiTListItem<FontManager::TextLine*>* xelem = pLineHead;
            while (xelem) {
                int32_t iLines = apData->iLines;
                if (iline >= iLines) {
                    break;
                }
                FontManager::TextLine** ppLine = &xelem->m_element;
                xelem = xelem->m_pkNext;
                FontManager::TextLine* pline = *ppLine;
                xpos.x = 0.0f;
                int32_t izone = 0;
                if (apData->bIsHypertext) {
                    izone = this->iPageWidth;
                }
                if (pline->iJustify == 4) {
                    int32_t iOffset = izone - pline->iWidth;
                    xpos.x = static_cast<float>(iOffset);
                }
                if (pline->iJustify == 2) {
                    int32_t iCenterOffset = (izone - pline->iWidth) / 2;
                    xpos.x = static_cast<float>(iCenterOffset);
                }
                xpos.y = 0.0f;
                int32_t iLineHeight = pline->iSkippedSpace + pline->iRise;
                xpos.z = xpos.z - static_cast<double>(iLineHeight);
                NiTListItem<FontManager::CharData*>* pCharHead = pline->xChars.m_pkHead;
                NiTListItem<FontManager::CharData*>* xelem2 = pCharHead;
                while (xelem2) {
                    FontManager::CharData** ppChar = &xelem2->m_element;
                    xelem2 = xelem2->m_pkNext;
                    FontManager::CharData* pchar = *ppChar;
                    Font* pfont = FontManager::pInstance->pFont[pchar->iFontIndex];
                    char* pString = pchar->xFilename.pString;
                    if (!pString || !*pchar->xFilename.pString) {
                        NiPoint3 xpos2 = xpos;
                        xpos2.x = static_cast<double>(pchar->iX) + xpos.x;
                        if (pchar->cChar == 1) {
                            Font* pIconFont = FontManager::pInstance->pFont[0];
                            pIconFont->AddIcon(iiconIndex++, pIconShape, xpos2);
                        }
                        else {
                            int32_t aiVert = ptri_index[pchar->iFontIndex]++;
                            pfont->AddChar(
                                &pfont->pFontData->pFontLetters[pchar->cChar],
                                aiVert,
                                pflats[pchar->iFontIndex],
                                xpos2,
                                &pchar->xColor
                            );
                        }
                    }
                    else {
                        xpos.x = static_cast<double>(pchar->iX) + xpos.x;
                        iImageHeight = pchar->iDrop + pchar->iRise;
                        int32_t iWidth = pchar->iWidth;
                        NiPoint3 kImagePos = xpos;
                        NiTriShape* ptri = FontManager::pInstance
                                               ->MakeImage(pchar->xFilename, kImagePos, iWidth, iImageHeight);
                        ptri_2 = ptri;
                        if (ptri) {
                            ximages.AddTail(ptri_2);
                        }
                    }
                }
            }
            for (uint32_t b = 0; b < 8; ++b) {
                if (pflats[b]) {
                    NiUpdateData UpdateData;
                    NiGeometryData* pkVertexData = pflats[b]->m_spModelData.m_pObject;
                    NiPoint3* ptemp = pkVertexData->m_pkVertex;
                    NiGeometryData* pkCountData = pflats[b]->m_spModelData.m_pObject;
                    uint16_t usVertices = pkCountData->m_usVertices;
                    NiGeometryData* pkBoundData = pflats[b]->m_spModelData.m_pObject;
                    pkBoundData->m_kBound.ComputeFromData(usVertices, ptemp);
                    NiTriShape* pkFlat = pflats[b];
                    pkFlat->m_kLocal.m_Translate = NiPoint3::ZERO;
                    pflats[b]->UpdateProperties();
                    UpdateData.fTime = 0.0f;
                    UpdateData.bUpdateControllers = false;
                    UpdateData.bParallelUpdate = false;
                    UpdateData.bFoundParticles = false;
                    UpdateData.bFoundMorphController = false;
                    UpdateData.bSceneGraphChange = false;
                    pflats[b]->Update(UpdateData);
                    apParent->AttachChild(pflats[b], 1);
                }
            }
            while (true) {
                NiUpdateData UpdateData;
                if (!ximages.m_uiCount) {
                    break;
                }
                NiTriShape* ptri = ximages.RemoveHead();
                NiGeometryData* pkVertexData = ptri->m_spModelData.m_pObject;
                NiPoint3* ptemp = pkVertexData->m_pkVertex;
                NiGeometryData* pkCountData = ptri->m_spModelData.m_pObject;
                uint16_t usVertices = pkCountData->m_usVertices;
                NiGeometryData* pkBoundData = ptri->m_spModelData.m_pObject;
                pkBoundData->m_kBound.ComputeFromData(usVertices, ptemp);
                ptri->m_kLocal.m_Translate = NiPoint3::ZERO;
                ptri->UpdateProperties();
                UpdateData.fTime = 0.0f;
                UpdateData.bUpdateControllers = false;
                UpdateData.bParallelUpdate = false;
                UpdateData.bFoundParticles = false;
                UpdateData.bFoundMorphController = false;
                UpdateData.bSceneGraphChange = false;
                ptri->Update(UpdateData);
                apParent->AttachChild(ptri, 1);
            }
        }
        Font* pIconFont = FontManager::pInstance->pFont[0];
        iImageHeight = 1;
        pIconFont->ButtonIcons.Clear(true);
        ximages.RemoveAll();
    }
}

FontManager::FontManager() {
    AutoMemContext kMemContext(MEM_CONTEXT_FONTS);
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
    for (Font* pkFont : pFont) {
        if (pkFont->pFontData) {
            bHasFontData = true;
            break;
        }
    }
    if (!bHasFontData) {
        InitializationFlow("Missing all font data. The program will be terminated.");
    }
}

FontManager::~FontManager() {
    for (Font*& rpFont : pFont) {
        delete rpFont;
        rpFont = nullptr;
    }
}

bool FontManager::CreateText(BSStringT<char> axTextString, FontManager::TextData& axTextData, NiNode* apNode) {
    BSStringT<char> xtext(axTextString);
    FontManager::TextDoc* pdocument = PrepText(xtext, axTextData);
    if (pdocument) {
        pdocument->Render(apNode, &axTextData);
        delete pdocument;
    }
    xtext.Set(nullptr, 0);
    axTextString.Set(nullptr, 0);
    return true;
}

NiPoint3
FontManager::CalculateStringDimensions(const char* apText, int32_t aiFont, float afWrapwidth, int32_t aiStartIndex) {
    if (aiFont >= 1 && (aiFont <= 8 && apText)) {
        float fMaxWidth;
        NiPoint3 kDimensions = NiPoint3::ZERO;
        int32_t iLength = std::strlen(apText);
        FontLetter* pFontLetters = pFont[aiFont - 1]->pFontData->pFontLetters;
        float fBreakWidth = 0.0f;
        float fLineWidth = 0.0f;
        float LinePadding = GetLinePadding(aiFont);
        float fPrevWidth = 0.0f;
        char cHyphenBreak = 0;
        int32_t iLineCount = 1;
        kDimensions.y = pFontLetters[32].fHeight;
        for (int32_t i = aiStartIndex; i < iLength; ++i) {
            uint8_t acChar = apText[i];
            float fCharWidth = 0.0f;
            Font::ConvertCharacter(acChar);
            fCharWidth = pFontLetters[acChar].fLeadingEdge + pFontLetters[acChar].fWidth +
                pFontLetters[acChar].fSpacing;
            switch (acChar) {
                case 9: {
                    float fTabOffset = std::fmod(static_cast<double>(fLineWidth), static_cast<double>(75.0f));
                    fCharWidth = 75.0 - fTabOffset;
                    break;
                }
                case 10:
                case 32: {
                    fBreakWidth = fLineWidth;
                    cHyphenBreak = 0;
                    break;
                }
                case 126: {
                    fBreakWidth = fLineWidth + pFontLetters[45].fLeadingEdge + pFontLetters[45].fWidth +
                        pFontLetters[45].fSpacing;
                    cHyphenBreak = 1;
                    break;
                }
                default: {
                    break;
                }
            }
            if (acChar != 126) {
                fLineWidth = fLineWidth + fCharWidth;
            }
            if (afWrapwidth < static_cast<double>(fLineWidth) || acChar == 10) {
                float x;
                if (!(fBreakWidth > 0.0)) {
                    fBreakWidth = fLineWidth - fCharWidth - fPrevWidth + pFontLetters[45].fLeadingEdge +
                        pFontLetters[45].fWidth + pFontLetters[45].fSpacing;
                    fLineWidth = fCharWidth + fPrevWidth;
                }
                else {
                    fLineWidth = fLineWidth - fBreakWidth;
                    if (!cHyphenBreak) {
                        if (acChar == 10) {
                            fLineWidth = 0.0f;
                        }
                        else {
                            fLineWidth = fLineWidth -
                                (pFontLetters[32].fLeadingEdge + pFontLetters[32].fWidth + pFontLetters[32].fSpacing);
                        }
                    }
                }
                if (!(fBreakWidth < static_cast<double>(kDimensions.x))) {
                    x = fBreakWidth;
                }
                else {
                    x = kDimensions.x;
                }
                kDimensions.x = x;
                kDimensions.y = LinePadding + pFont[aiFont - 1]->pFontData->fBaseLine + kDimensions.y;
                fBreakWidth = 0.0f;
                ++iLineCount;
            }
            fPrevWidth = fCharWidth;
        }
        if (!(fLineWidth < static_cast<double>(kDimensions.x))) {
            fMaxWidth = fLineWidth;
        }
        else {
            fMaxWidth = kDimensions.x;
        }
        kDimensions.z = static_cast<float>(iLineCount);
        return NiPoint3(fMaxWidth, kDimensions.y, kDimensions.z);
    }

    return NiPoint3::ZERO;
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
            if (acchar > 32) {
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

// Collects the text of apSource from character *aiIndex on, up to the first character whose type has a
// bit of aebfMatches (a space ends it with type 0x20 written to *aeResultMatch) or of aebfInvMatches (consumed, its
// type written), or lacks every bit of aeResultMatch when that is set. Line breaks are skipped; other characters are
// kept only with bKeep, where an HTML entity ("&...;") is replaced by its text replacement or text icon. The character
// that ended the run is left in *acResultChar. Every other character is dropped when bKeep is clear.
// The local buffer is appended to the string once it holds more than 2000 characters, without clearing its tail.
BSStringT<char> FontManager::CollectTo(
    const char* apSource,
    int32_t& aiIndex,
    int32_t aebfMatches,
    int32_t aebfInvMatches,
    int32_t& aeResultMatch,
    char& acResultChar,
    bool bKeep
) {
    BSStringT<char> xString;
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
                    char* pcEntity = static_cast<char*>(MemoryManager::Instance().Allocate(iEntityLength + 1));
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
                    MemoryManager::Instance().Deallocate(pcEntity);
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
    return xString;
}

// Parses the hypertext of axTextString into a new TextDoc, one CharData per character. Only a text whose first
// non-space character is '<' is hypertext; otherwise nothing is laid out and null is returned. Tags change the current
// character: <IMG SRC= WIDTH= HEIGHT=> makes it an image, <DIV ALIGN=> sets the justification after a line break, <FONT
// FACE= COLOR=RRGGBB> the font (by file name or 1-based number) and colour, </FONT> reverts to the default; <BR> and
// <P> add one and two line breaks, <HR> a new page (two breaks for an unbounded height). The text without its tags is
// written back to axTextString; axData gets the page count and the current page's line count, width and height.
FontManager::TextDoc* FontManager::PrepHypertext(BSStringT<char>& axTextString, FontManager::TextData& axData) {
    uint32_t uiLength = axTextString.Length();
    char* pcText = static_cast<char*>(MemoryManager::Instance().Allocate(uiLength + 1));
    std::memset(pcText, 0, uiLength + 1);
    std::strcpy(pcText, axTextString.pString);
    int32_t iIndex = 0;
    int32_t eMatch = 0;
    char cMatch = 0;

    // The text run read last, the tag name, the attribute name, the attribute value, and the text without its tags.
    alignas(BSStringT<char>) uint8_t aucText[sizeof(BSStringT<char>)];
    BSStringT<char>& kText = *static_cast<BSStringT<char>*>(static_cast<void*>(aucText));
    kText.Init();
    alignas(BSStringT<char>) uint8_t aucTag[sizeof(BSStringT<char>)];
    BSStringT<char>& kTag = *static_cast<BSStringT<char>*>(static_cast<void*>(aucTag));
    kTag.Init();
    alignas(BSStringT<char>) uint8_t aucAttribute[sizeof(BSStringT<char>)];
    BSStringT<char>& kAttribute = *static_cast<BSStringT<char>*>(static_cast<void*>(aucAttribute));
    kAttribute.Init();
    alignas(BSStringT<char>) uint8_t aucValue[sizeof(BSStringT<char>)];
    BSStringT<char>& kValue = *static_cast<BSStringT<char>*>(static_cast<void*>(aucValue));
    kValue.Init();
    alignas(BSStringT<char>) uint8_t aucPlainText[sizeof(BSStringT<char>)];
    BSStringT<char>& kPlainText = *static_cast<BSStringT<char>*>(static_cast<void*>(aucPlainText));
    kPlainText.Init();

    alignas(CharData) uint8_t aucChar[sizeof(CharData)];
    CharData* pChar = static_cast<CharData*>(static_cast<void*>(aucChar));
    ::new (aucChar) CharData(0, ' ', NiColorA(0.4588f, 0.2314f, 0.1294f, 1.0f), 1);
    bool bImage = false;
    int32_t iNewLines = 0;
    bool bNewPage = false;
    TextDoc* pDoc = nullptr;
    int32_t iIcon = 0;

    CollectTo(pcText, iIndex, 0, 4, eMatch, cMatch, false);
    if (eMatch == 1) {
        pDoc = new TextDoc(axData.iWidth, axData.iHeight, axData.iPageNum);
        iIndex = 0;
        while (true) {
            // The text up to the next '<'.
            kText.Set(CollectTo(pcText, iIndex, 1, 0, eMatch, cMatch, true));
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
                for (uint32_t i = 0; i < kText.Length(); ++i) {
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
            kTag.Set(CollectTo(pcText, iIndex, 6, 0, eMatch, cMatch, true));
            for (uint32_t i = 0; i < kTag.Length(); ++i) {
                kTag.pString[kTag.pString ? i : 0] = static_cast<char>(
                    std::toupper(kTag.pString[kTag.pString ? i : 0])
                );
            }
            if (eMatch & 0x20) {
                break;
            }
            if (eMatch & 4) {
                // The attributes, up to the '>'.
                while (true) {
                    CollectTo(pcText, iIndex, 0, 4, eMatch, cMatch, false);
                    if ((eMatch & 0x20) || (eMatch & 2)) {
                        break;
                    }
                    kAttribute.Set(CollectTo(pcText, iIndex, 0x16, 0, eMatch, cMatch, true));
                    for (uint32_t i = 0; i < kAttribute.Length(); ++i) {
                        kAttribute.pString[kAttribute.pString ? i : 0] = static_cast<char>(
                            std::toupper(kAttribute.pString[kAttribute.pString ? i : 0])
                        );
                    }
                    if ((eMatch & 0x20) || (eMatch & 2)) {
                        break;
                    }
                    if (eMatch & 4) {
                        // An attribute without a value.
                        kValue.Set("true", 0);
                    }
                    else {
                        CollectTo(pcText, iIndex, 0, 4, eMatch, cMatch, false);
                        if ((eMatch & 0x20) || (eMatch & 2)) {
                            break;
                        }
                        if (eMatch == 8) {
                            // A quoted value keeps its case.
                            ++iIndex;
                            kValue.Set(CollectTo(pcText, iIndex, 10, 0, eMatch, cMatch, true));
                            cMatch = pcText[iIndex];
                            ++iIndex;
                            eMatch = GetCharType(cMatch);
                        }
                        else {
                            kValue.Set(CollectTo(pcText, iIndex, 6, 0, eMatch, cMatch, true));
                            for (uint32_t i = 0; i < kValue.Length(); ++i) {
                                kValue.pString[kValue.pString ? i : 0] = static_cast<char>(
                                    std::toupper(kValue.pString[kValue.pString ? i : 0])
                                );
                            }
                        }
                    }
                    if (kTag.StrCmp("IMG", true) == 0) {
                        bImage = true;
                        if (kAttribute.StrCmp("SRC", true) == 0) {
                            pChar->xFilename.Set(kValue);
                        }
                        else if (kAttribute.StrCmp("WIDTH", true) == 0) {
                            std::sscanf(kValue.pString, "%i", &pChar->iWidth);
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
                            }
                            else if (kValue.StrCmp("CENTER", true) == 0) {
                                pChar->iJustification = 2;
                            }
                            else if (kValue.StrCmp("RIGHT", true) == 0) {
                                pChar->iJustification = 4;
                            }
                        }
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
                        if (kAttribute.StrCmp("COLOR", true) == 0 && kValue.Length() == 6) {
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
                                static_cast<float>(iRed / 255.0),
                                static_cast<float>(iGreen / 255.0),
                                static_cast<float>(iBlue / 255.0),
                                1.0f
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
            }
            else if (kTag.StrCmp("P", true) == 0) {
                iNewLines += 2;
            }
            else if (kTag.StrCmp("HR", true) == 0) {
                // An unbounded page cannot break: two line breaks instead.
                if (axData.iHeight == std::numeric_limits<int32_t>::max()) {
                    iNewLines += 2;
                }
                else {
                    bNewPage = true;
                    iNewLines = 0;
                }
            }
            else if (kTag.StrCmp("/FONT", true) == 0) {
                pChar->RevertToDefault();
            }
        }
        axTextString.Set(kPlainText);
    }
    MemoryManager::Instance().Deallocate(pcText);
    if (pDoc) {
        axData.bIsHypertext = true;
        axData.iNumPages = pDoc->xPages.GetSize();
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
            axData.iNumLines = pPage->xLines.GetSize();
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

void FontManager::SetNewFonts(bool abUseNewFonts) {
    bUseNewFonts = abUseNewFonts;
}

FontManager::TextDoc* FontManager::PrepText(BSStringT<char>& axTextString, FontManager::TextData& axData) {
    size_t sLen;
    FontManager::TextDoc* pdocument;
    if (!axTextString.pString) {
        return nullptr;
    }
    if (axData.iWidth < 2) {
        axData.iWidth = std::numeric_limits<int32_t>::max();
    }
    if (axData.iHeight < 2) {
        axData.iHeight = std::numeric_limits<int32_t>::max();
    }
    FontManager::TextDoc* pdocument_2 = PrepHypertext(axTextString, axData);
    if (pdocument_2) {
        return pdocument_2;
    }
    FontManager::TextDoc* pNewDoc = static_cast<FontManager::TextDoc*>(
        MemoryManager::Instance().Allocate(sizeof(FontManager::TextDoc))
    );
    if (pNewDoc) {
        int32_t iPageNum = axData.iPageNum;
        int32_t iHeight = axData.iHeight;
        int32_t iWidth = axData.iWidth;
        ::new (&pNewDoc->xPages) NiTPointerList<FontManager::TextPage*>;
        pNewDoc->iPageWidth = iWidth;
        pNewDoc->iPageHeight = iHeight;
        pNewDoc->iPageNum = iPageNum;
        pdocument = pNewDoc;
    }
    else {
        pdocument = nullptr;
    }
    pdocument_2 = pdocument;
    NiColorA kColor(0.4588f, 0.2314f, 0.1294f, 1.0f);
    FontManager::CharData xchar(0, 32, kColor, 1);
    xchar.RevertToDefault();
    xchar.iFontIndex = axData.iDefaultFont;
    xchar.xColor = axData.xColor;
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
    for (uint32_t uiIndex = 0;; ++uiIndex) {
        uint32_t uiLength = axTextString.sLen == 0xFFFF ? std::strlen(axTextString.pString) : axTextString.sLen;
        if (uiIndex >= uiLength) {
            break;
        }
        uint8_t ch = axTextString.pString[axTextString.pString != nullptr ? uiIndex : 0];
        Font::ConvertCharacter(ch);
        if (ch == axData.cLineSep) {
            ++inew_line;
        }
        else {
            if (ch == 9 || ch >= 32u) {
                xchar.SetChar(ch);
                int32_t iNewLines = inew_line;
                FontManager::CharData* pCharCopy = xchar.Copy();
                pdocument_2->AddChar(pCharCopy, iNewLines, false);
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
            NiTListItem<FontManager::TextLine*>* pLineItem = ppage->xLines.m_pkHead;
            while (pLineItem) {
                int32_t iNewWidth;
                FontManager::TextLine** ppLine = &pLineItem->m_element;
                pLineItem = pLineItem->m_pkNext;
                FontManager::TextLine* pLine = *ppLine;
                axData.iHeight += (*ppLine)->iSkippedSpace + (*ppLine)->iRise;
                if (axData.iWidth <= pLine->iWidth) {
                    iNewWidth = pLine->iWidth;
                }
                else {
                    iNewWidth = axData.iWidth;
                }
                axData.iWidth = iNewWidth;
            }
            axData.iNumLines = ppage->xLines.m_uiCount;
        }
    }
    return pdocument_2;
}

NiTriShape* FontManager::MakeImage(BSStringT<char> axFilename, NiPoint3 axPos, int32_t aiWidth, int32_t aiHeight) {
    TileShaderProperty* ptsp;
    NiTriShape* pshape;
    NiPoint2* ptextures;
    NiPoint3* pvertices;
    NiPoint3 kBottomRight;
    NiPoint3 kTopRight;
    float fscale;
    pvertices = new NiPoint3[4];
    ptextures = new NiPoint2[4];
    uint16_t* pvert_order = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
    float y = axPos.y;
    float fTop = static_cast<double>(aiHeight) + axPos.z;
    pvertices[0].x = axPos.x;
    pvertices[0].y = y;
    pvertices[0].z = fTop;
    pvertices[1] = axPos;
    kTopRight.x = static_cast<double>(aiWidth) + axPos.x;
    kTopRight.y = axPos.y;
    kTopRight.z = static_cast<double>(aiHeight) + axPos.z;
    pvertices[2] = kTopRight;
    kBottomRight.x = static_cast<double>(aiWidth) + axPos.x;
    kBottomRight.y = axPos.y;
    kBottomRight.z = axPos.z;
    pvertices[3] = kBottomRight;
    ptextures[0].x = 0.0f;
    ptextures[0].y = 0.0f;
    ptextures[1].x = 0.0f;
    ptextures[1].y = 1.0f;
    ptextures[2].x = 1.0f;
    ptextures[2].y = 0.0f;
    ptextures[3].x = 1.0f;
    ptextures[3].y = 1.0f;
    for (uint32_t a = 0;; ++a) {
        uint32_t sLen;
        if (axFilename.sLen == 0xFFFF) {
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
    if (axFilename.pString && (*axFilename.pString)) {
        NiPointer<NiTexture> spTempTexture;
        // Passes a null texture by value as the fifth argument.
        TileImage::AddToTesTextures(axFilename.pString, fscale, spTempTexture, 0.0f, NiPointer<NiTexture>());
        ptex = spTempTexture.m_pObject;
    }

    if (ptex) {
        ptextures[0].x = 0.0f;
        ptextures[0].y = 0.0f;
        int64_t iTexHeight = ptex->GetHeight();
        float fMaxV = fscale * static_cast<double>(aiHeight) / static_cast<double>(iTexHeight);
        ptextures[1].x = 0.0f;
        ptextures[1].y = fMaxV;
        int64_t iTexWidth = ptex->GetWidth();
        float fMaxU = fscale * static_cast<double>(aiWidth) / static_cast<double>(iTexWidth);
        ptextures[2].x = fMaxU;
        ptextures[2].y = 0.0f;
        int64_t iCornerTexWidth = ptex->GetWidth();
        float fCornerU = fscale * static_cast<double>(aiWidth) / static_cast<double>(iCornerTexWidth);
        int64_t iCornerTexHeight = ptex->GetHeight();
        float fCornerV = fscale * static_cast<double>(aiHeight) / static_cast<double>(iCornerTexHeight);
        ptextures[3].x = fCornerU;
        ptextures[3].y = fCornerV;
    }
    pvert_order[0] = 0;
    pvert_order[1] = 1;
    pvert_order[2] = 2;
    pvert_order[3] = 2;
    pvert_order[4] = 1;
    pvert_order[5] = 3;
    pshape = new BSScissorTriShape(
        4,
        pvertices,
        nullptr,
        nullptr,
        ptextures,
        1,
        NiGeometryData::NBT_METHOD_NONE,
        2,
        pvert_order,
        0,
        0,
        0,
        0
    );
    ptsp = new TileShaderProperty(false);
    ptsp->SetTileTexture(ptex);
    pshape->AttachProperty(ptsp);
    NiD3DShader* pshader = BSShaderManager::GetShader(32);
    pshape->m_spShader = static_cast<BSShader*>(pshader);
    pshape->UpdateProperties();
    axFilename.Set(nullptr, 0);
    return pshape;
}

void FontManager::Init() {
    if (!FontManager::pInstance) {
        FontManager::pInstance = new FontManager;
    }
}

FontManager* FontManager::Instance() {
    return FontManager::pInstance;
}

void FontManager::Free() {
    if (FontManager::pInstance) {
        delete FontManager::pInstance;
        FontManager::pInstance = nullptr;
    }
}
