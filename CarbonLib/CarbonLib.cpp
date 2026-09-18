// First darft for a CarbonLib to Darling Carbon.framework
// (c) 2026 Arucard1983
// Original licence maintains

#include <Carbon/Carbon.h>
#include <utility>
#include <tuple>
#include "NativeCall.h"
#include "VirtualMachine.h"

namespace CarbonLibEmulation {

    // 1. The Pass-Through Engine, it will unpack the PPC Stack, and calls Darling own implementation of Carbon.framework. 
    template <typename Ret, typename... Args, size_t... Is>
    Ret CallNativeImpl(void* funcPtr, PPCVM::ThreadContext& ctx, std::index_sequence<Is...>) {
        // Unpack the and convert all types by the correct order accordiling to the PowerPC ABI
        auto argsTuple = std::make_tuple(NativeCall::PopArgument<Args>(ctx)...);
        
        // Make the pointer cast obatained by Darling for the real C++ signature
        auto targetFunc = reinterpret_cast<Ret(*)(Args...)>(funcPtr);
        
        // Call the native function from Carbon.framework from the unpacked arguments 
        return targetFunc(std::get<Is>(argsTuple)...);
    }

    // 2. The passtrought stack 
    template <typename Ret, typename... Args>
    void CarbonForwarder(void* nativeFuncSymbol, PPCVM::ThreadContext& ctx) {
        if constexpr (std::is_void_v<Ret>) {
            CallNativeImpl<Ret, Args...>(nativeFuncSymbol, ctx, std::index_sequence_for<Args...>{});
        } else {
            Ret result = CallNativeImpl<Ret, Args...>(nativeFuncSymbol, ctx, std::index_sequence_for<Args...>{});
            NativeCall::PushResult(ctx, result); // Return the result to the PPCVM
        }
    }

    // 3. Forwarding Table, where C++ maintain type's coherence!
    struct CarbonMap {
        const char* name;
        void* nativeSymbolAddress;
        void (*forwarder)(void*, PPCVM::ThreadContext&);
    };

    // Using dlfcn (dlopen/dlsym) from Darling to fill the native pointers within a loop with two lines
    std::vector<CarbonMap> RegisterCarbonSymbols() {
        void* carbonFramework = dlopen("/System/Library/Frameworks/Carbon.framework/Carbon", RTLD_LAZY);
        
        // Where the static table is made, where it only needed the signature, and the compiler will made the rest! 
        return {
            { "MoveWindow", dlsym(carbonFramework, "MoveWindow"), &CarbonForwarder<void, WindowRef, int16_t, int16_t> },
            { "NewWindow",  dlsym(carbonFramework, "NewWindow"),  &CarbonForwarder<WindowRef, Rect*, ConstStr255Param, Boolean, int16_t, WindowRef, Boolean, int32_t> },
            { "ShowWindow", dlsym(carbonFramework, "ShowWindow"), &CarbonForwarder<void, WindowRef> }
            // TODO: Adding every new native Carbon.framework functions only required to use the template from the previous ones.
        };
    }
}
