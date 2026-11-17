#include "Fallout Shared/FaceGen/EGMData.h"

#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Reconstruction.hpp"
#include "SDK/FaceGen/EgmFileIO.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/iosfwd.h"

#include <memory>
#include <string>

// The basis key and vectors are read from the .egm file apFileName.
EGMData::EGMData(char* apFileName) {
	std::basic_string<char, std::char_traits<char>, std::allocator<char>>
		kFileName(static_cast<const char*>(apFileName));
	fffReadEgmFile(kFileName, lEGMBasisKey, EGMBasis[0], EGMBasis[1]);
}
