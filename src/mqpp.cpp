#include "mqpp.h"
#include "Mqpp.h"

namespace mqpp {

mqtt_client::mqtt_client() : impl{new(Mqpp)} {}

mqtt_client::~mqtt_client() {}

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

void mqtt_client::set_connect_status_callback(const std::function<void(ConnectionState, DisconnectReason)> &cb) {
    impl->set_connect_status_callback(cb);
}

int mqtt_client::loop() 
{
    return impl->loop();
}

}   // namespace mqpp
