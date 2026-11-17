#pragma once

#include "BSHavok/bhkConstraint.h"
#include "Reconstruction.hpp"

// The wrapper of an hkpHingeLimitsData constraint. Only its RTTI and the virtuals it overrides are modelled.
class bhkHingeLimitsConstraint : public bhkConstraint {
public:
	bhkHingeLimitsConstraint();
	~bhkHingeLimitsConstraint() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

	// Builds a bhkHingeLimitsConstraint from NiMemObject's allocator; the stream's create function.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	// Number of live instances; CreateObject counts it (the exe's constructor bodies are inlined there).
	static inline uint32_t iCount;
};

inline constexpr fnvd::Vtable bhkHingeLimitsConstraint_vftable{};
