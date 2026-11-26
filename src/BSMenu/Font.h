#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class NiTexturingProperty;
class NiTriShape;

// The menu fonts, by their index in FontManager (font 1 is index 0).
enum FONTS : int32_t {
    FONT_NONE = -1,
    FONT_1 = 0,
    FONT_2 = 1,
    FONT_3 = 2,
    FONT_4 = 3,
    FONT_5 = 4,
    FONT_6 = 5,
    FONT_7 = 6,
    FONT_8 = 7,
    FONT_COUNT = 8,
};

// One texture coordinate of a glyph's corner.
struct UVMap {
    float fU;
    float fV;
};

// One glyph of a font: its texture page, the texture coordinates of its four corners and its metrics.
struct FontLetter {
    int32_t iTextureIndex;
    // Top left, top right, bottom left, bottom right.
    UVMap pMapping[4];
    float fWidth;
    float fHeight;
    float fLeadingEdge;
    float fSpacing;
    float fTopEdge;
};

// One texture page of a font file.
struct TextureFile {
    int32_t iType;
    char pFilename[32];
};

// The glyph data of a font file, loaded as one block.
struct FontData {
    // The distance from the top of a line to the text's base line.
    float fBaseLine;
    int32_t iTextureCount;
    TextureFile pTextureFiles[8];
    // Indexed by character code.
    FontLetter pFontLetters[256];
};

// The placement and texture coordinates of one button icon in a font's text (32 bytes).
struct ButtonIcon {
    float fWidth;
    float fXOffset;
    float fZOffset;
    float fSpacing;
    NiRect<float> UVCoords;
};

// One menu font: its file, textures and glyph data.
class Font {
public:
    // The layout of one text: the wrapped text, its size and line range, and the width of each line.
    class TextData {
    public:
        TextData(int32_t w, int32_t h, int32_t s, int32_t e, char c);
        ~TextData();

        int32_t GetWidth() const {
            return iWidth;
        }
        int32_t GetHeight() const {
            return iHeight;
        }
        int32_t GetCharCount() const {
            return iCharCount;
        }

        // The width of line `line`, or -1 past the last line.
        int32_t GetLineWidth(int32_t line) {
            BSSimpleList<int32_t>* temp = &xLineWidths;
            for (int32_t i = 0; i < line && temp; ++i) {
                temp = temp->m_pkNext;
            }
            return temp ? temp->m_item : -1;
        }

        BSStringT<char> xNewText;
        int32_t iWidth;
        int32_t iHeight;
        int32_t iLineStart;
        int32_t iLineEnd;
        int32_t iCharCount;
        char cLineSep;
        BSSimpleList<int32_t> xLineWidths;
    };

    Font(int32_t aiFontNum, char* apFilename, bool abLoad);
    ~Font();

    // The distance from the top of a line to the text's base line; 0 without glyph data.
    float GetBaseLine();
    // Lays the text out in the font (aiWidth/aiHeight are the wrap width and limit in, the used size out) and builds
    // its text shape and, for button icons, its icon shape.
    void CreateText(
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
    );
    // Substitutes the text's &-variables and button tags, then wraps it at axData's width into axData's text, line
    // widths, size and line count, keeping only lines iLineStart to iLineEnd.
    void PrepText(const char* apOrigString, TextData& axData);
    // Builds the shape for aiChars glyphs, tinted axColor, through the tile shader.
    NiTriShape* MakeTriShape(int32_t aiChars, const NiColorA& axColor, bool abPrepareObject);
    // Builds the shape for the text's button icons.
    NiTriShape* MakeIconsTriShape();
    // Writes glyph apLetter as quad aiVert of apShape at axPos, then advances axPos.x past it.
    void AddChar(FontLetter* apLetter, int32_t aiVert, NiTriShape* apShape, NiPoint3& axPos, const NiColorA* apColor);
    // Writes button icon aiIconIndex as a quad of apShape at aPos, then advances aPos.x
    // past it.
    void AddIcon(int32_t aiIconIndex, NiTriShape* apShape, NiPoint3& aPos);
    // Loads button icon astrIcon into the font's icon list.
    void AddTextIcon(const char* astrIcon);
    void KillData();
    void Unload();
    // Takes a reference; the first one loads the font file's glyph data and its texture pages.
    void Load();
    // The shape of apTextString in this font at (afStartX, afStartY, afZ), its width in aiWidth. The result is a
    // NiTriShape.
    NiAVObject* MakeString(
        float afStartX,
        float afStartY,
        float afZ,
        BSStringT<char>& apTextString,
        int32_t& aiWidth,
        int32_t aiFlags,
        const NiColorA& axFontColor,
        bool abUpperLeftCorner,
        bool abPrepareObject
    );

protected:
    void GetLineOffsets(const char* apTextString, float& afX, float& afY, int32_t aiFlags, uint32_t aiStart);

public:
    void SetSpacingForButtonIcon(ButtonIcon* apIconData, const char* apIconID);
    void TrimToLine(BSStringT<char>& axTextString, uint32_t aiLine);
    int32_t LineBreakText(
        BSStringT<char>& axTextString,
        int32_t& aiWidth,
        int32_t& aiHeight,
        bool abSkipHyperLinkedCharacters,
        char acEolChar
    );
    void GetLineNumber(char* apSrcString, BSStringT<char>& axDestString, int32_t aiLineNum, char acSepChar);

