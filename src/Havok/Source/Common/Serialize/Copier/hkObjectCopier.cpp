#include "Havok/Source/Common/Serialize/Copier/hkObjectCopier.h"

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OArchive/hkOArchive.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/SceneData/AlignScene/hkAlignSceneToNodeOptions.h"
#include "Havok/Source/Common/Serialize/Serialize/hkRelocationInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "SDK/MSVC/string.h"

// Inferred type: return: int32_t: signed uses from 0xe03b0b; a2: int32_t: signed uses from 0xe049ca
static int32_t hkMin2(int32_t a1, int32_t a2);

static int32_t hkMin2(int32_t a1, int32_t a2) {
	int32_t result = a1;
	if (a1 >= a2) {
		return a2;
	}
	return result;
}

namespace {
// The PDB's `anonymous namespace'::ObjectCopier_DummyArray<T> of hkObjectCopier.cpp: an hkArray's or a simple
// array's storage as the copier reads it from the source object.
template <typename T> struct ObjectCopier_DummyArray {
	T* data;
	int32_t size;
	int32_t capAndFlags;
};

// The PDB's `anonymous namespace'::ObjectCopier_DummyHomogeneousArray of hkObjectCopier.cpp: a homogeneous array's
// class, storage and size.
struct ObjectCopier_DummyHomogeneousArray {
	const hkClass* klass;
	void* data;
	int32_t size;
};
}  // namespace

hkResult applyDefaultsFor(const hkClass& a1, const hkClassMember& a2, int32_t a3, hkStreamWriter* a4) {
	hkResult result = a1.getDefault(a3, a4);
	hkResult v5 = result;
	if (a2.m_type.m_storage == hkClassMember::TYPE_STRUCT) {
		const hkClass* Class = a2.getClass();
		if (Class) {
			int32_t v7 = hkObjectCopier::getNumElements(a2);
			if (v7 > 0) {
				int32_t a2a = v7;
				do {
					for (int32_t i = 0; i < Class->getNumMembers(); ++i) {
						const hkClassMember* Member = &Class->getMember(i);
						if (applyDefaultsFor(*Class, *Member, i, a4) == HK_SUCCESS) {
							v5 = HK_SUCCESS;
						}
					}
					--a2a;
				} while (a2a);
			}
		}
		return v5;
	}
	return result;
}

// The PDB's saveCstring, a free function of hkObjectCopier.cpp: pads the archive's writer to 2 bytes, records a local
// fixup from a2 to the string's offset and writes the string with its terminator. The exe passes the archive in ebx
// and the relocations in esi; objectCopierPadUp takes the writer in edi and the alignment in eax.
int32_t saveCstring(char* a1, int32_t a2, hkOArchive* r_ebx, hkRelocationInfo* r_esi) {
	objectCopierPadUp(r_ebx->getStreamWriter(), 2);
	hkStreamWriter* StreamWriter = r_ebx->getStreamWriter();
	int32_t m_toOffset = StreamWriter->tell();
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	if (r_esi->m_local.m_size ==
		(r_esi->m_local.m_capacityAndFlags & hkArrayBase<hkRelocationInfo::Local>::CAPACITY_MASK)) {
		hkArrayUtil::_reserveMore(*Instance->m_heap, &r_esi->m_local, sizeof(hkRelocationInfo::Local));
	}
	hkRelocationInfo::Local* v7 = &r_esi->m_local.m_data[r_esi->m_local.m_size];
	v7->m_fromOffset = a2;
	v7->m_toOffset = m_toOffset;
	++r_esi->m_local.m_size;
	int32_t v8 = hkString::strLen(a1);
	r_ebx->writeRaw(a1, v8 + 1);
	return 0;
}

bool inRange(int32_t num, int32_t lo, int32_t hi) {
	return num >= lo && num < hi;
}

