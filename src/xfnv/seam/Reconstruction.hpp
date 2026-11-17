#pragma once

// The seam between reconstructed code and the original executable.
//
// Hybrid build (FNVD_HYBRID, the 32-bit Windows DLL loaded into the original FalloutNV.exe):
//   - engine globals are the exe's own variables (FNVD_GLOBAL),
//   - a function that is not reconstructed yet is called at its original address (fnvd::original),
//   - every reconstructed function names the original it replaces (FNVD_REPLACES): the shim routes the exe's own
//     calls to it, and in verify mode the checker runs both on the same state and compares them.
// Standalone build (64-bit Linux, and FNVD_NO_EXE: the same 32-bit Windows target without the exe): globals are
// ordinary variables, calling a function that is not reconstructed yet stops the program with its address,
// FNVD_REPLACES only checks that the named function exists.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
// __m128: the 16-byte SSE vector IDA types the exe's packed-single code and its 16-byte pointer parameters with
// (hkVector4 and friends). The compiler's own type, spelled as IDA does.
#include <xmmintrin.h>

// A 64-bit target (the x64 program): for the few places where a pointer-sized value cannot keep the exe's 32-bit
// form (code only that build needs). The 32-bit builds never see its side.
#if INTPTR_MAX == INT64_MAX
#define FNVD_64BIT 1
#else
#define FNVD_64BIT 0
#endif

#if defined(_WIN32) && defined(_M_IX86)
// FNVD_NO_EXE (CMake option): the 32-bit Windows program without the original exe. The seam's forms take their
// standalone side. The program has no calling convention of its own: the Windows ABI at its boundary is spelled
// with the SDK's names (WINAPI, CALLBACK, STDMETHODCALLTYPE), defined in src/SDK/Windows.
// On a body whose exe copy has no /GS cookie: clang-cl's /GS check would leave the cookie in eax (and add a frame
// the exe lacks). Never use it where the exe body calls __security_check_cookie.
#define FNVD_NO_STACK_COOKIE __declspec(safebuffers)
#else
#define FNVD_NO_STACK_COOKIE
#endif

#define FNVD_CONCAT_(a, b) a##b
#define FNVD_CONCAT(a, b) FNVD_CONCAT_(a, b)

namespace fnvd {

    // Which registers carry a function's result, so the checker compares exactly those bits (a bool returned in al
    // leaves the rest of eax undefined).
    enum class ReturnKind : uint8_t {
        Void,
        Integer,
        Float,
    };

// The exe's layout of a type (sizes, member offsets). It binds only the hybrid build, where reconstructed and
// original code share objects; the standalone build lays types out for its own pointer size.
#define FNVD_LAYOUT(condition) static_assert(true)

// The exe address of a src function left unregistered on purpose (a KEEP body: an exe caller relies on something
// clang does not reproduce). It registers nothing in either build; tools bind the declaration to the address
// (srcindex `address` refs), and the build checks that the symbol exists: `FNVD_ADDRESS(0x403D30, ourMemset);`
// after the definition, with the function pointer type after the name for an overload, as FNVD_REPLACES_OVERLOAD.
// For a protected or private member, which namespace scope cannot name: inside the class, right after the member's
// declaration. An inline header body whose out-of-line exe copy has the address is tied the same way.
#define FNVD_ADDRESS(address, symbol, ...) static_assert(sizeof(__VA_OPT__(static_cast<__VA_ARGS__>)(&symbol)) != 0)

    // Where a usercall/userpurge function (FNVD_REPLACES_USERCALL) takes a parameter: one of the registers, in
    // fnvd::original::UserRegs' member order, or the stack.
    enum class UserLocation : uint8_t {
        eax,
        ecx,
        edx,
        ebx,
        esi,
        edi,
        stack,
    };

    struct Signature {
        uintptr_t code;
        ReturnKind returnKind;
        // bytes of the result in eax (1, 2, 4) or eax:edx (8); 0 for void and Float
        uint8_t returnBytes;
        // stack bytes the caller pushes, the hidden result pointer included (callees may reuse them as scratch)
        uint16_t argumentBytes;
        // A usercall/userpurge function's register arguments (FNVD_REPLACES_USERCALL): bit k for UserLocation k (eax
        // first); 0 for every standard convention. code is then the registration's adapter, entered with the exe's
        // registers and stack.
        uint8_t userRegisters = 0;
        // A userpurge: the function pops its argumentBytes (a usercall leaves them to the caller).
        bool userPurge = false;
    };

    // One reconstructed function and the original it replaces; the shim walks the list at start.
    struct Replacement {
        uint32_t original;
        Signature signature;
        const char* name;
        Replacement* next;

        Replacement(uint32_t original, Signature signature, const char* name) noexcept;
    };

