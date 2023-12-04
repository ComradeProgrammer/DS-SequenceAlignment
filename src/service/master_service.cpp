#include "service/master_service.h"

#include "controller/abstract_controller.h"
#include "crow.h"
#include "service/master_service.h"
using namespace std;
using nlohmann::json;

MasterService::MasterService(AbstractController* controller)
    : AbstractService(controller) {
    // constructor
}
void MasterService::onInit() {
    // todo: read the parameters  from somewhere and set them
    column_block_size_ = 2;
    row_block_size_ = 3;
    sequence_row_ = "ACACACTA";
    sequence_column_ = "AGCACACA";
    match_score_ = 2;
    mismatch_pentalty_ = -1;
    gap_open_ = -1;

    // calculate how many rows and columns of block and init the tasks_blocks
    int sequence_row_length = sequence_column_.size();
    int sequence_column_length = sequence_row_.size();

    int row_num = (sequence_row_length % row_block_size_) == 0
                      ? (sequence_row_length / row_block_size_)
                      : (sequence_row_length / row_block_size_ + 1);
    int column_num = (sequence_column_length % column_block_size_) == 0
                         ? (sequence_column_length / column_block_size_)
                         : (sequence_column_length / column_block_size_ + 1);

    for (int i = 0; i < row_num; i++) {
        score_matrix_task_blocks_.push_back(
            vector<shared_ptr<ScoreMatrixTaskResponse>>(column_num, nullptr));
        score_matrix_task_peer_id_.push_back(vector<string>(column_num, ""));
    }

    // init the task queue with the first task
    auto task = generateScoreMatrixTask(0, 0);
    task_queue_.push_back(task);
    CROW_LOG_INFO << "add task 0,0 into queue";
    lock_.lock();
    // wait for 10 s
    thread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
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
    // record the best score
    if (response->max_score_ > max_score_) {
        max_score_ = response->max_score_;
        max_score_x_ = x * row_block_size_ + response->max_score_x_;
        max_score_y_ = y * column_block_size_ + response->max_score_y_;
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
    score_matrix_history_tasks_[peer_id].push_back(current_task);

    score_matrix_task_blocks_[x][y] = response;

    if (x == score_matrix_task_blocks_.size() - 1 &&
        y == score_matrix_task_blocks_[0].size() - 1) {
        // all the blocks have been calculated
        // starting the first traceback task
        auto new_task = genearteTracebackTask(max_score_x_, max_score_y_);
        task_queue_.push_back(new_task);
        CROW_LOG_INFO << "add " << new_task->getShortName() << " into queue";
    } else {
        // update tasks queue
        if (checkDependencyForScoreMatrix(x, y + 1)) {
            auto new_task = generateScoreMatrixTask(x, y + 1);
            task_queue_.push_back(new_task);
            CROW_LOG_INFO << "add score matrix task " << x << "," << y + 1
                          << "into queue";
        }
        if (checkDependencyForScoreMatrix(x + 1, y)) {
            auto new_task = generateScoreMatrixTask(x + 1, y);
            task_queue_.push_back(new_task);
            CROW_LOG_INFO << "add score matrix task " << x + 1 << "," << y
                          << "into queue";
        }
    }
    // trigger resending tasks

    assignTasks();
}
void MasterService::onTracebackTaskResponse(
    string peer_id, shared_ptr<TracebackTaskResponse> response) {
    result_ = response->sequence_ + result_;
    int next_step_x = response->x_ * row_block_size_ + response->end_x_;
    int next_step_y = response->y_ * column_block_size_ + response->end_y_;
    // check whether trace back should stop
    if (response->halt_ || !(next_step_x >= 0 && next_step_y >= 0)) {
        // weh should stop now. we got the result
        cout << "Result got" << endl;
        cout << result_ << endl;
        return;
    }
    // clear the task of peer
    current_tasks[peer_id] = nullptr;

    auto new_task = genearteTracebackTask(next_step_x, next_step_y);
    task_queue_.push_back(new_task);

    // trigger resending tasks
    assignTasks();
}
void MasterService::onConnectionEstablished(const std::string peer_id) {
    lock_guard<mutex> lock_block(lock_);
    CROW_LOG_INFO << "peer " << peer_id << "connected";
    if (peer_id != BACKUP_MASTER_ID) {
        score_matrix_history_tasks_[peer_id] = {};
        current_tasks[peer_id] = nullptr;
    }
    // trigger resending tasks
    assignTasks();
}

void MasterService::onConnectionTerminated(const std::string peer_id) {
    // todo: implement
}

shared_ptr<ScoreMatrixTask> MasterService::generateScoreMatrixTask(int x,
                                                                   int y) {
    shared_ptr<ScoreMatrixTask> task = make_shared<ScoreMatrixTask>();
    task->x_ = x;
    task->y_ = y;
    task->match_score_ = match_score_;
    task->mismatch_pentalty_ = mismatch_pentalty_;
    // task->gap_extra_ = gap_extra_;
    task->gap_open_ = gap_open_;
    // get the sequence
    task->sequence_row_ =
        sequence_row_.substr(y * column_block_size_,
                             min(sequence_row_.size() - y * column_block_size_,
                                 size_t(column_block_size_)));

    task->sequence_column_ = sequence_column_.substr(
        x * row_block_size_, min(sequence_column_.size() - x * row_block_size_,
                                 size_t(row_block_size_)));
    // get the column on the top
    if (x != 0) {
        task->top_row_ = score_matrix_task_blocks_[x - 1][y]->bottom_row_;
    } else {
        task->top_row_ = vector<int>(task->sequence_row_.size(), 0);
    }
    // get the row on its left
    if (y != 0) {
        task->left_column_ = score_matrix_task_blocks_[x][y - 1]->right_column_;
    } else {
        task->left_column_ = vector<int>(task->sequence_column_.size(), 0);
    }
    // get the number on its left-top
    if (x != 0 && y != 0) {
        task->left_top_element_ =
            *(score_matrix_task_blocks_[x - 1][y - 1]->bottom_row_.end() - 1);
    } else {
        task->left_top_element_ = 0;
    }
    return task;
}

bool MasterService::checkDependencyForScoreMatrix(int x, int y) {
    if (x >= score_matrix_task_blocks_.size() ||
        y >= score_matrix_task_blocks_[0].size()) {
        return false;
    }
    if (x == 0 && y == 0) {
        return true;
    } else if (x == 0) {
        if (score_matrix_task_blocks_[x][y - 1] != nullptr) {
            return true;
        }
        return false;
    } else if (y == 0) {
        if (score_matrix_task_blocks_[x - 1][y] != nullptr) {
            return true;
        }
        return false;
    } else {
        if (score_matrix_task_blocks_[x][y - 1] == nullptr ||
            score_matrix_task_blocks_[x - 1][y] == nullptr) {
            return false;
        }
        if (score_matrix_task_blocks_[x - 1][y - 1] == nullptr) {
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
                score_matrix_task_peer_id_[score_task->x_][score_task->y_] =
                    idle_peer;
                task_queue_.pop_front();

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
            string peer_id = score_matrix_task_peer_id_[x][y];
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
                // this node is working. cancel the schedule and wait for the
                // next time
                task_queue_.pop_front();
                sendTaskToNode(peer_id, task);
            } else {
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
    CROW_LOG_INFO << "sending  " << task->getShortName() << " to " << peer_id;
    thread([task, peer_id, this]() {
        string message = task->toJson();
        sendMessageToPeer(peer_id, message);
    }).detach();
}

shared_ptr<TracebackTask> MasterService::genearteTracebackTask(int prev_pos_x,
                                                               int prev_pos_y) {
    auto task = make_shared<TracebackTask>();
    task->x_ = prev_pos_x / row_block_size_;
    task->y_ = prev_pos_y / column_block_size_;
    task->start_x_ = prev_pos_x % row_block_size_;
    task->start_y_ = prev_pos_y % column_block_size_;
    return task;
}