// The exe takes the writer in edi and aiAlign in eax.
// tell() comes first, then an hkLocalArray<char> of aiAlign bytes (constructor, setSize(aiAlign, 0) and destructor
// inlined as in hkaSkeletonUtils::getDescendants) supplies the zeros written.
void objectCopierPadUp(hkStreamWriter* w, int32_t pad) {
	const int32_t pos = w->tell();
	FNVD_INLINED_LOCAL(hkLocalArray<char>, zeros, std::nullopt, std::nullopt);
	zeros.m_data = nullptr;
	zeros.m_size = 0;
	zeros.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
	// EXE-INLINED none: hkAllocateStack<char>
	hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
	void* pBlock = stack.m_next.m_storage;
	stack.m_next.m_storage = static_cast<char*>(pBlock) + ((pad + 127) & ~127);
	char* pLocal = static_cast<char*>(pBlock);
	zeros.m_data = pLocal;
	zeros.m_capacityAndFlags = static_cast<uint32_t>(pad) | hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
	zeros.m_localMemory = pLocal;

	// EXE-INLINED none: hkArray<char>::setSize(aiAlign, 0)
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	const int32_t capacity = static_cast<int32_t>(zeros.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
	if (capacity < pad) {
		int32_t request = 2 * capacity;
		if (pad >= request)
			request = pad;
		hkArrayUtil::_reserve(pHeap, &zeros, request, sizeof(char));
	}
	const int32_t numNew = pad - zeros.m_size;
	if (numNew > 0)
		std::memset(zeros.m_data + zeros.m_size, 0, numNew);
	zeros.m_size = static_cast<uint32_t>(pad);

	const int32_t misalign = pos & (pad - 1);
	if (misalign != 0)
		w->write(zeros.m_data, pad - misalign);

	zeros.~hkLocalArray<char>();
}

// Writes the pointer-sized values as elemsize-byte elements: as they are when elemsize is 4, else widened to 64 bits
// in a buffer on the Havok stack.
void writeUlongArray(hkOArchive& oa, int32_t elemsize, int32_t nelem, const hkUlong* startAddress) {
	if (elemsize == 4) {
		oa.writeArrayGeneric(startAddress, elemsize, nelem);
	} else {
		// EXE-INLINED none: hkAllocateStack<hkUint64>
		hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
		void* pBlock = pRouter->m_stack.m_next.m_storage;
		pRouter->m_stack.m_next.m_storage =
			static_cast<char*>(pBlock) + ((static_cast<int32_t>(sizeof(hkUint64)) * nelem + 127) & ~127);
		hkUint64* pBuffer = static_cast<hkUint64*>(pBlock);
		for (int32_t i = 0; i < nelem; ++i) {
			pBuffer[i] = startAddress[i];
		}
		oa.writeArrayGeneric(pBuffer, elemsize, nelem);
		// EXE-INLINED none: hkDeallocateStack<hkUint64>
		hkMemoryRouter::getInstance().m_stack.m_next.m_storage = static_cast<char*>(pBlock);
	}
}

// Bytes are swapped where the two layouts' endianness differs.
hkObjectCopier::hkObjectCopier(const hkStructureLayout& layoutIn, const hkStructureLayout& layoutOut, uint32_t flags)
	: m_layoutIn(layoutIn), m_layoutOut(layoutOut), m_flags(flags),
	  m_byteSwap(layoutIn.m_rules.m_littleEndian != layoutOut.m_rules.m_littleEndian) {}

// Nothing to release: the members are plain data.
hkObjectCopier::~hkObjectCopier() {}

// Writes dataIn's object in klassOut's layout to dataOut through a local archive: the body, then the extras (arrays,
// strings, pointed-to data), each padded to 16 bytes; the result is the archive's state. objectCopierPadUp takes
// dataOut in edi and the alignment in eax.
hkResult hkObjectCopier::copyObject(const void* dataIn, const hkClass& klassIn, hkStreamWriter* dataOut,
	const hkClass& klassOut, hkRelocationInfo& reloc) {
	FNVD_CONSTRUCTED_LOCAL(hkOArchive, oa, &hkOArchive::Destruct, dataOut, m_byteSwap);
	const int32_t classStart = saveBody(dataIn, klassIn, oa, klassOut);
	objectCopierPadUp(dataOut, 16);
	saveExtras(dataIn, klassIn, oa, klassOut, classStart, reloc, 0);
	objectCopierPadUp(dataOut, 16);
	hkBool ok;
	const hkResult result = static_cast<hkResult>(oa.isOk(&ok)->m_bool == 0);
	return result;
}

// The types' bytes are compared as bytes; an enum member is compatible whatever its storage subtype.
int32_t hkObjectCopier::areMembersCompatible(const hkClassMember& src, const hkClassMember& dst) {
	const uint8_t eType = src.m_type.m_storage;
	if (eType == dst.m_type.m_storage &&
		(src.m_subtype.m_storage == dst.m_subtype.m_storage || eType == hkClassMember::TYPE_ENUM)) {
		return 1;
	}
	return 0;
}

// Writes the member's bytes in the destination layout as zeroes; an hkArray writes null data, size 0 and a capacity
// word of DONT_DEALLOCATE_FLAG. The zeroes come from a local array on the Havok stack (hkLocalArray<char>), whose
// construction, setSize and destruction the exe inlines.
void hkObjectCopier::writeZero(hkOArchive& oa, const hkClassMember& memberOut) {
	const uint8_t eType = memberOut.m_type.m_storage;
	int32_t iSize = 0;
	switch (eType) {
	case hkClassMember::TYPE_BOOL:
	case hkClassMember::TYPE_CHAR:
	case hkClassMember::TYPE_INT8:
	case hkClassMember::TYPE_UINT8:
	case hkClassMember::TYPE_INT16:
	case hkClassMember::TYPE_UINT16:
	case hkClassMember::TYPE_INT32:
	case hkClassMember::TYPE_UINT32:
	case hkClassMember::TYPE_INT64:
	case hkClassMember::TYPE_UINT64:
	case hkClassMember::TYPE_REAL:
	case hkClassMember::TYPE_VECTOR4:
	case hkClassMember::TYPE_QUATERNION:
	case hkClassMember::TYPE_MATRIX3:
	case hkClassMember::TYPE_ROTATION:
	case hkClassMember::TYPE_QSTRANSFORM:
	case hkClassMember::TYPE_MATRIX4:
	case hkClassMember::TYPE_TRANSFORM:
	case hkClassMember::TYPE_ENUM:
	case hkClassMember::TYPE_FLAGS:
	case hkClassMember::TYPE_HALF:
		iSize = memberOut.getSizeInBytes();
		break;
	case hkClassMember::TYPE_POINTER:
	case hkClassMember::TYPE_FUNCTIONPOINTER:
	case hkClassMember::TYPE_ULONG:
		iSize = getNumElements(memberOut);
		iSize *= m_layoutOut.m_rules.m_bytesInPointer;
		break;
	case hkClassMember::TYPE_ARRAY:
	case hkClassMember::TYPE_INPLACEARRAY:
	case hkClassMember::TYPE_SIMPLEARRAY:
	case hkClassMember::TYPE_HOMOGENEOUSARRAY:
		if (eType == hkClassMember::TYPE_HOMOGENEOUSARRAY) {
			iSize = m_layoutOut.m_rules.m_bytesInPointer;
		}
		iSize = iSize + m_layoutOut.m_rules.m_bytesInPointer + 4;
		if (eType == hkClassMember::TYPE_ARRAY) {
			char acZero[12] = {};
			oa.writeRaw(acZero, iSize);
			oa.write32u(hkArrayBase<char>::DONT_DEALLOCATE_FLAG);
			return;
		}
		break;
	case hkClassMember::TYPE_STRUCT:
		iSize = memberOut.getStructClass().getObjectSize();
		iSize *= getNumElements(memberOut);
		break;
	case hkClassMember::TYPE_VARIANT:
		iSize = getNumElements(memberOut);
		iSize *= m_layoutOut.m_rules.m_bytesInPointer;
		iSize += iSize;
		break;
	default: {
		char acBuffer[0x200];
		// Destroyed by hand below, in the exe's order.
		FNVD_CONSTRUCTED_LOCAL(hkErrStream, kError, nullptr, acBuffer, 0x200);
		// The exe's call target is 0xCC01C0, src's const char* overload.
		kError << "Unknown class member type found!";
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x5EF4E5A4, acBuffer, ".\\Copier\\hkObjectCopier.cpp",
				0xA8)) {
			__builtin_debugtrap();
		}
		break;
	}
	}

	// EXE-INLINED none: hkLocalArray<char>::hkLocalArray
	FNVD_INLINED_LOCAL(hkArray<char>, kZeroes, std::nullopt, std::nullopt);
	kZeroes.m_data = nullptr;
	kZeroes.m_size = 0;
	kZeroes.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
	hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
	char* pLocalMemory = rRouter.m_stack.m_next.m_storage;
	rRouter.m_stack.m_next.m_storage = pLocalMemory + ((iSize + 0x7F) & 0xFFFFFF80);
	kZeroes.m_data = pLocalMemory;
	kZeroes.m_capacityAndFlags = static_cast<uint32_t>(iSize) | hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
	// EXE-INLINED none: hkArrayBase<char>::_setSize
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	int32_t iCapacity = static_cast<int32_t>(kZeroes.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
	if (iCapacity < iSize) {
		iCapacity += iCapacity;
		if (iSize >= iCapacity) {
			iCapacity = iSize;
		}
		hkArrayUtil::_reserve(pHeap, &kZeroes, iCapacity, sizeof(char));
	}
	const int32_t iFill = iSize - kZeroes.m_size;
	if (iFill > 0) {
		std::memset(kZeroes.m_data + kZeroes.m_size, 0, iFill);
	}
	kZeroes.m_size = static_cast<uint32_t>(iSize);
	oa.writeRaw(kZeroes.m_data, iSize);
	// The exe's inlined ~hkLocalArray<char> on an array src holds as hkArray<char>: written out in the hybrid build.
	// Natively the local is raw storage, so no destructor runs at scope end: the stack block goes back here, then
	// ~hkArray frees a reallocated storage.
	hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pLocalMemory;
	kZeroes.~hkArray<char>();
}

