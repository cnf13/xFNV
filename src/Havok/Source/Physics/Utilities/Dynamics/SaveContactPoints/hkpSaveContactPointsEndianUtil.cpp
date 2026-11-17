#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpSaveContactPointsEndianUtil.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnMachine.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h"
#include "Havok/Source/Physics/Dynamics/SaveContactPoints/hkpSaveContactPointsUtil.h"
#include "Havok/Source/Physics/Utilities/Dynamics/SaveContactPoints/hkpSerializedAgentNnEntry.h"

static uint8_t* swapEndianStreamCommand(uint8_t* a1);
static uint8_t swapEndianContactPointCommand(uint8_t* a1);
// Not named in the PDB or IDA; name chosen here. The SDK's inline swapEndianType(hkpAgent1nMachineTimEntry*): the
// big-endian form of the entry's shape keys, separating-normal time and vector (swapEndianTypeInEntry 0xE06070 calls
// it on 1n entries of stream commands 4 to 6 and 12 to 14; it tail-calls swapEndianType, its eax unused).
static void swapEndianType1nTimEntry(hkpAgent1nMachineTimEntry* a1);
// Not named in the PDB or IDA; name chosen here. The SDK's inline swapEndianType(hkpAgentNnMachineTimEntry*): the
// big-endian form of the entry's separating-normal time and vector (swapEndianTypeInEntry 0xE06070 calls it on Nn
// entries of stream commands 4 to 6 and 12 to 14).
static void swapEndianTypeNnTimEntry(hkpAgentNnMachineTimEntry* a1);
static char swapEndianStreamCommandHeader(uint8_t* a1);
// Not named in the PDB or IDA; name chosen here. Swaps the bytes of the four 16-bit words at a1 (the exe passes an
// agent entry's address, 0xE061CE).
static int32_t swapEndian4Shorts(uint8_t* a1);
static int32_t swapEndianVariableEntry(uint8_t* a1);

static void swapEndianType1nTimEntry(hkpAgent1nMachineTimEntry* a1) {
	// The SDK's reverseBytes on the two shape keys and the time, as raw bytes, then the separating normal.
	uint8_t* pBytes = reinterpret_cast<uint8_t*>(&a1->m_shapeKeyPair);
	for (int32_t i = 0; i < 12; i += 4) {
		uint8_t v1 = pBytes[i];
		pBytes[i] = pBytes[i + 3];
		pBytes[i + 3] = v1;
		uint8_t v2 = pBytes[i + 1];
		pBytes[i + 1] = pBytes[i + 2];
		pBytes[i + 2] = v2;
	}
	hkpSaveContactPointsEndianUtil::swapEndianType(a1->m_separatingNormal);
}

static uint8_t* swapEndianStreamCommand(uint8_t* a1) {
	int8_t v2 = (int8_t)a1[4];
	a1[4] = a1[7];
	a1[7] = (uint8_t)v2;
	int8_t v3 = (int8_t)a1[5];
	a1[5] = a1[6];
	a1[6] = (uint8_t)v3;
	int8_t v4 = (int8_t)a1[8];
	a1[8] = a1[11];
	a1[11] = (uint8_t)v4;
	int8_t v5 = (int8_t)a1[9];
	a1[9] = a1[10];
	a1[10] = (uint8_t)v5;
	return a1;
}

static uint8_t swapEndianContactPointCommand(uint8_t* a1) {
	uint8_t* v2;
	uint8_t v3;
	uint8_t v6;
	uint8_t v7;
	int32_t v1 = 0;
	if (a1[33]) {
		v2 = a1 + 2;
		do {
			v3 = *v2;
			*v2 = v2[1];
			v2[1] = v3;
			++v1;
			v2 += 4;
		} while (v1 < ((int32_t)a1[33]));
	}
	uint8_t* v4 = a1 + 51;
	int32_t v5 = 4;
	do {
		v6 = *(v4 - 3);
		*(v4 - 3) = *v4;
		*v4 = v6;
		v7 = *(v4 - 2);
		*(v4 - 2) = *(v4 - 1);
		*(v4 - 1) = v7;
		v4 += 4;
		--v5;
	} while (v5);
	uint8_t v8 = a1[36];
	a1[36] = a1[39];
	a1[39] = v8;
	uint8_t result = a1[37];
	a1[37] = a1[38];
	a1[38] = result;
	return result;
}

