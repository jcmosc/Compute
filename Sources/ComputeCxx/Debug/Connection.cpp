#include "DebugServer.h"

#include <dispatch/dispatch.h>

#include <Utilities/ObjCPointer.h>

namespace IAG {

DebugServer::Connection::Connection(DebugServer *server, int socket) : _server(server), _socket(socket) {
    dispatch_source_t event_source =
        dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, socket, 0, dispatch_get_main_queue());
    dispatch_set_context(event_source, this);
    dispatch_source_set_event_handler_f(event_source, handler);
    dispatch_resume(event_source);
    _event_source = util::adopt_objc(event_source);
}

DebugServer::Connection::~Connection() {
    dispatch_source_set_event_handler_f(_event_source.get(), nullptr);
    dispatch_set_context(_event_source.get(), nullptr);
    close(_socket);
}

namespace {

bool blocking_read(int socket_fd, void *buffer, size_t size) {
    if (size == 0) {
        return true;
    }

    size_t total_read = 0;
    char *bytes = static_cast<char *>(buffer);

    while (total_read < size) {
        ssize_t bytes_read = read(socket_fd, bytes + total_read, size - total_read);

        if (bytes_read > 0) {
            total_read += bytes_read;
        } else if (bytes_read == 0) {
            // Socket closed
            return false;
        } else {
            if (errno == EINTR) {
                continue; // Interrupted, retry
            }
#if DEBUG
            assert(errno != EAGAIN && errno != EWOULDBLOCK); // blocking mode shouldn't encounter these errors
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Non-blocking mode: No data available, retry or handle accordingly
                continue;
            }
#endif
            perror("IAGDebugServer: read");
            return false;
        }
    }

    return true;
}

bool blocking_write(int socket_fd, const void *buffer, size_t size) {
    if (size == 0) {
        return true;
    }

    size_t total_written = 0;
    const char *bytes = static_cast<const char *>(buffer);

    while (total_written < size) {
        ssize_t bytes_written = write(socket_fd, bytes + total_written, size - total_written);

        if (bytes_written > 0) {
            total_written += bytes_written;
        } else if (bytes_written == 0) {
            return false; // Unexpected write failure
        } else {
            if (errno == EINTR) {
                continue; // Interrupted, retry
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue; // Non-blocking mode: Retry or handle accordingly
            } else {
                perror("IAGDebugServer: write");
                return false;
            }
        }
    }

    return true;
}

} // namespace

void DebugServer::Connection::handler(void *context) {
    Connection *connection = reinterpret_cast<Connection *>(context);

    uint8_t header_bytes[sizeof(IAGDebugServerMessageHeader)];
    if (!blocking_read(connection->_socket, header_bytes, sizeof(header_bytes))) {
        connection->_server->close_connection(connection);
        return;
    }

    IAGDebugServerMessageHeader *header = reinterpret_cast<IAGDebugServerMessageHeader *>(header_bytes);
    if (header->token != connection->_server->_token) {
        connection->_server->close_connection(connection);
        return;
    }

    CFIndex length = header->body_length;
    CFMutableDataRef request_data = CFDataCreateMutable(kCFAllocatorDefault, length);
    if (!request_data) {
        connection->_server->close_connection(connection);
        return;
    }

    CFDataSetLength(request_data, length);
    void *request_bytes = CFDataGetMutableBytePtr(request_data);

    if (blocking_read(connection->_socket, request_bytes, length)) {
        CFDataRef response_data = connection->_server->receive(connection, header, request_data);
        if (response_data) {
            CFIndex response_length = CFDataGetLength(response_data);
            if (response_length >> 32 == 0) {
                header->body_length = (uint32_t)response_length;
                if (blocking_write(connection->_socket, reinterpret_cast<const void *>(header),
                                   sizeof(IAGDebugServerMessageHeader))) {
                    const unsigned char *response_bytes = CFDataGetBytePtr(response_data);
                    if (blocking_write(connection->_socket, response_bytes, response_length)) {
                        connection = nullptr; // do not close connection
                    }
                }
            }

            CFRelease(response_data);
        }
    }

    CFRelease(request_data);

    if (connection) {
        connection->_server->close_connection(connection);
    }
}

} // namespace IAG
