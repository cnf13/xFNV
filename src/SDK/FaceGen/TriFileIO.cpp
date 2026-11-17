#include "SDK/FaceGen/TriFileIO.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Fallout Shared/FaceGen/FffTriFileDataS.h"
#include "Fallout Shared/ModelLoader.h"
#include "Reconstruction.hpp"
#include "SDK/FaceGen/BSFaceGenBinaryFile.h"
#include "SDK/FaceGen/lastError.hpp"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/exception.h"
#include "SDK/MSVC/iosfwd.h"
#include <cmath>
#include "SDK/MSVC/string.h"
#include <vector>
#include <locale>
#include <memory>
#include <string>
#include <map>
#include <algorithm>
#include <stdint.h>

struct FffTriDiffMorphS;
struct FffTriSurfLabelS;
struct FffTriVtxLabelS;

void FffTriHeaderS::fix() {
	;
}

// Copies d's lists into this one. Each list is resized to d's count; the vertices, faces and texture lists are then
// copied whole with memcpy (&list[0] taken through the checked operator[], the destination's size counted after the
// resize), and the labels and morphs element by element through VC9's checked iterators: begin() and end(), which
// the exe inlines around the const iterator's out-of-line constructor; the loop's != inlined as !(==). Each element
// access goes through operator* again, as the exe's -> does: the labels' strings assigned whole, the morphs' own lists
// resized to the source's count and copied whole.
void FffTriFileDataS::copyFrom(const FffTriFileDataS& d) {
	using VtxLabelIter = std::vector<FffTriVtxLabelS, std::allocator<FffTriVtxLabelS>>::iterator;
	using VtxLabelConstIter = std::vector<FffTriVtxLabelS, std::allocator<FffTriVtxLabelS>>::const_iterator;
	using SurfLabelIter = std::vector<FffTriSurfLabelS, std::allocator<FffTriSurfLabelS>>::iterator;
	using SurfLabelConstIter = std::vector<FffTriSurfLabelS, std::allocator<FffTriSurfLabelS>>::const_iterator;
	using DiffMorphIter = std::vector<FffTriDiffMorphS, std::allocator<FffTriDiffMorphS>>::iterator;
	using DiffMorphConstIter = std::vector<FffTriDiffMorphS, std::allocator<FffTriDiffMorphS>>::const_iterator;
	using StatMorphIter = std::vector<FffTriStatMorphS, std::allocator<FffTriStatMorphS>>::iterator;
	using StatMorphConstIter = std::vector<FffTriStatMorphS, std::allocator<FffTriStatMorphS>>::const_iterator;

	vtxList.resize(d.vtxList.size(), FR2Vect3TC<float>{0.0f, 0.0f, 0.0f});
	size_t uiCount = vtxList.size();
	const FR2Vect3TC<float>* pSrcVtx = &d.vtxList[0];
	FR2Vect3TC<float>* pDstVtx = &vtxList[0];
	rt::memcpy(pDstVtx, pSrcVtx, uiCount * sizeof(FR2Vect3TC<float>));

	triList.resize(d.triList.size(), FR2Vect3TC<int>{0, 0, 0});
	uiCount = triList.size();
	const FR2Vect3TC<int>* pSrcTri = &d.triList[0];
	FR2Vect3TC<int>* pDstTri = &triList[0];
	rt::memcpy(pDstTri, pSrcTri, uiCount * sizeof(FR2Vect3TC<int>));

	quadList.resize(d.quadList.size(), FutVect4TC<int>{0, 0, 0, 0});
	uiCount = quadList.size();
	const FutVect4TC<int>* pSrcQuad = &d.quadList[0];
	FutVect4TC<int>* pDstQuad = &quadList[0];
	rt::memcpy(pDstQuad, pSrcQuad, uiCount * sizeof(FutVect4TC<int>));

	txtData.resize(d.txtData.size(), FR2Vect2TC<float>{0.0f, 0.0f});
	uiCount = txtData.size();
	const FR2Vect2TC<float>* pSrcTxt = &d.txtData[0];
	FR2Vect2TC<float>* pDstTxt = &txtData[0];
	rt::memcpy(pDstTxt, pSrcTxt, uiCount * sizeof(FR2Vect2TC<float>));

	txtTriList.resize(d.txtTriList.size(), FR2Vect3TC<int>{0, 0, 0});
	uiCount = txtTriList.size();
	const FR2Vect3TC<int>* pSrcTxtTri = &d.txtTriList[0];
	FR2Vect3TC<int>* pDstTxtTri = &txtTriList[0];
	rt::memcpy(pDstTxtTri, pSrcTxtTri, uiCount * sizeof(FR2Vect3TC<int>));

	txtQuadList.resize(d.txtQuadList.size(), FutVect4TC<int>{0, 0, 0, 0});
	uiCount = txtQuadList.size();
	const FutVect4TC<int>* pSrcTxtQuad = &d.txtQuadList[0];
	FutVect4TC<int>* pDstTxtQuad = &txtQuadList[0];
	rt::memcpy(pDstTxtQuad, pSrcTxtQuad, uiCount * sizeof(FutVect4TC<int>));

	vtxLabelList.resize(d.vtxLabelList.size());
	VtxLabelIter kVtxLabel = vtxLabelList.begin();
	VtxLabelConstIter kSrcVtxLabel = d.vtxLabelList.begin();
	while (true) {
		VtxLabelIter kEnd = vtxLabelList.end();
		if (kVtxLabel == kEnd) {
			break;
		}
		FffTriVtxLabelS& rDst = *kVtxLabel;
		const FffTriVtxLabelS& rSrc = *kSrcVtxLabel;
		rDst.idx = rSrc.idx;
		rDst.label.assign(rSrc.label, 0, std::string::npos);
		++kVtxLabel;
		++kSrcVtxLabel;
	}

	FffTriSurfLabelS kSurfLabel;
	kSurfLabel.~FffTriSurfLabelS();
	::new (&kSurfLabel) FffTriSurfLabelS;
	surfLabelList.resize(d.surfLabelList.size(), kSurfLabel);
	SurfLabelIter kSurfLabelIt = surfLabelList.begin();
	SurfLabelConstIter kSrcSurfLabel = d.surfLabelList.begin();
	while (true) {
		SurfLabelIter kEnd = surfLabelList.end();
		if (kSurfLabelIt == kEnd) {
			break;
		}
		FffTriSurfLabelS& rDst = *kSurfLabelIt;
		const FffTriSurfLabelS& rSrc = *kSrcSurfLabel;
		rDst.idx = rSrc.idx;
		rDst.vtxIntWgt[0] = rSrc.vtxIntWgt[0];
		rDst.vtxIntWgt[1] = rSrc.vtxIntWgt[1];
		rDst.vtxIntWgt[2] = rSrc.vtxIntWgt[2];
		rDst.label.assign(rSrc.label, 0, std::string::npos);
		++kSurfLabelIt;
		++kSrcSurfLabel;
	}

	FffTriDiffMorphS kDiffMorph;
	kDiffMorph.~FffTriDiffMorphS();
	::new (&kDiffMorph) FffTriDiffMorphS;
	diffMorphList.resize(d.diffMorphList.size(), kDiffMorph);
	DiffMorphIter kDiffMorphIt = diffMorphList.begin();
	DiffMorphConstIter kSrcDiffMorph = d.diffMorphList.begin();
	while (true) {
		DiffMorphIter kEnd = diffMorphList.end();
		if (kDiffMorphIt == kEnd) {
			break;
		}
		const FffTriDiffMorphS& rSrcLabel = *kSrcDiffMorph;
		FffTriDiffMorphS& rDstLabel = *kDiffMorphIt;
		rDstLabel.label.assign(rSrcLabel.label, 0, std::string::npos);
		uiCount = (*kSrcDiffMorph).vtxDelta.size();
		(*kDiffMorphIt).vtxDelta.resize(uiCount, FR2Vect3TC<float>{0.0f, 0.0f, 0.0f});
		uiCount = (*kDiffMorphIt).vtxDelta.size();
		const FR2Vect3TC<float>* pSrcDelta = &(*kSrcDiffMorph).vtxDelta[0];
		FR2Vect3TC<float>* pDstDelta = &(*kDiffMorphIt).vtxDelta[0];
		rt::memcpy(pDstDelta, pSrcDelta, uiCount * sizeof(FR2Vect3TC<float>));
		++kDiffMorphIt;
		++kSrcDiffMorph;
	}

	FffTriStatMorphS kStatMorph;
	kStatMorph.~FffTriStatMorphS();
	::new (&kStatMorph) FffTriStatMorphS;
	statMorphList.resize(d.statMorphList.size(), kStatMorph);
	StatMorphIter kStatMorphIt = statMorphList.begin();
	StatMorphConstIter kSrcStatMorph = d.statMorphList.begin();
	while (true) {
		StatMorphIter kEnd = statMorphList.end();
		if (kStatMorphIt == kEnd) {
			break;
		}
		const FffTriStatMorphS& rSrcLabel = *kSrcStatMorph;
		FffTriStatMorphS& rDstLabel = *kStatMorphIt;
		rDstLabel.label.assign(rSrcLabel.label, 0, std::string::npos);
		const FffTriStatMorphS& rSrcOffset = *kSrcStatMorph;
		FffTriStatMorphS& rDstOffset = *kStatMorphIt;
		rDstOffset.firstVtxOffset = rSrcOffset.firstVtxOffset;
		uiCount = (*kSrcStatMorph).vtxIdxList.size();
		(*kStatMorphIt).vtxIdxList.resize(uiCount, 0);
		uiCount = (*kStatMorphIt).vtxIdxList.size();
		const uint32_t* pSrcIdx = &(*kSrcStatMorph).vtxIdxList[0];
		uint32_t* pDstIdx = &(*kStatMorphIt).vtxIdxList[0];
		rt::memcpy(pDstIdx, pSrcIdx, uiCount * sizeof(uint32_t));
		++kStatMorphIt;
		++kSrcStatMorph;
	}
}

