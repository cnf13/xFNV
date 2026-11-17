#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeAabbCastUtils.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/hkpBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayBundleCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppLongRayVirtualMachine.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppObbVirtualMachine.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppRayBundleVirtualMachine.h"

#include <bit>

static void finishLoadedObjecthkpMoppBvTreeShape(void* p, int32_t finishing);
static void cleanupLoadedObjecthkpMoppBvTreeShape(void* p);
static const void* getVtablehkpMoppBvTreeShape();

// The class records' member tables.
using hkMoppBvTreeShapeBaseClassMemberArray = const hkClassMember[4];
static hkMoppBvTreeShapeBaseClassMemberArray hkMoppBvTreeShapeBaseClass_Members{
    {"code",
     &hkpMoppCodeClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     0,
     0x14,
     nullptr},
    {"moppData",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x18,
     nullptr},
    {"moppDataSize",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x1C,
     nullptr},
    {"codeInfoCopy",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x20,
     nullptr}
};
using hkpMoppBvTreeShapeClassMemberArray = const hkClassMember[2];
static hkpMoppBvTreeShapeClassMemberArray hkpMoppBvTreeShapeClass_Members{
    {"child",
     &hkpSingleShapeContainerClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     0,
     0x30,
     nullptr},
    {"childSize",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x38,
     nullptr}
};
hkClass hkMoppBvTreeShapeBaseClass(
    "hkMoppBvTreeShapeBase",
    &hkpBvTreeShapeClass,
    0x30,
    nullptr,
    0,
    nullptr,
    0,
    hkMoppBvTreeShapeBaseClass_Members,
    4,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkpMoppBvTreeShapeClass(
    "hkpMoppBvTreeShape",
    &hkMoppBvTreeShapeBaseClass,
    0x40,
    nullptr,
    0,
    nullptr,
    0,
    hkpMoppBvTreeShapeClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpMoppBvTreeShapeTypeInfo{
    "hkpMoppBvTreeShape",          finishLoadedObjecthkpMoppBvTreeShape, cleanupLoadedObjecthkpMoppBvTreeShape,
    getVtablehkpMoppBvTreeShape(), &typeid(hkpMoppBvTreeShape),          sizeof(hkpMoppBvTreeShape)
};

static const void* getVtablehkpMoppBvTreeShape() {
    return hkGetVtableOf<hkpMoppBvTreeShape>();
}

static void cleanupLoadedObjecthkpMoppBvTreeShape(void* p) {
    static_cast<hkpMoppBvTreeShape*>(p)->~hkpMoppBvTreeShape();
}

static void finishLoadedObjecthkpMoppBvTreeShape(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpMoppBvTreeShape(kFlag);
    }
}

// Holds a reference on the code and caches its data and info.
hkMoppBvTreeShapeBase::hkMoppBvTreeShapeBase(hkpShapeType type, const hkpMoppCode* a2)
  : hkpBvTreeShape(type, BVTREE_MOPP), m_code(a2), m_moppData(a2->m_data.m_data), m_moppDataSize(a2->m_data.m_size),
    m_codeInfoCopy(a2->m_info.m_offset) {
    a2->addReference();
}

// The loaded code stays as loaded; a finished object is a MOPP tree again.
hkMoppBvTreeShapeBase::hkMoppBvTreeShapeBase(hkFinishLoadedObjectFlag flag) : hkpBvTreeShape(flag) {
    if (flag.m_finishing) {
        m_bvTreeType.m_storage = BVTREE_MOPP;
    }
}

hkMoppBvTreeShapeBase::~hkMoppBvTreeShapeBase() {
    m_code->removeReference();
}

// The SDK's: the oriented box is queried through the MOPP code, the hits collected into the caller's array.
void hkMoppBvTreeShapeBase::queryObb(
    const hkTransform& obbToMopp,
    const hkVector4& extent,
    float tolerance,
    hkArray<uint32_t, hkContainerHeapAllocator>& hits
) const {
    alignas(16) hkpMoppObbVirtualMachine machine;
    // The tolerance is copied to the call as a float load and store, not an integer copy.
    float fTolerance = tolerance;
    // The hits array's storage is treated as untyped 32-bit IDs: the shape keys are handed to the machine as its
    // primitive infos (one 32-bit ID each), as queryAabb does.
    machine.queryObb(
        m_code, obbToMopp, extent, fTolerance,
        reinterpret_cast<hkArray<hkpMoppPrimitiveInfo, hkContainerHeapAllocator>*>(&hits)
    );
}

void hkMoppBvTreeShapeBase::queryAabb(const hkAabb& aabb, hkArray<uint32_t, hkContainerHeapAllocator>& hits) const {
    hkVector4 v3;
    uint32_t v6;
    hkAabb a2;
    alignas(16) hkpMoppObbVirtualMachine v8;
    v3.m_quad = m_codeInfoCopy.m_quad;
    double v4 = 16777216.0 / m_codeInfoCopy.m_quad[3];
    // MAYBE-INLINED 0xD543A6: a write opening hkAabb's uses: may be its inlined constructor
    a2.m_min.m_quad = _mm_max_ps(v3.m_quad, aabb.m_min.m_quad);
    *reinterpret_cast<float*>(&v6) = v4;
    const hkpMoppCode* m_code = this->m_code;
    a2.m_max.m_quad = _mm_min_ps(
        _mm_add_ps(
            _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v6)), _mm_set_ss(std::bit_cast<float>(v6)), 0), v3.m_quad
        ),
        aabb.m_max.m_quad
    );
    // The SDK passes the shape-key hits as the machine's primitive infos (one 32-bit ID each):
    // reinterpret_cast<hkArray<hkpMoppPrimitiveInfo>*>(&hits).
    v8.queryAabb(m_code, a2, reinterpret_cast<hkArray<hkpMoppPrimitiveInfo, hkContainerHeapAllocator>*>(&hits));
}

