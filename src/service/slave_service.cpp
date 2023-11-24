#include "service/slave_service.h"

void SlaveService::onInit() {}

void SlaveService::onNewMessage(std::string peer_id, std::string&& message,
                                bool is_binary) {}

void SlaveService::onConnectionEstablished(const std::string peer_id) {}

void SlaveService::onConnectionTerminated(const std::string peer_id) {}
