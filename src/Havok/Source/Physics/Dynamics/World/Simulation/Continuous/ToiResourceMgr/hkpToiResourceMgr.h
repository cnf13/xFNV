#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

class hkContactPoint;
class hkpConstraintInstance;
class hkpContactPointProperties;
struct hkpToiEvent;
struct hkpToiResources;

// What a resource manager answers when the solver cannot continue a time of impact step.
enum hkpToiResourceMgrResponse : int32_t {
    HK_TOI_RESOURCE_MGR_RESPONSE_CONTINUE = 0x0,
    HK_TOI_RESOURCE_MGR_RESPONSE_DO_NOT_EXPAND_AND_CONTINUE = 0x1,
    HK_TOI_RESOURCE_MGR_RESPONSE_BACKSTEP = 0x2,
};

// The interface that provides the memory a time of impact solve needs.
class hkpToiResourceMgr : public hkReferencedObject {
public:
    // A constraint the solver could not satisfy, with the contact point that violates it (hkpdynamics.pdb, sizeof
    // 12; hkLs_localizedSolveToi frees its array of them as 12 * capacity bytes).
    struct ConstraintViolationInfo {
        hkpConstraintInstance* m_constraint;
        const hkContactPoint* m_contactPoint;
        const hkpContactPointProperties* m_contactPointProperties;
    };

    // The hkResult is spelled int32_t (0 is success).
    virtual hkResult beginToiAndSetupResources(
        const hkpToiEvent& event,
        const hkArray<hkpToiEvent>& allEvents,
        hkpToiResources& resources
    ) = 0;
    virtual hkpToiResourceMgrResponse cannotSolve(hkArray<ConstraintViolationInfo>& violatedConstraints) = 0;
    virtual hkpToiResourceMgrResponse resourcesDepleted() = 0;
    virtual void endToiAndFreeResources(
        const hkpToiEvent& event,
        const hkArray<hkpToiEvent>& allEvents,
        const hkpToiResources& resources
    ) = 0;
};
