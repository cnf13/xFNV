#include "SDK/FaceGen/FanControls.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Fallout Shared/FaceGen/FR2MatrixCTC.h"
#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"
#include "Fallout Shared/FaceGen/FanLinCtlGrpS.h"
#include "SDK/FaceGen/FutDensity.h"
#include "SDK/FaceGen/TriFileIO.h"
#include "SDK/FaceGen/assert.hpp"
#include "SDK/FaceGen/binaryFile.h"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/ehdata.h"
#include "SDK/MSVC/exception.h"
#include "SDK/MSVC/iosfwd.h"
#include "SDK/MSVC/string.h"
#include <vector>
#include <memory>
#include <string>
#include <stdint.h>

struct FanLinCtlS;

FanLinCtlS::FanLinCtlS(const FanLinCtlS& _Right) : ctl(_Right.ctl), label(_Right.label) {}

FanLinCtlS::~FanLinCtlS() {}

bool FanLinCtlGrpS::readFromFile(FutBinaryFileC& file, uint32_t (*dims)[2]) {
	uint32_t numCtls;
	uint32_t labSize;
	for (uint32_t tt = 0; tt < 2; ++tt) {
		for (uint32_t ss = 0; ss < 2; ++ss) {
			if (!file.read(&numCtls, 4u, 1u)) {
				return 0;
			}
			grp[tt][ss].resize(numCtls);
			for (uint32_t ii = 0; ii < numCtls; ++ii) {
				FR2MatrixVTC<float>* v14 = &grp[tt][ss][ii].ctl;
				size_t v13 = (*dims)[2 * tt + ss];
				v14->nrows = 1;
				v14->ncols = v13;
				v14->data.resize(v13, 0.0f);
				if (!file.read(grp[tt][ss][ii].ctl.operator[](0u).p, 4u, (*dims)[2 * tt + ss])) {
					return 0;
				}
				if (!file.read(&labSize, 4u, 1u)) {
					return 0;
				}
				std::string* p_label = &grp[tt][ss][ii].label;
				if (labSize > p_label->size()) {
					p_label->append(labSize - p_label->size(), 0);
				} else {
					p_label->erase(labSize, std::string::npos);
				}
				if (!file.read(&grp[tt][ss][ii].label[0], 1u, labSize)) {
					return 0;
				}
			}
		}
	}
	return 1;
}