// The SDK's: the box, clipped to the code's range, is queried into an array over the caller's key buffer, and the
// number of hits found is returned.
uint32_t hkMoppBvTreeShapeBase::queryAabbImpl(const hkAabb& aAabb, uint32_t* hits, int32_t maxNumKeys) const {
    alignas(16) hkpMoppObbVirtualMachine machine;
    // A MOPP code over this shape's code bytes: only its info, its data array and its build type are set (no vtable or
    // reference count), and no destructor runs on it.
    FNVD_INLINED_LOCAL(hkpMoppCode, code, std::nullopt, std::nullopt);
    code.m_info.m_offset = m_codeInfoCopy;
    code.m_data.m_data = const_cast<uint8_t*>(m_moppData);
    code.m_data.m_size = m_moppDataSize;
    code.m_data.m_capacityAndFlags = m_moppDataSize;
    // EXE-INLINED none: hkEnum<E, S>::operator=
    code.m_buildType.m_storage = static_cast<int8_t>(hkpMoppCode::BUILD_NOT_SET);
    // The results array over the caller's buffer, which is treated as untyped memory: the caller passes shape keys, the
    // query writes 4-byte primitive infos into it. The array is 16-aligned.
    // EXE-INLINED 0x62E230
    alignas(16) hkArray<hkpMoppPrimitiveInfo, hkContainerHeapAllocator> primitives;
    primitives.m_data = reinterpret_cast<hkpMoppPrimitiveInfo*>(hits);
    primitives.m_size = 0;
    primitives.m_capacityAndFlags =
        static_cast<int32_t>(maxNumKeys | hkArrayBase<hkpMoppPrimitiveInfo>::DONT_DEALLOCATE_FLAG);
    // The box: its low corner raised to the code's offset, its high corner lowered to the offset plus the code's
    // extent (2^24 integer steps of the scale in the offset's w), computed in double and stored as a float.
    hkAabb aabb;
    aabb.m_min.m_quad = _mm_max_ps(m_codeInfoCopy.m_quad, aAabb.m_min.m_quad);
    float fExtent = 16777216.0 / m_codeInfoCopy.m_quad[3];
    aabb.m_max.m_quad = _mm_min_ps(_mm_add_ps(_mm_set1_ps(fExtent), m_codeInfoCopy.m_quad), aAabb.m_max.m_quad);
    machine.queryAabb(&code, aabb, &primitives);
    // Equivalent to primitives.getSize(), read through the machine's pointer to the array.
    uint32_t uiNumHits = machine.m_primitives_out->m_size;
    // The array's destructor.
    // EXE-INLINED 0xC7B0F0
    primitives.clearAndDeallocate();
    return uiNumHits;
}

