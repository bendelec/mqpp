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

#pragma once

#include <chrono>
#include <functional>

#include "mqpp.h"
#include "MqttSocket.h"

namespace mqpp {

class mqtt_client::Mqpp {
    enum class CONNSTATE {
        NOT_CONNECTED,
        CONNECTION_PENDING,
        CONNECTED,
        PING_PENDING,
        SHUTTING_DOWN
    } connstate;

    detail::MqttSocket sock;
    std::deque<protocol::Message> inqueue;

    std::function<void(ConnectionState, DisconnectReason)> connect_status_callback;  
    std::function<void(LogLevel, std::string)> logging_callback;
    
    std::chrono::time_point<std::chrono::steady_clock> ctrl_event, now;


public:
    Mqpp();

    int connect(    const std::string &host, 
                    const int port, 
                    const std::chrono::duration<int> keepalive,
                    const std::string &bind_ip); 
    int publish(std::string topic, std::string payload, QoS qos, Retain retain);

    inline void set_connect_status_callback(const std::function<void(ConnectionState, DisconnectReason)> &cb) {
        connect_status_callback = cb;
    }

    inline void set_logging_callback(const std::function<void(LogLevel, std::string)> &cb, LogLevel lvl) {
        logging_callback = cb;
    }

    int loop();

private:
    void log(LogLevel lvl, std::string text);
};

}   // namespace mqpp

