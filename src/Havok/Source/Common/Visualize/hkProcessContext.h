#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Reconstruction.hpp"

class hkJobThreadPool;
class hkVisualDebugger;

// What a visual debugger process inspects (a physics context, for instance). Its code stays with the exe; src only
// declares the base so contexts convert to it.
class hkProcessContext {
public:
	virtual ~hkProcessContext();

	// The context's type name (pure in the SDK).
	virtual const char* getType() = 0;
	// Sets m_owner (the SDK's inline body).
	virtual void setOwner(hkVisualDebugger* vdb) { m_owner = vdb; }

	void syncTimers(hkJobThreadPool* threadPool);
	void addThreadPoolTimers(hkJobThreadPool* threadPool);

	// DeletingDestruct's free (ourFree).
	static void operator delete(void* apMemory);

	// The visual debugger serving the context.
	hkVisualDebugger* m_owner;
	hkInplaceArray<const char*, 6> m_monitorStreamBegins;
	hkInplaceArray<const char*, 6> m_monitorStreamEnds;
};
inline constexpr fnvd::Vtable hkProcessContext_vftable{};
