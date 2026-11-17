#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Visualize/hkCommandRouter.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessHandler.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkProcessContext;
class hkStreamReader;
class hkStreamWriter;

// The SDK's static hkServerProcessHandler::m_tag. Not named in the PDB or IDA; name chosen here.
inline int32_t _1268FB8;

// The visual debugger's per-client handler of processes: it routes the client's commands, and is itself the process
// that consumes the handler commands (register, select, create, delete). hkUint8 is spelled uint8_t.
class hkServerProcessHandler : public hkReferencedObject, public hkProcessHandler, public hkProcess {
public:
	hkServerProcessHandler(hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts, hkStreamReader* inStream,
		hkStreamWriter* outStream);
	~hkServerProcessHandler() override;

	void getConsumableCommands(uint8_t*& commands, int32_t& numCommands) override;
	void consumeCommand(uint8_t command) override;
	hkResult registerProcess(const char* name, int32_t id) override;
	hkResult selectProcess(int32_t id) override;
	hkResult createProcess(int32_t id) override;
	hkResult deleteProcess(int32_t id) override;
	int32_t getProcessId(const char* name) override;
	const char* getProcessName(int32_t tag) override;
	void step(float frameTimeInMs) override;
	// The SDK's inline; the exe's out-of-line copy 0xD61210 is registered in hkServerProcessHandler.cpp.
	int32_t getProcessTag() override { return _1268FB8; }

	int32_t findProcessByTag(int32_t tag);
	hkResult registerAllAvailableProcesss();

	// DeletingDestruct's free (hkReferencedObject's heap block); names one of the bases' operators.
	static void operator delete(void* apMemory) { hkReferencedObject::operator delete(apMemory); }

	hkCommandRouter m_commandRouter;
	hkArray<hkProcess*> m_processList;
	hkArray<hkProcessContext*> m_contexts;
};

// IDA name; not in the PDB. The commands the handler consumes (getConsumableCommands): the exe's bytes 0xC2, 0xC3.
using ServerProcessHandlerCommandArray = uint8_t[2];
inline ServerProcessHandlerCommandArray aAa{194, 195};
inline constexpr fnvd::Vtable hkServerProcessHandler_vftable_hkProcess{};
inline constexpr fnvd::Vtable hkServerProcessHandler_vftable_hkProcessHandler{};
inline constexpr fnvd::Vtable hkServerProcessHandler_vftable{};
