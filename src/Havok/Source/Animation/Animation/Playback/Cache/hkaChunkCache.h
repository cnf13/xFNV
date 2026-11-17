#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

class hkOstream;

// The cache of the blocks an animation format samples from: a block several samplers ask for is decompressed once,
// and locked while it is read. Every operation is pure and the exe only calls them through the vtable: its slots
// follow hkReferencedObject's three, so flushKey is slot 5, which hkaWaveletCompressedAnimation::clearAllCacheKeys
// (0xE2FA00) calls as `(*(*cache + 20))(cache, result, key, size)`, the hkBool result passed through a hidden
// pointer.
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

	// The chunk stored under key, null when there is none (slot 3).
	virtual const uint8_t* retrieveChunk(uint32_t key, uint32_t chunkSize) = 0;
	// A chunk to fill under key (slot 4).
	virtual uint8_t* allocateChunk(uint32_t key, uint32_t chunkSize) = 0;
	// Drops the chunk stored under key; whether there was one (slot 5).
	virtual hkBool flushKey(uint32_t key, uint32_t chunkSize) = 0;
	// Locks the chunk stored under key for reading; whether it is there (slot 6).
	virtual hkBool lockKeyForRead(uint32_t key, uint32_t chunkSize) = 0;
	// Releases a read lock; whether the key was locked (slot 7).
	virtual hkBool unlockKeyForRead(uint32_t key, uint32_t chunkSize) = 0;
	// Locks the chunk stored under key for writing (slot 8).
	virtual hkBool lockKeyForWrite(uint32_t key, uint32_t chunkSize) = 0;
	// Releases a write lock (slot 9).
	virtual hkBool unlockKeyForWrite(uint32_t key, uint32_t chunkSize) = 0;
	// Whether the key is write-locked (slot 10).
	virtual hkBool isKeyLockedForWrite(uint32_t key, uint32_t chunkSize) = 0;
	// Enters the cache's lock (slot 11).
	virtual void enterCriticalSection() = 0;
	// Leaves the cache's lock (slot 12).
	virtual void leaveCriticalSection() = 0;
	// Writes the cache's statistics; whether there are any (slot 13).
	virtual hkBool printCacheStats(hkOstream* oStream) const = 0;
};
