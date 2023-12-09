#include "service/slave_service.h"

#include "crow.h"
using namespace std;
using nlohmann::json;
void SlaveService::onInit() {}

void SlaveService::onNewMessage(std::string peer_id, const std::string& message,
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
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    if (object_type == "ScoreMatrixTask") {
        auto task = make_shared<ScoreMatrixTask>();
        task->loadFromJsonObject(j);
        onScoreMatrixTask(task);
    } else {
        auto task = make_shared<TracebackTask>();
        task->loadFromJsonObject(j);
        onTracebackTask(task);
    }
}

void SlaveService::onConnectionEstablished(const std::string peer_id) {
    CROW_LOG_INFO << "connection established with " << peer_id;
    lock_guard<mutex> lock_block(lock_);
    if (peer_id == MASTER_ID) {
        master_online_ = true;
    } else if (peer_id == BACKUP_MASTER_ID) {
        backup_master_online_ = true;
    }
}

void SlaveService::onConnectionTerminated(const std::string peer_id) {
    CROW_LOG_INFO << "connection terminated with " << peer_id;

    lock_guard<mutex> lock_block(lock_);
    if (peer_id == MASTER_ID) {
        master_online_ = false;
    } else if (peer_id == BACKUP_MASTER_ID) {
        backup_master_online_ = false;
    }
}

shared_ptr<ScoreMatrixTaskResponse> SlaveService::onScoreMatrixTask(
    std::shared_ptr<ScoreMatrixTask> task) {
    auto block = calculateScoreMatrixBlock(task);
    string key = to_string(task->x_) + "_" + to_string(task->y_);
    score_matrix_blocks_[key] = block;

    // generate the result
    auto response = make_shared<ScoreMatrixTaskResponse>();
    response->x_ = task->x_;
    response->y_ = task->y_;
    response->max_score_ = block->max_score_;
    response->max_score_x_ = block->max_score_x_;
    response->max_score_y_ = block->max_score_y_;

    int x_size = task->left_column_.size();
    int y_size = task->top_row_.size();
    for (int i = 0; i < y_size; i++) {
        response->bottom_row_.push_back(
            block->score_matrix_[x_size - 1][i].first);
    }

    for (int i = 0; i < x_size; i++) {
        response->right_column_.push_back(
            block->score_matrix_[i][y_size - 1].first);
    }

    sendResultBack(response);
    return response;
}
shared_ptr<TracebackTaskResponse> SlaveService::onTracebackTask(
    std::shared_ptr<TracebackTask> task) {
    string key = to_string(task->x_) + "_" + to_string(task->y_);
    if (score_matrix_blocks_.find(key) == score_matrix_blocks_.end()) {
        CROW_LOG_ERROR << "failed to find block " << key;
        return nullptr;
    }
    auto response = traceBackOnBlock(task, score_matrix_blocks_[key]);
    sendResultBack(response);
    return response;
}

void SlaveService::sendResultBack(std::shared_ptr<AbstractJsonObject> obj) {
    std::string message = obj->toJson();
    if (master_online_) {
        thread([message, this]() {
            sendMessageToPeer(MASTER_ID, message);
        }).detach();
    }
    if (backup_master_online_) {
        thread([message, this]() {
            sendMessageToPeer(BACKUP_MASTER_ID, message);
        }).detach();
    }
}

shared_ptr<ScoreMatrixBlock> SlaveService::calculateScoreMatrixBlock(
    shared_ptr<ScoreMatrixTask> task) {
    shared_ptr<ScoreMatrixBlock> res = make_shared<ScoreMatrixBlock>();
    res->task_ = task;
    int x_size = task->sequence_column_.size();
    int y_size = task->sequence_row_.size();
    for (int i = 0; i < x_size; i++) {
        res->score_matrix_.push_back(vector<pair<int, int>>(y_size, {0, 0}));
        for (int j = 0; j < y_size; j++) {
            int diag = getScore(i - 1, j - 1, res) +
                       (task->sequence_column_[i] == task->sequence_row_[j]
                            ? task->match_score_
                            : task->mismatch_pentalty_);
            int top = getScore(i - 1, j, res) + task->gap_open_;
            int left = getScore(i, j - 1, res) + task->gap_open_;
            int score = max(max(diag, top), max(left, 0));
            res->score_matrix_[i][j].first = score;
            if (score == diag) {
                res->score_matrix_[i][j].second = SW_LEFTTOP;
            } else if (score == left) {
                res->score_matrix_[i][j].second = SW_LEFT;
            } else if (score == top) {
                res->score_matrix_[i][j].second = SW_TOP;
            } else {
                res->score_matrix_[i][j].second = SW_HALT;
            }

            if (score >= res->max_score_) {
                res->max_score_ = score;
                res->max_score_x_ = i;
                res->max_score_y_ = j;
            }
        }
    }
    return res;
}

shared_ptr<TracebackTaskResponse> SlaveService::traceBackOnBlock(
    shared_ptr<TracebackTask> task, shared_ptr<ScoreMatrixBlock> block) {
    int x = task->start_x_;
    int y = task->start_y_;
    vector<char> sequence;
    auto res = make_shared<TracebackTaskResponse>();

    while (x >= 0 && y >= 0) {
        switch (block->score_matrix_[x][y].second) {
            case SW_LEFT:
                sequence.push_back('-');
                y--;
                break;
            case SW_TOP:
                sequence.push_back(block->task_->sequence_column_[x]);
                x--;
                break;

            case SW_LEFTTOP:
                sequence.push_back(block->task_->sequence_column_[x]);
                x--;
                y--;
                break;
            case SW_HALT:
                res->halt_ = true;
                goto LABELA;
        }
    }
LABELA:
    reverse(sequence.begin(), sequence.end());
    res->sequence_.insert(res->sequence_.begin(), sequence.begin(),
                          sequence.end());
    res->end_x_ = x;
    res->end_y_ = y;
    res->x_ = task->x_;
    res->y_ = task->y_;
    return res;
}

int SlaveService::getScore(int x, int y, shared_ptr<ScoreMatrixBlock> res) {
    return getScore(x, y, res->score_matrix_, res->task_->left_column_,
                    res->task_->top_row_, res->task_->left_top_element_);
}
int SlaveService::getScore(int x, int y,
                           const vector<vector<pair<int, int>>>& score_matrix,
                           const vector<int> left_column,
                           const vector<int> top_row, int left_top_number) {
    if (x >= 0 && y >= 0) {
        return score_matrix[x][y].first;
    } else if (x == -1 && y == -1) {
        return left_top_number;
    } else if (x == -1) {
        return top_row[y];
    } else {
        return left_column[x];
    }
}
