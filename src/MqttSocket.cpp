/**
 * Socket / Network handling component for a native
 * C++ mqtt client
 *
 * Copyright 2017 Christian Bendele
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <thread>
#include <chrono>

#include "MqttSocket.h"

namespace mqpp {
namespace detail {

using namespace std;

MqttSocket::MqttSocket() : sock(-1) {}

SocketState MqttSocket::connect_socket( const std::string &host, 
                                const int port, 
                                const std::string &bind_ip) 
{
    int status;
    struct addrinfo hints {};
    struct addrinfo *servinf;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(host.c_str(), "1883", &hints, &servinf);
    if(status != 0) {
        return(SocketState::resolv_error);
    }
    // FIXME: Do proper error handling
    // FIXME: Walk through linked list of servinf's and look for
    //          valid entries instead of assuming the first entry
    //          is valid

    sock = socket(servinf->ai_family, servinf->ai_socktype, servinf->ai_protocol);
    if (sock < 0) {
        return(SocketState::socket_error);
    }

    set_nonblock(sock);

    status = ::connect(sock, servinf->ai_addr, servinf->ai_addrlen);
    if(status != 0) {
        return(SocketState::connect_error);
    }
    // FIXME: Do proper Error Handling

    return SocketState::tcp_connected;
}

int MqttSocket::send(const protocol::Message &msg) {
    // FIXME: Do proper error handling and handling of incomplete
    // sends (bytes_send < msg.size())
    size_t bytes_sent = ::send(sock, msg.data(), msg.length(), 0);
    if(bytes_sent != msg.length()) {
        return -1;
    }
    return 0;
} 

// this should be called cyclically from the global loop. It will do
// all reception from a socket (or whatever), and will push new messages
// received on the inbound message queue
int MqttSocket::receive(std::deque<protocol::Message> &inqueue) {
    // FIXME: Do proper error handling and handling of incomplete
    // messages (kind of "suspend" reception of message, return
    // 0, and resume/complete reception on the next call

    std::vector<uint8_t> buf(5);
    int result = recv(sock, buf.data(), buf.size(), 0);
    if(result != -1) buf.resize(result);
    if(!handle_recv_return(result)) return 0;

    int missing = protocol::Message::get_missing_length(buf);
    
    if(missing > 0) {
        buf.resize(result + missing);
        result = recv(sock, buf.data()+5, buf.size()-5, 0);
        if(result != -1) buf.resize(result);
        if(!handle_recv_return(result)) return 0;
    }
            
    protocol::Message msg(buf);

    inqueue.push_back(msg);
    return 1;
}

}   // namespace detail
}   // namespace mqpp


