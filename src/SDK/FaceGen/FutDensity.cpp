#include "SDK/FaceGen/FutDensity.h"
#include <cmath>
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/time.h"
#include <stdint.h>

// This file's pi: only the initialiser below reads it, multiplying at
// run time (at the x87 precision then in force).
static const double dPi = 3.141592653589793;
uint8_t seeded;
const double FutDensityC::lnCoeff = -std::log(2.0 * dPi) / 2.0;

double FutDensityC::rngUniform()
{
	if (!seeded) {
		rt::srand(static_cast<uint32_t>(std::time(nullptr)));
		seeded = 1;
	}
	// The exe keeps the base in a local (fstp qword [ebp-8]) and multiplies it out at the end.
	double dBase = 32768.0;
	double dResult = 0.0;
	for (int32_t i = 0; i < 4; ++i) {
		dResult = static_cast<double>(rt::rand()) + dResult * dBase;
	}
	return dResult / (dBase * dBase * dBase * dBase);
}

double RngNormal(double* apThis) {
	double X;
	double v8;
	do {
		do {
			double v7 = FutDensityC::rngUniform() * 2.0 + -1.0;
			v8 = FutDensityC::rngUniform() * 2.0 + -1.0;
			X = v7 * v7 + v8 * v8;
		} while (X > 1.0);
	} while (0.0 == X);
	double v5 = *apThis;
	double v3 = 1.0 / *(apThis + 2) * v8;
	double v1 = std::log(X);
	return std::sqrt(v1 * -2.0 / X) * v3 + v5;
}

double RngStdNormal() {
	double X;
	double v4;
	do {
		do {
			double v3 = FutDensityC::rngUniform() * 2.0 + -1.0;
			v4 = FutDensityC::rngUniform() * 2.0 + -1.0;
			X = v3 * v3 + v4 * v4;
		} while (X > 1.0);
	} while (0.0 == X);
	double v0 = std::log(X);
	return std::sqrt(v0 * -2.0 / X) * v4;
}
