// winmm's multimedia file I/O (mmio) as the game uses it: BSWin32GameSound::Open reads a wave file through its own I/O
// procedure (mmio_IOProc, which reads the game's BSFile), walking the RIFF chunks with mmioDescend and mmioAscend and
// reading the data through mmioRead, or through the buffer itself with mmioGetInfo and mmioAdvance.
//
// A file is always buffered (8 KB, mmio's default size, or the caller's cchBuffer): the buffer holds the file's bytes
// from lBufOffset to pchEndRead, and pchNext is the file position. The I/O procedure keeps lDiskOffset, where its next
// read starts. Writing is not supported: the game only reads.
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

namespace audio::mmio {

    namespace {

        // mmsystem.h's values, as the SDK has them.
        constexpr uint32_t MMSYSERR_NOERROR = 0;
        constexpr uint32_t MMSYSERR_INVALHANDLE = 5;
        constexpr uint32_t MMSYSERR_INVALPARAM = 11;
        constexpr uint32_t MMIOERR_FILENOTFOUND = 0x101;
        constexpr uint32_t MMIOERR_CANNOTREAD = 0x103;
        constexpr uint32_t MMIOERR_CANNOTSEEK = 0x107;
        constexpr uint32_t MMIOERR_CHUNKNOTFOUND = 0x109;
        constexpr uint32_t MMIOERR_CANNOTEXPAND = 0x10D;
        constexpr uint32_t MMIO_RWMODE = 0x3;
        constexpr uint32_t MMIO_READ = 0x0;
        constexpr uint32_t MMIO_FINDCHUNK = 0x10;
        constexpr uint32_t MMIO_FINDRIFF = 0x20;
        constexpr uint32_t MMIO_FINDLIST = 0x40;
        constexpr uint32_t MMIOM_READ = 0;
        constexpr uint32_t MMIOM_SEEK = 2;
        constexpr uint32_t MMIOM_OPEN = 3;
        constexpr uint32_t MMIOM_CLOSE = 4;
        constexpr int32_t SEEK_FROM_START = 0;
        constexpr int32_t kDefaultBuffer = 8192;
        constexpr uint32_t FOURCC_RIFF = 0x46464952;
        constexpr uint32_t FOURCC_LIST = 0x5453494C;

        struct File {
            _MMIOINFO info;
            std::vector<char> buffer;
        };

        File* FromHandle(HMMIO handle) {
            return reinterpret_cast<File*>(handle);
        }

        // The file position: the buffer's file offset plus how far into it pchNext is.
        int32_t Position(const File* file) {
            return file->info.lBufOffset + static_cast<int32_t>(file->info.pchNext - file->info.pchBuffer);
        }

        int32_t Proc(File* file, uint32_t message, intptr_t first, intptr_t second) {
            return file->info.pIOProc(&file->info, message, first, second);
        }

        // Refills the buffer from the file at `position`. False when the I/O procedure fails; an empty buffer at the
        // end of the file is no failure.
        bool Fill(File* file, int32_t position) {
            _MMIOINFO& info = file->info;
            if (info.lDiskOffset != position) {
                Proc(file, MMIOM_SEEK, position, SEEK_FROM_START);
                if (info.lDiskOffset != position) {
                    return false;
                }
            }
            const int32_t got = Proc(file, MMIOM_READ, reinterpret_cast<intptr_t>(info.pchBuffer), info.cchBuffer);
            info.lBufOffset = position;
            info.pchNext = info.pchBuffer;
            info.pchEndRead = info.pchBuffer + (got > 0 ? got : 0);
            return got >= 0;
        }

        // Moves the file position to `position`: within the buffer, or to an empty buffer there that the next read
        // fills.
        void Seek(File* file, int32_t position) {
            _MMIOINFO& info = file->info;
            if (position >= info.lBufOffset && position <= info.lBufOffset + (info.pchEndRead - info.pchBuffer)) {
                info.pchNext = info.pchBuffer + (position - info.lBufOffset);
                return;
            }
            info.lBufOffset = position;
            info.pchNext = info.pchBuffer;
            info.pchEndRead = info.pchBuffer;
        }

        int32_t Read(File* file, char* out, int32_t bytes) {
            _MMIOINFO& info = file->info;
            int32_t done = 0;
            while (done < bytes) {
                if (info.pchNext == info.pchEndRead) {
                    if (!Fill(file, Position(file))) {
                        return -1;
                    }
                    if (info.pchNext == info.pchEndRead) {
                        break;
                    }
                }
                const int32_t chunk =
                    std::min<int32_t>(bytes - done, static_cast<int32_t>(info.pchEndRead - info.pchNext));
                std::memcpy(out + done, info.pchNext, chunk);
                info.pchNext += chunk;
                done += chunk;
            }
            return done;
        }

