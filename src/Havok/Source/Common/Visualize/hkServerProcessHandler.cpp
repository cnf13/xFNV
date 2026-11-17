#include "Havok/Source/Common/Visualize/hkServerProcessHandler.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Io/IArchive/hkIArchive.h"
#include "Havok/Source/Common/Base/System/Io/OArchive/hkOArchive.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Serialize/hkDisplaySerializeIStream.h"
#include "Havok/Source/Common/Visualize/Serialize/hkDisplaySerializeOStream.h"
#include "Havok/Source/Common/Visualize/hkCommandRouter.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Common/Visualize/hkProcessFactory.h"
#include "Havok/Source/Common/Visualize/hkProcessHandler.h"
#include "Havok/Source/Common/Visualize/hkServerDebugDisplayHandler.h"

// Not selectable; registered with its own command router for the handler commands; the contexts copied; a stream
// over each of inStream and outStream given, and a display handler writing to the out stream.
hkServerProcessHandler::hkServerProcessHandler(
    hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts,
    hkStreamReader* inStream,
    hkStreamWriter* outStream
)
  : hkProcess(false) {
    m_commandRouter.registerProcess(static_cast<hkProcess*>(this));
    m_contexts.append(contexts.begin(), contexts.m_size);
    m_inStream = inStream ? new hkDisplaySerializeIStream(inStream) : nullptr;
    m_outStream = outStream ? new hkDisplaySerializeOStream(outStream) : nullptr;
    m_processHandler = this;
    m_displayHandler = new hkServerDebugDisplayHandler(m_outStream);
}

// The handler owns its processes, its two streams and its display handler; the arrays and the command router are
// destroyed by their own destructors.
hkServerProcessHandler::~hkServerProcessHandler() {
    for (int32_t i = 0; i < m_processList.m_size; ++i) {
        delete m_processList.m_data[i];
    }
    delete m_inStream;
    delete m_outStream;
    delete m_displayHandler;
}

void hkServerProcessHandler::getConsumableCommands(uint8_t*& commands, int32_t& numCommands) {
    commands = aAa;
    numCommands = 2;
}

void hkServerProcessHandler::consumeCommand(uint8_t command) {
    // The SDK's read32: one 4-byte element.
    int32_t id;
    hkBool bOk;
    if (command == hkProcessHandler::HK_CREATE_PROCESS) {
        m_inStream->readArrayGeneric(&id, sizeof(id), 1);
        if (m_inStream->isOk(&bOk)->m_bool) {
            static_cast<hkProcessHandler*>(this)->createProcess(id);
        }
    }
    else if (command == hkProcessHandler::HK_DELETE_PROCESS) {
        m_inStream->readArrayGeneric(&id, sizeof(id), 1);
        if (m_inStream->isOk(&bOk)->m_bool) {
            static_cast<hkProcessHandler*>(this)->deleteProcess(id);
        }
    }
}

hkResult hkServerProcessHandler::registerProcess(const char* name, int32_t id) {
    int32_t v4 = hkString::strLen(name);
    if (v4 > 0xFFFF) {
        v4 = 0xFFFF;
    }
    m_outStream->write32(v4 + 7);
    m_outStream->write8u(192);
    m_outStream->write32(id);
    m_outStream->write16u(v4);
    m_outStream->writeRaw(name, v4);
    return HK_SUCCESS;
}

hkResult hkServerProcessHandler::selectProcess(int32_t id) {
    m_outStream->write32(5);
    m_outStream->write8u(193);
    m_outStream->write32(id);
    return HK_SUCCESS;
}

hkResult hkServerProcessHandler::createProcess(int32_t id) {
    if (findProcessByTag(id) >= 0) {
        return HK_SUCCESS;
    }
    hkProcess* pProcess = hkProcessFactory::s_instance->createProcessByTag(id, m_contexts);
    if (!pProcess) {
        return HK_FAILURE;
    }
    pProcess->m_inStream = m_inStream;
    pProcess->m_outStream = m_outStream;
    pProcess->m_processHandler = static_cast<hkProcessHandler*>(this);
    pProcess->m_displayHandler = m_displayHandler;
    pProcess->init();
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (m_processList.m_size == (m_processList.m_capacityAndFlags & hkArrayBase<hkProcess*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, &m_processList, sizeof(hkProcess*));
    }
    m_processList.m_data[m_processList.m_size++] = pProcess;
    m_commandRouter.registerProcess(pProcess);
    return HK_SUCCESS;
}

hkResult hkServerProcessHandler::deleteProcess(int32_t id) {
    int32_t ProcessByTag = findProcessByTag(id);
    if (ProcessByTag < 0) {
        return HK_SUCCESS;
    }
    hkProcess* v4 = m_processList.m_data[ProcessByTag];
    int32_t v5 = --m_processList.m_size;
    if (v5 != ProcessByTag) {
        m_processList.m_data[ProcessByTag] = m_processList.m_data[v5];
    }
    m_commandRouter.unregisterProcess(v4);
    if (v4) {
        delete v4;
    }
    return HK_SUCCESS;
}

int32_t hkServerProcessHandler::getProcessId(const char* name) {
    return hkProcessFactory::s_instance->getProcessId(name);
}

const char* hkServerProcessHandler::getProcessName(int32_t tag) {
    return hkProcessFactory::s_instance->getProcessName(tag);
}

void hkServerProcessHandler::step(float frameTimeInMs) {
    for (int32_t i = m_processList.m_size - 1; i >= 0; --i) {
        hkProcess* v4 = m_processList.m_data[i];
        v4->step(frameTimeInMs);
    }
    if (m_inStream) {
        hkBool bConsumed;
        m_commandRouter.consumeCommands(&bConsumed, m_inStream);
    }
}

int32_t hkServerProcessHandler::findProcessByTag(int32_t tag) {
    int32_t m_size = m_processList.m_size;
    int32_t v4 = 0;
    if (m_size <= 0) {
        return -1;
    }
    while (true) {
        hkProcess* v5 = m_processList.m_data[v4];
        if (v5->getProcessTag() == tag) {
            break;
        }
        if (++v4 >= m_size) {
            return -1;
        }
    }
    return v4;
}

hkResult hkServerProcessHandler::registerAllAvailableProcesss() {
    hkProcessFactory* pFactory = hkProcessFactory::s_instance;
    for (int32_t i = 0; i < pFactory->m_name2creationFunction.m_size; ++i) {
        static_cast<hkProcessHandler*>(this)->registerProcess(
            pFactory->m_name2creationFunction.m_data[i].m_name.cString(), i
        );
    }
    return HK_SUCCESS;
}

// m_commandMap frees its storage in its own destructor.
hkCommandRouter::~hkCommandRouter() {}

void hkProcess::getConsumableCommands(uint8_t*& commands, int32_t& numCommands) {
    commands = nullptr;
    numCommands = 0;
}

bool hkProcess::isOk() {
    // isOk writes bOk before it is read.
    hkBool bOk;
    hkDisplaySerializeOStream* pOutStream = m_outStream;
    return pOutStream && pOutStream->isOk(&bOk)->m_bool;
}
