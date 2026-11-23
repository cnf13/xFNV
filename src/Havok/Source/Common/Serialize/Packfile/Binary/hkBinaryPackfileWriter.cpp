#include "Havok/Source/Common/Serialize/Packfile/Binary/hkBinaryPackfileWriter.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkCachedHashMap.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Common/Base/System/Io/OArchive/hkOArchive.h"
#include "Havok/Source/Common/Base/System/Io/Writer/SubStream/hkSubStreamWriter.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Common/Serialize/Packfile/Binary/hkPackfileHeader.h"
#include "Havok/Source/Common/Serialize/Packfile/Binary/hkPackfileSectionHeader.h"
#include "Havok/Source/Common/Serialize/Packfile/hkPackfileWriter.h"
#include "Havok/Source/Common/Serialize/Serialize/Platform/hkPlatformObjectWriter.h"
#include "Havok/Source/Common/Serialize/Serialize/hkObjectWriter.h"
#include "Havok/Source/Common/Serialize/Serialize/hkRelocationInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"
#include "SDK/MSVC/string.h"

#include <bit>

void writePackfileHeader(
    hkStreamWriter* apStream,
    const hkPackfileWriter::Options& arOptions,
    hkPlatformObjectWriter* apWriter,
    int32_t aiNumSections
) {
    hkPackfileHeader header;
    header.m_userTag = static_cast<int32_t>(arOptions.m_userTag);
    header.m_fileVersion = 7;
    hkString::memCpy(header.m_layoutRules, &arOptions.m_layout, 4);
    header.m_numSections = aiNumSections;
    hkString::memSet(header.m_pad, -1, sizeof(header.m_pad));
    if (arOptions.m_contentsVersion) {
        hkString::strNcpy(header.m_contentsVersion, arOptions.m_contentsVersion, 16);
    }
    else {
        hkPackfileWriter::getCurrentVersion(header.m_contentsVersion, 16);
    }
    hkRelocationInfo fixups;
    apWriter->writeObject(apStream, &header, hkPackfileHeaderClass, fixups);
}

// Multiplies the header count by the header size instead of reading the table's bytes.
int32_t sectionTableSize(const hkArray<hkPackfileSectionHeader, hkContainerHeapAllocator>& arSections) {
    return 48 * arSections.m_size;
}

void padUp(int32_t pad, hkStreamWriter* writer) {
    using CharInplaceArray32 = hkInplaceArray<char, 32>;
    int32_t v3 = writer->tell();
    CharInplaceArray32 buf;
    buf.reserve(pad);
    int32_t v9 = pad - buf.m_size;
    char* v8 = buf.m_data + buf.m_size;
    if (v9 > 0) {
        std::memset(v8, -1, v9);
    }
    buf.m_size = pad;
    if ((v3 & pad - 1) != 0) {
        writer->write(buf.m_data, pad - (v3 & pad - 1));
    }
}

hkBinaryPackfileWriter::SaveContext::SaveContext(const Options& arOptions)
  : m_options(&arOptions), m_writer(arOptions.m_layout),
    m_endianSwapRequired(
        hkStructureLayout::HostLayoutRules.m_littleEndian != arOptions.m_layout.m_rules.m_littleEndian
    ) {}

hkBinaryPackfileWriter::hkBinaryPackfileWriter() {
    hkPackfileWriter::addSection(sectionTag);
    hkPackfileWriter::setSectionForClass(hkClassClass, hkPackfileWriter::SECTION_TAG_TYPES);
    hkPackfileWriter::setSectionForClass(hkClassMemberClass, hkPackfileWriter::SECTION_TAG_TYPES);
    hkPackfileWriter::setSectionForClass(hkClassEnumClass, hkPackfileWriter::SECTION_TAG_TYPES);
    hkPackfileWriter::setSectionForClass(hkClassEnumItemClass, hkPackfileWriter::SECTION_TAG_TYPES);
}

hkBinaryPackfileWriter::~hkBinaryPackfileWriter() {}

