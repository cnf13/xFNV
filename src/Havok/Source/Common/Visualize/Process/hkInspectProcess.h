#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessFactory.h"

#include <stdint.h>

class hkProcessContext;
class hkVisualDebugger;

// Havok SDK signature (hkVisualize.lib, tied by bytes,reloc).
void myTrackedObjectCallback(void* a1, const hkClass* a2, hkBool a3, hkUlong a4, void* a5);
// The address of the member a member path names inside an object: it follows path from index + 1 through matrices,
// arrays and structs and returns the address of the plain value it ends at, or null where the path cannot be followed.
void* getMemberAddressFromPath(
    hkClassMember::Type type,
    void* address,
    const hkArray<uint16_t>& path,
    int32_t index,
    const hkClassMember* member
);

// The process that lets the visual debugger inspect and edit objects. Only some of its members are declared.
class hkInspectProcess : public hkReferencedObject, public hkProcess {
public:
    // The commands it sends and consumes (the SDK's values).
    enum Command : int32_t {
        HK_SETUP = 0x20,
        HK_ADD_TOPLEVEL = 0x21,
        HK_REMOVE_TOPLEVEL = 0x22,
        HK_REQUEST_OBJECT = 0x23,
        HK_ADD_OBJECT = 0x24,
        HK_UPDATE_MEMBER = 0x25,
    };

    // An object the inspection has written, with its class.
    struct ObjectPair {
        const void* obj;
        const hkClass* klass;
    };

    hkInspectProcess(const hkArray<hkProcessContext*>& contexts);
    ~hkInspectProcess() override;

    int32_t getProcessTag() override;
    void getConsumableCommands(uint8_t*& commands, int32_t& numCommands) override;
    void consumeCommand(uint8_t command) override;
    void init() override;
    void step(float frameTimeInMs) override;

    // The tag is written with write64u.
    int32_t addTopLevelObject(void* ptr, const hkClass& klass, hkUlong tag);
    // Havok SDK signature (hkVisualize.lib, tied by bytes,reloc).
    int32_t removeTopLevelObject(void* ptr);
    // Writes the object ptr of class klass to the output stream, as its most derived class
    // from the debugger's class registry when klass has a vtable, and, when recurse is set, the objects it points to
    // that recursionStack does not hold yet; returns -1 on failure.
    int32_t writeObject(const void* ptr, const hkClass& klass, hkBool recurse, hkArray<ObjectPair>& recursionStack);

    // DeletingDestruct's free (the heap's blockFree of m_memSizeAndFlags, hkReferencedObject's); declared here
    // because hkProcess's makes the inherited one ambiguous.
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }
    // Registers the process with the process factory and keeps its tag.
    static void registerProcess();
    // Havok SDK signature (hkVisualize.lib, tied by bytes,reloc).
    static hkProcess* create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);

    // The objects step writes again each frame (the SDK's members).
    hkArray<ObjectPair> m_autoUpdateList;
    hkVisualDebugger* m_vdb;

    // The tag registerProcess stores.
    static inline int32_t m_tag;
};

// The process's name, which getName returns.
using hkInspectProcess_Name_t = const char[17];
inline hkInspectProcess_Name_t hkInspectProcess_Name{"ObjectInspection"};
