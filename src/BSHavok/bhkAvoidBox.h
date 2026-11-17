#pragma once

#include "BSHavok/bhkAabbPhantom.h"

// The phantom box actors steer around. Only its statics are declared; its members and virtual functions are not.
class bhkAvoidBox : public bhkAabbPhantom {
public:
    static NiRTTI ms_RTTI;
    // Set while the avoidance boxes are ignored; Main::RunScripts clears it every frame.
    static inline bool bNoAvoidance;
};
