#include "Decoder.hpp"

#include <cstring>

namespace exefacts {

const Operand* Insn::mem() const {
    for (const Operand& op : operands)
        if (op.type == X86_OP_MEM)
            return &op;
    return nullptr;
}

std::optional<uint32_t> Insn::target() const {
    if (!operands.empty() && operands[0].type == X86_OP_IMM)
        return uint32_t(operands[0].imm);
    return std::nullopt;
}

std::optional<uint32_t> Insn::imm() const {
    for (const Operand& op : operands)
        if (op.type == X86_OP_IMM)
            return uint32_t(op.imm);
    return std::nullopt;
}

std::optional<uint32_t> Insn::iatSlot() const {
    if (operands.size() != 1)
        return std::nullopt;
    const Operand& op = operands[0];
    if (op.type != X86_OP_MEM || op.base != X86_REG_INVALID || op.index != X86_REG_INVALID)
        return std::nullopt;
    return uint32_t(op.disp);
}

Decoder::Decoder(const Image& image) : image_(image) {
    if (cs_open(CS_ARCH_X86, CS_MODE_32, &handle_) != CS_ERR_OK)
        throw std::runtime_error("capstone: cs_open failed");
    cs_option(handle_, CS_OPT_DETAIL, CS_OPT_ON);
    scratch_ = cs_malloc(handle_);
    for (unsigned reg = 1; reg < X86_REG_ENDING; ++reg)
        if (const char* name = cs_reg_name(handle_, reg))
            families_[reg] = text::family(name);
}

Decoder::~Decoder() {
    cs_free(scratch_, 1);
    cs_close(&handle_);
}

const Insn* Decoder::at(uint32_t address) {
    auto [slot, inserted] = cache_.try_emplace(address);
    if (!inserted)
        return slot->second.get();
    std::span<const uint8_t> bytes = image_.from(address);
    const uint8_t* code = bytes.data();
    size_t size = std::min<size_t>(bytes.size(), 16);
    uint64_t pc = address;
    if (!cs_disasm_iter(handle_, &code, &size, &pc, scratch_))
        return nullptr;
    auto insn = std::make_unique<Insn>();
    insn->address = address;
    insn->size = scratch_->size;
    insn->mnemonic = scratch_->mnemonic;
    insn->ops = scratch_->op_str;
    const cs_x86& x86 = scratch_->detail->x86;
    for (uint8_t i = 0; i < x86.op_count; ++i) {
        const cs_x86_op& op = x86.operands[i];
        Operand out{op.type, 0, X86_REG_INVALID, X86_REG_INVALID, X86_REG_INVALID, 0, 0, op.size};
        if (op.type == X86_OP_IMM)
            out.imm = op.imm;
        else if (op.type == X86_OP_REG)
            out.reg = op.reg;
        else if (op.type == X86_OP_MEM) {
            out.base = op.mem.base;
            out.index = op.mem.index;
            out.scale = op.mem.scale;
            out.disp = op.mem.disp;
        }
        insn->operands.push_back(out);
    }
    cs_regs read, written;
    uint8_t readCount = 0, writtenCount = 0;
    insn->reads = insn->writes = 0;
    if (cs_regs_access(handle_, scratch_, read, &readCount, written, &writtenCount) == CS_ERR_OK) {
        for (uint8_t i = 0; i < readCount; ++i)
            insn->reads |= families_[read[i]];
        for (uint8_t i = 0; i < writtenCount; ++i)
            insn->writes |= families_[written[i]];
    }
    insn->sets = 0;
    for (uint8_t i = 0; i < scratch_->detail->groups_count; ++i)
        switch (scratch_->detail->groups[i]) {
        case X86_GRP_FPU: insn->sets |= kX87; break;
        case X86_GRP_SSE1: insn->sets |= kSse1; break;
        case X86_GRP_SSE2: case X86_GRP_SSE3: case X86_GRP_SSSE3: case X86_GRP_SSE41: case X86_GRP_SSE42:
        case X86_GRP_SSE4A: insn->sets |= kSse2; break;
        case X86_GRP_MMX: insn->sets |= kMmx; break;
        default: break;
        }
    slot->second = std::move(insn);
    return slot->second.get();
}

namespace {

// A whole 32-bit general register's name (draft: REG_FAMILY.get(name) == name)
bool fullRegister(std::string_view name) {
    return name.size() == 3 && name[0] == 'e' && text::family(name) != 0;
}

}  // namespace

RegisterUse registerUse(const Insn& insn) {
    RegisterUse use{insn.reads, insn.writes};
    const std::vector<std::string_view> parts = text::parts(insn.ops);
    if (parts.size() != 2)
        return use;
    const std::string_view dst = parts[0], src = parts[1];
    if ((insn.is("lea") || insn.is("mov")) && fullRegister(dst) &&
        (src == dst || (src.size() == dst.size() + 2 && src.front() == '[' && src.back() == ']' &&
                        src.substr(1, dst.size()) == dst)))
        return {};
    if ((insn.is("xor") || insn.is("sub")) && dst == src)
        use.reads &= ~text::family(dst);
    if (fullRegister(dst) &&
        ((insn.is("or") && (src == "-1" || src == "0xffffffff")) || (insn.is("and") && src == "0")))
        use.reads &= ~text::family(dst);
    return use;
}

namespace text {

std::string_view strip(std::string_view s) {
    constexpr std::string_view space = " \t\n\r\v\f";
    const size_t first = s.find_first_not_of(space);
    if (first == std::string_view::npos)
        return {};
    return s.substr(first, s.find_last_not_of(space) - first + 1);
}

std::vector<std::string_view> parts(std::string_view text, char sep) {
    std::vector<std::string_view> out;
    size_t start = 0;
    for (;;) {
        const size_t cut = text.find(sep, start);
        out.push_back(strip(text.substr(start, cut - start)));
        if (cut == std::string_view::npos)
            return out;
        start = cut + 1;
    }
}

std::optional<int64_t> pyInt(std::string_view s) {
    s = strip(s);
    bool negative = false;
    if (!s.empty() && (s[0] == '+' || s[0] == '-')) {
        negative = s[0] == '-';
        s.remove_prefix(1);
    }
    int base = 10;
    if (s.size() >= 2 && s[0] == '0' && std::strchr("xXoObB", s[1])) {
        base = std::strchr("xX", s[1]) ? 16 : std::strchr("oO", s[1]) ? 8 : 2;
        s.remove_prefix(2);
        if (!s.empty() && s[0] == '_')
            s.remove_prefix(1);
    }
    if (s.empty())
        return std::nullopt;
    // a decimal literal with a leading zero is all zeros
    const bool zeros = base == 10 && s[0] == '0';
    uint64_t value = 0;
    bool afterDigit = false;
    for (char c : s) {
        if (c == '_') {
            if (!afterDigit)
                return std::nullopt;
            afterDigit = false;
            continue;
        }
        int digit = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10
                    : c >= 'A' && c <= 'F' ? c - 'A' + 10 : 99;
        if (digit >= base || (zeros && digit != 0))
            return std::nullopt;
        value = value * base + digit;
        afterDigit = true;
    }
    if (!afterDigit)
        return std::nullopt;
    return negative ? -int64_t(value) : int64_t(value);
}

Families family(std::string_view name) {
    static constexpr struct {
        std::string_view name;
        Families family;
    } table[] = {
        {"eax", kEax}, {"ax", kEax}, {"al", kEax}, {"ah", kEax}, {"ebx", kEbx}, {"bx", kEbx}, {"bl", kEbx},
        {"bh", kEbx}, {"ecx", kEcx}, {"cx", kEcx}, {"cl", kEcx}, {"ch", kEcx}, {"edx", kEdx}, {"dx", kEdx},
        {"dl", kEdx}, {"dh", kEdx}, {"esi", kEsi}, {"si", kEsi}, {"edi", kEdi}, {"di", kEdi}, {"ebp", kEbp},
        {"bp", kEbp}, {"esp", kEsp}, {"sp", kEsp},
    };
    for (const auto& entry : table)
        if (entry.name == name)
            return entry.family;
    return 0;
}

int64_t signed32(int64_t value) {
    return int32_t(uint32_t(value));
}

}  // namespace text

}  // namespace exefacts