    // An exe vtable (FNVD_VFTABLE): what a reconstructed constructor or destructor stores as an object's vptr, as the
    // exe's own does. The standalone build has no exe vtables (the address is 0 there).
    struct Vtable {
        uintptr_t address;
    };

    // Stores the vtable as the vptr of object (a class with a vtable, or the base subobject of the table:
    // `setVtable(static_cast<Base*>(pThis), X_vftable_Base)`).
    [[gnu::always_inline]] inline void setVtable(void* object, Vtable table) {
        *static_cast<const void**>(object) = reinterpret_cast<const void*>(table.address);
    }

    // A local of a class whose constructor and destructor the exe inlines (FNVD_INLINED_LOCAL): typed storage for which
    // C++ runs no constructor or destructor, storing the vptrs the exe's inlined ones store instead: atStart where it
    // is declared, atEnd where its scope ends; std::nullopt where the exe's inlined constructor or destructor stores
    // none. Or a local the exe constructs and destroys with calls of the class's Construct and Destruct
    // (FNVD_CONSTRUCTED_LOCAL): no vptr stores; atEnd, the class's Destruct (nullptr for none), called where its scope
    // ends.
    template <class T>
    class InlinedLocal {
    public:
        using Destructor = void (*)(T*);

        InlinedLocal(std::optional<Vtable> atStart, std::optional<Vtable> atEnd) : m_atEnd(atEnd) {
            if (atStart) {
                setVtable(m_bytes, *atStart);
            }
        }
        explicit InlinedLocal(Destructor atEnd) : m_destruct(atEnd) {}
        InlinedLocal(const InlinedLocal&) = delete;
        InlinedLocal& operator=(const InlinedLocal&) = delete;
        ~InlinedLocal() {
            if (m_atEnd) {
                setVtable(m_bytes, *m_atEnd);
            }
            if (m_destruct) {
                m_destruct(&object());
            }
        }

        T& object() {
            return *reinterpret_cast<T*>(m_bytes);
        }

    private:
        alignas(T) unsigned char m_bytes[sizeof(T)];
        std::optional<Vtable> m_atEnd;
        Destructor m_destruct = nullptr;
    };

    // `type name` whose constructor and destructor the exe inlines down to their vptr stores (atStart, atEnd:
    // FNVD_VFTABLE constants or std::nullopt), the body writing the rest of them by hand (its EXE-INLINED steps).
    // Natively it is the same raw typed storage, so the program runs exactly the body's statements, as the exe does:
    // no destructor runs at the scope's end, and the only construction is the class's default constructor where atStart
    // names a vtable (it gives the object the class's own vptr: the exe's vtable constants are 0 natively; atEnd stores
    // nothing). The body constructs and destroys it with FNVD_CONSTRUCT and FNVD_DESTRUCT, never FNVD_RECONSTRUCT or
    // FNVD_DESTRUCT_IN_PLACE, whose native forms assume an object C++ constructed and destroys. At parity the form
    // becomes `type name;` with the hand-inlined steps (the EXE-INLINED lines) removed.
    template <class T>
    class NativeInlinedLocal {
    public:
        template <class E>
        NativeInlinedLocal(std::nullopt_t, E) {}
        template <class E>
        NativeInlinedLocal(Vtable, E) {
            ::new (static_cast<void*>(m_bytes)) T;
        }
        NativeInlinedLocal(const NativeInlinedLocal&) = delete;
        NativeInlinedLocal& operator=(const NativeInlinedLocal&) = delete;

        T& object() {
            return *reinterpret_cast<T*>(m_bytes);
        }