static void swapEndianTypeNnTimEntry(hkpAgentNnMachineTimEntry* a1) {
	// The SDK's reverseBytes on the float: its four bytes reversed in place, as raw bytes.
	uint8_t* pTime = reinterpret_cast<uint8_t*>(&a1->m_timeOfSeparatingNormal);
	uint8_t v1 = pTime[0];
	pTime[0] = pTime[3];
	pTime[3] = v1;
	uint8_t v2 = pTime[1];
	pTime[1] = pTime[2];
	pTime[2] = v2;
	hkpSaveContactPointsEndianUtil::swapEndianType(a1->m_separatingNormal);
}

static char swapEndianStreamCommandHeader(uint8_t* a1) {
	int32_t v1 = 0;
	uint8_t* v2 = a1 + 11;
	do {
		uint8_t v3 = a1[2 * v1];
		a1[2 * v1] = a1[2 * v1 + 1];
		a1[2 * v1 + 1] = v3;
		uint8_t v4 = *(v2 - 3);
		*(v2 - 3) = *v2;
		*v2 = v4;
		uint8_t v5 = *(v2 - 2);
		*(v2 - 2) = *(v2 - 1);
		*(v2 - 1) = v5;
		++v1;
		v2 += 4;
	} while (v1 < 3);
	uint8_t v6 = a1[20];
	a1[20] = a1[23];
	a1[23] = v6;
	uint8_t v7 = a1[21];
	a1[21] = a1[22];
	a1[22] = v7;
	uint8_t v8 = a1[24];
	a1[24] = a1[27];
	a1[27] = v8;
	char result = a1[25];
	a1[25] = a1[26];
	a1[26] = result;
	return result;
}

static int32_t swapEndian4Shorts(uint8_t* a1) {
	int32_t result;
	for (result = 0; result < 4; ++result) {
		char v2 = a1[2 * result];
		a1[2 * result] = a1[2 * result + 1];
		a1[2 * result + 1] = v2;
	}
	return result;
}

static int32_t swapEndianVariableEntry(uint8_t* a1) {
	int32_t v1 = 0;
	if (a1[2]) {
		uint8_t* v2 = a1 + 6;
		do {
			uint8_t v3 = *v2;
			*v2 = v2[1];
			v2[1] = v3;
			++v1;
			v2 += 8;
		} while (v1 < a1[2]);
	}
	int32_t v4 = *a1 + a1[1];
	int32_t result = 0;
	for (uint8_t* i = &a1[8 * a1[2] + 4]; result < v4; ++result) {
		uint8_t v7 = i[2 * result];
		i[2 * result] = i[2 * result + 1];
		i[2 * result + 1] = v7;
	}
	return result;
}

// Each of the four components' bytes reversed in place, as raw bytes.
void hkpSaveContactPointsEndianUtil::swapEndianType(hkVector4& arVector) {
	uint8_t* pBytes = reinterpret_cast<uint8_t*>(&arVector) + 3;
	int32_t iCount = 4;
	do {
		uint8_t ucByte = *(pBytes - 3);
		*(pBytes - 3) = *pBytes;
		*pBytes = ucByte;
		ucByte = *(pBytes - 2);
		*(pBytes - 2) = *(pBytes - 1);
		*(pBytes - 1) = ucByte;
		pBytes += 4;
		--iCount;
	} while (iCount);
}

