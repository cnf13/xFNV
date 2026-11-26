#include "SDK/FaceGen/FanControls.h"
#include "BSCore/NewOverloads.h"
#include "SDK/FaceGen/FanControls.h"
#include "SDK/FaceGen/FutDensity.h"
#include "SDK/FaceGen/MatrixCT.h"
#include "SDK/FaceGen/MatrixVT.h"
#include "SDK/FaceGen/TriFileIO.h"
#include "SDK/FaceGen/assert.hpp"
#include "SDK/FaceGen/binaryFile.h"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/ehdata.h"
#include "SDK/MSVC/exception.h"
#include "SDK/MSVC/iosfwd.h"
#include "SDK/MSVC/string.h"
#include <cmath>
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

float FanGenCtlsGrpC::getAttribute(
    FanRaceE race,
    FanAttribE att,
    FanTypeE type,
    const FR2MatrixVTC<float> (*coord)[2]
) {
    FR2MatrixVTC<float> kProduct;
    if (race >= FANRACE_SIZE) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 198);
    }
    if (att >= FANATT_SIZE) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 199);
    }
    if (type >= FANTYPE_SIZE) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 200);
    }
    if (initialized) {
        FR2MatrixVTC<float>* pProduct = attr[race][att][type].ctl.Mul(&kProduct, (*coord)[2 * type]);
        float fAttribute = *(*pProduct)[0].p;
        return fAttribute + attr[race][att][type].off;
    }

    return 0.0f;
}

void FanLinCtlS::operator delete(void* apMemory) {
    ourFree(apMemory);
}

FanFgResidualS::~FanFgResidualS() {}

void FanFgResidualS::operator delete(void* apMemory) {
    ourFree(apMemory);
}

FanRaceDistS::FanRaceDistS() {}

FanRaceDistS::~FanRaceDistS() {}

FanOffLinCtlS::FanOffLinCtlS() {}

FanOffLinCtlS::~FanOffLinCtlS() {}

FanPrtOffLinCtlS::FanPrtOffLinCtlS() {}

FanPrtOffLinCtlS::~FanPrtOffLinCtlS() {}

FanGenCtlsGrpC::FanGenCtlsGrpC() : initialized(false) {}

FanGenCtlsGrpC::~FanGenCtlsGrpC() {}

void FanGenCtlsGrpC::setAttributes(FanRaceE race, FanTypeE type, const float* val, FR2MatrixVTC<float> (*coord)[2]) {
    FR2MatrixVTC<float> kScaled;
    FR2MatrixVTC<float> kTransposed;
    if (race >= FANRACE_SIZE) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 266);
    }
    if (initialized) {
        float attBoundHi[2];
        FR2Vect2TC<float> delta;
        float attBoundLo[2];
        FR2Vect2TC<float> valDiff;
        uint32_t aa;
        attBoundLo[0] = 15.0f;
        attBoundLo[1] = -4.0f;
        attBoundHi[0] = 65.0f;
        attBoundHi[1] = 4.0f;
        valDiff.x1 = 0.0f;
        valDiff.x2 = 0.0f;
        for (aa = 0; aa < 2; ++aa) {
            float newVal = val[aa];
            if (attBoundLo[aa] > static_cast<double>(newVal)) {
                newVal = attBoundLo[aa];
            }
            if (attBoundHi[aa] < static_cast<double>(newVal)) {
                newVal = attBoundHi[aa];
            }
            // The source counts the attributes with an unsigned index and passes it as the attribute.
            double dAttribute = getAttribute(race, static_cast<FanAttribE>(aa), type, coord);
            *(&valDiff.x1 + aa) = newVal - dAttribute;
        }
        // The 2 x 2 inverse goes to the matrix-vector product as its four floats, row by row.
        MultiplyMatrixVector(&delta.x1, &invAttCov[race][type].data[0][0], &valDiff.x1);
        for (aa = 0; aa < 2; ++aa) {
            FR2MatrixVTC<float>* pTransposed = attr[race][aa][type].ctl.transpose(&kTransposed);
            FR2MatrixVTC<float>* pScaled = pTransposed->MulScalar(&kScaled, *(&delta.x1 + aa));
            (*coord)[2 * type] += *pScaled;
        }
    }
}