// Builds the context and the sub stream writer over the caller's stream, then writes the section table, the packfile
// header, the objects and the deferred writes, and seeks so the body lands where the header reserved room for it. The
// context writes through its m_stream, the sub stream writer.
hkResult hkBinaryPackfileWriter::save(hkStreamWriter* origStream, const Options& options) {
    SaveContext context(options);
    hkSubStreamWriter subStream(origStream);
    context.m_stream = &subStream;
    fillSectionTags(context);
    writePackfileHeader(context.m_stream, options, &context.m_writer, context.m_sectionHeaders.m_size);
    hkStreamWriter* pStream = &subStream;
    int32_t iHeaderEnd = pStream->tell();
    pStream->seek(sectionTableSize(context.m_sectionHeaders), hkStreamWriter::STREAM_CUR);
    writeAllObjects(context);
    int32_t iBodyEnd = pStream->tell();
    doDeferredWrites(context, iHeaderEnd);
    pStream->seek(iBodyEnd, hkStreamWriter::STREAM_SET);
    pStream->flush();
    return HK_SUCCESS;
}

void hkBinaryPackfileWriter::fillSectionTags(hkBinaryPackfileWriter::SaveContext& context) {
    int32_t m_size = m_knownSections.m_size;
    hkArray<hkPackfileSectionHeader, hkContainerHeapAllocator>* p_m_sectionHeaders = &context.m_sectionHeaders;
    p_m_sectionHeaders->reserve(m_size);
    int32_t v8 = context.m_sectionHeaders.m_size;
    int32_t v9 = m_size - v8;
    char* m_sectionTag = p_m_sectionHeaders->m_data[v8].m_sectionTag;
    if (v9 > 0) {
        char* v11 = m_sectionTag;
        int32_t arContexta = v9;
        do {
            if (v11) {
                hkString::memSet(v11, 0, 4);
            }
            v11 += 48;
            --arContexta;
        }
        while (arContexta);
    }
    p_m_sectionHeaders->m_size = m_size;
    int32_t v12 = sectionTableSize(*p_m_sectionHeaders);
    hkString::memSet(p_m_sectionHeaders->begin(), -1, v12);
    int32_t v13 = 0;
    if (m_knownSections.m_size > 0) {
        int32_t v14 = 0;
        do {
            hkString::strNcpy(p_m_sectionHeaders->m_data[v14++].m_sectionTag, m_knownSections.m_data[v13++], 19);
        }
        while (v13 < m_knownSections.m_size);
    }
}

