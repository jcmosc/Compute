#include "MachOFile.h"

#if __APPLE__

#include <cstring>

bool MachOFile::hasMachOMagic() const { return magic == MH_MAGIC || magic == MH_MAGIC_64; }

bool MachOFile::getUuid(uuid_t uuid) const {
    __block bool found = false;
    forEachLoadCommand(^(const load_command *cmd, bool &stop) {
      if (cmd->cmd == LC_UUID) {
          const uuid_command *uc = (const uuid_command *)cmd;
          memcpy(uuid, uc->uuid, sizeof(uuid_t));
          found = true;
          stop = true;
      }
    });
    /* diag.assertNoError();   // any malformations in the file should have been caught by earlier validate() call */
    if (!found) {
        bzero(uuid, sizeof(uuid_t));
    }
    return found;
}

bool MachOFile::hasMachOBigEndianMagic() const { return magic == MH_CIGAM || magic == MH_CIGAM_64; };

void MachOFile::forEachLoadCommand(/* Diagnostics& diag, */ void (^callback)(const load_command *cmd, bool &stop)) const {
    bool stop = false;
    const load_command *startCmds = nullptr;
    if (this->magic == MH_MAGIC_64) {
        startCmds = (load_command *)((char *)this + sizeof(mach_header_64));
    } else if (this->magic == MH_MAGIC) {
        startCmds = (load_command *)((char *)this + sizeof(mach_header));
    } else if (hasMachOBigEndianMagic()) {
        return; // can't process big endian mach-o
    } else {
        const uint32_t *h = (uint32_t *)this;
        /* diag.error("file does not start with MH_MAGIC[_64]: 0x%08X 0x%08X", h[0], h[1]); */
        return; // not a mach-o file
    }
    const load_command *const cmdsEnd = (load_command *)((char *)startCmds + this->sizeofcmds);
    const load_command *cmd = startCmds;
    for (uint32_t i = 0; i < this->ncmds; ++i) {
        const load_command *nextCmd = (load_command *)((char *)cmd + cmd->cmdsize);
        if (cmd->cmdsize < 8) {
            /* diag.error("malformed load command #%d of %d at %p with mh=%p, size (0x%X) too small", i, this->ncmds,
             * cmd, this, cmd->cmdsize); */
            return;
        }
        if ((nextCmd > cmdsEnd) || (nextCmd < startCmds)) {
            /* diag.error("malformed load command #%d of %d at %p with mh=%p, size (0x%X) is too large, load commands
             * end at %p", i, this->ncmds, cmd, this, cmd->cmdsize, cmdsEnd); */
            return;
        }
        callback(cmd, stop);
        if (stop) {
            return;
        }
        cmd = nextCmd;
    }
}

#endif
