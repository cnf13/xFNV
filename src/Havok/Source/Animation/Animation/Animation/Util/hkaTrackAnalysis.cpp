#include "Havok/Source/Animation/Animation/Animation/Util/hkaTrackAnalysis.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <cmath>

// The four-bit values with their bits reversed: a lane mask (x in bit 0) to a static mask's dynamic bits (x in the
// group's highest bit). analyzeQsTransform reads it.
using hkaTrackAnalysis_reverseBitsArray = uint8_t[16];
static hkaTrackAnalysis_reverseBitsArray hkaTrackAnalysis_reverseBits{0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE,
                                                                      0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF};

// The five arrays go with their members.
hkaTrackAnalysis::PerTransformTrackAnalysisInput::~PerTransformTrackAnalysisInput() {}

void hkaTrackAnalysis::AnalysisStats::displayReport() {
    char buf[512];
    hkError::s_instance->sectionBegin(0x432434A4, "Track Analysis");
    {
        hkErrStream apThis(buf, 512);
        uint32_t afValue = m_numTransformDOFS + m_numFloatDOFS;
        hkOstream* v2 = &(apThis << "Total DOFs ");
        *v2 << afValue;
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\Animation\\Util\\hkaTrackAnalysis.cpp", 47);
    }
    {
        hkErrStream apThis(buf, 512);
        uint32_t afValuea = m_staticPosDOF;
        hkOstream* v3 = &(apThis << "Static position DOFs ");
        *v3 << afValuea;
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\Animation\\Util\\hkaTrackAnalysis.cpp", 48);
    }
    {
        hkErrStream apThis(buf, 512);
        uint32_t afValueb = m_staticRotDOF;
        hkOstream* v4 = &(apThis << "Static rotation DOFs ");
        *v4 << afValueb;
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\Animation\\Util\\hkaTrackAnalysis.cpp", 49);
    }
    {
        hkErrStream apThis(buf, 512);
        uint32_t afValuec = m_staticSclDOF;
        hkOstream* v5 = &(apThis << "Static scale DOFs ");
        *v5 << afValuec;
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\Animation\\Util\\hkaTrackAnalysis.cpp", 50);
    }
    {
        hkErrStream apThis(buf, 512);
        uint32_t afValued = m_staticFloatDOF;
        hkOstream* v6 = &(apThis << "Static float DOFs ");
        *v6 << afValued;
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\Animation\\Util\\hkaTrackAnalysis.cpp", 51);
    }
    {
        hkErrStream apThis(buf, 512);
        uint32_t afValuee = m_clearPosDOF;
        hkOstream* v7 = &(apThis << "Clear position DOFs ");
        *v7 << afValuee;
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\Animation\\Util\\hkaTrackAnalysis.cpp", 52);
    }
    {
        hkErrStream apThis(buf, 512);
        uint32_t afValuef = m_clearRotDOF;
        hkOstream* v8 = &(apThis << "Clear rotation DOFs ");
        *v8 << afValuef;
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\Animation\\Util\\hkaTrackAnalysis.cpp", 53);
    }
    {
        hkErrStream apThis(buf, 512);
        uint32_t afValueg = m_clearSclDOF;
        hkOstream* v9 = &(apThis << "Clear scale DOFs ");
        *v9 << afValueg;
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\Animation\\Util\\hkaTrackAnalysis.cpp", 54);
    }
    {
        hkErrStream apThis(buf, 512);
        uint32_t afValueh = m_dynamicPosDOF;
        hkOstream* v10 = &(apThis << "Dynamic position DOFs ");
        *v10 << afValueh;
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\Animation\\Util\\hkaTrackAnalysis.cpp", 55);
    }
    {
        hkErrStream apThis(buf, 512);
        uint32_t afValuei = m_dynamicRotDOF;
        hkOstream* v11 = &(apThis << "Dynamic rotation DOFs ");
        *v11 << afValuei;
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\Animation\\Util\\hkaTrackAnalysis.cpp", 56);
    }
    {
        hkErrStream apThis(buf, 512);
        uint32_t afValuej = m_dynamicSclDOF;
        hkOstream* v12 = &(apThis << "Dynamic scale DOFs ");
        *v12 << afValuej;
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\Animation\\Util\\hkaTrackAnalysis.cpp", 57);
    }
    {
        hkErrStream apThis(buf, 512);
        uint32_t afValuek = m_dynamicFloatDOF;
        hkOstream* v13 = &(apThis << "Dynamic float DOFs ");
        *v13 << afValuek;
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\Animation\\Util\\hkaTrackAnalysis.cpp", 58);
    }
    {
        hkErrStream apThis(buf, 512);
        float afValuel = m_ratio;
        hkOstream* v14 = &(apThis << "Redundancy ");
        *v14 << afValuel;
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\Animation\\Util\\hkaTrackAnalysis.cpp", 59);
    }
    hkError::s_instance->sectionEnd();
}