double
FanGenCtlsGrpC::getCaricature(FanRaceE race, FanTypeE type, FanSymmE symm, const FR2MatrixVTC<float> (*coord)[2]) {
    FR2MatrixVTC<float> kScaled;
    FR2MatrixVTC<float> kTransposed;
    FR2MatrixVTC<float>::RowT kRow;
    FR2MatrixVTC<float> kProduct;
    FR2MatrixVTC<float> kNormalized;
    FR2MatrixVTC<float> kDelta;
    if (race >= 5) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 308);
    }
    if (type >= 2) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 309);
    }
    if (symm >= 2) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 310);
    }
    if (!initialized) {
        return 0.0;
    }
    if (symm == 1) {
        float fLengthSqr = coord[type][symm].lengthSqr();
        float fLength = std::sqrt(static_cast<double>(fLengthSqr));
        float fRootRows = std::sqrt(static_cast<double>(coord[type][symm].nrows));
        return fLength / fRootRows;
    }

    if (symm) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 344);
        return 0.0;
    }

    coord[type][symm].Sub(&kDelta, distrib[race].mean[type]);
    for (uint32_t i = 0; i < 2; ++i) {
        FR2MatrixVTC<float>* pProduct = basis[4 * race + 2 * type + i].Mul(&kProduct, kDelta);
        float fWeight = *(::new (&kRow) FR2MatrixVTC<float>::RowT((*pProduct)[0u]))->p;
        FR2MatrixVTC<float>* pTransposed = basis[4 * race + 2 * type + i].transpose(&kTransposed);
        FR2MatrixVTC<float>* pScaled = pTransposed->MulScalar(&kScaled, fWeight);
        kDelta -= *pScaled;
    }
    distrib[race].invEigBasis[type].Mul(&kNormalized, kDelta);
    float fLengthSqr = kNormalized.lengthSqr();
    float fLength = std::sqrt(static_cast<double>(fLengthSqr));
    float fDegrees = static_cast<double>(kNormalized.nrows) - 2.0;
    float fRootDegrees = std::sqrt(static_cast<double>(fDegrees));
    return fLength / fRootDegrees;
}

void FanGenCtlsGrpC::setCaricature(
    FanRaceE race,
    FanTypeE type,
    FanSymmE symm,
    float val,
    FR2MatrixVTC<float> (*coord)[2]
) {
    FR2MatrixVTC<float> kNewCoord;
    FR2MatrixVTC<float> kSum;
    FR2MatrixVTC<float> kScaled;
    FR2MatrixVTC<float> kTransposed;
    FR2MatrixVTC<float>::RowT kRow;
    FR2MatrixVTC<float> kProduct;
    FR2MatrixVTC<float> m;
    FR2MatrixVTC<float> kProjection;
    if (race >= 5) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 370);
    }
    if (type >= 2) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 371);
    }
    if (symm >= 2) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 372);
    }
    if (initialized) {
        if (val < 0.009999999776482582) {
            val = 0.01f;
        }
        if (symm == 1) {
            float fLengthSqr = coord[type][symm].lengthSqr();
            float fLength = std::sqrt(static_cast<double>(fLengthSqr));
            float fRootRows = std::sqrt(static_cast<double>(coord[type][symm].nrows));
            float fCaricature = fLength / fRootRows;
            if (fCaricature > 0.0) {
                float fScale = val / fCaricature;
                coord[type][symm] *= fScale;
            }
        }
        else if (!symm) {
            coord[type][symm].Sub(&m, distrib[race].mean[type]);
            uint32_t ncols = coord[type][symm].ncols;
            uint32_t nrows = coord[type][symm].nrows;
            kProjection.data.~vector<float, std::allocator<float>>();
            ::new (&kProjection.data) std::vector<float, std::allocator<float>>;
            kProjection.nrows = nrows;
            kProjection.ncols = ncols;
            kProjection.data.resize(nrows * ncols, 0.0f);
            kProjection.setZero();
            for (uint32_t i = 0; i < 2; ++i) {
                FR2MatrixVTC<float>* pProduct = basis[4 * race + 2 * type + i].Mul(&kProduct, m);
                float fWeight = *(::new (&kRow) FR2MatrixVTC<float>::RowT((*pProduct)[0u]))->p;
                FR2MatrixVTC<float>* pTransposed = basis[4 * race + 2 * type + i].transpose(&kTransposed);
                FR2MatrixVTC<float>* pScaled = pTransposed->MulScalar(&kScaled, fWeight);
                kProjection += *pScaled;
            }
            m -= kProjection;
            float fCaricature = getCaricature(race, type, symm, coord);
            if (fCaricature > 0.0) {
                float fScale = val / fCaricature;
                m *= fScale;
                FR2MatrixVTC<float>* pSum = m.Add(&kSum, kProjection);
                FR2MatrixVTC<float>* pNewCoord = pSum->Add(&kNewCoord, distrib[race].mean[type]);
                coord[type][symm] = *pNewCoord;
            }
        }
    }
}

