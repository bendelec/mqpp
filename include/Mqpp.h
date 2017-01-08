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
    std::queue<protocol::Message> inqueue;

    std::function<void(ConnectionState, DisconnectReason)> connect_status_callback;  
    
    std::chrono::time_point<std::chrono::steady_clock> ctrl_event, now;

public:
    Mqpp() : connstate(CONNSTATE::NOT_CONNECTED) {
    }

    int connect(    const std::string &host, 
                    const int port, 
                    const std::chrono::duration<int> keepalive,
                    const std::string &bind_ip) 
    {
        // FIXME: what should happen if we are already connected etc?
        if(0 == sock.connect_socket(host, port, bind_ip)) {  // FIXME: how to do this asynchronously?
            protocol::Message msg = protocol::build_connect_message("TestID");
            sock.send(msg);
            ctrl_event = std::chrono::steady_clock::now();
            connstate = CONNSTATE::CONNECTION_PENDING;
            return 0;
        }
        return 1;
    }

    int publish(std::string topic, std::string payload, QoS qos, Retain retain) {
        if(connstate == CONNSTATE::CONNECTED) { // FIXME: allow this later, once we have a queue || CONNSTATE::CONNECTION_PENDING)
            if(qos == QoS::at_most_once) {
                protocol::Message msg = protocol::build_publish_message(topic, payload, qos, retain);
                sock.send(msg);
                // FIXME: check mqtt spec: do we update the ping timer (ctrl_event) here?
                return 0;
            } else {
                std::cout << "QoS Not implemented yet!" << std::endl;
                exit(1);
            }
        } else {
            return -1;
        }
    }

    void set_connect_status_callback(const std::function<void(ConnectionState, DisconnectReason)> cb) {
        connect_status_callback = cb;
    }

    int loop() {
        // first receive inbound messages
        switch(connstate) {
            case CONNSTATE::CONNECTION_PENDING: 
            case CONNSTATE::CONNECTED:
            case CONNSTATE::PING_PENDING:
                    sock.receive(inqueue);
                break;
            default:
                break;
        }

        // then check pending qos messages
        
        // then check timer events
        auto now = std::chrono::steady_clock::now();
        auto ctrl_timer = now - ctrl_event;
        switch(connstate) {
            case CONNSTATE::CONNECTION_PENDING: 
            case CONNSTATE::PING_PENDING:
                if (ctrl_timer > std::chrono::seconds(1)) {
                    std::cout << "Connection attempts timed out (no CONNACK)" << std::endl;
                    exit(1);
                }
                break;
            case CONNSTATE::CONNECTED:
                if(ctrl_timer > std::chrono::seconds(5)) {
                    std::cout << "Sending PINGREQ" << std::endl;
                    ctrl_event = now;
                    sock.send(protocol::build_pingreq_message());
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
            std::cout << "*" << std::endl;
            protocol::Message msg = inqueue.front();
            inqueue.pop();
            switch(connstate) {
                case CONNSTATE::CONNECTION_PENDING: {
                    switch(msg.type) {
                        case protocol::MSGTYPE::CONNACK: {
                            connstate = CONNSTATE::CONNECTED;
                            if(connect_status_callback) {
                                connect_status_callback(ConnectionState::open, DisconnectReason::none);
                            }
                        }
                        break;
                        default: {
                            std::cout << "Unexpected Message in CONNECTION_PENDING state" << std::endl;
                        }
                        break;
                    }
                    break;
                }
            }
        }
        return 0;
    }
};

}   // namespace mqpp

