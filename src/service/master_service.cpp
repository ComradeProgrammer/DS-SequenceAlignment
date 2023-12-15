#include "service/master_service.h"

#include "controller/abstract_controller.h"
#include "crow.h"
#include "service/master_service.h"
#include "util/util.h"
using namespace std;
using nlohmann::json;

MasterService::MasterService(AbstractController* controller)
    : AbstractService(controller) {
    // constructor
}
void MasterService::onInit() {
    // todo: reconsider
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
    // this thread is for syncing messages to the backup master
    if (is_master_ && is_backup_master_online_) {
        thread([this]() {
            while (1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                lock_guard<mutex> lock_block(lock_);
                if (is_backup_master_online_) {
                    while (!to_back_master_queue_.empty()) {
                        auto obj = to_back_master_queue_.front();
                        to_back_master_queue_.pop_front();
                        string message = obj->toJson();
                        CROW_LOG_INFO << "send message to backup_master"
                                      << message;
                        sendMessageToPeer(BACKUP_MASTER_ID, message);
                    }
                }
            }
        }).detach();
    }
    // wait for a period of time
    lock_.lock();
    thread([this]() {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(1000 * init_wait_));
        start_time_ = getTimestamp();
        lock_.unlock();
    }).detach();
}

void MasterService::onNewMessage(std::string peer_id,
                                 const std::string& message, bool is_binary) {
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
    }
}
void MasterService::onScoreMatrixTaskResponse(
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
    current_tasks[peer_id] = nullptr;
    score_matrix_history_tasks_[row_id][peer_id].push_back(current_task);

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
        if (checkDependencyForScoreMatrix(row_id, x, y + 1) &&
            score_matrix_task_peer_id_[row_id][x][y + 1] == "") {
            score_matrix_task_peer_id_[row_id][x][y + 1] = "#pending";
            auto new_task = generateScoreMatrixTask(row_id, x, y + 1);
            task_queue_.push_back(new_task);

            if (is_master_ && is_backup_master_online_) {
                CROW_LOG_INFO << "send sync message to backup master";
                shared_ptr<StateSyncObject> ptr =
                    make_shared<StateSyncObject>();
                ptr->task_ = new_task;
                ptr->sync_type_ = "queuePushBack";
                to_back_master_queue_.push_back(ptr);
            }
            CROW_LOG_INFO << "add score matrix task "
                          << new_task->getShortName() << "into queue";
        }

        if (checkDependencyForScoreMatrix(row_id, x + 1, y) &&
            score_matrix_task_peer_id_[row_id][x + 1][y] == "") {
            score_matrix_task_peer_id_[row_id][x + 1][y] = "#pending";

            auto new_task = generateScoreMatrixTask(row_id, x + 1, y);
            task_queue_.push_back(new_task);
            if (is_master_ && is_backup_master_online_) {
                CROW_LOG_INFO << "send sync message to backup master";
                shared_ptr<StateSyncObject> ptr =
                    make_shared<StateSyncObject>();
                ptr->task_ = new_task;
                ptr->sync_type_ = "queuePushBack";
                to_back_master_queue_.push_back(ptr);
            }
            CROW_LOG_INFO << "add score matrix task "
                          << new_task->getShortName() << "into queue";
        }
    }
    // trigger resending tasks

    assignTasks();
}
void MasterService::onTracebackTaskResponse(
    string peer_id, shared_ptr<TracebackTaskResponse> response) {
    int row_id = response->row_id_;
    result_[row_id] = response->sequence_ + result_[row_id];
    int next_step_x = response->x_ * row_block_size_ + response->end_x_;
    int next_step_y = response->y_ * column_block_size_ + response->end_y_;

    // check whether trace back should stop
    if (response->halt_ || !(next_step_x >= 0 && next_step_y >= 0)) {
        // weh should stop now. we got the result
        end_time_ = getTimestamp();

        cout << "Result got for sequence " << row_id << endl;
        cout << result_[row_id] << endl;
        cout << "Time: " << (end_time_ - start_time_) << endl;
        // clear the task of peer
        current_tasks[peer_id] = nullptr;
        // trigger resending tasks
        assignTasks();
        return;
    }
    // clear the task of peer
    current_tasks[peer_id] = nullptr;

    auto new_task = genearteTracebackTask(row_id, next_step_x, next_step_y);
    if (is_master_ && is_backup_master_online_) {
        CROW_LOG_INFO << "send sync message to backup master";
        shared_ptr<StateSyncObject> ptr = make_shared<StateSyncObject>();
        ptr->task_ = new_task;
        ptr->sync_type_ = "queuePushBack";
        to_back_master_queue_.push_back(ptr);
    }
    task_queue_.push_back(new_task);

    // trigger resending tasks
    assignTasks();
}
void MasterService::onConnectionEstablished(const std::string peer_id) {
    lock_guard<mutex> lock_block(lock_);
    CROW_LOG_INFO << "peer " << peer_id << " connected";
    if (peer_id != BACKUP_MASTER_ID) {
        for (int i = 0; i < sequence_row_.size(); i++) {
            score_matrix_history_tasks_[i][peer_id] = {};
        }
        current_tasks[peer_id] = nullptr;
    } else {
        is_backup_master_online_ = true;
    }
    // trigger resending tasks
    assignTasks();
}

