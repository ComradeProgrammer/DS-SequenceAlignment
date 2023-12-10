#include "service/backup_master_service.h"

#include "controller/abstract_controller.h"
#include "crow.h"
#include "service/master_service.h"
#include "util/util.h"
using namespace std;
using nlohmann::json;

void BackupMasterService::onInit() {
    if (config_ != nullptr) {
        column_block_size_ = config_->column_block_size_;
        row_block_size_ = config_->row_block_size_;
        match_score_ = config_->match_score_;
        mismatch_pentalty_ = config_->mismatch_penalty_;
        gap_open_ = config_->gap_open_;
        init_wait_ = config_->master_wait_time;
        for (int i = 0; i < config_->row_sequences.size(); i++) {
            string sequence_row;
            getSequence(config_->row_sequences[i].sequence_row_type_,
                        config_->row_sequences[i].sequence_row_data_source_,
                        sequence_row);
            sequence_row_.push_back(sequence_row);
        }

        getSequence(config_->sequence_column_type_,
                    config_->sequence_column_data_source_, sequence_column_);
        // Print the current config param for good
        CROW_LOG_INFO << "column_block_size: " << column_block_size_;
        CROW_LOG_INFO << "row_block_size: " << row_block_size_;
        CROW_LOG_INFO << "match_score: " << match_score_;
        CROW_LOG_INFO << "mismatch_pentalty: " << mismatch_pentalty_;
        CROW_LOG_INFO << "gap_open: " << gap_open_;
        CROW_LOG_INFO << "init_wait: " << init_wait_;
        for (int i = 0; i < sequence_row_.size(); i++) {
            CROW_LOG_INFO << "sequence_row[" << i << "]: " << sequence_row_[i];
        }
        CROW_LOG_INFO << "sequence_column: " << sequence_column_;
    }
    max_score_ = vector<int>(sequence_row_.size(), -1);
    max_score_x_ = vector<int>(sequence_row_.size());
    max_score_y_ = vector<int>(sequence_row_.size());
    result_ = vector<string>(sequence_row_.size(), "");

    for (int j = 0; j < sequence_row_.size(); j++) {
        // calculate how many rows and columns of block and init the
        // tasks_blocks
        int sequence_row_length = sequence_column_.size();
        int sequence_column_length = sequence_row_[j].size();

        int row_num = (sequence_row_length % row_block_size_) == 0
                          ? (sequence_row_length / row_block_size_)
                          : (sequence_row_length / row_block_size_ + 1);
        int column_num =
            (sequence_column_length % column_block_size_) == 0
                ? (sequence_column_length / column_block_size_)
                : (sequence_column_length / column_block_size_ + 1);

        score_matrix_task_blocks_.push_back(
            vector<vector<shared_ptr<ScoreMatrixTaskResponse>>>());
        score_matrix_task_peer_id_.push_back(vector<vector<string>>());
        score_matrix_history_tasks_.push_back({});

        for (int i = 0; i < row_num; i++) {
            score_matrix_task_blocks_[j].push_back(
                vector<shared_ptr<ScoreMatrixTaskResponse>>(column_num,
                                                            nullptr));
            score_matrix_task_peer_id_[j].push_back(
                vector<string>(column_num, ""));
        }

        // init the task queue with the first task
        auto task = generateScoreMatrixTask(j, 0, 0);
        task_queue_.push_back(task);
        CROW_LOG_INFO << "add task " << task->getShortName() << " into queue";
    }
}

