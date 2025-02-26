#include "DebugServer.h"

#include <Foundation/Foundation.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <os/log.h>
#include <sys/socket.h>
#include <xlocale.h>

#include "Graph/Graph+Description.h"
#include "Graph/Graph.h"
#include "Log/Log.h"
#include "Utilities/FreeDeleter.h"

namespace AG {

constexpr int backlog = 5;

DebugServer *DebugServer::_shared_server = nullptr;

void DebugServer::start(uint32_t options) {
    if (options & 1 && !_shared_server) {
        if (true /* && os_variant_has_internal_diagnostics() */) {
            _shared_server = new DebugServer(options);
        }
    }
    return _shared_server;
}

void DebugServer::stop() {
    if (_shared_server) {
        delete _shared_server;
        _shared_server = nullptr;
    }
    return _shared_server;
}

DebugServer::DebugServer(uint32_t options) {
    _socket = -1;
    _ip_address = 0;
    _port = 0;

    _token = arc4random();
    _dispatch_source = nullptr;

    _socket = socket(AF_INET, SOCK_STREAM, 0);
    if (_socket < 0) {
        perror("AGDebugServer: socket");
        return;
    }

    fcntl(_socket, F_SETFD, FD_CLOEXEC);

    int option_value = 1;
    setsockopt(_socket, SOL_SOCKET, SO_NOSIGPIPE, &option_value, sizeof(option_value));

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = 0;
    address.sin_addr.s_addr = (options & 2) ? INADDR_ANY : htonl(INADDR_LOOPBACK);

    if (bind(_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("AGDebugServer: bind");
        shutdown();
        return;
    }

    socklen_t length = sizeof(address);
    if (getsockname(_socket, (struct sockaddr *)&address, &length) < 0) {
        perror("AGDebugServer: getsockname");
        shutdown();
        return;
    }

    _ip_address = ntohl(address.sin_addr.s_addr);
    _port = ntohs(address.sin_port);

    if (options & 2) {
        struct ifaddrs *ifaddr = nullptr;
        if (!getifaddrs(&ifaddr)) {
            for (auto *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                    struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
                    if (ntohl(sa->sin_addr.s_addr) != INADDR_LOOPBACK) {
                        _ip_address = ntohl(sa->sin_addr.s_addr);
                        break;
                    }
                }
            }
            freeifaddrs(ifaddr);
        }
    }

    if (listen(_socket, backlog) < 0) {
        perror("AGDebugServer: listen");
        shutdown();
        return this;
    }

    _dispatch_source = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, _socket, 0, dispatch_get_main_queue());
    dispatch_set_context(_dispatch_source, this);
    dispatch_source_set_event_handler_f(_dispatch_source, accept_handler);
    dispatch_resume(_dispatch_source);

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &_ip_address, ip_str, sizeof(ip_str));

    os_log(misc_log(), "debug server graph://%s:%d/?token=%u", ip_str, _port, _token);
    fprintf(stdout, "debug server graph://%s:%d/?token=%u\n", ip_str, _port, _token);
}

DebugServer::~DebugServer() { shutdown(); }

void DebugServer::shutdown() {
    if (auto dispatch_source = _dispatch_source) {
        dispatch_source_set_event_handler(dispatch_source, nullptr);
        dispatch_set_context(dispatch_source, nullptr);
        _dispatch_source = nullptr;
    }
    if (_socket >= 0) {
        close(_socket);
        _socket = -1;
    }
}

void DebugServer::accept_handler(void *context) {
    DebugServer *server = (DebugServer *)context;

    struct sockaddr address = {};
    socklen_t length = sizeof(address);
    int connection_socket = accept(server->_socket, &address, &length);
    if (connection_socket < 0) {
        perror("AGDebugServer: accept");
        return;
    }

    fcntl(server->_socket, F_SETFD, FD_CLOEXEC);

    server->_clients.push_back(std::make_unique<Connection>(server, connection_socket));
}

void DebugServer::run(int timeout) {
    fd_set write_fds;
    struct timeval tv;

    bool accepted = false;
    while (!accepted || !_clients.empty()) {
        FD_ZERO(&write_fds);
        FD_SET(_socket, &write_fds);

        int max_socket = _socket;
        for (auto &connection : _clients) {
            FD_SET(connection->socket(), &write_fds);
            if (connection->socket() > max_socket) {
                max_socket = connection->socket();
            }
        }

        tv.tv_sec = timeout;
        tv.tv_usec = 0;

        int activity = select(max_socket + 1, nullptr, &write_fds, nullptr, &tv);
        if (activity <= 0) {
            if (errno != EAGAIN) {
                perror("AGDebugServer: select");
                return;
            }
        } else {
            if (FD_ISSET(_socket, &write_fds)) {
                accept_handler(this);
                accepted = true;
            }

            for (uint64_t i = 0; i < _clients.size(); ++i) {
                Connection *connection = _clients[i].get();
                if (FD_ISSET(connection->socket(), &write_fds)) {
                    FD_CLR(connection->socket(), &write_fds);
                    connection->handler(connection);

                    // Restart loop to handle possible mutations to clients
                    i = 0;
                }
            }
        }
    }
}

