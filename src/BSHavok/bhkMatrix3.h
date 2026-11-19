#pragma once

#include "Fallout Shared/GlobalFunc.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"

class NiMatrix3;

// The 3x3 matrix a bhkTransform holds; its columns are bhkVector4s, so a new matrix is all zeros. Like Havok's
// hkMatrix3 the class has no base and no vtable.
class bhkMatrix3 {
public:
    bhkMatrix3();

    // Sets the columns from the NetImmerse matrix apMatrix's columns.
    bhkMatrix3& NI2HKNS(const NiMatrix3& arSrc);

    // Clears all three columns and puts ones on the diagonal.
    void setIdentity();

    // Copies the three columns into arDst's and returns arDst.
    hkRotation& ToHK(hkRotation& arDst) const {
        Col0.ToHK(arDst.m_col0);
        Col1.ToHK(arDst.m_col1);
        Col2.ToHK(arDst.m_col2);
        return arDst;
    }

    void Renormalize();

    bhkVector4 Col0;
    bhkVector4 Col1;
    bhkVector4 Col2;
};
