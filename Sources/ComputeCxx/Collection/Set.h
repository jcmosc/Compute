#pragma once

#include <algorithm>
#include <utility>

#include "ComputeCxx/IAGBase.h"
#include "Vector.h"

IAG_ASSUME_NONNULL_BEGIN

namespace IAG {

/// A set of pointers stored in a vector sorted by address.
template <typename T, typename size_type = std::size_t>
    requires std::is_pointer_v<T> && std::unsigned_integral<size_type>
class set {
  private:
    static constexpr uintptr_t removed = 0x1;

    vector<uintptr_t, 0, size_type> _slots;
    size_type _num_removed = 0;

    uintptr_t *_Nonnull find_slot(const T &value) {
        return std::lower_bound(_slots.begin(), _slots.end(), uintptr_t(value),
                                [](uintptr_t slot, uintptr_t value) { return (slot & ~removed) < value; });
    };

  public:
    class iterator {
      private:
        const uintptr_t *_Nonnull _slot;
        const uintptr_t *_Nonnull _end;

        void skip_removed() {
            while (_slot != _end && (*_slot & removed)) {
                ++_slot;
            }
        };

      public:
        iterator(const uintptr_t *_Nonnull slot, const uintptr_t *_Nonnull end) : _slot(slot), _end(end) {
            skip_removed();
        };

        T _Nonnull operator*() const { return reinterpret_cast<T>(*_slot); };
        iterator &operator++() {
            ++_slot;
            skip_removed();
            return *this;
        };
        bool operator==(const iterator &other) const { return _slot == other._slot; };
    };

    iterator begin() const { return iterator(_slots.begin(), _slots.end()); };
    iterator end() const { return iterator(_slots.end(), _slots.end()); };

    bool empty() const { return _slots.size() == _num_removed; };

    std::pair<iterator, bool> insert(const T &value) {
        auto *slot = find_slot(value);
        bool inserted = true;
        if (slot == _slots.end() || (*slot & ~removed) != uintptr_t(value)) {
            slot = _slots.insert(slot, uintptr_t(value));
        } else if (*slot & removed) {
            // A new element reused the address of a removed one
            *slot = uintptr_t(value);
            _num_removed -= 1;
        } else {
            inserted = false;
        }
        return {iterator(slot, _slots.end()), inserted};
    };

    size_type erase(const T &value) {
        auto *slot = find_slot(value);
        if (slot == _slots.end() || *slot != uintptr_t(value)) {
            return 0;
        }
        *slot |= removed;
        _num_removed += 1;
        if (_num_removed > _slots.size() / 2) {
            _slots.erase(std::remove_if(_slots.begin(), _slots.end(), [](uintptr_t slot) { return slot & removed; }),
                         _slots.end());
            _num_removed = 0;
        }
        return 1;
    };
};

} // namespace IAG

IAG_ASSUME_NONNULL_END
