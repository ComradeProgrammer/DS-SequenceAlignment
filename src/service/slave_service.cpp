#include "service/slave_service.h"

#include "crow.h"
using namespace std;
using nlohmann::json;
void SlaveService::onInit() {}

void SlaveService::onNewMessage(std::string peer_id, const std::string& message,
                                bool is_binary) {
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
    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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
    lock_guard<mutex> lock_block(lock_);
    if (peer_id == MASTER_ID) {
        master_online_ = true;
    } else if (peer_id == BACKUP_MASTER_ID) {
        backup_master_online_ = true;
    }
}

void SlaveService::onConnectionTerminated(const std::string peer_id) {
    lock_guard<mutex> lock_block(lock_);
    if (peer_id == MASTER_ID) {
        master_online_ = false;
    } else if (peer_id == BACKUP_MASTER_ID) {
        backup_master_online_ = false;
    }
}

shared_ptr<ScoreMatrixTaskResponse> SlaveService::onScoreMatrixTask(
    std::shared_ptr<ScoreMatrixTask> task) {
    long long t1 = getTimestamp();

    int row_id = task->row_id_;
    auto block = calculateScoreMatrixBlock(task);
    string key = to_string(task->x_) + "_" + to_string(task->y_);
    score_matrix_blocks_[row_id][key] = block;
    long long t2 = getTimestamp();

    // generate the result
    auto response = make_shared<ScoreMatrixTaskResponse>();
    response->x_ = task->x_;
    response->y_ = task->y_;
    response->max_score_ = block->max_score_;
    response->max_score_x_ = block->max_score_x_;
    response->max_score_y_ = block->max_score_y_;
    response->row_id_ = row_id;

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
    total += (t2 - t1);
     cout << total << endl;
    sendResultBack(response);
    return response;
}
shared_ptr<TracebackTaskResponse> SlaveService::onTracebackTask(
    std::shared_ptr<TracebackTask> task) {
    int row_id = task->row_id_;

    string key = to_string(task->x_) + "_" + to_string(task->y_);
    if (score_matrix_blocks_[row_id].find(key) ==
        score_matrix_blocks_[row_id].end()) {
        CROW_LOG_ERROR << "failed to find block " << key;
        return nullptr;
    }
    auto response = traceBackOnBlock(task, score_matrix_blocks_[row_id][key]);
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
    auto& matrix = res->score_matrix_;
    auto& left_column = res->task_->left_column_;
    auto& top_row = res->task_->top_row_;
    int lt_element = res->task_->left_top_element_;
    int max_score = -1;
    int max_score_x, max_score_y;
    matrix = vector<vector<pair<int, int>>>(
        x_size, vector<pair<int, int>>(y_size, {0, 0}));
    for (int i = 0; i < x_size; i++) {
        for (int j = 0; j < y_size; j++) {
            int diag = getScore(i - 1, j - 1, matrix, left_column, top_row,
                                lt_element) +
                       (task->sequence_column_[i] == task->sequence_row_[j]
                            ? task->match_score_
                            : task->mismatch_pentalty_);
            int top =
                getScore(i - 1, j, matrix, left_column, top_row, lt_element) +
                task->gap_open_;
            int left =
                getScore(i, j - 1, matrix, left_column, top_row, lt_element) +
                task->gap_open_;
            int score = max(max(diag, top), max(left, 0));
            matrix[i][j].first = score;
            if (score == diag) {
                matrix[i][j].second = SW_LEFTTOP;
            } else if (score == left) {
                matrix[i][j].second = SW_LEFT;
            } else if (score == top) {
                matrix[i][j].second = SW_TOP;
            } else {
                matrix[i][j].second = SW_HALT;
            }

            if (score >= res->max_score_) {
                max_score = score;
                max_score_x = i;
                max_score_y = j;
            }
        }
    }
    res->max_score_ = max_score;
    res->max_score_x_ = max_score_x;
    res->max_score_y_ = max_score_y;
    return res;
}

shared_ptr<TracebackTaskResponse> SlaveService::traceBackOnBlock(
    shared_ptr<TracebackTask> task, shared_ptr<ScoreMatrixBlock> block) {
    int x = task->start_x_;
    int y = task->start_y_;
    int row_id = task->row_id_;
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
    res->row_id_ = row_id;
    return res;
}

int SlaveService::getScore(int x, int y,
                           const vector<vector<pair<int, int>>>& score_matrix,
                           const vector<int>& left_column,
                           const vector<int>& top_row, int left_top_number) {
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
