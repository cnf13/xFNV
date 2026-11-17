#include "Havok/Source/Common/Visualize/Serialize/hkObjectSerialize.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Io/OArchive/hkOArchive.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Array/hkArrayStreamWriter.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Serialize/hkRelocationInfo.h"
#include "Havok/Source/Common/Serialize/Serialize/Platform/hkPlatformObjectWriter.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"

// Writes the object's header (dataID, classID), then r_ebx's local and global fixups and the dataSize bytes at data,
// through an hkOArchive on r_esi (byteSwap its byte swap flag); returns the bytes written or -1. Its only caller is
// hkObjectSerialize::writeObject, which passes &klass, zero-extended, as classID. dataID and classID are each written
// as one 64-bit value.
static int32_t writePacketWithRelocations(uint64_t dataID, uint64_t classID, const void* data, int32_t dataSize,
	hkBool byteSwap, hkRelocationInfo* r_ebx, hkStreamWriter* r_esi);

static int32_t writePacketWithRelocations(uint64_t dataID, uint64_t classID, const void* data, int32_t dataSize,
	hkBool byteSwap, hkRelocationInfo* r_ebx, hkStreamWriter* r_esi) {
	uint32_t m_size;
	int32_t v11;
	uint32_t m_size_3;
	hkBool arResult_;
	int32_t n20;
	uint32_t m_size_1;
	hkOArchive apMemory(r_esi, byteSwap);
	apMemory.write64u(dataID);
	if (!r_esi->isOk(&arResult_)->m_bool) {
		goto LABEL_2;
	}
	apMemory.write64u(classID);
	if (!r_esi->isOk(&arResult_)->m_bool) {
		goto LABEL_2;
	}
	m_size = r_ebx->m_local.m_size;
	apMemory.write32u(m_size);
	v11 = 0;
	n20 = 20;
	if (m_size) {
		do {
			apMemory.write32(r_ebx->m_local.m_data[v11].m_fromOffset);
			apMemory.write32(r_ebx->m_local.m_data[v11].m_toOffset);
			if (!r_esi->isOk(&arResult_)->m_bool) {
				goto LABEL_2;
			}
			n20 += 8;
		} while (++v11 < m_size);
	}
	m_size_1 = r_ebx->m_global.m_size;
	apMemory.write32u(m_size_1);
	n20 += 4;
	m_size_3 = 0;
	if (m_size_1) {
		int32_t v14 = 0;
		do {
			apMemory.write32(r_ebx->m_global.m_data[v14].m_fromOffset);
			// Each address is written as a 64-bit id (hkUlong widened).
			apMemory.write64u(reinterpret_cast<uintptr_t>(r_ebx->m_global.m_data[v14].m_toAddress));
			apMemory.write64u(reinterpret_cast<uintptr_t>(r_ebx->m_global.m_data[v14].m_toClass));
			if (!r_esi->isOk(&arResult_)->m_bool) {
				goto LABEL_2;
			}
			n20 += 20;
			++m_size_3;
			++v14;
		} while (m_size_3 < m_size_1);
	}
	apMemory.write32u(dataSize);
	if (!r_esi->isOk(&arResult_)->m_bool || (r_esi->write(data, dataSize), !r_esi->isOk(&arResult_)->m_bool)) {
	LABEL_2:;
		return -1;
	}
	return n20 + dataSize + 4;
}

