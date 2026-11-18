#pragma once

// What is left of the seam between the reconstructed code and the original executable. The hybrid build, a DLL loaded
// into the original FalloutNV.exe, is gone: each form below is the plain C++ it stands for, and goes as its uses are
// rewritten into that C++ (the "at parity" form each one names).

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
#define XFNV_64BIT 1
#else
#define XFNV_64BIT 0
#endif

namespace xfnv {

    // An exe vtable: the vptr the exe's inlined constructor stores in an XFNV_INLINED_LOCAL. There are no exe vtables
    // here: each constant is 0, and only says that the local starts with its class's own vptr.
    struct Vtable {
        uintptr_t address;
    };

    // `type name` whose constructor and destructor the exe inlines down to their vptr stores (atStart, atEnd: Vtable
    // constants or std::nullopt), the body writing the rest of them by hand (its EXE-INLINED steps). It is raw typed
    // storage, so the program runs exactly the body's statements, as the exe does: no destructor runs at the scope's
    // end, and the only construction is the class's default constructor where atStart names a vtable (it gives the
    // object the class's own vptr; atEnd stores nothing). At parity the form becomes `type name;` with the hand-inlined
    // steps (the EXE-INLINED lines) removed.
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
#define XFNV_INLINED_LOCAL(type, name, atStart, atEnd)                                                                 \
    ::xfnv::NativeInlinedLocal<type> name##Storage_{atStart, atEnd};                                                   \
    type& name = name##Storage_.object()

// What were calls of a class's Construct static, as the C++ each stands for: the type is the macro's argument (a
// template id with commas needs no alias), the call's own arguments follow: `XFNV_RECONSTRUCT(NiPoint3)(&kPoint, 0.0f,
// 0.0f, 0.0f)`. The _AS form named an exe copy of the static under another name, which it still carries; it is the
// plain form.
#define XFNV_RECONSTRUCT(...) ::xfnv::reconstruct<__VA_ARGS__>
#define XFNV_RECONSTRUCT_AS(name, ...) ::xfnv::reconstruct<__VA_ARGS__>

    // XFNV_RECONSTRUCT: the object exists already (C++ constructed it, as a local or a member), so it is destroyed and
    // constructed again. At parity the call goes: the object's own declaration or its owner's initializer list
    // constructs it with these arguments. With no arguments the object is default-initialised (`new (object) T`): the
    // exe's constructor runs its own stores and nothing else, where `T()` would value-initialise a T without a
    // user-provided default constructor and zero it first.
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

    // XFNV_IMPORT's export: the function type the slot's pointer type points to, its calling convention kept, with C
    // linkage (a stdcall export links as `_Name@N`), imported from its DLL on Windows. XFNV_IMPORT_AS: a callable that
    // calls the export at the line's type. It is a constant object that fetches the address at each call, not a pointer
    // variable: a dllimport address is no constant, so a pointer would be initialised dynamically, and a static
    // constructor of another file (MemoryManager's, through BSSpinLock) can call it before that, through null. ImportAs
    // is defined for the SDK's WINAPI pointer types, with the SDK's names (src/SDK/Windows/windef.h).
    template <class T>
    using FunctionOf = std::remove_pointer_t<T>;
    template <class T>
    struct ImportAs;
#ifdef _WIN32
#define XFNV_IMPORT(type, name, address) extern "C" __declspec(dllimport) ::xfnv::FunctionOf<type> name
#else
#define XFNV_IMPORT(type, name, address) extern "C" ::xfnv::FunctionOf<type> name
#endif
#define XFNV_IMPORT_AS(type, name, address, Export)                                                                    \
    inline constexpr ::xfnv::ImportAs<type> name {                                                                     \
        []() { return reinterpret_cast<type>(&Export); }                                                               \
    }
#define XFNV_IMPORT_STUB(type, name, address) extern "C" ::xfnv::FunctionOf<type> name

    // The adapters below are what a table or callback of the exe's type calls: each is named with its class or member
    // (`xfnv::constructElement<X>`, `xfnv::callMember<&X::f>`) and takes the rest of its parameters from the pointer
    // type its address is taken at.

    namespace detail {

        // The class a pointer to member belongs to (callMember's object type).
        template <class M>
        struct MemberClass {};
        template <class T, class C>
        struct MemberClass<T C::*> {
            using type = C;
        };

        // One callFunction argument at the function's parameter type P: a pointer the caller passes for a reference
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

    // A callback that constructs one object at its argument (an eh-vector or array constructor callback, the exe's
    // X::Construct or default constructor): `::new (memory) T`, default-initialised as the exe's constructor leaves it
    // (no zeroing first), returning the object when the callback type returns a pointer. At parity its caller builds
    // the objects in C++ (`new T[n]`, a std::vector<T>).
    template <class T, class R, class P>
    requires std::is_pointer_v<P> && (std::is_void_v<R> || std::is_pointer_v<R>)
    R constructElement(P apMemory) {
        T* pObject = ::new (static_cast<void*>(static_cast<T*>(apMemory))) T;
        if constexpr (!std::is_void_v<R>) {
            return pObject;
        }
    }

    // A callback that destroys one object at its argument (an eh-vector or array destructor callback, the exe's
    // X::Destruct or destructor): T's own destructor, not a virtual call, returning the object when the callback type
    // returns a pointer. At parity its caller destroys the objects in C++ (`delete[]`, the container).
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
    // member's class), the rest forwarded, its result converted to the callback type's return type. At parity the
    // callback becomes a lambda or a pointer to member (`[](X* p, A a) { return p->f(a); }`).
    template <auto Member, class R, class P, class... A>
    requires std::is_member_function_pointer_v<decltype(Member)> && std::is_pointer_v<P>
    R callMember(P apThis, A... aArgs) {
        using C = detail::MemberClass<decltype(Member)>::type;
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
    // calls it with the callback type's arguments, each converted to the function's parameter type (convertArgument),
    // its result converted to the callback type's return type (discarded for void). The template argument may be an
    // overload picked with a static_cast (`xfnv::callFunction<static_cast<F>(&X::f)>`). At parity the table or caller
    // takes the function itself at its own type.
    template <auto Function, class R, class... A>
    requires std::is_pointer_v<decltype(Function)> && std::is_function_v<std::remove_pointer_t<decltype(Function)>>
    R callFunction(A... aArgs) {
        // The callback type's parameter types passed on as they are: a reference stays a reference (no copy of an
        // abstract class).
        using Call = detail::FunctionCall<decltype(Function)>;
        if constexpr (std::is_void_v<R>) {
            Call::template call<A...>(Function, aArgs...);
        }
        else {
            return static_cast<R>(Call::template call<A...>(Function, aArgs...));
        }
    }

} // namespace xfnv
