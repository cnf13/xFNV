#include "Havok/Source/Common/Serialize/Util/hkSerializeUtil.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkCachedHashMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkClassNameRegistry.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkTypeInfoRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Io/IStream/hkIstream.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Compat/Deprecated/hkSerializeDeprecated.h"
#include "Havok/Source/Common/Serialize/Data/Dict/hkDataObjectDict.h"
#include "Havok/Source/Common/Serialize/Data/Native/hkDataObjectNative.h"
#include "Havok/Source/Common/Serialize/Data/Util/hkDataObjectUtil.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObjectImpl.h"
#include "Havok/Source/Common/Serialize/Data/hkDataWorld.h"
#include "Havok/Source/Common/Serialize/Packfile/Binary/hkBinaryPackfileWriter.h"
#include "Havok/Source/Common/Serialize/Packfile/Binary/hkPackfileHeader.h"
#include "Havok/Source/Common/Serialize/Packfile/hkPackfileWriter.h"
#include "Havok/Source/Common/Serialize/Resource/hkObjectResource.h"
#include "Havok/Source/Common/Serialize/Resource/hkResource.h"
#include "Havok/Source/Common/Serialize/Tagfile/Binary/hkBinaryTagfileCommon.h"
#include "Havok/Source/Common/Serialize/Tagfile/Binary/hkBinaryTagfileReader.h"
#include "Havok/Source/Common/Serialize/Tagfile/Binary/hkBinaryTagfileWriter.h"
#include "Havok/Source/Common/Serialize/Tagfile/hkTagfileReader.h"
#include "Havok/Source/Common/Serialize/Tagfile/hkTagfileWriter.h"
#include "Havok/Source/Common/Serialize/Util/hkBuiltinTypeRegistry.h"
#include "Havok/Source/Common/Serialize/Util/hkNativePackfileUtils.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"
#include "Havok/Source/Common/Serialize/Version/hkVersionPatchManager.h"
#include "Havok/Source/Common/Serialize/Version/hkVersionUtil.h"
#include "SDK/MSVC/string.h"

namespace hkSerializeUtil {
    struct FormatDetails;
}

namespace {

    // The packfile save's listener that forwards each data object the tagfile writer adds to a packfile writer's
    // listener.
    class ForwardingPackfileListerer : public hkTagfileWriter::AddDataObjectListener {
    public:
        ForwardingPackfileListerer(hkPackfileWriter::AddObjectListener* apListener, hkDataWorldNative& arWorld)
          : m_listener(apListener), m_nativeWorld(arWorld) {}

        // addDataObjectCallback: with a listener and unless the object test is set, the object's class is looked up by
        // name through hkBuiltinTypeRegistry's class name registry and handed with the object to m_listener's
        // addObjectCallback; a replaced object or class is wrapped with wrapObject, else the object is returned.
        hkDataObject addDataObjectCallback(const hkDataObject& arObject) const override;

        // Wraps object of class klass as a data object of m_nativeWorld, or a null data object for a null object.
        hkDataObject wrapObject(void* object, const hkClass& klass) const;

        // The packfile writer's listener.
        hkPackfileWriter::AddObjectListener* m_listener;
        // +0xC: the native world the objects live in.
        hkDataWorldNative& m_nativeWorld;
    };

    hkDataObject ForwardingPackfileListerer::addDataObjectCallback(const hkDataObject& arObject) const {
        if (m_listener && !arObject.isNull()) {
            hkDataClass dataClass = arObject.getClass();
            hkClassNameRegistry* pRegistry = hkBuiltinTypeRegistry::s_instance->getClassNameRegistry();
            const hkClass* pOrigClass = pRegistry->getClassByName(dataClass.getName());
            hkDataObject::Handle handle = arObject.getHandle();
            const void* object = handle.p0;
            if (handle.p1) {
                pOrigClass = static_cast<const hkClass*>(handle.p1);
            }
            const hkClass* klass = pOrigClass;
            m_listener->addObjectCallback(object, klass);
            // dataClass's destructor, which runs on both paths below, is an empty function.
            if (object != handle.p0 || klass != pOrigClass) {
                // The source's const_cast: wrapObject takes the object non-const.
                return wrapObject(const_cast<void*>(object), *klass);
            }
        }
        return arObject;
    }