void BackupMasterService::onNewMessage(std::string peer_id,
                                       const std::string& message,
                                       bool is_binary) {
    CROW_LOG_INFO << "new message from " << peer_id << ",: " << message;
    // first parse the message and try to identify the type
    json j;
    try {
        j = json::parse(message);
    } catch (const json::exception& e) {
        CROW_LOG_ERROR << "failed to parse: " << e.what();
        return;
    }
    string object_type = j["type"].template get<string>();
    lock_guard<mutex> lock_block(lock_);
    if (object_type == "ScoreMatrixTaskResponse") {
        auto response = make_shared<ScoreMatrixTaskResponse>();
        response->loadFromJsonObject(j);
        onScoreMatrixTaskResponse(peer_id, response);
    } else if (object_type == "TracebackTaskResponse") {
        auto response = make_shared<TracebackTaskResponse>();
        response->loadFromJsonObject(j);
        onTracebackTaskResponse(peer_id, response);
    } else if (object_type == "StateSyncObject") {
        auto response = make_shared<StateSyncObject>();
        response->loadFromJsonObject(j);
        onStateSyncObject(peer_id, response);
    }
}
void BackupMasterService::onStateSyncObject(
    string, shared_ptr<StateSyncObject> response) {
    if (response->sync_type_ == "queuePushBack") {
        task_queue_.push_back(response->task_);
    } else if (response->sync_type_ == "queuePushFront") {
        task_queue_.push_front(response->task_);
    } else if (response->sync_type_ == "assignTask") {
        task_queue_.pop_front();
        current_tasks[response->peer_id_] = response->task_;
        auto score_task =
            dynamic_pointer_cast<ScoreMatrixTask>(response->task_);
        if (score_task != nullptr) {
            score_matrix_task_peer_id_[score_task->row_id_][score_task->x_]
                                      [score_task->y_] = "#pending";
        }
    } else if (response->sync_type_ == "finishTask") {
        current_tasks[response->peer_id_] = nullptr;
        auto task = response->task_;
        auto score_task = dynamic_pointer_cast<ScoreMatrixTask>(task);
        if (score_task != nullptr) {
            score_matrix_history_tasks_[score_task->row_id_][response->peer_id_]
                .push_back(score_task);
        }
    }
}

void BackupMasterService::onConnectionEstablished(const std::string peer_id) {
    lock_guard<mutex> lock_block(lock_);
    CROW_LOG_INFO << "peer " << peer_id << " connected";
    if (peer_id != MASTER_ID) {
        for (int i = 0; i < sequence_row_.size(); i++) {
            score_matrix_history_tasks_[i][peer_id] = {};
        }
    } else {
        master_on_ = true;
    }
    if (!master_on_) {
        // trigger resending tasks
        assignTasks();
    }
}

void BackupMasterService::onConnectionTerminated(const std::string peer_id) {
    lock_guard<mutex> lock_block(lock_);
    CROW_LOG_INFO << "peer " << peer_id << " disconnected";
    if (peer_id == MASTER_ID) {
        // if the backup master is down, we just do nothing
        CROW_LOG_INFO << "master offline! backup master takes over";

        master_on_ = false;
        assignTasks();
        return;
    }

    if (!master_on_) {
        // original bussiness logic of master node

        // sequence of putting back tasks is essential!
        // put current task into the front of the queue
        //  we need to put all historical tasks into the front of the queue
        if (current_tasks.find(peer_id) != current_tasks.end()) {
            auto task = current_tasks[peer_id];

            if (task != nullptr) {
                task_queue_.push_front(task);
                CROW_LOG_INFO << "put back " << task->getShortName()
                              << "into taskqueue";
            }
        }
        current_tasks.erase(peer_id);

        for (int k = 0; k < sequence_row_.size(); k++) {
            if (score_matrix_history_tasks_[k].find(peer_id) !=
                score_matrix_history_tasks_[k].end()) {
                auto& history_tasks = score_matrix_history_tasks_[k][peer_id];
                for (int i = history_tasks.size() - 1; i >= 0; i--) {
                    task_queue_.push_front(history_tasks[i]);
                    CROW_LOG_INFO << "put back "
                                  << history_tasks[i]->getShortName()
                                  << "into taskqueue";
                }
                score_matrix_history_tasks_[k].erase(peer_id);
            }
            // wipe out records in score_matrix_task_peer_id_
            for (int i = 0; i < score_matrix_task_peer_id_[k].size(); i++) {
                for (int j = 0; j < score_matrix_task_peer_id_[k][i].size();
                     j++) {
                    if (score_matrix_task_peer_id_[k][i][j] == peer_id) {
                        score_matrix_task_peer_id_[k][i][j] = "#pending";
                    }
                }
            }
        }
        // trigger resending tasks
        assignTasks();
    } else {
        current_tasks.erase(peer_id);
        for (int k = 0; k < sequence_row_.size(); k++) {
            score_matrix_history_tasks_[k].erase(peer_id);
            for (int i = 0; i < score_matrix_task_peer_id_.size(); i++) {
                for (int j = 0; j < score_matrix_task_peer_id_[i].size(); j++) {
                    if (score_matrix_task_peer_id_[k][i][j] == peer_id) {
                        score_matrix_task_peer_id_[k][i][j] = "";
                    }
                }
            }
        }
    }
}