// Bits 0-1, 2-3 and 4-5 of a transform track's static mask are nonzero when its translation, rotation or scale is
// wholly static; bits 6 and up (the dynamic mask) flag each dynamic component, the highest bit of a group for x. A
// channel takes align floats: the component's value in every pose (the data are pose-major), then the last pose's value
// repeated. A component is read as an array element of its vector. The plain float copies may keep a signaling NaN's
// bits.
void hkaTrackAnalysis::channelSplit(const AnalysisSplit& input, float* channelData) {
    const uint32_t numT = input.m_numberOfTransformTracks;
    const uint32_t numP = input.m_numberOfPoses;
    // The compare is unsigned.
    const uint32_t align = numP > input.m_channelAlign ? numP : input.m_channelAlign;
    uint32_t dIdx = 0;
    for (uint32_t t = 0; t < numT; t++) {
        const uint16_t mask = input.m_staticMask[t];
        const uint16_t dynamicMask = static_cast<uint16_t>(mask >> 6);
        if ((mask & 0x3) == 0) {
            // Translation x, y, z: dynamic mask bits 2, 1, 0.
            for (int32_t c = 0; c < 3; c++) {
                if (dynamicMask & (1 << (2 - c))) {
                    float* pOut = channelData + dIdx * align;
                    for (uint32_t p = 0; p < numP; p++) {
                        pOut[p] =
                            (&reinterpret_cast<const float*>(&input.m_data[p * numT + t].m_translation.m_quad)[0])[c];
                    }
                    const uint32_t uiLast = (numP - 1) * numT + t;
                    for (uint32_t p = numP; p < align; p++) {
                        pOut[p] = (&reinterpret_cast<const float*>(&input.m_data[uiLast].m_translation.m_quad)[0])[c];
                    }
                    dIdx++;
                }
            }
        }
        if ((mask & 0xC) == 0) {
            // Rotation x, y, z, w: dynamic mask bits 6, 5, 4, 3.
            for (int32_t c = 0; c < 4; c++) {
                if (dynamicMask & (1 << (6 - c))) {
                    float* pOut = channelData + dIdx * align;
                    for (uint32_t p = 0; p < numP; p++) {
                        pOut[p] = (&reinterpret_cast<const float*>(
                            &input.m_data[p * numT + t].m_rotation.m_vec.m_quad
                        )[0])[c];
                    }
                    const uint32_t uiLast = (numP - 1) * numT + t;
                    for (uint32_t p = numP; p < align; p++) {
                        pOut[p] =
                            (&reinterpret_cast<const float*>(&input.m_data[uiLast].m_rotation.m_vec.m_quad)[0])[c];
                    }
                    dIdx++;
                }
            }
        }
        if ((mask & 0x30) == 0) {
            // Scale x, y, z: dynamic mask bits 9, 8, 7.
            for (int32_t c = 0; c < 3; c++) {
                if (dynamicMask & (1 << (9 - c))) {
                    float* pOut = channelData + dIdx * align;
                    for (uint32_t p = 0; p < numP; p++) {
                        pOut[p] = (&reinterpret_cast<const float*>(&input.m_data[p * numT + t].m_scale.m_quad)[0])[c];
                    }
                    const uint32_t uiLast = (numP - 1) * numT + t;
                    for (uint32_t p = numP; p < align; p++) {
                        pOut[p] = (&reinterpret_cast<const float*>(&input.m_data[uiLast].m_scale.m_quad)[0])[c];
                    }
                    dIdx++;
                }
            }
        }
    }

    // A float track is dynamic when its whole static mask is zero.
    for (uint32_t i = 0; i < input.m_numberOfFloatTracks; i++) {
        if (input.m_staticMask[input.m_numberOfTransformTracks + i] == 0) {
            float* pOut = channelData + dIdx * align;
            for (uint32_t p = 0; p < numP; p++) {
                pOut[p] = input.m_floatData[p * input.m_numberOfFloatTracks + i];
            }
            const uint32_t uiLast = (numP - 1) * input.m_numberOfFloatTracks + i;
            for (uint32_t p = numP; p < align; p++) {
                pOut[p] = input.m_floatData[uiLast];
            }
            dIdx++;
        }
    }
}

