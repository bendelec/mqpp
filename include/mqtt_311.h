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
#include <vector>

#include "mqpp.h"

namespace mqpp {
namespace protocol {

enum class MsgType : uint8_t {
    // message type is stored in high nibble of the first byte of
    // a message (section 2.2 of mqtt 3.1.1 oasis standard)
    connect = 0x10,
    connack = 0x20,
    publish = 0x30,
    puback = 0x40,
    pubrec = 0x50,
    pubrel = 0x60,
    pubcomp = 0x70,
    subscribe = 0x80,
    suback = 0x90,
    unsubscribe = 0xa0,
    unsuback = 0xb0,
    pingreq = 0xc0,
    pingresp = 0xd0,
    disconnect = 0xe0
};

/**
 * Preliminary, I'm not yet sure what the best abstraction is
 */
class Message {
    std::vector<uint8_t> buf;

public:

    const uint8_t *data() const {
        return buf.data();
    }
        
    size_t length() const {
        return buf.size();
    }

    void append_string(std::string s) {
        buf.push_back(s.size() >> 8);
        buf.push_back(s.size() & 0xff);
        std::copy(s.begin(), s.end(), std::back_inserter(buf));
    }

    /**
     * construct a mqtt message from a buffer (for reception)
     */
    explicit Message(    const std::vector<uint8_t> &buf) 
        : buf(buf)
    {
    }

    /**
     * construct a mqtt connect message
     */
    Message(    const std::string &client_id,
                const std::chrono::duration<int> keepalive,
                const std::string &username,
                const std::string &passwd,
                const CleanSession clean_session ) 
    {
        int remlength = 6 + 1 + 1 + 2 + (2 + client_id.size());
        if(!username.empty()) remlength += (2 + username.size());
        if(!passwd.empty()) remlength += (2 + passwd.size());

        buf.push_back(static_cast<uint8_t>(MsgType::connect));
        buf.push_back(remlength & 0x7f);      // FIXME: call calculating method here
        buf.reserve(remlength + 5);           // max possible length for fixed header
        
        append_string("MQTT");
        buf.push_back(4);   // protocol level, 4 for mqtt v3.1.1
        buf.push_back(2);   // connect flags
        buf.push_back(keepalive.count() >> 8);    
        buf.push_back(keepalive.count() & 0xff);
        append_string(client_id);
        if(!username.empty()) append_string(username);
        if(!passwd.empty()) append_string(passwd);
    }

    /**
     * construct a mqtt publish message
     */
    Message(    const std::string &topic,
                const std::string &payload,
                const QoS qos,
                const Retain retain)
    {
        int remlength = 2 + topic.size() + payload.size();
        buf.push_back(      static_cast<uint8_t>(MsgType::publish) 
                        |   static_cast<uint8_t>(qos)
                        |   static_cast<uint8_t>(retain));
        buf.push_back(remlength & 0x7f);      // FIXME: call calculating method here
        buf.reserve(remlength + 5);
        append_string(topic);
        if(!payload.empty()) std::copy(payload.begin(), payload.end(), std::back_inserter(buf));
    }

    /**
     * construct a mqtt pingreq message
     */
    Message(    void )
    {
        buf.push_back(static_cast<uint8_t>(MsgType::pingreq));
        buf.push_back(0);
    }

    MsgType type() {
        return static_cast<MsgType>(buf[0] & 0xf0);
    }

    static int get_missing_length(const std::vector<uint8_t> &buf) {
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

        int pos = 1;
        int length = 0;
        int step = 0;
        
        do {
            length += (buf[pos] & 127) * 1 << (step * 7);
            ++step;
            if(step > 3) {
                return -1;
            }
        } while (buf[pos] & 128);
        return length - 3 + step;
    }
};

}   // namespace protocol
}   // namespace mqpp