// Writes klassOut's members of the object at dataIn (laid out as klassIn) at the stream's position, member by member,
// pointers and arrays as zeroes; leaves the stream after klassOut's object size and returns where the object starts.
// KEEP (not registered): in the TYPE_FLAGS case the exe ORs bitValue into bitsOver where dstEnum.getValueOfName fails
// (0xE03C45), and getValueOfName (0xDB75B0) writes nothing on failure: a source flag name the destination enum lacks,
// before any successful lookup in this frame, reads the uninitialised slot [esp+0x40].
int32_t hkObjectCopier::saveBody(const void* dataIn, const hkClass& klassIn, hkOArchive& oa, const hkClass& klassOut) {
	char buf[512];
	hkStreamWriter* streamWriter = oa.getStreamWriter();
	int32_t classStart = streamWriter->tell();
	for (int32_t memberIndex = 0; memberIndex < klassOut.getNumMembers(); ++memberIndex) {
		const hkClassMember& dstMember = klassOut.getMember(memberIndex);
		streamWriter->seek(classStart + dstMember.m_offset, hkStreamWriter::STREAM_SET);
		if (dstMember.m_flags.get(hkClassMember::SERIALIZE_IGNORED)) {
			if (m_flags.get(FLAG_RESPECT_SERIALIZE_IGNORED)) {
				writeZero(oa, dstMember);
				continue;
			}
			if (m_flags.get(FLAG_APPLY_DEFAULT_IF_SERIALIZE_IGNORED)) {
				applyDefaultsFor(klassOut, dstMember, memberIndex, streamWriter);
				continue;
			}
		}
		const hkClassMember* srcMemberPtr = klassIn.getMemberByName(dstMember.m_name);
		if (!srcMemberPtr || !areMembersCompatible(*srcMemberPtr, dstMember)) {
			applyDefaultsFor(klassOut, dstMember, memberIndex, streamWriter);
			continue;
		}
		const hkClassMember& srcMember = *srcMemberPtr;
		const uint8_t dstType = dstMember.m_type.m_storage;
		const void* srcMemberAddress = static_cast<const char*>(dataIn) + srcMember.m_offset;
		switch (dstType) {
		case hkClassMember::TYPE_BOOL:
		case hkClassMember::TYPE_CHAR:
		case hkClassMember::TYPE_INT8:
		case hkClassMember::TYPE_UINT8:
		case hkClassMember::TYPE_INT16:
		case hkClassMember::TYPE_UINT16:
		case hkClassMember::TYPE_INT32:
		case hkClassMember::TYPE_UINT32:
		case hkClassMember::TYPE_INT64:
		case hkClassMember::TYPE_UINT64:
		case hkClassMember::TYPE_REAL:
		case hkClassMember::TYPE_VECTOR4:
		case hkClassMember::TYPE_QUATERNION:
		case hkClassMember::TYPE_MATRIX3:
		case hkClassMember::TYPE_ROTATION:
		case hkClassMember::TYPE_QSTRANSFORM:
		case hkClassMember::TYPE_MATRIX4:
		case hkClassMember::TYPE_TRANSFORM:
		case hkClassMember::TYPE_HALF: {
			int32_t srcCount = getNumElements(srcMember);
			int32_t dstCount = getNumElements(dstMember);
			int32_t dstSize = dstMember.getSizeInBytes();
			int32_t nelem = hkMin2(srcCount, dstCount);
			// writePodArray takes char* where the source object is const; it only reads it.
			writePodArray(nelem, static_cast<uint32_t>(dstSize / dstCount), &oa,
				static_cast<char*>(const_cast<void*>(srcMemberAddress)), dstMember.m_type.m_storage);
			break;
		}
		case hkClassMember::TYPE_ZERO: {
			break;
		}
		case hkClassMember::TYPE_POINTER:
		case hkClassMember::TYPE_FUNCTIONPOINTER:
		case hkClassMember::TYPE_CSTRING:
		case hkClassMember::TYPE_STRINGPTR: {
			int32_t count = getNumElements(dstMember);
			hkUint64 zero = 0;
			for (int32_t i = 0; i < count; ++i) {
				oa.writeRaw(&zero, m_layoutOut.m_rules.m_bytesInPointer);
			}
			break;
		}
		case hkClassMember::TYPE_ARRAY:
		case hkClassMember::TYPE_SIMPLEARRAY:
		case hkClassMember::TYPE_HOMOGENEOUSARRAY: {
			int32_t size;
			if (dstType == hkClassMember::TYPE_HOMOGENEOUSARRAY) {
				// The class pointer.
				hkUint64 zeroClass = 0;
				oa.writeArrayGeneric(&zeroClass, m_layoutOut.m_rules.m_bytesInPointer, 1);
				const ObjectCopier_DummyHomogeneousArray* srcArray =
					static_cast<const ObjectCopier_DummyHomogeneousArray*>(srcMemberAddress);
				if (srcArray->klass) {
					size = srcArray->size;
				} else {
					FNVD_CONSTRUCTED_LOCAL(hkErrStream, errStream, nullptr, buf, 512);
					const char* memberName = srcMember.m_name;
					const char* className = klassIn.getName();
					errStream << "Can't copy homogeneous array. No hkClass for " << className << "::" << memberName
							  << ".";
					hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA5A55), buf,
						".\\Copier\\hkObjectCopier.cpp", 440);
					size = 0;
				}
			} else if (dstMember.getSubType() != hkClassMember::TYPE_VOID) {
				size = static_cast<const ObjectCopier_DummyArray<char>*>(srcMemberAddress)->size;
			} else {
				size = 0;
			}
			// The data pointer.
			hkUint64 zeroData = 0;
			oa.writeArrayGeneric(&zeroData, m_layoutOut.m_rules.m_bytesInPointer, 1);
			oa.write32(size);
			if (dstMember.m_type.m_storage == hkClassMember::TYPE_ARRAY) {
				oa.write32(size | static_cast<int32_t>(hkArrayBase<char>::DONT_DEALLOCATE_FLAG));
			}
			break;
		}
		case hkClassMember::TYPE_ENUM: {
			if (srcMember.m_enum && dstMember.m_enum) {
				int32_t srcCount = getNumElements(srcMember);
				int32_t dstCount = getNumElements(dstMember);
				int32_t count = hkMin2(srcCount, dstCount);
				const hkClassEnum& srcEnum = srcMember.getEnumClass();
				const hkClassEnum& dstEnum = dstMember.getEnumClass();
				int32_t elemSize = srcMember.getSizeInBytes() / srcCount;
				const char* srcAddress = static_cast<const char*>(srcMemberAddress);
				for (int32_t i = 0; i < count; ++i) {
					int32_t srcValue = srcMember.getEnumValue(srcAddress);
					int32_t dstValue = 0;
					const char* valueName;
					if (srcEnum.getNameOfValue(srcValue, &valueName) == HK_SUCCESS) {
						dstEnum.getValueOfName(valueName, &dstValue);
					}
					switch (elemSize) {
					case 1:
						oa.write8(dstValue);
						break;
					case 2:
						oa.write16(static_cast<int16_t>(dstValue));
						break;
					case 4:
						oa.write32(dstValue);
						break;
					}
					srcAddress += elemSize;
				}
			}
			break;
		}
		case hkClassMember::TYPE_STRUCT: {
			const hkClass* srcClass = srcMember.getClass();
			const hkClass* dstClass = dstMember.getClass();
			if (srcClass && dstClass) {
				int32_t dstCount = getNumElements(dstMember);
				int32_t srcCount = getNumElements(srcMember);
				int32_t count = hkMin2(srcCount, dstCount);
				for (int32_t i = 0; i < count; ++i) {
					saveBody(static_cast<const char*>(srcMemberAddress) + srcClass->getObjectSize() * i, *srcClass, oa,
						*dstClass);
				}
			}
			break;
		}
		case hkClassMember::TYPE_VARIANT: {
			// The object and class pointers.
			hkUint64 zero[2] = {0, 0};
			int32_t dstCount = getNumElements(dstMember);
			int32_t srcCount = getNumElements(srcMember);
			int32_t count = hkMin2(srcCount, dstCount);
			for (int32_t i = 0; i < count; ++i) {
				oa.writeArrayGeneric(zero, m_layoutOut.m_rules.m_bytesInPointer, 2);
			}
			break;
		}
		case hkClassMember::TYPE_ULONG: {
			int32_t srcCount = getNumElements(srcMember);
			int32_t dstCount = getNumElements(dstMember);
			int32_t nelem = hkMin2(srcCount, dstCount);
			writeUlongArray(oa, m_layoutOut.m_rules.m_bytesInPointer, nelem,
				static_cast<const hkUlong*>(srcMemberAddress));
			break;
		}
		case hkClassMember::TYPE_FLAGS: {
			if (srcMember.m_enum && dstMember.m_enum) {
				int32_t srcCount = getNumElements(srcMember);
				int32_t dstCount = getNumElements(dstMember);
				int32_t count = hkMin2(srcCount, dstCount);
				const hkClassEnum& srcEnum = srcMember.getEnumClass();
				const hkClassEnum& dstEnum = dstMember.getEnumClass();
				int32_t elemSize = srcMember.getSizeInBytes() / srcCount;
				const char* srcAddress = static_cast<const char*>(srcMemberAddress);
				// Written only by a successful getValueOfName: the KEEP reason above.
				int32_t bitValue;
				for (int32_t i = 0; i < count; ++i) {
					int32_t srcValue = srcMember.getEnumValue(srcAddress);
					using bitNames_t = hkArray<const char*, hkContainerHeapAllocator>;
					FNVD_INLINED_LOCAL(bitNames_t, bitNames, std::nullopt, std::nullopt);
					bitNames.m_data = nullptr;
					bitNames.m_size = 0;
					bitNames.m_capacityAndFlags = hkArrayBase<const char*>::DONT_DEALLOCATE_FLAG;
					int32_t bitsOver = 0;
					srcEnum.decomposeFlags(srcValue, bitNames, bitsOver);
					int32_t dstValue = 0;
					for (int32_t j = 0; j < bitNames.m_size; ++j) {
						if (dstEnum.getValueOfName(bitNames.m_data[j], &bitValue) == HK_SUCCESS) {
							dstValue |= bitValue;
						} else {
							bitsOver |= bitValue;
						}
					}
					if (bitsOver) {
						int32_t dstBitsOver = 0;
						dstEnum.decomposeFlags(bitsOver, bitNames, dstBitsOver);
						dstValue |= dstBitsOver;
					}
					switch (elemSize) {
					case 1:
						oa.write8(dstValue);
						break;
					case 2:
						oa.write16(static_cast<int16_t>(dstValue));
						break;
					case 4:
						oa.write32(dstValue);
						break;
					}
					srcAddress += elemSize;
					// EXE-INLINED none: hkArray<const char*, hkContainerHeapAllocator>::~hkArray
					hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
					bitNames.m_size = 0;
					if (bitNames.m_capacityAndFlags >= 0) {
						heap.bufFree(bitNames.m_data,
							bitNames.getCapacity() * static_cast<int32_t>(sizeof(const char*)));
					}
					bitNames.m_data = nullptr;
					bitNames.m_capacityAndFlags = hkArrayBase<const char*>::DONT_DEALLOCATE_FLAG;
				}
			}
			break;
		}
		default: {
			FNVD_CONSTRUCTED_LOCAL(hkErrStream, errStream, nullptr, buf, 512);
			errStream << "Unknown class member found during write of data.";
			if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x641E3E03, buf, ".\\Copier\\hkObjectCopier.cpp",
					549)) {
				__builtin_debugtrap();
			}
			break;
		}
		}
	}
	hkStreamWriter* endWriter = oa.getStreamWriter();
	endWriter->seek(classStart + klassOut.getObjectSize(), hkStreamWriter::STREAM_SET);
	return classStart;
}

