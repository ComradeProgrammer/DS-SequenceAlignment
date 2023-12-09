#include "controller/backup_master_controller.h"
#include "config/configuration.h"

#include "crow/logging.h"
#include "model/IdentificationInfo.hpp"
#include "service/backup_master_service.h"
#include "util/util.h"
using nlohmann::json;
using std::bind;
using std::lock_guard;
using std::make_shared;
using std::move;
using std::mutex;
using std::string;
void BackupMasterController::onInit(std::shared_ptr<Configuration> config) {
    CROW_WEBSOCKET_ROUTE(app_, "/websocket")
        .onopen(
            bind(&BackupMasterController::onOpen, this, std::placeholders::_1))
        .onclose(
            bind(&BackupMasterController::onClose, this, std::placeholders::_1))
        .onmessage(bind(&BackupMasterController::onMessage, this,
                        std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3));

    auto service = make_shared<BackupMasterService>(this);
    service_ = service;
    service->setConfiguration(config);
    service->onInit();
}
void BackupMasterController::establishConnection() {
    master_client_.set_access_channels(websocketpp::log::alevel::none);
    std::atomic_bool master_ready(false);
    master_client_.init_asio();
    master_client_.set_open_handler(
        [&master_ready](websocketpp::connection_hdl hdl) {
            CROW_LOG_INFO << "websocket connected with master";
            master_ready = true;
        });
    master_client_.set_fail_handler([](websocketpp::connection_hdl hdl) {
        CROW_LOG_ERROR << "websocket connection with master failed to open";
    });
    master_client_.set_message_handler(
        std::bind(&BackupMasterController::onMasterMessage, this,
                  std::placeholders::_1, std::placeholders::_2));
    master_client_.set_close_handler(std::bind(
        &BackupMasterController::onMasterClose, this, std::placeholders::_1));
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
    identity.is_backup_master_ = true;
    identity.peer_id_ = BACKUP_MASTER_ID;
    std::string s = identity.toJson();
    master_client_.send(master_connection_->get_handle(), s,
                        websocketpp::frame::opcode::text);
    service_->onConnectionEstablished(MASTER_ID);
}

void BackupMasterController::run(uint16_t port) {
    app_.port(port).multithreaded().run();
}

void BackupMasterController::onOpen(crow::websocket::connection& conn) {
    CROW_LOG_INFO << "new websocket connection from " << conn.get_remote_ip();
    // do nothing, we need to wait for IdentificationInfo packet
}

void BackupMasterController::onClose(crow::websocket::connection& conn) {
    // check whether this connection has IdentificationInfo attached
    if (conn.userdata() == nullptr) {
        CROW_LOG_INFO << "new websocket connection from "
                      << conn.get_remote_ip();
    } else {
        auto tmp = static_cast<IdentificationInfo*>(conn.userdata());
        conn.userdata(nullptr);
        CROW_LOG_INFO << "new websocket connection from " << tmp->getPeerID()
                      << " " << conn.get_remote_ip();

        lock_.lock();
        id_to_connections.erase(tmp->getPeerID());
        lock_.unlock();
        // then inform this to the service layer
        service_->onConnectionTerminated(tmp->getPeerID());
        delete tmp;
    }
}

void BackupMasterController::onMessage(crow::websocket::connection& conn,
                                       const std::string& data,
                                       bool is_binary) {
    // first we need to check whether it is an
    // IdentificationInfo json object
    if (!is_binary) {
        CROW_LOG_INFO << "new text message " << data;
        json j;
        bool parse_ok = true;
        try {
            j = json::parse(data);
        } catch (const json::exception& e) {
            parse_ok = false;
        }
        if (parse_ok && j.contains("type") &&
            j["type"].template get<string>() == "IdentificationInfo") {
            // if this is an  IdentificationInfo json object
            lock_guard<mutex> block_lock(lock_);
            // store client's info in the connection's user data
            IdentificationInfo* user_info = new IdentificationInfo();
            user_info->loadFromJsonObject(j);
            conn.userdata(user_info);
            // and store the connection into the map
            string user_id = user_info->getPeerID();
            id_to_connections[user_id] = &conn;
            // inform this event to the service layer
            service_->onConnectionEstablished(user_id);
            return;
        }
        // if it is just some other kind of messages
        // try to get its user_id first
        // and forward this message to the service layer

        auto tmp = static_cast<IdentificationInfo*>(conn.userdata());
        if (tmp != nullptr) {
            service_->onNewMessage(tmp->getPeerID(), data, false);
        } else {
            CROW_LOG_WARNING
                << "some connection without identification is sending message";
        }
    } else {
        CROW_LOG_INFO << "new data message "
                      << charArrayToString(data.c_str(), data.size());
        auto tmp = static_cast<IdentificationInfo*>(conn.userdata());
        // forward this message to the service layer
        if (tmp != nullptr) {
            service_->onNewMessage(tmp->getPeerID(), data, true);
        } else {
            CROW_LOG_WARNING
                << "some connection without identification is sending message";
        }
    }
}

void BackupMasterController::onMasterMessage(
    websocketpp::connection_hdl hdl,
    websocketpp::config::asio_client::message_type::ptr msg) {
    std::string payload = msg->get_payload();
    if (msg->get_opcode() == websocketpp::frame::opcode::text) {
        CROW_LOG_INFO << "received text message from master:" << payload;
        service_->onNewMessage(MASTER_ID, payload, false);
    } else if (msg->get_opcode() == websocketpp::frame::opcode::binary) {
        CROW_LOG_INFO << "received binary data message from master:"
                      << charArrayToString(payload.data(), payload.size());
        service_->onNewMessage(MASTER_ID, payload, true);
    }
}

void BackupMasterController::onMasterClose(websocketpp::connection_hdl hdl) {
    CROW_LOG_INFO << "connection closed with master";
    service_->onConnectionTerminated(MASTER_ID);
}

void BackupMasterController::sendMessageToPeer(const std::string& peer_id,
                                               const std::string& message) {
    if (peer_id == MASTER_ID) {
        try {
            master_client_.send(master_connection_->get_handle(), message,
                                websocketpp::frame::opcode::text);
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << e.what();
        }
    } else {
        lock_.lock();
        // check whether this peer_id exists
        if (id_to_connections.find(peer_id) == id_to_connections.end()) {
            CROW_LOG_WARNING << peer_id << " does not exist";
        }
        auto connection = id_to_connections[peer_id];
        lock_.unlock();
        connection->send_text(message);
    }
}

void BackupMasterController::sendMessageToPeer(const std::string& peer_id,
                                               const char* data,
                                               const uint64_t len) {
    if (peer_id == MASTER_ID) {
        try {
            master_client_.send(master_connection_->get_handle(), data, len,
                                websocketpp::frame::opcode::binary);
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << e.what();
        }
    } else {
        lock_.lock();
        // check whether this peer_id exists
        if (id_to_connections.find(peer_id) == id_to_connections.end()) {
            CROW_LOG_WARNING << peer_id << " does not exist";
        }
        auto connection = id_to_connections[peer_id];
        lock_.unlock();
        connection->send_binary(string(data, len));
    }
}
