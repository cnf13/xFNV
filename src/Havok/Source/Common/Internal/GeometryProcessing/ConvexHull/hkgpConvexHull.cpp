#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"

#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Algorithm/PseudoRandom/hkPseudoRandomGenerator.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkStridedVertices.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/AbstractMesh/hkgpAbstractMesh.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Util/Welding/hkHistogram.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "SDK/Windows/winbase.h"

#include <bit>
#include <cmath>
#include <new>
#include <x86intrin.h>
#include <xmmintrin.h>

// Sets out to the matrix that scales space by scale along the plane's normal (plane xyz, w its offset): the identity
// plus (scale - 1) n n^T, translation (scale - 1) n w, w row (0, 0, 0, 1); returns &out.
static hkMatrix4* calcPlanarScaleMatrix(const hkVector4& plane, float scale, hkMatrix4& out);

// generateIndexedFaces<int, int>'s guarded static: the null edge it starts each face's boundary search from, and its
// guard.
static hkgpConvexHullImpl::Edge s_generateIndexedFacesNullEdge{};
static uint32_t s_generateIndexedFacesNullEdge_guard;

// The SDK's scale along a plane, written from the asm: s = scale - 1 is stored as a float, s n and
// s n*n are SSE products, every element is an x87 product or sum of their lanes stored as a float.
static hkMatrix4* calcPlanarScaleMatrix(const hkVector4& plane, float scale, hkMatrix4& out) {
	const float s = scale - 1.0f;
	const hkQuadReal sss = _mm_shuffle_ps(_mm_set_ss(s), _mm_set_ss(s), 0);
	const hkQuadReal sn = _mm_mul_ps(sss, plane.m_quad);
	const hkQuadReal snn = _mm_mul_ps(sss, _mm_mul_ps(plane.m_quad, plane.m_quad));
	out.m_col0.m_quad[0] = snn[0] + 1.0f;
	out.m_col1.m_quad[0] = plane.m_quad[0] * sn[1];
	out.m_col2.m_quad[0] = plane.m_quad[0] * sn[2];
	out.m_col3.m_quad[0] = sn[0] * plane.m_quad[3];
	// The symmetric elements are copied from the stored ones.
	out.m_col0.m_quad[1] = out.m_col1.m_quad[0];
	out.m_col1.m_quad[1] = snn[1] + 1.0f;
	out.m_col2.m_quad[1] = plane.m_quad[1] * sn[2];
	out.m_col3.m_quad[1] = sn[1] * plane.m_quad[3];
	out.m_col0.m_quad[2] = out.m_col2.m_quad[0];
	out.m_col1.m_quad[2] = out.m_col2.m_quad[1];
	out.m_col2.m_quad[2] = snn[2] + 1.0f;
	out.m_col3.m_quad[2] = sn[2] * plane.m_quad[3];
	out.m_col0.m_quad[3] = 0.0f;
	out.m_col1.m_quad[3] = 0.0f;
	out.m_col2.m_quad[3] = 0.0f;
	out.m_col3.m_quad[3] = 1.0f;
	return &out;
}

// The flag bytes after m_allowLowerDimensions and m_ensurePlaneEnclosing are deliberately left uninitialised, as the
// game does.
hkgpConvexHull::BuildConfig::BuildConfig() : m_minCosAngle(0.9998999834060669f) {
	m_allowLowerDimensions.m_bool = 0;
	m_setSourceIndices.m_bool = 0;
	m_buildIndices.m_bool = 1;
	m_buildMassProperties.m_bool = 0;
	m_sortInputs.m_bool = 1;
	m_internalInputs.m_bool = 0;
	m_ensurePlaneEnclosing.m_bool = 0;
}

hkgpConvexHull::SimplifyConfig::SimplifyConfig() : m_maxVertices(-1), m_minArea(0.0f), m_minEdgeLength(0.0f) {
	m_ensureContainment.m_bool = 1;
	m_removeUnreferencedVertices.m_bool = 0;
	m_forceRebuild.m_bool = 0;
}

hkgpConvexHull::AbsoluteScaleConfig::AbsoluteScaleConfig() : m_method(SKM_PLANES), m_minComDistance(1.1920929e-07f) {}

// The implementation lives in a Havok heap block of its size, null when that fails.
hkgpConvexHull::hkgpConvexHull() : m_data(nullptr), m_userData(nullptr), m_userObject(nullptr) {
	void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(sizeof(hkgpConvexHullImpl)));
	if (pMemory) {
		m_data = new (pMemory) hkgpConvexHullImpl();
	}
}

// Destroys the implementation and frees its Havok heap block, then deletes the user object.
hkgpConvexHull::~hkgpConvexHull() {
	if (m_data) {
		m_data->~hkgpConvexHullImpl();
		hkMemoryRouter::getInstance().m_heap->blockFree(m_data, static_cast<int32_t>(sizeof(hkgpConvexHullImpl)));
	}
	delete m_userObject;
	m_userObject = nullptr;
}

// KEEP: unregistered. The exe body touches only eax, and absoluteScale keeps ecx live across its call (0xD78928,
// then getProjectionPlane 0xD78933 on that ecx; also 0xD78AFC and 0xD78B2D), which clang does not guarantee.
const hkgpConvexHull::BuildConfig& hkgpConvexHull::getConfiguration() const {
	return m_data->m_config;
}

// KEEP: unregistered. The exe body touches only eax, and expandByPlanarMargin keeps edx live across its call
// (0xD77C40, then mov edx, [edx+0x24] at 0xD77C67), which clang does not guarantee.
int32_t hkgpConvexHull::getDimensions() const {
	return m_data->m_dimensions;
}

// KEEP: unregistered. The exe body touches only eax, and its callers keep ecx live across the call (0xD7596D calls
// D75060 with the ecx it gave this one; checkbody: "caller relies on preserved edx"), which clang does not guarantee.
int32_t hkgpConvexHull::getNumVertices() const {
	return m_data->m_mesh.m_vertices.m_numUsed;
}

// The source vertices' positions, or the quantized vertices' decoded: x * m_scaleInv + m_origin, w 0.
void hkgpConvexHull::fetchPositions(hkgpConvexHull::Inputs inputs,
	hkArray<hkVector4, hkContainerHeapAllocator>& positions) const {
	if (inputs == hkgpConvexHull::SOURCE_VERTICES) {
		hkgpConvexHullImpl* pData = m_data;
		hkVector4* pOut = positions.expandBy(pData->m_mesh.m_vertices.m_numUsed);
		for (hkgpConvexHullImpl::Vertex* v = pData->m_mesh.m_vertices.m_used; v; v = v->m_next) {
			*pOut++ = v->m_source;
		}
	} else {
		hkgpConvexHullImpl* pData = m_data;
		hkVector4* pOut = positions.expandBy(pData->m_mesh.m_vertices.m_numUsed);
		for (hkgpConvexHullImpl::Vertex* v = pData->m_mesh.m_vertices.m_used; v; v = v->m_next) {
			__m128 x = _mm_setr_ps(static_cast<float>(v->m_x[0]), static_cast<float>(v->m_x[1]),
				static_cast<float>(v->m_x[2]), 0.0f);
			(pOut++)->m_quad = _mm_add_ps(_mm_mul_ps(x, pData->m_scaleInv.m_quad), pData->m_origin.m_quad);
		}
	}
}

hkgpConvexHull* hkgpConvexHull::clone() const {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkgpConvexHull* v3 = static_cast<hkgpConvexHull*>(Instance->m_heap->blockAlloc(sizeof(hkgpConvexHull)));
	v3->m_memSizeAndFlags = sizeof(hkgpConvexHull);
	hkgpConvexHull* v4 = ::new (v3) hkgpConvexHull;
	hkgpConvexHullImpl* m_data = v4->m_data;
	if (m_data) {
		v4->m_data->~hkgpConvexHullImpl();
		hkMemoryRouter* v7 = &hkMemoryRouter::getInstance();
		v7->m_heap->blockFree(m_data, sizeof(hkgpConvexHullImpl));
	}
	v4->m_data = 0;
	v4->m_data = this->m_data->clone();
	v4->m_userData = m_userData;
	if (m_userObject) {
		v4->m_userObject = m_userObject->clone();
	} else {
		v4->m_userObject = 0;
	}
	return v4;
}

// Returns 0 when the mass properties are there.
hkResult hkgpConvexHull::buildMassProperties() {
	return m_data->buildMassProperties();
}

bool hkgpConvexHull::decimateVertices(int32_t numVerticesToKeep, bool bEnsureContainment) {
	return m_data->decimateVertices(numVerticesToKeep, bEnsureContainment);
}

void hkgpConvexHull::simplify(const hkgpConvexHull::SimplifyConfig& config) {
	m_data->simplify(config);
}

float hkgpConvexHull::getVolume() const {
	FNVD_INLINED_LOCAL(hkErrStream, apThis, std::nullopt, std::nullopt);
	hkgpConvexHullImpl* m_data = this->m_data;
	if (m_data->m_hasMassProperties.m_bool) {
		hkgpConvexHullImpl* v6 = this->m_data;
		return m_data->m_volume;
	} else {
		char buf[512];
		::new (&apThis) hkErrStream(buf, 512);
		uint32_t m_uid = m_data->m_uid;
		hkOstream* v3 = &apThis.operator<<("No mass properties available (");
		hkOstream* v4 = &v3->operator<<(m_uid);
		v4->operator<<(")");
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x79F9D887, buf,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 334)) {
			__builtin_debugtrap();
		}
		FNVD_DESTRUCT(hkOstream)(&apThis);
		return this->m_data->m_volume;
	}
}

float hkgpConvexHull::getSurfaceArea() const {
	FNVD_INLINED_LOCAL(hkErrStream, apThis, std::nullopt, std::nullopt);
	hkgpConvexHullImpl* m_data = this->m_data;
	if (m_data->m_hasMassProperties.m_bool) {
		hkgpConvexHullImpl* v6 = this->m_data;
		return m_data->m_surface;
	} else {
		char buf[512];
		::new (&apThis) hkErrStream(buf, 512);
		uint32_t m_uid = m_data->m_uid;
		hkOstream* v3 = &apThis.operator<<("No mass properties available (");
		hkOstream* v4 = &v3->operator<<(m_uid);
		v4->operator<<(")");
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x79F9D887, buf,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 334)) {
			__builtin_debugtrap();
		}
		FNVD_DESTRUCT(hkOstream)(&apThis);
		return this->m_data->m_surface;
	}
}

// The volume is raised to 2/3 and rounded to float; the product with the constant pi^(1/3) 6^(2/3) is kept as a double
// before the area call, and the quotient is returned rounded to float.
float hkgpConvexHull::getSphericity() const {
	float fVolume = getVolume();
	float fPow = static_cast<float>(std::pow(fVolume, 0.6666666865348816));
	double dScaled = fPow * 4.835975646972656;
	return static_cast<float>(dScaled / getSurfaceArea());
}

int32_t hkgpConvexHull::getNumPlanes() const {
	FNVD_INLINED_LOCAL(hkErrStream, apThis, std::nullopt, std::nullopt);
	hkgpConvexHullImpl* m_data = this->m_data;
	if (m_data->m_hasIndexing.m_bool) {
		hkgpConvexHullImpl* v6 = this->m_data;
		return m_data->m_planes.m_size;
	} else {
		char buf[512];
		::new (&apThis) hkErrStream(buf, 512);
		uint32_t m_uid = m_data->m_uid;
		hkOstream* v3 = &apThis.operator<<("No index available (");
		hkOstream* v4 = &v3->operator<<(m_uid);
		v4->operator<<(")");
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x79F9D886, buf,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 328)) {
			__builtin_debugtrap();
		}
		FNVD_DESTRUCT(hkOstream)(&apThis);
		return this->m_data->m_planes.m_size;
	}
}

const hkVector4& hkgpConvexHull::getPlane(int32_t index) const {
	hkgpConvexHullImpl* m_data = this->m_data;
	if (!m_data->m_hasIndexing.m_bool) {
		char buf[512];
		hkErrStream apThis(buf, 512);
		uint32_t m_uid = m_data->m_uid;
		hkOstream* v4 = &apThis.operator<<("No index available (");
		hkOstream* v5 = &v4->operator<<(m_uid);
		v5->operator<<(")");
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x79F9D886, buf,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 328)) {
			__builtin_debugtrap();
		}
	}
	return this->m_data->m_planes.m_data[index];
}

// The plane count is re-read every iteration. The distance is the dot3 (y + x, then + z) plus the plane's w; a distance
// not below the maximum (NaN included) replaces it.
float hkgpConvexHull::maximumDistanceToPlanes(const hkVector4& point) const {
	float fMax = -3.40282002e+38f;
	for (int32_t i = 0; i < getNumPlanes(); ++i) {
		const hkVector4& plane = getPlane(i);
		float fDistance = point.m_quad[2] * plane.m_quad[2] +
			(point.m_quad[1] * plane.m_quad[1] + point.m_quad[0] * plane.m_quad[0]) + plane.m_quad[3];
		if (!(fDistance < fMax)) {
			fMax = fDistance;
		}
	}
	return fMax;
}

// Returns the box cached for source, rebuilt from the vertices (source positions, or the integer coordinates
// transformed back) when it is empty on any of x, y and z; then grown by radialScale times half its diagonal and scaled
// about its center by scale. Whether scale == 1 and radialScale > 0 together decide whether the cached box is copied
// out as it is.
hkAabb* hkgpConvexHull::getBoundingBox(hkAabb* apResult, Inputs source, float scale, float radialScale) const {
	hkgpConvexHullImpl* pData = m_data;
	hkAabb* pBox = &pData->m_boundingBoxes[source];
	if ((_mm_movemask_ps(_mm_cmple_ps(pBox->m_max.m_quad, pBox->m_min.m_quad)) & 7) != 0 && getNumVertices() > 0) {
		hkgpConvexHull::Vertex* pVertex = D75060();
		if (source == SOURCE_VERTICES) {
			const hkVector4* pPosition = hkgpConvexHull::D74ED0(pVertex);
			pBox->m_min.m_quad = pPosition->m_quad;
			pBox->m_max.m_quad = pPosition->m_quad;
			for (pVertex = hkgpConvexHull::D75120(pVertex); pVertex; pVertex = hkgpConvexHull::D75120(pVertex)) {
				pPosition = hkgpConvexHull::D74ED0(pVertex);
				pBox->m_min.m_quad = _mm_min_ps(pBox->m_min.m_quad, pPosition->m_quad);
				pBox->m_max.m_quad = _mm_max_ps(pBox->m_max.m_quad, pPosition->m_quad);
			}
		} else {
			hkVector4 position;
			pBox->m_min.m_quad = D74E80(&position, pVertex)->m_quad;
			pBox->m_max.m_quad = D74E80(&position, pVertex)->m_quad;
			for (pVertex = hkgpConvexHull::D75120(pVertex); pVertex; pVertex = hkgpConvexHull::D75120(pVertex)) {
				const hkVector4* pPosition = D74E80(&position, pVertex);
				pBox->m_min.m_quad = _mm_min_ps(pBox->m_min.m_quad, pPosition->m_quad);
				pBox->m_max.m_quad = _mm_max_ps(pBox->m_max.m_quad, pPosition->m_quad);
			}
		}
	}
	if (scale == 1.0f && !(radialScale > 0.0f)) {
		*apResult = *pBox;
		return apResult;
	}
	hkAabb box = *pBox;
	if (radialScale > 0.0f) {
		const __m128 diagonal = _mm_sub_ps(box.m_max.m_quad, box.m_min.m_quad);
		const __m128 sq = _mm_mul_ps(diagonal, diagonal);
		const float fLength = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))));
		const float fGrowth = radialScale * 0.5 * fLength;
		const __m128 growth = _mm_unpacklo_ps(_mm_unpacklo_ps(_mm_set_ss(fGrowth), _mm_set_ss(fGrowth)),
			_mm_unpacklo_ps(_mm_set_ss(fGrowth), _mm_set_ss(0.0f)));
		box.m_min.m_quad = _mm_sub_ps(box.m_min.m_quad, growth);
		box.m_max.m_quad = _mm_add_ps(growth, box.m_max.m_quad);
	}
	if (1.0f != scale) {
		const float fHalfScale = scale * 0.5;
		const __m128 halfExtents = _mm_mul_ps(_mm_set1_ps(fHalfScale), _mm_sub_ps(box.m_max.m_quad, box.m_min.m_quad));
		const __m128 center = _mm_mul_ps(_mm_set1_ps(0.5f), _mm_add_ps(box.m_max.m_quad, box.m_min.m_quad));
		box.m_min.m_quad = _mm_sub_ps(center, halfExtents);
		box.m_max.m_quad = _mm_add_ps(center, halfExtents);
	}
	*apResult = box;
	return apResult;
}

// The configuration is copied into the implementation (four dwords, padding included) before it builds.
int32_t hkgpConvexHull::build(const hkVector4* points, int32_t numPoints, const BuildConfig& config) {
	m_data->m_config = config;
	return m_data->build(points, numPoints, false, nullptr);
}

// A stride of 16 already is a contiguous hkVector4 array and goes straight to the three-argument build. Any other
// stride is first copied into a local array, each vertex's three floats and a zero w; the array's storage is freed
// after the build.
int32_t hkgpConvexHull::build(const hkStridedVertices& vertices, const BuildConfig& config) {
	// The vertices are raw memory: a stride-16 block is handed straight on as hkVector4s, and any other block is walked
	// a byte stride at a time reading its three leading floats.
	if (vertices.m_striding == 16) {
		return build(reinterpret_cast<const hkVector4*>(vertices.m_vertices), vertices.m_numVertices, config);
	}
	FNVD_INLINED_LOCAL(hkArray<hkVector4>, points, std::nullopt, std::nullopt);
	points.m_data = nullptr;
	points.m_size = 0;
	points.m_capacityAndFlags = 0x80000000u;
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	if (vertices.m_numVertices > 0) {
		int32_t iCapacity = 2 * (points.m_capacityAndFlags & 0x3FFFFFFF);
		if (vertices.m_numVertices >= iCapacity) {
			iCapacity = vertices.m_numVertices;
		}
		hkArrayUtil::_reserve(pHeap, &points, iCapacity, sizeof(hkVector4));
	}
	points.m_size = static_cast<uint32_t>(vertices.m_numVertices);
	const char* pVertex = reinterpret_cast<const char*>(vertices.m_vertices);
	for (int32_t i = 0; i < vertices.m_numVertices; ++i) {
		const float* pFloats = reinterpret_cast<const float*>(pVertex);
		hkVector4 vertex;
		vertex.m_quad[0] = pFloats[0];
		vertex.m_quad[1] = pFloats[1];
		vertex.m_quad[2] = pFloats[2];
		vertex.m_quad[3] = 0.0f;
		points.m_data[i] = vertex;
		pVertex += vertices.m_striding;
	}
	int32_t iResult = build(points.m_size != 0 ? points.m_data : nullptr, points.m_size, config);
	pHeap = hkMemoryRouter::getInstance().m_heap;
	points.m_size = 0;
	if (points.m_capacityAndFlags >= 0) {
		pHeap->bufFree(points.m_data, sizeof(hkVector4) * static_cast<int32_t>(points.m_capacityAndFlags & 0x3FFFFFFF));
	}
	return iResult;
}

// buildPlanar's strided form, as build's: a stride of 16 goes straight to buildPlanar_0, any other is first copied
// into a local array (each vertex's three floats and a zero w), freed after the build.
int32_t hkgpConvexHull::buildPlanar(const hkStridedVertices& vertices, const hkVector4& plane,
	const hkgpConvexHull::BuildConfig& config) {
	// The vertices are raw memory: a stride-16 block is handed straight on as hkVector4s, and any other block is walked
	// a byte stride at a time reading its three leading floats.
	if (vertices.m_striding == 16) {
		return buildPlanar_0(reinterpret_cast<const hkVector4*>(vertices.m_vertices), vertices.m_numVertices, plane,
			config);
	}
	const char* pVertex = reinterpret_cast<const char*>(vertices.m_vertices);
	const int32_t iNumVertices = vertices.m_numVertices;
	FNVD_INLINED_LOCAL(hkArray<hkVector4>, points, std::nullopt, std::nullopt);
	points.m_data = nullptr;
	points.m_size = 0;
	points.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	int32_t iCapacity = points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
	if (iCapacity < iNumVertices) {
		iCapacity *= 2;
		if (iNumVertices >= iCapacity) {
			iCapacity = iNumVertices;
		}
		hkArrayUtil::_reserve(pHeap, &points, iCapacity, sizeof(hkVector4));
	}
	points.m_size = static_cast<uint32_t>(iNumVertices);
	for (int32_t i = 0; i < vertices.m_numVertices; ++i) {
		const float* pFloats = reinterpret_cast<const float*>(pVertex);
		points.m_data[i].m_quad = _mm_setr_ps(pFloats[0], pFloats[1], pFloats[2], 0.0f);
		pVertex += vertices.m_striding;
	}
	const int32_t iResult = buildPlanar_0(points.m_size != 0 ? points.m_data : nullptr, points.m_size, plane, config);
	pHeap = hkMemoryRouter::getInstance().m_heap;
	points.m_size = 0;
	if (points.m_capacityAndFlags >= 0) {
		pHeap->bufFree(points.m_data,
			static_cast<int32_t>(sizeof(hkVector4)) *
				static_cast<int32_t>(points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK));
	}
	return iResult;
}

// Every three planes meeting in one point that no other plane has more than FLT_EPSILON in front of give a vertex; with
// more than three vertices the hull is built from them, else -1.
int32_t hkgpConvexHull::buildFromPlanes(const hkVector4* planes, int32_t numPlanes,
	const hkgpConvexHull::BuildConfig& config) {
	using points_t = hkArray<hkVector4, hkContainerHeapAllocator>;
	alignas(16) FNVD_INLINED_LOCAL(points_t, points, std::nullopt, std::nullopt);
	points.m_data = nullptr;
	points.m_size = 0;
	points.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
	HK_TIMER_BEGIN("VertexEnumeration", nullptr);
	points.reserve(3 * numPlanes);
	for (int32_t i = 0; i < numPlanes; ++i) {
		// The right-hand side's lanes are written one per loop, as the game does; its w is never read.
		__m128 rhs;
		rhs[0] = 0.0f - planes[i].m_quad[3];
		for (int32_t j = i + 1; j < numPlanes; ++j) {
			rhs[1] = 0.0f - planes[j].m_quad[3];
			for (int32_t k = j + 1; k < numPlanes; ++k) {
				// The planes' normals as the matrix's rows.
				hkMatrix3 m;
				m.m_col0.m_quad = _mm_setr_ps(planes[i].m_quad[0], planes[j].m_quad[0], planes[k].m_quad[0], 0.0f);
				m.m_col1.m_quad = _mm_setr_ps(planes[i].m_quad[1], planes[j].m_quad[1], planes[k].m_quad[1], 0.0f);
				m.m_col2.m_quad = _mm_setr_ps(planes[i].m_quad[2], planes[j].m_quad[2], planes[k].m_quad[2], 0.0f);
				rhs[2] = 0.0f - planes[k].m_quad[3];
				if (m.invert(1.1920929e-07f) != HK_SUCCESS) {
					continue;
				}
				const __m128 x = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(rhs, rhs, 0x55), m.m_col1.m_quad),
												_mm_mul_ps(_mm_shuffle_ps(rhs, rhs, 0x00), m.m_col0.m_quad)),
					_mm_mul_ps(_mm_shuffle_ps(rhs, rhs, 0xAA), m.m_col2.m_quad));
				bool bInside = true;
				for (int32_t l = 0; l < numPlanes; ++l) {
					if (l == i || l == j || l == k) {
						continue;
					}
					const __m128 p = planes[l].m_quad;
					const __m128 xp = _mm_mul_ps(x, p);
					const float fDistance = static_cast<float>(xp[2] + p[3]) + static_cast<float>(xp[1] + xp[0]);
					if (fDistance > 1.1920929e-07f) {
						bInside = false;
						break;
					}
				}
				if (bInside) {
					// EXE-INLINED none: hkArray<hkVector4>::pushBack
					hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
					if (points.m_size == (points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
						hkArrayUtil::_reserveMore(heap, &points, sizeof(hkVector4));
					}
					points.m_data[points.m_size].m_quad = x;
					++points.m_size;
				}
			}
		}
	}
	HK_TIMER_END();
	int32_t result;
	if (points.m_size > 3) {
		result = build(points.m_data, points.m_size, config);
	} else {
		result = -1;
	}
	hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
	points.m_size = 0;
	if (points.m_capacityAndFlags >= 0) {
		heap.bufFree(points.m_data,
			sizeof(hkVector4) * (points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK));
	}
	return result;
}

// As the three-plane form above, with projectionPlane as every vertex's third plane: each two planes meeting
// projectionPlane in one point that no plane but the two has more than FLT_EPSILON in front of give a vertex; with at
// least three vertices the planar hull is built from them, else -1.
int32_t hkgpConvexHull::buildFromPlanes(const hkVector4* planes, int32_t numPlanes, const hkVector4& projectionPlane,
	const hkgpConvexHull::BuildConfig& config) {
	using points_t = hkArray<hkVector4, hkContainerHeapAllocator>;
	FNVD_INLINED_LOCAL(points_t, points, std::nullopt, std::nullopt);
	points.m_data = nullptr;
	points.m_size = 0;
	points.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
	HK_TIMER_BEGIN("VertexEnumeration", nullptr);
	points.reserve(3 * numPlanes);
	for (int32_t i = 0; i < numPlanes; ++i) {
		// The right-hand side's lanes are written one per loop, as the game does; its w is never read.
		__m128 rhs;
		rhs[0] = 0.0f - planes[i].m_quad[3];
		for (int32_t j = i + 1; j < numPlanes; ++j) {
			rhs[1] = 0.0f - planes[j].m_quad[3];
			// The planes' normals as the matrix's rows.
			hkMatrix3 m;
			m.m_col0.m_quad = _mm_setr_ps(planes[i].m_quad[0], planes[j].m_quad[0], projectionPlane.m_quad[0], 0.0f);
			m.m_col1.m_quad = _mm_setr_ps(planes[i].m_quad[1], planes[j].m_quad[1], projectionPlane.m_quad[1], 0.0f);
			m.m_col2.m_quad = _mm_setr_ps(planes[i].m_quad[2], planes[j].m_quad[2], projectionPlane.m_quad[2], 0.0f);
			rhs[2] = 0.0f - projectionPlane.m_quad[3];
			if (m.invert(1.1920929e-07f) != HK_SUCCESS) {
				continue;
			}
			const __m128 x = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(rhs, rhs, 0x55), m.m_col1.m_quad),
											_mm_mul_ps(_mm_shuffle_ps(rhs, rhs, 0x00), m.m_col0.m_quad)),
				_mm_mul_ps(_mm_shuffle_ps(rhs, rhs, 0xAA), m.m_col2.m_quad));
			bool bInside = true;
			for (int32_t l = 0; l < numPlanes; ++l) {
				if (l == i || l == j) {
					continue;
				}
				const __m128 p = planes[l].m_quad;
				const __m128 xp = _mm_mul_ps(x, p);
				const float fDistance = static_cast<float>(xp[2] + p[3]) + static_cast<float>(xp[1] + xp[0]);
				if (fDistance > 1.1920929e-07f) {
					bInside = false;
					break;
				}
			}
			if (bInside) {
				hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
				if (points.m_size == (points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(heap, &points, sizeof(hkVector4));
				}
				points.m_data[points.m_size].m_quad = x;
				++points.m_size;
			}
		}
	}
	HK_TIMER_END();
	int32_t result;
	if (points.m_size < 3) {
		result = -1;
	} else {
		hkStridedVertices vertices;
		vertices.m_vertices = &points.m_data[0](0);
		vertices.m_numVertices = points.m_size;
		vertices.m_striding = sizeof(hkVector4);
		result = buildPlanar(vertices, projectionPlane, config);
	}
	hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
	points.m_size = 0;
	if (points.m_capacityAndFlags >= 0) {
		heap.bufFree(points.m_data,
			sizeof(hkVector4) * (points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK));
	}
	return result;
}

// The hull is rebuilt with its vertices or its planes moved out from its center of mass (its centroid when it has no
// mass properties). With SKM_VERTICES each source vertex's distance to the center is measured: a vertex whose
// distance is not above config.m_minComDistance is kept, the others move to the center plus their offset scaled to
// max(distance + offset, m_minComDistance) / distance. With SKM_PLANES each plane's w is lowered by
// max(m_minComDistance + plane dot center, offset). A 2D hull goes through the planar builds with the hull's
// projection plane, any other through the plain builds; the array's storage is freed, and the mass properties and
// the indices are rebuilt. A method that is neither scales nothing and goes straight to the rebuild.
void hkgpConvexHull::absoluteScale(float offset, const AbsoluteScaleConfig& config) {
	hkgpConvexHullImpl* pImpl = m_data;
	if (!pImpl->m_hasIndexing.m_bool) {
		char acBuffer[0x200];
		hkErrStream kStream(acBuffer, 0x200);
		// m_uid is read before the first operator<<.
		const uint32_t uiUid = pImpl->m_uid;
		hkOstream* pStream = &kStream.operator<<("No index available (");
		pStream = &pStream->operator<<(uiUid);
		pStream->operator<<(")");
		// A handler that returns nonzero asks for the break.
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x79F9D886, acBuffer,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 328)) {
			__builtin_debugtrap();
		}
	}
	hkVector4 center;
	if (pImpl->m_hasMassProperties.m_bool) {
		center = getCenterOfMass();
	} else {
		center = D752D0();
	}
	const int32_t iMethod = config.m_method;
	if (iMethod != AbsoluteScaleConfig::SKM_VERTICES && iMethod != AbsoluteScaleConfig::SKM_PLANES) {
		buildMassProperties();
		buildIndices(0.9999f, false);
		return;
	}
	const __m128 kCenter = _mm_load_ps(&reinterpret_cast<float*>(&center.m_quad)[0]);
	alignas(16) FNVD_INLINED_LOCAL(hkArray<hkVector4>, points, std::nullopt, std::nullopt);
	points.m_data = nullptr;
	points.m_size = 0;
	points.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
	if (iMethod == AbsoluteScaleConfig::SKM_VERTICES) {
		const int32_t iNumVertices = getNumVertices();
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		if (static_cast<int32_t>(points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) < iNumVertices) {
			hkArrayUtil::_reserve(pHeap, &points, iNumVertices, sizeof(hkVector4));
		}
		Vertex* pVertex = D75060();
		while (pVertex) {
			const hkVector4& position = *D74ED0(pVertex);
			const __m128 kOffset =
				_mm_sub_ps(_mm_load_ps(&reinterpret_cast<const float*>(&position.m_quad)[0]), kCenter);
			const __m128 kSquared = _mm_mul_ps(kOffset, kOffset);
			// Square root of z + (y + x), summed in that order.
			const float fDistance =
				_mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_add_ss(_mm_shuffle_ps(kSquared, kSquared, 0x55), kSquared),
					_mm_shuffle_ps(kSquared, kSquared, 0xAA))));
			hkVector4 scaled;
			if (config.m_minComDistance < fDistance) {
				float fLimit = fDistance + offset;
				if (fLimit < config.m_minComDistance) {
					fLimit = config.m_minComDistance;
				}
				_mm_store_ps(&reinterpret_cast<float*>(&scaled.m_quad)[0],
					_mm_add_ps(_mm_mul_ps(_mm_set1_ps(fLimit / fDistance), kOffset), kCenter));
			} else {
				scaled = position;
			}
			// EXE-INLINED none: hkArray<hkVector4>::pushBack
			hkMemoryRouter& router = hkMemoryRouter::getInstance();
			if (points.m_size == (points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*router.m_heap, &points, sizeof(hkVector4));
			}
			points.m_data[points.m_size] = scaled;
			points.m_size++;
			pVertex = D75120(pVertex);
		}
		const int32_t iDimensions = getDimensions();
		// vertices is 16-aligned because buildPlanar gets an aligned address.
		alignas(16) hkStridedVertices vertices;
		vertices.m_vertices = reinterpret_cast<const float*>(points.m_data);
		vertices.m_numVertices = points.m_size;
		vertices.m_striding = 16;
		if (iDimensions == 2) {
			const BuildConfig& buildConfig = getConfiguration();
			hkVector4 plane;
			getProjectionPlane(&plane);
			buildPlanar(vertices, plane, buildConfig);
		} else {
			build(vertices, getConfiguration());
		}
	} else {
		const int32_t iNumPlanes = getNumPlanes();
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		if (static_cast<int32_t>(points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) < iNumPlanes) {
			hkArrayUtil::_reserve(pHeap, &points, iNumPlanes, sizeof(hkVector4));
		}
		for (int32_t i = 0; i < getNumPlanes(); ++i) {
			hkVector4 plane = getPlane(i);
			const __m128 kPlane = _mm_load_ps(&reinterpret_cast<float*>(&plane.m_quad)[0]);
			const __m128 kProduct = _mm_mul_ps(kPlane, kCenter);
			// Sums in the order (z + w) + (y + x).
			const float fDot = _mm_cvtss_f32(_mm_add_ps(_mm_add_ss(_mm_shuffle_ps(kProduct, kProduct, 0xAA),
															_mm_shuffle_ps(kPlane, kPlane, 0xFF)),
				_mm_add_ss(_mm_shuffle_ps(kProduct, kProduct, 0x55), kProduct)));
			float fDistance = config.m_minComDistance + fDot;
			// The distance is kept only when offset is below it; an equal (a zero of the other sign) or unordered
			// compare takes offset.
			if (!(offset < fDistance)) {
				fDistance = offset;
			}
			plane.m_quad[3] -= fDistance;
			// EXE-INLINED none: hkArray<hkVector4>::pushBack
			hkMemoryRouter& router = hkMemoryRouter::getInstance();
			if (points.m_size == (points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*router.m_heap, &points, sizeof(hkVector4));
			}
			points.m_data[points.m_size] = plane;
			points.m_size++;
		}
		const int32_t iDimensions = getDimensions();
		if (iDimensions == 2) {
			const BuildConfig& buildConfig = getConfiguration();
			hkVector4 plane;
			getProjectionPlane(&plane);
			buildFromPlanes(points.m_data, points.m_size, plane, buildConfig);
		} else {
			buildFromPlanes(points.m_data, points.m_size, getConfiguration());
		}
	}
	// EXE-INLINED none: hkArray<hkVector4>::~hkArray
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	points.m_size = 0;
	if (points.m_capacityAndFlags >= 0) {
		pHeap->bufFree(points.m_data,
			sizeof(hkVector4) *
				static_cast<int32_t>(points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK));
	}
	buildMassProperties();
	buildIndices(0.9999f, false);
}

const hkVector4& hkgpConvexHull::getCenterOfMass() const {
	hkgpConvexHullImpl* m_data = this->m_data;
	if (!m_data->m_hasMassProperties.m_bool) {
		char buf[512];
		hkErrStream apThis(buf, 512);
		uint32_t m_uid = m_data->m_uid;
		hkOstream* v3 = &apThis.operator<<("No mass properties available (");
		hkOstream* v4 = &v3->operator<<(m_uid);
		v4->operator<<(")");
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x79F9D887, buf,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 334)) {
			__builtin_debugtrap();
		}
	}
	return this->m_data->m_localTransform.m_translation;
}

const hkVector4& hkgpConvexHull::D752D0() const {
	return m_data->getCentroid<hkgpConvexHullImpl::PositionAsSource>();
}

hkVector4* hkgpConvexHull::getProjectionPlane(hkVector4* apOut) const {
	*apOut = m_data->m_projectionPlane;
	return apOut;
}

// The count (getNumPlanes) is read once to size the array and once more to walk it, and the heap allocator is fetched
// between the two. The destination starts at the array's old end and each entry (getPlane) is copied as sixteen bytes.
void hkgpConvexHull::appendVertices(hkArray<hkVector4>& arVertices) {
	const int32_t iNumVertices = getNumPlanes();
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	const int32_t iOldSize = arVertices.m_size;
	const int32_t iNewSize = iOldSize + iNumVertices;
	const int32_t iCapacity =
		static_cast<int32_t>(arVertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK);
	if (iCapacity < iNewSize) {
		int32_t iRequest = 2 * iCapacity;
		if (iNewSize >= iRequest) {
			iRequest = iNewSize;
		}
		hkArrayUtil::_reserve(pHeap, &arVertices, iRequest, sizeof(hkVector4));
	}
	arVertices.m_size = static_cast<uint32_t>(iNewSize);
	hkVector4* pVertexOut = arVertices.m_data + iOldSize;
	const int32_t iCount = getNumPlanes();
	for (int32_t i = 0; i < iCount; ++i) {
		*pVertexOut = getPlane(i);
		++pVertexOut;
	}
}

// KEEP: unregistered. The exe body leaves edx intact and its one caller, getBoundingBox (0xD75920), keeps this in edx
// across the calls (`mov ecx, edx` right after the call at 0xD759EA), which clang does not guarantee.
// The integer coordinates of the implementation's vertex (the public vertex is its base) converted to floats, w the
// exe's 0.0f at 0x1011D78, scaled back by m_scaleInv and moved by m_origin.
hkVector4* hkgpConvexHull::D74E80(hkVector4* apOut, hkgpConvexHull::Vertex* apVertex) const {
	const hkgpConvexHullImpl* pData = m_data;
	const hkgpConvexHullImpl::Vertex* pVertex = static_cast<hkgpConvexHullImpl::Vertex*>(apVertex);
	const __m128 x = _mm_set_ss(static_cast<float>(pVertex->m_x[0]));
	const __m128 y = _mm_set_ss(static_cast<float>(pVertex->m_x[1]));
	const __m128 z = _mm_set_ss(static_cast<float>(pVertex->m_x[2]));
	const __m128 w = _mm_set_ss(0.0f);
	const __m128 xyzw = _mm_unpacklo_ps(_mm_unpacklo_ps(x, z), _mm_unpacklo_ps(y, w));
	apOut->m_quad = _mm_add_ps(_mm_mul_ps(xyzw, pData->m_scaleInv.m_quad), pData->m_origin.m_quad);
	return apOut;
}

// KEEP: unregistered. The exe body touches only eax, and a caller keeps edx live across the call (getBoundingBox holds
// this in edx from 0xD7592E to its `mov ecx, edx` at 0xD759A1, calling this at 0xD7596D in between), which clang does
// not guarantee.
hkgpConvexHull::Vertex* hkgpConvexHull::D75060() const {
	return m_data->m_mesh.m_vertices.m_used;
}

void hkgpConvexHull::buildIndices(float minCosAngle, bool forceRebuild) {
	if (forceRebuild) {
		m_data->m_hasIndexing.m_bool = 0;
	}
	m_data->m_config.m_minCosAngle = minCosAngle;
	m_data->buildIndices();
}

float hkgpConvexHull::getMinimumDistanceFromPlane(const hkVector4& plane) const {
	hkVector4 arg0;
	hkVector4 a2;
	arg0.m_quad = _mm_xor_ps(plane.m_quad, hkQuadRealNegateMask);
	getSupportingVertex(arg0, a2);
	__m128 v2 = _mm_mul_ps(plane.m_quad, a2.m_quad);
	return _mm_cvtss_f32(_mm_add_ss(_mm_add_ss(_mm_shuffle_ps(v2, v2, 0xAA),
										_mm_add_ss(_mm_shuffle_ps(v2, v2, 0x55), v2)),
		_mm_shuffle_ps(plane.m_quad, plane.m_quad, 0xFF)));
}

hkgpConvexHull::Side hkgpConvexHull::splitByPlane(const hkVector4& plane, hkgpConvexHull** pos, hkgpConvexHull** neg,
	float thickness) const {
	return m_data->splitByPlane(plane, pos, neg, thickness);
}

bool hkgpConvexHull::isPointInside(const hkVector4& point, bool usePlanes, float epsilon) const {
	return m_data->isPointInside(point, usePlanes, epsilon);
}

// Rebuilds the hull from its points pushed out by a margin, with the configuration of its last build: a point
// becomes the six points half of halfMargin away along the axes, a segment the eight points half of halfMargin away
// from its ends along two perpendiculars, a planar hull its vertices halfMargin away on either side of its plane
// (when that build is not three-dimensional, it is rebuilt with its center added and expanded again, whose result
// is returned), and a volume its triangles' vertices moved halfMargin along each triangle's normalized plane. A
// three-dimensional result gets its indices and mass properties back where the hull had them before; whether the
// result is three-dimensional.
char hkgpConvexHull::expandByPlanarMargin(float margin) {
	hkgpConvexHullImpl* pImpl = m_data;
	const float fHalf = margin * 0.5;
	alignas(16) BuildConfig config = pImpl->m_config;
	const hkBool hasMassProperties = pImpl->m_hasMassProperties;
	const hkBool hasIndexing = pImpl->m_hasIndexing;
	FNVD_INLINED_LOCAL(hkArray<hkVector4>, points, std::nullopt, std::nullopt);
	points.m_data = nullptr;
	points.m_size = 0;
	points.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
	// The strided view reads the vectors as floats from their first float on.
	alignas(16) hkStridedVertices vertices;
	switch (getDimensions()) {
	case 0: {
		// The hull's one vertex, six times, each copy moved by fHalf up or down one axis.
		const hkVector4 point = pImpl->m_mesh.m_vertices.m_used->m_source;
		// EXE-INLINED none: hkArray<hkVector4>::setSize
		points._reserve(*hkMemoryRouter::getInstance().m_heap, 6);
		for (int32_t i = points.m_size; i < 6; i++) {
			points.m_data[i] = point;
		}
		points.m_size = 6;
		points.m_data[0].m_quad[0] = points.m_data[0].m_quad[0] + fHalf;
		points.m_data[1].m_quad[0] = points.m_data[1].m_quad[0] - fHalf;
		points.m_data[2].m_quad[1] = points.m_data[2].m_quad[1] + fHalf;
		points.m_data[3].m_quad[1] = points.m_data[3].m_quad[1] - fHalf;
		points.m_data[4].m_quad[2] = points.m_data[4].m_quad[2] + fHalf;
		points.m_data[5].m_quad[2] = points.m_data[5].m_quad[2] - fHalf;
		vertices.m_vertices = reinterpret_cast<const float*>(points.m_data);
		vertices.m_numVertices = points.m_size;
		vertices.m_striding = sizeof(hkVector4);
		build(vertices, config);
		break;
	}
	case 1: {
		points._reserve(*hkMemoryRouter::getInstance().m_heap, 10);
		fetchPositions(SOURCE_VERTICES, points);
		const hkVector4 p0 = points.m_data[0];
		const hkVector4 p1 = points.m_data[1];
		hkVector4 direction;
		direction.setSub4(p1, p0);
		// EXE-INLINED none: hkVector4Util::calculatePerpendicularVector
		int32_t iMin = 0;
		int32_t iOk1 = 1;
		int32_t iOk2 = 2;
		float fA0 = std::fabs(direction.m_quad[0]);
		const float fA1 = std::fabs(direction.m_quad[1]);
		const float fA2 = std::fabs(direction.m_quad[2]);
		if (fA1 < fA0) {
			iOk1 = 0;
			iMin = 1;
			fA0 = fA1;
		}
		if (fA2 < fA0) {
			iOk2 = iMin;
		}
		hkVector4 perpendicular;
		perpendicular.m_quad = _mm_setzero_ps();
		perpendicular.m_quad[iOk1] = direction.m_quad[iOk2];
		perpendicular.m_quad[iOk2] = -direction.m_quad[iOk1];
		hkVector4 cross;
		cross.setCross(direction, perpendicular);
		// Both scaled to fHalf: normalize3 written out with intrinsics.
		const __m128 kZero = _mm_set_ss(zero);
		const __m128 kHalfMargin = _mm_set1_ps(fHalf);
		// EXE-INLINED 0x62AEA0
		__m128 kSquared = _mm_mul_ps(perpendicular.m_quad, perpendicular.m_quad);
		__m128 kLength2 = _mm_add_ss(_mm_shuffle_ps(kSquared, kSquared, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(kSquared, kSquared, 0x55), kSquared));
		__m128 kApprox = _mm_rsqrt_ss(kLength2);
		__m128 kInverse = _mm_andnot_ps(_mm_cmpeq_ss(kLength2, kZero),
			_mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), kApprox),
				_mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kLength2, kApprox), kApprox))));
		perpendicular.m_quad =
			_mm_mul_ps(_mm_mul_ps(_mm_shuffle_ps(kInverse, kInverse, 0), perpendicular.m_quad), kHalfMargin);
		// EXE-INLINED 0x62AEA0
		kSquared = _mm_mul_ps(cross.m_quad, cross.m_quad);
		kLength2 = _mm_add_ss(_mm_shuffle_ps(kSquared, kSquared, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(kSquared, kSquared, 0x55), kSquared));
		kApprox = _mm_rsqrt_ss(kLength2);
		kInverse = _mm_andnot_ps(_mm_cmpeq_ss(kLength2, kZero),
			_mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), kApprox),
				_mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kLength2, kApprox), kApprox))));
		cross.m_quad = _mm_mul_ps(_mm_mul_ps(_mm_shuffle_ps(kInverse, kInverse, 0), cross.m_quad), kHalfMargin);
		// Each end moved both ways along the perpendicular, then along the cross product.
		hkVector4 corners[8];
		corners[0].m_quad = _mm_add_ps(perpendicular.m_quad, p0.m_quad);
		corners[1].m_quad = _mm_sub_ps(p0.m_quad, perpendicular.m_quad);
		corners[2].m_quad = _mm_add_ps(cross.m_quad, p0.m_quad);
		corners[3].m_quad = _mm_sub_ps(p0.m_quad, cross.m_quad);
		corners[4].m_quad = _mm_add_ps(perpendicular.m_quad, p1.m_quad);
		corners[5].m_quad = _mm_sub_ps(p1.m_quad, perpendicular.m_quad);
		corners[6].m_quad = _mm_add_ps(cross.m_quad, p1.m_quad);
		corners[7].m_quad = _mm_sub_ps(p1.m_quad, cross.m_quad);
		for (int32_t i = 0; i < 8; i++) {
			// EXE-INLINED none: hkArray<hkVector4>::pushBack
			hkMemoryRouter& router = hkMemoryRouter::getInstance();
			if (points.m_size == (points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*router.m_heap, &points, sizeof(hkVector4));
			}
			points.m_data[points.m_size] = corners[i];
			points.m_size++;
		}
		vertices.m_vertices = reinterpret_cast<const float*>(points.m_data);
		vertices.m_numVertices = points.m_size;
		vertices.m_striding = sizeof(hkVector4);
		build(vertices, config);
		break;
	}
	case 2: {
		const hkVector4 center = D752D0();
		hkVector4 plane;
		getProjectionPlane(&plane);
		const int32_t iNumPoints = 3 * getNumVertices();
		points._reserve(*hkMemoryRouter::getInstance().m_heap, iNumPoints);
		fetchPositions(SOURCE_VERTICES, points);
		Vertex* pVertex = D75060();
		if (pVertex) {
			const float fNegMargin = -margin;
			do {
				const hkVector4* pPosition = D74ED0(pVertex);
				// EXE-INLINED none: hkArray<hkVector4>::pushBack
				hkMemoryRouter& aboveRouter = hkMemoryRouter::getInstance();
				if (points.m_size == (points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(*aboveRouter.m_heap, &points, sizeof(hkVector4));
				}
				points.m_data[points.m_size].m_quad =
					_mm_add_ps(_mm_mul_ps(_mm_set1_ps(margin), plane.m_quad), pPosition->m_quad);
				points.m_size++;
				// EXE-INLINED none: hkArray<hkVector4>::pushBack
				hkMemoryRouter& belowRouter = hkMemoryRouter::getInstance();
				if (points.m_size == (points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(*belowRouter.m_heap, &points, sizeof(hkVector4));
				}
				points.m_data[points.m_size].m_quad =
					_mm_add_ps(_mm_mul_ps(_mm_set1_ps(fNegMargin), plane.m_quad), pPosition->m_quad);
				points.m_size++;
				pVertex = D75120(pVertex);
			} while (pVertex);
		}
		vertices.m_vertices = reinterpret_cast<const float*>(points.m_data);
		vertices.m_numVertices = points.m_size;
		vertices.m_striding = sizeof(hkVector4);
		if (build(vertices, config) == 3) {
			break;
		}
		// Not a volume: the points again with the center added, expanded once more.
		points.m_size = 0;
		// EXE-INLINED none: hkArray<hkVector4>::pushBack
		hkMemoryRouter& router = hkMemoryRouter::getInstance();
		if (points.m_size == (points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
			hkArrayUtil::_reserveMore(*router.m_heap, &points, sizeof(hkVector4));
		}
		points.m_data[points.m_size] = center;
		points.m_size++;
		vertices.m_vertices = reinterpret_cast<const float*>(points.m_data);
		vertices.m_numVertices = points.m_size;
		vertices.m_striding = sizeof(hkVector4);
		build(vertices, config);
		const char cResult = expandByPlanarMargin(margin);
		// EXE-INLINED none: hkArray<hkVector4>::~hkArray
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		points.m_size = 0;
		if (points.m_capacityAndFlags >= 0) {
			pHeap->bufFree(points.m_data,
				static_cast<int32_t>(sizeof(hkVector4)) *
					static_cast<int32_t>(points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK));
		}
		return cResult;
	}
	case 3: {
		const int32_t iNumPoints = 6 * getNumTriangles();
		points._reserve(*hkMemoryRouter::getInstance().m_heap, iNumPoints);
		fetchPositions(SOURCE_VERTICES, points);
		Triangle* pTriangle = getFirstTriangle();
		while (pTriangle) {
			hkVector4 plane;
			calcTrianglePlane(&plane.m_quad, pTriangle);
			if (plane.isNormalized3(0.0001f).m_bool) {
				for (int32_t i = 0; i < 3; i++) {
					const hkVector4* pPosition = D74ED0(getVertex(pTriangle, i));
					// EXE-INLINED none: hkArray<hkVector4>::pushBack
					hkMemoryRouter& router = hkMemoryRouter::getInstance();
					if (points.m_size == (points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
						hkArrayUtil::_reserveMore(*router.m_heap, &points, sizeof(hkVector4));
					}
					points.m_data[points.m_size].m_quad =
						_mm_add_ps(_mm_mul_ps(_mm_set1_ps(margin), plane.m_quad), pPosition->m_quad);
					points.m_size++;
				}
			}
			pTriangle = getNext(pTriangle);
		}
		vertices.m_vertices = reinterpret_cast<const float*>(points.m_data);
		vertices.m_numVertices = points.m_size;
		vertices.m_striding = sizeof(hkVector4);
		build(vertices, config);
		break;
	}
	default: {
		alignas(16) char acBuffer[0x200];
		hkErrStream errStream(acBuffer, 0x200);
		errStream << "Invalid dimemsions for that operation";
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x172CDF48, acBuffer,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 2679)) {
			__builtin_debugtrap();
		}
		goto rebuildIndices;
	}
	}
	{
		// EXE-INLINED none: hkArray<hkVector4>::~hkArray
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		points.m_size = 0;
		if (points.m_capacityAndFlags >= 0) {
			pHeap->bufFree(points.m_data,
				static_cast<int32_t>(sizeof(hkVector4)) *
					static_cast<int32_t>(points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK));
		}
	}
rebuildIndices:
	if (getDimensions() != 3) {
		return 0;
	}
	if (hasIndexing.m_bool) {
		buildIndices(0.9999f, false);
	}
	if (hasMassProperties.m_bool) {
		buildMassProperties();
	}
	return 1;
}

int32_t hkgpConvexHull::getNumTriangles() {
	return m_data->m_mesh.m_triangles.m_numUsed;
}

hkgpConvexHull::Triangle* hkgpConvexHull::getFirstTriangle() const {
	return m_data->m_mesh.m_triangles.m_used;
}

const hkMatrix3& hkgpConvexHull::getWorldInertia() const {
	hkgpConvexHullImpl* m_data = this->m_data;
	if (!m_data->m_hasMassProperties.m_bool) {
		char buf[512];
		hkErrStream apThis(buf, 512);
		uint32_t m_uid = m_data->m_uid;
		hkOstream* v3 = &apThis.operator<<("No mass properties available (");
		hkOstream* v4 = &v3->operator<<(m_uid);
		v4->operator<<(")");
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x79F9D887, buf,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 334)) {
			__builtin_debugtrap();
		}
	}
	return this->m_data->m_inertiaMatrix;
}

void hkgpConvexHull::getSupportingVertex(const hkVector4& direction, hkVector4& vertexOut) const {
	using a2a_t = hkArray<hkVector4, hkContainerHeapAllocator>;
	FNVD_INLINED_LOCAL(a2a_t, a2a, std::nullopt, std::nullopt);
	hkgpConvexHullImpl* m_data = this->m_data;
	if (!m_data->m_supportCache.m_size) {
		int32_t m_numUsed = m_data->m_mesh.m_vertices.m_numUsed;
		a2a.m_data = 0;
		a2a.m_size = 0;
		a2a.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
		hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
		int32_t v6 = m_numUsed;
		if (m_numUsed > 0) {
			if (m_numUsed < 2 * (a2a.m_capacityAndFlags & 0x3FFFFFFF)) {
				v6 = 2 * (a2a.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK);
			}
			hkArrayUtil::_reserve(*m_heap, &a2a, v6, sizeof(hkVector4));
		}
		hkgpConvexHullImpl::Vertex* m_used = m_data->m_mesh.m_vertices.m_used;
		a2a.m_size += m_numUsed;
		for (hkVector4* i = a2a.begin(); m_used; ++i) {
			*i = m_used->m_source;
			m_used = m_used->m_next;
		}
		hkGeometryProcessing::buildTransposedArray(a2a, m_data->m_supportCache);
		hkMemoryAllocator* v9 = (&hkMemoryRouter::getInstance())->m_heap;
		a2a.m_size = 0;
		if (a2a.m_capacityAndFlags >= 0) {
			v9->bufFree(a2a.begin(), sizeof(hkVector4) * a2a.m_capacityAndFlags);
		}
	}
	hkGeometryProcessing::getSupportingVertex(m_data->m_supportCache, direction, vertexOut);
}

// Writes the vertex nearest to point into vertexOut, comparing squared distances and keeping the first of equal ones;
// vertexOut is point itself when the hull has no vertex. SOURCE_VERTICES reads the source positions, else the integer
// coordinates scaled back by m_scaleInv and moved by m_origin, with w 0.0f. A distance is kept only while below the
// best; an unordered one never is.
void hkgpConvexHull::getNearestVertex(Inputs inputs, const hkVector4& point, hkVector4& vertexOut) const {
	float fBest = rdata_104DA48;
	const hkgpConvexHullImpl* pData = m_data;
	vertexOut.m_quad = point.m_quad;
	if (inputs == SOURCE_VERTICES) {
		for (const hkgpConvexHullImpl::Vertex* v = pData->m_mesh.m_vertices.m_used; v; v = v->m_next) {
			const __m128 position = v->m_source.m_quad;
			const __m128 diff = _mm_sub_ps(point.m_quad, position);
			const __m128 sq = _mm_mul_ps(diff, diff);
			const __m128 xy = _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq);
			const float fDistance = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), xy));
			if (fBest > fDistance) {
				fBest = fDistance;
				vertexOut.m_quad = position;
			}
		}
	} else {
		for (const hkgpConvexHullImpl::Vertex* v = pData->m_mesh.m_vertices.m_used; v; v = v->m_next) {
			const __m128 x = _mm_set_ss(static_cast<float>(v->m_x[0]));
			const __m128 y = _mm_set_ss(static_cast<float>(v->m_x[1]));
			const __m128 z = _mm_set_ss(static_cast<float>(v->m_x[2]));
			const __m128 xyzw = _mm_unpacklo_ps(_mm_unpacklo_ps(x, z), _mm_unpacklo_ps(y, _mm_set_ss(0.0f)));
			const __m128 position = _mm_add_ps(_mm_mul_ps(xyzw, pData->m_scaleInv.m_quad), pData->m_origin.m_quad);
			const __m128 diff = _mm_sub_ps(point.m_quad, position);
			const __m128 sq = _mm_mul_ps(diff, diff);
			const __m128 xy = _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq);
			const float fDistance = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), xy));
			if (fBest > fDistance) {
				fBest = fDistance;
				vertexOut.m_quad = position;
			}
		}
	}
}

// As getNearestVertex, finds the vertex farthest from point, starting from a best distance of 0.0f: a vertex is kept
// only while strictly above the best; an equal or unordered distance never is.
void hkgpConvexHull::getFarthestVertex(Inputs inputs, const hkVector4& point, hkVector4& vertexOut) const {
	float fBest = 0.0f;
	const hkgpConvexHullImpl* pData = m_data;
	vertexOut.m_quad = point.m_quad;
	if (inputs == SOURCE_VERTICES) {
		for (const hkgpConvexHullImpl::Vertex* v = pData->m_mesh.m_vertices.m_used; v; v = v->m_next) {
			const __m128 position = v->m_source.m_quad;
			const __m128 diff = _mm_sub_ps(point.m_quad, position);
			const __m128 sq = _mm_mul_ps(diff, diff);
			const __m128 xy = _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq);
			const float fDistance = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), xy));
			if (fBest < fDistance) {
				fBest = fDistance;
				vertexOut.m_quad = position;
			}
		}
	} else {
		for (const hkgpConvexHullImpl::Vertex* v = pData->m_mesh.m_vertices.m_used; v; v = v->m_next) {
			const __m128 x = _mm_set_ss(static_cast<float>(v->m_x[0]));
			const __m128 y = _mm_set_ss(static_cast<float>(v->m_x[1]));
			const __m128 z = _mm_set_ss(static_cast<float>(v->m_x[2]));
			const __m128 xyzw = _mm_unpacklo_ps(_mm_unpacklo_ps(x, z), _mm_unpacklo_ps(y, _mm_set_ss(0.0f)));
			const __m128 position = _mm_add_ps(_mm_mul_ps(xyzw, pData->m_scaleInv.m_quad), pData->m_origin.m_quad);
			const __m128 diff = _mm_sub_ps(point.m_quad, position);
			const __m128 sq = _mm_mul_ps(diff, diff);
			const __m128 xy = _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq);
			const float fDistance = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), xy));
			if (fBest < fDistance) {
				fBest = fDistance;
				vertexOut.m_quad = position;
			}
		}
	}
}

// The hull as a geometry: its vertices (the source ones, or the quantized ones decoded) appended to the geometry's,
// and its triangles, or for a planar hull a fan of triangles on both sides. Hulls of fewer dimensions add nothing.
void hkgpConvexHull::generateGeometry(hkgpConvexHull::Inputs inputs, hkGeometry& geometry) const {
	const hkgpConvexHullImpl* pData = m_data;
	if (pData->m_dimensions == 2) {
		const int32_t vertexBase = geometry.m_vertices.m_size;
		using indices_t = hkArray<int32_t, hkContainerHeapAllocator>;
		FNVD_INLINED_LOCAL(indices_t, indices, std::nullopt, std::nullopt);
		indices.m_data = nullptr;
		indices.m_size = 0;
		indices.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
		hkVector4* pVertex = geometry.m_vertices.expandBy(pData->m_mesh.m_vertices.m_numUsed);
		hkGeometry::Triangle* pTriangle = geometry.m_triangles.expandBy(2 * m_data->m_mesh.m_vertices.m_numUsed - 4);
		indices.reserve(m_data->m_mesh.m_vertices.m_numUsed);
		for (hkgpConvexHullImpl::Vertex* v = m_data->m_mesh.m_vertices.m_used; v; v = v->m_next) {
			// EXE-INLINED none: hkArray<int>::pushBackUnchecked
			indices.m_data[indices.m_size] = vertexBase + indices.m_size;
			++indices.m_size;
			if (inputs == hkgpConvexHull::SOURCE_VERTICES) {
				*pVertex++ = v->m_source;
			} else {
				__m128 x = _mm_setr_ps(static_cast<float>(v->m_x[0]), static_cast<float>(v->m_x[1]),
					static_cast<float>(v->m_x[2]), 0.0f);
				(pVertex++)->m_quad = _mm_add_ps(_mm_mul_ps(x, m_data->m_scaleInv.m_quad), m_data->m_origin.m_quad);
			}
		}
		// Only the corners are written: the triangles' m_material stays as expandBy left it.
		for (int32_t i = 1; i < indices.m_size - 1; ++i) {
			pTriangle->m_a = indices.m_data[0];
			pTriangle->m_b = indices.m_data[i];
			pTriangle->m_c = indices.m_data[i + 1];
			++pTriangle;
			pTriangle->m_a = indices.m_data[0];
			pTriangle->m_b = indices.m_data[i + 1];
			pTriangle->m_c = indices.m_data[i];
			++pTriangle;
		}
		// EXE-INLINED none: hkArray<int>::~hkArray
		hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
		indices.m_size = 0;
		if (indices.m_capacityAndFlags >= 0) {
			heap.bufFree(indices.m_data,
				sizeof(int32_t) * (indices.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK));
		}
	} else if (pData->m_dimensions == 3) {
		using MapBase = hkUlongMapBase;
		const int32_t vertexBase = geometry.m_vertices.m_size;
		using vertexIndices_t = hkPointerMap<const hkgpConvexHullImpl::Vertex*, int32_t>;
		FNVD_INLINED_LOCAL(vertexIndices_t, vertexIndices, std::nullopt, std::nullopt);
		MapBase& map = vertexIndices.m_map;
		map.m_elem = nullptr;
		map.m_numElems = 0;
		map.m_hashMod = -1;
		map.reserve(hkMemoryRouter::getInstance().heap(), 0);
		hkVector4* pVertex = geometry.m_vertices.expandBy(m_data->m_mesh.m_vertices.m_numUsed);
		hkGeometry::Triangle* pTriangle = geometry.m_triangles.expandBy(m_data->m_mesh.m_triangles.m_numUsed);
		const int32_t numVertices = m_data->m_mesh.m_vertices.m_numUsed;
		map.reserve(hkMemoryRouter::getInstance().heap(), numVertices);
		for (hkgpConvexHullImpl::Vertex* v = m_data->m_mesh.m_vertices.m_used; v; v = v->m_next) {
			vertexIndices.insert(v, vertexBase + (map.m_numElems & MapBase::NUM_ELEMS_MASK));
			if (inputs == hkgpConvexHull::SOURCE_VERTICES) {
				*pVertex++ = v->m_source;
			} else {
				__m128 x = _mm_setr_ps(static_cast<float>(v->m_x[0]), static_cast<float>(v->m_x[1]),
					static_cast<float>(v->m_x[2]), 0.0f);
				(pVertex++)->m_quad = _mm_add_ps(_mm_mul_ps(x, m_data->m_scaleInv.m_quad), m_data->m_origin.m_quad);
			}
		}
		for (hkgpConvexHullImpl::Triangle* t = m_data->m_mesh.m_triangles.m_used; t; t = t->m_next) {
			pTriangle->m_a = vertexIndices.getWithDefault(t->m_vertices[0], -1);
			pTriangle->m_b = vertexIndices.getWithDefault(t->m_vertices[1], -1);
			pTriangle->m_c = vertexIndices.getWithDefault(t->m_vertices[2], -1);
			pTriangle->m_material = -1;
			++pTriangle;
		}
		map.clearAndDeallocate(hkMemoryRouter::getInstance().heap());
		map.destructOperations();
	}
}

double hkgpConvexHull::getMaximumDistanceFromPlane(hkVector4* a1) const {
	hkVector4 a2;
	getSupportingVertex(*a1, a2);
	__m128 v2 = _mm_mul_ps(a1->m_quad, a2.m_quad);
	return _mm_cvtss_f32(_mm_add_ss(_mm_add_ss(_mm_shuffle_ps(v2, v2, 0xAA),
										_mm_add_ss(_mm_shuffle_ps(v2, v2, 0x55), v2)),
		_mm_shuffle_ps(a1->m_quad, a1->m_quad, 0xFF)));
}

// The support distance along the plane's normal (w 0.0f) minus the hull's minimum distance from that plane: the first
// kept as a double, the difference rounded to float.
double hkgpConvexHull::getWidth(const hkVector4* a1) const {
	hkVector4 plane;
	plane.m_quad = _mm_unpacklo_ps(_mm_unpacklo_ps(_mm_load_ss(&(*a1)(0)), _mm_load_ss(&(*a1)(2))),
		_mm_unpacklo_ps(_mm_load_ss(&(*a1)(1)), _mm_set_ss(0.0f)));
	const double dSupport = getMaximumDistanceFromPlane(&plane);
	const float fResult = dSupport - getMinimumDistanceFromPlane(plane);
	return fResult;
}

// Output starts as a copy of the hull; each round splits every hull of output by its major plane (after building its
// mass properties), the two parts replacing it (pushed, then the hull removed by moving the last element into its
// place) and the hull deleted; a split that leaves one part empty deletes the parts and keeps the hull. Each pushBack
// stores the element before it increments the size, as the game does where the two may overlap.
void hkgpConvexHull::subdivide(int32_t recursions, hkArray<hkgpConvexHull*, hkContainerHeapAllocator>& output,
	float thickness) const {
	output.m_size = 0;
	hkgpConvexHull* pCopy = clone();
	// EXE-INLINED none: hkArray<hkgpConvexHull*, hkContainerHeapAllocator>::pushBack
	hkMemoryAllocator& copyHeap = hkMemoryRouter::getInstance().heap();
	if (output.m_size == output.getCapacity()) {
		hkArrayUtil::_reserveMore(copyHeap, &output, static_cast<int32_t>(sizeof(hkgpConvexHull*)));
	}
	output.m_data[output.m_size] = pCopy;
	output.m_size += 1;
	if (recursions <= 0) {
		return;
	}
	int32_t remaining = recursions;
	do {
		const int32_t numHulls = output.m_size;
		for (int32_t i = 0; i < numHulls; ++i) {
			hkgpConvexHull* pHull = output.m_data[i];
			pHull->buildMassProperties();
			hkgpConvexHull* above;
			hkgpConvexHull* below;
			hkVector4 plane;
			const float fThickness = thickness;
			const hkVector4* pPlane = pHull->getMajorPlane(&plane);
			if (pHull->splitByPlane(*pPlane, &above, &below, fThickness) != CROSSING) {
				continue;
			}
			if (above && below) {
				// EXE-INLINED none: hkArray<hkgpConvexHull*, hkContainerHeapAllocator>::pushBack
				hkMemoryAllocator& aboveHeap = hkMemoryRouter::getInstance().heap();
				if (output.m_size == output.getCapacity()) {
					hkArrayUtil::_reserveMore(aboveHeap, &output, static_cast<int32_t>(sizeof(hkgpConvexHull*)));
				}
				output.m_data[output.m_size] = above;
				output.m_size += 1;
				// EXE-INLINED none: hkArray<hkgpConvexHull*, hkContainerHeapAllocator>::pushBack
				hkMemoryAllocator& belowHeap = hkMemoryRouter::getInstance().heap();
				if (output.m_size == output.getCapacity()) {
					hkArrayUtil::_reserveMore(belowHeap, &output, static_cast<int32_t>(sizeof(hkgpConvexHull*)));
				}
				output.m_data[output.m_size] = below;
				// EXE-INLINED none: hkArray<hkgpConvexHull*, hkContainerHeapAllocator>::removeAt
				const int32_t last = output.m_size;
				if (last != i) {
					output.m_data[i] = output.m_data[last];
				}
				if (pHull) {
					delete pHull;
				}
				continue;
			}
			if (above) {
				delete above;
			}
			if (below) {
				delete below;
			}
		}
	} while (--remaining != 0);
}

void hkgpConvexHull::deleteUserObject() {
	hkgpConvexHull::IUserObject* m_userObject = this->m_userObject;
	if (m_userObject) {
		delete m_userObject;
	}
	this->m_userObject = 0;
}

uint32_t hkgpConvexHull::uid() {
	return m_data->m_uid;
}

// A copy of m_data->m_projectionAxis.
hkVector4* hkgpConvexHull::getProjectionAxis(hkVector4* apResult) {
	*apResult = m_data->m_projectionAxis;
	return apResult;
}

bool hkgpConvexHull::clipLine(const hkVector4& a, const hkVector4& b, float& t0, float& t1, float epsilon) const {
	return m_data->clipLine(a, b, t0, t1, epsilon);
}

// The vertex's index (the implementation's vertex m_index, +0x34), after an error report when the hull has no
// indexing.
int32_t hkgpConvexHull::getIndex(const hkgpConvexHull::Vertex* a1) const {
	FNVD_INLINED_LOCAL(hkErrStream, apThis, std::nullopt, std::nullopt);
	char buf[512];
	hkgpConvexHullImpl* pData = m_data;
	if (pData->m_hasIndexing.m_bool) {
		return static_cast<const hkgpConvexHullImpl::Vertex*>(a1)->m_index;
	}
	::new (&apThis) hkErrStream(buf, 512);
	uint32_t uiUid = pData->m_uid;
	hkOstream* v3 = &apThis.operator<<("No index available (");
	hkOstream* v4 = &v3->operator<<(uiUid);
	v4->operator<<(")");
	if (hkError::s_instance->message(hkError::MESSAGE_ERROR, static_cast<int32_t>(0x79F9D886u), buf,
			".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 328)) {
		__builtin_debugtrap();
	}
	FNVD_DESTRUCT(hkOstream)(&apThis);
	return static_cast<const hkgpConvexHullImpl::Vertex*>(a1)->m_index;
}

// The triangle's index (the implementation's triangle m_index, +0x44), after an error report when the hull has no
// indexing.
int32_t hkgpConvexHull::getIndex(const hkgpConvexHull::Triangle* a1) const {
	FNVD_INLINED_LOCAL(hkErrStream, apThis, std::nullopt, std::nullopt);
	char buf[512];
	hkgpConvexHullImpl* pData = m_data;
	if (pData->m_hasIndexing.m_bool) {
		return static_cast<const hkgpConvexHullImpl::Triangle*>(a1)->m_index;
	}
	::new (&apThis) hkErrStream(buf, 512);
	uint32_t uiUid = pData->m_uid;
	hkOstream* v3 = &apThis.operator<<("No index available (");
	hkOstream* v4 = &v3->operator<<(uiUid);
	v4->operator<<(")");
	if (hkError::s_instance->message(hkError::MESSAGE_ERROR, static_cast<int32_t>(0x79F9D886u), buf,
			".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 328)) {
		__builtin_debugtrap();
	}
	FNVD_DESTRUCT(hkOstream)(&apThis);
	return static_cast<const hkgpConvexHullImpl::Triangle*>(a1)->m_index;
}

// The index of the triangle's plane (its quantized plane's m_index, +0x38), after an error report when the hull has
// no indexing.
int32_t hkgpConvexHull::getPlaneIndex(const hkgpConvexHull::Triangle* triangle) const {
	FNVD_INLINED_LOCAL(hkErrStream, apThis, std::nullopt, std::nullopt);
	char buf[512];
	hkgpConvexHullImpl* pData = m_data;
	if (pData->m_hasIndexing.m_bool) {
		return static_cast<const hkgpConvexHullImpl::Triangle*>(triangle)->m_plane.m_index;
	}
	::new (&apThis) hkErrStream(buf, 512);
	uint32_t uiUid = pData->m_uid;
	hkOstream* v3 = &apThis.operator<<("No index available (");
	hkOstream* v4 = &v3->operator<<(uiUid);
	v4->operator<<(")");
	if (hkError::s_instance->message(hkError::MESSAGE_ERROR, static_cast<int32_t>(0x79F9D886u), buf,
			".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 328)) {
		__builtin_debugtrap();
	}
	FNVD_DESTRUCT(hkOstream)(&apThis);
	return static_cast<const hkgpConvexHullImpl::Triangle*>(triangle)->m_plane.m_index;
}

// KEEP: unregistered. The exe body touches only eax and ecx, and a caller relies on edx surviving the call (checkbody:
// "caller relies on preserved edx"), which clang does not guarantee.
hkgpConvexHull::Vertex* hkgpConvexHull::getVertex(const hkgpConvexHull::Triangle* triangle, int32_t index) const {
	return static_cast<const hkgpConvexHullImpl::Triangle*>(triangle)->m_vertices[index];
}

// The implementation's triangle list link, of which the public triangle is the base.
hkgpConvexHull::Triangle* hkgpConvexHull::getNext(const hkgpConvexHull::Triangle* triangle) const {
	return static_cast<const hkgpConvexHullImpl::Triangle*>(triangle)->m_next;
}

// The SDK reads the implementation's triangle behind the public one. Its link packs the neighbour's address with the
// shared edge's index in the low two bits (hkgpTopology's Uid), so the address is the link with those bits cleared.
hkgpConvexHull::Triangle* hkgpConvexHull::getNeighbour(hkgpConvexHull::Triangle* triangle, int32_t index) const {
	const hkgpConvexHullImpl::Triangle* pTriangle = static_cast<const hkgpConvexHullImpl::Triangle*>(triangle);
	return reinterpret_cast<hkgpConvexHullImpl::Triangle*>(pTriangle->m_links[index] & ~static_cast<uintptr_t>(3));
}

// The plane through the inertia space's origin whose normal is the inertia axis of the largest reciprocal diagonal
// entry (the smallest moment), after an error report when the hull has no mass properties. The reciprocal is taken on
// (d.x, d.y, d.z, 1) and made absolute. w is 0.0f - dot3.
hkVector4* hkgpConvexHull::getMajorPlane(hkVector4* apResult) const {
	alignas(16) char buf[512];
	hkTransform inertiaSpace;
	hkVector4 inertiaDiagonal;
	const hkgpConvexHullImpl* pData = m_data;
	if (!pData->m_hasMassProperties.m_bool) {
		alignas(16) hkErrStream kErrStream(buf, 512);
		uint32_t uiUid = pData->m_uid;
		hkOstream* v4 = &kErrStream.operator<<("No mass properties available (");
		hkOstream* v5 = &v4->operator<<(uiUid);
		v5->operator<<(")");
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, static_cast<int32_t>(0x79F9D887u), buf,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 334)) {
			__builtin_debugtrap();
		}
	}
	getLocalInertia(inertiaSpace, inertiaDiagonal);
	const __m128 d =
		_mm_shuffle_ps(inertiaDiagonal.m_quad, _mm_unpackhi_ps(inertiaDiagonal.m_quad, hkQuadReal1111), 0xC4);
	const __m128 r = _mm_rcp_ps(d);
	hkVector4 inv;
	inv.m_quad = _mm_and_ps(hkVector4_setAbs4_absMask, _mm_sub_ps(_mm_add_ps(r, r), _mm_mul_ps(_mm_mul_ps(r, d), r)));
	int32_t iAxis;
	if (inv(1) > inv(0)) {
		iAxis = inv(2) > inv(1) ? 2 : 1;
	} else {
		iAxis = inv(2) > inv(0) ? 2 : 0;
	}
	const __m128 translation = inertiaSpace.m_translation.m_quad;
	*apResult = inertiaSpace.m_rotation.getColumn(iAxis);
	const __m128 prod = _mm_mul_ps(apResult->m_quad, translation);
	const __m128 dot = _mm_add_ss(_mm_shuffle_ps(prod, prod, 0xAA), _mm_add_ss(_mm_shuffle_ps(prod, prod, 0x55), prod));
	const float fW = _mm_cvtss_f32(_mm_sub_ss(_mm_setzero_ps(), dot));
	(*apResult)(3) = fW;
	return apResult;
}

int32_t hkgpConvexHull::classify(__m128* a1, float a2, float* a3) {
	int32_t v5[2];
	return m_data->classify(a1, a2, v5, a3);
}

void hkgpConvexHull::reset() {
	m_data->reset(0);
	hkgpConvexHull::IUserObject* m_userObject = this->m_userObject;
	if (m_userObject) {
		delete m_userObject;
	}
	this->m_userObject = 0;
}

__m128* hkgpConvexHull::calcTrianglePlane(__m128* a1, hkgpConvexHull::Triangle* a2) {
	const __m128 v0 = hkgpConvexHull::D74ED0(getVertex(a2, 0))->m_quad;
	const __m128 e1 = _mm_sub_ps(hkgpConvexHull::D74ED0(getVertex(a2, 1))->m_quad, v0);
	const __m128 e2 = _mm_sub_ps(hkgpConvexHull::D74ED0(getVertex(a2, 2))->m_quad, v0);
	// The normal e1 x e2, scaled by its length's reciprocal square root refined by one Newton step (0 for a zero
	// length).
	const __m128 n = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(e2, e2, 0xD2), _mm_shuffle_ps(e1, e1, 0xC9)),
		_mm_mul_ps(_mm_shuffle_ps(e2, e2, 0xC9), _mm_shuffle_ps(e1, e1, 0xD2)));
	const __m128 n2 = _mm_mul_ps(n, n);
	const float fXY = _mm_shuffle_ps(n2, n2, 0x55)[0] + n2[0];
	__m128 len2 = _mm_shuffle_ps(n2, n2, 0xAA);
	len2[0] = len2[0] + fXY;
	const float fRsqrt = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(len2[0])));
	const __m128 isZero = _mm_cmpeq_ss(len2, _mm_set_ss(0.0f));
	const float fRefine = 3.0f - static_cast<float>(static_cast<float>(len2[0] * fRsqrt) * fRsqrt);
	__m128 invLen = _mm_setzero_ps();
	invLen[0] = static_cast<float>(0.5f * fRsqrt) * fRefine;
	invLen = _mm_andnot_ps(isZero, invLen);
	const __m128 normal = _mm_mul_ps(_mm_shuffle_ps(invLen, invLen, 0x00), n);
	*a1 = normal;
	const __m128 nv0 = _mm_mul_ps(normal, v0);
	(*a1)[3] = 0.0f -
		static_cast<float>(_mm_shuffle_ps(nv0, nv0, 0xAA)[0] +
			static_cast<float>(_mm_shuffle_ps(nv0, nv0, 0x55)[0] + nv0[0]));
	return a1;
}

// Whether a mesh vertex has the quantized x and y of a1 (z is not compared): a1 is moved by m_origin and scaled by
// m_scale, then each of x, y and z, as a float, is clamped above at 8191, clamped below at -8192 with -0.5 added, else
// has 0.5 added (-0.5 when negative), and is truncated. An unordered value is neither clamped nor negative.
char hkgpConvexHull::isVertexUsed(const hkVector4* a1) {
	hkgpConvexHullImpl* pData = m_data;
	hkVector4 scaled;
	scaled.m_quad = _mm_mul_ps(_mm_sub_ps(a1->m_quad, pData->m_origin.m_quad), pData->m_scale.m_quad);
	int32_t aiX[3];
	for (int32_t i = 0; i < 3; ++i) {
		const float fLane = scaled(i);
		const float fClamped = fLane > 8191.0 ? 8191.0f : fLane;
		float fValue;
		float fHalf;
		if (fClamped < -8192.0) {
			fValue = -8192.0f;
			fHalf = -0.5f;
		} else {
			fValue = fClamped;
			fHalf = fClamped < 0.0f ? -0.5f : 0.5f;
		}
		aiX[i] = static_cast<int32_t>(static_cast<double>(fHalf) + fValue);
	}
	for (hkgpConvexHullImpl::Vertex* pVertex = pData->m_mesh.m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
		if (aiX[0] == pVertex->m_x[0] && aiX[1] == pVertex->m_x[1]) {
			return 1;
		}
	}
	return 0;
}

double hkgpConvexHull::getEdgeLengthSum() {
	return m_data->getEdgeLengthSum();
}

// Returns the three inertia axes, ordered by the hull's extent along each (getWidth, largest first), each normalized
// unless zero and made a plane through the inertia space's origin (w = 0.0f - dot3), after an error report when the
// hull has no mass properties.
void hkgpConvexHull::getMajorPlanes(hkVector4* planes) const {
	// The stream and the buffer are 16-aligned.
	alignas(16) char buf[512];
	hkTransform inertiaSpace;
	hkVector4 inertiaDiagonal;
	const hkgpConvexHullImpl* pData = m_data;
	if (!pData->m_hasMassProperties.m_bool) {
		alignas(16) hkErrStream kErrStream(buf, 512);
		uint32_t uiUid = pData->m_uid;
		hkOstream* v4 = &kErrStream.operator<<("No mass properties available (");
		hkOstream* v5 = &v4->operator<<(uiUid);
		v5->operator<<(")");
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, static_cast<int32_t>(0x79F9D887u), buf,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 334)) {
			__builtin_debugtrap();
		}
	}
	getLocalInertia(inertiaSpace, inertiaDiagonal);
	int32_t aiAxes[3];
	float afExtents[3];
	aiAxes[0] = 0;
	aiAxes[1] = 1;
	aiAxes[2] = 2;
	afExtents[0] = getWidth(&inertiaSpace.m_rotation.m_col0);
	afExtents[1] = getWidth(&inertiaSpace.m_rotation.m_col1);
	afExtents[2] = getWidth(&inertiaSpace.m_rotation.m_col2);
	for (int32_t i = 0; i < 2; ++i) {
		if (afExtents[i + 1] > afExtents[i]) {
			const int32_t iAxis = aiAxes[i];
			aiAxes[i] = aiAxes[i + 1];
			aiAxes[i + 1] = iAxis;
			const float fExtent = afExtents[i];
			afExtents[i] = afExtents[i + 1];
			afExtents[i + 1] = fExtent;
			i = -1;
		}
	}
	for (int32_t j = 0; j < 3; ++j) {
		planes[j] = inertiaSpace.m_rotation.getColumn(aiAxes[j]);
		const __m128 v = planes[j].m_quad;
		const __m128 sq = _mm_mul_ps(v, v);
		const float fLengthSq =
			_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
		if (fLengthSq != 0.0f) {
			const float fLength = std::sqrt(fLengthSq);
			const float fInvLength = 1.0f / fLength;
			planes[j].m_quad = _mm_mul_ps(_mm_set1_ps(fInvLength), v);
		}
		const __m128 prod = _mm_mul_ps(planes[j].m_quad, inertiaSpace.m_translation.m_quad);
		const __m128 dot =
			_mm_add_ss(_mm_shuffle_ps(prod, prod, 0xAA), _mm_add_ss(_mm_shuffle_ps(prod, prod, 0x55), prod));
		const float fW = _mm_cvtss_f32(_mm_sub_ss(_mm_setzero_ps(), dot));
		planes[j](3) = fW;
	}
}

// The SDK's generateIndexedFaces over byte counts and 16-bit indices: the implementation's <hkUint8, hkUint16>,
// with the source indices for SOURCE_VERTICES.
int32_t hkgpConvexHull::generateIndexedFaces(Inputs inputs, hkArray<uint8_t, hkContainerHeapAllocator>& verticesPerFace,
	hkArray<uint16_t, hkContainerHeapAllocator>& indices, bool maximizeArea) const {
	return m_data->generateIndexedFaces(verticesPerFace, indices, inputs == SOURCE_VERTICES, maximizeArea);
}

// The SDK's generateIndexedFaces over int arrays: the implementation's, with the source indices for
// SOURCE_VERTICES.
int32_t hkgpConvexHull::generateIndexedFaces(Inputs inputs, hkArray<int32_t, hkContainerHeapAllocator>& verticesPerFace,
	hkArray<int32_t, hkContainerHeapAllocator>& indices, bool maximizeArea) const {
	return m_data->generateIndexedFaces(verticesPerFace, indices, inputs == SOURCE_VERTICES, maximizeArea);
}

void hkgpConvexHull::getOrientedRectangle(const hkVector4& projectionPlane, hkVector4& halfExtents,
	hkTransform& worldTransform) const {
	m_data->getOrientedRectangle(projectionPlane, halfExtents, worldTransform);
}

void hkgpConvexHull::getOrientedBoundingBox(hkVector4& halfExtents, hkTransform& worldTransform) const {
	m_data->getOrientedBoundingBox(halfExtents, worldTransform);
}

// A unit normal for the hull's projection by its dimensions: for 1 a vector perpendicular to the projection axis
// (its two larger-magnitude lanes swapped and one negated, then normalized through __CIsqrt unless zero), for 2 the
// projection plane, for 3 the oriented box's axis of the smallest half extent; w is 0 in every case. The axis
// choices are x87 compares (unordered counts as not greater), the lane copies x87 float copies.
hkVector4* hkgpConvexHull::getBestProjectionPlane(hkVector4* apResult) const {
	apResult->m_quad = _mm_setzero_ps();
	const hkgpConvexHullImpl* pData = m_data;
	const __m128 zero = _mm_setzero_ps();
	switch (pData->m_dimensions) {
	case 1: {
		const float fAbsX = std::fabs(pData->m_projectionAxis(0));
		const float fAbsY = std::fabs(pData->m_projectionAxis(1));
		const float fAbsZ = std::fabs(pData->m_projectionAxis(2));
		int32_t iLow = 0;
		int32_t iMid = 1;
		int32_t iHigh = 2;
		float fMin = fAbsX;
		if (fAbsX > fAbsY) {
			fMin = fAbsY;
			iMid = 0;
			iLow = 1;
		}
		if (fMin > fAbsZ) {
			iHigh = iLow;
		}
		apResult->m_quad = zero;
		(*apResult)(iMid) = pData->m_projectionAxis(iHigh);
		(*apResult)(iHigh) = -pData->m_projectionAxis(iMid);
		const __m128 v = apResult->m_quad;
		const __m128 sq = _mm_mul_ps(v, v);
		const float fLengthSq =
			_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
		if (fLengthSq != 0.0f) {
			const float fLength = std::sqrt(fLengthSq);
			const float fInvLength = 1.0f / fLength;
			apResult->m_quad = _mm_mul_ps(_mm_set1_ps(fInvLength), v);
		}
		break;
	}
	case 2:
		apResult->m_quad = pData->m_projectionPlane.m_quad;
		break;
	case 3: {
		hkVector4 halfExtents;
		hkTransform transform;
		getOrientedBoundingBox(halfExtents, transform);
		int32_t iAxis;
		if (halfExtents(1) > halfExtents(0)) {
			iAxis = halfExtents(2) > halfExtents(0) ? 0 : 2;
		} else {
			iAxis = halfExtents(2) > halfExtents(1) ? 1 : 2;
		}
		hkVector4 dir;
		dir.m_quad = zero;
		dir(iAxis) = 1.0f;
		apResult->setRotatedDir(transform.m_rotation, dir);
		break;
	}
	}
	(*apResult)(3) = 0.0f;
	return apResult;
}

void hkgpConvexHull::ensureContainment(const hkArray<hkVector4, hkContainerHeapAllocator>& points) {
	m_data->ensureContainment(points);
}

int32_t hkgpConvexHull::buildPlanar_0(const hkVector4* points, int32_t numPoints, const hkVector4& plane,
	const hkgpConvexHull::BuildConfig& config) {
	m_data->m_config = config;
	m_data->m_projectionPlane.m_quad = plane.m_quad;
	return m_data->build(points, numPoints, 1, 0);
}

void*& hkgpConvexHull::userData() {
	return m_userData;
}

hkgpConvexHull::IUserObject*& hkgpConvexHull::userObject() {
	return m_userObject;
}

void hkgpConvexHull::generateVertexAdjacencyGraph(hkArray<int32_t, hkContainerHeapAllocator>& edges,
	hkArray<int32_t, hkContainerHeapAllocator>& vertexOffsets) {
	m_data->generateVertexAdjacencyGraph(edges, vertexOffsets);
}

void hkgpConvexHull::getLocalInertia(hkTransform& inertiaSpace, hkVector4& inertiaDiagonal) const {
	const hkgpConvexHullImpl* pImpl = m_data;
	if (!pImpl->m_hasMassProperties.m_bool) {
		alignas(16) char acBuffer[0x200];
		hkErrStream kStream(acBuffer, 0x200);
		// m_uid is read before the first operator<<.
		const uint32_t uiUid = pImpl->m_uid;
		hkOstream* pStream = &kStream.operator<<("No mass properties available (");
		pStream = &pStream->operator<<(uiUid);
		pStream->operator<<(")");
		// A handler that returns nonzero asks for the break.
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x79F9D887, acBuffer,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 334)) {
			__builtin_debugtrap();
		}
	}
	inertiaSpace = m_data->m_localTransform;
	inertiaDiagonal = m_data->m_inertia;
}

void hkgpConvexHull::fetchBevelPlanes(float maxCosAngle,
	hkArray<hkVector4, hkContainerHeapAllocator>& bevelPlanes) const {
	m_data->fetchBevelPlanes(maxCosAngle, bevelPlanes);
}

// The source position of the implementation's vertex, of which the public vertex is the base.
// KEEP: unregistered. The exe body touches only eax, and a caller relies on edx surviving the call (checkbody:
// "caller relies on preserved edx"), which clang does not guarantee.
hkVector4* hkgpConvexHull::D74ED0(hkgpConvexHull::Vertex* apVertex) {
	return &static_cast<hkgpConvexHullImpl::Vertex*>(apVertex)->m_source;
}

// KEEP: unregistered. The exe's callers keep edx live across the call (the exe body leaves it intact), which a
// clang body behind the seam does not guarantee. The public vertex is the base of the implementation's mesh vertex:
// the SDK downcasts to read the list link.
hkgpConvexHull::Vertex* hkgpConvexHull::D75120(hkgpConvexHull::Vertex* apVertex) {
	return static_cast<hkgpConvexHullImpl::Vertex*>(apVertex)->m_next;
}

// The planes of model are tried in turn, the one whose minimum distance to model is largest first. Each split keeps
// the part below the plane as the remaining operand and hands the part above to outside (or deletes it). A plane
// that leaves the whole operand above it ends the split with nothing inside.
void hkgpConvexHull::booleanSplit(const hkgpConvexHull* operand, const hkgpConvexHull* model, hkgpConvexHull** inside,
	hkArray<hkgpConvexHull*>* outside, float thickness) {
	hkgpConvexHull* pCurrent = operand->clone();
	hkgpConvexHullImpl* pModel = model->m_data;
	FNVD_INLINED_LOCAL(hkArray<int32_t>, planeIndices, std::nullopt, std::nullopt);
	planeIndices.m_data = nullptr;
	planeIndices.m_size = 0;
	planeIndices.m_capacityAndFlags = 0x80000000u;
	if (inside) {
		*inside = nullptr;
	}
	if (outside) {
		outside->m_size = 0;
	}
	int32_t iNumPlanes = model->getNumPlanes();
	// EXE-INLINED none: hkArray<int>::setSize
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	int32_t iCapacity = static_cast<int32_t>(planeIndices.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK);
	if (iCapacity < iNumPlanes) {
		int32_t iReserve = 2 * iCapacity;
		if (iNumPlanes >= iReserve) {
			iReserve = iNumPlanes;
		}
		hkArrayUtil::_reserve(pHeap, &planeIndices, iReserve, sizeof(int32_t));
	}
	planeIndices.m_size = static_cast<uint32_t>(iNumPlanes);
	int32_t iCount = model->getNumPlanes();
	for (int32_t i = 0; i < iCount; ++i) {
		planeIndices.m_data[i] = i;
	}
	while (pCurrent && planeIndices.m_size > 0) {
		int32_t iBest = 0;
		float fBest = model->getMinimumDistanceFromPlane(pModel->m_planes.m_data[planeIndices.m_data[0]]);
		for (int32_t i = 1; i < planeIndices.m_size; ++i) {
			float fDistance = model->getMinimumDistanceFromPlane(pModel->m_planes.m_data[planeIndices.m_data[i]]);
			if (fBest < fDistance) {
				fBest = fDistance;
				iBest = i;
			}
		}
		const hkVector4* pPlane = &pModel->m_planes.m_data[planeIndices.m_data[iBest]];
		// EXE-INLINED none: hkArray<int>::removeAt
		uint32_t uiLast = --planeIndices.m_size;
		if (uiLast != static_cast<uint32_t>(iBest)) {
			planeIndices.m_data[iBest] = planeIndices.m_data[uiLast];
		}
		hkgpConvexHull* pAbove;
		hkgpConvexHull* pBelow;
		Side eResult = pCurrent->splitByPlane(*pPlane, &pAbove, &pBelow, thickness);
		if (eResult == POSITIVE) {
			if (outside) {
				// EXE-INLINED none: hkArray<hkgpConvexHull*>::pushBack
				hkMemoryRouter& router = hkMemoryRouter::getInstance();
				if (outside->m_size == (outside->m_capacityAndFlags & hkArrayBase<hkgpConvexHull*>::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(*router.m_heap, outside, sizeof(hkgpConvexHull*));
				}
				outside->m_data[outside->m_size] = pCurrent;
				outside->m_size++;
			} else if (pCurrent) {
				delete pCurrent;
			}
			pCurrent = nullptr;
			break;
		}
		if (eResult == CROSSING) {
			delete pCurrent;
			pCurrent = pBelow;
			if (outside) {
				if (pAbove) {
					// EXE-INLINED none: hkArray<hkgpConvexHull*>::pushBack
					hkMemoryRouter& router = hkMemoryRouter::getInstance();
					if (outside->m_size ==
						(outside->m_capacityAndFlags & hkArrayBase<hkgpConvexHull*>::CAPACITY_MASK)) {
						hkArrayUtil::_reserveMore(*router.m_heap, outside, sizeof(hkgpConvexHull*));
					}
					outside->m_data[outside->m_size] = pAbove;
					outside->m_size++;
				}
			} else if (pAbove) {
				delete pAbove;
			}
		}
	}
	if (inside) {
		*inside = pCurrent;
	} else if (pCurrent) {
		delete pCurrent;
	}
	// EXE-INLINED none: hkArray<int>::~hkArray
	pHeap = hkMemoryRouter::getInstance().m_heap;
	planeIndices.m_size = 0;
	if (planeIndices.m_capacityAndFlags >= 0) {
		pHeap->bufFree(planeIndices.m_data,
			sizeof(int32_t) *
				static_cast<int32_t>(planeIndices.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK));
	}
}

// Only the inside part is asked for.
bool hkgpConvexHull::booleanIntersection(const hkgpConvexHull* operandA, const hkgpConvexHull* operandB,
	hkgpConvexHull*& output, float thickness) {
	booleanSplit(operandA, operandB, &output, nullptr, thickness);
	return output != nullptr;
}

// The results' size is read before the split. When the split returns an inside part, that part is deleted and true
// returned; otherwise every result from the old size on is deleted and the size set to 0 (not to the old size),
// returning false.
bool hkgpConvexHull::booleanSubtraction(const hkgpConvexHull* masterSet, const hkgpConvexHull* subSet,
	hkArray<hkgpConvexHull*>& results, float eps) {
	hkArray<hkgpConvexHull*>* pResults = &results;
	int32_t i = pResults->m_size;
	hkgpConvexHull* pInside;
	booleanSplit(masterSet, subSet, &pInside, pResults, eps);
	if (pInside) {
		delete pInside;
		return true;
	}
	for (; i < pResults->m_size; ++i) {
		hkgpConvexHull* pPart = pResults->m_data[i];
		if (pPart) {
			delete pPart;
		}
	}
	pResults->m_size = 0;
	return false;
}

// operandA is cloned, and each hull of operandsB is then subtracted from every piece still standing: a piece the
// subtraction splits is deleted, its parts appended to the next round and given indices (0.9999f, no rebuild), while
// a piece it leaves alone is carried over. The round's array swaps into place, and the surviving pieces are appended
// to output. True when a piece survives.
bool hkgpConvexHull::booleanSubtraction(const hkgpConvexHull* operandA, const hkArray<hkgpConvexHull*>& operandsB,
	hkArray<hkgpConvexHull*>& output, float thickness) {
	FNVD_INLINED_LOCAL(hkArray<hkgpConvexHull*>, remaining, std::nullopt, std::nullopt);
	remaining.m_data = nullptr;
	remaining.m_size = 0;
	remaining.m_capacityAndFlags = 0x80000000u;
	FNVD_INLINED_LOCAL(hkArray<hkgpConvexHull*>, pieces, std::nullopt, std::nullopt);
	pieces.m_data = nullptr;
	pieces.m_size = 0;
	pieces.m_capacityAndFlags = 0x80000000u;
	FNVD_INLINED_LOCAL(hkArray<hkgpConvexHull*>, parts, std::nullopt, std::nullopt);
	parts.m_data = nullptr;
	parts.m_size = 0;
	parts.m_capacityAndFlags = 0x80000000u;
	hkgpConvexHull* pClone = operandA->clone();
	// EXE-INLINED none: hkArray<hkgpConvexHull*>::pushBack
	hkMemoryRouter& router = hkMemoryRouter::getInstance();
	if (remaining.m_size == (remaining.m_capacityAndFlags & hkArrayBase<hkgpConvexHull*>::CAPACITY_MASK)) {
		hkArrayUtil::_reserveMore(*router.m_heap, &remaining, sizeof(hkgpConvexHull*));
	}
	remaining.m_data[remaining.m_size] = pClone;
	remaining.m_size++;
	int32_t iNumRemaining = remaining.m_size;
	for (int32_t i = 0; i < operandsB.m_size; ++i) {
		for (int32_t j = 0; j < iNumRemaining; ++j) {
			parts.m_size = 0;
			if (booleanSubtraction(remaining.m_data[j], operandsB.m_data[i], parts, thickness)) {
				delete remaining.m_data[j];
				for (int32_t k = 0; k < parts.m_size; ++k) {
					hkgpConvexHull* pPart = parts.m_data[k];
					// EXE-INLINED none: hkArray<hkgpConvexHull*>::pushBack
					hkMemoryRouter& pieceRouter = hkMemoryRouter::getInstance();
					if (pieces.m_size == (pieces.m_capacityAndFlags & hkArrayBase<hkgpConvexHull*>::CAPACITY_MASK)) {
						hkArrayUtil::_reserveMore(*pieceRouter.m_heap, &pieces, sizeof(hkgpConvexHull*));
					}
					pieces.m_data[pieces.m_size] = pPart;
					pieces.m_size++;
					pPart->buildIndices(0.9999f, false);
				}
			} else {
				hkgpConvexHull* pPiece = remaining.m_data[j];
				// EXE-INLINED none: hkArray<hkgpConvexHull*>::pushBack
				hkMemoryRouter& pieceRouter = hkMemoryRouter::getInstance();
				if (pieces.m_size == (pieces.m_capacityAndFlags & hkArrayBase<hkgpConvexHull*>::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(*pieceRouter.m_heap, &pieces, sizeof(hkgpConvexHull*));
				}
				pieces.m_data[pieces.m_size] = pPiece;
				pieces.m_size++;
			}
		}
		remaining.swap(pieces);
		iNumRemaining = remaining.m_size;
		pieces.m_size = 0;
	}
	for (int32_t i = 0; i < remaining.m_size; ++i) {
		hkgpConvexHull* pPiece = remaining.m_data[i];
		// EXE-INLINED none: hkArray<hkgpConvexHull*>::pushBack
		hkMemoryRouter& outputRouter = hkMemoryRouter::getInstance();
		if (output.m_size == (output.m_capacityAndFlags & hkArrayBase<hkgpConvexHull*>::CAPACITY_MASK)) {
			hkArrayUtil::_reserveMore(*outputRouter.m_heap, &output, sizeof(hkgpConvexHull*));
		}
		output.m_data[output.m_size] = pPiece;
		output.m_size++;
	}
	// EXE-INLINED none: hkArray<hkgpConvexHull*>::~hkArray
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	parts.m_size = 0;
	if (parts.m_capacityAndFlags >= 0) {
		pHeap->bufFree(parts.m_data,
			sizeof(hkgpConvexHull*) *
				static_cast<int32_t>(parts.m_capacityAndFlags & hkArrayBase<hkgpConvexHull*>::CAPACITY_MASK));
	}
	parts.m_data = nullptr;
	parts.m_capacityAndFlags = 0x80000000u;
	pHeap = hkMemoryRouter::getInstance().m_heap;
	pieces.m_size = 0;
	if (pieces.m_capacityAndFlags >= 0) {
		pHeap->bufFree(pieces.m_data,
			sizeof(hkgpConvexHull*) *
				static_cast<int32_t>(pieces.m_capacityAndFlags & hkArrayBase<hkgpConvexHull*>::CAPACITY_MASK));
	}
	pieces.m_data = nullptr;
	pieces.m_capacityAndFlags = 0x80000000u;
	pHeap = hkMemoryRouter::getInstance().m_heap;
	remaining.m_size = 0;
	if (remaining.m_capacityAndFlags >= 0) {
		pHeap->bufFree(remaining.m_data,
			sizeof(hkgpConvexHull*) *
				static_cast<int32_t>(remaining.m_capacityAndFlags & hkArrayBase<hkgpConvexHull*>::CAPACITY_MASK));
	}
	remaining.m_data = nullptr;
	remaining.m_capacityAndFlags = 0x80000000u;
	return iNumRemaining > 0;
}

// The first hull's configuration is copied and allows lower dimensions. The first user object found is cloned and every
// other hull's distinct object appended to the clone. The positions array is reserved for the total vertex count, the
// new hull takes the clone and builds from the array's data and size; a hull with dimensions -1 is deleted and null
// returned. The array's storage is freed on both paths.
hkgpConvexHull* hkgpConvexHull::createCompound(Inputs source, const hkgpConvexHull* const* hulls, int32_t numHulls) {
	if (numHulls <= 0) {
		return nullptr;
	}
	FNVD_INLINED_LOCAL(hkArray<hkVector4>, points, std::nullopt, std::nullopt);
	points.m_data = nullptr;
	points.m_size = 0;
	points.m_capacityAndFlags = 0x80000000u;
	BuildConfig config = hulls[0]->getConfiguration();
	int32_t iNumPoints = 0;
	IUserObject* pFirstObject = nullptr;
	IUserObject* pObject = nullptr;
	config.m_allowLowerDimensions.m_bool = 1;
	for (int32_t i = 0; i < numHulls; ++i) {
		iNumPoints += hulls[i]->getNumVertices();
		if (!pFirstObject) {
			IUserObject* pUserObject = hulls[i]->m_userObject;
			if (pUserObject) {
				pFirstObject = pUserObject;
			}
		}
	}
	if (pFirstObject) {
		pObject = pFirstObject->clone();
	}
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	int32_t iCapacity = static_cast<int32_t>(points.m_capacityAndFlags & 0x3FFFFFFF);
	if (iCapacity < iNumPoints) {
		iCapacity += iCapacity;
		if (iNumPoints >= iCapacity) {
			iCapacity = iNumPoints;
		}
		hkArrayUtil::_reserve(pHeap, &points, iCapacity, sizeof(hkVector4));
	}
	for (int32_t i = 0; i < numHulls; ++i) {
		if (pObject) {
			IUserObject* pUserObject = hulls[i]->m_userObject;
			if (pUserObject && pUserObject != pFirstObject) {
				pObject->append(pUserObject);
			}
		}
		hulls[i]->fetchPositions(source, points);
	}
	hkgpConvexHull* pMemory =
		static_cast<hkgpConvexHull*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkgpConvexHull)));
	pMemory->m_memSizeAndFlags = sizeof(hkgpConvexHull);
	hkgpConvexHull* pHull = ::new (pMemory) hkgpConvexHull;
	pHull->m_userObject = pObject;
	pHull->build(points.m_data, points.m_size, config);
	if (pHull->getDimensions() == -1) {
		delete pHull;
		pHeap = hkMemoryRouter::getInstance().m_heap;
		points.m_size = 0;
		if (points.m_capacityAndFlags >= 0) {
			pHeap->bufFree(points.m_data,
				sizeof(hkVector4) * static_cast<int32_t>(points.m_capacityAndFlags & 0x3FFFFFFF));
		}
		return nullptr;
	}
	pHeap = hkMemoryRouter::getInstance().m_heap;
	points.m_size = 0;
	if (points.m_capacityAndFlags >= 0) {
		pHeap->bufFree(points.m_data, sizeof(hkVector4) * static_cast<int32_t>(points.m_capacityAndFlags & 0x3FFFFFFF));
	}
	return pHull;
}

// The two hulls go to the array form as a two-entry array on the stack.
hkgpConvexHull* hkgpConvexHull::createCompound(hkgpConvexHull::Inputs source, const hkgpConvexHull* a,
	const hkgpConvexHull* b) {
	const hkgpConvexHull* hulls[2];
	hulls[0] = a;
	hulls[1] = b;
	return hkgpConvexHull::createCompound(source, hulls, 2);
}

// a's planes are tried in turn: b is split by each, and a crossing split (3) whose parts are both 3D and both have mass
// properties, with a below part whose volume is not below the best so far, gives a candidate union, a compound of a and
// that part, which f is asked about. An accepted candidate deletes the previous winner, takes over the best volume
// (rounded to float) and hands back the compound and the above part; a rejected one deletes its compound. Both split
// parts are deleted at the end of each iteration. Returns the best volume, -1 when no split gave one. The volume
// compare is in double.
float hkgpConvexHull::findBestUnion(const hkgpConvexHull* a, const hkgpConvexHull* b, const IBooleanFunction& f,
	hkgpConvexHull*& aUnb, hkgpConvexHull*& pb) {
	float fBest = -1.0f;
	aUnb = nullptr;
	pb = nullptr;
	hkgpConvexHullImpl* pImpl = a->m_data;
	if (!pImpl->m_hasIndexing.m_bool) {
		alignas(16) char acBuffer[0x200];
		alignas(16) hkErrStream kStream(acBuffer, 0x200);
		kStream << "No index available (" << pImpl->m_uid << ")";
		hkError::s_instance->message(hkError::MESSAGE_ERROR, 2046417030, acBuffer,
			".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 328);
	}
	// The plane count is read again after each plane. pPos and pNeg are left unset before the call because splitByPlane
	// nulls both parts first.
	for (int32_t i = 0; i < a->getNumPlanes(); ++i) {
		hkVector4 plane = a->getPlane(i);
		hkgpConvexHull* pPos;
		hkgpConvexHull* pNeg;
		if (b->splitByPlane(plane, &pPos, &pNeg, 1e-07f) == CROSSING) {
			if (pPos && pNeg && pPos->getDimensions() == 3 && pNeg->getDimensions() == 3 &&
				pNeg->buildMassProperties() == 0 && fBest < pNeg->getVolume() && pPos->buildMassProperties() == 0) {
				hkgpConvexHull* pCompound = createCompound(SOURCE_VERTICES, a, pNeg);
				if (pCompound) {
					hkBool bAccepted;
					const hkgpConvexHull* hulls[2];
					hulls[0] = pCompound;
					hulls[1] = pPos;
					if (pCompound->getDimensions() == 3 && pCompound->buildMassProperties() == 0 &&
						f(&bAccepted, hulls, 2)->m_bool) {
						delete aUnb;
						delete pb;
						fBest = pNeg->getVolume();
						aUnb = pCompound;
						pb = pPos;
						pPos = nullptr;
					} else {
						delete pCompound;
					}
				}
			}
		}
		if (pPos) {
			delete pPos;
		}
		if (pNeg) {
			delete pNeg;
		}
	}
	return fBest;
}

// Bisects the vertex count between 4 and the hull's count: each candidate is a fresh clone decimated to the midpoint.
// The base volume and each relative change are rounded to float. A change the threshold is above (NaN takes the other
// branch) makes the candidate the best and the midpoint the upper bound; otherwise the midpoint becomes the lower
// bound. The loser is deleted. A failed decimation deletes the candidate and returns the best so far.
hkgpConvexHull* hkgpConvexHull::volumeSimplify(const hkgpConvexHull* hull, float threshold, bool ensureContainment) {
	if (hull->getDimensions() != 3) {
		return nullptr;
	}
	hkgpConvexHull* pBest = hull->clone();
	float fVolume = pBest->getVolume();
	int32_t iLow = 4;
	int32_t iHigh = hull->getNumVertices();
	if (((iHigh - iLow) & ~1) == 0) {
		return pBest;
	}
	do {
		int32_t iMid = (iHigh + iLow) >> 1;
		hkgpConvexHull* pCandidate = hull->clone();
		pCandidate->buildMassProperties();
		if (!pCandidate->decimateVertices(iMid, ensureContainment)) {
			if (pCandidate) {
				delete pCandidate;
			}
			return pBest;
		}
		float fChange = static_cast<float>((static_cast<double>(pCandidate->getVolume()) - fVolume) / fVolume);
		// EXE-INLINED none: fabs
		fChange = __builtin_fabsf(fChange);
		if (threshold > fChange) {
			hkgpConvexHull* pOld = pBest;
			pBest = pCandidate;
			iHigh = iMid;
			pCandidate = pOld;
		} else {
			iLow = iMid;
		}
		if (pCandidate) {
			delete pCandidate;
		}
	} while (((iHigh - iLow) & ~1) != 0);
	return pBest;
}

void hkgpConvexHull::createVoronoiFeatures(const hkgpConvexHull* domain,
	const hkArray<hkVector4, hkContainerHeapAllocator>& points,
	hkArray<hkgpConvexHull*, hkContainerHeapAllocator>& cells,
	hkArray<hkgpConvexHull*, hkContainerHeapAllocator>& edges, float margin) {
	hkgpConvexHull::AbsoluteScaleConfig apMemory;
	hkgpConvexHull::createVoronoiCells(domain, points, cells, margin);
	hkgpConvexHull* v5 = domain->clone();
	hkgpConvexHull::AbsoluteScaleConfig* v6 = FNVD_RECONSTRUCT(hkgpConvexHull::AbsoluteScaleConfig)(&apMemory);
	v5->absoluteScale(-margin, *v6);
	hkgpConvexHull::booleanSubtraction(v5, cells, edges, 1e-07f);
	if (v5) {
		delete v5;
	}
}

// Returns the hull of a + bScale * b over every vertex pair of a and b, or null when the build fails. A vertex's w is
// its fourth float when the striding holds one, else 0. The sums go in a temp-allocated array.
hkgpConvexHull* hkgpConvexHull::computeMinkowski(const hkStridedVertices& a, const hkStridedVertices& b,
	const hkSimdReal& bScale) {
	const int32_t numVertices = b.m_numVertices * a.m_numVertices;
	using vertices_t = hkArray<hkVector4, hkContainerTempAllocator>;
	FNVD_INLINED_LOCAL(vertices_t, vertices, std::nullopt, std::nullopt);
	vertices.m_data = nullptr;
	vertices.m_size = 0;
	vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
	// EXE-INLINED none: hkArray<hkVector4, hkContainerTempAllocator>::reserve
	hkMemoryAllocator& reserveTemp = hkMemoryRouter::getInstance().temp();
	if (vertices.getCapacity() < numVertices) {
		hkArrayUtil::_reserve(reserveTemp, &vertices, numVertices, static_cast<int32_t>(sizeof(hkVector4)));
	}
	// A strided vertex array is raw floats, each vertex m_striding bytes after the last, so it is stepped through by
	// bytes.
	for (int32_t i = 0; i < a.m_numVertices; ++i) {
		const float* pA =
			reinterpret_cast<const float*>(reinterpret_cast<const char*>(a.m_vertices) + a.m_striding * i);
		const float fWa = a.m_striding < 16 ? 0.0f : pA[3];
		const __m128 va = _mm_unpacklo_ps(_mm_unpacklo_ps(_mm_load_ss(&pA[0]), _mm_load_ss(&pA[2])),
			_mm_unpacklo_ps(_mm_load_ss(&pA[1]), _mm_set_ss(fWa)));
		for (int32_t j = 0; j < b.m_numVertices; ++j) {
			const float* pB =
				reinterpret_cast<const float*>(reinterpret_cast<const char*>(b.m_vertices) + b.m_striding * j);
			const float fWb = b.m_striding < 16 ? 0.0f : pB[3];
			const __m128 vb = _mm_unpacklo_ps(_mm_unpacklo_ps(_mm_load_ss(&pB[0]), _mm_load_ss(&pB[2])),
				_mm_unpacklo_ps(_mm_load_ss(&pB[1]), _mm_set_ss(fWb)));
			// EXE-INLINED none: hkArray<hkVector4, hkContainerTempAllocator>::pushBack
			hkMemoryAllocator& pushTemp = hkMemoryRouter::getInstance().temp();
			if (vertices.m_size == vertices.getCapacity()) {
				hkArrayUtil::_reserveMore(pushTemp, &vertices, static_cast<int32_t>(sizeof(hkVector4)));
			}
			hkVector4& sum = vertices.m_data[vertices.m_size++];
			sum.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(bScale.m_real, bScale.m_real, 0), vb), va);
		}
	}
	alignas(16) hkgpConvexHull::BuildConfig config;
	config.m_allowLowerDimensions.m_bool = 1;
	hkgpConvexHull* pHull = static_cast<hkgpConvexHull*>(hkMemoryRouter::getInstance()
			.heap()
			.blockAlloc(static_cast<int32_t>(sizeof(hkgpConvexHull))));
	pHull->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkgpConvexHull));
	pHull = ::new (pHull) hkgpConvexHull;
	alignas(16) hkStridedVertices stridedVertices;
	// The sums go to build as a strided array of raw floats (16 bytes apart).
	stridedVertices.m_vertices = reinterpret_cast<const float*>(vertices.m_data);
	stridedVertices.m_numVertices = vertices.m_size;
	stridedVertices.m_striding = static_cast<int32_t>(sizeof(hkVector4));
	if (pHull->build(stridedVertices, config) == -1) {
		if (pHull) {
			delete pHull;
		}
		pHull = nullptr;
	}
	// EXE-INLINED none: hkArray<hkVector4, hkContainerTempAllocator>::~hkArray
	hkMemoryAllocator& freeTemp = hkMemoryRouter::getInstance().temp();
	vertices.m_size = 0;
	if (vertices.m_capacityAndFlags >= 0) {
		freeTemp.bufFree(vertices.m_data, static_cast<int32_t>(sizeof(hkVector4)) * vertices.getCapacity());
	}
	return pHull;
}

// Each sample is a random point of the domain's oriented bounding box, drawn again until it lies inside the hull;
// after 1024 misses for one sample the function warns and stops.
void hkgpConvexHull::createRandomSamples(const hkgpConvexHull* domain, hkPseudoRandomGenerator& generator,
	int32_t numSamples, hkArray<hkVector4, hkContainerHeapAllocator>& samples) {
	hkVector4 halfExtents = hkVector4Zero;
	hkTransform worldTransform = hkTransformIdentity;
	domain->getOrientedBoundingBox(halfExtents, worldTransform);
	for (int32_t i = 0; i < numSamples; ++i) {
		hkVector4 point;
		int32_t iTries = 0;
		while (true) {
			hkVector4 random;
			generator.getRandomVector11(random);
			point.m_quad = _mm_mul_ps(halfExtents.m_quad, random.m_quad);
			point.setTransformedPos(worldTransform, point);
			if (domain->isPointInside(point, domain->m_data->m_hasIndexing.m_bool, 0.0f)) {
				break;
			}
			if (++iTries >= 1024) {
				alignas(16) char buf[512];
				alignas(16) hkErrStream errStream(buf, 512);
				errStream << "Failed to generate samples";
				hkError::s_instance->message(hkError::MESSAGE_WARNING, 0x1559E, buf,
					".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 3618);
				return;
			}
		}
		// EXE-INLINED none: hkArray<hkVector4>::pushBack
		hkMemoryRouter& router = hkMemoryRouter::getInstance();
		if (samples.m_size == (samples.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
			hkArrayUtil::_reserveMore(*router.m_heap, &samples, sizeof(hkVector4));
		}
		samples.m_data[samples.m_size] = point;
		samples.m_size++;
	}
}

// The SDK's createVoronoiCells: the points closer than the radius (2 margin + 1.2e-6, at least 1.2e-6) to an
// earlier one are dropped; each remaining site's cell is the domain cut, nearest point first, by the bisector
// planes (moved by half the radius) of the points farther than the radius from it. A cut that leaves nothing drops
// the cell; the cuts stop once the nearest point left is farther than twice the cell's farthest vertex. Each cell
// gets the domain's indices and mass properties and goes to cells.
void hkgpConvexHull::createVoronoiCells(const hkgpConvexHull* domain,
	const hkArray<hkVector4, hkContainerHeapAllocator>& points,
	hkArray<hkgpConvexHull*, hkContainerHeapAllocator>& cells, float margin) {
	float fRadius = margin + margin + 1.1920928955078125e-06;
	if (fRadius < 1.1920928955078125e-06) {
		fRadius = 1.1920929e-06f;
	}
	const float fRadiusSquared = fRadius * fRadius;
	const float fHalfRadius = -fRadius * 0.5;
	using VectorArray = hkArray<hkVector4, hkContainerHeapAllocator>;
	FNVD_INLINED_LOCAL(VectorArray, sites, std::nullopt, std::nullopt);
	alignas(16) FNVD_INLINED_LOCAL(VectorArray, sorted, std::nullopt, std::nullopt);
	sites.m_data = nullptr;
	sites.m_size = 0;
	sites.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG);
	sorted.m_data = nullptr;
	sorted.m_size = 0;
	sorted.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG);
	// The plane offset's terms, kept as vectors.
	const __m128 kHalfRadius = _mm_set_ss(fHalfRadius);
	const __m128 kHalf = _mm_set_ss(0.5f);
	// EXE-INLINED none: hkArray<hkVector4, hkContainerHeapAllocator>::reserve (a signed compare, jge 0xD79B56)
	int32_t iReserve = points.m_size;
	hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
	int32_t iCapacity = sites.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
	if (iCapacity < iReserve) {
		hkArrayUtil::_reserve(*pRouter->m_heap, &sites, iReserve, sizeof(hkVector4));
	}
	// EXE-INLINED none: hkArray<hkVector4, hkContainerHeapAllocator>::reserve
	iReserve = points.m_size;
	pRouter = &hkMemoryRouter::getInstance();
	iCapacity = sorted.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
	if (iCapacity < iReserve) {
		hkArrayUtil::_reserve(*pRouter->m_heap, &sorted, iReserve, sizeof(hkVector4));
	}
	for (int32_t i = 0; i < points.m_size; ++i) {
		bool bKeep = true;
		for (int32_t j = 0; j < sites.m_size; ++j) {
			const __m128 d = _mm_sub_ps(sites.m_data[j].m_quad, points.m_data[i].m_quad);
			const __m128 sq = _mm_mul_ps(d, d);
			const float fDistance =
				_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
			// Dropped at or within the radius, kept when unordered.
			if (fDistance <= fRadiusSquared) {
				bKeep = false;
				break;
			}
		}
		if (bKeep) {
			// Both arrays were reserved for every point: no capacity check.
			sites.m_data[sites.m_size] = points.m_data[i];
			sites.m_size++;
			sorted.m_data[sorted.m_size] = points.m_data[i];
			sorted.m_size++;
		}
	}
	// The squared distance to the current cell's farthest vertex, -1 when it is to be computed again.
	float fFarthest = -1.0f;
	for (int32_t iSite = 0; iSite < sites.m_size; ++iSite) {
		hkgpConvexHull* pCell = domain->clone();
		hkVector4 site = sites.m_data[iSite];
		if (sorted.m_size > 1) {
			sortByDistance(sorted.m_data, 0, sorted.m_size - 1, &site);
		}
		pCell->m_data->m_config.m_buildIndices.m_bool = false;
		pCell->m_data->m_config.m_buildMassProperties.m_bool = false;
		for (int32_t j = 0; j < sorted.m_size; ++j) {
			const __m128 kSite = site.m_quad;
			const __m128 kPoint = sorted.m_data[j].m_quad;
			const __m128 kDelta = _mm_sub_ps(kPoint, kSite);
			const __m128 sq = _mm_mul_ps(kDelta, kDelta);
			const __m128 kLength2 =
				_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq));
			const float fDistance = _mm_cvtss_f32(kLength2);
			// Only a point strictly beyond the radius cuts.
			if (fRadiusSquared < fDistance) {
				// EXE-INLINED 0x62AEA0
				const __m128 approx = _mm_rsqrt_ss(kLength2);
				const __m128 refined = _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), approx),
					_mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kLength2, approx), approx)));
				const __m128 inverse = _mm_andnot_ps(_mm_cmpeq_ss(kLength2, _mm_setzero_ps()), refined);
				hkVector4 plane;
				plane.m_quad = _mm_mul_ps(_mm_shuffle_ps(inverse, inverse, 0), kDelta);
				const __m128 dSite = _mm_mul_ps(plane.m_quad, kSite);
				const __m128 dPoint = _mm_mul_ps(kPoint, plane.m_quad);
				const __m128 kDotSite = _mm_add_ss(_mm_shuffle_ps(dSite, dSite, 0xAA),
					_mm_add_ss(_mm_shuffle_ps(dSite, dSite, 0x55), dSite));
				const __m128 kDotPoint = _mm_add_ss(_mm_shuffle_ps(dPoint, dPoint, 0xAA),
					_mm_add_ss(_mm_shuffle_ps(dPoint, dPoint, 0x55), dPoint));
				// w = (0 - 0.5) ((dotPoint + dotSite) + -radius / 2).
				plane(3) = _mm_cvtss_f32(_mm_mul_ss(_mm_sub_ss(_mm_setzero_ps(), kHalf),
					_mm_add_ss(_mm_add_ss(kDotPoint, kDotSite), kHalfRadius)));
				hkgpConvexHull* pBelow = nullptr;
				switch (pCell->splitByPlane(plane, nullptr, &pBelow, 1.1920929e-06f)) {
				case POSITIVE:
					delete pCell;
					pCell = nullptr;
					break;
				case NEGATIVE: {
					if (fFarthest < 0.0f) {
						hkVector4 farthest;
						fFarthest = pCell->m_data->getFarthestVertexDistanceSquared(&farthest, &site)->m_quad[0];
					}
					// The remaining points are all too far.
					if (fFarthest * 4.0 < fDistance) {
						j = sorted.m_size;
					}
					break;
				}
				case CROSSING:
					delete pCell;
					fFarthest = -1.0f;
					pCell = pBelow;
					break;
				default:
					break;
				}
			}
			if (!pCell) {
				break;
			}
		}
		if (pCell) {
			if (domain->m_data->m_hasIndexing.m_bool) {
				pCell->buildIndices(0.9999f, false);
			}
			if (domain->m_data->m_hasMassProperties.m_bool) {
				pCell->buildMassProperties();
			}
			// EXE-INLINED none: hkArray<hkgpConvexHull*, hkContainerHeapAllocator>::pushBack
			hkMemoryRouter& router = hkMemoryRouter::getInstance();
			if (cells.m_size == (cells.m_capacityAndFlags & hkArrayBase<hkgpConvexHull*>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*router.m_heap, &cells, sizeof(hkgpConvexHull*));
			}
			cells.m_data[cells.m_size] = pCell;
			cells.m_size++;
		}
	}
	// EXE-INLINED none: hkArray<hkVector4, hkContainerHeapAllocator>::~hkArray
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	sorted.m_size = 0;
	if (sorted.m_capacityAndFlags >= 0) {
		pHeap->bufFree(sorted.m_data,
			static_cast<int32_t>((sorted.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) *
				sizeof(hkVector4)));
	}
	sorted.m_data = nullptr;
	sorted.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG);
	// EXE-INLINED none: hkArray<hkVector4, hkContainerHeapAllocator>::~hkArray
	pHeap = hkMemoryRouter::getInstance().m_heap;
	sites.m_size = 0;
	if (sites.m_capacityAndFlags >= 0) {
		pHeap->bufFree(sites.m_data,
			static_cast<int32_t>((sites.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) *
				sizeof(hkVector4)));
	}
}

void hkgpConvexHull::createVoronoiEdges(const hkgpConvexHull* domain,
	const hkArray<hkVector4, hkContainerHeapAllocator>& points,
	hkArray<hkgpConvexHull*, hkContainerHeapAllocator>& edges, float margin) {
	using pieces_t = hkArray<hkgpConvexHull*, hkContainerHeapAllocator>;
	FNVD_INLINED_LOCAL(pieces_t, pieces, std::nullopt, std::nullopt);
	pieces.m_data = 0;
	pieces.m_size = 0;
	pieces.m_capacityAndFlags = hkArrayBase<hkgpConvexHull*>::DONT_DEALLOCATE_FLAG;
	hkgpConvexHull::createVoronoiFeatures(domain, points, pieces, edges, margin);
	for (int32_t i = 0; i < pieces.m_size; ++i) {
		hkgpConvexHull* v5 = pieces.m_data[i];
		if (v5) {
			delete v5;
		}
	}
	hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
	pieces.m_size = 0;
	if (pieces.m_capacityAndFlags >= 0) {
		m_heap->bufFree(pieces.begin(), sizeof(hkgpConvexHull*) * pieces.m_capacityAndFlags);
	}
}

__m128 hkgpConvexHullImpl::PositionAsSource::get(const hkgpConvexHullImpl& hull, const Vertex& v) {
	return v.m_source.m_quad;
}

// The integer coordinates scaled back by m_scaleInv and moved by m_origin, w 0.
__m128 hkgpConvexHullImpl::PositionAsTransform::get(const hkgpConvexHullImpl& hull, const Vertex& v) {
	const __m128 x =
		_mm_setr_ps(static_cast<float>(v.m_x[0]), static_cast<float>(v.m_x[1]), static_cast<float>(v.m_x[2]), 0.0f);
	return _mm_add_ps(_mm_mul_ps(x, hull.m_scaleInv.m_quad), hull.m_origin.m_quad);
}

// The configuration's defaults, empty lists and arrays, the next unique id, then an empty hull (reset).
hkgpConvexHullImpl::hkgpConvexHullImpl() : m_config(), m_mesh(), m_pendings(), m_planes(), m_supportCache() {
	m_uid = InterlockedExchangeAdd(&g_uidGenerator, 1);
	reset(0);
}

// Empties the mesh (reset); the members free the support cache, the planes, the pending vertices and the mesh.
hkgpConvexHullImpl::~hkgpConvexHullImpl() {
	reset(0);
}

// The point is quantized to the hull's integer grid, all four lanes at once: (x - m_origin) * m_scale. Each of the
// first three components is then clamped to [-8192, 8191] and rounded to the nearest integer away from zero; a
// component above 8191 or below -8192 fails (a NaN compares false both ways and is carried through the clamp and the
// conversion). Each used triangle of the mesh then decides: its quantized normal dot the quantized point plus its
// integer offset above zero puts the point outside. With usePlanes the hull's plane equations are used instead,
// against planeThickness, and a hull without indices reports before that.
bool hkgpConvexHullImpl::isPointInside(const hkVector4& point, bool usePlanes, float epsilon) const {
	if (!usePlanes) {
		hkVector4 quantized;
		quantized.m_quad[0] = (point.m_quad[0] - m_origin.m_quad[0]) * m_scale.m_quad[0];
		quantized.m_quad[1] = (point.m_quad[1] - m_origin.m_quad[1]) * m_scale.m_quad[1];
		quantized.m_quad[2] = (point.m_quad[2] - m_origin.m_quad[2]) * m_scale.m_quad[2];
		quantized.m_quad[3] = (point.m_quad[3] - m_origin.m_quad[3]) * m_scale.m_quad[3];
		const float* afComponent = &reinterpret_cast<float*>(&quantized.m_quad)[0];
		int32_t aiQuantized[3];
		bool bOutside = false;
		for (int32_t i = 0; i < 3; ++i) {
			const float fComponent = afComponent[i];
			if (fComponent > 8191.0f || -8192.0f > fComponent) {
				bOutside = true;
			}
			float fClamped = fComponent;
			if (fComponent > 8191.0f) {
				fClamped = 8191.0f;
			}
			if (-8192.0f > fClamped) {
				fClamped = -8192.0f;
			}
			// The compare against zero takes the positive half for zero and for NaN.
			const float fHalf = fClamped < 0.0f ? -0.5f : 0.5f;
			aiQuantized[i] = static_cast<int32_t>(fClamped + fHalf);
		}
		if (bOutside) {
			return false;
		}
		const Triangle* pTriangle = m_mesh.m_triangles.m_used;
		while (pTriangle) {
			const Plane& plane = pTriangle->m_plane;
			const int64_t iDistance = static_cast<int64_t>(plane.m_c[0]) * aiQuantized[0] +
				static_cast<int64_t>(plane.m_c[1]) * aiQuantized[1] +
				static_cast<int64_t>(plane.m_c[2]) * aiQuantized[2] + plane.m_d;
			if (iDistance > 0) {
				return false;
			}
			pTriangle = pTriangle->m_next;
		}
		return true;
	}
	if (!m_hasIndexing.m_bool) {
		alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
		char acBuffer[0x200];
		hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
		*pStream << "No index available (" << m_uid << ")";
		hkError::s_instance->message(hkError::MESSAGE_ERROR, 2046417030, acBuffer,
			".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 328);
		FNVD_DESTRUCT(hkOstream)(pStream);
	}
	const int32_t iNumPlanes = m_planes.m_size;
	if (iNumPlanes <= 0) {
		return true;
	}
	const hkVector4* pPlane = m_planes.m_data;
	for (int32_t i = 0; i < iNumPlanes; ++i) {
		const float fDistance = pPlane->m_quad[2] * point.m_quad[2] +
			(pPlane->m_quad[1] * point.m_quad[1] + pPlane->m_quad[0] * point.m_quad[0]) + pPlane->m_quad[3];
		if (fDistance > epsilon) {
			return false;
		}
		++pPlane;
	}
	return true;
}

// The SDK's build. Unaligned points are copied into an aligned array and built from there. Otherwise the hull is
// reset, the quantization (m_origin, m_scale, m_scaleInv) taken from minp, set to the identity for internal inputs,
// or fitted to the points' bounding box (each axis at least 2e-5 wide, onto the integer grid -8191..8191). The
// dimension is guessed from the count (or 2 with maxp's byte set, the forced planar case; else the projection axis
// and plane are taken from the first points), each point becomes a pending vertex with its quantized coordinates
// (sorted with m_sortInputs), and the hull is built by dimension: one vertex, the two extreme vertices along the
// projection axis, a planar hull by the monotone chain on the two coordinates of the plane, or flushPending's
// volume; a lower dimension found on the way loops back. Returns m_dimensions (-1: failed).
// KEEP: each point's vertex is built in a local whose m_refs nothing writes (the SDK's Vertex constructor sets only
// m_scratch and m_index), and the exe copies the whole local into m_pendings (0xD81D72 reads [esp+0xE0], which no
// instruction of the function writes).
int32_t hkgpConvexHullImpl::build(const hkVector4* points, const int32_t numPoints, int32_t forcePlanar,
	const hkgpConvexHullImpl* domain) {
	// The points' address is tested as an integer for 16-byte alignment.
	if (numPoints && (reinterpret_cast<uintptr_t>(points) & 15) != 0) {
		using aligned_t = hkArray<hkVector4, hkContainerHeapAllocator>;
		FNVD_INLINED_LOCAL(aligned_t, aligned, std::nullopt, std::nullopt);
		aligned.m_data = nullptr;
		aligned.m_size = 0;
		aligned.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
		aligned.setSize(numPoints);
		for (int32_t i = 0; i < numPoints; ++i) {
			aligned.m_data[i].set(points[i].m_quad[0], points[i].m_quad[1], points[i].m_quad[2], points[i].m_quad[3]);
		}
		const int32_t result = build(aligned.m_data, numPoints, forcePlanar, domain);
		// EXE-INLINED none: hkArray<hkVector4>::~hkArray
		hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
		aligned.m_size = 0;
		if (aligned.m_capacityAndFlags >= 0) {
			heap->bufFree(aligned.m_data, static_cast<int32_t>(sizeof(hkVector4)) * aligned.getCapacity());
		}
		return result;
	}
	reset(forcePlanar);
	if (numPoints > 0) {
		if (domain) {
			m_origin = domain->m_origin;
			m_scale = domain->m_scale;
			m_scaleInv = domain->m_scaleInv;
		} else if (m_config.m_internalInputs.m_bool) {
			m_origin.set(0.0f, 0.0f, 0.0f, 0.0f);
			m_scale.set(1.0f, 1.0f, 1.0f, 0.0f);
			m_scaleInv.set(1.0f, 1.0f, 1.0f, 0.0f);
		} else {
			// The points' bounding box.
			hkVector4 boxMin = points[0];
			hkVector4 boxMax = points[0];
			for (int32_t i = 1; i < numPoints; ++i) {
				boxMin.setMin4(boxMin, points[i]);
				boxMax.setMax4(boxMax, points[i]);
			}
			for (int32_t k = 0; k < 3; ++k) {
				if (boxMin.m_quad[k] < -3.402820018375656e+38 || boxMax.m_quad[k] > 3.402820018375656e+38) {
					char buf[512];
					hkErrStream errStream(buf, 512);
					errStream.operator<<("Input domain out of range");
					if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x405A2174, buf,
							".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 552)) {
						__builtin_debugtrap();
					}
				}
				// An axis narrower than twice the epsilon is widened about its center.
				if (boxMax.m_quad[k] - boxMin.m_quad[k] < 9.999999747378752e-06) {
					const float center = (boxMax.m_quad[k] + boxMin.m_quad[k]) * 0.5;
					const float low = center - 9.999999747378752e-06;
					const float oldMin = boxMin.m_quad[k];
					boxMin.m_quad[k] = (low > oldMin) ? oldMin : low;
					const float oldMax = boxMax.m_quad[k];
					const float high = center + 9.999999747378752e-06;
					boxMax.m_quad[k] = (high < oldMax) ? oldMax : high;
				}
			}
			hkVector4 half;
			half.setAll(0.5f);
			hkVector4 halfExtents;
			halfExtents.m_quad = _mm_mul_ps(half.m_quad, _mm_sub_ps(boxMax.m_quad, boxMin.m_quad));
			m_origin.m_quad = _mm_add_ps(halfExtents.m_quad, boxMin.m_quad);
			m_scale.setReciprocal3(halfExtents);
			m_origin.m_quad[3] = 0.0f;
			m_scale.m_quad[3] = 0.0f;
			hkVector4 gridScale;
			gridScale.setAll(8191.0f);
			m_scale.m_quad = _mm_mul_ps(gridScale.m_quad, m_scale.m_quad);
			m_scaleInv.setReciprocal3(m_scale);
		}
		m_dimensions = (numPoints - 1 < 3) ? numPoints - 1 : 3;
		m_projectionAxis.m_quad = hkVector4Zero.m_quad;
		// Tests the low byte only.
		if (static_cast<uint8_t>(forcePlanar)) {
			m_dimensions = 2;
		} else {
			m_projectionPlane.m_quad = hkVector4Zero.m_quad;
			if (numPoints > 1) {
				m_projectionAxis.setSub4(points[1], points[0]);
			}
			if (numPoints > 2) {
				// The longest normal of the points' triangles, the search ending at the first one longer than 1e-2.
				for (int32_t i = 0; i < numPoints; ++i) {
					const hkVector4 a = points[i];
					for (int32_t j = i + 1; j < numPoints; ++j) {
						hkVector4 e1;
						e1.setSub4(points[j], a);
						for (int32_t k = j + 1; k < numPoints; ++k) {
							hkVector4 e2;
							e2.setSub4(points[k], a);
							hkVector4 normal;
							normal.setCross(e1, e2);
							const __m128 n2 = _mm_mul_ps(normal.m_quad, normal.m_quad);
							const float fNormalLength2 = _mm_shuffle_ps(n2, n2, 0xAA)[0] +
								static_cast<float>(_mm_shuffle_ps(n2, n2, 0x55)[0] + n2[0]);
							const __m128 p2 = _mm_mul_ps(m_projectionPlane.m_quad, m_projectionPlane.m_quad);
							const float fPlaneLength2 = _mm_shuffle_ps(p2, p2, 0xAA)[0] +
								static_cast<float>(_mm_shuffle_ps(p2, p2, 0x55)[0] + p2[0]);
							if (fPlaneLength2 < fNormalLength2) {
								m_projectionPlane = normal;
								if (fNormalLength2 > 9.999999747378752e-05) {
									k = numPoints;
									j = numPoints;
									i = numPoints;
								}
							}
						}
					}
				}
				// normalize3: (1, 0, 0) for a zero normal.
				const hkVector4 plane = m_projectionPlane;
				const __m128 q2 = _mm_mul_ps(plane.m_quad, plane.m_quad);
				const float fLength2 =
					_mm_shuffle_ps(q2, q2, 0xAA)[0] + static_cast<float>(_mm_shuffle_ps(q2, q2, 0x55)[0] + q2[0]);
				if (fLength2 == 0.0) {
					m_projectionPlane.set(1.0f, 0.0f, 0.0f, 0.0f);
				} else {
					const float fLength = static_cast<float>(std::sqrt(fLength2));
					const float fInvLength = 1.0 / fLength;
					m_projectionPlane.m_quad = _mm_mul_ps(_mm_set1_ps(fInvLength), plane.m_quad);
				}
				// The plane through the first point.
				const __m128 d = _mm_mul_ps(points[0].m_quad, m_projectionPlane.m_quad);
				m_projectionPlane.m_quad[3] = 0.0f -
					static_cast<float>(_mm_shuffle_ps(d, d, 0xAA)[0] +
						static_cast<float>(_mm_shuffle_ps(d, d, 0x55)[0] + d[0]));
			}
		}
		m_pendings.clear();
		m_pendings.setSize(numPoints);
		for (int32_t i = 0; i < numPoints; ++i) {
			// The SDK's Vertex() sets m_scratch and m_index only.
			Vertex v;
			v.m_scratch = 0.0f;
			v.m_source = points[i];
			hkVector4 x;
			x.m_quad = _mm_mul_ps(_mm_sub_ps(points[i].m_quad, m_origin.m_quad), m_scale.m_quad);
			v.m_index = -1;
			// Each coordinate clamped to -8192..8191 and rounded half away from zero.
			for (int32_t k = 0; k < 3; ++k) {
				float f = x.m_quad[k];
				if (f > 8191.0) {
					f = 8191.0f;
				}
				float fRound;
				if (f < -8192.0) {
					f = -8192.0f;
					fRound = -0.5f;
				} else if (f < 0.0) {
					fRound = -0.5f;
				} else {
					fRound = 0.5f;
				}
				v.m_x[k] = static_cast<int32_t>(f + fRound);
			}
			// Copies m_source through m_index, not the list links.
			Vertex& dst = m_pendings[i];
			dst.m_source = v.m_source;
			dst.m_x[0] = v.m_x[0];
			dst.m_x[1] = v.m_x[1];
			dst.m_x[2] = v.m_x[2];
			dst.m_scratch = v.m_scratch;
			dst.m_refs = v.m_refs;
			dst.m_index = v.m_index;
			if (m_config.m_setSourceIndices.m_bool) {
				m_pendings[i].m_source.setInt24W(i);
			}
		}
		if (!static_cast<uint8_t>(forcePlanar) && m_config.m_sortInputs.m_bool) {
			if (m_pendings.m_size > 1) {
				hkAlgorithm::quickSortRecursive(m_pendings.m_data, 0, m_pendings.m_size - 1, uint8_t(0));
			}
		}
		for (;;) {
			bool bAgain = false;
			switch (m_dimensions) {
			case 0: {
				Vertex item;
				item.m_scratch = 0.0f;
				item.m_index = -1;
				Vertex* pVertex = m_mesh.m_vertices.allocate(item);
				makeVertex(m_pendings[0], pVertex);
				break;
			}
			case 1: {
				// The extreme points along the projection axis.
				int32_t iMin = 0;
				int32_t iMax = 0;
				const hkVector4 axis = m_projectionAxis;
				for (int32_t i = 1; i < numPoints; ++i) {
					const __m128 di = _mm_mul_ps(axis.m_quad, m_pendings[i].m_source.m_quad);
					const float fDot =
						_mm_shuffle_ps(di, di, 0xAA)[0] + static_cast<float>(_mm_shuffle_ps(di, di, 0x55)[0] + di[0]);
					const __m128 dMin = _mm_mul_ps(m_pendings[iMin].m_source.m_quad, axis.m_quad);
					const float fMinDot = _mm_shuffle_ps(dMin, dMin, 0xAA)[0] +
						static_cast<float>(_mm_shuffle_ps(dMin, dMin, 0x55)[0] + dMin[0]);
					if (fMinDot > fDot) {
						iMin = i;
					}
					const __m128 dMax = _mm_mul_ps(m_pendings[iMax].m_source.m_quad, axis.m_quad);
					const float fMaxDot = _mm_shuffle_ps(dMax, dMax, 0xAA)[0] +
						static_cast<float>(_mm_shuffle_ps(dMax, dMax, 0x55)[0] + dMax[0]);
					if (fMaxDot < fDot) {
						iMax = i;
					}
				}
				const Vertex& a = m_pendings[iMin];
				const Vertex& b = m_pendings[iMax];
				if (a.m_x[0] == b.m_x[0] && a.m_x[1] == b.m_x[1]) {
					m_dimensions = 0;
					continue;
				}
				Vertex itemA;
				itemA.m_scratch = 0.0f;
				itemA.m_index = -1;
				Vertex* pA = m_mesh.m_vertices.allocate(itemA);
				Vertex itemB;
				itemB.m_scratch = 0.0f;
				itemB.m_index = -1;
				Vertex* pB = m_mesh.m_vertices.allocate(itemB);
				makeVertex(a, pA);
				makeVertex(b, pB);
				break;
			}
			case 2: {
				// The planar hull: the plane's major axis coordinate is moved to m_x[2], the vertices sorted, and the
				// monotone chain run on m_x[0] and m_x[1].
				hkArray<Vertex, hkContainerHeapAllocator> hull;
				hull.m_data = nullptr;
				hull.m_size = 0;
				hull.m_capacityAndFlags = hkArrayBase<Vertex>::DONT_DEALLOCATE_FLAG;
				const int32_t majorAxis = m_projectionPlane.getMajorAxis();
				if (majorAxis == 0) {
					const int32_t numPendings = m_pendings.m_size;
					for (int32_t i = 0; i < numPendings; ++i) {
						Vertex& v = m_pendings[i];
						const int32_t iSwap = v.m_x[0];
						v.m_x[0] = v.m_x[2];
						v.m_x[2] = iSwap;
					}
				} else if (majorAxis == 1) {
					const int32_t numPendings = m_pendings.m_size;
					for (int32_t i = 0; i < numPendings; ++i) {
						Vertex& v = m_pendings[i];
						const int32_t iSwap = v.m_x[1];
						v.m_x[1] = v.m_x[2];
						v.m_x[2] = iSwap;
					}
				}
				// quickSort, with the hkAlgorithm::less comparator passed as byte 0 as above.
				if (m_pendings.m_size > 1) {
					hkAlgorithm::quickSortRecursive(m_pendings.m_data, 0, m_pendings.m_size - 1, uint8_t(0));
				}
				hull.setSize(2 * numPoints);
				int32_t numHull = 0;
				// The upper chain, then the lower one.
				for (int32_t i = 0; i < numPoints; ++i) {
					const Vertex& p = m_pendings[i];
					const int32_t px = p.m_x[0];
					const int32_t py = p.m_x[1];
					while (numHull >= 2) {
						const Vertex& a = hull[numHull - 2];
						const Vertex& b = hull[numHull - 1];
						if ((b.m_x[0] - a.m_x[0]) * (py - a.m_x[1]) - (b.m_x[1] - a.m_x[1]) * (px - a.m_x[0]) > 0) {
							break;
						}
						--numHull;
					}
					// Copies m_source through m_index, not the list links.
					Vertex& dst = hull[numHull];
					dst.m_source = p.m_source;
					dst.m_x[0] = p.m_x[0];
					dst.m_x[1] = p.m_x[1];
					dst.m_x[2] = p.m_x[2];
					dst.m_scratch = p.m_scratch;
					dst.m_refs = p.m_refs;
					dst.m_index = p.m_index;
					++numHull;
				}
				const int32_t lowerStart = numHull + 1;
				for (int32_t i = numPoints - 2; i >= 0; --i) {
					const Vertex& p = m_pendings[i];
					if (numHull >= lowerStart) {
						const int32_t px = p.m_x[0];
						const int32_t py = p.m_x[1];
						while (numHull >= lowerStart) {
							const Vertex& a = hull[numHull - 2];
							const Vertex& b = hull[numHull - 1];
							if ((b.m_x[0] - a.m_x[0]) * (py - a.m_x[1]) - (b.m_x[1] - a.m_x[1]) * (px - a.m_x[0]) > 0) {
								break;
							}
							--numHull;
						}
					}
					Vertex& dst = hull[numHull];
					dst.m_source = p.m_source;
					dst.m_x[0] = p.m_x[0];
					dst.m_x[1] = p.m_x[1];
					dst.m_x[2] = p.m_x[2];
					dst.m_scratch = p.m_scratch;
					dst.m_refs = p.m_refs;
					dst.m_index = p.m_index;
					++numHull;
				}
				if (numHull <= 3) {
					// Too few corners: the coordinates are moved back and the points taken as a line.
					const int32_t backAxis = m_projectionPlane.getMajorAxis();
					if (backAxis == 0) {
						const int32_t numPendings = m_pendings.m_size;
						for (int32_t i = 0; i < numPendings; ++i) {
							Vertex& v = m_pendings[i];
							const int32_t iSwap = v.m_x[0];
							v.m_x[0] = v.m_x[2];
							v.m_x[2] = iSwap;
						}
					} else if (backAxis == 1) {
						const int32_t numPendings = m_pendings.m_size;
						for (int32_t i = 0; i < numPendings; ++i) {
							Vertex& v = m_pendings[i];
							const int32_t iSwap = v.m_x[1];
							v.m_x[1] = v.m_x[2];
							v.m_x[2] = iSwap;
						}
					}
					m_dimensions = 1;
					bAgain = true;
				} else {
					// The chain ends on its first vertex: the last copy is dropped.
					m_pendings.swap(hull);
					--numHull;
					m_pendings.setSize(numHull);
					const int32_t backAxis = m_projectionPlane.getMajorAxis();
					if (backAxis == 0) {
						const int32_t numPendings = m_pendings.m_size;
						for (int32_t i = 0; i < numPendings; ++i) {
							Vertex& v = m_pendings[i];
							const int32_t iSwap = v.m_x[0];
							v.m_x[0] = v.m_x[2];
							v.m_x[2] = iSwap;
						}
					} else if (backAxis == 1) {
						const int32_t numPendings = m_pendings.m_size;
						for (int32_t i = 0; i < numPendings; ++i) {
							Vertex& v = m_pendings[i];
							const int32_t iSwap = v.m_x[1];
							v.m_x[1] = v.m_x[2];
							v.m_x[2] = iSwap;
						}
					}
					for (int32_t i = 0; i < m_pendings.m_size; ++i) {
						Vertex item;
						item.m_scratch = 0.0f;
						item.m_index = -1;
						Vertex* pVertex = m_mesh.m_vertices.allocate(item);
						makeVertex(m_pendings[i], pVertex);
					}
					m_pendings.clear();
				}
				hull.clearAndDeallocate();
				break;
			}
			case 3:
				if (!flushPending()) {
					if (m_config.m_allowLowerDimensions.m_bool) {
						m_dimensions = 2;
						bAgain = true;
						break;
					}
					m_dimensions = -1;
				}
				break;
			default:
				break;
			}
			if (!bAgain) {
				break;
			}
		}
	}
	if (m_dimensions != -1) {
		if (m_config.m_buildIndices.m_bool) {
			buildIndices();
		}
		if (m_config.m_buildMassProperties.m_bool) {
			buildMassProperties();
		}
	}
	return m_dimensions;
}

void hkgpConvexHullImpl::reset(int32_t a1) {
	__m128 v2 = _mm_undefined_ps();
	__m128 v3 = _mm_undefined_ps();
	__m128 v4 = _mm_undefined_ps();
	hkVector4 v7;
	hkVector4 v8;
	hkVector4 v9;
	m_mesh.reset();
	v2[0] = 0.0f;
	v3[0] = 0.0f;
	v4[0] = 3.40282e+38f;
	m_planes.m_size = 0;
	m_pendings.m_size = 0;
	m_supportCache.m_size = 0;
	m_hasMassProperties.m_bool = 0;
	m_hasIndexing.m_bool = 0;
	m_pid = 0;
	__m128 v6 = _mm_unpacklo_ps(_mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))), v3),
		_mm_unpacklo_ps(v2, v4));
	m_centroids[0u].m_quad = v6;
	m_centroids[1].m_quad = v6;
	v6[0] = 3.40282e+38f;
	v7.m_quad = _mm_shuffle_ps(v6, v6, 0);
	m_boundingBoxes[0u].m_min.m_quad = v7.m_quad;
	v8.m_quad = _mm_shuffle_ps(_mm_set_ss(rdata_104E148), _mm_set_ss(rdata_104E148), 0);
	m_boundingBoxes[0u].m_max.m_quad = v8.m_quad;
	m_boundingBoxes[1].m_min.m_quad = v7.m_quad;
	m_boundingBoxes[1].m_max.m_quad = v8.m_quad;
	m_localTransform = hkTransformIdentity;
	m_inertia.m_quad = hkVector4Zero.m_quad;
	m_inertiaMatrix = hkMatrix3::getIdentity();
	v9.m_quad = hkVector4Zero.m_quad;
	m_volume = 0.0f;
	m_surface = 0.0f;
	m_projectionAxis.m_quad = v9.m_quad;
	m_dimensions = -1;
	m_scale.m_quad = hkVector4Zero.m_quad;
	m_scaleInv.m_quad = hkVector4Zero.m_quad;
	m_origin.m_quad = hkVector4Zero.m_quad;
	if (!static_cast<uint8_t>(a1)) {
		m_projectionPlane.m_quad = hkVector4Zero.m_quad;
	}
}

template <typename POSITION> hkVector4& hkgpConvexHullImpl::getCentroid() {
	hkVector4& centroid = m_centroids[POSITION::INDEX];
	if (centroid.m_quad[3] == 3.402820018375656e+38) {
		centroid.m_quad = hkVector4Zero.m_quad;
		for (const Vertex* v = m_mesh.m_vertices.m_used; v; v = v->m_next) {
			centroid.m_quad = _mm_add_ps(POSITION::get(*this, *v), centroid.m_quad);
		}
		const int32_t numVertices = m_mesh.m_vertices.m_numUsed;
		if (numVertices) {
			const float fInvCount = 1.0 / static_cast<double>(numVertices);
			centroid.m_quad = _mm_mul_ps(_mm_set1_ps(fInvCount), centroid.m_quad);
		}
		centroid.m_quad[3] = 0.0f;
	}
	return centroid;
}

// Clears the part of the hull the vertex v sees, from the edge e on: e's triangle, when this pass has not visited it
// yet (m_pid) and v is on or above its plane, is unlinked from its neighbours and released with each vertex it leaves
// unreferenced, after the triangles across its three edges were cleared the same way; the first edge whose triangle v
// is below goes into horizon.
void hkgpConvexHullImpl::clearHorizon(const hkgpConvexHullImpl::Vertex& a1, const hkgpConvexHullImpl::Edge& a2,
	hkgpConvexHullImpl::Edge& a3) {
	hkgpConvexHullImpl::Triangle* pTriangle = a2.m_triangle;
	if (!pTriangle || pTriangle->m_pid == m_pid) {
		return;
	}
	pTriangle->m_pid = m_pid;
	if (distanceToPlane(a1, a2.m_triangle->m_plane) < 0) {
		a3.m_triangle = a2.m_triangle;
		a3.m_index = a2.m_index;
		return;
	}
	// The links across e, then across its next and its previous edge ((9 >> 2i) & 3 and (18 >> 2i) & 3): a link is the
	// untyped word hkgpTopology's Uid, the neighbour's address with its edge index in the low two bits, which are
	// masked off to get the triangle back (hence the casts from the integer here and in the unbinds below).
	hkgpConvexHullImpl::Edge link;
	// EXE-INLINED none: hkgpTopology::Edge::link
	uintptr_t uid = a2.m_triangle->m_links[a2.m_index];
	link.m_index = static_cast<uint32_t>(uid & 3);
	link.m_triangle = reinterpret_cast<hkgpConvexHullImpl::Triangle*>(uid & ~static_cast<uintptr_t>(3));
	clearHorizon(a1, link, a3);
	// EXE-INLINED none: hkgpTopology::Edge::link
	uid = a2.m_triangle->m_links[(9 >> (2 * a2.m_index)) & 3];
	link.m_index = static_cast<uint32_t>(uid & 3);
	link.m_triangle = reinterpret_cast<hkgpConvexHullImpl::Triangle*>(uid & ~static_cast<uintptr_t>(3));
	clearHorizon(a1, link, a3);
	// EXE-INLINED none: hkgpTopology::Edge::link
	uid = a2.m_triangle->m_links[(18 >> (2 * a2.m_index)) & 3];
	link.m_index = static_cast<uint32_t>(uid & 3);
	link.m_triangle = reinterpret_cast<hkgpConvexHullImpl::Triangle*>(uid & ~static_cast<uintptr_t>(3));
	clearHorizon(a1, link, a3);

	// EXE-INLINED none: hkgpTopology::Edge::unbind
	uid = a2.m_triangle->m_links[a2.m_index];
	if ((uid & ~static_cast<uintptr_t>(3)) != 0) {
		reinterpret_cast<hkgpConvexHullImpl::Triangle*>(uid & ~static_cast<uintptr_t>(3))->m_links[uid & 3] = 0;
	}
	a2.m_triangle->m_links[a2.m_index] = 0;
	// EXE-INLINED none: hkgpTopology::Edge::unbind
	hkgpConvexHullImpl::Triangle* pNextTriangle = a2.m_triangle;
	const uint32_t uiNext = (9 >> (2 * a2.m_index)) & 3;
	uid = pNextTriangle->m_links[uiNext];
	if ((uid & ~static_cast<uintptr_t>(3)) != 0) {
		reinterpret_cast<hkgpConvexHullImpl::Triangle*>(uid & ~static_cast<uintptr_t>(3))->m_links[uid & 3] = 0;
	}
	pNextTriangle->m_links[uiNext] = 0;
	// EXE-INLINED none: hkgpTopology::Edge::unbind
	hkgpConvexHullImpl::Triangle* pPrevTriangle = a2.m_triangle;
	const uint32_t uiPrev = (18 >> (2 * a2.m_index)) & 3;
	uid = pPrevTriangle->m_links[uiPrev];
	if ((uid & ~static_cast<uintptr_t>(3)) != 0) {
		reinterpret_cast<hkgpConvexHullImpl::Triangle*>(uid & ~static_cast<uintptr_t>(3))->m_links[uid & 3] = 0;
	}
	pPrevTriangle->m_links[uiPrev] = 0;

	hkgpConvexHullImpl::Triangle* pDead = a2.m_triangle;
	for (int32_t k = 0; k < 3; k++) {
		hkgpConvexHullImpl::Vertex* pVertex = pDead->m_vertices[k];
		--pVertex->m_refs;
		if (pVertex->m_refs == 0) {
			// EXE-INLINED none: hkgpAbstractMeshDefinitions::List<hkgpConvexHullImpl::Vertex, ...>::release
			hkgpConvexHullImpl::Vertex* pItem = pDead->m_vertices[k];
			hkgpConvexHullImpl::Vertex* pNext = pItem->m_next;
			hkgpConvexHullImpl::Vertex* pPrev = pItem->m_prev;
			if (pNext) {
				pNext->m_prev = pPrev;
			}
			if (pPrev) {
				pPrev->m_next = pNext;
			} else {
				m_mesh.m_vertices.m_used = pNext;
			}
			--m_mesh.m_vertices.m_numUsed;
			m_mesh.m_vertices.m_allocator.release(pItem);
		}
	}
	// EXE-INLINED none: hkgpAbstractMeshDefinitions::List<hkgpConvexHullImpl::Triangle, ...>::release
	hkgpConvexHullImpl::Triangle* pNext = pDead->m_next;
	hkgpConvexHullImpl::Triangle* pPrev = pDead->m_prev;
	if (pNext) {
		pNext->m_prev = pPrev;
	}
	if (pPrev) {
		pPrev->m_next = pNext;
	} else {
		m_mesh.m_triangles.m_used = pNext;
	}
	--m_mesh.m_triangles.m_numUsed;
	m_mesh.m_triangles.m_allocator.release(pDead);
}

// With fewer than four pending vertices it reports "Internal error" and goes on. It looks for the first pending
// vertices i < j < k < l that span a volume: the plane of i, j and k with a non-zero normal (with
// m_allowLowerDimensions, m_projectionPlane keeps the longest of those planes' normalized float equations, which is the
// first non-degenerate one), then an l off it. Their four vertices and four triangles go into the mesh, the first two
// vertices swapped when l is above the plane, the four pendings are removed (the highest index first) and the search
// ends. Every other pending vertex is then inserted, from the last: the first triangle it is above starts the horizon
// clearHorizon clears, and a fan of triangles from the new vertex closes the hole. The pending array is then freed.
// Without a tetrahedron the pendings stay and false is returned. List::allocate is handed temporaries with only the
// fields their constructors set written (a vertex's m_scratch and m_index, a triangle's m_plane.m_index and m_index),
// and allocate copies the whole item into the pool, so the padding of the triangle plane's normal is not reproduced.
bool hkgpConvexHullImpl::flushPending() {
	bool done = false;
	const int32_t numPendings = m_pendings.getSize();
	if (numPendings < 4) {
		char buf[512];
		hkErrStream errStream(buf, 512);
		errStream.operator<<("Internal error");
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x642CF968, buf,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 756)) {
			__builtin_debugtrap();
		}
	}
	m_projectionPlane.m_quad = hkVector4Zero.m_quad;
	// The SDK's Vertex() and Triangle() temporaries handed to allocate: their constructors set only these fields.
	Vertex vertexItem;
	vertexItem.m_scratch = 0.0f;
	vertexItem.m_index = -1;
	Triangle triangleItem;
	triangleItem.m_plane.m_index = -1;
	triangleItem.m_index = -1;
	for (int32_t i = 0; i < numPendings; ++i) {
		for (int32_t j = i + 1; j < numPendings; ++j) {
			for (int32_t k = j + 1; k < numPendings; ++k) {
				const Vertex& a = m_pendings[i];
				const Vertex& b = m_pendings[j];
				const Vertex& c = m_pendings[k];
				Plane plane;
				if (!makePlane(a, b, c, plane)) {
					continue;
				}
				if (m_config.m_allowLowerDimensions.m_bool) {
					// EXE-INLINED none: hkgpConvexHullImpl::transform
					// The SDK's transform (inlined): the integer coordinates scaled by m_scaleInv about m_origin.
					const __m128 pa =
						_mm_add_ps(_mm_mul_ps(_mm_setr_ps(static_cast<float>(a.m_x[0]), static_cast<float>(a.m_x[1]),
												  static_cast<float>(a.m_x[2]), 0.0f),
									   m_scaleInv.m_quad),
							m_origin.m_quad);
					const __m128 pb =
						_mm_add_ps(_mm_mul_ps(_mm_setr_ps(static_cast<float>(b.m_x[0]), static_cast<float>(b.m_x[1]),
												  static_cast<float>(b.m_x[2]), 0.0f),
									   m_scaleInv.m_quad),
							m_origin.m_quad);
					const __m128 pc =
						_mm_add_ps(_mm_mul_ps(_mm_setr_ps(static_cast<float>(c.m_x[0]), static_cast<float>(c.m_x[1]),
												  static_cast<float>(c.m_x[2]), 0.0f),
									   m_scaleInv.m_quad),
							m_origin.m_quad);
					// EXE-INLINED none: hkgpConvexHullImpl::planeFromTriangle
					// The SDK's planeFromTriangle (inlined): the normal (pb - pa) x (pc - pa), scaled by its length's
					// reciprocal square root refined by one Newton step (0 for a zero length), and w = 0 - n.pa.
					const __m128 e1 = _mm_sub_ps(pb, pa);
					const __m128 e2 = _mm_sub_ps(pc, pa);
					const __m128 n = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(e2, e2, 0xD2), _mm_shuffle_ps(e1, e1, 0xC9)),
						_mm_mul_ps(_mm_shuffle_ps(e2, e2, 0xC9), _mm_shuffle_ps(e1, e1, 0xD2)));
					const __m128 n2 = _mm_mul_ps(n, n);
					const float fXY = _mm_shuffle_ps(n2, n2, 0x55)[0] + n2[0];
					__m128 len2 = _mm_shuffle_ps(n2, n2, 0xAA);
					len2[0] = len2[0] + fXY;
					const float fRsqrt = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(len2[0])));
					const __m128 isZero = _mm_cmpeq_ss(len2, _mm_set_ss(0.0f));
					const float fRefine = 3.0f - static_cast<float>(static_cast<float>(len2[0] * fRsqrt) * fRsqrt);
					__m128 invLen = _mm_setzero_ps();
					invLen[0] = static_cast<float>(0.5f * fRsqrt) * fRefine;
					invLen = _mm_andnot_ps(isZero, invLen);
					__m128 planeEquation = _mm_mul_ps(_mm_shuffle_ps(invLen, invLen, 0x00), n);
					const __m128 na = _mm_mul_ps(planeEquation, pa);
					planeEquation[3] = 0.0f -
						static_cast<float>(_mm_shuffle_ps(na, na, 0xAA)[0] +
							static_cast<float>(_mm_shuffle_ps(na, na, 0x55)[0] + na[0]));
					// Keeps the plane whose normal is longer than m_projectionPlane's (lengthSquared3 of each).
					const __m128 p2 = _mm_mul_ps(planeEquation, planeEquation);
					const float fPlaneLength2 =
						_mm_shuffle_ps(p2, p2, 0xAA)[0] + static_cast<float>(_mm_shuffle_ps(p2, p2, 0x55)[0] + p2[0]);
					const __m128 q2 = _mm_mul_ps(m_projectionPlane.m_quad, m_projectionPlane.m_quad);
					const float fProjectionLength2 =
						_mm_shuffle_ps(q2, q2, 0xAA)[0] + static_cast<float>(_mm_shuffle_ps(q2, q2, 0x55)[0] + q2[0]);
					if (fProjectionLength2 < fPlaneLength2) {
						m_projectionPlane.m_quad = planeEquation;
					}
				}
				for (int32_t l = k + 1; l < numPendings; ++l) {
					const int64_t distance = distanceToPlane(m_pendings[l], plane);
					if (distance == 0) {
						continue;
					}
					Vertex* v0 = m_mesh.m_vertices.allocate(vertexItem);
					Vertex* v1 = m_mesh.m_vertices.allocate(vertexItem);
					Vertex* v2 = m_mesh.m_vertices.allocate(vertexItem);
					Vertex* v3 = m_mesh.m_vertices.allocate(vertexItem);
					makeVertex(m_pendings[i], v0);
					makeVertex(m_pendings[j], v1);
					makeVertex(m_pendings[k], v2);
					makeVertex(m_pendings[l], v3);
					// The faces turn outwards: with l above the plane of i, j and k, the first two swap.
					if (distance > 0) {
						Vertex* pSwap = v0;
						v0 = v1;
						v1 = pSwap;
					}
					Triangle* t0 = m_mesh.m_triangles.allocate(triangleItem);
					Triangle* t1 = m_mesh.m_triangles.allocate(triangleItem);
					Triangle* t2 = m_mesh.m_triangles.allocate(triangleItem);
					Triangle* t3 = m_mesh.m_triangles.allocate(triangleItem);
					makeTriangle(v0, v1, v2, t0);
					makeTriangle(v0, v3, v1, t1);
					makeTriangle(v0, v2, v3, t2);
					makeTriangle(v1, v3, v2, t3);
					// The neighbour links, each the untyped word hkgpTopology's Uid: the neighbour's address plus the
					// index of the shared edge in it.
					t0->m_links[0] = reinterpret_cast<uintptr_t>(t1) + 2;
					t1->m_links[2] = reinterpret_cast<uintptr_t>(t0);
					t0->m_links[1] = reinterpret_cast<uintptr_t>(t3) + 2;
					t3->m_links[2] = reinterpret_cast<uintptr_t>(t0) + 1;
					t0->m_links[2] = reinterpret_cast<uintptr_t>(t2);
					t2->m_links[0] = reinterpret_cast<uintptr_t>(t0) + 2;
					t1->m_links[0] = reinterpret_cast<uintptr_t>(t2) + 2;
					t2->m_links[2] = reinterpret_cast<uintptr_t>(t1);
					t2->m_links[1] = reinterpret_cast<uintptr_t>(t3) + 1;
					t3->m_links[1] = reinterpret_cast<uintptr_t>(t2) + 1;
					t3->m_links[0] = reinterpret_cast<uintptr_t>(t1) + 1;
					t1->m_links[1] = reinterpret_cast<uintptr_t>(t3);
					m_pendings.removeAt(l);
					m_pendings.removeAt(k);
					m_pendings.removeAt(j);
					m_pendings.removeAt(i);
					// Ends the search: every loop index set to the original count.
					l = numPendings;
					k = numPendings;
					j = numPendings;
					i = numPendings;
					done = true;
				}
			}
		}
	}
	if (!done) {
		return done;
	}
	while (m_pendings.getSize() > 0) {
		const Vertex& v = m_pendings[m_pendings.getSize() - 1];
		// EXE-INLINED none: hkgpConvexHullImpl::insert
		// EXE-INLINED none: hkgpConvexHullImpl::locateBoundaryExhaustive
		// The SDK's insert (inlined): the first triangle v is above (locateBoundaryExhaustive, inlined); none when v is
		// inside the hull.
		Triangle* pAbove = m_mesh.m_triangles.m_used;
		while (pAbove && distanceToPlane(v, pAbove->m_plane) <= 0) {
			pAbove = pAbove->m_next;
		}
		if (pAbove) {
			Vertex* pNew = m_mesh.m_vertices.allocate(vertexItem);
			Edge horizon;
			horizon.m_triangle = nullptr;
			horizon.m_index = 0;
			makeVertex(v, pNew);
			Edge start;
			start.m_triangle = pAbove;
			start.m_index = 0;
			++m_pid;
			clearHorizon(*pNew, start, horizon);
			// The first naked edge of the horizon's triangle ((9 >> 2 * i) & 3 is ip1mod3, (i + 1) % 3).
			while ((horizon.m_triangle->m_links[horizon.m_index] & ~static_cast<uintptr_t>(3)) != 0) {
				horizon.m_index = (9 >> (2 * horizon.m_index)) & 3;
			}
			// The horizon's edges as Uids (the triangle's address plus the edge index; see the links above).
			const uintptr_t startUid = reinterpret_cast<uintptr_t>(horizon.m_triangle) + horizon.m_index;
			Triangle* pFirst = nullptr;
			Triangle* pPrevious = nullptr;
			for (;;) {
				horizon = nextHorizon(horizon);
				if (reinterpret_cast<uintptr_t>(horizon.m_triangle) + horizon.m_index == startUid) {
					break;
				}
				Triangle* pFan = m_mesh.m_triangles.allocate(triangleItem);
				makeTriangle(pNew, horizon.m_triangle->m_vertices[(9 >> (2 * horizon.m_index)) & 3],
					horizon.m_triangle->m_vertices[horizon.m_index], pFan);
				horizon.m_triangle->m_links[horizon.m_index] = reinterpret_cast<uintptr_t>(pFan) + 1;
				pFan->m_links[1] = reinterpret_cast<uintptr_t>(horizon.m_triangle) + horizon.m_index;
				if (pPrevious) {
					pPrevious->m_links[0] = reinterpret_cast<uintptr_t>(pFan) + 2;
					pFan->m_links[2] = reinterpret_cast<uintptr_t>(pPrevious);
				} else {
					pFirst = pFan;
				}
				pPrevious = pFan;
			}
			// The last fan triangle closes the fan on the start edge; pPrevious is not tested.
			Triangle* pLast = m_mesh.m_triangles.allocate(triangleItem);
			makeTriangle(pNew, horizon.m_triangle->m_vertices[(9 >> (2 * horizon.m_index)) & 3],
				horizon.m_triangle->m_vertices[horizon.m_index], pLast);
			horizon.m_triangle->m_links[horizon.m_index] = reinterpret_cast<uintptr_t>(pLast) + 1;
			pLast->m_links[1] = reinterpret_cast<uintptr_t>(horizon.m_triangle) + horizon.m_index;
			pPrevious->m_links[0] = reinterpret_cast<uintptr_t>(pLast) + 2;
			pLast->m_links[2] = reinterpret_cast<uintptr_t>(pPrevious);
			pLast->m_links[0] = reinterpret_cast<uintptr_t>(pFirst) + 2;
			if (pFirst) {
				pFirst->m_links[2] = reinterpret_cast<uintptr_t>(pLast);
			}
		}
		m_pendings.popBack();
	}
	m_pendings.clearAndDeallocate();
	return done;
}

// Only 3-dimensional hulls are simplified (below 3 nothing, above an error). Each step that changes the hull rebuilds
// it from its vertices: - decimateVertices to m_maxVertices (from 4 on), the change seen in the vertex count; - while
// m_minArea > 0: the triangle with the smallest |e1 x e2| below 2 m_minArea loses the vertex opposite its longest edge
// (m_index -1), and the hull is rebuilt from the vertices still indexed; - m_removeUnreferencedVertices: while the
// faces reference fewer vertices than the hull has, it is rebuilt from the referenced ones (an error first when the
// hull has no indices); - after any change (or m_forceRebuild), a last rebuild from every vertex, with indices and mass
// properties built again when the hull had them on entry.
void hkgpConvexHullImpl::simplify(const hkgpConvexHull::SimplifyConfig& config) {
	const hkBool hadMassProperties = m_hasMassProperties;
	const hkBool hadIndexing = m_hasIndexing;
	bool changed = config.m_forceRebuild.m_bool != 0;
	if (m_dimensions < 0 || m_dimensions > 3) {
		alignas(16) char acBuffer[512];
		hkErrStream kStream(acBuffer, 512);
		kStream << "Operation not implemented for that dimension";
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x3E0ABBAB, acBuffer,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 1403)) {
			__builtin_debugtrap();
		}
		return;
	}
	if (m_dimensions <= 2) {
		return;
	}
	if (config.m_maxVertices >= 4) {
		const int32_t iNumVertices = m_mesh.m_vertices.m_numUsed;
		decimateVertices(config.m_maxVertices, config.m_ensureContainment.m_bool != 0);
		changed = m_mesh.m_vertices.m_numUsed != iNumVertices;
	}
	if (0.0f < config.m_minArea) {
		const float fMaxArea = config.m_minArea + config.m_minArea;
		do {
			// The triangle with the smallest cross product length below fMaxArea.
			float fBest = fMaxArea;
			Triangle* pSmallest = nullptr;
			Triangle* pTriangle = m_mesh.m_triangles.m_used;
			if (!pTriangle) {
				break;
			}
			do {
				const hkQuadReal v0 = pTriangle->m_vertices[0]->m_source.m_quad;
				const hkQuadReal e1 = _mm_sub_ps(pTriangle->m_vertices[1]->m_source.m_quad, v0);
				const hkQuadReal e2 = _mm_sub_ps(pTriangle->m_vertices[2]->m_source.m_quad, v0);
				hkQuadReal n = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(e2, e2, 0xD2), _mm_shuffle_ps(e1, e1, 0xC9)),
					_mm_mul_ps(_mm_shuffle_ps(e2, e2, 0xC9), _mm_shuffle_ps(e1, e1, 0xD2)));
				n = _mm_mul_ps(n, n);
				const hkQuadReal sum =
					_mm_add_ss(_mm_shuffle_ps(n, n, 0xAA), _mm_add_ss(_mm_shuffle_ps(n, n, 0x55), n));
				const float fArea = _mm_cvtss_f32(_mm_sqrt_ss(sum));
				if (fBest > fArea) {
					fBest = fArea;
					pSmallest = pTriangle;
				}
				pTriangle = pTriangle->m_next;
			} while (pTriangle);
			if (!pSmallest) {
				break;
			}
			// Its longest edge (i, i + 1), the first of equal ones.
			hkQuadReal d =
				_mm_sub_ps(pSmallest->m_vertices[0]->m_source.m_quad, pSmallest->m_vertices[1]->m_source.m_quad);
			d = _mm_mul_ps(d, d);
			fBest = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(d, d, 0xAA), _mm_add_ss(_mm_shuffle_ps(d, d, 0x55), d)));
			int32_t iLongest = 0;
			for (int32_t i = 1; i < 3; i++) {
				d = _mm_sub_ps(pSmallest->m_vertices[i]->m_source.m_quad,
					pSmallest->m_vertices[(i + 1) % 3]->m_source.m_quad);
				d = _mm_mul_ps(d, d);
				const float fLength =
					_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(d, d, 0xAA), _mm_add_ss(_mm_shuffle_ps(d, d, 0x55), d)));
				if (fBest < fLength) {
					fBest = fLength;
					iLongest = i;
				}
			}
			if (iLongest == 0) {
				pSmallest->m_vertices[2]->m_index = -1;
			} else if (iLongest == 1) {
				pSmallest->m_vertices[0]->m_index = -1;
			} else {
				pSmallest->m_vertices[1]->m_index = -1;
			}
			// The hull again, from the vertices still indexed.
			const int32_t iNumVertices = m_mesh.m_vertices.m_numUsed;
			FNVD_INLINED_LOCAL(hkArray<hkVector4>, points, std::nullopt, std::nullopt);
			// EXE-INLINED none: hkArray<hkVector4>::hkArray
			points.m_data = nullptr;
			points.m_size = 0;
			points.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
			// EXE-INLINED none: hkArray<hkVector4>::reserve
			hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
			int32_t iCapacity = points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
			if (iCapacity < iNumVertices) {
				iCapacity += iCapacity;
				if (iNumVertices >= iCapacity) {
					iCapacity = iNumVertices;
				}
				hkArrayUtil::_reserve(*pHeap, &points, iCapacity, sizeof(hkVector4));
			}
			for (Vertex* pVertex = m_mesh.m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
				if (pVertex->m_index != -1) {
					// EXE-INLINED none: hkArray<hkVector4>::pushBack
					hkMemoryAllocator* pMore = hkMemoryRouter::getInstance().m_heap;
					if (points.m_size == (points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
						hkArrayUtil::_reserveMore(*pMore, &points, sizeof(hkVector4));
					}
					points.m_data[points.m_size] = pVertex->m_source;
					points.m_size++;
				}
			}
			if (build(points.m_data, points.m_size, 0, nullptr) == 3) {
				buildIndices();
			}
			changed = true;
			// EXE-INLINED none: hkArray<hkVector4>::~hkArray
			pHeap = hkMemoryRouter::getInstance().m_heap;
			points.m_size = 0;
			if (points.m_capacityAndFlags >= 0) {
				pHeap->bufFree(points.m_data,
					static_cast<int32_t>(sizeof(hkVector4)) *
						(points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK));
			}
		} while (m_dimensions == 3);
	}
	if (config.m_removeUnreferencedVertices.m_bool && m_dimensions == 3) {
		if (!m_hasIndexing.m_bool) {
			alignas(16) char acBuffer[512];
			hkErrStream kStream(acBuffer, 512);
			const uint32_t uiUid = m_uid;
			kStream << "No index available (" << uiUid << ")";
			if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x79F9D886, acBuffer,
					".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 328)) {
				__builtin_debugtrap();
			}
		}
		using IntArray = hkArray<int32_t, hkContainerHeapAllocator>;
		// EXE-INLINED none: hkArray<int>::hkArray
		FNVD_INLINED_LOCAL(IntArray, verticesPerFace, std::nullopt, std::nullopt);
		verticesPerFace.m_data = nullptr;
		verticesPerFace.m_capacityAndFlags = IntArray::DONT_DEALLOCATE_FLAG;
		FNVD_INLINED_LOCAL(IntArray, indices, std::nullopt, std::nullopt);
		indices.m_data = nullptr;
		indices.m_capacityAndFlags = IntArray::DONT_DEALLOCATE_FLAG;
		FNVD_INLINED_LOCAL(IntArray, used, std::nullopt, std::nullopt);
		used.m_data = nullptr;
		used.m_capacityAndFlags = IntArray::DONT_DEALLOCATE_FLAG;
		do {
			// EXE-INLINED none: hkArray<int>::clear
			verticesPerFace.m_size = 0;
			indices.m_size = 0;
			used.m_size = 0;
			generateIndexedFaces(verticesPerFace, indices, false, false);
			// The distinct vertex indices the faces use.
			for (int32_t i = 0; i < indices.m_size; i++) {
				// EXE-INLINED none: hkArray<int>::indexOf
				int32_t iFound = -1;
				for (int32_t k = 0; k < used.m_size; k++) {
					if (used.m_data[k] == indices.m_data[i]) {
						iFound = k;
						break;
					}
				}
				if (iFound == -1) {
					// EXE-INLINED none: hkArray<int>::pushBack
					hkMemoryAllocator* pMore = hkMemoryRouter::getInstance().m_heap;
					if (used.m_size == (used.m_capacityAndFlags & IntArray::CAPACITY_MASK)) {
						hkArrayUtil::_reserveMore(*pMore, &used, sizeof(int32_t));
					}
					used.m_data[used.m_size] = indices.m_data[i];
					used.m_size++;
				}
			}
			const int32_t iNumVertices = m_mesh.m_vertices.m_numUsed;
			if (used.m_size >= iNumVertices) {
				break;
			}
			changed = true;
			FNVD_INLINED_LOCAL(hkArray<hkVector4>, positions, std::nullopt, std::nullopt);
			positions.m_data = nullptr;
			positions.m_size = 0;
			positions.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
			FNVD_INLINED_LOCAL(hkArray<hkVector4>, points, std::nullopt, std::nullopt);
			points.m_data = nullptr;
			points.m_size = 0;
			points.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
			// Every vertex's position.
			hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
			const int32_t iOldSize = positions.m_size;
			int32_t iSize = iOldSize + iNumVertices;
			int32_t iCapacity = positions.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
			if (iCapacity < iSize) {
				iCapacity += iCapacity;
				if (iSize < iCapacity) {
					iSize = iCapacity;
				}
				hkArrayUtil::_reserve(*pHeap, &positions, iSize, sizeof(hkVector4));
			}
			positions.m_size += iNumVertices;
			hkVector4* pPosition = positions.m_data + iOldSize;
			for (Vertex* pVertex = m_mesh.m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
				*pPosition = pVertex->m_source;
				pPosition++;
			}
			// The referenced ones.
			const int32_t iNumUsed = used.m_size;
			pHeap = hkMemoryRouter::getInstance().m_heap;
			iCapacity = points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
			if (iCapacity < iNumUsed) {
				iCapacity += iCapacity;
				if (iNumUsed >= iCapacity) {
					iCapacity = iNumUsed;
				}
				hkArrayUtil::_reserve(*pHeap, &points, iCapacity, sizeof(hkVector4));
			}
			points.m_size = iNumUsed;
			for (int32_t i = 0; i < used.m_size; i++) {
				points.m_data[i] = positions.m_data[used.m_data[i]];
			}
			if (build(points.m_data, points.m_size, 0, nullptr) == 3) {
				buildIndices();
			}
			// EXE-INLINED none: hkArray<hkVector4>::clearAndDeallocate
			pHeap = hkMemoryRouter::getInstance().m_heap;
			points.m_size = 0;
			if ((points.m_capacityAndFlags & hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG) == 0) {
				pHeap->bufFree(points.m_data,
					static_cast<int32_t>(sizeof(hkVector4)) *
						(points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK));
			}
			points.m_data = nullptr;
			points.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
			pHeap = hkMemoryRouter::getInstance().m_heap;
			positions.m_size = 0;
			if ((positions.m_capacityAndFlags & hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG) == 0) {
				pHeap->bufFree(positions.m_data,
					static_cast<int32_t>(sizeof(hkVector4)) *
						(positions.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK));
			}
		} while (m_dimensions == 3);
		// EXE-INLINED none: hkArray<int>::clearAndDeallocate
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		used.m_size = 0;
		if ((used.m_capacityAndFlags & IntArray::DONT_DEALLOCATE_FLAG) == 0) {
			pHeap->bufFree(used.m_data,
				static_cast<int32_t>(sizeof(int32_t)) * (used.m_capacityAndFlags & IntArray::CAPACITY_MASK));
		}
		pHeap = hkMemoryRouter::getInstance().m_heap;
		indices.m_size = 0;
		if ((indices.m_capacityAndFlags & IntArray::DONT_DEALLOCATE_FLAG) == 0) {
			pHeap->bufFree(indices.m_data,
				static_cast<int32_t>(sizeof(int32_t)) * (indices.m_capacityAndFlags & IntArray::CAPACITY_MASK));
		}
		pHeap = hkMemoryRouter::getInstance().m_heap;
		verticesPerFace.m_size = 0;
		if ((verticesPerFace.m_capacityAndFlags & IntArray::DONT_DEALLOCATE_FLAG) == 0) {
			pHeap->bufFree(verticesPerFace.m_data,
				static_cast<int32_t>(sizeof(int32_t)) * (verticesPerFace.m_capacityAndFlags & IntArray::CAPACITY_MASK));
		}
	}
	if (changed && m_dimensions == 3) {
		// The last rebuild, from every vertex.
		const int32_t iNumVertices = m_mesh.m_vertices.m_numUsed;
		FNVD_INLINED_LOCAL(hkArray<hkVector4>, points, std::nullopt, std::nullopt);
		points.m_data = nullptr;
		points.m_size = 0;
		points.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
		// EXE-INLINED none: hkArray<hkVector4>::expandBy
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		const int32_t iOldSize = points.m_size;
		int32_t iSize = iOldSize + iNumVertices;
		int32_t iCapacity = points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
		if (iCapacity < iSize) {
			iCapacity += iCapacity;
			if (iSize < iCapacity) {
				iSize = iCapacity;
			}
			hkArrayUtil::_reserve(*pHeap, &points, iSize, sizeof(hkVector4));
		}
		points.m_size += iNumVertices;
		hkVector4* pPoint = points.m_data + iOldSize;
		for (Vertex* pVertex = m_mesh.m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
			*pPoint = pVertex->m_source;
			pPoint++;
		}
		build(points.m_data, points.m_size, 0, nullptr);
		if (hadIndexing.m_bool) {
			buildIndices();
		}
		if (hadMassProperties.m_bool) {
			buildMassProperties();
		}
		// EXE-INLINED none: hkArray<hkVector4>::~hkArray
		pHeap = hkMemoryRouter::getInstance().m_heap;
		points.m_size = 0;
		if (points.m_capacityAndFlags >= 0) {
			pHeap->bufFree(points.m_data,
				static_cast<int32_t>(sizeof(hkVector4)) *
					(points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK));
		}
	}
}

// Writes the hull's faces as their vertex counts into verticesPerFace and their vertex indices into indices (with
// useSourceIndices the source index each vertex keeps in its w, else its own index), returning the number of faces. In
// three dimensions a face is a plane's triangles (sorted by plane index) walked round their boundary, in two the whole
// vertex list. With maximizeArea each face's indices are rotated to start at the corner whose triangle with the next
// two has the largest area (64-bit squared cross product). Each pushBack stores the element before it increments the
// size in memory.
template <typename VPF, typename IDX>
int32_t hkgpConvexHullImpl::generateIndexedFaces(hkArray<VPF, hkContainerHeapAllocator>& vpf,
	hkArray<IDX, hkContainerHeapAllocator>& vi, bool useSourceIndex, bool maximizeArea) {
	int32_t numFaces = 0;
	if (!m_hasIndexing.m_bool) {
		char acBuffer[0x200];
		hkErrStream errStream(acBuffer, 0x200);
		errStream << "No index available (" << m_uid << ")";
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x79F9D886, acBuffer,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 328)) {
			__builtin_debugtrap();
		}
	}
	vpf.m_size = 0;
	{
		// EXE-INLINED none: hkArray<VPF, hkContainerHeapAllocator>::reserve
		const int32_t numPlanes = m_planes.m_size;
		hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
		const int32_t capacity = vpf.getCapacity();
		if (capacity < numPlanes) {
			const int32_t doubled = 2 * capacity;
			hkArrayUtil::_reserve(heap, &vpf, numPlanes < doubled ? doubled : numPlanes,
				static_cast<int32_t>(sizeof(VPF)));
		}
	}
	vi.m_size = 0;
	{
		// EXE-INLINED none: hkArray<IDX, hkContainerHeapAllocator>::reserve
		const int32_t numIndices = 3 * m_mesh.m_vertices.m_numUsed;
		hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
		const int32_t capacity = vi.getCapacity();
		if (capacity < numIndices) {
			const int32_t doubled = 2 * capacity;
			hkArrayUtil::_reserve(heap, &vi, numIndices < doubled ? doubled : numIndices,
				static_cast<int32_t>(sizeof(IDX)));
		}
	}
	if (m_dimensions == 3) {
		using triangles_t = hkArray<Triangle*, hkContainerHeapAllocator>;
		FNVD_INLINED_LOCAL(triangles_t, triangles, std::nullopt, std::nullopt);
		triangles.m_data = nullptr;
		triangles.m_size = 0;
		triangles.m_capacityAndFlags = hkArrayBase<Triangle*>::DONT_DEALLOCATE_FLAG;
		{
			// EXE-INLINED none: hkArray<Triangle*, hkContainerHeapAllocator>::reserve
			const int32_t numTriangles = m_mesh.m_triangles.m_numUsed;
			hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
			const int32_t capacity = triangles.getCapacity();
			if (capacity < numTriangles) {
				const int32_t doubled = 2 * capacity;
				hkArrayUtil::_reserve(heap, &triangles, numTriangles < doubled ? doubled : numTriangles,
					static_cast<int32_t>(sizeof(Triangle*)));
			}
		}
		for (Triangle* t = m_mesh.m_triangles.m_used; t; t = t->m_next) {
			// EXE-INLINED none: hkArray<Triangle*, hkContainerHeapAllocator>::pushBackUnchecked
			triangles.m_data[triangles.m_size++] = t;
		}
		if (triangles.m_size > 1) {
			// The last argument is the comparator, an empty Triangle::SortByPlaneId.
			sortTrianglesByPlaneIndex(triangles.m_data, 0, triangles.m_size - 1, 0);
		}
		const int32_t numTriangles = triangles.m_size;
		for (int32_t i = 0; i < numTriangles; ++i) {
			Triangle* t = triangles.m_data[i];
			if (t->m_plane.m_index != numFaces) {
				continue;
			}
			if ((s_generateIndexedFacesNullEdge_guard & 1) == 0) {
				s_generateIndexedFacesNullEdge_guard |= 1u;
				s_generateIndexedFacesNullEdge.m_triangle = nullptr;
				s_generateIndexedFacesNullEdge.m_index = 0;
			}
			Edge e = s_generateIndexedFacesNullEdge;
			// The first edge of t whose neighbour lies on another plane. A link is the untyped word hkgpTopology's
			// Uid: the neighbour's address with its edge index in the low two bits.
			for (uint32_t k = 0; k < 3; ++k) {
				const Triangle* pNeighbour =
					reinterpret_cast<const Triangle*>(t->m_links[k] & ~static_cast<uintptr_t>(3));
				if (pNeighbour->m_plane.m_index != numFaces) {
					e.m_triangle = t;
					e.m_index = k;
					break;
				}
			}
			if (!e.m_triangle) {
				continue;
			}
			const Vertex* pStart = e.m_triangle->m_vertices[e.m_index];
			int32_t numFaceVertices = 0;
			do {
				++numFaceVertices;
				if (useSourceIndex) {
					const IDX index = static_cast<IDX>(e.m_triangle->m_vertices[e.m_index]->m_source.getInt24W());
					// EXE-INLINED none: hkArray<IDX, hkContainerHeapAllocator>::pushBack
					hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
					if (vi.m_size == vi.getCapacity()) {
						hkArrayUtil::_reserveMore(heap, &vi, static_cast<int32_t>(sizeof(IDX)));
					}
					vi.m_data[vi.m_size] = index;
				} else {
					const Vertex* v = e.m_triangle->m_vertices[e.m_index];
					// EXE-INLINED none: hkArray<IDX, hkContainerHeapAllocator>::pushBack
					hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
					if (vi.m_size == vi.getCapacity()) {
						hkArrayUtil::_reserveMore(heap, &vi, static_cast<int32_t>(sizeof(IDX)));
					}
					vi.m_data[vi.m_size] = static_cast<IDX>(v->m_index);
				}
				++vi.m_size;
				// The next boundary edge: the next edge of the triangle, then across each link into the same plane.
				Triangle* pTriangle = e.m_triangle;
				uint32_t index = (9 >> (2 * e.m_index)) & 3;
				while (reinterpret_cast<const Triangle*>(pTriangle->m_links[index] & ~static_cast<uintptr_t>(3))
						   ->m_plane.m_index == numFaces) {
					const uintptr_t link = pTriangle->m_links[index];
					pTriangle = reinterpret_cast<Triangle*>(link & ~static_cast<uintptr_t>(3));
					index = (9 >> (2 * static_cast<uint32_t>(link & 3))) & 3;
				}
				e.m_triangle = pTriangle;
				e.m_index = index;
			} while (e.m_triangle->m_vertices[e.m_index] != pStart);
			// EXE-INLINED none: hkArray<VPF, hkContainerHeapAllocator>::pushBack
			hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
			if (vpf.m_size == vpf.getCapacity()) {
				hkArrayUtil::_reserveMore(heap, &vpf, static_cast<int32_t>(sizeof(VPF)));
			}
			vpf.m_data[vpf.m_size] = static_cast<VPF>(numFaceVertices);
			vpf.m_size += 1;
			numFaces += 1;
		}
		// EXE-INLINED none: hkArray<Triangle*, hkContainerHeapAllocator>::~hkArray
		hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
		triangles.m_size = 0;
		if (triangles.m_capacityAndFlags >= 0) {
			heap.bufFree(triangles.m_data, static_cast<int32_t>(sizeof(Triangle*)) * triangles.getCapacity());
		}
	} else if (m_dimensions == 2) {
		const int32_t numVertices = m_mesh.m_vertices.m_numUsed;
		numFaces = 1;
		{
			// EXE-INLINED none: hkArray<VPF, hkContainerHeapAllocator>::pushBack
			hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
			if (vpf.m_size == vpf.getCapacity()) {
				hkArrayUtil::_reserveMore(heap, &vpf, static_cast<int32_t>(sizeof(VPF)));
			}
			vpf.m_data[vpf.m_size] = static_cast<VPF>(numVertices);
			vpf.m_size += 1;
		}
		if (useSourceIndex) {
			for (const Vertex* v = m_mesh.m_vertices.m_used; v; v = v->m_next) {
				const IDX index = static_cast<IDX>(v->m_source.getInt24W());
				// EXE-INLINED none: hkArray<IDX, hkContainerHeapAllocator>::pushBack
				hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
				if (vi.m_size == vi.getCapacity()) {
					hkArrayUtil::_reserveMore(heap, &vi, static_cast<int32_t>(sizeof(IDX)));
				}
				vi.m_data[vi.m_size] = index;
				vi.m_size += 1;
			}
		} else {
			for (int32_t i = 0; i < m_mesh.m_vertices.m_numUsed; ++i) {
				// EXE-INLINED none: hkArray<IDX, hkContainerHeapAllocator>::pushBack
				hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
				if (vi.m_size == vi.getCapacity()) {
					hkArrayUtil::_reserveMore(heap, &vi, static_cast<int32_t>(sizeof(IDX)));
				}
				vi.m_data[vi.m_size] = static_cast<IDX>(i);
				vi.m_size += 1;
			}
		}
	}
	if (maximizeArea && numFaces != 0) {
		IDX* pFace = vi.m_data;
		using vertices_t = hkArray<Vertex*, hkContainerHeapAllocator>;
		FNVD_INLINED_LOCAL(vertices_t, vertices, std::nullopt, std::nullopt);
		vertices.m_data = nullptr;
		vertices.m_size = 0;
		vertices.m_capacityAndFlags = hkArrayBase<Vertex*>::DONT_DEALLOCATE_FLAG;
		{
			// EXE-INLINED none: hkArray<Vertex*, hkContainerHeapAllocator>::reserve
			const int32_t numVertices = m_mesh.m_vertices.m_numUsed;
			hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
			const int32_t capacity = vertices.getCapacity();
			if (capacity < numVertices) {
				const int32_t doubled = 2 * capacity;
				hkArrayUtil::_reserve(heap, &vertices, numVertices < doubled ? doubled : numVertices,
					static_cast<int32_t>(sizeof(Vertex*)));
			}
		}
		for (Vertex* v = m_mesh.m_vertices.m_used; v; v = v->m_next) {
			// EXE-INLINED none: hkArray<Vertex*, hkContainerHeapAllocator>::pushBack
			hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
			if (vertices.m_size == vertices.getCapacity()) {
				hkArrayUtil::_reserveMore(heap, &vertices, static_cast<int32_t>(sizeof(Vertex*)));
			}
			vertices.m_data[vertices.m_size] = v;
			vertices.m_size += 1;
		}
		for (int32_t f = 0; f < vpf.m_size; ++f) {
			const int32_t n = vpf.m_data[f];
			if (n >= 3) {
				int64_t best = -1;
				int32_t bestIndex = -1;
				for (int32_t j = 0; j < n; ++j) {
					const Vertex* c = vertices.m_data[pFace[(j + 2) % n]];
					const Vertex* b = vertices.m_data[pFace[(j + 1) % n]];
					const Vertex* a = vertices.m_data[pFace[j]];
					const int32_t ex = b->m_x[0] - a->m_x[0];
					const int32_t ey = b->m_x[1] - a->m_x[1];
					const int32_t ez = b->m_x[2] - a->m_x[2];
					const int32_t fx = c->m_x[0] - a->m_x[0];
					const int32_t fy = c->m_x[1] - a->m_x[1];
					const int32_t fz = c->m_x[2] - a->m_x[2];
					const int32_t nx = fz * ey - fy * ez;
					const int32_t ny = fx * ez - fz * ex;
					const int32_t nz = fy * ex - fx * ey;
					int64_t area = nx * nx;
					area += ny * ny;
					area += nz * nz;
					if (area > best) {
						best = area;
						bestIndex = j;
					}
				}
				if (bestIndex > 0) {
					using rotated_t = hkInplaceArray<IDX, 32>;
					FNVD_INLINED_LOCAL(rotated_t, head, std::nullopt, std::nullopt);
					// EXE-INLINED none: hkInplaceArray<IDX, 32>::hkInplaceArray
					head.m_data = head.m_storage;
					head.m_size = 0;
					head.m_capacityAndFlags = 32 | hkArrayBase<IDX>::DONT_DEALLOCATE_FLAG;
					IDX* pDst = pFace;
					{
						// EXE-INLINED none: hkArray<IDX, hkContainerHeapAllocator>::reserve
						hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
						const int32_t capacity = head.getCapacity();
						if (capacity < bestIndex) {
							const int32_t doubled = 2 * capacity;
							hkArrayUtil::_reserve(heap, &head, bestIndex < doubled ? doubled : bestIndex,
								static_cast<int32_t>(sizeof(IDX)));
						}
					}
					// EXE-INLINED none: hkArray<IDX, hkContainerHeapAllocator>::setSizeUnchecked
					head.m_size = bestIndex;
					for (int32_t k = 0; k < bestIndex; ++k) {
						head.m_data[k] = pFace[k];
					}
					for (int32_t k = bestIndex; k < n; ++k) {
						*pDst++ = pFace[k];
					}
					for (int32_t k = 0; k < bestIndex; ++k) {
						*pDst++ = head.m_data[k];
					}
					// EXE-INLINED none: hkInplaceArray<IDX, 32>::~hkInplaceArray
					hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
					head.m_size = 0;
					if (head.m_capacityAndFlags >= 0) {
						heap.bufFree(head.m_data, static_cast<int32_t>(sizeof(IDX)) * head.getCapacity());
					}
				}
			}
			pFace += n;
		}
		// EXE-INLINED none: hkArray<Vertex*, hkContainerHeapAllocator>::~hkArray
		hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
		vertices.m_size = 0;
		if (vertices.m_capacityAndFlags >= 0) {
			heap.bufFree(vertices.m_data, static_cast<int32_t>(sizeof(Vertex*)) * vertices.getCapacity());
		}
	}
	return numFaces;
}

// Only lane 0 of v2, v3, m_quad_2, m_quad_3, v6 and v7 is ever used (each feeds unpcklps pairs whose results keep only
// lane 0 of it), so their other lanes are left undefined.
hkResult hkgpConvexHullImpl::buildMassProperties() {
	__m128 v2 = _mm_undefined_ps();
	hkVector4 v3;
	v3.m_quad = _mm_undefined_ps();
	__m128 m_quad_2 = _mm_undefined_ps();
	__m128 v6 = _mm_undefined_ps();
	__m128 v7 = _mm_undefined_ps();
	uint32_t v52;
	uint32_t v54;
	uint32_t v55;
	uint32_t v56;
	uint32_t v57;
	alignas(16) uint32_t v58;
	alignas(16) uint32_t v60;
	using apArray_t = hkArray<hkVector4, hkContainerHeapAllocator>;
	FNVD_INLINED_LOCAL(apArray_t, apArray, std::nullopt, std::nullopt);
	uint32_t v65;
	hkMatrix3 m_inertiaMatrix;
	float v67;
	__m128 i;
	hkVector4 a1;
	hkVector4 r;
	hkVector4 eigenVal;
	float fZ1;
	hkRotation eigenVec;
	hkSimdReal result;
	if (m_hasMassProperties.m_bool) {
		return static_cast<hkResult>(m_hasMassProperties.m_bool == 0);
	}
	int32_t m_dimensions = this->m_dimensions;
	if (m_dimensions != 3) {
		if (m_dimensions == 2) {
			float v62 = 0.0f;
			int32_t m_numUsed = m_mesh.m_vertices.m_numUsed;
			// EXE-INLINED none: hkArray<hkVector4, hkContainerHeapAllocator>'s constructor
			apArray.m_data = 0;
			apArray.m_size = 0;
			a1.m_quad = hkVector4Zero.m_quad;
			apArray.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
			hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
			int32_t m_numUsed_1 = m_numUsed;
			if (m_numUsed > 0) {
				if (m_numUsed < 2 * (apArray.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
					m_numUsed_1 = 2 * (apArray.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK);
				}
				hkArrayUtil::_reserve(*m_heap, &apArray, m_numUsed_1, sizeof(hkVector4));
			}
			hkVector4* v39 = apArray.m_data;
			hkgpConvexHullImpl::Vertex* m_used = m_mesh.m_vertices.m_used;
			apArray.m_size += m_numUsed;
			__m128 v41 = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)));
			hkVector4* v42 = apArray.m_data;
			if (m_used) {
				do {
					__m128 v4 = _mm_undefined_ps();
					v2[0] = static_cast<float>(m_used->m_x[0]);
					v3.m_quad[0] = static_cast<float>(m_used->m_x[1]);
					v4[0] = static_cast<float>(m_used->m_x[2]);
					m_quad_2[0] = 0.0f;
					hkVector4* v43 = v42++;
					__m128 v44 =
						_mm_mul_ps(_mm_unpacklo_ps(_mm_unpacklo_ps(v2, v4), _mm_unpacklo_ps(v3.m_quad, m_quad_2)),
							m_scaleInv.m_quad);
					v3.m_quad = m_origin.m_quad;
					v2 = _mm_add_ps(v44, v3.m_quad);
					v43->m_quad = v2;
					m_used = m_used->m_next;
				} while (m_used);
				v39 = apArray.m_data;
			}
			__m128 m_quad = a1.m_quad;
			if (apArray.m_size - 1 > 1) {
				m_quad_2 = v39->m_quad;
				hkVector4* v46 = v39 + 1;
				int32_t v47 = apArray.m_size - 2;
				do {
					v3.m_quad = _mm_sub_ps(v46->m_quad, m_quad_2);
					__m128 v48 = _mm_sub_ps(v46[1].m_quad, m_quad_2);
					__m128 v49 = _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v48, v48, 210),
														   _mm_shuffle_ps(v3.m_quad, v3.m_quad, 201)),
												_mm_mul_ps(_mm_shuffle_ps(v48, v48, 201),
													_mm_shuffle_ps(v3.m_quad, v3.m_quad, 210))),
						m_projectionPlane.m_quad);
					float v59 = std::fabs(static_cast<double>(_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(v49, v49, 0xAA),
						_mm_add_ss(_mm_shuffle_ps(v49, v49, 0x55), v49)))));
					++v46;
					--v47;
					v62 = v59 + v62;
					*reinterpret_cast<float*>(&v60) = v59 * 0.3333333432674408;
					m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v60)),
													   _mm_set_ss(std::bit_cast<float>(v60)), 0),
											_mm_add_ps(v48, v3.m_quad)),
						m_quad);
				} while (v47);
				v41 = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)));
			}
			m_hasMassProperties.m_bool = 1;
			v3.m_quad[0] = v41[0];
			m_quad_2[0] = v41[0];
			m_surface = 0.5 * v62;
			this->m_inertiaMatrix.m_col0.m_quad[0u] = 1.0f;
			m_inertia.m_quad =
				_mm_unpacklo_ps(_mm_unpacklo_ps(_mm_set_ss(1.0f), m_quad_2), _mm_unpacklo_ps(v3.m_quad, v41));
			*reinterpret_cast<float*>(&v57) = 1.0f / v62;
			m_localTransform.m_translation.m_quad =
				_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v57)),
										  _mm_set_ss(std::bit_cast<float>(v57)), 0),
							   m_quad),
					v39->m_quad);
			hkMemoryAllocator* m_heap_1 = (&hkMemoryRouter::getInstance())->m_heap;
			apArray.m_size = 0;
			if (apArray.m_capacityAndFlags >= 0) {
				m_heap_1->bufFree(apArray.m_data,
					sizeof(hkVector4) * (apArray.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK));
				return static_cast<hkResult>(m_hasMassProperties.m_bool == 0);
			}
		} else {
			m_surface = 0.0f;
			m_hasMassProperties.m_bool = 1;
			this->m_volume = 0.0f;
			this->m_inertiaMatrix = hkMatrix3::getIdentity();
			m_inertia.m_quad = hkVector4Zero.m_quad;
			m_localTransform = hkTransformIdentity;
		}
		return static_cast<hkResult>(m_hasMassProperties.m_bool == 0);
	}
	const hkVector4* v10 = &getCentroid<PositionAsTransform>();
	float v68 = 0.0f;
	float v61 = 0.0f;
	__m128 v13 = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)));
	a1 = *v10;
	hkgpConvexHullImpl::Triangle* m_next = m_mesh.m_triangles.m_used;
	r.m_quad = hkVector4Zero.m_quad;
	__m128 m_quad_1 = hkVector4Zero.m_quad;
	for (i = hkVector4Zero.m_quad; m_next;
		i[2] = (m_inertiaMatrix.m_col1.m_quad[0] * m_inertiaMatrix.m_col1.m_quad[1] * 2.0 +
				   (m_inertiaMatrix.m_col0.m_quad[0] * m_inertiaMatrix.m_col0.m_quad[1] * 2.0 +
					   (m_inertiaMatrix.m_col2.m_quad[1] * m_inertiaMatrix.m_col1.m_quad[0] +
						   m_inertiaMatrix.m_col1.m_quad[1] * m_inertiaMatrix.m_col0.m_quad[0] +
						   m_inertiaMatrix.m_col0.m_quad[1] * m_inertiaMatrix.m_col2.m_quad[0] +
						   m_inertiaMatrix.m_col2.m_quad[1] * m_inertiaMatrix.m_col0.m_quad[0] +
						   m_inertiaMatrix.m_col0.m_quad[1] * m_inertiaMatrix.m_col1.m_quad[0] +
						   m_inertiaMatrix.m_col1.m_quad[1] * m_inertiaMatrix.m_col2.m_quad[0])) +
				   2.0 * (m_inertiaMatrix.m_col2.m_quad[0] * m_inertiaMatrix.m_col2.m_quad[1])) *
				*reinterpret_cast<float*>(&v65) +
			i[2]) {
		__m128 m_quad_3 = _mm_undefined_ps();
		hkVector4 v18;
		hkVector4 v19;
		const hkgpConvexHullImpl::Vertex* v15 = m_next->m_vertices[2];
		float v76 = static_cast<float>(v15->m_x[0]);
		float v75 = static_cast<float>(v15->m_x[1]);
		m_quad_3[0] = static_cast<float>(v15->m_x[2]);
		const hkgpConvexHullImpl::Vertex* v16 = m_next->m_vertices[1];
		v2[0] = static_cast<float>(v16->m_x[0]);
		v67 = m_quad_3[0];
		float v77 = static_cast<float>(v16->m_x[1]);
		m_quad_3[0] = static_cast<float>(v16->m_x[2]);
		hkgpConvexHullImpl::Vertex* v17 = m_next->m_vertices[0];
		v6[0] = static_cast<float>(v17->m_x[2]);
		v3.m_quad[0] = static_cast<float>(v17->m_x[1]);
		v18.m_quad = m_scaleInv.m_quad;
		fZ1 = m_quad_3[0];
		m_quad_3[0] = static_cast<float>(v17->m_x[0]);
		v19.m_quad = m_origin.m_quad;
		v7[0] = v13[0];
		__m128 v20 =
			_mm_add_ps(_mm_mul_ps(_mm_unpacklo_ps(_mm_unpacklo_ps(m_quad_3, v6), _mm_unpacklo_ps(v3.m_quad, v7)),
						   v18.m_quad),
				v19.m_quad);
		__m128 v21 = _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(v77))), v7);
		v7 = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(v67)));
		m_inertiaMatrix.m_col0.m_quad = _mm_sub_ps(v20, a1.m_quad);
		eigenVal.m_quad = m_inertiaMatrix.m_col0.m_quad;
		m_inertiaMatrix.m_col1.m_quad =
			_mm_sub_ps(_mm_add_ps(_mm_mul_ps(_mm_unpacklo_ps(_mm_unpacklo_ps(v2, _mm_set_ss(fZ1)), v21), v18.m_quad),
						   v19.m_quad),
				a1.m_quad);
		m_inertiaMatrix.m_col2.m_quad =
			_mm_sub_ps(_mm_add_ps(_mm_mul_ps(_mm_unpacklo_ps(_mm_unpacklo_ps(_mm_set_ss(std::bit_cast<
																				 float>(std::bit_cast<uint32_t>(v76))),
																 _mm_set_ss(std::bit_cast<
																	 float>(std::bit_cast<uint32_t>(v67)))),
												 _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<
																	 float>(std::bit_cast<uint32_t>(v75))),
													 v13)),
									  v18.m_quad),
						   v19.m_quad),
				a1.m_quad);
		hkSimdReal* Determinant = ::new (&result) hkSimdReal(m_inertiaMatrix.getDeterminant());
		m_quad_2 = m_inertiaMatrix.m_col2.m_quad;
		v65 = std::bit_cast<uint32_t>(Determinant->m_real[0]);
		__m128 v23 = _mm_sub_ps(m_inertiaMatrix.m_col1.m_quad, eigenVal.m_quad);
		__m128 v24 = _mm_sub_ps(m_inertiaMatrix.m_col2.m_quad, eigenVal.m_quad);
		v6 = _mm_shuffle_ps(v23, v23, 210);
		__m128 v25 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v24, v24, 210), _mm_shuffle_ps(v23, v23, 201)),
			_mm_mul_ps(_mm_shuffle_ps(v24, v24, 201), v6));
		v2 = _mm_mul_ps(v25, v25);
		v61 = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(v2, v2, 0xAA),
				  _mm_add_ss(_mm_shuffle_ps(v2, v2, 0x55), v2)))) +
			v61;
		m_quad_3 = r.m_quad;
		v3.m_quad = _mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_add_ps(eigenVal.m_quad, m_inertiaMatrix.m_col1.m_quad),
											  m_inertiaMatrix.m_col2.m_quad),
								   _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v65)),
									   _mm_set_ss(std::bit_cast<float>(v65)), 0)),
			r.m_quad);
		r.m_quad = v3.m_quad;
		v68 = *reinterpret_cast<float*>(&v65) + v68;
		m_quad_1[0] = *reinterpret_cast<float*>(&v65) *
				(m_inertiaMatrix.m_col0.m_quad[0] * m_inertiaMatrix.m_col0.m_quad[0] +
					(m_inertiaMatrix.m_col2.m_quad[0] * m_inertiaMatrix.m_col1.m_quad[0] +
						m_inertiaMatrix.m_col1.m_quad[0] * m_inertiaMatrix.m_col0.m_quad[0] +
						m_inertiaMatrix.m_col2.m_quad[0] * m_inertiaMatrix.m_col0.m_quad[0]) +
					m_inertiaMatrix.m_col1.m_quad[0] * m_inertiaMatrix.m_col1.m_quad[0] +
					m_inertiaMatrix.m_col2.m_quad[0] * m_inertiaMatrix.m_col2.m_quad[0]) +
			m_quad_1[0];
		i[0] = (m_inertiaMatrix.m_col2.m_quad[2] * m_inertiaMatrix.m_col1.m_quad[1] +
				   m_inertiaMatrix.m_col1.m_quad[2] * m_inertiaMatrix.m_col0.m_quad[1] +
				   m_inertiaMatrix.m_col0.m_quad[2] * m_inertiaMatrix.m_col2.m_quad[1] +
				   m_inertiaMatrix.m_col2.m_quad[2] * m_inertiaMatrix.m_col0.m_quad[1] +
				   m_inertiaMatrix.m_col0.m_quad[2] * m_inertiaMatrix.m_col1.m_quad[1] +
				   m_inertiaMatrix.m_col2.m_quad[1] * m_inertiaMatrix.m_col1.m_quad[2] +
				   (m_inertiaMatrix.m_col0.m_quad[2] * m_inertiaMatrix.m_col0.m_quad[1] +
					   m_inertiaMatrix.m_col0.m_quad[2] * m_inertiaMatrix.m_col0.m_quad[1]) +
				   (m_inertiaMatrix.m_col1.m_quad[1] * m_inertiaMatrix.m_col1.m_quad[2] +
					   m_inertiaMatrix.m_col1.m_quad[1] * m_inertiaMatrix.m_col1.m_quad[2]) +
				   (m_inertiaMatrix.m_col2.m_quad[1] * m_inertiaMatrix.m_col2.m_quad[2] +
					   m_inertiaMatrix.m_col2.m_quad[1] * m_inertiaMatrix.m_col2.m_quad[2])) *
				*reinterpret_cast<float*>(&v65) +
			i[0];
		m_quad_1[1] = (m_inertiaMatrix.m_col0.m_quad[1] * m_inertiaMatrix.m_col1.m_quad[1] +
						  m_inertiaMatrix.m_col2.m_quad[1] * m_inertiaMatrix.m_col1.m_quad[1] +
						  m_inertiaMatrix.m_col2.m_quad[1] * m_inertiaMatrix.m_col0.m_quad[1] +
						  m_inertiaMatrix.m_col0.m_quad[1] * m_inertiaMatrix.m_col0.m_quad[1] +
						  m_inertiaMatrix.m_col1.m_quad[1] * m_inertiaMatrix.m_col1.m_quad[1] +
						  m_inertiaMatrix.m_col2.m_quad[1] * m_inertiaMatrix.m_col2.m_quad[1]) *
				*reinterpret_cast<float*>(&v65) +
			m_quad_1[1];
		i[1] = (m_inertiaMatrix.m_col0.m_quad[2] * m_inertiaMatrix.m_col1.m_quad[0] +
				   m_inertiaMatrix.m_col1.m_quad[2] * m_inertiaMatrix.m_col2.m_quad[0] +
				   m_inertiaMatrix.m_col2.m_quad[2] * m_inertiaMatrix.m_col0.m_quad[0] +
				   m_inertiaMatrix.m_col0.m_quad[2] * m_inertiaMatrix.m_col2.m_quad[0] +
				   m_inertiaMatrix.m_col1.m_quad[2] * m_inertiaMatrix.m_col0.m_quad[0] +
				   m_inertiaMatrix.m_col2.m_quad[2] * m_inertiaMatrix.m_col1.m_quad[0] +
				   (m_inertiaMatrix.m_col0.m_quad[2] * m_inertiaMatrix.m_col0.m_quad[0] +
					   m_inertiaMatrix.m_col0.m_quad[2] * m_inertiaMatrix.m_col0.m_quad[0]) +
				   (m_inertiaMatrix.m_col1.m_quad[2] * m_inertiaMatrix.m_col1.m_quad[0] +
					   m_inertiaMatrix.m_col1.m_quad[2] * m_inertiaMatrix.m_col1.m_quad[0]) +
				   (m_inertiaMatrix.m_col2.m_quad[2] * m_inertiaMatrix.m_col2.m_quad[0] +
					   m_inertiaMatrix.m_col2.m_quad[2] * m_inertiaMatrix.m_col2.m_quad[0])) *
				*reinterpret_cast<float*>(&v65) +
			i[1];
		m_next = m_next->m_next;
		v13 = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)));
		m_quad_1[2] = (m_inertiaMatrix.m_col1.m_quad[2] * m_inertiaMatrix.m_col2.m_quad[2] +
						  m_inertiaMatrix.m_col0.m_quad[2] * m_inertiaMatrix.m_col1.m_quad[2] +
						  m_inertiaMatrix.m_col2.m_quad[2] * m_inertiaMatrix.m_col0.m_quad[2] +
						  m_inertiaMatrix.m_col0.m_quad[2] * m_inertiaMatrix.m_col0.m_quad[2] +
						  m_inertiaMatrix.m_col1.m_quad[2] * m_inertiaMatrix.m_col1.m_quad[2] +
						  m_inertiaMatrix.m_col2.m_quad[2] * m_inertiaMatrix.m_col2.m_quad[2]) *
				*reinterpret_cast<float*>(&v65) +
			m_quad_1[2];
	}
	double v26 = v68;
	*reinterpret_cast<float*>(&v52) = 1.0 / (4.0 * v68);
	__m128 m_quad_4 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v52)),
												_mm_set_ss(std::bit_cast<float>(v52)), 0),
									 r.m_quad),
		a1.m_quad);
	r.m_quad = m_quad_4;
	m_surface = v61 * 0.5;
	m_localTransform.m_translation.m_quad = m_quad_4;
	float m_volume = v26 / 6.0;
	this->m_volume = m_volume;
	if (!(m_volume > 1.1920929e-07f)) {
		hkVector4 v34;
		m_localTransform.m_rotation.set(hkQuaternionIdentity);
		this->m_volume = 0.0f;
		this->m_inertiaMatrix.m_col0.m_quad = hkVector4Zero.m_quad;
		this->m_inertiaMatrix.m_col1.m_quad = hkVector4Zero.m_quad;
		this->m_inertiaMatrix.m_col2.m_quad = hkVector4Zero.m_quad;
		v34.m_quad = hkVector4Zero.m_quad;
		m_hasMassProperties.m_bool = 1;
		bool v35 = m_hasMassProperties.m_bool == 0;
		m_inertia.m_quad = v34.m_quad;
		return static_cast<hkResult>(v35);
	} else {
		hkVector4 v30;
		hkMatrix3 arB;
		hkMatrix3 arA_;
		hkMatrix3 arA;
		hkMatrix3 v82;
		v7[0] = v13[0];
		*reinterpret_cast<float*>(&v54) = 1.0f / (v26 * 10.0);
		m_quad_1 =
			_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v54)), _mm_set_ss(std::bit_cast<float>(v54)), 0),
				m_quad_1);
		*reinterpret_cast<float*>(&v55) = 1.0f / (v26 * 20.0);
		__m128 i_1 = i;
		i = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v55)), _mm_set_ss(std::bit_cast<float>(v55)), 0),
			i);
		const __m128 negZ = _mm_sub_ss(_mm_set_ss(-0.0f), _mm_set_ss(i[2]));
		const __m128 negY = _mm_sub_ss(_mm_set_ss(-0.0f), _mm_set_ss(i[1]));
		const __m128 negX = _mm_sub_ss(_mm_set_ss(-0.0f), _mm_set_ss(i[0]));
		i_1[0] = negZ[0];
		*reinterpret_cast<float*>(&v56) = m_quad_1[2] + m_quad_1[1];
		m_quad_2[0] = negZ[0];
		v6[0] = negY[0];
		r.m_quad = _mm_sub_ps(m_quad_4, a1.m_quad);
		*reinterpret_cast<float*>(&v58) = m_quad_1[2] + m_quad_1[0];
		v67 = m_quad_1[0] + m_quad_1[1];
		m_inertiaMatrix.m_col0.m_quad =
			_mm_unpacklo_ps(_mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v56)), v6), _mm_unpacklo_ps(m_quad_2, v7));
		__m128 v29 = negX;
		v6[0] = negX[0];
		v30.m_quad =
			_mm_unpacklo_ps(_mm_unpacklo_ps(i_1, v6), _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v58)), v7));
		m_inertiaMatrix.m_col1.m_quad = v30.m_quad;
		v30.m_quad[0] = negY[0];
		m_inertiaMatrix.m_col2.m_quad =
			_mm_unpacklo_ps(_mm_unpacklo_ps(v30.m_quad, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(v67)))),
				_mm_unpacklo_ps(v29, v13));
		arA.setCrossSkewSymmetric(r);
		arB.setCrossSkewSymmetric(hkVector4Zero);
		v82.setMul(arA, arA);
		arA_.setMul(arB, arB);
		arA_.transpose();
		v82.sub(arA_);
		a1.m_quad = _mm_set_ss(1.0f);
		m_inertiaMatrix.addMul(&a1.m_quad, &v82.m_col0.m_quad);
		if (m_inertiaMatrix.diagonalizeSymmetric(eigenVec, eigenVal, 128, 0.0001f) == HK_FAILURE) {
			alignas(16) char buf[512];
			hkErrStream apThis_(buf, 512);
			uint32_t m_uid = this->m_uid;
			hkOstream* v31 = &apThis_.operator<<("Failed to diagonalize inertia matrix (");
			hkOstream* v32 = &v31->operator<<(m_uid);
			v32->operator<<(")");
			if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x45465, buf,
					".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 1753)) {
				__builtin_debugtrap();
			}
		}
		eigenVec.transpose();
		eigenVec.renormalize();
		this->m_inertiaMatrix = m_inertiaMatrix;
		m_inertiaMatrix.changeBasis(eigenVec);
		m_inertia.m_quad = _mm_unpacklo_ps(_mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<
															   uint32_t>(m_inertiaMatrix.m_col0.m_quad[0]))),
											   _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_inertiaMatrix
													   .m_col2.m_quad[2])))),
			_mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(m_inertiaMatrix.m_col1.m_quad[1]))),
				_mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))));
		eigenVec.transpose();
		m_localTransform.m_rotation.m_col0 = eigenVec.m_col0;
		m_localTransform.m_rotation.m_col1 = eigenVec.m_col1;
		m_localTransform.m_rotation.m_col2 = eigenVec.m_col2;
		m_hasMassProperties.m_bool = 1;
		return static_cast<hkResult>(m_hasMassProperties.m_bool == 0);
	}
}

// Builds the hull's face planes once (m_hasIndexing): numbers the triangles and the vertices and fills m_planes by
// m_dimensions. In 3D each triangle gets its plane, and the triangles, largest first, each seed a plane of m_planes
// that floods over the neighbours whose normal is not normalised or within m_config.m_minCosAngle of the seed's; a
// planar hull gets one plane per boundary edge, facing away from the vertices' centroid; a segment gets its two end
// planes. With m_ensurePlaneEnclosing each plane is then pushed out past every vertex in front of it.
void hkgpConvexHullImpl::buildIndices() {
	if (m_hasIndexing.m_bool) {
		return;
	}
	using MapBase = hkUlongMapBase;
	// The SDK's hkArray<SortedTriangle, hkContainerTempAllocator>: each triangle with twice its area.
	using sorted_t = hkArray<SortedTriangle, hkContainerTempAllocator>;
	FNVD_INLINED_LOCAL(sorted_t, sorted, std::nullopt, std::nullopt);
	sorted.m_data = nullptr;
	sorted.m_size = 0;
	sorted.m_capacityAndFlags = hkArrayBase<SortedTriangle>::DONT_DEALLOCATE_FLAG;
	// Maps each triangle to its index in trianglePlanes.
	using triangleIndices_t = hkPointerMap<Triangle*, int32_t>;
	FNVD_INLINED_LOCAL(triangleIndices_t, triangleIndices, std::nullopt, std::nullopt);
	MapBase& map = triangleIndices.m_map;
	map.m_elem = nullptr;
	map.m_numElems = 0;
	map.m_hashMod = -1;
	map.reserve(hkMemoryRouter::getInstance().heap(), 0);
	const int32_t numTriangles = m_mesh.m_triangles.m_numUsed;
	// Each triangle's plane, by its index.
	using trianglePlanes_t = hkArray<hkVector4, hkContainerTempAllocator>;
	FNVD_INLINED_LOCAL(trianglePlanes_t, trianglePlanes, std::nullopt, std::nullopt);
	trianglePlanes.m_data = nullptr;
	trianglePlanes.m_size = 0;
	trianglePlanes.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
	if (numTriangles != 0) {
		// EXE-INLINED none: hkArray<SortedTriangle, hkContainerTempAllocator>::reserve
		hkMemoryAllocator& sortedTemp = hkMemoryRouter::getInstance().temp();
		if (sorted.getCapacity() < numTriangles) {
			hkArrayUtil::_reserve(sortedTemp, &sorted, numTriangles, sizeof(SortedTriangle));
		}
		// The comparator's stack slot below: the bits of the last triangle's key.
		uint32_t uiLessSlot;
		for (Triangle* t = m_mesh.m_triangles.m_used; t; t = t->m_next) {
			// EXE-INLINED none: hkArray<SortedTriangle, hkContainerTempAllocator>::expandOneUnchecked
			SortedTriangle& st = sorted.m_data[sorted.m_size++];
			st.m_data = t;
			const Vertex* v0 = t->m_vertices[0];
			const Vertex* v1 = t->m_vertices[1];
			const Vertex* v2 = t->m_vertices[2];
			hkVector4 x0;
			hkVector4 x1;
			hkVector4 x2;
			x0.m_quad = _mm_add_ps(_mm_mul_ps(_mm_setr_ps(static_cast<float>(v0->m_x[0]),
												  static_cast<float>(v0->m_x[1]), static_cast<float>(v0->m_x[2]), 0.0f),
									   m_scaleInv.m_quad),
				m_origin.m_quad);
			x1.m_quad = _mm_add_ps(_mm_mul_ps(_mm_setr_ps(static_cast<float>(v1->m_x[0]),
												  static_cast<float>(v1->m_x[1]), static_cast<float>(v1->m_x[2]), 0.0f),
									   m_scaleInv.m_quad),
				m_origin.m_quad);
			x2.m_quad = _mm_add_ps(_mm_mul_ps(_mm_setr_ps(static_cast<float>(v2->m_x[0]),
												  static_cast<float>(v2->m_x[1]), static_cast<float>(v2->m_x[2]), 0.0f),
									   m_scaleInv.m_quad),
				m_origin.m_quad);
			hkVector4 e1;
			hkVector4 e2;
			hkVector4 normal;
			e1.setSub4(x1, x0);
			e2.setSub4(x2, x0);
			normal.setCross(e1, e2);
			const __m128 sq = _mm_mul_ps(normal.m_quad, normal.m_quad);
			const float fLength = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA),
				_mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))));
			st.m_value = fLength;
			uiLessSlot = std::bit_cast<uint32_t>(fLength);
		}
		if (sorted.m_size > 1) {
			// The empty less<SortedTriangle> is passed as a 32-bit value with its low byte cleared, so its upper three
			// bytes are those of the last triangle's key float, as the game does.
			hkAlgorithm::quickSortRecursive(sorted.m_data, 0, sorted.m_size - 1,
				static_cast<int32_t>(uiLessSlot & 0xFFFFFF00u));
		}
		map.reserve(hkMemoryRouter::getInstance().heap(), m_mesh.m_triangles.m_numUsed);
		// EXE-INLINED none: hkArray<hkVector4, hkContainerTempAllocator>::setSize
		const int32_t numPlanes = m_mesh.m_triangles.m_numUsed;
		hkMemoryAllocator& planesTemp = hkMemoryRouter::getInstance().temp();
		if (trianglePlanes.getCapacity() < numPlanes) {
			int32_t newCapacity = 2 * trianglePlanes.getCapacity();
			if (numPlanes >= newCapacity) {
				newCapacity = numPlanes;
			}
			hkArrayUtil::_reserve(planesTemp, &trianglePlanes, newCapacity, sizeof(hkVector4));
		}
		trianglePlanes.m_size = numPlanes;
		int32_t index = 0;
		for (Triangle* t = m_mesh.m_triangles.m_used; t; t = t->m_next) {
			triangleIndices.insert(t, index);
			const Vertex* v0 = t->m_vertices[0];
			const Vertex* v1 = t->m_vertices[1];
			const Vertex* v2 = t->m_vertices[2];
			hkVector4 x0;
			hkVector4 x1;
			hkVector4 x2;
			x0.m_quad = _mm_add_ps(_mm_mul_ps(_mm_setr_ps(static_cast<float>(v0->m_x[0]),
												  static_cast<float>(v0->m_x[1]), static_cast<float>(v0->m_x[2]), 0.0f),
									   m_scaleInv.m_quad),
				m_origin.m_quad);
			x1.m_quad = _mm_add_ps(_mm_mul_ps(_mm_setr_ps(static_cast<float>(v1->m_x[0]),
												  static_cast<float>(v1->m_x[1]), static_cast<float>(v1->m_x[2]), 0.0f),
									   m_scaleInv.m_quad),
				m_origin.m_quad);
			x2.m_quad = _mm_add_ps(_mm_mul_ps(_mm_setr_ps(static_cast<float>(v2->m_x[0]),
												  static_cast<float>(v2->m_x[1]), static_cast<float>(v2->m_x[2]), 0.0f),
									   m_scaleInv.m_quad),
				m_origin.m_quad);
			hkVector4 e1;
			hkVector4 e2;
			hkVector4 plane;
			e1.setSub4(x1, x0);
			e2.setSub4(x2, x0);
			plane.setCross(e1, e2);
			const __m128 sq = _mm_mul_ps(plane.m_quad, plane.m_quad);
			const float fLengthSquared =
				_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
			if (fLengthSquared != 0.0f) {
				const float fLength = static_cast<float>(std::sqrt(fLengthSquared));
				const float fInverse = 1.0f / fLength;
				plane.m_quad = _mm_mul_ps(_mm_set1_ps(fInverse), plane.m_quad);
			}
			const __m128 d = _mm_mul_ps(plane.m_quad, x0.m_quad);
			const float fDot =
				_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(d, d, 0xAA), _mm_add_ss(_mm_shuffle_ps(d, d, 0x55), d)));
			plane(3) = 0.0f - fDot;
			trianglePlanes.m_data[index] = plane;
			t->m_index = index;
			t->m_plane.m_index = -1;
			index++;
		}
	}
	int32_t vertexIndex = 0;
	for (Vertex* v = m_mesh.m_vertices.m_used; v; v = v->m_next) {
		v->m_index = vertexIndex++;
	}
	// EXE-INLINED none: hkArray<hkVector4>::clear
	m_planes.m_size = 0;
	switch (m_dimensions) {
	case 3: {
		// The SDK's hkArray<Edge, hkContainerTempAllocator>: the edges still to cross while flooding a plane.
		using edges_t = hkArray<Edge, hkContainerTempAllocator>;
		FNVD_INLINED_LOCAL(edges_t, edges, std::nullopt, std::nullopt);
		edges.m_data = nullptr;
		edges.m_size = 0;
		edges.m_capacityAndFlags = hkArrayBase<Edge>::DONT_DEALLOCATE_FLAG;
		for (int32_t i = 0; i < sorted.m_size; ++i) {
			Triangle* seed = sorted.m_data[i].m_data;
			if (seed->m_plane.m_index != -1) {
				continue;
			}
			const hkVector4* pSeedPlane = &trianglePlanes.m_data[triangleIndices.getWithDefault(seed, -1)];
			seed->m_plane.m_index = m_planes.m_size;
			// EXE-INLINED none: hkArray<hkVector4>::pushBack
			hkMemoryAllocator& planesHeap = hkMemoryRouter::getInstance().heap();
			if (m_planes.m_size == m_planes.getCapacity()) {
				hkArrayUtil::_reserveMore(planesHeap, &m_planes, sizeof(hkVector4));
			}
			m_planes.m_data[m_planes.m_size] = *pSeedPlane;
			m_planes.m_size++;
			// EXE-INLINED none: hkArray<Edge, hkContainerTempAllocator>::clear
			edges.m_size = 0;
			for (int32_t k = 0; k < 3; ++k) {
				const uintptr_t link = seed->m_links[k];
				// EXE-INLINED none: hkArray<Edge, hkContainerTempAllocator>::pushBack
				hkMemoryAllocator& edgesTemp = hkMemoryRouter::getInstance().temp();
				if (edges.m_size == edges.getCapacity()) {
					hkArrayUtil::_reserveMore(edgesTemp, &edges, sizeof(Edge));
				}
				// Placement new, which tests the address. A link packs the neighbour's address with the shared edge's
				// index in its low two bits (hkgpTopology's Uid), an integer that is masked apart.
				Edge* pEdge = edges.m_data + edges.m_size;
				if (pEdge) {
					pEdge->m_triangle = reinterpret_cast<Triangle*>(link & ~static_cast<uintptr_t>(3));
					pEdge->m_index = static_cast<uint32_t>(link & 3);
				}
				edges.m_size++;
			}
			while (edges.m_size != 0) {
				const Edge edge = edges.m_data[edges.m_size - 1];
				edges.m_size--;
				Triangle* t = edge.m_triangle;
				if (t->m_plane.m_index != -1) {
					continue;
				}
				const hkVector4* pPlane = &trianglePlanes.m_data[triangleIndices.getWithDefault(t, -1)];
				bool bMerge = true;
				if (pPlane->isNormalized3(0.0001f)) {
					const __m128 c = _mm_mul_ps(pPlane->m_quad, pSeedPlane->m_quad);
					const float fCos = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(c, c, 0xAA),
						_mm_add_ss(_mm_shuffle_ps(c, c, 0x55), c)));
					bMerge = m_config.m_minCosAngle <= fCos;
				}
				if (!bMerge) {
					continue;
				}
				t->m_plane.m_index = seed->m_plane.m_index;
				// The other two edges of t: hkgpTopology's next ((9 >> 2i) & 3) and prev ((0x12 >> 2i) & 3).
				const int32_t aiOther[2] = {(9 >> (2 * static_cast<int32_t>(edge.m_index))) & 3,
					(0x12 >> (2 * static_cast<int32_t>(edge.m_index))) & 3};
				for (int32_t k = 0; k < 2; ++k) {
					const uintptr_t link = t->m_links[aiOther[k]];
					// EXE-INLINED none: hkArray<Edge, hkContainerTempAllocator>::pushBack
					hkMemoryAllocator& edgesTemp = hkMemoryRouter::getInstance().temp();
					if (edges.m_size == edges.getCapacity()) {
						hkArrayUtil::_reserveMore(edgesTemp, &edges, sizeof(Edge));
					}
					// The SDK's placement new, which tests the address; the link decoded as above.
					Edge* pEdge = edges.m_data + edges.m_size;
					if (pEdge) {
						pEdge->m_triangle = reinterpret_cast<Triangle*>(link & ~static_cast<uintptr_t>(3));
						pEdge->m_index = static_cast<uint32_t>(link & 3);
					}
					edges.m_size++;
				}
			}
		}
		// EXE-INLINED none: hkArray<Edge, hkContainerTempAllocator>::~hkArray
		hkMemoryAllocator& edgesTemp = hkMemoryRouter::getInstance().temp();
		edges.m_size = 0;
		if (edges.m_capacityAndFlags >= 0) {
			edgesTemp.bufFree(edges.m_data, static_cast<int32_t>(sizeof(Edge)) * edges.getCapacity());
		}
		break;
	}
	case 2: {
		const hkVector4 centroid = getCentroid<PositionAsTransform>();
		Vertex* pVertex = m_mesh.m_vertices.m_used;
		// The boundary edges run from each vertex's predecessor, the last vertex's for the first one.
		Vertex* pPrevious = pVertex;
		if (pPrevious) {
			while (pPrevious->m_next) {
				pPrevious = pPrevious->m_next;
			}
		}
		for (; pVertex; pPrevious = pVertex, pVertex = pVertex->m_next) {
			hkVector4 x0;
			hkVector4 x1;
			x0.m_quad = _mm_add_ps(_mm_mul_ps(_mm_setr_ps(static_cast<float>(pPrevious->m_x[0]),
												  static_cast<float>(pPrevious->m_x[1]),
												  static_cast<float>(pPrevious->m_x[2]), 0.0f),
									   m_scaleInv.m_quad),
				m_origin.m_quad);
			x1.m_quad = _mm_add_ps(_mm_mul_ps(_mm_setr_ps(static_cast<float>(pVertex->m_x[0]),
												  static_cast<float>(pVertex->m_x[1]),
												  static_cast<float>(pVertex->m_x[2]), 0.0f),
									   m_scaleInv.m_quad),
				m_origin.m_quad);
			hkVector4 edge;
			edge.setSub4(x1, x0);
			// EXE-INLINED none: hkArray<hkVector4>::expandOne
			hkMemoryAllocator& planesHeap = hkMemoryRouter::getInstance().heap();
			if (m_planes.m_size == m_planes.getCapacity()) {
				hkArrayUtil::_reserveMore(planesHeap, &m_planes, sizeof(hkVector4));
			}
			const int32_t planeIndex = m_planes.m_size;
			m_planes.m_size = planeIndex + 1;
			hkVector4* pPlane = &m_planes.m_data[planeIndex];
			hkVector4 normal;
			normal.setCross(m_projectionPlane, edge);
			*pPlane = normal;
			const __m128 sq = _mm_mul_ps(normal.m_quad, normal.m_quad);
			const float fLengthSquared =
				_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
			if (fLengthSquared != 0.0f) {
				const float fLength = static_cast<float>(std::sqrt(fLengthSquared));
				const float fInverse = 1.0f / fLength;
				pPlane->m_quad = _mm_mul_ps(_mm_set1_ps(fInverse), normal.m_quad);
			}
			const __m128 d = _mm_mul_ps(x0.m_quad, pPlane->m_quad);
			const float fDot =
				_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(d, d, 0xAA), _mm_add_ss(_mm_shuffle_ps(d, d, 0x55), d)));
			(*pPlane)(3) = 0.0f - fDot;
			const __m128 c = _mm_mul_ps(pPlane->m_quad, centroid.m_quad);
			const float fDistance = _mm_cvtss_f32(_mm_add_ss(_mm_add_ss(_mm_shuffle_ps(c, c, 0xAA),
																 _mm_add_ss(_mm_shuffle_ps(c, c, 0x55), c)),
				_mm_shuffle_ps(pPlane->m_quad, pPlane->m_quad, 0xFF)));
			if (0.0f < fDistance) {
				pPlane->setNeg4(*pPlane);
			}
		}
		break;
	}
	case 1: {
		// Reads the first vertex before it tests the list, as the game does.
		Vertex* pFirst = m_mesh.m_vertices.m_used;
		hkVector4 x0;
		x0.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_setr_ps(static_cast<float>(pFirst->m_x[0]), static_cast<float>(pFirst->m_x[1]),
									  static_cast<float>(pFirst->m_x[2]), 0.0f),
						   m_scaleInv.m_quad),
				m_origin.m_quad);
		Vertex* pLast = pFirst;
		if (pLast) {
			while (pLast->m_next) {
				pLast = pLast->m_next;
			}
		}
		hkVector4 x1;
		x1.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_setr_ps(static_cast<float>(pLast->m_x[0]), static_cast<float>(pLast->m_x[1]),
									  static_cast<float>(pLast->m_x[2]), 0.0f),
						   m_scaleInv.m_quad),
				m_origin.m_quad);
		hkVector4 axis;
		axis.setSub4(x1, x0);
		// EXE-INLINED none: hkVector4::normalize3 (rsqrtss and one Newton step, 0 for a zero length)
		const __m128 sq = _mm_mul_ps(axis.m_quad, axis.m_quad);
		const __m128 lengthSquared =
			_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq));
		const __m128 approx = _mm_rsqrt_ss(lengthSquared);
		const __m128 refined = _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), approx),
			_mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(lengthSquared, approx), approx)));
		const __m128 inverse = _mm_andnot_ps(_mm_cmpeq_ss(lengthSquared, _mm_setzero_ps()), refined);
		hkVector4 plane;
		plane.m_quad = _mm_mul_ps(_mm_shuffle_ps(inverse, inverse, 0), axis.m_quad);
		const __m128 d1 = _mm_mul_ps(x1.m_quad, plane.m_quad);
		const float fDot1 =
			_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(d1, d1, 0xAA), _mm_add_ss(_mm_shuffle_ps(d1, d1, 0x55), d1)));
		plane(3) = 0.0f - fDot1;
		// EXE-INLINED none: hkArray<hkVector4>::pushBack
		hkMemoryAllocator& planesHeap = hkMemoryRouter::getInstance().heap();
		if (m_planes.m_size == m_planes.getCapacity()) {
			hkArrayUtil::_reserveMore(planesHeap, &m_planes, sizeof(hkVector4));
		}
		m_planes.m_data[m_planes.m_size] = plane;
		m_planes.m_size += 1;
		// The opposite plane through the first vertex.
		plane.m_quad = _mm_mul_ps(_mm_set1_ps(-1.0f), plane.m_quad);
		const __m128 d0 = _mm_mul_ps(x0.m_quad, plane.m_quad);
		const float fDot0 =
			_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(d0, d0, 0xAA), _mm_add_ss(_mm_shuffle_ps(d0, d0, 0x55), d0)));
		plane(3) = 0.0f - fDot0;
		// EXE-INLINED none: hkArray<hkVector4>::pushBack
		hkMemoryAllocator& planesHeap2 = hkMemoryRouter::getInstance().heap();
		if (m_planes.m_size == m_planes.getCapacity()) {
			hkArrayUtil::_reserveMore(planesHeap2, &m_planes, sizeof(hkVector4));
		}
		m_planes.m_data[m_planes.m_size] = plane;
		m_planes.m_size += 1;
		break;
	}
	}
	if (m_config.m_ensurePlaneEnclosing.m_bool) {
		for (Vertex* v = m_mesh.m_vertices.m_used; v; v = v->m_next) {
			for (int32_t i = 0; i < m_planes.m_size; ++i) {
				hkVector4& plane = m_planes.m_data[i];
				const __m128 d = _mm_mul_ps(plane.m_quad, v->m_source.m_quad);
				const float fDistance = _mm_cvtss_f32(_mm_add_ss(_mm_add_ss(_mm_shuffle_ps(d, d, 0xAA),
																	 _mm_add_ss(_mm_shuffle_ps(d, d, 0x55), d)),
					_mm_shuffle_ps(plane.m_quad, plane.m_quad, 0xFF)));
				if (fDistance > 0.0f) {
					plane(3) = plane(3) - fDistance;
				}
			}
		}
	}
	if (m_planes.m_size != 0) {
		m_planes.optimizeCapacity(0, 1);
	}
	m_hasIndexing = true;
	// EXE-INLINED none: hkArray<hkVector4, hkContainerTempAllocator>::clearAndDeallocate
	hkMemoryAllocator& planesTemp = hkMemoryRouter::getInstance().temp();
	trianglePlanes.m_size = 0;
	if (trianglePlanes.m_capacityAndFlags >= 0) {
		planesTemp.bufFree(trianglePlanes.m_data,
			static_cast<int32_t>(sizeof(hkVector4)) * trianglePlanes.getCapacity());
	}
	trianglePlanes.m_data = nullptr;
	trianglePlanes.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
	map.clearAndDeallocate(hkMemoryRouter::getInstance().heap());
	map.destructOperations();
	// EXE-INLINED none: hkArray<SortedTriangle, hkContainerTempAllocator>::~hkArray
	hkMemoryAllocator& sortedTemp = hkMemoryRouter::getInstance().temp();
	sorted.m_size = 0;
	if (sorted.m_capacityAndFlags >= 0) {
		sortedTemp.bufFree(sorted.m_data, static_cast<int32_t>(sizeof(SortedTriangle)) * sorted.getCapacity());
	}
}

// Appends the bevel planes of a 3D hull: for each edge between two triangles on different planes of m_planes whose
// normals' dot product is below maxCosAngle, the normalised sum of the two planes through the first vertex (with
// m_ensurePlaneEnclosing, moved out to the farthest vertex), facing outwards. An edge is taken from the side whose end
// vertex does not order below its start vertex, or from either side when it has no neighbour.
void hkgpConvexHullImpl::fetchBevelPlanes(float maxCosAngle,
	hkArray<hkVector4, hkContainerHeapAllocator>& bevelPlanesOut) {
	if (!m_hasIndexing.m_bool) {
		alignas(16) char acBuffer[512];
		alignas(16) hkErrStream kStream(acBuffer, 512);
		const uint32_t uiUid = m_uid;
		hkOstream* pStream = &kStream.operator<<("No index available (");
		pStream = &pStream->operator<<(uiUid);
		pStream->operator<<(")");
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x79F9D886, acBuffer,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 328)) {
			__builtin_debugtrap();
		}
	}
	if (m_dimensions != 3) {
		return;
	}
	for (Triangle* t = m_mesh.m_triangles.m_used; t; t = t->m_next) {
		for (int32_t i = 0; i < 3; ++i) {
			Triangle* pLinked = reinterpret_cast<Triangle*>(t->m_links[i] & ~static_cast<uintptr_t>(3));
			const Vertex* pStart = t->m_vertices[i];
			const Vertex* pEnd = t->m_vertices[(9 >> (2 * i)) & 3];
			if (*pEnd < *pStart && pLinked) {
				continue;
			}
			const int32_t iPlane = t->m_plane.m_index;
			if (iPlane == pLinked->m_plane.m_index) {
				continue;
			}
			const __m128 p0 = m_planes.m_data[iPlane].m_quad;
			const __m128 p1 = m_planes.m_data[pLinked->m_plane.m_index].m_quad;
			const __m128 c = _mm_mul_ps(p0, p1);
			const float fCos =
				_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(c, c, 0xAA), _mm_add_ss(_mm_shuffle_ps(c, c, 0x55), c)));
			if (!(maxCosAngle > fCos)) {
				continue;
			}
			hkVector4 plane;
			plane.m_quad = _mm_add_ps(p0, p1);
			const __m128 sq = _mm_mul_ps(plane.m_quad, plane.m_quad);
			const float fLengthSquared =
				_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
			if (fLengthSquared == 0.0f) {
				continue;
			}
			const float fLength = static_cast<float>(std::sqrt(fLengthSquared));
			const float fInverse = 1.0f / fLength;
			const Vertex* pVertex = m_mesh.m_vertices.m_used;
			const __m128 x0 = _mm_add_ps(_mm_mul_ps(_mm_setr_ps(static_cast<float>(pVertex->m_x[0]),
														static_cast<float>(pVertex->m_x[1]),
														static_cast<float>(pVertex->m_x[2]), 0.0f),
											 m_scaleInv.m_quad),
				m_origin.m_quad);
			plane.m_quad = _mm_mul_ps(_mm_set1_ps(fInverse), plane.m_quad);
			const __m128 d0 = _mm_mul_ps(x0, plane.m_quad);
			plane(3) =
				_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(d0, d0, 0xAA), _mm_add_ss(_mm_shuffle_ps(d0, d0, 0x55), d0)));
			if (m_config.m_ensurePlaneEnclosing.m_bool) {
				for (const Vertex* v = pVertex->m_next; v; v = v->m_next) {
					const __m128 x =
						_mm_add_ps(_mm_mul_ps(_mm_setr_ps(static_cast<float>(v->m_x[0]), static_cast<float>(v->m_x[1]),
												  static_cast<float>(v->m_x[2]), 0.0f),
									   m_scaleInv.m_quad),
							m_origin.m_quad);
					const __m128 d = _mm_mul_ps(x, plane.m_quad);
					const float fDistance = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(d, d, 0xAA),
						_mm_add_ss(_mm_shuffle_ps(d, d, 0x55), d)));
					// Keeps w only when the distance is below it: an unordered compare takes the distance.
					const float fW = plane(3);
					plane(3) = fDistance < fW ? fW : fDistance;
				}
			}
			plane(3) = -plane(3);
			// EXE-INLINED none: hkArray<hkVector4>::pushBack
			hkMemoryRouter& router = hkMemoryRouter::getInstance();
			if (bevelPlanesOut.m_size == (bevelPlanesOut.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*router.m_heap, &bevelPlanesOut, sizeof(hkVector4));
			}
			bevelPlanesOut.m_data[bevelPlanesOut.m_size] = plane;
			bevelPlanesOut.m_size++;
		}
	}
}

// Below three dimensions the oriented rectangle in the projection plane. Otherwise, for each plane of the hull: its
// extent along the plane's normal (getSpan) and, where that is below the smallest so far, the oriented rectangle
// in the plane; the box whose volume (the rectangle's area result times that extent) is the smallest wins. The
// extent is the box's z half extent once halved, and the translation is moved back along z by it.
void hkgpConvexHullImpl::getOrientedBoundingBox(hkVector4& halfExtents, hkTransform& worldTransform) {
	if (m_dimensions >= 3) {
		if (!m_hasIndexing.m_bool) {
			alignas(16) char buf[512];
			hkErrStream apThis(buf, 512);
			apThis << "No index available (" << m_uid << ")";
			// HK_ERROR's id.
			if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x79F9D886, buf,
					".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 328)) {
				__builtin_debugtrap();
			}
		}
		float fBestExtent = 3.40282e+38f;
		float fBestVolume = 3.40282e+38f;
		for (int32_t i = 0; i < m_planes.m_size; ++i) {
			// The extent's minimum and maximum are adjacent floats.
			float fMinMax[2];
			const float fExtent = getSpan(m_planes.m_data[i], &fMinMax[0], &fMinMax[1]);
			if (fBestExtent > fExtent) {
				hkVector4 rectHalfExtents;
				hkTransform rectTransform;
				const float fArea = getOrientedRectangle(m_planes.m_data[i], rectHalfExtents, rectTransform);
				const float fVolume = fArea * (fMinMax[1] - fMinMax[0]);
				if (fBestVolume > fVolume) {
					fBestExtent = fExtent;
					halfExtents.m_quad = rectHalfExtents.m_quad;
					fBestVolume = fVolume;
					halfExtents.m_quad[2] = fExtent;
					worldTransform.m_rotation.m_col0.m_quad = rectTransform.m_rotation.m_col0.m_quad;
					worldTransform.m_rotation.m_col1.m_quad = rectTransform.m_rotation.m_col1.m_quad;
					worldTransform.m_rotation.m_col2.m_quad = rectTransform.m_rotation.m_col2.m_quad;
					worldTransform.m_translation.m_quad = rectTransform.m_translation.m_quad;
				}
			}
		}
		halfExtents.m_quad[2] = halfExtents.m_quad[2] * 0.5;
		// translation.addMul4(-halfExtents(2), column 2): the broadcast float times the column, plus the translation.
		const float fOffset = -halfExtents.m_quad[2];
		worldTransform.m_translation.m_quad =
			_mm_add_ps(_mm_mul_ps(_mm_set1_ps(fOffset), worldTransform.m_rotation.m_col2.m_quad),
				worldTransform.m_translation.m_quad);
	} else {
		getOrientedRectangle(m_projectionPlane, halfExtents, worldTransform);
	}
}

// The vertices' distances to the plane go to their scratch slots; a hull on one side (or within margin of the plane) is
// not split. Otherwise each output asked for is a new hull built from the vertices on its side, those on the plane and
// the points where the hull's edges cross the plane (each (1 - t) a + t b, t = s(a) / (s(a) - s(b)) in doubles), and
// deleted when its dimension differs from this one's. - 2 dimensions: a vertex within margin is on the plane (scratch
// 0); the polygon's edges are its consecutive vertices (the last to the first), and the new hulls get the projection
// plane, built planar (maxp 1). - 3 dimensions: classify counts the sides and the distances' range; the split needs a
// range wider than margin whose nearest end is farther than margin, else the side of the range's middle is returned.
// The crossing edges are the triangles' edges from a vertex above.
hkgpConvexHull::Side hkgpConvexHullImpl::splitByPlane(const hkVector4& plane, hkgpConvexHull** pos,
	hkgpConvexHull** neg, float thickness) {
	using PointArray = hkArray<hkVector4, hkContainerHeapAllocator>;
	if (pos) {
		*pos = nullptr;
	}
	if (neg) {
		*neg = nullptr;
	}
	if (m_dimensions == 2) {
		int32_t iNumBelow = 0;
		int32_t iNumAbove = 0;
		Vertex* pVertex = m_mesh.m_vertices.m_used;
		if (!pVertex) {
			return hkgpConvexHull::ZERO;
		}
		do {
			const hkQuadReal prod = _mm_mul_ps(pVertex->m_source.m_quad, plane.m_quad);
			const float fDistance = _mm_cvtss_f32(_mm_add_ps(_mm_add_ss(_mm_shuffle_ps(prod, prod, 0xAA),
																 _mm_shuffle_ps(plane.m_quad, plane.m_quad, 0xFF)),
				_mm_add_ss(_mm_shuffle_ps(prod, prod, 0x55), prod)));
			pVertex->m_scratch = fDistance;
			const float fMagnitude = __builtin_fabsf(fDistance);
			if (fMagnitude <= thickness) {
				pVertex->m_scratch = 0.0f;
			} else if (fDistance < 0.0f) {
				iNumBelow++;
			} else {
				iNumAbove++;
			}
			pVertex = pVertex->m_next;
		} while (pVertex);
		if (!iNumBelow) {
			return static_cast<hkgpConvexHull::Side>(iNumAbove != 0);
		}
		if (!iNumAbove) {
			return hkgpConvexHull::NEGATIVE;
		}
		// EXE-INLINED none: hkArray<hkVector4>::hkArray
		// The exe's below array is 16-aligned ([esp+0x40]), its above array not ([esp+0x4C]).
		alignas(16) FNVD_INLINED_LOCAL(PointArray, belowPoints, std::nullopt, std::nullopt);
		belowPoints.m_data = nullptr;
		belowPoints.m_size = 0;
		belowPoints.m_capacityAndFlags = PointArray::DONT_DEALLOCATE_FLAG;
		FNVD_INLINED_LOCAL(PointArray, abovePoints, std::nullopt, std::nullopt);
		abovePoints.m_data = nullptr;
		abovePoints.m_size = 0;
		abovePoints.m_capacityAndFlags = PointArray::DONT_DEALLOCATE_FLAG;
		Vertex* pPrev = m_mesh.m_vertices.m_used;
		if (pPrev) {
			while (pPrev->m_next) {
				pPrev = pPrev->m_next;
			}
		}
		for (pVertex = m_mesh.m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
			if (static_cast<double>(pVertex->m_scratch) * pPrev->m_scratch < 0.0) {
				const float fT = static_cast<float>(static_cast<double>(pPrev->m_scratch) /
					(static_cast<double>(pPrev->m_scratch) - pVertex->m_scratch));
				// EXE-INLINED 0x5DBF70: hkVector4::setInterpolate4
				const hkQuadReal t = _mm_shuffle_ps(_mm_set_ss(fT), _mm_set_ss(fT), 0);
				hkVector4 kCut;
				kCut.m_quad = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), t), pPrev->m_source.m_quad),
					_mm_mul_ps(pVertex->m_source.m_quad, t));
				// EXE-INLINED none: hkArray<hkVector4>::pushBack
				hkMemoryAllocator* pMore = hkMemoryRouter::getInstance().m_heap;
				if (belowPoints.m_size == (belowPoints.m_capacityAndFlags & PointArray::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(*pMore, &belowPoints, sizeof(hkVector4));
				}
				belowPoints.m_data[belowPoints.m_size] = kCut;
				belowPoints.m_size++;
				pMore = hkMemoryRouter::getInstance().m_heap;
				if (abovePoints.m_size == (abovePoints.m_capacityAndFlags & PointArray::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(*pMore, &abovePoints, sizeof(hkVector4));
				}
				abovePoints.m_data[abovePoints.m_size] = kCut;
				abovePoints.m_size++;
			}
			if (0.0f >= pVertex->m_scratch) {
				hkMemoryAllocator* pMore = hkMemoryRouter::getInstance().m_heap;
				if (belowPoints.m_size == (belowPoints.m_capacityAndFlags & PointArray::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(*pMore, &belowPoints, sizeof(hkVector4));
				}
				belowPoints.m_data[belowPoints.m_size] = pVertex->m_source;
				belowPoints.m_size++;
			}
			if (0.0f <= pVertex->m_scratch) {
				hkMemoryAllocator* pMore = hkMemoryRouter::getInstance().m_heap;
				if (abovePoints.m_size == (abovePoints.m_capacityAndFlags & PointArray::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(*pMore, &abovePoints, sizeof(hkVector4));
				}
				abovePoints.m_data[abovePoints.m_size] = pVertex->m_source;
				abovePoints.m_size++;
			}
			pPrev = pVertex;
		}
		if (neg) {
			hkgpConvexHull* pMemory =
				static_cast<hkgpConvexHull*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkgpConvexHull)));
			pMemory->m_memSizeAndFlags = sizeof(hkgpConvexHull);
			hkgpConvexHull* pHull = ::new (pMemory) hkgpConvexHull;
			*neg = pHull;
			pHull->m_data->m_config = m_config;
			(*neg)->m_data->m_projectionPlane = m_projectionPlane;
			if ((*neg)->m_data->build(belowPoints.m_data, belowPoints.m_size, 1, this) != m_dimensions) {
				if (*neg) {
					delete *neg;
				}
				*neg = nullptr;
			}
		}
		if (pos) {
			hkgpConvexHull* pMemory =
				static_cast<hkgpConvexHull*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkgpConvexHull)));
			pMemory->m_memSizeAndFlags = sizeof(hkgpConvexHull);
			hkgpConvexHull* pHull = ::new (pMemory) hkgpConvexHull;
			*pos = pHull;
			pHull->m_data->m_config = m_config;
			(*pos)->m_data->m_projectionPlane = m_projectionPlane;
			if ((*pos)->m_data->build(abovePoints.m_data, abovePoints.m_size, 1, this) != m_dimensions) {
				if (*pos) {
					delete *pos;
				}
				*pos = nullptr;
			}
		}
		// EXE-INLINED none: hkArray<hkVector4>::~hkArray
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		abovePoints.m_size = 0;
		if (abovePoints.m_capacityAndFlags >= 0) {
			pHeap->bufFree(abovePoints.m_data,
				static_cast<int32_t>(sizeof(hkVector4)) * (abovePoints.m_capacityAndFlags & PointArray::CAPACITY_MASK));
		}
		pHeap = hkMemoryRouter::getInstance().m_heap;
		belowPoints.m_size = 0;
		if (belowPoints.m_capacityAndFlags >= 0) {
			pHeap->bufFree(belowPoints.m_data,
				static_cast<int32_t>(sizeof(hkVector4)) * (belowPoints.m_capacityAndFlags & PointArray::CAPACITY_MASK));
		}
		return hkgpConvexHull::CROSSING;
	}
	if (m_dimensions == 3) {
		// The distances' range ([0] min, [1] max) and the counts above ([0]) and not above ([1]).
		float afRange[2];
		alignas(16) int32_t aiCounts[2];
		aiCounts[0] = 0;
		aiCounts[1] = 0;
		if (classify(&plane.m_quad, thickness, aiCounts, afRange) != 3) {
			if (aiCounts[1] > aiCounts[0]) {
				return hkgpConvexHull::NEGATIVE;
			}
			return static_cast<hkgpConvexHull::Side>(aiCounts[0] != 0);
		}
		// EXE-INLINED none: hkArray<hkVector4>::hkArray
		// Both 16-aligned as the exe's slots ([esp+0x20], [esp+0x10]).
		alignas(16) FNVD_INLINED_LOCAL(PointArray, abovePoints, std::nullopt, std::nullopt);
		abovePoints.m_data = nullptr;
		abovePoints.m_size = 0;
		abovePoints.m_capacityAndFlags = PointArray::DONT_DEALLOCATE_FLAG;
		alignas(16) FNVD_INLINED_LOCAL(PointArray, belowPoints, std::nullopt, std::nullopt);
		belowPoints.m_data = nullptr;
		belowPoints.m_size = 0;
		belowPoints.m_capacityAndFlags = PointArray::DONT_DEALLOCATE_FLAG;
		const float fMin = afRange[0];
		const float fMax = afRange[1];
		const float fWidth = fMax - fMin;
		const float fAbsMax = __builtin_fabsf(fMax);
		const float fAbsMin = __builtin_fabsf(fMin);
		const float fNearest = fAbsMax > fAbsMin ? fAbsMin : fAbsMax;
		if (!(thickness >= fWidth) && !(fNearest <= thickness)) {
			// EXE-INLINED none: hkArray<hkVector4>::reserve
			int32_t iCount = m_mesh.m_vertices.m_numUsed / 4 + aiCounts[0];
			hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
			int32_t iCapacity = abovePoints.m_capacityAndFlags & PointArray::CAPACITY_MASK;
			if (iCapacity < iCount) {
				iCapacity += iCapacity;
				if (iCount >= iCapacity) {
					iCapacity = iCount;
				}
				hkArrayUtil::_reserve(*pHeap, &abovePoints, iCapacity, sizeof(hkVector4));
			}
			iCount = m_mesh.m_vertices.m_numUsed / 4 + aiCounts[1];
			pHeap = hkMemoryRouter::getInstance().m_heap;
			iCapacity = belowPoints.m_capacityAndFlags & PointArray::CAPACITY_MASK;
			if (iCapacity < iCount) {
				iCapacity += iCapacity;
				if (iCount >= iCapacity) {
					iCapacity = iCount;
				}
				hkArrayUtil::_reserve(*pHeap, &belowPoints, iCapacity, sizeof(hkVector4));
			}
			// A vertex on the plane goes to both sides, one above (scratch above 0) above, any other below.
			for (Vertex* pVertex = m_mesh.m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
				const float fScratch = pVertex->m_scratch;
				const bool bOnPlane = fScratch == 0.0f;
				if (bOnPlane || fScratch > 0.0f) {
					// EXE-INLINED none: hkArray<hkVector4>::pushBack
					hkMemoryAllocator* pMore = hkMemoryRouter::getInstance().m_heap;
					if (abovePoints.m_size == (abovePoints.m_capacityAndFlags & PointArray::CAPACITY_MASK)) {
						hkArrayUtil::_reserveMore(*pMore, &abovePoints, sizeof(hkVector4));
					}
					abovePoints.m_data[abovePoints.m_size] = pVertex->m_source;
					abovePoints.m_size++;
				}
				if (bOnPlane || !(fScratch > 0.0f)) {
					hkMemoryAllocator* pMore = hkMemoryRouter::getInstance().m_heap;
					if (belowPoints.m_size == (belowPoints.m_capacityAndFlags & PointArray::CAPACITY_MASK)) {
						hkArrayUtil::_reserveMore(*pMore, &belowPoints, sizeof(hkVector4));
					}
					belowPoints.m_data[belowPoints.m_size] = pVertex->m_source;
					belowPoints.m_size++;
				}
			}
			// The crossing points of the edges from a vertex above to a vertex below.
			for (Triangle* pTriangle = m_mesh.m_triangles.m_used; pTriangle; pTriangle = pTriangle->m_next) {
				for (int32_t i = 0; i < 3; i++) {
					const Vertex* pA = pTriangle->m_vertices[i];
					const float fA = pA->m_scratch;
					if (!(fA > 0.0f)) {
						continue;
					}
					const Vertex* pB = pTriangle->m_vertices[(9 >> (2 * i)) & 3];
					const float fB = pB->m_scratch;
					if (!(static_cast<double>(fB) * fA < 0.0)) {
						continue;
					}
					const float fT = static_cast<float>(static_cast<double>(fA) / (static_cast<double>(fA) - fB));
					// EXE-INLINED 0x5DBF70: hkVector4::setInterpolate4
					const hkQuadReal t = _mm_shuffle_ps(_mm_set_ss(fT), _mm_set_ss(fT), 0);
					hkVector4 kCut;
					kCut.m_quad = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), t), pA->m_source.m_quad),
						_mm_mul_ps(pB->m_source.m_quad, t));
					hkMemoryAllocator* pMore = hkMemoryRouter::getInstance().m_heap;
					if (abovePoints.m_size == (abovePoints.m_capacityAndFlags & PointArray::CAPACITY_MASK)) {
						hkArrayUtil::_reserveMore(*pMore, &abovePoints, sizeof(hkVector4));
					}
					abovePoints.m_data[abovePoints.m_size] = kCut;
					abovePoints.m_size++;
					pMore = hkMemoryRouter::getInstance().m_heap;
					if (belowPoints.m_size == (belowPoints.m_capacityAndFlags & PointArray::CAPACITY_MASK)) {
						hkArrayUtil::_reserveMore(*pMore, &belowPoints, sizeof(hkVector4));
					}
					belowPoints.m_data[belowPoints.m_size] = kCut;
					belowPoints.m_size++;
				}
			}
			if (pos) {
				hkgpConvexHull* pMemory = static_cast<hkgpConvexHull*>(hkMemoryRouter::getInstance()
						.m_heap->blockAlloc(sizeof(hkgpConvexHull)));
				pMemory->m_memSizeAndFlags = sizeof(hkgpConvexHull);
				hkgpConvexHull* pHull = ::new (pMemory) hkgpConvexHull;
				*pos = pHull;
				pHull->m_data->m_config = m_config;
			}
			if (neg) {
				hkgpConvexHull* pMemory = static_cast<hkgpConvexHull*>(hkMemoryRouter::getInstance()
						.m_heap->blockAlloc(sizeof(hkgpConvexHull)));
				pMemory->m_memSizeAndFlags = sizeof(hkgpConvexHull);
				hkgpConvexHull* pHull = ::new (pMemory) hkgpConvexHull;
				*neg = pHull;
				pHull->m_data->m_config = m_config;
			}
			bool bAboveBuilt = false;
			if (pos) {
				bAboveBuilt = (*pos)->m_data->build(abovePoints.m_data, abovePoints.m_size, 0, this) == m_dimensions;
			}
			bool bBelowBuilt = false;
			if (neg) {
				bBelowBuilt = (*neg)->m_data->build(belowPoints.m_data, belowPoints.m_size, 0, this) == m_dimensions;
			}
			if (!bAboveBuilt && pos) {
				if (*pos) {
					delete *pos;
				}
				*pos = nullptr;
			}
			if (!bBelowBuilt && neg) {
				if (*neg) {
					delete *neg;
				}
				*neg = nullptr;
			}
			// EXE-INLINED none: hkArray<hkVector4>::~hkArray
			pHeap = hkMemoryRouter::getInstance().m_heap;
			belowPoints.m_size = 0;
			if (belowPoints.m_capacityAndFlags >= 0) {
				pHeap->bufFree(belowPoints.m_data,
					static_cast<int32_t>(sizeof(hkVector4)) *
						(belowPoints.m_capacityAndFlags & PointArray::CAPACITY_MASK));
			}
			pHeap = hkMemoryRouter::getInstance().m_heap;
			abovePoints.m_size = 0;
			if (abovePoints.m_capacityAndFlags >= 0) {
				pHeap->bufFree(abovePoints.m_data,
					static_cast<int32_t>(sizeof(hkVector4)) *
						(abovePoints.m_capacityAndFlags & PointArray::CAPACITY_MASK));
			}
			return hkgpConvexHull::CROSSING;
		}
		// One side: that of the range's middle.
		const hkgpConvexHull::Side side =
			(static_cast<double>(fMax) + fMin) * 0.5 >= 0.0 ? hkgpConvexHull::POSITIVE : hkgpConvexHull::NEGATIVE;
		// EXE-INLINED none: hkArray<hkVector4>::~hkArray
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		belowPoints.m_size = 0;
		if ((belowPoints.m_capacityAndFlags & PointArray::DONT_DEALLOCATE_FLAG) == 0) {
			pHeap->bufFree(belowPoints.m_data,
				static_cast<int32_t>(sizeof(hkVector4)) * (belowPoints.m_capacityAndFlags & PointArray::CAPACITY_MASK));
		}
		pHeap = hkMemoryRouter::getInstance().m_heap;
		abovePoints.m_size = 0;
		if ((abovePoints.m_capacityAndFlags & PointArray::DONT_DEALLOCATE_FLAG) == 0) {
			pHeap->bufFree(abovePoints.m_data,
				static_cast<int32_t>(sizeof(hkVector4)) * (abovePoints.m_capacityAndFlags & PointArray::CAPACITY_MASK));
		}
		return side;
	}
	alignas(16) char acBuffer[512];
	alignas(16) hkErrStream kStream(acBuffer, 512);
	kStream << "Current dimension not implemented for that operation";
	if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x3A86E3EF, acBuffer,
			".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 2132)) {
		__builtin_debugtrap();
	}
	return hkgpConvexHull::INVALID;
}

// The SDK's clone: a new implementation with this one's build configuration, projection, quantization, transform and
// mass properties (not m_inertiaMatrix, m_uid or the caches), a copy of its pending vertices and planes, and copies of
// its mesh: the vertices are allocated from the last to the first and mapped old to new in a pointer map, each new
// triangle's vertices are remapped through it, and its links rebuilt through a second map from each old edge still
// waiting for its twin to the new edge. The x87 copies of m_volume, m_surface and each pending
// vertex's m_scratch are plain float copies here.
hkgpConvexHullImpl* hkgpConvexHullImpl::clone() const {
	// Keys and values are untyped pointer-sized words (hkUlong), so vertices and edge uids go in and out through
	// reinterpret_cast.
	using PointerMap = hkMapBase<hkUlong, hkUlong, hkMapOperations<hkUlong>>;
	void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkgpConvexHullImpl));
	hkgpConvexHullImpl* pClone = pMemory ? ::new (pMemory) hkgpConvexHullImpl : nullptr;
	pClone->m_config = m_config;
	pClone->m_projectionAxis = m_projectionAxis;
	pClone->m_projectionPlane = m_projectionPlane;
	pClone->m_origin = m_origin;
	pClone->m_scale = m_scale;
	pClone->m_scaleInv = m_scaleInv;
	pClone->m_localTransform.m_rotation.m_col0 = m_localTransform.m_rotation.m_col0;
	pClone->m_localTransform.m_rotation.m_col1 = m_localTransform.m_rotation.m_col1;
	pClone->m_localTransform.m_rotation.m_col2 = m_localTransform.m_rotation.m_col2;
	pClone->m_localTransform.m_translation = m_localTransform.m_translation;
	pClone->m_inertia = m_inertia;
	pClone->m_volume = m_volume;
	pClone->m_surface = m_surface;
	pClone->m_dimensions = m_dimensions;
	pClone->m_pid = m_pid;
	pClone->m_hasMassProperties.m_bool = m_hasMassProperties.m_bool;
	pClone->m_hasIndexing.m_bool = m_hasIndexing.m_bool;
	{
		// EXE-INLINED none: hkArray<Vertex, hkContainerHeapAllocator>::setSize
		const int32_t numPendings = m_pendings.m_size;
		hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
		const int32_t capacity = pClone->m_pendings.getCapacity();
		if (capacity < numPendings) {
			const int32_t doubled = 2 * capacity;
			hkArrayUtil::_reserve(heap, &pClone->m_pendings, numPendings < doubled ? doubled : numPendings,
				static_cast<int32_t>(sizeof(Vertex)));
		}
		// The new elements are constructed in place (null-tested as placement new does): Vertex's constructor
		// clears m_scratch and sets m_index to -1.
		Vertex* pElem = pClone->m_pendings.m_data + pClone->m_pendings.m_size;
		for (int32_t n = numPendings - pClone->m_pendings.m_size; n > 0; --n, ++pElem) {
			if (pElem) {
				// EXE-INLINED none: hkgpConvexHullImpl::Vertex::Vertex
				pElem->m_scratch = 0.0f;
				pElem->m_index = -1;
			}
		}
		pClone->m_pendings.m_size = numPendings;
	}
	for (int32_t i = 0; i < m_pendings.m_size; ++i) {
		const Vertex& source = m_pendings.m_data[i];
		Vertex& target = pClone->m_pendings.m_data[i];
		// EXE-INLINED none: hkgpConvexHullImpl::Vertex::operator= (the vertex's own fields; the list links stay)
		target.m_source = source.m_source;
		target.m_x[0] = source.m_x[0];
		target.m_x[1] = source.m_x[1];
		target.m_x[2] = source.m_x[2];
		target.m_scratch = source.m_scratch;
		target.m_refs = source.m_refs;
		target.m_index = source.m_index;
	}
	{
		// EXE-INLINED none: hkArray<hkVector4, hkContainerHeapAllocator>::setSize
		const int32_t numPlanes = m_planes.m_size;
		hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
		const int32_t capacity = pClone->m_planes.getCapacity();
		if (capacity < numPlanes) {
			const int32_t doubled = 2 * capacity;
			hkArrayUtil::_reserve(heap, &pClone->m_planes, numPlanes < doubled ? doubled : numPlanes,
				static_cast<int32_t>(sizeof(hkVector4)));
		}
		pClone->m_planes.m_size = numPlanes;
	}
	for (int32_t i = 0; i < m_planes.m_size; ++i) {
		pClone->m_planes.m_data[i] = m_planes.m_data[i];
	}
	FNVD_INLINED_LOCAL(PointerMap, vertexMap, std::nullopt, std::nullopt);
	// EXE-INLINED none: hkMap<hkUlong, hkUlong, hkMapOperations<hkUlong>, hkContainerHeapAllocator>::hkMap
	vertexMap.m_elem = nullptr;
	vertexMap.m_numElems = 0;
	vertexMap.m_hashMod = -1;
	vertexMap.reserve(hkMemoryRouter::getInstance().heap(), 0);
	const int32_t numVertices = m_mesh.m_vertices.m_numUsed;
	if (numVertices) {
		vertexMap.reserve(hkMemoryRouter::getInstance().heap(), numVertices);
		Vertex* v = m_mesh.m_vertices.m_used;
		if (v) {
			while (v->m_next) {
				v = v->m_next;
			}
			for (; v; v = v->m_prev) {
				Vertex* pNew = pClone->m_mesh.m_vertices.allocate(*v);
				vertexMap.insert(hkMemoryRouter::getInstance().heap(), reinterpret_cast<hkUlong>(v),
					reinterpret_cast<hkUlong>(pNew));
			}
		}
	}
	if (m_mesh.m_triangles.m_numUsed) {
		FNVD_INLINED_LOCAL(PointerMap, edgeMap, std::nullopt, std::nullopt);
		// EXE-INLINED none: hkMap<hkUlong, hkUlong, hkMapOperations<hkUlong>, hkContainerHeapAllocator>::hkMap
		edgeMap.m_elem = nullptr;
		edgeMap.m_numElems = 0;
		edgeMap.m_hashMod = -1;
		edgeMap.reserve(hkMemoryRouter::getInstance().heap(), 0);
		const int32_t numTriangles = m_mesh.m_triangles.m_numUsed;
		edgeMap.reserve(hkMemoryRouter::getInstance().heap(), numTriangles);
		Triangle* t = m_mesh.m_triangles.m_used;
		if (t) {
			while (t->m_next) {
				t = t->m_next;
			}
		}
		for (; t; t = t->m_prev) {
			Triangle* pNew = pClone->m_mesh.m_triangles.allocate(*t);
			for (int32_t k = 0; k < 3; ++k) {
				pNew->m_vertices[k] =
					reinterpret_cast<Vertex*>(vertexMap.getWithDefault(reinterpret_cast<hkUlong>(pNew->m_vertices[k]),
						0));
			}
			// An edge's uid is its triangle's address with the edge index in the low two bits; a link (copied from
			// the old triangle) is the uid of the old twin edge, decoded into its triangle and index and encoded again.
			for (uint32_t k = 0; k < 3; ++k) {
				const uintptr_t link = pNew->m_links[k];
				const hkUlong twin = edgeMap.getWithDefault((link & 3) + (link & ~static_cast<uintptr_t>(3)), 0);
				Triangle* pTwin = reinterpret_cast<Triangle*>(twin & ~static_cast<uintptr_t>(3));
				if (pTwin) {
					const uintptr_t key = pNew->m_links[k];
					edgeMap.remove((key & 3) + (key & ~static_cast<uintptr_t>(3)));
					pNew->m_links[k] = (twin & 3) + reinterpret_cast<uintptr_t>(pTwin);
					pTwin->m_links[twin & 3] = reinterpret_cast<uintptr_t>(pNew) + k;
				} else {
					edgeMap.insert(hkMemoryRouter::getInstance().heap(), reinterpret_cast<hkUlong>(t) + k,
						reinterpret_cast<hkUlong>(pNew) + k);
				}
			}
		}
		edgeMap.clearAndDeallocate(hkMemoryRouter::getInstance().heap());
	}
	vertexMap.clearAndDeallocate(hkMemoryRouter::getInstance().heap());
	return pClone;
}

// Each vertex's neighbours (the vertex after it in each of its triangles) are gathered in a local array of per-vertex
// arrays; edges then holds, vertex by vertex, the neighbour count followed by the neighbours, and vertexOffsets each
// vertex's offset in edges.
void hkgpConvexHullImpl::generateVertexAdjacencyGraph(hkArray<int32_t, hkContainerHeapAllocator>& edges,
	hkArray<int32_t, hkContainerHeapAllocator>& vertexOffsets) {
	using vertexEdges_t = hkArray<int32_t, hkContainerHeapAllocator>;
	using adjacency_t = hkArray<vertexEdges_t, hkContainerHeapAllocator>;
	if (!m_hasIndexing.m_bool) {
		char acBuffer[0x200];
		hkErrStream errStream(acBuffer, 0x200);
		errStream << "No index available (" << m_uid << ")";
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x79F9D886, acBuffer,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 328)) {
			__builtin_debugtrap();
		}
	}
	const int32_t numVertices = m_mesh.m_vertices.m_numUsed;
	FNVD_INLINED_LOCAL(adjacency_t, adjacency, std::nullopt, std::nullopt);
	// EXE-INLINED none: hkArray<hkArray<int, hkContainerHeapAllocator>, hkContainerHeapAllocator>::hkArray
	adjacency.m_data = nullptr;
	adjacency.m_size = 0;
	adjacency.m_capacityAndFlags = hkArrayBase<vertexEdges_t>::DONT_DEALLOCATE_FLAG;
	{
		// EXE-INLINED none: hkArray<hkArray<int, hkContainerHeapAllocator>, hkContainerHeapAllocator>::setSize
		hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
		const int32_t capacity = adjacency.getCapacity();
		if (capacity < numVertices) {
			const int32_t doubled = 2 * capacity;
			hkArrayUtil::_reserve(heap, &adjacency, numVertices < doubled ? doubled : numVertices,
				static_cast<int32_t>(sizeof(vertexEdges_t)));
		}
		// The elements past the new size are destroyed, the new ones constructed in place (null-tested as placement
		// new does).
		vertexEdges_t* pOld = adjacency.m_data + numVertices;
		for (int32_t n = adjacency.m_size - numVertices; n > 0; --n, ++pOld) {
			// EXE-INLINED none: hkArray<int, hkContainerHeapAllocator>::~hkArray
			hkMemoryAllocator& elementHeap = hkMemoryRouter::getInstance().heap();
			pOld->m_size = 0;
			if (pOld->m_capacityAndFlags >= 0) {
				elementHeap.bufFree(pOld->m_data, static_cast<int32_t>(sizeof(int32_t)) * pOld->getCapacity());
			}
			pOld->m_data = nullptr;
			pOld->m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
		}
		vertexEdges_t* pNew = adjacency.m_data + adjacency.m_size;
		for (int32_t n = numVertices - adjacency.m_size; n > 0; --n, ++pNew) {
			if (pNew) {
				// EXE-INLINED none: hkArray<int, hkContainerHeapAllocator>::hkArray
				pNew->m_data = nullptr;
				pNew->m_size = 0;
				pNew->m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
			}
		}
		adjacency.m_size = numVertices;
	}
	for (Triangle* t = m_mesh.m_triangles.m_used; t; t = t->m_next) {
		for (uint32_t k = 0; k < 3; ++k) {
			vertexEdges_t& vertexEdges = adjacency.m_data[t->m_vertices[k]->m_index];
			// The triangle's next vertex after k.
			const int32_t* pNeighbour = &t->m_vertices[(9 >> (2 * k)) & 3]->m_index;
			// EXE-INLINED none: hkArray<int, hkContainerHeapAllocator>::pushBack
			hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
			if (vertexEdges.m_size == vertexEdges.getCapacity()) {
				hkArrayUtil::_reserveMore(heap, &vertexEdges, static_cast<int32_t>(sizeof(int32_t)));
			}
			vertexEdges.m_data[vertexEdges.m_size] = *pNeighbour;
			vertexEdges.m_size += 1;
		}
	}
	edges.m_size = 0;
	{
		// EXE-INLINED none: hkArray<int, hkContainerHeapAllocator>::reserve
		const int32_t numEdges = 3 * m_mesh.m_triangles.m_numUsed + m_mesh.m_vertices.m_numUsed;
		hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
		const int32_t capacity = edges.getCapacity();
		if (capacity < numEdges) {
			const int32_t doubled = 2 * capacity;
			hkArrayUtil::_reserve(heap, &edges, numEdges < doubled ? doubled : numEdges,
				static_cast<int32_t>(sizeof(int32_t)));
		}
	}
	{
		// EXE-INLINED none: hkArray<int, hkContainerHeapAllocator>::setSize
		const int32_t numOffsets = m_mesh.m_vertices.m_numUsed;
		hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
		const int32_t capacity = vertexOffsets.getCapacity();
		if (capacity < numOffsets) {
			const int32_t doubled = 2 * capacity;
			hkArrayUtil::_reserve(heap, &vertexOffsets, numOffsets < doubled ? doubled : numOffsets,
				static_cast<int32_t>(sizeof(int32_t)));
		}
		vertexOffsets.m_size = numOffsets;
	}
	int32_t offset = 0;
	for (int32_t i = 0; i < adjacency.m_size; ++i) {
		const int32_t numNeighbours = adjacency.m_data[i].m_size;
		{
			// EXE-INLINED none: hkArray<int, hkContainerHeapAllocator>::pushBack
			hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
			if (edges.m_size == edges.getCapacity()) {
				hkArrayUtil::_reserveMore(heap, &edges, static_cast<int32_t>(sizeof(int32_t)));
			}
			edges.m_data[edges.m_size] = numNeighbours;
			edges.m_size += 1;
		}
		for (int32_t j = 0; j < adjacency.m_data[i].m_size; ++j) {
			const int32_t* pNeighbour = &adjacency.m_data[i].m_data[j];
			// EXE-INLINED none: hkArray<int, hkContainerHeapAllocator>::pushBack
			hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
			if (edges.m_size == edges.getCapacity()) {
				hkArrayUtil::_reserveMore(heap, &edges, static_cast<int32_t>(sizeof(int32_t)));
			}
			edges.m_data[edges.m_size] = *pNeighbour;
			edges.m_size += 1;
		}
		vertexOffsets.m_data[i] = offset;
		offset += adjacency.m_data[i].m_size + 1;
	}
	// EXE-INLINED none: hkArray<hkArray<int, hkContainerHeapAllocator>, hkContainerHeapAllocator>::~hkArray
	hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
	vertexEdges_t* pElement = adjacency.m_data;
	for (int32_t n = adjacency.m_size; n > 0; --n, ++pElement) {
		// EXE-INLINED none: hkArray<int, hkContainerHeapAllocator>::~hkArray
		hkMemoryAllocator& elementHeap = hkMemoryRouter::getInstance().heap();
		pElement->m_size = 0;
		if (pElement->m_capacityAndFlags >= 0) {
			elementHeap.bufFree(pElement->m_data, static_cast<int32_t>(sizeof(int32_t)) * pElement->getCapacity());
		}
		pElement->m_data = nullptr;
		pElement->m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
	}
	adjacency.m_size = 0;
	if (adjacency.m_capacityAndFlags >= 0) {
		heap.bufFree(adjacency.m_data, static_cast<int32_t>(sizeof(vertexEdges_t)) * adjacency.getCapacity());
	}
}

void hkgpConvexHullImpl::getSupportingVertexTransformed(__m128* a1, __m128* a2, __m128* a3) {
	hkgpConvexHullImpl::Vertex* m_used = m_mesh.m_vertices.m_used;
	float v9 = 0.0f;
	for (hkgpConvexHullImpl::Vertex* i = 0; m_used; m_used = m_used->m_next) {
		__m128 v6 =
			_mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(m_used->m_source.m_quad, m_used->m_source.m_quad,
															85),
												 a1[1]),
									  _mm_mul_ps(_mm_shuffle_ps(m_used->m_source.m_quad, m_used->m_source.m_quad, 0),
										  *a1)),
						   _mm_mul_ps(_mm_shuffle_ps(m_used->m_source.m_quad, m_used->m_source.m_quad, 170), a1[2])),
				a1[3]);
		__m128 v7 = _mm_mul_ps(*a2, v6);
		float v8 = _mm_shuffle_ps(v7, v7, 170)[0] + static_cast<float>(_mm_shuffle_ps(v7, v7, 85)[0] + v7[0]);
		if (!i || v9 < static_cast<double>(v8)) {
			v9 = v8;
			*a3 = v6;
			i = m_used;
		}
	}
}

// The vertices' sides of the plane a1 (dot3 of the source position plus w, summed (z + w) + (y + x) in SSE): each
// distance goes to the vertex's scratch slot and counts on a3[0] (above) or a3[1] (not above) unless its magnitude is
// below the margin a2 (scratch 0 then); the range of distances goes to a4[0] (min) and a4[1] (max), or to a local
// when a4 is null. Returns 3 when both sides have vertices, 2 when only the lower side has more, else whether any is
// above. The compares are x87 (an unordered distance counts as not above and does not move the range).
int32_t hkgpConvexHullImpl::classify(const __m128* a1, float a2, int32_t* a3, float* a4) {
	float afLocalRange[2];
	float* pRange = a4 ? a4 : afLocalRange;
	a3[1] = 0;
	a3[0] = 0;
	pRange[1] = 0.0f;
	pRange[0] = 0.0f;
	for (Vertex* pVertex = m_mesh.m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
		const __m128 plane = *a1;
		const __m128 prod = _mm_mul_ps(pVertex->m_source.m_quad, plane);
		const float fDistance =
			_mm_cvtss_f32(_mm_add_ps(_mm_add_ss(_mm_shuffle_ps(prod, prod, 0xAA), _mm_shuffle_ps(plane, plane, 0xFF)),
				_mm_add_ss(_mm_shuffle_ps(prod, prod, 0x55), prod)));
		if (pVertex->m_prev == nullptr) {
			pRange[1] = fDistance;
			pRange[0] = fDistance;
		} else {
			const float fMin = pRange[0];
			pRange[0] = fMin > fDistance ? fDistance : fMin;
			const float fMax = pRange[1];
			pRange[1] = fMax < fDistance ? fDistance : fMax;
		}
		const float fMagnitude = std::fabs(fDistance);
		if (fMagnitude < a2) {
			pVertex->m_scratch = 0.0f;
		} else {
			pVertex->m_scratch = fDistance;
			if (fDistance > 0.0f) {
				++a3[0];
			} else {
				++a3[1];
			}
		}
	}
	if (a3[0] && a3[1]) {
		return 3;
	}
	if (a3[1] > a3[0]) {
		return 2;
	}
	return a3[0] != 0;
}

// The vertex farthest from point (the first of equals, point itself when there is none or all are at zero
// distance): its squared distance to point in a1's lane 0, the z term in lanes 1 to 3.
hkVector4* hkgpConvexHullImpl::getFarthestVertexDistanceSquared(hkVector4* a1, const hkVector4* point) {
	const __m128 p = point->m_quad;
	__m128 farthest = p;
	float fBest = 0.0f;
	for (Vertex* v = m_mesh.m_vertices.m_used; v; v = v->m_next) {
		const __m128 source = v->m_source.m_quad;
		const __m128 d = _mm_sub_ps(p, source);
		const __m128 sq = _mm_mul_ps(d, d);
		const float fDistance =
			_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
		// Only a strictly larger distance replaces the best.
		if (fBest < fDistance) {
			fBest = fDistance;
			farthest = source;
		}
	}
	const __m128 d = _mm_sub_ps(p, farthest);
	const __m128 sq = _mm_mul_ps(d, d);
	a1->m_quad = _mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq));
	return a1;
}

// For each point and each triangle: the triangle's vertices transformed by transform (x c0 + y c1 + z c2 + c3), its
// unit normal (zero when degenerate) with w the plane's offset; where the point lies farther above that plane than
// the best so far (FLT_EPSILON at first), the plane goes to *planeOut. Returns the best distance.
double hkgpConvexHullImpl::maxDistanceToTransformedPlanes(const hkArrayBase<hkVector4>* points,
	const hkMatrix4* transform, hkVector4* planeOut) {
	float fBest = 1.1920929e-07f;
	for (int32_t i = 0; i < points->m_size; ++i) {
		const hkVector4* pPoint = &points->m_data[i];
		for (Triangle* t = m_mesh.m_triangles.m_used; t; t = t->m_next) {
			const __m128 c0 = transform->m_col0.m_quad;
			const __m128 c1 = transform->m_col1.m_quad;
			const __m128 c2 = transform->m_col2.m_quad;
			const __m128 c3 = transform->m_col3.m_quad;
			__m128 x[3];
			for (int32_t k = 0; k < 3; ++k) {
				const __m128 s = t->m_vertices[k]->m_source.m_quad;
				x[k] = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(s, s, 0x00), c0),
												 _mm_mul_ps(_mm_shuffle_ps(s, s, 0x55), c1)),
									  _mm_mul_ps(_mm_shuffle_ps(s, s, 0xAA), c2)),
					c3);
			}
			const __m128 e1 = _mm_sub_ps(x[1], x[0]);
			const __m128 e2 = _mm_sub_ps(x[2], x[0]);
			const __m128 normal = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(e2, e2, 0xD2), _mm_shuffle_ps(e1, e1, 0xC9)),
				_mm_mul_ps(_mm_shuffle_ps(e2, e2, 0xC9), _mm_shuffle_ps(e1, e1, 0xD2)));
			// EXE-INLINED 0x62AEA0
			const __m128 sq = _mm_mul_ps(normal, normal);
			const __m128 length2 =
				_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq));
			const __m128 approx = _mm_rsqrt_ss(length2);
			const __m128 refined = _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), approx),
				_mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(length2, approx), approx)));
			const __m128 inverse = _mm_andnot_ps(_mm_cmpeq_ss(length2, _mm_setzero_ps()), refined);
			hkVector4 plane;
			plane.m_quad = _mm_mul_ps(_mm_shuffle_ps(inverse, inverse, 0), normal);
			const __m128 d0 = _mm_mul_ps(plane.m_quad, x[0]);
			const float fDot0 =
				_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(d0, d0, 0xAA), _mm_add_ss(_mm_shuffle_ps(d0, d0, 0x55), d0)));
			plane(3) = 0.0f - fDot0;
			const __m128 d = _mm_mul_ps(pPoint->m_quad, plane.m_quad);
			const float fDistance = _mm_cvtss_f32(_mm_add_ss(_mm_add_ss(_mm_shuffle_ps(d, d, 0xAA),
																 _mm_add_ss(_mm_shuffle_ps(d, d, 0x55), d)),
				_mm_shuffle_ps(plane.m_quad, plane.m_quad, 0xFF)));
			// Only a strictly larger distance replaces the best.
			if (fBest < fDistance) {
				fBest = fDistance;
				*planeOut = plane;
			}
		}
	}
	return fBest;
}

// The support cache is built where empty, both times it is looked up.
double hkgpConvexHullImpl::getSpan(const hkVector4& direction, float* apMin, float* apMax) const {
	using apArray__t = hkArray<hkVector4, hkContainerHeapAllocator>;
	FNVD_INLINED_LOCAL(apArray__t, apArray_, std::nullopt, std::nullopt);
	hkVector4 direction_;
	hkVector4 vertexOut;
	hkVector4 vertexOut_;
	direction_.m_quad = _mm_xor_ps(direction.m_quad, hkQuadRealNegateMask);
	if (!m_supportCache.m_size) {
		int32_t m_numUsed = m_mesh.m_vertices.m_numUsed;
		apArray_.m_data = 0;
		apArray_.m_size = 0;
		apArray_.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
		hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
		int32_t m_numUsed_1 = m_numUsed;
		if (m_numUsed > 0) {
			if (m_numUsed < 2 * apArray_.getCapacity()) {
				m_numUsed_1 = 2 * apArray_.getCapacity();
			}
			hkArrayUtil::_reserve(*m_heap, &apArray_, m_numUsed_1, sizeof(hkVector4));
		}
		hkgpConvexHullImpl::Vertex* m_used = m_mesh.m_vertices.m_used;
		apArray_.m_size += m_numUsed;
		for (hkVector4* i = apArray_.begin(); m_used; ++i) {
			*i = m_used->m_source;
			m_used = m_used->m_next;
		}
		hkGeometryProcessing::buildTransposedArray(apArray_, m_supportCache);
		hkMemoryAllocator* m_heap_1 = (&hkMemoryRouter::getInstance())->m_heap;
		apArray_.m_size = 0;
		if (apArray_.m_capacityAndFlags >= 0) {
			m_heap_1->bufFree(apArray_.begin(),
				sizeof(hkVector4) * (apArray_.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK));
		}
	}
	hkArray<hkGeometryProcessing::TransposedVectors, hkContainerHeapAllocator>* vertices = &m_supportCache;
	hkGeometryProcessing::getSupportingVertex(m_supportCache, direction, vertexOut);
	if (!m_supportCache.m_size) {
		int32_t m_numUsed_2 = m_mesh.m_vertices.m_numUsed;
		apArray_.m_data = 0;
		apArray_.m_size = 0;
		apArray_.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
		hkMemoryAllocator* m_heap_2 = (&hkMemoryRouter::getInstance())->m_heap;
		int32_t m_size = apArray_.m_size;
		int32_t aiReqElem = m_numUsed_2 + apArray_.m_size;
		// The capacity compare is signed, so a negative count reserves nothing.
		if (apArray_.getCapacity() < m_numUsed_2 + apArray_.m_size) {
			if (aiReqElem < 2 * apArray_.getCapacity()) {
				aiReqElem = 2 * apArray_.getCapacity();
			}
			hkArrayUtil::_reserve(*m_heap_2, &apArray_, aiReqElem, sizeof(hkVector4));
		}
		hkgpConvexHullImpl::Vertex* m_next = m_mesh.m_vertices.m_used;
		apArray_.m_size += m_numUsed_2;
		for (hkVector4* j = &apArray_.m_data[m_size]; m_next; ++j) {
			*j = m_next->m_source;
			m_next = m_next->m_next;
		}
		hkGeometryProcessing::buildTransposedArray(apArray_, *vertices);
		hkMemoryAllocator* m_heap_3 = (&hkMemoryRouter::getInstance())->m_heap;
		apArray_.m_size = 0;
		if (apArray_.m_capacityAndFlags >= 0) {
			m_heap_3->bufFree(apArray_.begin(),
				sizeof(hkVector4) * (apArray_.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK));
		}
	}
	hkGeometryProcessing::getSupportingVertex(*vertices, direction_, vertexOut_);
	// Each dot is summed as (y + x) + z in the low lane, and the difference is rounded to a float.
	const __m128 v18 = _mm_mul_ps(direction.m_quad, vertexOut.m_quad);
	*apMax = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(v18, v18, 170), _mm_add_ss(_mm_shuffle_ps(v18, v18, 85), v18)));
	const __m128 v20 = _mm_mul_ps(direction.m_quad, vertexOut_.m_quad);
	const float v21 =
		_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(v20, v20, 170), _mm_add_ss(_mm_shuffle_ps(v20, v20, 85), v20)));
	*apMin = v21;
	return static_cast<float>(*apMax - v21);
}

// The total length of the hull's edges, by dimension: none for a point, the distance between the two vertices of a
// segment, the polygon's perimeter in vertex order (through a copy of the positions), and for a solid each edge once
// (from its smaller vertex, or either side when it has no neighbour) that does not lie between two triangles of one
// plane.
double hkgpConvexHullImpl::getEdgeLengthSum() {
	const auto length = [](__m128 a, __m128 b) {
		const __m128 d = _mm_sub_ps(a, b);
		const __m128 sq = _mm_mul_ps(d, d);
		return _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq))));
	};
	float fSum = 0.0f;
	switch (m_dimensions) {
	case 1: {
		const Vertex* pFirst = m_mesh.m_vertices.m_used;
		fSum = length(pFirst->m_source.m_quad, pFirst->m_next->m_source.m_quad);
		return fSum;
	}
	case 2: {
		using VectorArray = hkArray<hkVector4, hkContainerHeapAllocator>;
		FNVD_INLINED_LOCAL(VectorArray, positions, std::nullopt, std::nullopt);
		const int32_t iNumVertices = m_mesh.m_vertices.m_numUsed;
		positions.m_data = nullptr;
		positions.m_size = 0;
		positions.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG);
		// EXE-INLINED none: hkArray<hkVector4, hkContainerHeapAllocator>::expandBy
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		const int32_t iOldSize = positions.m_size;
		const int32_t iNewSize = iOldSize + iNumVertices;
		const int32_t iCapacity = positions.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
		if (iCapacity < iNewSize) {
			const int32_t iReserve = iNewSize < 2 * iCapacity ? 2 * iCapacity : iNewSize;
			hkArrayUtil::_reserve(*pHeap, &positions, iReserve, sizeof(hkVector4));
		}
		hkVector4* pOut = positions.m_data + iOldSize;
		positions.m_size = iNewSize;
		for (Vertex* v = m_mesh.m_vertices.m_used; v; v = v->m_next) {
			*pOut++ = v->m_source;
		}
		for (int32_t i = 0, iPrevious = positions.m_size - 1; i < positions.m_size; iPrevious = i++) {
			fSum = length(positions.m_data[iPrevious].m_quad, positions.m_data[i].m_quad) + fSum;
		}
		// EXE-INLINED none: hkArray<hkVector4, hkContainerHeapAllocator>::~hkArray
		hkMemoryAllocator* pFreeHeap = hkMemoryRouter::getInstance().m_heap;
		positions.m_size = 0;
		if (positions.m_capacityAndFlags >= 0) {
			pFreeHeap->bufFree(positions.m_data,
				static_cast<int32_t>((positions.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) *
					sizeof(hkVector4)));
		}
		return fSum;
	}
	case 3: {
		for (Triangle* t = m_mesh.m_triangles.m_used; t; t = t->m_next) {
			for (int32_t k = 0; k < 3; ++k) {
				const Vertex* pVertex = t->m_vertices[k];
				const Vertex* pNext = t->m_vertices[(9 >> (2 * k)) & 3];
				// The edge from its larger vertex is left to the neighbour's side.
				if (*pNext < *pVertex && (t->m_links[k] & ~static_cast<uintptr_t>(3)) != 0) {
					continue;
				}
				const int32_t iPlane = t->m_plane.m_index;
				// The link is hkgpTopology's Uid, an integer packing the neighbour's address with the shared edge's
				// index in its low two bits: it is masked to read the neighbour's plane id.
				const Triangle* pNeighbour =
					reinterpret_cast<const Triangle*>(t->m_links[k] & ~static_cast<uintptr_t>(3));
				if (iPlane != -1 && iPlane == pNeighbour->m_plane.m_index) {
					continue;
				}
				fSum = length(pVertex->m_source.m_quad, pNext->m_source.m_quad) + fSum;
			}
		}
		return fSum;
	}
	default:
		return fSum;
	}
}

// The SDK's ensureContainment: while a point lies more than 1e-4 above a face of the hull transformed by the
// accumulated matrix (at most points + 64 rounds), the matrix is scaled along that face's plane so that the
// transformed hull reaches the point. When anything changed, the hull is rebuilt from its vertices transformed by the
// matrix, then its indices and mass properties where it had them before.
void hkgpConvexHullImpl::ensureContainment(const hkArray<hkVector4, hkContainerHeapAllocator>& points) {
	const bool bHadIndexing = m_hasIndexing.m_bool;
	const bool bHadMassProperties = m_hasMassProperties.m_bool;
	const int32_t iMaxIterations = points.m_size + 64;
	hkMatrix4 transform = hkMatrix4Identity;
	hkVector4 plane;
	bool bChanged = false;
	for (int32_t iIteration = 0; iIteration < iMaxIterations; ++iIteration) {
		const float fDistance = maxDistanceToTransformedPlanes(&points, &transform, &plane);
		// Stops at or below 1e-4.
		if (!(fDistance > 9.999999747378752e-05)) {
			break;
		}
		// The transformed vertex farthest along the plane's negation, and its offset from the plane.
		hkVector4 support = hkVector4Zero;
		hkVector4 negated;
		negated.m_quad = _mm_xor_ps(hkQuadRealNegateMask, plane.m_quad);
		getSupportingVertexTransformed(&transform.m_col0.m_quad, &negated.m_quad, &support.m_quad);
		const __m128 c = _mm_mul_ps(support.m_quad, plane.m_quad);
		const float fDot =
			_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(c, c, 0xAA), _mm_add_ss(_mm_shuffle_ps(c, c, 0x55), c)));
		const float fOffset = -(plane(3) + fDot);
		if (!(fOffset * fOffset > 9.999999747378752e-06)) {
			break;
		}
		plane(3) = -fDot;
		hkMatrix4 previous = transform;
		const float fScale = (fOffset + fDistance) / fOffset;
		calcPlanarScaleMatrix(plane, fScale, transform);
		transform.mul(&previous);
		bChanged = true;
	}
	if (!bChanged) {
		return;
	}
	using VectorArray = hkArray<hkVector4, hkContainerHeapAllocator>;
	alignas(16) FNVD_INLINED_LOCAL(VectorArray, vertices, std::nullopt, std::nullopt);
	const int32_t iNumVertices = m_mesh.m_vertices.m_numUsed;
	vertices.m_data = nullptr;
	vertices.m_size = 0;
	vertices.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG);
	// EXE-INLINED none: hkArray<hkVector4, hkContainerHeapAllocator>::expandBy
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	const int32_t iOldSize = vertices.m_size;
	const int32_t iNewSize = iOldSize + iNumVertices;
	const int32_t iCapacity = vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
	if (iCapacity < iNewSize) {
		const int32_t iReserve = iNewSize < 2 * iCapacity ? 2 * iCapacity : iNewSize;
		hkArrayUtil::_reserve(*pHeap, &vertices, iReserve, sizeof(hkVector4));
	}
	hkVector4* pOut = vertices.m_data + iOldSize;
	vertices.m_size = iNewSize;
	for (Vertex* v = m_mesh.m_vertices.m_used; v; v = v->m_next) {
		*pOut++ = v->m_source;
	}
	for (int32_t i = 0; i < vertices.m_size; ++i) {
		const __m128 x = vertices.m_data[i].m_quad;
		vertices.m_data[i].m_quad =
			_mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(x, x, 0x55), transform.m_col1.m_quad),
									  _mm_mul_ps(_mm_shuffle_ps(x, x, 0x00), transform.m_col0.m_quad)),
						   _mm_mul_ps(_mm_shuffle_ps(x, x, 0xAA), transform.m_col2.m_quad)),
				transform.m_col3.m_quad);
	}
	build(vertices.m_data, vertices.m_size, 0, nullptr);
	if (bHadIndexing) {
		buildIndices();
	}
	if (bHadMassProperties) {
		buildMassProperties();
	}
	// EXE-INLINED none: hkArray<hkVector4, hkContainerHeapAllocator>::~hkArray
	hkMemoryAllocator* pFreeHeap = hkMemoryRouter::getInstance().m_heap;
	vertices.m_size = 0;
	if (vertices.m_capacityAndFlags >= 0) {
		pFreeHeap->bufFree(vertices.m_data,
			static_cast<int32_t>((vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) *
				sizeof(hkVector4)));
	}
}

bool hkgpConvexHullImpl::clipLine(const hkVector4& a, const hkVector4& b, float& t0, float& t1, float epsilon) const {
	if (!m_hasIndexing.m_bool) {
		alignas(16) char acBuffer[0x200];
		alignas(16) hkErrStream kStream(acBuffer, 0x200);
		// m_uid is read before the first operator<<.
		const uint32_t uiUid = m_uid;
		hkOstream* pStream = &kStream.operator<<("No index available (");
		pStream = &pStream->operator<<(uiUid);
		pStream->operator<<(")");
		// A handler that returns nonzero asks for the break.
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x79F9D886, acBuffer,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 328)) {
			__builtin_debugtrap();
		}
	}
	// The clipped ends, recomputed from a and b after each cut.
	__m128 x0 = a.m_quad;
	__m128 x1 = b.m_quad;
	t0 = 0.0f;
	t1 = 1.0f;
	for (int32_t i = 0; i < m_planes.m_size; ++i) {
		const __m128 plane = m_planes.m_data[i].m_quad;
		const __m128 w = _mm_shuffle_ps(plane, plane, 0xFF);
		const __m128 p0 = _mm_mul_ps(plane, x0);
		const __m128 p1 = _mm_mul_ps(plane, x1);
		float d[2];
		d[0] = _mm_cvtss_f32(_mm_add_ps(_mm_add_ss(_mm_shuffle_ps(p0, p0, 0xAA), w),
			_mm_add_ss(_mm_shuffle_ps(p0, p0, 0x55), p0)));
		d[1] = _mm_cvtss_f32(_mm_add_ps(_mm_add_ss(_mm_shuffle_ps(p1, p1, 0x55), p1),
			_mm_add_ss(_mm_shuffle_ps(p1, p1, 0xAA), w)));
		// Bit k set when end k lies above the plane's thickness; a distance below FLT_EPSILON in size becomes 0.
		int32_t iOutside = 0;
		for (int32_t k = 0; k < 2; ++k) {
			if (d[k] > epsilon) {
				iOutside |= 1 << k;
			}
			const float fAbs = std::fabs(d[k]);
			if (fAbs < 1.1920929e-07f) {
				d[k] = 0.0f;
			}
		}
		if (iOutside == 3) {
			return false;
		}
		if (d[0] * d[1] < 0.0f) {
			const float t = d[0] / (d[0] - d[1]);
			if (d[0] < 0.0f) {
				t0 = t;
			} else {
				t1 = t;
			}
			const __m128 s0 = _mm_set1_ps(t0);
			const __m128 s1 = _mm_set1_ps(t1);
			const __m128 one = _mm_set1_ps(1.0f);
			x0 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(one, s0), a.m_quad), _mm_mul_ps(b.m_quad, s0));
			x1 = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(one, s1), a.m_quad), _mm_mul_ps(b.m_quad, s1));
		}
	}
	return true;
}

// The smallest-area rectangle in projectionPlane around the hull: each candidate is a unit axis in the plane with
// the plane's normal crossed with it, the hull's extents along both from getSpan. A 3D hull tries, for each edge
// between triangles of different planes that face opposite sides of projectionPlane (the product of their planes' dots
// with it not above zero), projectionPlane crossed with the edge's direction; a 2D hull tries each of its edge planes.
// The best pair goes to worldTransform's first two columns (projectionPlane the third), its center, moved onto the
// plane by its w, to the translation and its half extents to halfExtents. Returns the best area, HK_REAL_MAX
// (3.40282e+38f) when no pair qualified.
float hkgpConvexHullImpl::getOrientedRectangle(const hkVector4& projectionPlane, hkVector4& halfExtents,
	hkTransform& worldTransform) const {
	typedef hkgpAbstractMesh<Edge, Vertex, Triangle, hkContainerHeapAllocator>::EdgeIterator EdgeIterator;
	if (!m_hasIndexing.m_bool) {
		char buf[512];
		hkErrStream errStream(buf, 512);
		errStream << "No index available (" << m_uid << ")";
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x79f9d886, buf,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 328)) {
			__builtin_debugtrap();
		}
	}
	float fBestArea = 3.40282e+38f;
	float fBestExtent0 = 3.40282e+38f;
	float fBestExtent1 = 3.40282e+38f;
	float fCenter0 = 3.40282e+38f;
	float fCenter1 = 3.40282e+38f;
	hkVector4 bestAxis0 = hkVector4::getZero();
	hkVector4 bestAxis1 = hkVector4::getZero();
	// getSpan writes each min/max pair as two adjacent floats into one of two fixed slots, pSlotLow (whose max is
	// 16-aligned) and pSlotHigh. The 3D path gives the first axis pSlotHigh, the 2D path gives it pSlotLow. Using one
	// 16-aligned array keeps both slots aligned.
	alignas(16) float fExtentSlots[12];
	float* const pSlotLow = &fExtentSlots[3];
	float* const pSlotHigh = &fExtentSlots[10];
	if (m_dimensions == 3) {
		Edge firstEdge;
		firstEdge.m_triangle = m_mesh.m_triangles.m_used;
		firstEdge.m_index = 0;
		EdgeIterator it(firstEdge);
		for (; it.m_edge.m_triangle; it.next()) {
			const Triangle* pTriangle = it.m_edge.m_triangle;
			// The link is an untyped word: the neighbour's address with the shared edge's index in the low two bits
			// (hkgpTopology's Uid), as getNeighbour reads it.
			const Triangle* pNeighbour =
				reinterpret_cast<const Triangle*>(pTriangle->m_links[it.m_edge.m_index] & ~static_cast<uintptr_t>(3));
			const int32_t iPlane = pTriangle->m_plane.m_index;
			const int32_t iNeighbourPlane = pNeighbour->m_plane.m_index;
			if (iPlane == iNeighbourPlane) {
				continue;
			}
			const hkVector4& neighbourPlane = m_planes.m_data[iNeighbourPlane];
			const hkVector4& plane = m_planes.m_data[iPlane];
			// The two dots' product in SSE, compared with zero on the x87.
			const __m128 d0 = _mm_mul_ps(projectionPlane.m_quad, neighbourPlane.m_quad);
			const __m128 dot0 = _mm_add_ss(_mm_shuffle_ps(d0, d0, 0xAA), _mm_add_ss(_mm_shuffle_ps(d0, d0, 0x55), d0));
			const __m128 d1 = _mm_mul_ps(plane.m_quad, projectionPlane.m_quad);
			const __m128 dot1 = _mm_add_ss(_mm_shuffle_ps(d1, d1, 0xAA), _mm_add_ss(_mm_shuffle_ps(d1, d1, 0x55), d1));
			const float fFacing = _mm_cvtss_f32(_mm_mul_ss(dot1, dot0));
			if (!(fFacing <= 0.0f)) {
				continue;
			}
			hkVector4 edge;
			edge.setCross(plane, neighbourPlane);
			hkVector4 axis0;
			axis0.setCross(projectionPlane, edge);
			const __m128 s0 = _mm_mul_ps(axis0.m_quad, axis0.m_quad);
			const float fLengthSquared0 =
				_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(s0, s0, 0xAA), _mm_add_ss(_mm_shuffle_ps(s0, s0, 0x55), s0)));
			if (fLengthSquared0 == 0.0f) {
				continue;
			}
			const float fLength0 = static_cast<float>(std::sqrt(fLengthSquared0));
			const float fInverse0 = 1.0f / fLength0;
			axis0.m_quad = _mm_mul_ps(_mm_set1_ps(fInverse0), axis0.m_quad);
			hkVector4 axis1;
			axis1.setCross(axis0, projectionPlane);
			const __m128 s1 = _mm_mul_ps(axis1.m_quad, axis1.m_quad);
			const float fLengthSquared1 =
				_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(s1, s1, 0xAA), _mm_add_ss(_mm_shuffle_ps(s1, s1, 0x55), s1)));
			if (fLengthSquared1 == 0.0f) {
				continue;
			}
			const float fLength1 = static_cast<float>(std::sqrt(fLengthSquared1));
			const float fInverse1 = 1.0f / fLength1;
			axis1.m_quad = _mm_mul_ps(_mm_set1_ps(fInverse1), axis1.m_quad);
			const float fExtent0 = static_cast<float>(getSpan(axis0, &pSlotHigh[0], &pSlotHigh[1]));
			const float fExtent1 = static_cast<float>(getSpan(axis1, &pSlotLow[0], &pSlotLow[1]));
			const float fArea = fExtent1 * fExtent0;
			if (fArea < fBestArea) {
				fBestArea = fArea;
				bestAxis0 = axis0;
				fBestExtent0 = fExtent0;
				bestAxis1 = axis1;
				fBestExtent1 = fExtent1;
				fCenter0 = static_cast<float>((pSlotHigh[1] + pSlotHigh[0]) * 0.5);
				fCenter1 = static_cast<float>((pSlotLow[1] + pSlotLow[0]) * 0.5);
			}
		}
	} else if (m_dimensions == 2) {
		for (int32_t i = 0; i < m_planes.m_size; ++i) {
			const hkVector4 axis0 = m_planes.m_data[i];
			hkVector4 axis1;
			axis1.setCross(projectionPlane, axis0);
			const __m128 s1 = _mm_mul_ps(axis1.m_quad, axis1.m_quad);
			const float fLengthSquared1 =
				_mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(s1, s1, 0xAA), _mm_add_ss(_mm_shuffle_ps(s1, s1, 0x55), s1)));
			if (fLengthSquared1 == 0.0f) {
				continue;
			}
			const float fLength1 = static_cast<float>(std::sqrt(fLengthSquared1));
			const float fInverse1 = 1.0f / fLength1;
			axis1.m_quad = _mm_mul_ps(_mm_set1_ps(fInverse1), axis1.m_quad);
			const float fExtent0 = static_cast<float>(getSpan(axis0, &pSlotLow[0], &pSlotLow[1]));
			const float fExtent1 = static_cast<float>(getSpan(axis1, &pSlotHigh[0], &pSlotHigh[1]));
			const float fArea = fExtent1 * fExtent0;
			if (fArea < fBestArea) {
				fBestArea = fArea;
				bestAxis0 = axis0;
				fBestExtent0 = fExtent0;
				bestAxis1 = axis1;
				fBestExtent1 = fExtent1;
				fCenter0 = static_cast<float>((pSlotLow[1] + pSlotLow[0]) * 0.5);
				fCenter1 = static_cast<float>((pSlotHigh[1] + pSlotHigh[0]) * 0.5);
			}
		}
	}
	const float fHalfExtent0 = static_cast<float>(fBestExtent0 * 0.5);
	const float fHalfExtent1 = static_cast<float>(0.5 * fBestExtent1);
	hkVector4& translation = worldTransform.m_translation;
	translation.m_quad = _mm_mul_ps(_mm_set1_ps(fCenter0), bestAxis0.m_quad);
	translation.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(fCenter1), bestAxis1.m_quad), translation.m_quad);
	const __m128 w = _mm_shuffle_ps(projectionPlane.m_quad, projectionPlane.m_quad, 0xFF);
	translation.m_quad = _mm_sub_ps(translation.m_quad, _mm_mul_ps(w, projectionPlane.m_quad));
	worldTransform.m_rotation.m_col1 = bestAxis1;
	worldTransform.m_rotation.m_col0 = bestAxis0;
	worldTransform.m_rotation.m_col2 = projectionPlane;
	halfExtents.set(fHalfExtent0, fHalfExtent1, 0.0f, 0.0f);
	return fBestArea;
}

// Below 4 vertices to keep, 4 are kept. Each round: every vertex's fan volume, the sum of the determinants of the
// tetrahedra about it (the vertices' integer coordinates back in world space), is pushed with the vertex's first edge
// found (m_scratch marks a vertex done); the edges are sorted, largest volume first; the vertices of the smallest
// volumes are removed, up to the first one sharing a neighbour with a removed one (m_scratch counts each vertex's
// removed neighbours); the hull is rebuilt from the remaining vertices. Returns false when the build fails (-1). A
// triangle's link is hkgpTopology's untyped Uid word (the neighbour's address, its edge index in the low two bits), as
// in nextHorizon: hence the casts from the integer, and the edge compare on address plus index.
bool hkgpConvexHullImpl::decimateVertices(int32_t numVerticesToKeep, bool containment) {
	if (numVerticesToKeep < 4) {
		numVerticesToKeep = 4;
	}
	if (m_mesh.m_vertices.m_numUsed <= numVerticesToKeep) {
		return true;
	}
	const hkBool hadIndexing = m_hasIndexing;
	const hkBool hadMassProperties = m_hasMassProperties;
	using SourceArray = hkArray<hkVector4, hkContainerHeapAllocator>;
	using EdgeArray = hkArray<SortedEdge, hkContainerHeapAllocator>;
	// EXE-INLINED none: hkArray<hkVector4>::hkArray
	FNVD_INLINED_LOCAL(SourceArray, sources, std::nullopt, std::nullopt);
	sources.m_data = nullptr;
	sources.m_size = 0;
	sources.m_capacityAndFlags = SourceArray::DONT_DEALLOCATE_FLAG;
	// EXE-INLINED none: hkArray<SortedEdge>::hkArray
	FNVD_INLINED_LOCAL(EdgeArray, edges, std::nullopt, std::nullopt);
	edges.m_data = nullptr;
	edges.m_size = 0;
	edges.m_capacityAndFlags = EdgeArray::DONT_DEALLOCATE_FLAG;
	if (m_dimensions != 3) {
		alignas(16) char acBuffer[512];
		alignas(16) hkErrStream kStream(acBuffer, 512);
		kStream << "Invalid dimension for this operation (" << static_cast<int32_t>(3) << " required)";
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x79F9D888, acBuffer,
				".\\GeometryProcessing\\ConvexHull\\hkgpConvexHull.cpp", 340)) {
			__builtin_debugtrap();
		}
	}
	if (containment) {
		// The source positions, for ensureContainment.
		const int32_t iNumVertices = m_mesh.m_vertices.m_numUsed;
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		const int32_t iOldSize = sources.m_size;
		int32_t iSize = iOldSize + iNumVertices;
		const int32_t iCapacity = sources.m_capacityAndFlags & SourceArray::CAPACITY_MASK;
		if (iCapacity < iSize) {
			if (iSize < iCapacity + iCapacity) {
				iSize = iCapacity + iCapacity;
			}
			hkArrayUtil::_reserve(*pHeap, &sources, iSize, sizeof(hkVector4));
		}
		sources.m_size += iNumVertices;
		hkVector4* pSource = sources.m_data + iOldSize;
		for (Vertex* pVertex = m_mesh.m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
			*pSource = pVertex->m_source;
			pSource++;
		}
	}
	{
		// EXE-INLINED none: hkArray<SortedEdge>::reserveExactly
		const int32_t iNumVertices = m_mesh.m_vertices.m_numUsed;
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		if ((edges.m_capacityAndFlags & EdgeArray::CAPACITY_MASK) < iNumVertices) {
			hkArrayUtil::_reserve(*pHeap, &edges, iNumVertices, sizeof(SortedEdge));
		}
	}
	while (m_mesh.m_vertices.m_numUsed > numVerticesToKeep) {
		edges.m_size = 0;
		for (Vertex* pVertex = m_mesh.m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
			pVertex->m_scratch = 0.0f;
		}
		for (Triangle* pStart = m_mesh.m_triangles.m_used; pStart; pStart = pStart->m_next) {
			for (int32_t i = 0; i < 3; i++) {
				if (0.0f != pStart->m_vertices[i]->m_scratch) {
					continue;
				}
				// The fan of the vertex: forward through each edge's previous edge's link until the start edge
				// comes back, else backward from the start edge's link until a naked edge.
				float fVolume = 0.0f;
				Triangle* pTriangle = pStart;
				int32_t j = i;
				bool bForward = true;
				for (;;) {
					const hkQuadReal scaleInv = m_scaleInv.m_quad;
					const hkQuadReal origin = m_origin.m_quad;
					const Vertex* pV0 = pTriangle->m_vertices[j];
					const Vertex* pV1 = pTriangle->m_vertices[(9 >> (2 * j)) & 3];
					const Vertex* pV2 = pTriangle->m_vertices[(18 >> (2 * j)) & 3];
					const uintptr_t link = pTriangle->m_links[j];
					const Triangle* pOpposite = reinterpret_cast<Triangle*>(link & ~static_cast<uintptr_t>(3));
					const Vertex* pV3 = pOpposite->m_vertices[(18 >> (2 * static_cast<int32_t>(link & 3))) & 3];
					// EXE-INLINED 0xD74E80: hkgpConvexHull's vertex position
					const hkQuadReal p1 =
						_mm_add_ps(_mm_mul_ps(_mm_unpacklo_ps(_mm_unpacklo_ps(_mm_set_ss(static_cast<float>(pV1
																					  ->m_x[0])),
																  _mm_set_ss(static_cast<float>(pV1->m_x[2]))),
												  _mm_unpacklo_ps(_mm_set_ss(static_cast<float>(pV1->m_x[1])),
													  _mm_set_ss(0.0f))),
									   scaleInv),
							origin);
					hkMatrix3 kMatrix;
					kMatrix.m_col0.m_quad =
						_mm_sub_ps(_mm_add_ps(_mm_mul_ps(_mm_unpacklo_ps(_mm_unpacklo_ps(_mm_set_ss(static_cast<
																							 float>(pV0->m_x[0])),
																			 _mm_set_ss(static_cast<float>(pV0
																					 ->m_x[2]))),
															 _mm_unpacklo_ps(_mm_set_ss(static_cast<float>(pV0
																					 ->m_x[1])),
																 _mm_set_ss(0.0f))),
												  scaleInv),
									   origin),
							p1);
					kMatrix.m_col1.m_quad =
						_mm_sub_ps(_mm_add_ps(_mm_mul_ps(_mm_unpacklo_ps(_mm_unpacklo_ps(_mm_set_ss(static_cast<
																							 float>(pV2->m_x[0])),
																			 _mm_set_ss(static_cast<float>(pV2
																					 ->m_x[2]))),
															 _mm_unpacklo_ps(_mm_set_ss(static_cast<float>(pV2
																					 ->m_x[1])),
																 _mm_set_ss(0.0f))),
												  scaleInv),
									   origin),
							p1);
					kMatrix.m_col2.m_quad =
						_mm_sub_ps(_mm_add_ps(_mm_mul_ps(_mm_unpacklo_ps(_mm_unpacklo_ps(_mm_set_ss(static_cast<
																							 float>(pV3->m_x[0])),
																			 _mm_set_ss(static_cast<float>(pV3
																					 ->m_x[2]))),
															 _mm_unpacklo_ps(_mm_set_ss(static_cast<float>(pV3
																					 ->m_x[1])),
																 _mm_set_ss(0.0f))),
												  scaleInv),
									   origin),
							p1);
					const hkSimdReal det = kMatrix.getDeterminant();
					const float fDet = det.m_real[0];
					fVolume = fDet + fVolume;
					if (bForward) {
						const uintptr_t next = pTriangle->m_links[(18 >> (2 * j)) & 3];
						pTriangle = reinterpret_cast<Triangle*>(next & ~static_cast<uintptr_t>(3));
						j = static_cast<int32_t>(next & 3);
						if (pTriangle) {
							if (reinterpret_cast<uintptr_t>(pTriangle) + j == reinterpret_cast<uintptr_t>(pStart) + i) {
								break;
							}
							continue;
						}
						bForward = false;
						const uintptr_t back = pStart->m_links[i];
						j = (9 >> (2 * static_cast<int32_t>(back & 3))) & 3;
						pTriangle = reinterpret_cast<Triangle*>(back & ~static_cast<uintptr_t>(3));
					} else {
						const uintptr_t next = pTriangle->m_links[j];
						j = (9 >> (2 * static_cast<int32_t>(next & 3))) & 3;
						pTriangle = reinterpret_cast<Triangle*>(next & ~static_cast<uintptr_t>(3));
					}
					if (!pTriangle) {
						break;
					}
				}
				// EXE-INLINED none: hkArray<SortedEdge>::pushBack
				hkMemoryAllocator* pMore = hkMemoryRouter::getInstance().m_heap;
				if (edges.m_size == (edges.m_capacityAndFlags & EdgeArray::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(*pMore, &edges, sizeof(SortedEdge));
				}
				SortedEdge& kEdge = edges.m_data[edges.m_size];
				kEdge.m_value = fVolume;
				edges.m_size++;
				kEdge.m_data.m_triangle = pStart;
				kEdge.m_data.m_index = static_cast<uint32_t>(i);
				pStart->m_vertices[i]->m_scratch = 1.0f;
			}
		}
		for (Vertex* pVertex = m_mesh.m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
			pVertex->m_scratch = 0.0f;
		}
		if (edges.m_size > 1) {
			hkAlgorithm::quickSortRecursive(edges.m_data, 0, edges.m_size - 1, 0);
		}
		const int32_t iNumToRemove = m_mesh.m_vertices.m_numUsed - numVerticesToKeep;
		int32_t iNumRemoved = 0;
		if (iNumToRemove > 0) {
			for (; iNumRemoved < iNumToRemove; iNumRemoved++) {
				Triangle* pStart = edges.m_data[iNumRemoved].m_data.m_triangle;
				const int32_t i = static_cast<int32_t>(edges.m_data[iNumRemoved].m_data.m_index);
				bool bShared = false;
				Vertex* pCenter = pStart->m_vertices[i];
				pCenter->m_scratch = pCenter->m_scratch + 1.0f;
				// Each neighbour of the vertex, by the same walk as the fan above.
				Triangle* pTriangle = pStart;
				int32_t j = i;
				bool bForward = true;
				for (;;) {
					Vertex* pNeighbour = pTriangle->m_vertices[(9 >> (2 * j)) & 3];
					const float fCount = pNeighbour->m_scratch + 1.0f;
					pNeighbour->m_scratch = fCount;
					if (fCount > 1.0f) {
						bShared = true;
					}
					if (bForward) {
						const uintptr_t next = pTriangle->m_links[(18 >> (2 * j)) & 3];
						pTriangle = reinterpret_cast<Triangle*>(next & ~static_cast<uintptr_t>(3));
						j = static_cast<int32_t>(next & 3);
						if (pTriangle) {
							if (reinterpret_cast<uintptr_t>(pTriangle) + j == reinterpret_cast<uintptr_t>(pStart) + i) {
								break;
							}
							continue;
						}
						bForward = false;
						const uintptr_t back = pStart->m_links[i];
						j = (9 >> (2 * static_cast<int32_t>(back & 3))) & 3;
						pTriangle = reinterpret_cast<Triangle*>(back & ~static_cast<uintptr_t>(3));
					} else {
						const uintptr_t next = pTriangle->m_links[j];
						j = (9 >> (2 * static_cast<int32_t>(next & 3))) & 3;
						pTriangle = reinterpret_cast<Triangle*>(next & ~static_cast<uintptr_t>(3));
					}
					if (!pTriangle) {
						break;
					}
				}
				if (bShared) {
					break;
				}
			}
			for (int32_t k = 0; k < iNumRemoved; k++) {
				// EXE-INLINED none: hkgpAbstractMesh::List::release
				Vertex* pVertex = edges.m_data[k].m_data.m_triangle->m_vertices[edges.m_data[k].m_data.m_index];
				Vertex* pNext = pVertex->m_next;
				Vertex* pPrev = pVertex->m_prev;
				if (pNext) {
					pNext->m_prev = pPrev;
				}
				if (pPrev) {
					pPrev->m_next = pNext;
				} else {
					m_mesh.m_vertices.m_used = pNext;
				}
				m_mesh.m_vertices.m_numUsed--;
				m_mesh.m_vertices.m_allocator.release(pVertex);
			}
		}
		// The hull again, from the remaining vertices.
		const int32_t iNumVertices = m_mesh.m_vertices.m_numUsed;
		alignas(16) FNVD_INLINED_LOCAL(SourceArray, points, std::nullopt, std::nullopt);
		points.m_data = nullptr;
		points.m_size = 0;
		points.m_capacityAndFlags = SourceArray::DONT_DEALLOCATE_FLAG;
		// EXE-INLINED none: hkArray<hkVector4>::expandBy
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		const int32_t iOldSize = points.m_size;
		int32_t iSize = iOldSize + iNumVertices;
		const int32_t iCapacity = points.m_capacityAndFlags & SourceArray::CAPACITY_MASK;
		if (iCapacity < iSize) {
			if (iSize < iCapacity + iCapacity) {
				iSize = iCapacity + iCapacity;
			}
			hkArrayUtil::_reserve(*pHeap, &points, iSize, sizeof(hkVector4));
		}
		points.m_size += iNumVertices;
		hkVector4* pPoint = points.m_data + iOldSize;
		for (Vertex* pVertex = m_mesh.m_vertices.m_used; pVertex; pVertex = pVertex->m_next) {
			*pPoint = pVertex->m_source;
			pPoint++;
		}
		if (build(points.m_data, points.m_size, 0, nullptr) == -1) {
			// EXE-INLINED none: hkArray::~hkArray
			pHeap = hkMemoryRouter::getInstance().m_heap;
			points.m_size = 0;
			if ((points.m_capacityAndFlags & SourceArray::DONT_DEALLOCATE_FLAG) == 0) {
				pHeap->bufFree(points.m_data,
					static_cast<int32_t>(sizeof(hkVector4)) * (points.m_capacityAndFlags & SourceArray::CAPACITY_MASK));
			}
			pHeap = hkMemoryRouter::getInstance().m_heap;
			edges.m_size = 0;
			if ((edges.m_capacityAndFlags & EdgeArray::DONT_DEALLOCATE_FLAG) == 0) {
				pHeap->bufFree(edges.m_data,
					static_cast<int32_t>(sizeof(SortedEdge)) * (edges.m_capacityAndFlags & EdgeArray::CAPACITY_MASK));
			}
			pHeap = hkMemoryRouter::getInstance().m_heap;
			sources.m_size = 0;
			if ((sources.m_capacityAndFlags & SourceArray::DONT_DEALLOCATE_FLAG) == 0) {
				pHeap->bufFree(sources.m_data,
					static_cast<int32_t>(sizeof(hkVector4)) *
						(sources.m_capacityAndFlags & SourceArray::CAPACITY_MASK));
			}
			return false;
		}
		// EXE-INLINED none: hkArray<hkVector4>::~hkArray
		pHeap = hkMemoryRouter::getInstance().m_heap;
		points.m_size = 0;
		if (points.m_capacityAndFlags >= 0) {
			pHeap->bufFree(points.m_data,
				static_cast<int32_t>(sizeof(hkVector4)) * (points.m_capacityAndFlags & SourceArray::CAPACITY_MASK));
		}
	}
	if (containment) {
		this->ensureContainment(sources);
	}
	if (hadIndexing.m_bool) {
		buildIndices();
	}
	if (hadMassProperties.m_bool) {
		buildMassProperties();
	}
	// EXE-INLINED none: hkArray::~hkArray
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	edges.m_size = 0;
	if (edges.m_capacityAndFlags >= 0) {
		pHeap->bufFree(edges.m_data,
			static_cast<int32_t>(sizeof(SortedEdge)) * (edges.m_capacityAndFlags & EdgeArray::CAPACITY_MASK));
	}
	pHeap = hkMemoryRouter::getInstance().m_heap;
	sources.m_size = 0;
	if (sources.m_capacityAndFlags >= 0) {
		pHeap->bufFree(sources.m_data,
			static_cast<int32_t>(sizeof(hkVector4)) * (sources.m_capacityAndFlags & SourceArray::CAPACITY_MASK));
	}
	return true;
}

// Quicksort over triangle pointers keyed by their plane's index (Triangle::SortByPlaneId, signed compares): sorts
// apArr[aiLow..aiHigh] in place (inclusive bounds), recursing on the lower part and looping on the upper. The pivot's
// key is read through the pivot pointer on each pass. aiLess is the comparator (an empty class), passed on to the
// recursive call.
int32_t sortTrianglesByPlaneIndex(hkgpConvexHullImpl::Triangle** apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess) {
	for (;;) {
		int32_t i = aiLow;
		int32_t j = aiHigh;
		const hkgpConvexHullImpl::Triangle* pPivot = apArr[(aiLow + aiHigh) >> 1];
		do {
			const int32_t iPivotKey = pPivot->m_plane.m_index;
			while (apArr[i]->m_plane.m_index < iPivotKey) {
				++i;
			}
			while (iPivotKey < apArr[j]->m_plane.m_index) {
				--j;
			}
			if (j < i) {
				break;
			}
			if (j != i) {
				hkgpConvexHullImpl::Triangle* pTriangle = apArr[i];
				apArr[i] = apArr[j];
				apArr[j] = pTriangle;
			}
			--j;
			++i;
		} while (i <= j);
		if (aiLow < j) {
			sortTrianglesByPlaneIndex(apArr, aiLow, j, aiLess);
		}
		if (i >= aiHigh) {
			return aiHigh;
		}
		aiLow = i;
	}
}

// The SDK's quickSortRecursive with the comparison written out: points by squared distance to origin, the middle
// element the pivot, recursing on the lower part and looping on the upper one.
void sortByDistance(hkVector4* points, int32_t lo, int32_t hi, const hkVector4* origin) {
	const auto distance2 = [](__m128 a, __m128 b) {
		const __m128 d = _mm_sub_ps(a, b);
		const __m128 sq = _mm_mul_ps(d, d);
		return _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(sq, sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(sq, sq, 0x55), sq)));
	};
	while (true) {
		int32_t i = lo;
		int32_t j = hi;
		const __m128 pivot = points[(lo + hi) >> 1].m_quad;
		do {
			const __m128 o = origin->m_quad;
			const float fPivot = distance2(pivot, o);
			// Each scan stops at an equal or unordered distance.
			while (fPivot > distance2(points[i].m_quad, o)) {
				++i;
			}
			while (distance2(points[j].m_quad, o) > fPivot) {
				--j;
			}
			if (j < i) {
				break;
			}
			if (j != i) {
				const __m128 swap = points[j].m_quad;
				points[j].m_quad = points[i].m_quad;
				points[i].m_quad = swap;
			}
			--j;
			++i;
		} while (i <= j);
		if (lo < j) {
			sortByDistance(points, lo, j, origin);
		}
		if (i >= hi) {
			return;
		}
		lo = i;
	}
}

// hkgpAbstractMeshDefinitions::List<...>::Item::operator= copies nothing (hkgpAbstractMesh.h), so element copies copy
// only the vertex's own members. Item here declares no such operator, so the copies are written member by member.
int32_t hkAlgorithm::quickSortRecursive(hkgpConvexHullImpl::Vertex* apArr, int32_t aiLow, int32_t aiHigh,
	uint8_t aiLess) {
	less<hkgpConvexHullImpl::Vertex> cmpLess;
	int32_t d = aiLow;
	for (;;) {
		int32_t i = aiHigh;
		int32_t j = d;
		hkgpConvexHullImpl::Vertex str;
		const hkgpConvexHullImpl::Vertex& rMid = apArr[(d + aiHigh) >> 1];
		str.m_source = rMid.m_source;
		str.m_x[0] = rMid.m_x[0];
		str.m_x[1] = rMid.m_x[1];
		str.m_x[2] = rMid.m_x[2];
		str.m_scratch = rMid.m_scratch;
		str.m_refs = rMid.m_refs;
		str.m_index = rMid.m_index;
		do {
			while (cmpLess(apArr[j], str)) {
				j++;
			}
			while (cmpLess(str, apArr[i])) {
				i--;
			}
			if (i < j) {
				break;
			}
			if (i != j) {
				hkgpConvexHullImpl::Vertex zal;
				hkgpConvexHullImpl::Vertex& pi = apArr[i];
				hkgpConvexHullImpl::Vertex& pj = apArr[j];
				zal.m_source = pi.m_source;
				zal.m_x[0] = pi.m_x[0];
				zal.m_x[1] = pi.m_x[1];
				zal.m_x[2] = pi.m_x[2];
				zal.m_scratch = pi.m_scratch;
				zal.m_refs = pi.m_refs;
				zal.m_index = pi.m_index;
				pi.m_source = pj.m_source;
				pi.m_x[0] = pj.m_x[0];
				pi.m_x[1] = pj.m_x[1];
				pi.m_x[2] = pj.m_x[2];
				pi.m_scratch = pj.m_scratch;
				pi.m_refs = pj.m_refs;
				pi.m_index = pj.m_index;
				pj.m_source = zal.m_source;
				pj.m_x[0] = zal.m_x[0];
				pj.m_x[1] = zal.m_x[1];
				pj.m_x[2] = zal.m_x[2];
				pj.m_scratch = zal.m_scratch;
				pj.m_refs = zal.m_refs;
				pj.m_index = zal.m_index;
			}
			i--;
			j++;
		} while (j <= i);
		int32_t iResult = d;
		if (d < i) {
			iResult = quickSortRecursive(apArr, d, i, aiLess);
		}
		if (j >= aiHigh) {
			return iResult;
		}
		d = j;
	}
}

// Sorts decimateVertices's edges apArr[aiLow..aiHigh] in place (inclusive bounds), largest key first (ORDER -1). The
// pivot is copied whole.
void hkAlgorithm::quickSortRecursive(hkgpConvexHullImpl::SortedEdge* apArr, int32_t aiLow, int32_t aiHigh,
	int32_t aiLess) {
	less<hkgpConvexHullImpl::SortedEdge> cmpLess;
	int32_t d = aiLow;
	for (;;) {
		int32_t i = aiHigh;
		int32_t j = d;
		const hkgpConvexHullImpl::SortedEdge pivot = apArr[(d + aiHigh) >> 1];
		do {
			while (cmpLess(apArr[j], pivot)) {
				j++;
			}
			while (cmpLess(pivot, apArr[i])) {
				i--;
			}
			if (i < j) {
				break;
			}
			if (i != j) {
				hkgpConvexHullImpl::SortedEdge zal = apArr[i];
				apArr[i] = apArr[j];
				apArr[j] = zal;
			}
			i--;
			j++;
		} while (j <= i);
		if (d < i) {
			quickSortRecursive(apArr, d, i, aiLess);
		}
		if (j >= aiHigh) {
			return;
		}
		d = j;
	}
}

void hkAlgorithm::quickSortRecursive(hkgpConvexHullImpl::SortedTriangle* apArr, int32_t aiLow, int32_t aiHigh,
	int32_t aiLess) {
	less<hkgpConvexHullImpl::SortedTriangle> cmpLess;
	int32_t d = aiLow;
	for (;;) {
		int32_t i = aiHigh;
		int32_t j = d;
		const hkgpConvexHullImpl::SortedTriangle str = apArr[(d + aiHigh) >> 1];
		do {
			while (cmpLess(apArr[j], str)) {
				j++;
			}
			while (cmpLess(str, apArr[i])) {
				i--;
			}
			if (i < j) {
				break;
			}
			if (i != j) {
				hkgpConvexHullImpl::SortedTriangle zal = apArr[i];
				apArr[i] = apArr[j];
				apArr[j] = zal;
			}
			i--;
			j++;
		} while (j <= i);
		if (d < i) {
			quickSortRecursive(apArr, d, i, aiLess);
		}
		if (j >= aiHigh) {
			return;
		}
		d = j;
	}
}

void hkPseudoRandomGenerator::getRandomVector11(hkVector4& vecOut) {
	uint32_t v3 = 0x19660D * m_current + 0x3C6EF35F;
	m_current = v3;
	float v4 = static_cast<double>(v3) * 2.3283064365386963e-10;
	vecOut.m_quad[0] = v4 * 2.0 - 1.0;
	uint32_t vecOuta = 0x19660D * m_current + 0x3C6EF35F;
	m_current = vecOuta;
	float vecOutb = static_cast<double>(vecOuta) * 2.3283064365386963e-10;
	vecOut.m_quad[1] = vecOutb * 2.0 - 1.0;
	uint32_t vecOutc = 0x19660D * m_current + 0x3C6EF35F;
	m_current = vecOutc;
	float vecOutd = static_cast<double>(vecOutc) * 2.3283064365386963e-10;
	vecOut.m_quad[2] = vecOutd * 2.0 - 1.0;
	uint32_t vecOute = 0x19660D * m_current + 0x3C6EF35F;
	m_current = vecOute;
	float vecOutf = 2.3283064365386963e-10 * static_cast<double>(vecOute);
	vecOut.m_quad[3] = 2.0 * vecOutf - 1.0;
}

// The smaller of two floats; y when they are equal or either is NaN.
float hkMath::min2(float x, float y) {
	return x < y ? x : y;
}

// hkHistogram's entries sort by value; a NaN orders as not less.
hkBool operator<(const hkEntry& a, const hkEntry& b) {
	return a.m_value < b.m_value;
}
