#include "SDK/FaceGen/binaryFile.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/ModelLoader.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "SDK/FaceGen/BSFaceGenBinaryFile.h"
#include "SDK/FaceGen/assert.hpp"
#include "SDK/FaceGen/lastError.hpp"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/iosfwd.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"
#include <vector>
#include <memory>
#include <string>

// The byte buffer the ID is read into: std::vector<char>, the FaceGen SDK's own element type.
using CharVector = std::vector<char, std::allocator<char>>;

void FutBinaryFileC::operator delete(void* apMemory) {
	MemoryManager::Instance().Deallocate(apMemory);
}

// The file's ID is the first fileId.size() bytes of the file; the sixth character (the file's version digit) is
// replaced by fileId's before the compare. The exe returns true when the IDs are equal (0xAC42AD: neg/sbb/add 1 makes
// eax 1 for a zero compare, and 0xAC42B9 jumps to the false block when it is 0). Its cleanup-only EH frame is left
// out. The byte vector's base destructor is inlined in the compare-equal branch and called out of line (0xABC120) in
// the compare-differs one.
bool FutBinaryFileC::isCorrectFileId(const std::string& aFname)
{
	const char* pcName = aFname.c_str();
	std::FILE* pStream = rt::fopen(pcName, "rb");
	bool bResult = false;
	if (pStream != 0) {
		CharVector buf(fileId.size() + 1);
		if (std::fread(&buf[0], fileId.size(), 1, pStream) == 1) {
			buf[fileId.size()] = 0;
			std::fclose(pStream);
			pStream = 0;
			buf[5] = fileId[5];
			bResult = fileId.compare(0, fileId.size(), &buf[0], std::strlen(&buf[0])) == 0;
			if (bResult) {
			} else {
			}
		} else {
			std::fclose(pStream);
			pStream = 0;
			std::string sMessage(aFname);
			futSetLastError(FUTERR_FILE_CORRUPT, sMessage);
		}
	} else {
		std::string sMessage(aFname);
		futSetLastError(FUTERR_FILE_NOT_READABLE, sMessage);
	}
	if (pStream != 0) {
		std::fclose(pStream);
	}
	return bResult;
}

// The by-value std::string parameter is 28 bytes of stack (ret 0x1C); the exe destroys it on every path, which src
// writes out because its hybrid string has no destructor. futSetLastError's message is a by-value copy the exe builds
// with the inlined copy constructor, and the callee destroys it. The byte vector's destructor is _Tidy and then the
// base destructor, which the exe calls out of line (0xABC120) at some sites and inlines to its
// MemoryManager::Deallocate at others. The exe's cleanup-only EH frame is left out.
bool FutBinaryFileC::openForRead(std::string aFname)
{
	close();
	const char* pcName = aFname.c_str();
	fptr = rt::fopen(pcName, "rb");
	if (fptr == 0) {
		std::string sMessage(aFname);
		futSetLastError(FUTERR_FILE_NOT_READABLE, sMessage);
		return false;
	}
	CharVector buf(fileId.size() + 1);
	if (!readHeader(&buf[0], fileId.size(), 1)) {
		return false;
	}
	CharVector copy;
	copy.~CharVector();
	::new (&copy) CharVector(buf);
	copy[5] = fileId[5];
	if (fileId.compare(0, fileId.size(), &copy[0], std::strlen(&copy[0])) != 0) {
		std::string sMessage(aFname);
		futSetLastError(FUTERR_FILE_INCORRECT_TYPE, sMessage);
		close();
		return false;
	}
	// The sixth character is the file's version digit: a later one is an error, an earlier or equal one is accepted.
	if (buf[5] > '0' && fileId[5] != buf[5]) {
		std::string sMessage(aFname);
		futSetLastError(FUTERR_FILE_NOT_READABLE, sMessage);
		close();
		return false;
	}
	fileId[5] = buf[5];
	m_fname.assign(aFname, 0, std::string::npos);
	return true;
}

