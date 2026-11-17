#pragma once

#include <cstdint>

// An empty function Random::Next calls on the buffer index.
void DoNothing(int32_t);

// What kind of value an ExtReal holds.
enum EXT_REAL_CODE : int32_t {
	Finite = 0,
	PlusInfinity = 1,
	MinusInfinity = 2,
	Indefinite = 3,
	Missing = 4,
};

// A real that can also be infinite, indefinite or missing (the mean and variance of a distribution).
class ExtReal {
public:
	ExtReal() = default;
	ExtReal(float v);

	float value;
	EXT_REAL_CODE c;
};

// SpeedTree's copy of Robert Davies' random number library: the uniform generator over a shared seed, and the base
// of the distributions (PosGen, SymGen, Normal, Uniform). SpeedTree builds it with float reals.
class Random {
public:
	// The shared buffer of the uniform generator (Real is float in SpeedTree's build).
	using BufferArray = float[128];

	virtual ~Random();

	// The next value of the distribution.
	virtual float Next();
	// The name of the distribution.
	virtual const char* Name();
	// The probability density at the value.
	virtual float Density(float) const;
	virtual ExtReal Mean() const;
	virtual ExtReal Variance() const;
	// The number of values Next returns at a time.
	virtual int32_t nelems() const;
	virtual void tDelete() {}
	virtual void load(int32_t*, float*, Random**);

	// Deleting destructors free through ourFree.
	static void operator delete(void* apMemory);
	static void SetLong(int32_t s);

protected:
	// The next uniform value in (0, 1): one step of the Park-Miller minimal standard generator on seed.
	static float Raw();

public:
	// Seeds the generator with s in (0, 1) and refills the buffer; throws logic_error outside it.
	static void Set(double s);

	static inline BufferArray Buffer{};

	// The shared seed of the uniform generator.
	static inline double seed;
};

// The uniform distribution on [0, 1].
class Uniform : public Random {
public:
	~Uniform() override;

	// Random's next value.
	float Next() override;
	const char* Name() override;
	// 1 inside [0, 1], 0 outside; NaN gives 1.
	float Density(float x) const override;
};

// The distributions with a positive density: Next looks the value up in tables built on first use (xi, sx, sfx).
class PosGen : public Random {
public:
	PosGen();
	~PosGen() override;

	float Next() override;
	const char* Name() override;
	ExtReal Mean() const override;
	ExtReal Variance() const override;

	void Build(bool sym);

	float xi;
	float* sx;
	float* sfx;
	bool NotReady;
	uint8_t padding11[3];
};

// The distributions symmetric about 0: PosGen's value with a random sign.
class SymGen : public PosGen {
public:
	~SymGen() override;

	float Next() override;
	const char* Name() override;
};

// The standard normal distribution; its tables are static and shared by every instance.
class Normal : public SymGen {
public:
	Normal();
	~Normal() override;

	const char* Name() override;
	float Density(float x) const override;
	ExtReal Mean() const override;
	ExtReal Variance() const override;

	// The tables the first Normal builds (xi, sx, sfx), copied by every later one, and the number of Normals alive.
	static inline float Nxi;
	static inline float* Nsx;
	static inline float* Nsfx;
	static inline int32_t count;
};