// The Havok SDK's analysis of transform and float tracks: the transform tracks through analyzeQsTransform (no report),
// then per float track its range over the poses (two hkLocalArray<hkReal>): static when the range is below twice the
// track's absolute tolerance, its mean then going to staticDOFs after the transform tracks' static values.
void hkaTrackAnalysis::analyze(
    const hkaTrackAnalysis::PerTransformTrackAnalysisInput& input,
    const hkaTrackAnalysis::PerFloatTrackAnalysisInput& finput,
    uint16_t* staticMask,
    float* staticDOFs,
    hkBool displayStatsReport
) {
    hkaTrackAnalysis::analyzeQsTransform(input, staticMask, staticDOFs, static_cast<hkBool>(false));
    float* pStaticFloatDOFs;
    {
        // The SDK's AnalysisStats constructor: every count and the ratio 0.
        hkaTrackAnalysis::AnalysisStats stats = {};
        hkaTrackAnalysis::calcStatsQsTransform(staticMask, input.m_numberOfTransformTracks, stats);
        pStaticFloatDOFs = staticDOFs + (stats.m_staticSclDOF + stats.m_staticRotDOF + stats.m_staticPosDOF);
    }

    hkLocalArray<float> minValues(static_cast<int32_t>(finput.m_numberOfFloatTracks));
    minValues.setSize(static_cast<int32_t>(finput.m_numberOfFloatTracks));
    hkLocalArray<float> maxValues(static_cast<int32_t>(finput.m_numberOfFloatTracks));
    maxValues.setSize(static_cast<int32_t>(finput.m_numberOfFloatTracks));

    for (uint32_t i = 0; i < finput.m_numberOfFloatTracks; ++i) {
        float fTolerance = finput.m_absoluteTolerance.m_data[i];
        minValues.m_data[i] = 3.40282e+38f;
        maxValues.m_data[i] = -3.40282e+38f;
        for (uint32_t pose = 0; pose < input.m_numberOfPoses; ++pose) {
            const float fValue = finput.m_floatData[pose * finput.m_numberOfFloatTracks + i];
            if (minValues.m_data[i] > fValue) {
                minValues.m_data[i] = fValue;
            }
            if (maxValues.m_data[i] < fValue) {
                maxValues.m_data[i] = fValue;
            }
        }
        const float fMean = (minValues.m_data[i] + maxValues.m_data[i]) * 0.5;
        const uint32_t maskIndex = input.m_numberOfTransformTracks + i;
        const float fRange = maxValues.m_data[i] - minValues.m_data[i];
        const float fAbsRange = std::fabs(static_cast<double>(fRange));
        const float fLimit = fTolerance * 2.0;
        if (fLimit > fAbsRange) {
            staticMask[maskIndex] = 1;
            *pStaticFloatDOFs++ = fMean;
        }
        else {
            staticMask[maskIndex] = 0;
        }
    }

    if (displayStatsReport.m_bool) {
        // The SDK's AnalysisStats constructor: every count and the ratio 0.
        hkaTrackAnalysis::AnalysisStats stats = {};
        const uint32_t numTransformTracks = input.m_numberOfTransformTracks;
        hkaTrackAnalysis::calcStats(
            staticMask, &staticMask[numTransformTracks], numTransformTracks, finput.m_numberOfFloatTracks, stats
        );
        stats.displayReport();
    }
}

