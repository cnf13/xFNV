#include "Havok/Source/Animation/Animation/Motion/Default/hkaDefaultAnimatedReferenceFrame.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Motion/hkaAnimatedReferenceFrame.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

static void finishLoadedObjecthkaDefaultAnimatedReferenceFrame(void* p, int32_t finishing);
// Havok SDK signature (hkaAnimation.lib, tied by reloc,target).
static const void* getVtablehkaDefaultAnimatedReferenceFrame();
// Samples a track of aiNumSamples transforms spread evenly over
// afDuration at afTime into apResult, which it returns.
static hkQsTransform* sampleReferenceFrame(
    hkQsTransform* apResult,
    float afTime,
    const hkQsTransform* apSamples,
    int32_t aiNumSamples,
    float afDuration
);
static void cleanupLoadedObjecthkaDefaultAnimatedReferenceFrame(void* p);

// The reflection's member table.
using hkaDefaultAnimatedReferenceFrameClass_MembersArray = const hkClassMember[4];
static hkaDefaultAnimatedReferenceFrameClass_MembersArray hkaDefaultAnimatedReferenceFrameClass_Members{
    {"up", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x10, nullptr},
    {"forward", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr},
    {"duration", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x30, nullptr},
    {"referenceFrameSamples",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_VECTOR4},
     0,
     {0},
     0x34,
     nullptr}
};
hkClass hkaDefaultAnimatedReferenceFrameClass(
    "hkaDefaultAnimatedReferenceFrame",
    &hkaAnimatedReferenceFrameClass,
    0x40,
    nullptr,
    0,
    nullptr,
    0,
    hkaDefaultAnimatedReferenceFrameClass_Members,
    4,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkaDefaultAnimatedReferenceFrameTypeInfo{
    "hkaDefaultAnimatedReferenceFrame",
    finishLoadedObjecthkaDefaultAnimatedReferenceFrame,
    cleanupLoadedObjecthkaDefaultAnimatedReferenceFrame,
    getVtablehkaDefaultAnimatedReferenceFrame(),
    &typeid(hkaDefaultAnimatedReferenceFrame),
    sizeof(hkaDefaultAnimatedReferenceFrame)
};

// The type info's finish-loaded-object callback for hkaDefaultAnimatedReferenceFrame: runs its finish constructor.
static void finishLoadedObjecthkaDefaultAnimatedReferenceFrame(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag aFlag;
        aFlag.m_finishing = finishing;
        ::new (p) hkaDefaultAnimatedReferenceFrame(aFlag);
    }
}

static const void* getVtablehkaDefaultAnimatedReferenceFrame() {
    return hkGetVtableOf<hkaDefaultAnimatedReferenceFrame>();
}

// Samples a track of aiNumSamples transforms spread evenly over afDuration at afTime: the last sample past the
// second-to-last frame, else the two samples around the frame interpolated (hkQsTransform::setInterpolate4, built in a
// local and copied out). Returns apResult.
static hkQsTransform* sampleReferenceFrame(
    hkQsTransform* apResult,
    float afTime,
    const hkQsTransform* apSamples,
    int32_t aiNumSamples,
    float afDuration
) {
    const float fFrame = afTime / afDuration * static_cast<double>(static_cast<uint32_t>(aiNumSamples - 1));
    const uint32_t uiFrame = static_cast<uint32_t>(static_cast<int64_t>(fFrame));
    if (uiFrame > static_cast<uint32_t>(aiNumSamples - 2)) {
        const hkQsTransform& rLast = apSamples[aiNumSamples - 1];
        apResult->m_translation.m_quad = rLast.m_translation.m_quad;
        apResult->m_rotation.m_vec.m_quad = rLast.m_rotation.m_vec.m_quad;
        apResult->m_scale.m_quad = rLast.m_scale.m_quad;
        return apResult;
    }
    const float fT = fFrame - static_cast<double>(uiFrame);
    hkSimdReal kT;
    kT.~hkSimdReal();
    ::new (&kT) hkSimdReal(fT);
    hkQsTransform kResult;
    kResult.setInterpolate4(apSamples[uiFrame], apSamples[uiFrame + 1], kT);
    apResult->m_translation.m_quad = kResult.m_translation.m_quad;
    apResult->m_rotation.m_vec.m_quad = kResult.m_rotation.m_vec.m_quad;
    apResult->m_scale.m_quad = kResult.m_scale.m_quad;
    return apResult;
}

static void cleanupLoadedObjecthkaDefaultAnimatedReferenceFrame(void* p) {
    static_cast<hkaDefaultAnimatedReferenceFrame*>(p)->~hkaDefaultAnimatedReferenceFrame();
}