        uint32_t ReadWord(File* file, uint32_t* out) {
            return Read(file, reinterpret_cast<char*>(out), 4) == 4 ? MMSYSERR_NOERROR : MMIOERR_CHUNKNOTFOUND;
        }

    } // namespace

    // The file opened with the caller's I/O procedure; a file without one (DOS files, memory files) is not supported.
    HMMIO WINAPI Open(char* name, _MMIOINFO* settings, uint32_t openFlags) {
        if (!settings || !settings->pIOProc || (openFlags & MMIO_RWMODE) != MMIO_READ) {
            if (settings) {
                settings->wErrorRet = MMSYSERR_INVALPARAM;
            }
            return nullptr;
        }
        File* file = new File;
        file->info = *settings;
        file->info.dwFlags = openFlags;
        file->info.wErrorRet = 0;
        file->info.lDiskOffset = 0;
        file->info.cchBuffer = settings->cchBuffer > 0 ? settings->cchBuffer : kDefaultBuffer;
        file->buffer.assign(static_cast<size_t>(file->info.cchBuffer), 0);
        file->info.pchBuffer = file->buffer.data();
        file->info.pchNext = file->info.pchBuffer;
        file->info.pchEndRead = file->info.pchBuffer;
        file->info.pchEndWrite = file->info.pchBuffer + file->info.cchBuffer;
        file->info.lBufOffset = 0;
        file->info.hmmio = reinterpret_cast<HMMIO>(file);
        const int32_t result = Proc(file, MMIOM_OPEN, reinterpret_cast<intptr_t>(name), 0);
        if (result != 0) {
            settings->wErrorRet = result > 0 ? static_cast<uint32_t>(result) : MMIOERR_FILENOTFOUND;
            delete file;
            return nullptr;
        }
        settings->wErrorRet = 0;
        return file->info.hmmio;
    }

    uint32_t WINAPI Close(HMMIO handle, uint32_t closeFlags) {
        File* file = FromHandle(handle);
        if (!file) {
            return MMSYSERR_INVALHANDLE;
        }
        const int32_t result = Proc(file, MMIOM_CLOSE, closeFlags, 0);
        delete file;
        return static_cast<uint32_t>(result);
    }

    int32_t WINAPI ReadBytes(HMMIO handle, char* out, int32_t bytes) {
        File* file = FromHandle(handle);
        if (!file || !out || bytes < 0) {
            return -1;
        }
        return Read(file, out, bytes);
    }

    // Finds a chunk: the next one (no flags), or the next with the caller's ckid (MMIO_FINDCHUNK) or with the caller's
    // fccType in a RIFF or LIST chunk (MMIO_FINDRIFF, MMIO_FINDLIST), within `parent`'s data when given. The position
    // ends after the chunk's header (and after a RIFF or LIST chunk's form type); on failure it is where it was.
    uint32_t WINAPI Descend(HMMIO handle, _MMCKINFO* chunk, const _MMCKINFO* parent, uint32_t descendFlags) {
        File* file = FromHandle(handle);
        if (!file) {
            return MMSYSERR_INVALHANDLE;
        }
        if (!chunk) {
            return MMSYSERR_INVALPARAM;
        }
        const int32_t start = Position(file);
        const int64_t end = parent ? static_cast<int64_t>(parent->dwDataOffset) + parent->cksize : INT64_MAX;
        const uint32_t wantId = chunk->ckid;
        const uint32_t wantType = chunk->fccType;
        for (;;) {
            const int32_t at = Position(file);
            _MMCKINFO found{};
            if (at + 8 > end || ReadWord(file, &found.ckid) != 0 || ReadWord(file, &found.cksize) != 0) {
                Seek(file, start);
                return MMIOERR_CHUNKNOTFOUND;
            }
            found.dwDataOffset = static_cast<uint32_t>(at + 8);
            const bool list = found.ckid == FOURCC_RIFF || found.ckid == FOURCC_LIST;
            if (list && ReadWord(file, &found.fccType) != 0) {
                Seek(file, start);
                return MMIOERR_CHUNKNOTFOUND;
            }
            bool match = true;
            if (descendFlags & MMIO_FINDCHUNK) {
                match = found.ckid == wantId;
            }
            else if (descendFlags & MMIO_FINDRIFF) {
                match = found.ckid == FOURCC_RIFF && found.fccType == wantType;
            }
            else if (descendFlags & MMIO_FINDLIST) {
                match = found.ckid == FOURCC_LIST && found.fccType == wantType;
            }
            if (match) {
                *chunk = found;
                return MMSYSERR_NOERROR;
            }
            // The next chunk: data padded to an even size.
            Seek(file, static_cast<int32_t>(found.dwDataOffset + found.cksize + (found.cksize & 1)));
        }
    }

