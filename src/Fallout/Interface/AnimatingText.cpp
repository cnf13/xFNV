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
    return GetAnimationMode(1);
}

bool HackingText::GetFlashMode() {
    return GetAnimationMode(2);
}

void HackingText::ResetBuffer() {
    char* v2;
    ourFree(pCurrentlyDisplayedText);
    if (strText.pString) {
        uint32_t Length = strText.GetLength();
        v2 = static_cast<char*>(ourCalloc(Length + 1, 1u));
    }
    else {
        v2 = nullptr;
    }
    pCurrentlyDisplayedText = v2;
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
    ucFlags = 1;
    bIsTextRemaining = false;
    return this;
}

char HackingText::Update() {
    uint32_t v1;
    if (GetDisplayMode() && BSGetTickCount() >= uiNextDisplayTime && !bIsTextRemaining) {
        if ((pTileValueVisibility) && (!GetFlashMode())) {
            pTileValueVisibility->SetFloat(1.0f, true);
        }

        if ((pTileValueVisibility) && (pTileText) && (!strText.pString)) {
            pTileValueVisibility->SetFloat(0.0f, true);
        }

        if (pTileText &&
            (pCurrentlyDisplayedText && strText.operator!=(static_cast<const char*>(pCurrentlyDisplayedText)))) {
            uint32_t v3;
            uint32_t v6 = BSstrlen(pCurrentlyDisplayedText);
            if (uiNextDisplayTime) {
                v3 = (BSGetTickCount() - uiNextDisplayTime) / uiDisplayRate + 1;
            }
            else {
                v3 = 1;
            }
            size_t aCount = v3;
            if (v3 > strText.GetLength() - v6) {
                aCount = strText.GetLength() - v6;
                bIsTextRemaining = true;
            }
            ourMemcpy(&pCurrentlyDisplayedText[v6], &strText.pString[v6], aCount);
            pTileText->SetString(pCurrentlyDisplayedText, true);
        }
        else {
            bIsTextRemaining = true;
        }
        uiNextDisplayTime = uiDisplayRate + BSGetTickCount();
    }
    v1 = static_cast<uint32_t>((v1 & 0xFFFFFF00u) | (static_cast<uint32_t>(static_cast<uint8_t>(GetFlashMode()))));
    if (static_cast<uint8_t>(v1)) {
        v1 = BSGetTickCount();
        if (v1 >= uiNextBlinkTime) {
            if (pTileValueVisibility->fValue == 1.0) {
                pTileValueVisibility->SetFloat(0.0f, true);
                v1 = uiFlashOffDuration + BSGetTickCount();
            }
            else {
                pTileValueVisibility->SetFloat(1.0f, true);
                v1 = uiFlashOnDuration + BSGetTickCount();
            }
            uiNextBlinkTime = v1;
        }
    }
    return v1;
}

void HackingText::ForceFinish() {
    if (GetDisplayMode() && !bIsTextRemaining) {
        if ((pTileValueVisibility) && (!GetFlashMode())) {
            pTileValueVisibility->SetFloat(1.0f, true);
        }

        if ((pTileValueVisibility) && (pTileText) && (!strText.pString)) {
            pTileValueVisibility->SetFloat(0.0f, true);
        }

        if ((pTileText) && (pCurrentlyDisplayedText)) {
            uint32_t v1 = BSstrlen(strText.pString);
            ourMemcpy(pCurrentlyDisplayedText, strText.pString, v1);
            pTileText->SetString(pCurrentlyDisplayedText, true);
            bIsTextRemaining = true;
        }
    }
}

Tile* HackingText::GetParent() {
    if (pTileText && (pTileValueVisibility && pTileText->pParent == pTileValueVisibility->pParent)) {
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
    SetFlagBit(2, abBlink);
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
        float afValue;
        if (abOn) {
            afValue = 1.0;
        }
        else {
            afValue = 0.0;
        }
        pTileValueVisibility->SetFloat(afValue, true);
    }
}
