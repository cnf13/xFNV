#include "SDK/FaceGen/EgmFileIO.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Reconstruction.hpp"
#include "SDK/FaceGen/BSFaceGenBinaryFile.h"
#include "SDK/FaceGen/TriFileIO.h"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/iosfwd.h"
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <stdint.h>

struct FffModeGS;

// Opens fname as a "FREGM002" file and reads its header; then each symmetric mode, then each asymmetric one: its
// vertex offsets sized to the header's numPoints and zeroed, its scale, and numPoints triples of shorts read into one
// buffer and copied into the offsets, four vertices a step and then one at a time. The by-value arguments the exe
// builds in its outgoing argument area (the file ID, the file name) are built in locals here and copied; their
// callees destroy them, as in the exe. A failed read returns without freeing the buffer, as the exe does. The exe's
// cleanup-only C++ EH frame is left out.
bool fffReadEgmFile(const std::basic_string<char, std::char_traits<char>, std::allocator<char>>& fname,
	uint32_t& basisKey, std::vector<FffModeGS, std::allocator<FffModeGS>>& symm,
	std::vector<FffModeGS, std::allocator<FffModeGS>>& asym) {
	FffEgmFileHeaderS hdr;
	XFNV_INLINED_LOCAL(BSFaceGenBinaryFile, bfile, std::nullopt, std::nullopt);
	float scale;
	std::string kFileId("FREGM002");
	::new (&bfile) BSFaceGenBinaryFile(kFileId);
	// Each return's destructor, which the exe inlines.
	auto fnClose = [&bfile](bool abResult) -> bool {
		bfile.~BSFaceGenBinaryFile();
		return abResult;
	};
	std::string kFileName(fname);
	if (!bfile.BSFaceGenBinaryFile::openForRead(kFileName, false)) {
		return fnClose(false);
	}
	if (!bfile.readHeader(&hdr, 1, sizeof(hdr))) {
		return fnClose(false);
	}
	hdr.fix();
	basisKey = hdr.basisKey;
	symm.clear();
	asym.clear();
	symm.resize(hdr.numSym);
	asym.resize(hdr.numAsym);
	int16_t* psData =
		static_cast<int16_t*>(MemoryManager_Singleton.Allocate(hdr.numPoints * sizeof(FR2Vect3TC<int16_t>)));
	// The triples read into psData, copied into pMode's vertex offsets.
	auto fnCopyPoints = [&hdr, psData](FffModeGS* pMode) {
		uint32_t j;
		for (j = 0; j + 3 < hdr.numPoints; j += 4) {
			FR2Vect3TC<int16_t>& kPt0 = pMode->vertPos[j];
			FR2Vect3TC<int16_t>& kPt1 = pMode->vertPos[j + 1];
			FR2Vect3TC<int16_t>& kPt2 = pMode->vertPos[j + 2];
			FR2Vect3TC<int16_t>& kPt3 = pMode->vertPos[j + 3];
			kPt0.x1 = psData[3 * j];
			kPt0.x2 = psData[3 * j + 1];
			kPt0.x3 = psData[3 * j + 2];
			kPt1.x1 = psData[3 * (j + 1)];
			kPt1.x2 = psData[3 * (j + 1) + 1];
			kPt1.x3 = psData[3 * (j + 1) + 2];
			kPt2.x1 = psData[3 * (j + 2)];
			kPt2.x2 = psData[3 * (j + 2) + 1];
			kPt2.x3 = psData[3 * (j + 2) + 2];
			kPt3.x1 = psData[3 * (j + 3)];
			kPt3.x2 = psData[3 * (j + 3) + 1];
			kPt3.x3 = psData[3 * (j + 3) + 2];
		}
		for (; j < hdr.numPoints; ++j) {
			pMode->vertPos[j].x1 = psData[3 * j];
			pMode->vertPos[j].x2 = psData[3 * j + 1];
			pMode->vertPos[j].x3 = psData[3 * j + 2];
		}
	};
	// The symmetric modes index the vector again for the resize and the scale; the asymmetric ones keep the first
	// element pointer.
	for (uint32_t i = 0; i < hdr.numSym; ++i) {
		FffModeGS* pMode = &symm[i];
		symm[i].vertPos.resize(hdr.numPoints, FR2Vect3TC<int16_t>{0, 0, 0});
		if (!bfile.BSFaceGenBinaryFile::read(&scale, sizeof(float), 1)) {
			return fnClose(false);
		}
		symm[i].scale = scale;
		if (!bfile.readShorts(psData, static_cast<int32_t>(hdr.numPoints * 3))) {
			return fnClose(false);
		}
		fnCopyPoints(pMode);
	}
	for (uint32_t i = 0; i < hdr.numAsym; ++i) {
		FffModeGS* pMode = &asym[i];
		pMode->vertPos.resize(hdr.numPoints, FR2Vect3TC<int16_t>{0, 0, 0});
		if (!bfile.BSFaceGenBinaryFile::read(&scale, sizeof(float), 1)) {
			return fnClose(false);
		}
		pMode->scale = scale;
		if (!bfile.readShorts(psData, static_cast<int32_t>(hdr.numPoints * 3))) {
			return fnClose(false);
		}
		fnCopyPoints(pMode);
	}
	if (psData) {
		MemoryManager_Singleton.Deallocate(psData);
	}
	return fnClose(true);
}

void FffEgmFileHeaderS::fix() {}

// isCorrectFileId's answer for the file named a1 against the id "FREGM002" (a FaceGen egm file). The exe builds the
// id string straight in FutBinaryFileC's by-value argument slot, which the constructor destroys. The exe's
// cleanup-only C++ EH frame is left out.
char fffIsEgmFile(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1) {
	std::string kFileId("FREGM002");
	FutBinaryFileC kFile(kFileId);
	char cResult = kFile.isCorrectFileId(*a1);
	return cResult;
}
