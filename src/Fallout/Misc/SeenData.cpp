#include "Fallout/Misc/SeenData.h"

#include "BSCore/NewOverloads.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderNoLightingProperty.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"

#include <limits>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> iNumBitsForFullySeen("iNumBitsForFullySeen:General", 248);

// A flat red rectangle of afDrawSize.
NiGeometry* MakeRectangle(float afDrawSize, NiColorA* aColor, bool abWithNoLightingProp) {
    NiPoint3* pLineVerts = new NiPoint3[4];
    NiColorA* pLineClrs = new NiColorA[4];
    uint8_t* pLineConn = static_cast<uint8_t*>(ourMalloc(4u));
    float fNegSize = -afDrawSize;
    NiPoint3 kCorner1(fNegSize, fNegSize, 0.0f);
    NiPoint3 kCorner2(-afDrawSize, afDrawSize, 0.0f);
    NiPoint3 kCorner3(afDrawSize, afDrawSize, 0.0f);
    NiPoint3 kCorner4(afDrawSize, -afDrawSize, 0.0f);
    pLineVerts[0] = kCorner1;
    pLineVerts[1] = kCorner2;
    pLineVerts[2] = kCorner3;
    pLineVerts[3] = kCorner4;
    for (int32_t i = 0; i < 4; ++i) {
        pLineClrs[i] = *aColor;
        pLineConn[i] = 1;
    }
    NiLines* pLines = new NiLines(4, pLineVerts, pLineClrs, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, pLineConn);
    if (abWithNoLightingProp) {
        BSShaderNoLightingProperty* pShProp = new BSShaderNoLightingProperty;
        pLines->AttachProperty(pShProp);
        BSShaderManager::PrepareObject(pLines, 0, 0);
    }
    return pLines;
}

SeenData::SeenData() : pSeenBits{} {}

SeenData::~SeenData() {}

// One red square of fPosSize per set bit, drawn from the seen grid's start: each square is centred by adding half of
// fPosSize to its x and y, rounded to float. abDrawSeen is not read.
void SeenData::DrawSeenData(NiNode* apDrawNode, const NiPoint3& aSeenStart, bool abDrawSeen) {
    if (apDrawNode) {
        NiColorA kColor(1.0f, 0.0f, 0.0f, 0.0f);
        float fHalfSize = fPosSize / 2.0f;
        for (uint32_t i = 0; i < static_cast<uint32_t>(iTotalBitCount); ++i) {
            if (IsSeenBitTrue(i)) {
                NiPoint3 kCoord;
                GetPosOffset(i, kCoord);
                kCoord += aSeenStart;
                NiGeometry* pRectangle = MakeRectangle(fPosSize, &kColor, true);
                apDrawNode->AttachChild(pRectangle, true);
                pRectangle->SetTranslate(kCoord.x + fHalfSize, kCoord.y + fHalfSize, kCoord.z);
            }
        }
    }
}

uint16_t SeenData::GetSaveSize(bool abRecursing) {
    return 32;
}

// The parameter is not read.
void SeenData::SaveGame(bool abRecursing) {
    pSaveLoadGameOLD->SaveGameDataOLD(pSeenBits, sizeof(pSeenBits));
}

// Passes 0 for the unused meta-data argument.
void SeenData::SaveGame(BGSSaveFormBuffer* abRecursing) {
    abRecursing->SaveDataEndian(reinterpret_cast<char*>(pSeenBits), sizeof(pSeenBits), 0);
}

// The parameter is not read.
void SeenData::LoadGame(uint16_t acCount) {
    pSaveLoadGameOLD->LoadGameDataOLD(pSeenBits, sizeof(pSeenBits));
}

void SeenData::LoadGame(BGSLoadFormBuffer* acCount) {
    acCount->LoadData(reinterpret_cast<char*>(pSeenBits), sizeof(pSeenBits));
}

bool SeenData::IsSeenBitTrue(uint32_t auiBit) {
    return (pSeenBits[auiBit >> 5] & (1 << (auiBit - 32 * (auiBit >> 5)))) != 0;
}

bool SeenData::IsSeenBitTrue(uint32_t aiX, uint32_t aiY) {
    uint32_t uiPos = aiX + (aiY << 4);
    return IsSeenBitTrue(uiPos);
}