hkaDefaultAnimatedReferenceFrame::MotionExtractionOptions::MotionExtractionOptions()
  : m_referenceFrameTransforms(nullptr), m_numReferenceFrameTransforms(0), m_referenceFrameDuration(0.0f),
    m_numSamples(2), m_allowUpDown(false), m_allowFrontBack(true), m_allowRightLeft(false), m_allowTurning(false) {
    m_up.m_quad = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
    m_forward.m_quad = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
}

// Samples the reference frame numSamples times over the duration, each sample the motion since the first transform
// projected by hkaDefaultAnimatedReferenceFrame::project against the sample before it (the identity against zero for
// the first projection, which the turning check reads). The first transform and the step are kept as locals, and the
// motion's scale (all ones) is built once before the loop.
hkaDefaultAnimatedReferenceFrame::hkaDefaultAnimatedReferenceFrame(
    const hkaDefaultAnimatedReferenceFrame::MotionExtractionOptions& options
) {
    hkaDefaultAnimatedReferenceFrame* pThis = this;
    hkArray<hkVector4>& samples = m_referenceFrameSamples;
    int32_t numSamples = options.m_numSamples;
    if (numSamples == -1) {
        numSamples = options.m_numReferenceFrameTransforms;
    }
    pThis->m_up = options.m_up;
    pThis->m_forward = options.m_forward;
    const float duration = options.m_referenceFrameDuration;
    pThis->m_duration = duration;
    const hkVector4 startTranslation = options.m_referenceFrameTransforms[0].m_translation;
    const hkQuaternion startRotation = options.m_referenceFrameTransforms[0].m_rotation;
    const float timeStep = duration / static_cast<double>(numSamples - 1);
    hkVector4 previous;
    previous.setZero4();
    hkVector4 projected;
    hkaDefaultAnimatedReferenceFrame::project(options, hkQsTransformIdentity, previous, projected);
    previous = projected;
    if (options.m_allowTurning.m_bool) {
        const float forwardError = std::fabs(static_cast<float>(options.m_forward.dot3(projected)));
        hkVector4 right;
        right.setCross(options.m_up, options.m_forward);
        const float rightError = std::fabs(static_cast<float>(right.dot3(projected)));
        if (forwardError > 0.01f || rightError > 0.01f) {
            char buf[512];
            {
                hkErrStream ostr(buf, 512);
                ostr << "Extracting turning motion on an animation works best when the start frame is vertically "
                        "aligned with the origin.";
                hkError::s_instance->message(
                    hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA9245), buf,
                    ".\\Motion\\Default\\hkaDefaultAnimatedReferenceFrame.cpp", 88
                );
            }
            {
                hkErrStream ostr2(buf, 512);
                ostr2 << "Some misalignment (" << forwardError << ", " << rightError << ") found.";
                hkError::s_instance->message(
                    hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA9245), buf,
                    ".\\Motion\\Default\\hkaDefaultAnimatedReferenceFrame.cpp", 89
                );
            }
            {
                hkErrStream ostr3(buf, 512);
                ostr3 << "For best results, try moving the animation to the origin in the Create Animations filter.";
                hkError::s_instance->message(
                    hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA9245), buf,
                    ".\\Motion\\Default\\hkaDefaultAnimatedReferenceFrame.cpp", 90
                );
            }
        }
    }
    // The heap is fetched before the capacity test.
    hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
    const int32_t capacity = samples.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (capacity < numSamples) {
        int32_t newCapacity = 2 * capacity;
        if (numSamples >= newCapacity) {
            newCapacity = numSamples;
        }
        hkArrayUtil::_reserve(heap, &samples, newCapacity, sizeof(hkVector4));
    }
    samples.m_size = numSamples;
    if (numSamples > 0) {
        hkVector4 ones;
        ones.setAll(1.0f);
        for (int32_t i = 0; i < numSamples; ++i) {
            const float time = static_cast<double>(i) * timeStep;
            hkQsTransform sample;
            sampleReferenceFrame(
                &sample, time, options.m_referenceFrameTransforms, options.m_numReferenceFrameTransforms,
                options.m_referenceFrameDuration
            );
            hkQsTransform motion;
            motion.m_rotation.setMulInverse(sample.m_rotation, startRotation);
            motion.m_translation.setSub4(sample.m_translation, startTranslation);
            motion.m_scale = ones;
            hkaDefaultAnimatedReferenceFrame::project(options, motion, previous, projected);
            samples.m_data[i] = projected;
            previous = projected;
        }
    }
}

// The sample array goes with its member.
hkaDefaultAnimatedReferenceFrame::~hkaDefaultAnimatedReferenceFrame() {}