// The Havok SDK's analysis of the transform tracks. Per track: the average translation, rotation and scale over the
// poses (each rotation added with the sign that keeps it on the running sum's side, the sum then normalized), then per
// degree of freedom whether every pose is within the track's tolerance of the average (static), and whether a static
// translation is near zero, rotation near the identity or scale near one (clear). The static values go to staticDOFs in
// mask order. The averages are a stack-allocated hkLocalArray<hkQsTransform>. HK_REAL_MAX is the SDK's 3.40282e+38f
// (0x7F7FFFEE, not FLT_MAX).
void hkaTrackAnalysis::analyzeQsTransform(
    const hkaTrackAnalysis::PerTransformTrackAnalysisInput& input,
    uint16_t* staticMask,
    float* staticDOFs,
    hkBool displayStatsReport
) {
    const int32_t numTracks = static_cast<int32_t>(input.m_numberOfTransformTracks);
    hkLocalArray<hkQsTransform> average(numTracks);
    average.setSize(static_cast<int32_t>(input.m_numberOfTransformTracks));

    // The bounding box of every track's translation in every pose.
    hkVector4 minPos;
    minPos.setAll(3.40282e+38f);
    hkVector4 maxPos;
    maxPos.setAll(-3.40282e+38f);
    for (uint32_t t = 0; t < input.m_numberOfTransformTracks; ++t) {
        for (uint32_t p = 0; p < input.m_numberOfPoses; ++p) {
            const hkVector4 pos = input.m_boneData[p * input.m_numberOfTransformTracks + t].m_translation;
            minPos.setMin4(minPos, pos);
            maxPos.setMax4(maxPos, pos);
        }
    }
    hkVector4 range;
    range.setSub4(maxPos, minPos);

    for (uint32_t t = 0; t < input.m_numberOfTransformTracks; ++t) {
        const float absPosTol = input.m_absolutePositionTolerance.m_data[t];
        float relPosTol = input.m_relativePositionTolerance.m_data[t];
        const float rotTol = input.m_rotationTolerance.m_data[t];
        const float sclTol = input.m_scaleTolerance.m_data[t];
        const bool useThreeComponents = input.m_useThreeComponentQuaternions.m_data[t].m_bool;

        // A relative tolerance below epsilon over the range of motion is none. The tolerance is squared and rounded to
        // a float before the multiply.
        const float relPosTolSq = relPosTol * relPosTol;
        const float scaledRangeSq =
            _mm_cvtss_f32(_mm_mul_ss(range.lengthSquared3().getQuad(), _mm_set_ss(relPosTolSq)));
        if (1.1920929e-07f > scaledRangeSq && 0.0f < static_cast<float>(range.lengthSquared3())) {
            relPosTol = 0.0f;
        }

        hkQsTransform& avg = average.m_data[t];
        avg.m_translation.setZero4();
        avg.m_rotation.m_vec.setZero4();
        avg.m_scale.setZero4();
        for (uint32_t p = 0; p < input.m_numberOfPoses; ++p) {
            const hkQsTransform& pose = input.m_boneData[p * input.m_numberOfTransformTracks + t];
            hkSimdReal weight;
            weight.m_real = _mm_set_ss(1.0f);
            avg.m_translation.addMul4(weight, pose.m_translation);
            avg.m_scale.addMul4(weight, pose.m_scale);
            hkSimdReal rotWeight = weight;
            if (0.0f > static_cast<float>(pose.m_rotation.m_vec.dot4(avg.m_rotation.m_vec))) {
                rotWeight.m_real = _mm_sub_ss(hkQuadReal0000, weight.m_real);
            }
            avg.m_rotation.m_vec.addMul4(rotWeight, pose.m_rotation.m_vec);
        }
        const float numPoses = static_cast<float>(input.m_numberOfPoses);
        const float absNumPoses = std::fabs(numPoses);
        if (absNumPoses < 1.1920929e-07f) {
            avg.m_translation.setZero4();
            avg.m_rotation.m_vec.m_quad = hkQuadReal0001;
            avg.m_scale.setAll(1.0f);
        }
        else {
            hkSimdReal invNumPoses;
            invNumPoses.m_real = _mm_div_ss(_mm_set_ss(1.0f), _mm_set_ss(numPoses));
            avg.m_translation.mul4(invNumPoses);
            avg.m_scale.mul4(invNumPoses);
            if (1.1920929e-07f > static_cast<float>(avg.m_rotation.m_vec.lengthSquared4())) {
                avg.m_rotation.m_vec.m_quad = hkQuadReal0001;
            }
            else {
                avg.m_rotation.m_vec.normalize4();
            }
            if (1.1920929e-07f > static_cast<float>(avg.m_scale.lengthSquared3())) {
                avg.m_scale.setAll(1.0f);
            }
        }

        // A three-component quaternion track whose w keeps one sign (|w| >= 0.1 in every pose) gets w left out.
        bool positiveW = false;
        bool negativeW = false;
        if (useThreeComponents) {
            float maxW = -3.40282e+38f;
            float minW = 3.40282e+38f;
            for (uint32_t p = 0; p < input.m_numberOfPoses; ++p) {
                const float w = input.m_boneData[p * input.m_numberOfTransformTracks + t].m_rotation.m_vec(3);
                if (maxW < w) {
                    maxW = w;
                }
                if (minW > w) {
                    minW = w;
                }
            }
            positiveW = minW >= 0.1f;
            negativeW = maxW <= -0.1f;
        }

        // Bits 0-1, 2-3, 4-5: translation, rotation, scale static; a dynamic component sets its bit from 6 up.
        uint16_t mask = HK_TRACK_STATIC | (HK_TRACK_STATIC << 2) | (HK_TRACK_STATIC << 4);
        if (relPosTol > 0.0f || absPosTol > 0.0f) {
            hkVector4 relTol;
            relTol.setAll(relPosTol);
            relTol.mul4(range);
            hkVector4 absTol;
            absTol.setAll(absPosTol);
            for (uint32_t p = 0; p < input.m_numberOfPoses; ++p) {
                const hkQsTransform& pose = input.m_boneData[p * input.m_numberOfTransformTracks + t];
                hkVector4 diff;
                diff.setSub4(avg.m_translation, pose.m_translation);
                diff.setAbs4(diff);
                const int32_t withinAbs = _mm_movemask_ps(diff.compareLessThanEqual4(absTol).m_mask) & 7;
                const int32_t withinRel = _mm_movemask_ps(diff.compareLessThanEqual4(relTol).m_mask) & 7;
                if (absPosTol > 0.0f && withinAbs != 7) {
                    mask = static_cast<uint16_t>((mask & 0xfffc) | (hkaTrackAnalysis_reverseBits[~withinAbs & 7] << 5));
                }
                if (relPosTol > 0.0f && withinRel != 7) {
                    mask = static_cast<uint16_t>((mask & 0xfffc) | (hkaTrackAnalysis_reverseBits[~withinRel & 7] << 5));
                }
            }
        }
        else {
            mask = (HK_TRACK_STATIC << 2) | (HK_TRACK_STATIC << 4) | (7 << 6);
        }

        if (0.0f < rotTol) {
            // w of a quaternion that loses it is dynamic only through x, y and z.
            const bool dropW = useThreeComponents && (positiveW || negativeW);
            hkVector4 tol;
            tol.setAll(rotTol);
            for (uint32_t p = 0; p < input.m_numberOfPoses; ++p) {
                const hkVector4& rot = input.m_boneData[p * input.m_numberOfTransformTracks + t].m_rotation.m_vec;
                hkVector4 diff;
                diff.setSub4(rot, avg.m_rotation.m_vec);
                diff.setAbs4(diff);
                if ((_mm_movemask_ps(diff.compareLessThanEqual4(tol).m_mask) & 0xf) != 0xf) {
                    hkVector4 back;
                    back.setSub4(avg.m_rotation.m_vec, rot);
                    back.setAbs4(back);
                    int32_t within = _mm_movemask_ps(back.compareLessThanEqual4(tol).m_mask) & 0xf;
                    if (dropW) {
                        within |= 8;
                    }
                    if (within != 0xf) {
                        const int32_t dynamic = hkaTrackAnalysis_reverseBits[~within & 0xf];
                        mask = static_cast<uint16_t>((mask & 0xfff3) | (dynamic << 9));
                    }
                }
            }
            // The marker of a left out w, with its sign: recompose rebuilds w from x, y and z.
            if (dropW) {
                if (positiveW) {
                    avg.m_rotation.m_vec(3) = 2.0f;
                }
                if (negativeW) {
                    avg.m_rotation.m_vec(3) = -2.0f;
                }
            }
        }
        else {
            mask = static_cast<uint16_t>((mask & 0xfff3) | (0xf << 9));
        }

        if (sclTol > 0.0f) {
            hkVector4 tol;
            tol.setAll(sclTol);
            for (uint32_t p = 0; p < input.m_numberOfPoses; ++p) {
                const hkQsTransform& pose = input.m_boneData[p * input.m_numberOfTransformTracks + t];
                hkVector4 diff;
                diff.setSub4(avg.m_scale, pose.m_scale);
                diff.setAbs4(diff);
                const int32_t within = _mm_movemask_ps(diff.compareLessThanEqual4(tol).m_mask) & 7;
                if (within != 7) {
                    mask = static_cast<uint16_t>((mask & 0xffcf) | (hkaTrackAnalysis_reverseBits[~within & 7] << 12));
                }
            }
        }
        else {
            mask = static_cast<uint16_t>((mask & 0xffcf) | (7 << 13));
        }
        staticMask[t] = mask;

        // A static translation within half the tolerance of zero is clear: the absolute tolerance first, then the
        // relative one over the range's major axis.
        if ((mask & 3) == HK_TRACK_STATIC) {
            bool clear = absPosTol > 0.0f && avg.m_translation.equals3(hkVector4::getZero(), absPosTol * 0.5f);
            if (!clear && 0.0f < relPosTol) {
                clear =
                    avg.m_translation.equals3(hkVector4::getZero(), relPosTol * (range(range.getMajorAxis()) * 0.5f));
            }
            if (clear) {
                staticMask[t] = static_cast<uint16_t>((mask & 0xfffc) | HK_TRACK_CLEAR);
                avg.m_translation.setZero4();
            }
        }
        // A static rotation whose |w| is 1 is the identity: clear.
        if (((staticMask[t] & 0xc) == (HK_TRACK_STATIC << 2)) && (std::fabs(avg.m_rotation.m_vec(3)) == 1.0f)) {
            staticMask[t] = static_cast<uint16_t>((staticMask[t] & 0xfff3) | (HK_TRACK_CLEAR << 2));
            avg.m_rotation.m_vec.m_quad = hkQuadReal0001;
        }

        // A static scale within half the tolerance of one is clear.
        if ((staticMask[t] & 0x30) == (HK_TRACK_STATIC << 4)) {
            hkVector4 one;
            one.m_quad = hkQuadReal1111;
            if (avg.m_scale.equals3(one, sclTol * 0.5f)) {
                staticMask[t] = static_cast<uint16_t>((staticMask[t] & 0xffcf) | (HK_TRACK_CLEAR << 4));
                avg.m_scale.setAll(1.0f);
            }
        }
    }

    // The static components that are not clear, in mask order.
    for (uint32_t t = 0; t < input.m_numberOfTransformTracks; ++t) {
        const uint32_t mask = staticMask[t];
        const uint32_t dynamic = static_cast<uint16_t>(mask >> 6);
        const hkQsTransform& avg = average.m_data[t];
        if ((mask & 3) != HK_TRACK_CLEAR) {
            if (!(dynamic & 4)) {
                *staticDOFs++ = avg.m_translation(0);
            }
            if (!(dynamic & 2)) {
                *staticDOFs++ = avg.m_translation(1);
            }
            if (!(dynamic & 1)) {
                *staticDOFs++ = avg.m_translation(2);
            }
        }
        if ((mask & 0xc) != (HK_TRACK_CLEAR << 2)) {
            if (!(dynamic & 0x40)) {
                *staticDOFs++ = avg.m_rotation.m_vec(0);
            }
            if (!(dynamic & 0x20)) {
                *staticDOFs++ = avg.m_rotation.m_vec(1);
            }
            if (!(dynamic & 0x10)) {
                *staticDOFs++ = avg.m_rotation.m_vec(2);
            }
            if (!(dynamic & 8)) {
                *staticDOFs++ = avg.m_rotation.m_vec(3);
            }
        }
        if ((mask & 0x30) != (HK_TRACK_CLEAR << 4)) {
            if (!(dynamic & 0x200)) {
                *staticDOFs++ = avg.m_scale(0);
            }
            if (!(dynamic & 0x100)) {
                *staticDOFs++ = avg.m_scale(1);
            }
            if (!(dynamic & 0x80)) {
                *staticDOFs++ = avg.m_scale(2);
            }
        }
    }

    if (displayStatsReport.m_bool) {
        // The SDK's AnalysisStats constructor: every count and the ratio 0.
        hkaTrackAnalysis::AnalysisStats stats = {};
        hkaTrackAnalysis::calcStatsQsTransform(staticMask, input.m_numberOfTransformTracks, stats);
        stats.displayReport();
    }
}

