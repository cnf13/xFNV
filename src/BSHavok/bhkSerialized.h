#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Reconstruction.hpp"

// The serialized forms of the Havok creation infos. A model file holds a bhk block's creation info as the 32-bit
// program's structure, which that program reads raw over its own. Each such structure has a serialized form beside
// it (`X::Serialized`): its file layout in fixed-width members, pinned by static_asserts on every width, and the
// native structure is filled from it member by member (`X::ReadSerialized`). A loader reads the serialized form's
// size (`arStream.Istr()->BinaryRead(&kForm, sizeof(kForm), nullptr, 0)`), never the native structure's, so the same
// code is right on the 32-bit and the 64-bit programs.

// A pointer member of the 32-bit image: 4 bytes in the file. Its value is an address in the program that wrote the
// file and means nothing here; the loaders set the native pointer from a link ID or leave it null.
using bhkSerializedPointer = uint32_t;

// An hkArray member of the 32-bit image: its data address, size and capacity word (12 bytes). The files hold empty
// arrays that do not own their storage (capacity word DONT_DEALLOCATE_FLAG); the elements, when there are any, follow
// in the stream on their own.
struct bhkSerializedArray {
	// The native array: no storage (the file's address is dropped), the file's size and capacity word.
	template <class T> void ToNative(hkArray<T>& arArray) const {
		arArray.m_data = nullptr;
		arArray.m_size = m_size;
		arArray.m_capacityAndFlags = m_capacityAndFlags;
	}

	bhkSerializedPointer m_data;
	int32_t m_size;
	int32_t m_capacityAndFlags;
};

static_assert(sizeof(bhkSerializedArray) == 0xC);
