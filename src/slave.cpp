#include <iostream>
#include <thread>

#include "controller/slave_controller.h"
using namespace std;
char datab[] = {1, 2, 3, 4};
int main() {
    SlaveController controller;
    controller.setID("slave1");
    controller.setMasterUri("ws://localhost:8000/websocket");
    controller.setBackupMasterUri("ws://localhost:8001/websocket");
    controller.onInit();
    controller.establishConnection();   
    // todo: remove test code
    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        controller.getServiceObject()->sendMessageToPeer(MASTER_ID, "hello");
        controller.getServiceObject()->sendMessageToPeer(BACKUP_MASTER_ID,
                                                         datab, 4);
    }
    return 0;
}