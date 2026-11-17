#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Fallout Shared/FaceGen/FR2MatrixVTC.inl"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "SDK/FaceGen/TriFileIO.h"
#include "SDK/FaceGen/assert.hpp"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

#include <cstdint>
#include <memory>
#include <vector>

// The standalone build's one instantiation of the matrix's generic members.
template class FR2MatrixVTC<float>;
