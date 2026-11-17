#include "BSCore/MemoryManager.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/stdexcept.h"
#include "SpeedTree/Random.h"

void DoNothing(int32_t iValue) {
	;
}

// This is the exe's throw sequence, kept until the throws are plain C++: logic_error is built in the frame by its
// constructor (0x44A310) and that object goes straight to _CxxThrowException with the exe's ThrowInfo (0x11739EC), so
// the exe's catch (std::exception&) handlers of Next's callers (it is reached through Random's vtable) catch it. The
// real-C++-EH convention (exception.h) stays for the classes it covers.
// The checker's Windows models (harness/checker/emu.py MODELS) do not include RaiseException, so a call that throws
// ends as "unverifiable: calls RaiseException"; only paths that don't throw are compared.
// Throws logic_error when seed is exactly 0.0 (fldz/fcomp qword/test ah,0x44/jp: NaN does not throw). The exe's
// _Tidy of the message after the throw (0xB43419) never runs; it is written as the exe has it. The cleanup-only EH
// frame is left out. The index is Raw() * 128.0 (double) truncated; the old buffer value goes through a float local
// before Raw refills the slot.
float Random::Next() {
	if (seed == 0.0) {
		std::string strMessage;
		FNVD_RECONSTRUCT(std::string)(&strMessage, "Random number generator not initialised");
		throw std::logic_error(strMessage);
	}
	int32_t iIndex = static_cast<int32_t>(Raw() * 128.0);
	DoNothing(iIndex);
	DoNothing(iIndex);
	float fValue = Buffer[iIndex];
	Buffer[iIndex] = Raw();
	return fValue;
}

// Each name is the exe's constant string.
const char* Random::Name() {
	return "Random";
}

// The base distribution has no density.
float Random::Density(float) const {
	return 0.0f;
}

// The exe's cleanup-only C++ EH frame is left out. The string's _Tidy after the throw never runs; it is written as the
// exe has it.
void Random::load(int32_t* a1, float* a2, Random** a3) {
	using _Message_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
	FNVD_CONSTRUCTED_LOCAL(_Message_t, _Message, nullptr, "Newran: illegal combination");
	throw std::logic_error(_Message);
}

void Random::SetLong(int32_t s) {
	int32_t i;
	Random::seed = (double)s;
	for (i = 0; i < 128; ++i) {
		Random::Buffer[i] = Random::Raw();
	}
}

// Park-Miller: seed = 16807 * seed mod (2^31 - 1), by Schrage's method so nothing overflows. The new seed is stored
// as a double; the result is rounded to float before and after the scaling by 1 / (2^31 - 1).
float Random::Raw() {
	int32_t iSeed = static_cast<int32_t>(seed);
	int32_t iHi = iSeed / 127773;
	int32_t iLo = iSeed - iHi * 127773;
	int32_t iTest = 16807 * iLo - 2836 * iHi;
	if (iTest <= 0) {
		iTest += 0x7FFFFFFF;
	}
	seed = iTest;
	float fSeed = static_cast<float>(seed);
	return static_cast<float>(fSeed * 4.656612873077393e-10);
}

// The exe's cleanup-only C++ EH frame is left out. Throws unless 0 < s < 1 (fld1/fcomp, test ah,0x41/jnp, then
// fldz/fcomp, test ah,1/jne: NaN does not throw). The string's _Tidy after the throw never runs; it is written as the
// exe has it.
void Random::Set(double s) {
	if (s >= 1.0 || s <= 0.0) {
		using _Message_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
		FNVD_CONSTRUCTED_LOCAL(_Message_t, _Message, nullptr, "Newran: seed out of range");
		throw std::logic_error(_Message);
	}
	Random::seed = static_cast<double>(static_cast<int32_t>(s * 2147483648.0));
	for (int32_t i = 0; i < 128; ++i) {
		Random::Buffer[i] = Random::Raw();
	}
}

PosGen::PosGen() : xi(0.0f), sx(nullptr), sfx(nullptr), NotReady(true) {}

// The tables are freed once built.
PosGen::~PosGen() {
	if (!NotReady) {
		ourFree(sx);
		ourFree(sfx);
	}
}

float PosGen::Next() {
	float v8;
	float v9;
	int32_t v10;
	if (NotReady) {
		Build(false);
	}
	do {
		float v7 = Random::Next();
		v10 = static_cast<int32_t>(v7 * xi);
		float v6 = sx[v10];
		double v4 = sx[v10 + 1] - v6;
		v8 = Random::Next() * v4 + v6;
		float* sfx = this->sfx;
		v9 = Random::Next() * sfx[v10];
	} while (this->sfx[v10 + 1] <= static_cast<double>(v9) && static_cast<double>(v9) >= Density(v8));
	return v8;
}

