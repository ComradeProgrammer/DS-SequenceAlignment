#include"controller/master_controller.h"

int main()
{
    MasterController controller;
    controller.onInit();
    controller.run(8000);
}