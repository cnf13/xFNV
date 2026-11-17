#include "BSHavok/bhkTransform.h"

// The engine's constructor leaves the rotation to bhkMatrix3's constructor and the translation to bhkVector4's.
bhkTransform::bhkTransform() {}

void bhkTransform::setIdentity() {
	Rot.setIdentity();
	Trans.setZero4();
}
