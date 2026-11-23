#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Serialize/Resource/hkResource.h"

#include <stdint.h>

class hkPackfileSectionHeader;
class hkTypeInfoRegistry;

// Returns base + the section header's m_absoluteDataStart + offset, or null when its m_localFixupsOffset (getDataSize)
// is 0.
void* _getSectionDataByIndex(
    void* base,
    hkArray<hkPackfileSectionHeader, hkContainerHeapAllocator>& sections,
    int32_t sectionIndex,
    int32_t offset
);

// Loads binary packfiles built for this platform in place.
namespace hkNativePackfileUtils {
    // The bytes loading packfileData needs.
    int32_t getRequiredBufferSize(const void* packfileData, int32_t packfileSize);
    // Loads packfileData into outBuffer.
    void* load(
        const void* packfileData,
        int32_t dataSize,
        void* outBuffer,
        int32_t outBufferSize,
        const hkTypeInfoRegistry* userRegistry
    );
    // Loads packfileData into a buffer of its own and returns the resource.
    hkResource* load(const void* packfileData, int32_t dataSize, const hkTypeInfoRegistry* userRegistry);

    // Whether packfileData starts with a packfile header this build can load in place: 0
    // (HK_SUCCESS), else 1 with *errOut, when errOut is not null, set to the reason. hkResult is spelled int32_t.
    hkResult validatePackfileHeader(const void* packfileData, const char** errOut);
} // namespace hkNativePackfileUtils

namespace hkNativePackfileUtils {
    // Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
    void getImportsExports(
        const void* loadedBuffer,
        hkArray<hkResource::Export, hkContainerHeapAllocator>& expOut,
        hkArray<hkResource::Import, hkContainerHeapAllocator>& impOut
    );
} // namespace hkNativePackfileUtils

namespace hkNativePackfileUtils {
    // Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
    void unload(void* buffer, int32_t bufferSize);
} // namespace hkNativePackfileUtils