const char* PosGen::Name() {
	return "PosGen";
}

ExtReal PosGen::Mean() const {
	ExtReal result;
	result.c = Missing;
	result.value = 0.0f;
	return result;
}

// The variance is unknown.
ExtReal PosGen::Variance() const {
	ExtReal result;
	result.c = Missing;
	result.value = 0.0f;
	return result;
}

// The exe's cleanup-only C++ EH frame is left out. runtime_error is built in the frame by its constructor (0xB096A0)
// and thrown with the exe's ThrowInfo; each string's _Tidy after its throw never runs and is written as the exe has
// it. f1 <= 0.0 compares with a double (test ah,0x41/jp: NaN goes on).
void PosGen::Build(bool sym) {
	double v2;
	float inc;
	int32_t i;
	NotReady = false;
	float* v10 = static_cast<float*>(ourMalloc(60 * sizeof(float)));
	sx = v10;
	float* v9 = static_cast<float*>(ourMalloc(60 * sizeof(float)));
	sfx = v9;
	float v12 = 0.0f;
	if (sym) {
		inc = 0.01f;
	} else {
		inc = 0.02f;
	}
	for (i = 0;; ++i) {
		if (i >= 60) {
			using _Message_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
			FNVD_CONSTRUCTED_LOCAL(_Message_t, strMessage, nullptr, "Newran: area too large");
			FNVD_INLINED_LOCAL(rt::runtime_error, cError, std::nullopt, std::nullopt);
			::new (&cError) rt::runtime_error(strMessage);
			throw cError;
			break;
		}
		sx[i] = v12;
		float f1 = Density(v12);
		sfx[i] = f1;
		if (f1 <= 0.0) {
			break;
		}
		v12 = inc / f1 + v12;
	}
	if (i < 50) {
		using _Message_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
		FNVD_CONSTRUCTED_LOCAL(_Message_t, strMessage, nullptr, "Newran: area too small");
		FNVD_INLINED_LOCAL(rt::runtime_error, cError, std::nullopt, std::nullopt);
		::new (&cError) rt::runtime_error(strMessage);
		throw cError;
	}
	if (sym) {
		v2 = static_cast<double>(i) * 2.0;
	} else {
		v2 = static_cast<double>(i);
	}
	xi = static_cast<float>(v2);
}

SymGen::~SymGen() {}

float SymGen::Next() {
	float v9;
	float v10;
	float v11;
	int32_t v12;
	if (NotReady) {
		Build(true);
	}
	do {
		v10 = 1.0;
		float v8 = Random::Next();
		if (v8 > 0.5) {
			v10 = -1.0f;
			v8 = 1.0 - v8;
		}
		v12 = static_cast<int32_t>(v8 * xi);
		float v7 = sx[v12];
		double v5 = sx[v12 + 1] - v7;
		v9 = Random::Next() * v5 + v7;
		float* sfx = this->sfx;
		v11 = Random::Next() * sfx[v12];
	} while (this->sfx[v12 + 1] <= static_cast<double>(v11) && static_cast<double>(v11) >= Density(v9));
	return v10 * v9;
}

const char* SymGen::Name() {
	return "SymGen";
}

// The shared tables, built by the first Normal and copied into every later one.
Normal::Normal() {
	if (count) {
		NotReady = false;
		xi = Nxi;
		sx = Nsx;
		sfx = Nsfx;
	} else {
		Build(true);
		Nxi = xi;
		Nsx = sx;
		Nsfx = sfx;
	}
	++count;
}

// While other Normals are alive, the shared tables are marked as not this one's, so that PosGen's destructor keeps
// them.
Normal::~Normal() {
	if (--count) {
		NotReady = true;
	}
}

const char* Normal::Name() {
	return aNormal_0;
}

float Normal::Density(float x) const {
	float v4;
	float v5;
	float v7 = (float)std::fabs(static_cast<double>(x));
	if (!(v7 > 8.0)) {
		v5 = (float)((x * (-x)) / 2.0);
		// The exe calls the CRT's _exp 0xECAE30 (double), not the float overload 0x630AA0.
		v4 = (float)std::exp(static_cast<double>(v5));
		return (float)(v4 * 0.3989422917366028);
	} else {
		return (float)0.0;
	}
}

ExtReal Normal::Mean() const {
	ExtReal result;
	result.c = Finite;
	result.value = 0.0f;
	return result;
}

ExtReal Normal::Variance() const {
	ExtReal result;
	result.c = Finite;
	result.value = 1.0f;
	return result;
}