double FanGenCtlsGrpC::getRace(int32_t race1, int32_t race2, const FR2MatrixVTC<float> (*coord)[2]) {
    FR2MatrixVTC<float>::RowT kRow;
    FR2MatrixVTC<float> kProduct;
    if (race1 >= 5) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 436);
    }
    if (race2 >= 5) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 437);
    }
    if (race1 == race2) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 438);
    }
    if (!initialized) {
        return 0.0;
    }
    float fSum = 0.0f;
    for (uint32_t i = 0; i < 2; ++i) {
        FR2MatrixVTC<float>* pProduct = race[race1][race2].ctl[i].Mul(&kProduct, coord[i][0]);
        fSum = *(::new (&kRow) FR2MatrixVTC<float>::RowT((*pProduct)[0u]))->p + fSum;
    }
    return fSum + race[race1][race2].off;
}

void FanGenCtlsGrpC::setRace(int32_t race1, int32_t race2, float val, FR2MatrixVTC<float> (*coord)[2]) {
    FR2MatrixVTC<float> kScaled;
    FR2MatrixVTC<float> kTransposed;
    if (race1 >= 5) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 468);
    }
    if (race2 >= 5) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 469);
    }
    if (race1 == race2) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 470);
    }
    if (initialized) {
        float s = (val - getRace(race1, race2, coord)) * race[race1][race2].scale;
        for (uint32_t i = 0; i < 2; ++i) {
            FR2MatrixVTC<float>* pTransposed = race[race1][race2].ctl[i].transpose(&kTransposed);
            FR2MatrixVTC<float>* pScaled = pTransposed->MulScalar(&kScaled, s);
            coord[i][0] += *pScaled;
        }
    }
}

void FanGenCtlsGrpC::random(int32_t aiRace, FR2MatrixVTC<float>* apCoord) {
    FR2MatrixVTC<float> kLowerCoord;
    FR2MatrixVTC<float> kLower;
    FR2MatrixVTC<float> kUpperCoord;
    FR2MatrixVTC<float> kUpper;
    FR2MatrixVTC<float> kProduct;
    FR2MatrixVTC<float>::RowT kRow;
    FR2MatrixVTC<float> kConcat;
    if (aiRace >= 5) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 498);
    }
    if (initialized) {
        for (uint32_t j = 0; j < 2; ++j) {
            for (uint32_t k = 0; k < 2; ++k) {
                for (uint32_t i = 0; i < apCoord[2 * j + k].nrows; ++i) {
                    kRow = apCoord[2 * j + k][i];
                    float* p = kRow.p;
                    *p = RngStdNormal();
                }
            }
        }
        ConcatRows(&kConcat, apCoord, apCoord + 2);
        FR2MatrixVTC<float>* pProduct = distrib[aiRace].eigBasis.Mul(&kProduct, kConcat);
        kConcat = *pProduct;
        uint32_t nr = apCoord->nrows;
        uint32_t nrows = apCoord[2].nrows;
        FR2MatrixVTC<float>* pUpper = kConcat.subMatrix(&kUpper, 0, 0, nr, 1u);
        FR2MatrixVTC<float>* pUpperCoord = pUpper->Add(&kUpperCoord, *distrib[aiRace].mean);
        *apCoord = *pUpperCoord;
        FR2MatrixVTC<float>* pLower = kConcat.subMatrix(&kLower, nr, 0, nrows, 1u);
        FR2MatrixVTC<float>* pLowerCoord = pLower->Add(&kLowerCoord, distrib[aiRace].mean[1]);
        *(apCoord + 2) = *pLowerCoord;
    }
}

