#include "Havok/Source/Common/Serialize/Packfile/Binary/hkBinaryPackfileWriter.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Common/Serialize/Packfile/Binary/hkPackfileHeader.h"
#include "Havok/Source/Common/Serialize/Packfile/hkPackfileWriter.h"
#include "Havok/Source/Common/Serialize/Serialize/Platform/hkPlatformObjectWriter.h"
#include "Havok/Source/Common/Serialize/Serialize/hkRelocationInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkCachedHashMap.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/System/Io/OArchive/hkOArchive.h"
#include "Havok/Source/Common/Base/System/Io/Writer/SubStream/hkSubStreamWriter.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkCustomAttributes.h"
#include "Havok/Source/Common/Serialize/Packfile/Binary/hkPackfileSectionHeader.h"
#include "Havok/Source/Common/Serialize/Serialize/hkObjectWriter.h"
#include "SDK/MSVC/string.h"

// The exe takes the options in eax and the writer in edi, and the stream and the section count
// on the stack.
void writePackfileHeader(hkStreamWriter* apStream, const hkPackfileWriter::Options& arOptions,
	hkPlatformObjectWriter* apWriter, int32_t aiNumSections) {
	hkRelocationInfo v8;
	uint32_t v9[4];
	uint8_t apDest[4];
	char apcDest[16];
	uint8_t v13[8];
	hkString::memSet(v9, -1, 64);
	uint32_t m_userTag = arOptions.m_userTag;
	v9[0] = 0x57E0E057;
	v9[1] = 0x10C0C010;
	apcDest[0] = 0;
	v9[2] = m_userTag;
	v9[3] = 7;
	hkString::memCpy(apDest, &arOptions.m_layout, 4);
	hkString::memSet(v13, -1, 8);
	const char* m_contentsVersion = arOptions.m_contentsVersion;
	if (m_contentsVersion) {
		hkString::strNcpy(apcDest, m_contentsVersion, 16);
	} else {
		hkPackfileWriter::getCurrentVersion(apcDest, 16);
	}
	v8.m_local.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Local>::DONT_DEALLOCATE_FLAG;
	v8.m_global.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Global>::DONT_DEALLOCATE_FLAG;
	v8.m_finish.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Finish>::DONT_DEALLOCATE_FLAG;
	v8.m_imports.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Import>::DONT_DEALLOCATE_FLAG;
	v8.m_local.m_data = 0;
	v8.m_local.m_size = 0;
	v8.m_global.m_data = 0;
	v8.m_global.m_size = 0;
	v8.m_finish.m_data = 0;
	v8.m_finish.m_size = 0;
	v8.m_imports.m_data = 0;
	v8.m_imports.m_size = 0;
	v8.m_pool = 0;
	apWriter->writeObject(apStream, v9, hkPackfileHeaderClass, v8);
}

// The exe multiplies the header count by the header size instead of reading the table's bytes.
int32_t sectionTableSize(const hkArray<hkPackfileSectionHeader, hkContainerHeapAllocator>& arSections) {
	return 48 * arSections.m_size;
}

void padUp(int32_t r_eax, hkStreamWriter* r_edi) {
	using CharInplaceArray32 = hkInplaceArray<char, 32>;
	int32_t v3 = r_edi->tell();
	// hkInplaceArray<char, 32>'s inlined constructor and setSize(r_eax, -1).
	FNVD_INLINED_LOCAL(CharInplaceArray32, buf, std::nullopt, std::nullopt);
	buf.m_data = buf.m_storage;
	buf.m_size = 0;
	buf.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<char>::DONT_DEALLOCATE_FLAG | 32);
	hkMemoryAllocator* m_heap = hkMemoryRouter::getInstance().m_heap;
	int32_t v4 = buf.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK;
	if (v4 < r_eax) {
		int32_t v5 = 2 * v4;
		if (r_eax >= v5) {
			v5 = r_eax;
		}
		hkArrayUtil::_reserve(*m_heap, &buf, v5, sizeof(char));
	}
	int32_t v9 = r_eax - buf.m_size;
	char* v8 = buf.m_data + buf.m_size;
	if (v9 > 0) {
		std::memset(v8, -1, v9);
	}
	buf.m_size = r_eax;
	if ((v3 & r_eax - 1) != 0) {
		r_edi->write(buf.m_data, r_eax - (v3 & r_eax - 1));
	}
	// The array's inlined destructor.
	hkMemoryAllocator* v6 = hkMemoryRouter::getInstance().m_heap;
	int32_t v10 = buf.m_capacityAndFlags;
	buf.m_size = 0;
	if (v10 >= 0) {
		v6->bufFree(buf.m_data, v10 & hkArrayBase<char>::CAPACITY_MASK);
	}
}

