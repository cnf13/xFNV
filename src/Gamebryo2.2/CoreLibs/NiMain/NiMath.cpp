#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"

#include <cmath>

// The step is a double; the angle is a float that is rounded after every add. Each sin/cos result is rounded to float
// before it is stored.
void CreateSinTable() {
	float fAngle = 0.0f;
	double dStep = NI_TWO_PI * 0.001953125;
	for (uint32_t i = 0; i < 512; i++) {
		s_fSinTableA[i] = static_cast<float>(std::sin(fAngle));
		s_fCosTableA[i] = static_cast<float>(std::cos(fAngle));
		fAngle = fAngle + dStep;
	}
}

void NiSinCos(float fRadians, float& fSin, float& fCos) {
	fSin = std::sin(fRadians);
	fCos = std::cos(fRadians);
}

float NiFastATan2(float fY, float fX) {
	double v2 = 0.0;
	double v3 = fX;
	if (fX != 0.0 || 0.0 != fY) {
		double v6;
		double v4 = fY;
		float v14 = 0.0;
		float fXb = std::fabs((double)fY);
		double v5 = fXb;
		float fXc = std::fabs(v3);
		if (!(fXc < v5)) {
			float fXo = v4 / v3;
			v6 = fXo;
			if (fXo != 0.0) {
				goto LABEL_7;
			}
			double v11 = 0.0;
			if (!(v3 > 0.0)) {
				return 3.1415927f;
			}
			return v11;
		} else {
			double v7;
			float fXd;
			fXd = v3 / v4;
			v6 = fXd;
			if (fXd > 0.0) {
				v7 = NiFastATan2_NI_HALF_PI;
			LABEL_6:;
				v14 = v7;
			LABEL_7:;
				float fXe = v6 * v6;
				double v8 = fXe;
				float fXf = 0.02083509974181652 * fXe;
				float fXg = fXf - 0.08513300120830536;
				float fXh = fXg * v8;
				float fXi = fXh + 0.18014100193977356;
				float fXj = fXi * v8;
				float fXk = fXj - 0.3302994966506958;
				float fXl = v8 * fXk;
				float fXm = fXl + 0.9998660087585449;
				float fXa = v6 * fXm;
				if (v14 != 0.0) {
					fXa = v14 - fXa;
				}
				double v12 = 3.1415927410125732;
				if (v4 < 0.0) {
					if (v3 < 0.0) {
						fXa = fXa - 3.1415927410125732;
					}
					v12 = 3.1415927410125732;
				}
				if (!(fY > 0.0) || !(v3 < 0.0)) {
					return fXa;
				} else {
					return v12 + fXa;
				}
			}
			if (v6 < 0.0) {
				v7 = -NiFastATan2_NI_HALF_PI;
				goto LABEL_6;
			}
			bool v9 = v4 > 0.0;
			double v10 = NiFastATan2_NI_HALF_PI;
			if (!v9) {
				return -v10;
			}
			return v10;
		}
	}
	return v2;
}
