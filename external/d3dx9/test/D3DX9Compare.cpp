// Compares the d3dx9 stand-in (external/d3dx9) with a real d3dx9_38.dll under Wine, function by function, on the
// game's own data where the function takes data:
//   D3DX9Compare <d3dx9_38.dll path> <scratch dir> [-stride N] [-limit N] <archive.bsa | package.sdp>...
// - math: the 12 functions on random inputs, each output's bits (exact, within 1 ulp, worse);
// - textures: every DDS of the given BSA archives (every Nth with -stride): image info, the created texture's
//   levels, formats and locked bytes; levels the file has must be equal, generated levels are compared by value
//   (exact for packed formats, decoded error for DXT); plus made-up DDS files for the formats and the volume maps
//   the archives lack;
// - D3DXLoadSurfaceFromSurface on DXT -> A8R8G8B8 (bytes) and A8R8G8B8 -> DXT1/3/5 (decoded error against the
//   source, beside the DLL's own error);
// - D3DXSaveTextureToFileA: the stand-in's PNG read back by the DLL, against the texture saved;
// - D3DXGetShaderConstantTable on every shader of the given shader packages (.sdp): the table's description, each
//   constant's description, the buffer bytes;
// - the shader profiles on the device and on made-up caps; D3DXCreateBuffer; the compile failure.
// It prints a summary per part and exits 0 when every exact comparison held.

#include "D3DX9Internal.h"

#include <cmath>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <map>
#include <string>
#include <utility>

#include "zlib.h"

extern "C" void* TestCreateDevice();
extern "C" void* TestLoadLibrary(const char* apPath);
extern "C" void* TestGetProc(void* apModule, const char* apName);

namespace {

// The functions under test, from one implementation.
struct Lib {
	const char* name;
	D3DXMatrixMultiply_t MatrixMultiply;
	D3DXMatrixMultiplyTranspose_t MatrixMultiplyTranspose;
	D3DXMatrixTranspose_t MatrixTranspose;
	D3DXMatrixInverse_t MatrixInverse;
	D3DXMatrixRotationYawPitchRoll_t MatrixRotationYawPitchRoll;
	D3DXPlaneNormalize_t PlaneNormalize;
	D3DXPlaneTransform_t PlaneTransform;
	D3DXVec3Normalize_t Vec3Normalize;
	D3DXVec3TransformCoord_t Vec3TransformCoord;
	D3DXVec3TransformNormal_t Vec3TransformNormal;
	D3DXVec4Transform_t Vec4Transform;
	D3DXGetImageInfoFromFileInMemory_t GetImageInfo;
	D3DXCreateTextureFromFileInMemory_t CreateTexture;
	D3DXCreateCubeTextureFromFileInMemory_t CreateCubeTexture;
	D3DXCreateVolumeTextureFromFileInMemory_t CreateVolumeTexture;
	D3DXLoadSurfaceFromSurface_t LoadSurfaceFromSurface;
	D3DXSaveTextureToFileA_t SaveTextureToFileA;
	D3DXGetShaderConstantTable_t GetShaderConstantTable;
	D3DXGetVertexShaderProfile_t GetVertexShaderProfile;
	D3DXGetPixelShaderProfile_t GetPixelShaderProfile;
	D3DXCreateBuffer_t CreateBuffer;
};

Lib StandIn()
{
	return {"stand-in", D3DXMatrixMultiply, D3DXMatrixMultiplyTranspose, D3DXMatrixTranspose, D3DXMatrixInverse,
		D3DXMatrixRotationYawPitchRoll, D3DXPlaneNormalize, D3DXPlaneTransform, D3DXVec3Normalize,
		D3DXVec3TransformCoord, D3DXVec3TransformNormal, D3DXVec4Transform, D3DXGetImageInfoFromFileInMemory,
		D3DXCreateTextureFromFileInMemory, D3DXCreateCubeTextureFromFileInMemory,
		D3DXCreateVolumeTextureFromFileInMemory, D3DXLoadSurfaceFromSurface, D3DXSaveTextureToFileA,
		D3DXGetShaderConstantTable, D3DXGetVertexShaderProfile, D3DXGetPixelShaderProfile, D3DXCreateBuffer};
}

template <class T>
void Bind(void* apModule, const char* apName, T& arOut)
{
	arOut = reinterpret_cast<T>(TestGetProc(apModule, apName));
	if (!arOut) {
		std::printf("missing export %s\n", apName);
		std::exit(2);
	}
}

Lib FromDll(const char* apPath)
{
	void* pModule = TestLoadLibrary(apPath);
	if (!pModule) {
		std::printf("cannot load %s\n", apPath);
		std::exit(2);
	}
	Lib kLib{};
	kLib.name = "dll";
	Bind(pModule, "D3DXMatrixMultiply", kLib.MatrixMultiply);
	Bind(pModule, "D3DXMatrixMultiplyTranspose", kLib.MatrixMultiplyTranspose);
	Bind(pModule, "D3DXMatrixTranspose", kLib.MatrixTranspose);
	Bind(pModule, "D3DXMatrixInverse", kLib.MatrixInverse);
	Bind(pModule, "D3DXMatrixRotationYawPitchRoll", kLib.MatrixRotationYawPitchRoll);
	Bind(pModule, "D3DXPlaneNormalize", kLib.PlaneNormalize);
	Bind(pModule, "D3DXPlaneTransform", kLib.PlaneTransform);
	Bind(pModule, "D3DXVec3Normalize", kLib.Vec3Normalize);
	Bind(pModule, "D3DXVec3TransformCoord", kLib.Vec3TransformCoord);
	Bind(pModule, "D3DXVec3TransformNormal", kLib.Vec3TransformNormal);
	Bind(pModule, "D3DXVec4Transform", kLib.Vec4Transform);
	Bind(pModule, "D3DXGetImageInfoFromFileInMemory", kLib.GetImageInfo);
	Bind(pModule, "D3DXCreateTextureFromFileInMemory", kLib.CreateTexture);
	Bind(pModule, "D3DXCreateCubeTextureFromFileInMemory", kLib.CreateCubeTexture);
	Bind(pModule, "D3DXCreateVolumeTextureFromFileInMemory", kLib.CreateVolumeTexture);
	Bind(pModule, "D3DXLoadSurfaceFromSurface", kLib.LoadSurfaceFromSurface);
	Bind(pModule, "D3DXSaveTextureToFileA", kLib.SaveTextureToFileA);
	Bind(pModule, "D3DXGetShaderConstantTable", kLib.GetShaderConstantTable);
	Bind(pModule, "D3DXGetVertexShaderProfile", kLib.GetVertexShaderProfile);
	Bind(pModule, "D3DXGetPixelShaderProfile", kLib.GetPixelShaderProfile);
	Bind(pModule, "D3DXCreateBuffer", kLib.CreateBuffer);
	return kLib;
}

int32_t giFailures = 0;

void Fail(const char* apFormat, ...)
{
	static int32_t siPrinted = 0;
	++giFailures;
	if (++siPrinted > 200) {
		return;
	}
	va_list kArgs;
	va_start(kArgs, apFormat);
	std::fputs("  FAIL ", stdout);
	std::vprintf(apFormat, kArgs);
	std::fputc('\n', stdout);
	va_end(kArgs);
}

// --- math -----------------------------------------------------------------------------------------------------------

uint64_t gulRandom = 0x9E3779B97F4A7C15ull;

uint32_t Next()
{
	gulRandom ^= gulRandom << 13;
	gulRandom ^= gulRandom >> 7;
	gulRandom ^= gulRandom << 17;
	return static_cast<uint32_t>(gulRandom >> 16);
}

float Uniform(float afLow, float afHigh)
{
	return afLow + (afHigh - afLow) * static_cast<float>(Next() & 0xFFFFFF) / 16777216.0f;
}

// Mostly the game's ranges, some large and small magnitudes.
float RandomFloat()
{
	const uint32_t uiKind = Next() % 10;
	if (uiKind < 7) {
		return Uniform(-4.0f, 4.0f);
	}
	if (uiKind < 9) {
		return Uniform(-5000.0f, 5000.0f);
	}
	return std::ldexp(Uniform(-1.0f, 1.0f), static_cast<int32_t>(Next() % 41) - 20);
}

int64_t Ordered(float afValue)
{
	int32_t iBits;
	std::memcpy(&iBits, &afValue, 4);
	return iBits < 0 ? -static_cast<int64_t>(iBits & 0x7FFFFFFF) : iBits;
}

struct UlpStats {
	uint64_t values = 0;
	uint64_t exact = 0;
	uint64_t oneUlp = 0;
	uint64_t worse = 0;
	int64_t maxUlp = 0;
	uint64_t nullMismatch = 0;
	// The largest difference over the largest magnitude of the same output (a matrix, a vector): the error
	// relative to the result's scale, which ulps overstate for elements near zero.
	double maxScaled = 0;

