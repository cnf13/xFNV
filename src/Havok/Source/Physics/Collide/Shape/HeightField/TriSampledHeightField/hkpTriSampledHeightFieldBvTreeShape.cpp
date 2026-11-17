#include "Havok/Source/Physics/Collide/Shape/HeightField/TriSampledHeightField/hkpTriSampledHeightFieldBvTreeShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/hkpBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldShape.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/TriSampledHeightField/hkpTriSampledHeightFieldCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include <bit>
#include <xmmintrin.h>

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpTriSampledHeightFieldBvTreeShape, as the exe's .rdata holds them.
using hkpTriSampledHeightFieldBvTreeShapeClass_MembersArray = const hkClassMember[4];
static hkpTriSampledHeightFieldBvTreeShapeClass_MembersArray hkpTriSampledHeightFieldBvTreeShapeClass_Members{
	{"childContainer", &hkpSingleShapeContainerClass, nullptr, {hkClassMember::TYPE_STRUCT}, {hkClassMember::TYPE_VOID},
		0, {0}, 0x14, nullptr},
	{"childSize", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, 0x1C, nullptr},
	{"wantAabbRejectionTest", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20,
		nullptr},
	{"padding", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 12, {0}, 0x21, nullptr}};
hkClass hkpTriSampledHeightFieldBvTreeShapeClass("hkpTriSampledHeightFieldBvTreeShape", &hkpBvTreeShapeClass, 0x30,
	nullptr, 0, nullptr, 0, hkpTriSampledHeightFieldBvTreeShapeClass_Members, 4, nullptr, nullptr, 0, 0);
hkTypeInfo hkpTriSampledHeightFieldBvTreeShapeTypeInfo = {"hkpTriSampledHeightFieldBvTreeShape",
	finishLoadedObjecthkpTriSampledHeightFieldBvTreeShape, cleanupLoadedObjecthkpTriSampledHeightFieldBvTreeShape,
	getVtablehkpTriSampledHeightFieldBvTreeShape(), &typeid(hkpTriSampledHeightFieldBvTreeShape),
	sizeof(hkpTriSampledHeightFieldBvTreeShape)};

hkpTriSampledHeightFieldBvTreeShape::hkpTriSampledHeightFieldBvTreeShape(const hkpTriSampledHeightFieldCollection* c,
	hkBool doAabbRejection)
	: hkpBvTreeShape(HK_SHAPE_BV_TREE, hkpBvTreeShape::BVTREE_TRISAMPLED_HEIGHTFIELD), m_childContainer(c),
	  m_wantAabbRejectionTest(doAabbRejection) {}

hkpTriSampledHeightFieldBvTreeShape::~hkpTriSampledHeightFieldBvTreeShape() {}

// Turns the oriented box into its axis-aligned bounds (hkAabbUtil::calcAabb, out of line in the exe) and asks for the
// keys that overlap it through the class's queryAabb slot, which the exe calls through the vtable (slot 12), so src
// must not devirtualise it. The box is a 16-byte-aligned local the exe never constructs (no hkAabb constructor call),
// hence the raw storage.
void hkpTriSampledHeightFieldBvTreeShape::queryObb(const hkTransform& obbTransform, const hkVector4& obbExtent,
	float tolerance, hkArray<uint32_t>& hits) const {
	alignas(16) uint8_t aAabbStorage[sizeof(hkAabb)];
	hkAabb& aabb = *reinterpret_cast<hkAabb*>(aAabbStorage);
	hkAabbUtil::calcAabb(obbTransform, obbExtent, tolerance, aabb);
	const hkpBvTreeShape* pThis = this;
	pThis->queryAabb(aabb, hits);
}

