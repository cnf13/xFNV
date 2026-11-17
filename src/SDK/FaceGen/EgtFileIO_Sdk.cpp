#include "SDK/FaceGen/EgtFileIO_Sdk.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Reconstruction.hpp"
#include "SDK/FaceGen/BSFaceGenBinaryFile.h"
#include "SDK/FaceGen/TriFileIO.h"
#include <vector>
#include <memory>
#include <string>
#include <stdint.h>

void FffEgtFileHeaderS::fix() {
}

// Opens fname as a "FREGT003" file and reads its header; then each symmetric mode, then each asymmetric one: its
// scale, the header's sizes, and three images of nrows rows of ncols bytes, the row stride padded to 8 bytes, read
// from the last row up. The by-value arguments the exe builds in its outgoing argument area (the file ID, the file
// name, the default modes) are built in locals here and copied; their callees destroy them, as in the exe. The
// asymmetric images take their first row through the out-of-line operator[] (0x65CC70) and the last image read's
// failure and the final return call the destructor out of line (0xAAAB30); the other failures inline it. The
// exe's cleanup-only C++ EH frame is left out.
bool fffReadEgtFile(const std::string& fname, uint32_t& basisKey,
	std::vector<FffModeTexS, std::allocator<FffModeTexS>>& symm,
	std::vector<FffModeTexS, std::allocator<FffModeTexS>>& asym)
{
	FffEgtFileHeaderS header;
	FNVD_INLINED_LOCAL(BSFaceGenBinaryFile, kFile, std::nullopt, std::nullopt);
	std::string kFileId("FREGT003");
	::new (&kFile) BSFaceGenBinaryFile(kFileId);
	// The early returns' destructor, which the exe inlines.
	auto fnFail = [&kFile]() -> bool {
		kFile.~BSFaceGenBinaryFile();
		return false;
	};
	std::string kFileName;
	// The by-value file name's copy constructor.
	FNVD_RECONSTRUCT_AS(CopyConstruct, std::string)(&kFileName, fname);
	if (!kFile.BSFaceGenBinaryFile::openForRead(kFileName, false)) {
		return fnFail();
	}
	if (!kFile.readHeader(&header, 1, sizeof(header))) {
		return fnFail();
	}
	header.fix();
	basisKey = header.basisKey;
	symm.clear();
	asym.clear();
	// The exe keeps the element size in a local and multiplies and divides by it (unsigned div).
	uint32_t uiElemSize = 1;
	uint32_t uiRowPad = ((header.ncols * uiElemSize + 7) & 0xFFFFFFF8u) / uiElemSize - header.ncols;
	uint32_t uiStride = header.ncols + uiRowPad;
	uint32_t uiTotal = uiStride * header.nrows;

	FffModeTexS kSymMode;
	kSymMode.~FffModeTexS();
	::new (&kSymMode) FffModeTexS;
	symm.resize(header.numSym, kSymMode);
	for (uint32_t i = 0; i < header.numSym; ++i) {
		if (!kFile.BSFaceGenBinaryFile::read(&symm[i], sizeof(float), 1)) {
			return fnFail();
		}
		symm[i].width = header.ncols;
		symm[i].height = header.nrows;
		symm[i].rowPad = uiRowPad;
		for (uint32_t j = 0; j < 3; ++j) {
			symm[i].img[j].resize(uiTotal, 0);
			uint8_t* pRow = &symm[i].img[j][uiTotal - uiStride];
			for (uint32_t k = 0; k < header.nrows; ++k) {
				if (!kFile.BSFaceGenBinaryFile::read(pRow, uiElemSize, header.ncols)) {
					return fnFail();
				}
				pRow -= header.ncols + uiRowPad;
			}
		}
	}

	FffModeTexS kAsymMode;
	kAsymMode.~FffModeTexS();
	::new (&kAsymMode) FffModeTexS;
	asym.resize(header.numAsym, kAsymMode);
	for (uint32_t i = 0; i < header.numAsym; ++i) {
		if (!kFile.BSFaceGenBinaryFile::read(&asym[i], sizeof(float), 1)) {
			return fnFail();
		}
		asym[i].width = header.ncols;
		asym[i].height = header.nrows;
		asym[i].rowPad = uiRowPad;
		for (uint32_t j = 0; j < 3; ++j) {
			asym[i].img[j].resize(uiTotal, 0);
			uint8_t* pRow = &asym[i].img[j][uiTotal - uiStride];
			for (uint32_t k = 0; k < header.nrows; ++k) {
				if (!kFile.BSFaceGenBinaryFile::read(pRow, uiElemSize, header.ncols)) {
					kFile.~BSFaceGenBinaryFile();
					return false;
				}
				pRow -= header.ncols + uiRowPad;
			}
		}
	}
	kFile.~BSFaceGenBinaryFile();
	return true;
}
