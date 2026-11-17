#pragma once

class hkpWorld;

// The world's heap accounting: the memory watchdog is told what the world allocated before and after each step.
class hkpWorldMemoryUtil {
public:
    // Splits or marks the largest islands (or has the watchdog reduce their constraints) until the solver allocator
    // can hold the integration's largest block.
    static void checkMemoryForIntegration(hkpWorld* world);
    // Hands apWorld's memory watchdog the heap's use.
    static void watchHeapMemory(hkpWorld* world);
    static void tryToRecoverFromMemoryErrors(hkpWorld* world);
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    static void repeatCollideAndToiHandling(hkpWorld* world);
};
