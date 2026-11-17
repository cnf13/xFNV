#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Packfile/hkPackfileWriter.h"
#include "Havok/Source/Common/Serialize/Tagfile/hkTagfileWriter.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"

#include <stdint.h>

class hkClassNameRegistry;
class hkDataObject;
class hkDataWorldDict;
class hkObjectResource;
class hkResource;
class hkStreamReader;
class hkStreamWriter;
class hkTypeInfoRegistry;
class hkVersionPatchManager;

// Havok's load and save entry points for serialized files, and the types its callers pass around.
namespace hkSerializeUtil {

    // Why a load failed.
    struct ErrorDetails {
        enum ErrorID : int32_t {
            ERRORID_NONE = 0,
            ERRORID_READ_FAILED = 1,
            ERRORID_UNSUPPORTED_FORMAT = 2,
            ERRORID_PACKFILE_PLATFORM = 3,
            ERRORID_VERSIONING_FAILED = 4,
            ERRORID_NONHEAP_OBJECT = 5,
            ERRORID_LOAD_FAILED = 6,
            ERRORID_DEPRECATED_NOT_INITIALIZED = 7,
            ERRORID_MAX_ID = 8,
        };

        hkEnum<ErrorID, int32_t> id;
        hkStringPtr defaultMessage;
    };

    // The kinds of file detectFormat tells apart.
    enum FormatType : int32_t {
        FORMAT_ERROR = 0,
        FORMAT_UNKNOWN = 1,
        FORMAT_PACKFILE_BINARY = 2,
        FORMAT_PACKFILE_XML = 3,
        FORMAT_TAGFILE_BINARY = 4,
    };

    // What detectFormat found out about a file.
    struct FormatDetails {
        hkEnum<FormatType, int32_t> m_formatType;
        hkStringPtr m_version;
        hkStructureLayout::LayoutRules m_layoutRules;
    };

    // Whether sr holds a file this build can load (hkBool32).
    int32_t isLoadable(hkStreamReader* sr);

    // Saves object to writer as a binary tagfile; fails when writer is null. binary is not read: this build writes only
    // binary tagfiles. The SDK's hkBool32 is a plain int here.
    hkResult saveTagfile(const hkDataObject& object, hkStreamWriter* writer, int32_t binary);

    // Saves object of class klass to writer as a tagfile through a native data world, telling userListener of each
    // object; binary is not read.
    hkResult saveTagfile(
        const void* object,
        const hkClass& klass,
        hkStreamWriter* writer,
        int32_t binary,
        hkPackfileWriter::AddObjectListener* userListener
    );

} // namespace hkSerializeUtil

// File-local helper of hkSerializeUtil.cpp: saves obj to stream through a stack hkBinaryTagfileWriter, telling listener
// of each object, or returns HK_FAILURE when stream is null.
hkResult
_saveTagfile(const hkDataObject& obj, hkStreamWriter* stream, hkTagfileWriter::AddDataObjectListener* listener);
// The first character of r_eax that is not a space.
char* skipSpaces(char* r_eax);
// Finds the attribute `key="value"` in the text: a1 gets the value and the function returns 1, or 0 when the key, the
// '=' or either quote is missing.
int32_t findTextValue(hkStringPtr* a1, const char* r_eax, const char* r_edi);
// Reads the whole of reader into buf: at once through seek and tell when the reader supports them, else in 128 KB
// chunks until a read returns nothing. Returns the last read's result.
int32_t readStreamToBuffer(hkStreamReader* reader, hkArray<char, hkContainerTempAllocator>* buf);
// Loads a packfile: a binary one of this build's layout and contents version natively through typeReg, any other
// through the deprecated serializer; a binary one of another layout fails with ERRORID_PACKFILE_PLATFORM.
hkResource* loadPackfile(
    hkSerializeUtil::FormatDetails* details,
    hkStreamReader* stream,
    const hkTypeInfoRegistry* typeReg,
    hkSerializeUtil::ErrorDetails* errorOut
);
// Loads a native binary packfile on the heap: its contents wrapped in a native data world and copied into an object
// resource.
hkObjectResource* loadNewPackfileOnHeap(
    hkStreamReader* stream,
    hkSerializeUtil::FormatDetails* details,
    hkSerializeUtil::ErrorDetails* errorOut,
    const hkTypeInfoRegistry* typeReg,
    const hkClassNameRegistry* classReg
);
// Whether manager can version every class of world that classReg lacks or has at another version: 1, or 0 with errorOut
// set to ERRORID_VERSIONING_FAILED.
int32_t checkTagfileWorld(
    hkSerializeUtil::ErrorDetails* errorOut,
    hkVersionPatchManager* manager,
    const hkClassNameRegistry* classReg,
    hkDataWorldDict* world
);
// Reads a binary tagfile from stream into world and versions it to classReg's classes.
hkResult loadTagfileIntoWorld(
    hkStreamReader* stream,
    hkSerializeUtil::ErrorDetails* errorOut,
    hkDataWorldDict* world,
    const hkClassNameRegistry* classReg
);

