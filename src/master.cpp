#include "controller/master_controller.h"
#include "gflags/gflags.h"

DEFINE_int32(port, 8000, "port on which the websocket listens");
int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    MasterController controller;
    controller.onInit();
    controller.run(FLAGS_port);
    return 0;
}