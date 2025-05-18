#pragma once

#include <CoreFoundation/CFBase.h>
#include <swift/Runtime/HeapObject.h>

#include "AGSwiftSupport.h"

namespace AG {

template <typename ReturnType, typename... Args> class ClosureFunction {
  public:
    using Context = const void *_Nullable;
    using Function = AG_SWIFT_CC(swift) ReturnType (*_Nullable)(Context AG_SWIFT_CONTEXT, Args...);

  private:
    Function _function;
    Context _context;

  public:
    inline ClosureFunction(Function function, Context context) noexcept : _function(function), _context(context) {
        ::swift::swift_retain((::swift::HeapObject *)_context);
    }
    inline ClosureFunction() : _function(), _context() {}
    inline ClosureFunction(std::nullptr_t) : _function(nullptr), _context(nullptr) {}
    inline ~ClosureFunction() { ::swift::swift_release((::swift::HeapObject *)_context); }

    explicit operator bool() { return _function != nullptr; }

    const ReturnType operator()(Args... args) const noexcept {
        return _function(_context, std::forward<Args>(args)...);
    }
};

template <typename ReturnType>
    requires std::is_pointer_v<ReturnType>
using ClosureFunctionVP = ClosureFunction<ReturnType>;

template <typename ReturnType>
    requires std::same_as<ReturnType, void>
using ClosureFunctionVV = ClosureFunction<ReturnType>;

template <typename ReturnType, typename Arg>
    requires std::unsigned_integral<ReturnType>
using ClosureFunctionCI = ClosureFunction<ReturnType, Arg>;

template <typename ReturnType, typename Arg>
    requires std::same_as<ReturnType, void>
using ClosureFunctionPV = ClosureFunction<ReturnType, Arg>;

template <typename ReturnType, typename Arg>
    requires std::same_as<ReturnType, void> && std::unsigned_integral<Arg>
using ClosureFunctionAV = ClosureFunction<ReturnType, Arg>;

template <typename ReturnType, typename Arg>
    requires std::same_as<ReturnType, bool> && std::unsigned_integral<Arg>
using ClosureFunctionAB = ClosureFunction<ReturnType, Arg>;

} // namespace AG