void hkaTrackAnalysis::calcDynamicStatsQsTransform(
    const uint16_t* mask,
    uint32_t numTracks,
    hkaTrackAnalysis::AnalysisStats& out
) {
    uint32_t v3 = 0;
    out.m_dynamicPosDOF = 0;
    out.m_dynamicRotDOF = 0;
    for (out.m_dynamicSclDOF = 0; v3 < numTracks; ++v3) {
        uint32_t v4 = mask[v3] >> 6;
        out.m_dynamicPosDOF +=
            ((mask[v3] & 64) != 0) + ((mask[v3] >> 7 & 1) + (static_cast<uint8_t>(mask[v3] >> 8) & 1));
        out.m_dynamicRotDOF += (v4 >> 3 & 1) + ((v4 >> 4 & 1) + ((v4 >> 5 & 1) + (v4 >> 6 & 1)));
        out.m_dynamicSclDOF += (v4 >> 7 & 1) + ((v4 >> 8 & 1) + (v4 >> 9 & 1));
    }
}

void hkaTrackAnalysis::calcDynamicStats(
    const uint16_t* staticMaskT,
    const uint16_t* staticMaskF,
    uint32_t numTracksT,
    uint32_t numTracksF,
    hkaTrackAnalysis::AnalysisStats& out
) {
    hkaTrackAnalysis::calcDynamicStatsQsTransform(staticMaskT, numTracksT, out);
    uint32_t v5 = 0;
    for (out.m_dynamicFloatDOF = 0; v5 < numTracksF; ++v5) {
        if (!staticMaskF[v5]) {
            ++out.m_dynamicFloatDOF;
        }
    }
}