// queryAabbImpl's keys appended to hits, with no limit; when the rejection test finds the box wholly above or below
// every sample, hits is set back to its size on entry.
void hkpTriSampledHeightFieldBvTreeShape::queryAabb(const hkAabb& aabbIn,
	hkArray<uint32_t, hkContainerHeapAllocator>& hits) const {
	FNVD_INLINED_LOCAL(hkAabb, kAabb, std::nullopt, std::nullopt);
	kAabb.m_min = aabbIn.m_min;
	kAabb.m_max = aabbIn.m_max;
	hkBool kOverlaps;
	// The exe's slots for minX and maxZ are 16-aligned in its realigned frame (esp+0x30 and esp+0x20 of the and-esp
	// frame).
	alignas(16) uint32_t uiMinX;
	uint32_t uiMaxX;
	uint32_t uiMinZ;
	alignas(16) uint32_t uiMaxZ;
	getExtentsForQueryAabb(&kOverlaps, kAabb, getShapeCollection(), uiMinX, uiMaxX, uiMinZ, uiMaxZ);
	if (!kOverlaps.m_bool) {
		return;
	}
	const hkpSampledHeightFieldShape* pHeightField = getShapeCollection()->getHeightFieldShape();
	const int32_t iOriginalSize = static_cast<int32_t>(hits.m_size);
	if (m_wantAabbRejectionTest.m_bool) {
		// Every sample so far at or below the box's bottom; every sample so far at or above its top.
		bool bAabbAbove = true;
		bool bAabbBelow = true;
		for (uint32_t x = uiMinX; x <= uiMaxX; ++x) {
			for (uint32_t z = uiMinZ; z <= uiMaxZ; ++z) {
				const uint32_t uiKey = ((z << 15) + x) * 2;
				{
					// EXE-INLINED none: hkArray<uint32_t>::pushBack
					hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&hits);
					if (hits.m_size == static_cast<uint32_t>(hits.getCapacity())) {
						hkArrayUtil::_reserveMore(alloc, &hits, sizeof(uint32_t));
					}
					hits.m_data[hits.m_size++] = uiKey;
				}
				{
					// EXE-INLINED none: hkArray<uint32_t>::pushBack
					hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&hits);
					if (hits.m_size == static_cast<uint32_t>(hits.getCapacity())) {
						hkArrayUtil::_reserveMore(alloc, &hits, sizeof(uint32_t));
					}
					hits.m_data[hits.m_size++] = uiKey | 1;
				}
				if (bAabbAbove || bAabbBelow) {
					float fHeight = pHeightField->getHeightAtImpl(static_cast<int32_t>(x), static_cast<int32_t>(z));
					fHeight = pHeightField->m_intToFloatScale.m_quad[1] * fHeight;
					if (fHeight > kAabb.m_min.m_quad[1]) {
						bAabbAbove = false;
					}
					if (kAabb.m_max.m_quad[1] > fHeight) {
						bAabbBelow = false;
					}
				}
			}
		}
		if (bAabbAbove || bAabbBelow) {
			const uint32_t uiEdgeX = uiMaxX + 1;
			const uint32_t uiEdgeZ = uiMaxZ + 1;
			for (uint32_t x = uiMinX; x <= uiEdgeX; ++x) {
				float fHeight = pHeightField->getHeightAtImpl(static_cast<int32_t>(x), static_cast<int32_t>(uiEdgeZ));
				fHeight = pHeightField->m_intToFloatScale.m_quad[1] * fHeight;
				if (fHeight > kAabb.m_min.m_quad[1]) {
					bAabbAbove = false;
				}
				if (kAabb.m_max.m_quad[1] > fHeight) {
					bAabbBelow = false;
				}
			}
			for (uint32_t z = uiMinZ; z <= uiEdgeZ; ++z) {
				float fHeight = pHeightField->getHeightAtImpl(static_cast<int32_t>(uiEdgeX), static_cast<int32_t>(z));
				fHeight = pHeightField->m_intToFloatScale.m_quad[1] * fHeight;
				if (fHeight > kAabb.m_min.m_quad[1]) {
					bAabbAbove = false;
				}
				if (kAabb.m_max.m_quad[1] > fHeight) {
					bAabbBelow = false;
				}
			}
			if (bAabbAbove || bAabbBelow) {
				hits.setSize(iOriginalSize);
			}
		}
	} else {
		for (uint32_t x = uiMinX; x <= uiMaxX; ++x) {
			for (uint32_t z = uiMinZ; z <= uiMaxZ; ++z) {
				const uint32_t uiKey = ((z << 15) + x) * 2;
				{
					// EXE-INLINED none: hkArray<uint32_t>::pushBack
					hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&hits);
					if (hits.m_size == static_cast<uint32_t>(hits.getCapacity())) {
						hkArrayUtil::_reserveMore(alloc, &hits, sizeof(uint32_t));
					}
					hits.m_data[hits.m_size++] = uiKey;
				}
				{
					// EXE-INLINED none: hkArray<uint32_t>::pushBack
					hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&hits);
					if (hits.m_size == static_cast<uint32_t>(hits.getCapacity())) {
						hkArrayUtil::_reserveMore(alloc, &hits, sizeof(uint32_t));
					}
					hits.m_data[hits.m_size++] = uiKey | 1;
				}
			}
		}
	}
}

