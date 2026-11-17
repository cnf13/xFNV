#pragma once

#include "Reconstruction.hpp"

// IDA name; not in the PDB. The exe's boolean: one byte, 0 (false) or 1 (true). The exe's declarations name it eBool
// (BSShaderManager::Shutdown 0x4DA4D0, BGSDistantObjectBlock::LoadFromTask 0x6F6150, CombatGroup::CreateDebugGeometry
// 0x9878B0). IDA's enumerators are the C++ keywords false and true, so they are spelled with an e prefix here; the
// Xbox PDB's build spells the same parameters bool, the same byte with the same two values.
enum eBool : uint8_t {
	eFalse = 0x0,
	eTrue = 0x1,
};
