#pragma once

#include "Reconstruction.hpp"

class NiAVObject;
class NiPoint3;

// Gamebryo's collision helpers (a namespace: the PDB has no class record for it). Only the functions src defines are
// declared.
namespace NiCollisionUtils {
// Whether the ray from kOrigin along kDir meets pkObj's world bound.
bool FindBoundIntersect(NiAVObject* pkObj, const NiPoint3& kOrigin, const NiPoint3& kDir);
// Inferred type: a1: NiPoint3* from 0xeb899e (parameter 0 of NiOBBLeaf::FindIntersections), its members used at
// 0xeaddde; a2: NiPoint3* from 0xeb899e (parameter 1 of NiOBBLeaf::FindIntersections), its members used at 0xeadbdf;
// a3: NiPoint3* from 0xeb899e (NiOBBLeaf::m_pWorldVertex (this of NiOBBLeaf::FindIntersections)), its members used at
// 0xeadb9d; a4: NiPoint3* from 0xeb899e (NiOBBLeaf::m_pWorldVertex (this of NiOBBLeaf::FindIntersections)), its members
// used at 0xeadb9b
char IntersectTriangle(const NiPoint3* a1, const NiPoint3* a2, const NiPoint3* a3, const NiPoint3* a4, const float* a5,
	char a6, NiPoint3* a7, float* a8, float* a9, float* a10);
} // namespace NiCollisionUtils
