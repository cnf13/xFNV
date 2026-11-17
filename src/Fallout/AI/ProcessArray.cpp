#include "Fallout/AI/ProcessArray.h"

#include "Fallout/AI/MobileObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"

// 1000 empty actor slots and every level empty.
ProcessArray::ProcessArray() : AIProcessArray(0, 1) {
    AIProcessArray.SetSize(1000u);
    for (uint32_t i = 0; i < 1000; ++i) {
        AIProcessArray.SetAt(i, nullptr);
    }
    for (int32_t p = 0; p < PROCESS_TYPE_COUNT; ++p) {
        ProcessLevelHeads[p] = 0;
        ProcessLevelTails[p] = 0;
        ProcessLevelNextUpdate[p] = 0;
    }
}

ProcessArray::~ProcessArray() {}

PROCESS_TYPE ProcessArray::GetProcessLevelFromIndex(uint32_t auiIndex) const {
    for (PROCESS_TYPE eLevel = PROCESS_TYPE_HIGH; eLevel < PROCESS_TYPE_COUNT;
         eLevel = static_cast<PROCESS_TYPE>(eLevel + 1)) {
        if (auiIndex >= ProcessLevelHeads[eLevel] && auiIndex < ProcessLevelTails[eLevel]) {
            return eLevel;
        }
    }
    return PROCESS_TYPE_NONE;
}

int32_t ProcessArray::GetIndexFromActor(MobileObject* apActor, PROCESS_TYPE aProcessLevel) {
    if (!apActor) {
        return -1;
    }
    // Scan from the level head through the end of the whole buffer; the tail of the low level bounds every search.
    for (uint32_t auiIndex = ProcessLevelHeads[aProcessLevel]; auiIndex < ProcessLevelTails[PROCESS_TYPE_LOW];
         ++auiIndex) {
        if (AIProcessArray[auiIndex] == apActor) {
            return static_cast<int32_t>(auiIndex);
        }
    }
    return -1;
}

uint32_t ProcessArray::GetProcessLevelHead(PROCESS_TYPE aProcessLevel) const {
    return ProcessLevelHeads[aProcessLevel];
}

uint32_t ProcessArray::GetProcessLevelTail(PROCESS_TYPE aProcessLevel) const {
    return ProcessLevelTails[aProcessLevel];
}

void ProcessArray::AddActor(MobileObject* apActor, PROCESS_TYPE aWhichList) {
    if (aWhichList != PROCESS_TYPE_NONE) {
        // level after the actor's, valid unless this is the low level
        PROCESS_TYPE eNextLevel = static_cast<PROCESS_TYPE>(aWhichList + 1);
        if (aWhichList < PROCESS_TYPE_LOW && ProcessLevelTails[aWhichList] + 1 > ProcessLevelHeads[eNextLevel]) {
            // no room left in this level: push the first actor of the next
            // level one level deeper first
            MobileObject* apMoved = AIProcessArray[ProcessLevelHeads[eNextLevel]];
            AddActor(apMoved, eNextLevel);
            ProcessLevelHeads[eNextLevel] = ProcessLevelTails[aWhichList] + 1;
        }
        AIProcessArray.SetAt(ProcessLevelTails[aWhichList], apActor);
        ++ProcessLevelTails[aWhichList];
        if (ProcessLevelTails[aWhichList] >= AIProcessArray.GetSize()) {
            GrowArraySize(100);
        }
    }
    // Re-clamps the next-update cursors after every add, also for PROCESS_TYPE_NONE.
    CheckNextUpdateBounds();
}

void ProcessArray::RemoveActor(MobileObject* apActor, PROCESS_TYPE aWhichList) {
    RemoveActor(GetIndexFromActor(apActor, aWhichList));
}

void ProcessArray::RemoveActor(uint32_t auiIndex) {
    PROCESS_TYPE eLevel = GetProcessLevelFromIndex(auiIndex);
    if (eLevel == PROCESS_TYPE_NONE) {
        return;
    }
    if (ProcessLevelTails[eLevel] > ProcessLevelHeads[eLevel]) {
        --ProcessLevelTails[eLevel];
    }
    if (auiIndex != ProcessLevelTails[eLevel]) {
        // fill the hole with the level's last actor
        AIProcessArray.SetAt(auiIndex, AIProcessArray[ProcessLevelTails[eLevel]]);
    }
    MobileObject* apNull = nullptr;
    AIProcessArray.SetAt(ProcessLevelTails[eLevel], apNull);
    CheckNextUpdateBounds();
}

void ProcessArray::GrowArraySize(int32_t aSize) {
    int32_t lastsize = AIProcessArray.m_usSize;
    AIProcessArray.SetSize(static_cast<uint32_t>(aSize + lastsize));
    for (uint32_t i = lastsize;; ++i) {
        int32_t lastsize_2 = AIProcessArray.m_usSize;
        if (i >= static_cast<uint32_t>(lastsize_2)) {
            break;
        }
        MobileObject* a3 = nullptr;
        AIProcessArray.SetAt(i, a3);
    }
}

void ProcessArray::CheckNextUpdateBounds() {
    int32_t i;
    for (i = 0; i < 4; ++i) {
        if ((ProcessLevelNextUpdate[i] < ProcessLevelHeads[i]) || (ProcessLevelNextUpdate[i] >= ProcessLevelTails[i])) {
            ProcessLevelNextUpdate[i] = ProcessLevelHeads[i];
        }
    }
}

uint32_t ProcessArray::GetSize() {
    return ProcessLevelTails[PROCESS_TYPE_LOW];
}

MobileObject* ProcessArray::GetItem(uint32_t auiIndex) {
    return AIProcessArray.GetAt(auiIndex);
}