// The keys of the triangles whose cells overlap aabb, at most maxNumKeys of them (two per cell, z within x). With the
// rejection test, the cells' sample heights, and those of the row and column past the extents, are compared with the
// box: when the box lies wholly above or wholly below every sample, no key counts.
uint32_t hkpTriSampledHeightFieldBvTreeShape::queryAabbImpl(const hkAabb& aabbIn, uint32_t* hits,
	int32_t maxNumKeys) const {
	FNVD_INLINED_LOCAL(hkAabb, kAabb, std::nullopt, std::nullopt);
	kAabb.m_min = aabbIn.m_min;
	const hkpTriSampledHeightFieldCollection* pCollection = getShapeCollection();
	kAabb.m_max = aabbIn.m_max;
	hkBool kOverlaps;
	uint32_t uiMinX;
	// The exe's slot for maxX is 16-aligned in its realigned frame (esp+0x20 of the and-esp frame).
	alignas(16) uint32_t uiMaxX;
	uint32_t uiMinZ;
	uint32_t uiMaxZ;
	getExtentsForQueryAabb(&kOverlaps, kAabb, pCollection, uiMinX, uiMaxX, uiMinZ, uiMaxZ);
	if (!kOverlaps.m_bool) {
		return 0;
	}
	int32_t iNumKeys = 0;
	const hkpSampledHeightFieldShape* pHeightField = pCollection->getHeightFieldShape();
	if (m_wantAabbRejectionTest.m_bool) {
		// Every sample so far at or below the box's bottom; every sample so far at or above its top.
		bool bAabbAbove = true;
		bool bAabbBelow = true;
		for (uint32_t x = uiMinX; x <= uiMaxX; ++x) {
			for (uint32_t z = uiMinZ; z <= uiMaxZ; ++z) {
				const uint32_t uiKey = ((z << 15) + x) * 2;
				if (iNumKeys < maxNumKeys) {
					hits[iNumKeys++] = uiKey;
					if (iNumKeys < maxNumKeys) {
						hits[iNumKeys++] = uiKey | 1;
					}
				}
				if (bAabbAbove || bAabbBelow) {
					float fHeight = pHeightField->getHeightAtImpl(static_cast<int32_t>(x), static_cast<int32_t>(z));
					fHeight = pHeightField->m_intToFloatScale.m_quad[1] * fHeight;
					if (fHeight > kAabb.m_min.m_quad[1]) {
						bAabbAbove = false;
					}
					if (kAabb.m_max.m_quad[1] > fHeight) {
						bAabbBelow = false;
					}
				}
			}
		}
		if (bAabbAbove || bAabbBelow) {
			const uint32_t uiEdgeX = uiMaxX + 1;
			const uint32_t uiEdgeZ = uiMaxZ + 1;
			for (uint32_t x = uiMinX; x <= uiEdgeX; ++x) {
				float fHeight = pHeightField->getHeightAtImpl(static_cast<int32_t>(x), static_cast<int32_t>(uiEdgeZ));
				fHeight = fHeight * pHeightField->m_intToFloatScale.m_quad[1];
				if (fHeight > kAabb.m_min.m_quad[1]) {
					bAabbAbove = false;
				}
				if (kAabb.m_max.m_quad[1] > fHeight) {
					bAabbBelow = false;
				}
			}
			for (uint32_t z = uiMinZ; z <= uiEdgeZ; ++z) {
				float fHeight = pHeightField->getHeightAtImpl(static_cast<int32_t>(uiEdgeX), static_cast<int32_t>(z));
				fHeight = fHeight * pHeightField->m_intToFloatScale.m_quad[1];
				if (fHeight > kAabb.m_min.m_quad[1]) {
					bAabbAbove = false;
				}
				if (kAabb.m_max.m_quad[1] > fHeight) {
					bAabbBelow = false;
				}
			}
			if (bAabbAbove || bAabbBelow) {
				return 0;
			}
		}
	} else {
		for (uint32_t x = uiMinX; x <= uiMaxX; ++x) {
			for (uint32_t z = uiMinZ; z <= uiMaxZ; ++z) {
				const uint32_t uiKey = ((z << 15) + x) * 2;
				if (iNumKeys < maxNumKeys) {
					hits[iNumKeys++] = uiKey;
					if (iNumKeys < maxNumKeys) {
						hits[iNumKeys++] = uiKey | 1;
					}
				}
			}
		}
	}
	return iNumKeys;
}

