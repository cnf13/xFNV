#include "Fallout Shared/GlobalFunc.h"
#include "SDK/FaceGen/assert.hpp"
#include <stdint.h>

// The exe's message format; not in the PDB. fr2Assert passes this address to Warning.
using fr2Assert_Format61_t = const char[61];
// "DEFAULT: FR2 ASSERT violation in %s line %i. Code may crash."
static fr2Assert_Format61_t fr2Assert_Format{"DEFAULT: FR2 ASSERT violation in %s line %i. Code may crash."};

void fr2Assert(const char* fname, int32_t line)
{
	Warning(fr2Assert_Format, fname, line);
}

int32_t WriteLog(const char* a1, int32_t a2, const char* a3) {
	return Warning("DEFAULT: FR2 ERROR in %s line %i : '%s'. Code may crash.", a1, a2, a3);
}