void hkaTrackAnalysis::calcStatsQsTransform(
    const uint16_t* mask,
    uint32_t numTracks,
    hkaTrackAnalysis::AnalysisStats& out
) {
    double v3 = 0.0;
    out.m_ratio = 0.0f;
    out.m_numTransformDOFS = 10 * numTracks;
    uint32_t v4 = 0;
    out.m_numFloatDOFS = 0;
    out.m_staticPosDOF = 0;
    out.m_staticRotDOF = 0;
    out.m_staticSclDOF = 0;
    out.m_staticFloatDOF = 0;
    out.m_clearPosDOF = 0;
    out.m_clearRotDOF = 0;
    out.m_clearSclDOF = 0;
    for (out.m_dynamicFloatDOF = 0; v4 < numTracks; ++v4) {
        if ((mask[v4] & 3) == 2) {
            out.m_clearPosDOF += 3;
        }
        if ((mask[v4] & 12) == 8) {
            out.m_clearRotDOF += 4;
        }
        if ((mask[v4] & 48) == 32) {
            out.m_clearSclDOF += 3;
        }
    }
    hkaTrackAnalysis::calcDynamicStatsQsTransform(mask, numTracks, out);
    uint32_t m_dynamicPosDOF = out.m_dynamicPosDOF;
    uint32_t v6 = 4 * numTracks - out.m_clearRotDOF;
    out.m_staticPosDOF = 3 * numTracks - m_dynamicPosDOF - out.m_clearPosDOF;
    uint32_t m_dynamicRotDOF = out.m_dynamicRotDOF;
    out.m_staticRotDOF = v6 - m_dynamicRotDOF;
    uint32_t m_dynamicSclDOF = out.m_dynamicSclDOF;
    out.m_staticSclDOF = 3 * numTracks - m_dynamicSclDOF - out.m_clearSclDOF;
    uint32_t v9 = m_dynamicRotDOF + m_dynamicSclDOF + m_dynamicPosDOF;
    if (v9) {
        v3 = static_cast<double>(out.m_numTransformDOFS) / static_cast<double>(v9);
    }
    out.m_ratio = v3;
}

void hkaTrackAnalysis::calcStats(
    const uint16_t* staticMaskT,
    const uint16_t* staticMaskF,
    uint32_t numTracksT,
    uint32_t numTracksF,
    hkaTrackAnalysis::AnalysisStats& out
) {
    double v5 = 0.0;
    out.m_ratio = 0.0f;
    out.m_numTransformDOFS = 10 * numTracksT;
    uint32_t v6 = 0;
    out.m_numFloatDOFS = numTracksF;
    out.m_staticPosDOF = 0;
    out.m_staticRotDOF = 0;
    out.m_staticSclDOF = 0;
    out.m_clearPosDOF = 0;
    out.m_clearRotDOF = 0;
    out.m_clearSclDOF = 0;
    for (out.m_dynamicFloatDOF = 0; v6 < numTracksT; ++v6) {
        if ((staticMaskT[v6] & 3) == 2) {
            out.m_clearPosDOF += 3;
        }
        if ((staticMaskT[v6] & 12) == 8) {
            out.m_clearRotDOF += 4;
        }
        if ((staticMaskT[v6] & 48) == 32) {
            out.m_clearSclDOF += 3;
        }
    }
    hkaTrackAnalysis::calcDynamicStats(staticMaskT, staticMaskF, numTracksT, numTracksF, out);
    uint32_t m_dynamicPosDOF = out.m_dynamicPosDOF;
    uint32_t v8 = 3 * numTracksT - out.m_clearSclDOF;
    out.m_staticPosDOF = 3 * numTracksT - out.m_clearPosDOF - m_dynamicPosDOF;
    uint32_t m_dynamicRotDOF = out.m_dynamicRotDOF;
    out.m_staticRotDOF = 4 * numTracksT - m_dynamicRotDOF - out.m_clearRotDOF;
    uint32_t m_dynamicSclDOF = out.m_dynamicSclDOF;
    out.m_staticSclDOF = v8 - m_dynamicSclDOF;
    uint32_t m_dynamicFloatDOF = out.m_dynamicFloatDOF;
    uint32_t v12 = numTracksF - m_dynamicFloatDOF;
    uint32_t v13 = m_dynamicRotDOF + (m_dynamicSclDOF + m_dynamicFloatDOF);
    bool v14 = m_dynamicPosDOF + v13 == 0;
    uint32_t v15 = m_dynamicPosDOF + v13;
    out.m_staticFloatDOF = v12;
    if (!v14) {
        v5 = static_cast<double>(out.m_numTransformDOFS + out.m_numFloatDOFS) / static_cast<double>(v15);
    }
    out.m_ratio = v5;
}

