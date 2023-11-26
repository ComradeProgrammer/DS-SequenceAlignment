#include "service/abstract_service.h"

#include <iostream>

#include "controller/abstract_controller.h"

void AbstractService::sendMessageToPeer(const std::string& peer_id,
                                        const std::string& message) {
    controller_->sendMessageToPeer(peer_id, message);
}
void AbstractService::sendMessageToPeer(const std::string& peer_id,
                                        const char* data, const uint64_t len) {
    controller_->sendMessageToPeer(peer_id, data, len);
}