// The same ID check as openForRead, on a file opened for update, with the sixth character reset to '0' and the name
// recorded when it matches; the name is a reference (one stack dword), so no parameter destructor. Both byte vectors'
// base destructors are inlined in the compare-differs branch. The exe's cleanup-only EH frame is left out.
bool FutBinaryFileC::openForUpdate(const std::string& aFname)
{
	close();
	const char* pcName = aFname.c_str();
	fptr = rt::fopen(pcName, "r+b");
	if (fptr == 0) {
		std::string sMessage(aFname);
		futSetLastError(FUTERR_FILE_NOT_READABLE, sMessage);
		return false;
	}
	CharVector buf(fileId.size() + 1);
	if (!readHeader(&buf[0], fileId.size(), 1)) {
		return false;
	}
	CharVector copy;
	copy.~CharVector();
	::new (&copy) CharVector(buf);
	copy[5] = fileId[5];
	if (fileId.compare(0, fileId.size(), &copy[0], std::strlen(&copy[0])) != 0) {
		std::string sMessage(aFname);
		futSetLastError(FUTERR_FILE_INCORRECT_TYPE, sMessage);
		close();
		return false;
	}
	fileId[5] = '0';
	m_fname.assign(aFname, 0, std::string::npos);
	return true;
}

// The same ID check as openForUpdate, on a file opened for append, without the sixth character's reset; only one byte
// vector, whose base destructor is inlined except in the compare-differs branch. The exe's cleanup-only EH frame is
// left out.
bool FutBinaryFileC::openForAppend(const std::string& aFname)
{
	close();
	const char* pcName = aFname.c_str();
	fptr = rt::fopen(pcName, "a+b");
	if (fptr == 0) {
		std::string sMessage(aFname);
		futSetLastError(FUTERR_FILE_NOT_READABLE, sMessage);
		return false;
	}
	CharVector buf(fileId.size() + 1);
	if (!readHeader(&buf[0], fileId.size(), 1)) {
		return false;
	}
	buf[5] = fileId[5];
	if (fileId.compare(0, fileId.size(), &buf[0], std::strlen(&buf[0])) != 0) {
		std::string sMessage(aFname);
		futSetLastError(FUTERR_FILE_INCORRECT_TYPE, sMessage);
		close();
		return false;
	}
	m_fname.assign(aFname, 0, std::string::npos);
	return true;
}

BSFaceGenBinaryFile::~BSFaceGenBinaryFile() {
	close();
}
bool FutBinaryFileC::write(const void* data, uint32_t size, uint32_t num)
{
	return writeHeader(data, size, num);
}

bool FutBinaryFileC::read(void* data, uint32_t size, uint32_t num)
{
	return readHeader(data, size, num);
}

BSFaceGenBinaryFile::BSFaceGenBinaryFile(std::string apString)
	: FutBinaryFileC(apString), pFile(nullptr), pLoadedFile(nullptr) {}