    private:
        alignas(T) unsigned char m_bytes[sizeof(T)];
    };
#define FNVD_INLINED_LOCAL(type, name, atStart, atEnd)                                                                 \
    ::fnvd::NativeInlinedLocal<type> name##Storage_{atStart, atEnd};                                                   \
    type& name = name##Storage_.object()

// `type name(args...)`, which the exe constructs with `type::Construct(&name, args...)` here and destroys with
// `type::Destruct(&name)` where its scope ends (destruct: `&type::Destruct`; nullptr where the exe destroys nothing,
// the class's destructor being trivial): the macro makes both calls, on typed storage for which C++ runs no
// constructor or destructor (no vptr stores of its own). At parity, when Construct and Destruct become the class's
// real constructor and destructor: `type name(args...);`, which is the standalone side (destruct not evaluated).
#define FNVD_CONSTRUCTED_LOCAL(type, name, destruct, ...) type name __VA_OPT__((__VA_ARGS__))

// Calls of a class's Construct, Destruct and DeletingDestruct statics (declarations.md "Constructors and
// destructors") in code both builds compile. The type is the macro's argument (a template id with commas needs no
// alias); the call's own arguments follow: `FNVD_CONSTRUCT(NiNode)(pMemory, 4)`. The hybrid side is the static's
// call, token for token: `NiNode::Construct(pMemory, 4)`. The standalone side is the C++ the call stands for,
// through the templates below. The _AS forms name an exe copy of the static under another name
// (`FNVD_CONSTRUCT_AS(Construct_006C73B0, X)(p)` is `X::Construct_006C73B0(p)`); standalone they are the plain forms.
//   FNVD_CONSTRUCT(T)(memory, args...)            construct a T in raw memory
//   FNVD_RECONSTRUCT(T)(&object, args...)         construct an object that already exists again in place
//   FNVD_DESTRUCT(T)(p)                           destroy *p
//   FNVD_DESTRUCT_IN_PLACE(T)(&object)            the exe destroys an object its scope or owner destroys in C++
//   FNVD_DELETE(T)(p, flags)                      the deleting destructor: destroy *p, free it when flags & 1; with
//                                                 flags & 2 (the exe's vector deleting destructor) p is an array
#define FNVD_CONSTRUCT(...) ::fnvd::construct<__VA_ARGS__>
#define FNVD_CONSTRUCT_AS(name, ...) ::fnvd::construct<__VA_ARGS__>
#define FNVD_RECONSTRUCT(...) ::fnvd::reconstruct<__VA_ARGS__>
#define FNVD_RECONSTRUCT_AS(name, ...) ::fnvd::reconstruct<__VA_ARGS__>
#define FNVD_DESTRUCT(...) ::fnvd::destruct<__VA_ARGS__>
#define FNVD_DESTRUCT_AS(name, ...) ::fnvd::destruct<__VA_ARGS__>
#define FNVD_DESTRUCT_IN_PLACE(...) ::fnvd::destructInPlace<__VA_ARGS__>
#define FNVD_DESTRUCT_IN_PLACE_AS(name, ...) ::fnvd::destructInPlace<__VA_ARGS__>
#define FNVD_DELETE(...) ::fnvd::deletingDestruct<__VA_ARGS__>
#define FNVD_DELETE_AS(name, ...) ::fnvd::deletingDestruct<__VA_ARGS__>

    // FNVD_CONSTRUCT: at parity `new (memory) T(args...)`, or `new T(args...)` where the exe allocated the memory just
    // before. The memory is the address as passed, as Construct's `void*` takes it. With no arguments the object is
    // default-initialised (`new (memory) T`): the exe's constructor runs its own stores and nothing else, where `T()`
    // would value-initialise a T without a user-provided default constructor and zero it first, wiping what was stored
    // before (a Havok object's m_memSizeAndFlags, set by newObject).
    template <class T, class... A>
    inline T* construct(void* memory, A&&... args) {
        if constexpr (sizeof...(A) == 0) {
            return ::new (memory) T;
        }
        else {
            return ::new (memory) T(std::forward<A>(args)...);
        }
    }

    // FNVD_RECONSTRUCT: the object exists already (C++ constructed it, as a local or a member), so it is destroyed and
    // constructed again. At parity the call goes: the object's own declaration or its owner's initializer list
    // constructs it with these arguments. With no arguments default-initialised, as FNVD_CONSTRUCT.
    template <class T, class... A>
    inline T* reconstruct(void* object, A&&... args) {
        T* pObject = static_cast<T*>(object);
        pObject->~T();
        if constexpr (sizeof...(A) == 0) {
            return ::new (object) T;
        }
        else {
            return ::new (object) T(std::forward<A>(args)...);
        }
    }

    // FNVD_DESTRUCT: at parity `p->~T()`, or `delete p` where the exe frees the memory just after.
    template <class T>
    inline void destruct(T* object) {
        object->~T();
    }

    // FNVD_DESTRUCT_IN_PLACE: nothing; C++ destroys the object where its scope or its owner ends. At parity the call
    // goes.
    template <class T>
    inline void destructInPlace(T*) {}

    // FNVD_DELETE: at parity `delete p` (flags 1) or `p->~T()` (flags 0). Flags 2 is the exe's vector deleting
    // destructor (`??_E`): p is the first item of an array whose count the exe keeps in the dword before it, the items
    // are destroyed last to first, and the block (at p - 4) is freed where flags & 1. Standalone the array comes from
    // `new T[n]`: flags 3 is `delete[] p`; flags 2 destroys the items in place, their count read from the cookie the
    // Itanium C++ ABI stores in the size_t just before the array (a T with a trivial destructor has none, nor needs
    // one).
    template <class T>
    inline T* deletingDestruct(T* object, uint32_t flags) {
        if (flags & 2) {
            if (flags & 1) {
                delete[] object;
            }
            else if constexpr (!std::is_trivially_destructible_v<T>) {
                for (size_t uiIndex = reinterpret_cast<const size_t*>(object)[-1]; uiIndex > 0; --uiIndex) {
                    object[uiIndex - 1].~T();
                }
            }
        }
        else if (flags & 1) {
            delete object;
        }
        else {
            object->~T();
        }
        return object;
    }

