#include "dyld.h"

#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach-o/dyld_images.h>

#include "MachOFile.h"

void dyld_get_image_uuid(const mach_header *mh, uuid_t uuid) {
    const MachOFile *mf = (MachOFile *)mh;
    if (!mf->hasMachOMagic()) {
        return false;
    }
    return mf->getUuid(uuid);
}

void dyld_images_for_addresses(unsigned count, const void *addresses[], dyld_image_uuid_offset infos[]) {
    for (unsigned i = 0; i < count; i++) {
        const void *addr = addresses[i];
        bzero(&infos[i], sizeof(dyld_image_uuid_offset));

        Dl_info dl_info;
        if (dladdr(addr, &dl_info)) {
            infos[i].image = (mach_header *)dl_info.dli_fbase;
            infos[i].offsetInImage = (uintptr_t)addr - (uintptr_t)dl_info.dli_fbase;
            dyld_get_image_uuid((mach_header *)dl_info.dli_fbase, infos[i].uuid);
        }
    }
}