// Records the fixups of the object at dataIn (laid out as klassIn) that saveBody wrote as klassOut at classStart: the
// class to finish (top level only), the pointers to relocate, and the out-of-line data of arrays, strings and nested
// structs, which it writes at the stream's position, each member's padded to 16 bytes.
void hkObjectCopier::saveExtras(const void* dataIn, const hkClass& klassIn, hkOArchive& dataOut,
	const hkClass& klassOut, int32_t classStart, hkRelocationInfo& fixups, int32_t level) {
	if (level == 0) {
		// EXE-INLINED none: hkRelocationInfo::addFinish
		const char* className = klassOut.getName();
		hkMemoryRouter& router = hkMemoryRouter::getInstance();
		if (fixups.m_finish.m_size ==
			(fixups.m_finish.m_capacityAndFlags & hkArrayBase<hkRelocationInfo::Finish>::CAPACITY_MASK)) {
			hkArrayUtil::_reserveMore(*router.m_heap, &fixups.m_finish, sizeof(hkRelocationInfo::Finish));
		}
		hkRelocationInfo::Finish& finish = fixups.m_finish.m_data[fixups.m_finish.m_size];
		finish.m_fromOffset = classStart;
		finish.m_className = className;
		++fixups.m_finish.m_size;
	}
	++level;
	for (int32_t memberIndex = 0; memberIndex < klassOut.getNumMembers(); ++memberIndex) {
		const hkClassMember& memberOut = klassOut.getMember(memberIndex);
		const hkClassMember* memberInPtr = klassIn.getMemberByName(memberOut.m_name);
		if (!memberInPtr) {
			continue;
		}
		const hkClassMember& memberIn = *memberInPtr;
		// The source member's bytes: memory the copier reads untyped, as its class record describes it (the SDK's
		// casts of addrIn below are the exe's reads).
		const char* addrIn = static_cast<const char*>(dataIn) + memberIn.m_offset;
		if (!areMembersCompatible(memberIn, memberOut)) {
			continue;
		}
		if (m_flags.get(FLAG_RESPECT_SERIALIZE_IGNORED) && memberOut.m_flags.get(hkClassMember::SERIALIZE_IGNORED)) {
			continue;
		}
		switch (memberOut.m_type.m_storage) {
		case hkClassMember::TYPE_BOOL:
		case hkClassMember::TYPE_CHAR:
		case hkClassMember::TYPE_INT8:
		case hkClassMember::TYPE_UINT8:
		case hkClassMember::TYPE_INT16:
		case hkClassMember::TYPE_UINT16:
		case hkClassMember::TYPE_INT32:
		case hkClassMember::TYPE_UINT32:
		case hkClassMember::TYPE_INT64:
		case hkClassMember::TYPE_UINT64:
		case hkClassMember::TYPE_REAL:
		case hkClassMember::TYPE_VECTOR4:
		case hkClassMember::TYPE_QUATERNION:
		case hkClassMember::TYPE_MATRIX3:
		case hkClassMember::TYPE_ROTATION:
		case hkClassMember::TYPE_QSTRANSFORM:
		case hkClassMember::TYPE_MATRIX4:
		case hkClassMember::TYPE_TRANSFORM:
		case hkClassMember::TYPE_FUNCTIONPOINTER:
		case hkClassMember::TYPE_ENUM:
		case hkClassMember::TYPE_ULONG:
		case hkClassMember::TYPE_FLAGS:
		case hkClassMember::TYPE_HALF:
			break;
		case hkClassMember::TYPE_ZERO: {
			char buf[512];
			FNVD_INLINED_LOCAL(hkErrStream, error, std::nullopt, std::nullopt);
			::new (&error) hkErrStream(buf, 512);
			error << "TYPE_ZERO should not occur.";
			if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x641E3E05, buf, ".\\Copier\\hkObjectCopier.cpp",
					926)) {
				__builtin_debugtrap();
			}
			FNVD_DESTRUCT(hkOstream)(&error);
			break;
		}
		case hkClassMember::TYPE_POINTER: {
			// The exe takes the destination's count first, the source's second.
			const int32_t dstCount = getNumElements(memberOut);
			const int32_t srcCount = getNumElements(memberIn);
			const int32_t numPointers = hkMin2(srcCount, dstCount);
			const int32_t srcCountForStride = getNumElements(memberIn);
			const int32_t stride = memberIn.getSizeInBytes() / srcCountForStride;
			const char* pointerIn = addrIn;
			for (int32_t i = 0; i < numPointers; ++i) {
				void* pointer = *reinterpret_cast<void* const*>(pointerIn);
				if (pointer) {
					const int32_t fromOffset = m_layoutOut.m_rules.m_bytesInPointer * i + memberOut.m_offset;
					if (memberOut.m_subtype.m_storage == hkClassMember::TYPE_CHAR) {
						saveCstring(static_cast<char*>(pointer), classStart + fromOffset, &dataOut, &fixups);
					} else {
						const hkClass* klass = memberOut.getClass();
						fixups.addGlobal(classStart + fromOffset, pointer, klass, 0);
					}
				}
				pointerIn += stride;
			}
			break;
		}
		case hkClassMember::TYPE_ARRAY:
		case hkClassMember::TYPE_INPLACEARRAY:
		case hkClassMember::TYPE_SIMPLEARRAY: {
			const ObjectCopier_DummyArray<char>& arrayIn =
				*reinterpret_cast<const ObjectCopier_DummyArray<char>*>(addrIn);
			if (arrayIn.size == 0) {
				break;
			}
			const uint8_t subType = memberOut.m_subtype.m_storage;
			switch (subType) {
			case hkClassMember::TYPE_VOID:
				break;
			case hkClassMember::TYPE_POINTER: {
				hkStreamWriter* writer = dataOut.getStreamWriter();
				fixups.addLocal(classStart + memberOut.m_offset, writer->tell());
				uint32_t zero[2] = {0, 0};
				const int32_t elemSize = memberIn.getArrayMemberSize();
				for (int32_t i = 0; i < arrayIn.size; ++i) {
					void* pointer = *reinterpret_cast<void* const*>(arrayIn.data + i * elemSize);
					if (pointer) {
						hkStreamWriter* elemWriter = dataOut.getStreamWriter();
						const hkClass* klass = memberIn.getClass();
						const int32_t fromOffset = elemWriter->tell();
						fixups.addGlobal(fromOffset, pointer, klass, 0);
					}
					dataOut.writeRaw(zero, m_layoutOut.m_rules.m_bytesInPointer);
				}
				break;
			}
			case hkClassMember::TYPE_CSTRING: {
				const ObjectCopier_DummyArray<char*>& strings =
					*reinterpret_cast<const ObjectCopier_DummyArray<char*>*>(addrIn);
				hkStreamWriter* writer = dataOut.getStreamWriter();
				fixups.addLocal(classStart + memberOut.m_offset, writer->tell());
				uint32_t zero[2] = {0, 0};
				const int32_t start = dataOut.getStreamWriter()->tell();
				for (int32_t i = 0; i < strings.size; ++i) {
					dataOut.writeRaw(zero, m_layoutOut.m_rules.m_bytesInPointer);
				}
				for (int32_t i = 0; i < strings.size; ++i) {
					if (strings.data[i]) {
						saveCstring(strings.data[i], start + m_layoutOut.m_rules.m_bytesInPointer * i, &dataOut,
							&fixups);
					}
				}
				break;
			}
			case hkClassMember::TYPE_STRINGPTR: {
				// An hkStringPtr is its string's address with the owned flag in bit 0.
				const ObjectCopier_DummyArray<hkUlong>& strings =
					*reinterpret_cast<const ObjectCopier_DummyArray<hkUlong>*>(addrIn);
				hkStreamWriter* writer = dataOut.getStreamWriter();
				fixups.addLocal(classStart + memberOut.m_offset, writer->tell());
				uint32_t zero[2] = {0, 0};
				const int32_t start = dataOut.getStreamWriter()->tell();
				for (int32_t i = 0; i < strings.size; ++i) {
					dataOut.writeRaw(zero, m_layoutOut.m_rules.m_bytesInPointer);
				}
				for (int32_t i = 0; i < strings.size; ++i) {
					const hkUlong stringAndFlag = strings.data[i];
					if (stringAndFlag & ~static_cast<hkUlong>(1)) {
						saveCstring(reinterpret_cast<char*>(stringAndFlag & ~static_cast<hkUlong>(1)),
							start + m_layoutOut.m_rules.m_bytesInPointer * i, &dataOut, &fixups);
					}
				}
				break;
			}
			case hkClassMember::TYPE_STRUCT: {
				hkStreamWriter* writer = dataOut.getStreamWriter();
				const int32_t start = writer->tell();
				fixups.addLocal(classStart + memberOut.m_offset, start);
				const hkClass* structIn = memberIn.getClass();
				const hkClass* structOut = memberOut.getClass();
				if (structIn && structOut) {
					char* data = arrayIn.data;
					for (int32_t i = 0; i < arrayIn.size; ++i) {
						saveBody(data + i * structIn->getObjectSize(), *structIn, dataOut, *structOut);
					}
					for (int32_t i = 0; i < arrayIn.size; ++i) {
						const int32_t objectStart = start + i * structOut->getObjectSize();
						const char* objectIn = data + i * structIn->getObjectSize();
						saveExtras(objectIn, *structIn, dataOut, *structOut, objectStart, fixups, level);
					}
				}
				break;
			}
			case hkClassMember::TYPE_VARIANT: {
				const ObjectCopier_DummyArray<hkVariant>& variants =
					*reinterpret_cast<const ObjectCopier_DummyArray<hkVariant>*>(addrIn);
				hkStreamWriter* writer = dataOut.getStreamWriter();
				const int32_t start = writer->tell();
				fixups.addLocal(classStart + memberOut.m_offset, start);
				for (int32_t i = 0; i < variants.size; ++i) {
					uint32_t zero[4] = {0, 0, 0, 0};
					dataOut.writeArrayGeneric(zero, m_layoutOut.m_rules.m_bytesInPointer, 2);
				}
				for (int32_t i = 0; i < variants.size; ++i) {
					const int32_t fromOffset = 2 * (m_layoutOut.m_rules.m_bytesInPointer * i);
					void* object = variants.data[i].m_object;
					if (object) {
						fixups.addGlobal(start + fromOffset, object, variants.data[i].m_class, 0);
					}
					const hkClass* klass = variants.data[i].m_class;
					if (klass) {
						fixups.addGlobal(start + fromOffset + m_layoutOut.m_rules.m_bytesInPointer,
							const_cast<hkClass*>(klass), &hkClassClass, 0);
					}
				}
				break;
			}
			default: {
				hkStreamWriter* writer = dataOut.getStreamWriter();
				fixups.addLocal(classStart + memberOut.m_offset, writer->tell());
				if (subType == hkClassMember::TYPE_ULONG) {
					const ObjectCopier_DummyArray<hkUlong>& ulongs =
						*reinterpret_cast<const ObjectCopier_DummyArray<hkUlong>*>(addrIn);
					writeUlongArray(dataOut, m_layoutOut.m_rules.m_bytesInPointer, ulongs.size, ulongs.data);
				} else {
					char* data = arrayIn.data;
					const uint32_t elemSize = memberOut.getArrayMemberSize();
					writePodArray(arrayIn.size, elemSize, &dataOut, data, subType);
				}
				break;
			}
			}
			break;
		}
		case hkClassMember::TYPE_STRUCT: {
			const hkClass* structIn = memberIn.getClass();
			const hkClass* structOut = memberOut.getClass();
			if (structIn && structOut) {
				const int32_t structStart = classStart + memberOut.m_offset;
				const int32_t srcCount = getNumElements(memberIn);
				const int32_t dstCount = getNumElements(memberOut);
				const int32_t numStructs = hkMin2(srcCount, dstCount);
				for (int32_t i = 0; i < numStructs; ++i) {
					const char* objectIn = addrIn + structIn->getObjectSize() * i;
					const int32_t objectStart = structStart + structOut->getObjectSize() * i;
					saveExtras(objectIn, *structIn, dataOut, *structOut, objectStart, fixups, level);
				}
			}
			break;
		}
		case hkClassMember::TYPE_HOMOGENEOUSARRAY: {
			const ObjectCopier_DummyHomogeneousArray& arrayIn =
				*reinterpret_cast<const ObjectCopier_DummyHomogeneousArray*>(addrIn);
			const hkClass* klass = arrayIn.klass;
			if (klass) {
				fixups.addGlobal(classStart + memberOut.m_offset, const_cast<hkClass*>(klass), &hkClassClass, 1);
				hkStreamWriter* writer = dataOut.getStreamWriter();
				const int32_t start = writer->tell();
				fixups.addLocal(classStart + m_layoutOut.m_rules.m_bytesInPointer + memberOut.m_offset, start);
				char* data = static_cast<char*>(arrayIn.data);
				for (int32_t i = 0; i < arrayIn.size; ++i) {
					saveBody(data + klass->getObjectSize() * i, *klass, dataOut, *klass);
				}
				for (int32_t i = 0; i < arrayIn.size; ++i) {
					const int32_t objectStart = start + klass->getObjectSize() * i;
					const char* objectIn = data + klass->getObjectSize() * i;
					saveExtras(objectIn, *klass, dataOut, *klass, objectStart, fixups, level);
				}
			}
			break;
		}
		case hkClassMember::TYPE_VARIANT: {
			// A C-style array of variants in the source layout: object and class pointers of m_layoutIn's pointer
			// size, alternating.
			const int32_t srcCount = getNumElements(memberIn);
			const int32_t dstCount = getNumElements(memberOut);
			const int32_t numVariants = hkMin2(srcCount, dstCount);
			for (int32_t i = 0; i < numVariants; ++i) {
				const int32_t pointerSizeIn = m_layoutIn.m_rules.m_bytesInPointer;
				void* object = *reinterpret_cast<void* const*>(addrIn + 2 * (pointerSizeIn * i));
				const hkClass* klass = *reinterpret_cast<const hkClass* const*>(addrIn + pointerSizeIn * (2 * i + 1));
				if (object) {
					fixups.addGlobal(classStart + memberOut.m_offset + 2 * (m_layoutOut.m_rules.m_bytesInPointer * i),
						object, klass, 0);
				}
				if (klass) {
					fixups.addGlobal(classStart + m_layoutOut.m_rules.m_bytesInPointer * (2 * i + 1) +
							memberOut.m_offset,
						const_cast<hkClass*>(klass), &hkClassClass, 0);
				}
			}
			break;
		}
		case hkClassMember::TYPE_CSTRING: {
			char* string = *reinterpret_cast<char* const*>(addrIn);
			if (string) {
				saveCstring(string, classStart + memberOut.m_offset, &dataOut, &fixups);
			}
			break;
		}
		case hkClassMember::TYPE_STRINGPTR: {
			// An hkStringPtr is its string's address with the owned flag in bit 0.
			const hkUlong stringAndFlag = *reinterpret_cast<const hkUlong*>(addrIn);
			if (stringAndFlag & ~static_cast<hkUlong>(1)) {
				saveCstring(reinterpret_cast<char*>(stringAndFlag & ~static_cast<hkUlong>(1)),
					classStart + memberOut.m_offset, &dataOut, &fixups);
			}
			break;
		}
		default: {
			char buf[512];
			FNVD_INLINED_LOCAL(hkErrStream, error, std::nullopt, std::nullopt);
			::new (&error) hkErrStream(buf, 512);
			error << "Unknown class member found during write of data.";
			if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x641E3E05, buf, ".\\Copier\\hkObjectCopier.cpp",
					931)) {
				__builtin_debugtrap();
			}
			FNVD_DESTRUCT(hkOstream)(&error);
			break;
		}
		}
		objectCopierPadUp(dataOut.getStreamWriter(), 16);
	}
}

