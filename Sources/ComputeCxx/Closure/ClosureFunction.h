#pragma once

#include <swift/Runtime/HeapObject.h>

#include "ComputeCxx/IAGBase.h"

namespace IAG {

/// C++ function type that is equivalent to the lowered Swift closure type.
template <typename Result, typename... Args> class ClosureFunction {
  public:
    using Context = const void *_Nullable;
    using Function = IAG_SWIFT_CC(swift) Result (*_Nullable)(Args..., Context IAG_SWIFT_CONTEXT);

  private:
    Function _function;
    Context _context;

  public:
    inline ClosureFunction(std::nullptr_t): _function(nullptr), _context(nullptr) {}
    inline ClosureFunction(Function function, Context context) noexcept : _function(function), _context(context) {
        if (_context) {
            void *mutable_context = const_cast<void *>(_context);
            ::swift::swift_retain(reinterpret_cast<::swift::HeapObject *>(mutable_context));
        }
    }

    inline ~ClosureFunction() {
        if (_context) {
            void *mutable_context = const_cast<void *>(_context);
            ::swift::swift_release(reinterpret_cast<::swift::HeapObject *>(mutable_context));
        }
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
            Context new_context = other._context;
            if (new_context) {
                new_context = ::swift::swift_retain((::swift::HeapObject *)new_context);
            }
            Context old_context = _context;
            _function = other._function;
            _context = new_context;
            if (old_context) {
                ::swift::swift_release((::swift::HeapObject *)old_context);
            }
        }
        return *this;
    };

    // Move

    ClosureFunction(ClosureFunction &&other) noexcept
        : _function(std::exchange(other._function, nullptr)), _context(std::exchange(other._context, nullptr)) {};

    ClosureFunction &operator=(ClosureFunction &&other) noexcept {
        if (this != &other) {
            Context old_context = _context;
            _function = other._function;
            _context = other._context;
            other._function = nullptr;
            other._context = nullptr;
            if (old_context) {
                ::swift::swift_release((::swift::HeapObject *)old_context);
            }
        }
        return *this;
    }

    explicit operator bool() { return _function != nullptr; }

    const Result operator()(Args... args) const noexcept {
        return _function(std::forward<Args>(args)..., _context);
    }
};

template <typename Result>
    requires std::is_pointer_v<Result>
using ClosureFunctionVP = ClosureFunction<Result>;

template <typename Result>
    requires std::same_as<Result, void>
using ClosureFunctionVV = ClosureFunction<Result>;

template <typename Result, typename Arg>
    requires std::same_as<Result, void> && std::unsigned_integral<Arg>
using ClosureFunctionAV = ClosureFunction<Result, Arg>;

template <typename Result, typename Arg>
    requires std::same_as<Result, bool> && std::unsigned_integral<Arg>
using ClosureFunctionAB = ClosureFunction<Result, Arg>;

template <typename Result, typename Arg>
    requires std::same_as<Result, void>
using ClosureFunctionPV = ClosureFunction<Result, Arg>;

template <typename Result, typename Arg>
    requires std::unsigned_integral<Result>
using ClosureFunctionCI = ClosureFunction<Result, Arg>;

} // namespace IAG
