#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkpWeldingUtility.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkAabb;
class hkpConvexShape;
class hkpMeshMaterial;
class hkpMeshShape;

// Havok's mesh of triangle and shape subparts: each subpart is a strided vertex and index buffer, or a list of convex
// shapes with a transform, and a shape key addresses a subpart and a child inside it. It embeds one triangles subpart.
// The vectors are 16-byte aligned.
class hkpExtendedMeshShape : public hkpShapeCollection {
public:
	enum SubpartType : int32_t {
		SUBPART_TRIANGLES = 0x0,
		SUBPART_SHAPE = 0x1,
	};

	enum IndexStridingType : int32_t {
		INDICES_INVALID = 0x0,
		INDICES_INT8 = 0x1,
		INDICES_INT16 = 0x2,
		INDICES_INT32 = 0x3,
		INDICES_MAX_ID = 0x4,
	};

	enum MaterialIndexStridingType : int32_t {
		MATERIAL_INDICES_INVALID = 0x0,
		MATERIAL_INDICES_INT8 = 0x1,
		MATERIAL_INDICES_INT16 = 0x2,
		MATERIAL_INDICES_MAX_ID = 0x3,
	};

	// What every subpart starts with: its type, and where its materials are and how they are strided.
	struct Subpart {
		// Havok SDK signature (hkpCollide.lib, tied by target).
		static const hkClass& staticClass();

		hkEnum<SubpartType, int8_t> m_type;
		hkEnum<MaterialIndexStridingType, int8_t> m_materialIndexStridingType;
		int16_t m_materialStriding;
		const void* m_materialIndexBase;
		uint16_t m_materialIndexStriding;
		uint16_t m_numMaterials;
		const hkpMeshMaterial* m_materialBase;
		const hkClass* m_materialClass;
	};

	// A subpart of triangles: strided vertices and indices, and the extrusion applied to each triangle.
	struct alignas(16) TrianglesSubpart : Subpart {
		// Copies every member.
		TrianglesSubpart& operator=(const TrianglesSubpart& other);

		// Havok SDK signature (hkpCollide.lib, tied by target).
		static const hkClass& staticClass();

		int32_t m_numTriangleShapes;
		const float* m_vertexBase;
		int32_t m_vertexStriding;
		int32_t m_numVertices;
		hkVector4 m_extrusion;
		const void* m_indexBase;
		int32_t m_indexStriding;
		hkEnum<IndexStridingType, int8_t> m_stridingType;
		int8_t m_flipAlternateTriangles;
		int32_t m_triangleOffset;
		hkUlong m_userData;
	};

	// A subpart of convex shapes, with the transform that places them.
	struct alignas(16) ShapesSubpart : Subpart {
		// The transform flags kept in m_translation's w (getInt24W).
		enum {
			FLAG_NONE = 0x0,
			FLAG_TRANSLATE_SET = 0x1,
			FLAG_ROTATE_SET = 0x2,
		};

		// hkpExtendedMeshShape::ShapesSubpart::Members: the reflection's member table.
		using MembersArray = const hkClassMember[3];

		ShapesSubpart(hkFinishLoadedObjectFlag flag);
		// Constructors from the child shapes and their offset or transform.
		ShapesSubpart(const hkpConvexShape* const* a1, int32_t a2, const hkVector4& a3);
		ShapesSubpart(const hkpConvexShape* const* a1, int32_t a2, const hkTransform& r);
		~ShapesSubpart();

		// Copies the Subpart, assigns m_childShapes with each shape's reference counted, then m_rotation and
		// m_translation.
		ShapesSubpart& operator=(const ShapesSubpart& other);

		// Havok SDK signature (hkpCollide.lib, tied by target).
		static const hkClass& staticClass();

		hkArray<hkpConvexShape*> m_childShapes;
		hkQuaternion m_rotation;
		hkVector4 m_translation;

		static MembersArray Members;
	};

	// hkpExtendedMeshShape::Members: the reflection's member table (its weldingType member's enum is stored at
	// start-up).
	using MembersArray = hkClassMember[13];

	hkpExtendedMeshShape(hkFinishLoadedObjectFlag flag);
	hkpExtendedMeshShape(float a1, int32_t a2);
	hkpExtendedMeshShape(const hkpMeshShape* apMeshShape);
	~hkpExtendedMeshShape() override;

