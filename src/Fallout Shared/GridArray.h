#pragma once

#include "Reconstruction.hpp"

// The square grid of loaded cells centered on the cell the player stands in.
class GridArray {
public:
	GridArray();
	virtual ~GridArray();

	// resets the center to the "no center" value
	virtual void Init();
	// detaches every item of the grid
	virtual void DetachAll();
	virtual void KillAll();
	// Returns whether the center moved: the exe's callers test al.
	virtual bool SetCenter(int32_t aiX, int32_t aiY);
	virtual void Shift(int32_t aiCols, int32_t aiRows);
	virtual void Detach(uint32_t aiX, uint32_t aiY) = 0;
	virtual void ClearItem(uint32_t aiX, uint32_t aiY) = 0;
	virtual void MoveItem(uint32_t aiX1, uint32_t aiY1, uint32_t aiX2, uint32_t aiY2) = 0;
	virtual void SwapItem(uint32_t aiX1, uint32_t aiY1, uint32_t aiX2, uint32_t aiY2) = 0;

	// The grid's side length (thiscall, ret 0, the shared +0xC getter 0x84E3A0).
	uint32_t GetDimension();

	// The exe's deleting destructor (0x4B9E20) frees through ourFree.
	static void operator delete(void* apMemory);

	int32_t iCenterX;
	int32_t iCenterY;
	uint32_t iDimension;
};
inline constexpr fnvd::Vtable GridArray_vftable{};