    // The array cookie of MSVC's new[] for an element type with a destructor: the element count in one size_t slot
    // right before the array (4 bytes on x86, as the exe has it; 8 on x64, as MSVC x64 lays it out). arrayCount(p)
    // reads the count of the array p; arrayBlock(p) is the block new[] allocated, which delete[] frees. They stand for
    // `new T[n]`/`delete[] p` in the hand-written cookie code until that code goes at parity.
    inline size_t& arrayCount(void* array) {
        return static_cast<size_t*>(array)[-1];
    }
    inline void* arrayBlock(void* array) {
        return static_cast<size_t*>(array) - 1;
    }

// A dynamic_cast in code both builds compile (declarations.md "Dynamic casts"): the exe's call of its CRT's
// __RTDynamicCast (SDK/MSVC/rtti.h) with the type descriptors of the source and destination classes (BSCore/RTTI.h).
// The destination class is the macro's argument (a template id with commas needs no alias); the object and the two
// descriptors follow in their own parentheses: `FNVD_DYNAMIC_CAST(TESNPC)(pForm, RTTI_TESForm, RTTI_TESNPC)`. The
// hybrid side is the call, token for token: `static_cast<TESNPC*>(rt::__RTDynamicCast(pForm, 0, &RTTI_TESForm,
// &RTTI_TESNPC, 0))`. FNVD_DYNAMIC_CAST_VOID is the call alone, where the exe's void* result is used as it is. The
// standalone side is C++'s dynamic_cast (the descriptors are not evaluated), which both forms become when the
// hybrid goes: `dynamic_cast<T*>(object)`.
//   FNVD_DYNAMIC_CAST(T)(object, from, to)        the cast to T*
//   FNVD_DYNAMIC_CAST_VOID(T)(object, from, to)   the cast to T*, as a void*
#define FNVD_DYNAMIC_CAST(...) ::fnvd::dynamicCast<__VA_ARGS__> FNVD_DYNAMIC_CAST_OBJECT_
#define FNVD_DYNAMIC_CAST_VOID(...) ::fnvd::dynamicCastVoid<__VA_ARGS__> FNVD_DYNAMIC_CAST_OBJECT_
#define FNVD_DYNAMIC_CAST_OBJECT_(object, from, to) (object)

    // The exe's call tests at run time what the object is; C++'s dynamic_cast of an object whose static type already
    // is T (or derived from T) is an identity that tests nothing, so the native side refuses it. The operand has the
    // from descriptor's class: retype the variable to it, or static_cast the operand to it at the cast.
    template <class T, class S>
    constexpr bool dynamicCastTests = !std::is_base_of_v<std::remove_cv_t<T>, std::remove_cv_t<S>>;

    // FNVD_DYNAMIC_CAST: at parity `dynamic_cast<T*>(object)`.
    template <class T, class S>
    inline T* dynamicCast(S* object) {
        static_assert(
            dynamicCastTests<T, S>,
            "FNVD_DYNAMIC_CAST of an object whose static type is already T or derived "
            "from it: natively an identity that tests nothing. Give the operand the from descriptor's class (retype "
            "the "
            "variable, or static_cast<From*> it at the cast)."
        );
        return dynamic_cast<T*>(object);
    }

    // FNVD_DYNAMIC_CAST_VOID: at parity `dynamic_cast<T*>(object)`; the void* keeps the type the exe's call returns.
    template <class T, class S>
    inline void* dynamicCastVoid(S* object) {
        static_assert(
            dynamicCastTests<T, S>,
            "FNVD_DYNAMIC_CAST_VOID of an object whose static type is already T or "
            "derived from it: natively an identity that tests nothing. Give the operand the from descriptor's class "
            "(retype the variable, or static_cast<From*> it at the cast)."
        );
        return dynamic_cast<T*>(object);
    }

// A throw of a VC9 exception object in code both builds compile (declarations.md "Throws"). The hybrid side is the
// exe's call of its CRT's _CxxThrowException (SDK/MSVC/ehdata.h) with the object's address and the `_ThrowInfo`
// record the compiler emitted for its type, token for token: `FNVD_THROW(kError, rt::length_error_ThrowInfo)` is
// `rt::_CxxThrowException(static_cast<void*>(&kError), &rt::length_error_ThrowInfo)`. The records are
// compiler data, never defined in src, so the standalone side does not name them: it is C++'s throw of the object,
// `throw (kError)`, which the form becomes when the hybrid goes.
#define FNVD_THROW(object, throwInfo) throw(object)

// Binds the reference member `member` of `*object` to `referent` in a Construct static, where the exe's constructor
// stores the referent's address: C++ binds a reference member only in a constructor's initializer list, and reading
// it back (`&object->member`) gives that address. At parity, when Construct becomes the class's real constructor:
// the initializer `member(referent)`.
#define FNVD_BIND_REFERENCE(object, member, referent)                                                                  \
    (*reinterpret_cast<std::remove_reference_t<decltype((object)->member)>**>(                                         \
         reinterpret_cast<unsigned char*>(object) + offsetof(std::remove_pointer_t<decltype(object)>, member)          \
     ) = &(referent))

