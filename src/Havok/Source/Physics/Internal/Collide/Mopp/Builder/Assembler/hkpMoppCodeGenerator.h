#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

class hkpMoppCode;

// The buffer the MOPP assembler writes its code into: m_size bytes at m_code, filled from the end (m_pos counts the
// bytes written), and the jump commands still to be patched. The constructor (0xE065F0) allocates nbytes from the
// heap; the destructor (0xE06650) frees them and the jump array. The exe's vtable overrides only the destructor.
class hkpMoppCodeGenerator : public hkReferencedObject {
public:
	// A jump command's place in the code and the chunk it belongs to.
	struct JumpCommandInfo {
		int32_t m_position;
		int32_t m_chunkId;
	};

	hkpMoppCodeGenerator(int32_t a1);
	~hkpMoppCodeGenerator() override;

	// The PDB's compileCode: the written code as a new hkpMoppCode (thiscall, ret 0, 0xE064F0).
	hkpMoppCode* compileCode();
	void resize();
	// The PDB's addCommand8: writes the one-byte param, then the command (0xE067B0, thiscall, ret 8).
	void addCommand8(int32_t a1, int32_t param);
	// The PDB's addCommand16: writes the two-byte param, then the command (0xE06800, thiscall, ret 8).
	void addCommand16(int32_t a1, int32_t param);
	void addCommand24(int32_t a1, int32_t param);
	void addParam24(int32_t param);
	void addCommand32(int32_t a1, uint32_t param);
	void pad(int32_t size);
	// The PDB's addCode: writes one byte (0xE06780, thiscall, ret 4).
	void addCode(int32_t code);

	int32_t m_size;
	int32_t m_pos;
	uint8_t* m_code;
	hkArray<JumpCommandInfo> m_jumpCommands;
};
inline constexpr fnvd::Vtable hkpMoppCodeGenerator_vftable{};
