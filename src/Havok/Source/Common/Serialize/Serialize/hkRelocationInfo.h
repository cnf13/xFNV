#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"

// The SDK's header forward-declares it the same way.
template <typename V, typename Allocator>
class hkStorageStringMap;

// The pointer fixups of an object a writer or a copier has laid out: offsets within the object to patch with another
// offset in it (local), an absolute address (global), a class to finish or an import to resolve.
class hkRelocationInfo {
public:
    struct Local {
        // The SDK's, as for the other three records: their arrays treat them as plain data (hkTypeTraits.h).
        HK_DECLARE_POD_TYPE();

        int32_t m_fromOffset;
        int32_t m_toOffset;
    };

    struct Global {
        HK_DECLARE_POD_TYPE();

        int32_t m_fromOffset;
        void* m_toAddress;
        const hkClass* m_toClass;
        // hkBool32. Tested as a whole word in hkPackfileWriter::addObject.
        int32_t m_related;
    };

    struct Finish {
        HK_DECLARE_POD_TYPE();

        int32_t m_fromOffset;
        const char* m_className;
    };

    struct Import {
        HK_DECLARE_POD_TYPE();

        int32_t m_fromOffset;
        const char* m_identifier;
    };

    hkRelocationInfo();
    ~hkRelocationInfo();

    // Records that fromOffset refers to the import named name; the name is interned in m_pool, which is created on
    // first use.
    void addImport(int32_t off, const char* name);
    void applyLocalAndGlobal(void* buffer);
    void addLocal(int32_t fromOffset, int32_t toOffset);
    void addGlobal(int32_t fromOffset, void* toAddr, const hkClass* klass, int32_t related);
    void addFinish(int32_t fromOffset, const char* klassName) {
        m_finish.pushBack(Finish{fromOffset, klassName});
    }

    hkArray<Local, hkContainerHeapAllocator> m_local;
    hkArray<Global, hkContainerHeapAllocator> m_global;
    hkArray<Finish, hkContainerHeapAllocator> m_finish;
    hkArray<Import, hkContainerHeapAllocator> m_imports;
    hkStorageStringMap<int32_t, hkContainerHeapAllocator>* m_pool;
};