    hkDataObject ForwardingPackfileListerer::wrapObject(void* object, const hkClass& klass) const {
        if (object) {
            return hkDataObject(m_nativeWorld.wrapObject(object, klass));
        }
        return hkDataObject(nullptr);
    }

} // namespace

// Accepts a tagfile, a binary packfile of this build's layout and contents version, or whatever the deprecated
// serializer accepts (an older binary packfile or an XML packfile).
int32_t hkSerializeUtil::isLoadable(hkStreamReader* sr) {
    if (!sr) {
        return 0;
    }
    if (!sr->isOk().m_bool) {
        return 0;
    }
    hkEnum<hkTagfileReader::FormatType, int32_t> tagfileFormat;
    if (hkTagfileReader::detectFormat(&tagfileFormat, sr)->m_storage > hkTagfileReader::FORMAT_UNKNOWN) {
        return 1;
    }
    hkSerializeUtil::FormatDetails details;
    detectFormat(sr, details);
    if (details.m_formatType.m_storage == FORMAT_PACKFILE_BINARY) {
        if (hkString::memCmp(
                &details.m_layoutRules,
                &hkStructureLayout::HostLayoutRules,
                sizeof(hkStructureLayout::LayoutRules)
            )) {
            return 0;
        }
        if (details.m_version == hkVersionUtil::getCurrentVersion()) {
            return 1;
        }
    }
    else if (details.m_formatType.m_storage != FORMAT_PACKFILE_XML) {
        return 0;
    }
    if (hkSerializeDeprecated::s_instance->isLoadable(details)) {
        return 1;
    }
    return 0;
}

// Passes no listener and ignores binary: _saveTagfile always writes a binary tagfile.
hkResult hkSerializeUtil::saveTagfile(const hkDataObject& object, hkStreamWriter* writer, int32_t binary) {
    return _saveTagfile(object, writer, nullptr);
}

// The listener is a ForwardingPackfileListerer; the contents are built in place.
hkResult hkSerializeUtil::saveTagfile(
    const void* object,
    const hkClass& klass,
    hkStreamWriter* writer,
    int32_t binary,
    hkPackfileWriter::AddObjectListener* userListener
) {
    hkDataWorldNative world;
    // The source's const_cast: setContents takes the object non-const.
    world.setContents(const_cast<void*>(object), klass);
    ForwardingPackfileListerer listener(userListener, world);
    // Calls getContents directly.
    hkDataObject contents = world.hkDataWorldNative::getContents();
    hkResult result = _saveTagfile(contents, writer, &listener);
    return result;
}

hkResult
_saveTagfile(const hkDataObject& obj, hkStreamWriter* stream, hkTagfileWriter::AddDataObjectListener* listener) {
    if (!stream) {
        return HK_FAILURE;
    }
    hkBinaryTagfileWriter v4;
    return v4.hkBinaryTagfileWriter::save(obj, stream, listener);
}

char* skipSpaces(char* text) {
    for (char i = *text; i; i = *++text) {
        if (i != 32) {
            break;
        }
    }
    return text;
}

int32_t findTextValue(hkStringPtr* valueOut, const char* text, const char* key) {
    const char* pcFound = hkString::strStr(text, key);
    if (!pcFound) {
        return 0;
    }
    // skipSpaces only reads the text; it is declared on char*.
    char* pcValue = skipSpaces(const_cast<char*>(pcFound + hkString::strLen(key)));
    if (*pcValue != '=') {
        return 0;
    }
    pcValue = skipSpaces(pcValue + 1);
    if (*pcValue != '"') {
        return 0;
    }
    ++pcValue;
    const char* pcEnd = hkString::strChr(pcValue, '"');
    if (!pcEnd) {
        return 0;
    }
    valueOut->set(pcValue, static_cast<int32_t>(pcEnd - pcValue));
    return 1;
}