// The child container takes a reference on the collection.
hkpMoppBvTreeShape::hkpMoppBvTreeShape(const hkpShapeCollection* collection, const hkpMoppCode* code)
  : hkMoppBvTreeShapeBase(HK_SHAPE_MOPP, code), m_child(collection) {}

// A finished object recaches its code's info and data.
hkpMoppBvTreeShape::hkpMoppBvTreeShape(hkFinishLoadedObjectFlag flag) : hkMoppBvTreeShapeBase(flag), m_child(flag) {
    if (flag.m_finishing == 1) {
        m_type = HK_SHAPE_MOPP;
        m_codeInfoCopy = m_code->m_info.m_offset;
        m_moppData = m_code->m_data.m_data;
        m_moppDataSize = m_code->m_data.m_size;
    }
}

// The child container releases the collection.
hkpMoppBvTreeShape::~hkpMoppBvTreeShape() {}

void hkpMoppBvTreeShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    collector->addReferencedObject("Mesh", m_child.m_childShape, 0);
    collector->addReferencedObject("Mopp", m_code, 0);
    hkReferencedObject::calcContentStatistics(collector, &hkpBvTreeShapeClass);
}

int32_t
hkpMoppBvTreeShape::calcSizeForSpu(const hkpShape::CalcSizeForSpuInput& input, int32_t spuBufferSizeLeft) const {
    int8_t m_storage = m_code->m_buildType.m_storage;
    if (m_storage) {
        if (m_storage != 1) {
            return -1;
        }
        return -1;
    }
    uint32_t v6 = m_child.m_childShape->calcSizeForSpu(input, 256);
    if (v6 > 256) {
        return -1;
    }
    m_childSize = v6;
    return 64;
}

// The MOPP tree's AABB is its child shape's.
void hkpMoppBvTreeShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    m_child.m_childShape->getAabbImpl(localToWorld, tolerance, out);
}

// The SDK's: the ray is walked through the MOPP code over the child collection, one level down the output's shape
// key path; a hit's key at this level is 0.
hkBool hkpMoppBvTreeShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
    HK_TIMER_BEGIN("rcMopp", nullptr);
    // EXE-INLINED none: hkpShapeRayCastOutput::changeLevel
    ++results.m_shapeKeyIndex;
    // A MOPP code over this shape's code bytes: only its info, its data array and its build type are set (no vtable or
    // reference count), and no destructor runs on it.
    FNVD_INLINED_LOCAL(hkpMoppCode, code, std::nullopt, std::nullopt);
    code.m_info.m_offset = m_codeInfoCopy;
    code.m_data.m_data = const_cast<uint8_t*>(m_moppData);
    code.m_data.m_size = m_moppDataSize;
    code.m_data.m_capacityAndFlags = m_moppDataSize;
    hkpMoppLongRayVirtualMachine machine;
    // Initialises the machine's m_ray as hkpShapeRayCastInput's constructor does.
    // EXE-INLINED 0x632CA0
    machine.m_ray.m_filterInfo = 0;
    machine.m_ray.m_rayShapeCollectionFilter = nullptr;
    // EXE-INLINED none: hkEnum<E, S>::operator=
    code.m_buildType.m_storage = static_cast<int8_t>(hkpMoppCode::BUILD_NOT_SET);
    // The child is the shape collection the code was built over; the SDK casts it down.
    const hkpShapeCollection* collection = static_cast<const hkpShapeCollection*>(m_child.m_childShape);
    hkBool result;
    machine.queryLongRay_00DD2DF0(&result, collection, &code, input, results);
    // EXE-INLINED none: hkpShapeRayCastOutput::changeLevel
    --results.m_shapeKeyIndex;
    if (result.m_bool) {
        results.m_shapeKeys[results.m_shapeKeyIndex] = 0;
    }
    HK_TIMER_END();
    return result;
}

