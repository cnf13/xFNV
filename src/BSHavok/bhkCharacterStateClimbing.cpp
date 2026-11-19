#include "BSHavok/bhkCharacterStateClimbing.h"

#include "BSHavok/bhkCharacterController.h"
#include "xfnv/Compiler.h"

hkpCharacterStateType bhkCharacterStateClimbing::getType() const {
    return HK_CHARACTER_CLIMBING;
}

// With no state wanted (HK_CHARACTER_MAX_STATE_ID) the character may jump off and keeps only the y of its direction, as
// the z of its output velocity (0, 0, kDirection.y, 0).
void bhkCharacterStateClimbing::Update(bhkCharacterController& arInput) {
    if (arInput.eWantState != HK_CHARACTER_MAX_STATE_ID) {
        arInput.UpdateState();
        return;
    }
    // ORs the whole 32-bit flags value.
    uint32_t& rFlags = arInput.iFlags;
    rFlags |= bhkCharacterController::CANJUMP;
    XFNV_M128_F32(arInput.OutVelocity.m_quad, 0) = 0.0f;
    XFNV_M128_F32(arInput.OutVelocity.m_quad, 1) = 0.0f;
    XFNV_M128_F32(arInput.OutVelocity.m_quad, 2) = XFNV_M128_F32(arInput.Direction.m_quad, 1);
    XFNV_M128_F32(arInput.OutVelocity.m_quad, 3) = 0.0f;
}
