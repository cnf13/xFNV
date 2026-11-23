#include "Havok/Source/Common/Visualize/hkVisualDebugger.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkVtableClassRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/FileSystem/hkFileSystem.h"
#include "Havok/Source/Common/Base/System/Io/OArchive/hkOArchive.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/System/Io/Socket/hkSocket.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/System/Stopwatch/hkStopwatch.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Visualize/Process/hkDebugDisplayProcess.h"
#include "Havok/Source/Common/Visualize/Serialize/hkDisplaySerializeOStream.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Common/Visualize/hkProcessHandler.h"
#include "Havok/Source/Common/Visualize/hkProcessRegisterUtil.h"
#include "Havok/Source/Common/Visualize/hkServerProcessHandler.h"
#include "Havok/Source/Common/Visualize/hkVersionReporter.h"

#include <cmath>

// The contexts copied and told their owner, the report, the common processes registered and the three default viewers
// added. hkStopwatch has no constructor, so the name and reset() are done here.
hkVisualDebugger::hkVisualDebugger(const hkArray<hkProcessContext*>* contexts, const hkVtableClassRegistry* classReg)
  : m_server(nullptr), s_SuppressPollForNewClients(false), m_classReg(classReg), m_amTimingFrame(false) {
    m_frameTimer.m_name = "Frame Timer";
    m_frameTimer.reset();
    m_contexts.copyFromArray(*hkMemoryRouter::getInstance().m_heap, *contexts, hkTypeIsPod());
    for (int32_t i = 0; i < m_contexts.m_size; ++i) {
        m_contexts.m_data[i]->setOwner(this);
    }
    hkError::s_instance->sectionBegin(0x1293ADEF, "Visual Debugger");
    {
        char acBuffer[512];
        hkErrStream kStream(acBuffer, 512);
        kStream << "VDB Server instance has been created";
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\hkVisualDebugger.cpp", 40);
    }
    hkProcessRegisterUtil::registerAllCommonProcesses();
    addDefaultProcess(hkDebugDisplayProcess::getName());
    addDefaultProcess("Shapes");
    addDefaultProcess("MousePicking");
    if (m_classReg) {
        m_classReg->addReference();
    }
    hkError::s_instance->sectionEnd();
}

// The SDK's destructor: shut down, the contexts let go, the default processes dropped and the class registry
// released; the arrays free themselves.
hkVisualDebugger::~hkVisualDebugger() {
    shutdown();
    for (int32_t i = 0; i < m_contexts.m_size; ++i) {
        m_contexts.m_data[i]->setOwner(nullptr);
    }
    m_defaultProcesses.clear();
    if (m_classReg) {
        m_classReg->removeReference();
    }
}

// Serves one frame. When the debugger times the frame itself, the elapsed time (rounded to a float of milliseconds)
// replaces the caller's frame time; the timing then stops and the flag is set. The frame time falls back to 16 ms when
// the caller passes 0 (or a value of magnitude at most FLT_EPSILON). Each client is asked twice whether its handler is
// still alive: once to step and write it, and once more to drop it, so a handler that dies during its own step is
// cleaned up at once.
void hkVisualDebugger::step(float frameTimeInMs) {
    float fFrameTime = 16.0f;
    if (m_amTimingFrame.m_bool) {
        // The elapsed ticks are read before the clock's rate; clang would evaluate divide64's arguments right to left,
        // so the ticks go through a local first.
        const uint64_t uiTicks = m_frameTimer.getElapsedTicks();
        fFrameTime = static_cast<float>(hkStopwatch::divide64(uiTicks, hkStopwatch::getTicksPerSecond()) * 1000.0);
        m_frameTimer.stop();
    }
    m_amTimingFrame.m_bool = 1;
    if (std::fabsf(frameTimeInMs) > HK_REAL_EPSILON) {
        fFrameTime = frameTimeInMs;
    }
    pollForNewClients();
    for (int32_t i = m_clients.m_size - 1; i >= 0; --i) {
        if (m_clients.m_data[i].m_processHandler->isOk()) {
            m_clients.m_data[i].m_processHandler->step(fFrameTime);
            writeStep(i, fFrameTime);
        }
        if (!m_clients.m_data[i].m_processHandler->isOk()) {
            hkError::s_instance->sectionBegin(0x76E3A642, "Client Dies");
            alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
            char acBuffer[0x200];
            hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
            *pStream << "Client has died, cleaning up (host name not available at present)";
            hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\hkVisualDebugger.cpp", 288);
            pStream->~hkErrStream();
            hkError::s_instance->sectionEnd();
            deleteClient(i);
        }
    }
    // The virtual calls above can clear the flag, so it is tested again before restarting the timer.
    if (m_amTimingFrame.m_bool) {
        m_frameTimer.reset();
        m_frameTimer.start();
    }
}

