#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

class hkBool;
class hkVector4;
struct hkpAgentEntry;
class hkpProcessCollisionInput;
struct hkpSerializedAgentNnEntry;
struct hkpSerializedTrack1nInfo;

// Havok's big-endian conversion of the contact points a save records (the PDB's hkpSaveContactPointsEndianUtil):
// static functions only, no instance members. The original source is Havok's
// Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpSaveContactPointsEndianUtil.cpp.
class hkpSaveContactPointsEndianUtil {
public:
	// Reverses the bytes of each of the vector's four dwords (HK_CALL cdecl, 0xE05DE0, ret 0; the PDB's
	// swapEndianType(hkVector4&)). The PDB's twelve other swapEndianType overloads (hkUint16&, hkUint32&, hkReal&,
	// the agent entries, the manifolds, the caches and hkpContactPointProperties*) are not declared yet.
	static void swapEndianType(hkVector4& arVector);
	// Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
	static void swapEndianTypeInEntry(const hkpProcessCollisionInput* input, hkpAgentEntry* entry,
		hkpSerializedTrack1nInfo& serializedTrack, hkBool isNnEntry, int32_t& sizeOfThisEntryOut, bool& trackUsedOut);
	// Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
	static void swapEndianTypeIn1nTrack(const hkpProcessCollisionInput* input,
		hkpSerializedTrack1nInfo& serializedTrack);
	// Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
	static void swapEndianTypeInCollisionEntry(const hkpProcessCollisionInput* input,
		hkpSerializedAgentNnEntry& serializedEntry);
};
