#include <thread>
#include<iostream>
#include "controller/slave_controller.h"
using namespace std;
char datab[] = {1, 2, 3, 4};
int main() {
    SlaveController controller;
    controller.setMasterUri("ws://localhost:8000/websocket");
    controller.establishConnection();
    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        controller.sendMessageToPeer(MASTER_ID, "hello");
        controller.sendMessageToPeer(MASTER_ID, datab, 4);
    }
    return 0;
}