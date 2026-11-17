#pragma once

#include "BSHavok/bhkAabbPhantom.h"
#include "Reconstruction.hpp"

// The phantom box actors steer around. Only its statics are declared so far: the exe has no vtable or constructor
// of its own for it (its NiRTTI is built at start-up, nothing reads it), so its members and virtuals have no PC
// evidence yet.
class bhkAvoidBox : public bhkAabbPhantom {
public:
	static NiRTTI ms_RTTI;
	// Set while the avoidance boxes are ignored; Main::RunScripts clears it every frame.
	static inline bool bNoAvoidance;
};
