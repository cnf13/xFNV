#include "SDK/FaceGen/FanCtlFile.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/FaceGen/FanLinCtlGrpS.h"
#include "Reconstruction.hpp"
#include "SDK/FaceGen/BSFaceGenBinaryFile.h"
#include "SDK/FaceGen/binaryFile.h"
#include "SDK/MSVC/iosfwd.h"
#include <memory>
#include <string>

class FanGenCtlsGrpC;
class FanLinCtlGrpS;

void fanCtlFileHeaderS::fix() {
}

// Whether a1 names a file whose id is "FRCTL001" (a FaceGen ctl file). The exe builds the id string straight in
// FutBinaryFileC's by-value argument slot, which the constructor destroys; the dword it spills next to its byte
// result is that slot's address, kept for its EH frame. The exe's cleanup-only C++ EH frame is left out.
char fanIsCtlFile(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1)
{
	FNVD_INLINED_LOCAL(FutBinaryFileC, kFile, std::nullopt, std::nullopt);
	FNVD_CONSTRUCTED_LOCAL(std::string, kFileId, nullptr, "FRCTL001");
	::new (&kFile) FutBinaryFileC(kFileId);
	char cResult = kFile.isCorrectFileId(*a1);
	kFile.~FutBinaryFileC();
	return cResult;
}

// Reads the FaceGen control file fname: its header (the basis keys and sizes, passed out), then the linear and the
// general controls. The file object is built by the exe's constructor (0xAC5490) from the file ID, passed by value;
// each return runs its destructor inlined: this class's vtable, close, then the base destructor (0xAAAA80). The
// by-value file name's copy constructor is inlined too.
// The exe's cleanup-only C++ EH frame is left out.
bool fanReadCtlFile(const std::basic_string<char, std::char_traits<char>, std::allocator<char>>& fname,
	uint32_t& basisKeyGeo, uint32_t& basisKeyTex, uint32_t (*basisSize)[2], FanLinCtlGrpS& linCtl,
	FanGenCtlsGrpC& genCtl)
{
	fanCtlFileHeaderS header;
	FNVD_CONSTRUCTED_LOCAL(std::string, kFileId, nullptr, "FRCTL001");
	FNVD_CONSTRUCTED_LOCAL(BSFaceGenBinaryFile, kFile, nullptr, kFileId);
	// Each return's destructor, which the exe inlines.
	auto fnClose = [&kFile](bool abResult) -> bool {
		return abResult;
	};
	std::string kFileName;
	FNVD_RECONSTRUCT_AS(CopyConstruct, std::string)(&kFileName, fname);
	if (!kFile.BSFaceGenBinaryFile::openForRead(kFileName, true)) {
		return fnClose(false);
	}
	if (!kFile.readHeader(&header, 1, sizeof(header))) {
		return fnClose(false);
	}
	header.fix();
	basisKeyGeo = header.basisKeyGeo;
	basisKeyTex = header.basisKeyTex;
	for (uint32_t i = 0; i < 2; ++i) {
		for (uint32_t j = 0; j < 2; ++j) {
			basisSize[i][j] = header.basisSize[i][j];
		}
	}
	if (!linCtl.readFromFile(kFile, basisSize)) {
		return fnClose(false);
	}
	if (!genCtl.readFromFile(kFile, basisSize)) {
		return fnClose(false);
	}
	return fnClose(true);
}