int32_t hkObjectCopier::getNumElements(const hkClassMember& member) {
	if (member.getCstyleArraySize()) {
		return member.getCstyleArraySize();
	} else {
		return 1;
	}
}

// Writes nelem plain members of msize bytes each. The vector and matrix types go out as their 4-byte reals, so that a
// byte-swapping archive reverses each real; an unknown type is reported. writeArrayGeneric takes the SDK's int
// element size; the exe computes it unsigned (shr).
void hkObjectCopier::writePodArray(int32_t nelem, uint32_t msize, hkOArchive* oa, char* data, int32_t mtype) {
	FNVD_INLINED_LOCAL(hkErrStream, error, std::nullopt, std::nullopt);
	switch (mtype) {
	case hkClassMember::TYPE_BOOL:
	case hkClassMember::TYPE_CHAR:
	case hkClassMember::TYPE_INT8:
	case hkClassMember::TYPE_UINT8:
	case hkClassMember::TYPE_INT16:
	case hkClassMember::TYPE_UINT16:
	case hkClassMember::TYPE_INT32:
	case hkClassMember::TYPE_UINT32:
	case hkClassMember::TYPE_INT64:
	case hkClassMember::TYPE_UINT64:
	case hkClassMember::TYPE_REAL:
	case hkClassMember::TYPE_HALF:
		oa->writeArrayGeneric(data, static_cast<int32_t>(msize), nelem);
		break;
	case hkClassMember::TYPE_VECTOR4:
	case hkClassMember::TYPE_QUATERNION:
	case hkClassMember::TYPE_MATRIX3:
	case hkClassMember::TYPE_ROTATION:
	case hkClassMember::TYPE_QSTRANSFORM:
	case hkClassMember::TYPE_MATRIX4:
	case hkClassMember::TYPE_TRANSFORM:
		nelem *= msize >> 2;
		msize = 4;
		oa->writeArrayGeneric(data, static_cast<int32_t>(msize), nelem);
		break;
	default: {
		char buf[512];
		::new (&error) hkErrStream(buf, 512);
		error << "Unknown class member found during write of plain data array.";
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x747E1E03, buf, ".\\Copier\\hkObjectCopier.cpp",
				251)) {
			__builtin_debugtrap();
		}
		FNVD_DESTRUCT(hkOstream)(&error);
		break;
	}
	}
}

