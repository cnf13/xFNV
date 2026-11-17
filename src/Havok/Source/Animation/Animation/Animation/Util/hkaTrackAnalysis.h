#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <stdint.h>

class hkQsTransform;

// Havok SDK: static helpers that sort the degrees of freedom of an animation's tracks into static and dynamic ones
// and split or recompose the dynamic ones (hkaAnimation compression). It has no instances and no data.
class hkaTrackAnalysis {
public:
	// Havok SDK: tolerances shared by all transform tracks for analyze (hkaanimation.pdb, sizeof 0x20).
	struct AnalysisInput {
		const hkQsTransform* m_boneData;
		uint32_t m_numberOfTransformTracks;
		uint32_t m_numberOfPoses;
		float m_absolutePositionTolerance;
		float m_relativePositionTolerance;
		float m_rotationTolerance;
		float m_scaleTolerance;
		hkBool m_useThreeComponentQuaternions;
	};

	// Havok SDK: the input of channelSplit. Pose-major transform and float track data with one static mask per
	// track (transform tracks first, then float tracks).
	struct AnalysisSplit {
		const hkQsTransform* m_data;
		uint32_t m_numberOfTransformTracks;
		const float* m_floatData;
		uint32_t m_numberOfFloatTracks;
		uint32_t m_numberOfPoses;
		uint32_t m_channelAlign;
		const uint16_t* m_staticMask;
	};

	// Per-track tolerances for the analysis of transform tracks.
	struct PerTransformTrackAnalysisInput {
		~PerTransformTrackAnalysisInput();

		const hkQsTransform* m_boneData;
		uint32_t m_numberOfTransformTracks;
		uint32_t m_numberOfPoses;
		hkArray<float> m_absolutePositionTolerance;
		hkArray<float> m_relativePositionTolerance;
		hkArray<float> m_rotationTolerance;
		hkArray<float> m_scaleTolerance;
		hkArray<hkBool> m_useThreeComponentQuaternions;
	};

	// Havok SDK: per-track tolerances for the analysis of float tracks.
	struct PerFloatTrackAnalysisInput {
		const float* m_floatData;
		uint32_t m_numberOfFloatTracks;
		uint32_t m_numberOfPoses;
		hkArray<float> m_absoluteTolerance;
	};

	// Havok SDK: how a degree of freedom of a track behaves.
	enum TrackType : int32_t {
		HK_TRACK_DYNAMIC = 0x00,
		HK_TRACK_STATIC = 0x01,
		HK_TRACK_CLEAR = 0x02,
	};

	// Havok SDK: the counts of static, clear and dynamic degrees of freedom the analysis found, and the redundancy
	// ratio (hkaanimation.pdb, sizeof 0x38). displayReport reads the dwords at +0..+0x30 and the float at
	// +0x34.
	struct AnalysisStats {
		// Reports the counts and the ratio through hkError.
		// Havok SDK signature (hkaAnimation.lib, tied by bytes,reloc,string).
		void displayReport();

		uint32_t m_staticPosDOF;
		uint32_t m_staticRotDOF;
		uint32_t m_staticSclDOF;
		uint32_t m_staticFloatDOF;
		uint32_t m_clearPosDOF;
		uint32_t m_clearRotDOF;
		uint32_t m_clearSclDOF;
		uint32_t m_dynamicPosDOF;
		uint32_t m_dynamicRotDOF;
		uint32_t m_dynamicSclDOF;
		uint32_t m_dynamicFloatDOF;
		uint32_t m_numTransformDOFS;
		uint32_t m_numFloatDOFS;
		float m_ratio;
	};

	// Havok SDK: the input of recompose: the static masks, static and dynamic degrees of freedom of the transform
	// and float tracks (hkaanimation.pdb, sizeof 0x20).
	struct AnalysisCompose {
		uint32_t m_numberOfTransformTracks;
		uint32_t m_numberOfFloatTracks;
		const uint16_t* m_staticMaskTransform;
		const uint16_t* m_staticMaskFloat;
		const float* m_staticDOFsTransform;
		const float* m_staticDOFsFloat;
		const float* m_dynamicDOFsTransform;
		const float* m_dynamicDOFsFloat;
	};

	// Havok SDK: writes each dynamic degree of freedom of input as one channel of channelData: its value in every
	// pose, then its last value repeated up to the channel length max(m_numberOfPoses, m_channelAlign).
	static void channelSplit(const AnalysisSplit& input, float* channelData);
	// Fills staticMask and staticDOFs for the transform tracks of input (analyzeQsTransform, without its report), then
	// for the float tracks of finput, then reports when displayStatsReport.
	static void analyze(const PerTransformTrackAnalysisInput& input, const PerFloatTrackAnalysisInput& finput,
		uint16_t* staticMask, float* staticDOFs, hkBool displayStatsReport);
	// Fills staticMask and staticDOFs for the transform tracks of input, then reports when displayStatsReport is set.
	static void analyzeQsTransform(const hkaTrackAnalysis::PerTransformTrackAnalysisInput& input, uint16_t* staticMask,
		float* staticDOFs, hkBool displayStatsReport);
	// Havok SDK signature (hkaAnimation.lib, tied by bytes,reloc).
	static void calcDynamicStatsQsTransform(const uint16_t* mask, uint32_t numTracks,
		hkaTrackAnalysis::AnalysisStats& out);
	// Havok SDK signature (hkaAnimation.lib, tied by bytes,reloc).
	static void calcDynamicStats(const uint16_t* staticMaskT, const uint16_t* staticMaskF, uint32_t numTracksT,
		uint32_t numTracksF, hkaTrackAnalysis::AnalysisStats& out);
	// Havok SDK signature (hkaAnimation.lib, tied by bytes,reloc).
	static void calcStatsQsTransform(const uint16_t* mask, uint32_t numTracks, hkaTrackAnalysis::AnalysisStats& out);
	// Havok SDK signature (hkaAnimation.lib, tied by bytes,reloc).
	static void calcStats(const uint16_t* staticMaskT, const uint16_t* staticMaskF, uint32_t numTracksT,
		uint32_t numTracksF, hkaTrackAnalysis::AnalysisStats& out);
	// Havok SDK signature (hkaAnimation.lib, tied by reloc).
	static void recompose(const hkaTrackAnalysis::AnalysisCompose& input, hkQsTransform* outputPose,
		float* outputFloatTracks);
	// Havok SDK signature (hkaAnimation.lib, tied by bytes).
	static void analyze(const hkaTrackAnalysis::AnalysisInput& input, uint16_t* staticMask, float* staticDOFs);
};
