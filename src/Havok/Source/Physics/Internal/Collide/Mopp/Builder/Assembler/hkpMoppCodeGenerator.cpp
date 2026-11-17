#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Assembler/hkpMoppCodeGenerator.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"

// The code is written backwards into a buffer of a1 bytes.
hkpMoppCodeGenerator::hkpMoppCodeGenerator(int32_t a1)
  : m_size(a1), m_pos(0),
    m_code(static_cast<uint8_t*>(hkMemoryRouter::easyAlloc(*hkMemoryRouter::getInstance().m_heap, a1))) {}

// The jump commands free themselves; the code buffer is owned.
hkpMoppCodeGenerator::~hkpMoppCodeGenerator() {
    if (m_code) {
        hkMemoryRouter::easyFree(*hkMemoryRouter::getInstance().m_heap, m_code);
    }
}

// Moves the written code (the last m_pos bytes of the buffer) into a new hkpMoppCode and frees the buffer.
hkpMoppCode* hkpMoppCodeGenerator::compileCode() {
    int32_t codeSize = m_pos;
    hkpMoppCode* code =
        static_cast<hkpMoppCode*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpMoppCode)));
    code->m_memSizeAndFlags = sizeof(hkpMoppCode);
    ::new (code) hkpMoppCode;
    code->m_data.reserveExactly(codeSize);
    code->m_data.setSize(codeSize);
    hkString::memCpy(code->m_data.m_data, &m_code[m_size - m_pos], codeSize);
    uint8_t* oldCode = m_code;
    hkMemoryRouter::easyFree(*hkMemoryRouter::getInstance().m_heap, oldCode);
    m_code = nullptr;
    m_pos = 0;
    m_size = codeSize;
    return code;
}

void hkpMoppCodeGenerator::resize() {
    int32_t v2 = 2 * m_size;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    char* v4 = static_cast<char*>(hkMemoryRouter::easyAlloc(*Instance->m_heap, v2));
    hkString::memSet(v4, 205, m_size);
    hkString::memCpy(&v4[m_size], this->m_code, m_size);
    uint8_t* m_code = this->m_code;
    m_size *= 2;
    hkMemoryRouter* v6 = &hkMemoryRouter::getInstance();
    hkMemoryRouter::easyFree(*v6->m_heap, m_code);
    this->m_code = reinterpret_cast<uint8_t*>(v4);
}
