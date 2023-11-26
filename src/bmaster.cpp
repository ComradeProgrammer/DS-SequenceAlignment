#include <iostream>
#include <thread>

#include "controller/backup_master_controller.h"
#include "gflags/gflags.h"

DEFINE_int32(port, 8001, "port on which the websocket listens");
DEFINE_string(master_uri, "",
              "uri of the master node's websocket endpoint, e.g. "
              "ws://localhost:8000/websocket");
int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    BackupMasterController controller;
    controller.onInit();
    controller.setMasterUri(FLAGS_master_uri);
    controller.establishConnection();
    // todo: remove test code
    // std::thread([&controller]() {
    //     while (1) {
    //         std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    //         controller.getServiceObject()->sendMessageToPeer(
    //             MASTER_ID, "hello from backup master");
    //     }
    // }).detach();
    controller.run(FLAGS_port);
}