// The types section (section 0) gets the class records of the four reflection classes and of every pending object's
// class, once each; then each other section gets its objects, written raw or through the platform writer, and after
// them its local fixups, global and virtual fixup space (written later by doDeferredWrites), exports and imports, each
// block padded. A global fixup whose target is an import becomes one.
void hkBinaryPackfileWriter::writeAllObjects(hkBinaryPackfileWriter::SaveContext& context) {
    hkPackfileSectionHeader* pTypesHeader = context.m_sectionHeaders.m_data;
    pTypesHeader->m_absoluteDataStart = context.m_stream->tell();
    using ClassNameMap_t = hkCachedHashMap<hkStringMapOperations, hkContainerHeapAllocator>;
    ClassNameMap_t doneClasses;
    const hkClass* const apClasses[4] = {&hkClassClass, &hkClassMemberClass, &hkClassEnumClass, &hkClassEnumItemClass};
    for (const hkClass* pClass : apClasses) {
        // The map's keys are hkUlong words holding the name pointers.
        doneClasses.insert(reinterpret_cast<hkUlong>(pClass->getName()), 1);
        writeClassName(context, *pClass, pTypesHeader->m_absoluteDataStart);
    }
    for (const PendingWrite& rWrite : m_pendingWrites) {
        hkBool bDone = doneClasses.isValid(doneClasses.findKey(reinterpret_cast<hkUlong>(rWrite.m_klass->getName())));
        if (!bDone.m_bool) {
            doneClasses.insert(reinterpret_cast<hkUlong>(rWrite.m_klass->getName()), 1);
            writeClassName(context, *rWrite.m_klass, pTypesHeader->m_absoluteDataStart);
        }
    }
    padUp(16, context.m_stream);
    const int32_t iTypesEnd = context.m_stream->tell() - pTypesHeader->m_absoluteDataStart;
    pTypesHeader->m_localFixupsOffset = iTypesEnd;
    pTypesHeader->m_globalFixupsOffset = iTypesEnd;
    pTypesHeader->m_virtualFixupsOffset = iTypesEnd;
    pTypesHeader->m_exportsOffset = iTypesEnd;
    pTypesHeader->m_importsOffset = iTypesEnd;
    pTypesHeader->m_endOffset = iTypesEnd;

    context.m_locations.setSize(m_pendingWrites.m_size);

    context.m_relocsBySect.setSize(context.m_sectionHeaders.m_size);

    const int32_t iTypesSection = static_cast<int32_t>(sectionTagToIndex(hkPackfileWriter::SECTION_TAG_TYPES));
    for (int32_t iSection = 1; iSection < context.m_sectionHeaders.m_size; ++iSection) {
        hkPackfileSectionHeader* pHeader = context.m_sectionHeaders.m_data + iSection;
        pHeader->m_absoluteDataStart = context.m_stream->tell();
        hkRelocationInfo relocs;

        if (!context.m_options->m_writeMetaInfo.m_bool && iSection == iTypesSection) {
            // The class records are not written: their objects get no location.
            for (int32_t i = 0; i < m_pendingWrites.m_size; ++i) {
                if (m_pendingWrites.m_data[i].m_sectionIndex == iTypesSection) {
                    context.m_locations.m_data[i].m_section = -1;
                    context.m_locations.m_data[i].m_offset = -1;
                }
            }
        }
        else {
            for (int32_t i = 0; i < m_pendingWrites.m_size; ++i) {
                const PendingWrite& rWrite = m_pendingWrites.m_data[i];
                if (rWrite.m_sectionIndex != iSection) {
                    continue;
                }
                const hkClass* pKlass = rWrite.m_klass;
                const void* pObject = rWrite.m_pointer;
                context.m_locations.m_data[i].m_section = iSection;
                Location& rLocation = context.m_locations.m_data[i];
                rLocation.m_offset = context.m_stream->tell() - pHeader->m_absoluteDataStart;
                if (pKlass) {
                    // Calls writeObject virtually through the hkObjectWriter base.
                    hkObjectWriter* pWriter = &context.m_writer;
                    pWriter->writeObject(context.m_stream, pObject, *pKlass, relocs);
                }
                else {
                    context.m_stream->write(rWrite.m_pointer, rWrite.m_dataSize);
                }
            }
        }

        // The local fixups become offsets from the section's start.
        for (hkRelocationInfo::Local& rLocal : relocs.m_local) {
            rLocal.m_fromOffset -= pHeader->m_absoluteDataStart;
            rLocal.m_toOffset -= pHeader->m_absoluteDataStart;
        }
        // A global fixup whose target is an import becomes an import, its fixup removed (the last one moved in).
        for (int32_t g = relocs.m_global.m_size - 1; g >= 0; --g) {
            const char* pImport = m_imports.getWithDefault(relocs.m_global.m_data[g].m_toAddress, nullptr);
            if (!pImport) {
                continue;
            }
            const hkRelocationInfo::Global& rGlobal = relocs.m_global.m_data[g];
            hkRelocationInfo::Import& rImport = relocs.m_imports.expandOne();
            const int32_t iFrom = rGlobal.m_fromOffset - pHeader->m_absoluteDataStart;
            rImport.m_identifier = pImport;
            rImport.m_fromOffset = iFrom;
            relocs.m_global.removeAt(g);
        }

        {
            hkOArchive oa(context.m_stream, context.m_endianSwapRequired);
            pHeader->m_localFixupsOffset = context.m_stream->tell() - pHeader->m_absoluteDataStart;
            // Each local fixup is two words.
            oa.writeArrayGeneric(relocs.m_local.m_data, sizeof(int32_t), 2 * relocs.m_local.m_size);
            padUp(16, context.m_stream);
            // The global and virtual fixups (three words each) are written by doDeferredWrites: room is left.
            pHeader->m_globalFixupsOffset = context.m_stream->tell() - pHeader->m_absoluteDataStart;
            context.m_stream->seek(
                3 * static_cast<int32_t>(sizeof(int32_t)) * relocs.m_global.m_size,
                hkStreamWriter::STREAM_CUR
            );
            context.m_relocsBySect.m_data[iSection].m_global.swap(relocs.m_global);
            padUp(16, context.m_stream);
            pHeader->m_virtualFixupsOffset = context.m_stream->tell() - pHeader->m_absoluteDataStart;
            context.m_stream->seek(
                3 * static_cast<int32_t>(sizeof(int32_t)) * relocs.m_finish.m_size,
                hkStreamWriter::STREAM_CUR
            );
            context.m_relocsBySect.m_data[iSection].m_finish.swap(relocs.m_finish);
            padUp(16, context.m_stream);

            // The exports of this section's objects: each object's offset and its export name.
            pHeader->m_exportsOffset = context.m_stream->tell() - pHeader->m_absoluteDataStart;
            int32_t it = m_exports.getIterator();
            hkBool bAnyExports;
            bAnyExports.m_bool = false;
            if (it <= m_exports.m_map.m_hashMod) {
                do {
                    const int32_t iWrite = m_knownObjects.getWithDefault(m_exports.getKey(it), -1);
                    if (iWrite >= 0 && m_pendingWrites.m_data[iWrite].m_sectionIndex == iSection) {
                        const char* pName = m_exports.getValue(it);
                        const int32_t iOffset = context.m_locations.m_data[iWrite].m_offset;
                        bAnyExports.m_bool = true;
                        oa.write32(iOffset);
                        oa.writeRaw(pName, hkString::strLen(pName) + 1);
                        padUp(4, context.m_stream);
                    }
                    it = m_exports.getNext(it);
                }
                while (it <= m_exports.m_map.m_hashMod);
                if (bAnyExports.m_bool) {
                    oa.write32(-1);
                    padUp(16, context.m_stream);
                }
            }

            pHeader->m_importsOffset = context.m_stream->tell() - pHeader->m_absoluteDataStart;
            for (const hkRelocationInfo::Import& rImport : relocs.m_imports) {
                oa.write32(rImport.m_fromOffset);
                oa.writeRaw(rImport.m_identifier, hkString::strLen(rImport.m_identifier) + 1);
                padUp(4, context.m_stream);
            }
            if (relocs.m_imports.m_size != 0) {
                oa.write32(-1);
                context.m_relocsBySect.m_data[iSection].m_imports.swap(relocs.m_imports);
                padUp(16, context.m_stream);
            }
            pHeader->m_endOffset = context.m_stream->tell() - pHeader->m_absoluteDataStart;
        }
    }
}