// Clamps time to the samples' range and blends the two samples around it; the sample's w is the angle about m_up.
void hkaDefaultAnimatedReferenceFrame::getReferenceFrame(float time, hkQsTransform& motionOut) const {
    hkVector4 motion;
    const int32_t iNumSamples = m_referenceFrameSamples.m_size;
    const double fTime = time;
    if (!(m_duration <= fTime)) {
        if (!(fTime < 0.0)) {
            const float fDelta = m_duration / static_cast<double>(iNumSamples - 1);
            const double fFrame = fTime / fDelta;
            int32_t iFrame = static_cast<int32_t>(fFrame);
            if (iFrame == iNumSamples - 1) {
                --iFrame;
            }
            const hkVector4* pSamples = &m_referenceFrameSamples.m_data[iFrame];
            const hkVector4 kFrom = pSamples[0];
            const hkVector4 kTo = pSamples[1];
            const float fT = fFrame - static_cast<double>(iFrame);
            const __m128 t = _mm_shuffle_ps(_mm_set_ss(fT), _mm_set_ss(fT), 0);
            motion.m_quad =
                _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, t), kFrom.m_quad), _mm_mul_ps(kTo.m_quad, t));
        }
        else {
            motion.m_quad = m_referenceFrameSamples.m_data->m_quad;
        }
    }
    else {
        motion.m_quad = m_referenceFrameSamples.m_data[iNumSamples - 1].m_quad;
    }
    motionOut.m_translation.m_quad = motion.m_quad;
    motionOut.m_rotation.setAxisAngle(m_up, XFNV_M128_F32(motion.m_quad, 3));
    motionOut.m_scale.m_quad = _mm_set1_ps(1.0f);
}

