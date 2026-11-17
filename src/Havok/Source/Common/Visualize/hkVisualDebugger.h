#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Stopwatch/hkStopwatch.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkProcess;
class hkProcessContext;
class hkServerProcessHandler;
class hkSocket;
class hkStreamReader;
class hkStreamWriter;
class hkVtableClassRegistry;

// The function the debugger calls when an object starts or stops being tracked.
using hkVisualDebuggerTrackedObjectCallback = void (*)(void* ptr, const hkClass* klass, hkBool wasAdded, hkUlong tagId,
	void* userHandle);

// A connected client: its socket and the process handler serving it.
struct hkVisualDebuggerClient {
	hkSocket* m_socket;
	hkServerProcessHandler* m_processHandler;
};

// An object the debugger tracks: the object, its class and its tag.
struct hkVisualDebuggerTrackedObject {
	void* m_ptr;
	const hkClass* m_class;
	// Callers pass an address, such as &entity->m_collidable.
	hkUlong m_tag;
};

// The Havok visual debugger server. Only the calls the game makes into it are declared.
class hkVisualDebugger : public hkReferencedObject {
public:
	hkVisualDebugger(const hkArray<hkProcessContext*>* contexts, const hkVtableClassRegistry* classReg);

	~hkVisualDebugger() override;

	// Serves one frame to the connected clients.
	virtual void step(float frameTimeInMs);
	// Accepts the clients waiting on the server socket.
	virtual void pollForNewClients();

	// Starts listening for clients on the port.
	void serve(int32_t listenPort);

	// Captures the simulation state to the named file through a new client.
	void capture(const char* captureFilename);

	// The processes of every client's handler, appended to processes.
	void getCurrentProcesses(hkArray<hkProcess*>& process) const;
	// Drops callback and its handle, the last pair moving into their place. Returns the index searched to (the count
	// when absent).
	int32_t removeTrackedObjectCallback(hkVisualDebuggerTrackedObjectCallback callback);
	// Drops the object a physics listener reports gone. Its parameter's type is unknown: the callers pass an entity, a
	// phantom or an action.
	void removeTrackedObject(void* obj);
	// Deletes every client whose socket is gone, from the last one
	// down.
	void deleteDeadClients();

protected:
	// Releases client index's handler and socket; the last client moves into its slot.
	void deleteClient(int32_t i);
	// Adds a client for the socket, its reader and its writer, the writer referenced.
	void createClient(hkSocket* socket, hkStreamReader* reader, hkStreamWriter* writer);
	// Writes one step of the frame time to client aiIndex.
	void writeStep(int32_t i, float t);

public:
	void SupressPollForNewClient(bool val);
	void shutdown();
	void addDefaultProcess(const char* viewerName);
	void addTrackedObject(void* obj, const hkClass& klass, const char* group, hkUlong tagId);
	void removeDefaultProcess(const char* processName);
	void addTrackedObjectCallback(hkVisualDebuggerTrackedObjectCallback callback, void* userHandle);

	hkSocket* m_server;
	bool s_SuppressPollForNewClients;
	hkArray<hkVisualDebuggerClient> m_clients;
	hkArray<hkProcessContext*> m_contexts;
	hkArray<hkVisualDebuggerTrackedObject> m_trackedObjects;
	hkArray<hkVisualDebuggerTrackedObjectCallback> m_trackCallbacks;
	hkArray<void*> m_trackCallbackHandles;
	const hkVtableClassRegistry* m_classReg;
	// The processes every new client gets.
	hkArray<hkStringPtr> m_defaultProcesses;
	// Whether the debugger measures the frame time itself instead of taking the caller's.
	hkBool m_amTimingFrame;
	// The frame timer step() reads and restarts.
	hkStopwatch m_frameTimer;
};