// The SDK's doDeferredWrites: for each section its global fixups (each from offset whose target is written, with the
// target's section and offset) and its virtual fixups (each from offset with 0 and its class name's offset), at
// their offsets in the section's data; then the section headers from aiStart on, and the file header's contents
// location and contents class name.
void hkBinaryPackfileWriter::doDeferredWrites(
    hkBinaryPackfileWriter::SaveContext& context,
    int32_t sectionHeadersStart
) {
    hkOArchive archive(context.m_stream, context.m_endianSwapRequired);
    for (int32_t i = 0; i < context.m_sectionHeaders.m_size; ++i) {
        hkPackfileSectionHeader& rHeader = context.m_sectionHeaders.m_data[i];
        hkArray<hkRelocationInfo::Global, hkContainerHeapAllocator>& rGlobals = context.m_relocsBySect.m_data[i]
                                                                                    .m_global;
        {
            // The SDK's hkArray<hkUint32> globalFixups(3 * globals, -1), inline after the empty array's construction.
            hkArray<uint32_t> globalFixups;
            int32_t iNumWords = 3 * rGlobals.m_size;
            globalFixups.reserve(iNumWords);
            if (iNumWords - globalFixups.m_size > 0) {
                hkString::memSet4(&globalFixups.m_data[globalFixups.m_size], -1, iNumWords - globalFixups.m_size);
            }
            globalFixups.m_size = iNumWords;
            int32_t iWord = 0;
            for (const hkRelocationInfo::Global& rGlobal : rGlobals) {
                int32_t iIndex = m_knownObjects.getWithDefault(rGlobal.m_toAddress, INDEX_ERROR);
                if (iIndex >= 0 && context.m_locations.m_data[iIndex].m_section >= 0) {
                    globalFixups.m_data[iWord] = rGlobal.m_fromOffset - rHeader.m_absoluteDataStart;
                    globalFixups.m_data[iWord + 1] = context.m_locations.m_data[iIndex].m_section;
                    globalFixups.m_data[iWord + 2] = context.m_locations.m_data[iIndex].m_offset;
                    iWord += 3;
                }
            }
            context.m_stream->seek(
                rHeader.m_absoluteDataStart + rHeader.m_globalFixupsOffset,
                hkStreamWriter::STREAM_SET
            );
            archive.writeArray32u(globalFixups.m_data, globalFixups.m_size);
        }

        hkArray<hkRelocationInfo::Finish, hkContainerHeapAllocator>& rFinishes = context.m_relocsBySect.m_data[i]
                                                                                     .m_finish;
        hkArray<uint32_t> virtualFixups;
        virtualFixups.setSize(3 * rFinishes.m_size);
        for (int32_t j = 0; j < rFinishes.m_size; ++j) {
            virtualFixups.m_data[3 * j] = rFinishes.m_data[j].m_fromOffset - rHeader.m_absoluteDataStart;
            virtualFixups.m_data[3 * j + 1] = 0;
            virtualFixups.m_data[3 * j + 2] = static_cast<uint32_t>(context.m_usedClassNames.m_map.getWithDefault(
                reinterpret_cast<hkUlong>(rFinishes.m_data[j].m_className),
                static_cast<hkUlong>(-1)
            ));
        }
        context.m_stream->seek(rHeader.m_absoluteDataStart + rHeader.m_virtualFixupsOffset, hkStreamWriter::STREAM_SET);
        archive.writeArray32u(virtualFixups.m_data, virtualFixups.m_size);
    }
    context.m_stream->seek(sectionHeadersStart, hkStreamWriter::STREAM_SET);
    for (const hkPackfileSectionHeader& rHeader : context.m_sectionHeaders) {
        hkRelocationInfo reloc;
        hkObjectWriter* pWriter = &context.m_writer;
        pWriter->writeObject(context.m_stream, &rHeader, hkPackfileSectionHeaderClass, reloc);
    }
    context.m_stream->seek(offsetof(hkPackfileHeader, m_contentsSectionIndex), hkStreamWriter::STREAM_SET);
    bool bSwap = context.m_writer.getLayout().m_rules.m_littleEndian !=
        hkStructureLayout::HostLayoutRules.m_littleEndian;
    hkOArchive headerArchive(context.m_stream, hkBool(bSwap));
    headerArchive.write32(context.m_locations.m_data[m_contentsPtrPWIndex].m_section);
    headerArchive.write32(context.m_locations.m_data[m_contentsPtrPWIndex].m_offset);
    const hkClass* pContentsClass = static_cast<const hkClass*>(
        m_pendingWrites.m_data[m_contentsClassPWIndex].m_pointer
    );
    headerArchive.write32(0);
    headerArchive.write32(
        static_cast<int32_t>(context.m_usedClassNames.m_map.getWithDefault(
            reinterpret_cast<hkUlong>(pContentsClass->getName()),
            static_cast<hkUlong>(-1)
        ))
    );
}