// Reads the .tri file fname into triData: the header, then each list resized to the header's count and read whole;
// the labels' and morphs' names go through a byte vector into their strings, then the diff morphs' scaled deltas and
// the static morphs' index lists. The first failed read closes the file and returns false; at the end the static
// morphs' vertices must add up to the header's count, else the error is recorded with the file name.
// The file object is raw stack storage the exe's constructor (0xAC5490) builds. The by-value arguments the exe builds
// in place in its outgoing argument area (the file ID, the file name, the default list elements, the error text) are
// built in locals here and copied; their callees own and destroy them, as in the exe. The exe's cleanup-only EH frame
// is left out.
// The asm decides two quirks: the vertex and surface label loops copy only the name's last byte (an empty loop, then
// one store at index len - 1; 0xAB668B..0xAB6791 and the surface labels' copy), and the static morph index read's
// failure and the two final returns call the destructor out of line (0xAAAB30), the other failures inline it.
bool fffReadTriFile(const std::string& fname, FffTriFileDataS& triData) {
	using CharVector = std::vector<char, std::allocator<char>>;
	FffTriHeaderS header;
	// EXE-INLINED none: FffTriHeaderS::FffTriHeaderS
	std::memset(&header, 0, sizeof(header));
	alignas(BSFaceGenBinaryFile) uint8_t aucFile[sizeof(BSFaceGenBinaryFile)];
	BSFaceGenBinaryFile* pFile = static_cast<BSFaceGenBinaryFile*>(static_cast<void*>(aucFile));
	std::string kFileId;
	FNVD_RECONSTRUCT(std::string)(&kFileId, "FRTRI003");
	::new (aucFile) BSFaceGenBinaryFile(kFileId);
	// The early returns' destructor (the exe inlines it here).
	auto fnFail = [pFile]() -> bool {
		pFile->~BSFaceGenBinaryFile();
		return false;
	};
	std::string kFileName;
	// The by-value file name's copy constructor.
	FNVD_RECONSTRUCT_AS(CopyConstruct, std::string)(&kFileName, fname);
	if (!pFile->BSFaceGenBinaryFile::openForRead(kFileName, false)) {
		return fnFail();
	}
	if (!pFile->readHeader(&header, 1, sizeof(header))) {
		return fnFail();
	}
	header.fix();

	triData.vtxList.resize(header.numVtx + header.numStatMorphVtx, FR2Vect3TC<float>{0.0f, 0.0f, 0.0f});
	triData.triList.resize(header.numTri, FR2Vect3TC<int>{0, 0, 0});
	triData.quadList.resize(header.numQuad, FutVect4TC<int>{0, 0, 0, 0});
	triData.vtxLabelList.resize(header.numVtxLabel);
	FffTriSurfLabelS kSurfLabel;
	kSurfLabel.~FffTriSurfLabelS();
	::new (&kSurfLabel) FffTriSurfLabelS;
	triData.surfLabelList.resize(header.numSurfLabel, kSurfLabel);
	FffTriDiffMorphS kDiffMorph;
	kDiffMorph.~FffTriDiffMorphS();
	::new (&kDiffMorph) FffTriDiffMorphS;
	triData.diffMorphList.resize(header.numDiffMorphs, kDiffMorph);
	FffTriStatMorphS kStatMorph;
	kStatMorph.~FffTriStatMorphS();
	::new (&kStatMorph) FffTriStatMorphS;
	triData.statMorphList.resize(header.numStatMorphs, kStatMorph);

	if (header.numVtx + header.numStatMorphVtx != 0) {
		if (!pFile->BSFaceGenBinaryFile::read(&triData.vtxList[0], sizeof(FR2Vect3TC<float>),
				header.numVtx + header.numStatMorphVtx)) {
			return fnFail();
		}
	}
	if (header.numTri != 0) {
		if (!pFile->BSFaceGenBinaryFile::read(&triData.triList[0], sizeof(FR2Vect3TC<int>), header.numTri)) {
			return fnFail();
		}
	}
	if (header.numQuad != 0) {
		if (!pFile->BSFaceGenBinaryFile::read(&triData.quadList[0], sizeof(FutVect4TC<int>), header.numQuad)) {
			return fnFail();
		}
	}

	for (uint32_t i = 0; i < header.numVtxLabel; ++i) {
		if (!pFile->BSFaceGenBinaryFile::read(&triData.vtxLabelList[i].idx, sizeof(uint32_t), 1)) {
			return fnFail();
		}
		uint32_t uiLen;
		if (!pFile->BSFaceGenBinaryFile::read(&uiLen, sizeof(uiLen), 1)) {
			return fnFail();
		}
		if (uiLen != 0) {
			CharVector kChars;
			kChars.~CharVector();
			::new (&kChars) CharVector(uiLen);
			if (!pFile->BSFaceGenBinaryFile::read(&kChars[0], 1, uiLen)) {
				return fnFail();
			}
			if (uiLen <= 1) {
				std::string kEmpty;
				FNVD_RECONSTRUCT(std::string)(&kEmpty, "");
				// EXE-INLINED none: basic_string::operator=
				triData.vtxLabelList[i].label.assign(kEmpty, 0, std::string::npos);
			} else {
				triData.vtxLabelList[i].label.resize(uiLen - 1, '\0');
				uint32_t j;
				for (j = 0; j < uiLen - 1; ++j) {
				}
				char* pcSrc = &kChars[j];
				char& rcDest = triData.vtxLabelList[i].label[j];
				rcDest = *pcSrc;
			}
		}
	}

	for (uint32_t i = 0; i < header.numSurfLabel; ++i) {
		if (!pFile->BSFaceGenBinaryFile::read(&triData.surfLabelList[i].idx, sizeof(uint32_t), 1)) {
			return fnFail();
		}
		if (!pFile->BSFaceGenBinaryFile::read(triData.surfLabelList[i].vtxIntWgt, sizeof(FffTriSurfLabelS::vtxIntWgt),
				1)) {
			return fnFail();
		}
		uint32_t uiLen;
		if (!pFile->BSFaceGenBinaryFile::read(&uiLen, sizeof(uiLen), 1)) {
			return fnFail();
		}
		if (uiLen != 0) {
			CharVector kChars;
			kChars.~CharVector();
			::new (&kChars) CharVector(uiLen);
			if (!pFile->BSFaceGenBinaryFile::read(&kChars[0], 1, uiLen)) {
				return fnFail();
			}
			if (uiLen <= 1) {
				std::string kEmpty;
				FNVD_RECONSTRUCT(std::string)(&kEmpty, "");
				// EXE-INLINED none: basic_string::operator=
				triData.surfLabelList[i].label.assign(kEmpty, 0, std::string::npos);
			} else {
				triData.surfLabelList[i].label.resize(uiLen - 1, '\0');
				uint32_t k;
				for (k = 0; k < uiLen - 1; ++k) {
				}
				char* pcSrc = &kChars[k];
				char& rcDest = triData.surfLabelList[i].label[k];
				rcDest = *pcSrc;
			}
		}
	}

	// extraInfo's bit 0: the file carries texture coordinates, per texture point or else per vertex.
	if ((header.extraInfo & 1) != 0) {
		if (header.numTexPts != 0) {
			triData.txtData.resize(header.numTexPts, FR2Vect2TC<float>{0.0f, 0.0f});
			if (header.numTexPts != 0) {
				if (!pFile->BSFaceGenBinaryFile::read(&triData.txtData[0], sizeof(FR2Vect2TC<float>),
						header.numTexPts)) {
					return fnFail();
				}
			}
			triData.txtTriList.resize(header.numTri, FR2Vect3TC<int>{0, 0, 0});
			if (header.numTri != 0) {
				if (!pFile->BSFaceGenBinaryFile::read(&triData.txtTriList[0], sizeof(FR2Vect3TC<int>), header.numTri)) {
					return fnFail();
				}
			}
			triData.txtQuadList.resize(header.numQuad, FutVect4TC<int>{0, 0, 0, 0});
			if (header.numQuad != 0) {
				if (!pFile->BSFaceGenBinaryFile::read(&triData.txtQuadList[0], sizeof(FutVect4TC<int>),
						header.numQuad)) {
					return fnFail();
				}
			}
		} else {
			triData.txtData.resize(header.numVtx, FR2Vect2TC<float>{0.0f, 0.0f});
			if (header.numVtx != 0) {
				if (!pFile->BSFaceGenBinaryFile::read(&triData.txtData[0], sizeof(FR2Vect2TC<float>), header.numVtx)) {
					return fnFail();
				}
			}
			triData.txtTriList.clear();
			triData.txtQuadList.clear();
		}
	}

	for (uint32_t i = 0; i < header.numDiffMorphs; ++i) {
		uint32_t uiLen;
		if (!pFile->BSFaceGenBinaryFile::read(&uiLen, sizeof(uiLen), 1)) {
			return fnFail();
		}
		if (uiLen != 0) {
			CharVector kChars;
			kChars.~CharVector();
			::new (&kChars) CharVector(uiLen);
			if (!pFile->BSFaceGenBinaryFile::read(&kChars[0], 1, uiLen)) {
				return fnFail();
			}
			if (uiLen <= 1) {
				std::string kEmpty;
				FNVD_RECONSTRUCT(std::string)(&kEmpty, "");
				// EXE-INLINED none: basic_string::operator=
				triData.diffMorphList[i].label.assign(kEmpty, 0, std::string::npos);
			} else {
				triData.diffMorphList[i].label.resize(uiLen - 1, '\0');
				for (uint32_t m = 0; m < uiLen - 1; ++m) {
					char* pcSrc = &kChars[m];
					char& rcDest = triData.diffMorphList[i].label[m];
					rcDest = *pcSrc;
				}
			}
		}
		float fScale;
		if (!pFile->BSFaceGenBinaryFile::read(&fScale, sizeof(fScale), 1)) {
			return fnFail();
		}
		triData.diffMorphList[i].vtxDelta.resize(header.numVtx, FR2Vect3TC<float>{0.0f, 0.0f, 0.0f});
		for (uint32_t n = 0; n < header.numVtx; ++n) {
			int16_t asDelta[3];
			if (!pFile->BSFaceGenBinaryFile::read(asDelta, sizeof(int16_t), 3)) {
				return fnFail();
			}
			// The exe multiplies with fimul on the int16_t widened to int32_t and rounds each product to a float
			// (fstp dword), z first; the product of a float and a 16-bit integer is exact before that rounding, so
			// the float multiply here gives the same values.
			FR2Vect3TC<float> kDelta;
			kDelta.x3 = fScale * asDelta[2];
			kDelta.x2 = fScale * asDelta[1];
			kDelta.x1 = fScale * asDelta[0];
			triData.diffMorphList[i].vtxDelta[n] = kDelta;
		}
	}

	uint32_t uiFirstVtx = header.numVtx;
	for (uint32_t i = 0; i < header.numStatMorphs; ++i) {
		// One word for the name's length, then the index count, as in the exe.
		uint32_t uiCount;
		if (!pFile->BSFaceGenBinaryFile::read(&uiCount, sizeof(uiCount), 1)) {
			return fnFail();
		}
		if (uiCount != 0) {
			CharVector kChars;
			kChars.~CharVector();
			::new (&kChars) CharVector(uiCount);
			if (!pFile->BSFaceGenBinaryFile::read(&kChars[0], 1, uiCount)) {
				return fnFail();
			}
			if (uiCount <= 1) {
				std::string kEmpty;
				FNVD_RECONSTRUCT(std::string)(&kEmpty, "");
				// EXE-INLINED none: basic_string::operator=
				triData.statMorphList[i].label.assign(kEmpty, 0, std::string::npos);
			} else {
				triData.statMorphList[i].label.resize(uiCount - 1, '\0');
				for (uint32_t ii = 0; ii < uiCount - 1; ++ii) {
					char* pcSrc = &kChars[ii];
					char& rcDest = triData.statMorphList[i].label[ii];
					rcDest = *pcSrc;
				}
			}
		}
		triData.statMorphList[i].firstVtxOffset = uiFirstVtx;
		if (!pFile->BSFaceGenBinaryFile::read(&uiCount, sizeof(uiCount), 1)) {
			return fnFail();
		}
		triData.statMorphList[i].vtxIdxList.resize(uiCount, 0);
		if (uiCount != 0) {
			const UIntVector& rIndices = triData.statMorphList[i].vtxIdxList;
			if (!pFile->BSFaceGenBinaryFile::read(const_cast<uint32_t*>(&rIndices[0]), sizeof(uint32_t), uiCount)) {
				pFile->~BSFaceGenBinaryFile();
				return false;
			}
		}
		uiFirstVtx += uiCount;
	}

	if (uiFirstVtx == header.numVtx + header.numStatMorphVtx) {
		pFile->~BSFaceGenBinaryFile();
		return true;
	}
	std::string kText;
	// The string's copy: natively the default-constructed local is assigned.
	kText = fname;
	futSetLastError(FUTERR_FILE_CORRUPT, kText);
	pFile->~BSFaceGenBinaryFile();
	return false;
}

