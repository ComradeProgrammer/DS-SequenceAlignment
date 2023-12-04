#include <ctime>
#include <iostream>
#include <thread>

#include "gtest/gtest.h"
#include "nlohmann/json.hpp"
using namespace std;
// hacks private fields
#define private public
#define protected public
#include "../mock/CustomizedController.hpp"
#include "../mock/NoopController.hpp"
#include "service/master_service.h"
#include "service/slave_service.h"
#include "util/sequential_smith_waterman.h"
using nlohmann::json;
class NoAssignMasterService : public MasterService {
   protected:
    NoAssignMasterService(AbstractController* controller)
        : MasterService(controller) {}
    virtual void assignTasks() override{
        // do nothing
    };
};

class TestSmithWaterman : public testing::Test {
   protected:
    // Sets up the test fixture.
    virtual void SetUp() override {}

    // Tears down the test fixture.
    virtual void TearDown() override {}

    string generateRandomDNA() {
        srand(time(NULL));
        // length between 50-100
        stringstream ss;
        int len = rand() % 50 + 50;
        for (int i = 0; i < len; i++) {
            switch (rand() % 4) {
                case 0:
                    ss << "A";
                    break;
                case 1:
                    ss << "T";
                    break;
                case 2:
                    ss << "C";
                    break;
                case 3:
                    ss << "G";
                    break;
            }
        }
        return ss.str();
    }

    // for test use
    MasterService* master_service_;
    SlaveService* slave_service_;
};