void hkpTriSampledHeightFieldBvTreeShape::getAabbImpl(const hkTransform& localToWorld, float tolerance,
	hkAabb& out) const {
	m_childContainer.m_childShape->getAabbImpl(localToWorld, tolerance, out);
}

// Casts the ray against the heightfield shape the collection wraps.
hkBool hkpTriSampledHeightFieldBvTreeShape::castRayImpl(const hkpShapeRayCastInput& input,
	hkpShapeRayCastOutput& output) const {
	const hkpSampledHeightFieldShape* HeightFieldShape = getShapeCollection()->getHeightFieldShape();
	return HeightFieldShape->castRayImpl(input, output);
}

void hkpTriSampledHeightFieldBvTreeShape::castRayWithCollectorImpl(const hkpShapeRayCastInput& input,
	const hkpCdBody& cdBody, hkpRayHitCollector& collector) const {
	const hkpSampledHeightFieldShape* HeightFieldShape = getShapeCollection()->getHeightFieldShape();
	HeightFieldShape->castRayWithCollectorImpl(input, cdBody, collector);
}

// The collection's size given a 256-byte buffer (an unsigned compare in the exe), kept in m_childSize; this shape's
// own size is returned.
int32_t hkpTriSampledHeightFieldBvTreeShape::calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input,
	int32_t spuBufferSizeLeft) const {
	const uint32_t uiChildSize = m_childContainer.m_childShape->calcSizeForSpu(input, 256);
	if (uiChildSize > 256) {
		return -1;
	}
	m_childSize = uiChildSize;
	return sizeof(*this);
}

const hkClass& hkpTriSampledHeightFieldBvTreeShape::staticClass() {
	return hkpTriSampledHeightFieldBvTreeShapeClass;
}

