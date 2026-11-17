#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkpRigidBody;

// An operation the world runs on one body after its collision step; the world's operation queue defers them.
// hkpBodyOperationEntry keeps a pointer to it and references are taken on it through the hkReferencedObject base. Its
// destructor override is not declared yet.
class hkpBodyOperation : public hkReferencedObject {
public:
    enum ExecutionState : int32_t {
        EXECUTE_SINGLE_THREADED = 0,
        EXECUTE_MULTI_THREADED = 1,
    };

    // What an operation reports back to the queue that runs it (Havok SDK; the SDK's constructor clears
    // m_bodyIsDeleted).
    struct UpdateInfo {
        hkBool m_bodyIsDeleted;
    };

    // Runs the operation on body. hkpWorldOperationQueue::executeAllPendingBodyOperations passes a freshly cleared
    // UpdateInfo for each call.
    virtual void execute(hkpRigidBody* body, UpdateInfo& arUpdateInfo) = 0;
};