TEST_F(TestSmithWaterman, TestCalculation1) {
    NoopController* controller1 = new NoopController();
    master_service_ = new NoAssignMasterService(controller1);
    NoopController* controller2 = new NoopController();
    slave_service_ = new SlaveService(controller2);

    master_service_->column_block_size_ = 2;
    master_service_->row_block_size_ = 3;
    master_service_->sequence_row_ = "ACACACTA";
    master_service_->sequence_column_ = "AGCACACA";
    master_service_->match_score_ = 2;
    master_service_->mismatch_pentalty_ = -1;
    // master_service_->gap_extra_ = 0;
    master_service_->gap_open_ = -1;

    vector<vector<pair<int, int>>> correct_score_matrix;
    // first run the sequential program
    auto correct_res = sequentialSmithWaterman(
        master_service_->sequence_row_, master_service_->sequence_column_,
        master_service_->match_score_, master_service_->mismatch_pentalty_,
        master_service_->gap_open_, correct_score_matrix);

    master_service_->onConnectionEstablished("slave1");
    master_service_->onInit();
    slave_service_->onInit();
    while (!master_service_->task_queue_.empty()) {
        // check the type of the task
        auto task = master_service_->task_queue_.front();
        master_service_->task_queue_.pop_front();
        if (dynamic_pointer_cast<ScoreMatrixTask>(task) != nullptr) {
            auto score_task = dynamic_pointer_cast<ScoreMatrixTask>(task);
            auto res = slave_service_->onScoreMatrixTask(score_task);
            ASSERT_NE(res, nullptr);
            master_service_->onScoreMatrixTaskResponse("slave1", res);
            // check the matrix score
            string key =
                to_string(score_task->x_) + "_" + to_string(score_task->y_);
            vector<vector<pair<int, int>>>& partial_matrix =
                slave_service_->score_matrix_blocks_[key]->score_matrix_;

            int x = score_task->x_;
            int y = score_task->y_;
            // cout << "checking for block" << x << " " << y << endl;
            int size_x = score_task->left_column_.size();
            int size_y = score_task->top_row_.size();
            ASSERT_EQ(partial_matrix.size(), size_x);
            ASSERT_EQ(partial_matrix[0].size(), size_y);

            for (int i = 0; i < size_x; i++) {
                for (int j = 0; j < size_y; j++) {
                    int old_index_x = master_service_->row_block_size_ * x + i;
                    int old_index_y =
                        master_service_->column_block_size_ * y + j;
                    //  cout << "checking for" << old_index_x << " " <<
                    //  old_index_y
                    //       << endl;

                    ASSERT_EQ(
                        correct_score_matrix[old_index_x][old_index_y].first,
                        partial_matrix[i][j].first);

                    // cout << "\t" << partial_matrix[i][j].first << endl;
                }
            }
            // cout << "max "
            //      << slave_service_->score_matrix_blocks_[key]->max_score_
            //      << endl;
        } else {
            auto traceback_task = dynamic_pointer_cast<TracebackTask>(task);
            auto res = slave_service_->onTracebackTask(traceback_task);
            ASSERT_NE(res, nullptr);
            master_service_->onTracebackTaskResponse("slave1", res);
            // cout <<res->x_<<","<<res->y_<<":"<< res->sequence_ << endl;
            // cout << master_service_->result_ << endl;
        }
    }

    ASSERT_EQ(master_service_->max_score_, correct_res.first);
    ASSERT_EQ(correct_res.second, master_service_->result_);

    delete controller1;
    delete master_service_;
    delete controller2;
    delete slave_service_;
}
TEST_F(TestSmithWaterman, TestCalculation2) {
    NoopController* controller1 = new NoopController();
    master_service_ = new NoAssignMasterService(controller1);
    NoopController* controller2 = new NoopController();
    slave_service_ = new SlaveService(controller2);

    master_service_->column_block_size_ = 3;
    master_service_->row_block_size_ = 7;
    master_service_->sequence_row_ = "TGTTACGG";
    master_service_->sequence_column_ = "GGTTGACTA";
    master_service_->match_score_ = 3;
    master_service_->mismatch_pentalty_ = -3;
    // master_service_->gap_extra_ = 0;
    master_service_->gap_open_ = -2;

    vector<vector<pair<int, int>>> correct_score_matrix;
    // first run the sequential program
    auto correct_res = sequentialSmithWaterman(
        master_service_->sequence_row_, master_service_->sequence_column_,
        master_service_->match_score_, master_service_->mismatch_pentalty_,
        master_service_->gap_open_, correct_score_matrix);

    master_service_->onConnectionEstablished("slave1");
    master_service_->onInit();
    slave_service_->onInit();
    while (!master_service_->task_queue_.empty()) {
        // check the type of the task
        auto task = master_service_->task_queue_.front();
        master_service_->task_queue_.pop_front();
        if (dynamic_pointer_cast<ScoreMatrixTask>(task) != nullptr) {
            auto score_task = dynamic_pointer_cast<ScoreMatrixTask>(task);
            auto res = slave_service_->onScoreMatrixTask(score_task);
            ASSERT_NE(res, nullptr);
            master_service_->onScoreMatrixTaskResponse("slave1", res);
            // check the matrix score
            string key =
                to_string(score_task->x_) + "_" + to_string(score_task->y_);
            vector<vector<pair<int, int>>>& partial_matrix =
                slave_service_->score_matrix_blocks_[key]->score_matrix_;

            int x = score_task->x_;
            int y = score_task->y_;
            // cout << "checking for block" << x << " " << y << endl;
            int size_x = score_task->left_column_.size();
            int size_y = score_task->top_row_.size();
            ASSERT_EQ(partial_matrix.size(), size_x);
            ASSERT_EQ(partial_matrix[0].size(), size_y);

            for (int i = 0; i < size_x; i++) {
                for (int j = 0; j < size_y; j++) {
                    int old_index_x = master_service_->row_block_size_ * x + i;
                    int old_index_y =
                        master_service_->column_block_size_ * y + j;
                    // cout << "checking for" << old_index_x << " " <<
                    // old_index_y
                    //      << endl;

                    ASSERT_EQ(
                        correct_score_matrix[old_index_x][old_index_y].first,
                        partial_matrix[i][j].first);

                    // cout << "\t" << partial_matrix[i][j].first << endl;
                }
            }
            // cout << "max "
            //      << slave_service_->score_matrix_blocks_[key]->max_score_
            //      << endl;
        } else {
            auto traceback_task = dynamic_pointer_cast<TracebackTask>(task);
            auto res = slave_service_->onTracebackTask(traceback_task);
            ASSERT_NE(res, nullptr);
            master_service_->onTracebackTaskResponse("slave1", res);
            // cout <<res->x_<<","<<res->y_<<":"<< res->sequence_ << endl;
            // cout << master_service_->result_ << endl;
        }
    }

    ASSERT_EQ(master_service_->max_score_, correct_res.first);
    ASSERT_EQ(correct_res.second, master_service_->result_);

    delete controller1;
    delete master_service_;
    delete controller2;
    delete slave_service_;
}

