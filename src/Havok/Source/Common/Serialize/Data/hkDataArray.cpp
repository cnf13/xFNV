#include "Havok/Source/Common/Serialize/Data/hkDataArray.h"

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObjectImpl.h"

hkDataObject::Type hkDataArray::Value::getType() const {
    return m_impl->getType();
}

int32_t hkDataArray::Value::asInt() const {
    return m_impl->asInt(m_index);
}

float hkDataArray::Value::asReal() const {
    return m_impl->asReal(m_index);
}

const float* hkDataArray::Value::asVec(int32_t nreal) const {
    return m_impl->asVec(m_index);
}

void hkDataArray::Value::setVec(const float* ra, int32_t nreal) {
    m_impl->setVec(m_index, ra);
}

hkDataObject* hkDataArray::Value::asObject(hkDataObject* apResult) const {
    hkDataObjectImpl* v2 = m_impl->asObject(m_index);
    return ::new (apResult) hkDataObject(v2);
}

hkDataArray* hkDataArray::Value::asArray(hkDataArray* apResult) const {
    hkDataArrayImpl* v2 = m_impl->asArray(m_index);
    return ::new (apResult) hkDataArray(v2);
}

const char* hkDataArray::Value::asString() const {
    return m_impl->asString(m_index);
}

void hkDataArray::Value::operator=(int32_t aiValue) {
    m_impl->setInt(m_index, aiValue);
}

void hkDataArray::Value::operator=(float afValue) {
    m_impl->setReal(m_index, afValue);
}

void hkDataArray::Value::operator=(const hkDataObject& arValue) {
    m_impl->setObject(m_index, arValue.m_impl);
}

void hkDataArray::Value::operator=(const char* apValue) {
    m_impl->setString(m_index, apValue);
}

void hkDataArray::Value::setHalf(hkHalf h) {
    m_impl->setHalf(m_index, h);
}

void hkDataArray::Value::setInt64(int64_t i) {
    m_impl->setInt64(m_index, i);
}

hkDataArray::hkDataArray(hkDataArrayImpl* a1) : m_impl(a1) {
    if (a1) {
        ++a1->m_externalCount;
        ++a1->m_count;
    }
}

void hkDataArray::clear() {
    m_impl->clear();
}

void hkDataArray::reserve(int32_t n) {
    m_impl->reserve(n);
}

void hkDataArray::setAll(const hkBool* a, int32_t n) {
    m_impl->setAll(a, n);
}

void hkDataArray::setAll(const char* a, int32_t n) {
    m_impl->setAll(a, n);
}

void hkDataArray::setAll(const int8_t* a, int32_t n) {
    m_impl->setAll(a, n);
}

void hkDataArray::setAll(const uint8_t* a, int32_t n) {
    m_impl->setAll(a, n);
}

void hkDataArray::setAll(const int16_t* a, int32_t n) {
    m_impl->setAll(a, n);
}

void hkDataArray::setAll(const uint16_t* a, int32_t n) {
    m_impl->setAll(a, n);
}

void hkDataArray::setAll(const int32_t* a, int32_t n) {
    m_impl->setAll(a, n);
}

void hkDataArray::setAll(const uint32_t* a, int32_t n) {
    m_impl->setAll(a, n);
}

void hkDataArray::setAll(const int64_t* a, int32_t n) {
    m_impl->setAll(a, n);
}

void hkDataArray::setAll(const uint64_t* a, int32_t n) {
    m_impl->setAll(a, n);
}

void hkDataArray::setAll(const float* a, int32_t n) {
    m_impl->setAll(a, n);
}

void hkDataArray::setAll(const hkHalf* a, int32_t n) {
    m_impl->setAll(a, n);
}

void hkDataArray::setSize(int32_t n) {
    m_impl->setSize(n);
}

hkDataObject::Type hkDataArray::getType() const {
    return m_impl->getType();
}

// The SDK's hkDataClass constructor takes a non-const impl, the impl's getClass returns a const one: Havok casts the
// constness away here.
hkDataClass* hkDataArray::getClass(hkDataClass* apResult) const {
    const hkDataClassImpl* pClass = m_impl->getClass();
    ::new (apResult) hkDataClass(const_cast<hkDataClassImpl*>(pClass));
    return apResult;
}

hkDataArray* hkDataArray::swizzleObjectMember(hkDataArray* apResult, const char* name) const {
    hkDataArrayImpl* v3 = m_impl->swizzleObjectMember(name);
    ::new (apResult) hkDataArray(v3);
    return apResult;
}
