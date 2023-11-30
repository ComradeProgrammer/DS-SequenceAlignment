#include "service/backup_master_service.h"

void BackupMasterService::onInit() {}

void BackupMasterService::onNewMessage(std::string peer_id,
                                       const std::string& message,
                                       bool is_binary) {}

void BackupMasterService::onConnectionEstablished(const std::string peer_id) {
    std::lock_guard<std::mutex> lock_function(lock_);
    existing_peers_.insert(peer_id);
}

void BackupMasterService::onConnectionTerminated(const std::string peer_id) {}
