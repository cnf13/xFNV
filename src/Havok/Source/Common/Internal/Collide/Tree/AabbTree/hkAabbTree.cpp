#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"

#include <bit>
#include <xmmintrin.h>

// The implicit copy assignment: the volume, the union, then the parent and skip indices.
hkAabbTreeNode* hkAabbTreeNode_assign(hkAabbTreeNode* apThis, const hkAabbTreeNode& arOther) {
	apThis->m_volume.m_min = arOther.m_volume.m_min;
	apThis->m_volume.m_max = arOther.m_volume.m_max;
	apThis->m_nodeData = arOther.m_nodeData;
	apThis->m_leafData = arOther.m_leafData;
	apThis->m_parentIdx = arOther.m_parentIdx;
	apThis->m_skipNodeIdx = arOther.m_skipNodeIdx;
	return apThis;
}

// splitInto2's twin for the 32-bit tree's full-precision volumes: a Hoare partition of the leaves by their centres
// about the middle of aabb's longest axis. The split value and both side widths are kept as floats.
int32_t splitInto2(const hkAabb& aabb, const hkAabbTreeData::hkAabbTreeAabb* volumes, hkArray<uint32_t>& indices,
	uint32_t start, int32_t num, hkAabb& leftAabb, hkAabb& rightAabb, int32_t& numLeftOut, int32_t& numRightOut) {
	leftAabb.setEmpty();
	rightAabb.setEmpty();
	hkVector4 sum;
	sum.setAdd4(aabb.m_min, aabb.m_max);
	hkVector4 extent;
	extent.setSub4(aabb.m_max, aabb.m_min);
	int32_t axis = extent.getMajorAxis();
	float mid = sum(axis) * 0.5;
	bool canSplit = false;
	float maxSide = aabb.m_max(axis) - mid;
	if (maxSide > 2.3841858e-07f) {
		float minSide = mid - aabb.m_min(axis);
		canSplit = minSide > 2.3841858e-07f;
	}
	int32_t i;
	if (canSplit) {
		i = static_cast<int32_t>(start);
		int32_t j = static_cast<int32_t>(start) + num - 1;
		do {
			hkVector4 centerI;
			volumes[indices.m_data[i]].center(centerI);
			while (centerI(axis) <= mid) {
				leftAabb.includePoint(centerI);
				++i;
				volumes[indices.m_data[i]].center(centerI);
			}
			hkVector4 centerJ;
			volumes[indices.m_data[j]].center(centerJ);
			while (centerJ(axis) > mid) {
				rightAabb.includePoint(centerJ);
				--j;
				volumes[indices.m_data[j]].center(centerJ);
			}
			if (i >= j) {
				break;
			}
			uint32_t swapped = indices.m_data[i];
			indices.m_data[i] = indices.m_data[j];
			indices.m_data[j] = swapped;
			volumes[indices.m_data[i]].center(centerI);
			volumes[indices.m_data[j]].center(centerJ);
			++i;
			--j;
			leftAabb.includePoint(centerI);
			rightAabb.includePoint(centerJ);
		} while (i <= j);
	} else {
		leftAabb = aabb;
		rightAabb = aabb;
		i = static_cast<int32_t>(start) + (num >> 1);
	}
	int32_t numLeft = i - static_cast<int32_t>(start);
	numLeftOut = numLeft;
	int32_t numRight = num - numLeft;
	numRightOut = numRight;
	return numRight;
}

// A Hoare partition of the leaves by their centres about the middle of aabb's longest axis. The split value and both
// side widths are kept as floats.
int32_t splitInto2(const hkAabb& aabb, const hkAabbTreeData::hkAabbTreeAabb16* volumes, hkArray<uint16_t>& indices,
	uint16_t start, int32_t num, hkAabb& leftAabb, hkAabb& rightAabb, int32_t& numLeftOut, int32_t& numRightOut) {
	leftAabb.setEmpty();
	rightAabb.setEmpty();
	hkVector4 sum;
	sum.setAdd4(aabb.m_min, aabb.m_max);
	hkVector4 extent;
	extent.setSub4(aabb.m_max, aabb.m_min);
	int32_t axis = extent.getMajorAxis();
	float mid = sum(axis) * 0.5;
	bool canSplit = false;
	float maxSide = aabb.m_max(axis) - mid;
	if (maxSide > 2.3841858e-07f) {
		float minSide = mid - aabb.m_min(axis);
		canSplit = minSide > 2.3841858e-07f;
	}
	int32_t i;
	if (canSplit) {
		i = start;
		int32_t j = start + num - 1;
		do {
			hkVector4 centerI;
			volumes[indices[i]].center(centerI);
			while (centerI(axis) <= mid) {
				leftAabb.includePoint(centerI);
				++i;
				volumes[indices[i]].center(centerI);
			}
			hkVector4 centerJ;
			volumes[indices[j]].center(centerJ);
			while (centerJ(axis) > mid) {
				rightAabb.includePoint(centerJ);
				--j;
				volumes[indices[j]].center(centerJ);
			}
			if (i >= j) {
				break;
			}
			uint16_t swapped = indices[i];
			indices[i] = indices[j];
			indices[j] = swapped;
			volumes[indices[i]].center(centerI);
			volumes[indices[j]].center(centerJ);
			++i;
			--j;
			leftAabb.includePoint(centerI);
			rightAabb.includePoint(centerJ);
		} while (i <= j);
	} else {
		leftAabb = aabb;
		rightAabb = aabb;
		i = start + (num >> 1);
	}
	int32_t numLeft = i - start;
	numLeftOut = numLeft;
	int32_t numRight = num - numLeft;
	numRightOut = numRight;
	return numRight;
}
