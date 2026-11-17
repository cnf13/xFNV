#include "BSSystem/BSSystemDir.h"

BSSystemDir::BSSystemDir(const char* apcPath) : eLastError(BSSystemFile::EC_NONE), uiEntryPos(0) {
    DoOpen(apcPath);
}

BSSystemDir::~BSSystemDir() {
    DoClose();
}

bool BSSystemDir::QRegular() {
    return (((FindData_.dwFileAttributes & 32) != 0) || ((FindData_.dwFileAttributes & 128) != 0));
}

int32_t BSSystemDir::IsDirectory() {
    return (int32_t)((uint8_t)((FindData_.dwFileAttributes & 16) != 0));
}