// Reads one object as writeObject wrote it: its id, its class id, its local fixups, its global fixups (into
// globalFixups), its size and its bytes; patches the local fixups into the new buffer and hands it out through
// data. Returns the object's id, or 0 when the stream fails (the buffer freed when its bytes did not read).
uint64_t hkObjectSerialize::readObject(hkStreamReader* s, void*& data, uint32_t& dataSize,
	hkArray<hkObjectSerialize::GlobalFixup, hkContainerHeapAllocator>& globalFixups, uint64_t& klassID) {
	using LocalFixupArray = hkArray<hkObjectSerialize::LocalFixup, hkContainerHeapAllocator>;
	hkBool ok;
	uint64_t id;
	uint32_t numLocalFixups;
	uint32_t numGlobalFixups;
	char* buffer;
	s->read(&id, 8);
	if (!s->isOk(&ok)->m_bool) {
		return 0;
	}
	s->read(&klassID, 8);
	if (!s->isOk(&ok)->m_bool) {
		return 0;
	}
	// EXE-INLINED none: hkArray<hkObjectSerialize::LocalFixup, hkContainerHeapAllocator>::hkArray
	FNVD_INLINED_LOCAL(LocalFixupArray, localFixups, std::nullopt, std::nullopt);
	localFixups.m_data = 0;
	localFixups.m_size = 0;
	localFixups.m_capacityAndFlags = LocalFixupArray::DONT_DEALLOCATE_FLAG;
	s->read(&numLocalFixups, 4);
	if (!s->isOk(&ok)->m_bool) {
		goto fail;
	}
	{
		// EXE-INLINED none: hkArray<hkObjectSerialize::LocalFixup, hkContainerHeapAllocator>::setSize
		hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
		int32_t capacity = localFixups.m_capacityAndFlags & LocalFixupArray::CAPACITY_MASK;
		if (capacity < static_cast<int32_t>(numLocalFixups)) {
			int32_t newCapacity = 2 * capacity;
			if (static_cast<int32_t>(numLocalFixups) >= newCapacity) {
				newCapacity = static_cast<int32_t>(numLocalFixups);
			}
			hkArrayUtil::_reserve(*heap, &localFixups, newCapacity, sizeof(hkObjectSerialize::LocalFixup));
		}
		localFixups.m_size = static_cast<int32_t>(numLocalFixups);
	}
	for (uint32_t i = 0; i < numLocalFixups; ++i) {
		hkObjectSerialize::LocalFixup* pFixup = &localFixups.m_data[i];
		s->read(&pFixup->fromOffset, 4);
		s->read(&pFixup->toOffset, 4);
		if (!s->isOk(&ok)->m_bool) {
			goto fail;
		}
	}
	s->read(&numGlobalFixups, 4);
	if (!s->isOk(&ok)->m_bool) {
		goto fail;
	}
	{
		// EXE-INLINED none: hkArray<hkObjectSerialize::GlobalFixup, hkContainerHeapAllocator>::setSize
		hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
		int32_t capacity = globalFixups.m_capacityAndFlags & hkArrayBase<hkObjectSerialize::GlobalFixup>::CAPACITY_MASK;
		if (capacity < static_cast<int32_t>(numGlobalFixups)) {
			int32_t newCapacity = 2 * capacity;
			if (static_cast<int32_t>(numGlobalFixups) >= newCapacity) {
				newCapacity = static_cast<int32_t>(numGlobalFixups);
			}
			hkArrayUtil::_reserve(*heap, &globalFixups, newCapacity, sizeof(hkObjectSerialize::GlobalFixup));
		}
		globalFixups.m_size = static_cast<int32_t>(numGlobalFixups);
	}
	for (uint32_t i = 0; i < numGlobalFixups; ++i) {
		hkObjectSerialize::GlobalFixup* pFixup = &globalFixups.m_data[i];
		s->read(&pFixup->fromOffset, 4);
		s->read(&pFixup->toID, 8);
		s->read(&pFixup->toClassID, 8);
		if (!s->isOk(&ok)->m_bool) {
			goto fail;
		}
	}
	s->read(&dataSize, 4);
	if (!s->isOk(&ok)->m_bool) {
		goto fail;
	}
	buffer = static_cast<char*>(hkMemoryRouter::easyAlloc(*hkMemoryRouter::getInstance().m_heap, dataSize));
	s->read(buffer, dataSize);
	if (!s->isOk(&ok)->m_bool) {
		hkMemoryRouter::easyFree(*hkMemoryRouter::getInstance().m_heap, buffer);
		goto fail;
	}
	for (uint32_t i = 0; i < numLocalFixups; ++i) {
		if (localFixups.m_data[i].fromOffset >= 0) {
			// The object's bytes are untyped here: the patched pointer is stored at a byte offset.
			*reinterpret_cast<char**>(buffer + localFixups.m_data[i].fromOffset) =
				buffer + localFixups.m_data[i].toOffset;
		}
	}
	data = buffer;
	{
		// EXE-INLINED none: hkArray<hkObjectSerialize::LocalFixup, hkContainerHeapAllocator>::~hkArray
		hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
		localFixups.m_size = 0;
		if (localFixups.m_capacityAndFlags >= 0) {
			heap->bufFree(localFixups.m_data,
				(localFixups.m_capacityAndFlags & LocalFixupArray::CAPACITY_MASK) *
					sizeof(hkObjectSerialize::LocalFixup));
		}
	}
	return id;
fail:
	{
		// EXE-INLINED none: hkArray<hkObjectSerialize::LocalFixup, hkContainerHeapAllocator>::~hkArray
		hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
		localFixups.m_size = 0;
		if (localFixups.m_capacityAndFlags >= 0) {
			heap->bufFree(localFixups.m_data,
				(localFixups.m_capacityAndFlags & LocalFixupArray::CAPACITY_MASK) *
					sizeof(hkObjectSerialize::LocalFixup));
		}
	}
	return 0;
}

