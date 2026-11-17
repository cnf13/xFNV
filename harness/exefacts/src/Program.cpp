#include "Program.hpp"

#include <algorithm>
#include <fstream>
#include <set>

namespace exefacts {

Program::Program(const std::string& root, const std::string& functionList)
    : image_(root + "/env/exe/FalloutNV.exe"), decoder_(image_) {
    // idakit functions: `0xADDR<TAB>0xEND<TAB>NAME`
    const std::string listPath = functionList.empty() ? root + "/env/cache/ida/functions.tsv" : functionList;
    std::ifstream list(listPath);
    if (!list)
        throw std::runtime_error("cannot open " + listPath +
                                 " (write it: idakit functions --idb env/ida/FalloutNV.exe.i64)");
    for (std::string line; std::getline(list, line);) {
        const size_t tab = line.find('\t');
        const size_t tab2 = line.find('\t', tab + 1);
        functions_.push_back({uint32_t(std::stoul(line.substr(0, tab), nullptr, 16)), 0, line.substr(tab2 + 1)});
    }
    std::stable_sort(functions_.begin(), functions_.end(),
                     [](const FunctionEntry& a, const FunctionEntry& b) { return a.address < b.address; });

    // fn.build_graph: a function runs to the next one (or .text's end), less trailing int3 padding
    const uint32_t textStart = image_.textStart(), textEnd = image_.textRawEnd();
    std::span<const uint8_t> code = image_.from(textStart);
    for (size_t i = 0; i < functions_.size(); ++i) {
        FunctionEntry& f = functions_[i];
        byAddress_[f.address] = i;
        uint32_t end = std::min(i + 1 < functions_.size() ? functions_[i + 1].address : textEnd, textEnd);
        if (f.address < textStart || f.address >= end)
            continue;
        while (end > f.address && code[end - 1 - textStart] == 0xCC)
            --end;
        f.size = end - f.address;
    }

    // the pops of imported functions from a table, not IDA's purge, which can be wrong (hkQuaternion::setMul 0xC66320
    // is `ret 8`; IDA purges 4 at createMapping's calls)
    const std::string popsPath = root + "/harness/tools/import_pops.tsv";
    std::ifstream pops(popsPath);
    if (!pops)
        throw std::runtime_error("cannot open " + popsPath);
    for (std::string line; std::getline(pops, line);) {
        std::vector<std::string_view> f = text::parts(line, '\t');
        if (f.size() >= 3 && f[0].starts_with("0x") && !f[2].empty() &&
            std::all_of(f[2].begin(), f[2].end(), [](char c) { return c >= '0' && c <= '9'; }))
            importPops_[uint32_t(std::stoul(std::string(f[0]), nullptr, 16))] = uint32_t(std::stoul(std::string(f[2])));
    }
}

const FunctionEntry* Program::at(uint32_t address) const {
    auto found = byAddress_.find(address);
    return found == byAddress_.end() ? nullptr : &functions_[found->second];
}

const FunctionEntry* Program::containing(uint32_t address) const {
    auto after = std::upper_bound(functions_.begin(), functions_.end(), address,
                                  [](uint32_t a, const FunctionEntry& f) { return a < f.address; });
    if (after == functions_.begin())
        return nullptr;
    const FunctionEntry& f = *(after - 1);
    return address < f.address + std::max<uint32_t>(f.size, 1) ? &f : nullptr;
}

std::optional<uint32_t> Program::importPops(uint32_t slot) const {
    auto found = importPops_.find(slot);
    return found == importPops_.end() ? std::nullopt : std::optional(found->second);
}

const std::vector<uint32_t>& Program::callers(uint32_t address) {
    if (!callersBuilt_)
        buildCallers();
    static const std::vector<uint32_t> none;
    auto found = callers_.find(address);
    return found == callers_.end() ? none : found->second;
}

void Program::buildCallers() {
    // fn.build_graph: a linear decode of each function's bytes (an undecodable byte skipped); its direct calls and
    // jumps to another function's start
    csh lite;
    if (cs_open(CS_ARCH_X86, CS_MODE_32, &lite) != CS_ERR_OK)
        throw std::runtime_error("capstone: cs_open failed");
    cs_insn* insn = cs_malloc(lite);
    std::span<const uint8_t> code = image_.from(image_.textStart());
    for (const FunctionEntry& f : functions_) {
        if (f.size == 0)
            continue;
        const uint32_t end = f.address + f.size;
        std::set<uint32_t> callees;
        uint64_t pc = f.address;
        while (pc < end) {
            const uint8_t* bytes = code.data() + (pc - image_.textStart());
            size_t left = end - pc;
            while (cs_disasm_iter(lite, &bytes, &left, &pc, insn)) {
                const std::string_view m = insn->mnemonic, ops = insn->op_str;
                if ((m == "call" || m == "jmp") && ops.starts_with("0x")) {
                    const uint32_t target = uint32_t(std::stoul(std::string(ops), nullptr, 16));
                    if (target != f.address && byAddress_.contains(target))
                        callees.insert(target);
                }
            }
            if (pc < end)
                ++pc;
        }
        for (uint32_t callee : callees)
            callers_[callee].push_back(f.address);
    }
    cs_free(insn, 1);
    cs_close(&lite);
    callersBuilt_ = true;
}

}  // namespace exefacts
