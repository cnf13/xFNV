#include "Havok/Source/Physics/Collide/Util/ShapeDepth/hkpShapeDepthUtil.h"

#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"

// The exe keeps the container's vtable pointer in a register across the loop; src calls the virtuals as usual.
int32_t hkpShapeDepthUtil::getShapeDepth(const hkpShape* shape) {
	const hkpShapeContainer* container = shape->getContainer();
	if (!container) {
		return 0;
	}
	uint8_t maxDepth = 0;
	for (uint32_t key = container->getFirstKey(); key != 0xFFFFFFFFu; key = container->getNextKey(key)) {
		alignas(16) char buffer[512];
		const hkpShape* childShape = container->getChildShape(key, buffer);
		// The exe compares only al.
		uint8_t depth = static_cast<uint8_t>(hkpShapeDepthUtil::getShapeDepth(childShape));
		if (depth > maxDepth) {
			maxDepth = depth;
		}
	}
	return maxDepth + 1;
}
