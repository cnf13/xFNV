#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkDebugDisplayHandler;
class hkDisplaySerializeIStream;
class hkDisplaySerializeOStream;
class hkProcessHandler;

// A visual debugger process: it reads commands from the client stream and draws through its display handler.
class hkProcess {
public:
	// The constructor stores only m_selectable.
	hkProcess(hkBool selectable);
	virtual ~hkProcess();

	// The tag the process was registered under.
	virtual int32_t getProcessTag() = 0;
	virtual void getConsumableCommands(uint8_t*& commands, int32_t& numCommands);
	virtual void consumeCommand(uint8_t command) {}
	virtual bool isOk();
	virtual void init() {}
	virtual void step(float frameTimeInMs) {}

	// DeletingDestruct's free (ourFree).
	static void operator delete(void* apMemory);

	hkBool m_selectable;
	uint8_t padding05[3];
	hkDisplaySerializeIStream* m_inStream;
	hkDisplaySerializeOStream* m_outStream;
	hkDebugDisplayHandler* m_displayHandler;
	hkProcessHandler* m_processHandler;
};
