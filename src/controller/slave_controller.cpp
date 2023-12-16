#include "controller/slave_controller.h"

#include <atomic>
#include <functional>

#include "crow/logging.h"
#include "model/IdentificationInfo.hpp"
#include "service/slave_service.h"
#include "util/util.h"
using std::make_shared;
typedef websocketpp::client<websocketpp::config::asio_client> client;
void SlaveController::establishConnection() {
    master_client_.set_access_channels(websocketpp::log::alevel::none);
    backup_master_client_.set_access_channels(websocketpp::log::alevel::none);

    if (master_uri_ != "") {
        std::atomic_bool master_ready(false);
        master_client_.init_asio();
        master_client_.set_open_handler(
            [&master_ready](websocketpp::connection_hdl hdl) {
                master_ready = true;
            });
        master_client_.set_fail_handler([](websocketpp::connection_hdl hdl) {
            CROW_LOG_ERROR << "websocket connection with master failed to open";
        });
        master_client_.set_message_handler(
            std::bind(&SlaveController::onMasterMessage, this,
                      std::placeholders::_1, std::placeholders::_2));
        master_client_.set_close_handler(std::bind(
            &SlaveController::onMasterClose, this, std::placeholders::_1));
        websocketpp::lib::error_code ec;
        master_connection_ = master_client_.get_connection(master_uri_, ec);

        master_client_.connect(master_connection_);

        std::thread t([this]() {
            try {
                master_client_.run();
            } catch (const std::exception& e) {
                CROW_LOG_ERROR << "master crashed:" << e.what();
            } catch (websocketpp::lib::error_code e) {
                CROW_LOG_ERROR << "master crashed:" << e.message();
            } catch (...) {
                CROW_LOG_ERROR << "master crashed: "
                               << "other exception";
            }
        });
        t.detach();
        // wait for connection established
        while (!master_ready) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        // send the identification information
        IdentificationInfo identity;
        identity.is_backup_master_ = false;
        identity.peer_id_ = id_;
        std::string s = identity.toJson();
        master_client_.send(master_connection_->get_handle(), s,
                            websocketpp::frame::opcode::text);
        service_->onConnectionEstablished(MASTER_ID);
    }
    if (backup_master_uri_ != "") {
        std::atomic_bool backup_master_ready(false);

        backup_master_client_.init_asio();
        backup_master_client_.set_open_handler(
            [&backup_master_ready](websocketpp::connection_hdl hdl) {
                backup_master_ready = true;
            });
        backup_master_client_.set_fail_handler(
            [](websocketpp::connection_hdl hdl) {
                CROW_LOG_ERROR
                    << "websocket connection with backup_master failed to open";
            });
        backup_master_client_.set_message_handler(
            std::bind(&SlaveController::onBackupMasterMessage, this,
                      std::placeholders::_1, std::placeholders::_2));
        backup_master_client_.set_close_handler(
            std::bind(&SlaveController::onBackupMasterClose, this,
                      std::placeholders::_1));

        websocketpp::lib::error_code ec;
        backup_master_connection_ =
            backup_master_client_.get_connection(backup_master_uri_, ec);
        backup_master_client_.connect(backup_master_connection_);
        std::thread t([this]() {
            try {
                backup_master_client_.run();
            } catch (const std::exception& e) {
                CROW_LOG_ERROR << "backup master crashed" << e.what();
            } catch (websocketpp::lib::error_code e) {
                CROW_LOG_ERROR << "backup master crashed" << e.message();
            } catch (...) {
                CROW_LOG_ERROR << "backup master crashed:"
                               << "other exception";
            }
        });
        t.detach();
        // wait for connection established
        while (!backup_master_ready) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        IdentificationInfo identity;
        identity.is_backup_master_ = false;
        identity.peer_id_ = id_;
        std::string s = identity.toJson();
        backup_master_client_.send(backup_master_connection_->get_handle(), s,
                                   websocketpp::frame::opcode::text);
        service_->onConnectionEstablished(BACKUP_MASTER_ID);
    }
}

void SlaveController::onInit() {
    service_ = make_shared<SlaveService>(this);
    service_->onInit();
}
void SlaveController::onMasterMessage(
    websocketpp::connection_hdl hdl,
    websocketpp::config::asio_client::message_type::ptr msg) {
    std::string payload = msg->get_payload();
    if (msg->get_opcode() == websocketpp::frame::opcode::text) {
        service_->onNewMessage(MASTER_ID, payload, false);
    } else if (msg->get_opcode() == websocketpp::frame::opcode::binary) {
        service_->onNewMessage(MASTER_ID, payload, true);
    }
}

void SlaveController::onBackupMasterMessage(
    websocketpp::connection_hdl hdl,
    websocketpp::config::asio_client::message_type::ptr msg) {
    std::string payload = msg->get_payload();
    if (msg->get_opcode() == websocketpp::frame::opcode::text) {
        service_->onNewMessage(BACKUP_MASTER_ID, payload, false);

    } else if (msg->get_opcode() == websocketpp::frame::opcode::binary) {
        service_->onNewMessage(BACKUP_MASTER_ID, payload, true);
    }
}

void SlaveController::onMasterClose(websocketpp::connection_hdl hdl) {
    service_->onConnectionTerminated(MASTER_ID);
}

void SlaveController::onBackupMasterClose(websocketpp::connection_hdl hdl) {
    service_->onConnectionTerminated(BACKUP_MASTER_ID);
}

void SlaveController::sendMessageToPeer(const std::string& peer_id,
                                        const std::string& message) {
    try {
        if (peer_id == MASTER_ID) {
            master_client_.send(master_connection_->get_handle(), message,
                                websocketpp::frame::opcode::text);
        } else if (peer_id == BACKUP_MASTER_ID) {
            backup_master_client_.send(backup_master_connection_->get_handle(),
                                       message,
                                       websocketpp::frame::opcode::text);
        } else {
            CROW_LOG_ERROR << "invalid peer id " << peer_id;
        }
    } catch (const std::exception& e) {
        CROW_LOG_ERROR << e.what();
    }
}

void SlaveController::sendMessageToPeer(const std::string& peer_id,
                                        const char* data, const uint64_t len) {
    try {
        if (peer_id == MASTER_ID) {
            master_client_.send(master_connection_->get_handle(), data, len,
                                websocketpp::frame::opcode::binary);
        } else if (peer_id == BACKUP_MASTER_ID) {
            backup_master_client_.send(backup_master_connection_->get_handle(),
                                       data, len,
                                       websocketpp::frame::opcode::binary);
        } else {
            CROW_LOG_ERROR << "invalid peer id " << peer_id;
        }
    } catch (const std::exception& e) {
        CROW_LOG_ERROR << e.what();
    }
}
