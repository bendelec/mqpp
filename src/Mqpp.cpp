/**
 * Implementation class (pimpl idiom) declaration for a native
 * C++ mqtt client
 *
 * This contains the queue handling and state machines and is the
 * central part of the client library.
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

#include <chrono>
#include <functional>

#include "mqpp.h"
#include "MqttSocket.h"
#include "Mqpp.h"

namespace mqpp {

using namespace detail;

    mqtt_client::Mqpp::Mqpp() : connstate(CONNSTATE::NOT_CONNECTED) {
    }

    int mqtt_client::Mqpp::connect(    const std::string &host, 
                    const int port, 
                    const std::chrono::duration<int> keepalive,
                    const std::string &bind_ip) 
    {
        // FIXME: what should happen if we are already connected etc?
        SocketState res = sock.connect_socket(host, port, bind_ip);
        switch (res) {
            case SocketState::tcp_connected:
                {
                    protocol::Message msg("TestID", std::chrono::seconds(20), "", "", CleanSession::yes);
                    if(sock.send(msg)) {
                        log(LogLevel::warn, "Couldn't send CONNECT message (socket send failed)");
                        return 1;
                    }
                    ctrl_event = std::chrono::steady_clock::now();
                    connstate = CONNSTATE::CONNECTION_PENDING;
                    log(LogLevel::info, "Sent CONNECT message, connstate is now pending");
                    return 0;
                }
                break;
            case SocketState::resolv_error:
                log(LogLevel::error, "Error extablishing connection: can't resolv hostname");
                break;
            case SocketState::socket_error:
                log(LogLevel::error, "Error getting socket for connection");
                break;
            case SocketState::connect_error:
                log(LogLevel::error, "Error establishing TCP connection");
                break;
        }
         

        return 1;
    }

    int mqtt_client::Mqpp::publish(std::string topic, std::string payload, QoS qos, Retain retain) {
        if(connstate == CONNSTATE::CONNECTED) { // FIXME: allow this later, once we have a queue || CONNSTATE::CONNECTION_PENDING)
            if(qos == QoS::at_most_once) {
                protocol::Message msg(topic, payload, qos, retain);
                sock.send(msg);
                // FIXME: check mqtt spec: do we update the ping timer (ctrl_event) here?
                return 0;
            } else {
                log(LogLevel::error, "QoS Not implemented yet!");
                exit(1);
            }
        } else {
            return -1;
        }
    }

    int mqtt_client::Mqpp::loop() {
        // first receive inbound messages
        switch(connstate) {
            case CONNSTATE::CONNECTION_PENDING: 
            case CONNSTATE::CONNECTED:
            case CONNSTATE::PING_PENDING:
                    if(sock.receive(inqueue)) {
                        log(LogLevel::trace, "Received and enqueued a Message from the Broker");
                    }
                break;
            default:
                break;
        }

        // then check pending (outbound) qos message epochs
        
        // then check timer events
        auto now = std::chrono::steady_clock::now();
        auto ctrl_timer = now - ctrl_event;
        switch(connstate) {
            case CONNSTATE::CONNECTION_PENDING: 
            case CONNSTATE::PING_PENDING:
                if (ctrl_timer > std::chrono::seconds(1)) {
                    log(LogLevel::warn, "Connection attempts timed out (no CONNACK)");
                    exit(1);
                }
                break;
            case CONNSTATE::CONNECTED:
                if(ctrl_timer > std::chrono::seconds(5)) {
                    log(LogLevel::info, "Sending PINGREQ");
                    ctrl_event = now;
                    sock.send(protocol::Message());
                }
                break;
            default:
                break;
        }
        // then serve outbound msg queue
        // FIXME: currently not necessary/implemented, since outbound
        // messages are sent directly without enqueuing, blocking the caller.
        // This will be fixed later

        // finally serve global event queue
        if(!inqueue.empty()) {
            protocol::Message msg = inqueue.front();
            inqueue.pop_front();
            switch(connstate) {
                case CONNSTATE::CONNECTION_PENDING: {
                    switch(msg.type()) {
                        case protocol::MsgType::connack: {
                            connstate = CONNSTATE::CONNECTED;
                            if(connect_status_callback) {
                                connect_status_callback(ConnectionState::open, DisconnectReason::none);
                            }
                        }
                        break;
                        default: {
                            log(LogLevel::warn, "Unexpected Message in CONNECTION_PENDING state");
                        }
                        break;
                    }
                    break;
                }
                break;
                case CONNSTATE::PING_PENDING: {
                    switch(msg.type()) {
                        case protocol::MsgType::pingresp: {
                            connstate = CONNSTATE::CONNECTED;
                        }
                        break;
                        default: {
                            log(LogLevel::warn, "Unexpected Message in CONNECTION_PENDING state");
                        }
                    }
                }
                break;
            }
        }
        return 0;
    }

    void mqtt_client::Mqpp::log(LogLevel lvl, std::string text) {
        if(logging_callback) {
            logging_callback(lvl, text);
        }
    }

 
}   // namespace mqpp

