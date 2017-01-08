/**
 * Interface definition (tentative, unstable, will change!) for a native
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
#pragma once

#include <string>
#include <chrono>
#include <memory>

namespace mqpp {

enum class CleanSession : uint8_t {
    no  = 0x00 << 1,
    yes = 0x01 << 1
};

/**
 * Retain messages on the broker when publishing?
 *  see mqtt documentation for details
 *  passed as last parameter to publish()
 */
enum class Retain : uint8_t {
    no      = 0x00,
    yes     = 0x01
};

/**
 * Quality of Service Level
 *  for messages, topic subscriptions etc.
 */
enum class QoS : uint8_t {
    at_most_once = 0x00 << 1,
    at_least_once = 0x01 << 1,
    exactly_once = 0x02 << 1
};

/**
 * Current state of the mqtt connection
 * as passed to the connect_status_callback
 */
enum class ConnectionState {
    closed,
    establishing,
    open,
    closing
};

/**
 * Reason for a disconnect
 * as passed to the connect_status_callback
 * will always be "none" if the connectionstate is establishing
 * or open
 */
enum class DisconnectReason {
    none,
    socket_error,
    
};

enum class LogLevel {
    trace,
    info,
    warn,
    error
};

/**
 * All api is meant to be asynchronous, so typically results  of
 * api calls will be passed to the client by way of callbacks.
 * Therefore all return types are void
 */
class mqtt_client {

public:
    mqtt_client();
    ~mqtt_client();

    void connect(   const std::string &host = "localhost", 
                    const int port = 1883, 
                    const std::chrono::duration<int> keepalive = std::chrono::seconds(20), 
                    const std::string &bind_ip = "");
    
    void set_reconnect_opts(int first_delay_s = 1, int max_delay_s = 64, bool exponential_delay = true);
    void set_qos_opts(int retry_s = 10, int max_inflight_messages = 0);

    void set_logging_callback(const std::function<void(LogLevel, std::string)> &cb, LogLevel lvl = LogLevel::warn);

    void set_connect_status_callback(const std::function<void(ConnectionState, DisconnectReason)> &cb);
    void set_message_callback();
    void set_publish_callback();
    void set_subscribe_callback();
    void set_unsubscribe_callback();

    void publish(std::string topic, std::string payload, QoS qos=QoS::at_most_once, Retain retain=Retain::yes);
    void subscribe();
    void unsubscribe();

    // FIXME: temporary for initial development, remove later:
    int loop();

private:

    class Mqpp;
    std::unique_ptr<Mqpp> impl;

};

}   // namespace mqpp