    namespace original {

        // An argument of a C-variadic engine function after the default argument promotions a `...` call applies (float
        // to double, integers narrower than int to int). The forwarders below call through fixed prototypes, so a
        // forwarder of a variadic function passes each of its variadic arguments through this.
        template <class T>
        constexpr auto promoted(T value) {
            static_assert(std::is_trivially_copyable_v<T>, "a C variadic argument is trivially copyable");
            if constexpr (std::is_same_v<T, float>) {
                return static_cast<double>(value);
            }
            else if constexpr (std::is_integral_v<T> && sizeof(T) < sizeof(int)) {
                return static_cast<int>(value);
            }
            else {
                return value;
            }
        }

    } // namespace original

#define FNVD_REPLACES(address, function) static_assert(sizeof(&function) != 0)
#define FNVD_REPLACES_OVERLOAD(address, function, ...) static_assert(sizeof(static_cast<__VA_ARGS__>(&function)) != 0)
// FnvdAccess re-declares the member public, as in the hybrid adapter, so protected members check too. `entry` is the
// class whose `this` the exe passes (type itself, or FNVD_REPLACES_VIRTUAL_VIA's base): it must be a base of type that
// converts down to type. The check sits inside FnvdAccess (derived from type), as the hybrid adapter's cast does, so
// a protected base of type (Havok's listener bases) converts too.
#define FNVD_REPLACES_VIRTUAL_(type, entry, member, ...)                                                               \
    namespace {                                                                                                        \
        struct FNVD_CONCAT(FnvdVirtual, __LINE__) {                                                                    \
            struct FnvdAccess : type {                                                                                 \
                using type::member;                                                                                    \
                static_assert(sizeof(static_cast<type*>(static_cast<entry*>(nullptr))) != 0);                          \
            };                                                                                                         \
            static_assert(sizeof(__VA_ARGS__) != 0);                                                                   \
        };                                                                                                             \
    }
#define FNVD_REPLACES_VIRTUAL(address, type, member) FNVD_REPLACES_VIRTUAL_(type, type, member, &FnvdAccess::member)
#define FNVD_REPLACES_MEMBER(address, type, member, ...)                                                               \
    FNVD_REPLACES_VIRTUAL_(type, type, member, __VA_OPT__(static_cast<__VA_ARGS__>)(&FnvdAccess::member))
#define FNVD_REPLACES_VIRTUAL_OVERLOAD(address, type, member, ...)                                                     \
    FNVD_REPLACES_VIRTUAL_(type, type, member, static_cast<__VA_ARGS__>(&FnvdAccess::member))
#define FNVD_REPLACES_VIRTUAL_VIA(address, type, base, member)                                                         \
    FNVD_REPLACES_VIRTUAL_(type, base, member, &FnvdAccess::member)
#define FNVD_REPLACES_VIRTUAL_VIA_OVERLOAD(address, type, base, member, ...)                                           \
    FNVD_REPLACES_VIRTUAL_(type, base, member, static_cast<__VA_ARGS__>(&FnvdAccess::member))
#define FNVD_REPLACES_CONSTRUCTOR(address, type, ...)                                                                  \
    static_assert(std::is_constructible_v<type __VA_OPT__(, ) __VA_ARGS__>)
#define FNVD_REPLACES_DESTRUCTOR(address, type) static_assert(std::is_destructible_v<type>)
#define FNVD_REPLACES_CONSTRUCT(address, function) static_assert(sizeof(&function) != 0)
#define FNVD_REPLACES_CONSTRUCT_OVERLOAD(address, function, ...)                                                       \
    static_assert(sizeof(static_cast<__VA_ARGS__>(&function)) != 0)
#define FNVD_REPLACES_DESTRUCT(address, function) static_assert(sizeof(&function) != 0)
#define FNVD_REPLACES_DELETING_DESTRUCT(address, function) static_assert(sizeof(&function) != 0)
    // The function and the location names only checked.
    template <class... L>
    constexpr bool userLocations(L...) {
        return (std::is_same_v<L, UserLocation> && ...);
    }
#define FNVD_REPLACES_USER_(function, ...)                                                                             \
    namespace {                                                                                                        \
        struct FNVD_CONCAT(FnvdUsercall, __LINE__) {                                                                   \
            using enum ::fnvd::UserLocation;                                                                           \
            static_assert(::fnvd::userLocations(__VA_ARGS__) && sizeof(&function) != 0);                               \
        };                                                                                                             \
    }
#define FNVD_REPLACES_USERCALL(address, function, ...) FNVD_REPLACES_USER_(function, __VA_ARGS__)
#define FNVD_REPLACES_USERPURGE(address, function, ...) FNVD_REPLACES_USER_(function, __VA_ARGS__)
#define FNVD_VFTABLE(name, address) inline constexpr ::fnvd::Vtable name{0}

// The plain declarations a definition (FNVD_DEFINE_DATA, FNVD_DEFINE_GLOBAL) completes in its home source file.
#define FNVD_GLOBAL(type, name, address) extern type name
#define FNVD_STATIC_MEMBER(type, name, address) static type name

