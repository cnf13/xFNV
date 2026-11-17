#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// Gamebryo's base for the moving-object intersection tests: the time limit, the pseudodistance iteration settings and
// the contact the last test found. Subclasses supply the shape-specific Test, Find and Pseudodistance.
class NiIntersector {
public:
    enum IntersectionType : int32_t {
        IT_INVALID = 0x0,
        IT_EMPTY = 0x1,
        IT_CONTACT = 0x2,
        IT_OVERLAP = 0x3,
    };

    NiIntersector(float fMaxTime, float fDerivativeTimeStep, float fPseudodistanceThreshold, int32_t iMaxIterations);
    virtual ~NiIntersector();

    virtual void Test(const NiPoint3& kV0, const NiPoint3& kV1);
    virtual void Find(const NiPoint3& kV0, const NiPoint3& kV1);
    virtual float Pseudodistance(const NiPoint3&, const NiPoint3&, float) const = 0;
    virtual void ComputeContactInformation(const NiPoint3&, const NiPoint3&) = 0;

    float GetContactTime() const;
    const NiPoint3& GetContactPoint() const;
    const NiPoint3& GetContactNormal() const;

    // Intersectors are freed through MemoryManager::Deallocate.
    static void operator delete(void* apMemory);

    float m_fMaxTime;
    float m_fDerivativeTimeStep;
    float m_fInvDerivativeTimeStep;
    float m_fPseudodistanceThreshold;
    int32_t m_iMaxIterations;
    IntersectionType m_eIntersectionType;
    float m_fContactTime;
    NiPoint3 m_kContactPoint;
    NiPoint3 m_kContactNormal;
};
