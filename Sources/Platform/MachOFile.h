#pragma once

#if __APPLE__

#include <mach-o/loader.h>
#include <uuid/uuid.h>

// A partial port of https://github.com/PureDarwin/dyld/blob/master/dyld3/MachOFile.h
struct MachOFile : mach_header {
  public:
    bool hasMachOMagic() const;
    bool getUuid(uuid_t uuid) const;

  protected:
    bool hasMachOBigEndianMagic() const;
    void forEachLoadCommand(/* Diagnostics &diag, */ void (^callback)(const load_command *cmd, bool &stop)) const;
};

#endif
