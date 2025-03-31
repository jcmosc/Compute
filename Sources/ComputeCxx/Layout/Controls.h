#pragma once

namespace AG {
namespace LayoutDescriptor {

// See BytecodeLayouts.h

enum Controls : unsigned char {
    EqualsItemBegin = '\x01',
    EqualsItemTypePointerSize = 8,
    EqualsItemEquatablePointerSize = 8,

    IndirectItemBegin = '\x02',
    IndirectItemTypePointerSize = 8,
    IndirectItemLayoutPointerSize = 8,

    ExistentialItemBegin = '\x03',
    ExistentialItemTypePointerSize = 8,

    HeapRefItemBegin = '\x04',
    FunctionItemBegin = '\x05',

    NestedItemBegin = '\x06',
    NestedItemLayoutPointerSize = 8,

    CompactNestedItemBegin = '\x07',
    CompactNestedItemLayoutRelativePointerSize = 4,
    CompactNestedItemLayoutSize = 2,

    EnumItemBeginVariadicCaseIndex = '\x08',
    EnumItemBeginCaseIndexFirst = '\x09',
    EnumItemBeginCaseIndex0 = '\x09',
    EnumItemBeginCaseIndex1 = '\x0a',
    EnumItemBeginCaseIndex2 = '\x0b',
    EnumItemBeginCaseIndexLast = '\x0b',
    EnumItemContinueVariadicCaseIndex = '\x0c',
    EnumItemContinueCaseIndexFirst = '\x0d',
    EnumItemContinueCaseIndex0 = '\x0d',
    EnumItemContinueCaseIndex1 = '\x0e',
    EnumItemContinueCaseIndex2 = '\x0f',
    EnumItemContinueCaseIndex3 = '\x10',
    EnumItemContinueCaseIndex4 = '\x11',
    EnumItemContinueCaseIndex5 = '\x12',
    EnumItemContinueCaseIndex6 = '\x13',
    EnumItemContinueCaseIndex7 = '\x14',
    EnumItemContinueCaseIndex8 = '\x15',
    EnumItemContinueCaseIndexLast = '\x15',
    EnumItemEnd = '\x16',
    EnumItemTypePointerSize = 8,

    LastControlCharacter = '\x16',
};

}
} // namespace AG
