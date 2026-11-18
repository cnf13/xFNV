#pragma once

#include <stdint.h>

class hkBool;
class hkClass;
class hkClassMember;
class hkReferencedObject;
class hkStatisticsCollector;

// Returns object's "name" member when klass has one (a C string or an hkStringPtr, "unnamed" when null), else null.
const char* _findObjectName(void* object, const hkClass* klass);

// Static helpers for collecting Havok memory statistics; no instance state.
class hkStatisticsCollectorUtil {
public:
    // Reports the memory every owned member of obj (an instance of cls) refers to.
    static void addClassContents(const void* obj, const hkClass& cls, hkStatisticsCollector* collector);

protected:
    // Reports the object obj points to under fieldName.
    static void
    _addObject(const char* fieldName, const void* obj, const hkClass& cls, hkStatisticsCollector* collector);
    // Reports what the embedded object obj refers to, under fieldName.
    static void
    _addObjectContents(const char* fieldName, const void* obj, const hkClass& cls, hkStatisticsCollector* collector);
    // Reports an array's buffer: size used and capacity elements of objSize bytes.
    static void _addArrayData(
        const char* fieldName,
        void* data,
        int32_t size,
        int32_t capacity,
        int32_t objSize,
        hkStatisticsCollector* collector
    );
    // The size of one element of the array member mem, or -1 when it has none.
    static int32_t _getArrayElementSize(const hkClassMember& mem);
    // Reports what the size elements of the array member mem refer to.
    static void _addArrayContents(const hkClassMember& mem, void* data, int32_t size, hkStatisticsCollector* collector);
    // Reports what the size cls objects at data refer to.
    static void _addHomogeneousArrayContents(
        const char* fieldName,
        const hkClass& cls,
        void* data,
        int32_t size,
        hkStatisticsCollector* collector
    );
    // Reports the C string member mem's string.
    static void _addCstring(const hkClassMember& mem, const char* string, hkStatisticsCollector* collector);

public:
    static void defaultCalcStatistics(const void* obj, const hkClass& cls, hkStatisticsCollector* collector);
    static void defaultCalcStatistics(const hkReferencedObject* obj, hkStatisticsCollector* collector);
    static void addClassContentsAll(const hkReferencedObject* obj, hkStatisticsCollector* collector);
    // Reports the contents of obj as cls and as each of cls's parents.
    static void addClassContentsAll(const void* obj, const hkClass& cls, hkStatisticsCollector* collector);
    // Whether cls or a parent below hkReferencedObject declares members.
    static hkBool hasContents(const hkClass& cls);
    static hkBool isMemberSimple(const hkClassMember& mem);
    static hkBool _isClassSimple(const hkClass& clsIn);
    static hkBool isClassSimple(const hkClass& clsIn);
};