int32_t readStreamToBuffer(hkStreamReader* reader, hkArray<char, hkContainerTempAllocator>* buf) {
    if (reader->seekTellSupported().m_bool) {
        reader->seek(0, hkStreamReader::STREAM_END);
        int32_t size = reader->tell();
        buf->setSize(size);
        reader->seek(0, hkStreamReader::STREAM_SET);
        return reader->read(buf->m_data, buf->m_size);
    }
    const int32_t chunkSize = 0x20000;
    int32_t numRead;
    do {
        buf->reserve(buf->m_size + chunkSize);
        numRead = reader->read(buf->m_data + buf->m_size, chunkSize);
        buf->m_size += numRead;
    }
    while (numRead > 0);
    return numRead;
}

hkResource* loadPackfile(
    hkSerializeUtil::FormatDetails* details,
    hkStreamReader* stream,
    const hkTypeInfoRegistry* typeReg,
    hkSerializeUtil::ErrorDetails* errorOut
) {
    if (details->m_formatType.m_storage == hkSerializeUtil::FORMAT_PACKFILE_BINARY) {
        if (hkString::memCmp(
                &details->m_layoutRules,
                &hkStructureLayout::HostLayoutRules,
                sizeof(hkStructureLayout::LayoutRules)
            )) {
            if (errorOut) {
                errorOut->set(hkSerializeUtil::ErrorDetails::ERRORID_PACKFILE_PLATFORM, "Wrong platform for packfile");
            }
            return nullptr;
        }
        if (details->m_version == hkVersionUtil::getCurrentVersion()) {
            using TempCharArray = hkArray<char, hkContainerTempAllocator>;
            TempCharArray buf;
            readStreamToBuffer(stream, &buf);
            hkResource* resource = hkNativePackfileUtils::load(buf.m_data, buf.m_size, typeReg);
            return resource;
        }
    }
    return hkSerializeDeprecated::s_instance->loadOldPackfile(*stream, *details, errorOut);
}

hkObjectResource* loadNewPackfileOnHeap(
    hkStreamReader* stream,
    hkSerializeUtil::FormatDetails* details,
    hkSerializeUtil::ErrorDetails* errorOut,
    const hkTypeInfoRegistry* typeReg,
    const hkClassNameRegistry* classReg
) {
    hkTypeInfoRegistry registry;
    registry.m_finishFlag.m_finishing = 0;
    registry.hkTypeInfoRegistry::merge(*typeReg);
    hkResource* resource = loadPackfile(details, stream, &registry, errorOut);
    hkObjectResource* result = nullptr;
    {
        hkDataWorldNative world;
        world.setClassRegistry(classReg);
        const hkClass* klass = classReg->getClassByName(resource->getContentsTypeName());
        if (klass) {
            void* contents = resource->getContentsPointer(nullptr, typeReg);
            hkDataObjectImpl* impl = world.wrapObject(contents, *klass);
            hkDataObject obj(impl);
            result = hkDataObjectUtil::toObjectWithRegistry(obj, classReg, typeReg);
        }
    }
    resource->removeReference();
    return result;
}

int32_t checkTagfileWorld(
    hkSerializeUtil::ErrorDetails* errorOut,
    hkVersionPatchManager* manager,
    const hkClassNameRegistry* classReg,
    hkDataWorldDict* world
) {
    manager->recomputePatchDependencies();
    hkArray<hkDataClassImpl*> classes;
    world->findAllClasses(classes);
    int32_t ok = 1;
    for (int32_t i = 0; i < classes.m_size; ++i) {
        const char* name = classes.m_data[i]->getName();
        int32_t version = classes.m_data[i]->getVersion();
        const hkClass* klass = classReg->getClassByName(name);
        if (!klass || klass->getDescribedVersion() != version) {
            hkUint64 uid = manager->getUid(name, version);
            if (manager->m_patchIndexFromUid.getWithDefault(uid, -1) == -1) {
                errorOut->id = hkSerializeUtil::ErrorDetails::ERRORID_VERSIONING_FAILED;
                printfStringPtr(
                    &errorOut->defaultMessage,
                    "Unable to version data of class %s, version 0x%p.",
                    name,
                    version
                );
                ok = 0;
                break;
            }
        }
    }
    return ok;
}

