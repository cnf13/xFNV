#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Reconstruction.hpp"

class bhkShape;
class hkAabb;
class hkSphere;
class hkTransform;
class hkpCdBody;
class hkpCdVertex;
class hkpConvexShape;
class hkpRayHitCollector;
class hkpShapeContainer;
struct hkpShapeRayBundleCastInput;
struct hkpShapeRayCastInput;

// The shape key that names no child shape (the SDK's hkpShape.h).
#define HK_INVALID_SHAPE_KEY 0xffffffff

// Base of every Havok collision shape: a user data pointer (the owning bhkShape) and the shape type. The queries a
// shape answers (bounding box, ray casts) are virtual; the base implements only the projection and the SPU size.
class hkpShape : public hkReferencedObject {
public:
	// What a shape is told when its size on the SPU is calculated.
	struct CalcSizeForSpuInput {
		bool m_midphaseAgent3Registered;
		bool m_isFixedOrKeyframed;
		bool m_hasDynamicMotionSaved;
	};

	// The table of per-shape-type query functions the SPU-style dispatch uses (cdecl, each taking the shape as
	// thisObj): hkPadSpu<function pointer> on PC is the pointer itself. Signatures from the PDB; the aliases are the
	// Havok SDK's names, not in the PDB.
	struct ShapeFuncs {
		using GetSupportingVertexFunc = void (*)(const void* thisObj, const hkVector4& direction,
			hkpCdVertex& supportingVertexOut);
		using ConvertVertexIdsToVerticesFunc = void (*)(const void* thisObj, const uint16_t* ids, int32_t numIds,
			hkpCdVertex* verticesOut);
		using WeldContactPointFunc = int32_t (*)(const void* thisObj, uint16_t*, uint8_t&, hkVector4&,
			const hkTransform*, const hkpConvexShape*, const hkTransform*, hkVector4&);
		using GetCentreFunc = void (*)(const void* thisObj, hkVector4& centreOut);
		using GetNumCollisionSpheresFunc = int32_t (*)(const void* thisObj);
		using GetCollisionSpheresFunc = const hkSphere* (*)(const void* thisObj, hkSphere* sphereBuffer);
		using GetAabbFunc = void (*)(const void* thisObj, const hkTransform& localToWorld, float tolerance,
			hkAabb& out);
		using CastRayFunc = hkBool (*)(const void* thisObj, const hkpShapeRayCastInput& input,
			hkpShapeRayCastOutput& output);
		using CastRayWithCollectorFunc = void (*)(const void* thisObj, const hkpShapeRayCastInput& input,
			const hkpCdBody& cdBody, hkpRayHitCollector& collector);
		using CastRayBundleFunc = hkVector4Comparison (*)(const void* thisObj, const hkpShapeRayBundleCastInput& input,
			hkpShapeRayBundleCastOutput& output, const hkVector4Comparison& mask);
		using GetChildShapeFunc = const hkpShape* (*)(const void* thisObj, uint32_t key, char (&buffer)[512]);
		using GetCollisionFilterInfoFunc = uint32_t (*)(const void* thisObj, uint32_t key);

		hkPadSpu<GetSupportingVertexFunc> m_getSupportingVertexFunc;
		hkPadSpu<ConvertVertexIdsToVerticesFunc> m_convertVertexIdsToVertices;
		hkPadSpu<WeldContactPointFunc> m_weldContactPointFunc;
		hkPadSpu<GetCentreFunc> m_getCentreFunc;
		hkPadSpu<GetNumCollisionSpheresFunc> m_getNumCollisionSpheresFunc;
		hkPadSpu<GetCollisionSpheresFunc> m_getCollisionSpheresFunc;
		hkPadSpu<GetAabbFunc> m_getAabbFunc;
		hkPadSpu<CastRayFunc> m_castRay;
		hkPadSpu<CastRayWithCollectorFunc> m_castRayWithCollector;
		hkPadSpu<CastRayBundleFunc> m_castRayBundle;
		hkPadSpu<GetChildShapeFunc> m_getChildShapeFunc;
		hkPadSpu<GetCollisionFilterInfoFunc> m_getCollisionFilterInfoFunc;
	};

	hkpShape(hkpShapeType a1);

	// The SDK's finish constructor: a finished shape is invalid until its class sets its type.
	hkpShape(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag) {
		if (aFlag.m_finishing) {
			m_type.m_storage = HK_SHAPE_INVALID;
		}
	}

	~hkpShape() override;

	virtual float getMaximumProjection(const hkVector4& direction) const;
	virtual const hkpShapeContainer* getContainer() const { return nullptr; }
	virtual bool isConvex() const { return false; }
	virtual int32_t calcSizeForSpu(const hkpShape::CalcSizeForSpuInput&, int32_t) const;
	// PDB getAabbImpl (protected); IDA names the slot getAabb(hkTransform*, float, hkAabb*).
	virtual void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const = 0;
	// PDB castRayImpl (protected). The exe returns the hkBool through a hidden result pointer (ret 0xC).
	virtual hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& output) const = 0;
	// PDB castRayWithCollectorImpl (protected).
	virtual void castRayWithCollectorImpl(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
		hkpRayHitCollector& collector) const = 0;
	// PDB castRayBundleImpl (protected). The exe returns the hkVector4Comparison through a hidden pointer, so the
	// slot is thiscall with four stack words (ret 0x10): the result, the input, the output and the mask.
	virtual hkVector4Comparison* castRayBundleImpl(hkVector4Comparison* apResult,
		const hkpShapeRayBundleCastInput& input, hkpShapeRayBundleCastOutput& output,
		const hkVector4Comparison& mask) const;

	// The SDK's getAabb: getAabbImpl through the shape's vtable. The exe's out-of-line copy (thiscall, ret 0xC,
	// 0x6286A0) is registered in hkpShape.cpp.
	void getAabb(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
		getAabbImpl(localToWorld, tolerance, out);
	}

	// m_type, through the exe's shared one-field getter (thiscall, ret 0).
	hkpShapeType getType() const;

	// The SDK's castRay: castRayImpl through the shape's vtable; the hkBool comes back through a hidden pointer. The
	// exe's out-of-line copy (thiscall, ret 0xC, 0x632C30) is registered in ViewCaster.cpp.
	hkBool castRay(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& output) const {
		return castRayImpl(input, output);
	}

	bhkShape* m_userData;
	hkEnum<hkpShapeType, uint32_t> m_type;
};
inline constexpr fnvd::Vtable hkpShape_vftable{};