// The class's signature (byte-swapped for a target of the other endianness) and the tag byte 9, then the class's
// name with its terminator; the name's offset from absoluteDataStart is recorded in context's used class names.
void hkBinaryPackfileWriter::writeClassName(
    hkBinaryPackfileWriter::SaveContext& context,
    const hkClass& klass,
    int32_t absoluteDataStart
) {
    uint32_t uiSignature = klass.getSignature(0);
    if (context.m_endianSwapRequired.m_bool) {
        uiSignature = std::byteswap(uiSignature);
    }
    // The signature as a 32-bit little-endian value, then the tag.
    uint8_t aucHeader[5];
    aucHeader[0] = static_cast<uint8_t>(uiSignature);
    aucHeader[1] = static_cast<uint8_t>(uiSignature >> 8);
    aucHeader[2] = static_cast<uint8_t>(uiSignature >> 16);
    aucHeader[3] = static_cast<uint8_t>(uiSignature >> 24);
    aucHeader[4] = 9;
    context.m_stream->write(aucHeader, 5);
    const char* pName = klass.getName();
    int32_t iOffset = context.m_stream->tell() - absoluteDataStart;
    context.m_usedClassNames.m_map.insert(reinterpret_cast<hkUlong>(pName), static_cast<hkUlong>(iOffset));
    context.m_stream->write(pName, hkString::strLen(pName) + 1);
}

void hkRelocationInfo::addLocal(int32_t fromOffset, int32_t toOffset) {
    hkRelocationInfo::Local& rLocal = m_local.expandOne();
    rLocal.m_fromOffset = fromOffset;
    rLocal.m_toOffset = toOffset;
}