// The loaded contents are released at once.
hkResult loadTagfileIntoWorld(
    hkStreamReader* stream,
    hkSerializeUtil::ErrorDetails* errorOut,
    hkDataWorldDict* world,
    const hkClassNameRegistry* classReg
) {
    hkBinaryTagfileReader reader;
    hkDataObject contents = reader.hkBinaryTagfileReader::load(stream, *world);
    if (errorOut && !checkTagfileWorld(errorOut, hkVersionPatchManager::s_instance, classReg, world)) {
        return HK_FAILURE;
    }
    return hkVersionPatchManager::s_instance->applyPatches(*world, classReg);
}

// Binary: an hkBinaryPackfileWriter on the stack; XML: the deprecated serializer's saveXmlPackfile.
hkResult hkSerializeUtil::savePackfile(
    const void* object,
    const hkClass& klass,
    hkStreamWriter* writer,
    const hkPackfileWriter::Options& options,
    int32_t binary,
    hkPackfileWriter::AddObjectListener* userListener
) {
    if (!writer) {
        return HK_FAILURE;
    }
    if (!binary) {
        return hkSerializeDeprecated::s_instance->saveXmlPackfile(object, klass, writer, options, userListener);
    }
    hkBinaryPackfileWriter apThis;
    apThis.hkPackfileWriter::setContents(object, klass, userListener);
    hkResult v6 = apThis.hkBinaryPackfileWriter::save(writer, options);
    return v6;
}

hkResult hkSerializeUtil::save(const hkDataObject& object, hkStreamWriter* writer) {
    return hkSerializeUtil::saveTagfile(object, writer, 1);
}

hkResult hkSerializeUtil::save(const void* object, const hkClass& klass, hkStreamWriter* writer) {
    return hkSerializeUtil::saveTagfile(object, klass, writer, 1, nullptr);
}

// In-memory load: a binary packfile of this build's layout and contents version (its header compared against a
// default-constructed header) is loaded in place; anything else goes through a memory stream to the stream overload.
hkResource* hkSerializeUtil::load(
    const void* buf,
    int32_t buflen,
    hkSerializeUtil::ErrorDetails* resultOut,
    const hkClassNameRegistry* classReg
) {
    hkPackfileHeader magic;
    const hkPackfileHeader* pHeader = static_cast<const hkPackfileHeader*>(buf);
    if (pHeader->m_magic[0] == magic.m_magic[0] &&
        pHeader->m_magic[1] == magic.m_magic[1] &&
        !hkString::memCmp(
            pHeader->m_layoutRules,
            &hkStructureLayout::HostLayoutRules,
            sizeof(hkStructureLayout::LayoutRules)
        ) &&
        !hkString::strCmp(pHeader->m_contentsVersion, hkVersionUtil::getCurrentVersion())) {
        return hkNativePackfileUtils::load(buf, buflen, nullptr);
    }
    hkIstream stream(buf, buflen);
    hkIstream* pStream = &stream;
    hkResource* pResource = hkSerializeUtil::load(pStream->m_streamReader, resultOut, classReg);
    return pResource;
}

hkResource* hkSerializeUtil::load(
    const char* filename,
    hkSerializeUtil::ErrorDetails* detailsOut,
    const hkClassNameRegistry* classReg
) {
    hkIstream apMemory(filename);
    hkIstream* v3 = &apMemory;
    hkResource* v4 = hkSerializeUtil::load(v3->m_streamReader, detailsOut, classReg);
    return v4;
}

