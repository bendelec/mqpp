/**
 * mqtt protocol specific code for a native
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

#include <iostream>

#include "mqpp.h"

namespace mqpp {
namespace protocol {

enum class MSGTYPE : uint8_t {
    // message type is stored in high nibble of the first byte of
    // a message (section 2.2 of mqtt 3.1.1 oasis standard)
    CONNECT = 0x10,
    CONNACK = 0x20,
    PUBLISH = 0x30,
    PUBACK = 0x40,
    PUBREC = 0x50,
    PUBREL = 0x60,
    PUBCOMP = 0x70,
    SUBSCRIBE = 0x80,
    SUBACK = 0x90,
    UNSUBSCRIBE = 0xa0,
    UNSUBACK = 0xb0,
    PINGREQ = 0xc0,
    PINGRESP = 0xd0,
    DISCONNECT = 0xe0
};

inline constexpr MSGTYPE
operator| (MSGTYPE A, QoS B) {
    return static_cast<MSGTYPE>(static_cast<uint8_t>(A) | static_cast<uint8_t>(B));
}

inline constexpr MSGTYPE
operator| (MSGTYPE A, Retain B) {
    return static_cast<MSGTYPE>(static_cast<uint8_t>(A) | static_cast<uint8_t>(B));
}

/**
 * Preliminary, I'm not yet sure what the best abstraction is
 */
struct Message {
    MSGTYPE type;
    std::string length;     // length field in mqtt big endian base 128 notation
    std::string remainder;  // raw message without msgtype and remaining_length parts
};

/**
 * return a mqtt big endian base 128 notation length field as a string
 *
 * The string length will be between 1 and 4 bytes, encoding a length of
 * 0..0x7fffffff
 */
inline std::string build_length_field(size_t s) {
    std::string ss;
    if(s < 128) {
        ss.push_back(s);
    } else {
        std::cout << "Size fields for size > 127 not yet implemented" << std::endl;
        exit(1);
    }
    return ss;
}

/**
 * create a mqtt string with 16 bit length
 */
inline std::string build_mqtt_string(std::string s) {
    if(s.size() > 65635) {
        std::cout << "Error: String longer than 65535 encountered" << std::endl;
        return std::string{};
    }
    std::string ss;
    ss.push_back(s.size() >> 8);
    ss.push_back(s.size() & 0xff);
    ss.append(s);
    return ss;
}

/**
 * build a mqtt connect message
 */
inline Message build_connect_message(std::string id) {
    Message msg;
    msg.type = MSGTYPE::CONNECT;
    msg.remainder = build_mqtt_string("MQTT");
    msg.remainder.push_back(4);   // protocol level 4 (for mqtt v3.1.1)
    msg.remainder.push_back(2);   // connect flags
    msg.remainder.push_back(0);   // keepalive MSB
    msg.remainder.push_back(5);   // keepalive LSB
    msg.remainder.append(build_mqtt_string(id));
    msg.length = build_length_field(msg.remainder.size());
    return msg;
}

/**
 * build a mqtt publish message
 */
inline Message build_publish_message(std::string topic, std::string payload, QoS qos, Retain retain) {
    Message msg;
    msg.type = MSGTYPE::PUBLISH | qos | retain;
    msg.remainder=build_mqtt_string(topic); 
    msg.remainder.append(payload);
    msg.length = build_length_field(msg.remainder.size());
    return msg;
}

/**
 * build a mqtt pingreq message
 */
inline Message build_pingreq_message() {
    Message msg;
    msg.type = MSGTYPE::PINGREQ;
    msg.remainder = "";
    msg.length = build_length_field(msg.remainder.size());
    return msg;
}

}   // namespace protocol
}   // namespace mqpp
