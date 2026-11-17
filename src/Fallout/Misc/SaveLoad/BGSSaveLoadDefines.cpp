#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFile.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiEndian.h"

#include <cstdarg>

void SaveGameWarning(const char* apMessage, ...) {
    va_list aList;
    va_start(aList, apMessage);
    if (pSaveLoadGame) {
        pSaveLoadGame->SaveGameWarningFunction(apMessage, aList);
    }
    va_end(aList);
}

void SaveGameDataToFileFunction(BGSSaveLoadFile* apFile, void* apData, uint32_t aiSize) {
    apFile->Write(apData, aiSize);
}

void LoadGameDataFromFileFunction(BGSSaveLoadFile* apFile, void* apData, uint32_t aiSize) {
    apFile->Read(apData, aiSize);
}

SIZE_TYPE GetSizeTypeForSize(uint32_t aiSize) {
    if (aiSize <= 255) {
        return SIZE_CHAR;
    }
    if (aiSize > 0xFFFF) {
        return SIZE_INT;
    }
    return SIZE_SHORT;
}

uint32_t GetSizeTypeSize(SIZE_TYPE aeSizeType) {
    return iSizeTypeSize[aeSizeType];
}

void SaveGameSizeToFile(BGSSaveLoadFile* apFile, uint32_t aiSize, SIZE_TYPE aeSizeType) {
    char apBuffer;
    if (aeSizeType) {
        if (aeSizeType == SIZE_SHORT) {
            int16_t v3[3];
            v3[0] = aiSize;
            apFile->Write(v3, 2u);
        }
        else {
            if (aeSizeType == SIZE_INT) {
                apFile->Write(&aiSize, 4u);
            }
        }
    }
    else {
        apBuffer = aiSize;
        apFile->Write(&apBuffer, 1u);
    }
}

void SaveGameSizeToBuffer(char* apBuffer, uint32_t aiSize, SIZE_TYPE aeSizeType) {
    char apSrc;
    if (aeSizeType) {
        if (aeSizeType == SIZE_SHORT) {
            int16_t v3[3];
            v3[0] = aiSize;
            ourMemcpy(apBuffer, v3, 2u);
        }
        else {
            if (aeSizeType == SIZE_INT) {
                ourMemcpy(apBuffer, &aiSize, 4u);
            }
        }
    }
    else {
        apSrc = aiSize;
        ourMemcpy(apBuffer, &apSrc, 1u);
    }
}

uint32_t LoadGameSizeFromBuffer(const char* apBuffer, uint32_t& ariSize, SIZE_TYPE aeSizeType) {
    uint8_t apDest;
    if (aeSizeType) {
        if (aeSizeType == SIZE_SHORT) {
            uint16_t v4[3];
            v4[0] = 0;
            ourMemcpy(v4, apBuffer, sizeof(uint16_t) * 1);
            ariSize = v4[0];
            return 2;
        }
        else {
            if (aeSizeType == SIZE_INT) {
                ourMemcpy(&ariSize, apBuffer, sizeof(ariSize));
                return 4;
            }
            else {
                return 0;
            }
        }
    }
    else {
        apDest = 0;
        ourMemcpy(&apDest, apBuffer, 1u);
        ariSize = apDest;
        return 1;
    }
}

uint32_t LoadVariableSizedValueFromBuffer(const char* apBuffer, uint32_t& ariVariableValue) {
    uint32_t v3;
    uint8_t apDest;
    uint32_t aCount = 0;
    char v6 = *apBuffer;
    int32_t v7 = v6 & 3;
    if ((v6 & 3) != 0) {
        if (v7 == 1) {
            uint16_t v4[3];
            v4[0] = 0;
            aCount = 2;
            ourMemcpy(v4, apBuffer, sizeof(uint16_t) * 1);
            v4[0] = static_cast<int32_t>(v4[0]) >> 2;
            ariVariableValue = v4[0];
        }
        else {
            if (v7 == 2) {
                v3 = 0;
                aCount = 4;
                ourMemcpy(&v3, apBuffer, sizeof(v3));
                v3 >>= 2;
                ariVariableValue = v3;
            }
        }
    }
    else {
        apDest = 0;
        aCount = 1;
        ourMemcpy(&apDest, apBuffer, 1u);
        apDest = static_cast<int32_t>(apDest) >> 2;
        ariVariableValue = apDest;
    }
    if (apBuffer[aCount] != 124) {
        SaveGameWarning("SAVELOAD: Invalid spacer hit while loading data from the buffer");
    }
    return aCount + 1;
}

