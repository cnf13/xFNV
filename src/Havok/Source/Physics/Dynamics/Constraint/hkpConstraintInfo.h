#pragma once

#include <cstdint>

// How much solver memory a constraint needs: its schema size, the number of solver results and element temporaries.
struct hkpConstraintInfo {
    // Zeroes the four counts. The SDK's inline.
    void clear() {
        m_maxSizeOfSchema = 0;
        m_sizeOfSchemas = 0;
        m_numSolverResults = 0;
        m_numSolverElemTemps = 0;
    }

    // Adds a command's cost to the totals.
    void add(int32_t schemaSize, int32_t numSolverResults, int32_t numSolverTempElems) {
        m_sizeOfSchemas += schemaSize;
        m_numSolverResults += numSolverResults;
        m_numSolverElemTemps += numSolverTempElems;
    }

    void merge(const hkpConstraintInfo& other) {
        m_maxSizeOfSchema = m_maxSizeOfSchema > other.m_maxSizeOfSchema ? m_maxSizeOfSchema : other.m_maxSizeOfSchema;
        m_sizeOfSchemas += other.m_sizeOfSchemas;
        m_numSolverResults += other.m_numSolverResults;
        m_numSolverElemTemps += other.m_numSolverElemTemps;
    }

    int32_t m_maxSizeOfSchema;
    int32_t m_sizeOfSchemas;
    int32_t m_numSolverResults;
    int32_t m_numSolverElemTemps;
};