hkBinaryPackfileWriter::SaveContext::SaveContext(const Options& arOptions)
	: m_options(&arOptions), m_writer(arOptions.m_layout),
	  m_endianSwapRequired(hkStructureLayout::HostLayoutRules.m_littleEndian !=
		  arOptions.m_layout.m_rules.m_littleEndian) {}

hkBinaryPackfileWriter::hkBinaryPackfileWriter() {
	hkPackfileWriter::addSection(sectionTag);
	hkPackfileWriter::setSectionForClass(hkClassClass, hkPackfileWriter::SECTION_TAG_TYPES);
	hkPackfileWriter::setSectionForClass(hkClassMemberClass, hkPackfileWriter::SECTION_TAG_TYPES);
	hkPackfileWriter::setSectionForClass(hkClassEnumClass, hkPackfileWriter::SECTION_TAG_TYPES);
	hkPackfileWriter::setSectionForClass(hkClassEnumItemClass, hkPackfileWriter::SECTION_TAG_TYPES);
}

// The exe's destructor (0xDBC6E0) is the base's alone.
hkBinaryPackfileWriter::~hkBinaryPackfileWriter() {}

// The exe's hkBinaryPackfileWriter::save (0xDBD620): the context and the sub stream writer over the caller's stream,
// then the section table, the packfile header, the objects and the deferred writes, and the seeks that put the body
// where the header reserved room for it. The context writes through its m_stream, the sub stream writer.
hkResult hkBinaryPackfileWriter::save(hkStreamWriter* origStream, const Options& options) {
	FNVD_CONSTRUCTED_LOCAL(SaveContext, context, &SaveContext::Destruct, options);
	FNVD_CONSTRUCTED_LOCAL(hkSubStreamWriter, subStream, &hkSubStreamWriter::Destruct, origStream);
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
	hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t v6 = context.m_sectionHeaders.m_capacityAndFlags & hkArrayBase<hkPackfileSectionHeader>::CAPACITY_MASK;
	if (v6 < m_size) {
		int32_t v7 = 2 * v6;
		if (m_size >= v7) {
			v7 = m_size;
		}
		hkArrayUtil::_reserve(*m_heap, p_m_sectionHeaders, v7, sizeof(hkPackfileSectionHeader));
	}
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
		} while (arContexta);
	}
	p_m_sectionHeaders->m_size = m_size;
	int32_t v12 = sectionTableSize(*p_m_sectionHeaders);
	hkString::memSet(p_m_sectionHeaders->begin(), -1, v12);
	int32_t v13 = 0;
	if (m_knownSections.m_size > 0) {
		int32_t v14 = 0;
		do {
			hkString::strNcpy(p_m_sectionHeaders->m_data[v14++].m_sectionTag, m_knownSections.m_data[v13++], 19);
		} while (v13 < m_knownSections.m_size);
	}
}

