#include "exefacts.h"

#include "Facts.hpp"

#include <string>

using namespace exefacts;

static_assert(int(EXEFACTS_EAX) == kEax && int(EXEFACTS_ECX) == kEcx && int(EXEFACTS_EDX) == kEdx &&
              int(EXEFACTS_EBX) == kEbx && int(EXEFACTS_ESP) == kEsp && int(EXEFACTS_EBP) == kEbp &&
              int(EXEFACTS_ESI) == kEsi && int(EXEFACTS_EDI) == kEdi);

struct ExefactsContext {
    explicit ExefactsContext(const char* root) : program(root), facts(program) {}

    Program program;
    Facts facts;
};

namespace {

thread_local std::string lastError;

// Decoded instructions a context keeps between queries (Facts::trimInstructions)
constexpr size_t kMaxInsns = 200000;

// Runs a fact query: 0 with its answer, -1 with the reason where it has none
template <typename Query>
int answer(ExefactsContext* context, Query query) {
    try {
        context->facts.trimInstructions(kMaxInsns);
        query();
        lastError.clear();
        return 0;
    } catch (const std::exception& error) {
        lastError = error.what();
        return -1;
    }
}

}  // namespace

extern "C" {

ExefactsContext* exefacts_open(const char* root) {
    try {
        lastError.clear();
        return new ExefactsContext(root);
    } catch (const std::exception& error) {
        lastError = error.what();
        return nullptr;
    }
}

void exefacts_close(ExefactsContext* context) {
    delete context;
}

const char* exefacts_error(void) {
    return lastError.c_str();
}

int exefacts_abi(ExefactsContext* context, uint32_t address, ExefactsAbi* out) {
    return answer(context, [&] {
        const Abi& abi = context->facts.abi(address);
        *out = {abi.ecx, abi.edx, abi.args.data(), uint32_t(abi.args.size())};
    });
}

int exefacts_pops(ExefactsContext* context, uint32_t address, ExefactsPops* out) {
    return answer(context, [&] {
        std::optional<uint32_t> pops = context->facts.pops(address);
        *out = {pops.has_value(), pops.value_or(0)};
    });
}

int exefacts_reg_args(ExefactsContext* context, uint32_t address, uint8_t* out) {
    return answer(context, [&] { *out = context->facts.regArgs(address); });
}

int exefacts_clobbers(ExefactsContext* context, uint32_t address, uint8_t* out) {
    return answer(context, [&] { *out = context->facts.clobbers(address); });
}

int exefacts_variadic_fixed_args(ExefactsContext* context, uint32_t address, int32_t* out) {
    return answer(context, [&] { *out = context->facts.variadicFixedArgs(address).value_or(-1); });
}

int exefacts_frame_align(ExefactsContext* context, uint32_t address, uint32_t* out) {
    return answer(context, [&] { *out = context->facts.frameAlign(address); });
}

int exefacts_instruction_sets(ExefactsContext* context, uint32_t address, uint8_t* out) {
    return answer(context, [&] { *out = context->facts.instructionSets(address); });
}

int exefacts_leaves_eax(ExefactsContext* context, uint32_t address, uint8_t* out) {
    return answer(context, [&] { *out = context->facts.leavesEax(address); });
}

int exefacts_returns_eax(ExefactsContext* context, uint32_t address, uint8_t* out) {
    return answer(context, [&] { *out = context->facts.leavesEax(address, false); });
}

int exefacts_returns_this(ExefactsContext* context, uint32_t address, uint8_t* out) {
    return answer(context, [&] { *out = context->facts.returnsThis(address); });
}

int exefacts_return_width(ExefactsContext* context, uint32_t address, ExefactsReturnWidth* out) {
    return answer(context, [&] {
        std::optional<ReturnWidth> width = context->facts.returnWidth(address);
        *out = width ? ExefactsReturnWidth{width->width, width->extension} : ExefactsReturnWidth{0, 0};
    });
}

int exefacts_throws(ExefactsContext* context, uint32_t address, uint8_t* out) {
    return answer(context, [&] { *out = context->facts.throws(address); });
}

int exefacts_trivial_accessor(ExefactsContext* context, uint32_t address, ExefactsAccessor* out) {
    return answer(context, [&] {
        *out = {EXEFACTS_ACCESSOR_NONE, 0, 0, 0, 0, 0};
        if (std::optional<Accessor> accessor = context->facts.trivialAccessor(address)) {
            const int kind = accessor->store  ? EXEFACTS_ACCESSOR_STORE
                             : accessor->x87  ? EXEFACTS_ACCESSOR_FLOAT
                             : accessor->load ? EXEFACTS_ACCESSOR_LOAD
                                              : EXEFACTS_ACCESSOR_ADDRESS;
            *out = {uint8_t(kind), accessor->width, accessor->extension, accessor->base, accessor->offset,
                    accessor->pops};
        }
    });
}

int exefacts_empty_function(ExefactsContext* context, uint32_t address, ExefactsEmpty* out) {
    return answer(context, [&] {
        std::optional<uint32_t> pops = context->facts.emptyFunction(address);
        *out = {uint8_t(pops.has_value()), pops.value_or(0)};
    });
}

}  // extern "C"
