#pragma once

#include "SDK/FaceGen/MatrixCT.h"
#include "SDK/FaceGen/MatrixVT.h"

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

class FutBinaryFileC;

// FaceGen's two coordinate spaces: geometry and texture.
enum FanTypeE : int32_t {
    FANTYPE_GEO = 0,
    FANTYPE_TEX = 1,
    FANTYPE_SIZE = 2,
};

// FaceGen's attributes: the first index of a control group's attr array (getAttribute).
enum FanAttribE : int32_t {
    FANATT_AGE = 0,
    FANATT_GENDER = 1,
    FANATT_SIZE = 2,
};

// FaceGen's symmetry: the second index of a control group's linear controls (see GetFaceCoordValue).
enum FanSymmE : int32_t {
    FANSYMM_SYMM = 0,
    FANSYMM_ASYM = 1,
    FANSYMM_SIZE = 2,
};

// FaceGen's race groups.
enum FanRaceE : int32_t {
    FANRACE_ALL = 0,
    FANRACE_AFRO = 1,
    FANRACE_ASIA = 2,
    FANRACE_EIND = 3,
    FANRACE_EURO = 4,
    FANRACE_SIZE = 5,
};

// FaceGen's linear control: a control matrix and its label.
struct FanLinCtlS {
    FanLinCtlS();
    // The copy constructor: the control matrix's copy, then the label's.
    FanLinCtlS(const FanLinCtlS& _Right);
    ~FanLinCtlS();

    // Deleting frees through ourFree (BSCore/MemoryManager.h).
    static void operator delete(void* apMemory);

    FR2MatrixVTC<float> ctl;
    std::string label;
};

// FaceGen's face residual: an offset, a scale and four control matrices.
struct FanFgResidualS {
    ~FanFgResidualS();

    // Deleting frees through ourFree (BSCore/MemoryManager.h).
    static void operator delete(void* apMemory);

    float resOffset;
    float resScale;
    FR2MatrixVTC<float> resMode[2][2];
};

// FanLinCtlGrpS::grp: one list of linear controls.
using FanLinCtlSVector = std::vector<FanLinCtlS, std::allocator<FanLinCtlS>>;

// The FaceGen manager's group of linear controls: a 2 by 2 grid of control lists.
class FanLinCtlGrpS {
public:
    FanLinCtlGrpS();
    ~FanLinCtlGrpS();

    bool readFromFile(FutBinaryFileC& file, uint32_t (*dims)[2]);

    std::vector<FanLinCtlS, std::allocator<FanLinCtlS>> grp[2][2];
};

// FaceGen's distribution of one race: the means and the eigen bases, 0xA0 bytes (the constructor's element stride).
struct FanRaceDistS {
    FanRaceDistS();
    ~FanRaceDistS();

    FR2MatrixVTC<float> mean[2];
    FR2MatrixVTC<float> eigBasis;
    FR2MatrixVTC<float> invEigBasis[2];
};

// FaceGen's offset linear control on two matrices (geometry and texture): 0x48 bytes. setRace scales by scale.
struct FanOffLinCtlS {
    FanOffLinCtlS();
    ~FanOffLinCtlS();

    FR2MatrixVTC<float> ctl[2];
    float scale;
    float off;
};

// FaceGen's offset linear control on one matrix: 0x28 bytes. getAttribute adds off to the control's product.
struct FanPrtOffLinCtlS {
    FanPrtOffLinCtlS();
    ~FanPrtOffLinCtlS();

    FR2MatrixVTC<float> ctl;
    float scale;
    float off;
};

// The FaceGen SDK's control groups (SDK\FaceGen\FanControls.cpp), 0x106C bytes in the manager. The layout is the
// constructor's: 5 race distributions, 20 attribute controls indexed [race][att][type] by getAttribute,
// 25 race-to-race controls indexed [race][race] by the race functions, 20 basis matrices and the 5 x 2 inverse
// attribute matrices setAttribute writes.
class FanGenCtlsGrpC {
public:
    FanGenCtlsGrpC();
    ~FanGenCtlsGrpC();

