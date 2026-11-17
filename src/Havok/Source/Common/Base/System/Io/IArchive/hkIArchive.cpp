#include "Havok/Source/Common/Base/System/Io/IArchive/hkIArchive.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Io/FileSystem/hkFileSystem.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/System/Io/Reader/Memory/hkMemoryStreamReader.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

void byteswap(char& a1, char& r_eax) {
	char v2 = a1;
	a1 = r_eax;
	r_eax = v2;
}

hkIArchive::hkIArchive(const void* mem, int32_t memSize, hkBool byteswap) : m_byteSwap(byteswap) {
	m_streamReader = new (hkReferencedObject::newObject(sizeof(hkMemoryStreamReader)))
		hkMemoryStreamReader(mem, memSize, hkMemoryStreamReader::MEMORY_INPLACE);
}

hkIArchive::hkIArchive(const char* apFileName, hkBool abByteSwap)
	: m_streamReader(hkFileSystem::s_instance->openReader(apFileName)), m_byteSwap(abByteSwap) {}

hkIArchive::hkIArchive(hkStreamReader* sb, hkBool bs) : m_streamReader(sb), m_byteSwap(bs) {
	sb->addReference();
}

hkIArchive::~hkIArchive() {
	if (m_streamReader) {
		m_streamReader->removeReference();
	}
}

void hkIArchive::setStreamReader(hkStreamReader* a1) {
	a1->addReference();
	m_streamReader->removeReference();
	m_streamReader = a1;
}

hkStreamReader* hkIArchive::getStreamReader() {
	return m_streamReader;
}

void hkIArchive::readArrayGeneric(void* array, int32_t elemsize, int32_t arraySize) {
	int32_t v4 = arraySize;
	m_streamReader->read(array, arraySize * elemsize);
	if (m_byteSwap.m_bool) {
		char* v6 = static_cast<char*>(array);
		if (elemsize == 2) {
			if (arraySize > 0) {
				do {
					char v15 = *v6;
					*v6 = v6[1];
					v6[1] = v15;
					v6 += 2;
					--v4;
				} while (v4);
			}
		} else {
			if (elemsize == 4) {
				if (arraySize > 0) {
					char* v12 = static_cast<char*>(array) + 2;
					do {
						char v13 = *(v12 - 2);
						*(v12 - 2) = v12[1];
						v12[1] = v13;
						char v14 = *(v12 - 1);
						*(v12 - 1) = *v12;
						*v12 = v14;
						v12 += 4;
						--v4;
					} while (v4);
				}
			} else {
				if (elemsize == 8 && arraySize > 0) {
					char* v7 = static_cast<char*>(array) + 6;
					do {
						char v8 = *(v7 - 6);
						*(v7 - 6) = v7[1];
						v7[1] = v8;
						char v9 = *(v7 - 5);
						*(v7 - 5) = *v7;
						*v7 = v9;
						char v10 = *(v7 - 4);
						*(v7 - 4) = *(v7 - 1);
						*(v7 - 1) = v10;
						char v11 = *(v7 - 3);
						*(v7 - 3) = *(v7 - 2);
						*(v7 - 2) = v11;
						v7 += 8;
						--v4;
					} while (v4);
				}
			}
		}
	}
}

int32_t hkIArchive::readRaw(void* buf, int32_t nbytes) {
	return m_streamReader->read(buf, nbytes);
}

// On a short read the stream reader leaves f unwritten, and it is returned uninitialised.
float hkIArchive::readFloat32() {
	float f;
	readArrayGeneric(&f, sizeof(float), 1);
	return f;
}

hkBool* hkIArchive::isOk(hkBool* apResult) const {
	m_streamReader->isOk(apResult);
	return apResult;
}