hkResource* hkSerializeUtil::load(
    hkStreamReader* stream,
    hkSerializeUtil::ErrorDetails* detailsOut,
    const hkClassNameRegistry* classReg
) {
    if (!stream) {
        if (detailsOut) {
            detailsOut->set(ErrorDetails::ERRORID_READ_FAILED, "Stream pointer is null");
        }
        return nullptr;
    }
    if (!stream->isOk().m_bool) {
        if (detailsOut) {
            detailsOut->set(ErrorDetails::ERRORID_READ_FAILED, "Stream is not ok");
        }
        return nullptr;
    }
    if (!classReg) {
        classReg = hkBuiltinTypeRegistry::s_instance->getClassNameRegistry();
    }
    hkSerializeUtil::FormatDetails details;
    detectFormat(stream, details);
    int32_t format = details.m_formatType.m_storage;
    if (format >= FORMAT_PACKFILE_BINARY) {
        if (format <= FORMAT_PACKFILE_XML) {
            hkResource* resource = loadPackfile(
                &details,
                stream,
                hkBuiltinTypeRegistry::s_instance->getTypeInfoRegistry(),
                detailsOut
            );
            return resource;
        }
        if (format == FORMAT_TAGFILE_BINARY) {
            hkDataWorldDict world;
            if (loadTagfileIntoWorld(stream, detailsOut, &world, classReg) != HK_SUCCESS) {
                return nullptr;
            }
            // getContents is called directly.
            hkDataObject contents = world.hkDataWorldDict::getContents();
            hkResource* resource = hkDataObjectUtil::toResourceWithRegistry(contents, classReg);
            return resource;
        }
    }
    if (detailsOut) {
        detailsOut->set(ErrorDetails::ERRORID_UNSUPPORTED_FORMAT, "Unable to detect format from stream");
    }
    return nullptr;
}

hkObjectResource* hkSerializeUtil::loadOnHeap(
    hkStreamReader* stream,
    hkSerializeUtil::ErrorDetails* errorOut,
    const hkClassNameRegistry* classReg,
    const hkTypeInfoRegistry* typeReg
) {
    if (!stream) {
        if (errorOut) {
            errorOut->set(ErrorDetails::ERRORID_READ_FAILED, "Stream pointer is null");
        }
        return nullptr;
    }
    if (!stream->isOk().m_bool) {
        if (errorOut) {
            errorOut->set(ErrorDetails::ERRORID_READ_FAILED, "Stream is not ok");
        }
        return nullptr;
    }
    if (!classReg) {
        classReg = hkBuiltinTypeRegistry::s_instance->getClassNameRegistry();
    }
    if (!typeReg) {
        typeReg = hkBuiltinTypeRegistry::s_instance->getTypeInfoRegistry();
    }
    hkSerializeUtil::FormatDetails details;
    detectFormat(stream, details);
    hkObjectResource* resource = nullptr;
    switch (details.m_formatType.m_storage) {
        case FORMAT_PACKFILE_BINARY: {
            if (details.m_layoutRules.IsEqual(hkStructureLayout::HostLayoutRules).m_bool &&
                details.m_version == "Havok-7.1.0-r1") {
                resource = loadNewPackfileOnHeap(stream, &details, errorOut, typeReg, classReg);
                break;
            }
            resource = hkSerializeDeprecated::s_instance->loadOldPackfileOnHeap(*stream, details, errorOut);
            break;
        }
        case FORMAT_PACKFILE_XML:
            resource = hkSerializeDeprecated::s_instance->loadOldPackfileOnHeap(*stream, details, errorOut);
            break;
        case FORMAT_TAGFILE_BINARY: {
            hkDataWorldDict world;
            if (loadTagfileIntoWorld(stream, errorOut, &world, classReg) == HK_SUCCESS) {
                // getContents is called directly.
                hkDataObject contents = world.hkDataWorldDict::getContents();
                resource = hkDataObjectUtil::toObjectWithRegistry(contents, classReg, typeReg);
            }
            break;
        }
        default:
            if (errorOut) {
                errorOut->set(ErrorDetails::ERRORID_UNSUPPORTED_FORMAT, "Unable to detect format from stream");
            }
            break;
    }
    return resource;
}

hkObjectResource* hkSerializeUtil::loadOnHeap(
    const void* buf,
    int32_t buflen,
    hkSerializeUtil::ErrorDetails* errorOut,
    const hkClassNameRegistry* classReg,
    const hkTypeInfoRegistry* typeReg
) {
    hkIstream apThis(buf, buflen);
    hkIstream* v5 = &apThis;
    hkObjectResource* v6 = hkSerializeUtil::loadOnHeap(v5->m_streamReader, errorOut, classReg, typeReg);
    return v6;
}

