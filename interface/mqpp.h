#pragma once

#include <string>
#include <memory>
#include <chrono>

namespace mqpp {

enum class Retain : uint8_t {
    no      = 0x00,
    yes     = 0x01
};

enum class QoS : uint8_t {
    at_most_once = 0x00 << 1,
    at_least_once = 0x01 << 1,
    exactly_once = 0x02 << 1
};

enum class ConnectionState {
    closed,
    establishing,
    open,
    closing
};

enum class DisconnectReason {
    none,
    socket_error,
    
};

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
