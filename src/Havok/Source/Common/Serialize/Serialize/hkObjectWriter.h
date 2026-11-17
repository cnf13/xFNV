#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

class hkRelocationInfo;
class hkStreamWriter;

// Interface for writing reflected objects and unstructured bytes.
class hkObjectWriter : public hkReferencedObject {
public:
    virtual hkResult writeObject(hkStreamWriter*, const void*, const hkClass&, hkRelocationInfo&) = 0;
    virtual hkResult writeRaw(hkStreamWriter*, const void*, int32_t) = 0;
};
