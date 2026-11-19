#include "Havok/Source/Common/Base/System/StackTracer/hkStackTracer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "platform/System.h"

// buf is the storage create placement-constructs the tracer in, aligned as the tracer; _stackTracerWin32DataBuf is the
// hkStackTracerWin32Data the constructor points m_impl at.
alignas(hkStackTracer) static char buf[sizeof(hkStackTracer)]{};
static hkStackTracerImpl _stackTracerWin32DataBuf{};

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode
    hkSingletonRegisterhkStackTracer("hkStackTracer", nullptr, reinterpret_cast<void**>(&hkStackTracer::s_instance));

// The handle is tested again after the warning, as the game does. GetProcAddress (platform::LibrarySymbol) returns an
// untyped address: each entry point is cast to the field's function type.
void hkStackTracerImpl::init() {
    m_dbgHelpDll = platform::LibraryLoad(hkStackTracerDbghelpPath);
    if (m_dbgHelpDll == nullptr) {
        platform::DebuggerOutput("Havok StackTracer: hkStackTracer: Unable to load dbghelp.dll");
    }
    if (m_dbgHelpDll) {
        m_dllReferences = 1;
        m_symInitialize =
            reinterpret_cast<decltype(m_symInitialize)>(platform::LibrarySymbol(m_dbgHelpDll, "SymInitialize"));
        m_symRefreshModuleList = reinterpret_cast<decltype(m_symRefreshModuleList)>(
            platform::LibrarySymbol(m_dbgHelpDll, "SymRefreshModuleList")
        );
        if (m_symRefreshModuleList == nullptr) {
            platform::DebuggerOutput(
                "Havok StackTracer: Could not load symbol SymRefreshModuleList from dbghelp.dll, "
                "version too old, but will continue without it."
            );
        }
        m_symGetOptions =
            reinterpret_cast<decltype(m_symGetOptions)>(platform::LibrarySymbol(m_dbgHelpDll, "SymGetOptions"));
        m_symSetOptions =
            reinterpret_cast<decltype(m_symSetOptions)>(platform::LibrarySymbol(m_dbgHelpDll, "SymSetOptions"));
        m_getSymbol =
            reinterpret_cast<decltype(m_getSymbol)>(platform::LibrarySymbol(m_dbgHelpDll, "SymGetSymFromAddr"));
        m_stackWalk64 = reinterpret_cast<decltype(m_stackWalk64)>(platform::LibrarySymbol(m_dbgHelpDll, "StackWalk64"));
        m_symFunctionTableAccess64 = reinterpret_cast<decltype(m_symFunctionTableAccess64)>(
            platform::LibrarySymbol(m_dbgHelpDll, "SymFunctionTableAccess64")
        );
        m_symGetModuleBase64 = reinterpret_cast<decltype(m_symGetModuleBase64)>(
            platform::LibrarySymbol(m_dbgHelpDll, "SymGetModuleBase64")
        );
        m_getLine =
            reinterpret_cast<decltype(m_getLine)>(platform::LibrarySymbol(m_dbgHelpDll, "SymGetLineFromAddr64"));
        m_symCleanup = reinterpret_cast<decltype(m_symCleanup)>(platform::LibrarySymbol(m_dbgHelpDll, "SymCleanup"));
        uint32_t uiSymOptions = m_symGetOptions();
        // SYMOPT_LOAD_LINES (0x10) | SYMOPT_DEBUG (0x80000000), dbghelp.h.
        uiSymOptions |= 0x80000010u;
        m_symSetOptions(uiSymOptions);
        m_symInitialize(currentProcess(), nullptr, 1);
    }
}

