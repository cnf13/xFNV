#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"

#include <cstdarg>

// An empty string points at the shared empty string instead of owning a buffer.
hkStringPtr::hkStringPtr() : m_stringAndFlag(s_emptyString) {}

hkStringPtr::hkStringPtr(hkFinishLoadedObjectFlag flag) {}

// The shared empty string is stored before each assignment, so that the assignment cannot free the uninitialised
// field.
hkStringPtr::hkStringPtr(const char* apcString) : m_stringAndFlag(s_emptyString) {
	::assign(-1, apcString, this);
}

hkStringPtr::hkStringPtr(const char* apcString, int32_t aiLength) : m_stringAndFlag(s_emptyString) {
	::assign(aiLength, apcString, this);
}

hkStringPtr::hkStringPtr(const hkStringPtr& arOther) : m_stringAndFlag(s_emptyString) {
	::assign(-1, arOther.cString(), this);
}

// Frees the owned string, if any.
hkStringPtr::~hkStringPtr() {
	::assign(-1, nullptr, this);
}

// Copies s into an owned string, freeing the old one.
hkStringPtr& hkStringPtr::operator=(const char* s) {
	::assign(-1, s, this);
	return *this;
}

hkStringPtr& hkStringPtr::operator=(const hkStringPtr& s) {
	::assign(-1, s.cString(), this);
	return *this;
}

// The string's length; 0 for a null string.
int32_t hkStringPtr::getLength() const {
	const char* pcString = cString();
	if (pcString) {
		return hkString::strLen(pcString);
	}
	return 0;
}

// Replaces the string with a copy of the first len characters of s.
void hkStringPtr::set(const char* s, int32_t len) {
	::assign(len, s, this);
}

// File-static copy of a C string; assign calls it.
char* strDup(const char* src) {
	const uint32_t uiBytes = hkString::strLen(src) + 1;
	char* pcCopy = static_cast<char*>(hkMemoryRouter::easyAlloc(*hkMemoryRouter::getInstance().m_heap,
		static_cast<int32_t>(uiBytes)));
	hkString::strCpy(pcCopy, src);
	return pcCopy;
}

// Bounded copy of src, at most maxlen characters.
char* strNdup(const char* src, int32_t maxlen) {
	int32_t iLength = static_cast<int32_t>(hkString::strLen(src));
	if (iLength > maxlen) {
		iLength = maxlen;
	}
	char* pcCopy = static_cast<char*>(hkMemoryRouter::easyAlloc(*hkMemoryRouter::getInstance().m_heap, iLength + 1));
	hkString::strNcpy(pcCopy, src, iLength);
	pcCopy[iLength] = 0;
	return pcCopy;
}

const char* assign(int32_t aiLength, const char* apcString, hkStringPtr* apDest) {
	const char* pcOld = apDest->m_stringAndFlag;
	if (apDest->cString() == apcString) {
		return pcOld;
	}
	// The field is a tagged pointer: OWNED_FLAG in its low bit marks a string apDest owns.
	if (reinterpret_cast<uintptr_t>(pcOld) & hkStringPtr::OWNED_FLAG) {
		// An owned string stores the block easyAlloc returned plus OWNED_FLAG, so the block is pcOld - 1.
		hkMemoryRouter::easyFree(*hkMemoryRouter::getInstance().m_heap, const_cast<char*>(pcOld) - 1);
	}
	if (apcString) {
		if (aiLength == -1) {
			apDest->m_stringAndFlag = strDup(apcString) + 1;
		} else {
			apDest->m_stringAndFlag = strNdup(apcString, aiLength) + 1;
		}
	} else {
		apDest->m_stringAndFlag = s_emptyString;
	}
	return apDest->m_stringAndFlag;
}

// Formats into a stack buffer that grows on the heap when the text does not fit, then stores the text in apDest.
// Assigns the text twice, as the game does, then frees the buffer if it had grown.
uint32_t printfStringPtr(hkStringPtr* apDest, const char* apcFormat, ...) {
	char acBuffer[256];
	// The local array is untyped: its address is handed to hkArrayUtil and the text is read through it.
	hkArrayBase<void> kBuffer;
	kBuffer.m_data = acBuffer;
	kBuffer.m_size = 0;
	kBuffer.m_capacityAndFlags = 0x800000FFu;
	va_list arguments;
	va_start(arguments, apcFormat);
	for (;;) {
		const int32_t iCapacity = static_cast<int32_t>(kBuffer.m_capacityAndFlags & hkArrayBase<void>::CAPACITY_MASK);
		const int32_t iLength =
			hkString::vsnprintf(static_cast<char*>(kBuffer.m_data), iCapacity, apcFormat, arguments);
		if (iLength >= 0 && iLength < iCapacity) {
			break;
		}
		int32_t iRequired = iLength < 0 ? 2 * iCapacity : iLength;
		if (iRequired < 2 * iCapacity) {
			iRequired = 2 * iCapacity;
		}
		hkArrayUtil::_reserve(hkMemoryRouter::getInstance().m_heap, &kBuffer, iRequired, 1);
	}
	(*apDest) = static_cast<const char*>(kBuffer.m_data);
	(*apDest) = static_cast<const char*>(kBuffer.m_data);
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	const uint32_t uiFlags = kBuffer.m_capacityAndFlags;
	kBuffer.m_size = 0;
	if (static_cast<int32_t>(uiFlags) >= 0) {
		pHeap->bufFree(kBuffer.m_data, static_cast<int32_t>(uiFlags & hkArrayBase<void>::CAPACITY_MASK));
	}
	return uiFlags;
}
