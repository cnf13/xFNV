#pragma once

#include "BSSystem/BSTaskletManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

// The Havok tasklet callback interface and the per-thread routers shared by its startup and exit callbacks.
class bhkTaskletManagerCallback : public BSTaskletManagerCallback {
public:
	// The startup and exit callbacks are built with BSTaskletManagerCallback's constructor and their phase.
	using BSTaskletManagerCallback::BSTaskletManagerCallback;

	struct bhkThreadStateElem {
		bhkThreadStateElem();

		uint32_t ThreadID;
		hkMemoryRouter LocalRouter;
		void* pStackBuffer;
	};

	struct bhkThreadState {
		bhkThreadState();

		// Sets up the calling tasklet thread's Havok memory router in the next free element, under StateLock.
		void CreateThreadMemory();

		// Destroys the elements, then StateLock.
		static void Destruct(bhkThreadState* apThis);

		_RTL_CRITICAL_SECTION StateLock;
		bhkThreadStateElem Elem[16];
		uint32_t iNumElem;
	};

	static bhkTaskletManagerCallback::bhkThreadState sbhkThreadState;
};
