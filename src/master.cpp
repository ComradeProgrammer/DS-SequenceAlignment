#include "config/configuration.h"
#include "controller/master_controller.h"
#include "gflags/gflags.h"

DEFINE_string(config, "", "path to the json config file ");
DEFINE_int32(port, 8000, "port on which the websocket listens");
DEFINE_bool(backup, true, "port on which the websocket listens");
int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    auto config = std::make_shared<Configuration>();
    config->loadFromFile(FLAGS_config);

    MasterController controller;
    controller.onInit(config, FLAGS_backup);
    controller.run(FLAGS_port);
    return 0;
}