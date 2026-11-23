#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"

class hkDataArrayImpl;
class hkDataClassImpl;
class hkDataObjectImpl;

// A world of serialized classes and objects: native (live objects through their reflection records) or a
// dictionary (built at load time).
class hkDataWorld : public hkReferencedObject {
public:
    enum DataWorldType {
        TYPE_NATIVE = 0,
        TYPE_DICTIONARY = 1,
    };

    ~hkDataWorld() override;

    virtual hkDataClassImpl* newClass(const hkDataClass::Cinfo& cinfo) = 0;
    virtual hkDataObjectImpl* newObject(const hkDataClass& klass) const = 0;
    virtual hkDataArrayImpl* newArray(hkDataObject& obj, const hkDataClass::MemberInfo& minfo) const = 0;
    virtual hkEnum<DataWorldType, int32_t> getType() const = 0;
    virtual hkDataObject getContents() const = 0;
    virtual void findAllClasses(hkArray<hkDataClassImpl*>& classesOut) const = 0;
    virtual hkDataClassImpl* findClass(const char* name) const = 0;
    virtual hkDataObject findObject(const hkDataObject::Handle& handle) const = 0;
};
