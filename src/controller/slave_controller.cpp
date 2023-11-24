#include "controller/slave_controller.h"

#include <atomic>
#include <functional>

#include "crow/logging.h"
#include "util/util.h"
typedef websocketpp::client<websocketpp::config::asio_client> client;
void SlaveController::establishConnection() {
    master_client_.set_access_channels(websocketpp::log::alevel::none);
    backup_master_client_.set_access_channels(websocketpp::log::alevel::none);

    if (master_uri_ != "") {
        master_client_.init_asio();
        std::atomic_bool master_ready(false);
        master_client_.set_open_handler(
            [&master_ready](websocketpp::connection_hdl hdl) {
                CROW_LOG_INFO << "websocket connected with master";
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
        // wait for connection established
        // while (!master_ready) {
        //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // }
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
    }

    if (backup_master_uri_ != "") {
        backup_master_client_.init_asio();
        backup_master_client_.set_open_handler(
            [](websocketpp::connection_hdl hdl) {
                CROW_LOG_INFO << "websocket connected with backup_master";
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
    }
}

void SlaveController::onMasterMessage(
    websocketpp::connection_hdl hdl,
    websocketpp::config::asio_client::message_type::ptr msg) {
    if (msg->get_opcode() == websocketpp::frame::opcode::text) {
        CROW_LOG_INFO << "received text message from master:"
                      << msg->get_payload();
    } else if (msg->get_opcode() == websocketpp::frame::opcode::binary) {
        std::string payload = msg->get_payload();
        CROW_LOG_INFO << "received binary data message from master:"
                      << charArrayToString(payload.data(), payload.size());
    }
}

void SlaveController::onBackupMasterMessage(
    websocketpp::connection_hdl hdl,
    websocketpp::config::asio_client::message_type::ptr msg) {
    if (msg->get_opcode() == websocketpp::frame::opcode::text) {
        CROW_LOG_INFO << "received text message from backup master:"
                      << msg->get_payload();
    } else if (msg->get_opcode() == websocketpp::frame::opcode::binary) {
        std::string payload = msg->get_payload();
        CROW_LOG_INFO << "received binary data message from backup master:"
                      << charArrayToString(payload.data(), payload.size());
    }
}

void SlaveController::onMasterClose(websocketpp::connection_hdl hdl) {
    CROW_LOG_INFO << "connection closed with master";
}

void SlaveController::onBackupMasterClose(websocketpp::connection_hdl hdl) {
    CROW_LOG_INFO << "connection closed with backup master";
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