bool SeenData::SetSeenBit(uint32_t aiPos, bool abVal) {
    uint32_t uiWord = aiPos >> 5;
    uint32_t uiBit = aiPos - (uiWord << 5);
    bool bWasSet = (pSeenBits[uiWord] & (1 << uiBit)) != 0;
    if (abVal) {
        pSeenBits[uiWord] = (1 << uiBit) | pSeenBits[uiWord];
    }
    else {
        pSeenBits[uiWord] = ~(1 << uiBit) & pSeenBits[uiWord];
    }
    return bWasSet;
}

bool SeenData::UpdateSeenData(const NiPoint3& aCoord, const NiPoint3& aSeenStart) {
    NiPoint3 kOffset;
    int32_t iSeenCount = 0;
    for (uint32_t uiBit = 0; uiBit < SeenData::iTotalBitCount; ++uiBit) {
        if (IsSeenBitTrue(uiBit)) {
            ++iSeenCount;
        }
        else {
            NiPoint3 kDelta;
            SeenData::GetPosOffset(uiBit, kOffset);
            kOffset += aSeenStart;
            kDelta = kOffset - aCoord;
            double dDistance = kDelta.Length();
            if (TESObjectCELL::GetSeenDataUpdateRadius() > dDistance) {
                SetSeenBit(uiBit, true);
                ++iSeenCount;
            }
        }
    }
    const int32_t& iFullySeenBits = iNumBitsForFullySeen.UInt();
    return iSeenCount >= iFullySeenBits;
}

// Cells are 4096 units. The offsets within the cell are stored as float before FloatToInt.
void SeenData::ShiftCoordToSeenPos(NiPoint3& aCoord) {
    int32_t iCellX = FloatToInt(aCoord.x) >> 12;
    int32_t iCellY = FloatToInt(aCoord.y) >> 12;
    float fOffsetX = static_cast<float>(aCoord.x - static_cast<double>(iCellX << 12));
    int32_t iSeenX = static_cast<int32_t>(static_cast<double>(FloatToInt(fOffsetX)) / fPosSize);
    float fOffsetY = static_cast<float>(aCoord.y - static_cast<double>(iCellY << 12));
    int32_t iSeenY = static_cast<int32_t>(static_cast<double>(FloatToInt(fOffsetY)) / fPosSize);
    aCoord.x = static_cast<float>(static_cast<double>(iCellX << 12) + static_cast<double>(iSeenX) * fPosSize);
    aCoord.y = static_cast<float>(static_cast<double>(iCellY << 12) + static_cast<double>(iSeenY) * fPosSize);
    aCoord.z = 0.0f;
}

float SeenData::GetSeenPosSize() {
    return fPosSize;
}

// fPosSize is truncated to an integer and the integer offsets are divided by it.
void SeenData::GetPosForCoord(const NiPoint3& aCoord, bool abInterior, int32_t& aiOutX, int32_t& aiOutY) {
    if (abInterior) {
        int32_t iCellX = (FloatToInt(aCoord.x) - 2048) >> 12;
        int32_t iCellY = (FloatToInt(aCoord.y) - 2048) >> 12;
        aiOutX = FloatToInt(aCoord.x) - ((iCellX << 12) + 2048);
        aiOutX /= static_cast<int32_t>(fPosSize);
        aiOutY = FloatToInt(aCoord.y) - ((iCellY << 12) + 2048);
    }
    else {
        int32_t iCellX = FloatToInt(aCoord.x) >> 12;
        int32_t iCellY = FloatToInt(aCoord.y) >> 12;
        aiOutX = FloatToInt(aCoord.x) - (iCellX << 12);
        aiOutX /= static_cast<int32_t>(fPosSize);
        aiOutY = FloatToInt(aCoord.y) - (iCellY << 12);
    }
    aiOutY /= static_cast<int32_t>(fPosSize);
}

void SeenData::GetPosOffset(uint32_t aiPos, NiPoint3& aOutCoord) {
    NiPoint3* pCoord;
    NiPoint3 kInvalidCoord;
    NiPoint3 kCoord;
    if (aiPos >= SeenData::iTotalBitCount) {
        pCoord = ::new (&kInvalidCoord) NiPoint3(
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()
        );
    }
    else {
        float fColumn = static_cast<float>(aiPos % 16);
        float fRow = static_cast<float>(aiPos >> 4);
        float fY = SeenData::fPosSize * fRow;
        float fX = SeenData::fPosSize * fColumn;
        pCoord = ::new (&kCoord) NiPoint3(fX, fY, 0.0f);
    }
    aOutCoord = *pCoord;
}

