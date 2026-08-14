#pragma once

#include "ComputeCxx/IAGBase.h"

#if TARGET_OS_MAC

#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFURL.h>
#include <dispatch/dispatch.h>

#include <Utilities/ObjCPointer.h>

#include "ComputeCxx/IAGDebugServer.h"
#include "Vector/Vector.h"

IAG_ASSUME_NONNULL_BEGIN

namespace IAG {

class DebugServer {
  public:
    class Connection {
      private:
        DebugServer *_server;
        int _socket;
        util::objc_ptr<dispatch_source_t> _event_source = nullptr;

      public:
        Connection(DebugServer *server, int socket);
        ~Connection();

        int socket() const { return _socket; };

        static void handler(void *context);

        friend class DebugServer;
    };

  private:
    int _socket;
    uint32_t _ip;
    uint16_t _port;
    uint32_t _token;
    util::objc_ptr<dispatch_source_t> _accept_source = nullptr;
    vector<std::unique_ptr<Connection>, 0, uint64_t> _connections;

    static DebugServer *_Nullable _shared_server;

    static void accept_handler(void *context);
    void close_connection(Connection *connection);

    static CFDataRef _Nullable receive(Connection *connection, IAGDebugServerMessageHeader *header, CFDataRef body);

  public:
    static DebugServer *shared() { return _shared_server; };

    static DebugServer *_Nullable start(IAGDebugServerOptions options);
    static void stop();

    DebugServer(IAGDebugServerOptions options);
    ~DebugServer();

    // Non-copyable
    DebugServer(const DebugServer &) = delete;
    void operator=(const DebugServer &) = delete;

    // Non-movable
    DebugServer(DebugServer &&) = delete;
    DebugServer &operator=(DebugServer &&) = delete;

    void run(uint32_t timeout);
    void shutdown();

    CFURLRef _Nullable copy_url();
};

} // namespace IAG

IAG_ASSUME_NONNULL_END

#endif
