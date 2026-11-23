#include "BSMenu/RadialTile.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileImage.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout/Interface/Interface.h"

#include <cmath>

RadialTile::RadialTile() {}

RadialTile::~RadialTile() {}

bool RadialTile::SpecialBoundsCheck(float fx, float fy) {
    float fDegree;
    float fFocusX = GetFloat(kTileValue_User0);
    float fFocusY = GetFloat(kTileValue_User1);
    float fMinDegree = GetFloat(kTileValue_User2);
    float fMaxDegree = GetFloat(kTileValue_User3);
    float fMinMagnitude = GetFloat(kTileValue_User4);
    float fMaxMagnitude = GetFloat(kTileValue_User5);
    float fscalar = Interface::GetRealScreenWidth();
    if (fscalar == 0.0) {
        fscalar = 1.0f;
    }
    Tile* pMenuRoot = Interface::GetMenusRoot();
    if (pMenuRoot) {
        Tile::Value* pValue = pMenuRoot->GetValueQ(4017);
        if (pValue) {
            fscalar = pValue->fValue / fscalar;
        }
        else {
            fscalar = 1.0f;
        }
    }
    else {
        fscalar = 1.0f;
    }
    float fScaledX = fx * fscalar;
    float fScaledY = fy * fscalar;
    float fRelX = fScaledX - fFocusX;
    float fRelY = fScaledY - fFocusY;
    if (fRelY == 0.0) {
        if (!(fRelX > 0.0)) {
            fDegree = 4.71238f;
        }
        else {
            fDegree = 1.57075f;
        }
    }
    else {
        float fTangent = -fRelX / fRelY;
        float fMouseDegree = std::atan(static_cast<double>(fTangent));
        fDegree = fMouseDegree;
    }
    if (fRelY > 0.0) {
        fDegree = fDegree + 3.14159;
    }
    if (fRelX < 0.0 && fRelY < 0.0) {
        fDegree = fDegree + 6.28318;
    }
    if (fDegree >= static_cast<double>(fMinDegree) && fMaxDegree >= static_cast<double>(fDegree)) {
        float fMagnitudeSq = fRelY * fRelY + fRelX * fRelX;
        float fMouseMagnitude = std::sqrt(static_cast<double>(fMagnitudeSq));
        BSStringT<char> text;
        text.SPrintF("--- fx %f fy %f  --- Magnitude: %f ", fRelX, fRelY, fMouseMagnitude);
        Warning(text.pString);
        if (fMouseMagnitude >= static_cast<double>(fMinMagnitude) &&
            fMaxMagnitude >= static_cast<double>(fMouseMagnitude)) {
            return true;
        }
    }
    return false;
}

void RadialTile::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}
