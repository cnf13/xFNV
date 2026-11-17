#include "Havok/Source/Physics/Collide/Util/ShapeDepth/hkpShapeDepthUtil.h"

#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"

int32_t hkpShapeDepthUtil::getShapeDepth(const hkpShape* shape) {
	const hkpShapeContainer* container = shape->getContainer();
	if (!container) {
		return 0;
	}
	uint8_t maxDepth = 0;
	for (uint32_t key = container->getFirstKey(); key != 0xFFFFFFFFu; key = container->getNextKey(key)) {
		alignas(16) char buffer[512];
		const hkpShape* childShape = container->getChildShape(key, buffer);
		// Only the low byte of the depth is compared.
		uint8_t depth = static_cast<uint8_t>(hkpShapeDepthUtil::getShapeDepth(childShape));
		if (depth > maxDepth) {
			maxDepth = depth;
		}
	}
	return maxDepth + 1;
}