// The exe's cleanup-only C++ EH frame is left out. The elements are read through row 0 of each matrix (the
// out-of-line operator[] 0x60B370, its RowT returned through a hidden result).
bool FanGenCtlsGrpC::readFromFile(FutBinaryFileC& file, uint32_t (*dims)[2]) {
	FR2MatrixVTC<float> v61;
	FR2MatrixVTC<float> v62;
	FR2MatrixVTC<float> v63;
	FR2MatrixVTC<float> v64;
	FR2MatrixVTC<float> v66;
	FR2MatrixVTC<float> apResult;
	FR2MatrixVTC<float> v68;
	uint32_t i;
	for (i = 0; i < 5; ++i) {
		for (uint32_t j = 0; j < 2; ++j) {
			for (uint32_t k = 0; k < 2; ++k) {
				size_t v50 = (*dims)[2 * k];
				FR2MatrixVTC<float>* v51 = &attr[i][j][k].ctl;
				v51->nrows = 1;
				v51->ncols = v50;
				v51->data.resize(v50, 0.0f);
				if (!file.read(attr[i][j][k].ctl.operator[](0u).p, 4u, (*dims)[2 * k])) {
					return 0;
				}
				if (!file.read(&attr[i][j][k].off, 4u, 1u)) {
					return 0;
				}
				double v4 = attr[i][j][k].ctl.lengthSqr();
				attr[i][j][k].scale = 1.0 / v4;
			}
		}
	}
	for (i = 0; i < 5; ++i) {
		for (uint32_t n = 0; n < 5; ++n) {
			if (n != i) {
				float v84 = 0.0f;
				for (uint32_t ii = 0; ii < 2; ++ii) {
					size_t v46 = (*dims)[2 * ii];
					FR2MatrixVTC<float>* v47 = &race[i][n].ctl[ii];
					v47->nrows = 1;
					v47->ncols = v46;
					v47->data.resize(v46, 0.0f);
					if (!file.read(race[i][n].ctl[ii].operator[](0u).p, 4u, (*dims)[2 * ii])) {
						return 0;
					}
					double v5 = race[i][n].ctl[ii].lengthSqr();
					v84 = v5 + v84;
				}
				if (!file.read(&race[i][n].off, 4u, 1u)) {
					return 0;
				}
				race[i][n].scale = 1.0 / v84;
			}
		}
	}
	uint32_t v89 = (*dims)[2] + (*dims)[0];
	for (i = 0; i < 5; ++i) {
		uint32_t jj;
		for (jj = 0; jj < 2; ++jj) {
			uint32_t a2 = (*dims)[2 * jj];
			FR2MatrixVTC<float>* v43 = &distrib[i].mean[jj];
			v43->nrows = a2;
			v43->ncols = 1;
			v43->data.resize(a2, 0.0f);
			if (!file.read(distrib[i].mean[jj].operator[](0u).p, 4u, a2)) {
				return 0;
			}
		}
		FR2MatrixVTC<float>* p_eigBasis = &distrib[i].eigBasis;
		distrib[i].eigBasis.nrows = v89;
		p_eigBasis->ncols = v89;
		p_eigBasis->data.resize(v89 * v89, 0.0f);
		if (!file.read(distrib[i].eigBasis.operator[](0u).p, 4u, v89 * v89)) {
			return 0;
		}
		for (jj = 0; jj < 2; ++jj) {
			uint32_t v80 = (*dims)[2 * jj];
			FR2MatrixVTC<float>* v37 = &distrib[i].invEigBasis[jj];
			distrib[i].invEigBasis[jj].nrows = v80;
			v37->ncols = v80;
			v37->data.resize(v80 * v80, 0.0f);
			if (!file.read(distrib[i].invEigBasis[jj].operator[](0u).p, 4u, v80 * v80)) {
				return 0;
			}
		}
	}
	for (i = 0; i < 5; ++i) {
		for (uint32_t kk = 0; kk < 2; ++kk) {
			float v34 = attr[i][0][kk].ctl.lengthSqr();
			float v18 = std::sqrt(static_cast<double>(v34));
			float v77 = v18;
			if (!(v18 > 0.001)) {
				fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 144);
			}
			FR2MatrixVTC<float>* hProcess = attr[i][0][kk].ctl.DivScalar(&v68, v77);
			basis[4 * i + 2 * kk].operator=(*hProcess);
			FR2MatrixVTC<float>* m = basis[4 * i + 2 * kk].transpose(&apResult);
			FR2MatrixVTC<float>* v15 = attr[i][1][kk].ctl.Mul(&v66, *m);
			float v78 = *v15->operator[](0u).p;
			FR2MatrixVTC<float>* arg4 = basis[4 * i + 2 * kk].MulScalar(&v64, v78);
			FR2MatrixVTC<float>* v13 = attr[i][1][kk].ctl.Sub(&v63, *arg4);
			basis[4 * i + 1 + 2 * kk].operator=(*v13);
			float v28 = basis[4 * i + 1 + 2 * kk].lengthSqr();
			float v12 = std::sqrt(static_cast<double>(v28));
			v77 = v12;
			if (!(v12 > 0.001)) {
				fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 152);
			}
			float v11 = 1.0 / v77;
			basis[4 * i + 1 + 2 * kk].operator*=(v11);
		}
	}
	for (i = 0; i < 5; ++i) {
		for (uint32_t mm = 0; mm < 2; ++mm) {
			FR2MatrixCTC<float, 2, 2> kInverse;
			std::memset(&invAttCov[i][mm], 0, sizeof(invAttCov[i][mm]));
			for (uint32_t nn = 0; nn < 2; ++nn) {
				for (uint32_t i1 = 0; i1 < 2; ++i1) {
					float* v10 = &invAttCov[i][mm].data[nn][i1];
					FR2MatrixVTC<float>* v9 = attr[i][i1][mm].ctl.transpose(&v62);
					FR2MatrixVTC<float>* v8 = attr[i][nn][mm].ctl.Mul(&v61, *v9);
					*v10 = *v8->operator[](0u).p + *v10;
				}
			}
			// The 2 x 2 inverse (Invert2x2) takes and returns its matrices as their four floats, row by row; the
			// result is copied back element by element.
			float* v6 = Invert2x2(&kInverse.data[0][0], &invAttCov[i][mm].data[0][0]);
			float v21 = v6[0];
			float v22 = v6[1];
			float v23 = v6[2];
			float v24 = v6[3];
			FR2MatrixCTC<float, 2, 2>* v7 = &invAttCov[i][mm];
			v7->data[0][0] = v21;
			v7->data[0][1] = v22;
			v7->data[1][0] = v23;
			v7->data[1][1] = v24;
		}
	}
	initialized = 1;
	return 1;
}