    // Turns the curly quotes 0x91-0x94 into straight ones.
    static void ConvertCharacter(uint8_t& acChar);

    uint16_t iRefCount;
    char* pFontFile;
    uint32_t iFontNum;
    NiPointer<NiTexturingProperty> pTextureData[8];
    float fFontHeight;
    float fMaxDrop;
    int32_t iLineOverlap;
    FontData* pFontData;
    BSStringT<char> IconAtlasTextureName;
    BSSimpleArray<ButtonIcon> ButtonIcons;
};

// The menu fonts: loads the font files and lays text out for the tiles.
class FontManager {
public:
    // One character of a laid-out text. A character or an inline image (xFilename), with the colour, justification and
    // metrics.
    struct CharData {
        // The destructor frees only xFilename, which starts empty.
        CharData() = default;
        CharData(int32_t aiFontIndex, uint8_t acChar, NiColorA axColor, int32_t aiJustification);
        ~CharData();

        // A heap copy of this character.
        CharData* Copy();
        // Back to font 1, the default colour, left justification, no image and a space.
        void RevertToDefault();
        // Sets the character and its metrics from the font.
        void SetChar(uint8_t acChar);

        int32_t iFontIndex;
        uint8_t cChar;
        NiColorA xColor;
        int32_t iJustification;
        BSStringT<char> xFilename;
        int32_t iWidth;
        int32_t iRise;
        int32_t iDrop;
        int32_t iLeadingEdge;
        int32_t iX;
    };

    struct TextPage;
    struct TextDoc;
    struct TextData;

    // A line of characters.
    struct TextLine {
        TextLine(
            FontManager::TextPage* apPage,
            FontManager::CharData* apChar,
            int32_t aiSkippedSpace,
            int32_t aiPageWidth
        );
        ~TextLine();

        FontManager::TextLine* AddChar(FontManager::CharData* apChar, bool abToFront);

        NiTPointerList<CharData*> xChars;
        int32_t iWidth;
        int32_t iNextSpacing;
        int32_t iRise;
        int32_t iDrop;
        int32_t iSkippedSpace;
        int32_t iJustify;
        int32_t iPageWidth;
        int32_t iY;
        TextPage* pPage;
    };

    // A page of lines.
    struct TextPage {
        TextPage(FontManager::TextDoc* apDoc, FontManager::CharData* apChar, int32_t aiPageWidth, int32_t aiPageHeight);
        TextPage(FontManager::TextDoc* apDoc, FontManager::TextLine* apLine, int32_t aiPageWidth, int32_t aiPageHeight);
        ~TextPage();

        int32_t GetCharCountForFont(int32_t aiFont);
        FontManager::TextPage* AddChar(FontManager::CharData* apChar, int32_t aiNewLines);

        NiTPointerList<TextLine*> xLines;
        int32_t iWidth;
        int32_t iHeight;
        int32_t iPageWidth;
        int32_t iPageHeight;
        int32_t iLastFontHeight;
        // Holds counts per font despite the pointer-style name.
        int32_t pCharsPerFont[8];
        TextDoc* pDoc;
    };