// The SDK's: the ray is walked through the MOPP code over the child collection, each hit going to the collector
// with a body for the child below the caller's.
void hkpMoppBvTreeShape::castRayWithCollectorImpl(
    const hkpShapeRayCastInput& input,
    const hkpCdBody& cdBody,
    hkpRayHitCollector& collector
) const {
    HK_TIMER_BEGIN("rcMopp", nullptr);
    const hkpShape* pChild = m_child.m_childShape;
    hkpCdBody body;
    body.m_parent = &cdBody;
    body.m_motion = cdBody.m_motion;
    // A MOPP code over this shape's code bytes: only its info, its data array and its build type are set (no vtable or
    // reference count), and no destructor runs on it.
    FNVD_INLINED_LOCAL(hkpMoppCode, code, std::nullopt, std::nullopt);
    code.m_info.m_offset = m_codeInfoCopy;
    code.m_data.m_data = const_cast<uint8_t*>(m_moppData);
    code.m_data.m_size = m_moppDataSize;
    code.m_data.m_capacityAndFlags = m_moppDataSize;
    hkpMoppLongRayVirtualMachine machine;
    // Initialises the machine's m_ray as hkpShapeRayCastInput's constructor does.
    // EXE-INLINED 0x632CA0
    machine.m_ray.m_filterInfo = 0;
    machine.m_ray.m_rayShapeCollectionFilter = nullptr;
    // EXE-INLINED none: hkEnum<E, S>::operator=
    code.m_buildType.m_storage = static_cast<int8_t>(hkpMoppCode::BUILD_NOT_SET);
    // EXE-INLINED none: hkpCdBody::setShape
    body.m_shape = pChild;
    body.m_shapeKey = 0;
    // The child is the shape collection the code was built over; the SDK casts it down.
    const hkpShapeCollection* collection = static_cast<const hkpShapeCollection*>(pChild);
    machine.queryLongRay(collection, &code, input, body, collector);
    HK_TIMER_END();
}

// The SDK's: the bundle's rays are walked through the MOPP code over the child collection, each ray's output one
// level down its shape key path; a ray's hit key at this level is 0. The lanes hit come back through apResult.
hkVector4Comparison* hkpMoppBvTreeShape::castRayBundleImpl(
    hkVector4Comparison* apResult,
    const hkpShapeRayBundleCastInput& input,
    hkpShapeRayBundleCastOutput& output,
    const hkVector4Comparison& mask
) const {
    HK_TIMER_BEGIN("rcBundleMopp", nullptr);
    // EXE-INLINED none: hkpShapeRayCastOutput::changeLevel
    ++output.m_outputs[0].m_shapeKeyIndex;
    ++output.m_outputs[1].m_shapeKeyIndex;
    ++output.m_outputs[2].m_shapeKeyIndex;
    ++output.m_outputs[3].m_shapeKeyIndex;
    // A MOPP code over this shape's code bytes: only its info, its data array and its build type are set (no vtable or
    // reference count), and no destructor runs on it.
    FNVD_INLINED_LOCAL(hkpMoppCode, code, std::nullopt, std::nullopt);
    code.m_info.m_offset = m_codeInfoCopy;
    code.m_data.m_data = const_cast<uint8_t*>(m_moppData);
    code.m_data.m_size = m_moppDataSize;
    code.m_data.m_capacityAndFlags = m_moppDataSize;
    // EXE-INLINED none: hkEnum<E, S>::operator=
    code.m_buildType.m_storage = static_cast<int8_t>(hkpMoppCode::BUILD_NOT_SET);
    // The child is the shape collection the code was built over; the SDK casts it down.
    const hkpShapeCollection* collection = static_cast<const hkpShapeCollection*>(m_child.m_childShape);
    hkpMoppRayBundleVirtualMachine machine;
    machine.queryRayBundle(apResult, collection, &code, input, output, mask);
    for (int32_t i = 0; i < 4; i++) {
        hkpShapeRayCastOutput& rayOutput = output.m_outputs[i];
        // EXE-INLINED none: hkpShapeRayCastOutput::changeLevel
        --rayOutput.m_shapeKeyIndex;
        // Equivalent to anyIsSet(hkVector4Comparison::getMaskForComponent(i)): tests the lane's bit from
        // axisMaskBits_10C92E0.
        if ((_mm_movemask_ps(apResult->m_mask) & axisMaskBits_10C92E0[i]) != 0) {
            rayOutput.m_shapeKeys[rayOutput.m_shapeKeyIndex] = 0;
        }
    }
    HK_TIMER_END();
    return apResult;
}

const hkClass& hkpMoppBvTreeShape::staticClass() {
    return hkpMoppBvTreeShapeClass;
}