	void Add(const float* apA, const float* apB, uint32_t auiCount)
	{
		float fScale = 0.0f;
		for (uint32_t i = 0; i < auiCount; ++i) {
			fScale = std::fabs(apB[i]) > fScale ? std::fabs(apB[i]) : fScale;
		}
		for (uint32_t i = 0; i < auiCount; ++i) {
			++values;
			uint32_t uiA, uiB;
			std::memcpy(&uiA, &apA[i], 4);
			std::memcpy(&uiB, &apB[i], 4);
			if (uiA == uiB || (std::isnan(apA[i]) && std::isnan(apB[i]))) {
				++exact;
				continue;
			}
			const int64_t lUlp = std::llabs(Ordered(apA[i]) - Ordered(apB[i]));
			if (lUlp <= 1) {
				++oneUlp;
			} else {
				++worse;
			}
			maxUlp = lUlp > maxUlp ? lUlp : maxUlp;
			if (fScale > 0.0f && std::isfinite(fScale)) {
				const double fScaled = std::fabs(static_cast<double>(apA[i]) - apB[i]) / fScale;
				maxScaled = fScaled > maxScaled ? fScaled : maxScaled;
			}
		}
	}

	void Print(const char* apName, const char* apWho) const
	{
		std::printf("  %-30s %-16s %7llu values: exact %llu, 1 ulp %llu, worse %llu (max %lld ulp, max %.2g of the "
			"output's largest)%s\n", apName, apWho, static_cast<unsigned long long>(values),
			static_cast<unsigned long long>(exact), static_cast<unsigned long long>(oneUlp),
			static_cast<unsigned long long>(worse), static_cast<long long>(maxUlp), maxScaled,
			nullMismatch ? " NULL-RESULT MISMATCH" : "");
	}
};

D3DXMATRIX RandomMatrix(const Lib& arLib, bool* apAffine = nullptr)
{
	D3DXMATRIX kMatrix;
	const bool bGeneral = Next() & 1;
	if (apAffine) {
		*apAffine = !bGeneral;
	}
	if (bGeneral) {
		float* pValues = &kMatrix._11;
		for (int32_t i = 0; i < 16; ++i) {
			pValues[i] = RandomFloat();
		}
		return kMatrix;
	}
	// An affine transform, as the game's world and view matrices.
	arLib.MatrixRotationYawPitchRoll(&kMatrix, Uniform(-7, 7), Uniform(-7, 7), Uniform(-7, 7));
	const float fScale = Uniform(0.1f, 10.0f);
	float* pValues = &kMatrix._11;
	for (int32_t i = 0; i < 12; ++i) {
		pValues[i] *= fScale;
	}
	kMatrix._41 = RandomFloat();
	kMatrix._42 = RandomFloat();
	kMatrix._43 = RandomFloat();
	return kMatrix;
}

// The inverse in double precision (Gauss-Jordan, partial pivoting) of the float matrix: the reference both float
// inverses are measured against. False when a pivot is 0.
bool DoubleInverse(const D3DXMATRIX& arM, double (&arOut)[16])
{
	double afA[4][8];
	for (int32_t r = 0; r < 4; ++r) {
		for (int32_t c = 0; c < 4; ++c) {
			afA[r][c] = (&arM._11)[r * 4 + c];
			afA[r][c + 4] = r == c ? 1.0 : 0.0;
		}
	}
	for (int32_t c = 0; c < 4; ++c) {
		int32_t iPivot = c;
		for (int32_t r = c + 1; r < 4; ++r) {
			iPivot = std::fabs(afA[r][c]) > std::fabs(afA[iPivot][c]) ? r : iPivot;
		}
		if (afA[iPivot][c] == 0.0) {
			return false;
		}
		for (int32_t k = 0; k < 8; ++k) {
			std::swap(afA[c][k], afA[iPivot][k]);
		}
		const double fInv = 1.0 / afA[c][c];
		for (int32_t k = 0; k < 8; ++k) {
			afA[c][k] *= fInv;
		}
		for (int32_t r = 0; r < 4; ++r) {
			if (r != c) {
				const double f = afA[r][c];
				for (int32_t k = 0; k < 8; ++k) {
					afA[r][k] -= f * afA[c][k];
				}
			}
		}
	}
	for (int32_t r = 0; r < 4; ++r) {
		for (int32_t c = 0; c < 4; ++c) {
			arOut[r * 4 + c] = afA[r][c + 4];
		}
	}
	return true;
}

// The error of a float inverse against the double one: the largest element difference over the largest element.
double InverseError(const D3DXMATRIX& arM, const double (&arExact)[16])
{
	double fScale = 0, fMax = 0;
	for (int32_t i = 0; i < 16; ++i) {
		fScale = std::fabs(arExact[i]) > fScale ? std::fabs(arExact[i]) : fScale;
	}
	for (int32_t i = 0; i < 16; ++i) {
		const double fDiff = std::fabs((&arM._11)[i] - arExact[i]);
		fMax = fDiff > fMax ? fDiff : fMax;
	}
	return fScale > 0 ? fMax / fScale : 0;
}

// Both float inverses against the double one, per matrix kind: which is closer, the largest and the mean error.
struct ExactStats {
	uint64_t cases = 0, aCloser = 0, bCloser = 0;
	double maxA = 0, maxB = 0, sumA = 0, sumB = 0;
};

void TestMath(const Lib& arA, const Lib& arB, const char* apWho, uint32_t auiCases)
{
	std::map<std::string, UlpStats> kStats;
	std::map<std::string, ExactStats> kExact;
	for (uint32_t uiCase = 0; uiCase < auiCases; ++uiCase) {
		bool bAffine = false;
		const D3DXMATRIX kM1 = RandomMatrix(arB, &bAffine);
		const char* pKind = bAffine ? " (affine)" : " (general)";
		const D3DXMATRIX kM2 = RandomMatrix(arB);
		D3DXMATRIX kOutA, kOutB;
		arA.MatrixMultiply(&kOutA, &kM1, &kM2);
		arB.MatrixMultiply(&kOutB, &kM1, &kM2);
		kStats["D3DXMatrixMultiply"].Add(&kOutA._11, &kOutB._11, 16);
		arA.MatrixMultiplyTranspose(&kOutA, &kM1, &kM2);
		arB.MatrixMultiplyTranspose(&kOutB, &kM1, &kM2);
		kStats["D3DXMatrixMultiplyTranspose"].Add(&kOutA._11, &kOutB._11, 16);
		arA.MatrixTranspose(&kOutA, &kM1);
		arB.MatrixTranspose(&kOutB, &kM1);
		kStats["D3DXMatrixTranspose"].Add(&kOutA._11, &kOutB._11, 16);
		float fDetA = 0, fDetB = 0;
		const bool bA = arA.MatrixInverse(&kOutA, &fDetA, &kM1) != nullptr;
		const bool bB = arB.MatrixInverse(&kOutB, &fDetB, &kM1) != nullptr;
		UlpStats& rInverse = kStats[std::string("D3DXMatrixInverse") + pKind];
		if (bA != bB) {
			++rInverse.nullMismatch;
		} else if (bA) {
			rInverse.Add(&kOutA._11, &kOutB._11, 16);
			double afExact[16];
			if (DoubleInverse(kM1, afExact)) {
				const double fA = InverseError(kOutA, afExact), fB = InverseError(kOutB, afExact);
				ExactStats& rExact = kExact[pKind];
				++rExact.cases;
				rExact.aCloser += fA < fB;
				rExact.bCloser += fB < fA;
				rExact.maxA = fA > rExact.maxA ? fA : rExact.maxA;
				rExact.maxB = fB > rExact.maxB ? fB : rExact.maxB;
				rExact.sumA += fA;
				rExact.sumB += fB;
			}
		}
		kStats[std::string("D3DXMatrixInverse det") + pKind].Add(&fDetA, &fDetB, 1);
		const float fYaw = Uniform(-7, 7), fPitch = Uniform(-7, 7), fRoll = Uniform(-7, 7);
		arA.MatrixRotationYawPitchRoll(&kOutA, fYaw, fPitch, fRoll);
		arB.MatrixRotationYawPitchRoll(&kOutB, fYaw, fPitch, fRoll);
		kStats["D3DXMatrixRotationYawPitchRoll"].Add(&kOutA._11, &kOutB._11, 16);
		// D3DXPLANE's constructors are the game's; the test builds planes as plain floats.
		float afPlane[4] = {RandomFloat(), RandomFloat(), RandomFloat(), RandomFloat()};
		if (uiCase % 97 == 0) {
			afPlane[0] = afPlane[1] = afPlane[2] = 0.0f;
		}
		float afOutA[4], afOutB[4];
		arA.PlaneNormalize(reinterpret_cast<D3DXPLANE*>(afOutA), reinterpret_cast<D3DXPLANE*>(afPlane));
		arB.PlaneNormalize(reinterpret_cast<D3DXPLANE*>(afOutB), reinterpret_cast<D3DXPLANE*>(afPlane));
		kStats["D3DXPlaneNormalize"].Add(afOutA, afOutB, 4);
		arA.PlaneTransform(reinterpret_cast<D3DXPLANE*>(afOutA), reinterpret_cast<D3DXPLANE*>(afPlane), &kM1);
		arB.PlaneTransform(reinterpret_cast<D3DXPLANE*>(afOutB), reinterpret_cast<D3DXPLANE*>(afPlane), &kM1);
		kStats["D3DXPlaneTransform"].Add(afOutA, afOutB, 4);
		D3DXVECTOR3 kV{RandomFloat(), RandomFloat(), RandomFloat()};
		if (uiCase % 89 == 0) {
			kV = {0.0f, 0.0f, 0.0f};
		}
		D3DXVECTOR3 kVA, kVB;
		arA.Vec3Normalize(&kVA, &kV);
		arB.Vec3Normalize(&kVB, &kV);
		kStats["D3DXVec3Normalize"].Add(&kVA.x, &kVB.x, 3);
		arA.Vec3TransformCoord(&kVA, &kV, &kM1);
		arB.Vec3TransformCoord(&kVB, &kV, &kM1);
		kStats["D3DXVec3TransformCoord"].Add(&kVA.x, &kVB.x, 3);
		arA.Vec3TransformNormal(&kVA, &kV, &kM1);
		arB.Vec3TransformNormal(&kVB, &kV, &kM1);
		kStats["D3DXVec3TransformNormal"].Add(&kVA.x, &kVB.x, 3);
		const D3DXVECTOR4 kV4{RandomFloat(), RandomFloat(), RandomFloat(), RandomFloat()};
		D3DXVECTOR4 kV4A, kV4B;
		arA.Vec4Transform(&kV4A, &kV4, &kM1);
		arB.Vec4Transform(&kV4B, &kV4, &kM1);
		kStats["D3DXVec4Transform"].Add(&kV4A.x, &kV4B.x, 4);
		// Aliased output, as the game calls some of them (pOut == pM1).
		D3DXMATRIX kAliasA = kM1, kAliasB = kM1;
		arA.MatrixMultiply(&kAliasA, &kAliasA, &kM2);
		arB.MatrixMultiply(&kAliasB, &kAliasB, &kM2);
		kStats["D3DXMatrixMultiply aliased"].Add(&kAliasA._11, &kAliasB._11, 16);
	}
	for (const auto& [rName, rStats] : kStats) {
		rStats.Print(rName.c_str(), apWho);
	}
	for (const auto& [rKind, rExact] : kExact) {
		std::printf("  D3DXMatrixInverse%-12s against a double inverse, %llu cases: %s closer %llu, dll closer %llu; "
			"error (of the output's largest) max %.2g / dll %.2g, mean %.2g / dll %.2g\n", rKind.c_str(),
			static_cast<unsigned long long>(rExact.cases), apWho, static_cast<unsigned long long>(rExact.aCloser),
			static_cast<unsigned long long>(rExact.bCloser), rExact.maxA, rExact.maxB,
			rExact.sumA / static_cast<double>(rExact.cases), rExact.sumB / static_cast<double>(rExact.cases));
	}
}

// --- archives -------------------------------------------------------------------------------------------------------

struct Archive {
	FILE* file = nullptr;
	uint32_t flags = 0;
	struct Entry {
		std::string name;
		uint32_t size;
		uint32_t offset;
	};
	std::vector<Entry> entries;

