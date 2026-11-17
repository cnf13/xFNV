#include "BSHavok/bhkCharControllerShape.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConvexShape.h"
#include "BSHavok/bhkConvexVerticesShape.h"
#include "BSHavok/bhkGeometryUtil.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkSphereRepShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkStridedVertices.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h"

#include <xmmintrin.h>

NiRTTI bhkCharControllerShape::ms_RTTI("bhkCharControllerShape", &bhkConvexVerticesShape::ms_RTTI);

hkCharControllerShape::hkCharControllerShape(const hkStridedVertices& aVerts, const hkArray<hkVector4>& arPlanes,
	float afRadius)
	: hkpConvexVerticesShape(aVerts, arPlanes, afRadius) {}

void hkCharControllerShape::getAabb(const hkTransform& arLocalToWorld, float afTolerance, hkAabb& arOut) const {
	__m128 v4 = _mm_add_ps(arLocalToWorld.m_translation.m_quad, m_aabbCenter.m_quad);
	__m128 v5 =
		_mm_add_ps(_mm_shuffle_ps(_mm_set_ss(afTolerance), _mm_set_ss(afTolerance), 0), m_aabbHalfExtents.m_quad);
	arOut.m_min.m_quad = _mm_sub_ps(v4, v5);
	arOut.m_max.m_quad = _mm_add_ps(v5, v4);
}

// The exe bumps the convex vertices shapes' count a second time here: this class has no count of its own.
bhkCharControllerShape::bhkCharControllerShape(hkConvexVerticesShapeCinfo& arInfo) {
	bhkCharControllerShape::Init(arInfo);
	++g_iNumBhkConvexVerticesShape;
}

const NiRTTI* bhkCharControllerShape::GetRTTI() const {
	return &bhkCharControllerShape::ms_RTTI;
}

// Builds the Havok shape from the creation info on the thread's Havok heap and hands it to the wrapper, which takes
// its own reference. The exe passes the info's vertices as a stack hkStridedVertices (16 bytes apart) and the plane
// equations by reference, and inlines the shape's constructor. The exe's cleanup-only EH frame is left out.
void bhkCharControllerShape::Init(const hkSerializableCinfo& arInfo) {
	const hkConvexVerticesShapeCinfo& rInfo = static_cast<const hkConvexVerticesShapeCinfo&>(arInfo);
	int32_t iOldMemContext = GetThreadMemContext();
	GetThreadMemContext() = MEM_CONTEXT_HAVOK;
	hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
	hkCharControllerShape* pShape = static_cast<hkCharControllerShape*>(kRouter.heap()
			.blockAlloc(static_cast<int32_t>(sizeof(hkCharControllerShape))));
	// The exe records the allocated size in the block before the constructor runs (a 2-byte store).
	pShape->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkCharControllerShape));
	hkStridedVertices kVertices;
	kVertices.m_vertices = &rInfo.m_vertices.m_data[0](0);
	kVertices.m_numVertices = rInfo.m_vertices.m_size;
	kVertices.m_striding = static_cast<int32_t>(sizeof(hkVector4));
	::new (pShape) hkCharControllerShape(kVertices, rInfo.m_planeEquations, rInfo.m_radius);
	// The exe dispatches both through the vtable (+0x8C and +0xBC), so call them through a base reference.
	bhkSerializable& rThis = *this;
	rThis.SethkObject(pShape);
	pShape->removeReference();
	rThis.PostInit(arInfo);
	GetThreadMemContext() = iOldMemContext;
}

// Builds a character controller shape from a hull's vertices: the plane equations and the rounded vertices the
// geometry utility makes from them go into a creation info on the stack, and the shape is built from that info on the
// NiMemObject heap. Empty vertices give no shape; the creation info is released either way. The exe's cleanup-only EH
// frame is left out.
bhkCharControllerShape* bhkCharControllerShape::CreateFromHull(hkArray<hkVector4, hkContainerHeapAllocator>& arVerts) {
	int32_t iOldMemContext = GetThreadMemContext();
	GetThreadMemContext() = MEM_CONTEXT_HAVOK;
	bhkCharControllerShape* pShape = nullptr;
	FNVD_INLINED_LOCAL(hkConvexVerticesShapeCinfo, kInfo, std::nullopt, std::nullopt);
	kInfo.m_userData = 0;
	kInfo.m_radius = 0.1f;
	kInfo.m_vertices.m_data = nullptr;
	kInfo.m_vertices.m_size = 0;
	kInfo.m_vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
	kInfo.m_planeEquations.m_data = nullptr;
	kInfo.m_planeEquations.m_size = 0;
	kInfo.m_planeEquations.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
	if (arVerts.m_size != 0) {
		bhkGeometryUtil::ComputePlanesFromVertices(arVerts, kInfo.m_planeEquations, kInfo.m_vertices);
		bhkCharControllerShape* pObject =
			static_cast<bhkCharControllerShape*>(NiMemObject::operator new(sizeof(bhkCharControllerShape)));
		if (pObject) {
			pShape = ::new (pObject) bhkCharControllerShape(kInfo);
		}
	}
	kInfo.~hkConvexVerticesShapeCinfo();
	GetThreadMemContext() = iOldMemContext;
	return pShape;
}