void hkVisualDebugger::pollForNewClients() {
    if (!s_SuppressPollForNewClients) {
        if (m_amTimingFrame.m_bool && m_frameTimer.isRunning()) {
            m_frameTimer.stop();
        }

        if (m_server) {
            hkSocket* clientSocket = m_server->pollForNewClient();
            if (clientSocket) {
                char buf[512];
                {
                    hkErrStream ostr(buf, 512);
                    ostr << "A new network client has been received (host name not available at present)";
                    hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\hkVisualDebugger.cpp", 232);
                }
                createClient(clientSocket, &clientSocket->m_reader, &clientSocket->m_writer);
            }
        }
        if (m_amTimingFrame.m_bool) {
            m_frameTimer.start();
        }
    }
}

// Creates the server socket through the platform's factory and listens on the port; one report on every path.
void hkVisualDebugger::serve(int32_t listenPort) {
    char description[512];
    hkError::s_instance->sectionBegin(0x1293ADE8, "Serving");
    if (m_server) {
        hkErrStream kStream(description, 512);
        kStream << "Server has already been created, only one server allowed per visual debugger instance";
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, description, ".\\hkVisualDebugger.cpp", 112);
    }
    else {
        m_server = hkSocket::create();
        if (m_server) {
            m_server->listen(listenPort);
            hkErrStream kStream(description, 512);
            kStream << "Server created and will poll for new client(s) on port " << listenPort << " every frame";
            hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, description, ".\\hkVisualDebugger.cpp", 103);
        }
        else {
            hkErrStream kStream(description, 512);
            kStream << "Server could not be created, please check that you platform supports sockets with the hkBase "
                       "library";
            hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, description, ".\\hkVisualDebugger.cpp", 107);
        }
    }
    hkError::s_instance->sectionEnd();
}

// Captures to a file: the file system's writer becomes the new client's writer (the client is appended, so the last
// client is the new one), and every default process is created on that client's handler and listed in the report. A
// writer that cannot be opened, or that reports not ok, gets a message instead.
void hkVisualDebugger::capture(const char* captureFilename) {
    hkError::s_instance->sectionBegin(0x1293ADE7, "Capturing");
    hkStreamWriter* pWriter = hkFileSystem::s_instance->openWriter(captureFilename);
    // Tests isOk's result byte rather than using hkBool's conversion operator.
    if (pWriter && pWriter->isOk().m_bool) {
        alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
        char acBuffer[0x200];
        hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
        *pStream << "Capturing simulation state to '" << captureFilename << "'";
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\hkVisualDebugger.cpp", 125);
        pStream->~hkErrStream();
        createClient(nullptr, nullptr, pWriter);
        pWriter->removeReference();
        hkVisualDebuggerClient* pClient = &m_clients.m_data[m_clients.m_size - 1];
        hkStringBuf kViewers;
        kViewers = "Turning on the following viewers: [";
        for (const hkStringPtr& kProcessName : m_defaultProcesses) {
            const char* pcName = kProcessName.cString();
            const int32_t iId = pClient->m_processHandler->getProcessId(pcName);
            if (iId >= 0) {
                kViewers.appendJoin(" ", pcName, nullptr, nullptr, nullptr, nullptr);
                pClient->m_processHandler->createProcess(iId);
            }
        }
        kViewers += "]";
        pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
        *pStream << kViewers.m_string.m_data;
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\hkVisualDebugger.cpp", 147);
        pStream->~hkErrStream();
    }
    else {
        alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
        char acBuffer[0x200];
        hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
        *pStream << "Capture file '" << captureFilename << "' could not be opened for writing";
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\hkVisualDebugger.cpp", 151);
        pStream->~hkErrStream();
    }
    hkError::s_instance->sectionEnd();
}

