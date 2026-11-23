#pragma once

// A four-float vector. On Xenon a union of a VMX vector and the x, y, z, w struct; the PC build keeps the floats.
class NiPoint4 {
public:
    void SetX(float fX) {
        x = fX;
    }
    void SetY(float fY) {
        y = fY;
    }
    void SetZ(float fZ) {
        z = fZ;
    }
    void SetW(float fW) {
        w = fW;
    }
    float X() const {
        return x;
    }
    float Y() const {
        return y;
    }
    float Z() const {
        return z;
    }
    float W() const {
        return w;
    }

    float x;
    float y;
    float z;
    float w;
};
