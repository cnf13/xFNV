#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInfo.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Reconstruction.hpp"

// What owns constraints: an island or the world. It adds up the solver memory the owned constraints need. The four
// virtual functions are empty in the base.
class hkpConstraintOwner : public hkReferencedObject {
public:
    virtual void addConstraintToCriticalLockedIsland(hkpConstraintInstance* constraint) {}
    virtual void removeConstraintFromCriticalLockedIsland(hkpConstraintInstance* constraint) {}
    virtual void addCallbackRequest(hkpConstraintInstance* constraint, int32_t request) {}
    virtual void checkAccessRw() {}

    // Adds delta to the owner's and the constraint's info, with hkpConstraintInfo::add,
    // hkConstraintInternal::addConstraintInfo and hkMath::max2 written out.
    void addConstraintInfo(hkpConstraintInstance* constraint, hkpConstraintInfo& delta) {
        if (constraint->m_internal) {
            int32_t iMax = m_constraintInfo.m_maxSizeOfSchema;
            iMax = iMax > delta.m_maxSizeOfSchema ? iMax : delta.m_maxSizeOfSchema;
            m_constraintInfo.m_maxSizeOfSchema = iMax;
            iMax = iMax > delta.m_sizeOfSchemas ? iMax : delta.m_sizeOfSchemas;
            m_constraintInfo.m_maxSizeOfSchema = iMax;
            m_constraintInfo.m_sizeOfSchemas += delta.m_sizeOfSchemas;
            m_constraintInfo.m_numSolverResults += delta.m_numSolverResults;
            m_constraintInfo.m_numSolverElemTemps += delta.m_numSolverElemTemps;

            hkConstraintInternal* pInternal = constraint->m_internal;
            const uint32_t numSolverResults = pInternal->m_numSolverResults + delta.m_numSolverResults;
            const uint32_t numSolverElemTemps = pInternal->m_numSolverElemTemps + delta.m_numSolverElemTemps;
            const uint32_t sizeOfSchemas = pInternal->m_sizeOfSchemas + delta.m_sizeOfSchemas;
            pInternal->m_numSolverResults = static_cast<uint16_t>(numSolverResults);
            pInternal->m_numSolverElemTemps = static_cast<uint16_t>(numSolverElemTemps);
            pInternal->m_sizeOfSchemas = static_cast<uint16_t>(sizeOfSchemas);

            const int32_t iSchemas = constraint->m_internal->m_sizeOfSchemas;
            m_constraintInfo.m_maxSizeOfSchema =
                m_constraintInfo.m_maxSizeOfSchema > iSchemas ? m_constraintInfo.m_maxSizeOfSchema : iSchemas;
        }
    }

    // Subtracts delta from the owner's and the constraint's info, with hkpConstraintInfo::sub and
    // hkConstraintInternal::subConstraintInfo written out.
    void subConstraintInfo(hkpConstraintInstance* constraint, hkpConstraintInfo& delta) {
        if (constraint->m_internal) {
            m_constraintInfo.m_sizeOfSchemas -= delta.m_sizeOfSchemas;
            m_constraintInfo.m_numSolverResults -= delta.m_numSolverResults;
            m_constraintInfo.m_numSolverElemTemps -= delta.m_numSolverElemTemps;

            hkConstraintInternal* pInternal = constraint->m_internal;
            const uint32_t numSolverResults = pInternal->m_numSolverResults - delta.m_numSolverResults;
            const uint32_t numSolverElemTemps = pInternal->m_numSolverElemTemps - delta.m_numSolverElemTemps;
            const uint32_t sizeOfSchemas = pInternal->m_sizeOfSchemas - delta.m_sizeOfSchemas;
            pInternal->m_numSolverResults = static_cast<uint16_t>(numSolverResults);
            pInternal->m_numSolverElemTemps = static_cast<uint16_t>(numSolverElemTemps);
            pInternal->m_sizeOfSchemas = static_cast<uint16_t>(sizeOfSchemas);
        }
    }

    // Declared as hkpConstraintInfo rather than hkpConstraintInfoSpu2, which holds the same four counts on PC.
    hkpConstraintInfo m_constraintInfo;
};
