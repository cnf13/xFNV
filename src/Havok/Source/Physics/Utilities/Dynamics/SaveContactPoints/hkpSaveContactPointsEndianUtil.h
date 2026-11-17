#pragma once

#include <stdint.h>

class hkBool;
struct hkpAgentEntry;
class hkpProcessCollisionInput;
struct hkpSerializedAgentNnEntry;
struct hkpSerializedTrack1nInfo;
class hkVector4;

// Havok's big-endian conversion of the contact points a save records: static functions only, no instance members.
class hkpSaveContactPointsEndianUtil {
public:
    // Reverses the bytes of each of the vector's four 32-bit components. The other swapEndianType overloads (hkUint16&,
    // hkUint32&, hkReal&, the agent entries, the manifolds, the caches and hkpContactPointProperties*) are not
    // declared.
    static void swapEndianType(hkVector4& arVector);
    // Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
    static void swapEndianTypeInEntry(
        const hkpProcessCollisionInput* input,
        hkpAgentEntry* entry,
        hkpSerializedTrack1nInfo& serializedTrack,
        hkBool isNnEntry,
        int32_t& sizeOfThisEntryOut,
        bool& trackUsedOut
    );
    // Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
    static void
    swapEndianTypeIn1nTrack(const hkpProcessCollisionInput* input, hkpSerializedTrack1nInfo& serializedTrack);
    // Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
    static void
    swapEndianTypeInCollisionEntry(const hkpProcessCollisionInput* input, hkpSerializedAgentNnEntry& serializedEntry);
};
