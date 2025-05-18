#include "AGDebugServer.h"

#include "DebugServer.h"

CFURLRef AGDebugServerCopyURL() {
    if (AG::DebugServer::shared() != nullptr) {
        return AG::DebugServer::shared()->copy_url();
    }
    return nullptr;
}

void AGDebugServerRun(uint32_t timeout) {
    if (AG::DebugServer::shared() != nullptr) {
        AG::DebugServer::shared()->run(timeout);
    }
}

void AGDebugServerStart(AGDebugServerRef server, uint32_t options) {
    reinterpret_cast<AG::DebugServer *>(server)->start(options);
}

void AGDebugServerStop(AGDebugServerRef server) { reinterpret_cast<AG::DebugServer *>(server)->stop(); }