void MasterService::onConnectionTerminated(const std::string peer_id) {
    lock_guard<mutex> lock_block(lock_);
    CROW_LOG_INFO << "peer " << peer_id << " disconnected";
    if (peer_id == BACKUP_MASTER_ID) {
        // if the backup master is down, we just do nothing
        return;
    }
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
                CROW_LOG_INFO << "put back " << history_tasks[i]->getShortName()
                              << "into taskqueue";
            }
            score_matrix_history_tasks_[k].erase(peer_id);
        }
        // wipe out records in score_matrix_task_peer_id_
        for (int i = 0; i < score_matrix_task_peer_id_[k].size(); i++) {
            for (int j = 0; j < score_matrix_task_peer_id_[k][i].size(); j++) {
                if (score_matrix_task_peer_id_[k][i][j] == peer_id) {
                    score_matrix_task_peer_id_[k][i][j] = "#pending";
                }
            }
        }
    }

    // trigger resending tasks
    assignTasks();
}

shared_ptr<ScoreMatrixTask> MasterService::generateScoreMatrixTask(int row_id,
                                                                   int x,
                                                                   int y) {
    shared_ptr<ScoreMatrixTask> task = make_shared<ScoreMatrixTask>();
    task->row_id_ = row_id;
    task->x_ = x;
    task->y_ = y;
    task->match_score_ = match_score_;
    task->mismatch_pentalty_ = mismatch_pentalty_;
    // task->gap_extra_ = gap_extra_;
    task->gap_open_ = gap_open_;
    // get the sequence
    task->sequence_row_ = sequence_row_[row_id].substr(
        y * column_block_size_,
        min(sequence_row_[row_id].size() - y * column_block_size_,
            size_t(column_block_size_)));

    task->sequence_column_ = sequence_column_.substr(
        x * row_block_size_, min(sequence_column_.size() - x * row_block_size_,
                                 size_t(row_block_size_)));
    // get the column on the top
    if (x != 0) {
        task->top_row_ =
            score_matrix_task_blocks_[row_id][x - 1][y]->bottom_row_;
    } else {
        task->top_row_ = vector<int>(task->sequence_row_.size(), 0);
    }
    // get the row on its left
    if (y != 0) {
        task->left_column_ =
            score_matrix_task_blocks_[row_id][x][y - 1]->right_column_;
    } else {
        task->left_column_ = vector<int>(task->sequence_column_.size(), 0);
    }
    // get the number on its left-top
    if (x != 0 && y != 0) {
        task->left_top_element_ = *(
            score_matrix_task_blocks_[row_id][x - 1][y - 1]->bottom_row_.end() -
            1);
    } else {
        task->left_top_element_ = 0;
    }
    return task;
}

