#pragma once

#include <string>

#include "ComputeCxx/AGBase.h"
#include "ComputeCxx/AGComparison.h"

AG_ASSUME_NONNULL_BEGIN

namespace AG {

namespace swift {
class metadata;
class existential_type_metadata;
class context_descriptor;
} // namespace swift

/// A string that encodes an object's layout in memory.
using ValueLayout = const unsigned char *;

extern const ValueLayout ValueLayoutTrivial;

namespace LayoutDescriptor {

enum class HeapMode : uint16_t {
    NonHeap = 0,
    Class = 1 << 0,
    Locals = 1 << 1,
    GenericLocals = 1 << 2,
};
inline bool operator&(HeapMode a, HeapMode b) { return (uint16_t)a & (uint16_t)b; }
inline HeapMode operator|(HeapMode a, HeapMode b) { return a | b; }

extern unsigned char base_address;

// MARK: Managing comparison modes

AGComparisonMode mode_for_type(const swift::metadata *_Nullable type, AGComparisonMode default_mode);
void add_type_descriptor_override(const swift::context_descriptor *_Nullable type_descriptor,
                                  AGComparisonMode override_mode);

// MARK: Obtaining layouts

ValueLayout fetch(const swift::metadata &type, AGComparisonOptions options, uint32_t priority);

ValueLayout make_layout(const swift::metadata &type, AGComparisonMode default_mode, HeapMode heap_mode);

// MARK: Comparing values

/// Returns the number of characters in the layout, up to the next sibling enum marker or until the end of the
/// layout.
size_t length(ValueLayout layout);

// MARK: Comparing values

bool compare(ValueLayout layout, const unsigned char *lhs, const unsigned char *rhs, size_t size,
             AGComparisonOptions options);
bool compare_bytes_top_level(const unsigned char *lhs, const unsigned char *rhs, size_t size,
                             AGComparisonOptions options);
bool compare_bytes(char unsigned const *lhs, char unsigned const *rhs, size_t size, size_t *_Nullable failure_location);
bool compare_heap_objects(char unsigned const *lhs, char unsigned const *rhs, AGComparisonOptions options,
                          bool is_function);
bool compare_indirect(ValueLayout _Nullable *_Nullable layout_ref, const swift::metadata &lhs_type,
                      const swift::metadata &rhs_type, AGComparisonOptions options, const unsigned char *lhs,
                      const unsigned char *rhs);
bool compare_existential_values(const swift::existential_type_metadata &type, const unsigned char *lhs,
                                const unsigned char *rhs, AGComparisonOptions options);

bool compare_partial(ValueLayout layout, const unsigned char *lhs, const unsigned char *rhs, size_t offset, size_t size,
                     AGComparisonOptions options);
struct Partial {
    ValueLayout layout;
    size_t location;
};
Partial find_partial(ValueLayout layout, size_t range_location, size_t range_size);

// MARK: Printing

void print(std::string &output, ValueLayout layout);

} // namespace LayoutDescriptor

} // namespace AG

AG_ASSUME_NONNULL_END
