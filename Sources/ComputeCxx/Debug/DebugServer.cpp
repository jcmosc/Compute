#include "DebugServer.h"

#if TARGET_OS_MAC

#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <xlocale.h>

#include "Log/Log.h"

namespace IAG {

constexpr int backlog = 5;

DebugServer *DebugServer::_shared_server = nullptr;

DebugServer *_Nullable DebugServer::start(IAGDebugServerOptions options) {
    if (options & IAGDebugServerOptionsEnabled && !_shared_server) {
        if (true /* && os_variant_has_internal_diagnostics() */) {
            _shared_server = new DebugServer(options);
        }
    }
    return _shared_server;
}

void DebugServer::stop() {
    if (!_shared_server) {
        return;
    }
    delete _shared_server;
    _shared_server = nullptr;
}

DebugServer::DebugServer(IAGDebugServerOptions options) : _socket(-1), _ip(0), _port(0), _token(arc4random()) {

    _socket = socket(AF_INET, SOCK_STREAM, 0);
    if (_socket < 0) {
        perror("IAGDebugServer: socket");
        return;
    }

    fcntl(_socket, F_SETFD, FD_CLOEXEC);

    int option_value = 1;
    setsockopt(_socket, SOL_SOCKET, SO_NOSIGPIPE, &option_value, sizeof(option_value));

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = 0; // Let system assign port
    address.sin_addr.s_addr = (options & IAGDebugServerOptionsNetworkInterface) ? INADDR_ANY : htonl(INADDR_LOOPBACK);

    if (bind(_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("IAGDebugServer: bind");
        shutdown();
        return;
    }

    socklen_t length = sizeof(address);
    if (getsockname(_socket, (struct sockaddr *)&address, &length) < 0) {
        perror("IAGDebugServer: getsockname");
        shutdown();
        return;
    }

    _ip = ntohl(address.sin_addr.s_addr);
    _port = ntohs(address.sin_port);

    if (options & IAGDebugServerOptionsNetworkInterface) {
        struct ifaddrs *ifaddr = nullptr;
        if (!getifaddrs(&ifaddr)) {
            for (auto *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                    struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
                    uint32_t ip_data = ntohl(sa->sin_addr.s_addr);
                    if (ip_data != INADDR_LOOPBACK) {
                        _ip = ip_data;
                        break;
                    }
                }
            }
            freeifaddrs(ifaddr);
        }
    }

    if (listen(_socket, backlog) < 0) {
        perror("IAGDebugServer: listen");
        shutdown();
        return;
    }

    dispatch_source_t accept_source =
        dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, _socket, 0, dispatch_get_main_queue());
    dispatch_set_context(accept_source, this);
    dispatch_source_set_event_handler_f(accept_source, accept_handler);
    dispatch_resume(accept_source);
    _accept_source = util::adopt_objc(accept_source);

    char ip_str[INET_ADDRSTRLEN];
    uint32_t ip_network = htonl(_ip);
    inet_ntop(AF_INET, &ip_network, ip_str, sizeof(ip_str));

    platform_log_info(misc_log(), "debug server graph://%s:%d/?token=%u", ip_str, _port, _token);
    fprintf(stdout, "debug server graph://%s:%d/?token=%u\n", ip_str, _port, _token);
}

DebugServer::~DebugServer() { shutdown(); }

void DebugServer::accept_handler(void *context) {
    DebugServer *server = reinterpret_cast<DebugServer *>(context);

    struct sockaddr address = {};
    socklen_t length = sizeof(address);
    int connection_socket = accept(server->_socket, &address, &length);
    if (connection_socket < 0) {
        perror("IAGDebugServer: accept");
        return;
    }

    fcntl(connection_socket, F_SETFD, FD_CLOEXEC);

    server->_connections.push_back(std::make_unique<Connection>(server, connection_socket));
}

void DebugServer::close_connection(Connection *connection) {
    auto iter = std::remove_if(_connections.begin(), _connections.end(),
                               [&connection](auto &candidate) -> bool { return candidate.get() == connection; });
    _connections.erase(iter, _connections.end());
}

void DebugServer::run(uint32_t timeout) {
    fd_set writefds;
    struct timeval tv;

    bool accepted = false;
    while (!accepted || !_connections.empty()) {
        FD_ZERO(&writefds);
        FD_SET(_socket, &writefds);

        int nfds = _socket;
        for (auto &connection : _connections) {
            FD_SET(connection->socket(), &writefds);
            if (connection->socket() > nfds) {
                nfds = connection->socket();
            }
        }

        tv.tv_sec = timeout;
        tv.tv_usec = 0;

        int num_sockets_ready = select(nfds + 1, nullptr, &writefds, nullptr, &tv);
        if (num_sockets_ready <= 0) {
            if (errno == EAGAIN) {
                continue;
            }
            perror("IAGDebugServer: select");
            return;
        }

        // Check if server is ready
        if (FD_ISSET(_socket, &writefds)) {
            accept_handler(this);
            accepted = true;
        }

        // Process ready connections
        uint64_t i = 0;
        while (i < _connections.size()) {
            Connection *connection = _connections[i].get();
            if (FD_ISSET(connection->socket(), &writefds)) {
                FD_CLR(connection->socket(), &writefds);
                Connection::handler(connection);

                // Restart loop to handle possible mutations to connections
                i = 0;
            } else {
                ++i;
            }
        }
    }
}

void DebugServer::shutdown() {
    if (auto accept_source = _accept_source.get()) {
        dispatch_source_set_event_handler(accept_source, nullptr);
        dispatch_set_context(accept_source, nullptr);
        _accept_source = nullptr;
    }
    if (_socket >= 0) {
        close(_socket);
        _socket = -1;
    }
}

CFURLRef DebugServer::copy_url() {
    if (_socket < 0) {
        return nullptr;
    }

    char ip_str[INET_ADDRSTRLEN];
    uint32_t ip_network = htonl(_ip);
    inet_ntop(AF_INET, &ip_network, ip_str, sizeof(ip_str));

    char bytes[0x100];
    snprintf_l(bytes, 0x100, nullptr, "graph://%s:%d/?token=%u", ip_str, _port, _token);

    CFIndex length = strlen(bytes);
    return CFURLCreateWithBytes(kCFAllocatorDefault, (const unsigned char *)bytes, length, kCFStringEncodingUTF8,
                                nullptr);
}

} // namespace IAG

#endif
