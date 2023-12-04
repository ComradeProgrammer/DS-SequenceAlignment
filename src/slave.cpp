#include <iostream>
#include <thread>

#include "controller/slave_controller.h"
#include "gflags/gflags.h"
using namespace std;
DEFINE_string(node_id, "", "id of the slave node");
DEFINE_string(master_uri, "",
              "uri of the master node's websocket endpoint, e.g. "
              "ws://localhost:8000/websocket");
DEFINE_string(backup_master_uri, "",
              "uri of the backup master node's websocket endpoint, e.g. "
              "ws://localhost:8001/websocket");

int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    SlaveController controller;
    controller.setID(FLAGS_node_id);
    controller.setMasterUri(FLAGS_master_uri);
    controller.setBackupMasterUri(FLAGS_backup_master_uri);
    controller.onInit();
    controller.establishConnection();
    
    // todo: remove test code
    // char datab[] = {1, 2, 3, 4};
    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return 0;
}