// Writes triData to the .tri file fname: a FutBinaryFileC with the ID "FRTRI003", opened for overwrite, then the
// open file's overload (0xAB7F40); false when the file does not open. No exe caller (cdecl, ret 0). The by-value
// strings the exe builds in place in its outgoing argument area (the file ID, the file name) are built in locals here
// and copied; their callees own and destroy them, as in the exe. The exe's cleanup-only EH frame is left out.
bool fffWriteTriFile(const std::string& fname, const FffTriFileDataS& triData) {
	std::string kFileId;
	FNVD_RECONSTRUCT(std::string)(&kFileId, "FRTRI003");
	FutBinaryFileC kFile(kFileId);
	std::string kFileName;
	// The by-value file name's copy constructor.
	FNVD_RECONSTRUCT_AS(CopyConstruct, std::string)(&kFileName, fname);
	if (!kFile.FutBinaryFileC::openForOverwrite(kFileName)) {
		return false;
	}
	return fffWriteTriFile(kFile, triData);
}

// Writes triData to the open file: the header (the counts, with extraInfo's bit 0 set when the texture lists match
// the model's vertices or its faces), the vertex, triangle and quad lists, the labels and morphs with their names
// as length-prefixed, zero-terminated byte strings, the texture lists, the diff morphs' deltas as int16_t scaled to
// the largest component, and the static morphs' index lists. The first failed write returns false; at the end the
// static morphs' indices must add up to the header's static morph vertices, else error 11 is recorded.
// The asm decides: every element access re-indexes its vector with VC9's inline checked operator[] (the diff morphs'
// through an inline idiv 0x34, not their out-of-line size 0x662840), fabs and floor are the CRT's double functions
// called out of line, the scale is stored as a float, and each name's byte vector is destroyed with the base
// destructor called out of line (0xABC120) for the labels and after a failed name write, inlined to its Deallocate
// elsewhere. The exe's cleanup-only EH frame is left out.
bool fffWriteTriFile(FutBinaryFileC& file, const FffTriFileDataS& triData) {
	using CharVector = std::vector<char, std::allocator<char>>;
	// A name's characters into rChars, each read through the element fnLabel indexes again, then the terminator.
	auto fnFillName = [](CharVector& rChars, uint32_t auiLen, auto&& fnLabel) {
		for (uint32_t j = 0; j < auiLen - 1; ++j) {
			const char* pcSrc = &fnLabel()[j];
			char& rcDest = rChars[j];
			rcDest = *pcSrc;
		}
		rChars[auiLen - 1] = '\0';
	};

	FffTriHeaderS header;
	std::memset(&header, 0, sizeof(header));
	header.numVtx = triData.numModelVtx();
	header.numTri = triData.triList.size();
	header.numQuad = triData.quadList.size();
	header.numVtxLabel = triData.vtxLabelList.size();
	header.numSurfLabel = triData.surfLabelList.size();
	header.numDiffMorphs = triData.diffMorphList.size();
	header.numStatMorphs = triData.statMorphList.size();
	uint32_t uiVtxCount = triData.vtxList.size();
	header.numStatMorphVtx = uiVtxCount - triData.numModelVtx();
	// Texture coordinates per vertex (no texture faces), per texture point (the faces match the model's), or none.
	if (triData.txtData.size() == header.numVtx && triData.txtTriList.size() == 0 && triData.txtQuadList.size() == 0) {
		header.extraInfo |= 1;
		header.numTexPts = 0;
	} else if (triData.txtData.size() != 0 && triData.txtTriList.size() == header.numTri &&
		triData.txtQuadList.size() == header.numQuad) {
		header.extraInfo |= 1;
		header.numTexPts = triData.txtData.size();
	} else {
		header.extraInfo &= ~1u;
		header.numTexPts = 0;
	}
	if (!file.writeHeader(&header, sizeof(header), 1)) {
		return false;
	}

	if (header.numVtx + header.numStatMorphVtx != 0) {
		if (!file.write(&triData.vtxList[0], sizeof(FR2Vect3TC<float>), header.numVtx + header.numStatMorphVtx)) {
			return false;
		}
	}
	if (header.numTri != 0) {
		if (!file.write(&triData.triList[0], sizeof(FR2Vect3TC<int>), header.numTri)) {
			return false;
		}
	}
	if (header.numQuad != 0) {
		if (!file.write(&triData.quadList[0], sizeof(FutVect4TC<int>), header.numQuad)) {
			return false;
		}
	}

	for (uint32_t i = 0; i < header.numVtxLabel; ++i) {
		if (!file.write(&triData.vtxLabelList[i].idx, sizeof(uint32_t), 1)) {
			return false;
		}
		uint32_t uiLen = triData.vtxLabelList[i].label.size() + 1;
		if (!file.write(&uiLen, sizeof(uiLen), 1)) {
			return false;
		}
		CharVector kChars;
		kChars.~CharVector();
		::new (&kChars) CharVector(uiLen);
		fnFillName(kChars, uiLen, [&]() -> const std::string& { return triData.vtxLabelList[i].label; });
		if (!file.write(&kChars[0], 1, uiLen)) {
			return false;
		}
	}

	for (uint32_t i = 0; i < header.numSurfLabel; ++i) {
		if (!file.write(&triData.surfLabelList[i].idx, sizeof(uint32_t), 1)) {
			return false;
		}
		if (!file.write(triData.surfLabelList[i].vtxIntWgt, sizeof(FffTriSurfLabelS::vtxIntWgt), 1)) {
			return false;
		}
		uint32_t uiLen = triData.surfLabelList[i].label.size() + 1;
		if (!file.write(&uiLen, sizeof(uiLen), 1)) {
			return false;
		}
		CharVector kChars;
		kChars.~CharVector();
		::new (&kChars) CharVector(uiLen);
		fnFillName(kChars, uiLen, [&]() -> const std::string& { return triData.surfLabelList[i].label; });
		if (!file.write(&kChars[0], 1, uiLen)) {
			return false;
		}
	}

	if ((header.extraInfo & 1) != 0) {
		if (header.numTexPts == 0) {
			if (!file.write(&triData.txtData[0], sizeof(FR2Vect2TC<float>), header.numVtx)) {
				return false;
			}
		} else {
			if (!file.write(&triData.txtData[0], sizeof(FR2Vect2TC<float>), header.numTexPts)) {
				return false;
			}
			if (!file.write(&triData.txtTriList[0], sizeof(FR2Vect3TC<int>), header.numTri)) {
				return false;
			}
			if (!file.write(&triData.txtQuadList[0], sizeof(FutVect4TC<int>), header.numQuad)) {
				return false;
			}
		}
	}

	for (uint32_t i = 0; i < header.numDiffMorphs; ++i) {
		uint32_t uiLen = triData.diffMorphList[i].label.size() + 1;
		if (!file.write(&uiLen, sizeof(uiLen), 1)) {
			return false;
		}
		CharVector kChars;
		kChars.~CharVector();
		::new (&kChars) CharVector(uiLen);
		fnFillName(kChars, uiLen, [&]() -> const std::string& { return triData.diffMorphList[i].label; });
		if (!file.write(&kChars[0], 1, uiLen)) {
			return false;
		}
		// The largest absolute delta component; each is rounded to float (fstp dword) before the ordered compare
		// (fcompp / test ah, 0x41: NaN never replaces it), and read again for the store.
		float fScale = 0.0f;
		// The exe calls the double _fabs (0xEC6CDE), not the float overload (0x408840): the component is widened.
		for (uint32_t n = 0; n < header.numVtx; ++n) {
			float fAbs = static_cast<float>(std::fabs(static_cast<double>(triData.diffMorphList[i].vtxDelta[n].x1)));
			if (fAbs > fScale) {
				fScale = static_cast<float>(std::fabs(static_cast<double>(triData.diffMorphList[i].vtxDelta[n].x1)));
			}
			fAbs = static_cast<float>(std::fabs(static_cast<double>(triData.diffMorphList[i].vtxDelta[n].x2)));
			if (fAbs > fScale) {
				fScale = static_cast<float>(std::fabs(static_cast<double>(triData.diffMorphList[i].vtxDelta[n].x2)));
			}
			fAbs = static_cast<float>(std::fabs(static_cast<double>(triData.diffMorphList[i].vtxDelta[n].x3)));
			if (fAbs > fScale) {
				fScale = static_cast<float>(std::fabs(static_cast<double>(triData.diffMorphList[i].vtxDelta[n].x3)));
			}
		}
		// The double constant 32767.0.
		fScale = static_cast<float>(fScale / 32767.0);
		if (!file.write(&fScale, sizeof(fScale), 1)) {
			return false;
		}
		fScale = 1.0f / fScale;
		for (uint32_t n = 0; n < header.numVtx; ++n) {
			// Each component times the scale (exact in double) plus the double 0.5, floored, then
			// truncated by _ftol2_sse, of which the low 16 bits are stored.
			int16_t asDelta[3];
			asDelta[0] = static_cast<int16_t>(static_cast<
				int64_t>(std::floor(static_cast<double>(triData.diffMorphList[i].vtxDelta[n].x1) * fScale + 0.5)));
			asDelta[1] = static_cast<int16_t>(static_cast<
				int64_t>(std::floor(static_cast<double>(triData.diffMorphList[i].vtxDelta[n].x2) * fScale + 0.5)));
			asDelta[2] = static_cast<int16_t>(static_cast<
				int64_t>(std::floor(static_cast<double>(triData.diffMorphList[i].vtxDelta[n].x3) * fScale + 0.5)));
			if (!file.write(asDelta, sizeof(int16_t), 3)) {
				return false;
			}
		}
	}

	uint32_t uiStatMorphVtx = 0;
	for (uint32_t i = 0; i < header.numStatMorphs; ++i) {
		uint32_t uiLen = triData.statMorphList[i].label.size() + 1;
		if (!file.write(&uiLen, sizeof(uiLen), 1)) {
			return false;
		}
		CharVector kChars;
		kChars.~CharVector();
		::new (&kChars) CharVector(uiLen);
		fnFillName(kChars, uiLen, [&]() -> const std::string& { return triData.statMorphList[i].label; });
		if (!file.write(&kChars[0], 1, uiLen)) {
			return false;
		}
		// The exe reuses the length's stack word for the index count.
		uiLen = triData.statMorphList[i].vtxIdxList.size();
		if (!file.write(&uiLen, sizeof(uiLen), 1)) {
			return false;
		}
		if (uiLen != 0) {
			if (!file.write(&triData.statMorphList[i].vtxIdxList[0], sizeof(uint32_t), uiLen)) {
				return false;
			}
		}
		uiStatMorphVtx += uiLen;
	}

	if (uiStatMorphVtx == header.numStatMorphVtx) {
		return true;
	}
	std::string kText;
	FNVD_RECONSTRUCT(std::string)(&kText, "Invalid morph data during TRI file save.");
	futSetLastError(FUTERR_SEE_TEXT, kText);
	return false;
}

