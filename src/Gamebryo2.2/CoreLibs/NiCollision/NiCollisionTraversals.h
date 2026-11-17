#pragma once

#include "Reconstruction.hpp"

class NiAVObject;
class NiNode;
class NiPick;
class NiPoint3;
class NiSwitchNode;
class NiTriBasedGeom;
class NiTriShape;
class NiTriStrips;

// Gamebryo's collision and pick traversals of a scene graph.
// Only the members called so far; the rest is not reconstructed yet.
class NiCollisionTraversals {
public:
	// . Adds the hits of the ray from kOrigin along kDir under pkRoot
	// to kPick's results; whether there were any.
	static bool FindIntersections(const NiPoint3& kOrigin, const NiPoint3& kDir, NiPick& kPick, NiAVObject* pkRoot);
	// Picks only the switch node's active child; a first-hit unsorted pick that already has a result stops at once.
	static bool FindIntersectionsSwitchNode(const NiPoint3& kOrigin, const NiPoint3& kDir, NiPick& kPick,
		NiSwitchNode* pkSwitch);
	// . Adds the hits of the ray under pkNode's children to kPick's
	// results; whether there were any.
	static bool FindIntersectionsNode(const NiPoint3& kOrigin, const NiPoint3& kDir, NiPick& kPick, NiNode* pkNode);
	// Adds the ray's hits on a triangle-based geometry to kPick's results; whether there were any.
	static bool FindIntersectionsTriBasedGeom(const NiPoint3& kOrigin, const NiPoint3& kDir, NiPick& kPick,
		NiTriBasedGeom* pkTBG);
	// Adds the ray's hits on a one-strip geometry's triangles to kPick's results; whether there were any (cdecl,
	// ret 0, 0xE993D0). The PDB's name and parameters; IDA calls it FindIntersectionsTriStrips.
	static bool FindIntersectionsOneStripFastPath(const NiPoint3& kOrigin, const NiPoint3& kDir, NiPick& kPick,
		NiTriStrips* pkTBG);
	// Adds the ray's hits on a triangle-list shape's triangles to kPick's results; whether there were any (cdecl,
	// ret 0, 0xE9A280). The PDB's name and parameters; IDA calls it FindIntersectionsTriShape.
	static bool FindIntersectionsTriShapeFastPath(const NiPoint3& kOrigin, const NiPoint3& kDir, NiPick& kPick,
		NiTriShape* pkTBG);

	static inline bool sbPickParticles;
	static inline bool sbPickLines;
};