    // FNVD_IMPORT's export: the function type the slot's pointer type points to, its calling convention kept, with C
    // linkage (a stdcall export links as `_Name@N`), imported from its DLL on Windows. FNVD_IMPORT_AS: a callable that
    // calls the export at the line's type. It is a constant object that fetches the address at each call, not a pointer
    // variable: a dllimport address is no constant, so a pointer would be initialised dynamically, and a static
    // constructor of another file (MemoryManager's, through BSSpinLock) can call it before that, through null. ImportAs
    // is defined for the SDK's WINAPI pointer types, with the SDK's names (src/SDK/Windows/windef.h).
    template <class T>
    using FunctionOf = std::remove_pointer_t<T>;
    template <class T>
    struct ImportAs;
#ifdef _WIN32
#define FNVD_IMPORT(type, name, address) extern "C" __declspec(dllimport) ::fnvd::FunctionOf<type> name
#else
#define FNVD_IMPORT(type, name, address) extern "C" ::fnvd::FunctionOf<type> name
#endif
#define FNVD_IMPORT_AS(type, name, address, Export)                                                                    \
    inline constexpr ::fnvd::ImportAs<type> name {                                                                     \
        []() { return reinterpret_cast<type>(&Export); }                                                               \
    }
#define FNVD_IMPORT_STUB(type, name, address) extern "C" ::fnvd::FunctionOf<type> name

    namespace detail {

        // FNVD_ORIGINAL_FUNCTION's value: the named function at the line's pointer type (which picks an overload), a
        // null pointer when the line names none.
        template <class F>
        struct OriginalFunction {
            static constexpr F of(F apFunction = nullptr) {
                return apFunction;
            }
        };

        // An untyped line (`void*`): the function's address, converted.
        template <>
        struct OriginalFunction<void*> {
            static constexpr void* of() {
                return nullptr;
            }
            template <class G>
            requires std::is_function_v<G>
            static void* of(G* apFunction) {
                return reinterpret_cast<void*>(apFunction);
            }
        };

        // The class a pointer to member belongs to (callMember's object type).
        template <class M>
        struct MemberClass {};
        template <class T, class C>
        struct MemberClass<T C::*> {
            using type = C;
        };

        // One callFunction argument at the function's parameter type P: a pointer the line passes for a reference
        // parameter (the exe's ABI is the same), typed or a (const) void* such as an eh-vector copy callback's source,
        // is cast to the referee's pointer and dereferenced; anything else is static_cast. aArg is the caller's own
        // parameter, so a reference P bound to it lives through the call.
        template <class P, class A>
        P convertArgument(A& aArg) {
            using Arg = std::remove_reference_t<A>;
            if constexpr (
                std::is_lvalue_reference_v<P> && std::is_pointer_v<Arg> &&
                (std::is_void_v<std::remove_pointer_t<Arg>> || std::is_convertible_v<Arg, std::remove_reference_t<P>*>)
            ) {
                return *static_cast<std::remove_reference_t<P>*>(aArg);
            }
            else {
                return static_cast<P>(aArg);
            }
        }

        // A plain function pointer's call with each argument converted to its own parameter type (callFunction).
        template <class F>
        struct FunctionCall {};
        template <class R, class... P>
        struct FunctionCall<R (*)(P...)> {
            template <class... A>
            requires(sizeof...(A) == sizeof...(P))
            static R call(R (*apFunction)(P...), A... aArgs) {
                return apFunction(convertArgument<P, A>(aArgs)...);
            }
        };

    } // namespace detail

    // The native sides of FNVD_ORIGINAL_FUNCTION lines whose exe function src has no plain function of the line's
    // type: each is named with its class or member (`fnvd::constructElement<X>`, `fnvd::callMember<&X::f>`) and takes
    // the rest of its parameters from the line's pointer type when the macro takes its address.

