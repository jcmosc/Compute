#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFURL.h>
#include <dispatch/dispatch.h>

#include "Containers/Vector.h"

CF_ASSUME_NONNULL_BEGIN

struct AGDebugServerMessageHeader {};

namespace AG {

class DebugServer {
  public:
    enum Options : uint32_t {

    };

    class Connection {
      private:
        DebugServer *_server;
        int _socket;
        dispatch_source_t _dispatch_source;

      public:
        Connection(DebugServer *server, int socket);
        ~Connection();

        int socket() const { return _socket; };

        static void handler(void *context);
    };

  private:
    int _socket;
    uint32_t _ip_address;
    uint16_t _port;
    uint16_t _padding;
    uint32_t _token;
    dispatch_source_t _dispatch_source;
    vector<std::unique_ptr<Connection>, 0, uint64_t> _clients;

    static DebugServer *_shared_server;

  public:
    static void start(uint32_t options);
    static void stop();

    static void accept_handler(void *context);
    static void receive(CFDataRef _Nonnull *_Nonnull body);

    DebugServer(uint32_t options);
    ~DebugServer();

    void run(int timeout);
    void shutdown();

    void close_connection(Connection *connection);

    CFURLRef copy_url();
};

} // namespace AG

CF_ASSUME_NONNULL_END
