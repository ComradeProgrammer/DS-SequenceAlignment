#ifndef __BACKUP_MASTER_SERVICE_H__
#define __BACKUP_MASTER_SERVICE_H__
#include "model/StateSyncObject.hpp"
#include "service/abstract_service.h"
#include "service/master_service.h"

// task queue and current task  is modified by the sync from
// master score_matrix_task_peer_id_ and score_matrix_task_blocks_ can be
// modified by the slave
class BackupMasterService : public MasterService {
   public:
    BackupMasterService(AbstractController* controller)
        : MasterService(controller) {
        is_master_ = false;
        master_on_ = true;
    }
    virtual void onInit() override;
    virtual void onNewMessage(std::string peer_id, const std::string& message,
                              bool is_binary) override;
    virtual void onConnectionEstablished(const std::string peer_id) override;
    virtual void onConnectionTerminated(const std::string peer_id) override;

   protected:
    bool master_on_ = true;

   protected:
    virtual void onScoreMatrixTaskResponse(
        std::string peer_id,
        std::shared_ptr<ScoreMatrixTaskResponse> response) override;
    virtual void onTracebackTaskResponse(
        std::string peer_id,
        std::shared_ptr<TracebackTaskResponse> response) override;
    virtual void onStateSyncObject(std::string peer_id,
                                   std::shared_ptr<StateSyncObject> response);
};

#endif