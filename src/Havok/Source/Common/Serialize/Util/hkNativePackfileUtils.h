#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Serialize/Resource/hkResource.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkPackfileSectionHeader;
class hkTypeInfoRegistry;

// The PDB's file-local _getSectionDataByIndex of this file: base + the section header's
// dword at +0x14 (the SDK's m_absoluteDataStart) + offset, or null when its dword at +0x18 (m_localFixupsOffset,
// the SDK's getDataSize) is 0. Its arguments come in registers and on the stack (ecx sections, eax sectionIndex,
// then base and offset pushed; ret 0, 0xDC07B0), so it is reached through usercall. No exe call, jump or pointer
// reaches it.
void* _getSectionDataByIndex(void* base, hkArray<hkPackfileSectionHeader, hkContainerHeapAllocator>& sections,
	int32_t sectionIndex, int32_t offset);

// Loads binary packfiles built for this platform in place (the SDK's and PDB's namespace).
namespace hkNativePackfileUtils {
// The bytes loading packfileData needs.
int32_t getRequiredBufferSize(const void* packfileData, int32_t packfileSize);
// Loads packfileData into outBuffer.
void* load(const void* packfileData, int32_t dataSize, void* outBuffer, int32_t outBufferSize,
	const hkTypeInfoRegistry* userRegistry);
// Loads packfileData into a buffer of its own and returns the resource.
hkResource* load(const void* packfileData, int32_t dataSize, const hkTypeInfoRegistry* userRegistry);

// Whether packfileData starts with a packfile header this build can load in place: 0
// (HK_SUCCESS), else 1 with *errOut, when errOut is not null, set to the reason. hkResult is spelled int32_t.
hkResult validatePackfileHeader(const void* packfileData, const char** errOut);
} // namespace hkNativePackfileUtils

namespace hkNativePackfileUtils {
// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
void getImportsExports(const void* loadedBuffer, hkArray<hkResource::Export, hkContainerHeapAllocator>& expOut,
	hkArray<hkResource::Import, hkContainerHeapAllocator>& impOut);
}  // namespace hkNativePackfileUtils

namespace hkNativePackfileUtils {
// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
void unload(void* buffer, int32_t bufferSize);
}  // namespace hkNativePackfileUtils
