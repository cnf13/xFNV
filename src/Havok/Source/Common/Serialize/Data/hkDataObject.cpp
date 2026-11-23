#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Data/hkDataArray.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObjectImpl.h"
#include "Havok/Source/Common/Serialize/Data/hkDataWorld.h"
#include "SDK/MSVC/string.h"

#include <new>

int32_t hkDataObject::Value::asInt() const {
    return m_impl->asInt(m_name);
}

const float* hkDataObject::Value::asVec(int32_t n) const {
    return m_impl->asVec(m_name, n);
}

const char* hkDataObject::Value::asString() const {
    return m_impl->asString(m_name);
}

float hkDataObject::Value::asReal() const {
    return m_impl->asReal(m_name);
}

// Stores an int through m_impl's int64 assign, with the value sign-extended.
void hkDataObject::Value::setInt(int32_t i) {
    m_impl->assign(m_name, static_cast<int64_t>(i));
}

void hkDataObject::Value::setObject(const hkDataObject& o) {
    m_impl->assign(m_name, o.m_impl);
}

void hkDataObject::Value::setInt64(int32_t low, int32_t high) {
    m_impl->assign(
        m_name,
        static_cast<int64_t>((static_cast<uint64_t>(static_cast<uint32_t>(high)) << 32) | static_cast<uint32_t>(low))
    );
}

void hkDataObject::Value::setReal(float r) {
    m_impl->assign(m_name, r);
}

void hkDataObject::Value::operator=(hkHalf h) {
    m_impl->assign(m_name, h);
}

void hkDataObject::Value::operator=(bool b) {
    // Uses the assign overload taking an int64; the byte is zero-extended.
    m_impl->assign(m_name, static_cast<int64_t>(b));
}

void hkDataObject::Value::operator=(const hkDataObject::Value& l) {
    m_impl->assign(m_name, l);
}

void hkDataObject::Value::setVec(const float* r, int32_t n) {
    m_impl->assign(m_name, r, n);
}

void hkDataObject::Value::setString(const char* s) {
    m_impl->assign(m_name, s);
}

void hkDataObject::Value::setArray(const hkDataArray& a) {
    m_impl->assign(m_name, a.m_impl);
}

hkDataObject::Type hkDataObject::Value::getType() const {
    hkDataClass::MemberInfo v8;
    hkDataObjectImpl* m_impl = this->m_impl;
    if (!m_impl) {
        return hkDataObject::TYPE_VOID;
    }
    const hkDataClassImpl* v3 = m_impl->getClass();
    const char* m_name = this->m_name;
    std::memset(&v8, 0, sizeof(v8));
    int32_t v6 = v3->getMemberIndexByName(m_name);
    v3->getMemberInfo(v6, v8);
    return v8.m_type;
}

hkDataObject hkDataObject::Value::asObject() const {
    return hkDataObject(m_impl->asObject(m_name));
}

hkDataArray hkDataObject::Value::asArray() const {
    return hkDataArray(m_impl->asArray(m_name));
}

hkDataObject::hkDataObject() : m_impl(nullptr) {}

hkDataObject::hkDataObject(hkDataObjectImpl* impl) : m_impl(impl) {
    if (impl) {
        ++impl->m_externalCount;
        ++impl->m_count;
    }
}

hkDataObject::hkDataObject(const hkDataObject& o) : m_impl(o.m_impl) {
    if (m_impl) {
        ++m_impl->m_externalCount;
        ++m_impl->m_count;
    }
}

hkDataObject::~hkDataObject() {
    if (m_impl) {
        --m_impl->m_externalCount;
        if (m_impl->m_count-- == 1) {
            delete m_impl;
        }
    }
}

void hkDataObject::operator=(const hkDataObject& o) {
    hkDataObjectImpl* m_impl = o.m_impl;
    if (o.m_impl) {
        ++m_impl->m_externalCount;
        ++m_impl->m_count;
    }
    hkDataObjectImpl* v4 = this->m_impl;
    if (this->m_impl) {
        --v4->m_externalCount;
        if (v4->m_count-- == 1) {
            delete v4;
        }
        this->m_impl = o.m_impl;
    }
    else {
        this->m_impl = o.m_impl;
    }
}

int32_t hkDataObject::getMemberIterator() const {
    return m_impl->getMemberIterator();
}

int32_t hkDataObject::isValid(int32_t it) const {
    return m_impl->isValid(it);
}

int32_t hkDataObject::getNextMember(int32_t it) const {
    return m_impl->getNextMember(it);
}

int32_t hkDataObject::hasMember(const char* name) const {
    return m_impl->hasMember(name);
}

hkDataObject::Handle hkDataObject::getHandle() const {
    if (m_impl) {
        return m_impl->getHandle();
    }
    Handle h = {nullptr, nullptr};
    return h;
}

// The SDK's hkDataClass constructor takes a non-const impl, the impl's getClass returns a const one: Havok casts the
// constness away here.
hkDataClass hkDataObject::getClass() const {
    return hkDataClass(const_cast<hkDataClassImpl*>(m_impl->getClass()));
}

