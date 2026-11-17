#pragma once

#include <cstdint>

class BSSimpleArray_void;

class BSParallelTask {
public:
    // Called with pArray and the item's index.
    void (*pCallBack)(BSSimpleArray_void* apArray, uint32_t auiIndex);
    BSSimpleArray_void* pArray;
    uint32_t uiSize;
    uint32_t uiPosition;
};
