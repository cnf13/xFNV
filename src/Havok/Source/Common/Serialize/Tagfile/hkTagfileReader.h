#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"

class hkDataWorld;
class hkStreamReader;

// The interface of the tagfile readers: load reads a tagfile from stream into world and returns its root object. No
// data of its own. Abstract.
class hkTagfileReader : public hkReferencedObject {
public:
    enum FormatType : int32_t {
        FORMAT_ERROR = 0,
        FORMAT_UNKNOWN = 1,
        FORMAT_BINARY = 2,
    };

    virtual hkDataObject load(hkStreamReader* stream, hkDataWorld& world) = 0;

    static hkEnum<hkTagfileReader::FormatType, int32_t>*
    detectFormat(hkEnum<hkTagfileReader::FormatType, int32_t>* apResult, hkStreamReader* stream);
};
