#include "Fallout Shared/TEX_USER_DATA.h"

#include "BSCore/NewOverloads.h"

void TEX_USER_DATA::operator delete(void* apMemory) {
    ourFree(apMemory);
}
