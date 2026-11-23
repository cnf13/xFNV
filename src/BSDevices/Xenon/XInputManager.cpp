#include "BSDevices/Xenon/XInputManager.h"

#include "Fallout Shared/GlobalFunc.h"

uint32_t BSMAX(uint32_t iA, uint16_t iB) {
    if (iA <= iB) {
        return iB;
    }

    return iA;
}

char BSMIN(char iA, char iB) {
    if (iA >= iB) {
        return iB;
    }

    return iA;
}

// The larger of two floats: afB unless it is below afA, so afB is returned for a tie and for a NaN.
float BSMAX(float afA, float afB) {
    if (afB < afA) {
        return afA;
    }
    return afB;
}

// The smaller of two floats: afB only when it is below afA (a tie returns afA and a NaN returns afB).
float BSMIN(float afA, float afB) {
    if (!(afB >= afA)) {
        return afB;
    }
    return afA;
}

// The larger of two unsigned ints.
uint32_t BSMAX(uint32_t auiA, uint32_t auiB) {
    if (auiA <= auiB) {
        return auiB;
    }
    return auiA;
}

int32_t BSMIN(int32_t aiA, int32_t aiB) {
    if (aiA >= aiB) {
        return aiB;
    }
    return aiA;
}

uint32_t BSMIN(uint32_t auiA, uint32_t auiB) {
    if (auiA >= auiB) {
        return auiB;
    }
    return auiA;
}
