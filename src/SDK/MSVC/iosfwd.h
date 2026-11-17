#pragma once

#include <iosfwd>
#include <stdint.h>
#include <string>

// The PDB's std::fpos<int>::_Stz, fpos's private static conversion state that pos_type(_BADOFF) stores into _Mystate
// (VC9's <iosfwd>; basic_filebuf seekoff/seekpos, 0xACD834). The linker folded it with the .rdata float zero Havok's
// code loads (hkgpConvexHull's movss at 0xC681C0), which IDA names `zero`: src keeps that name and the float type for
// the one address, as most of its users read it.
extern float zero;