// Reads the client count once; each handler's list is inserted at the end through the heap.
void hkVisualDebugger::getCurrentProcesses(hkArray<hkProcess*>& process) const {
    const int32_t iNumClients = m_clients.m_size;
    for (int32_t i = 0; i < iNumClients; ++i) {
        hkServerProcessHandler* pHandler = m_clients.m_data[i].m_processHandler;
        if (pHandler) {
            hkArray<hkProcess*>& rList = pHandler->m_processList;
            const int32_t iCount = rList.m_size;
            hkProcess* const* ppList = rList.m_data;
            const int32_t iIndex = process.m_size;
            process._insertAt(*hkMemoryRouter::getInstance().m_heap, iIndex, ppList, iCount);
        }
    }
}

// indexOf, then removeAt on both arrays (the last element moves into the hole). Keeps indexOf's `index >= 0` test,
// which the search cannot fail.
int32_t hkVisualDebugger::removeTrackedObjectCallback(hkVisualDebuggerTrackedObjectCallback callback) {
    const int32_t iSize = m_trackCallbacks.m_size;
    int32_t iIndex = 0;
    if (iSize <= 0) {
        return iIndex;
    }
    for (hkVisualDebuggerTrackedObjectCallback* pCallback = m_trackCallbacks.m_data; *pCallback != callback;
         ++pCallback) {
        if (++iIndex >= iSize) {
            return iIndex;
        }
    }
    if (iIndex >= 0) {
        --m_trackCallbacks.m_size;
        const int32_t iLast = m_trackCallbacks.m_size;
        if (iLast != iIndex) {
            m_trackCallbacks.m_data[iIndex] = m_trackCallbacks.m_data[iLast];
        }
        --m_trackCallbackHandles.m_size;
        const int32_t iLastHandle = m_trackCallbackHandles.m_size;
        if (iLastHandle != iIndex) {
            m_trackCallbackHandles.m_data[iIndex] = m_trackCallbackHandles.m_data[iLastHandle];
        }
    }
    return iIndex;
}

// Untracks obj: removeAt on the tracked objects (the last record moves into the hole), then every callback is told,
// with no class and no tag.
void hkVisualDebugger::removeTrackedObject(void* obj) {
    const int32_t iSize = m_trackedObjects.m_size;
    int32_t iIndex = 0;
    if (iSize <= 0) {
        return;
    }
    for (hkVisualDebuggerTrackedObject* pObject = m_trackedObjects.m_data; pObject->m_ptr != obj; ++pObject) {
        if (++iIndex >= iSize) {
            return;
        }
    }
    const int32_t iLast = --m_trackedObjects.m_size;
    if (iLast != iIndex) {
        m_trackedObjects.m_data[iIndex] = m_trackedObjects.m_data[iLast];
    }
    for (int32_t j = 0; j < m_trackCallbacks.m_size; ++j) {
        const int32_t iAdded = (4 * j) & ~0xFF;
        reinterpret_cast<void (*)(void*, const hkClass*, int32_t, hkUlong, void*)>(m_trackCallbacks.m_data[j])(
            obj,
            nullptr,
            iAdded,
            0,
            m_trackCallbackHandles.m_data[j]
        );
    }
}

// Reads the client count once and the client array on every pass (deleteClient moves the last client down).
void hkVisualDebugger::deleteDeadClients() {
    for (int32_t i = m_clients.m_size - 1; i >= 0; --i) {
        if (!m_clients.m_data[i].m_socket) {
            deleteClient(i);
        }
    }
}

// Releases the client's handler, then its socket; removeAt moves the last client into the slot.
void hkVisualDebugger::deleteClient(int32_t i) {
    hkVisualDebuggerClient* pClient = &m_clients.m_data[i];
    if (pClient->m_processHandler) {
        pClient->m_processHandler->removeReference();
    }
    if (pClient->m_socket) {
        pClient->m_socket->removeReference();
    }
    const int32_t iLast = --m_clients.m_size;
    if (iLast != i) {
        m_clients.m_data[i] = m_clients.m_data[iLast];
    }
}

