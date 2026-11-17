#pragma once

class NiExtraData;
class NiTriBasedGeom;

// FaceGen helpers; only the instances this code calls are declared.
class BSFaceGenUtils {
public:
	// The extra data of apGeom named by T's tag, or null for a null geometry.
	template <class T> static NiExtraData* GetFaceGenExtraData(NiTriBasedGeom* apGeom);
};