void hkpSaveContactPointsEndianUtil::swapEndianTypeInEntry(const hkpProcessCollisionInput* input, hkpAgentEntry* entry,
	hkpSerializedTrack1nInfo& serializedTrack, hkBool isNnEntry, int32_t& sizeOfThisEntryOut, bool& trackUsedOut) {
	hkpAgentEntry* v6;
	trackUsedOut = 0;
	switch (entry->m_streamCommand) {
	case 0:
	case 1: {
		sizeOfThisEntryOut = 16;
		break;
	}
	case 2:
	case 3:
	case 10:
	case 11: {
		if (isNnEntry.m_bool) {
			v6 = entry + 8;
		} else {
			v6 = entry + 4;
			swapEndianStreamCommand(&entry->m_streamCommand);
		}
		goto LABEL_8;
	}
	case 4:
	case 5:
	case 6:
	case 12:
	case 13:
	case 14: {
		if (isNnEntry.m_bool) {
			v6 = entry + 12;
			swapEndianTypeNnTimEntry(static_cast<hkpAgentNnMachineTimEntry*>(entry));
		} else {
			v6 = entry + 8;
			swapEndianType1nTimEntry(static_cast<hkpAgent1nMachineTimEntry*>(entry));
		}
	LABEL_8:;
		hkpSerializedAgentNnEntry::SerializedAgentType SerializedAgentType =
			hkpSaveContactPointsUtil::getSerializedAgentType(input->m_dispatcher->m_agent3Func[entry->m_agentType]
					.m_processFunc);
		sizeOfThisEntryOut = entry->m_size;
		switch (SerializedAgentType) {
		case hkpSerializedAgentNnEntry::BOX_BOX_AGENT3: {
			swapEndianContactPointCommand(&v6->m_streamCommand);
			return;
		}
		case hkpSerializedAgentNnEntry::CAPSULE_TRIANGLE_AGENT3: {
			swapEndianStreamCommandHeader(&v6->m_streamCommand);
			return;
		}
		case hkpSerializedAgentNnEntry::PRED_GSK_AGENT3:
		case hkpSerializedAgentNnEntry::PRED_GSK_CYLINDER_AGENT3: {
			goto LABEL_12;
		}
		case hkpSerializedAgentNnEntry::CONVEX_LIST_AGENT3: {
			if ((v6[2].m_size & 32) == 0) {
				goto LABEL_13;
			}
		LABEL_12:;
			swapEndian4Shorts(&v6->m_streamCommand);
			swapEndianVariableEntry(&(&v6[3])->m_streamCommand);
			break;
		}
		case hkpSerializedAgentNnEntry::LIST_AGENT3:
		case hkpSerializedAgentNnEntry::BV_TREE_AGENT3:
		case hkpSerializedAgentNnEntry::COLLECTION_COLLECTION_AGENT3:
		case hkpSerializedAgentNnEntry::COLLECTION_AGENT3: {
		LABEL_13:;
			hkpSaveContactPointsEndianUtil::swapEndianTypeIn1nTrack(input, serializedTrack);
			trackUsedOut = 1;
			break;
		}
		default: {
			return;
		}
		}
		break;
	}
	default: {
		return;
	}
	}
}

// Walks every entry of every saved sector, handing each the saved sub track whose first entry lies at or after it
// (the sub tracks are in sector order). The exe keeps the entry's end as an address and compares unsigned. A
// sector's data is untyped bytes holding agent entries of varying sizes, which the exe steps through as raw memory.
void hkpSaveContactPointsEndianUtil::swapEndianTypeIn1nTrack(const hkpProcessCollisionInput* input,
	hkpSerializedTrack1nInfo& serializedTrack) {
	hkpSerializedSubTrack1nInfo* pSubTrack;
	int32_t iSubTrack;
	if (serializedTrack.m_subTracks.m_size) {
		pSubTrack = serializedTrack.m_subTracks.m_data[0];
		iSubTrack = 0;
	} else {
		iSubTrack = -1;
		pSubTrack = nullptr;
	}
	for (int32_t iSector = 0; iSector < serializedTrack.m_sectors.m_size; ++iSector) {
		hkpAgent1nSector* pSector = serializedTrack.m_sectors.m_data[iSector];
		const uintptr_t uiEnd = reinterpret_cast<uintptr_t>(pSector->m_data) + pSector->m_bytesAllocated;
		uint8_t* pEntry = pSector->m_data;
		if (reinterpret_cast<uintptr_t>(pEntry) < uiEnd) {
			hkBool isNnEntry(false);
			int32_t iNext = iSubTrack + 1;
			do {
				int32_t iSizeOfEntry = 0;
				if (pSubTrack) {
					if (pSubTrack->m_sectorIndex != iSector ||
						pSubTrack->m_offsetInSector < static_cast<int32_t>(pEntry - pSector->m_data)) {
						if (iNext >= serializedTrack.m_subTracks.m_size) {
							pSubTrack = nullptr;
						} else {
							pSubTrack = serializedTrack.m_subTracks.m_data[iSubTrack + 1];
							++iSubTrack;
							++iNext;
						}
					}
				}
				bool bTrackUsed;
				// The exe passes the current sub track, null when none is left (the callee then reads none of it).
				hkpSaveContactPointsEndianUtil::swapEndianTypeInEntry(input, reinterpret_cast<hkpAgentEntry*>(pEntry),
					*pSubTrack, isNnEntry, iSizeOfEntry, bTrackUsed);
				pEntry += iSizeOfEntry;
			} while (reinterpret_cast<uintptr_t>(pEntry) < uiEnd);
		}
	}
}

