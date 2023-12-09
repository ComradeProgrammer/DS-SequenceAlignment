#ifndef __MASTER_SERVICE_H__
#define __MASTER_SERVICE_H__
#include <deque>
#include <memory>

#include "config/configuration.h"
#include "model/ScoreMatrixTask.hpp"
#include "model/ScoreMatrixTaskResponse.hpp"
#include "model/StateSyncObject.hpp"
#include "model/TracebackTask.hpp"
#include "model/TracebackTaskResponse.hpp"
#include "nlohmann/json.hpp"
#include "service/abstract_service.h"
class MasterService : public AbstractService {
   public:
    MasterService(AbstractController* controller);
    virtual void onInit() override;
    virtual void onNewMessage(std::string peer_id, const std::string& message,
                              bool is_binary) override;
    virtual void onConnectionEstablished(const std::string peer_id) override;
    virtual void onConnectionTerminated(const std::string peer_id) override;
    virtual void setConfiguration(std::shared_ptr<Configuration> config) {
        config_ = config;
    }

   protected:
    std::shared_ptr<Configuration> config_;

    std::string sequence_row_;
    std::string sequence_column_;

    int column_block_size_;
    int row_block_size_;
    // parameters for the sw algorithm
    int match_score_;
    int mismatch_pentalty_;
    // int gap_extra_;
    int gap_open_;

    // records where the max score show up
    int max_score_ = 0;
    int max_score_x_;
    int max_score_y_;

    int init_wait_ = 0;

    // records the best aligned sequence
    std::string result_;

    // score_matrix_task_blocks_[x][y] stores the response of block(x,y).
    // nullptr means this block hasn't got the response.
    std::vector<std::vector<std::shared_ptr<ScoreMatrixTaskResponse>>>
        score_matrix_task_blocks_;
    // score_matrix_task_peer_id_ stores which peer does the score matrix task
    // belongs to
    std::vector<std::vector<std::string>> score_matrix_task_peer_id_;

    // core_matrix_history_tasks_[peer_id] stores all finished tasks of peer_id
    std::unordered_map<std::string,
                       std::vector<std::shared_ptr<ScoreMatrixTask>>>
        score_matrix_history_tasks_;
    // currrent_tasks[peer_id] stores the current task of peer_id
    // all peers will appear in this map
    // if a peers is idle, its task will be nullptr;
    std::unordered_map<std::string, std::shared_ptr<AbstractTask>>
        current_tasks;

    std::deque<std::shared_ptr<AbstractTask>> task_queue_;

    std::mutex lock_;

    bool is_master_ = true;
    bool is_backup_master_online_ = false;

    std::deque<std::shared_ptr<StateSyncObject>> to_back_master_queue_;

   protected:
    virtual void onScoreMatrixTaskResponse(
        std::string peer_id, std::shared_ptr<ScoreMatrixTaskResponse> response);
    virtual void onTracebackTaskResponse(
        std::string peer_id, std::shared_ptr<TracebackTaskResponse> response);

    // generateTask construct a task object
    // and fill in fixed parameters
    // lock needs to be aquired before calling this function
    std::shared_ptr<ScoreMatrixTask> generateScoreMatrixTask(int x, int y);

    // checkDependency checks whether block(x,y) can be calculated
    // it will check whether this blocks exists so it is okay if the index
    // overflows lock needs to be aquired before calling this function
    virtual bool checkDependencyForScoreMatrix(int x, int y);

    // assignTasks will look for idle nodes
    // and try to assign some tasks for them
    // and use multiple threads to send them
    // lock needs to be aquired before calling this function
    virtual void assignTasks();

    // assignTaskToNode assign a task to specified node if there is any
    // lock needs to be aquired before calling this function
    virtual void sendTaskToNode(std::string peer_id,
                                std::shared_ptr<AbstractTask> task);

    // genearteTracebackTask generate a traceback task for a specified point
    virtual std::shared_ptr<TracebackTask> genearteTracebackTask(int prev_x,
                                                                 int prev_y);

    virtual void getSequence(std::string data_type, std::string data_source,
                             std::string& out_res);
};

#endif