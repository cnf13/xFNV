#include "Gamebryo2.2/CoreLibs/NiMain/NiCompUtility.h"

void NiCompUtility::DecompressFloatArray(const int16_t* psSrcArray, uint32_t uiSrcCount, float fOffset,
	float fHalfRange, float* pfDestArray, uint32_t uiDestCount) {
	int16_t* v11;
	uint32_t v12;
	float* v13;
	int32_t uiDestCounta;
	int32_t uiDestCountb;
	double v6 = (double)fHalfRange;
	int16_t* v7 = const_cast<int16_t*>(psSrcArray);
	double v8 = (double)fOffset;
	uint32_t v10 = 0;
	if (((int32_t)uiDestCount) >= 4) {
		v11 = const_cast<int16_t*>(psSrcArray + 2);
		v12 = ((uiDestCount - 4) >> 2) + 1;
		v13 = pfDestArray + 2;
		v10 = 4 * v12;
		do {
			uiDestCounta = (int32_t)(*(v11 - 2));
			v11 += 4;
			v13 += 4;
			--v12;
			*(v13 - 6) = (float)(((((double)uiDestCounta) / 32767.0) * v6) + v8);
			*(v13 - 5) = (float)(((((double)(*(v11 - 5))) / 32767.0) * v6) + v8);
			*(v13 - 4) = (float)(((((double)(*(v11 - 4))) / 32767.0) * v6) + v8);
			*(v13 - 3) = (float)(((((double)(*(v11 - 3))) / 32767.0) * v6) + v8);
		} while (v12);
		v7 = const_cast<int16_t*>(psSrcArray);
	}
	for (; v10 < uiDestCount; pfDestArray[v10 - 1] = (float)(((((double)uiDestCountb) / 32767.0) * v6) + v8)) {
		uiDestCountb = (int32_t)v7[v10++];
	}
}
