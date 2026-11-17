#pragma once

#include "Reconstruction.hpp"

// cdecl, ret 0, 0xAA1070. A zero size is raised to 1; 4-byte aligned (event NI_MALLOC).
void* _NiMalloc(size_t astSizeInBytes);
// cdecl, ret 0, 0xAA10F0. Null is ignored (event NI_FREE).
void _NiFree(void* pvMem);
// cdecl, ret 0, 0xAA11B0. Realloc on the external allocator: frees on a zero size, allocates on a null block.
void* _NiExternalRealloc(void* pvMem, size_t astSizeInBytes);
// cdecl, ret 0, 0xAA1250. As _NiExternalRealloc, aligned to astAlignment.
void* _NiExternalAlignedRealloc(void* pvMem, size_t astSizeInBytes, size_t stAlignment);
// cdecl, ret 0, 0xAA1110. Frees a block from the aligned allocator; null is ignored.
void _NiAlignedFree(void* pvMem);
// cdecl, ret 0, 0xAA1190. Frees a block from the external aligned allocator; null is ignored
// (event NI_EXTERNAL_ALIGNEDFREE). The name is IDA's; not in the PDB.
void _NiExternalAlignedFree(void* pvMem);
// cdecl, ret 0, 0xAA10B0. A zero size is raised to 1; aligned to astAlignment.
void* _NiAlignedMalloc(size_t stSizeInBytes, size_t stAlignment);
// cdecl, ret 0, 0xAA1130. A zero size is raised to 1; aligned to astAlignment on the external
// allocator (event NI_EXTERNAL_ALIGNEDMALLOC).
void* _NiExternalAlignedMalloc(size_t stSizeInBytes, size_t stAlignment);
// cdecl, ret 0, 0xAA1170. Frees a block from the external allocator; null is ignored (event
// NI_EXTERNAL_FREE).
void _NiExternalFree(void* pvMem);

// Gamebryo heap entry points for plain blocks (no constructor or destructor runs); objects use NiMemObject's new and
// delete. All go through NiMemManager's allocator, so memory from them may be freed by the exe and the reverse.
