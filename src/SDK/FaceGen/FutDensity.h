#pragma once

#include "Reconstruction.hpp"

// Not named in the PDB or IDA; name chosen here. A normal deviate: RngStdNormal's polar method
// over two uniform draws (0xAC6220), scaled by 1 / apThis[2] and offset by apThis[0] (thiscall, ret 0; the result in
// ST0). No direct caller.
double RngNormal(double* apThis);

// Inferred type: return: double: used as one at 0xaad60d
double RngStdNormal();

extern uint8_t seeded;

// The PDB's class of the density statics (its seeded flag is seeded above).
class FutDensityC {
public:
	// -ln(sqrt(2 pi)), the Gaussian density's log normaliser; no code of the exe reads it.
	static const double lnCoeff;
	// Not named in the PDB or IDA; name chosen here. A uniform deviate in [0, 1): four rand() draws as base-32768 digits;
	// the CRT is seeded from _time64 on the first call (cdecl, ret 0; the result in ST0).
	static double rngUniform();
};