hkObjectResource* hkSerializeUtil::loadOnHeap(
    const char* filename,
    hkSerializeUtil::ErrorDetails* errorOut,
    const hkClassNameRegistry* classReg,
    const hkTypeInfoRegistry* typeReg
) {
    hkIstream apMemory(filename);
    hkIstream* v4 = &apMemory;
    hkObjectResource* v5 = hkSerializeUtil::loadOnHeap(v4->m_streamReader, errorOut, classReg, typeReg);
    return v5;
}

// Returns the format type found by the full detection.
hkEnum<hkSerializeUtil::FormatType, int32_t>*
hkSerializeUtil::detectFormat(hkEnum<hkSerializeUtil::FormatType, int32_t>* apResult, hkStreamReader* reader) {
    hkSerializeUtil::FormatDetails details;
    detectFormat(reader, details);
    apResult->m_storage = details.m_formatType.m_storage;
    return apResult;
}

// The bytes read are compared with the magic numbers that hkPackfileHeader's constructor sets.
void hkSerializeUtil::detectFormat(hkStreamReader* reader, hkSerializeUtil::FormatDetails& details) {
    details.m_formatType = FORMAT_ERROR;
    details.m_version = static_cast<const char*>(nullptr);
    hkMemUtil::memSet(&details.m_layoutRules, 0, sizeof(hkStructureLayout::LayoutRules));
    char buf[128];
    std::memset(buf, 0, sizeof(buf));
    reader->setMark(sizeof(buf));
    int32_t nbytes = reader->read(buf, sizeof(buf));
    reader->rewindToMark();
    hkPackfileHeader magic;
    // buf is raw, untyped bytes from the stream, read as a packfile header.
    const hkPackfileHeader* header = reinterpret_cast<const hkPackfileHeader*>(buf);
    uint8_t isTagfile;
    if (nbytes >= 8 && *hkBinaryTagfile::isBinaryMagic(&isTagfile, header->m_magic[0], header->m_magic[1])) {
        details.m_formatType = FORMAT_TAGFILE_BINARY;
        return;
    }
    if (nbytes >= static_cast<int32_t>(sizeof(hkPackfileHeader)) &&
        header->m_magic[0] == magic.m_magic[0] &&
        header->m_magic[1] == magic.m_magic[1]) {
        details.m_formatType = FORMAT_PACKFILE_BINARY;
        details.m_version = header->m_contentsVersion;
        hkMemUtil::memCpy(&details.m_layoutRules, header->m_layoutRules, sizeof(hkStructureLayout::LayoutRules));
        return;
    }
    if (!hkString::strStr(buf, "<hkpackfile ") && !hkString::strStr(buf, "<hkpackfile>")) {
        return;
    }
    details.m_formatType = FORMAT_PACKFILE_XML;
    hkStringBuf tag(hkString::strStr(buf, "<hkpackfile"));
    hkStringPtr classVersion;
    int32_t version;
    if (!findTextValue(&classVersion, tag.m_string.m_data, "classversion") ||
        (version = hkString::atoi(classVersion.cString(), 0)) == 1) {
        details.m_version = "Havok-3.0.0";
    }
    else if (static_cast<uint32_t>(version - 2) > 5) {
        details.m_version = "error";
    }
    else if (!findTextValue(&details.m_version, tag.m_string.m_data, "contentsversion")) {
        details.m_version = "Havok-3.1.0";
    }
}

uint8_t* hkBinaryTagfile::isBinaryMagic(uint8_t* result, int32_t number0, int32_t number1) {
    if ((number0 == -0x354FF2E2 && number1 == -0x2FEE0532) ||
        (std::byteswap(number0) == -0x354FF2E2 && std::byteswap(number1) == -0x2FEE0532)) {
        *result = 1;
        return result;
    }

    *result = 0;
    return result;
}

hkBool hkStructureLayout::LayoutRules::IsEqual(const hkStructureLayout::LayoutRules& other) const {
    if (m_bytesInPointer == other.m_bytesInPointer &&
        (m_littleEndian == other.m_littleEndian &&
         (m_reusePaddingOptimization == other.m_reusePaddingOptimization &&
          m_emptyBaseClassOptimization == other.m_emptyBaseClassOptimization))) {
        return 1;
    }

    return 0;
}