    // Attribute att of type type for race, measured on coord.
    float getAttribute(FanRaceE race, FanAttribE att, FanTypeE type, const FR2MatrixVTC<float> (*coord)[2]);
    // Sets the type attributes of coord for race from val.
    void setAttributes(FanRaceE race, FanTypeE type, const float* val, FR2MatrixVTC<float> (*coord)[2]);
    bool readFromFile(FutBinaryFileC& file, uint32_t (*dims)[2]);
    // The caricature of coord's type and symmetry against race's distribution. The result is kept as a double. The
    // enums' int32_t base gives signed compares.
    double getCaricature(FanRaceE race, FanTypeE type, FanSymmE symm, const FR2MatrixVTC<float> (*coord)[2]);
    // Scales coord's type and symmetry to caricature val against race's distribution.
    void setCaricature(FanRaceE race, FanTypeE type, FanSymmE symm, float val, FR2MatrixVTC<float> (*coord)[2]);
    double getRace(int32_t race1, int32_t race2, const FR2MatrixVTC<float> (*coord)[2]);
    void setRace(int32_t race1, int32_t race2, float val, FR2MatrixVTC<float> (*coord)[2]);
    void random(int32_t aiRace, FR2MatrixVTC<float>* apCoord);
    // Sizes each of outCoord's 2 x 2 matrices as currCoord's and fills it with currCoord's values plus Gaussian noise
    // scaled by randomness. Both coordinates are indexed as FR2MatrixVTC<float>[2][2], reading currCoord and writing
    // outCoord.
    void geneticRandom(float randomness, const FR2MatrixVTC<float> (*currCoord)[2], FR2MatrixVTC<float> (*outCoord)[2]);
    void geneticRandom(
        float afGeneticVariation,
        FR2MatrixVTC<float> (*apSourceCoord)[2][2],
        FR2MatrixVTC<float> (*apOutCoord)[2][2]
    );
    // Reflects coord about race's mean: the symmetric part of each type becomes twice the mean less itself, the
    // asymmetric part its negation.
    void reflect(FanRaceE race, FR2MatrixVTC<float> (*coord)[2]);
    // Blends parent1 and parent2's type and symmetry by ratio into child, then sets child's caricature to the parents'
    // caricatures blended the same way, ratio clamped to [0, 1].
    void tween(
        FanTypeE type,
        FanSymmE symm,
        float ratio,
        const FR2MatrixVTC<float> (*parent1)[2],
        const FR2MatrixVTC<float> (*parent2)[2],
        FR2MatrixVTC<float> (*child)[2]
    );
    void setAttribute(FanRaceE race, FanAttribE att, FanTypeE type, const float val, FR2MatrixVTC<float> (*coord)[2]);

    bool initialized;
    FanRaceDistS distrib[5];
    FanPrtOffLinCtlS attr[5][2][2];
    FanOffLinCtlS race[5][5];
    FR2MatrixVTC<float> basis[20];
    // The 2 x 2 inverse of each attribute matrix, indexed [race][type]: setAttribute writes the 16-byte
    // invAttCov[race][type] at this + 0xFCC + race * 0x20 + type * 0x10 and readFromFile
    // clears and fills it.
    FR2MatrixCTC<float, 2, 2> invAttCov[5][2];
};

// The FaceGen SDK's per-face data.
struct FanFgFileDataS {
    FanFgFileDataS();
    ~FanFgFileDataS();

    uint32_t egmKey;
    uint32_t egtKey;
    // The face coordinates, geometry first, then texture (CopyFaceGenCoord's abGeometryOnly copies coord[0]).
    FR2MatrixVTC<float> coord[2][2];
    std::vector<FanFgResidualS, std::allocator<FanFgResidualS>> residualCtrls;
    // The detail texture's JPEG bytes.
    std::vector<uint8_t, std::allocator<uint8_t>> detailTexJpeg;
};

// Assigns the controls [_First, _Last) to the ones from _Dest, each control's matrix then its label, and returns the
// end of the copy. None of the three tag bytes are read.
FanLinCtlS*
CopyOpt(FanLinCtlS* _First, FanLinCtlS* _Last, FanLinCtlS* _Dest, uint8_t _IterCat, uint8_t _PtrCat, uint8_t _Checked);

float* Invert2x2(float* a1, float* a2);
