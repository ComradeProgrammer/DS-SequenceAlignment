#ifndef __STATE_SYNC_OBJECT_HPP__
#define __STATE_SYNC_OBJECT_HPP__
#include "model/AbstractJsonObject.h"
#include "model/AbstractTask.hpp"
#include "model/ScoreMatrixTask.hpp"
#include "model/TracebackTask.hpp"
class StateSyncObject : public AbstractJsonObject {
   public:
    virtual ~StateSyncObject() = default;
    virtual nlohmann::json toJsonObject() override {
        nlohmann::json j;
        j["syncType"] = sync_type_;
        j["peerID"] = peer_id_;
        j["type"] = "StateSyncObject";
        if (task_ != nullptr) {
            j["task"] = task_->toJsonObject();
        }
        return j;
    }
    virtual void loadFromJsonObject(const nlohmann::json& j) override {
        peer_id_ = j["peerID"].template get<std::string>();
        sync_type_ = j["syncType"].template get<std::string>();
        if (j.contains("task")) {
            std::string object_type =
                j["task"]["type"].template get<std::string>();
            // std::this_thread::sleep_for(std::chrono::milliseconds(5000));
            if (object_type == "ScoreMatrixTask") {
                task_ = std::make_shared<ScoreMatrixTask>();
                task_->loadFromJsonObject(j["task"]);
            } else {
                task_ = std::make_shared<TracebackTask>();
                task_->loadFromJsonObject(j["task"]);
            }
        } else {
            task_ = nullptr;
        }
    }

   public:
    // "queuePushFront" "queuePushBack" "assignTask" "finishTask"
    std::string sync_type_;
    std::string peer_id_;
    std::shared_ptr<AbstractTask> task_;
};
#endif