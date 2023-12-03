#ifndef __MASTER_SERVICE_H__
#define __MASTER_SERVICE_H__
#include <deque>
#include <memory>

#include "model/ScoreMatrixTask.hpp"
#include "model/ScoreMatrixTaskResponse.hpp"
#include "service/abstract_service.h"
class MasterService : public AbstractService {
   public:
    MasterService(AbstractController* controller);
    virtual void onInit() override;
    virtual void onNewMessage(std::string peer_id, const std::string& message,
                              bool is_binary) override;
    virtual void onConnectionEstablished(const std::string peer_id) override;
    virtual void onConnectionTerminated(const std::string peer_id) override;

   private:
    std::string sequence_row_;
    std::string sequence_column_;

    int column_block_size_;
    int row_block_size_;

    int match_score_;
    int mismatch_pentalty_;
    int gap_extra_;
    int gap_open_;

    // tasks_blocks[x][y] stores the response of block(x,y).
    // nullptr means this block hasn't got the response.
    std::vector<std::vector<std::shared_ptr<ScoreMatrixTaskResponse>>>
        task_blocks_;

    // history_tasks_[peer_id] stores all finished tasks of peer_id
    std::unordered_map<std::string, std::vector<std::shared_ptr<ScoreMatrixTask>>>
        history_tasks_;
    // currrent_tasks[peer_id] stores the current task of peer_id
    // all peers will appear in this map
    // if a peers is idle, its task will be nullptr;
    std::unordered_map<std::string, std::shared_ptr<ScoreMatrixTask>>
        current_tasks;

    std::deque<std::shared_ptr<ScoreMatrixTask>>task_queue_;

    std::mutex lock_;

    private:
    // generateTask construct a task object
    // and fill in fixed parameters
    // lock needs to be aquired before calling this function
    std::shared_ptr<ScoreMatrixTask>generateScoreMatrixTask(int x,int y);

    // checkDependency checks whether block(x,y) can be calculated
    // it will check whether this blocks exists so it is okay if the index overflows 
    // lock needs to be aquired before calling this function
    bool checkDependency(int x, int y);

    // assignTasks will look for idle nodes
    // and try to assign some tasks for them
    // and use multiple threads to send them
    // lock needs to be aquired before calling this function
    void assignTasks();

    // assignTaskToNode assign a task to specified node if there is any
    // lock needs to be aquired before calling this function
    void assignTaskToNode(std::string peer_id);

};

#endif