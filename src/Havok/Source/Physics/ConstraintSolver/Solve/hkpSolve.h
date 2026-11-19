#pragma once

#include <stdint.h>

class hkpConstraintInstance;
class hkpJacobianSchema;
struct hkpSolverInfo;
class hkpSolverResults;
class hkpVelocityAccumulator;

// Writes the relative velocity of each Jacobian row of one constraint's schemas into velocitiesOut (contact rows
// clamped to at most 0), stopping at the constraint's end or before maxNumVelocities would be passed; returns the
// number written. The SDK's hkpSolve.h signature.
int32_t hkSolveGetToiViolatingConstraintVelocity(
    hkpSolverInfo& i,
    const hkpJacobianSchema* schemas,
    const hkpVelocityAccumulator* accums,
    int32_t maxNumVelocities,
    float* velocitiesOut
);

// The sizes of the solver's schemas in bytes, for uncompressed schemas. Three depend on the pointer size: the header,
// the 3D friction and the 2D rolling friction schemas hold pointers and are 16, 144 and 80 bytes for 4-byte pointers,
// 32, 160 and 96 for 8-byte pointers.
#define HK_SIZE_OF_JACOBIAN_END_SCHEMA 16
#if XFNV_64BIT
#define HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA 32
#else
#define HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA 16
#endif
#define HK_SIZE_OF_JACOBIAN_SINGLE_CONTACT_SCHEMA 48
#define HK_SIZE_OF_JACOBIAN_PAIR_CONTACT_SCHEMA 112
#define HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA 112
#if XFNV_64BIT
#define HK_SIZE_OF_JACOBIAN_3D_FRICTION_SCHEMA 160
#define HK_SIZE_OF_JACOBIAN_2D_ROLLING_FRICTION_SCHEMA 96
#else
#define HK_SIZE_OF_JACOBIAN_3D_FRICTION_SCHEMA 144
#define HK_SIZE_OF_JACOBIAN_2D_ROLLING_FRICTION_SCHEMA 80
#endif
#define HK_SIZE_OF_JACOBIAN_SET_MASS_SCHEMA 32
#define HK_SIZE_OF_JACOBIAN_ADD_VELOCITY_SCHEMA 32
#define HK_SIZE_OF_JACOBIAN_SET_CENTER_OF_MASS_SCHEMA 96

// One constraint whose impulse exceeded its limit during the solve; m_type is aligned to 16 bytes.
struct alignas(16) hkpImpulseLimitBreachedElem {
    uint16_t m_type;
    uint16_t m_isContact;
    hkpConstraintInstance* m_constraintInstance;
    hkpSolverResults* m_solverResult;
};

// The head of one block of breached elements: their count and the next block; the elements follow the header. The
// header is 16-byte aligned.
struct alignas(16) hkpImpulseLimitBreachedHeader {
    int32_t m_numBreached;
    hkpImpulseLimitBreachedHeader* m_next;
};
