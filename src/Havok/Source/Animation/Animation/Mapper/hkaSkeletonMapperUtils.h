#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapperData.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkaSkeleton;
class hkaSkeletonMapper;

// Havok's helpers that build a skeleton mapper from two skeletons. Only the parameter types are declared.
class hkaSkeletonMapperUtils {
public:
	// A chain of bones the user maps by hand, by the names of its first and last bone. createMapping
	// steps the array by 8 bytes and passes each name to hkaSkeletonUtils::findBoneWithName.
	struct UserChain {
		const char* m_start;
		const char* m_end;
	};

	// A bone pair the user maps by hand, by name. createMapping copies the array's elements as two dwords
	// with an 8-byte stride and passes each name to hkaSkeletonUtils::findBoneWithName.
	struct UserMapping {
		const char* m_boneIn;
		const char* m_boneOut;
	};

	// How the motion extracted from the root is scaled between the two skeletons.
	struct ExtractedMotionParams {
		// Values from the Xbox PDB.
		enum ExtractedMotionMode : int32_t {
			HK_USE_BOUNDING_BOX_DIAGONAL = 0x0,
			HK_USE_BONE_LENGTHS = 0x1,
			HK_USE_FACTORS_DIRECTLY = 0x2,
		};

		// The SDK's: the scales of the motion extracted for A and for B (aMotionOut, bMotionOut, otherwise the
		// identity), from the poses' bounding box diagonals, the extracted motion bones' lengths or the factors
		// alone, as m_mode says. The SDK's HK_CALL member: cdecl with this first (ret 0, 0xCE55D0).
		void computeExtractedMotionTransforms(hkaSkeletonMapperData::MappingType type, const hkQsTransform* poseAMS,
			int32_t numBonesA, const hkQsTransform* poseBMS, int32_t numBonesB, hkQsTransform& aMotionOut,
			hkQsTransform& bMotionOut);

		// The SDK's: the smallest and largest components of the numBones translations of the pose.
		static void getPoseMinMax(const hkQsTransform* poseModelSpace, int32_t numBones, hkVector4& minOut,
			hkVector4& maxOut);

		hkEnum<ExtractedMotionMode, int32_t> m_mode;
		int32_t m_extractedMotionBoneA;
		int32_t m_extractedMotionBoneB;
		float m_extractedMotionFactorA;
		float m_extractedMotionFactorB;
	};

	// What the mapper builder starts from: the two skeletons, the matching tolerances and the user's chains and
	// mappings.
	struct Params {
		// The hybrid's implicit default constructor; Init fills the members.
		Params() = default;
		Params(const hkaSkeletonMapperUtils::Params& other);

		hkaSkeletonMapperUtils::Params* Init();
		void Free();

		const hkaSkeleton* m_skeletonA;
		const hkaSkeleton* m_skeletonB;
		// Compares two bone names, 0 when they match.
		int32_t (*m_compareNames)(const char* str1, const char* str2);
		float m_positionMatchTolerance;
		float m_rotationMatchTolerance;
		hkArray<UserChain> m_userChains;
		hkArray<UserMapping> m_userMappingsAtoB;
		hkArray<UserMapping> m_userMappingsBtoA;
		hkBool m_autodetectSimple;
		hkBool m_autodetectChains;
		hkEnum<hkaSkeletonMapperData::MappingType, int32_t> m_mappingType;
		ExtractedMotionParams m_motionParams;
	};

	// Havok SDK signature (hkaAnimation.lib, tied by reloc,string).
	static void computeMappingTransform(hkaSkeletonMapperData::MappingType type, const hkQsTransform& a,
		const hkQsTransform& b, hkQsTransform& mapOut, float tolerance, float defaultScale);
	// The SDK's: the mappings from A to B and from B to A of the params' skeletons.
	static void createMapping(const hkaSkeletonMapperUtils::Params& originalParams, hkaSkeletonMapperData& originalAToB,
		hkaSkeletonMapperData& originalBToA);
	// The SDK's: the chain mapping from startA-endA in A to startB-endB in B.
	static void computeChainMapping(hkaSkeletonMapperData::MappingType type, const hkaSkeleton* skeletonA,
		const hkQsTransform* poseA, int16_t startA, int16_t endA, const hkaSkeleton* skeletonB,
		const hkQsTransform* poseB, int16_t startB, int16_t endB, hkaSkeletonMapperData::ChainMapping& mapAFromBOut);
	// The SDK's: the summed bone lengths of the chain from end up to start and its end's position relative to
	// start.
	static void computeChainLengthAndPosition(const hkaSkeleton* skeleton, const hkQsTransform* pose, int16_t start,
		int16_t end, float& lengthOut, hkVector4& posOut);
	// Havok SDK signature (hkaAnimation.lib, tied by bytes).
	static const hkaSkeletonMapperData::SimpleMapping* findSimpleMapping(int16_t boneA,
		const hkaSkeletonMapperData& mapperdata);
	// Havok SDK signature (hkaAnimation.lib, tied by bytes).
	static bool checkRotation(const hkaSkeletonMapperUtils::Params& params, int16_t boneA, int16_t boneB,
		const hkQsTransform& aToBTransform, const hkQsTransform& bToATransform);
	// Havok SDK signature (hkaAnimation.lib, tied by bytes,string).
	static hkResult lockTranslationsAutomatically(const hkaSkeletonMapperData& mapperData);
	// Havok SDK: scales the translations of the mapper's mapping transforms by scale.
	static void scaleMapping(hkaSkeletonMapper& mapper, float scale);
	// Havok SDK signature (hkaAnimation.lib, tied by bytes).
	static void computeMappingTransform(hkaSkeletonMapperData::MappingType type, const hkQsTransform* poseA,
		int16_t boneA, const hkQsTransform* poseB, int16_t boneB, hkQsTransform& mapOut, float tolerance,
		float defaultScale);
};