// FutBinaryFileC::openForRead's ID check on a file from the model loader's map or from FileFinder (meshes, or misc
// archives for a CTL file). The loader's file is held by a counted local (IncRefCount here, DecRefCount on every
// exit) and stored into pLoadedFile with a reference of its own, without releasing the old one. The by-value name is
// destroyed on every path (ret 0x20). The byte vectors' base destructors are called out of line (0xABC120) on the
// error paths and inlined to MemoryManager::Deallocate on success. The exe's cleanup-only C++ EH frame is left out.
bool BSFaceGenBinaryFile::openForRead(std::string apFileName, bool abCTLFile)
{
	uint32_t uiArchiveType = ARCHIVE_TYPE_MESHES;
	if (abCTLFile) {
		uiArchiveType = ARCHIVE_TYPE_MISC;
	}
	LoadedFile* pFileFromMap;
	if (pModelLoader) {
		pFileFromMap = pModelLoader->GetLoadedFileFromMap(apFileName.c_str());
	} else {
		pFileFromMap = 0;
	}
	if (pFileFromMap) {
		pFileFromMap->IncRefCount();
	}
	if (pFileFromMap) {
		pLoadedFile = pFileFromMap;
		pLoadedFile->IncRefCount();
		LoadedFile* pLoaded = pLoadedFile;
		pLoaded->bFileUsed = true;
		pFile = pLoaded->pFile;
	} else {
		const char* pcName = apFileName.c_str();
		pFile = FileFinder::GetFile(pcName, NiFile::READ_ONLY, 0x2800, uiArchiveType);
	}
	if (!pFile) {
		std::string sMessage(apFileName);
		futSetLastError(FUTERR_FILE_NOT_READABLE, sMessage);
		if (pFileFromMap) {
			pFileFromMap->DecRefCount();
		}
		return false;
	}
	CharVector buf(fileId.size() + 1);
	if (!readHeader(&buf[0], fileId.size(), 1)) {
		if (pFileFromMap) {
			pFileFromMap->DecRefCount();
		}
		return false;
	}
	CharVector copy;
	copy.~CharVector();
	::new (&copy) CharVector(buf);
	copy[5] = fileId[5];
	// The exe range-checks &copy[0] once and passes that pointer to both strlen and compare.
	const char* pcCopy = &copy[0];
	if (fileId.compare(0, fileId.size(), pcCopy, std::strlen(pcCopy)) != 0) {
		std::string sMessage(apFileName);
		futSetLastError(FUTERR_FILE_INCORRECT_TYPE, sMessage);
		close();
		if (pFileFromMap) {
			pFileFromMap->DecRefCount();
		}
		return false;
	}
	// The sixth character is the file's version digit: a later one is an error, an earlier or equal one is accepted.
	if (buf[5] > '0' && fileId[5] != buf[5]) {
		std::string sMessage(apFileName);
		futSetLastError(FUTERR_FILE_NOT_READABLE, sMessage);
		close();
		if (pFileFromMap) {
			pFileFromMap->DecRefCount();
		}
		return false;
	}
	fileId[5] = buf[5];
	m_fname.assign(apFileName, 0, std::string::npos);
	bool bResult = pFile != 0;
	if (pFileFromMap) {
		pFileFromMap->DecRefCount();
	}
	return bResult;
}

// Called directly by fffReadTriFile on its own file object.
bool BSFaceGenBinaryFile::read(void* data, uint32_t size, uint32_t num) {
	int32_t readSize;
	if (!this->pFile) {
		return 0;
	}
	uint32_t uiBytes = num * size;
	if (size && (size <= 2 || size == 4)) {
		BSFile* pFile = static_cast<BSFile*>(this->pFile);
		int32_t v5 = pFile->m_pfnRead(pFile, data, uiBytes, &size, 1);
		pFile->m_uiAbsoluteCurrentPos += v5;
		readSize = v5;
	} else {
		if (size % 4) {
			fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\binaryFile.cpp", 508);
			size = 1;
		} else {
			size = 4;
		}
		BSFile* v8 = static_cast<BSFile*>(this->pFile);
		int32_t v6 = v8->m_pfnRead(v8, data, uiBytes, &size, 1);
		v8->m_uiAbsoluteCurrentPos += v6;
		readSize = v6;
	}
	return uiBytes == readSize;
}

// Reads num items of size bytes through the virtual read as size * num one-byte items; on failure records
// FUTERR_FILE_CORRUPT with a by-value copy of m_fname and closes the file.
bool BSFaceGenBinaryFile::readHeader(void* data, uint32_t size, uint32_t num)
{
	if (!pFile) {
		return false;
	}
	if (read(data, 1, size * num)) {
		return true;
	}
	std::string sMessage(m_fname);
	futSetLastError(FUTERR_FILE_CORRUPT, sMessage);
	close();
	return false;
}

// The file ID must have 8 characters; its sixth is set to '0'. Both strings start empty, fileId is assigned the
// ID, and the by-value ID is destroyed when the constructor returns, as Construct does.
FutBinaryFileC::FutBinaryFileC(std::basic_string<char, std::char_traits<char>, std::allocator<char>> a_fileId)
	: fptr(nullptr) {
	fileId.assign(a_fileId, 0, std::string::npos);
	if (fileId.size() != 8) {
		fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\binaryFile.cpp", 34);
	}
	fileId[5] = '0';
}

