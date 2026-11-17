#pragma once

#include "BSHavok/bhkTaskletManagerCallback.h"
#include "Reconstruction.hpp"

// Initializes a tasklet thread's Havok router and its stack buffer.
class bhkOnStartupCallback : public bhkTaskletManagerCallback {
public:
	// The exe's initialiser (0xFBCB40): BSTaskletManagerCallback's constructor for the startup phase.
	bhkOnStartupCallback() : bhkTaskletManagerCallback(TMCB_STARTUP) {}
	~bhkOnStartupCallback() override;

	void operator()() override;
};

// The tasklet threads' exit callback (PDB: bhkWorld.cpp). Its slot 0 holds BSTaskletManagerCallback's deleting
// destructor (0xC66600).
class bhkOnExitCallback : public bhkTaskletManagerCallback {
public:
	// The exe's initialiser (0xFBCB70): BSTaskletManagerCallback's constructor for the exit phase.
	bhkOnExitCallback() : bhkTaskletManagerCallback(TMCB_EXIT) {}
	~bhkOnExitCallback() override;

	// The exe's slot holds its shared empty function (0xA29680): nothing to do on PC.
	void operator()() override {}
};

inline constexpr fnvd::Vtable bhkOnStartupCallback_vftable{};
inline constexpr fnvd::Vtable bhkOnExitCallback_vftable{};
