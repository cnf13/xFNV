#pragma once

#include "BSCore/BSStringT.h"
#include "BSMenu/Tile.h"

// A line of text the hacking terminal types out and blinks.
class HackingText {
public:
    enum ANIMATION_FLAGS : uint32_t {
        // Typed out a character at a time.
        ANIMATION_DISPLAY = 0x1,
        ANIMATION_FLASH = 0x2,
    };

    HackingText();
    HackingText(const char* apString);
    ~HackingText();

    // Assigns the full text and resets the display buffer; returns *this.
    HackingText& operator=(const char* apString);
    // Sets the tile value that shows the text and the one that toggles its visibility.
    void SetTraits(Tile::Value* apStringTrait, Tile::Value* apVisTrait);
    // Sets the interval between displayed characters, at least 1.
    void SetDisplayInterval(uint32_t aiInterval);
    // Whether any of the aiType animation bits is set in ucFlags.
    bool GetAnimationMode(int32_t aiType);
    // Whether the text is displayed a character at a time (animation bit 1).
    bool GetDisplayMode();
    // Whether the text flashes (animation bit 2).
    bool GetFlashMode();
    // Frees the display buffer, allocates a zeroed one when there is text, and restarts the display.
    void ResetBuffer();
    HackingText* init();
    void Update();
    // Shows the whole text at once when it is being typed out and is not finished yet.
    void ForceFinish();
    // The tile of the text trait's or the visibility trait's owner (pParent of the Tile::Value).
    Tile* GetParent();
    // Returns the full text (strFullText's string).
    const char* GetFullText();
    HackingText* HackingText_();
    // Sets or clears the blink flag (2) of ucFlags.
    void SetShouldBlink(bool abBlink);
    // Sets or clears auiFlag in ucFlags.
    void SetFlagBit(uint32_t auiFlag, bool abSet);
    void SetVisibility(bool abOn);
    bool operator!=(const char* apString);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    BSStringT<char> strText;
    Tile::Value* pTileText;
    Tile::Value* pTileValueVisibility;
    uint32_t uiDisplayRate;
    uint32_t uiFlashOnDuration;
    uint32_t uiFlashOffDuration;
    uint32_t uiNextDisplayTime;
    uint32_t uiNextBlinkTime;
    char* pCurrentlyDisplayedText;
    uint8_t ucFlags;
    bool bIsTextRemaining;
};

// HackingText::init's data: this file's .data run is g_iComputersDisplayRateForCurrentMenu and the two flash
// durations, between two copies of <new>'s _bad_alloc_Message.
inline int32_t g_iHackingFlashOnDuration = 750;

inline int32_t g_iHackingFlashOffDuration = 500;
