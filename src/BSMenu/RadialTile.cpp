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
    float fMouseDegree_2;
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
    float fxb = fx * fscalar;
    float fyb = fy * fscalar;
    float fxa = fxb - fFocusX;
    float fya = fyb - fFocusY;
    if (fya == 0.0) {
        if (!(fxa > 0.0)) {
            fMouseDegree_2 = 4.71238f;
        }
        else {
            fMouseDegree_2 = 1.57075f;
        }
    }
    else {
        float v7 = -fxa / fya;
        float fMouseDegree = std::atan(static_cast<double>(v7));
        fMouseDegree_2 = fMouseDegree;
    }
    if (fya > 0.0) {
        fMouseDegree_2 = fMouseDegree_2 + 3.14159;
    }
    if (fxa < 0.0 && fya < 0.0) {
        fMouseDegree_2 = fMouseDegree_2 + 6.28318;
    }
    if (fMouseDegree_2 >= static_cast<double>(fMinDegree) && fMaxDegree >= static_cast<double>(fMouseDegree_2)) {
        float v5 = fya * fya + fxa * fxa;
        float fMouseMagnitude = std::sqrt(static_cast<double>(v5));
        BSStringT<char> text;
        text.SPrintF("--- fx %f fy %f  --- Magnitude: %f ", fxa, fya, fMouseMagnitude);
        Warning(text.pString);
        if (fMouseMagnitude >= static_cast<double>(fMinMagnitude) &&
            fMaxMagnitude >= static_cast<double>(fMouseMagnitude)) {
            return true;
        }
    }
    return false;
}

void RadialTile::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}
