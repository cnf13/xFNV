#pragma once

#include "Reconstruction.hpp"

class NiExtraData;
class NiTriBasedGeom;

// IDA name; not in the PDB. FaceGen helpers; only the instances the reconstructed code calls are declared.
class BSFaceGenUtils {
public:
	// The extra data of apGeom named by T's tag, or null for a null geometry.
	template <class T> static NiExtraData* GetFaceGenExtraData(NiTriBasedGeom* apGeom);
};