void hkaDefaultAnimatedReferenceFrame::getDeltaReferenceFrame(
    float time,
    float nextTime,
    int32_t loops,
    hkQsTransform& output
) const {
    int32_t v6;
    hkVector4 v10;
    hkQsTransform quat_8;
    __m128 v42;
    hkQuaternion v43;
    hkQsTransform v46;
    hkQsTransform v52;
    getReferenceFrame(time, v52);
    getReferenceFrame(nextTime, quat_8);
    if (!loops) {
        goto LABEL_12;
    }
    getReferenceFrame(m_duration, v46);
    v6 = loops;
    if (loops < 0) {
        v6 = -loops;
    }
    if (v6) {
        hkQuaternion v7;
        v7.m_vec.m_quad = v46.m_rotation.m_vec.m_quad;
        __m128 v8 = _mm_shuffle_ps(v46.m_rotation.m_vec.m_quad, v46.m_rotation.m_vec.m_quad, 255);
        __m128 v9 = _mm_shuffle_ps(v46.m_rotation.m_vec.m_quad, v46.m_rotation.m_vec.m_quad, 201);
        __m128 v45 = _mm_shuffle_ps(v46.m_rotation.m_vec.m_quad, v46.m_rotation.m_vec.m_quad, 210);
        v10.m_quad = quat_8.m_translation.m_quad;
        __m128 v48 = v8;
        __m128 v47 = v9;
        int32_t v11 = v6;
        while (true) {
            __m128 v25;
            if (loops >= 0) {
                __m128 v26 = _mm_mul_ps(v10.m_quad, v7.m_vec.m_quad);
                __m128 v27 = _mm_shuffle_ps(v26, v26, 170);
                XFNV_M128_F32(v27, 0) =
                    XFNV_M128_F32(v27, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v26, v26, 85), 0) + XFNV_M128_F32(v26, 0));
                __m128 v28 = _mm_add_ps(
                    _mm_mul_ps(
                        _mm_sub_ps(
                            _mm_mul_ps(_mm_shuffle_ps(v10.m_quad, v10.m_quad, 210), v9),
                            _mm_mul_ps(_mm_shuffle_ps(v10.m_quad, v10.m_quad, 201), v45)
                        ),
                        v8
                    ),
                    _mm_add_ps(
                        _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v8, v8), hkQuadRealMinusHalf), v10.m_quad),
                        _mm_mul_ps(_mm_shuffle_ps(v27, v27, 0), v7.m_vec.m_quad)
                    )
                );
                v42 = _mm_add_ps(_mm_add_ps(v28, v28), v46.m_translation.m_quad);
                v43.setMul(v46.m_rotation, quat_8.m_rotation);
                v25 = _mm_mul_ps(v46.m_scale.m_quad, quat_8.m_scale.m_quad);
            }
            else {
                hkQuaternion q0_8;
                __m128 v12 = _mm_mul_ps(v46.m_translation.m_quad, v7.m_vec.m_quad);
                __m128 v13 = _mm_shuffle_ps(v12, v12, 170);
                XFNV_M128_F32(v13, 0) =
                    XFNV_M128_F32(v13, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v12, v12, 85), 0) + XFNV_M128_F32(v12, 0));
                __m128 v14 = _mm_mul_ps(_mm_shuffle_ps(v46.m_translation.m_quad, v46.m_translation.m_quad, 210), v9);
                __m128 v15 = _mm_add_ps(
                    _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v8, v8), hkQuadRealMinusHalf), v46.m_translation.m_quad),
                    _mm_mul_ps(_mm_shuffle_ps(v13, v13, 0), v7.m_vec.m_quad)
                );
                __m128 v16 =
                    _mm_shuffle_ps(v46.m_scale.m_quad, _mm_unpackhi_ps(v46.m_scale.m_quad, hkQuadReal1111), 196);
                __m128 v17 = _mm_rcp_ps(v16);
                __m128 v18 = _mm_add_ps(
                    _mm_mul_ps(
                        _mm_sub_ps(
                            _mm_mul_ps(_mm_shuffle_ps(v46.m_translation.m_quad, v46.m_translation.m_quad, 201), v45),
                            v14
                        ),
                        v8
                    ),
                    v15
                );
                __m128 v51 =
                    _mm_and_ps(_mm_sub_ps(_mm_add_ps(v17, v17), _mm_mul_ps(_mm_mul_ps(v17, v16), v17)), rdata_10C4C10);
                __m128 v19 = _mm_add_ps(v18, v18);
                __m128 v20 = _mm_xor_ps(rdata_10C4C00, v7.m_vec.m_quad);
                __m128 v21 = _mm_mul_ps(v20, v10.m_quad);
                __m128 v22 = _mm_shuffle_ps(v21, v21, 170);
                XFNV_M128_F32(v22, 0) =
                    XFNV_M128_F32(v22, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v21, v21, 85), 0) + XFNV_M128_F32(v21, 0));
                __m128 v23 = _mm_shuffle_ps(v20, v20, 255);
                __m128 v24 = _mm_add_ps(
                    _mm_mul_ps(
                        _mm_sub_ps(
                            _mm_mul_ps(_mm_shuffle_ps(v10.m_quad, v10.m_quad, 210), _mm_shuffle_ps(v20, v20, 201)),
                            _mm_mul_ps(_mm_shuffle_ps(v10.m_quad, v10.m_quad, 201), _mm_shuffle_ps(v20, v20, 210))
                        ),
                        v23
                    ),
                    _mm_add_ps(
                        _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v23, v23), hkQuadRealMinusHalf), v10.m_quad),
                        _mm_mul_ps(_mm_shuffle_ps(v22, v22, 0), v20)
                    )
                );
                __m128 direction_8 = _mm_xor_ps(v19, hkQuadRealNegateMask);
                q0_8.m_vec.m_quad = v20;
                v42 = _mm_add_ps(_mm_add_ps(v24, v24), direction_8);
                v43.setMul(q0_8, quat_8.m_rotation);
                v25 = _mm_mul_ps(quat_8.m_scale.m_quad, v51);
            }
            --v11;
            v10.m_quad = v42;
            quat_8.m_scale.m_quad = v25;
            quat_8.m_rotation.m_vec.m_quad = v43.m_vec.m_quad;
            quat_8.m_translation.m_quad = v42;
            if (!v11) {
                break;
            }
            v7.m_vec.m_quad = v46.m_rotation.m_vec.m_quad;
            v8 = v48;
            v9 = v47;
        }
    }
    else {
    LABEL_12:;
        v10.m_quad = quat_8.m_translation.m_quad;
    }
    __m128 v29 = _mm_mul_ps(v52.m_rotation.m_vec.m_quad, v52.m_translation.m_quad);
    __m128 v30 = _mm_shuffle_ps(v29, v29, 170);
    XFNV_M128_F32(v30, 0) =
        XFNV_M128_F32(v30, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v29, v29, 85), 0) + XFNV_M128_F32(v29, 0));
    __m128 v31 = _mm_shuffle_ps(v52.m_rotation.m_vec.m_quad, v52.m_rotation.m_vec.m_quad, 255);
    __m128 v32 = _mm_xor_ps(v52.m_rotation.m_vec.m_quad, rdata_10C4C00);
    __m128 v33 = _mm_add_ps(
        _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(v52.m_rotation.m_vec.m_quad, v52.m_rotation.m_vec.m_quad, 210),
                    _mm_shuffle_ps(v52.m_translation.m_quad, v52.m_translation.m_quad, 201)
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(v52.m_rotation.m_vec.m_quad, v52.m_rotation.m_vec.m_quad, 201),
                    _mm_shuffle_ps(v52.m_translation.m_quad, v52.m_translation.m_quad, 210)
                )
            ),
            v31
        ),
        _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v31, v31), hkQuadRealMinusHalf), v52.m_translation.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v30, v30, 0), v52.m_rotation.m_vec.m_quad)
        )
    );
    __m128 v34 = _mm_add_ps(v33, v33);
    __m128 v35 = _mm_shuffle_ps(v52.m_scale.m_quad, _mm_unpackhi_ps(v52.m_scale.m_quad, hkQuadReal1111), 196);
    __m128 v36 = _mm_rcp_ps(v35);
    __m128 v44 = _mm_and_ps(_mm_sub_ps(_mm_add_ps(v36, v36), _mm_mul_ps(_mm_mul_ps(v36, v35), v36)), rdata_10C4C10);
    __m128 v37 = _mm_mul_ps(v32, v10.m_quad);
    __m128 v38 = _mm_shuffle_ps(v37, v37, 170);
    XFNV_M128_F32(v38, 0) =
        XFNV_M128_F32(v38, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v37, v37, 85), 0) + XFNV_M128_F32(v37, 0));
    __m128 v39 = _mm_shuffle_ps(v32, v32, 255);
    v42 = _mm_xor_ps(v34, hkQuadRealNegateMask);
    v43.m_vec.m_quad = v32;
    __m128 v40 = _mm_add_ps(
        _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(v32, v32, 201), _mm_shuffle_ps(v10.m_quad, v10.m_quad, 210)),
                _mm_mul_ps(_mm_shuffle_ps(v32, v32, 210), _mm_shuffle_ps(v10.m_quad, v10.m_quad, 201))
            ),
            v39
        ),
        _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v39, v39), hkQuadRealMinusHalf), v10.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v38, v38, 0), v32)
        )
    );
    output.m_translation.m_quad = _mm_add_ps(_mm_add_ps(v40, v40), v42);
    output.m_rotation.setMul(v43, quat_8.m_rotation);
    output.m_scale.m_quad =
        _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(0x3F800000u)), _mm_set_ss(std::bit_cast<float>(0x3F800000u)), 0);
}

