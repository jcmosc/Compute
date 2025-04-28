#pragma once

#include <CoreFoundation/CFBase.h>
#include <string>

CF_ASSUME_NONNULL_BEGIN

namespace AG {

namespace swift {
class metadata;
class existential_type_metadata;
class context_descriptor;
} // namespace swift

/// A string that encodes an object's layout in memory.
using ValueLayout = const unsigned char *;

extern const ValueLayout ValueLayoutEmpty;

namespace LayoutDescriptor {

enum class HeapMode : uint16_t {
    Option0 = 0,
    Option1 = 1,
    Option2 = 2,
};

extern uintptr_t base_address;

enum ComparisonMode : uint16_t {

};

struct ComparisonOptions {
  private:
    uint32_t _value;

  public:
    enum {
        ComparisonModeMask = 0xff,
        CopyOnWrite = 1 << 8,
        FetchLayoutsSynchronously = 1 << 9,
        ReportFailures = 1ul << 31, // -1 signed int
    };

    ComparisonOptions(uint32_t value = 0) : _value(value) {}

    operator uint32_t() { return _value; };
    ComparisonMode comparision_mode() { return ComparisonMode(_value & ComparisonModeMask); };
    bool copy_on_write() { return _value & CopyOnWrite; };
    bool fetch_layouts_synchronously() { return _value & FetchLayoutsSynchronously; };
    bool report_failures() { return _value & ReportFailures; };

    ComparisonOptions without_copying_on_write() { return ComparisonOptions(_value & ~CopyOnWrite); };
    ComparisonOptions without_reporting_failures() { return ComparisonOptions(_value & ~ReportFailures); };
};
inline ComparisonOptions &operator|=(ComparisonOptions &lhs, ComparisonOptions rhs) {
    lhs = ComparisonOptions(uint32_t(lhs) | uint32_t(rhs));
    return lhs;
}

// MARK: Managing comparison modes

ComparisonMode mode_for_type(const swift::metadata *_Nullable type, ComparisonMode default_mode);
void add_type_descriptor_override(const swift::context_descriptor *_Nullable type_descriptor,
                                  ComparisonMode override_mode);

// MARK: Obtaining layouts

ValueLayout fetch(const swift::metadata &type, ComparisonOptions options, uint32_t priority);

ValueLayout make_layout(const swift::metadata &type, ComparisonMode default_mode, HeapMode heap_mode);

// MARK: Comparing values

/// Returns the number of characters in the layout, up to the next sibling enum marker or until the end of the layout.
size_t length(ValueLayout layout);

// MARK: Comparing values

bool compare(ValueLayout layout, const unsigned char *lhs, const unsigned char *rhs, size_t size,
             ComparisonOptions options);
bool compare_bytes_top_level(const unsigned char *lhs, const unsigned char *rhs, size_t size,
                             ComparisonOptions options);
bool compare_bytes(char unsigned const *lhs, char unsigned const *rhs, size_t size, size_t *_Nullable failure_location);
bool compare_heap_objects(char unsigned const *lhs, char unsigned const *rhs, ComparisonOptions options,
                          bool is_function);
bool compare_indirect(ValueLayout *_Nullable layout_ref, const swift::metadata &lhs_type,
                      const swift::metadata &rhs_type, ComparisonOptions options, const unsigned char *lhs,
                      const unsigned char *rhs);
bool compare_existential_values(const swift::existential_type_metadata &type, const unsigned char *lhs,
                                const unsigned char *rhs, ComparisonOptions options);

bool compare_partial(ValueLayout layout, const unsigned char *lhs, const unsigned char *rhs, size_t offset, size_t size,
                     ComparisonOptions options);
struct Partial {
    ValueLayout layout;
    size_t location;
};
Partial find_partial(ValueLayout layout, size_t range_location, size_t range_size);

// MARK: Printing

void print(std::string &output, ValueLayout layout);

} // namespace LayoutDescriptor

} // namespace AG

CF_ASSUME_NONNULL_END
