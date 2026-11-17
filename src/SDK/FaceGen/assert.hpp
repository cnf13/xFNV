#pragma once

#include "Reconstruction.hpp"

// Reports a failed FaceGen check at line of fname as a warning.
void fr2Assert(const char* fname, int32_t line);

// IDA name; not in the PDB. The exe only passes the two strings on to Warning's format.
int32_t WriteLog(const char* a1, int32_t a2, const char* a3);
