#include "BSCore/HeapBlock.h"

HeapBlock::HeapBlock() {}

void HeapBlock::Init(int32_t aiSize, LogItem* apLogItem) {
    iMemSize = aiSize;
    pPrevious = nullptr;
}
