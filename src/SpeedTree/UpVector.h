#pragma once

// The UPVECTOR_POS_Z form: the point is copied unchanged.
inline void Assign3d(float* pDest, const float* pSource) {
    pDest[0] = pSource[0];
    pDest[1] = pSource[1];
    pDest[2] = pSource[2];
}