FffTriVtxLabelS::FffTriVtxLabelS(const FffTriVtxLabelS& _Right) : idx(_Right.idx), label(_Right.label) {}

FffTriSurfLabelS::FffTriSurfLabelS(const FffTriSurfLabelS& _Right)
	: idx(_Right.idx), vtxIntWgt{_Right.vtxIntWgt[0], _Right.vtxIntWgt[1], _Right.vtxIntWgt[2]}, label(_Right.label) {}

// idx is left as it is.
FffTriSurfLabelS::FffTriSurfLabelS() : vtxIntWgt{0.0f, 0.0f, 0.0f}, label() {}

void FffTriSurfLabelS::operator delete(void* apMemory) {
	ourFree(apMemory);
}

FffTriDiffMorphS::FffTriDiffMorphS() : label(), vtxDelta() {}

// firstVtxOffset is left as it is.
FffTriStatMorphS::FffTriStatMorphS() : label(), vtxIdxList() {}

FffTriDiffMorphS::FffTriDiffMorphS(const FffTriDiffMorphS& _Right) : label(_Right.label), vtxDelta(_Right.vtxDelta) {}

FffTriStatMorphS::FffTriStatMorphS(const FffTriStatMorphS& _Right)
	: label(_Right.label), firstVtxOffset(_Right.firstVtxOffset), vtxIdxList(_Right.vtxIdxList) {}

