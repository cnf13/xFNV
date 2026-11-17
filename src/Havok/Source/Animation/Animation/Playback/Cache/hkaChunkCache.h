#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkOstream;

// The cache of the blocks an animation format samples from: a block several samplers ask for is decompressed once, and
// locked while it is read. Every operation is pure and is only called through the vtable.
class hkaChunkCache : public hkReferencedObject {
public:
	// What a lock or flush does to a key.
	enum ManipulationType : int32_t {
		MANIPULATE_RETRIEVE = 0,
		MANIPULATE_LOCK,
		MANIPULATE_UNLOCK,
		MANIPULATE_FLUSH,
		MANIPULATE_LOCKWRITE,
		MANIPULATE_UNLOCKWRITE,
		MANIPULATE_ISLOCKEDWRITE,
	};

	// The chunk stored under key, null when there is none.
	virtual const uint8_t* retrieveChunk(uint32_t key, uint32_t chunkSize) = 0;
	// A chunk to fill under key.
	virtual uint8_t* allocateChunk(uint32_t key, uint32_t chunkSize) = 0;
	// Drops the chunk stored under key; returns whether there was one.
	virtual hkBool flushKey(uint32_t key, uint32_t chunkSize) = 0;
	// Locks the chunk stored under key for reading; returns whether it is there.
	virtual hkBool lockKeyForRead(uint32_t key, uint32_t chunkSize) = 0;
	// Releases a read lock; returns whether the key was locked.
	virtual hkBool unlockKeyForRead(uint32_t key, uint32_t chunkSize) = 0;
	// Locks the chunk stored under key for writing.
	virtual hkBool lockKeyForWrite(uint32_t key, uint32_t chunkSize) = 0;
	virtual hkBool unlockKeyForWrite(uint32_t key, uint32_t chunkSize) = 0;
	virtual hkBool isKeyLockedForWrite(uint32_t key, uint32_t chunkSize) = 0;
	virtual void enterCriticalSection() = 0;
	virtual void leaveCriticalSection() = 0;
	// Writes the cache's statistics; returns whether there are any.
	virtual hkBool printCacheStats(hkOstream* oStream) const = 0;
};
