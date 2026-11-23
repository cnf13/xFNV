#pragma once

// Three rows of four floats, the layout of an hkMatrix3 of three hkVector4, with no extra alignment.
class hkMatrix3x4 {
public:
    float cr[3][4];
};
