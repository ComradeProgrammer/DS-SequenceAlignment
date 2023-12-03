#include "service/master_service.h"

#include "controller/abstract_controller.h"
#include "crow.h"
#include "nlohmann/json.hpp"
#include "service/master_service.h"
using namespace std;
using nlohmann::json;

MasterService::MasterService(AbstractController* controller)
    : AbstractService(controller) {
    // constructor
}
void MasterService::onInit() {
    lock_guard<mutex> lock_block(lock_);

    // todo: read the parameters  from somewhere and set them

    // calculate how many rows and columns of block and init the tasks_blocks
    int sequence_row_length = sequence_row_.size();
    int sequence_column_length = sequence_column_.size();

    int row_num = (sequence_row_length % row_block_size_) == 0
                      ? (sequence_row_length / row_block_size_)
                      : (sequence_row_length / row_block_size_ + 1);
    int column_num = (sequence_column_length % column_block_size_) == 0
                         ? (sequence_column_length / column_block_size_)
                         : (sequence_column_length / column_block_size_ + 1);

    for (int i = 0; i < row_num; i++) {
        task_blocks_.push_back(
            vector<shared_ptr<ScoreMatrixTaskResponse>>(column_num, nullptr));
    }

    // init the task queue with the first task
    auto task = generateScoreMatrixTask(0, 0);
    task_queue_.push_back(task);
    CROW_LOG_INFO << "add task 0,0 into queue";
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
        int x = response->x_;
        int y = response->y_;
        // mark current node to be idle, and record current task into the
        // history task
        auto current_task = current_tasks[peer_id];
        current_tasks[peer_id] = nullptr;
        history_tasks_[peer_id].push_back(current_task);

        task_blocks_[x][y] = response;
        // update tasks queue
        if (checkDependency(x, y + 1)) {
            auto new_task = generateScoreMatrixTask(x, y + 1);
            task_queue_.push_back(new_task);
            CROW_LOG_INFO << "add task " << x << "," << y + 1 << "into queue";
        }
        if (checkDependency(x + 1, y)) {
            auto new_task = generateScoreMatrixTask(x + 1, y);
            task_queue_.push_back(new_task);
            CROW_LOG_INFO << "add task " << x + 1 << "," << y << "into queue";
        }
        // trigger resending tasks
        assignTasks();
    }
}

void MasterService::onConnectionEstablished(const std::string peer_id) {
    lock_guard<mutex> lock_block(lock_);
    if (peer_id != BACKUP_MASTER_ID) {
        history_tasks_[peer_id] = {};
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
    task->gap_extra_ = gap_extra_;
    task->gap_open_ = gap_open_;
    // get the sequence
    task->sequence_row_ = sequence_row_.substr(
        x * row_block_size_,
        min(sequence_row_.size(), size_t((x + 1) * row_block_size_)));

    task->sequence_column_ = sequence_column_.substr(
        x * column_block_size_,
        min(sequence_column_.size(), size_t((x + 1) * column_block_size_)));
    // get the column on the top
    if (x != 0) {
        task->top_row_ = task_blocks_[x - 1][y]->bottom_row_;
    }
    // get the row on its left
    if (y != 0) {
        task->left_column_ = task_blocks_[x][y - 1]->right_column_;
    }
    // get the number on its left-top
    if (x != 0 && y != 0) {
        task->left_top_element_ =
            *(task_blocks_[x - 1][y - 1]->bottom_row_.end() - 1);
    }
    return task;
}

bool MasterService::checkDependency(int x, int y) {
    if (x >= task_blocks_.size() || y >= task_blocks_[0].size()) {
        return false;
    }
    if (x == 0 && y == 0) {
        return true;
    } else if (x == 0) {
        if (task_blocks_[x][y - 1] != nullptr) {
            return true;
        }
        return false;
    } else if (y == 0) {
        if (task_blocks_[x - 1][y] != nullptr) {
            return true;
        }
        return false;
    } else {
        if (task_blocks_[x][y - 1] == nullptr ||
            task_blocks_[x - 1][y] == nullptr) {
            return false;
        }
        if (task_blocks_[x - 1][y - 1] == nullptr) {
            return false;
        }
        return true;
    }
}
void MasterService::assignTasks() {
    for (auto p : current_tasks) {
        if (task_queue_.empty()) {
            break;
        }
        if (p.second == nullptr) {
            // this is an idle node
            assignTaskToNode(p.first);
        }
    }
}
void MasterService::assignTaskToNode(std::string peer_id) {
    if (!task_queue_.empty()) {
        auto task = task_queue_.front();
        task_queue_.pop_front();
        current_tasks[peer_id] = task;
        CROW_LOG_INFO << "sending task " << task->x_ << "," << task->y_
                      << " to " << peer_id;
        thread([task, peer_id, this]() {
            string message = task->toJson();
            sendMessageToPeer(peer_id, message);
        }).detach();
    }
}