bool MasterService::checkDependencyForScoreMatrix(int row_id, int x, int y) {
    if (x >= score_matrix_task_blocks_[row_id].size() ||
        y >= score_matrix_task_blocks_[row_id][0].size()) {
        return false;
    }
    if (x == 0 && y == 0) {
        return true;
    } else if (x == 0) {
        if (score_matrix_task_blocks_[row_id][x][y - 1] != nullptr &&
            score_matrix_task_peer_id_[row_id][x][y - 1] != "") {
            return true;
        }
        return false;
    } else if (y == 0) {
        if (score_matrix_task_blocks_[row_id][x - 1][y] != nullptr &&
            score_matrix_task_peer_id_[row_id][x - 1][y] != "") {
            return true;
        }
        return false;
    } else {
        if (score_matrix_task_blocks_[row_id][x][y - 1] == nullptr ||
            score_matrix_task_blocks_[row_id][x - 1][y] == nullptr ||
            score_matrix_task_peer_id_[row_id][x][y - 1] == "" ||
            score_matrix_task_peer_id_[row_id][x - 1][y] == "") {
            return false;
        }
        if (score_matrix_task_blocks_[row_id][x - 1][y - 1] == nullptr ||
            score_matrix_task_peer_id_[row_id][x - 1][y - 1] == "") {
            return false;
        }
        return true;
    }
}
void MasterService::assignTasks() {
    while (!task_queue_.empty()) {
        auto task = task_queue_.front();
        auto score_task = dynamic_pointer_cast<ScoreMatrixTask>(task);
        string idle_peer = "";

        if (score_task != nullptr) {
            // this is a score  matrix task
            for (auto p : current_tasks) {
                if (p.second == nullptr) {
                    idle_peer = p.first;
                    break;
                }
            }
            if (idle_peer != "") {
                // this is an idle node
                // record that we sent this matrix task to this slave
                score_matrix_task_peer_id_[score_task->row_id_][score_task->x_]
                                          [score_task->y_] = idle_peer;
                task_queue_.pop_front();
                if (is_master_ && is_backup_master_online_) {
                    CROW_LOG_INFO << "send sync message to backup master";
                    shared_ptr<StateSyncObject> ptr =
                        make_shared<StateSyncObject>();
                    ptr->task_ = task;
                    ptr->sync_type_ = "assignTask";
                    ptr->peer_id_ = idle_peer;
                    to_back_master_queue_.push_back(ptr);
                }

                sendTaskToNode(idle_peer, task);
            } else {
                // cannot find an idle peer, return;
                return;
            }
        } else {
            // this is a traceback task, first check  whom it should be
            // assigned
            auto traceback_task = dynamic_pointer_cast<TracebackTask>(task);

            int x = traceback_task->x_;
            int y = traceback_task->y_;
            string peer_id =
                score_matrix_task_peer_id_[traceback_task->row_id_][x][y];
            if (peer_id == "") {
                CROW_LOG_WARNING << "trying to assign trace back task " << x
                                 << "," << y
                                 << " whose score matrix is unknown";

                return;
            }
            if (current_tasks.find(peer_id) == current_tasks.end()) {
                CROW_LOG_WARNING << "trying to assign trace back task " << x
                                 << "," << y << " to a non-existing node "
                                 << peer_id;

                return;
            }
            if (current_tasks[peer_id] == nullptr) {
                task_queue_.pop_front();
                if (is_master_ && is_backup_master_online_) {
                    CROW_LOG_INFO << "send sync message to backup master";
                    shared_ptr<StateSyncObject> ptr =
                        make_shared<StateSyncObject>();
                    ptr->task_ = task;
                    ptr->sync_type_ = "assignTask";
                    ptr->peer_id_ = idle_peer;
                    to_back_master_queue_.push_back(ptr);
                }
                sendTaskToNode(peer_id, task);
            } else {
                // this node is working. cancel the schedule and wait for the
                // next time
                CROW_LOG_WARNING << peer_id
                                 << " is working, schdeuling canceled";
            }

            return;
        }
    }
}
void MasterService::sendTaskToNode(std::string peer_id,
                                   shared_ptr<AbstractTask> task) {
    current_tasks[peer_id] = task;
    CROW_LOG_INFO << "sending  " << task->getShortName() << " to " << peer_id
                  << ": " << task->toJson();
    thread([task, peer_id, this]() {
        string message = task->toJson();
        sendMessageToPeer(peer_id, message);
    }).detach();
}

shared_ptr<TracebackTask> MasterService::genearteTracebackTask(int row_id,
                                                               int prev_pos_x,
                                                               int prev_pos_y) {
    auto task = make_shared<TracebackTask>();
    task->x_ = prev_pos_x / row_block_size_;
    task->y_ = prev_pos_y / column_block_size_;
    task->start_x_ = prev_pos_x % row_block_size_;
    task->start_y_ = prev_pos_y % column_block_size_;
    task->row_id_ = row_id;
    return task;
}
void MasterService::getSequence(string data_type, string data_source,
                                string& out_res) {
    if (data_type == "string") {
        out_res = data_source;
    } else if (data_type == "text") {
        // read the text string
        stringstream ss;
        ifstream f(data_source, ios::in);
        ss << f.rdbuf();
        out_res = ss.str();
        // remove the space
        out_res.erase(0, out_res.find_first_not_of(" \r\n\t\v\f"));
        out_res.erase(out_res.find_last_not_of(" \r\n\t\v\f") + 1);
    } else {
        readFastaFile(data_source, out_res);
    }
}
