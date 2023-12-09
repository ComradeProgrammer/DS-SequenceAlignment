#ifndef __IDENTIFICATION_INFO_HPP__
#define __IDENTIFICATION_INFO_HPP__
#include <string>

#include "model/AbstractJsonObject.h"
class IdentificationInfo : public AbstractJsonObject {
   public:
    virtual ~IdentificationInfo() = default;
    std::string peer_id_;
    bool is_backup_master_;

    std::string getPeerID() { return peer_id_; }
    bool isBackUpMaster() { return is_backup_master_; }

    virtual nlohmann::json toJsonObject()override {
        nlohmann::json j;
        j["type"] = "IdentificationInfo";
        j["peerID"] = peer_id_;
        j["isBackUpMaster"] = is_backup_master_;
        return j;
    }

    virtual void loadFromJsonObject(const nlohmann::json& j) override {
        peer_id_ = j["peerID"].template get<std::string>();
        is_backup_master_ = j["isBackUpMaster"].template get<bool>();
    }
};
#endif