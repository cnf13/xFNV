#include "BSHavok/bhkMatrix3.h"

// The engine's constructor leaves each column to bhkVector4's constructor, which clears it.
bhkMatrix3::bhkMatrix3() {}

void bhkMatrix3::setIdentity() {
    Col0.setZero4();
    Col1.setZero4();
    Col2.setZero4();
    Col0.fX = 1.0f;
    Col1.fY = 1.0f;
    Col2.fZ = 1.0f;
}
