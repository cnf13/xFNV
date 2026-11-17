#include "SpeedTree/LibRandom_Source/IdvRandom.h"

#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/time.h"
#include "SpeedTree/Random.h"

Uniform SIdvRandomImpl::m_cUniform;
Normal SIdvRandomImpl::m_cNormal;

// The shared generator is seeded by the first CIdvRandom.
CIdvRandom::CIdvRandom() {
	if (!SIdvRandomImpl::m_bInit) {
		Reseed(-1);
	}
}

CIdvRandom::~CIdvRandom() {}

// Keeps the range as a double and rounds the result to float. Calls Random::Next directly rather than through the
// vtable, so the call is qualified.
float CIdvRandom::GetUniform(float fMin, float fMax) {
	double dRange = (double)fMax - (double)fMin;
	return (float)(SIdvRandomImpl::m_cUniform.Random::Next() * dRange + fMin);
}

// Only the low 32 bits of _time64's result are kept. The seed's range constant is the float 2^31.
void CIdvRandom::Reseed(int32_t lSeed) {
	if (lSeed == -1) {
		int32_t nNewSeed = 12345;
		if (SIdvRandomImpl::m_bInit) {
			nNewSeed = static_cast<int32_t>(GetUniform(0.0f, 2147483648.0f));
		}
		int32_t nTimeValue = static_cast<int32_t>(std::time(nullptr));
		if (nNewSeed <= nTimeValue) {
			Random::Set(static_cast<double>(nNewSeed) / static_cast<double>(nTimeValue));
		} else {
			Random::Set(static_cast<double>(nTimeValue) / static_cast<double>(nNewSeed));
		}
	} else {
		if (lSeed <= 1) {
			lSeed = 1;
		}
		Random::SetLong(lSeed);
	}
	SIdvRandomImpl::m_bInit = 1;
}

Random::~Random() {}

ExtReal Random::Mean() const {
	ExtReal result;
	result.c = Finite;
	result.value = 0.5f;
	return result;
}

ExtReal Random::Variance() const {
	ExtReal result;
	result.c = Finite;
	result.value = 0.083333336f;
	return result;
}

int32_t Random::nelems() const {
	return 1;
}

void Random::operator delete(void* apMemory) {
	ourFree(apMemory);
}

Uniform::~Uniform() {}

float Uniform::Next() {
	return Random::Next();
}

const char* Uniform::Name() {
	return "Uniform";
}

// Compares x with the double constants 0.0 and 1.0.
float Uniform::Density(float x) const {
	return (x < 0.0 || x > 1.0) ? 0.0f : 1.0f;
}
