#pragma once

template <class T>
class FR2Vect2TC;

// FR2Vect2TC<float>: a .tri texture coordinate. The texture vectors' rows copy one element as two floats.
template <>
class FR2Vect2TC<float> {
public:
    float x1;
    float x2;
};