hkDataObject::Value hkDataObject::access(const char* name) {
    return m_impl->accessByName(name);
}

hkDataObject::Value hkDataObject::access(const char* name) const {
    return m_impl->accessByName(name);
}

const char* hkDataObject::getMemberName(int32_t it) const {
    return m_impl->getMemberName(it);
}

hkDataObject::Value hkDataObject::getMemberValue(int32_t i) const {
    return m_impl->getMemberValue(i);
}

int32_t hkDataObject::isNull() const {
    return m_impl == nullptr;
}

int32_t hkDataObject::operator==(const hkDataObject& o) {
    Handle kThis = getHandle();
    Handle kOther = o.getHandle();
    return kThis.p0 == kOther.p0 && kThis.p1 == kOther.p1;
}

bool hkDataObject::operator!=(const hkDataObject& o) {
    return !(*this == o);
}

hkDataClass::hkDataClass(hkDataClassImpl* impl) : m_impl(impl) {}

const char* hkDataClass::getName() const {
    return m_impl->getName();
}

// The class's name, version, parent's name and declared members, each member's info read into a zeroed MemberInfo.
void hkDataClass::getCinfo(hkDataClass::Cinfo& cinfo) const {
    hkDataClass::MemberInfo kInfo;
    cinfo.name = m_impl->getName();
    cinfo.version = m_impl->getVersion();
    const char* pParentName;
    if (m_impl->getParent()) {
        pParentName = m_impl->getParent()->getName();
    }
    else {
        pParentName = nullptr;
    }
    cinfo.parent = pParentName;
    int32_t iNumMembers = m_impl->getNumDeclaredMembers();
    hkArray<hkDataClass::Cinfo::Member>* pMembers = &cinfo.members;
    pMembers->setSize(iNumMembers);
    for (int32_t i = 0; i < iNumMembers; ++i) {
        std::memset(&kInfo, 0, sizeof(kInfo));
        m_impl->getDeclaredMemberInfo(i, kInfo);
        hkDataClass::Cinfo::Member& rMember = pMembers->m_data[i];
        const char* pClassName = kInfo.m_class ? kInfo.m_class->getName() : nullptr;
        rMember.type = kInfo.m_type;
        rMember.name = kInfo.m_name;
        rMember.className = pClassName;
        rMember.tupleCount = kInfo.m_tupleCount;
    }
}

hkDataClass hkDataClass::getParent() const {
    return hkDataClass(m_impl->getParent());
}

int32_t hkDataClass::isNull() const {
    return m_impl == nullptr;
}

hkBool hkDataClass::isSuperClass(const hkDataClass& k) {
    return m_impl->isSuperClass(k.m_impl);
}

int32_t hkDataClass::getDeclaredMemberIndexByName(const char* name) {
    return m_impl->getDeclaredMemberIndexByName(name);
}

void hkDataClass::getDeclaredMemberInfo(int32_t i, hkDataClass::MemberInfo& info) {
    m_impl->getDeclaredMemberInfo(i, info);
}

void hkDataClass::getMemberInfo(int32_t i, hkDataClass::MemberInfo& info) {
    m_impl->getMemberInfo(i, info);
}

int32_t hkDataClass::getNumMembers() const {
    return m_impl->getNumMembers();
}

int32_t hkDataClass::getMemberIndexByName(const char* name) {
    return m_impl->getMemberIndexByName(name);
}

void hkDataClass::getAllMemberInfo(hkArrayBase<hkDataClass::MemberInfo>& minfos) const {
    const int32_t numMembers = getNumMembers();
    for (int32_t i = 0; i < numMembers; ++i) {
        m_impl->getMemberInfo(i, minfos.m_data[i]);
    }
}

const hkDataWorld* hkDataClass::getWorld() {
    return m_impl->getWorld();
}

int32_t hkDataClass::getVersion() const {
    return m_impl->getVersion();
}

int32_t hkDataClass::getNumDeclaredMembers() const {
    return m_impl->getNumDeclaredMembers();
}

void hkDataClass::getAllDeclaredMemberInfo(hkArrayBase<hkDataClass::MemberInfo>& minfos) const {
    int32_t NumDeclaredMembers = getNumDeclaredMembers();
    int32_t v4 = 0;
    if (NumDeclaredMembers > 0) {
        int32_t v5 = 0;
        do {
            m_impl->getDeclaredMemberInfo(v4++, minfos.m_data[v5++]);
        }
        while (v4 < NumDeclaredMembers);
    }
}

hkDataArray::~hkDataArray() {
    if (m_impl) {
        --m_impl->m_externalCount;
        if (m_impl->m_count-- == 1) {
            delete m_impl;
        }
    }
}

// Value operator[](int i): the array and the index. Non-const: callers such as Cloner::copyArray assign through the
// element it returns.
hkDataArray::Value hkDataArray::access(int32_t i) {
    return {m_impl, i};
}

hkDataArray::Value hkDataArray::access(int32_t i) const {
    return {m_impl, i};
}

int32_t hkDataArray::getSize() const {
    return m_impl->getSize();
}
