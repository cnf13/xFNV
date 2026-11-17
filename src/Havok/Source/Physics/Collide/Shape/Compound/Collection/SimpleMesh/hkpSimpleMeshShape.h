#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// Defined in hkxAttributeGroupClass.cpp; hkpSimpleMeshShapeTypeInfo holds it.
void finishLoadedObjecthkpSimpleMeshShape(void* p, int32_t finishing);
void cleanupLoadedObjecthkpSimpleMeshShape(void* p);

// A plain triangle mesh: a vertex array, triangles indexing it with their welding information, and an optional
// material index per triangle.
class hkpSimpleMeshShape : public hkpShapeCollection {
public:
	// A triangle: three indices into m_vertices and its edge welding information.
	struct Triangle {
		static const hkClass& staticClass();

		int32_t m_a;
		int32_t m_b;
		int32_t m_c;
		uint16_t m_weldingInfo;
	};

	hkpSimpleMeshShape(float radius);
	// The SDK's finish constructor; defined in hkxAttributeGroupClass.cpp.
	hkpSimpleMeshShape(hkFinishLoadedObjectFlag flag);
	~hkpSimpleMeshShape() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	void initWeldingInfo(hkpWeldingUtility::WeldingType weldingType) override;
	void setWeldingInfo(uint32_t key, int16_t weldingInfo) override;
	// hkpShapeContainer's vtable at +0x10.
	uint32_t getFirstKey() const override;
	// hkpShapeContainer's vtable at +0x10.
	// Havok SDK signature (hkpCollide.lib, tied by bytes,vtable).
	uint32_t getNextKey(uint32_t oldKey) const override;
	// hkpShapeContainer's vtable at +0x10.
	// Havok SDK signature (hkpCollide.lib, tied by vtable).
	const hkpShape* getChildShape(uint32_t key, char (&buffer)[512]) const override;

	static const hkClass& staticClass();

	hkArray<hkVector4> m_vertices;
	hkArray<Triangle> m_triangles;
	hkArray<uint8_t> m_materialIndices;
	float m_radius;
	hkEnum<hkpWeldingUtility::WeldingType, uint8_t> m_weldingType;
};

extern hkClass hkpSimpleMeshShapeTriangleClass;
extern hkClass hkpSimpleMeshShapeClass;
extern hkTypeInfo hkpSimpleMeshShapeTypeInfo;
inline constexpr fnvd::Vtable hkpSimpleMeshShape_vftable_hkpShapeContainer{};
inline constexpr fnvd::Vtable hkpSimpleMeshShape_vftable{};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpSimpleMeshShapeTriangleTypeInfo;
