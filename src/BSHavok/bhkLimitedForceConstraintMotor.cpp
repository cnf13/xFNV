#include "BSHavok/bhkLimitedForceConstraintMotor.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpLimitedForceConstraintMotor.h"

// The 4-byte component size is re-stored before each read; both reads are 4 bytes.
void bhkLimitedForceConstraintMotor::LoadBinary(NiStream& arStream, hkpLimitedForceConstraintMotor& arMotor) {
    uint32_t uiComponentSize = 4;
    arStream.m_pkIstr->BinaryRead(&arMotor.m_minForce, 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    arStream.m_pkIstr->BinaryRead(&arMotor.m_maxForce, 4, &uiComponentSize, 1);
}

// The 4-byte component size is re-stored before each write; both writes are 4 bytes.
void bhkLimitedForceConstraintMotor::SaveBinary(NiStream& arStream, hkpLimitedForceConstraintMotor& arMotor) {
    uint32_t uiComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(&arMotor.m_minForce, 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(&arMotor.m_maxForce, 4, &uiComponentSize, 1);
}
