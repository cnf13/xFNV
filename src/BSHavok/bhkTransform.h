#pragma once

#include "BSHavok/bhkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"

class NiTransform;

// A rotation and a translation: the placement the collision shapes are built and given to Havok in. Like bhkMatrix3
// it mirrors a Havok class and has no base and no vtable.
class bhkTransform {
public:
	bhkTransform();

	// The rotation becomes the identity and the translation the zero vector.
	void setIdentity();
	// Sets the rotation from arNiTran's matrix (transposed, no scaling) and the translation from its translation,
	// scaled to Havok units; returns this.
	bhkTransform& NI2HK(const NiTransform& arNiTran);

	// Copies the rotation and the translation into arDst's and returns arDst.
	hkTransform& ToHK(hkTransform& arDst) const {
		Rot.ToHK(arDst.getRotation());
		// EXE-INLINED 0x4B4D10
		arDst.getTranslation().set(Trans.fX, Trans.fY, Trans.fZ, Trans.fW);
		return arDst;
	}

	bhkMatrix3 Rot;
	bhkVector4 Trans;
};
