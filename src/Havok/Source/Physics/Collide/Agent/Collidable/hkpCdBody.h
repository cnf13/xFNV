#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

class hkClass;
class hkMotionState;
class hkpCollidable;
class hkpShape;
class hkTransform;
class hkTypeInfo;

// The motion of a body in a collision hierarchy: a plain transform for a static body, a motion state for a moving
// one.
union HavokMotionTransform {
    hkMotionState* pState;
    hkTransform* pMotion;
};

// One level of the collision hierarchy: a shape, the transform it is placed by, and the level above it.
class hkpCdBody {
public:
    // The reflection's member table.
    using MembersArray = const hkClassMember[4];

    hkpCdBody() = default;
    // A child body of parent, at the parent's motion.
    explicit hkpCdBody(const hkpCdBody* parent) {
        m_parent = parent;
        m_motion = parent->m_motion;
    }
    // A child body of parent, at its own motion state or transform; the motion field is not const.
    hkpCdBody(const hkpCdBody* parent, const hkMotionState* ms) {
        m_parent = parent;
        m_motion.pState = const_cast<hkMotionState*>(ms);
    }
    hkpCdBody(const hkpCdBody* parent, const hkTransform* t) {
        m_parent = parent;
        m_motion.pMotion = const_cast<hkTransform*>(t);
    }

    const hkpShape* getShape() const {
        return m_shape;
    }
    const hkpCdBody* getParent() const {
        return m_parent;
    }
    // The motion state of a moving body.
    const hkMotionState* getMotionState() const {
        return m_motion.pState;
    }
    void setShape(const hkpShape* shape, uint32_t key) {
        m_shape = shape;
        m_shapeKey = key;
    }
    // The collidable at the top of the hierarchy: the body whose m_parent is null.
    const hkpCollidable* getRootCollidable() const;

    // Havok SDK signature (hkpCollide.lib, tied by target).
    static const hkClass& staticClass();

    const hkpShape* m_shape;
    uint32_t m_shapeKey;
    HavokMotionTransform m_motion;
    const hkpCdBody* m_parent;

    static MembersArray Members;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpCdBodyTypeInfo;
