#pragma once

#include <memory>
#include <string>
#include "SDK/MSVC/iosfwd.h"

// The standalone build's std::FILE is the system's FILE, named by a using-declaration.
#include "SDK/MSVC/stdio.h"

// FaceGen's binary file: a C stdio file with the name it was opened under and an ID.
class FutBinaryFileC {
public:
    virtual ~FutBinaryFileC();
    // The PDB's parameter is a std::basic_string<char> by value: 28 bytes of stack, the exe's entry does ret 0x1C.
    virtual bool openForRead(std::string aFname);
    // Same by-value std::string parameter as openForRead (0xAC4910, ret 0x1C; the body reads aFname's buffer,
    // assigns it to m_fname and destroys it with _Tidy).
    virtual bool openForOverwrite(std::string aFname);
    virtual bool write(const void* data, uint32_t size, uint32_t num);
    virtual bool read(void* data, uint32_t size, uint32_t num);
    // Writes num items of size bytes from data.
    bool writeHeader(const void* data, uint32_t size, uint32_t num);
    // Reads num items of size bytes into data.
    bool readHeader(void* data, uint32_t size, uint32_t num);
    // Whether the file named aFname carries an ID other than this object's fileId: fopen "rb", fread the fileId's
    // length, then compare with the sixth character replaced by fileId's (thiscall, ret 4, 0xAC3FE0). The exe's result
    // is true when the IDs differ.
    bool isCorrectFileId(const std::string& aFname);
    // Opens aFname for update ("r+b"), reads its ID and, when it matches, sets the sixth character to '0' and records
    // the name.
    bool openForUpdate(const std::string& aFname);
    // Opens aFname for append ("a+b"), checks its ID and records the name.
    bool openForAppend(const std::string& aFname);
    // Closes fptr and clears m_fname.
    void close();
    static void operator delete(void* apMemory);

    // The name the file was opened under (IDA: std::string_debug).
    std::string m_fname;
    // The file's 8-character ID; the constructor sets its sixth character to '0'.
    std::string fileId;
    std::FILE* fptr;

public:
    FutBinaryFileC(std::basic_string<char, std::char_traits<char>, std::allocator<char>> a_fileId);
    void changeFileId(const std::basic_string<char, std::char_traits<char>, std::allocator<char>>& newFileId);
    bool seek(int32_t offsetFromBeginning);
};

// Closes the file *apThis holds when there is
// one.
void CloseFile(std::FILE** apThis);
