#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Reconstruction.hpp"

int32_t add1SaturateAndSetLowBit(int32_t r_eax);

// Havok's contact welding helpers: snaps contact normals on triangle edges against the neighbour triangles' angles.
// The shapes store a WeldingType per triangle or mesh; the sin/cos table holds the sector bounds per edge angle.
class hkpWeldingUtility {
public:
	enum SectorType : int32_t {
		SNAP_0 = 0x0,
		ACCEPT_0 = 0x1,
		REJECT = 0x2,
		ACCEPT_1 = 0x3,
		SNAP_1 = 0x4,
	};

	enum WeldingType : int32_t {
		WELDING_TYPE_ANTICLOCKWISE = 0x0,
		WELDING_TYPE_CLOCKWISE = 0x4,
		WELDING_TYPE_TWO_SIDED = 0x5,
		WELDING_TYPE_NONE = 0x6,
	};

	enum NumAngles : int32_t {
		NUM_ANGLES = 0x1F,
	};

	// The sector bounds for one edge angle: the cosines and sines of the accept and snap angles.
	struct SinCosTableEntry {
		float m_cosAccept0;
		float m_sinAccept0;
		float m_cosSnap0;
		float m_cosSnap1;
		float m_cosAccept1;
		float m_sinAccept1;
	};

	// NUM_ANGLES + 1 entries, as the SDK has it: initWeldingTable fills the first NUM_ANGLES, and the shapes index
	// the table with a five-bit edge bitcode (0 to 31; hkpTriangleShape::weldContactPointImpl, 0xD20614).
	using SinCosTable = SinCosTableEntry[NUM_ANGLES + 1];

	// Fills m_sinCosTable for NUM_ANGLES edge angles from -pi in steps of 0.2094 (cdecl, 0xD10890). Parameter names
	// not in the PDB; chosen here.
	static void initWeldingTable(float edgeNormalSnapDeltaAngle, float triangleNormalSnapDeltaAngle);
	// Havok SDK signature (hkpCollide.lib, tied by target).
	static const hkClass& staticClass();

	static inline SinCosTable m_sinCosTable{};
};

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The class's reflected enums (.rdata):
// file-local in the SDK's hkpWeldingUtilityClass.cpp; extern here because the member tables of five other files hold
// its address (the exe's relocations).
using hkpWeldingUtilityEnumsArray = const hkClassEnum[3];
extern hkpWeldingUtilityEnumsArray hkpWeldingUtilityEnums;

// The PDB's name: the reflection's WeldingType enum, by pointer (.data; the first of the class's enum table at
// 0x10D76D4). The mesh shapes' member tables store it into their weldingType members at start-up.
inline const hkClassEnum* hkpWeldingUtilityWeldingTypeEnum = &hkpWeldingUtilityEnums[0];

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpWeldingUtilityTypeInfo;
extern hkClass hkpWeldingUtilityClass;
