#pragma once

#include "Havok/Source/Common/Base/System/Io/FileSystem/hkFileSystem.h"

#include <stdint.h>

// Havok's file system for the game: it opens readers and writers that go through the game's BSFile. It has no data
// of its own.
class HavokStreambufFactory : public hkFileSystem {
public:
	HavokStreambufFactory();
	~HavokStreambufFactory() override;

	hkStreamReader* openReader(const char* apName) override;
	hkStreamWriter* openWriter(const char* apName) override;

	// The deleting destructor frees through hkReferencedObject::deleteObject.
	static void operator delete(void* apMemory);
};
