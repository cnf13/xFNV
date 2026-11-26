#include "Fallout Shared/GridArray.h"

#include "BSCore/NewOverloads.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

GridArray::GridArray() {}

GridArray::~GridArray() {}

void GridArray::Init() {
    iCenterY = -10000;
    iCenterX = -10000;
}

void GridArray::DetachAll() {
    for (uint32_t i = 0; i < iDimension; ++i) {
        for (uint32_t j = 0; j < iDimension; ++j) {
            Detach(i, j);
        }
    }
}

void GridArray::KillAll() {
    DetachAll();
}

// Moves the grid's center to (aiX, aiY), returning whether it moved.
bool GridArray::SetCenter(int32_t aiX, int32_t aiY) {
    int32_t iCols = aiX - iCenterX;
    int32_t iRows = aiY - iCenterY;
    Shift(iCols, iRows);
    return iCols || iRows;
}

void GridArray::Shift(int32_t aiCols, int32_t aiRows) {
    iCenterX += aiCols;
    iCenterY += aiRows;
    if (static_cast<uint32_t>(std::abs(aiCols)) < iDimension && static_cast<uint32_t>(std::abs(aiRows)) < iDimension) {
        while (aiCols) {
            if (aiCols >= 0) {
                for (uint32_t uiY = 0; uiY < iDimension; ++uiY) {
                    uint32_t uiCol = 0;
                    Detach(0, uiY);
                    while (uiCol < iDimension - 1) {
                        MoveItem(uiCol + 1, uiY, uiCol, uiY);
                        ++uiCol;
                    }
                    ClearItem(uiCol, uiY);
                }
                --aiCols;
            }
            else {
                for (uint32_t uiY = 0; uiY < iDimension; ++uiY) {
                    int32_t iBackCol = static_cast<int32_t>(iDimension - 1);
                    Detach(iBackCol, uiY);
                    while (iBackCol) {
                        MoveItem(iBackCol - 1, uiY, iBackCol, uiY);
                        --iBackCol;
                    }
                    ClearItem(0, uiY);
                }
                ++aiCols;
            }
        }
        while (aiRows) {
            if (aiRows >= 0) {
                for (uint32_t uiX = 0; uiX < iDimension; ++uiX) {
                    uint32_t uiRow = 0;
                    Detach(uiX, 0);
                    while (uiRow < iDimension - 1) {
                        MoveItem(uiX, uiRow + 1, uiX, uiRow);
                        ++uiRow;
                    }
                    ClearItem(uiX, uiRow);
                }
                --aiRows;
            }
            else {
                for (uint32_t uiX = 0; uiX < iDimension; ++uiX) {
                    int32_t iBackRow = static_cast<int32_t>(iDimension - 1);
                    Detach(uiX, iBackRow);
                    while (iBackRow) {
                        MoveItem(uiX, iBackRow - 1, uiX, iBackRow);
                        --iBackRow;
                    }
                    ClearItem(uiX, 0);
                }
                ++aiRows;
            }
        }
    }
    else {
        DetachAll();
    }
}

void GridArray::operator delete(void* apMemory) {
    ourFree(apMemory);
}