	bool Open(const char* apPath)
	{
		file = std::fopen(apPath, "rb");
		if (!file) {
			return false;
		}
		uint32_t auiHeader[9];
		if (std::fread(auiHeader, 4, 9, file) != 9 || auiHeader[0] != 0x415342 || auiHeader[1] != 104) {
			return false;
		}
		flags = auiHeader[3];
		const uint32_t uiFolders = auiHeader[4];
		const uint32_t uiFileNamesLength = auiHeader[7];
		std::fseek(file, static_cast<long>(auiHeader[2]), SEEK_SET);
		std::vector<uint32_t> kCounts(uiFolders);
		for (uint32_t i = 0; i < uiFolders; ++i) {
			uint32_t auiFolder[4];
			std::fread(auiFolder, 4, 4, file);
			kCounts[i] = auiFolder[2];
		}
		for (uint32_t i = 0; i < uiFolders; ++i) {
			const int32_t iLength = std::fgetc(file);
			std::string kFolder(static_cast<size_t>(iLength), '\0');
			std::fread(kFolder.data(), 1, static_cast<size_t>(iLength), file);
			kFolder.pop_back();
			for (uint32_t j = 0; j < kCounts[i]; ++j) {
				uint32_t auiRecord[4];
				std::fread(auiRecord, 4, 4, file);
				entries.push_back({kFolder + "\\", auiRecord[2], auiRecord[3]});
			}
		}
		std::string kNames(uiFileNamesLength, '\0');
		std::fread(kNames.data(), 1, uiFileNamesLength, file);
		size_t uiAt = 0;
		for (Entry& rEntry : entries) {
			const size_t uiEnd = kNames.find('\0', uiAt);
			rEntry.name += kNames.substr(uiAt, uiEnd - uiAt);
			uiAt = uiEnd + 1;
		}
		return true;
	}

