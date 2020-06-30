#ifndef AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

#include <cstring>

#include <sys/epoll.h>

#include <protocol/Parser.h>
#include <spdlog/logger.h>
#include <afina/execute/Command.h>

namespace Afina {
namespace Network {
namespace STnonblock {

class Connection {
public:
    Connection(int s, std::shared_ptr<Afina::Storage> ps)
    : _socket(s),_pStorage(ps) {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
        is_alive = true;
        written_bytes = read_bytes = 0;
        arg_remains = 0;
    }

    inline bool isAlive() const { return is_alive; }

    void Start();

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:
    friend class ServerImpl;
    std::shared_ptr<Afina::Storage> _pStorage;

    int _socket;
    struct epoll_event _event;
    bool is_alive;
    std::vector<std::string> outputs;
    int written_bytes,  read_bytes;
    char client_buffer[4096];
    std::size_t arg_remains;
    Protocol::Parser parser;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;

};

} // namespace STnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
