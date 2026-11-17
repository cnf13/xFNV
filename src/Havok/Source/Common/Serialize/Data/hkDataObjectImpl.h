#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"

class hkDataArrayImpl;
class hkDataClassImpl;
class hkDataWorld;

// The reference count of the serialized data objects: the allocation size, the external and internal counts.
class hkDataRefCounted {
public:
    // Starts with no references; the constructor sets only the two counts and leaves m_memSize to the allocating
    // caller.
    hkDataRefCounted() : m_externalCount(0), m_count(0) {}
    virtual ~hkDataRefCounted();

    // The deleting destructor's free: the block of m_memSize bytes back to the thread's heap.
    static void operator delete(void* apMemory);

    uint16_t m_memSize;
    int16_t m_externalCount;
    int32_t m_count;
};

// A serialized class: its name, version, parent and members.
class hkDataClassImpl : public hkDataRefCounted {
public:
    typedef hkDataClass::MemberInfo MemberInfo;

    ~hkDataClassImpl() override;

    virtual hkDataWorld* getWorld() const = 0;
    virtual const char* getName() const = 0;
    virtual int32_t getVersion() const = 0;
    virtual hkDataClassImpl* getParent() const = 0;
    virtual hkBool isSuperClass(const hkDataClassImpl* k) const = 0;
    virtual int32_t getNumDeclaredMembers() const = 0;
    virtual void getDeclaredMemberInfo(int32_t i, MemberInfo& info) const = 0;
    virtual int32_t getDeclaredMemberIndexByName(const char* name) const = 0;
    virtual int32_t getNumMembers() const = 0;
    virtual void getMemberInfo(int32_t i, MemberInfo& info) const = 0;
    virtual int32_t getMemberIndexByName(const char* name) const = 0;
};

// A serialized data object: its members by name or by iterator, and the value accessors its member Values call.
class hkDataObjectImpl : public hkDataRefCounted {
public:
    ~hkDataObjectImpl() override;

    virtual hkDataObject::Handle getHandle() const = 0;
    virtual const hkDataClassImpl* getClass() const = 0;
    virtual hkDataObject::Value accessByName(const char* name) = 0;
    // Havok's hkBool32.
    virtual int32_t hasMember(const char* name) const = 0;
    virtual int32_t getMemberIterator() const = 0;
    // Havok's hkBool32.
    virtual int32_t isValid(int32_t it) const = 0;
    virtual int32_t getNextMember(int32_t it) const = 0;
    virtual const char* getMemberName(int32_t it) const = 0;
    virtual const hkDataObject::Value getMemberValue(int32_t it) const = 0;
    virtual hkDataArrayImpl* asArray(const char* handle) = 0;
    virtual const char* asString(const char* handle) = 0;
    virtual int64_t asInt(const char* handle) = 0;
    virtual hkDataObjectImpl* asObject(const char* handle) = 0;
    virtual const float* asVec(const char* handle, int32_t nreal) = 0;
    virtual float asReal(const char* handle) = 0;
    // The assign overloads, in source order; the MS ABI puts them in the vtable in reverse.
    virtual void assign(const char* handle, const hkDataObject::Value& value) = 0;
    virtual void assign(const char* handle, hkDataObjectImpl* value) = 0;
    virtual void assign(const char* handle, hkDataArrayImpl* value) = 0;
    virtual void assign(const char* handle, const char* value) = 0;
    virtual void assign(const char* handle, float r) = 0;
    virtual void assign(const char* handle, hkHalf r) = 0;
    virtual void assign(const char* handle, const float* r, int32_t nreal) = 0;
    virtual void assign(const char* handle, int64_t valueIn) = 0;
};

// An array of serialized data values. The accessors a derived array does not override are shared stubs (return 0, 0.0
// or nothing).
class hkDataArrayImpl : public hkDataRefCounted {
public:
    ~hkDataArrayImpl() override;

    virtual hkDataObject::Type getType() const = 0;
    virtual void clear() = 0;
    virtual void reserve(int32_t n) = 0;
    virtual void setSize(int32_t n) = 0;
    virtual int32_t getSize() const = 0;
    virtual const hkDataClassImpl* getClass() const {
        return nullptr;
    }
    // The array of a struct array's member name.
    virtual hkDataArrayImpl* swizzleObjectMember(const char* name) const {
        return nullptr;
    }
    virtual const float* asVec(int32_t index) const {
        return nullptr;
    }
    virtual void setVec(int32_t index, const float* val) {}
    virtual const char* asString(int32_t index) const {
        return nullptr;
    }
    virtual void setString(int32_t index, const char* val) {}
    virtual float asReal(int32_t index) const {
        return 0.0f;
    }
    virtual void setReal(int32_t index, float val) {}
    virtual hkHalf asHalf(int32_t index) const;
    virtual void setHalf(int32_t index, hkHalf val) {}
    virtual int32_t asInt(int32_t index) const {
        return 0;
    }
    virtual void setInt(int32_t index, int32_t val) {}
    virtual int64_t asInt64(int32_t index) const;
    virtual void setInt64(int32_t index, int64_t val) {}
    virtual hkDataObjectImpl* asObject(int32_t index) const {
        return nullptr;
    }
    virtual void setObject(int32_t index, hkDataObjectImpl* val) {}
    virtual hkDataArrayImpl* asArray(int32_t index) const {
        return nullptr;
    }
    virtual void setArray(int32_t index, hkDataArrayImpl* val) {}
    virtual void setAll(const hkBool* v, int32_t n) {}
    virtual void setAll(const char* v, int32_t n) {}
    virtual void setAll(const int8_t* v, int32_t n) {}
    virtual void setAll(const uint8_t* v, int32_t n) {}
    virtual void setAll(const int16_t* v, int32_t n) {}
    virtual void setAll(const uint16_t* v, int32_t n) {}
    virtual void setAll(const int32_t* v, int32_t n) {}
    virtual void setAll(const uint32_t* v, int32_t n) {}
    virtual void setAll(const int64_t* v, int32_t n) {}
    virtual void setAll(const uint64_t* v, int32_t n) {}
    virtual void setAll(const float* v, int32_t n) {}
    virtual void setAll(const hkHalf* v, int32_t n) {}

    // Element index set from the value of another object's member.
    void set(int32_t index, hkDataObject::Value val);
};