float hkaDefaultAnimatedReferenceFrame::getDuration() const {
    return m_duration;
}

const hkClass& hkaDefaultAnimatedReferenceFrame::staticClass() {
    return hkaDefaultAnimatedReferenceFrameClass;
}

// Projects motion's translation on the axes options allows (forward, up, and up x forward) into projectedOut's xyz;
// with turning allowed, w is motion's turn about the up axis, unwrapped by whole turns to within pi of previous's w,
// else 0. The angle is kept as a float between the steps; the differences and turns are computed in double.
void hkaDefaultAnimatedReferenceFrame::project(
    const hkaDefaultAnimatedReferenceFrame::MotionExtractionOptions& options,
    const hkQsTransform& motion,
    const hkVector4& previous,
    hkVector4& projectedOut
) {
    hkVector4 upMotion;
    upMotion.setMul4(motion.m_translation.dot3(options.m_up), options.m_up);
    hkVector4 rightAxis;
    rightAxis.setCross(options.m_up, options.m_forward);
    hkVector4 rightMotion;
    rightMotion.setMul4(motion.m_translation.dot3(rightAxis), rightAxis);
    hkVector4 forwardMotion;
    forwardMotion.setMul4(motion.m_translation.dot3(options.m_forward), options.m_forward);
    projectedOut.setZero4();
    if (options.m_allowFrontBack.m_bool) {
        projectedOut.add4(forwardMotion);
    }
    if (options.m_allowUpDown.m_bool) {
        projectedOut.add4(upMotion);
    }
    if (options.m_allowRightLeft.m_bool) {
        projectedOut.add4(rightMotion);
    }
    if (options.m_allowTurning.m_bool) {
        hkQuaternion rest;
        float angle;
        motion.m_rotation.decomposeRestAxis(options.m_up, rest, angle);
        const float previousAngle = XFNV_M128_F32(previous.m_quad, 3);
        while (angle - previousAngle > 3.1415927410125732) {
            angle = angle - 6.2831854820251465;
        }
        while (angle - previousAngle < -3.1415927410125732) {
            angle = angle + 6.2831854820251465;
        }
        XFNV_M128_F32(projectedOut.m_quad, 3) = angle;
    }
    else {
        XFNV_M128_F32(projectedOut.m_quad, 3) = 0.0f;
    }
}
