#pragma once

#include <platform/base.h>

PLATFORM_ASSUME_NONNULL_BEGIN
PLATFORM_EXTERN_C_BEGIN

#define PLATFORM_IMAGE_INFO_IDENTIFIER_LENGTH 16

typedef struct _platform_image_info {
    unsigned char identifier[PLATFORM_IMAGE_INFO_IDENTIFIER_LENGTH];
    uint64_t offset;
} platform_image_info_t;

PLATFORM_EXPORT
void platform_image_infos_for_addresses(unsigned count, const void *_Nonnull addresses[_Nonnull],
                                        platform_image_info_t infos[_Nonnull]);

PLATFORM_EXTERN_C_END
PLATFORM_ASSUME_NONNULL_END
