#pragma once

#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppVirtualMachine.h"
#include "Reconstruction.hpp"

class hkSphere;
class hkpMoppCode;

// The MOPP interpreter that collects the primitives overlapping a sphere, in the code's integer space: the sphere's
// centre and radius. Layout from the release Havok PDB (hkpcollide.pdb, hkpMoppSphereVirtualMachine, sizeof 32,
// members m_x at +0x10 ... m_radius at +0x1C); the exe's querySphere 0xE64F00 writes m_x at +0x10 and m_radius at
// +0x1C.
class hkpMoppSphereVirtualMachine : public hkpMoppVirtualMachine {
public:
	// The query the walk carries: the sphere and the code's integer-space offsets, the primitive offset, the shift
	// and the properties. Layout from the release Havok PDB (hkpcollide.pdb, sizeof 40); the caller querySphere
	// 0xE64F00 fills a 40-byte local (the four ints at +0, the offsets and primitive offset zeroed at +0x10..+0x1C,
	// the shift 16 at +0x20 and the properties at +0x24) and passes it to querySphereOnTree.
	struct hkpMoppSphereVirtualMachineQuery {
		int32_t m_x;
		int32_t m_y;
		int32_t m_z;
		int32_t m_radius;
		int32_t m_offset_x;
		int32_t m_offset_y;
		int32_t m_offset_z;
		uint32_t m_primitiveOffset;
		int32_t m_shift;
		uint32_t m_properties[1];
	};

	// The PDB's querySphere: stores the output array, the sphere's centre in the code's
	// integer space less one and its radius plus two, and walks the tree from the code's data (thiscall, ret 0xC,
	// 0xE64F00). The parameters are the SDK's, in the order the exe reads them (the code's +0x10 offset, +0x1C
	// scale and +0x20 data; the sphere's four floats; the array into +0).
	void querySphere(const hkpMoppCode* code, const hkSphere& sphere, hkArray<hkpMoppPrimitiveInfo>* primitiveResults);
	// Walks the code under the query, narrowing the sphere at each node and appending the hits (0xE64870, ret 8). The
	// PDB's protected member; public here so that its address can be tied (FNVD_ADDRESS).
	void querySphereOnTree(const hkpMoppSphereVirtualMachineQuery* query, const uint8_t* PC);

	int32_t m_x;
	int32_t m_y;
	int32_t m_z;
	int32_t m_radius;
};

// The flag hkpProcessFlyingColors fills for querySphereOnTree, which
// does nothing
// while it is clear.
inline uint8_t hkpMoppSphereVirtualMachine_HK_flyingcolors_mopp;