// Writes the object into a borrowed buffer through a platform object writer, appends its global fixups to globalFixups
// and writes the result, byte-swapped when destLayout's endianness is not the host's.
int32_t hkObjectSerialize::writeObject(hkStreamWriter* s, const hkStructureLayout& destLayout, uint64_t dataID,
	const void* data, const hkClass& klass,
	hkArray<hkObjectSerialize::GlobalFixup, hkContainerHeapAllocator>& globalFixups) {
	using buffer_t = hkArray<char, hkContainerHeapAllocator>;
	FNVD_INLINED_LOCAL(buffer_t, buffer, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(hkArrayStreamWriter, apThis, std::nullopt, std::nullopt);
	hkRelocationInfo arReloc_;
	hkPlatformObjectWriter apMemory(destLayout);
	buffer.m_data = 0;
	buffer.m_size = 0;
	buffer.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
	::new (&apThis) hkArrayStreamWriter(&buffer, hkArrayStreamWriter::ARRAY_BORROW);
	arReloc_.m_local.m_data = 0;
	arReloc_.m_local.m_size = 0;
	arReloc_.m_local.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Local>::DONT_DEALLOCATE_FLAG;
	arReloc_.m_global.m_data = 0;
	arReloc_.m_global.m_size = 0;
	arReloc_.m_global.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Global>::DONT_DEALLOCATE_FLAG;
	arReloc_.m_finish.m_data = 0;
	arReloc_.m_finish.m_size = 0;
	arReloc_.m_finish.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Finish>::DONT_DEALLOCATE_FLAG;
	arReloc_.m_imports.m_data = 0;
	arReloc_.m_imports.m_size = 0;
	arReloc_.m_imports.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Import>::DONT_DEALLOCATE_FLAG;
	arReloc_.m_pool = 0;
	apMemory.hkPlatformObjectWriter::writeObject(&apThis, data, klass, arReloc_);
	int32_t aiReqElem = arReloc_.m_global.m_size + globalFixups.m_size;
	hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t aiReqElem_1 = globalFixups.m_capacityAndFlags & hkArrayBase<hkObjectSerialize::GlobalFixup>::CAPACITY_MASK;
	if (aiReqElem_1 < aiReqElem) {
		int32_t aiReqElem_2 = 2 * aiReqElem_1;
		if (aiReqElem < aiReqElem_2) {
			aiReqElem = aiReqElem_2;
		}
		hkArrayUtil::_reserve(*m_heap, &globalFixups, aiReqElem, sizeof(hkObjectSerialize::GlobalFixup));
	}
	int32_t m_size_1 = 0;
	if (arReloc_.m_global.m_size > 0) {
		int32_t v11 = 0;
		do {
			hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
			if (globalFixups.m_size ==
				(globalFixups.m_capacityAndFlags & hkArrayBase<hkObjectSerialize::GlobalFixup>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*Instance->m_heap, &globalFixups, sizeof(hkObjectSerialize::GlobalFixup));
			}
			int32_t m_size = globalFixups.m_size;
			hkObjectSerialize::GlobalFixup* m_data = (&globalFixups)->begin();
			globalFixups.m_size = m_size + 1;
			hkObjectSerialize::GlobalFixup* v15 = &m_data[m_size];
			v15->fromOffset = arReloc_.m_global.m_data[v11].m_fromOffset;
			// Each address is kept as a 64-bit id (hkUlong widened).
			v15->toID = reinterpret_cast<uintptr_t>(arReloc_.m_global.m_data[v11].m_toAddress);
			++m_size_1;
			v15->toClassID = reinterpret_cast<uintptr_t>(arReloc_.m_global.m_data[v11].m_toClass);
			++v11;
		} while (m_size_1 < arReloc_.m_global.m_size);
	}
	int32_t v16 = writePacketWithRelocations(dataID, reinterpret_cast<uintptr_t>(&klass), buffer.begin(), buffer.m_size,
		hkBool(destLayout.m_rules.m_littleEndian != hkStructureLayout::HostLayoutRules.m_littleEndian), &arReloc_, s);
	apThis.~hkArrayStreamWriter();
	hkMemoryAllocator* m_heap_1 = (&hkMemoryRouter::getInstance())->m_heap;
	buffer.m_size = 0;
	if (buffer.m_capacityAndFlags >= 0) {
		m_heap_1->bufFree(buffer.begin(), buffer.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
	}
	buffer.m_data = 0;
	buffer.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
	return v16;
}
