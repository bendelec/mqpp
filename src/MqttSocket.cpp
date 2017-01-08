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

#include <string>
#include <thread>
#include <chrono>

#include "MqttSocket.h"

namespace mqpp {
namespace detail {

using namespace std;

int MqttSocket::connect_socket( const std::string &host, 
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
        std::cout << "Error resolving hostname" << std::endl;
        exit(1);
    }
    // FIXME: Do proper error handling
    // FIXME: Walk through linked list of servinf's and look for
    //          valid entries instead of assuming the first entry
    //          is valid

    sock = socket(servinf->ai_family, servinf->ai_socktype, servinf->ai_protocol);
    if (sock < 0) {
        std::cout << "Failed to acquire socket: " << strerror(errno) << std::endl;
        exit(1);
    }

    set_nonblock(sock);

    status = ::connect(sock, servinf->ai_addr, servinf->ai_addrlen);
    // FIXME: Do proper Error Handling

    return 0;
}

int MqttSocket::send(const protocol::Message &msg) {
    size_t bytes_sent = 0;
    // FIXME: Do proper error handling and handling of incomplete
    // sends (bytes_send < msg.size())
    bytes_sent = ::send(sock, &msg.type, 1, 0);
    bytes_sent = ::send(sock, msg.length.c_str(), msg.length.size(), 0);
    bytes_sent = ::send(sock, msg.remainder.c_str(), msg.remainder.size(), 0);
    if(bytes_sent != msg.remainder.size()) {
        std::cout << "Message send incomplete, resuming/continuing not yet implemented. aborting." << std::endl;
        exit(1);
    }
    return 0;
} 

// this should be called cyclically from the global loop. It will do
// all reception from a socket (or whatever), and will push new messages
// received on the inbound message queue
int MqttSocket::receive(std::queue<protocol::Message> &inqueue) {
    // FIXME: Do proper error handling and handling of incomplete
    // messages (kind of "suspend" reception of message, return
    // 0, and resume/complete reception on the next call
    protocol::Message msg;

    while(1) {
        // receive one mqtt message if one is available
        uint8_t c;

        // receive first byte and look for message type
        if(!handle_recv_return(recv(sock, &c, 1, 0))) return 0;

        switch(c & 0xf0) {
            case 0x20:  msg.type = protocol::MSGTYPE::CONNACK;
                        std::cout << "Receiving CONNACK ";
                break;
            case 0xd0:  msg.type = protocol::MSGTYPE::PINGRESP;
                        std::cout << "Receiving PINGRESP ";
                break;
            default:
                std::cout << "Received inexpected message type: " << (int)c << std::endl;
                // FIXME: push disconnect event instead of exiting;
                exit(1);
                break;
        }
        
        // now get remaining length

        // example "pseudo code" directly from the standard document:
        // multiplier = 1
        // value = 0
        // do
        //    encodedByte = 'next byte from stream'
        //    value += (encodedByte AND 127) * multiplier
        //    multiplier *= 128
        //    if (multiplier > 128*128*128)
        //       throw Error(Malformed Remaining Length)
        // while ((encodedByte AND 128) != 0)

        int length = 0;
        int multiplier = 1;

        while(handle_recv_return(recv(sock, &c, 1, 0))) {
            msg.length.push_back(c);
            length += (c & 127) * multiplier;
            multiplier *= 128;
            if(multiplier > 128*128*128) {
                std::cout << "Received illegal remaining length field." << std::endl;
                //FIXME: push disconnect event instead of exiting
                exit(1);
            }
            if(!(c & 128)) break;
        } 

        std::cout << "- length is " << length;
        
        // now get the rest of the message
        if(length > 0) {
            msg.remainder.resize(length, 0);
            if(int bytes_recvd = handle_recv_return(recv(sock, const_cast<char*>(msg.remainder.data()), length, 0))) {
                inqueue.push(msg);
                std::cout << " - received " << bytes_recvd << " bytes";
                if(length == bytes_recvd) std::cout << " - done." << std::endl;
                return 1;
            } else {
                return 0;
            }
        } else {
            std::cout << " - done." << std::endl;
            return 0;
        }
    }
}


}   // namespace detail
}   // namespace mqpp


