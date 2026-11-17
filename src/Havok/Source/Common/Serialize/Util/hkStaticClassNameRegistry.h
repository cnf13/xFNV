#pragma once

#include "Havok/Source/Common/Base/Reflection/Registry/hkClassNameRegistry.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// A class registry over a null terminated array of classes the program was linked with (one per Havok version). The
// exe's vtable overrides the destructor and the three registry slots.
class hkStaticClassNameRegistry : public hkClassNameRegistry {
public:
	// The two exe constructors take the same arguments and differ in m_ready only; the SDK tells them apart by the
	// classes' constness: Construct (0xDB7B40, not ready: the classes still need finishing) is the SDK's over
	// hkClass* const*, Construct_00DB7AB0 (ready) the one over const hkClass* const*.
	hkStaticClassNameRegistry(hkClass* const* classes, int32_t classVersion, const char* name);
	hkStaticClassNameRegistry(const hkClass* const* classes, int32_t classVersion, const char* name);
	~hkStaticClassNameRegistry() override;

	const char* getName() const override;
	const hkClass* getClassByName(const char* className) const override;
	void getClasses(hkArray<const hkClass*>& classes) const override;

protected:
	// The SDK has it private; protected here so that the registration can name it.
	// Makes the registry ready for lookups on first use.
	void checkIfReady() const;

public:
	const char* m_name;
	const hkClass* const* m_classes;
	int32_t m_classVersion;
	// The PDB's mutable hkBool32.
	mutable int32_t m_ready;
};
inline constexpr fnvd::Vtable hkStaticClassNameRegistry_vftable{};
