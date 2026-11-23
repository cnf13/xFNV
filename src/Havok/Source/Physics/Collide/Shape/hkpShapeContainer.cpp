#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"

#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nMachine.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeBuffer.h"

hkClass
    hkpShapeContainerClass("hkpShapeContainer", nullptr, 4, nullptr, 1, nullptr, 0, nullptr, 0, nullptr, nullptr, 0, 0);

int32_t hkpShapeContainer::getNumChildShapes() const {
    uint32_t i;
    int32_t v2 = 0;
    for (i = getFirstKey(); i != (-1); i = getNextKey(i)) {
        ++v2;
    }
    return v2;
}

uint32_t hkpShapeContainer::getCollisionFilterInfo(uint32_t key) const {
    return 0;
}

// Gives body the child of container under key (the shape built into shapeBuffer where it needs one) and the key, and
// returns the child.
const hkpShape* getChildShapeIntoBody(
    const hkpShapeContainer* container,
    char (*shapeBuffer)[HK_SHAPE_BUFFER_SIZE],
    uint32_t key,
    hkpCdBody* body
) {
    const hkpShape* shape = container->getChildShape(key, *shapeBuffer);
    body->m_shape = shape;
    body->m_shapeKey = key;
    return shape;
}

// The Nm form of getChildShapeIntoBody: for each key of keys that differs from the one last seen, remembers it and
// gives the body of that side the container's child under it, B first.
void updateChildShapes(
    const hkpShapeContainer* containerB,
    char (*shapeBufferB)[HK_SHAPE_BUFFER_SIZE],
    const hkShapeKeyPair* keys,
    const hkpShapeContainer* containerA,
    char (*shapeBufferA)[HK_SHAPE_BUFFER_SIZE],
    hkpCdBody* bodyA,
    hkpCdBody* bodyB,
    hkShapeKeyPair* lastKeys
) {
    uint32_t keyB = keys->m_shapeKeyB;
    if (lastKeys->m_shapeKeyB != keyB) {
        lastKeys->m_shapeKeyB = keyB;
        bodyB->m_shape = containerB->getChildShape(keyB, *shapeBufferB);
        bodyB->m_shapeKey = keyB;
    }
    uint32_t keyA = keys->m_shapeKeyA;
    if (lastKeys->m_shapeKeyA != keyA) {
        lastKeys->m_shapeKeyA = keyA;
        bodyA->m_shape = containerA->getChildShape(keyA, *shapeBufferA);
        bodyA->m_shapeKey = keyA;
    }
}
