#include "Fallout Shared/TESWeatherList.h"

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESWeather.h"
#include "SDK/MSVC/string.h"

tmpWeatherEntry::tmpWeatherEntry() : uiNumericID(0), uiChance(0), uiGlobalID(0) {}

void tmpWeatherEntry::Endian() {
    ENDIAN(uiNumericID, false);
    ENDIAN(uiChance, false);
    ENDIAN(uiGlobalID, false);
}

WeatherEntry::WeatherEntry() : pWeather(nullptr), uiChance(0), pChanceVar(nullptr) {}

TESWeatherList::TESWeatherList() {
    FlushList();
}

TESWeatherList::~TESWeatherList() {
    FlushList();
}

void TESWeatherList::CopyComponent(const TESWeatherList* apCopyWL, bool abAppend) {
    if (apCopyWL) {
        if (!abAppend) {
            FlushList();
        }
        if (!apCopyWL->IsEmpty()) {
            for (const BSSimpleList<WeatherEntry*>* pNode = apCopyWL; pNode; pNode = pNode->GetNext()) {
                WeatherEntry* pNewEntry;
                WeatherEntry* apMemory = static_cast<WeatherEntry*>(ourMalloc(sizeof(WeatherEntry)));
                if (apMemory) {
                    pNewEntry = ::new (apMemory) WeatherEntry;
                }
                else {
                    pNewEntry = nullptr;
                }
                ourMemcpy(pNewEntry, pNode->GetItem(), sizeof(WeatherEntry));
                AddTail(pNewEntry);
            }
        }
    }
}

TESWeather* TESWeatherList::GetRandomWeather() {
    uint32_t uiSum = 0;
    for (BSSimpleList<WeatherEntry*>* pCurrentNode = this; pCurrentNode; pCurrentNode = pCurrentNode->m_pkNext) {
        if (pCurrentNode->m_item) {
            if (pCurrentNode->m_item->pChanceVar) {
                uiSum += static_cast<int64_t>(pCurrentNode->m_item->pChanceVar->fValue);
            }
            else {
                uiSum += pCurrentNode->m_item->uiChance;
            }
        }
    }
    if (!uiSum) {
        return nullptr;
    }
    uint32_t uiRand = Random() % uiSum;
    uint32_t uiLow = 0;
    uint32_t uiHigh = -1;
    WeatherEntry* pEntry = nullptr;
    for (BSSimpleList<WeatherEntry*>* pCurrentNode_2 = this; pCurrentNode_2;
         pCurrentNode_2 = pCurrentNode_2->m_pkNext) {
        pEntry = pCurrentNode_2->m_item;
        if (pEntry) {
            uint32_t i = pEntry->pChanceVar ? static_cast<int64_t>(pEntry->pChanceVar->fValue) : pEntry->uiChance;
            if (i) {
                uiHigh += i;
                if (uiLow <= uiRand && uiRand <= uiHigh) {
                    break;
                }
                uiLow += i;
            }
        }
    }
    if (pEntry) {
        return pEntry->pWeather;
    }

    return nullptr;
}

void TESWeatherList::Save(CHUNK_ID aeChunkID) {
    uint32_t uiCount = ItemsInList();
    if (uiCount) {
        tmpWeatherEntry* tmpArray;
        // new tmpWeatherEntry[uiCount]: on overflow the size test passes SIZE_MAX to ourMalloc.
        void* pMemory =
            ourMalloc(uiCount <= SIZE_MAX / sizeof(tmpWeatherEntry) ? sizeof(tmpWeatherEntry) * uiCount : SIZE_MAX);
        if (pMemory) {
            rt::eh_vector_constructor_iterator(
                pMemory, sizeof(tmpWeatherEntry), static_cast<int32_t>(uiCount),
                &xfnv::constructElement<tmpWeatherEntry>
            );
            tmpArray = static_cast<tmpWeatherEntry*>(pMemory);
        }
        else {
            tmpArray = nullptr;
        }
        uint32_t uiIndex = 0;
        for (BSSimpleList<WeatherEntry*>* pCurrentNode = this; pCurrentNode; pCurrentNode = pCurrentNode->m_pkNext) {
            tmpArray[uiIndex].uiNumericID = pCurrentNode->m_item->pWeather->iFormID;
            tmpArray[uiIndex].uiChance = pCurrentNode->m_item->uiChance;
            uint32_t uiGlobalID;
            if (pCurrentNode->m_item->pChanceVar) {
                uiGlobalID = pCurrentNode->m_item->pChanceVar->iFormID;
            }
            else {
                uiGlobalID = 0;
            }
            tmpArray[uiIndex++].uiGlobalID = uiGlobalID;
        }
        if (TESForm::QEndianSwapOnSave()) {
            for (uint32_t i = 0; i < uiIndex; ++i) {
                tmpArray[i].Endian();
            }
        }
        TESForm::__AddChunkData(aeChunkID, tmpArray, sizeof(tmpWeatherEntry) * uiIndex);
        if (TESForm::QEndianSwapOnSave()) {
            for (uint32_t j = 0; j < uiIndex; ++j) {
                tmpArray[j].Endian();
            }
        }
        ourFree(tmpArray);
    }
}