    // A text laid out as pages of lines, with the page the next character goes to.
    struct TextDoc {
        TextDoc(int32_t aiPageWidth, int32_t aiPageHeight, int32_t aiPageNum)
          : iPageWidth(aiPageWidth), iPageHeight(aiPageHeight), iPageNum(aiPageNum) {}
        ~TextDoc();

        // Adds apChar after aiNewLines line breaks, on a new page when abNewPage.
        void AddChar(CharData* apChar, int32_t aiNewLines, bool abNewPage);
        // The page iPageNum of xPages, or null.
        TextPage* CurrentPage();
        // Builds the shapes of the current page under apParent and reports the page and line
        // counts to apData.
        void Render(NiNode* apParent, TextData* apData);

        NiTPointerList<TextPage*> xPages;
        int32_t iPageWidth;
        int32_t iPageHeight;
        int32_t iPageNum;
    };

    // What a text layout asks for and what it reports back (page and line counts, the used size).
    struct TextData {
        ~TextData();

        int32_t iDefaultFont;
        int32_t iJustification;
        NiColorA xColor;
        char cLineSep;
        int32_t iWidth;
        int32_t iHeight;
        int32_t iPageNum;
        int32_t iLines;
        int32_t iNumPages;
        int32_t iNumLines;
        bool bIsHypertext;
        BSStringT<char> xNewText;
    };

    FontManager();
    ~FontManager();

    // Lays out the (hyper)text into shapes under apNode; false when it cannot.
    bool CreateText(BSStringT<char> axTextString, FontManager::TextData& axTextData, NiNode* apNode);
    // The size the text takes in font aiFont wrapped at afWrapwidth, from character aiStartIndex on, written to
    // apResult (x the width, y the height); returns apResult.
    NiPoint3 CalculateStringDimensions(const char* apText, int32_t aiFont, float afWrapwidth, int32_t aiStartIndex);
    // The extra gap between the lines of font aiFont: 4 for fonts 2 and 6, 6 for font 3, 2 for font 7, else 0.
    float GetLinePadding(int32_t aiFont);
    int32_t GetCharType(char acchar);
    // Collects the text of apSource from *aiIndex up to a stop character into apResult.
    BSStringT<char> CollectTo(
        const char* apSource,
        int32_t& aiIndex,
        int32_t aebfMatches,
        int32_t aebfInvMatches,
        int32_t& aeResultMatch,
        char& acResultChar,
        bool bKeep
    );
    // Lays the hypertext of axTextString out into a new TextDoc, which it returns (null when the text does not start
    // with a tag); writes the tag-free text back to axTextString and the page, line and size counts to axData.
    TextDoc* PrepHypertext(BSStringT<char>& axTextString, FontManager::TextData& axData);
    void SetNewFonts(bool abUseNewFonts);
    FontManager::TextDoc* PrepText(BSStringT<char>& axTextString, FontManager::TextData& axData);
    NiTriShape* MakeImage(BSStringT<char> axFilename, NiPoint3 axPos, int32_t aiWidth, int32_t aiHeight);
    Font* GetFont(int32_t aiIndex);

    // Creates the font manager and loads the fonts.
    static void Init();
    static FontManager* Instance();
    static void Free();

    Font* pFont[8];
    bool bUseNewFonts;

    static inline FontManager* pInstance;
    static SettingT<INISettingCollection> sFont_1_Filename;
    static SettingT<INISettingCollection> sFont_2_Filename;
    static SettingT<INISettingCollection> sFont_3_Filename;
    static SettingT<INISettingCollection> sFont_4_Filename;
    static SettingT<INISettingCollection> sFont_5_Filename;
    static SettingT<INISettingCollection> sFont_6_Filename;
    static SettingT<INISettingCollection> sFont_7_Filename;
    static SettingT<INISettingCollection> sFont_8_Filename;
};

// The heap block Font::Destruct frees through MemoryManager::Deallocate.
inline void* fontBuffer;
inline int32_t fontBufferSize;

// The rise of each of the eight fonts, which TextLine::AddChar reads by iFontIndex.
using aFontRise_t = int32_t[8];
inline aFontRise_t aFontRise{35, 35, 30, 60, 60, 0, 0, 0};

inline Font* FontManager::GetFont(int32_t aiIndex) {
    return pFont[aiIndex];
}