// The class registry's finish-loaded-object callback of the class at 0xE02700: the null check, then the class's
// out-of-line finish constructor (cdecl, 2 stack arguments, 0xE02700). IDA name; the PDB's finishLoadedObject<Class>
// name of this address is not resolved here.
void finishLoadedObjecthkAlignSceneToNodeOptions(void* p, int32_t finishing) {
	void* apMemory = static_cast<void*>(p);
	hkFinishLoadedObjectFlag aFlag;
	aFlag.m_finishing = finishing;
	if (!apMemory) {
		return;
	}
	::new (apMemory) hkAlignSceneToNodeOptions(aFlag);
}

// The SDK's m_global.expandOne() inlined: the heap is fetched first, the array grown by _reserveMore when full.
void hkRelocationInfo::addGlobal(int32_t fromOffset, void* toAddr, const hkClass* klass, int32_t related) {
	// EXE-INLINED none: hkArray<hkRelocationInfo::Global, hkContainerHeapAllocator>::expandOne
	hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
	if (m_global.m_size == (m_global.m_capacityAndFlags & hkArrayBase<Global>::CAPACITY_MASK)) {
		hkArrayUtil::_reserveMore(*pRouter->m_heap, &m_global, sizeof(Global));
	}
	Global* pGlobal = &m_global.m_data[m_global.m_size];
	pGlobal->m_fromOffset = fromOffset;
	pGlobal->m_toAddress = toAddr;
	pGlobal->m_toClass = klass;
	pGlobal->m_related = related;
	++m_global.m_size;
}
