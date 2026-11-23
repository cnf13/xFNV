#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

class hkpRigidBody;

// The destruction data an entity can carry (hkpEntity::m_breakableBody). hkpRigidBody::clone calls cloneBreakableBody.
class hkpBreakableBody : public hkReferencedObject {
public:
    ~hkpBreakableBody() override {}

    virtual hkpBreakableBody* cloneBreakableBody(hkpRigidBody* clonedRigidBody = nullptr) const = 0;
};

// The reflection class of hkpBreakableBody (hkpBreakableBodyClass.cpp); hkBuiltinTypeRegistry's class table holds it.
extern hkClass hkpBreakableBodyClass;
