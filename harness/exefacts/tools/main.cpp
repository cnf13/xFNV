// exefacts dump --out FILE.tsv: the facts idakit reads, for every game function (address < 0xEC30E8, the
// order of env/cache/ida/functions.tsv's addresses), one row each after a header row. Columns:
//   address              hex, no 0x
//   ecx, edx             1 when read before written (draft.abi)
//   args                 the stack argument slots read, ascending, comma-separated (draft.abi's args keys)
//   pops                 the bytes the function pops (its `ret N`, else what its proven tail-jump targets agree on;
//                        no vtable-slot resolver here), '-' for unknown
//   reg_args             usercall register inputs among eax, ebx, edi, esi (read before written)
//   clobbers             ebx, edi, esi written outside a push or pop
//   variadic_fixed_args  the fixed arguments of a C-variadic function, '-' for none
//   frame_align          the prologue's `and esp` alignment, 0 for none
//   leaves_eax           1 when some return path sets eax
//   throws               1 when every path throws (checkbody World.throws, queried in address order)
// A fact with no answer (the Python raises) is '!'. Register lists are comma-separated, alphabetical.
#include "Facts.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>

using namespace exefacts;

namespace {

constexpr uint32_t kGameEnd = 0xEC30E8;

std::string registers(Families families) {
    static constexpr std::pair<Families, const char*> names[] = {
        {kEax, "eax"}, {kEbx, "ebx"}, {kEcx, "ecx"}, {kEdi, "edi"}, {kEdx, "edx"}, {kEsi, "esi"}};
    std::string out;
    for (const auto& [family, name] : names)
        if (families & family)
            out += (out.empty() ? "" : ",") + std::string(name);
    return out;
}

// The column text, or '!' where the fact has no answer
std::string column(const std::function<std::string()>& fact) {
    try {
        return fact();
    } catch (const Raised&) {
        return "!";
    }
}

int dump(const std::string& out) {
    const auto started = std::chrono::steady_clock::now();
    Program program(EXEFACTS_ROOT);
    Facts facts(program);
    std::ofstream tsv(out);
    tsv << "address\tecx\tedx\targs\tpops\treg_args\tclobbers\tvariadic_fixed_args\tframe_align\tleaves_eax"
           "\tthrows\n";
    size_t rows = 0;
    for (const FunctionEntry& f : program.functions()) {
        if (f.address >= kGameEnd)
            continue;
        const uint32_t a = f.address;
        char address[16];
        std::snprintf(address, sizeof address, "%x", a);
        std::string abi = column([&] {
            const Abi& info = facts.abi(a);
            std::string args;
            for (int32_t slot : info.args)
                args += (args.empty() ? "" : ",") + std::to_string(slot);
            return std::string(info.ecx ? "1" : "0") + "\t" + (info.edx ? "1" : "0") + "\t" + args;
        });
        if (abi == "!")
            abi = "!\t!\t!";
        tsv << address << '\t' << abi << '\t' << column([&] {
                   std::optional<uint32_t> pops = facts.pops(a);
                   return pops ? std::to_string(*pops) : std::string("-");
               })
            << '\t'
            << column([&] { return registers(facts.regArgs(a)); }) << '\t'
            << column([&] { return registers(facts.clobbers(a)); }) << '\t' << column([&] {
                   std::optional<int32_t> fixed = facts.variadicFixedArgs(a);
                   return fixed ? std::to_string(*fixed) : std::string("-");
               })
            << '\t' << column([&] { return std::to_string(facts.frameAlign(a)); }) << '\t'
            << column([&] { return std::string(facts.leavesEax(a) ? "1" : "0"); }) << '\t'
            << column([&] { return std::string(facts.throws(a) ? "1" : "0"); }) << '\n';
        ++rows;
        facts.trim();
    }
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    std::fprintf(stderr, "rows %zu wall %.1fs\n", rows, seconds);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 4 && std::strcmp(argv[1], "dump") == 0 && std::strcmp(argv[2], "--out") == 0) {
        try {
            return dump(argv[3]);
        } catch (const std::exception& error) {
            std::fprintf(stderr, "exefacts: %s\n", error.what());
            return 1;
        }
    }
    std::fprintf(stderr, "usage: exefacts dump --out FILE.tsv\n");
    return 2;
}
