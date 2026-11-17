#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

// One range of a histogram: the angles between m_lo and m_hi and the weight summed over them. The exe builds one as
// three floats and inserts it into hkHistogram::m_entries (0xDDECF0); the Xbox PDB files the type with
// hkpMeshWeldingUtility.cpp, where the histogram is used.
struct hkEntry {
	float m_lo;
	float m_hi;
	float m_value;
};

// The entries' order for hkAlgorithm::quickSort: by m_value (the PDB's ??M@YA?AVhkBool@@ABUhkEntry@@0@Z; cdecl,
// the hkBool returned through a hidden pointer, 0xDDCDA0). Defined in Visualize/Process/Unplaced.cpp.
hkBool operator<(const hkEntry& a, const hkEntry& b);

// The histogram of the angles between welded triangle edges: the free angle returned when it holds no entry, and the
// entries the welding utility accumulates.
class hkHistogram {
public:
	void addEntry(float lo, float hi, float value);
	float evaluate();

	float m_freeAngle;
	hkArray<hkEntry, hkContainerHeapAllocator> m_entries;
};