uint32_t GetVariableSizedValueSize(uint32_t aiVariableValue) {
    int32_t v2 = 0;
    if (aiVariableValue >= 64) {
        if (aiVariableValue >= 16384) {
            if (aiVariableValue < 1073741824) {
                v2 = 4;
            }
        }
        else {
            v2 = 2;
        }
    }
    else {
        v2 = 1;
    }
    return (uint32_t)(v2 + 1);
}

void SaveVariableSizedValueToBuffer(char* apBuffer, uint32_t aiVariableValue) {
    uint32_t v2;
    uint16_t v3[3];
    int8_t apSrc;
    uint32_t aCount = 0;
    if (aiVariableValue >= 64) {
        if (aiVariableValue >= 16384) {
            if (aiVariableValue >= 1073741824) {
                SaveGameWarning("SAVELOAD: Variable-sized value %08X is too large to be stored.", aiVariableValue);
            }
            else {
                v2 = (4 * aiVariableValue) | 2;
                aCount = 4;
                ourMemcpy(apBuffer, &v2, 4u);
            }
        }
        else {
            v3[0] = (uint16_t)((4 * aiVariableValue) | 1);
            aCount = 2;
            ourMemcpy(apBuffer, v3, 2u);
        }
    }
    else {
        apSrc = (int8_t)(4 * aiVariableValue);
        aCount = 1;
        ourMemcpy(apBuffer, &apSrc, 1u);
    }
    apBuffer[aCount] = 124;
}

uint32_t LoadGameSizeFromFile(BGSSaveLoadFile* apFile, uint32_t& ariSize, SIZE_TYPE aeSizeType) {
    uint16_t v4[3];
    uint8_t apBuffer;
    if (aeSizeType) {
        if (((int32_t)aeSizeType) == 1) {
            v4[0] = (uint16_t)0;
            apFile->Read(v4, 2u);
            ariSize = (uint32_t)v4[0];
            return (uint32_t)2;
        }
        else {
            if (((int32_t)aeSizeType) == 2) {
                apFile->Read(&ariSize, 4u);
                return (uint32_t)4;
            }
            else {
                return (uint32_t)0;
            }
        }
    }
    else {
        apBuffer = 0;
        apFile->Read(&apBuffer, 1u);
        ariSize = (uint32_t)apBuffer;
        return (uint32_t)1;
    }
}

bool GetLocationCellOrWorld(TESForm* apLocation, TESObjectCELL*& arpCell, TESWorldSpace*& arpWorld) {
    if (!apLocation) {
        return 0;
    }
    if (apLocation->GetFormType() == 65) {
        arpWorld = static_cast<TESWorldSpace*>(apLocation);
        return 1;
    }
    else {
        if (apLocation->GetFormType() == 57) {
            arpCell = static_cast<TESObjectCELL*>(apLocation);
            return 1;
        }
        else {
            return 0;
        }
    }
}

void NiEndian::Swap32(char* pcValue, uint32_t uiCount) {
    for (uint32_t i = uiCount; i; --i) {
        char c0 = pcValue[0];
        pcValue[0] = pcValue[3];
        pcValue[3] = c0;
        char c1 = pcValue[1];
        pcValue[1] = pcValue[2];
        pcValue[2] = c1;
        pcValue += 4;
    }
}

void NiEndian::Swap64(void* a1, int32_t x0) {
    for (int32_t i = x0; i; --i) {
        char v4 = *static_cast<uint8_t*>(a1);
        *static_cast<uint8_t*>(a1) = *(static_cast<uint8_t*>(a1) + 7);
        *(static_cast<uint8_t*>(a1) + 7) = v4;
        char v5 = *(static_cast<uint8_t*>(a1) + 1);
        *(static_cast<uint8_t*>(a1) + 1) = *(static_cast<uint8_t*>(a1) + 6);
        *(static_cast<uint8_t*>(a1) + 6) = v5;
        char v6 = *(static_cast<uint8_t*>(a1) + 2);
        *(static_cast<uint8_t*>(a1) + 2) = *(static_cast<uint8_t*>(a1) + 5);
        *(static_cast<uint8_t*>(a1) + 5) = v6;
        char v7 = *(static_cast<uint8_t*>(a1) + 3);
        *(static_cast<uint8_t*>(a1) + 3) = *(static_cast<uint8_t*>(a1) + 4);
        *(static_cast<uint8_t*>(a1) + 4) = v7;
        a1 = static_cast<char*>(a1) + 8;
    }
}
