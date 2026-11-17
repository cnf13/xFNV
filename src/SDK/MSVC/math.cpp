#include "SDK/MSVC/math.h"

double _Pow_int(float a1, int32_t a2)
{
	uint32_t v6;
	if (a2 < 0) {
		v6 = (uint32_t)(-a2);
	} else {
		v6 = (uint32_t)a2;
	}
	float v5 = (float)1.0;
	while (1) {
		if ((v6 & 1) != 0) {
			v5 = v5 * a1;
		}
		v6 >>= 1;
		if (!v6) {
			break;
		}
		a1 = (float)(a1 * a1);
	}
	if (a2 >= 0) {
		return v5;
	} else {
		return (float)(1.0 / v5);
	}
}