TEST_F(TestSmithWaterman, TestCalculationRandom) {
    NoopController* controller1 = new NoopController();
    master_service_ = new NoAssignMasterService(controller1);
    NoopController* controller2 = new NoopController();
    slave_service_ = new SlaveService(controller2);

    master_service_->column_block_size_ = 3;
    master_service_->row_block_size_ = 7;
    master_service_->sequence_row_ = generateRandomDNA();
    master_service_->sequence_column_ = generateRandomDNA();
    master_service_->match_score_ = 3;
    master_service_->mismatch_pentalty_ = -3;
    // master_service_->gap_extra_ = 0;
    master_service_->gap_open_ = -2;

    vector<vector<pair<int, int>>> correct_score_matrix;
    // first run the sequential program
    auto correct_res = sequentialSmithWaterman(
        master_service_->sequence_row_, master_service_->sequence_column_,
        master_service_->match_score_, master_service_->mismatch_pentalty_,
        master_service_->gap_open_, correct_score_matrix);

    master_service_->onConnectionEstablished("slave1");
    master_service_->onInit();
    slave_service_->onInit();
    while (!master_service_->task_queue_.empty()) {
        // check the type of the task
        auto task = master_service_->task_queue_.front();
        master_service_->task_queue_.pop_front();
        if (dynamic_pointer_cast<ScoreMatrixTask>(task) != nullptr) {
            auto score_task = dynamic_pointer_cast<ScoreMatrixTask>(task);
            auto res = slave_service_->onScoreMatrixTask(score_task);
            ASSERT_NE(res, nullptr);
            master_service_->onScoreMatrixTaskResponse("slave1", res);
            // check the matrix score
            string key =
                to_string(score_task->x_) + "_" + to_string(score_task->y_);
            vector<vector<pair<int, int>>>& partial_matrix =
                slave_service_->score_matrix_blocks_[key]->score_matrix_;

            int x = score_task->x_;
            int y = score_task->y_;
            // cout << "checking for block" << x << " " << y << endl;
            int size_x = score_task->left_column_.size();
            int size_y = score_task->top_row_.size();
            ASSERT_EQ(partial_matrix.size(), size_x);
            ASSERT_EQ(partial_matrix[0].size(), size_y);

            for (int i = 0; i < size_x; i++) {
                for (int j = 0; j < size_y; j++) {
                    int old_index_x = master_service_->row_block_size_ * x + i;
                    int old_index_y =
                        master_service_->column_block_size_ * y + j;
                    // cout << "checking for" << old_index_x << " " <<
                    // old_index_y
                    //      << endl;

                    ASSERT_EQ(
                        correct_score_matrix[old_index_x][old_index_y].first,
                        partial_matrix[i][j].first);

                    // cout << "\t" << partial_matrix[i][j].first << endl;
                }
            }
            // cout << "max "
            //      << slave_service_->score_matrix_blocks_[key]->max_score_
            //      << endl;
        } else {
            auto traceback_task = dynamic_pointer_cast<TracebackTask>(task);
            auto res = slave_service_->onTracebackTask(traceback_task);
            ASSERT_NE(res, nullptr);
            master_service_->onTracebackTaskResponse("slave1", res);
            // cout <<res->x_<<","<<res->y_<<":"<< res->sequence_ << endl;
            // cout << master_service_->result_ << endl;
        }
    }

    ASSERT_EQ(master_service_->max_score_, correct_res.first);
    ASSERT_EQ(correct_res.second, master_service_->result_);

    delete controller1;
    delete master_service_;
    delete controller2;
    delete slave_service_;
}
