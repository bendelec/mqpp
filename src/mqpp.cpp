/**
 * Interface implementation (pimpl idiom) for a native
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

#include "mqpp.h"   // contains the interface definition
#include "Mqpp.h"   // contains the declaration of the implementation class

namespace mqpp {

mqtt_client::mqtt_client() : impl{new(Mqpp)} {}

mqtt_client::~mqtt_client() {
}

void mqtt_client::connect(  const std::string &host, 
                            const int port, 
                            const std::chrono::duration<int> keepalive, 
                            const std::string &bind_ip) {
    impl->connect(host, port, keepalive, bind_ip);
}

void mqtt_client::publish(  std::string topic, 
                            std::string payload, 
                            QoS qos, 
                            Retain retain) 
{
    impl->publish(topic, payload, qos, retain);
}

void mqtt_client::set_logging_callback(  const std::function<void(LogLevel, std::string)> &cb, 
                            LogLevel lvl)
{
    impl->set_logging_callback(cb, lvl);
}

void mqtt_client::set_connect_status_callback(const std::function<void(ConnectionState, DisconnectReason)> &cb) {
    impl->set_connect_status_callback(cb);
}

int mqtt_client::loop() 
{
    return impl->loop();
}

}   // namespace mqpp
