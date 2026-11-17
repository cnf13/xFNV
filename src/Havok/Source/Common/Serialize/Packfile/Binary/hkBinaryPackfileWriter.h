#pragma once

#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Serialize/Packfile/hkPackfileWriter.h"
#include "Havok/Source/Common/Serialize/Serialize/Platform/hkPlatformObjectWriter.h"

#include <stdint.h>

class hkPackfileSectionHeader;
class hkRelocationInfo;
class hkStreamWriter;

// Writes the packfile header of a file with aiNumSections sections: the magic, the options' user tag and layout, the
// section count, the version string and the -1 pad.
void writePackfileHeader(
    hkStreamWriter* apStream,
    const hkPackfileWriter::Options& arOptions,
    hkPlatformObjectWriter* apWriter,
    int32_t aiNumSections
);
// The bytes one hkPackfileSectionHeader takes.
int32_t sectionTableSize(const hkArray<hkPackfileSectionHeader>& arSections);
// Pads the stream with 0xFF bytes up to the next multiple of r_eax (a power of two), through a 32-byte
// hkInplaceArray<char>.
void padUp(int32_t r_eax, hkStreamWriter* r_edi);

// Writes the packfile in Havok's binary format. No fields of its own beyond hkPackfileWriter's. Overrides the
// destructor and save; the other virtual functions are hkPackfileWriter's.
class hkBinaryPackfileWriter : public hkPackfileWriter {
public:
    // Where the objects were written in the section being filled: the section and the offset in it.
    struct Location {
        int32_t m_section;
        int32_t m_offset;
    };

    // What save and its helpers carry while writing: the options, the section headers, the object locations, the
    // stream, the platform object writer, the class names used so far, the relocations of each section and whether the
    // bytes are swapped.
    struct SaveContext {
        // The options, empty containers, the object writer over the options' layout, and whether the layout's byte
        // order differs from the host's. m_stream is deliberately left unset, as the game does.
        SaveContext(const Options& arOptions);

        const Options* m_options;
        hkArray<hkPackfileSectionHeader> m_sectionHeaders;
        hkArray<Location> m_locations;
        hkStreamWriter* m_stream;
        hkPlatformObjectWriter m_writer;
        hkStringMap<const char*> m_usedClassNames;
        hkArray<hkRelocationInfo> m_relocsBySect;
        hkBool m_endianSwapRequired;
    };

    // The data section and the class records' types section.
    hkBinaryPackfileWriter();
    ~hkBinaryPackfileWriter() override;

    hkResult save(hkStreamWriter* origStream, const Options& options) override;

protected:
    // Reserves one section header per known section tag and fills its tag.
    void fillSectionTags(SaveContext& context);
    // Writes every pending object and the class records.
    void writeAllObjects(SaveContext& context);
    // Writes each section's global and virtual fixups, the section headers from aiStart on and the file header's
    // contents fields.
    void doDeferredWrites(SaveContext& context, int32_t sectionHeadersStart);

public:
    // The class's signature, a tag byte and its name, recording the name's offset from a3.
    // Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
    void writeClassName(hkBinaryPackfileWriter::SaveContext& context, const hkClass& a2, int32_t a3);
};
