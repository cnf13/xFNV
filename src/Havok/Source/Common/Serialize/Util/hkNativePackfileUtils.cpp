#include "Havok/Source/Common/Serialize/Util/hkNativePackfileUtils.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkTypeInfoRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Packfile/Binary/hkPackfileHeader.h"
#include "Havok/Source/Common/Serialize/Packfile/Binary/hkPackfileSectionHeader.h"
#include "Havok/Source/Common/Serialize/Resource/hkResource.h"
#include "Havok/Source/Common/Serialize/Tagfile/Binary/hkBinaryTagfileWriter.h"
#include "Havok/Source/Common/Serialize/Util/hkBuiltinTypeRegistry.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"
#include "Havok/Source/Common/Serialize/Version/hkVersionUtil.h"

static int32_t getSizeInBytesFor(int32_t a1);

static int32_t getSizeInBytesFor(int32_t a1) {
    int32_t i;
    for (i = 8; i < (2 * a1); i *= 2) {
        ;
    }
    return 8 * i;
}

namespace {
    // The resource load builds (hkNativePackfileUtils.cxx's hkNativeResource): the buffer, the contents inside it and
    // the contents' type name.
    struct hkNativeResource : hkResource {
        // Destruct's work: callDestructors' steps; m_data frees its buffer in its own destructor.
        ~hkNativeResource() override;

        // "hkNativeResource".
        const char* getName() const override;
        // With contents, unloads the buffer (hkNativePackfileUtils::unload on m_data's data and size) and clears
        // m_contents and m_contentsTypeName.
        void callDestructors() override;
        // The buffer's imports and exports, read from m_data's data, the exports' array before the imports'.
        void getImportsExports(hkArray<hkResource::Import>& impOut, hkArray<hkResource::Export>& expOut) const override;
        // m_contents, whatever the type name and registry.
        void* getContentsPointer(const char* typeName, const hkTypeInfoRegistry* typeRegistry) const override;
        const char* getContentsTypeName() const override {
            return m_contentsTypeName;
        }

        hkArray<char> m_data;
        void* m_contents;
        const char* m_contentsTypeName;
    };

    hkNativeResource::~hkNativeResource() {
        if (m_contents) {
            hkNativePackfileUtils::unload(m_data.begin(), m_data.m_size);
            m_contents = nullptr;
            m_contentsTypeName = nullptr;
        }
    }

    const char* hkNativeResource::getName() const {
        return "hkNativeResource";
    }

    void hkNativeResource::callDestructors() {
        if (m_contents) {
            hkNativePackfileUtils::unload(m_data.begin(), m_data.m_size);
            m_contents = nullptr;
            m_contentsTypeName = nullptr;
        }
    }

    void hkNativeResource::getImportsExports(
        hkArray<hkResource::Import, hkContainerHeapAllocator>& impOut,
        hkArray<hkResource::Export, hkContainerHeapAllocator>& expOut
    ) const {
        hkNativePackfileUtils::getImportsExports(m_data.m_data, expOut, impOut);
    }

    void* hkNativeResource::getContentsPointer(const char* typeName, const hkTypeInfoRegistry* typeRegistry) const {
        return m_contents;
    }

    // The start of a loaded buffer (hkNativePackfileUtils.cxx's InplaceDataHeader): the magic, the contents' offset in
    // the buffer, then the map from each contents pointer to the address of its type name's slot. The section headers
    // follow the map.
    struct InplaceDataHeader {
        int32_t m_magic;
        int32_t m_contentsOffset;
        hkUlongMapBase m_contentsTypeNames;
        // The loaded sections' headers, their data offsets relative to the buffer.
        hkArray<hkPackfileSectionHeader> m_sections;
    };
} // namespace

namespace hkNativePackfileUtils {
    // The SDK's getRequiredBufferSize: the loaded buffer's header, the section headers, each section's data and its
    // export and import tables (from its local fixups' offset, which ends its data, and from its exports' offset to its
    // end), and the type name map sized for one object per virtual fixup (3 ints each). The section headers follow the
    // packfile header in the raw packfile data.
    int32_t getRequiredBufferSize(const void* packfileData, int32_t packfileSize) {
        const hkPackfileHeader* pHeader = static_cast<const hkPackfileHeader*>(packfileData);
        const hkPackfileSectionHeader* pSections =
            pHeader->m_numSections > 0 ? reinterpret_cast<const hkPackfileSectionHeader*>(pHeader + 1) : nullptr;
        const int32_t iVirtualFixupSize = 3 * sizeof(int32_t);
        int32_t iNumObjects = 0;
        int32_t iSectionsSize = 0;
        for (int32_t i = 0; i < pHeader->m_numSections; ++i) {
            const hkPackfileSectionHeader& rSection = pSections[i];
            iNumObjects += (rSection.m_exportsOffset - rSection.m_virtualFixupsOffset) / iVirtualFixupSize;
            iSectionsSize += rSection.m_localFixupsOffset + rSection.m_endOffset - rSection.m_exportsOffset;
        }
        const int32_t iMapSize = getSizeInBytesFor(iNumObjects);
        return static_cast<int32_t>(sizeof(InplaceDataHeader)) +
            pHeader->m_numSections * static_cast<int32_t>(sizeof(hkPackfileSectionHeader)) + iSectionsSize + iMapSize;
    }

