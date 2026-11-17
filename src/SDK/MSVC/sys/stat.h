#pragma once

#include "Reconstruction.hpp"
#include <stdint.h>
// Included first so that the platform's header is done with: glibc's defines st_atime, st_mtime and st_ctime as
// macros (st_atim.tv_sec, ...), which would rewrite the members below and every read of them. src never reads the
// platform's struct stat, so those macros go.
#include <sys/stat.h>
#ifdef st_atime
#undef st_atime
#undef st_mtime
#undef st_ctime
#endif

// rt: runtime support the game relies on. Here VC9's file status record and its fill, on the game path's native path.
namespace rt {

// The VC9 CRT's <sys/stat.h> file status with a 32-bit size and 64-bit times, which the exe's _stat64i32
// (0xEC923C) fills. PDB members and offsets; IDA's struct has the same layout, sizeof 0x30.
struct _stat64i32 {
	uint32_t st_dev;
	uint16_t st_ino;
	uint16_t st_mode;
	int16_t st_nlink;
	int16_t st_uid;
	int16_t st_gid;
	uint32_t st_rdev;
	int32_t st_size;
	int64_t st_atime;
	int64_t st_mtime;
	int64_t st_ctime;
};

// _stat64i32, 0xEC923C: fills apStat with apPath's status; 0, or -1 with errno set. The function hides the struct's
// name: the struct is written `struct _stat64i32`. Defined in stat.cpp.
int32_t _stat64i32(const char* apPath, struct _stat64i32* apStat);

} // namespace rt
