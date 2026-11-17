#pragma once

#include "Reconstruction.hpp"

class NiMatrix3;
class NiTransform;

// The processor-specific vertex transforms: two function pointers the exe's SetPointTransformFunction and
// SetVectorTransformFunction fill in. A static class, no instances (the PDB's sizeof is 1).
class NiProcessorSpecificCode {
public:
	typedef void (*PointTransformFunc)(uint16_t uiVerts, const float* pModel, float* pWorld, const NiTransform* pXform);
	typedef void (*VectorTransformFunc)(uint16_t uiVerts, const float* pModel, float* pWorld, NiMatrix3* pMatrix);

	// The values IdentifyProcessor stores in ms_eProcessorID
	// (Gamebryo's reference source names).
	enum ProcessorID : int32_t {
		NI_UNSUPPORTED = 0,
		NI_PENTIUM = 1,
		NI_PENTIUM_MMX = 2,
		NI_PENTIUM_PRO = 3,
		NI_PENTIUM_II = 4,
		NI_PENTIUM_III = 5,
		NI_3DNOW = 6,
		NI_NUM_PROCESSOR_IDS = 7,
	};

	// Not named in the PDB or IDA; name chosen here (Gamebryo's reference source name). Each ProcessorID's name, in
	// the exe's .data right after ms_pPointsTransform; no code in the exe reads it.
	using IDStringTable = const char* [NI_NUM_PROCESSOR_IDS];

	// IDA name; not in the PDB. Probes cpuid inside SEH __try/__except blocks (kept unregistered: src cannot
	// express them), sets ms_bIDSet and ms_eProcessorID (0xA74A10; no ecx use, plain ret).
	static void IdentifyProcessor();
	// Stores ptf in ms_pPointsTransform.
	static void SetPointTransformFunction(PointTransformFunc ptf);
	// Stores vtf in ms_pVectorsTransform.
	static void SetVectorTransformFunction(VectorTransformFunc vtf);
	// Identifies the processor once, then picks the transform functions.
	static void OptimizeMainEngineForProcessor();
	// The PDB's TransformPoints: runs ms_pPointsTransform (cdecl, 0x4B4A10).
	static void TransformPoints(uint16_t uiVerts, const float* pModel, float* pWorld, const NiTransform* pXform);

	static VectorTransformFunc ms_pVectorsTransform;
	static PointTransformFunc ms_pPointsTransform;
	static inline IDStringTable ms_ppIDStrings{"Unsupported processor", "Pentium", "Pentium with MMX", "Pentium Pro",
		"Pentium II", "Pentium III", "3DNow-compatible"};
	static inline ProcessorID ms_eProcessorID{};
	static inline bool ms_bIDSet;
	static inline int32_t ms_uiCPUIDFeatures;
};
