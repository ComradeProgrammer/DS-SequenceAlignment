#include "service/master_service.h"

#include "controller/abstract_controller.h"
#include "service/master_service.h"
MasterService::MasterService(AbstractController* controller)
    : AbstractService(controller) {
        // constructor
}
void MasterService::onInit() {}

void MasterService::onNewMessage(std::string peer_id, std::string&& message,
                                 bool is_binary) {}

void MasterService::onConnectionEstablished(const std::string peer_id) {}

void MasterService::onConnectionTerminated(const std::string peer_id) {}
