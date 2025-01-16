#include "ContextDescriptor.h"

#include "Metadata.h"

namespace AG {
namespace swift {

context_descriptor::generic_params_info::generic_params_info(const context_descriptor &context,
                                                             const metadata *_Nullable type) {
    if (context.isGeneric()) {
        switch (context.getKind()) {
        case ::swift::ContextDescriptorKind::Class: {
            auto cls = reinterpret_cast<const class_type_descriptor *>(context.base());
            generic_header = &cls->base()->getFullGenericContextHeader();

            if (type) {
                auto asWords = reinterpret_cast<const metadata *const *>(type);
                generic_args = asWords + cls->immediate_members_offset();
            }
            break;
        }
        case ::swift::ContextDescriptorKind::Struct:
        case ::swift::ContextDescriptorKind::Enum: {
            auto val = reinterpret_cast<const ::swift::TypeContextDescriptor *>(context.base());
            generic_header = &val->getFullGenericContextHeader();

            if (type) {
                auto asWords = reinterpret_cast<const metadata *const *>(type);
                generic_args = asWords + val->getGenericArgumentOffset();
            }
            break;
        }
        default:
            return;
        }

        if (generic_header->Base.NumParams > 0) {
            // TODO: correct cast for class?
            auto val = reinterpret_cast<const ::swift::TypeContextDescriptor *>(context.base());
            auto generic_params = val->getGenericParams();
            params = generic_params;

            if (generic_header->Base.Flags.hasTypePacks()) {
                pack_shape_header = val->getGenericContext()->getGenericPackShapeHeader();
                pack_shape_descriptors = val->getGenericContext()->getGenericPackShapeDescriptors();
            }
        }
    }
}

uint64_t context_descriptor::count_generic_args() const {
    auto info = generic_params_info(*this, nullptr);

    uint64_t count = 0;
    for (auto param : info.params) {
        switch (param.getKind()) {
        case ::swift::GenericParamKind::Type:
        case ::swift::GenericParamKind::TypePack: {
            if (param.hasKeyArgument()) {
                count += 1;
            }
            break;
        }
        default:
            break;
        }
    }
    return count;
}

void context_descriptor::push_generic_args(const metadata &type,
                                           vector<generic_arg, 8, uint64_t> &generic_args_vector) const {
    auto info = generic_params_info(*this, &type);
    if (info.params.empty()) {
        return;
    }

    // The generic arguments array consists of:
    // - a sequence of pack lengths
    // - a sequence of metadata or metadata pack pointers
    // - a sequence of witness table or witness table pack pointers
    // See
    // https://github.com/swiftlang/swift/blob/d4756cbe7fa6058786035aa4cb9a2a83c1b839ed/include/swift/ABI/GenericContext.h

    // Start from the metadata or metadata pack points
    unsigned arg_index = info.pack_shape_header.NumShapeClasses;

    // Iterate through over the pack shape descriptors array in parallel with the generic arguments array
    unsigned pack_index = 0;

    for (auto param : info.params) {
        if (!param.hasKeyArgument()) {
            continue;
        }

        switch (param.getKind()) {
        case ::swift::GenericParamKind::Type: {
            generic_args_vector.push_back({
                info.generic_args[arg_index],
                1,
                false,
            });
            break;
        }
        case ::swift::GenericParamKind::TypePack: {
            const metadata *types = nullptr;
            uint64_t num_types = 0;
            if (info.generic_args[arg_index] != nullptr) {
                types = info.generic_args[arg_index];
                // The shape class is represented as the length of the type parameter pack
                num_types = reinterpret_cast<uint64_t>(info.generic_args[info.pack_shape_descriptors[pack_index].ShapeClass]);
            }

            pack_index += 1;

            generic_args_vector.push_back({
                types,
                num_types,
                true,
            });
            break;
        }
        default:
            break;
        }

        arg_index += 1;
    }
}

uint64_t class_type_descriptor::immediate_members_offset() const {
    if (base()->hasResilientSuperclass()) {
        // Equivalent to ::swift::getResilientImmediateMembersOffset but without using any atomic-ordered loads,
        // as we already have the metadata pointer
        const auto &stored_bounds = base()->ResilientMetadataBounds.get();
        ptrdiff_t offset = stored_bounds->ImmediateMembersOffset.load(std::memory_order_relaxed);
        return offset / sizeof(void *);
    }
    return base()->getNonResilientImmediateMembersOffset();
}

} // namespace swift
} // namespace AG
