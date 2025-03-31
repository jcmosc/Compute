#pragma once

#include "LayoutDescriptor.h"

#include "Containers/Vector.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {
namespace LayoutDescriptor {

class Compare {
  public:
    struct Enum {
        enum Mode : uint32_t {
            Unmanaged = 0,
            Managed = 1,
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

        Enum()
            : type(nullptr), enum_tag(0), lhs(nullptr), rhs(nullptr), lhs_copy(nullptr), rhs_copy(nullptr), offset(0),
              mode(Mode::Unmanaged), owns_copies(false){};
        Enum(const swift::metadata *type, Mode mode, unsigned int enum_tag, size_t offset, const unsigned char *lhs,
             const unsigned char *lhs_copy, const unsigned char *rhs, const unsigned char *rhs_copy, bool owns_copies);
        ~Enum();
    };

    class Frame {
      private:
        vector<Enum, 8, uint64_t> *_enums;
        uint64_t _start;

      public:
        ~Frame();
    };

  private:
    vector<Enum, 8, uint64_t> _enums;

  public:
    bool operator()(ValueLayout layout, const unsigned char *lhs, const unsigned char *rhs, size_t offset, size_t size,
                    ComparisonOptions options);

    bool failed(ComparisonOptions options, const unsigned char *lhs, const unsigned char *rhs, size_t offset,
                size_t size, const swift::metadata *_Nullable type);
};

} // namespace LayoutDescriptor
} // namespace AG

CF_ASSUME_NONNULL_END
