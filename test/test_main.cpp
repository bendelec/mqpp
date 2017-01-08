#include <thread>
#include <chrono>
#include <iostream>

#include "mqpp.h"

void connect_status_callback(mqpp::ConnectionState state, mqpp::DisconnectReason reason) {
    std::cout << "Oh! I reveived a connect status callback." << std::endl;
}

int main(int argc, char *argv[]) {
    mqpp::mqtt_client instance;
    instance.set_connect_status_callback(connect_status_callback);
    instance.connect();
    int count = 0;
    while(!instance.loop()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if(count == 30) {
            instance.publish("foo/bar", "Teststring");
        }

        ++count;
    } 
}