// The SDK's writeAllObjects: the types section (section 0) gets the class records of the four reflection classes and
// of every pending object's class, once each; then each other section gets its objects, written raw or through the
// platform writer, and after them its local fixups, global and virtual fixup space (written later by
// doDeferredWrites), exports and imports, each block padded. A global fixup whose target is an import becomes one.
// The exe inlines the locations' and relocation arrays' setSize, the relocation info's constructor and the map
// iterations; the class name map is cleared and freed, never destroyed.
void hkBinaryPackfileWriter::writeAllObjects(hkBinaryPackfileWriter::SaveContext& context) {
	hkPackfileSectionHeader* pTypesHeader = context.m_sectionHeaders.m_data;
	pTypesHeader->m_absoluteDataStart = context.m_stream->tell();
	using ClassNameMap_t = hkCachedHashMap<hkStringMapOperations, hkContainerHeapAllocator>;
	FNVD_INLINED_LOCAL(ClassNameMap_t, doneClasses, std::nullopt, std::nullopt);
	// The exe passes the empty Operations object in a stack dword whose low byte it clears.
	::new (&doneClasses) ClassNameMap_t(0u);
	const hkClass* const apClasses[4] = {&hkClassClass, &hkClassMemberClass, &hkClassEnumClass, &hkClassEnumItemClass};
	for (int32_t i = 0; i < 4; ++i) {
		// The map's keys are hkUlong words holding the name pointers.
		doneClasses.insert(reinterpret_cast<hkUlong>(apClasses[i]->getName()), 1);
		writeClassName(context, *apClasses[i], pTypesHeader->m_absoluteDataStart);
	}
	for (int32_t i = 0; i < m_pendingWrites.m_size; ++i) {
		hkBool bDone;
		doneClasses.isValid(&bDone,
			doneClasses.findKey(reinterpret_cast<hkUlong>(m_pendingWrites.m_data[i].m_klass->getName())));
		if (!bDone.m_bool) {
			doneClasses.insert(reinterpret_cast<hkUlong>(m_pendingWrites.m_data[i].m_klass->getName()), 1);
			writeClassName(context, *m_pendingWrites.m_data[i].m_klass, pTypesHeader->m_absoluteDataStart);
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
	doneClasses.clearAndDeallocate();

	// EXE-INLINED none: hkArray<hkBinaryPackfileWriter::Location>::setSize
	const int32_t iNumWrites = m_pendingWrites.m_size;
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	const int32_t iLocCap = context.m_locations.m_capacityAndFlags & hkArrayBase<Location>::CAPACITY_MASK;
	if (iLocCap < iNumWrites) {
		int32_t iReq = 2 * iLocCap;
		if (iNumWrites >= iReq) {
			iReq = iNumWrites;
		}
		hkArrayUtil::_reserve(*pHeap, &context.m_locations, iReq, sizeof(Location));
	}
	context.m_locations.m_size = iNumWrites;

	// EXE-INLINED none: hkArray<hkRelocationInfo>::setSize, with the exe's out-of-line destructor for the elements
	// dropped and the constructor (hkRelocationInfo::Construct, 0xDBD870) inlined for the new ones.
	const int32_t iNumSections = context.m_sectionHeaders.m_size;
	hkMemoryAllocator* pHeap2 = hkMemoryRouter::getInstance().m_heap;
	const int32_t iRelocCap = context.m_relocsBySect.m_capacityAndFlags & hkArrayBase<hkRelocationInfo>::CAPACITY_MASK;
	if (iRelocCap < iNumSections) {
		int32_t iReq = 2 * iRelocCap;
		if (iNumSections >= iReq) {
			iReq = iNumSections;
		}
		hkArrayUtil::_reserve(*pHeap2, &context.m_relocsBySect, iReq, sizeof(hkRelocationInfo));
	}
	hkRelocationInfo* pDropped = context.m_relocsBySect.m_data + iNumSections;
	for (int32_t iNum = context.m_relocsBySect.m_size - iNumSections; iNum > 0; --iNum) {
		pDropped->~hkRelocationInfo();
		++pDropped;
	}
	hkRelocationInfo* pNew = context.m_relocsBySect.m_data + context.m_relocsBySect.m_size;
	for (int32_t iNum = iNumSections - context.m_relocsBySect.m_size; iNum > 0; --iNum, ++pNew) {
		// The placement new's null test kept.
		if (pNew) {
			pNew->m_local.m_data = nullptr;
			pNew->m_local.m_size = 0;
			pNew->m_local.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Local>::DONT_DEALLOCATE_FLAG;
			pNew->m_global.m_data = nullptr;
			pNew->m_global.m_size = 0;
			pNew->m_global.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Global>::DONT_DEALLOCATE_FLAG;
			pNew->m_finish.m_data = nullptr;
			pNew->m_finish.m_size = 0;
			pNew->m_finish.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Finish>::DONT_DEALLOCATE_FLAG;
			pNew->m_imports.m_data = nullptr;
			pNew->m_imports.m_size = 0;
			pNew->m_imports.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Import>::DONT_DEALLOCATE_FLAG;
			pNew->m_pool = nullptr;
		}
	}
	context.m_relocsBySect.m_size = iNumSections;

	const int32_t iTypesSection = static_cast<int32_t>(sectionTagToIndex(hkPackfileWriter::SECTION_TAG_TYPES));
	for (int32_t iSection = 1; iSection < context.m_sectionHeaders.m_size; ++iSection) {
		hkPackfileSectionHeader* pHeader = context.m_sectionHeaders.m_data + iSection;
		pHeader->m_absoluteDataStart = context.m_stream->tell();
		// EXE-INLINED 0xDBD870: hkRelocationInfo::hkRelocationInfo
		FNVD_INLINED_LOCAL(hkRelocationInfo, relocs, std::nullopt, std::nullopt);
		relocs.m_local.m_data = nullptr;
		relocs.m_local.m_size = 0;
		relocs.m_local.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Local>::DONT_DEALLOCATE_FLAG;
		relocs.m_global.m_data = nullptr;
		relocs.m_global.m_size = 0;
		relocs.m_global.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Global>::DONT_DEALLOCATE_FLAG;
		relocs.m_finish.m_data = nullptr;
		relocs.m_finish.m_size = 0;
		relocs.m_finish.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Finish>::DONT_DEALLOCATE_FLAG;
		relocs.m_imports.m_data = nullptr;
		relocs.m_imports.m_size = 0;
		relocs.m_imports.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Import>::DONT_DEALLOCATE_FLAG;
		relocs.m_pool = nullptr;

		if (!context.m_options->m_writeMetaInfo.m_bool && iSection == iTypesSection) {
			// The class records are not written: their objects get no location.
			for (int32_t i = 0; i < m_pendingWrites.m_size; ++i) {
				if (m_pendingWrites.m_data[i].m_sectionIndex == iTypesSection) {
					context.m_locations.m_data[i].m_section = -1;
					context.m_locations.m_data[i].m_offset = -1;
				}
			}
		} else {
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
					// Through the base: the exe calls the writer's slot.
					hkObjectWriter* pWriter = &context.m_writer;
					pWriter->writeObject(context.m_stream, pObject, *pKlass, relocs);
				} else {
					context.m_stream->write(rWrite.m_pointer, rWrite.m_dataSize);
				}
			}
		}

		// The local fixups become offsets from the section's start.
		for (int32_t j = 0; j < relocs.m_local.m_size; ++j) {
			relocs.m_local.m_data[j].m_fromOffset -= pHeader->m_absoluteDataStart;
			relocs.m_local.m_data[j].m_toOffset -= pHeader->m_absoluteDataStart;
		}
		// A global fixup whose target is an import becomes an import, its fixup removed (the last one moved in).
		for (int32_t g = relocs.m_global.m_size - 1; g >= 0; --g) {
			const char* pImport = m_imports.getWithDefault(relocs.m_global.m_data[g].m_toAddress, nullptr);
			if (!pImport) {
				continue;
			}
			const hkRelocationInfo::Global& rGlobal = relocs.m_global.m_data[g];
			// EXE-INLINED none: hkArray<hkRelocationInfo::Import>::expandOne
			hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
			if (relocs.m_imports.m_size ==
				(relocs.m_imports.m_capacityAndFlags & hkArrayBase<hkRelocationInfo::Import>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*rRouter.m_heap, &relocs.m_imports, sizeof(hkRelocationInfo::Import));
			}
			hkRelocationInfo::Import& rImport = relocs.m_imports.m_data[relocs.m_imports.m_size++];
			const int32_t iFrom = rGlobal.m_fromOffset - pHeader->m_absoluteDataStart;
			rImport.m_identifier = pImport;
			rImport.m_fromOffset = iFrom;
			// EXE-INLINED none: hkArray<hkRelocationInfo::Global>::removeAt
			const int32_t iLast = --relocs.m_global.m_size;
			if (iLast != g) {
				relocs.m_global.m_data[g] = relocs.m_global.m_data[iLast];
			}
		}

		{
			FNVD_CONSTRUCTED_LOCAL(hkOArchive, oa, &hkOArchive::Destruct, context.m_stream,
				context.m_endianSwapRequired);
			pHeader->m_localFixupsOffset = context.m_stream->tell() - pHeader->m_absoluteDataStart;
			// Each local fixup is two words.
			oa.writeArrayGeneric(relocs.m_local.m_data, sizeof(int32_t), 2 * relocs.m_local.m_size);
			padUp(16, context.m_stream);
			// The global and virtual fixups (three words each) are written by doDeferredWrites: room is left.
			pHeader->m_globalFixupsOffset = context.m_stream->tell() - pHeader->m_absoluteDataStart;
			context.m_stream->seek(3 * static_cast<int32_t>(sizeof(int32_t)) * relocs.m_global.m_size,
				hkStreamWriter::STREAM_CUR);
			context.m_relocsBySect.m_data[iSection].m_global.swap(relocs.m_global);
			padUp(16, context.m_stream);
			pHeader->m_virtualFixupsOffset = context.m_stream->tell() - pHeader->m_absoluteDataStart;
			context.m_stream->seek(3 * static_cast<int32_t>(sizeof(int32_t)) * relocs.m_finish.m_size,
				hkStreamWriter::STREAM_CUR);
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
				} while (it <= m_exports.m_map.m_hashMod);
				if (bAnyExports.m_bool) {
					oa.write32(-1);
					padUp(16, context.m_stream);
				}
			}

			pHeader->m_importsOffset = context.m_stream->tell() - pHeader->m_absoluteDataStart;
			for (int32_t k = 0; k < relocs.m_imports.m_size; ++k) {
				const hkRelocationInfo::Import& rImport = relocs.m_imports.m_data[k];
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
		relocs.~hkRelocationInfo();
	}
}

