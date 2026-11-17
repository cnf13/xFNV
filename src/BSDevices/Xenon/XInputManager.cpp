#include "BSDevices/Xenon/XInputManager.h"

#include "Fallout Shared/GlobalFunc.h"

uint32_t BSMAX(uint32_t iA, uint16_t iB) {
	if (iA <= iB) {
		return iB;
	} else {
		return iA;
	}
}

char BSMIN(char iA, char iB) {
	if (iA >= iB) {
		return iB;
	} else {
		return iA;
	}
}

// The larger of two floats: iB unless it is below iA (fcompp, test ah, 5, jp: the jump takes iB for a tie and for a
// NaN).
float BSMAX(float afA, float afB) {
	if (afB < afA) {
		return afA;
	}
	return afB;
}

// The smaller of two floats: iB only when it is below iA (fcompp, test ah, 0x41, jne: a tie falls through to iA and a
// NaN takes the jump to iB).
float BSMIN(float afA, float afB) {
	if (!(afB >= afA)) {
		return afB;
	}
	return afA;
}

// The larger of two unsigned ints: iB unless iA is above it (unsigned jbe).
uint32_t BSMAX(uint32_t auiA, uint32_t auiB) {
	if (auiA <= auiB) {
		return auiB;
	}
	return auiA;
}

// The smaller of two ints (the int instantiation): iA unless it is not below iB (signed jge).
// KEEP, not registered (0x4A8F20): an exe caller reads ecx after the call, relying on the exe body's ecx (iA when iA
// is below iB, else unchanged), which clang does not guarantee.
int32_t BSMIN(int32_t aiA, int32_t aiB) {
	if (aiA >= aiB) {
		return aiB;
	}
	return aiA;
}

// The smaller of two unsigned ints (the unsigned int instantiation): iA when it is below iB (unsigned jae), else iB.
uint32_t BSMIN(uint32_t auiA, uint32_t auiB) {
	if (auiA >= auiB) {
		return auiB;
	}
	return auiA;
}
