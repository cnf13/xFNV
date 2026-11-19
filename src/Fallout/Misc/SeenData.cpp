#include "Fallout/Misc/SeenData.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "SDK/MSVC/ehvec.h"

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> iNumBitsForFullySeen("iNumBitsForFullySeen:General", 248);

// A flat red rectangle of afDrawSize.
NiGeometry* MakeRectangle(float afDrawSize, NiColorA* aColor, bool abWithNoLightingProp) {
    NiLines* plines;
    NiColorA* plineclrs;
    NiPoint3* plineverts;
    NiPoint3 v17;
    NiPoint3 v18;
    NiPoint3 v19;
    NiPoint3 dst;
    void* ptr = ourMalloc(4 * sizeof(NiPoint3));
    if (ptr) {
        rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint3), 4, &emptyPointConstructor);
        plineverts = static_cast<NiPoint3*>(ptr);
    }
    else {
        plineverts = nullptr;
    }
    void* v22 = ourMalloc(4 * sizeof(NiColorA));
    if (v22) {
        plineclrs = ::new (v22) NiColorA[4];
    }
    else {
        plineclrs = nullptr;
    }
    uint8_t* plineconn = static_cast<uint8_t*>(ourMalloc(4u));
    float y = -afDrawSize;
    NiPoint3* v3 = ::new (&dst) NiPoint3(y, y, 0.0f);
    *plineverts = *v3;
    NiPoint3* v4 = ::new (&v19) NiPoint3(-afDrawSize, afDrawSize, 0.0f);
    plineverts[1] = *v4;
    NiPoint3* v5 = ::new (&v18) NiPoint3(afDrawSize, afDrawSize, 0.0f);
    plineverts[2] = *v5;
    NiPoint3* v6 = ::new (&v17) NiPoint3(afDrawSize, -afDrawSize, 0.0f);
    plineverts[3] = *v6;
    for (int32_t i = 0; i < 4; ++i) {
        plineclrs[i] = *aColor;
        plineconn[i] = 1;
    }
    plines = new NiLines(4, plineverts, plineclrs, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, plineconn);
    if (abWithNoLightingProp) {
        BSShaderNoLightingProperty* pshprop;
        pshprop = new BSShaderNoLightingProperty;
        NiProperty* pkProperty = pshprop;
        plines->AttachProperty(pshprop);
        BSShaderManager::PrepareObject(plines, 0, 0);
    }
    return plines;
}

SeenData::SeenData() : pSeenBits{} {}

SeenData::~SeenData() {}

// One red square of fPosSize per set bit, drawn from the seen grid's start: each square is centred by adding half of
// fPosSize to its x and y, rounded to float. abDrawSeen is not read.
void SeenData::DrawSeenData(NiNode* apDrawNode, const NiPoint3& aSeenStart, bool abDrawSeen) {
    if (apDrawNode) {
        NiColorA color;
        color.~NiColorA();
        ::new (&color) NiColorA(1.0f, 0.0f, 0.0f, 0.0f);
        float fHalfSize = fPosSize / 2.0f;
        for (uint32_t i = 0; i < static_cast<uint32_t>(iTotalBitCount); ++i) {
            if (IsSeenBitTrue(i)) {
                NiPoint3 coord;
                coord.~NiPoint3();
                ::new (&coord) NiPoint3;
                GetPosOffset(i, coord);
                coord += aSeenStart;
                NiGeometry* pRectangle = MakeRectangle(fPosSize, &color, true);
                apDrawNode->AttachChild(pRectangle, true);
                pRectangle->SetTranslate(coord.x + fHalfSize, coord.y + fHalfSize, coord.z);
            }
        }
    }
}

uint16_t SeenData::GetSaveSize(bool abRecursing) {
    return (uint16_t)32;
}

// The parameter is not read.
void SeenData::SaveGame(bool abRecursing) {
    pSaveLoadGameOLD->SaveGameDataOLD(pSeenBits, 0x20);
}

// Passes 0 for the unused meta-data argument.
void SeenData::SaveGame(BGSSaveFormBuffer* abRecursing) {
    abRecursing->SaveDataEndian(reinterpret_cast<char*>(pSeenBits), 0x20, 0);
}

// The parameter is not read.
void SeenData::LoadGame(uint16_t acCount) {
    pSaveLoadGameOLD->LoadGameDataOLD(pSeenBits, 0x20);
}

void SeenData::LoadGame(BGSLoadFormBuffer* acCount) {
    acCount->LoadData(reinterpret_cast<char*>(pSeenBits), 0x20);
}