// An empty control matrix and an empty label.
FanLinCtlS::FanLinCtlS() {}

// The exe's cleanup-only C++ EH frame is left out.
void FanGenCtlsGrpC::setAttribute(FanRaceE race, FanAttribE att, FanTypeE type, const float val,
	FR2MatrixVTC<float> (*coord)[2]) {
	FR2MatrixVTC<float> v10;
	FR2MatrixVTC<float> apResult;
	if (race >= FANRACE_SIZE) {
		fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 222);
	}
	if (initialized) {
		float attBoundHi[2];
		FR2Vect2TC<float> delta;
		FR2Vect2TC<float> vectDiff;
		float attBoundLo[2];
		attBoundLo[0] = 15.0f;
		attBoundLo[1] = -4.0f;
		attBoundHi[0] = 65.0f;
		attBoundHi[1] = 4.0f;
		float newVal = val;
		if (attBoundLo[att] > static_cast<double>(val)) {
			newVal = attBoundLo[att];
		}
		if (attBoundHi[att] < static_cast<double>(newVal)) {
			newVal = attBoundHi[att];
		}
		double Attribute = getAttribute(race, att, type, coord);
		float valDiff = newVal - Attribute;
		vectDiff.x1 = 0.0f;
		vectDiff.x2 = 0.0f;
		*(&vectDiff.x1 + att) = valDiff;
		// The 2 x 2 inverse goes to the matrix-vector product as its four floats, row by row.
		MultiplyMatrixVector(&delta.x1, &invAttCov[race][type].data[0][0], &vectDiff.x1);
		FR2MatrixVTC<float>* v8 = attr[race][att][type].ctl.transpose(&apResult);
		FR2MatrixVTC<float>* m = v8->MulScalar(&v10, *(&delta.x1 + att));
		(*coord)[2 * type].operator+=(*m);
	}
}

// Each output coordinate is the current one plus a normal deviate (RngStdNormal) scaled by randomness and 1.5. Both rows
// come from the out-of-line operator[] (0x60B370, the const one folded with it); the element accesses are inline.
void FanGenCtlsGrpC::geneticRandom(float randomness, const FR2MatrixVTC<float> (*currCoord)[2],
	FR2MatrixVTC<float> (*outCoord)[2]) {
	for (int32_t j = 0; j < 2; ++j) {
		for (int32_t k = 0; k < 2; ++k) {
			uint32_t ncols = (*currCoord)[2 * j + k].ncols;
			uint32_t nrows = (*currCoord)[2 * j + k].nrows;
			FR2MatrixVTC<float>* v8 = &(*outCoord)[2 * j + k];
			v8->nrows = nrows;
			v8->ncols = ncols;
			v8->data.resize(ncols * nrows, 0.0f);
			for (uint32_t i = 0; i < (*currCoord)[2 * j + k].nrows; ++i) {
				for (uint32_t m = 0; m < (*currCoord)[2 * j + k].ncols; ++m) {
					double v13 = RngStdNormal() * randomness * 1.5;
					const float* pCurr = &(*currCoord)[2 * j + k][i].p[m];
					float* pOut = &(*outCoord)[2 * j + k][i].p[m];
					*pOut = *pCurr + v13;
				}
			}
		}
	}
}

float* Invert2x2(float* a1, float* a2)
{
	float v3 = ((*a2) * a2[3]) - (a2[1] * a2[2]);
	float v4 = (float)(1.0 / v3);
	float v5 = a2[3] * v4;
	float v6 = (-a2[1]) * v4;
	float v7 = (-a2[2]) * v4;
	float v8 = (*a2) * v4;
	*a1 = v5;
	a1[1] = v6;
	a1[2] = v7;
	a1[3] = v8;
	return a1;
}

float* MultiplyMatrixVector(float* a1, float* a2, float* a3)
{
	float v4 = ((*a2) * (*a3)) + (a2[1] * a3[1]);
	float v5 = (a2[2] * (*a3)) + (a2[3] * a3[1]);
	*a1 = v4;
	a1[1] = v5;
	return a1;
}