int32_t hkStackTracer::CallTree::insertCallStack(const hkUlong* addrs, int32_t numAddrs) {
    if (!m_nodes.m_size) {
        hkMemoryAllocator* m_allocator = this->m_allocator;
        if ((m_nodes.m_capacityAndFlags & hkArrayBase<hkStackTracer::CallTree::Node>::CAPACITY_MASK) == 0) {
            hkArrayUtil::_reserve(*m_allocator, this, 1, sizeof(hkStackTracer::CallTree::Node));
        }
        int32_t m_size = m_nodes.m_size;
        int32_t v6 = 1 - m_size;
        hkStackTracer::CallTree::Node* v7 = &m_nodes.m_data[m_size];
        if (v6 > 0) {
            // Stepped by element: Node's m_value is pointer-sized, so its size differs on x64.
            do {
                if (v7) {
                    v7->m_value = 0;
                    v7->m_parent = -1;
                    v7->m_firstChild = -1;
                    v7->m_next = -1;
                }
                ++v7;
                --v6;
            }
            while (v6);
        }
        m_nodes.m_size = 1;
    }
    int32_t result = 0;
    bool v10 = numAddrs - 1 < 0;
    int32_t v11 = numAddrs - 1;
    int32_t v19 = 0;
    int32_t a2 = numAddrs - 1;
    if (!v10) {
        int32_t v16;
        do {
            uintptr_t v12 = addrs[v11];
            int32_t v13 = v19;
            int32_t m_firstChild = m_nodes.m_data[v19].m_firstChild;
            if (m_firstChild <= 0) {
            LABEL_16:;
                v16 = m_nodes.m_size;
                if (v16 == (m_nodes.m_capacityAndFlags & hkArrayBase<hkStackTracer::CallTree::Node>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(*this->m_allocator, this, sizeof(hkStackTracer::CallTree::Node));
                }
                hkStackTracer::CallTree::Node* v17 = &m_nodes.m_data[m_nodes.m_size];
                if (v17) {
                    v17->m_value = 0;
                    v17->m_parent = -1;
                    v17->m_firstChild = -1;
                    v17->m_next = -1;
                }
                hkStackTracer::CallTree::Node* v18 = &m_nodes.m_data[m_nodes.m_size++];
                v18->m_value = v12;
                v18->m_parent = v19;
                v18->m_next = m_nodes.m_data[v13].m_firstChild;
                m_nodes.m_data[v13].m_firstChild = v16;
            }
            else {
                while (true) {
                    hkStackTracer::CallTree::Node* v15 = &m_nodes.m_data[m_firstChild];
                    if (v15->m_value == v12) {
                        break;
                    }
                    m_firstChild = v15->m_next;
                    if (m_firstChild <= 0) {
                        goto LABEL_16;
                    }
                }
                v16 = m_firstChild;
            }
            v11 = a2 - 1;
            v10 = a2 - 1 < 0;
            v19 = v16;
            --a2;
        }
        while (!v10);
        return v16;
    }
    return result;
}

// Records the current call stack, without this function's own frame (the first address): up to 64 addresses in a
// 0x100-byte local buffer.
int32_t hkStackTracer::CallTree::insertCallStack(hkStackTracer& tracer) {
    hkUlong trace[64];
    int32_t numTrace = tracer.getStackTrace(trace, 64);
    if (numTrace <= 0) {
        return 0;
    }
    return insertCallStack(trace + 1, numTrace - 1);
}

int32_t hkStackTracer::CallTree::getCallStack(int32_t id, hkUlong* addrs, int32_t maxAddrs) const {
    int32_t result;
    for (result = 0; result < maxAddrs; ++result) {
        if (id <= 0) {
            break;
        }
        int32_t v6 = id;
        addrs[result] = m_nodes.m_data[v6].m_value;
        id = m_nodes.m_data[v6].m_parent;
    }
    return result;
}

hkStackTracer::hkStackTracer() : m_impl(&_stackTracerWin32DataBuf) {
    _stackTracerWin32DataBuf.init();
}

// Calls SymCleanup on the current process, clears every entry point and unloads dbghelp.
hkStackTracer::~hkStackTracer() {
    hkStackTracerImpl* pImpl = static_cast<hkStackTracerImpl*>(m_impl);
    if (pImpl->m_dbgHelpDll) {
        pImpl->m_symCleanup(hkStackTracerImpl::currentProcess());
        pImpl->m_symInitialize = nullptr;
        pImpl->m_symRefreshModuleList = nullptr;
        pImpl->m_symGetOptions = nullptr;
        pImpl->m_symSetOptions = nullptr;
        pImpl->m_getSymbol = nullptr;
        pImpl->m_stackWalk64 = nullptr;
        pImpl->m_symFunctionTableAccess64 = nullptr;
        pImpl->m_symGetModuleBase64 = nullptr;
        pImpl->m_getLine = nullptr;
        pImpl->m_symCleanup = nullptr;
        platform::LibraryFree(pImpl->m_dbgHelpDll);
        pImpl->m_dbgHelpDll = nullptr;
    }
}

// Refreshes dbghelp's module list for the current process, when the entry point was loaded.
void hkStackTracer::refreshSymbols() {
    hkStackTracerImpl* pImpl = static_cast<hkStackTracerImpl*>(m_impl);
    if (pImpl->m_symRefreshModuleList) {
        pImpl->m_symRefreshModuleList(hkStackTracerImpl::currentProcess());
    }
}

hkStackTracer* hkStackTracer::create() {
    return ::new (buf) hkStackTracer;
}
