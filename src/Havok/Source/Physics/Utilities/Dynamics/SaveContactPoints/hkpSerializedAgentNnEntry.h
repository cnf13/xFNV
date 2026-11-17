#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
// The native hkArray<hkContactPoint> destroys and sizes its elements: the struct is complete there.
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Reconstruction.hpp"

struct hkpAgent1nSector;
class hkpEntity;
struct hkpSerializedSubTrack1nInfo;

// The saved sectors of a 1n agent track and the saved sub tracks that point into them.
struct hkpSerializedTrack1nInfo {
	hkpSerializedTrack1nInfo() = default;
	// The SDK's finish constructor: the loaded arrays stay as loaded.
	hkpSerializedTrack1nInfo(hkFinishLoadedObjectFlag aFlag) : m_sectors(aFlag), m_subTracks(aFlag) {}
	~hkpSerializedTrack1nInfo();

	static const hkClass& staticClass();

	hkArray<hkpAgent1nSector*> m_sectors;
	hkArray<hkpSerializedSubTrack1nInfo*> m_subTracks;
};

// A sub track: where its first entry lies in the sectors.
struct hkpSerializedSubTrack1nInfo : public hkpSerializedTrack1nInfo {
	static const hkClass& staticClass();

	int32_t m_sectorIndex;
	int32_t m_offsetInSector;
};

// The saved state of one entity-entity agent: its contact points and properties, the agent's own bytes and its 1n
// tracks, for saving a simulation with its contacts.
struct hkpSerializedAgentNnEntry : public hkReferencedObject {
	enum SerializedAgentType {
		INVALID_AGENT_TYPE = 0x0,
		BOX_BOX_AGENT3 = 0x1,
		CAPSULE_TRIANGLE_AGENT3 = 0x2,
		PRED_GSK_AGENT3 = 0x3,
		PRED_GSK_CYLINDER_AGENT3 = 0x4,
		CONVEX_LIST_AGENT3 = 0x5,
		LIST_AGENT3 = 0x6,
		BV_TREE_AGENT3 = 0x7,
		COLLECTION_COLLECTION_AGENT3 = 0x8,
		COLLECTION_AGENT3 = 0x9,
	};

	enum {
		ENDIAN_CHECK_VALUE = 0x103,
	};

	hkpSerializedAgentNnEntry();

	// The SDK's finish constructor: the loaded fields stay; the atom finishes its striding.
	hkpSerializedAgentNnEntry(hkFinishLoadedObjectFlag aFlag)
		: hkReferencedObject(aFlag), m_atom(aFlag), m_propertiesStream(aFlag), m_contactPoints(aFlag), m_cpIdMgr(aFlag),
		  m_trackInfo(aFlag) {}

	~hkpSerializedAgentNnEntry() override;

	// The SDK's inline: the endian check bytes as the 32-bit word written on save (ENDIAN_CHECK_VALUE natively).
	uint32_t& endianCheckUint32() { return *reinterpret_cast<uint32_t*>(&m_endianCheckBuffer[0]); }

	static const hkClass& staticClass();

	hkpEntity* m_bodyA;
	hkpEntity* m_bodyB;
	hkUlong m_bodyAId;
	hkUlong m_bodyBId;
	hkBool m_useEntityIds;
	hkEnum<SerializedAgentType, int8_t> m_agentType;
	hkpSimpleContactConstraintAtom m_atom;
	hkArray<uint8_t> m_propertiesStream;
	hkArray<hkContactPoint> m_contactPoints;
	hkArray<uint8_t> m_cpIdMgr;
	uint8_t m_nnEntryData[160];
	hkpSerializedTrack1nInfo m_trackInfo;
	uint8_t m_endianCheckBuffer[4];
	uint32_t m_version;
};

inline constexpr fnvd::Vtable hkpSerializedAgentNnEntry_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpSerializedTrack1nInfoTypeInfo;
extern hkTypeInfo hkpSerializedSubTrack1nInfoTypeInfo;
