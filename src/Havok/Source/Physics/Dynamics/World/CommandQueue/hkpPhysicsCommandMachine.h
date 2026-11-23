#pragma once

#include <stdint.h>

class hkpConstraintInstance;
struct hkpPhysicsCommand;
class hkpWorld;

// File-local helper of hkPhysicsCommandMachineProcess: adds the constraint to the locked island and ors the command's
// callback request into its internal.
void addConstraintToCriticalLockedIsland(hkpWorld* world, hkpConstraintInstance* constraint, int32_t callbackRequest);
// Runs a block of queued physics commands against the world, stopping at the first command of an unknown type.
void hkPhysicsCommandMachineProcess(hkpWorld* world, hkpPhysicsCommand* begin, int32_t size);