void FanGenCtlsGrpC::geneticRandom(
    float afGeneticVariation,
    FR2MatrixVTC<float> (*apSourceCoord)[2][2],
    FR2MatrixVTC<float> (*apOutCoord)[2][2]
) {
    FR2MatrixVTC<float>::RowT kRow;
    for (int32_t j = 0; j < 2; ++j) {
        for (int32_t k = 0; k < 2; ++k) {
            uint32_t ncols = (*apSourceCoord)[j][k].ncols;
            uint32_t nrows = (*apSourceCoord)[j][k].nrows;
            FR2MatrixVTC<float>* pOutMatrix = &(*apOutCoord)[j][k];
            pOutMatrix->nrows = nrows;
            pOutMatrix->ncols = ncols;
            pOutMatrix->data.resize(ncols * nrows, 0.0f);
            for (uint32_t i = 0; i < (*apSourceCoord)[j][k].nrows; ++i) {
                for (uint32_t m = 0; m < (*apSourceCoord)[j][k].ncols; ++m) {
                    if (k) {
                        (*apOutCoord)[j][k][i].p[m] = 0.0f;
                    }
                    else {
                        double dNoise = RngStdNormal() * afGeneticVariation * 1.5;
                        float* pSource = &(*apSourceCoord)[j][k][i].p[m];
                        kRow = (*apOutCoord)[j][k][i];
                        kRow.p[m] = *pSource + dNoise;
                    }
                }
            }
        }
    }
}

void FanGenCtlsGrpC::reflect(FanRaceE race, FR2MatrixVTC<float> (*coord)[2]) {
    FR2MatrixVTC<float> kDiff;
    FR2MatrixVTC<float> kScaled;
    FR2MatrixVTC<float> kNegated;
    if (race >= 5) {
        fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\FanControls.cpp", 629);
    }
    if (initialized) {
        for (uint32_t i = 0; i < 2; ++i) {
            FR2MatrixVTC<float>* pNegated = coord[i][1].MulScalar(&kNegated, -1.0f);
            coord[i][1] = *pNegated;
            FR2MatrixVTC<float>* pScaled = distrib[race].mean[i].MulScalar(&kScaled, 2.0f);
            FR2MatrixVTC<float>* pDiff = pScaled->Sub(&kDiff, coord[i][0]);
            coord[i][0] = *pDiff;
        }
    }
}

void FanGenCtlsGrpC::tween(
    FanTypeE type,
    FanSymmE symm,
    float ratio,
    const FR2MatrixVTC<float> (*parent1)[2],
    const FR2MatrixVTC<float> (*parent2)[2],
    FR2MatrixVTC<float> (*child)[2]
) {
    FR2MatrixVTC<float> kTween;
    FR2MatrixVTC<float> kWeighted2;
    FR2MatrixVTC<float> kWeighted1;
    float fCaricature;
    float fCaricature1 = getCaricature(FANRACE_ALL, type, symm, parent1);
    float fCaricature2 = getCaricature(FANRACE_ALL, type, symm, parent2);
    if (ratio < 0.0) {
        fCaricature = fCaricature2;
    }
    else if (ratio > 1.0) {
        fCaricature = fCaricature1;
    }
    else {
        fCaricature = fCaricature1 * ratio + (1.0 - ratio) * fCaricature2;
    }
    float s = 1.0 - ratio;
    FR2MatrixVTC<float>* pWeighted2 = parent2[type][symm].MulScalar(&kWeighted2, s);
    FR2MatrixVTC<float>* pWeighted1 = parent1[type][symm].MulScalar(&kWeighted1, ratio);
    FR2MatrixVTC<float>* pTween = pWeighted1->Add(&kTween, *pWeighted2);
    child[type][symm] = *pTween;
    setCaricature(FANRACE_ALL, type, symm, fCaricature, child);
}

FanFgFileDataS::FanFgFileDataS() {}

FanFgFileDataS::~FanFgFileDataS() {}

FanLinCtlGrpS::FanLinCtlGrpS() {}

FanLinCtlGrpS::~FanLinCtlGrpS() {}

// The end of the copy is computed first, from the byte span divided as signed: a plain pointer
// difference would let clang assume an exact multiple.
FanLinCtlS* CopyOpt(
    FanLinCtlS* _First,
    FanLinCtlS* _Last,
    FanLinCtlS* _Dest,
    [[maybe_unused]] uint8_t _IterCat,
    [[maybe_unused]] uint8_t _PtrCat,
    [[maybe_unused]] uint8_t _Checked
) {
    ptrdiff_t iBytes = static_cast<char*>(static_cast<void*>(_Last)) - static_cast<char*>(static_cast<void*>(_First));
    FanLinCtlS* pEnd = _Dest + iBytes / static_cast<ptrdiff_t>(sizeof(FanLinCtlS));
    for (; _First != _Last; ++_Dest, ++_First) {
        _Dest->ctl = _First->ctl;
        _Dest->label.assign(_First->label, 0, std::string::npos);
    }
    return pEnd;
}
