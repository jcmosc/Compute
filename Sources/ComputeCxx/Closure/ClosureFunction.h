#pragma once

#include <CoreFoundation/CFBase.h>
#include <swift/Runtime/HeapObject.h>

#include "ComputeCxx/AGSwiftSupport.h"

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
        void *mutable_context = const_cast<void *>(_context);
        ::swift::swift_retain(reinterpret_cast<::swift::HeapObject *>(mutable_context));
    }

    inline ~ClosureFunction() {
        void *context = const_cast<void *>(_context);
        ::swift::swift_release(reinterpret_cast<::swift::HeapObject *>(context));
    }

    // Copyable

    ClosureFunction(const ClosureFunction &other) noexcept : _function(other._function), _context(other._context) {
        if (_context) {
            void *mutable_context = const_cast<void *>(_context);
            ::swift::swift_retain(reinterpret_cast<::swift::HeapObject *>(mutable_context));
        }
    };

    ClosureFunction &operator=(const ClosureFunction &other) noexcept {
        if (this != &other) {
            _function = other._function;
            if (_context) {
                ::swift::swift_release((::swift::HeapObject *)_context);
            }
            _context = other._context;
            if (_context) {
                ::swift::swift_retain((::swift::HeapObject *)_context);
            }
        }
        return *this;
    };

    // Move

    ClosureFunction(ClosureFunction &&other) noexcept
        : _function(std::exchange(other._function, nullptr)), _context(std::exchange(other._context, nullptr)) {};

    ClosureFunction &operator=(ClosureFunction &&other) noexcept {
        if (this != &other) {
            _function = other._function;
            other._function = nullptr;
            if (_context) {
                ::swift::swift_release((::swift::HeapObject *)_context);
            }
            _context = other._context;
            other._context = nullptr;
        }
        return *this;
    }

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
    requires std::same_as<ReturnType, void> && std::unsigned_integral<Arg>
using ClosureFunctionAV = ClosureFunction<ReturnType, Arg>;

template <typename ReturnType, typename Arg>
    requires std::same_as<ReturnType, bool> && std::unsigned_integral<Arg>
using ClosureFunctionAB = ClosureFunction<ReturnType, Arg>;

template <typename ReturnType, typename Arg>
    requires std::same_as<ReturnType, void>
using ClosureFunctionPV = ClosureFunction<ReturnType, Arg>;

} // namespace AG