	std::vector<uint8_t> Read(const Entry& arEntry)
	{
		const bool bCompressed = bool(flags & 4) != bool(arEntry.size & 0x40000000);
		const uint32_t uiSize = arEntry.size & 0x3FFFFFFF;
		std::vector<uint8_t> kData(uiSize);
		std::fseek(file, static_cast<long>(arEntry.offset), SEEK_SET);
		std::fread(kData.data(), 1, uiSize, file);
		size_t uiSkip = 0;
		if (flags & 0x100) {
			uiSkip = 1 + kData[0];
		}
		if (!bCompressed) {
			return std::vector<uint8_t>(kData.begin() + static_cast<ptrdiff_t>(uiSkip), kData.end());
		}
		uint32_t uiOriginal;
		std::memcpy(&uiOriginal, kData.data() + uiSkip, 4);
		std::vector<uint8_t> kOut(uiOriginal);
		uLongf ulOut = uiOriginal;
		uncompress(kOut.data(), &ulOut, kData.data() + uiSkip + 4, static_cast<uLong>(uiSize - uiSkip - 4));
		return kOut;
	}
};

// --- textures -------------------------------------------------------------------------------------------------------

IDirect3DDevice9* gpDevice = nullptr;

struct ClassStats {
	uint32_t files = 0;
	uint32_t infoEqual = 0;
	uint32_t createBoth = 0;
	uint32_t shapeEqual = 0;
	uint32_t fileLevels = 0;
	uint32_t fileLevelsEqual = 0;
	uint32_t generatedLevels = 0;
	uint32_t generatedEqual = 0;
	double generatedSquares = 0;
	double generatedTexels = 0;
	uint32_t generatedMax = 0;
};

std::map<std::string, ClassStats> gkTextureStats;

// One level of a texture as packed rows (copied out of the lock) and its format and size.
struct Level {
	uint32_t format;
	uint32_t width, height, depth;
	std::vector<uint8_t> bytes;
};

std::vector<Level> ReadLevels(IDirect3DBaseTexture9* apTexture)
{
	std::vector<Level> kLevels;
	const uint32_t uiType = apTexture->GetType();
	const uint32_t uiFaces = uiType == D3DRTYPE_CUBETEXTURE ? 6 : 1;
	for (uint32_t uiFace = 0; uiFace < uiFaces; ++uiFace) {
		for (uint32_t uiLevel = 0; uiLevel < apTexture->GetLevelCount(); ++uiLevel) {
			Level kLevel{};
			uint8_t* pBits = nullptr;
			uint32_t uiPitch = 0, uiSlice = 0;
			if (uiType == D3DRTYPE_VOLUMETEXTURE) {
				auto* pVolume = static_cast<IDirect3DVolumeTexture9*>(apTexture);
				D3DVOLUME_DESC kDesc;
				pVolume->GetLevelDesc(uiLevel, &kDesc);
				kLevel = {kDesc.Format, kDesc.Width, kDesc.Height, kDesc.Depth, {}};
				D3DLOCKED_BOX kBox;
				pVolume->LockBox(uiLevel, &kBox, nullptr, D3DLOCK_READONLY);
				pBits = static_cast<uint8_t*>(kBox.pBits);
				uiPitch = static_cast<uint32_t>(kBox.RowPitch);
				uiSlice = static_cast<uint32_t>(kBox.SlicePitch);
			} else {
				D3DSURFACE_DESC kDesc;
				D3DLOCKED_RECT kRect;
				if (uiType == D3DRTYPE_CUBETEXTURE) {
					static_cast<IDirect3DCubeTexture9*>(apTexture)->GetLevelDesc(uiLevel, &kDesc);
					static_cast<IDirect3DCubeTexture9*>(apTexture)->LockRect(uiFace, uiLevel, &kRect, nullptr,
						D3DLOCK_READONLY);
				} else {
					static_cast<IDirect3DTexture9*>(apTexture)->GetLevelDesc(uiLevel, &kDesc);
					static_cast<IDirect3DTexture9*>(apTexture)->LockRect(uiLevel, &kRect, nullptr, D3DLOCK_READONLY);
				}
				kLevel = {kDesc.Format, kDesc.Width, kDesc.Height, 1, {}};
				pBits = static_cast<uint8_t*>(kRect.pBits);
				uiPitch = static_cast<uint32_t>(kRect.Pitch);
			}
			const d3dx::FormatInfo* pInfo = d3dx::FindFormat(kLevel.format);
			if (pInfo && pBits) {
				const uint32_t uiRowBytes = d3dx::RowBytes(*pInfo, kLevel.width);
				const uint32_t uiRows = d3dx::RowCount(*pInfo, kLevel.height);
				for (uint32_t z = 0; z < kLevel.depth; ++z) {
					for (uint32_t y = 0; y < uiRows; ++y) {
						const uint8_t* pRow = pBits + z * uiSlice + y * uiPitch;
						kLevel.bytes.insert(kLevel.bytes.end(), pRow, pRow + uiRowBytes);
					}
				}
			}
			if (uiType == D3DRTYPE_VOLUMETEXTURE) {
				static_cast<IDirect3DVolumeTexture9*>(apTexture)->UnlockBox(uiLevel);
			} else if (uiType == D3DRTYPE_CUBETEXTURE) {
				static_cast<IDirect3DCubeTexture9*>(apTexture)->UnlockRect(uiFace, uiLevel);
			} else {
				static_cast<IDirect3DTexture9*>(apTexture)->UnlockRect(uiLevel);
			}
			kLevels.push_back(std::move(kLevel));
		}
	}
	return kLevels;
}

d3dx::Image Decode(const Level& arLevel)
{
	d3dx::Image kImage;
	kImage.width = arLevel.width;
	kImage.height = arLevel.height;
	kImage.depth = arLevel.depth;
	kImage.pixels.resize(static_cast<size_t>(kImage.width) * kImage.height * kImage.depth);
	const d3dx::FormatInfo& rInfo = *d3dx::FindFormat(arLevel.format);
	const uint32_t uiPitch = d3dx::RowBytes(rInfo, arLevel.width);
	const uint32_t uiSlice = uiPitch * d3dx::RowCount(rInfo, arLevel.height);
	for (uint32_t z = 0; z < arLevel.depth; ++z) {
		d3dx::DecodeSlice(rInfo, arLevel.bytes.data() + z * uiSlice, uiPitch, kImage, z);
	}
	return kImage;
}

int32_t Byte(float afValue)
{
	return static_cast<int32_t>(afValue * 255.0f + 0.5f);
}

// The 8-bit channel differences of two images: sum of squares, count, max.
void Difference(const d3dx::Image& arA, const d3dx::Image& arB, double& arSquares, double& arCount, uint32_t& arMax)
{
	for (size_t i = 0; i < arA.pixels.size(); ++i) {
		const float afA[4] = {arA.pixels[i].r, arA.pixels[i].g, arA.pixels[i].b, arA.pixels[i].a};
		const float afB[4] = {arB.pixels[i].r, arB.pixels[i].g, arB.pixels[i].b, arB.pixels[i].a};
		for (int32_t c = 0; c < 4; ++c) {
			const uint32_t uiDiff = static_cast<uint32_t>(std::abs(Byte(afA[c]) - Byte(afB[c])));
			arSquares += static_cast<double>(uiDiff) * uiDiff;
			arMax = uiDiff > arMax ? uiDiff : arMax;
		}
		arCount += 4;
	}
}

IDirect3DBaseTexture9* Create(const Lib& arLib, uint32_t auiType, const std::vector<uint8_t>& arData, HRESULT& arResult)
{
	IDirect3DBaseTexture9* pTexture = nullptr;
	const uint32_t uiSize = static_cast<uint32_t>(arData.size());
	if (auiType == D3DRTYPE_CUBETEXTURE) {
		arResult = arLib.CreateCubeTexture(gpDevice, arData.data(), uiSize,
			reinterpret_cast<IDirect3DCubeTexture9**>(&pTexture));
	} else if (auiType == D3DRTYPE_VOLUMETEXTURE) {
		arResult = arLib.CreateVolumeTexture(gpDevice, arData.data(), uiSize,
			reinterpret_cast<IDirect3DVolumeTexture9**>(&pTexture));
	} else {
		arResult = arLib.CreateTexture(gpDevice, arData.data(), uiSize, reinterpret_cast<IDirect3DTexture9**>(&pTexture));
	}
	return pTexture;
}

std::string FormatName(uint32_t auiFormat)
{
	if (auiFormat > 0xFF) {
		return std::string(reinterpret_cast<const char*>(&auiFormat), 4);
	}
	return "fmt" + std::to_string(auiFormat);
}

void CompareTexture(const Lib& arTest, const Lib& arReference, const std::string& arName,
	const std::vector<uint8_t>& arData)
{
	D3DXIMAGE_INFO kInfoTest{}, kInfoRef{};
	const HRESULT hTest = arTest.GetImageInfo(arData.data(), static_cast<uint32_t>(arData.size()), &kInfoTest);
	const HRESULT hRef = arReference.GetImageInfo(arData.data(), static_cast<uint32_t>(arData.size()), &kInfoRef);
	const std::string kClass = std::string(arName.rfind("made-up", 0) == 0 ? "made-up " : "") +
		FormatName(kInfoRef.Format) + (kInfoRef.ResourceType == D3DRTYPE_CUBETEXTURE ? " cube" :
		(kInfoRef.ResourceType == D3DRTYPE_VOLUMETEXTURE ? " volume" : " 2d")) +
		(kInfoRef.MipLevels > 1 ? " mips" : " 1-level");
	ClassStats& rStats = gkTextureStats[kClass];
	++rStats.files;
	if (hTest != hRef || std::memcmp(&kInfoTest, &kInfoRef, sizeof(kInfoRef))) {
		Fail("info %s: %08lX %ux%ux%u m%u f%X r%u | dll %08lX %ux%ux%u m%u f%X r%u", arName.c_str(), hTest,
			kInfoTest.Width, kInfoTest.Height, kInfoTest.Depth, kInfoTest.MipLevels, kInfoTest.Format,
			kInfoTest.ResourceType, hRef, kInfoRef.Width, kInfoRef.Height, kInfoRef.Depth, kInfoRef.MipLevels,
			kInfoRef.Format, kInfoRef.ResourceType);
		return;
	}
	++rStats.infoEqual;
	HRESULT hCreateTest, hCreateRef;
	IDirect3DBaseTexture9* pTest = Create(arTest, kInfoRef.ResourceType, arData, hCreateTest);
	IDirect3DBaseTexture9* pRef = Create(arReference, kInfoRef.ResourceType, arData, hCreateRef);
	if (!pTest && !pRef && hCreateTest == hCreateRef) {
		// Both refuse the file with the same error (an A8 DDS): equal, nothing to compare.
	} else if (!pTest || !pRef) {
		Fail("create %s: %08lX / dll %08lX", arName.c_str(), hCreateTest, hCreateRef);
	} else {
		++rStats.createBoth;
		const std::vector<Level> kTest = ReadLevels(pTest);
		const std::vector<Level> kRef = ReadLevels(pRef);
		bool bShape = kTest.size() == kRef.size();
		for (size_t i = 0; bShape && i < kTest.size(); ++i) {
			bShape = kTest[i].format == kRef[i].format && kTest[i].width == kRef[i].width &&
				kTest[i].height == kRef[i].height && kTest[i].depth == kRef[i].depth &&
				kTest[i].bytes.size() == kRef[i].bytes.size();
		}
		if (!bShape) {
			Fail("shape %s: %zu levels fmt %X / dll %zu levels fmt %X", arName.c_str(), kTest.size(),
				kTest.empty() ? 0 : kTest[0].format, kRef.size(), kRef.empty() ? 0 : kRef[0].format);
		} else {
			++rStats.shapeEqual;
			const uint32_t uiLevels = pRef->GetLevelCount();
			for (size_t i = 0; i < kTest.size(); ++i) {
				// The DLL decodes and re-encodes a DXT level narrower or lower than a block (its compressor): those
				// count with the generated levels.
				const bool bPartialBlock = d3dx::FindFormat(kRef[i].format)->block &&
					(kRef[i].width < 4 || kRef[i].height < 4);
				const bool bFromFile = (i % uiLevels) < kInfoRef.MipLevels && kRef[i].format == kInfoRef.Format &&
					!bPartialBlock;
				const bool bEqual = kTest[i].bytes == kRef[i].bytes;
				if (bFromFile) {
					++rStats.fileLevels;
					rStats.fileLevelsEqual += bEqual;
					if (!bEqual) {
						Fail("bytes %s level %zu (from the file)", arName.c_str(), i);
					}
				} else {
					++rStats.generatedLevels;
					rStats.generatedEqual += bEqual;
					if (!bEqual) {
						Difference(Decode(kTest[i]), Decode(kRef[i]), rStats.generatedSquares, rStats.generatedTexels,
							rStats.generatedMax);
					}
				}
			}
		}
	}
	if (pTest) {
		pTest->Release();
	}
	if (pRef) {
		pRef->Release();
	}
}

void PrintTextureStats()
{
	for (const auto& [rClass, rStats] : gkTextureStats) {
		const double fRms = rStats.generatedTexels ? std::sqrt(rStats.generatedSquares / rStats.generatedTexels) : 0.0;
		std::printf("  %-22s files %u, info equal %u, both created %u, same levels/formats %u; file levels %u "
			"equal %u; generated levels %u equal %u (rms %.3f, max %u of 255 over the differing ones)\n",
			rClass.c_str(), rStats.files, rStats.infoEqual, rStats.createBoth, rStats.shapeEqual, rStats.fileLevels,
			rStats.fileLevelsEqual, rStats.generatedLevels, rStats.generatedEqual, fRms, rStats.generatedMax);
	}
}

// A DDS file of random pixels: a format the archives lack, or a volume map.
std::vector<uint8_t> MakeDds(uint32_t auiFormat, uint32_t auiWidth, uint32_t auiHeight, uint32_t auiDepth,
	uint32_t auiLevels)
{
	const d3dx::FormatInfo& rInfo = *d3dx::FindFormat(auiFormat);
	DDS_HEADER kHeader{};
	kHeader.dwSize = sizeof(DDS_HEADER);
	kHeader.dwHeaderFlags = 0x1007 | (auiLevels > 1 ? 0x20000 : 0) | (auiDepth > 1 ? 0x800000 : 0);
	kHeader.dwHeight = auiHeight;
	kHeader.dwWidth = auiWidth;
	kHeader.dwDepth = auiDepth > 1 ? auiDepth : 0;
	kHeader.dwMipMapCount = auiLevels;
	kHeader.ddspf.dwSize = sizeof(DDS_PIXELFORMAT);
	if (rInfo.block) {
		kHeader.ddspf.dwFlags = DDPF_FOURCC;
		kHeader.ddspf.dwFourCC = auiFormat;
	} else {
		kHeader.ddspf.dwFlags = (rInfo.luminance ? 0x20000 : (rInfo.rMask ? 0x40 : 0x2)) | (rInfo.aMask ? 0x1 : 0);
		kHeader.ddspf.dwRGBBitCount = rInfo.bits;
		kHeader.ddspf.dwRBitMask = rInfo.rMask;
		kHeader.ddspf.dwGBitMask = rInfo.gMask;
		kHeader.ddspf.dwBBitMask = rInfo.bMask;
		kHeader.ddspf.dwABitMask = rInfo.aMask;
	}
	kHeader.dwSurfaceFlags = 0x1000 | (auiLevels > 1 ? 0x400008 : 0);
	kHeader.dwCubemapFlags = auiDepth > 1 ? 0x200000 : 0;
	std::vector<uint8_t> kFile(4 + sizeof(DDS_HEADER));
	std::memcpy(kFile.data(), "DDS ", 4);
	std::memcpy(kFile.data() + 4, &kHeader, sizeof(kHeader));
	for (uint32_t i = 0; i < auiLevels; ++i) {
		const uint32_t w = auiWidth >> i ? auiWidth >> i : 1;
		const uint32_t h = auiHeight >> i ? auiHeight >> i : 1;
		const uint32_t d = auiDepth >> i ? auiDepth >> i : 1;
		const uint32_t uiBytes = d3dx::RowBytes(rInfo, w) * d3dx::RowCount(rInfo, h) * d;
		for (uint32_t b = 0; b < uiBytes; ++b) {
			kFile.push_back(static_cast<uint8_t>(Next()));
		}
	}
	return kFile;
}

// --- surfaces, save -------------------------------------------------------------------------------------------------

// A one-level texture the test makes itself; one the device refuses ends the run with a message.
IDirect3DTexture9* NewTexture(uint32_t auiWidth, uint32_t auiHeight, uint32_t auiUsage, uint32_t auiFormat,
	D3DPOOL aePool)
{
	IDirect3DTexture9* pTexture = nullptr;
	const HRESULT hResult = gpDevice->CreateTexture(auiWidth, auiHeight, 1, auiUsage, auiFormat, aePool, &pTexture,
		nullptr);
	if (hResult < 0 || !pTexture) {
		std::printf("CreateTexture %ux%u %s usage %X pool %d: %08lX; run stopped, %d failures so far\n", auiWidth,
			auiHeight, FormatName(auiFormat).c_str(), auiUsage, static_cast<int32_t>(aePool), hResult, giFailures);
		std::exit(2);
	}
	return pTexture;
}

IDirect3DTexture9* SystemTexture(uint32_t auiWidth, uint32_t auiHeight, uint32_t auiFormat)
{
	return NewTexture(auiWidth, auiHeight, 0, auiFormat, D3DPOOL_SYSTEMMEM);
}

IDirect3DSurface9* Level0(IDirect3DTexture9* apTexture)
{
	IDirect3DSurface9* pSurface = nullptr;
	apTexture->GetSurfaceLevel(0, &pSurface);
	return pSurface;
}

struct ErrorStats {
	uint32_t surfaces = 0;
	double squaresTest = 0, squaresRef = 0, squaresBetween = 0, texels = 0;
	uint32_t maxTest = 0, maxRef = 0, maxBetween = 0;
};

std::map<std::string, ErrorStats> gkEncodeStats;
uint32_t guiDecodeSurfaces = 0;
uint32_t guiDecodeEqual = 0;
uint32_t guiEncodeSkipped = 0;

// The game's ConvertTexture calls on one texture's level 0: DXT -> A8R8G8B8 through both (bytes compared), then that
// A8R8G8B8 -> DXT1/3/5 through both (each decoded and compared with the source).
void CompareSurfaces(const Lib& arTest, const Lib& arReference, IDirect3DTexture9* apSource)
{
	D3DSURFACE_DESC kDesc;
	apSource->GetLevelDesc(0, &kDesc);
	IDirect3DSurface9* pSource = Level0(apSource);
	IDirect3DTexture9* pArgbTest = SystemTexture(kDesc.Width, kDesc.Height, D3DFMT_A8R8G8B8);
	IDirect3DTexture9* pArgbRef = SystemTexture(kDesc.Width, kDesc.Height, D3DFMT_A8R8G8B8);
	IDirect3DSurface9* pTest = Level0(pArgbTest);
	IDirect3DSurface9* pRef = Level0(pArgbRef);
	const HRESULT hTest = arTest.LoadSurfaceFromSurface(pTest, nullptr, nullptr, pSource, nullptr, nullptr,
		D3DX_DEFAULT, 0);
	const HRESULT hRef = arReference.LoadSurfaceFromSurface(pRef, nullptr, nullptr, pSource, nullptr, nullptr,
		D3DX_DEFAULT, 0);
	++guiDecodeSurfaces;
	const std::vector<Level> kTest = ReadLevels(pArgbTest);
	const std::vector<Level> kRef = ReadLevels(pArgbRef);
	if (hTest < 0 || hRef < 0 || kTest[0].bytes != kRef[0].bytes) {
		size_t uiAt = 0;
		while (uiAt + 4 < kRef[0].bytes.size() && !std::memcmp(&kTest[0].bytes[uiAt], &kRef[0].bytes[uiAt], 4)) {
			uiAt += 4;
		}
		uint32_t uiTest = 0, uiRef = 0;
		std::memcpy(&uiTest, &kTest[0].bytes[uiAt], 4);
		std::memcpy(&uiRef, &kRef[0].bytes[uiAt], 4);
		Fail("LoadSurfaceFromSurface %s -> A8R8G8B8 %ux%u: %08lX / dll %08lX, first difference at pixel %zu: "
			"%08X / dll %08X", FormatName(kDesc.Format).c_str(), kDesc.Width, kDesc.Height, hTest, hRef, uiAt / 4,
			uiTest, uiRef);
	} else {
		++guiDecodeEqual;
	}
	const d3dx::Image kSource = Decode(kRef[0]);
	// The device makes no DXT surface whose size is not a whole number of blocks (D3DERR_INVALIDCALL for a 2x8 one).
	const bool bWholeBlocks = kDesc.Width % 4 == 0 && kDesc.Height % 4 == 0;
	guiEncodeSkipped += !bWholeBlocks;
	for (uint32_t uiFormat : {D3DFMT_DXT1, D3DFMT_DXT3, D3DFMT_DXT5}) {
		if (!bWholeBlocks) {
			break;
		}
		IDirect3DTexture9* pDxtTest = SystemTexture(kDesc.Width, kDesc.Height, uiFormat);
		IDirect3DTexture9* pDxtRef = SystemTexture(kDesc.Width, kDesc.Height, uiFormat);
		IDirect3DSurface9* pDstTest = Level0(pDxtTest);
		IDirect3DSurface9* pDstRef = Level0(pDxtRef);
		const HRESULT hEncodeTest = arTest.LoadSurfaceFromSurface(pDstTest, nullptr, nullptr, pRef, nullptr, nullptr,
			D3DX_DEFAULT, 0);
		const HRESULT hEncodeRef = arReference.LoadSurfaceFromSurface(pDstRef, nullptr, nullptr, pRef, nullptr,
			nullptr, D3DX_DEFAULT, 0);
		if (hEncodeTest < 0 || hEncodeRef < 0) {
			Fail("LoadSurfaceFromSurface A8R8G8B8 -> %s: %08lX / dll %08lX", FormatName(uiFormat).c_str(),
				hEncodeTest, hEncodeRef);
		} else {
			ErrorStats& rStats = gkEncodeStats[FormatName(uiFormat)];
			++rStats.surfaces;
			const d3dx::Image kDecodedTest = Decode(ReadLevels(pDxtTest)[0]);
			const d3dx::Image kDecodedRef = Decode(ReadLevels(pDxtRef)[0]);
			double fCount = 0;
			Difference(kDecodedTest, kSource, rStats.squaresTest, fCount, rStats.maxTest);
			fCount = 0;
			Difference(kDecodedRef, kSource, rStats.squaresRef, fCount, rStats.maxRef);
			Difference(kDecodedTest, kDecodedRef, rStats.squaresBetween, rStats.texels, rStats.maxBetween);
		}
		pDstTest->Release();
		pDstRef->Release();
		pDxtTest->Release();
		pDxtRef->Release();
	}
	pTest->Release();
	pRef->Release();
	pArgbTest->Release();
	pArgbRef->Release();
	pSource->Release();
}

std::vector<uint8_t> ReadFile(const std::string& arPath)
{
	std::vector<uint8_t> kData;
	FILE* pFile = std::fopen(arPath.c_str(), "rb");
	if (!pFile) {
		return kData;
	}
	std::fseek(pFile, 0, SEEK_END);
	kData.resize(static_cast<size_t>(std::ftell(pFile)));
	std::fseek(pFile, 0, SEEK_SET);
	std::fread(kData.data(), 1, kData.size(), pFile);
	std::fclose(pFile);
	return kData;
}

// The stand-in saves level 0 as PNG; the DLL reads the PNG back; the pixels must equal the saved texture's (alpha
// only where the format has it).
void CompareSave(const Lib& arTest, const Lib& arReference, IDirect3DTexture9* apTexture, const std::string& arPath,
	const char* apWhat)
{
	const HRESULT hSave = arTest.SaveTextureToFileA(arPath.c_str(), D3DXIFF_PNG, apTexture, nullptr);
	const std::vector<uint8_t> kPng = ReadFile(arPath);
	IDirect3DTexture9* pBack = nullptr;
	const HRESULT hLoad = kPng.empty() ? E_FAIL :
		arReference.CreateTexture(gpDevice, kPng.data(), static_cast<uint32_t>(kPng.size()), &pBack);
	if (hSave < 0 || hLoad < 0 || !pBack) {
		Fail("save %s: save %08lX, the DLL's load %08lX", apWhat, hSave, hLoad);
		return;
	}
	// A render target is read through a system-memory copy.
	D3DSURFACE_DESC kDesc;
	apTexture->GetLevelDesc(0, &kDesc);
	IDirect3DTexture9* pReadable = apTexture;
	if (kDesc.Pool == D3DPOOL_DEFAULT) {
		pReadable = SystemTexture(kDesc.Width, kDesc.Height, kDesc.Format);
		IDirect3DSurface9* pFrom = Level0(apTexture);
		IDirect3DSurface9* pTo = Level0(pReadable);
		gpDevice->GetRenderTargetData(pFrom, pTo);
		pFrom->Release();
		pTo->Release();
	}
	const d3dx::Image kSaved = Decode(ReadLevels(pReadable)[0]);
	const d3dx::Image kRead = Decode(ReadLevels(pBack)[0]);
	if (pReadable != apTexture) {
		pReadable->Release();
	}
	pBack->Release();
	uint32_t uiMax = 0;
	const bool bAlpha = d3dx::FindFormat(kDesc.Format)->aMask != 0;
	for (size_t i = 0; i < kSaved.pixels.size(); ++i) {
		const d3dx::Color& a = kSaved.pixels[i];
		const d3dx::Color& b = kRead.pixels[i];
		const int32_t aiDiff[4] = {Byte(a.r) - Byte(b.r), Byte(a.g) - Byte(b.g), Byte(a.b) - Byte(b.b),
			bAlpha ? Byte(a.a) - Byte(b.a) : 0};
		for (int32_t iDiff : aiDiff) {
			uiMax = static_cast<uint32_t>(std::abs(iDiff)) > uiMax ? static_cast<uint32_t>(std::abs(iDiff)) : uiMax;
		}
	}
	std::printf("  save %-40s %ux%u %s: PNG %zu bytes, read back by the DLL, max difference %u\n", apWhat,
		kDesc.Width, kDesc.Height, FormatName(kDesc.Format).c_str(), kPng.size(), uiMax);
	if (uiMax) {
		Fail("save %s: pixels differ", apWhat);
	}
}

// --- shaders --------------------------------------------------------------------------------------------------------

uint32_t guiShaders = 0, guiShadersEqual = 0, guiConstants = 0;

void CompareConstantTable(const Lib& arTest, const Lib& arReference, const std::string& arName,
	const uint32_t* apCode)
{
	++guiShaders;
	ID3DXConstantTable* pTest = nullptr;
	ID3DXConstantTable* pRef = nullptr;
	const HRESULT hTest = arTest.GetShaderConstantTable(apCode, &pTest);
	const HRESULT hRef = arReference.GetShaderConstantTable(apCode, &pRef);
	bool bEqual = (hTest < 0) == (hRef < 0);
	if (pTest && pRef) {
		D3DXCONSTANTTABLE_DESC kDescTest{}, kDescRef{};
		pTest->GetDesc(&kDescTest);
		pRef->GetDesc(&kDescRef);
		bEqual = bEqual && kDescTest.Version == kDescRef.Version && kDescTest.Constants == kDescRef.Constants &&
			!std::strcmp(kDescTest.Creator ? kDescTest.Creator : "", kDescRef.Creator ? kDescRef.Creator : "");
		bEqual = bEqual && pTest->GetBufferSize() == pRef->GetBufferSize() &&
			!std::memcmp(pTest->GetBufferPointer(), pRef->GetBufferPointer(), pRef->GetBufferSize());
		for (uint32_t i = 0; bEqual && i < kDescRef.Constants; ++i) {
			++guiConstants;
			D3DXCONSTANT_DESC kTest{}, kRef{};
			uint32_t uiCountTest = 1, uiCountRef = 1;
			const D3DXHANDLE hConstantRef = pRef->GetConstant(nullptr, i);
			pRef->GetConstantDesc(hConstantRef, &kRef, &uiCountRef);
			pTest->GetConstantDesc(pTest->GetConstant(nullptr, i), &kTest, &uiCountTest);
			D3DXCONSTANT_DESC kByName{};
			uint32_t uiCountByName = 1;
			pTest->GetConstantDesc(pTest->GetConstantByName(nullptr, kRef.Name), &kByName, &uiCountByName);
			const bool bDefault = (kTest.DefaultValue == nullptr) == (kRef.DefaultValue == nullptr) &&
				(!kRef.DefaultValue || !std::memcmp(kTest.DefaultValue, kRef.DefaultValue, kRef.Bytes));
			bEqual = !std::strcmp(kTest.Name, kRef.Name) && !std::strcmp(kByName.Name, kRef.Name) &&
				kTest.RegisterSet == kRef.RegisterSet && kTest.RegisterIndex == kRef.RegisterIndex &&
				kTest.RegisterCount == kRef.RegisterCount && kTest.Class == kRef.Class && kTest.Type == kRef.Type &&
				kTest.Rows == kRef.Rows && kTest.Columns == kRef.Columns && kTest.Elements == kRef.Elements &&
				kTest.StructMembers == kRef.StructMembers && kTest.Bytes == kRef.Bytes && bDefault &&
				uiCountTest == uiCountRef;
			if (!bEqual) {
				Fail("constant %s %s: set %u/%u index %u/%u count %u/%u class %u/%u type %u/%u %ux%u/%ux%u "
					"elements %u/%u members %u/%u bytes %u/%u default %d descs %u/%u", arName.c_str(), kRef.Name,
					kTest.RegisterSet, kRef.RegisterSet, kTest.RegisterIndex, kRef.RegisterIndex, kTest.RegisterCount,
					kRef.RegisterCount, kTest.Class, kRef.Class, kTest.Type, kRef.Type, kTest.Rows, kTest.Columns,
					kRef.Rows, kRef.Columns, kTest.Elements, kRef.Elements, kTest.StructMembers, kRef.StructMembers,
					kTest.Bytes, kRef.Bytes, bDefault, uiCountTest, uiCountRef);
				break;
			}
		}
	}
	if (!bEqual) {
		Fail("constant table %s: %08lX / dll %08lX", arName.c_str(), hTest, hRef);
	} else {
		++guiShadersEqual;
	}
	if (pTest) {
		pTest->Release();
	}
	if (pRef) {
		pRef->Release();
	}
}

// A device whose only working method is GetDeviceCaps, for the profile functions on made-up caps.
struct FakeDevice {
	void** vtable;
	D3DCAPS9 caps;
};

HRESULT WINAPI FakeQueryInterface(FakeDevice*, const GUID&, void**) { return E_NOINTERFACE; }
uint32_t WINAPI FakeRefs(FakeDevice*) { return 1; }
HRESULT WINAPI FakeGetDeviceCaps(FakeDevice* apDevice, D3DCAPS9* apCaps)
{
	*apCaps = apDevice->caps;
	return D3D_OK;
}

void CompareProfiles(const Lib& arTest, const Lib& arReference)
{
	const char* pVsTest = arTest.GetVertexShaderProfile(gpDevice);
	const char* pVsRef = arReference.GetVertexShaderProfile(gpDevice);
	const char* pPsTest = arTest.GetPixelShaderProfile(gpDevice);
	const char* pPsRef = arReference.GetPixelShaderProfile(gpDevice);
	std::printf("  device: vertex %s / dll %s, pixel %s / dll %s\n", pVsTest, pVsRef, pPsTest, pPsRef);
	if (std::strcmp(pVsTest, pVsRef) || std::strcmp(pPsTest, pPsRef)) {
		Fail("profiles on the device");
	}
	static void* apVtable[120];
	apVtable[0] = reinterpret_cast<void*>(&FakeQueryInterface);
	apVtable[1] = reinterpret_cast<void*>(&FakeRefs);
	apVtable[2] = reinterpret_cast<void*>(&FakeRefs);
	apVtable[7] = reinterpret_cast<void*>(&FakeGetDeviceCaps);
	FakeDevice kFake{apVtable, {}};
	auto* pFake = reinterpret_cast<IDirect3DDevice9*>(&kFake);
	uint32_t uiCases = 0, uiEqual = 0;
	const uint32_t auiVersions[] = {0x0000, 0x0100, 0x0101, 0x0102, 0x0103, 0x0104, 0x0200, 0x0201, 0x0300, 0x0301};
	const uint32_t auiPsCaps[] = {0, 0x1F, 0x10, 0x1E};
	const int32_t aiTemps[] = {12, 13, 22, 32};
	const int32_t aiFlow[] = {0, 24};
	for (uint32_t uiVersion : auiVersions) {
		for (uint32_t uiCaps : auiPsCaps) {
			for (int32_t iTemps : aiTemps) {
				for (int32_t iFlow : aiFlow) {
					kFake.caps.VertexShaderVersion = 0xFFFE0000 | uiVersion;
					kFake.caps.PixelShaderVersion = 0xFFFF0000 | uiVersion;
					kFake.caps.VS20Caps = {uiCaps, iFlow, iTemps, 0};
					kFake.caps.PS20Caps = {uiCaps, iFlow, iTemps, 0, 512};
					const char* apNames[4] = {arTest.GetVertexShaderProfile(pFake),
						arReference.GetVertexShaderProfile(pFake), arTest.GetPixelShaderProfile(pFake),
						arReference.GetPixelShaderProfile(pFake)};
					uiCases += 2;
					for (int32_t i = 0; i < 4; i += 2) {
						const bool bSame = (!apNames[i] && !apNames[i + 1]) ||
							(apNames[i] && apNames[i + 1] && !std::strcmp(apNames[i], apNames[i + 1]));
						uiEqual += bSame;
						if (!bSame) {
							Fail("%s profile for version %X caps %X temps %d flow %d: %s / dll %s", i ? "pixel" : "vertex",
								uiVersion, uiCaps, iTemps, iFlow, apNames[i] ? apNames[i] : "null",
								apNames[i + 1] ? apNames[i + 1] : "null");
						}
					}
				}
			}
		}
	}
	std::printf("  made-up caps: %u profile names, %u equal\n", uiCases, uiEqual);
}

// One DDS file in detail: per level, the channels' rms difference between the two, and for each the difference to
// the box filter of the DLL's level above.
void Detail(const Lib& arTest, const Lib& arReference, const std::vector<uint8_t>& arData)
{
	D3DXIMAGE_INFO kInfo;
	arReference.GetImageInfo(arData.data(), static_cast<uint32_t>(arData.size()), &kInfo);
	HRESULT hTest, hRef;
	IDirect3DBaseTexture9* pTest = Create(arTest, kInfo.ResourceType, arData, hTest);
	IDirect3DBaseTexture9* pRef = Create(arReference, kInfo.ResourceType, arData, hRef);
	if (!pTest || !pRef) {
		std::printf("create %08lX / dll %08lX\n", hTest, hRef);
		return;
	}
	const std::vector<Level> kTest = ReadLevels(pTest);
	const std::vector<Level> kRef = ReadLevels(pRef);
	for (size_t i = 0; i < kRef.size() && i < kTest.size(); ++i) {
		const d3dx::Image kA = Decode(kTest[i]);
		const d3dx::Image kB = Decode(kRef[i]);
		double afBetween[4] = {0, 0, 0, 0}, afTestBox[4] = {0, 0, 0, 0}, afRefBox[4] = {0, 0, 0, 0};
		d3dx::Image kBox;
		if (i) {
			kBox = d3dx::BoxDown(Decode(kRef[i - 1]));
		}
		for (size_t p = 0; p < kA.pixels.size(); ++p) {
			const float* pA = &kA.pixels[p].r;
			const float* pB = &kB.pixels[p].r;
			for (int32_t c = 0; c < 4; ++c) {
				const int32_t iBetween = Byte(pA[c]) - Byte(pB[c]);
				afBetween[c] += iBetween * iBetween;
				if (i) {
					const float* pBox = &kBox.pixels[p].r;
					afTestBox[c] += (Byte(pA[c]) - Byte(pBox[c])) * (Byte(pA[c]) - Byte(pBox[c]));
					afRefBox[c] += (Byte(pB[c]) - Byte(pBox[c])) * (Byte(pB[c]) - Byte(pBox[c]));
				}
			}
		}
		const double fN = static_cast<double>(kA.pixels.size());
		std::printf("level %zu %ux%u %s equal %d: between rgba %.2f %.2f %.2f %.2f; stand-in to box %.2f %.2f %.2f "
			"%.2f; dll to box %.2f %.2f %.2f %.2f\n", i, kRef[i].width, kRef[i].height, FormatName(kRef[i].format).c_str(),
			kTest[i].bytes == kRef[i].bytes, std::sqrt(afBetween[0] / fN), std::sqrt(afBetween[1] / fN),
			std::sqrt(afBetween[2] / fN), std::sqrt(afBetween[3] / fN), std::sqrt(afTestBox[0] / fN),
			std::sqrt(afTestBox[1] / fN), std::sqrt(afTestBox[2] / fN), std::sqrt(afTestBox[3] / fN),
			std::sqrt(afRefBox[0] / fN), std::sqrt(afRefBox[1] / fN), std::sqrt(afRefBox[2] / fN),
			std::sqrt(afRefBox[3] / fN));
		if (kTest[i].bytes.size() <= 64 && kTest[i].bytes != kRef[i].bytes) {
			for (size_t b = 0; b < kTest[i].bytes.size(); ++b) {
				std::printf("%02x", kTest[i].bytes[b]);
			}
			std::printf(" / dll ");
			for (size_t b = 0; b < kRef[i].bytes.size(); ++b) {
				std::printf("%02x", kRef[i].bytes[b]);
			}
			std::printf("\n");
		}
	}
	pTest->Release();
	pRef->Release();
}

} // namespace

