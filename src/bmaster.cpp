#include <iostream>
#include <thread>

#include "controller/backup_master_controller.h"

int main() {
    BackupMasterController controller;
    controller.onInit();
    controller.setMasterUri("ws://localhost:8000/websocket");
    controller.establishConnection();
    // todo: remove test code
    std::thread([&controller]() {
        while (1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            controller.getServiceObject()->sendMessageToPeer(
                MASTER_ID, "hello from backup master");
        }
    }).detach();
    controller.run(8001);
}