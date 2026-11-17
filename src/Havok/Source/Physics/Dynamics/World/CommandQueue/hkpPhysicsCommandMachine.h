#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

class hkpConstraintInstance;
struct hkpPhysicsCommand;
class hkpWorld;

// The PDB's file-local helper of hkPhysicsCommandMachineProcess (0xDF25F0): adds the constraint to the locked island
// and ors the command's callback request into its internal. The exe passes world in eax, constraint in esi and
// callbackRequest on the stack (cdecl, ret 0).
void addConstraintToCriticalLockedIsland(hkpWorld* world, hkpConstraintInstance* constraint, int32_t callbackRequest);
// Runs a block of queued physics commands against the world, stopping at the first command of an unknown type.
void hkPhysicsCommandMachineProcess(hkpWorld* world, hkpPhysicsCommand* begin, int32_t size);