    // A callback that constructs one object at its argument (an eh-vector or array constructor callback, the exe's
    // X::Construct or default constructor): `::new (memory) T`, default-initialised as the exe's constructor leaves it
    // (no zeroing first), returning the object when the line's type returns a pointer. At parity the line goes and its
    // caller builds the objects in C++ (`new T[n]`, a std::vector<T>).
    template <class T, class R, class P>
    requires std::is_pointer_v<P> && (std::is_void_v<R> || std::is_pointer_v<R>)
    R constructElement(P apMemory) {
        T* pObject = ::new (static_cast<void*>(static_cast<T*>(apMemory))) T;
        if constexpr (!std::is_void_v<R>) {
            return pObject;
        }
    }

    // A callback that destroys one object at its argument (an eh-vector or array destructor callback, the exe's
    // X::Destruct or destructor): T's own destructor, not a virtual call, returning the object when the line's type
    // returns a pointer. At parity the line goes and its caller destroys the objects in C++ (`delete[]`, the
    // container).
    template <class T, class R, class P>
    requires std::is_pointer_v<P> && (std::is_void_v<R> || std::is_pointer_v<R>)
    R destructElement(P apObject) {
        T* pObject = static_cast<T*>(apObject);
        pObject->T::~T();
        if constexpr (!std::is_void_v<R>) {
            return pObject;
        }
    }

    // A non-static member the exe passes as a plain callback: calls it on the first argument (converted to the
    // member's class), the rest forwarded, its result converted to the line's return type. At parity the line goes and
    // the callback becomes a lambda or a pointer to member (`[](X* p, A a) { return p->f(a); }`).
    template <auto Member, class R, class P, class... A>
    requires std::is_member_function_pointer_v<decltype(Member)> && std::is_pointer_v<P>
    R callMember(P apThis, A... aArgs) {
        using C = typename detail::MemberClass<decltype(Member)>::type;
        if constexpr (std::is_void_v<R>) {
            (static_cast<C*>(apThis)->*Member)(aArgs...);
        }
        else {
            return static_cast<R>((static_cast<C*>(apThis)->*Member)(aArgs...));
        }
    }

    // A plain function (a static member included) the exe calls through a table or callback type that differs from its
    // own only in its parameter types (a widened `int32_t` flag the exe's dispatcher passes on whole, for the
    // function's `bool`; an eh-vector callback's `void*` for the function's `X*`; an `X*` for the function's `X&`):
    // calls it with the line's arguments, each converted to the function's parameter type (convertArgument), its result
    // converted to the line's return type (discarded for void). The template argument may be an overload picked with a
    // static_cast (`fnvd::callFunction<static_cast<F>(&X::f)>`). At parity the line goes and the table or caller takes
    // the function itself at its own type.
    template <auto Function, class R, class... A>
    requires std::is_pointer_v<decltype(Function)> && std::is_function_v<std::remove_pointer_t<decltype(Function)>>
    R callFunction(A... aArgs) {
        // The line's parameter types passed on as they are: a reference stays a reference (no copy of an abstract
        // class).
        using Call = detail::FunctionCall<decltype(Function)>;
        if constexpr (std::is_void_v<R>) {
            Call::template call<A...>(Function, aArgs...);
        }
        else {
            return static_cast<R>(Call::template call<A...>(Function, aArgs...));
        }
    }

#define FNVD_ORIGINAL_FUNCTION(type, name, address, ...)                                                               \
    inline type const name = ::fnvd::detail::OriginalFunction<type>::of(__VA_OPT__(&__VA_ARGS__))

} // namespace fnvd

