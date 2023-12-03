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
using nlohmann::json;

class TestMasterService : public testing::Test {
   protected:
    // Sets up the test fixture.
    virtual void SetUp() override {}

    // Tears down the test fixture.
    virtual void TearDown() override {}
    // for test use
    AbstractController* controller;
    MasterService* service;
    void initialize1() {
        // inject some test data
        service->column_block_size_ = 2;
        service->row_block_size_ = 2;
        service->sequence_row_ = "ACACACTA";
        service->sequence_column_ = "AGCACACA";
        service->match_score_ = 1;
        service->mismatch_pentalty_ = 2;
        service->gap_extra_ = 3;
        service->gap_open_ = 4;
    }
};

TEST_F(TestMasterService, TestOnInit) {
    controller = new NoopController();
    service = new MasterService(controller);
    // insert some fake results so that as if we have finished some blocks
    initialize1();
    // call the onInit method
    service->onInit();

    // check whether task_blocks were properly initialzied
    ASSERT_EQ(service->task_blocks_.size(), 4);
    ASSERT_EQ(service->task_blocks_[0].size(), 4);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            ASSERT_EQ(service->task_blocks_[i][j], nullptr);
        }
    }
    // there should be task (0,0) in the queue
    ASSERT_EQ(service->task_queue_.size(), 1);
    auto first_task = service->task_queue_.front();
    ASSERT_EQ(first_task->x_, 0);
    ASSERT_EQ(first_task->y_, 0);
    ASSERT_EQ(first_task->gap_extra_, service->gap_extra_);
    ASSERT_EQ(first_task->gap_open_, service->gap_open_);
    ASSERT_EQ(first_task->match_score_, service->match_score_);
    ASSERT_EQ(first_task->gap_extra_, service->gap_extra_);

    // delete the objects
    delete controller;
    delete service;
}

TEST_F(TestMasterService, TestOnConnectionEstablished) {
    auto customized_controller = new CustomizedController();
    atomic<bool> ok = false;
    controller = customized_controller;
    service = new MasterService(controller);
    customized_controller->send_message_to_peer_text_ =
        [&ok, this](string peer_id, string message) {
            // should be "slave1"
            ASSERT_EQ(peer_id, "slave1");
            json j = json::parse(message);
            // check the type
            ASSERT_EQ(j["type"].template get<string>(), "ScoreMatrixTask");
            // deseralize
            ScoreMatrixTask assign;
            assign.loadFromJsonObject(j);

            ASSERT_EQ(assign.x_, 0);
            ASSERT_EQ(assign.y_, 0);
            ASSERT_EQ(assign.gap_extra_, service->gap_extra_);
            ASSERT_EQ(assign.gap_open_, service->gap_open_);
            ASSERT_EQ(assign.match_score_, service->match_score_);
            ASSERT_EQ(assign.gap_extra_, service->gap_extra_);
            ASSERT_EQ(assign.gap_extra_, service->gap_extra_);
            ASSERT_EQ(assign.sequence_column_, "AG");
            ASSERT_EQ(assign.sequence_row_, "AC");
            ok = true;
        };

    // insert some fake results so that as if we have finished some blocks
    initialize1();
    // call the onInit method
    service->onInit();

    // peer_1 connects; it should get the task via message
    service->onConnectionEstablished("slave1");
    while (!ok) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    delete service;
    delete controller;
}

TEST_F(TestMasterService, TestOnNewMessage) {
    auto customized_controller = new CustomizedController();
    atomic<int> counter = 0;
    controller = customized_controller;
    service = new MasterService(controller);
    customized_controller->send_message_to_peer_text_ =
        [&counter, this](string peer_id, string message) { counter++; };

    // insert some fake results so that as if we have finished some blocks
    initialize1();
    // call the onInit method
    service->onInit();
    // peer_1 connects; it should get the task via message
    service->onConnectionEstablished("slave1");
    while (counter != 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // create a fake message so it looks like that(0,0) is finished calculation
    auto resp = make_shared<ScoreMatrixTaskResponse>();
    resp->bottom_row_ = {1, 2};
    resp->right_column_ = {3, 4};
    resp->x_ = 0;
    resp->y_ = 0;
    resp->max_score_ = 100;
    resp->max_score_x_ = 1;
    resp->max_score_y_ = 0;
    string json_message = resp->toJson();

    service->onNewMessage("slave1", json_message, false);
    // slave1 should get task （0，1）
    while (counter != 2) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    ASSERT_EQ(service->current_tasks["slave1"]->x_, 0);
    ASSERT_EQ(service->current_tasks["slave1"]->y_, 1);
    // task(0,0) should be in history task of slave1
    ASSERT_EQ(service->history_tasks_["slave1"].size(), 1);
    ASSERT_EQ(service->history_tasks_["slave1"][0]->x_, 0);
    ASSERT_EQ(service->history_tasks_["slave1"][0]->y_, 0);

    // task(1,0) should be in the task queue
    ASSERT_EQ(service->task_queue_.size(), 1);
    ASSERT_EQ(service->task_queue_[0]->x_, 1);
    ASSERT_EQ(service->task_queue_[0]->y_, 0);
}