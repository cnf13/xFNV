#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

class hkpRigidBody;

// An operation the world runs on one body after its collision step; the world's operation queue defers them. The
// exe keeps a pointer to it in hkpBodyOperationEntry and takes references on it through the hkReferencedObject base,
// which sits at offset 0. Its destructor override is not declared yet.
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

	// Slot 3: runs the operation on body. hkpWorldOperationQueue::executeAllPendingBodyOperations (0xCF7AE0) builds
	// a cleared UpdateInfo on its stack for each call (`mov byte [esp+0x13], bl`) and passes its address.
	virtual void execute(hkpRigidBody* body, UpdateInfo& arUpdateInfo) = 0;
};
