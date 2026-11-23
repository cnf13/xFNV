#pragma once

#include <cstdint>

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
    // Returns whether the center moved.
    virtual bool SetCenter(int32_t aiX, int32_t aiY);
    virtual void Shift(int32_t aiCols, int32_t aiRows);
    virtual void Detach(uint32_t aiX, uint32_t aiY) = 0;
    virtual void ClearItem(uint32_t aiX, uint32_t aiY) = 0;
    virtual void MoveItem(uint32_t aiX1, uint32_t aiY1, uint32_t aiX2, uint32_t aiY2) = 0;
    virtual void SwapItem(uint32_t aiX1, uint32_t aiY1, uint32_t aiX2, uint32_t aiY2) = 0;

    // The grid's side length.
    uint32_t GetDimension();

    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory);

    int32_t iCenterX;
    int32_t iCenterY;
    uint32_t iDimension;
};