void BackupMasterService::onScoreMatrixTaskResponse(
    string peer_id, shared_ptr<ScoreMatrixTaskResponse> response) {
    int x = response->x_;
    int y = response->y_;
    int row_id = response->row_id_;

    // record the best score
    if (response->max_score_ > max_score_[row_id]) {
        max_score_[row_id] = response->max_score_;
        max_score_x_[row_id] = x * row_block_size_ + response->max_score_x_;
        max_score_y_[row_id] = y * column_block_size_ + response->max_score_y_;
    }
    // mark current node to be idle, and record current task into the
    // history task
    if (current_tasks.find(peer_id) == current_tasks.end()) {
        CROW_LOG_ERROR << "peer does not exist: " << peer_id;
        return;
    }

    auto current_task =
        dynamic_pointer_cast<ScoreMatrixTask>(current_tasks[peer_id]);
    if (!master_on_) {
        current_tasks[peer_id] = nullptr;
        score_matrix_history_tasks_[row_id][peer_id].push_back(current_task);
    }

    score_matrix_task_blocks_[row_id][x][y] = response;
    score_matrix_task_peer_id_[row_id][current_task->x_][current_task->y_] =
        peer_id;

    if (x == score_matrix_task_blocks_[row_id].size() - 1 &&
        y == score_matrix_task_blocks_[row_id][0].size() - 1) {
        // all the blocks have been calculated
        // starting the first traceback task
        auto new_task = genearteTracebackTask(row_id, max_score_x_[row_id],
                                              max_score_y_[row_id]);
        task_queue_.push_back(new_task);
        if (is_master_ && is_backup_master_online_) {
            CROW_LOG_INFO << "send sync message to backup master";
            shared_ptr<StateSyncObject> ptr = make_shared<StateSyncObject>();
            ptr->task_ = new_task;
            ptr->sync_type_ = "queuePushBack";
            to_back_master_queue_.push_back(ptr);
        }
        CROW_LOG_INFO << "add " << new_task->getShortName() << " into queue";
    } else {
        // update tasks queue
        if (!master_on_) {
            if (checkDependencyForScoreMatrix(row_id, x, y + 1) &&
                score_matrix_task_peer_id_[row_id][x][y + 1] == "") {
                score_matrix_task_peer_id_[row_id][x][y + 1] = "#pending";

                auto new_task = generateScoreMatrixTask(row_id, x, y + 1);
                task_queue_.push_back(new_task);
                CROW_LOG_INFO << "add score matrix task "
                              << new_task->getShortName() << " into queue";
            }

            if (checkDependencyForScoreMatrix(row_id, x + 1, y) &&
                score_matrix_task_peer_id_[row_id][x + 1][y] == "") {
                score_matrix_task_peer_id_[row_id][x + 1][y] = "#pending";

                auto new_task = generateScoreMatrixTask(row_id, x + 1, y);
                task_queue_.push_back(new_task);
                CROW_LOG_INFO << "add score matrix task "
                              << new_task->getShortName() << "into queue";
            }
        }
    }
    // trigger resending tasks
    if (!master_on_) {
        assignTasks();
    }
}

void BackupMasterService::onTracebackTaskResponse(
    string peer_id, shared_ptr<TracebackTaskResponse> response) {
    if (!master_on_) {
        MasterService::onTracebackTaskResponse(peer_id, response);
    }
}