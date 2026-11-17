#pragma once

#include "Reconstruction.hpp"

// Havok's node access for hkAabbTree walks (the SDK's getNode, an inline template that returns the node it is given
// off the SPU). No data: sizeof 1 in Havok's release hkinternal.pdb; the game PDB has its getNode instances for both
// trees and both index widths.
class hkAabbTreeUtil {};
