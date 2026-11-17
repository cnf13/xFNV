#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

class hkpMoppCode;

// The buffer the MOPP assembler writes its code into: m_size bytes at m_code, filled from the end (m_pos counts the
// bytes written), and the jump commands still to be patched. The constructor allocates nbytes from the heap; the
// destructor frees them and the jump array. Only the destructor is overridden.
class hkpMoppCodeGenerator : public hkReferencedObject {
public:
    // A jump command's place in the code and the chunk it belongs to.
    struct JumpCommandInfo {
        int32_t m_position;
        int32_t m_chunkId;
    };

    hkpMoppCodeGenerator(int32_t a1);
    ~hkpMoppCodeGenerator() override;

    // Returns the written code as a new hkpMoppCode.
    hkpMoppCode* compileCode();
    void resize();
    // Writes the one-byte param, then the command.
    void addCommand8(int32_t a1, int32_t param);
    // Writes the two-byte param, then the command.
    void addCommand16(int32_t a1, int32_t param);
    void addCommand24(int32_t a1, int32_t param);
    void addParam24(int32_t param);
    void addCommand32(int32_t a1, uint32_t param);
    void pad(int32_t size);
    // addCode writes one byte.
    void addCode(int32_t code);

    int32_t m_size;
    int32_t m_pos;
    uint8_t* m_code;
    hkArray<JumpCommandInfo> m_jumpCommands;
};