void DebugServer::receive(CFDataRef *body) {
    @autoreleasepool {
        id body_json = [NSJSONSerialization JSONObjectWithData:(__bridge NSData *)*body options:0 error:nullptr];
        if (!body_json) {
            body = nullptr;
            return;
        }

        if (![body_json isKindOfClass:[NSDictionary class]]) {
            body = nullptr;
            return;
        }
        NSDictionary *body_dict = (NSDictionary *)body_json;

        NSString *command = body_dict[@"command"];

        if ([command isEqual:@"graph/description"]) {
            NSMutableDictionary *options = [NSMutableDictionary dictionaryWithDictionary:body_dict];
            options[AGDescriptionFormat] = @"graph/dict";

            id desc = Graph::description(nullptr, (__bridge CFDictionaryRef)options);
            if (!desc) {
                *body = nullptr;
                return;
            }

            NSData *new_data = [NSJSONSerialization dataWithJSONObject:desc options:0 error:nil];

            *body = (__bridge CFDataRef)new_data;
        } else if ([command isEqual:@"profiler/start"]) {
            id profiler_flags_json = body_dict[@"flags"];
            uint32_t profiler_flags =
                ([profiler_flags_json isKindOfClass:[NSNumber class]] ? [profiler_flags_json unsignedIntValue] : 0) | 1;
            Graph::all_start_profiling(profiler_flags);
        } else if ([command isEqual:@"profiler/stop"]) {
            Graph::all_stop_profiling();
        } else if ([command isEqual:@"profiler/reset"]) {
            Graph::all_reset_profile();
        } else if ([command isEqual:@"profiler/mark"]) {
            id name_json = body_dict[@"name"];
            if ([name_json isKindOfClass:[NSString class]]) {
                Graph::all_mark_profile([name_json UTF8String]);
            }
        } else if ([command isEqual:@"tracing/start"]) {
            id tracing_flags_json = body_dict[@"flags"];
            uint32_t tracing_flags =
                ([tracing_flags_json isKindOfClass:[NSNumber class]] ? [tracing_flags_json unsignedIntValue] : 0) | 1;

            auto subsystems = std::span<const char *>();
            auto subsystems_vector = vector<std::unique_ptr<const char, util::free_deleter>, 0, uint64_t>();
            id trace_subsystems_json = body_dict[@"subsystems"];
            if ([trace_subsystems_json isKindOfClass:[NSArray class]]) {
                for (id trace_subsystem_json in trace_subsystems_json) {
                    if ([trace_subsystem_json isKindOfClass:[NSString class]]) {
                        const char *str = [trace_subsystem_json UTF8String];
                        subsystems_vector.push_back(std::unique_ptr<const char, util::free_deleter>(str));
                    }
                }
                subsystems = std::span((const char **)subsystems_vector.data(), subsystems_vector.size());
            }

            Graph::all_start_tracing(tracing_flags, subsystems);
        } else if ([command isEqual:@"tracing/stop"]) {
            Graph::all_stop_tracing();
        } else if ([command isEqual:@"tracing/sync"]) {
            AG::Graph::all_sync_tracing();
        }

        *body = nullptr;
        return;
    }
}

void DebugServer::close_connection(Connection *connection) {
    auto iter = std::remove_if(_clients.begin(), _clients.end(),
                               [&connection](auto &candidate) -> bool { return candidate.get() == connection; });
    _clients.erase(iter);
}

CFURLRef DebugServer::copy_url() {
    if (_socket < 0) {
        return nullptr;
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &_ip_address, ip_str, sizeof(ip_str));

    char bytes[0x100];
    snprintf_l(bytes, 0x100, nullptr, "graph://%s:%d/?token=%u", ip_str, _port, _token);

    CFIndex length = strlen(bytes);
    return CFURLCreateWithBytes(kCFAllocatorDefault, (const unsigned char *)bytes, length, kCFStringEncodingUTF8,
                                nullptr);
}

} // namespace AG