// The SDK's getExtentsForQueryAabb: grows aabb by the collection's radius, clips its x and z to the heightfield's own
// box (aabb keeps the result) and turns it into sample ranges, each clamped to [0, res - 2] and converted by adding
// 65536.0f and taking bits 7-22; false when the clipped box is empty in any of x, y, z. x87 copy at 0xD26955: the
// exe moves the radius through fld/fstp before splatting it.
hkBool* hkpTriSampledHeightFieldBvTreeShape::getExtentsForQueryAabb(hkBool* apResult, hkAabb& aabb,
	const hkpTriSampledHeightFieldCollection* collection, uint32_t& minX, uint32_t& maxX, uint32_t& minZ,
	uint32_t& maxZ) {
	const hkpSampledHeightFieldShape* pHeightField = collection->getHeightFieldShape();
	const float fRadius = collection->m_radius;
	const __m128 kRadius = _mm_set1_ps(fRadius);
	aabb.m_max.m_quad = _mm_add_ps(aabb.m_max.m_quad, kRadius);
	aabb.m_min.m_quad = _mm_sub_ps(aabb.m_min.m_quad, kRadius);
	FNVD_INLINED_LOCAL(hkAabb, kHeightFieldAabb, std::nullopt, std::nullopt);
	pHeightField->getAabbImpl(hkTransformIdentity, 0.0f, kHeightFieldAabb);

	const __m128 kMaskXZ = hkVector4Comparison_s_maskFromBits[hkVector4Comparison::MASK_XZ];
	const __m128 kMinSelect = _mm_and_ps(_mm_cmplt_ps(aabb.m_min.m_quad, kHeightFieldAabb.m_min.m_quad), kMaskXZ);
	const __m128 kMin =
		_mm_or_ps(_mm_andnot_ps(kMinSelect, aabb.m_min.m_quad), _mm_and_ps(kMinSelect, kHeightFieldAabb.m_min.m_quad));
	const __m128 kMaxSelect = _mm_and_ps(_mm_cmplt_ps(kHeightFieldAabb.m_max.m_quad, aabb.m_max.m_quad), kMaskXZ);
	const __m128 kMax =
		_mm_or_ps(_mm_and_ps(kMaxSelect, kHeightFieldAabb.m_max.m_quad), _mm_andnot_ps(kMaxSelect, aabb.m_max.m_quad));
	aabb.m_max.m_quad = kMax;
	aabb.m_min.m_quad = kMin;
	if ((_mm_movemask_ps(_mm_cmple_ps(kMax, kMin)) & hkVector4Comparison::MASK_XYZ) != 0) {
		apResult->m_bool = 0;
		return apResult;
	}

	// The largest cell index per axis in x and z; y and w are clamped to 0.
	const __m128 kClamp = _mm_setr_ps(static_cast<float>(pHeightField->m_xRes - 2), 0.0f,
		static_cast<float>(pHeightField->m_zRes - 2), 0.0f);
	const __m128 kZero = _mm_setzero_ps();
	const __m128 kLow =
		_mm_add_ps(_mm_max_ps(_mm_min_ps(_mm_mul_ps(_mm_add_ps(kMin,
														pHeightField->m_floatToIntOffsetFloorCorrected.m_quad),
											 pHeightField->m_floatToIntScale.m_quad),
								  kClamp),
					   kZero),
			hkpTriSampledHeightFieldBvTreeShape_s_quadReal65536);
	minX = static_cast<uint16_t>(std::bit_cast<uint32_t>(kLow[0]) >> 7);
	minZ = static_cast<uint16_t>(std::bit_cast<uint32_t>(kLow[2]) >> 7);
	const __m128 kHigh =
		_mm_add_ps(_mm_max_ps(_mm_min_ps(_mm_mul_ps(_mm_add_ps(pHeightField->m_floatToIntOffsetFloorCorrected.m_quad,
														aabb.m_max.m_quad),
											 pHeightField->m_floatToIntScale.m_quad),
								  kClamp),
					   kZero),
			hkpTriSampledHeightFieldBvTreeShape_s_quadReal65536);
	maxX = static_cast<uint16_t>(std::bit_cast<uint32_t>(kHigh[0]) >> 7);
	maxZ = static_cast<uint16_t>(std::bit_cast<uint32_t>(kHigh[2]) >> 7);
	apResult->m_bool = 1;
	return apResult;
}
