#include "ComputeCxx/IAGDebugServer.h"

#include "DebugServer.h"

void IAGDebugServerStart(IAGDebugServerOptions options) { IAG::DebugServer::start(options); }

void IAGDebugServerStop(void) { IAG::DebugServer::stop(); }

void IAGDebugServerRun(uint32_t timeout) {
    auto debug_server = IAG::DebugServer::shared();
    if (!debug_server) {
        return;
    }

    debug_server->run(timeout);
}

CFURLRef IAGDebugServerCopyURL(void) {
    auto debug_server = IAG::DebugServer::shared();
    if (!debug_server) {
        return nullptr;
    }

    return debug_server->copy_url();
}
