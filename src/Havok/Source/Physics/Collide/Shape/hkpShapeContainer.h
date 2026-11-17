#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nMachine.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeBuffer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpShape;
class hkpShapeContainer;

// The interface of a shape that holds child shapes, which are walked by key. Only the vtable pointer is a member.
class hkpShapeContainer {
public:
	// Values from the Xbox PDB.
	enum ReferencePolicy : int32_t {
		REFERENCE_POLICY_IGNORE = 0x0,
		REFERENCE_POLICY_INCREMENT = 0x1,
	};

	virtual ~hkpShapeContainer();

	virtual int32_t getNumChildShapes() const;
	virtual uint32_t getFirstKey() const = 0;
	virtual uint32_t getNextKey(uint32_t) const = 0;
	// Havok SDK signature (hkpCollide.lib, tied by vtable).
	virtual uint32_t getCollisionFilterInfo(uint32_t key) const;
	virtual const hkpShape* getChildShape(uint32_t, char (&)[512]) const = 0;
	virtual bool isWeldingEnabled() const { return true; }

	// DeletingDestruct's free (ourFree).
	static void operator delete(void* apMemory);
};

// Not named in the PDB or IDA; name chosen here. Register arguments: container in ecx, shapeBuffer in edx, key in
// edi, body in esi.
const hkpShape* getChildShapeIntoBody(const hkpShapeContainer* container, char (*shapeBuffer)[HK_SHAPE_BUFFER_SIZE],
	uint32_t key, hkpCdBody* body);
// Not named in the PDB or IDA; name chosen here. Register arguments: containerB in ecx, shapeBufferB in edx, lastKeys
// in edi; the rest on the stack.
void updateChildShapes(const hkpShapeContainer* containerB, char (*shapeBufferB)[HK_SHAPE_BUFFER_SIZE],
	const hkShapeKeyPair* keys, const hkpShapeContainer* containerA, char (*shapeBufferA)[HK_SHAPE_BUFFER_SIZE],
	hkpCdBody* bodyA, hkpCdBody* bodyB, hkShapeKeyPair* lastKeys);

inline constexpr fnvd::Vtable hkpShapeContainer_vftable{};

// The class's reflection record.
extern hkClass hkpShapeContainerClass;
