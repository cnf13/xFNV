#include "Havok/Source/Physics/Collide/Shape/Deprecated/FastMesh/hkpFastMeshShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/Mesh/hkpMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

static void cleanupLoadedObjecthkpFastMeshShape(void* p);
// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
static const void* getVtablehkpFastMeshShape();

hkClass hkpFastMeshShapeClass("hkpFastMeshShape", &hkpMeshShapeClass, 96, nullptr, 0, nullptr, 0, nullptr, 0, nullptr,
	nullptr, 0, 0);
hkTypeInfo hkpFastMeshShapeTypeInfo = {"hkpFastMeshShape", finishLoadedObjecthkpFastMeshShape,
	cleanupLoadedObjecthkpFastMeshShape, getVtablehkpFastMeshShape(), &typeid(hkpFastMeshShape),
	sizeof(hkpFastMeshShape)};

static void cleanupLoadedObjecthkpFastMeshShape(void* p) {
	static_cast<hkpFastMeshShape*>(p)->~hkpFastMeshShape();
}

// Havok's getVtable: a finish-constructed hkpFastMeshShape on the stack, whose vtable is returned. The exe keeps only
// hkpMeshShape's finish constructor (flag 0) of it and returns the vtable as a constant.
// The local is an hkpMeshShape (the same 0x60 bytes): what the exe constructs in it is only that base.
static const void* getVtablehkpFastMeshShape() {
	return hkGetVtableOf<hkpFastMeshShape>();
}

// The triangle of key in the first subpart, built in buffer: the triangle shape's constructor (inlined) with the
// mesh's radius, the triangle's welding info (none without a welding array) and the mesh's welding type, then the
// three vertices times m_scaling. The indices are 16-bit; a subpart that flips alternate triangles swaps the second
// and third vertex of odd keys. The exe null-checks the buffer (placement new) but goes on to store the vertices at
// null.
const hkpShape* hkpFastMeshShape::getChildShape(uint32_t key, char (&buffer)[512]) const {
	const hkpMeshShape::Subpart& part = m_subparts.m_data[0];
	const int32_t vertexStriding = part.m_vertexStriding;
	// The index and vertex buffers are untyped strided memory the exe reads as 16-bit indices and 16-byte vertices.
	const uint16_t* indices =
		reinterpret_cast<const uint16_t*>(static_cast<const char*>(part.m_indexBase) + key * part.m_indexStriding);
	const uint32_t flip = key & part.m_flipAlternateTriangles;
	const char* vertexBase = reinterpret_cast<const char*>(part.m_vertexBase);
	const hkVector4* vertex0 = reinterpret_cast<const hkVector4*>(vertexBase + vertexStriding * indices[0]);
	const hkVector4* vertex1 = reinterpret_cast<const hkVector4*>(vertexBase + vertexStriding * indices[flip + 1]);
	const hkVector4* vertex2 =
		reinterpret_cast<const hkVector4*>(vertexBase + vertexStriding * indices[(flip ^ 1) + 1]);
	uint16_t weldingInfo;
	if (m_weldingInfo.m_size) {
		weldingInfo = m_weldingInfo.m_data[key + part.m_triangleOffset];
	} else {
		weldingInfo = 0;
	}
	hkpTriangleShape* pTriangle = nullptr;
	char* pBuffer = buffer;
	if (pBuffer) {
		const uint8_t ucWeldingType = m_weldingType.m_storage;
		pTriangle = ::new (pBuffer)
			hkpTriangleShape(m_radius, weldingInfo, static_cast<hkpWeldingUtility::WeldingType>(ucWeldingType));
	}
	pTriangle->m_vertexA.setMul4(*vertex0, m_scaling);
	pTriangle->m_vertexB.setMul4(*vertex1, m_scaling);
	pTriangle->m_vertexC.setMul4(*vertex2, m_scaling);
	return pTriangle;
}

const hkClass& hkpFastMeshShape::staticClass() {
	return hkpFastMeshShapeClass;
}