void TESWeatherList::FlushList() {
    for (BSSimpleList<WeatherEntry*>* pNode = this; pNode;) {
        WeatherEntry* pEntry;
        pEntry = pNode->GetItem();
        ourFree(pEntry);
        pNode = pNode->GetNext();
    }
    RemoveAll();
}

bool TESWeatherList::CompareComponent(TESWeatherList* apCompareWL) {
    if (!apCompareWL) {
        return true;
    }
    if (IsEmpty() && apCompareWL->IsEmpty()) {
        return false;
    }
    int32_t v3 = ItemsInList();
    if (v3 != apCompareWL->ItemsInList()) {
        return true;
    }
    for (BSSimpleList<WeatherEntry*>* pThisNode = this; pThisNode; pThisNode = pThisNode->m_pkNext) {
        bool bFound = false;
        for (BSSimpleList<WeatherEntry*>* pCompNode = apCompareWL; !bFound && pCompNode;
             pCompNode = pCompNode->m_pkNext) {
            bFound = std::memcmp(pThisNode->m_item, pCompNode->m_item, sizeof(WeatherEntry)) == 0;
        }
        if (!bFound) {
            return true;
        }
    }
    return false;
}

int32_t TESWeatherList::SortCompare(WeatherEntry* p1, WeatherEntry* p2) {
    if (p1->uiChance <= p2->uiChance) {
        return (int32_t)((uint8_t)(p1->uiChance < p2->uiChance));
    }

    return (-1);
}

void TESWeatherList::LoadWeatherListChunk(TESWeatherList* apWeatherList, TESFile* apFile, TESForm* pForm) {
    if ((apFile) && (apWeatherList) && (pForm)) {
        uint32_t uiCount = apFile->GetChunkSize() / sizeof(tmpWeatherEntry);
        if (uiCount) {
            tmpWeatherEntry* tmpArray;
            apWeatherList->FlushList();
            // new tmpWeatherEntry[uiCount]: on overflow the size test passes SIZE_MAX to ourMalloc.
            void* pMemory =
                ourMalloc(uiCount <= SIZE_MAX / sizeof(tmpWeatherEntry) ? sizeof(tmpWeatherEntry) * uiCount : SIZE_MAX);
            if (pMemory) {
                rt::eh_vector_constructor_iterator(
                    pMemory, sizeof(tmpWeatherEntry), static_cast<int32_t>(uiCount),
                    &xfnv::constructElement<tmpWeatherEntry>
                );
                tmpArray = static_cast<tmpWeatherEntry*>(pMemory);
            }
            else {
                tmpArray = nullptr;
            }
            if (!apFile->GetChunkData(tmpArray, sizeof(tmpWeatherEntry) * uiCount)) {
                Warning("FORMS: Error getting TESWeatherList chunk for form ID: %d", pForm->iFormID);
            }
            for (uint32_t ui = 0; ui < uiCount; ++ui) {
                WeatherEntry* pEntry;
                if (apFile->QEndian()) {
                    tmpArray[ui].Endian();
                }
                TESForm::AddCompileIndex(tmpArray[ui].uiNumericID, apFile);
                WeatherEntry* apMemory = static_cast<WeatherEntry*>(ourMalloc(sizeof(WeatherEntry)));
                if (apMemory) {
                    pEntry = ::new (apMemory) WeatherEntry;
                }
                else {
                    pEntry = nullptr;
                }
                apWeatherList->AddHead(pEntry);
                WeatherEntry* m_item = apWeatherList->m_item;
                TESForm* FormByNumericID = TESForm::GetFormByNumericID(tmpArray[ui].uiNumericID);
                m_item->pWeather = dynamic_cast<TESWeather*>(FormByNumericID);
                if (!m_item->pWeather) {
                    Warning(
                        "DEFAULT: Error while loading weather list...cannot identify TESWeather from form "
                        "ID: "
                        "(%08X)",
                        tmpArray[ui].uiNumericID
                    );
                }
                apWeatherList->m_item->uiChance = tmpArray[ui].uiChance;
                TESForm::AddCompileIndex(tmpArray[ui].uiGlobalID, apFile);
                TESForm* pGlobalForm = TESForm::GetFormByNumericID(tmpArray[ui].uiGlobalID);
                apWeatherList->m_item->pChanceVar = dynamic_cast<TESGlobal*>(pGlobalForm);
            }
            apWeatherList->Sort(&TESWeatherList::SortCompare);
            ourFree(tmpArray);
        }
    }
}