void hkVisualDebugger::createClient(hkSocket* socket, hkStreamReader* reader, hkStreamWriter* writer) {
    if (writer) {
        hkVersionReporter::sendVersionInformation(writer);
    }
    hkVisualDebuggerClient* v10 = m_clients.expandBy(1);
    v10->m_socket = socket;
    hkServerProcessHandler* v13 = new hkServerProcessHandler(m_contexts, reader, writer);
    v10->m_processHandler = v13;
    v13->registerAllAvailableProcesss();
    for (const hkStringPtr& kProcessName : m_defaultProcesses) {
        int32_t v15 = v10->m_processHandler->getProcessId(
            reinterpret_cast<const char*>(
                reinterpret_cast<uintptr_t>(kProcessName.m_stringAndFlag) & ~static_cast<uintptr_t>(1)
            )
        );
        if (v15 >= 0) {
            v10->m_processHandler->selectProcess(v15);
        }
    }
    writeStep(m_clients.m_size - 1, 0.0f);
}

void hkVisualDebugger::writeStep(int32_t i, float t) {
    hkVisualDebuggerClient* v3 = &m_clients.m_data[i];
    if (v3->m_processHandler->isOk()) {
        hkOArchive* m_outStream = v3->m_processHandler->m_outStream;
        m_outStream->write32u(5u);
        m_outStream->write8u(0);
        m_outStream->writeFloat32(t);
        hkStreamWriter* StreamWriter = m_outStream->getStreamWriter();
        StreamWriter->flush();
    }
}

void hkVisualDebugger::SupressPollForNewClient(bool val) {
    s_SuppressPollForNewClients = val;
}

void hkVisualDebugger::shutdown() {
    char buf[512];
    hkError::s_instance->sectionBegin(0x1293ADEF, "Shuting down Visual Debugger..");
    for (int32_t i = m_clients.m_size - 1; i >= 0; --i) {
        writeStep(i, 0.0f);
        deleteClient(i);
        hkErrStream ostr(buf, 512);
        ostr << "Client deleted.";
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\hkVisualDebugger.cpp", 81);
    }
    hkSocket* m_server = this->m_server;
    if (m_server) {
        delete m_server;
        this->m_server = nullptr;
        hkErrStream ostr(buf, 512);
        ostr << "Server deleted.";
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\hkVisualDebugger.cpp", 88);
    }
    hkError::s_instance->sectionEnd();
}

void hkVisualDebugger::addDefaultProcess(const char* viewerName) {
    m_defaultProcesses.expandOne() = viewerName;
}

// Tracks obj: appends its record, then tells every callback.
void hkVisualDebugger::addTrackedObject(void* obj, const hkClass& klass, const char* group, hkUlong tagId) {
    hkVisualDebuggerTrackedObject* pObject = &m_trackedObjects.expandOne();
    pObject->m_ptr = obj;
    pObject->m_class = &klass;
    pObject->m_tag = tagId;
    for (int32_t i = 0; i < m_trackCallbacks.m_size; ++i) {
        const int32_t iAdded = ((4 * i) & ~0xFF) | 1;
        reinterpret_cast<void (*)(void*, const hkClass*, int32_t, hkUlong, void*)>(m_trackCallbacks.m_data[i])(
            obj,
            &klass,
            iAdded,
            tagId,
            m_trackCallbackHandles.m_data[i]
        );
    }
}

void hkVisualDebugger::removeDefaultProcess(const char* processName) {
    for (int32_t v3 = 0; v3 < m_defaultProcesses.m_size; ++v3) {
        if (!hkString::strCmp(
                reinterpret_cast<const char*>(
                    reinterpret_cast<uintptr_t>(m_defaultProcesses.m_data[v3].m_stringAndFlag) &
                    ~static_cast<uintptr_t>(1)
                ),
                processName
            )) {
            m_defaultProcesses.m_data[v3].~hkStringPtr();
            int32_t v6 = --m_defaultProcesses.m_size;
            if (v6 != v3) {
                m_defaultProcesses.m_data[v3].m_stringAndFlag = m_defaultProcesses.m_data[v6].m_stringAndFlag;
            }
            return;
        }
    }
    char buf[512];
    hkError::s_instance->sectionBegin(0x76565454, "removeDefaultProcess");
    {
        hkErrStream ostr(buf, 512);
        ostr << "The default Process'" << processName
             << "', cannot not be removed from the default process list as it cannot be found!";
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\hkVisualDebugger.cpp", 318);
    }
    hkError::s_instance->sectionEnd();
}

void hkVisualDebugger::addTrackedObjectCallback(hkVisualDebuggerTrackedObjectCallback callback, void* userHandle) {
    m_trackCallbacks.pushBack(callback);
    m_trackCallbackHandles.pushBack(userHandle);
}
