#include "AGEquatable.h"

#include "SwiftEquatableSupport.h"

bool AGDispatchEquatable(const void *lhs, const void *rhs, const AG::swift::metadata *type, const AG::swift::equatable_witness_table *equatable) {
    return ::swift::equatable_support::_swift_stdlib_Equatable_isEqual_indirect(lhs, rhs, type, equatable);
}
