#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

class hkpRigidBody;

// The destruction data an entity can carry (hkpEntity::m_breakableBody). The interface of the Havok SDK; the exe has
// no vtable of the class itself. hkpRigidBody::clone (0xCA90C0) calls cloneBreakableBody through slot 3 (+0xC) of the
// body's vtable, after hkReferencedObject's three.
class hkpBreakableBody : public hkReferencedObject {
public:
	~hkpBreakableBody() override {}

	virtual hkpBreakableBody* cloneBreakableBody(hkpRigidBody* clonedRigidBody = nullptr) const = 0;
};

// The reflection class of hkpBreakableBody (hkpBreakableBodyClass.cpp); hkBuiltinTypeRegistry's class table holds it.
extern hkClass hkpBreakableBodyClass;