    // Leaves a chunk: the position goes past its data, padded to an even size.
    uint32_t WINAPI Ascend(HMMIO handle, _MMCKINFO* chunk, uint32_t) {
        File* file = FromHandle(handle);
        if (!file) {
            return MMSYSERR_INVALHANDLE;
        }
        if (!chunk) {
            return MMSYSERR_INVALPARAM;
        }
        Seek(file, static_cast<int32_t>(chunk->dwDataOffset + chunk->cksize + (chunk->cksize & 1)));
        return MMSYSERR_NOERROR;
    }

    uint32_t WINAPI GetInfo(HMMIO handle, _MMIOINFO* out, uint32_t) {
        File* file = FromHandle(handle);
        if (!file) {
            return MMSYSERR_INVALHANDLE;
        }
        if (!out) {
            return MMSYSERR_INVALPARAM;
        }
        *out = file->info;
        return MMSYSERR_NOERROR;
    }

    // Refills the buffer from where its data ends, after taking the caller's pchNext (its reads through the buffer);
    // the caller's MMIOINFO gets the new buffer state. At the end of the file it succeeds with pchNext at pchEndRead.
    uint32_t WINAPI Advance(HMMIO handle, _MMIOINFO* info, uint32_t advanceFlags) {
        File* file = FromHandle(handle);
        if (!file) {
            return MMSYSERR_INVALHANDLE;
        }
        if (advanceFlags != MMIO_READ) {
            return MMIOERR_CANNOTEXPAND;
        }
        if (info) {
            file->info.pchNext = info->pchNext;
        }
        const int32_t end = file->info.lBufOffset + static_cast<int32_t>(file->info.pchEndRead - file->info.pchBuffer);
        if (!Fill(file, end)) {
            return file->info.lDiskOffset == end ? MMIOERR_CANNOTREAD : MMIOERR_CANNOTSEEK;
        }
        if (info) {
            *info = file->info;
        }
        return MMSYSERR_NOERROR;
    }

} // namespace audio::mmio

// The exports the game's imports name: on Linux the C functions themselves; on Windows (FNVD_SDL_AUDIO) the import
// slots `__imp_` the game's dllimport calls go through, so winmm.lib (which the program links for its other
// functions) never supplies these.
#if defined(_WIN32)
#if defined(_M_IX86)
#define FNVD_MMIO_SLOT(name, bytes, impl)                                                                              \
    extern "C" {                                                                                                       \
    decltype(&audio::mmio::impl) fnvd_imp_##name __asm__("__imp__" #name "@" #bytes) = &audio::mmio::impl;             \
    }
#else
#define FNVD_MMIO_SLOT(name, bytes, impl)                                                                              \
    extern "C" {                                                                                                       \
    decltype(&audio::mmio::impl) fnvd_imp_##name __asm__("__imp_" #name) = &audio::mmio::impl;                         \
    }
#endif
FNVD_MMIO_SLOT(mmioOpenA, 12, Open)
FNVD_MMIO_SLOT(mmioClose, 8, Close)
FNVD_MMIO_SLOT(mmioRead, 12, ReadBytes)
FNVD_MMIO_SLOT(mmioDescend, 16, Descend)
FNVD_MMIO_SLOT(mmioAscend, 12, Ascend)
FNVD_MMIO_SLOT(mmioGetInfo, 12, GetInfo)
FNVD_MMIO_SLOT(mmioAdvance, 12, Advance)
#else
extern "C" {

HMMIO WINAPI mmioOpenA(char* name, _MMIOINFO* settings, uint32_t openFlags) {
    return audio::mmio::Open(name, settings, openFlags);
}

uint32_t WINAPI mmioClose(HMMIO handle, uint32_t closeFlags) {
    return audio::mmio::Close(handle, closeFlags);
}

int32_t WINAPI mmioRead(HMMIO handle, char* out, int32_t bytes) {
    return audio::mmio::ReadBytes(handle, out, bytes);
}

uint32_t WINAPI mmioDescend(HMMIO handle, _MMCKINFO* chunk, const _MMCKINFO* parent, uint32_t descendFlags) {
    return audio::mmio::Descend(handle, chunk, parent, descendFlags);
}

uint32_t WINAPI mmioAscend(HMMIO handle, _MMCKINFO* chunk, uint32_t ascendFlags) {
    return audio::mmio::Ascend(handle, chunk, ascendFlags);
}

uint32_t WINAPI mmioGetInfo(HMMIO handle, _MMIOINFO* info, uint32_t infoFlags) {
    return audio::mmio::GetInfo(handle, info, infoFlags);
}

uint32_t WINAPI mmioAdvance(HMMIO handle, _MMIOINFO* info, uint32_t advanceFlags) {
    return audio::mmio::Advance(handle, info, advanceFlags);
}

} // extern "C"
#endif
