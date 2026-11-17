// Capstone (the venv's, the one the Python tools use) in detail mode, one cached instruction per address, and the
// text helpers the facts read capstone's operand strings with (the Python tools match on mnemonic and op_str text).
#pragma once

#include "Image.hpp"

#include <capstone/capstone.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace exefacts {

// A general register family (draft.REG_FAMILY: al, ah, ax and eax are eax), as a bit
enum Family : uint8_t {
    kEax = 1, kEcx = 2, kEdx = 4, kEbx = 8, kEsp = 16, kEbp = 32, kEsi = 64, kEdi = 128,
};
using Families = uint8_t;

// An instruction set beyond the base integer one (capstone's groups), as a bit: x87 (fpu), SSE1, SSE2 or later, MMX
enum InstructionSet : uint8_t {
    kX87 = 1, kSse1 = 2, kSse2 = 4, kMmx = 8,
};
using InstructionSets = uint8_t;

// A capstone operand: an immediate's value, a register operand's register, a memory operand's base, index, scale and
// displacement, and the operand's size in bytes
struct Operand {
    x86_op_type type;
    int64_t imm;
    x86_reg reg;
    x86_reg base;
    x86_reg index;
    int scale;
    int64_t disp;
    uint8_t size;
};

struct Insn {
    uint32_t address;
    uint32_t size;
    std::string mnemonic;
    std::string ops;  // capstone's op_str
    std::vector<Operand> operands;
    // the families of the registers cs_regs_access lists (none where it fails)
    Families reads;
    Families writes;
    // the instruction set it belongs to (capstone's groups), 0 for the base one
    InstructionSets sets;

    uint32_t next() const { return address + size; }
    bool is(std::string_view m) const { return mnemonic == m; }
    bool startsWith(std::string_view prefix) const { return mnemonic.starts_with(prefix); }
    // The first memory operand (draft.mem_operand)
    const Operand* mem() const;
    // A direct branch target: the first operand as an immediate (draft.Func._imm_target)
    std::optional<uint32_t> target() const;
    // The first immediate operand (draft._imm)
    std::optional<uint32_t> imm() const;
    // The IAT slot of `call/jmp dword ptr [slot]`: the one operand, an absolute memory operand (draft._iat_target)
    std::optional<uint32_t> iatSlot() const;
};

class Decoder {
public:
    explicit Decoder(const Image& image);
    ~Decoder();
    Decoder(const Decoder&) = delete;
    Decoder& operator=(const Decoder&) = delete;

    // The instruction at address (decoded from up to 16 image bytes), null when capstone cannot decode one
    const Insn* at(uint32_t address);
    // The instructions decoded so far, and dropping them (every Insn pointer at() returned dangles after clear)
    size_t size() const { return cache_.size(); }
    void clear() { cache_.clear(); }

private:
    const Image& image_;
    csh handle_ = 0;
    cs_insn* scratch_ = nullptr;
    Families families_[X86_REG_ENDING] = {};
    std::unordered_map<uint32_t, std::unique_ptr<Insn>> cache_;
};

// draft._family_reads_writes: the register families an instruction reads and writes (cs_regs_access), less the
// reads of alignment padding (`mov edi, edi`, `lea ecx, [ecx]`) and of results that do not depend on the old value
// (`xor r, r`, `sub r, r`, `or r32, -1`, `and r32, 0`)
struct RegisterUse {
    Families reads = 0;
    Families writes = 0;
};
RegisterUse registerUse(const Insn& insn);

namespace text {

// Python's str.strip() (ASCII whitespace)
std::string_view strip(std::string_view s);
// Python's [s.strip() for s in text.split(sep)]
std::vector<std::string_view> parts(std::string_view text, char sep = ',');
// Python's int(s, 0); nullopt where it raises ValueError
std::optional<int64_t> pyInt(std::string_view s);
// draft.REG_FAMILY.get(name), 0 for none
Families family(std::string_view name);
// draft._signed
int64_t signed32(int64_t value);

}  // namespace text

}  // namespace exefacts
