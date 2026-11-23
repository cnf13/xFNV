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
    char cSize;
    if (aeSizeType) {
        if (aeSizeType == SIZE_SHORT) {
            int16_t asSize[3];
            asSize[0] = aiSize;
            apFile->Write(asSize, 2u);
        }
        else if (aeSizeType == SIZE_INT) {
            apFile->Write(&aiSize, 4u);
        }
    }
    else {
        cSize = aiSize;
        apFile->Write(&cSize, 1u);
    }
}

void SaveGameSizeToBuffer(char* apBuffer, uint32_t aiSize, SIZE_TYPE aeSizeType) {
    char cSize;
    if (aeSizeType) {
        if (aeSizeType == SIZE_SHORT) {
            int16_t asSize[3];
            asSize[0] = aiSize;
            ourMemcpy(apBuffer, asSize, 2u);
        }
        else if (aeSizeType == SIZE_INT) {
            ourMemcpy(apBuffer, &aiSize, 4u);
        }
    }
    else {
        cSize = aiSize;
        ourMemcpy(apBuffer, &cSize, 1u);
    }
}

uint32_t LoadGameSizeFromBuffer(const char* apBuffer, uint32_t& ariSize, SIZE_TYPE aeSizeType) {
    uint8_t ucSize;
    if (aeSizeType) {
        if (aeSizeType == SIZE_SHORT) {
            uint16_t ausSize[3];
            ausSize[0] = 0;
            ourMemcpy(ausSize, apBuffer, sizeof(uint16_t) * 1);
            ariSize = ausSize[0];
            return 2;
        }

        if (aeSizeType == SIZE_INT) {
            ourMemcpy(&ariSize, apBuffer, sizeof(ariSize));
            return 4;
        }

        return 0;
    }

    ucSize = 0;
    ourMemcpy(&ucSize, apBuffer, 1u);
    ariSize = ucSize;
    return 1;
}

uint32_t LoadVariableSizedValueFromBuffer(const char* apBuffer, uint32_t& ariVariableValue) {
    uint32_t uiValue;
    uint8_t ucValue;
    uint32_t uiCount = 0;
    char cFirst = *apBuffer;
    int32_t iSizeBits = cFirst & 3;
    if ((cFirst & 3) != 0) {
        if (iSizeBits == 1) {
            uint16_t ausValue[3];
            ausValue[0] = 0;
            uiCount = 2;
            ourMemcpy(ausValue, apBuffer, sizeof(uint16_t) * 1);
            ausValue[0] = static_cast<int32_t>(ausValue[0]) >> 2;
            ariVariableValue = ausValue[0];
        }
        else if (iSizeBits == 2) {
            uiValue = 0;
            uiCount = 4;
            ourMemcpy(&uiValue, apBuffer, sizeof(uiValue));
            uiValue >>= 2;
            ariVariableValue = uiValue;
        }
    }
    else {
        ucValue = 0;
        uiCount = 1;
        ourMemcpy(&ucValue, apBuffer, 1u);
        ucValue = static_cast<int32_t>(ucValue) >> 2;
        ariVariableValue = ucValue;
    }
    if (apBuffer[uiCount] != 124) {
        SaveGameWarning("SAVELOAD: Invalid spacer hit while loading data from the buffer");
    }
    return uiCount + 1;
}

uint32_t GetVariableSizedValueSize(uint32_t aiVariableValue) {
    int32_t iSize = 0;
    if (aiVariableValue >= 64) {
        if (aiVariableValue >= 16384) {
            if (aiVariableValue < 1073741824) {
                iSize = 4;
            }
        }
        else {
            iSize = 2;
        }
    }
    else {
        iSize = 1;
    }
    return static_cast<uint32_t>(iSize + 1);
}

void SaveVariableSizedValueToBuffer(char* apBuffer, uint32_t aiVariableValue) {
    uint32_t uiValue;
    uint16_t ausValue[3];
    int8_t cValue;
    uint32_t uiCount = 0;
    if (aiVariableValue >= 64) {
        if (aiVariableValue >= 16384) {
            if (aiVariableValue >= 1073741824) {
                SaveGameWarning("SAVELOAD: Variable-sized value %08X is too large to be stored.", aiVariableValue);
            }
            else {
                uiValue = (4 * aiVariableValue) | 2;
                uiCount = 4;
                ourMemcpy(apBuffer, &uiValue, 4u);
            }
        }
        else {
            ausValue[0] = static_cast<uint16_t>((4 * aiVariableValue) | 1);
            uiCount = 2;
            ourMemcpy(apBuffer, ausValue, 2u);
        }
    }
    else {
        cValue = static_cast<int8_t>(4 * aiVariableValue);
        uiCount = 1;
        ourMemcpy(apBuffer, &cValue, 1u);
    }
    apBuffer[uiCount] = 124;
}

uint32_t LoadGameSizeFromFile(BGSSaveLoadFile* apFile, uint32_t& ariSize, SIZE_TYPE aeSizeType) {
    uint16_t ausSize[3];
    uint8_t ucSize;
    if (aeSizeType) {
        if (aeSizeType == SIZE_SHORT) {
            ausSize[0] = 0;
            apFile->Read(ausSize, 2u);
            ariSize = ausSize[0];
            return 2;
        }

        if (aeSizeType == SIZE_INT) {
            apFile->Read(&ariSize, 4u);
            return 4;
        }

        return 0;
    }

    ucSize = 0;
    apFile->Read(&ucSize, 1u);
    ariSize = ucSize;
    return 1;
}

bool GetLocationCellOrWorld(TESForm* apLocation, TESObjectCELL*& arpCell, TESWorldSpace*& arpWorld) {
    if (!apLocation) {
        return false;
    }
    if (apLocation->GetFormType() == 65) {
        arpWorld = static_cast<TESWorldSpace*>(apLocation);
        return true;
    }

    if (apLocation->GetFormType() == 57) {
        arpCell = static_cast<TESObjectCELL*>(apLocation);
        return true;
    }

    return false;
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

void NiEndian::Swap64(void* pvValue, int32_t iCount) {
    for (int32_t i = iCount; i; --i) {
        char c0 = *static_cast<uint8_t*>(pvValue);
        *static_cast<uint8_t*>(pvValue) = *(static_cast<uint8_t*>(pvValue) + 7);
        *(static_cast<uint8_t*>(pvValue) + 7) = c0;
        char c1 = *(static_cast<uint8_t*>(pvValue) + 1);
        *(static_cast<uint8_t*>(pvValue) + 1) = *(static_cast<uint8_t*>(pvValue) + 6);
        *(static_cast<uint8_t*>(pvValue) + 6) = c1;
        char c2 = *(static_cast<uint8_t*>(pvValue) + 2);
        *(static_cast<uint8_t*>(pvValue) + 2) = *(static_cast<uint8_t*>(pvValue) + 5);
        *(static_cast<uint8_t*>(pvValue) + 5) = c2;
        char c3 = *(static_cast<uint8_t*>(pvValue) + 3);
        *(static_cast<uint8_t*>(pvValue) + 3) = *(static_cast<uint8_t*>(pvValue) + 4);
        *(static_cast<uint8_t*>(pvValue) + 4) = c3;
        pvValue = static_cast<char*>(pvValue) + 8;
    }
}
