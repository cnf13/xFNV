#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMenu/Tile.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout/Interface/Menus/ComputersMenu.h"
#include "Fallout/Interface/Menus/HackingText.h"

#include <cstdint>

HackingText::HackingText() {
    init();
}

// strText is destroyed by its own destructor; the display buffer is a raw allocation.
HackingText::~HackingText() {
    ourFree(pCurrentlyDisplayedText);
}

void HackingText::SetTraits(Tile::Value* apStringTrait, Tile::Value* apVisTrait) {
    pTileText = apStringTrait;
    pTileValueVisibility = apVisTrait;
}

bool HackingText::GetAnimationMode(int32_t aiType) {
    return (ucFlags & aiType) != 0;
}

bool HackingText::GetDisplayMode() {
    return GetAnimationMode(ANIMATION_DISPLAY);
}

bool HackingText::GetFlashMode() {
    return GetAnimationMode(ANIMATION_FLASH);
}

void HackingText::ResetBuffer() {
    ourFree(pCurrentlyDisplayedText);
    char* pBuffer = nullptr;
    if (strText.pString) {
        uint32_t uiLength = strText.GetLength();
        pBuffer = static_cast<char*>(ourCalloc(uiLength + 1, 1u));
    }
    pCurrentlyDisplayedText = pBuffer;
    bIsTextRemaining = false;
    uiNextDisplayTime = 0;
}

HackingText* HackingText::init() {
    SetTraits(nullptr, nullptr);
    uiDisplayRate = g_iComputersDisplayRateForCurrentMenu;
    uiFlashOnDuration = g_iHackingFlashOnDuration;
    uiFlashOffDuration = g_iHackingFlashOffDuration;
    uiNextBlinkTime = 0;
    uiNextDisplayTime = 0;
    pCurrentlyDisplayedText = nullptr;
    ucFlags = ANIMATION_DISPLAY;
    bIsTextRemaining = false;
    return this;
}

void HackingText::Update() {
    if (GetDisplayMode() && BSGetTickCount() >= uiNextDisplayTime && !bIsTextRemaining) {
        if (pTileValueVisibility && !GetFlashMode()) {
            pTileValueVisibility->SetFloat(1.0f, true);
        }

        if (pTileValueVisibility && pTileText && !strText.pString) {
            pTileValueVisibility->SetFloat(0.0f, true);
        }

        if (pTileText && pCurrentlyDisplayedText && strText != static_cast<const char*>(pCurrentlyDisplayedText)) {
            uint32_t uiNumChars;
            uint32_t uiDisplayedLength = BSstrlen(pCurrentlyDisplayedText);
            if (uiNextDisplayTime) {
                uiNumChars = (BSGetTickCount() - uiNextDisplayTime) / uiDisplayRate + 1;
            }
            else {
                uiNumChars = 1;
            }
            size_t uiCopyCount = uiNumChars;
            if (uiNumChars > strText.GetLength() - uiDisplayedLength) {
                uiCopyCount = strText.GetLength() - uiDisplayedLength;
                bIsTextRemaining = true;
            }
            ourMemcpy(&pCurrentlyDisplayedText[uiDisplayedLength], &strText.pString[uiDisplayedLength], uiCopyCount);
            pTileText->SetString(pCurrentlyDisplayedText, true);
        }
        else {
            bIsTextRemaining = true;
        }
        uiNextDisplayTime = uiDisplayRate + BSGetTickCount();
    }
    if (GetFlashMode() && BSGetTickCount() >= uiNextBlinkTime) {
        if (pTileValueVisibility->fValue == 1.0) {
            pTileValueVisibility->SetFloat(0.0f, true);
            uiNextBlinkTime = uiFlashOffDuration + BSGetTickCount();
        }
        else {
            pTileValueVisibility->SetFloat(1.0f, true);
            uiNextBlinkTime = uiFlashOnDuration + BSGetTickCount();
        }
    }
}

void HackingText::ForceFinish() {
    if (GetDisplayMode() && !bIsTextRemaining) {
        if (pTileValueVisibility && !GetFlashMode()) {
            pTileValueVisibility->SetFloat(1.0f, true);
        }

        if (pTileValueVisibility && pTileText && !strText.pString) {
            pTileValueVisibility->SetFloat(0.0f, true);
        }

        if (pTileText && pCurrentlyDisplayedText) {
            uint32_t uiLength = BSstrlen(strText.pString);
            ourMemcpy(pCurrentlyDisplayedText, strText.pString, uiLength);
            pTileText->SetString(pCurrentlyDisplayedText, true);
            bIsTextRemaining = true;
        }
    }
}

Tile* HackingText::GetParent() {
    if (pTileText && pTileValueVisibility && pTileText->pParent == pTileValueVisibility->pParent) {
        return pTileText->pParent;
    }
    if (pTileText && !pTileValueVisibility) {
        return pTileText->pParent;
    }
    if (pTileText || !pTileValueVisibility) {
        return nullptr;
    }
    return pTileValueVisibility->pParent;
}

HackingText* HackingText::HackingText_() {
    strText.~BSStringT<char>();
    ::new (&strText) BSStringT<char>;
    init();
    return this;
}

void HackingText::SetShouldBlink(bool abBlink) {
    SetFlagBit(ANIMATION_FLASH, abBlink);
}

void HackingText::SetFlagBit(uint32_t auiFlag, bool abSet) {
    if (abSet) {
        ucFlags |= auiFlag;
    }
    else {
        ucFlags &= ~auiFlag;
    }
}

void HackingText::SetVisibility(bool abOn) {
    if (pTileValueVisibility) {
        float fValue = abOn ? 1.0 : 0.0;
        pTileValueVisibility->SetFloat(fValue, true);
    }
}
