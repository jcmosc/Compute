#pragma once

#include <CoreFoundation/CFBase.h>
#include <swift/Runtime/HeapObject.h>

#include "AGSwiftSupport.h"

namespace AG {

template <typename ReturnType, typename... Args> class ClosureFunction {
  public:
    using Context = const void *_Nullable;
    using Callable = AG_SWIFT_CC(swift) const ReturnType (*_Nullable)(Args..., Context AG_SWIFT_CONTEXT);

  private:
    Callable _function;
    Context _context;

  public:
    operator bool() { return _function != nullptr; }
    
    const ReturnType operator()(Args... args) const noexcept {
        // TODO: check _context is first or last parameter
        return _function(std::forward<Args>(args)..., _context);
    }

    void release_context() { ::swift::swift_release((::swift::HeapObject *)_context); };
};

// void *
template <typename ReturnType>
    requires std::is_pointer_v<ReturnType>
using ClosureFunctionVP = ClosureFunction<ReturnType>;

// void
template <typename ReturnType>
    requires std::same_as<ReturnType, void>
using ClosureFunctionVV = ClosureFunction<ReturnType>;

// unsigned long, AGUnownedGraphContext *
template <typename ReturnType, typename Arg>
    requires std::unsigned_integral<ReturnType>
using ClosureFunctionCI = ClosureFunction<ReturnType, Arg>;

// void, void *
template <typename ReturnType, typename Arg>
    requires std::same_as<ReturnType, void>
using ClosureFunctionPV = ClosureFunction<ReturnType, Arg>;

// void, unsigned int
template <typename ReturnType, typename Arg>
    requires std::same_as<ReturnType, void> && std::unsigned_integral<Arg>
using ClosureFunctionAV = ClosureFunction<ReturnType, Arg>;

// bool, unsigned int
template <typename ReturnType, typename Arg>
    requires std::same_as<ReturnType, bool> && std::unsigned_integral<Arg>
using ClosureFunctionAB = ClosureFunction<ReturnType, Arg>;

} // namespace AG