IntSeenData::IntSeenData(char acSectionX, char acSectionY)
  : SeenData(), cSectionX(acSectionX), cSectionY(acSectionY), pNextSection(nullptr) {}

IntSeenData::~IntSeenData() {
    delete pNextSection;
}

void IntSeenData::DrawSeenData(NiNode* apDrawNode, const NiPoint3& aSeenStart, bool abDrawSeen) {
    NiPoint3 kSectionStart;
    int32_t iSectionX = cSectionX;
    int32_t iSectionY = cSectionY;
    float fY = static_cast<float>(static_cast<double>((iSectionY << 12) + 2048));
    float fX = static_cast<float>(static_cast<double>((iSectionX << 12) + 2048));
    ::new (&kSectionStart) NiColor(fX, fY, aSeenStart.z);
    SeenData::DrawSeenData(apDrawNode, kSectionStart, abDrawSeen);
    if (pNextSection) {
        pNextSection->DrawSeenData(apDrawNode, aSeenStart, abDrawSeen);
    }
}

uint16_t IntSeenData::GetSaveSize(bool abRecursing) {
    uint16_t usSaveSize = SeenData::GetSaveSize(false);
    if (!abRecursing) {
        usSaveSize += 2;
    }
    uint16_t usSize = static_cast<uint16_t>(usSaveSize + 2);
    if (pNextSection) {
        usSize += pNextSection->GetSaveSize(true);
    }
    return usSize;
}

void IntSeenData::SaveGame(BGSSaveFormBuffer* abRecursing) {
    uint32_t uiSectionCount = 0;
    uint32_t uiPosition = abRecursing->StartVariableSizedValue();
    IntSeenData* pSection = this;
    while (pSection) {
        abRecursing->SaveDataEndian(&pSection->cSectionX, sizeof(pSection->cSectionX), 0);
        abRecursing->SaveDataEndian(&pSection->cSectionY, sizeof(pSection->cSectionY), 0);
        pSection->SeenData::SaveGame(abRecursing);
        pSection = pSection->pNextSection;
        ++uiSectionCount;
    }
    abRecursing->SaveVariableSizedValue(uiSectionCount, uiPosition);
}

void IntSeenData::SaveGame(bool abRecursing) {
    SeenData::SaveGame(false);
    if (!abRecursing) {
        int16_t sSectionCount = 1;
        for (IntSeenData* pSection = pNextSection; pSection; pSection = pSection->pNextSection) {
            ++sSectionCount;
        }
        pSaveLoadGameOLD->SaveGameDataOLD(&sSectionCount, sizeof(sSectionCount));
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&cSectionX, sizeof(cSectionX));
    pSaveLoadGameOLD->SaveGameDataOLD(&cSectionY, sizeof(cSectionY));
    if (pNextSection) {
        pNextSection->SaveGame(true);
    }
}

void IntSeenData::LoadGame(BGSLoadFormBuffer* acCount) {
    uint32_t uiSectionCount = acCount->LoadVariableSizedValue();
    IntSeenData* pSection = this;
    for (uint32_t i = 0; i < uiSectionCount; ++i) {
        acCount->LoadData(&pSection->cSectionX, sizeof(pSection->cSectionX));
        acCount->LoadData(&pSection->cSectionY, sizeof(pSection->cSectionY));
        pSection->SeenData::LoadGame(acCount);
        if (i < uiSectionCount - 1) {
            pSection->pNextSection = new IntSeenData(0, 0);
        }
        pSection = pSection->pNextSection;
    }
}

void IntSeenData::LoadGame(uint16_t acCount) {
    SeenData::LoadGame(0xFFFF);
    if (acCount == 0xFFFF) {
        pSaveLoadGameOLD->LoadGameDataOLD(&acCount, sizeof(acCount));
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&cSectionX, sizeof(cSectionX));
    pSaveLoadGameOLD->LoadGameDataOLD(&cSectionY, sizeof(cSectionY));
    if (--acCount) {
        pNextSection = new IntSeenData(0, 0);
        pNextSection->LoadGame(acCount);
    }
}