// The SDK's doDeferredWrites: for each section its global fixups (each from offset whose target is written, with the
// target's section and offset) and its virtual fixups (each from offset with 0 and its class name's offset), at
// their offsets in the section's data; then the section headers from aiStart on, and the file header's contents
// location and contents class name.
void hkBinaryPackfileWriter::doDeferredWrites(hkBinaryPackfileWriter::SaveContext& context,
	int32_t sectionHeadersStart) {
	FNVD_CONSTRUCTED_LOCAL(hkOArchive, archive, &hkOArchive::Destruct, context.m_stream, context.m_endianSwapRequired);
	for (int32_t i = 0; i < context.m_sectionHeaders.m_size; ++i) {
		hkPackfileSectionHeader& rHeader = context.m_sectionHeaders.m_data[i];
		hkArray<hkRelocationInfo::Global, hkContainerHeapAllocator>& rGlobals =
			context.m_relocsBySect.m_data[i].m_global;
		// The SDK's hkArray<hkUint32> globalFixups(3 * globals, -1), inline.
		FNVD_INLINED_LOCAL(hkArrayBase<uint32_t>, globalFixups, std::nullopt, std::nullopt);
		globalFixups.m_data = nullptr;
		globalFixups.m_size = 0;
		globalFixups.m_capacityAndFlags = hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG;
		int32_t iNumWords = 3 * rGlobals.m_size;
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		int32_t iCapacity = globalFixups.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK;
		if (iCapacity < iNumWords) {
			int32_t iNewCapacity = 2 * iCapacity;
			if (iNumWords >= iNewCapacity) {
				iNewCapacity = iNumWords;
			}
			hkArrayUtil::_reserve(*pHeap, &globalFixups, iNewCapacity, sizeof(uint32_t));
		}
		if (iNumWords - globalFixups.m_size > 0) {
			hkString::memSet4(&globalFixups.m_data[globalFixups.m_size], -1, iNumWords - globalFixups.m_size);
		}
		globalFixups.m_size = iNumWords;
		int32_t iWord = 0;
		for (int32_t j = 0; j < rGlobals.m_size; ++j) {
			int32_t iIndex = m_knownObjects.getWithDefault(rGlobals.m_data[j].m_toAddress, INDEX_ERROR);
			if (iIndex >= 0 && context.m_locations.m_data[iIndex].m_section >= 0) {
				globalFixups.m_data[iWord] = rGlobals.m_data[j].m_fromOffset - rHeader.m_absoluteDataStart;
				globalFixups.m_data[iWord + 1] = context.m_locations.m_data[iIndex].m_section;
				globalFixups.m_data[iWord + 2] = context.m_locations.m_data[iIndex].m_offset;
				iWord += 3;
			}
		}
		context.m_stream->seek(rHeader.m_absoluteDataStart + rHeader.m_globalFixupsOffset, hkStreamWriter::STREAM_SET);
		archive.writeArray32u(globalFixups.m_data, globalFixups.m_size);
		// EXE-INLINED none: hkArray::~hkArray
		pHeap = hkMemoryRouter::getInstance().m_heap;
		globalFixups.m_size = 0;
		if (globalFixups.m_capacityAndFlags >= 0) {
			pHeap->bufFree(globalFixups.m_data,
				sizeof(uint32_t) * (globalFixups.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK));
		}
		globalFixups.m_data = nullptr;
		globalFixups.m_capacityAndFlags = hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG;

		hkArray<hkRelocationInfo::Finish, hkContainerHeapAllocator>& rFinishes =
			context.m_relocsBySect.m_data[i].m_finish;
		// The SDK's hkArray<hkUint32> virtualFixups; virtualFixups.setSize(3 * finishes), inline.
		FNVD_INLINED_LOCAL(hkArrayBase<uint32_t>, virtualFixups, std::nullopt, std::nullopt);
		virtualFixups.m_data = nullptr;
		virtualFixups.m_size = 0;
		virtualFixups.m_capacityAndFlags = hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG;
		iNumWords = 3 * rFinishes.m_size;
		pHeap = hkMemoryRouter::getInstance().m_heap;
		iCapacity = virtualFixups.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK;
		if (iCapacity < iNumWords) {
			int32_t iNewCapacity = 2 * iCapacity;
			if (iNumWords >= iNewCapacity) {
				iNewCapacity = iNumWords;
			}
			hkArrayUtil::_reserve(*pHeap, &virtualFixups, iNewCapacity, sizeof(uint32_t));
		}
		virtualFixups.m_size = iNumWords;
		for (int32_t j = 0; j < rFinishes.m_size; ++j) {
			virtualFixups.m_data[3 * j] = rFinishes.m_data[j].m_fromOffset - rHeader.m_absoluteDataStart;
			virtualFixups.m_data[3 * j + 1] = 0;
			virtualFixups.m_data[3 * j + 2] = static_cast<uint32_t>(context.m_usedClassNames.m_map
					.getWithDefault(reinterpret_cast<hkUlong>(rFinishes.m_data[j].m_className),
						static_cast<hkUlong>(-1)));
		}
		context.m_stream->seek(rHeader.m_absoluteDataStart + rHeader.m_virtualFixupsOffset, hkStreamWriter::STREAM_SET);
		archive.writeArray32u(virtualFixups.m_data, virtualFixups.m_size);
		// EXE-INLINED none: hkArray::~hkArray
		pHeap = hkMemoryRouter::getInstance().m_heap;
		virtualFixups.m_size = 0;
		if (virtualFixups.m_capacityAndFlags >= 0) {
			pHeap->bufFree(virtualFixups.m_data,
				sizeof(uint32_t) * (virtualFixups.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK));
		}
		virtualFixups.m_data = nullptr;
		virtualFixups.m_capacityAndFlags = hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG;
	}
	context.m_stream->seek(sectionHeadersStart, hkStreamWriter::STREAM_SET);
	for (int32_t i = 0; i < context.m_sectionHeaders.m_size; ++i) {
		FNVD_INLINED_LOCAL(hkRelocationInfo, reloc, std::nullopt, std::nullopt);
		// EXE-INLINED none: hkRelocationInfo::hkRelocationInfo
		reloc.m_local.m_data = nullptr;
		reloc.m_local.m_size = 0;
		reloc.m_local.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Local>::DONT_DEALLOCATE_FLAG;
		reloc.m_global.m_data = nullptr;
		reloc.m_global.m_size = 0;
		reloc.m_global.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Global>::DONT_DEALLOCATE_FLAG;
		reloc.m_finish.m_data = nullptr;
		reloc.m_finish.m_size = 0;
		reloc.m_finish.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Finish>::DONT_DEALLOCATE_FLAG;
		reloc.m_imports.m_data = nullptr;
		reloc.m_imports.m_size = 0;
		reloc.m_imports.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Import>::DONT_DEALLOCATE_FLAG;
		reloc.m_pool = nullptr;
		// The exe calls through the writer's vtable (slot 3).
		hkObjectWriter* pWriter = &context.m_writer;
		pWriter->writeObject(context.m_stream, &context.m_sectionHeaders.m_data[i], hkPackfileSectionHeaderClass,
			reloc);
		reloc.~hkRelocationInfo();
	}
	context.m_stream->seek(offsetof(hkPackfileHeader, m_contentsSectionIndex), hkStreamWriter::STREAM_SET);
	bool bSwap =
		context.m_writer.getLayout().m_rules.m_littleEndian != hkStructureLayout::HostLayoutRules.m_littleEndian;
	FNVD_CONSTRUCTED_LOCAL(hkOArchive, headerArchive, &hkOArchive::Destruct, context.m_stream, hkBool(bSwap));
	headerArchive.write32(context.m_locations.m_data[m_contentsPtrPWIndex].m_section);
	headerArchive.write32(context.m_locations.m_data[m_contentsPtrPWIndex].m_offset);
	const hkClass* pContentsClass =
		static_cast<const hkClass*>(m_pendingWrites.m_data[m_contentsClassPWIndex].m_pointer);
	headerArchive.write32(0);
	headerArchive.write32(static_cast<int32_t>(context.m_usedClassNames.m_map
			.getWithDefault(reinterpret_cast<hkUlong>(pContentsClass->getName()), static_cast<hkUlong>(-1))));
}