bool SeenData::IsSeenBitTrue(uint32_t auiBit) {
    return ((pSeenBits[auiBit >> 5] & ((int32_t)(1 << (auiBit - (32 * (auiBit >> 5)))))) != 0);
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
    NiPoint3 a2;
    int32_t v10 = 0;
    for (uint32_t a1 = 0; a1 < SeenData::iTotalBitCount; ++a1) {
        if (IsSeenBitTrue(a1)) {
            ++v10;
        }
        else {
            NiPoint3 result;
            SeenData::GetPosOffset(a1, a2);
            a2 += aSeenStart;
            result = a2 - aCoord;
            double v5 = result.Length();
            if (TESObjectCELL::GetSeenDataUpdateRadius() > v5) {
                SetSeenBit(a1, true);
                ++v10;
            }
        }
    }
    const int32_t& v3 = iNumBitsForFullySeen.UInt();
    return v10 >= v3;
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

// fPosSize is truncated to an integer and the integer offsets are divided by it, as the game does.
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
    NiPoint3* v2;
    NiPoint3 v5;
    NiPoint3 dst;
    if (aiPos >= SeenData::iTotalBitCount) {
        v2 = ::new (&v5) NiPoint3(3.4028235e+38f, 3.4028235e+38f, 3.4028235e+38f);
    }
    else {
        float v8 = static_cast<float>(aiPos % 16);
        float v7 = static_cast<float>(aiPos >> 4);
        float y = SeenData::fPosSize * v7;
        float x = SeenData::fPosSize * v8;
        v2 = ::new (&dst) NiPoint3(x, y, static_cast<float>(0.0));
    }
    aOutCoord = *v2;
}

IntSeenData::IntSeenData(char acSectionX, char acSectionY)
  : SeenData(), cSectionX(acSectionX), cSectionY(acSectionY), pNextSection(nullptr) {}

IntSeenData::~IntSeenData() {
    delete pNextSection;
}

void IntSeenData::DrawSeenData(NiNode* apDrawNode, const NiPoint3& aSeenStart, bool abDrawSeen) {
    NiPoint3 dst;
    int32_t cSectionX;
    int32_t cSectionY;
    cSectionX = (int32_t)this->cSectionX;
    cSectionY = (int32_t)this->cSectionY;
    float y = (float)(double)((cSectionY << 12) + 2048);
    float x = (float)(double)((cSectionX << 12) + 2048);
    ::new (&dst) NiColor(x, y, (&aSeenStart)->z);
    SeenData::DrawSeenData(apDrawNode, dst, abDrawSeen);
    if (pNextSection) {
        pNextSection->DrawSeenData(apDrawNode, aSeenStart, abDrawSeen);
    }
}

uint16_t IntSeenData::GetSaveSize(bool abRecursing) {
    uint16_t saveSize = SeenData::GetSaveSize(false);
    if (!abRecursing) {
        saveSize += 2;
    }
    uint16_t v5 = (uint16_t)(saveSize + 2);
    if (pNextSection) {
        v5 += pNextSection->GetSaveSize(true);
    }
    return v5;
}

void IntSeenData::SaveGame(BGSSaveFormBuffer* abRecursing) {
    uint32_t aiVariableValue = 0;
    uint32_t aiPosition = abRecursing->StartVariableSizedValue();
    IntSeenData* v4 = this;
    while (v4) {
        abRecursing->SaveDataEndian(&v4->cSectionX, 1u, 0);
        abRecursing->SaveDataEndian(&v4->cSectionY, 1u, 0);
        v4->SeenData::SaveGame(abRecursing);
        v4 = v4->pNextSection;
        ++aiVariableValue;
    }
    abRecursing->SaveVariableSizedValue(aiVariableValue, aiPosition);
}

void IntSeenData::SaveGame(bool abRecursing) {
    int16_t apData;
    SeenData::SaveGame(false);
    if (!abRecursing) {
        apData = 1;
        IntSeenData* v4 = this;
        while (true) {
            v4 = v4->pNextSection;
            if (!v4) {
                break;
            }
            ++apData;
        }
        pSaveLoadGameOLD->SaveGameDataOLD(&apData, 2u);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&cSectionX, 1u);
    pSaveLoadGameOLD->SaveGameDataOLD(&cSectionY, 1u);
    if (pNextSection) {
        pNextSection->SaveGame(true);
    }
}

void IntSeenData::LoadGame(BGSLoadFormBuffer* acCount) {
    uint32_t VariableSizedValue = acCount->LoadVariableSizedValue();
    IntSeenData* v7 = this;
    for (uint32_t i = 0; i < VariableSizedValue; ++i) {
        acCount->LoadData(&v7->cSectionX, 1u);
        acCount->LoadData(&v7->cSectionY, 1u);
        v7->SeenData::LoadGame(acCount);
        if (i < VariableSizedValue - 1) {
            IntSeenData* v2;
            IntSeenData* apMemory = static_cast<IntSeenData*>(ourMalloc(sizeof(IntSeenData)));
            if (apMemory) {
                v2 = ::new (apMemory) IntSeenData(0, 0);
            }
            else {
                v2 = nullptr;
            }
            v7->pNextSection = v2;
        }
        v7 = v7->pNextSection;
    }
}

void IntSeenData::LoadGame(uint16_t acCount) {
    SeenData::LoadGame(0xFFFF);
    if (acCount == 0xFFFF) {
        pSaveLoadGameOLD->LoadGameDataOLD(&acCount, 2u);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&cSectionX, 1u);
    pSaveLoadGameOLD->LoadGameDataOLD(&cSectionY, 1u);
    if (--acCount) {
        IntSeenData* v2;
        IntSeenData* apMemory = static_cast<IntSeenData*>(ourMalloc(sizeof(IntSeenData)));
        if (apMemory) {
            v2 = ::new (apMemory) IntSeenData(0, 0);
        }
        else {
            v2 = nullptr;
        }
        pNextSection = v2;
        pNextSection->LoadGame(acCount);
    }
}