// The entry's own agent data as an Nn entry, then each saved contact point property: its first three dwords, the
// dword at +0x10 and the user datas after it reversed byte by byte.
void hkpSaveContactPointsEndianUtil::swapEndianTypeInCollisionEntry(const hkpProcessCollisionInput* input,
	hkpSerializedAgentNnEntry& serializedEntry) {
	int32_t iSizeOfEntry;
	bool bTrackUsed;
	// m_nnEntryData is the raw bytes of the saved agent entry.
	hkpSaveContactPointsEndianUtil::swapEndianTypeInEntry(input,
		reinterpret_cast<hkpAgentEntry*>(serializedEntry.m_nnEntryData), serializedEntry.m_trackInfo, hkBool(true),
		iSizeOfEntry, bTrackUsed);
	const int32_t iStriding = serializedEntry.m_atom.getContactPointPropertiesStriding();
	const int32_t iNumProperties = serializedEntry.m_propertiesStream.m_size / iStriding;
	serializedEntry.endianCheckUint32() = hkpSerializedAgentNnEntry::ENDIAN_CHECK_VALUE;
	if (iNumProperties > 0) {
		int32_t iOffset = 0;
		int32_t iCount = iNumProperties;
		do {
			uint8_t* pProperty = &serializedEntry.m_propertiesStream.m_data[iOffset];
			uint8_t ucByte = pProperty[16];
			pProperty[16] = pProperty[19];
			pProperty[19] = ucByte;
			ucByte = pProperty[17];
			pProperty[17] = pProperty[18];
			pProperty[18] = ucByte;
			ucByte = pProperty[0];
			pProperty[0] = pProperty[3];
			pProperty[3] = ucByte;
			ucByte = pProperty[1];
			pProperty[1] = pProperty[2];
			pProperty[2] = ucByte;
			ucByte = pProperty[4];
			pProperty[4] = pProperty[7];
			pProperty[7] = ucByte;
			ucByte = pProperty[5];
			pProperty[5] = pProperty[6];
			pProperty[6] = ucByte;
			ucByte = pProperty[8];
			pProperty[8] = pProperty[11];
			pProperty[11] = ucByte;
			ucByte = pProperty[9];
			pProperty[9] = pProperty[10];
			pProperty[10] = ucByte;
			uint8_t* pUserData = pProperty + 23;
			for (int32_t i = 0; i < 2 * serializedEntry.m_atom.m_numUserDatasForBodyA; ++i) {
				ucByte = *(pUserData - 3);
				*(pUserData - 3) = *pUserData;
				*pUserData = ucByte;
				ucByte = *(pUserData - 2);
				*(pUserData - 2) = *(pUserData - 1);
				*(pUserData - 1) = ucByte;
				pUserData += 4;
			}
			iOffset += iStriding;
			--iCount;
		} while (iCount);
	}
}