int main(int argc, char** argv)
{
	if (argc < 3) {
		std::printf("usage: D3DX9Compare <d3dx9_38.dll> <scratch dir> [-stride N] [-limit N] [-wine <dll>] "
			"<archive.bsa | package.sdp>...\n");
		return 2;
	}
	const Lib kTest = StandIn();
	const Lib kRef = FromDll(argv[1]);
	const std::string kScratch = argv[2];
	uint32_t uiStride = 1;
	uint32_t uiLimit = 0xFFFFFFFF;
	uint32_t uiMathCases = 20000;
	std::vector<std::string> kInputs;
	const char* pWine = nullptr;
	const char* pDetail = nullptr;
	for (int32_t i = 3; i < argc; ++i) {
		if (!std::strcmp(argv[i], "-stride") && i + 1 < argc) {
			uiStride = static_cast<uint32_t>(std::atoi(argv[++i]));
		} else if (!std::strcmp(argv[i], "-limit") && i + 1 < argc) {
			uiLimit = static_cast<uint32_t>(std::atoi(argv[++i]));
		} else if (!std::strcmp(argv[i], "-math") && i + 1 < argc) {
			uiMathCases = static_cast<uint32_t>(std::atoi(argv[++i]));
		} else if (!std::strcmp(argv[i], "-wine") && i + 1 < argc) {
			pWine = argv[++i];
		} else if (!std::strcmp(argv[i], "-detail") && i + 1 < argc) {
			pDetail = argv[++i];
		} else {
			kInputs.push_back(argv[i]);
		}
	}
	gpDevice = static_cast<IDirect3DDevice9*>(TestCreateDevice());
	if (!gpDevice) {
		std::printf("no Direct3D 9 device\n");
		return 2;
	}
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	if (pDetail) {
		Detail(kTest, kRef, ReadFile(pDetail));
		return 0;
	}

	std::printf("math (%u random cases per function):\n", uiMathCases);
	TestMath(kTest, kRef, "stand-in vs dll", uiMathCases);
	if (pWine) {
		const Lib kWine = FromDll(pWine);
		TestMath(kWine, kRef, "other vs dll", uiMathCases);
	}

	std::printf("textures:\n");
	uint32_t uiSeen = 0, uiDone = 0, uiSurfaceTests = 0;
	for (const std::string& rInput : kInputs) {
		if (rInput.size() < 4 || rInput.compare(rInput.size() - 4, 4, ".bsa")) {
			continue;
		}
		Archive kArchive;
		if (!kArchive.Open(rInput.c_str())) {
			std::printf("  cannot read %s\n", rInput.c_str());
			continue;
		}
		for (const Archive::Entry& rEntry : kArchive.entries) {
			if (rEntry.name.size() < 4 || rEntry.name.compare(rEntry.name.size() - 4, 4, ".dds") ||
				uiSeen++ % uiStride || uiDone >= uiLimit) {
				continue;
			}
			++uiDone;
			const std::vector<uint8_t> kData = kArchive.Read(rEntry);
			CompareTexture(kTest, kRef, rEntry.name, kData);
			// D3DXLoadSurfaceFromSurface on every 50th texture, through the DLL's texture as the source.
			if (uiDone % 50 == 1) {
				D3DXIMAGE_INFO kInfo;
				IDirect3DTexture9* pSource = nullptr;
				if (kRef.GetImageInfo(kData.data(), static_cast<uint32_t>(kData.size()), &kInfo) >= 0 &&
					kInfo.ResourceType == D3DRTYPE_TEXTURE &&
					kRef.CreateTexture(gpDevice, kData.data(), static_cast<uint32_t>(kData.size()), &pSource) >= 0) {
					CompareSurfaces(kTest, kRef, pSource);
					++uiSurfaceTests;
					if (uiSurfaceTests == 1) {
						CompareSave(kTest, kRef, pSource, kScratch + "\\save-dxt.png", rEntry.name.c_str());
					}
					pSource->Release();
				}
			}
		}
		std::fclose(kArchive.file);
	}
	// The formats and texture types the archives lack, with and without a mip chain.
	const uint32_t auiFormats[] = {D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, D3DFMT_A8B8G8R8, D3DFMT_X8B8G8R8, D3DFMT_R8G8B8,
		D3DFMT_R5G6B5, D3DFMT_X1R5G5B5, D3DFMT_A1R5G5B5, D3DFMT_A4R4G4B4, D3DFMT_X4R4G4B4, D3DFMT_A8, D3DFMT_L8,
		D3DFMT_A8L8, D3DFMT_DXT1, D3DFMT_DXT3, D3DFMT_DXT5};
	for (uint32_t uiFormat : auiFormats) {
		for (uint32_t uiCase = 0; uiCase < 4; ++uiCase) {
			const uint32_t uiW = 4u << (uiCase % 3), uiH = 64u >> (uiCase % 2);
			const std::string kName = "made-up " + FormatName(uiFormat) + " " + std::to_string(uiCase);
			CompareTexture(kTest, kRef, kName + " 2d full", MakeDds(uiFormat, uiW, uiH, 1, 1 + (uiH > uiW ?
				__builtin_ctz(uiH) : __builtin_ctz(uiW))));
			CompareTexture(kTest, kRef, kName + " 2d 1-level", MakeDds(uiFormat, uiW, uiH, 1, 1));
			CompareTexture(kTest, kRef, kName + " volume full", MakeDds(uiFormat, 16, 8, 4, 5));
			CompareTexture(kTest, kRef, kName + " volume 1-level", MakeDds(uiFormat, 16, 8, 4, 1));
		}
	}
	std::printf("  %u archive textures compared (every %uth)\n", uiDone, uiStride);
	PrintTextureStats();
	std::printf("  LoadSurfaceFromSurface DXT/packed -> A8R8G8B8: %u surfaces, %u byte-equal (%u not encoded back: not "
		"whole DXT blocks)\n", guiDecodeSurfaces, guiDecodeEqual, guiEncodeSkipped);
	for (const auto& [rFormat, rStats] : gkEncodeStats) {
		std::printf("  LoadSurfaceFromSurface A8R8G8B8 -> %s: %u surfaces; error against the source: stand-in rms "
			"%.3f max %u, dll rms %.3f max %u; stand-in against dll rms %.3f max %u\n", rFormat.c_str(),
			rStats.surfaces, std::sqrt(rStats.squaresTest / rStats.texels), rStats.maxTest,
			std::sqrt(rStats.squaresRef / rStats.texels), rStats.maxRef, std::sqrt(rStats.squaresBetween / rStats.texels),
			rStats.maxBetween);
	}
	// Save: a managed A8R8G8B8 and X8R8G8B8 texture of random pixels and a render target.
	for (uint32_t uiFormat : {D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8}) {
		IDirect3DTexture9* pTexture = NewTexture(256, 128, 0, uiFormat, D3DPOOL_MANAGED);
		D3DLOCKED_RECT kRect;
		pTexture->LockRect(0, &kRect, nullptr, 0);
		for (uint32_t y = 0; y < 128; ++y) {
			for (uint32_t x = 0; x < 256 * 4; ++x) {
				static_cast<uint8_t*>(kRect.pBits)[y * kRect.Pitch + x] = static_cast<uint8_t>(Next());
			}
		}
		pTexture->UnlockRect(0);
		CompareSave(kTest, kRef, pTexture, kScratch + "\\save-" + FormatName(uiFormat) + ".png",
			("random " + FormatName(uiFormat)).c_str());
		pTexture->Release();
	}
	{
		IDirect3DTexture9* pTarget = NewTexture(256, 256, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT);
		IDirect3DSurface9* pSurface = Level0(pTarget);
		for (uint32_t i = 0; i < 16; ++i) {
			const RECT kArea{static_cast<int32_t>(i * 16), 0, static_cast<int32_t>(i * 16 + 16), 256};
			gpDevice->ColorFill(pSurface, &kArea, Next() | 0xFF000000);
		}
		pSurface->Release();
		CompareSave(kTest, kRef, pTarget, kScratch + "\\save-target.png", "render target X8R8G8B8");
		pTarget->Release();
	}

	std::printf("shaders:\n");
	for (const std::string& rInput : kInputs) {
		if (rInput.size() < 4 || rInput.compare(rInput.size() - 4, 4, ".sdp")) {
			continue;
		}
		const std::vector<uint8_t> kPackage = ReadFile(rInput);
		uint32_t auiHeader[3];
		std::memcpy(auiHeader, kPackage.data(), 12);
		size_t uiAt = 12;
		for (uint32_t i = 0; i < auiHeader[1] && uiAt + 0x104 <= kPackage.size(); ++i) {
			const std::string kName(reinterpret_cast<const char*>(kPackage.data() + uiAt));
			uint32_t uiSize;
			std::memcpy(&uiSize, kPackage.data() + uiAt + 0x100, 4);
			std::vector<uint32_t> kCode(uiSize / 4 + 1);
			std::memcpy(kCode.data(), kPackage.data() + uiAt + 0x104, uiSize);
			CompareConstantTable(kTest, kRef, rInput + ":" + kName, kCode.data());
			uiAt += 0x104 + uiSize;
		}
	}
	std::printf("  %u shaders, %u tables equal (%u constants compared)\n", guiShaders, guiShadersEqual, guiConstants);
	CompareProfiles(kTest, kRef);
	ID3DXBuffer* pBuffer = nullptr;
	if (kTest.CreateBuffer(100, &pBuffer) < 0 || pBuffer->GetBufferSize() != 100 || !pBuffer->GetBufferPointer() ||
		pBuffer->Release()) {
		Fail("D3DXCreateBuffer");
	}
	ID3DXBuffer* pShader = nullptr;
	ID3DXBuffer* pErrors = nullptr;
	ID3DXConstantTable* pTable = nullptr;
	const char acSource[] = "float4 main() : COLOR { return 1; }";
	const HRESULT hCompile = D3DXCompileShader(acSource, sizeof(acSource) - 1, nullptr, nullptr, "main", "ps_2_0", 0,
		&pShader, &pErrors, &pTable);
	std::printf("  D3DXCompileShader: %08lX, error buffer \"%s\"\n", hCompile,
		pErrors ? static_cast<const char*>(pErrors->GetBufferPointer()) : "(none)");
	if (hCompile >= 0 || pShader || pTable || !pErrors) {
		Fail("D3DXCompileShader should fail with an error buffer");
	}
	if (pErrors) {
		pErrors->Release();
	}
	std::printf("%d failures\n", giFailures);
	return giFailures ? 1 : 0;
}
