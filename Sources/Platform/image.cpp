#include "platform/image.h"

#if __APPLE__
#include <dlfcn.h>

#include "MachOFile.h"
#endif

#include <strings.h>

void platform_image_infos_for_addresses(unsigned count, const void *_Nonnull addresses[_Nonnull],
                                        platform_image_info_t infos[_Nonnull]) {
    for (unsigned i = 0; i < count; i++) {
        const void *addr = addresses[i];
        bzero(&infos[i], sizeof(platform_image_info_t));

#if __APPLE__
        Dl_info dl_info;
        if (dladdr(addr, &dl_info)) {
            //            infos[i].image = (mach_header *)dl_info.dli_fbase;
            infos[i].offset = (uintptr_t)addr - (uintptr_t)dl_info.dli_fbase;

            const MachOFile *mf = (MachOFile *)dl_info.dli_fbase;
            if (mf->hasMachOMagic()) {
                mf->getUuid(infos[i].identifier);
            } else {
                bzero(infos[i].identifier, sizeof(uuid_t));
            }
        }
#else
        // TODO
#endif
    }
}
