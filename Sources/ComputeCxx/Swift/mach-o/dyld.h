#pragma once

#include <stdint.h>
#include <uuid/uuid.h>

extern "C" {

struct dyld_image_uuid_offset {
    uuid_t uuid;
    uint64_t offsetInImage;
    const struct mach_header *image;
};

// Given an array of addresses, returns info about each address.
// For each address, returns the where that image was loaded, the offset
// of the address in the image, and the image's uuid.  If a specified
// address is unknown to dyld, all fields will be returned a zeros.
extern void dyld_images_for_addresses(unsigned count, const void *addresses[], struct dyld_image_uuid_offset infos[]);
}
