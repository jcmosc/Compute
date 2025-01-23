#pragma once

#include "LayoutDescriptor.h"

#include "Vector/Vector.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {
namespace LayoutDescriptor {

class Compare {
  public:
    class Frame {
      private:
        Compare * _compare;
        uint64_t _start;

      public:
        ~Frame();
    };

    struct Enum {
        enum Mode : uint32_t {
            InitializingCopies = 1,
        };

        const swift::metadata *type;
        unsigned int enum_tag;
        const unsigned char *lhs;
        const unsigned char *rhs;
        const unsigned char *lhs_copy;
        const unsigned char *rhs_copy;
        size_t offset;
        Mode mode;
        bool owns_copies;

        Enum(const swift::metadata *type, Mode mode, unsigned int enum_tag, size_t offset, const unsigned char *lhs,
             const unsigned char *lhs_copy, const unsigned char *rhs, const unsigned char *rhs_copy, bool owns_copies);
        ~Enum();
    };

  private:
    vector<Enum, 8, uint64_t> _enums;

  public:
    Compare();
    
    vector<Enum, 8, uint64_t> enums() { return _enums; };

    bool operator()(ValueLayout layout, const unsigned char *lhs, const unsigned char *rhs, size_t offset, size_t size,
                    ComparisonOptions options);

    bool failed(ComparisonOptions options, const unsigned char *lhs, const unsigned char *rhs, size_t offset,
                size_t size, const swift::metadata *_Nullable type);
};

} // namespace LayoutDescriptor
} // namespace AG

CF_ASSUME_NONNULL_END