void FutBinaryFileC::changeFileId(const std::basic_string<char, std::char_traits<char>, std::allocator<char>>& newFileId) {
	if (newFileId.size() != 8) {
		fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\binaryFile.cpp", 48);
	}
	if (fptr) {
		fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\binaryFile.cpp", 49);
	}
	fileId.assign(newFileId, 0, std::string::npos);
	fileId[5] = '0';
}

// Creates fname and writes the file ID as its header. The by-value string parameter is destroyed with _Tidy(true, 0)
// on every path, as in openForRead. The exe's cleanup-only C++ EH frame is left out.
bool FutBinaryFileC::openForOverwrite(std::string aFname) {
	close();
	const char* pcName = aFname.c_str();
	fptr = rt::fopen(pcName, "wb");
	if (fptr == 0) {
		std::string sMessage(aFname);
		futSetLastError(FUTERR_FILE_NOT_WRITEABLE, sMessage);
		return false;
	}
	uint32_t uiSize = fileId.size();
	const char* pcId = fileId.c_str();
	if (!writeHeader(pcId, uiSize, 1)) {
		std::string sMessage(aFname);
		futSetLastError(FUTERR_FILE_NOT_WRITEABLE, sMessage);
		close();
		return false;
	}
	m_fname.assign(aFname, 0, std::string::npos);
	return true;
}

bool FutBinaryFileC::seek(int32_t offsetFromBeginning) {
	if (!fptr) {
		return false;
	}
	if (!std::fseek(fptr, offsetFromBeginning, 0)) {
		return true;
	}
	std::string sMessage(m_fname);
	futSetLastError(FUTERR_FILE_SEEK_ERROR, sMessage);
	close();
	return false;
}

// Reads a2 two-byte items into a1 through BSFile::ReadF (vtable slot 17) and adds the bytes read to the stream's
// position; whether all of them were read.
bool BSFaceGenBinaryFile::readShorts(void* a1, int32_t a2)
{
	if (!pFile) {
		return false;
	}
	int32_t iBytes = 2 * a2;
	int32_t iRead = static_cast<BSFile*>(pFile)->ReadF(a1, iBytes);
	pFile->m_uiAbsoluteCurrentPos += iRead;
	return iBytes == iRead;
}

bool BSFaceGenBinaryFile::seek(int32_t offsetFromBeginning) {
	if (!pFile) {
		return 0;
	}
	pFile->Seek(offsetFromBeginning, NiFile::ms_iSeekSet);
	return 1;
}

// Closes the file; C++ then frees the two strings, as Destruct does.
FutBinaryFileC::~FutBinaryFileC() {
	close();
}

bool FutBinaryFileC::writeHeader(const void* data, uint32_t size, uint32_t num) {
	if (!fptr) {
		return false;
	}
	if (std::fwrite(data, size, num, fptr) == num) {
		return true;
	}
	std::string sMessage(m_fname);
	futSetLastError(FUTERR_FILE_WRITE_ERROR, sMessage);
	close();
	return false;
}

bool FutBinaryFileC::readHeader(void* data, uint32_t size, uint32_t num) {
	if (!fptr) {
		return false;
	}
	if (std::fread(data, size, num, fptr) == num) {
		return true;
	}
	std::string sMessage(m_fname);
	futSetLastError(FUTERR_FILE_CORRUPT, sMessage);
	close();
	return false;
}

void FutBinaryFileC::close() {
	if (fptr) {
		std::fclose(fptr);
	}
	unsigned int SourceSize = std::strlen("");
	m_fname.assign("", SourceSize);
	fptr = 0;
}

void BSFaceGenBinaryFile::close() {
	if (pLoadedFile) {
		pFile = 0;
		pLoadedFile->DecRefCount();
		pLoadedFile = 0;
	} else {
		if (pFile) {
			delete pFile;
		}
		pFile = 0;
	}
}

void CloseFile(std::FILE** apThis) {
	if (*apThis) {
		std::fclose(*apThis);
	}
}