	// Havok SDK signature (hkpCollide.lib, tied by bytes,vtable).
	int32_t calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	void initWeldingInfo(hkpWeldingUtility::WeldingType weldingType) override;
	void setWeldingInfo(uint32_t key, int16_t weldingInfo) override;
	virtual void addTrianglesSubpart(const TrianglesSubpart& part);
	virtual int32_t addShapesSubpart(const ShapesSubpart& part);
	// hkpShapeContainer's vtable at +0x10.
	int32_t getNumChildShapes() const override;
	// hkpShapeContainer's vtable at +0x10.
	uint32_t getFirstKey() const override;
	// hkpShapeContainer's vtable at +0x10.
	uint32_t getNextKey(uint32_t initialKey) const override;
	// hkpShapeContainer's vtable at +0x10.
	uint32_t getCollisionFilterInfo(uint32_t key) const override;
	// hkpShapeContainer's vtable at +0x10.
	const hkpShape* getChildShape(uint32_t key, char (&buffer)[512]) const override;

	// Havok SDK signature (hkpCollide.lib, tied by bytes).
	int32_t _getNumChildShapesInTrianglesSubpart(const hkpExtendedMeshShape::TrianglesSubpart& subpart,
		int32_t subpartIndex) const;
	// Grows an AABB over m_trianglesSubparts (by the triangle radius) and m_shapesSubparts and stores m_aabbCenter and
	// m_aabbHalfExtents.
	void recalcAabbExtents();
	hkpExtendedMeshShape::TrianglesSubpart* expandOneTriangleSubparts();
	hkpExtendedMeshShape::ShapesSubpart* expandOneShapesSubparts();
	void setScaling(const hkVector4& scaling);
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
	int32_t _getNumChildShapesInShapesSubpart(const hkpExtendedMeshShape::ShapesSubpart& subpart) const;
	// Havok SDK signature (hkpCollide.lib, tied by reloc).
	const hkpMeshMaterial* getMeshMaterial(uint32_t key) const;

	// The reflection class hkpExtendedMeshShapeClass.
	static const hkClass& staticClass();

	TrianglesSubpart m_embeddedTrianglesSubpart;
	hkVector4 m_scaling;
	hkVector4 m_aabbHalfExtents;
	hkVector4 m_aabbCenter;
	int32_t m_numBitsForSubpartIndex;
	hkArray<TrianglesSubpart> m_trianglesSubparts;
	hkArray<ShapesSubpart> m_shapesSubparts;
	hkArray<uint16_t> m_weldingInfo;
	hkEnum<hkpWeldingUtility::WeldingType, uint8_t> m_weldingType;
	uint32_t m_defaultCollisionFilterInfo;
	// The SDK's mutable cache: the const getNumChildShapes stores it.
	mutable int32_t m_cachedNumChildShapes;
	float m_triangleRadius;
	int32_t m_padding[2];

	static MembersArray Members;
};

// File-local calcAabbExtents of hkpExtendedMeshShape.cpp (the triangles overload): out becomes the box of every
// triangle vertex of part scaled by scaling.
void calcAabbExtents(const hkpExtendedMeshShape::TrianglesSubpart& part, hkVector4& scaling, hkAabb& out);
void includeScaledTransformedPoint(const float* a1, const hkTransform* pTransform, const float* r_eax, hkAabb* r_esi);
void sub_D55500();
void sub_D55510();
// Copies 4 quads (64 bytes), as hkString::memCpy16<64>; hkpExtendedMeshShape::getChildShape copies a ShapesSubpart with
// it.
void copy64Bytes(void* dst, const void* src);
// The AABB of a shapes subpart under its transform; hkpExtendedMeshShape::recalcAabbExtents and addShapesSubpart call
// it.
void calcAabbExtents(const hkpExtendedMeshShape::ShapesSubpart* part, hkAabb* scaling);

// Stands for hkpExtendedMeshShape::m_triangleDengeneracyTolerance, a static member.
inline float hkDefaultTriangleDegeneracyTolerance = 1e-07f;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpExtendedMeshShapeSubpartTypeInfo;
extern hkTypeInfo hkpExtendedMeshShapeTrianglesSubpartTypeInfo;
extern hkTypeInfo hkpExtendedMeshShapeShapesSubpartTypeInfo;