    // The SDK's in-place load: outBuffer gets the InplaceDataHeader, the section headers (their offsets made relative
    // to the buffer, the fixup tables dropped) and each section's data with its export and import tables; the local
    // fixups are applied, then the global ones (null into a section without data), the type name map is laid over the
    // rest of the buffer and every virtual fixup's object is finished through userRegistry (the built-in registry's
    // when null), the type of each one finished recorded in the map. Returns the contents.
    void* load(
        const void* packfileData,
        int32_t dataSize,
        void* outBuffer,
        int32_t outBufferSize,
        const hkTypeInfoRegistry* userRegistry
    ) {
        // The packfile and outBuffer are treated as raw memory: the section headers follow each one's header, and the
        // fixup tables are int32_t runs inside the sections whose entries name pointer slots in the copied data.
        const hkPackfileHeader* pHeader = static_cast<const hkPackfileHeader*>(packfileData);
        const char* pPackfile = static_cast<const char*>(packfileData);
        const hkPackfileSectionHeader* pSrcSections =
            pHeader->m_numSections > 0 ? reinterpret_cast<const hkPackfileSectionHeader*>(pHeader + 1) : nullptr;
        char* pOut = static_cast<char*>(outBuffer);
        InplaceDataHeader* pData = static_cast<InplaceDataHeader*>(outBuffer);
        pData->m_magic = 0xd5109142;
        // EXE-INLINED none: hkArray<hkPackfileSectionHeader>'s constructor over an external buffer (placement new)
        hkArray<hkPackfileSectionHeader>* pSections = &pData->m_sections;
        if (pSections) {
            pSections->m_size = pHeader->m_numSections;
            pSections->m_data = reinterpret_cast<hkPackfileSectionHeader*>(pData + 1);
            pSections->m_capacityAndFlags =
                pHeader->m_numSections | hkArrayBase<hkPackfileSectionHeader>::DONT_DEALLOCATE_FLAG;
        }
        int32_t iOffset = pHeader->m_numSections * static_cast<int32_t>(sizeof(hkPackfileSectionHeader)) +
            static_cast<int32_t>(sizeof(InplaceDataHeader));
        for (int32_t i = 0; i < pHeader->m_numSections; ++i) {
            const hkPackfileSectionHeader& rSrc = pSrcSections[i];
            hkPackfileSectionHeader& rDst = pData->m_sections.m_data[i];
            hkString::memCpy(rDst.m_sectionTag, rSrc.m_sectionTag, sizeof(rSrc.m_sectionTag));
            rDst.m_nullByte = rSrc.m_nullByte;
            rDst.m_absoluteDataStart = iOffset;
            rDst.m_localFixupsOffset = rSrc.m_localFixupsOffset;
            rDst.m_globalFixupsOffset = rSrc.m_localFixupsOffset;
            rDst.m_virtualFixupsOffset = rSrc.m_localFixupsOffset;
            rDst.m_exportsOffset = rSrc.m_localFixupsOffset;
            rDst.m_importsOffset = rSrc.m_importsOffset - rSrc.m_exportsOffset + rSrc.m_localFixupsOffset;
            char* pDataDst = pOut + iOffset;
            rDst.m_endOffset = rSrc.m_endOffset - rSrc.m_exportsOffset + rSrc.m_localFixupsOffset;
            const char* pSrcData = pPackfile + rSrc.m_absoluteDataStart;
            hkString::memCpy(pDataDst, pSrcData, rSrc.m_localFixupsOffset);
            hkString::memCpy(
                pDataDst + rDst.m_exportsOffset, pSrcData + rSrc.m_exportsOffset,
                rSrc.m_importsOffset - rSrc.m_exportsOffset
            );
            hkString::memCpy(
                pDataDst + rDst.m_importsOffset, pSrcData + rSrc.m_importsOffset,
                rSrc.m_endOffset - rSrc.m_importsOffset
            );
            if (i == pHeader->m_contentsSectionIndex) {
                pData->m_contentsOffset = pHeader->m_contentsSectionOffset + iOffset;
            }
            // Each local fixup is a pair of offsets in the section: the pointer's slot and its target.
            const int32_t* pLocal = reinterpret_cast<const int32_t*>(pSrcData + rSrc.m_localFixupsOffset);
            for (int32_t j = 0;
                 j < (rSrc.m_globalFixupsOffset - rSrc.m_localFixupsOffset) / static_cast<int32_t>(sizeof(int32_t));
                 j += 2) {
                int32_t iSlot = pLocal[j];
                if (iSlot != -1) {
                    *reinterpret_cast<char**>(pDataDst + iSlot) = pDataDst + pLocal[j + 1];
                }
            }
            iOffset += rDst.m_endOffset;
        }
        // Each global fixup is the pointer's slot in its section, then the target's section and offset there.
        for (int32_t i = 0; i < pHeader->m_numSections; ++i) {
            const hkPackfileSectionHeader& rSrc = pSrcSections[i];
            char* pDataDst = pOut + pData->m_sections.m_data[i].m_absoluteDataStart;
            const int32_t* pGlobal =
                reinterpret_cast<const int32_t*>(pPackfile + rSrc.m_absoluteDataStart + rSrc.m_globalFixupsOffset);
            for (int32_t j = 0;
                 j < (rSrc.m_virtualFixupsOffset - rSrc.m_globalFixupsOffset) / static_cast<int32_t>(sizeof(int32_t));
                 j += 3) {
                if (pGlobal[j] != -1) {
                    const hkPackfileSectionHeader& rTarget = pData->m_sections.m_data[pGlobal[j + 1]];
                    char* pTarget =
                        rTarget.m_localFixupsOffset ? pOut + rTarget.m_absoluteDataStart + pGlobal[j + 2] : nullptr;
                    *reinterpret_cast<char**>(pDataDst + pGlobal[j]) = pTarget;
                }
            }
        }
        // EXE-INLINED none: hkPointerMap's constructor over an external buffer (placement new)
        hkUlongMapBase* pTypeNames = &pData->m_contentsTypeNames;
        if (pTypeNames) {
            pTypeNames->init(pOut + iOffset, outBufferSize - iOffset);
        }
        const hkTypeInfoRegistry* pRegistry = userRegistry;
        if (!pRegistry) {
            pRegistry = hkBuiltinTypeRegistry::s_instance->getTypeInfoRegistry();
        }
        // Each virtual fixup is the object's offset in its section, then the section and offset of its class name.
        for (int32_t i = 0; i < pHeader->m_numSections; ++i) {
            const hkPackfileSectionHeader& rSrc = pSrcSections[i];
            char* pDataDst = pOut + pData->m_sections.m_data[i].m_absoluteDataStart;
            const int32_t* pVirtual =
                reinterpret_cast<const int32_t*>(pPackfile + rSrc.m_absoluteDataStart + rSrc.m_virtualFixupsOffset);
            for (int32_t j = 0;
                 j < (rSrc.m_exportsOffset - rSrc.m_virtualFixupsOffset) / static_cast<int32_t>(sizeof(int32_t));
                 j += 3) {
                int32_t iObject = pVirtual[j];
                if (iObject != -1) {
                    const hkPackfileSectionHeader& rNames = pData->m_sections.m_data[pVirtual[j + 1]];
                    const char* pClassName =
                        rNames.m_localFixupsOffset ? pOut + rNames.m_absoluteDataStart + pVirtual[j + 2] : nullptr;
                    void* pObject = pDataDst + iObject;
                    const hkTypeInfo* pType = pRegistry->finishLoadedObject(pObject, pClassName);
                    if (pType) {
                        // The map holds pointer-sized keys and values (the SDK's hkPointerMap storage), as the
                        // resource load below reads it.
                        pData->m_contentsTypeNames.insert(
                            hkMemoryRouter::getInstance().heap(), reinterpret_cast<uintptr_t>(pObject),
                            reinterpret_cast<uintptr_t>(pType)
                        );
                    }
                }
            }
        }
        return pOut + pData->m_contentsOffset;
    }

