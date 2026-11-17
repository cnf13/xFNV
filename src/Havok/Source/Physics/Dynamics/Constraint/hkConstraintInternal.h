#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

struct hkpConstraintAtom;
class hkpConstraintInstance;
class hkpEntity;

// The world's internal record of a constraint instance: its entities, atoms, solver sizes and runtime. Not
// polymorphic. The Havok SDK declares it HK_CLASSALIGN16 (hkpConstraintInstance.h); its members end at 0x24 and the
// PC release PDB (hkpinternal.pdb, sizeof 48) rounds that up to 0x30, the next 16-byte boundary.
class alignas(16) hkConstraintInternal {
public:
	// The SDK's inline (hkpConstraintInstance.inl, its hkSelectOther): the entity of the two that is not entity. The
	// SDK xors the three addresses as integers, so the pointers go through uintptr_t.
	hkpEntity* getOtherEntity(const hkpEntity* entity) const {
		return reinterpret_cast<hkpEntity*>(reinterpret_cast<uintptr_t>(m_entities[0]) ^
			reinterpret_cast<uintptr_t>(m_entities[1]) ^ reinterpret_cast<uintptr_t>(entity));
	}

	hkpConstraintInstance* m_constraint;
	hkpEntity* m_entities[2];
	hkpConstraintAtom* m_atoms;
	uint16_t m_atomsSize;
	uint8_t m_callbackRequest;
	// enum hkpConstraintInstance::ConstraintPriority spelled as uint32_t (src has no such enum).
	hkEnum<uint32_t, uint8_t> m_priority;
	uint16_t m_sizeOfSchemas;
	uint16_t m_numSolverResults;
	uint16_t m_numSolverElemTemps;
	uint8_t m_whoIsMaster;
	hkBool m_isNormalType;
	void* m_runtime;
	uint16_t m_runtimeSize;
	uint16_t m_slaveIndex;
};