// The class's signature (byte-swapped for a target of the other endianness) and the tag byte 9, then the class's
// name with its terminator; the name's offset from a3 is recorded in context's used class names.
void hkBinaryPackfileWriter::writeClassName(hkBinaryPackfileWriter::SaveContext& context, const hkClass& a2,
	int32_t a3) {
	uint32_t uiSignature = a2.getSignature(0);
	if (context.m_endianSwapRequired.m_bool) {
		// EXE-INLINED none: hkByteSwap32
		uiSignature = (uiSignature >> 24) | ((uiSignature >> 8) & 0xFF00u) | ((uiSignature << 8) & 0xFF0000u) |
			(uiSignature << 24);
	}
	// The signature as the exe stores it (a dword, little-endian), then the tag.
	uint8_t aucHeader[5];
	aucHeader[0] = static_cast<uint8_t>(uiSignature);
	aucHeader[1] = static_cast<uint8_t>(uiSignature >> 8);
	aucHeader[2] = static_cast<uint8_t>(uiSignature >> 16);
	aucHeader[3] = static_cast<uint8_t>(uiSignature >> 24);
	aucHeader[4] = 9;
	context.m_stream->write(aucHeader, 5);
	const char* pName = a2.getName();
	int32_t iOffset = context.m_stream->tell() - a3;
	context.m_usedClassNames.m_map.insert(reinterpret_cast<hkUlong>(pName), static_cast<hkUlong>(iOffset));
	context.m_stream->write(pName, hkString::strLen(pName) + 1);
}

void hkRelocationInfo::addLocal(int32_t fromOffset, int32_t toOffset) {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	if (m_local.m_size == (m_local.m_capacityAndFlags & hkArrayBase<hkRelocationInfo::Local>::CAPACITY_MASK)) {
		hkArrayUtil::_reserveMore(*Instance->m_heap, this, sizeof(hkRelocationInfo::Local));
	}
	hkRelocationInfo::Local* v5 = &m_local.m_data[m_local.m_size];
	v5->m_fromOffset = fromOffset;
	v5->m_toOffset = toOffset;
	++m_local.m_size;
}
