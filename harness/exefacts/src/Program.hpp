// The exe's functions as the Python tools list them: env/cache/ida/functions.tsv's addresses and names (idakit
// functions, from the canonical IDA database), fn.py's bounds (up to the next function, trailing int3 padding cut), its
// direct-call graph, and harness/tools/import_pops.tsv (the bytes each import pops, by IAT slot).
#pragma once

#include "Decoder.hpp"
#include "Image.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace exefacts {

struct FunctionEntry {
    uint32_t address;
    uint32_t size;
    std::string name;
};

class Program {
public:
    // functionList: another database's list (idakit sync's of the database it syncs); empty: the canonical's
    explicit Program(const std::string& root, const std::string& functionList = "");

    const Image& image() const { return image_; }
    Decoder& decoder() { return decoder_; }
    const std::vector<FunctionEntry>& functions() const { return functions_; }
    // The function starting at address, else null
    const FunctionEntry* at(uint32_t address) const;
    // The function whose bounds hold address (fn.Graph.containing), else null
    const FunctionEntry* containing(uint32_t address) const;
    // The bytes an import pops, by IAT slot
    std::optional<uint32_t> importPops(uint32_t slot) const;
    // The functions that call or tail-jump to address (fn.Graph.callers), ascending; built on first use
    const std::vector<uint32_t>& callers(uint32_t address);

private:
    void buildCallers();

    Image image_;
    Decoder decoder_;
    std::vector<FunctionEntry> functions_;
    std::unordered_map<uint32_t, size_t> byAddress_;
    std::unordered_map<uint32_t, uint32_t> importPops_;
    std::unordered_map<uint32_t, std::vector<uint32_t>> callers_;
    bool callersBuilt_ = false;
};

}  // namespace exefacts