// The file-local helpers of hkSerializeUtil.cpp.

namespace hkSerializeUtil {
    hkResult savePackfile(
        const void* object,
        const hkClass& klass,
        hkStreamWriter* writer,
        const hkPackfileWriter::Options& options,
        int32_t binary,
        hkPackfileWriter::AddObjectListener* userListener
    );
} // namespace hkSerializeUtil

namespace hkSerializeUtil {
    hkResult save(const hkDataObject& object, hkStreamWriter* writer);
} // namespace hkSerializeUtil

namespace hkSerializeUtil {
    hkResult save(const void* object, const hkClass& klass, hkStreamWriter* writer);
} // namespace hkSerializeUtil

namespace hkSerializeUtil {
    hkResource* load(
        const void* buf,
        int32_t buflen,
        hkSerializeUtil::ErrorDetails* resultOut,
        const hkClassNameRegistry* classReg
    );
} // namespace hkSerializeUtil

namespace hkSerializeUtil {
    // The filename form: opens an hkIstream on filename and loads through the stream overload.
    hkResource*
    load(const char* filename, hkSerializeUtil::ErrorDetails* detailsOut, const hkClassNameRegistry* classReg);
} // namespace hkSerializeUtil

namespace hkSerializeUtil {
    // The stream form: fails with stream null or not ok (hkStreamReader
    // vtable +0xC), else loads the packfile or tagfile detectFormat finds. The SDK's signature (hkSerializeUtil.h:46);
    // the filename and buffer forms call it with their hkIstream's reader.
    hkResource*
    load(hkStreamReader* stream, hkSerializeUtil::ErrorDetails* detailsOut, const hkClassNameRegistry* classReg);
} // namespace hkSerializeUtil

namespace hkSerializeUtil {
    // The stream form: like load, with typeReg (the builtin registry's
    // when null) for the loaded objects. The SDK's signature (hkSerializeUtil.h:54).
    hkObjectResource* loadOnHeap(
        hkStreamReader* stream,
        hkSerializeUtil::ErrorDetails* errorOut,
        const hkClassNameRegistry* classReg,
        const hkTypeInfoRegistry* typeReg
    );
} // namespace hkSerializeUtil

namespace hkSerializeUtil {
    hkObjectResource* loadOnHeap(
        const void* buf,
        int32_t buflen,
        hkSerializeUtil::ErrorDetails* errorOut,
        const hkClassNameRegistry* classReg,
        const hkTypeInfoRegistry* typeReg
    );
} // namespace hkSerializeUtil

namespace hkSerializeUtil {
    // The filename form: opens an hkIstream on filename and loads through the stream overload.
    hkObjectResource* loadOnHeap(
        const char* filename,
        hkSerializeUtil::ErrorDetails* errorOut,
        const hkClassNameRegistry* classReg,
        const hkTypeInfoRegistry* typeReg
    );
} // namespace hkSerializeUtil

namespace hkSerializeUtil {
    // Returns the format the full detectFormat finds in reader, through apResult.
    hkEnum<hkSerializeUtil::FormatType, int32_t>*
    detectFormat(hkEnum<hkSerializeUtil::FormatType, int32_t>* apResult, hkStreamReader* reader);
} // namespace hkSerializeUtil

namespace hkSerializeUtil {
    // The full detection: peeks at reader's first 128 bytes through its mark and sets the format type, the contents
    // version and the layout rules.
    void detectFormat(hkStreamReader* reader, hkSerializeUtil::FormatDetails& details);
} // namespace hkSerializeUtil