// Each transform track's mask (bits 0-5: clear/static state of translation, rotation and scale; bits 6-15: which degree
// of freedom is dynamic) picks each component from the dynamic or the static list. Every component goes through a float
// temporary. A rotation whose |w| is 2 is the encoder's marker for "w left out": w is rebuilt from x, y and z with the
// marker's sign.
void hkaTrackAnalysis::recompose(
    const hkaTrackAnalysis::AnalysisCompose& input,
    hkQsTransform* outputPose,
    float* outputFloatTracks
) {
    const float* staticDOFs = input.m_staticDOFsTransform;
    const float* dynamicDOFs = input.m_dynamicDOFsTransform;
    for (uint32_t t = 0; t < input.m_numberOfTransformTracks; ++t) {
        const uint32_t mask = input.m_staticMaskTransform[t];
        const uint32_t dynamicMask = mask >> 6;
        hkQsTransform& pose = outputPose[t];

        if ((mask & 3) == 2) {
            pose.m_translation.setZero4();
        }
        else {
            for (int32_t c = 0; c < 3; ++c) {
                const float value = (dynamicMask & (4u >> c)) ? *dynamicDOFs++ : *staticDOFs++;
                pose.m_translation(c) = value;
            }
        }

        if ((mask & 12) == 8) {
            pose.m_rotation.setIdentity();
        }
        else {
            float q[4];
            for (int32_t c = 0; c < 4; ++c) {
                const float value = (dynamicMask & (0x40u >> c)) ? *dynamicDOFs++ : *staticDOFs++;
                q[c] = value;
                pose.m_rotation.m_vec(c) = value;
            }
            const float absW = std::fabs(q[3]);
            if (absW == 2.0f) {
                float lenSq = q[0] * q[0] + q[1] * q[1] + q[2] * q[2];
                if (lenSq > 1.0f) {
                    lenSq = 1.0f;
                }
                const float rest = 1.0f - lenSq;
                const float w = std::sqrt(rest);
                if (q[3] > 0.0f) {
                    pose.m_rotation.m_vec(3) = w;
                }
                else {
                    pose.m_rotation.m_vec(3) = -w;
                }
            }
        }

        if ((mask & 48) == 32) {
            pose.m_scale.setAll(1.0f);
        }
        else {
            for (int32_t c = 0; c < 3; ++c) {
                const float value = (dynamicMask & (0x200u >> c)) ? *dynamicDOFs++ : *staticDOFs++;
                pose.m_scale(c) = value;
            }
        }
    }

    const float* staticDOFsFloat = input.m_staticDOFsFloat;
    const float* dynamicDOFsFloat = input.m_dynamicDOFsFloat;
    for (uint32_t i = 0; i < input.m_numberOfFloatTracks; ++i) {
        outputFloatTracks[i] = input.m_staticMaskFloat[i] ? *staticDOFsFloat++ : *dynamicDOFsFloat++;
    }
}