// Each surface label's point: its face's three vertices weighted by the label's vtxIntWgt and summed. A face index
// past the triangles names half of a quad (the PDB's signed half of the index past them, tested signed against the
// quad count); odd halves take the quad's x1, x4, x3, even ones x3, x2, x1. Every element access is VC9's inline
// checked operator[], each re-reading its vector's size with an idiv of the byte span (size()).
void FffTriFileDataS::extractSurfLblPts(std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>& ptList) {
	FR2Vect3TC<float> kSum;
	FR2Vect3TC<float> kWeighted2;
	FR2Vect3TC<float> kPartialSum;
	FR2Vect3TC<float> kWeighted1;
	FR2Vect3TC<float> kWeighted0;
	ptList.clear();
	uint32_t uiNumTris = triList.size();
	uint32_t uiNumFaces = uiNumTris + 2 * quadList.size();
	for (uint32_t i = 0; i < surfLabelList.size(); ++i) {
		uint32_t uiIdx = surfLabelList[i].idx;
		if (uiIdx >= uiNumFaces) {
			continue;
		}
		uint32_t uiVtx0 = 0;
		uint32_t uiVtx1 = 0;
		uint32_t uiVtx2 = 0;
		if (uiIdx >= uiNumTris) {
			uint32_t uiQuadFace = uiIdx - uiNumTris;
			int32_t iQuad = static_cast<int32_t>(uiQuadFace) >> 1;
			if (iQuad >= static_cast<int32_t>(quadList.size())) {
				continue;
			}
			const FutVect4TC<int32_t>& rQuadA = quadList[static_cast<uint32_t>(iQuad)];
			const FutVect4TC<int32_t>& rQuadB = quadList[static_cast<uint32_t>(iQuad)];
			const FutVect4TC<int32_t>& rQuadC = quadList[static_cast<uint32_t>(iQuad)];
			if ((uiQuadFace & 1) != 0) {
				uiVtx0 = rQuadC.x3;
				uiVtx1 = rQuadB.x4;
				uiVtx2 = rQuadA.x1;
			} else {
				uiVtx0 = rQuadC.x1;
				uiVtx1 = rQuadB.x2;
				uiVtx2 = rQuadA.x3;
			}
		} else {
			const FR2Vect3TC<int32_t>& rTri = triList[uiIdx];
			uiVtx0 = rTri.x1;
			uiVtx1 = rTri.x2;
			uiVtx2 = rTri.x3;
		}
		float* pfWeights = surfLabelList[i].vtxIntWgt;
		FR2Vect3TC<float>& rVtx2 = vtxList[uiVtx2];
		FR2Vect3TC<float>& rVtx1 = vtxList[uiVtx1];
		FR2Vect3TC<float>& rVtx0 = vtxList[uiVtx0];
		FR2Vect3TC<float>* pWeighted2 = rVtx2.MulScalar(&kWeighted2, pfWeights[2]);
		FR2Vect3TC<float>* pWeighted1 = rVtx1.MulScalar(&kWeighted1, pfWeights[1]);
		FR2Vect3TC<float>* pWeighted0 = rVtx0.MulScalar(&kWeighted0, pfWeights[0]);
		FR2Vect3TC<float>* pPartialSum = pWeighted0->Add(&kPartialSum, *pWeighted1);
		FR2Vect3TC<float>* pSum = pPartialSum->Add(&kSum, *pWeighted2);
		ptList.push_back(*pSum);
	}
}
void FffTriFileDataS::extractVtxLblPts(std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>& ptList) {
	ptList.clear();
	for (uint32_t i = 0; i < vtxLabelList.size(); ++i) {
		if (vtxLabelList[i].idx < vtxList.size()) {
			ptList.push_back(vtxList[vtxLabelList[i].idx]);
		}
	}
}
FR2Vect3TC<float>* FR2Vect3TC<float>::Add(FR2Vect3TC<float>* apResult, const FR2Vect3TC<float>& v1) const {
	FR2Vect3TC<float> v4;
	v4.x1 = v1.x1 + x1;
	v4.x2 = v1.x2 + x2;
	v4.x3 = v1.x3 + x3;
	*apResult = v4;
	return apResult;
}

FR2Vect3TC<float>* FR2Vect3TC<float>::MulScalar(FR2Vect3TC<float>* apResult, float val) const {
	FR2Vect3TC<float> v4;
	v4.x1 = val * x1;
	v4.x2 = val * x2;
	v4.x3 = val * x3;
	*apResult = v4;
	return apResult;
}

// Whether a1 names a file whose id is "FRTRI003" (a FaceGen tri file). The exe builds the id string straight in
// FutBinaryFileC's by-value argument slot, which the constructor destroys; the dword it spills next to its byte
// result is that slot's address, kept for its EH frame. The exe's cleanup-only C++ EH frame is left out.
char fffIsTriFile(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1) {
	std::basic_string<char, std::char_traits<char>, std::allocator<char>> kFileId;
	kFileId.~basic_string<char, std::char_traits<char>, std::allocator<char>>();
	::new (&kFileId) std::basic_string<char, std::char_traits<char>, std::allocator<char>>("FRTRI003");
	FutBinaryFileC kFile(kFileId);
	char cResult = kFile.isCorrectFileId(*a1);
	return cResult;
}