    // Loads packfileData into a buffer of its own: the buffer grows to the size the packfile needs, the contents are
    // loaded into it, the contents' type name comes from the header's map, and the resource takes the buffer over.
    hkResource* load(const void* packfileData, int32_t dataSize, const hkTypeInfoRegistry* userRegistry) {
        FNVD_INLINED_LOCAL(hkArray<char>, data, std::nullopt, std::nullopt);
        // EXE-INLINED none: hkArray<char>::hkArray
        data.m_data = nullptr;
        data.m_size = 0;
        data.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
        int32_t requiredSize = getRequiredBufferSize(packfileData, dataSize);
        // Takes the heap before the capacity test, as the game does.
        hkMemoryAllocator& reserveHeap = hkMemoryRouter::getInstance().heap();
        int32_t capacity = static_cast<int32_t>(data.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
        if (capacity < requiredSize) {
            int32_t newCapacity = 2 * capacity;
            if (newCapacity <= requiredSize) {
                newCapacity = requiredSize;
            }
            hkArrayUtil::_reserve(&reserveHeap, &data, newCapacity, sizeof(char));
        }
        data.m_size = static_cast<uint32_t>(requiredSize);
        void* contents = load(packfileData, dataSize, data.m_data, requiredSize, userRegistry);
        InplaceDataHeader* header = reinterpret_cast<InplaceDataHeader*>(data.m_data);
        uintptr_t typeNameAddress =
            header->m_contentsTypeNames.getWithDefault(reinterpret_cast<uintptr_t>(contents), 0);
        const char* contentsTypeName = nullptr;
        if (typeNameAddress) {
            contentsTypeName = *reinterpret_cast<const char**>(typeNameAddress);
        }
        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        hkNativeResource* resource = static_cast<hkNativeResource*>(heap.blockAlloc(sizeof(hkNativeResource)));
        resource->m_memSizeAndFlags = sizeof(hkNativeResource);
        ::new (resource) hkNativeResource;
        resource->m_contents = contents;
        resource->m_contentsTypeName = contentsTypeName;
        resource->m_data.swap(data);
        hkMemoryAllocator& freeHeap = hkMemoryRouter::getInstance().heap();
        data.m_size = 0;
        if (data.m_capacityAndFlags >= 0) {
            freeHeap.bufFree(
                data.m_data, static_cast<int32_t>(data.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK)
            );
        }
        return resource;
    }
} // namespace hkNativePackfileUtils

void* _getSectionDataByIndex(
    void* base,
    hkArray<hkPackfileSectionHeader, hkContainerHeapAllocator>& sections,
    int32_t sectionIndex,
    int32_t offset
) {
    hkPackfileSectionHeader* v4 = &sections.m_data[sectionIndex];
    if (v4->m_localFixupsOffset) {
        return static_cast<char*>(base) + (v4->m_absoluteDataStart + offset);
    }

    return nullptr;
}

// A header this build can load in place has the packfile magic (compared against a default-constructed header), the
// host's layout rules, a 4-aligned address and the current contents version; otherwise errOut, when given, gets the
// reason.
hkResult hkNativePackfileUtils::validatePackfileHeader(const void* packfileData, const char** errOut) {
    if (!packfileData) {
        if (errOut) {
            *errOut = "Pointer is null";
        }
        return HK_FAILURE;
    }
    // EXE-INLINED none: hkPackfileHeader::hkPackfileHeader
    hkPackfileHeader magic;
    hkString::memSet(&magic, -1, sizeof(hkPackfileHeader));
    magic.m_magic[0] = 0x57E0E057;
    magic.m_magic[1] = 0x10C0C010;
    magic.m_contentsVersion[0] = 0;
    const hkPackfileHeader* pHeader = static_cast<const hkPackfileHeader*>(packfileData);
    if (pHeader->m_magic[0] != magic.m_magic[0] || pHeader->m_magic[1] != magic.m_magic[1]) {
        if (errOut) {
            *errOut = "Missing packfile magic header. Is this from a binary file?";
        }
        return HK_FAILURE;
    }
    if (pHeader->m_layoutRules[0] != hkStructureLayout::HostLayoutRules.m_bytesInPointer) {
        if (errOut) {
            *errOut = "Trying to process a binary file with a different pointer size than this platform.";
        }
        return HK_FAILURE;
    }
    if (pHeader->m_layoutRules[1] != hkStructureLayout::HostLayoutRules.m_littleEndian) {
        if (errOut) {
            *errOut = "Trying to process a binary file with a different endian than this platform.";
        }
        return HK_FAILURE;
    }
    if (pHeader->m_layoutRules[2] != hkStructureLayout::HostLayoutRules.m_reusePaddingOptimization) {
        if (errOut) {
            *errOut = "Trying to process a binary file with a different padding optimization than this platform.";
        }
        return HK_FAILURE;
    }
    if (pHeader->m_layoutRules[3] != hkStructureLayout::HostLayoutRules.m_emptyBaseClassOptimization) {
        if (errOut) {
            *errOut = "Trying to process a binary file with a different empty base class optimization than this "
                      "platform.";
        }
        return HK_FAILURE;
    }
    // The address is read back through a volatile: clang would take the header's dereferences as proof of its
    // alignment and fold the test away.
    volatile uintptr_t uiAddress = reinterpret_cast<uintptr_t>(packfileData);
    if ((uiAddress & 3) != 0) {
        if (errOut) {
            *errOut = "Packfile data source needs to be 4 byte aligned";
        }
        return HK_FAILURE;
    }
    if (static_cast<uint8_t>(pHeader->m_contentsVersion[0]) == 0xFF) {
        if (errOut) {
            *errOut = "Packfile file format is too old";
        }
        return HK_FAILURE;
    }
    if (hkString::strCmp(pHeader->m_contentsVersion, hkVersionUtil::getCurrentVersion()) != 0) {
        if (errOut) {
            *errOut = "Packfile contents are not up to date";
        }
        return HK_FAILURE;
    }
    return HK_SUCCESS;
}

// The SDK's getImportsExports: each loaded section's exports and imports, read from the section's data in the
// buffer.
void hkNativePackfileUtils::getImportsExports(
    const void* loadedBuffer,
    hkArray<hkResource::Export, hkContainerHeapAllocator>& expOut,
    hkArray<hkResource::Import, hkContainerHeapAllocator>& impOut
) {
    const InplaceDataHeader* pHeader = static_cast<const InplaceDataHeader*>(loadedBuffer);
    for (int32_t i = 0; i < pHeader->m_sections.m_size; ++i) {
        const hkPackfileSectionHeader& rSection = pHeader->m_sections.m_data[i];
        const void* pSectionBegin = static_cast<const char*>(loadedBuffer) + rSection.m_absoluteDataStart;
        rSection.getExports(pSectionBegin, expOut);
        rSection.getImports(pSectionBegin, impOut);
    }
}

// Unload: the magic is cleared, each loaded object's type info cleanup function is run on it (iterating the map's slots
// whose key is not -1), then the section array and the map are freed. The map holds the object and type info addresses
// as integers.
void hkNativePackfileUtils::unload(void* buffer, int32_t bufferSize) {
    InplaceDataHeader* pHeader = static_cast<InplaceDataHeader*>(buffer);
    hkUlongMapBase& rMap = pHeader->m_contentsTypeNames;
    pHeader->m_magic = 0;
    // EXE-INLINED none: hkMapBase<hkUlong, hkUlong>::getIterator
    int32_t it = 0;
    while (it <= rMap.m_hashMod && rMap.m_elem[it].key == static_cast<uintptr_t>(-1)) {
        ++it;
    }
    while (it <= rMap.m_hashMod) {
        const hkTypeInfo* pTypeInfo = reinterpret_cast<const hkTypeInfo*>(static_cast<uintptr_t>(rMap.m_elem[it].val));
        if (pTypeInfo->m_cleanupLoadedObjectFunction) {
            pTypeInfo->m_cleanupLoadedObjectFunction(
                reinterpret_cast<void*>(static_cast<uintptr_t>(rMap.m_elem[it].key))
            );
        }
        // EXE-INLINED none: hkMapBase<hkUlong, hkUlong>::getNext
        ++it;
        while (it <= rMap.m_hashMod && rMap.m_elem[it].key == static_cast<uintptr_t>(-1)) {
            ++it;
        }
    }
    // EXE-INLINED none: hkArray<hkPackfileSectionHeader>::~hkArray
    hkMemoryAllocator& rHeap = *hkMemoryRouter::getInstance().m_heap;
    const int32_t iCap = pHeader->m_sections.m_capacityAndFlags;
    pHeader->m_sections.m_size = 0;
    if (iCap >= 0) {
        rHeap.bufFree(
            pHeader->m_sections.m_data,
            (iCap & hkArrayBase<hkPackfileSectionHeader>::CAPACITY_MASK) * sizeof(hkPackfileSectionHeader)
        );
    }
    pHeader->m_sections.m_data = nullptr;
    pHeader->m_sections.m_capacityAndFlags = hkArrayBase<hkPackfileSectionHeader>::DONT_DEALLOCATE_FLAG;
    rMap.clearAndDeallocate(*hkMemoryRouter::getInstance().m_heap);
}

hkResource::~hkResource() {}