void hkaTrackAnalysis::analyze(const hkaTrackAnalysis::AnalysisInput& input, uint16_t* staticMask, float* staticDOFs) {
    hkaTrackAnalysis::PerTransformTrackAnalysisInput v16;
    uint32_t m_numberOfPoses = input.m_numberOfPoses;
    int32_t m_numberOfTransformTracks = input.m_numberOfTransformTracks;
    v16.m_absolutePositionTolerance.m_capacityAndFlags = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
    v16.m_relativePositionTolerance.m_capacityAndFlags = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
    v16.m_rotationTolerance.m_capacityAndFlags = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
    v16.m_scaleTolerance.m_capacityAndFlags = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
    v16.m_useThreeComponentQuaternions.m_capacityAndFlags = hkArrayBase<hkBool>::DONT_DEALLOCATE_FLAG;
    const hkQsTransform* m_boneData = input.m_boneData;
    v16.m_absolutePositionTolerance.m_data = nullptr;
    v16.m_absolutePositionTolerance.m_size = 0;
    v16.m_relativePositionTolerance.m_data = nullptr;
    v16.m_relativePositionTolerance.m_size = 0;
    v16.m_rotationTolerance.m_data = nullptr;
    v16.m_rotationTolerance.m_size = 0;
    v16.m_scaleTolerance.m_data = nullptr;
    v16.m_scaleTolerance.m_size = 0;
    v16.m_useThreeComponentQuaternions.m_data = nullptr;
    v16.m_useThreeComponentQuaternions.m_size = 0;
    v16.m_boneData = m_boneData;
    v16.m_numberOfTransformTracks = m_numberOfTransformTracks;
    v16.m_numberOfPoses = m_numberOfPoses;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    if (m_numberOfTransformTracks > 0) {
        hkArrayUtil::_reserve(*m_heap, &v16.m_absolutePositionTolerance, m_numberOfTransformTracks, sizeof(float));
    }
    v16.m_absolutePositionTolerance.m_size = m_numberOfTransformTracks;
    hkMemoryAllocator* v7 = (&hkMemoryRouter::getInstance())->m_heap;
    if ((v16.m_relativePositionTolerance.m_capacityAndFlags & 0x3FFFFFFF) < m_numberOfTransformTracks) {
        int32_t v8 = 2 * (v16.m_relativePositionTolerance.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK);
        if (m_numberOfTransformTracks >= v8) {
            v8 = m_numberOfTransformTracks;
        }
        hkArrayUtil::_reserve(*v7, &v16.m_relativePositionTolerance, v8, sizeof(float));
    }
    v16.m_relativePositionTolerance.m_size = m_numberOfTransformTracks;
    hkMemoryAllocator* v9 = (&hkMemoryRouter::getInstance())->m_heap;
    if ((v16.m_rotationTolerance.m_capacityAndFlags & 0x3FFFFFFF) < m_numberOfTransformTracks) {
        int32_t v10 = 2 * (v16.m_rotationTolerance.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK);
        if (m_numberOfTransformTracks >= v10) {
            v10 = m_numberOfTransformTracks;
        }
        hkArrayUtil::_reserve(*v9, &v16.m_rotationTolerance, v10, sizeof(float));
    }
    v16.m_rotationTolerance.m_size = m_numberOfTransformTracks;
    hkMemoryAllocator* v11 = (&hkMemoryRouter::getInstance())->m_heap;
    if ((v16.m_scaleTolerance.m_capacityAndFlags & 0x3FFFFFFF) < m_numberOfTransformTracks) {
        int32_t v12 = 2 * (v16.m_scaleTolerance.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK);
        if (m_numberOfTransformTracks >= v12) {
            v12 = m_numberOfTransformTracks;
        }
        hkArrayUtil::_reserve(*v11, &v16.m_scaleTolerance, v12, sizeof(float));
    }
    v16.m_scaleTolerance.m_size = m_numberOfTransformTracks;
    hkMemoryAllocator* v13 = (&hkMemoryRouter::getInstance())->m_heap;
    if ((v16.m_useThreeComponentQuaternions.m_capacityAndFlags & 0x3FFFFFFF) < m_numberOfTransformTracks) {
        int32_t v14 = 2 * (v16.m_useThreeComponentQuaternions.m_capacityAndFlags & hkArrayBase<hkBool>::CAPACITY_MASK);
        if (m_numberOfTransformTracks >= v14) {
            v14 = m_numberOfTransformTracks;
        }
        hkArrayUtil::_reserve(*v13, &v16.m_useThreeComponentQuaternions, v14, sizeof(hkBool));
    }
    int32_t v15 = 0;
    v16.m_useThreeComponentQuaternions.m_size = m_numberOfTransformTracks;
    if (m_numberOfTransformTracks >= 4) {
        do {
            v16.m_absolutePositionTolerance.m_data[v15] = input.m_absolutePositionTolerance;
            v15 += 4;
            v16.m_relativePositionTolerance.m_data[v15 - 4] = input.m_relativePositionTolerance;
            v16.m_rotationTolerance.m_data[v15 - 4] = input.m_rotationTolerance;
            v16.m_scaleTolerance.m_data[v15 - 4] = input.m_scaleTolerance;
            v16.m_useThreeComponentQuaternions.m_data[v15 - 4].m_bool = input.m_useThreeComponentQuaternions.m_bool;
            v16.m_absolutePositionTolerance.m_data[v15 - 3] = input.m_absolutePositionTolerance;
            v16.m_relativePositionTolerance.m_data[v15 - 3] = input.m_relativePositionTolerance;
            v16.m_rotationTolerance.m_data[v15 - 3] = input.m_rotationTolerance;
            v16.m_scaleTolerance.m_data[v15 - 3] = input.m_scaleTolerance;
            v16.m_useThreeComponentQuaternions.m_data[v15 - 3].m_bool = input.m_useThreeComponentQuaternions.m_bool;
            v16.m_absolutePositionTolerance.m_data[v15 - 2] = input.m_absolutePositionTolerance;
            v16.m_relativePositionTolerance.m_data[v15 - 2] = input.m_relativePositionTolerance;
            v16.m_rotationTolerance.m_data[v15 - 2] = input.m_rotationTolerance;
            v16.m_scaleTolerance.m_data[v15 - 2] = input.m_scaleTolerance;
            v16.m_useThreeComponentQuaternions.m_data[v15 - 2].m_bool = input.m_useThreeComponentQuaternions.m_bool;
            v16.m_absolutePositionTolerance.m_data[v15 - 1] = input.m_absolutePositionTolerance;
            v16.m_relativePositionTolerance.m_data[v15 - 1] = input.m_relativePositionTolerance;
            v16.m_rotationTolerance.m_data[v15 - 1] = input.m_rotationTolerance;
            v16.m_scaleTolerance.m_data[v15 - 1] = input.m_scaleTolerance;
            v16.m_useThreeComponentQuaternions.m_data[v15 - 1].m_bool = input.m_useThreeComponentQuaternions.m_bool;
        }
        while (v15 < m_numberOfTransformTracks - 3);
    }
    for (; v15 < m_numberOfTransformTracks;
         v16.m_useThreeComponentQuaternions.m_data[v15 - 1].m_bool = input.m_useThreeComponentQuaternions.m_bool) {
        v16.m_absolutePositionTolerance.m_data[v15++] = input.m_absolutePositionTolerance;
        v16.m_relativePositionTolerance.m_data[v15 - 1] = input.m_relativePositionTolerance;
        v16.m_rotationTolerance.m_data[v15 - 1] = input.m_rotationTolerance;
        v16.m_scaleTolerance.m_data[v15 - 1] = input.m_scaleTolerance;
    }
    hkaTrackAnalysis::analyzeQsTransform(v16, staticMask, staticDOFs, static_cast<hkBool>(true));
}
