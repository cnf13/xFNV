#pragma once

#include <cstdint>

// The single billboard a speed tree is drawn as at a distance: its twelve coordinates and whether they are valid.
class CSimpleBillboard {
public:
    // The unit billboard the tree billboards scale: four corners of three coordinates.
    using UnitBillboard = float[4][3];

    CSimpleBillboard();

    const float* GetBillboardCoords(float fWidth, float fHeight);

    // Rotates the unit billboard's four corners to face the camera direction pCameraDir.
    static void ComputeUnitBillboard(const float* pCameraDir);

    float m_acTreeBillboard[12];
    bool m_bValid;
    uint8_t padding31[3];

    static inline UnitBillboard m_acUnitBillboard =
        {{0.0f, 0.5f, 1.0f}, {0.0f, -0.5f, 1.0f}, {0.0f, -0.5f, 0.0f}, {0.0f, 0.5f, 0.0f}};
};