// Definitions: one per global, in its home source file (the .cpp whose object the exe's data and initialiser come
// from), in the exe's order within that file: its .data, its zero-filled .bss, then the globals its dynamic
// initialisers construct, in the order of the exe's initialiser table. Each line stands for one plain C++
// definition, which is what the standalone build compiles; the hybrid build keeps the exe's object and its
// initialiser, and the DLL runs and holds nothing of a definition.
//
// FNVD_DEFINE_DATA(type, name, address, init...): data the exe's image holds as it is, a constant or zero.
//   Standalone `type name{init...};` (no init: zero). Hybrid: the value is compiled against the declared type,
//   nothing is emitted; compiled with FNVD_DATACHECK (harness/tools/datacheck.py) the value is also the constant
//   fnvdData_<address>, whose bytes and relocations the data check compares with the exe's.
// FNVD_DEFINE_GLOBAL(type, name, address, args...): a global the exe constructs in a dynamic initialiser.
//   Standalone `type name(args...);` (no args: `type name;`), a scalar's too (`float f(x * 255.0);` is `float f =
//   x * 255.0;`). Hybrid: fnvdInit_<address> does what the exe's initialiser does, fnvdExit_<address> what its atexit
//   thunk does; neither is called or emitted in the DLL. checkbody compares fnvdInit_<address> with the exe's
//   initialiser of the global and fnvdExit_<address> with its atexit thunk: a PASS proves the arguments and the
//   construction the exe's initialiser has, never what the constructor itself does (its own row checks that).
//   - fnvdInit_ constructs through the class's own Construct (the exe's out-of-line constructor; one found in a base
//     class does not compile), else by C++ construction in place: a scalar's store, an inline constructor, and with
//     no args default-initialisation. A global no code initialises (no args, trivially default-constructible, no
//     Construct, nothing to destroy) is a zero data line and does not compile as this form. It then registers
//     fnvdExit_ with the exe CRT's atexit where the class has its own Destruct or a non-trivial C++ destructor.
//   - fnvdExit_ calls the class's own Destruct, else its destructor non-virtually, as the exe's thunk does.
//   - The two builds resolve the args against different overload sets: the hybrid against the class's Construct
//     statics, standalone against its C++ constructors. A PASS proves the Construct the hybrid picked; that
//     standalone picks the matching constructor is the constructors' work.
//   - The args are evaluated with source literal types (`-0.5f` is a float, not the hybrid build's double), as the
//     exe's compiler types them.
//   - An arg that reads as a type (`Bar()`) turns standalone's line into a function declaration: never write one.
// FNVD_DEFINE_LOCAL_DATA / FNVD_DEFINE_LOCAL_GLOBAL: the same for a global the file declares for itself (internal
//   linkage): they also declare it, the hybrid build's the exe's object at its address.
// A static member is defined from namespace scope in the hybrid (decltype, its address): it must be public.
// FNVD_FUNCTION_STATIC(type, name, address, guard, mask, atexit, args...) in a body, where the original had
//   `static type name = init;`: standalone that line (no args: `static type name;`; several: `static type
//   name(args...);`). Hybrid: name is the exe's object, and the line is what the exe's code does there: when the
//   function's guard word at `guard` lacks `mask` (1, 2, 4...: the bit the exe tests), it sets it, constructs the
//   object as fnvdInit_ does, and registers `atexit`, the exe's atexit thunk for it (0 when the exe registers none;
//   a value that disagrees with the type's destructor does not compile). The body's checkbody verdict covers it.
//   The thunk stays the exe's.
// FNVD_TU_STATIC(type, name, init...) in a header: a header static, one copy per file that includes it, as in the
//   exe. Standalone the original's `static type name{init...};`. Hybrid: name is declared, and the init is compiled
//   into fnvdTu_<name>, never emitted, so a typo fails to compile. FNVD_TU_COPY(type, name, address) in a .cpp:
//   standalone nothing; hybrid name is that file's own copy in the exe, and fnvdInit_<address> stores the init into
//   it (fnvdExit_<address> as above) for checkbody against that copy's initialiser. A header inline that reads the
//   static compiles in every file; one emitted in a file without a FNVD_TU_COPY line for it fails to link (the
//   name, internal to the file, is undefined there).
// A global the exe builds in a way none of these says (an array constructed element by element, a constructor the
// exe inlines, several globals in one initialiser) is written by hand: `#if FNVD_HYBRID` the init function, tied to
// the exe's initialiser with FNVD_ADDRESS, `#else` the plain definition.
// A type with a comma, or an array type, needs a `using` alias named by what it holds (`StepSoundNameTable`), as for
// FNVD_GLOBAL.

// The standalone initialiser of a function static: none, `= init` for one, `(args...)` for several (at most 8).
#define FNVD_STATIC_INIT_PICK_(_1, _2, _3, _4, _5, _6, _7, _8, picked, ...) picked
#define FNVD_STATIC_INIT_(...)                                                                                         \
    __VA_OPT__(FNVD_STATIC_INIT_PICK_(                                                                                 \
        __VA_ARGS__, (__VA_ARGS__), (__VA_ARGS__), (__VA_ARGS__), (__VA_ARGS__), (__VA_ARGS__), (__VA_ARGS__),         \
        (__VA_ARGS__), = __VA_ARGS__,                                                                                  \
    ))

#define FNVD_DEFINE_DATA(type, name, address, ...)                                                                     \
    type name {                                                                                                        \
        __VA_ARGS__                                                                                                    \
    }
#define FNVD_DEFINE_GLOBAL(type, name, address, ...) type name __VA_OPT__((__VA_ARGS__))
#define FNVD_DEFINE_LOCAL_DATA(type, name, address, ...)                                                               \
    static type name {                                                                                                 \
        __VA_ARGS__                                                                                                    \
    }
#define FNVD_DEFINE_LOCAL_GLOBAL(type, name, address, ...) static type name __VA_OPT__((__VA_ARGS__))
#define FNVD_FUNCTION_STATIC(type, name, address, guard, mask, atexit, ...)                                            \
    static type name FNVD_STATIC_INIT_(__VA_ARGS__)
#define FNVD_TU_STATIC(type, name, ...)                                                                                \
    static type name {                                                                                                 \
        __VA_ARGS__                                                                                                    \
    }
#define FNVD_TU_COPY(type, name, address) static_assert(true)
