#pragma once

#include "mqtt_311.h"

#include <queue>
#include <cstring>
#include <fcntl.h>

namespace mqpp {
namespace detail {

class MqttSocket {

    int sock;

public:

    /** 
     * try to establish a socket connection 
     * 
     * This will try to establish a socket (only). 
     * @return 0 if success, -1 on any error*/
    int connect_socket( const std::string &host,
                        const int port,
                        const std::string &bind_ip);

    /** send one message out on the socket */
    int send(const protocol::Message &msg);

    /**
     * receive pending message from socket (if any)
     *
     * This method should be called cyclically from the main loop
     * It will receive data from the socket until either there is no
     * more data pending or one mqtt message is completely received.
     * In the latter case, the message will be pushed on the inbound
     * message queue. This method can also push events on the event 
     * loop, for example in case of a connection error etc.
     *
     * @return will return 1 is more data might be waiting on the socket after a mqtt
     *              message is completed, the loop mechanism can then decide wether
     *              to call receive() again or not. 0 if no more data seems to be
     *              waiting on the socket. */
    int receive(std::queue<protocol::Message> &inqueue);

private:

    int handle_recv_return(int in) {
        if (in == 0) {   // socket was closed
            // FIXME: push disconnect event here
            std::cout << "Socket was closed." << std::endl;
            exit(0);
        } if (in == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0; // no error, just no data.
            } else {
                // FIXME: push disconnect event here
                std::cout << "Socket error:" << strerror(errno) << std::endl;
                exit(0);
            }
        }
        return in;
    }

    /**
     * small helper function to make the socket nonblocking */
    inline int set_nonblock(const int socket) const {
        int flags;
        flags = fcntl(socket,F_GETFL,0);
        if(flags != -1) {
            fcntl(socket, F_SETFL, flags | O_NONBLOCK);
            return 0;
        }
        return -1;
    }
};

}   // namespace detail
}   // namespace mqpp
