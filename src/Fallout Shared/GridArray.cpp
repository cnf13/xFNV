#include "Fallout Shared/GridArray.h"

#include "BSCore/MemoryManager.h"
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
	int32_t v6 = aiX - iCenterX;
	int32_t v5 = aiY - iCenterY;
	Shift(v6, v5);
	return v6 || v5;
}

void GridArray::Shift(int32_t aiCols, int32_t aiRows) {
	int32_t v4;
	uint32_t v5;
	uint32_t m;
	uint32_t k;
	uint32_t j;
	uint32_t i;
	int32_t v10;
	uint32_t v11;
	iCenterX += aiCols;
	iCenterY += aiRows;
	if ((((uint32_t)std::abs(aiCols)) < iDimension) && (((uint32_t)std::abs(aiRows)) < iDimension)) {
		while (aiCols) {
			if (aiCols >= 0) {
				for (i = 0; i < iDimension; ++i) {
					v5 = 0;
					Detach(0, (int32_t)i);
					while (v5 < (iDimension - 1)) {
						MoveItem((int32_t)(v5 + 1), (int32_t)i, (int32_t)v5, (int32_t)i);
						++v5;
					}
					ClearItem((int32_t)v5, (int32_t)i);
				}
				--aiCols;
			} else {
				for (j = 0; j < iDimension; ++j) {
					v4 = (int32_t)(iDimension - 1);
					Detach(v4, (int32_t)j);
					while (v4) {
						MoveItem(v4 - 1, (int32_t)j, v4, (int32_t)j);
						--v4;
					}
					ClearItem(0, (int32_t)j);
				}
				++aiCols;
			}
		}
		while (aiRows) {
			if (aiRows >= 0) {
				for (k = 0; k < iDimension; ++k) {
					v11 = 0;
					Detach((int32_t)k, 0);
					while (v11 < (iDimension - 1)) {
						MoveItem((int32_t)k, (int32_t)(v11 + 1), (int32_t)k, (int32_t)v11);
						++v11;
					}
					ClearItem((int32_t)k, (int32_t)v11);
				}
				--aiRows;
			} else {
				for (m = 0; m < iDimension; ++m) {
					v10 = (int32_t)(iDimension - 1);
					Detach((int32_t)m, v10);
					while (v10) {
						MoveItem((int32_t)m, v10 - 1, (int32_t)m, v10);
						--v10;
					}
					ClearItem((int32_t)m, 0);
				}
				++aiRows;
			}
		}
	} else {
		DetachAll();
	}
}

void GridArray::operator delete(void* apMemory) {
	ourFree(apMemory);
}
